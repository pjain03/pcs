# COMP112: Networks, Final Project

## Compilation
Run the following commands:
1. `chmod u+x ./scripts/*`
2. `./scripts/compile`

## Usage
1. Run the proxy using:
    * `./scripts/exe_proxy <port number> <eviction policy>`
    Eviction policies to choose from: lru, mru, random
    If no eviction policy was provided, `lru` is the default
2. Test the proxy using our test script. Edit the `PROXY` and `RESRC` variables defined in `./scripts/test` as indicated to test a different machine or resource respectively:
    * `./scripts/test <port number>`
    * NOTE: We use python version 2.7.12

## Development Notes

1. NOTE: in our implementation we assume that no header will be formatted such that the lines in the header end with a LF followed by a CR. Lines may end with LF or CR or CRLF but not LFCR
2. In our HTTPRequest and HTTPResponse we terminate all char* fields with \0 except the body field. The body field contains the data of a message and that shouldn't be tampered with because the body itself could contain a \0 character. We can use strlen on every field other than the body. This is why we store the body's length as a field in our Request and Response objects.
3. CONNECT