# COMP112: Networks, Final Project

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
3. TODO: Making the proxy more robust, setting benchmarks (cache testing), handling memory
    - Our proxy should be able to run on our local machines (Macbook Air) for around 30-45 minutes of regular browsing
        - Write extensive testing scripts (~2 hours, Annie)
            - Stress Testing
            - Memory Benchmarking
            - Speed Benchmarking
        - Fixing memory stuff + making proxy robust (Annie, Pulkit)
            - Before editing, contact the other person
4. TODO: Search engine
    - Set up website/proxy (~2 hours, Pulkit)
        - Website (DONE)
            - Forms for accepting proxy, port, and search bar
            - On submitting the form, it queries the cache (Sends a GET for the requested keywords)
        - Proxy (DONE)
            - Handle GET to our proxy with query parameters
    - Set up search engine functionality
5. Don't double free raw!!!
