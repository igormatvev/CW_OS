//server
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>

#define BUFFER_SIZE        500
#define MAX_NICKNAME_SIZE  30
#define MSG_SIZE           BUFFER_SIZE-MAX_NICKNAME_SIZE-4-1

struct client{
    int socketHandle;
    int id;
    char nickname[MAX_NICKNAME_SIZE + 1]{'\0'};
};

std::vector<client> clients;
std::mutex mutex;

void sendToAll(char msg[BUFFER_SIZE]) {
    mutex.lock();

    for(unsigned int i = 0; i < clients.size(); i++) {
        if(send(clients[i].socketHandle, msg, BUFFER_SIZE, 0) < 0)
            fprintf(stderr, "[ERROR]\tFailed to delete message to user \"%s\"\n", clients[i].nickname);
    }
    mutex.unlock();
}

void removeClient(int id) {
    int index = -1;

    mutex.lock();
    for(unsigned int i = 0; i < clients.size(); i++) {
        if(clients[i].id == id)
            index = (int)i;
    }
    if(index != -1) {
        clients.erase(clients.begin() + index);
    }
    else {
        fprintf(stderr, "[ERROR]\tFailed to remove client\n");
    }
    mutex.unlock();
}


void* handleClient(int clientSocketHandle) {
    char name[MAX_NICKNAME_SIZE+1];
    char msg[MSG_SIZE+1];
    char buffer[BUFFER_SIZE];


    struct client* newClient = new client();
    newClient->id = clients.size();
    newClient->socketHandle = clientSocketHandle;

    memset(buffer, 0, sizeof(buffer));
    int bytesRecv;

    if(recv(clientSocketHandle, name, sizeof(name), 0) < 0){
        delete newClient;
        return nullptr;
    }
    else{
        strcpy(newClient->nickname, name);
        fprintf(stdout, "[INFO]\tA new user has joined! He was given an ID #%d, his nickname:%s!\n", newClient->id, newClient->nickname);

        buffer[0] = '\0';
        strcat(buffer, "[SERVER]\t");
        strcat(buffer, newClient->nickname);
        strcat(buffer, " joined the chat");
        sendToAll(buffer);
        mutex.lock();
        clients.push_back(*newClient);
        mutex.unlock();
    }

    while (true) {
        buffer[0] = '\0';
        bytesRecv = recv(clientSocketHandle, msg, sizeof(msg), 0);

        if(bytesRecv == 0) {
            fprintf(stdout, "[INFO]\tUser #%d(%s) disconnected from the server!\n", newClient->id, newClient->nickname);
            buffer[0] = '\0';
            strcat(buffer, "[SERVER]\t");
            strcat(buffer, newClient->nickname);
            strcat(buffer, " left the chat...");
            sendToAll(buffer);

            close(clientSocketHandle);
            removeClient(newClient->id);
            break;
        }
        else if(bytesRecv < 0) {
            fprintf(stderr, "[ERROR]\tError while receiving message from client with id #%d!\n", newClient->id);
        }
        else {
            strcat(buffer, newClient->nickname);
            strcat(buffer, ">  ");
            strcat(buffer, msg);
            fprintf(stdout, "[INFO]\tUser #%d(%s) sent a message:\"%s\"\n",  newClient->id, newClient->nickname, msg);
            sendToAll(buffer);
        }
    }
    return nullptr;
}


int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        fprintf(stderr, "./server [port]\nFor example, \"./server 2314\"\n");
        return 1;
    }
    char* port = argv[1];

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        fprintf(stderr, "[Error]\tFailed to create serverSocket\n");
        return -1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(strtol(port, nullptr, 0));

    if(bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr_in)) < 0){
        fprintf(stderr, "[Error]\tFailed to name socket\n");
        return 1;
    }

    listen(serverSocket, SOMAXCONN);
    fprintf(stderr, "[INFO]\tServer is up!\n");
    fprintf(stderr, "[INFO]\tThe server is waiting for calls...\n");

    int clientSocketHandle;
    while (true){
        if((clientSocketHandle = accept(serverSocket, nullptr, nullptr)) < 0) {
            fprintf(stderr, "[ERROR]\tError establishing connection with client\n");
        }
        else{
            std::thread thread(handleClient, clientSocketHandle);
            thread.detach();
        }
    }

    return 0;
}
