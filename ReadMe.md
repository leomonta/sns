# simple Http server from socket library

## this was previuosly my graduation project -> [Tesi_matura_2021](https://github.com/leomonta/Tesi_Matura_2021)

### Here are some features

* Head http method
* Get http method
* wait for clients to make a connection, give the client socket to a thread for its resolution
* Gz compression for data sent
* basic header options implemented
	* HTTP/1.1 -> 200 | 404 
	* Content-Lenght -> variable
	* Content-Encoding -> gzip
	* Cache-Control -> max-age=604800
	* Content-Type -> appropriate MIME type
	* Date -> UTC
	* Connection -> Close
	* Vary -> Accept-Encoding
	* Server -> LeonardCustom/3.2 (Ubuntu64)
* Mime type retrived from hash map
* Query parameters parsing
* Form data parsing
* Abstraction of an http message