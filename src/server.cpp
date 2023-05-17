#include <thread>
#include <mutex>
#include <filesystem>
// my headers
//#undef _DEBUG
#include "HTTP_conn.hpp"
#include "HTTP_message.hpp"
#include "DB_conn.hpp"
#include "utils.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

const char* server_init_file = "server_options.ini";

// Http Server
std::string HTTP_Basedir = "/";
std::string HTTP_IP = "127.0.0.1";
std::string HTTP_Port = "80";
// Database Server
sql::SQLString DB_Port = "3306";
sql::SQLString DB_Host = "localhost";
sql::SQLString DB_Username = "root";
sql::SQLString DB_Password = "";
sql::SQLString DB_Name = "mb_db";

Database_connection conn;

// for controlling debug prints
std::mutex mtx;

void readIni();
void resolveRequest(SOCKET clientSocket, HTTP_conn* http_);
void Head(HTTP_message& inbound, HTTP_message& outbound);
void Get(HTTP_message& inbound, HTTP_message& outbound);
bool manageApi(HTTP_message& inbound, HTTP_message& result);
void composeHeader(const char* filename, std::map<std::string, std::string>& result);
std::string getFile(const char* file);

// create a json string from data got from the given query
std::string getJSONProduct(sql::SQLString& query, bool full_data);
std::string getJSONSupplier(sql::SQLString& query);
std::string getJSONTag(sql::SQLString& query);

// columns name grouped for the json formatter
sql::SQLString prod_cols[] = {"name", "options", "tag", "ID_product", "price", "quantity"};
sql::SQLString supp_cols[] = {"company_name", "email", "website", "ID_supplier", "place"};
sql::SQLString tags_cols[] = {"name"};
sql::SQLString plac_cols[] = {"country", "region", "city", "street", "number"};


// DB - interfacing function
void getContentType(const std::string* filetype, std::string& result);

int __cdecl main() {

	readIni();
	// initialize winsock and the server options
	HTTP_conn http(HTTP_Basedir.c_str(), HTTP_IP.c_str(), HTTP_Port.c_str());

	// create connection
	sql::SQLString l_host("tcp://" + DB_Host + ":" + DB_Port);

	conn.connect(l_host, DB_Username, DB_Password, DB_Name);

	// used for controlling 
	int iResult;
	SOCKET client;
	std::string request;

	// Receive until the peer shuts down the connection
	while (true) {

		client = http.acceptClientSock();
		if (client == INVALID_SOCKET) {
			continue;
		} else {
			//resolveRequest(client, &http);
			std::thread(resolveRequest, client, &http).detach();
		}

	}

	if (iResult == SOCKET_ERROR) {
		std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
		http.closeClientSock(&client);
		WSACleanup();
		return 1;
	}

	// cleanup
	http.closeClientSock(&client);
	WSACleanup();

	return 0;
}

void resolveRequest(SOCKET clientSocket, HTTP_conn* http_) {

	int iResult;
	int iSendResult;

	std::string request;
	HTTP_message response;

	while (true) {

		// ---------------------------------------------------------------------- RECEIVE
		iResult = http_->receiveRequest(&clientSocket, request);

		// received some bytes
		if (iResult > 0) {

			HTTP_message mex(request);

			// no really used 
			switch (mex.method) {
			case HTTP_HEAD:
				Head(mex, response);
				break;

			case HTTP_GET:
				Get(mex, response);
				break;

			}

			// make the message a single formatted string
			response.compileMessage();

			// ------------------------------------------------------------------ SEND
			// acknowledge the segment back to the sender
			iSendResult = http_->sendResponse(&clientSocket, &response.message);

			// send failed, close socket and close program
			if (iSendResult == SOCKET_ERROR) {
				std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
				http_->closeClientSock(&clientSocket);
				WSACleanup();
				break;
			}

			http_->shutDown(&clientSocket);
			break;
		}

		// received an error
		if (iResult < 0) {
			iResult = 0;
			std::cout << "Error! Cannot keep on listening" << WSAGetLastError();

			http_->shutDown(&clientSocket);
			break;
		}

		// actually impossible cus recv block the thread for any communication
		// nothing received, depend on the request
		if (iResult == 0) {
			break;
		}
	}

}

