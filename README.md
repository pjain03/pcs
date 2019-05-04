# COMP112: Networks, Final Project
1) Overview: 

For our COMP112 Networks class final project, we implemented a HTTP high performance proxy that returns searched HTTP results to the user much faster than the user having to connect to the server. This proxy has a cache and a search engine website for the cache. The proxy can handle multiple clients concurrently, and supports GET, CONNECT, OPTIONS methods, though only the GET method responses will be stored in the cache. In other words, only HTTP websites will be cached, and not HTTPS. If a HTTPS request was sent to our proxy, the proxy will make a new connection to the server and retrieve the data and pass it back to the client, in a store and forward manner. The implemented search engine has a search bar to search the proxy's cache by keyword(s). The search engine will return a list of URLs that have data with the same keyword(s) as the query. The user can then click on a URL they wish to see, and the cached data will be sent to the web interface as a preview of the actual webpage. 


Github: https://github.com/pjain03/comp112_final_project/tree/master

2) Design: Details of the key modules, their interfaces, how they interact with each other, etc. 

    - proxy.c
    - ap_utilities.h
    - cache.h
    _ search_engine.h

    - website
    - testing scripts


3) Evaluation: Performance (e.g., latency of small transfers, throughput of large transfers); scalability (e.g., how many requests per second, number of concurrent clients, etc), any other experimental evaluation that makes sense for your project. 

4) Reflections: Each group member should write about their contribution to the project, what new things they learnt, what were the main challenges they faced, etc. 




## Compilation
Run the following commands:
1. `chmod u+x ./scripts/*`
2. `./scripts/compile`
3. `pip install -r requirements.txt`

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
