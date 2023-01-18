#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5059"
#define MAX_WORKERS 10
#define _CRT_SECURE_NO_WARNINGS

CRITICAL_SECTION cs;

#pragma region kju
typedef struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
}Queue;
Queue* createQueue(unsigned capacity)
{
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->array = (int*)malloc(
        queue->capacity * sizeof(int));
    return queue;
}
int isFull(Queue* queue)
{
    return (queue->size == queue->capacity);
}
int isEmpty(Queue* queue)
{
    return (queue->size == 0);
}
void enqueue(Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)
        % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    //printf("%d enqueued to queue\n", item);
}
int dequeue(Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)
        % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
int front(Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}
int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}
#pragma endregion kju

typedef struct parameters
{
    Queue* kju;
    int broj;
}parameters;

HANDLE incomingQueueSemaphor;
HANDLE outgoingQueueSemaphor;
HANDLE stopWorkerSemaphor[MAX_WORKERS];
int activeWorkers[MAX_WORKERS];
int brObradjenihBrojeva = 0;
int brPoslatihBrojeva = 0;

int ubacujeURed(Queue* red, int number)
{
    //ubacuje u red
    EnterCriticalSection(&cs);
    enqueue(red, number);
    LeaveCriticalSection(&cs);
    return 0;
}
int izbacujeIzReda(Queue* red)
{
    int ret = 0;
    ret = dequeue(red);
    return ret;
}
#pragma region worker
DWORD WINAPI worker(LPVOID lpParam)
{
    char debugBuffer[512];


    Queue* readQueue = ((parameters*)lpParam)->kju;
    int brojWorkera = ((parameters*)lpParam)->broj;

    printf("Broj threada %d je upaljen\n", brojWorkera);

    const int semaphore_num = 2;
    HANDLE semaphores[semaphore_num] = { stopWorkerSemaphor[brojWorkera], incomingQueueSemaphor };

    //ceka ili incomingQueue ili stopWorker semafor zbog false
    // 
    //ako je signalizirao incomingQueue sto je 0. semafor u nizu onda
    //vraca WAIT_OBJECT_0 (+ 0) u suprotnom signalizirao je stopWorker i izlazi iz while
    //dok ne dobije signal od bilo kojeg ceka
    while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
    {
        Sleep(500);
        int broj = izbacujeIzReda(readQueue);
        printf("Worker %d uzeo %d iz reda\n", brojWorkera, broj);
        broj *= -1;
        sprintf_s(debugBuffer, "%d\n", broj);
        brObradjenihBrojeva++;
        OutputDebugStringA(debugBuffer);
    }
    printf("Ugasen worker %d\n", brojWorkera);
    return 0;
}
#pragma endregion worker

bool InitializeWindowsSockets();
int brojGlobal = 0;
int aktivniWorkeri = 0;

int  main(void)
{
    Queue* incomingQueue = new Queue; //red u koji klijent smesta brojeve
    incomingQueue = createQueue(10);

    Queue* outgoingQueue = new Queue;
    outgoingQueue = createQueue(10);

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET acceptedSocket = INVALID_SOCKET;

    int iResult;
    char recvbuf[DEFAULT_BUFLEN];
    char messageToSend[DEFAULT_BUFLEN];

    parameters parametri;

    incomingQueueSemaphor = CreateSemaphore(NULL, 0, 10, NULL);
    outgoingQueueSemaphor = CreateSemaphore(NULL, 0, 10, NULL);
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        stopWorkerSemaphor[i] = CreateSemaphore(NULL, 0, 1, NULL);
    }
    DWORD workerID[MAX_WORKERS];
    HANDLE workers[MAX_WORKERS];
#pragma region komunikacijaStuff
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

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server initialized, waiting for clients.\n");
#pragma endregion

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
            // Receive data until the client shuts down the connection
            iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
            {
                //printf("Message received from client: %s.\n", recvbuf);
                int number = atoi(recvbuf);
                char proveravamo[] = "BROJGLOBAL:";
                if (strstr(recvbuf, proveravamo) != NULL) {
                    brojGlobal = atoi(recvbuf + 11);
                    while (aktivniWorkeri < brojGlobal)
                    {
                        parametri.kju = incomingQueue;
                        parametri.broj = aktivniWorkeri;
                        workers[aktivniWorkeri] = CreateThread(NULL, 0, &worker, &parametri, 0, workerID + aktivniWorkeri);
                        aktivniWorkeri++;
                    }
                    while (aktivniWorkeri > brojGlobal)
                    {
                        aktivniWorkeri--;
                        ReleaseSemaphore(stopWorkerSemaphor[aktivniWorkeri], 1, NULL);
                        CloseHandle(workers[aktivniWorkeri]);
                    }
                }
                else
                {
                    brPoslatihBrojeva++;
                    if (aktivniWorkeri > 0) {

                        enqueue(incomingQueue, number);
                        ReleaseSemaphore(incomingQueueSemaphor, 1, NULL);
                    }
                    else {
                        OutputDebugStringA("********Nije upaljen ni jedan worker pa vrednost ");
                        char s[10];
                        sprintf_s(s, "%d", number);
                        OutputDebugStringA(s);
                        OutputDebugStringA(" nije obradjena.\n");
                    }

                    if (brPoslatihBrojeva >= 100)
                    {
                        char printBuffer[256];
                        sprintf_s(printBuffer, "Od %d poslatih zahteva obradjeno je %d.\n", brPoslatihBrojeva, brObradjenihBrojeva);
                        OutputDebugStringA(printBuffer);
                        brPoslatihBrojeva = 0;
                        brObradjenihBrojeva = 0;

                    }
                }
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


