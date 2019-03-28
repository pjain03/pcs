# COMP112: Networks, Final Project

## Compilation
Run the following commands:
1. `chmod u+x ./scripts/*`
2. `./scripts/compile`

## Usage
1. Run the proxy using:
    * `./scripts/proxy <port number>`
2. Test the proxy using our test script. Edit the `PROXY` and `RESRC` variables defined in `./scripts/test` as indicated to test a different machine or resource respectively:
    * `./scripts/test <port number>`

## Development Notes

1. Concurrently handling multiple clients. (Send client request to the server, while we
   wait for a repsonse, do other stuff etc)
2. Do we even need the client? Maybe we can use it to run a variety of different tests? Or maybe run the test script on a variety of different machines...?
3. NOTE: in our implementation we assume that no header will be formatted such that the lines in the header end with a LF followed by a CR. Lines may end with LF or CR or CRLF but not LFCR
4. In our HTTPRequest and HTTPResponse we terminate all char* fields with \0 except the body field. The body field contains the data of a message and that shouldn't be tampered with because the body itself could contain a \0 character. We can use strlen on every field other than the body. This is why we store the body's length as a field in our Request and Response objects.