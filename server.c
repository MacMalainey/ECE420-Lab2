#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>

#include "common.h"

int theLen;
char** theArray;
pthread_mutex_t* theArrayLocks;

pthread_mutex_t nextClientMutex;
pthread_cond_t handlerIsIdleCond;
pthread_cond_t incomingClientCond;

int incomingClient = NULL;
int theThreadsWaiting = 0;

void* handleClient(void*)
{
    int clientSocket;
    char str[COM_BUFF_SIZE];
    ClientRequest req;

    while (1) {
        pthread_mutex_lock(&nextClientMutex);
        while (incomingClient == NULL) {
            theThreadsWaiting++;
            pthread_cond_signal(&handlerIsIdleCond);
            pthread_cond_wait(&incomingClientCond, &nextClientMutex);
            theThreadsWaiting--;
        }

        clientSocket = incomingClient;
        incomingClient = NULL;
        pthread_mutex_unlock(&nextClientMutex);

        read(clientSocket, str, COM_BUFF_SIZE);
        ParseMsg(&str, &req);

        if (req.is_read > 0) {

        }

        write(clientSocket, str, COM_BUFF_SIZE);
        close(clientSocket);

    }
    return NULL;
}


int main(int argc, char* argv[])
{
    // todo: args
    // 1 - size of string array
    // 2 - server IP
    // 3 - server port

    // Initialize theArray and theArrayLocks
    theArray = (char**)malloc(theLen * sizeof(char*));
    theArrayLocks = (pthread_mutex_t*)malloc(theLen * sizeof(pthread_mutex_t));
    for (int i = 0; i < theLen; i++) {
        theArray[i] = (char*)malloc(COM_BUFF_SIZE * sizeof(char));
        pthread_mutex_init(&theArrayLocks[i], NULL);
    }

    pthread_t* t = malloc(COM_NUM_REQUEST*sizeof(pthread_t));
    // TODO - init threads

    // Initialize socket info
    struct sockaddr_in sock_var;
    int hostSocket=socket(AF_INET,SOCK_STREAM,0);

    // Setup server info
    sock_var.sin_addr.s_addr=inet_addr("127.0.0.1"); // TODO - set from argument
    sock_var.sin_port=3000; // TODO - set from argument
    sock_var.sin_family=AF_INET;
    if (
        bind(
            hostSocket,
            (struct sockaddr*) &sock_var,
            sizeof(sock_var)
        ) < 0
    ) {
        printf("Failed to bind socket - exiting\n");
    }

    listen(hostSocket,2000);
    while (1) {
        int next = accept(hostSocket, NULL, NULL);

        pthread_mutex_lock(&nextClientMutex);
        while (theThreadsWaiting <= 0) {
            pthread_cond_signal(&incomingClientCond);
            pthread_cond_wait(&handlerIsIdleCond, &nextClientMutex);
        }

        incomingClient = next;
        pthread_mutex_unlock(&nextClientMutex);
    }
    close(hostSocket);
    return 0;
}