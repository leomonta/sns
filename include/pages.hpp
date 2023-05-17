#pragma once

const char *Not_Found_Page = "<html><head><title>SNS 404 Page Not Found</title><style>body {font-family: monospace;}h1,h2 {padding: 10px;}</style></head><body><h1>Error 404 <br />The page you are searching for does not exist</h1><hr /><h2>You can go back to the website root via <a href='/'>this</a> link.</h2></body></html>";

// the directories must be added between these two parts
const char *Dir_View_Page_Pre  = "<html><head><meta http-equiv='content-type' content='text/html; charset=windows-1252' /><title>Index of /repos/</title></head><body><h1>Index of /repos/</h1><hr /><pre><table>";
const char *Dir_View_Page_Post = "</table></pre><hr /></body></html>";

const char *Dir_View_Page_Item = "	<tr><td><a href='https://URL'>FILENAME</a></th><th>TIMESTAMP</td></tr>";