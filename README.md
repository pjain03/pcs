# COMP112: Networks, Final Project
1) Overview: 

For our COMP112 Networks class final project, we implemented a HTTP high performance proxy with the goal of increasing performance when the user searches for the same content, as well as providing an interface for a user to query the proxy cache via related keywords. The proxy can handle multiple clients concurrently, and supports GET, CONNECT, OPTIONS methods, though only the GET method responses will be stored in the cache. In other words, only HTTP websites will be cached, and not HTTPS. If a HTTPS request was sent to our proxy, the proxy will make a new connection to the server and retrieve the data and pass it back to the client, in a cuthrough technique. The implemented search engine has a search bar to search the proxy's cache by keyword(s). The search engine will return a list of URLs that have data with the same keyword(s) as the query. The user can then click on a URL they wish to see, and the cached data will be sent to the web interface as a preview of the actual webpage. 


Github: https://github.com/pjain03/comp112_final_project/tree/master

2) Design: Details of the key modules, their interfaces, how they interact with each other, etc. 

    - proxy.c: Holds the main code to run the proxy in a select loop

    - ap_utilities.h: Contains functions and struct definitions relating to parsing HTTP requests and responses, adding clients to a list to keep track of concurrent clients. Note that both a connection from a user to our proxy and a connection our proxy makes to a server are counted as a client connection.

    - cache.h: Contains the functions and hash table definition relating to the cache. Different cache eviction policies are also implemented and can be specified on the command line.

    _ search_engine.h: Contains the functions and struct definitions for the backend of the search engine. This involves extracting keywords from the response bodies before they are cached, as well as calculating relevant search results to return the most relevant set of data available in the cache.

    - webpage: This folder contains the HTML, JS, and CSS files necessary to run the search engine webpage. The webpage needs to be hosted on a separate server from the proxy. This is not a problem because CORS has already been enabled.  

    - testing scripts:
        - test: This file contains a python script that tests the functional correctness of the proxy. This means comparing the results returned from our proxy with results returned directly from the server, and making sure there is no difference in results returned.
        - benchmark.py: This file holds the python script that runs latency and scalability benchmarks for the proxy. The results are printed out to standard output. The latency tests calculate the average latency of small cached transfers, average latency of large uncached transfers, and throughput of each. The scalability test makes multiple connections to the proxy until the proxy can no longer handle the more connections. Results for these are described below.


3) Evaluation: Performance (e.g., latency of small transfers, throughput of large transfers); scalability (e.g., how many requests per second, number of concurrent clients, etc), any other experimental evaluation that makes sense for your project. 






4) Reflections: Each group member should write about their contribution to the project, what new things they learnt, what were the main challenges they faced, etc. 
Annie: My main contributions to this project were implementing the cache, working with Pulkit together on connecting multiple clients concurrently, implementing the backend logic of the search engine, and contributing to the scripting for testing and benchmarking. I learned to think more deeply on how to build a large program in terms of using appropriate data structures and organizing the code into separate modules. As well, when dealing with multiple clients concurrently, we really had to think about how to keep track of all the sockets opened, buffering the read bytes appropriately, and sending the data back to the right clients. As well, a major technical challenge was getting the search engine to return the relevant result(s) based on a keyword querying technique. This meant that there was a lot of thought and research gone into determining keywords for a data response, and sorting the result(s) based on relevance. Especially if two or more data responses have the queried keyword, then the question of how to determine which data response is more relevant came up. As well, another tricky case was when there are multiple keywords entered in the query, determining which response would be the most relevant for all the keywords. 


Pulkit:







## Compilation
Run the following commands:
1. `chmod u+x ./scripts/*`
2. `./scripts/compile`

## Usage
1. Run the proxy using:
    * `./scripts/exe_proxy <host name> <port number> <OPTIONAL: eviction policy>`
    Eviction policies to choose from: `lru`, `mru`, `random`
    If no eviction policy was provided, `lru` is the default
    * `./scripts/proxy <port number> <OPTIONAL: eviction policy>`
    We set the host name to our default in this script. It allows us to run the proxy easily on the same machine several times
2. Test the proxy using our test script. Edit the `PROXY` and `RESRC` variables defined in `./scripts/test` as indicated to test a different machine or resource respectively:
    * `./scripts/test <port number>`
    * NOTE: We use python version 2.7.12

## Development Notes

1. NOTE: in our implementation we assume that no header will be formatted such that the lines in the header end with a LF followed by a CR. Lines may end with LF or CR or CRLF but not LFCR
2. NOTE: In our HTTPRequest and HTTPResponse we terminate all char* fields with \0 except the body field. The body field contains the data of a message and that shouldn't be tampered with because the body itself could contain a \0 character. We can use strlen on every field other than the body. This is why we store the body's length as a field in our Request and Response objects.

3. NOTE: As of now, we cannot handle requests that do not have content length as a header field. One way to handle this in the future is if content length is not available, then keep reading from the socket until we get a read of 0 bytes, in which we can assume the server has finished sending all the data. 

4. TODO: Stress testing/scalability

5. Don't double free raw!!!

