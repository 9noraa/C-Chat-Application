#Name: Aaron Cohen
#FSUID: ajc17d

chat: chat_server.c chat_client.c
	gcc chat_server.c -o chat_server
	gcc chat_client.c -pthread -o chat_client

clean:
	rm chat_server
	rm chat_client