/**
* retrive initilization data from the .ini file
*/
void readIni() {

	std::string buf;
	std::fstream Read(server_init_file, std::ios::in);;
	std::vector<std::string> key_val;

	// read props from the ini file and sets the important variables
	if (Read.is_open()) {
		while (std::getline(Read, buf)) {
			key_val = split(buf, "=");

			if (key_val.size() > 1) {
				// sectioon [HTTP]
				if (key_val[0] == "IP") {
					HTTP_IP = key_val[1];
				} else if (key_val[0] == "HTTP_Port") {
					HTTP_Port = key_val[1];
				} else if (key_val[0] == "Base_Directory") {
					HTTP_Basedir = key_val[1];
				}
				// section [Database]
				else if (key_val[0] == "DB_Port") {
					DB_Port = key_val[1];
				} else if (key_val[0] == "host") {
					DB_Host = key_val[1];
				} else if (key_val[0] == "password") {
					DB_Password = key_val[1];
				} else if (key_val[0] == "username") {
					DB_Username = key_val[1];
				} else if (key_val[0] == "Database_Name") {
					DB_Name = key_val[1];
				}
			}
		}

		Read.close();
	} else {
		std::cout << "server_options.ini file not found!\n Proceeding with default values!" << std::endl;
	}
}

/**
* the http method, set the value of result as the header that would have been sent in a GET response
*/
void Head(HTTP_message& inbound, HTTP_message& outbound) {

	char* dst = (char*) (inbound.filename.c_str());
	urlDecode(dst, inbound.filename.c_str());

	// re set the filename as the base directory and the decoded filename
	std::string file = HTTP_Basedir + dst;

	// usually to request index.html browsers does not specify it, they usually use /, if thats the case I scpecify index.html
	// back access the last char of the string
	if (file.back() == '/') {
		file += "index.html";
	}

	// insert in the outbound message the necessaire header options, filename is used to determine the response code
	composeHeader(file.c_str(), outbound.headerOptions);

	// i know that i'm loading an entire file, if i find a better solution i'll use it
	std::string content = getFile(file.c_str());
	outbound.headerOptions["Content-Lenght"] = std::to_string(content.length());
	outbound.filename = file;
}

/**
* the http method, get both the header and the body
*/
void Get(HTTP_message& inbound, HTTP_message& outbound) {

	// I just need to add the body to the head, 
	Head(inbound, outbound);

	// i know that i'm loading an entire file, if i find a better solution i'll use it
	// 
	if (!manageApi(inbound, outbound)) {
		outbound.rawBody = getFile(outbound.filename.c_str());
	}

	std::string compressed;
	compressGz(compressed, outbound.rawBody.c_str(), outbound.rawBody.length());
	// set the content of the message
	outbound.rawBody = compressed;

	outbound.headerOptions["Content-Lenght"] = std::to_string(compressed.length());
	outbound.headerOptions["Content-Encoding"] = "gzip";
}

