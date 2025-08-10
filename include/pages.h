#pragma once

#include "StringRef.h"

static StringRef Not_Found_Page = TO_STRINGREF("<html><head><title>SNS 404 Page Not Found</title><style>body {font-family: monospace;}h1,h2 {padding: 10px;}</style></head><body><h1>Error 404 <br />The page you are searching for does not exist</h1><hr /><h2>You can go back to the website root via <a href='/'>this</a> link.</h2></body></html>");

// the directories must be added between these two parts
static StringRef Dir_View_Page_Pre  = TO_STRINGREF("<html><head><meta http-equiv='content-type' content='text/html; charset=windows-1252' /><title>Index of /repos/</title></head><body><h1>Index of /repos/</h1><hr /><pre><table>");
static StringRef Dir_View_Page_Post = TO_STRINGREF("</table></pre><hr /></body></html>");

static StringRef Dir_View_Page_Item = TO_STRINGREF("	<tr><td><a href='https://URL'>FILENAME</a></th><th>TIMESTAMP</td></tr>");
