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
#define DEFAULT_PORT 5059
#define SERVER_IP_ADDRESS "127.0.0.1"


CRITICAL_SECTION cs;

int globalna = 0;
int brPoslatihBrojeva = 0;



DWORD WINAPI ActivateWorkers(LPVOID lpvThreadParam) {

    SOCKET connectSocket = (SOCKET)lpvThreadParam;
    bool changedValue = false;
    do {
        int a;
        printf("Da upalis workera pritisni 1, a da ugasis pritisni 0.\n");
        scanf_s("%d", &a);
        if (a == 1)
        {
            if (globalna == 10) {
                printf("Maksimalan broj upaljenih workera je 10. Ne mozete upaliti ni jednog vise.\n");
            }
            else {
                printf("Upalio si ga :>\n");
                globalna++;
                changedValue = true;
            }
        }
        else if (a == 0)
        {
            printf("Ugasio si ga :<\n");
            if (globalna == 0) {
                printf("Ne mozete ugasiti workera jer nije ni jedan upaljen.\n");
            }
            else  globalna--;
            changedValue = true;
        }
        else
        {
            printf("Niste dobro ukucali broj.\n");
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
    } while (true);
}



bool InitializeWindowsSockets();

int  main()
{
    SOCKET connectSocket = INVALID_SOCKET;

    int iResult = 0;
    char messageToSend[DEFAULT_BUFLEN];
    char temp[DEFAULT_BUFLEN]; //prevelik je al sta je tu je 
    char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == false)
    {

        return 1;
    }

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
        sprintf(messageToSend, "%d", rand() % 5000);
        brPoslatihBrojeva++;
        iResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);

        Sleep(100);

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