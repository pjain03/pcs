# COMP112: Networks, Final Project

## Compilation
Run the following commands:
1. `chmod u+x ./scripts/compile`
2. `./scripts/compile`

## Usage
1. Run the following commands:
    * `chmod u+x ./scripts/proxy`
    * `chmod u+x ./scripts/client`
2. Run the proxy using:
    * `./scripts/proxy <port number>`
3. Edit the `./scripts/client` file to include the proxy's address. Then run the client using:
    * `./scripts/client <port number>`

## Development Notes

1. Discuss with Annie about possibly implementing various different cache systems as individual modules/files that can be imported and built in separate ways so as to easily compare and contrast their impact on performance etc.