/**
* the API logic is here
*/
bool manageApi(HTTP_message& inbound, HTTP_message& outbound) {


	int type = 0;
	bool full_data = false;

	sql::SQLString query;

	if (inbound.filename == "/products" || inbound.filename == "/products/") {
		type = 1;
		query = "SELECT * FROM products JOIN product_supplier USING(ID_product) JOIN suppliers USING(ID_supplier) JOIN places USING(ID_place) WHERE 1=1";

	} else if (inbound.filename == "/suppliers" || inbound.filename == "/suppliers/") {
		type = 2;
		query = "SELECT * FROM suppliers JOIN places USING(ID_place) WHERE 1=1";

	} else if (inbound.filename == "/tags" || inbound.filename == "/tags/") {
		type = 3;
		query = "SELECT * FROM tags WHERE 1=1";

	} else {
		return false;
	}

	std::cout << "API endpoint = " << inbound.filename << std::endl;

	// create the query
	for (auto const& [key, value] : inbound.parameters) {

		switch (type) {

		case 1:
			if (key == "ID" and value != "-1") {
				query += " and ID_product=" + value;
				break; // if I have the id i don't need to search for anything else
			}

			if (key == "tag" && !value.empty()) {
				query += " and tag='" + value + "'";
			}

			if (key == "search") {
				query += " and (products.name LIKE '%" + value + "%' or products.options LIKE '%" + value + "%')";
			}

			if (key == "long" && value == "true") {
				full_data = true;
			}

			if (key == "minPrice") {
				query += " and price > " + value;
			}

			if (key == "maxPrice") {
				query += " and price < " + value;
			}
			break;

		case 2:
			if (key == "ID" and value != "-1") {
				query += " and ID_supplier=" + value;
				break; // if I have the id i don't need to search for anything else
			}

			if (key == "search") {
				query += " and company_name LIKE '%" + value + "%' ";
			}
			break;

		case 3:

			if (key == "search") {
				query += " and name LIKE '%" + value + "%' ";
			}
			break;
		}
	}

	if (inbound.parameters["results"] >= "0") {
		query += " LIMIT " + inbound.parameters["results"];
	} else {
		query += " LIMIT 1";
	}

	std::cout << query << std::endl;

	std::string result;
	switch (type) {

	case 1:
		result = getJSONProduct(query, full_data);
		break;
	case 2:
		result = getJSONSupplier(query);
		break;
	case 3:
		result = getJSONTag(query);
		break;
	}

	outbound.rawBody = result;
	outbound.headerOptions["HTTP/1.1"] = "200 OK";
	outbound.headerOptions["Content-Type"] = "application/json";
	std::cout << result << std::endl;

	return true;

}

/**
* compose the header given the file requested
*/
void composeHeader(const char* filename, std::map<std::string, std::string>& result) {

	// I use map to easily manage key : value, the only problem is when i compile the header, the response code must be at the top

	// if the requested file actually exist
	if (std::filesystem::exists(filename)) {

		// status code OK
		result["HTTP/1.1"] = "200 OK";

		// get the file extension, i'll use it to get the content type
		std::string temp = split(filename, ".").back(); // + ~24 alloc

		// get the content type
		std::string content_type = "";
		getContentType(&temp, content_type);

		// fallback if finds nothing
		if (content_type == "") {
			content_type = "text/plain";
		}

		// and actually add it in
		result["Content-Type"] = content_type;

	} else {
		// status code Not Found
		result["HTTP/1.1"] = "404 Not Found";
		result["Content-Type"] = "text/html";
	}

	// various header options

	result["Date"] = getUTC();
	result["Connection"] = "close";
	result["Vary"] = "Accept-Encoding";
	result["Server"] = "LeonardoCustom/3.0 (Win64)";
}

/**
* get the file to a string and if its empty return the page 404.html
* https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c maybe better
*/
std::string getFile(const char* file) {

	// get the required file
	std::fstream ifs(file, std::ios::binary | std::ios::in);

	// read the file in one go to a string
	//							start iterator							end iterator
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));


	// if the file does not exist i load a default 404.html 
	if (content.empty()) {
		std::fstream ifs("404.html", std::ios::binary | std::ios::in);
		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	}

	ifs.close();

	return content;
}

/**
* Query the database for the correct file type
*/
void getContentType(const std::string* filetype, std::string& result) {

	// build query 1, don't accept anything but the exact match

	sql::SQLString query = "SELECT * FROM types WHERE type LIKE '" + *filetype + "'";

	// execute query

	sql::ResultSet* res;
	mtx.lock();
	res = conn.Query(&query);
	mtx.unlock();

	// i only get the first result
	if (res == nullptr) {
		result = "text/plain";
		return;
	}

	if (res->next()) {
		result = res->getString("content");
	} else {
		// build query 2, anything before the type is accepted
		query = "SELECT * FROM types WHERE type LIKE '%" + *filetype + "'";

		// execute query
		mtx.lock();
		res = conn.Query(&query);
		mtx.unlock();

		// check first result
		if (res->next()) {
			result = res->getString("content");
		} else {
			// default type
			result = "text/plain";
		}
	}

	delete res;
}

