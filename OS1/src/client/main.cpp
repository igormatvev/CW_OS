//client
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

#define BUFFER_SIZE        500
#define MAX_NICKNAME_SIZE  30
#define MSG_SIZE           BUFFER_SIZE-MAX_NICKNAME_SIZE-4-1

void* SendMessage(int clientSocketHandle) {
    char msg[MSG_SIZE+1];
    char *pos;
    char c;

    while (true) {
        fgets(msg, sizeof(msg), stdin);

        if ((pos = strchr(msg, '\n')) != nullptr)
        {
            *pos = '\0';
        }
        else
        {
            fprintf(stdout, "[Notification]\tThe message has exceeded the allowed length. Post truncated to %d characters.\n", MSG_SIZE);
            msg[sizeof(msg)-1] = '\0';
            while ((c = getchar()) != EOF && c != '\n');
        }
        if(send(clientSocketHandle, msg, sizeof(msg), 0) < 0)
            fprintf(stderr, "[Error]\tFailed to send message.\nPlease check your connection and try again.\n");
    }
    return nullptr;
}

int connectServer(const char*  host, const char* port, const char* name){

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        fprintf(stderr, "[Error]\tFailed to create clientSocket\n");
        return -1;
    }

    struct sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(strtol(port, nullptr, 0));
    if (inet_aton(host, &clientAddr.sin_addr) == 0) {
        fprintf(stderr, "[Error]\tIncorrect IP\n");
        return -1;
    }

    if(connect(clientSocket, (struct sockaddr *) &clientAddr, sizeof(sockaddr_in)) < 0){
        fprintf(stderr, "[Error]\tFailed to connect to the server.\nCheck server connection and availability.\n");
        return -1;
    }
    
    if(send(clientSocket, name, sizeof(name), 0) < 0){
        fprintf(stderr, "[Error]\tFailed to send nickname to server.\nPlease check your connection and try again.\n");
        return -1;
    }

    return clientSocket;
}

int main(int argc, char *argv[]) {
    if(argc != 4)
    {
        fprintf(stderr, "./client [host] [port] [nickname]\nFor example, \"./client 127.0.0.1 2314 qwerty\"\n*Warning: the length of the nickname should not exceed %d, otherwise it will be cut\n*Warning: message must not exceed %d, otherwise it will be cut.\n", MAX_NICKNAME_SIZE, MSG_SIZE);
        return 1;
    }

    char name[MAX_NICKNAME_SIZE+1]{'\0'};
    if (strlen(argv[3]) > MAX_NICKNAME_SIZE){
        argv[3][MAX_NICKNAME_SIZE] = '\0';
        fprintf(stdout, "[Notification]\tNickname length exceeds the allowed value(%d). Nickname cut to \"%s\"\n", MAX_NICKNAME_SIZE, argv[3]);
    }

    strcpy(name, argv[3]);
    int clientSocket;
    if ((clientSocket = connectServer(argv[1], argv[2], name)) == -1){
        return 3;
    }

    fprintf(stdout, "\nCHAT\n");
    std::thread thread(SendMessage, clientSocket);
    thread.detach();

    int bytesRecv;
    char msg[BUFFER_SIZE];
    while ((bytesRecv = recv(clientSocket, msg, sizeof(msg), 0))) {
        if(bytesRecv < 0) {
            fprintf(stderr, "[Error]\tError receiving message from server\n");
        }
        else {
            fprintf(stdout, "%s\n", msg);
        }
        memset(msg, '\0', sizeof(msg));

    }
    std::cerr << "[Error]\tServer down\n" << std::endl;
    close(clientSocket);

    return 0;
}
