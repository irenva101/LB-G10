#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define SERVER_IP_ADDRESS "127.0.0.1"


HANDLE globalSemaphore;
CRITICAL_SECTION cs;
int globalna = 0;

DWORD WINAPI ActivateWorkers(LPVOID lpvThreadParam) {

    SOCKET connectSocket = (SOCKET)lpvThreadParam;
    globalSemaphore = CreateSemaphore(0, 0, 1, NULL);
    bool changedValue = false;
    do {
        //char temp[DEFAULT_BUFLEN];
        int a;
        printf("Da upalis workera pritisni 1, a da ugasis pritisni 0.\n");
        scanf_s("%d", &a);
        if (a == 1)
        {
            printf("Upalio si ga :>\n");
            globalna++;
            changedValue = true;
        }
        else if (a == 0)
        {
            printf("Ugasio si ga :<\n");
            globalna--;
            changedValue = true;
        }
        else
        {
            printf("Niste uneli 0 ili 1. Probajte opet.\n");
            changedValue = false;
        }

        if (changedValue)
        {
            //ReleaseSemaphore(globalSemaphore, 1, NULL);
            char messageToSend[DEFAULT_BUFLEN];
            int iResult;
            sprintf(messageToSend, "BROJGLOBAL:%d", globalna);
            iResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);
        }
        //gets_s(temp, DEFAULT_BUFLEN);
        

    } while (true);
}

bool InitializeWindowsSockets();

int  main()
{
    SOCKET connectSocket = INVALID_SOCKET;

    int iResult;
    char messageToSend[DEFAULT_BUFLEN];
    char temp[DEFAULT_BUFLEN]; //prevelik je al sta je tu je 
    char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
    serverAddress.sin_port = htons(DEFAULT_PORT);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    DWORD myThreadID1;
    DWORD WINAPI myThread1(LPVOID lpvThreadParam);
    HANDLE activeWorkerThread = CreateThread(NULL, 0, &ActivateWorkers, (LPVOID)connectSocket, 0, &myThreadID1);
    do {
        
        //gets_s(messageToSend, DEFAULT_BUFLEN);
        //WaitForSingleObject(globalSemaphore, INFINITE);
        sprintf(messageToSend, "%d", rand() % 5000);
        iResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);

        
        if (messageToSend[0] == 'q') {
            closesocket(connectSocket);
            WSACleanup();
            break;
        }
        Sleep(100);
        /*iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0)
        {
            printf("%s\n", recvbuf);
            //printf("Message received from client: %s.\n", recvbuf);
        }
        else if (iResult == 0)
        {
            // connection was closed gracefully
            printf("Connection with client closed.\n");
            closesocket(connectSocket);
        }
        else
        {
            // there was an error during recv
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
        }
        */

    } while (1);

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    closesocket(connectSocket);
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