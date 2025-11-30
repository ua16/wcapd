
// Begin - Windows Libraries for threading and sockets

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <windef.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 8

// End - End of the Windows Libraries

#include <signal.h>
#include <string.h>

#include "helpers.c"
#include <stdio.h>

// miniaudio import
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
// sqlite3 - idk what I'm gonna do with this yet
#include <sqlite3.h>

// This was brought over from simpleHTTPServer
// Probably adjust the types to figure out which one we're supposed to use
enum RequestType { UNSET, GET, POST };

// The program runs while this is true
static int stayup = TRUE;

void signalHandler(int sig)
{
    printf("Caught signal %d\n", sig);
    stayup = FALSE;
    exit(0);
}

#define MAXTHREADS 30
static int activeThreads[MAXTHREADS]; // TRUE is active and FALSE is not
static int threadData[MAXTHREADS]; // Thread data needs to be passea as pointers
                                   // to the function
static SOCKET activeSockets[MAXTHREADS]; // 1 client socket per thread

// This will be where the request will be handled in
// a separate thread `threadID`` will be used to access
// both the thread and the socket
DWORD WINAPI requestHandler(void *arg);

int main(int argc, char *argv[])
{
    printf("Starting server \n");

    // Non-Network Initialization
    signal(SIGINT, signalHandler);

    for (int i = 0; i < MAXTHREADS; i++) {
        activeThreads[i] = FALSE;
    }

    HANDLE threadArray[MAXTHREADS];

    // Ignore the errors.
    // clang doesn't parse the types in windows headers very well

    // Initialize and start the socket

    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Create socket

    SOCKET listenSocket =
        socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    while (stayup == TRUE) {
        // Listen for connections
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            printf("Listen failed with error: %ld\n", WSAGetLastError());
            stayup = FALSE;
        }

        // Create a new thread and pass the threadID

        int nextActiveThread = 1;
        while (TRUE) {
            if (nextActiveThread == 30) {
                nextActiveThread = 0;
            }
            if (activeThreads[nextActiveThread] == FALSE) {
                break;
            }

            nextActiveThread++;
        }

        activeThreads[nextActiveThread] = TRUE;
        activeSockets[nextActiveThread] = accept(listenSocket, NULL, NULL);
        threadData[nextActiveThread] = nextActiveThread;

        threadArray[nextActiveThread] = CreateThread(
            NULL, 0, requestHandler, &threadData[nextActiveThread], 0, NULL);
    }

    freeaddrinfo(result);

    closesocket(listenSocket);

    WaitForMultipleObjects(MAXTHREADS, threadArray, TRUE, INFINITE);

    WSACleanup();

    return 0;
}

DWORD WINAPI requestHandler(void *arg)
{
    int threadID = *(int *)arg;
    printf("Started thread with ID : %d\n", threadID);
    SOCKET clientSocket = activeSockets[threadID];
    // Beginning of clientSocket code
    if (clientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        stayup = FALSE;
    }

    char buffer[DEFAULT_BUFLEN + 1];

    int bytesRead;

    int maxRequestSize = 7000;
    char *request = malloc(maxRequestSize);
    ZeroMemory(request, maxRequestSize);
    int requestCurLen = 0;

    do {
        // Read the request sent into `buffer` 8 bytes at a time -------- #
        buffer[DEFAULT_BUFLEN + 1] = '\0';
        bytesRead = recv(clientSocket, buffer, DEFAULT_BUFLEN, 0);

        if ((requestCurLen + bytesRead) < maxRequestSize) {
            // Copy to the request buffer
            strncpy(request + requestCurLen, buffer, bytesRead);
            requestCurLen += bytesRead;
        }

        if (bytesRead > 0) {
            // printf("%s", buffer);
        } else if (bytesRead == 0) {
        } else {
            printf("\nrecv failed: %d\n", WSAGetLastError());
            stayup = FALSE;
        }

        if (strncmp(request + requestCurLen - 4, "!BYE", 4) == 0) {
            break;
        }

    } while (bytesRead > 0);

    request[requestCurLen + 1] = '\0';

    printf("Request %s \n", request);

    // Variables about the request metadata
    enum RequestType requestType = UNSET;
    char pathname[400] = ".";

    char *token = strtok(request, " ");
    int tokenNo = 0;
    while (token != NULL) {
        // Process the stuff based on index.
        switch (tokenNo) {
        case 0:
            if (strncmp(token, "GET", 3) == 0) {
                requestType = GET;
            }
            break;
        case 1:
            strncat(pathname, token, sizeof(pathname) - 1);
            break;
        defaut:
            break;
        }
        token = strtok(NULL, " ");
        tokenNo++;
    }

    // Do something like this to send stuff
    // send(clientSocket, fileInMem, fileSize, 0);

    free(request);
    // Close the socket and send a FIN
    int iResult = shutdown(clientSocket, SD_SEND);

    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed %d\n", WSAGetLastError());
        stayup = FALSE;
    }

    // cleanup
    closesocket(clientSocket);
    // End of client Socket code

    // This might be a race condition
    activeThreads[threadID] = FALSE;
    printf("Exiting thread %d\n", threadID);
    return 0;
}
