# C-Chat-Application
2024
Chat application using C

Description:
Chat application using two separate programs. Server and client interact via a socket.

Execution:
Compile using the "make" command. Two files, chat_server and chat_client are created. Then run by passing a configuration file to the server and client, config and config2 respectively. Login to the server via the client to start chatting.

Pthread and Select:
I use the select function in the server program to detect when a client has written to the socket and process that information. In the client program I use pthread to read from the server anytime there is output.
Without the pthread in the client I would have to wait until the client makes a request to the server before they would read what was posted.
