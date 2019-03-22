# COMP112: Networks, Final Project

## Compilation
Run the following commands:
1. `chmod u+x compile`
2. `./compile`

## Usage
1. Run the following commands:
    * `chmod u+x server`
    * `chmod u+x client`
2. Run the server using:
    * `./server <port number>`
3. Edit the `client` file to include the server's address. Then run the client using:
    * `./client <port number>`

## Development Notes

1. Discuss with Annie about possibly implementing various different cache systems as individual modules/files that can be imported and built in separate ways so as to easily compare and contrast their impact on performance etc.
2. Since client and server share so many structures (such as packets and their manipulation) maybe we should explore the idea of a common utilities class that both the client and the server can use.
    * UPDATE: It's been set up. It works for the very basic stage we're at at the moment.