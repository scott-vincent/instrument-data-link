#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include "simvarDefs.h"
#include "SimConnect.h"

// Data will be served on this port
const int Port = 52020;

bool quit = false;
HANDLE hSimConnect = NULL;
extern const char* SimVarDefs[][2];

// Create server thread
void server();
std::thread serverThread(server);

SimVars simVars;
double *varStart = (double *)&simVars + 1;
int varSize = 0;

enum EVENT_ID {
    SIM_START,
    SIM_STOP
};

enum DEFINITION_ID {
    DEF_ID
};

enum REQUEST_ID {
    REQ_ID
};

void CALLBACK MyDispatchProcRD(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_EVENT:
    {
        SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;

        switch (evt->uEventID)
        {
        case SIM_START:
        {
            break;
        }

        case SIM_STOP:
        {
            break;
        }

        default:
        {
            printf("Unknown event id: %ld\n", evt->uEventID);
            break;
        }
        }

        break;
    }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
    {
        SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

        switch (pObjData->dwRequestID)
        {
        case REQ_ID:
        {
            memcpy(varStart, &pObjData->dwData, varSize);
            //printf("Rpm Percent: %f\n", simVars.rpmPercent);
            break;
        }
        default:
        {
            printf("Unknown request id: %ld\n", pObjData->dwRequestID);
            break;
        }
        }

        break;
    }

    case SIMCONNECT_RECV_ID_QUIT:
    {
        quit = true;
        break;
    }
    }
}

void addDataDefs()
{
    for (int i = 0;; i++) {
        if (SimVarDefs[i][0] == NULL) {
            break;
        }

        if (SimVarDefs[i][1] == NULL) {
            // Add string
            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_ID, SimVarDefs[i][0], NULL, SIMCONNECT_DATATYPE_STRING256) < 0) {
                printf("Data def failed: %s (string)\n", SimVarDefs[i][0]);
            }
            else {
                varSize += 256;
            }
        }
        else if (strcmp(SimVarDefs[i][1], "todo") != 0) {
            // Add double (float64)
            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_ID, SimVarDefs[i][0], SimVarDefs[i][1]) < 0) {
                printf("Data def failed: %s, %s\n", SimVarDefs[i][0], SimVarDefs[i][1]);
            }
            else {
                varSize += sizeof(double);
            }
        }
    }
}

void subscribeEvents()
{
    // Request an event when the simulation starts
    //if (SimConnect_SubscribeToSystemEvent(hSimConnect, SIM_START, "SimStart") < 0) {
    //    printf("Subscribe event failed: SimStart\n");
    //}

    //if (SimConnect_SubscribeToSystemEvent(hSimConnect, SIM_STOP, "SimStop") < 0) {
    //    printf("Subscribe event failed: SimStop\n");
    //}
}

void cleanUp()
{
    if (hSimConnect) {
        if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_ID, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_NEVER, 0, 0, 0, 0) < 0) {
            printf("Failed to stop requesting data\n");
        }

        printf("Disconnecting from MS FS2020\n");
        SimConnect_Close(hSimConnect);
    }

    if (!quit) {
        printf("Stopping server\n");
        quit = true;
    }

    // Wait for server to quit
    serverThread.join();

    WSACleanup();
    printf("Finished\n");
}

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    // Yield so server can start
    Sleep(100);

    printf("Searching for local MS FS2020...\n");
    simVars.connected = 0;

    HRESULT result;

    int retryDelay = 0;
    while (!quit)
    {
        if (simVars.connected) {
            result = SimConnect_CallDispatch(hSimConnect, MyDispatchProcRD, NULL);
            if (result != 0) {
                printf("Disconnected from MS FS2020\n");
                simVars.connected = 0;
                printf("Searching for local MS FS2020...\n");
            }
        }
        else if (retryDelay > 0) {
            retryDelay--;
        }
        else {
            result = SimConnect_Open(&hSimConnect, "Instrument Data Link", NULL, 0, 0, 0);
            if (result == 0) {
                printf("Connected to MS FS2020\n");
                addDataDefs();
                subscribeEvents();
                simVars.connected = 1;

                // Start requesting data
                if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_ID, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME, 0, 0, 0, 0) < 0) {
                    printf("Failed to start requesting data\n");
                }
            }
            else {
                retryDelay = 200;
            }
        }

        Sleep(10);
    }

    cleanUp();
    return 0;
}

void server()
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("Failed to initialise Windows Sockets: %d\n", err);
        exit(1);
    }

    // Create a UDP socket
    SOCKET sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Server failed to create UDP socket\n");
        exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(Port);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Server failed to bind to localhost port %d: %ld\n", Port, WSAGetLastError());
        exit(1);
    }

    printf("Server listening on port %d\n", Port);

    sockaddr_in senderAddr;
    int addrSize = sizeof(senderAddr);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    long dataSize = sizeof(SimVars);
    long requestedSize;
    int active = -1;
    int bytes;

    while (!quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Wait for instrument panel to poll (non-blocking, 1 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, (char*)&requestedSize, sizeof(long), 0, (SOCKADDR*)&senderAddr, &addrSize);
            if (bytes > 0) {
                if (active != 1) {
                    printf("Instrument panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                    active = 1;
                }

                if (requestedSize == dataSize) {
                    // Send latest data to the client that polled us
                    bytes = sendto(sockfd, (char*)&simVars, dataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
                }
                else {
                    // Data size mismatch
                    bytes = sendto(sockfd, (char*)&dataSize, sizeof(long), 0, (SOCKADDR*)&senderAddr, addrSize);
                    printf("Client at %s requested %ld bytes instead of %ld bytes\n",
                        inet_ntoa(senderAddr.sin_addr), requestedSize, dataSize);
                }

                if (bytes <= 0) {
                    bytes = SOCKET_ERROR;
                }
            }
            else {
                bytes = SOCKET_ERROR;
            }
        }
        else {
            bytes = SOCKET_ERROR;
        }

        if (bytes == SOCKET_ERROR && active != 0) {
            printf("Waiting for instrument panel to connect\n");
            active = 0;
        }
    }

    closesocket(sockfd);

    printf("Server stopped\n");
}
