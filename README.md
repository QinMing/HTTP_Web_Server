This is a simple HTTP server written in C. It can serve static contents in the file system.

---------------------

Some of its behaviors to clarify:

* Reply with 400 error if the URI is not started with '/'
* Will not recognize a directory if it's not ended with '/'. E.g. "GET /page1" is different from "GET /page1/"
