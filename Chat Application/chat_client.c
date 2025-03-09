/*
* Name: Aaron Cohen
* FSUID: ajc17d
* Course: COP 5570
* Assignment: 2
*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#define MAX_LENGTH 80

			//Global client socket variable
int client_socket = 0;

char* lower(char* word);
char* strremove(char* str, const char* sub);
void* read_server(void* arg);
void intHandler(int sig);

int main(int argc, char* argv[]) {
			//Usage 
	if (argc != 2) {
		printf("Usage: chat_client [config_file]\n");
		return 0;
	}

	bool logged = false;
	bool send = true;
	char message[MAX_LENGTH];
	//int client_socket = 0;
	int server_read;
	struct sockaddr_in server_address;
	FILE* conf;
	char* line1;
	char* line2;
	size_t len = 0;
	ssize_t read1;
	char* substring = "servhost: ";
	char* servhost;
	pthread_t tid;

			//Opening configuration file
	conf = fopen(argv[1], "r");
	if (conf == NULL) {
		printf("Failed to open file");
		exit(EXIT_FAILURE);
	}

			//Reading to get the servhost line
	while ((read1 = getline(&line1, &len, conf)) != -1) {
		if (strstr(lower(line1), "servhost") != NULL) {
			printf("%s \n", line1);
			break;
		}
	}
	len = 0;
	fclose(conf);
	conf = fopen(argv[1], "r");
	if (conf == NULL) {
		printf("Failed to open file");
		exit(EXIT_FAILURE);
	}

			//Reading to get the servport line
	while ((read1 = getline(&line2, &len, conf)) != -1) {
		if (strstr(lower(line2), "servport") != NULL) {
			printf("%s \n", line2);
			break;
		}
	}

			//Extracting the port number from the line
	char* temp = line2;
	long port;
	while (*temp) {
		if (isdigit(*temp)) {
			port = strtol(temp, &temp, 10); // Read number
			printf("%ld\n", port);
		}
		else {
			temp++;
		}
	}

	/*temp = line1;
	int i = 0;
	int j = 0;
	int k = sizeof(line1);
	while (*temp) {
		if (isdigit(*temp)) {
			k = k + i;
			for (j = 0; i < k; j++) {
				printf("testing\n");
				servhost[j] = line1[i];
				i++;
			}
			break;
		}
		else {
			temp++;
		}
		i++;
	}*/

	servhost = strremove(line1, substring);
	printf("%s", servhost);

			//Creating the client's socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (client_socket == -1) {
		perror("socket failed\n");
		exit(EXIT_FAILURE);
	}
	
	bzero(&server_address, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(servhost);
	server_address.sin_port = htons(port);

			//Connecting to the server 
	if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
		printf("connection failed\n");
		exit(EXIT_FAILURE);
	}
	else {
		printf("connected to server\n");
	}

			//Signal handler
	signal(SIGINT, intHandler);

	printf("Welcome to the chat server!\n");
			//Thread creator for the read_server function
	pthread_create(&tid, NULL, read_server, NULL);

	int a;

			//Loop to read user input and write to the server 
	while (true) {
		//printf(">> ");
		bzero(message, sizeof(message));
		a = 0;
		while ((message[a++] = getchar()) != '\n');
		message[strcspn(message, "\n")] = 0;
		//printf("testing\n");

			//Cases for login, logout, exit and chat
		if ((strncmp(message, "login", 5)) == 0 && (logged == true)) {
			printf("Already logged in\n");
			send = false;
		}else if ((strncmp(message, "login", 5)) == 0 && (logged == false)) {
			logged = true;
			send = true;
		}else if ((strncmp(message, "exit", 4)) == 0 && (logged == false)) {
			printf("Not logged in\n");
			send = false;
		}else if ((strncmp(message, "exit", 4)) == 0 && (logged == true)) {
			write(client_socket, message, sizeof(message));
			break;
		}else if ((strncmp(message, "logout", 6)) == 0 && (logged == false)) {
			printf("Already logged out\n");
			send = false;
		}else if ((strncmp(message, "logout", 6)) == 0 && (logged == true)) {
			logged = false;
			send = true;
		}else if (((strncmp(message, "chat", 4)) == 0) && logged == false) {
			printf("Must be logged in to chat\n");
			send = false;
		}else if (((strncmp(message, "chat", 4)) == 0) && logged == true) {
			send = true;
		}else {
			printf("Invalid command\n");
			send = false;
		}

			//Sending the message to the server 
		if (send) {
			write(client_socket, message, sizeof(message));
			bzero(message, sizeof(message));
			//read(client_socket, message, sizeof(message));
			//printf("%s\n", message);
		}
		send = true;
	}

	fclose(conf);
	close(client_socket);
	return 0;
}

char* lower(char* word) {
	int i = 0;
	for (i = 0; word[i]; i++)
		word[i] = tolower(word[i]);
	return word;
}

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

			//Function to read from server when there is input
void* read_server(void* arg) {
	char buf[1024];
	int r;
	pthread_detach(pthread_self());

	while (true) {
		r = read(client_socket, buf, sizeof(buf));

		if (r <= 0) {
			if (r == 0) {
				printf("Server has closed\n");
			}
			close(client_socket);
			exit(1);
		}
		printf("%s\n", buf);
		bzero(buf, sizeof(buf));
	}
}

			//Signal handler for ctrl c
void intHandler(int sig) {
	char r;
	signal(sig, SIG_IGN);

			//Asking user for confirmation
	printf("\nDo you want to exit? y or n\n");
	r = getchar();
	if (r == 'y')
		exit(1);

	signal(SIGINT, intHandler);
	getchar();

}