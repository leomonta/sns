# Project school for graduation

## I'm developing this project for my graduation on june 2021. I'll discuss this work on my exam
#### __I hope it will be the best part of the exam__


### The idea is this:

* I have a database containing different products from different suppliers, each supplier supply a specific product with a specific price,
each product and supplier are invented and have randomly generated data.

* I permit the use of the data in my database with an API available from my http server, the API support different values for searching products:
	* Name / part of the name of the product
	* Name / part of the name of the supplier
	* Maximum and Minimum price for the products
	* Category / tag of the product
	* part of the options of a product

* API entries: _host_/
	* products/?
		* ID=_number_		the exact numeric value of the product id, it returns a single product
		* tag=_string_		the exact tag string of the searched product, there can be only be on tag
		* search=_string_	a string containing part of the desired product name and options
		* results=_number_	number of maximum results that will be returned if nothing provided return only one result
		* supplier=_string_	a string containing part of the supplier name
		* maxPrice=_number_	maximun acceptable price of the product
		* minPrice=_number_	minimum acceptable price of the product
		* long=_bool_		true if with the product is needed all the info of the supplier, false if is acceptable just the supplier ID, default is false 
	* suppliers/?
		* ID=_number_		the exact numeric value of the  id, it returns a single supplier
		* results=_number_	number of maximum results that will be returned
		* search=_string_	a string containing part of the supplier name
	* tags/?				if nothing passed return all tags
		* search=_string_	a string containing part of the tag name
		* results=_number_	number of maximum results that will be returned

> I also have a separate database for the mime types
