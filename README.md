# COMP112: Networks, Final Project

## Compilation
Run the following commands:
1. `chmod u+x ./scripts/*`
2. `./scripts/compile`

## Usage
1. Run the proxy using:
    * `./scripts/proxy <port number>`
2. Test the proxy using our test script. Edit the `PROXY` and `RESRC` variables as indicated to test a different machine or resource respectively:
    * `./scripts/test <port number>`

## Development Notes

1. Discuss with Annie about possibly implementing various different cache systems as individual modules/files that can be imported and built in separate ways so as to easily compare and contrast their impact on performance etc.
2. We first add all the basic functionality from assignment 1 before proceeding further.
    * This includes handling one client completely (forward request to the server, forward server response to the client) before moving to another. In the future, we can handle clients by sending their requests to the server and returning to handle other clients while the server responds. When the server responds, we can forward its request to the client
3. Do we even need the client?