#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include "SimConnect.h"
#include <strsafe.h>

// Data will be served on this port
const int Port = 52020;

bool connected = false;
bool quit = false;
HANDLE hSimConnect = NULL;

// Create server thread
void server();
std::thread serverThread(server);

struct SimVars
{
    double kohlsmann = 29.92;
    double altitude = 0;
    double latitude = 123.45;
    double longitude = 234.56;
    char title[256] = "A title";
} simVars;

SimVars* pSimVars = &simVars;

enum EVENT_ID {
    EVENT_SIM_START,
};

enum DATA_DEFINE_ID {
    DEFINITION_1,
};

enum DATA_REQUEST_ID {
    REQUEST_1,
};

void CALLBACK MyDispatchProcRD(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    HRESULT hr;

    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_EVENT:
    {
        SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;

        switch (evt->uEventID)
        {
        case EVENT_SIM_START:
            printf("Sim started\n");
            // Now the sim is running, request information on the user aircraft
            hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, REQUEST_1, DEFINITION_1, 0, SIMCONNECT_SIMOBJECT_TYPE_USER);
            break;
        }

        break;
    }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
    {
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE*)pData;

        switch (pObjData->dwRequestID)
        {
        case REQUEST_1:
        {
            DWORD ObjectID = pObjData->dwObjectID;
            pSimVars = (SimVars*)&pObjData->dwData;
            if (SUCCEEDED(StringCbLengthA(&pSimVars->title[0], sizeof(pSimVars->title), NULL))) // security check
            {
                printf("\nObjectID=%d  Lat=%f  Lon=%f  Alt=%f  Kohlsman=%.2f  Title=\"%s\"", ObjectID,
                    pSimVars->latitude, pSimVars->longitude, pSimVars->altitude, pSimVars->kohlsmann, pSimVars->title);
            }
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

    default:
        printf("\nReceived id: %d", pData->dwID);
        break;
    }
}

void addDataDefs()
{
    //HRESULT hr;

    if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Kohlsman setting hg", "inHg"))) {
        printf("Data def failed: kohlsman\n");
    }
    if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Altitude", "feet"))) {
        printf("Data def failed: Altitude\n");
    }
    if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Latitude", "degrees"))) {
        printf("Data def failed: Latitude\n");
    }
    if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Longitude", "degrees"))) {
        printf("Data def failed: Longitude\n");
    }
    if (!SUCCEEDED(SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Title", NULL, SIMCONNECT_DATATYPE_STRING256))) {
        printf("Data def failed: Title\n");
    }
}

void subscribeEvents()
{
    //HRESULT hr;

    // Request an event when the simulation starts
    if (!SUCCEEDED(SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart"))) {
        printf("Subscribe event failed: SimStart\n");
    }
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
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    char buff[1024];
    int active = -1;
    int bytes;

    while (!quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Wait for instrument panel to poll (non-blocking, 1 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, buff, 1024, 0, (SOCKADDR*)&senderAddr, &addrSize);
            if (bytes > 0) {
                if (active != 1) {
                    printf("Instrument panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                    active = 1;
                }

                // Send latest data to the client that polled us
                bytes = sendto(sockfd, (char *)pSimVars, sizeof(SimVars), 0, (SOCKADDR*)&senderAddr, addrSize);
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

void cleanUp()
{
    printf("Disconnecting from MS FS2020\n");
    if (hSimConnect) {
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
    while (!quit)
    {
        if (!connected) {
            SimConnect_Open(&hSimConnect, "Request Data", NULL, 0, 0, 0);

            if (hSimConnect) {
                printf("\nConnected");
                addDataDefs();
                subscribeEvents();
                connected = true;
            }
        }

        if (connected) {
            SimConnect_CallDispatch(hSimConnect, MyDispatchProcRD, NULL);
        }

        Sleep(10);
    }

    cleanUp();
    return 0;
}