/**
* Format the query result with or without the full supplier data
*/
std::string getJSONProduct(sql::SQLString& query, bool full_data) {

	json root;

	json products = json::array();

	json single_product = json::object();
	json supplier = json::object();
	json place = json::object();

	sql::ResultSet* res;
	mtx.lock();
	res = conn.Query(&query);
	mtx.unlock();

	using DTypes = sql::DataType;

	while (res != nullptr && res->next()) {

		single_product[prod_cols[0]] = res->getString(prod_cols[0]);
		single_product[prod_cols[1]] = res->getString(prod_cols[1]);
		single_product[prod_cols[2]] = res->getString(prod_cols[2]);

		single_product[prod_cols[3]] = res->getInt(prod_cols[3]);
		single_product[prod_cols[4]] = res->getDouble(prod_cols[4]);
		single_product[prod_cols[5]] = res->getInt(prod_cols[5]);

		if (full_data) {
			supplier[supp_cols[0]] = res->getString(supp_cols[0]);
			supplier[supp_cols[1]] = res->getString(supp_cols[1]);
			supplier[supp_cols[2]] = res->getString(supp_cols[2]);
			supplier[supp_cols[3]] = res->getInt(supp_cols[3]);

			place[plac_cols[0]] = res->getString(plac_cols[0]);
			place[plac_cols[1]] = res->getString(plac_cols[1]);
			place[plac_cols[2]] = res->getString(plac_cols[2]);
			place[plac_cols[3]] = res->getInt(plac_cols[3]);
			place[plac_cols[4]] = res->getInt(plac_cols[4]);

			supplier["place"] = place;
			single_product["supplier"] = supplier;


		} else {
			single_product["supplier"] = res->getInt(supp_cols[3]);
		}

		products.push_back(single_product);
	}

	root["products"] = products;

	return root.dump(4);

}

std::string getJSONTag(sql::SQLString& query) {

	json root;

	json tags = json::array();


	sql::ResultSet* res;
	mtx.lock();
	res = conn.Query(&query);
	mtx.unlock();

	using DTypes = sql::DataType;

	while (res != nullptr && res->next()) {
		tags.push_back(res->getString(tags_cols[0]));
	}

	root["tags"] = tags;

	std::cout << std::setw(4) << root << std::endl;
	return root.dump(4);

}

std::string getJSONSupplier(sql::SQLString& query) {

	json root;

	json suppliers = json::array();

	json single_supplier = json::object();
	json place = json::object();

	sql::ResultSet* res;
	mtx.lock();
	res = conn.Query(&query);
	mtx.unlock();

	using DTypes = sql::DataType;

	while (res != nullptr && res->next()) {

		single_supplier[supp_cols[0]] = res->getString(supp_cols[0]);
		single_supplier[supp_cols[1]] = res->getString(supp_cols[1]);
		single_supplier[supp_cols[2]] = res->getString(supp_cols[2]);
		single_supplier[supp_cols[3]] = res->getInt(supp_cols[3]);

		place[plac_cols[0]] = res->getString(plac_cols[0]);
		place[plac_cols[1]] = res->getString(plac_cols[1]);
		place[plac_cols[2]] = res->getString(plac_cols[2]);
		place[plac_cols[3]] = res->getInt(plac_cols[3]);
		place[plac_cols[4]] = res->getInt(plac_cols[4]);

		single_supplier["place"] = place;
		suppliers.push_back(single_supplier);
	}

	root["suppliers"] = suppliers;

	std::cout << std::setw(4) << root << std::endl;
	return root.dump(4);

}
