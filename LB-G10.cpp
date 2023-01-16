#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define _CRT_SECURE_NO_WARNINGS
#define MAX_WORKER_COUNT 10  

bool InitializeWindowsSockets();

#pragma region red
#define true 1
#define false 0
#define MAX 10
struct QUEUE {
    int F;
    int R;
    int data[MAX];
};

int isFull(struct QUEUE* s) {
    return s->R == MAX - 1 ? true : false;
}

int isEmpty(struct QUEUE* s) {
    return s->R < s->F ? true : false;
}

void enqueue(struct QUEUE* s, int element) {
    s->R += 1; //kada dodajemo element REAR povecavamo za 1
    s->data[s->R] = element;
}

int dequeue(struct QUEUE* s) {
    int ret = s->data[s->F]; //vrednost prvog u redu
    s->F += 1;
    return ret;
}

int checkFront(struct QUEUE* s) {
    return s->data[s->F];
}

int checkRear(struct QUEUE* s) {
    return s->data[s->R];
}
#pragma endregion 
#pragma region Worker
HANDLE globalSemaphore;
DWORD WINAPI WorkersDoingWork(LPVOID lpvThreadParam) {
    do {
        //waitforsingle object
        //if queue not empty
        //izvucem iz reda 
        //ispisem u outputu

    } while (true);
}
#pragma endregion
int brAktivnihWorkera = 0;
CRITICAL_SECTION cs;

int  main(void)
{
    DWORD workerID[MAX_WORKER_COUNT]; //10 spremnih
    HANDLE workerThreads[MAX_WORKER_COUNT];
    
   

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET acceptedSocket = INVALID_SOCKET;

    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    char messageToSend[DEFAULT_BUFLEN];

    struct QUEUE red = { 0,-1 };

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server initialized, waiting for clients.\n");
    
    do
    {
        acceptedSocket = accept(listenSocket, NULL, NULL);
        if (acceptedSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        do
        {
            iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
            {
                printf("Message received from client: %s.\n", recvbuf);

                char subString[DEFAULT_BUFLEN];
                memset(subString, '\0', sizeof(subString));
                strncpy_s(subString, recvbuf, 11);
                if(strcmp(subString, "BROJGLOBAL:")==0){
                    strncpy_s(subString, recvbuf + 11, 2);
                    printf("%s\n", subString);
                    brAktivnihWorkera = atoi(subString);
                }
                else {
                    int number = atoi(recvbuf);
                    if (!isFull(&red)) {
                        EnterCriticalSection(&cs);
                        enqueue(&red, number);
                        LeaveCriticalSection(&cs);
                        //releasesemaphore
                    }
                    else {
                        //nista, broj samo nece biti obradjen u tom momentu
                    }
                }
                
                // taj broj poredim sa trenutnim br aktivnim tredova
                // 
                //trebace mi i funkcija close i open worker()
                //u zavisnosti koliko imam workera toliko imam niti (open close hadle)
              
                
                OutputDebugStringA(recvbuf);//ovo treba da ispise u debug kozoli
                OutputDebugStringA("\n");
            }
            else if (iResult == 0)
            {
                // connection was closed gracefully
                printf("Connection with client closed.\n");
                closesocket(acceptedSocket);
            }
            else
            {
                // there was an error during recv
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocket);
            }
        } while (iResult > 0);

        // here is where server shutdown loguc could be placed

    } while (1);

    // shutdown the connection since we're done
    iResult = shutdown(acceptedSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocket);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocket);
    closesocket(acceptedSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}