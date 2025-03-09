/*
* Name: Aaron Cohen
* FSUID: ajc17d
* Course: COP 5570
* Assignment: 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <stdbool.h>
#define MAXCONNS 5

			//Client struct to hold the usernames and fd for each connected user
struct client {
	char username[20];
	int fd;
};

			//Global client array
struct client clients[MAXCONNS];

char* lower(char* word);
char* strremove(char* str, const char* sub);

int main(int argc, char* argv[]) {

			//Check for valid cmd line usage
	if (argc != 2) {
		printf("Usage: chat_server [config_file]\n");
		return 0;
	}
	
	int server_fd;
	int con_socket, read_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char message[1024] = { 0 };
	int opt = 1;
	int maxfd;
	int addrLen;
	fd_set allset, rset;
	//int clients[MAXCONNS];
	char* line;
	size_t len = 0;
	ssize_t readf;
	FILE* conf;

			//Opening config file 
	conf = fopen(argv[1], "r");
	if (conf == NULL) {
		printf("Failed to open file\n");
		exit(EXIT_FAILURE);
	}
	printf("Success opening file\n");

			//Reading lines in the file until I see the port keyword
	while ((readf = getline(&line, &len, conf)) != -1) {
		if (strstr(lower(line), "port") != NULL) {
			printf("%s \n", line);
			break;
		}
	}

			//Extracting the port value by taking each character/digit
	char* temp = line;
	long port;
	while (*temp) {
		if (isdigit(*temp)) {
			port = strtol(temp, &temp, 10);
			printf("%ld\n", port);
		}
		else {
			temp++;
		}
	}
	
			//Opening the server socket 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed\n");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

				//Binding the address to the socket I just made
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed\n");
		exit(EXIT_FAILURE);
	}

	addrLen = sizeof(address);
	if (getsockname(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen) < 0) {
		perror("get socket failed\n");
		exit(EXIT_FAILURE);
	}
	printf("\tSERVER \nDOMAIN: %s\nPORT: %d\n", inet_ntoa(address.sin_addr), htons(address.sin_port));

			//Initializing the listen on the created socket 
	if (listen(server_fd, 5) < 0) {
		perror("listen failed\n");
		exit(EXIT_FAILURE);
	}

	int i;
	int j;
			//Initializing the values for each client element
	for (i = 0; i < MAXCONNS; i++) {
		clients[i].fd = -1;
		clients[i].username[0] = '@';
	}
	//for (i = 0; i < MAXCONNS; i++) clients[i] = -1;

	FD_ZERO(&allset);
	FD_SET(server_fd, &allset);
	maxfd = server_fd;
	char t[1024];
	char* tag = ">> ";
	char* name = "@";
	char* msg = "";

			//Server loop
	while (true) {
		//printf("test\n");
		rset = allset;
		select(maxfd + 1, &rset, NULL, NULL, NULL);

			//Accepting each client that tries to connect to the socket
		if (FD_ISSET(server_fd, &rset)) {
			if ((con_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
				if (errno == EINTR) {
					continue;
				}
				else {
					perror("accept error\n");
					exit(EXIT_FAILURE);
				}
			}

			//Printing when a user connects
			printf("new user connected\n");

			//Initializing the new client fd
			for (i = 0; i < MAXCONNS; i++) {
				if (clients[i].fd < 0) {
					clients[i].fd = con_socket;
					FD_SET(clients[i].fd, &allset);
					break;
				}
			}
			if (i == MAXCONNS) {
				printf("too many connections\n");
				close(con_socket);
			}
			if (con_socket > maxfd) {
				maxfd = con_socket;
			}
		}
		for (i = 0; i < MAXCONNS; i++) {
			if (clients[i].fd < 0)
				continue;
			//If there is something to be read 
			if (FD_ISSET(clients[i].fd, &rset)) {
				int num;
				num = read(clients[i].fd, message, sizeof(message));

			//Checking if the user wants to exit
				if ((strncmp(message, "exit", 4)) == 0) {
					num = 0;
				}

			//Exiting from the server
				if (num == 0) {
					close(clients[i].fd);
					FD_CLR(clients[i].fd, &allset);
					clients[i].fd = -1;
					bzero(clients[i].username, sizeof(clients[i].username));
					clients[i].username[0] = '@';
				}
				else {

			//Login sequence 
					if ((strncmp(message, "login", 5)) == 0) {
						char* s = strremove(message, "login ");
						strcat(clients[i].username, s);
						msg = "You have logged in";
						printf("Logged in as: %s\n", clients[i].username);
						write(clients[i].fd, msg, num);
						//write(clients[i].fd, msg, num);
					}
			
			//Logout sequence
					if ((strncmp(message, "logout", 6)) == 0) {
						printf("%s has logged out", clients[i].username);
						clients[i].username[0] = '@';
						msg = "You have logged out";
						write(clients[i].fd, msg, num);
					}

			//Chat sequence is split into @ and no @
					if ((strncmp(message, "chat", 4)) == 0) {
						bzero(t, sizeof(t));
						strcat(t, clients[i].username);
						strncpy(t, strremove(t, "@"), sizeof(strremove(t, "@")));
						strcat(t, tag);
						printf("chat sent by: %s\n", clients[i].username);

			//If there is a user to send it to go through each client and compare their usernames to the given name parameter from user
						if ((strncmp(message, "chat @", 6)) == 0) {
							for (j = 0; j < MAXCONNS; j++) {
								//printf("%s and %s and %ld\n", strremove(message, "chat "), clients[j].username, strlen(clients[j].username));
								if (strncmp(strremove(message, "chat "), clients[j].username, strlen(clients[j].username)) == 0) {
									strcpy(message, strremove(message, "chat "));
									strcat(t, strremove(message, clients[j].username));
									
									//printf("sending to client\n");
									write(clients[i].fd, t, num);
									write(clients[j].fd, t, num);
								}
							}
							
						}

			//If no user to send to I send it to every user
						else {
							strcat(t, strremove(message, "chat "));
							write(clients[i].fd, t, num);
							for (j = 0; j < MAXCONNS; j++) {
								if (strncmp(clients[i].username, clients[j].username, sizeof(clients[i].username)) != 0) {
									write(clients[j].fd, t, num);
								}
							}
						}
					}
					bzero(message, sizeof(message));
					//write(clients[i].fd, message, num);
				}
			}
		}
	}
	/*if ((con_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
		perror("accept failed\n");
		exit(EXIT_FAILURE);
	}*/
	//read_socket = read(con_socket, message, 1024);
	fclose(conf);
	close(server_fd);
	return 0;
}

			//Function that returns lower case of a string
char* lower(char* word) {
	int i = 0;
	for (i = 0; word[i]; i++)
		word[i] = tolower(word[i]);
	return word;
}

			//Function to remove a substring from a string and return
char* strremove(char* str, const char* sub) {
	size_t len = strlen(sub);
	if (len > 0) {
		char* p = str;
		while ((p = strstr(p, sub)) != NULL) {
			memmove(p, p + len, strlen(p + len) + 1);
		}
	}
	return str;
}