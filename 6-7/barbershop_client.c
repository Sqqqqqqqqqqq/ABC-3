#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define MAX_NAME_LENGTH 20


int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <client_name> <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    char client_name[MAX_NAME_LENGTH];
    strcpy(client_name, argv[1]);

    char server_ip[16];
    strcpy(server_ip, argv[2]);

    int server_port = atoi(argv[3]);

    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        return 1;
    }

    send(client_socket, client_name, sizeof(client_name), 0);

    char server_message[100];
    recv(client_socket, server_message, sizeof(server_message), 0);
    printf("Server message: %s\n", server_message);
    
    recv(client_socket, server_message, sizeof(server_message), 0);
    printf("Server message: %s\n", server_message);
    
    recv(client_socket, server_message, sizeof(server_message), 0);
    printf("Server message: %s\n", server_message);

    close(client_socket);

    return 0;
}

