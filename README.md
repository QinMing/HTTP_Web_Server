This is a simple HTTP server.

---------------------

Some of its behaviors to be clarify:

* Response 400 error if the URI is not started with '/'
* Will not recognize a directory if it's not ended with '/'. E.g. "GET /page1" is different from "GET /page1/"