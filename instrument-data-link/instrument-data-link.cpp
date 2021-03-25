/*
 * Flight Simulator Instrument Data Link
 * Copyright (c) 2021 Scott Vincent
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include "simvarDefs.h"
#include "SimConnect.h"

 // Data will be served on this port
const int Port = 52020;

// Some SimConnect events don't work with certain aircraft
// so use vJoy to simulate joystick button preses instead.
// To use vJoy you must install the device driver from here:
//    http://vjoystick.sourceforge.net/site/index.php/download-a-install/download
//
// Comment the following line out if you don't want to use vJoy.
#define vJoyFallback

#ifdef vJoyFallback
#include "..\vJoy_SDK\inc\public.h"
#include "..\vJoy_SDK\inc\vjoyinterface.h"

const char *VJOY_CONFIG_EXE = "C:\\Program Files\\vJoy\\x64\\vJoyConf.exe (Run as Admin)";

void vJoyInit();
void vJoyButtonPress(int button);

int vJoyDeviceId = 1;
bool vJoyInitialised = false;
int vJoyConfiguredButtons;
#endif

// SimConnect doesn't currently support readuing local (lvar)
// variables (params for 3rd party aircraft) but Jetbridge
// allows us to do this.
// To use jetbridge you must copy the Redist\a32nx-jetbridge
// package to your FS2020 Community folder. Source available from:
//    https://github.com/theomessin/jetbridge
//
// Comment the following line out if you don't want to use Jetbridge.
#define jetbridgeFallback

#ifdef jetbridgeFallback
#include "jetbridge\client.h"

const char* JETBRIDGE_APU_MASTER_SW = "L:A32NX_OVHD_APU_MASTER_SW_PB_IS_ON, bool";
const int JETBRIDGE_APU_MASTER_SW_LEN = 41;
const char* JETBRIDGE_APU_START = "L:A32NX_OVHD_APU_START_PB_IS_ON, bool";
const int JETBRIDGE_APU_START_LEN = 37;
const char* JETBRIDGE_APU_START_AVAIL = "L:A32NX_OVHD_APU_START_PB_IS_AVAILABLE, bool";
const int JETBRIDGE_APU_START_AVAIL_LEN = 44;
const char* JETBRIDGE_APU_BLEED = "L:A32NX_OVHD_PNEU_APU_BLEED_PB_IS_ON, bool";
const int JETBRIDGE_APU_BLEED_LEN = 42;

jetbridge::Client* jetbridgeClient = 0;
#endif

bool quit = false;
HANDLE hSimConnect = NULL;
extern const char* versionString;
extern const char* SimVarDefs[][2];
extern WriteEvent WriteEvents[];

SimVars simVars;
double *varStart = (double *)&simVars + 1;
int varSize = 0;

// Some panels request less data to save bandwidth
long writeDataSize = sizeof(WriteData);
long instrumentsDataSize = sizeof(SimVars);
long autopilotDataSize = (long)((LONG_PTR)(&simVars.autothrottleActive) + (long)sizeof(double) - (LONG_PTR)&simVars);
long radioDataSize = (long)((LONG_PTR)(&simVars.transponderCode) + (long)sizeof(double) - (LONG_PTR)&simVars);
long lightsDataSize = (long)((LONG_PTR)(&simVars.apuPercentRpm) + (long)sizeof(double) - (LONG_PTR)&simVars);

int active = -1;
int bytes;
bool autopilotPanelConnected = false;
bool radioPanelConnected = false;
bool lightsPanelConnected = false;
SOCKET sockfd;
sockaddr_in senderAddr;
int addrSize = sizeof(senderAddr);

struct {
    long requestedSize;
    WriteData writeData;
} recvBuffer;

// Create server thread
void server();
std::thread serverThread(server);

enum DEFINITION_ID {
    DEF_READ_ALL
};

enum REQUEST_ID {
    REQ_ID
};

#ifdef jetbridgeFallback
void readJetbridgeVar(const char *var)
{
    char rpnCode[128];
    sprintf_s(rpnCode, "(%s)", var);
    jetbridgeClient->request(rpnCode);
}

void writeJetbridgeVar(const char* var, double val)
{
    // FS2020 uses RPN (Reverse Polish Notation).
    char rpnCode[128];
    sprintf_s(rpnCode, "%f (>%s)", val, var);
    jetbridgeClient->request(rpnCode);
}

void updateVarFromJetbridge(const char* data)
{
    if (strncmp(&data[1], JETBRIDGE_APU_MASTER_SW, JETBRIDGE_APU_MASTER_SW_LEN) == 0) {
        simVars.apuMasterSw = atof(&data[JETBRIDGE_APU_MASTER_SW_LEN] + 2);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_START, JETBRIDGE_APU_START_LEN) == 0) {
        simVars.apuStart = atof(&data[JETBRIDGE_APU_START_LEN] + 2);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_START_AVAIL, JETBRIDGE_APU_START_AVAIL_LEN) == 0) {
        simVars.apuStartAvail = atof(&data[JETBRIDGE_APU_START_AVAIL_LEN] + 2);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_BLEED, JETBRIDGE_APU_BLEED_LEN) == 0) {
        simVars.apuBleed = atof(&data[JETBRIDGE_APU_BLEED_LEN] + 2);
    }
}

void jetbridgeButtonPress(int eventId, double value)
{
    switch (eventId) {
    case KEY_APU_OFF_SWITCH:
        writeJetbridgeVar(JETBRIDGE_APU_MASTER_SW, value);
        break;
    case KEY_APU_STARTER:
        writeJetbridgeVar(JETBRIDGE_APU_START, value);
        break;
    case KEY_BLEED_AIR_SOURCE_CONTROL_SET:
        writeJetbridgeVar(JETBRIDGE_APU_BLEED, value);
        break;
    }
}

void pollJetbridge()
{
    // Use low frequency for jetbridge as vars not critical
    int loopMillis = 500;

    while (!quit)
    {
        if (simVars.connected) {
            readJetbridgeVar(JETBRIDGE_APU_MASTER_SW);
            readJetbridgeVar(JETBRIDGE_APU_START);
            readJetbridgeVar(JETBRIDGE_APU_START_AVAIL);
            readJetbridgeVar(JETBRIDGE_APU_BLEED);
        }

        Sleep(loopMillis);
    }
}
#endif

void CALLBACK MyDispatchProc(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    static int displayDelay = 0;

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
        auto pObjData = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(pData);

        switch (pObjData->dwRequestID)
        {
        case REQ_ID:
        {
            memcpy(varStart, &pObjData->dwData, varSize);
            //if (displayDelay > 0) {
            //    displayDelay--;
            //}
            //else {
            //    //printf("Aircraft: %s   Cruise Speed: %f\n", simVars.aircraft, simVars.cruiseSpeed);
            //}
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

#ifdef jetbridgeFallback
    case SIMCONNECT_RECV_ID_CLIENT_DATA: {
        auto pClientData = static_cast<SIMCONNECT_RECV_CLIENT_DATA*>(pData);

        if (pClientData->dwRequestID == jetbridge::kDownlinkRequest) {
            auto packet = static_cast<jetbridge::Packet*>((jetbridge::Packet*)&pClientData->dwData);
            updateVarFromJetbridge(packet->data);
        }
        break;
    }
#endif

    case SIMCONNECT_RECV_ID_QUIT:
    {
        // Comment out next line to stay running when FS2020 quits
        //quit = true;
        break;
    }
    }
}

void addReadDefs()
{
    for (int i = 0;; i++) {
        if (SimVarDefs[i][0] == NULL) {
            break;
        }

        if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
            // Add string
            SIMCONNECT_DATATYPE dataType = SIMCONNECT_DATATYPE_STRING256;
            int dataLen = 256;

            if (strcmp(SimVarDefs[i][1], "string8") == 0) {
                dataType = SIMCONNECT_DATATYPE_STRING8;
                dataLen = 8;
            }
            else if (strcmp(SimVarDefs[i][1], "string32") == 0) {
                dataType = SIMCONNECT_DATATYPE_STRING32;
                dataLen = 32;
            }
            else if (strcmp(SimVarDefs[i][1], "string64") == 0) {
                dataType = SIMCONNECT_DATATYPE_STRING64;
                dataLen = 64;
            }

            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], NULL, dataType) < 0) {
                printf("Data def failed: %s (string)\n", SimVarDefs[i][0]);
            }
            else {
                varSize += dataLen;
            }
        }
        else if (_stricmp(SimVarDefs[i][1], "jetbridge") == 0) {
            // SimConnect variables start after all Jetbridge variables
            varStart++;
        }
        else {
            // Add double (float64)
            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], SimVarDefs[i][1]) < 0) {
                printf("Data def failed: %s, %s\n", SimVarDefs[i][0], SimVarDefs[i][1]);
            }
            else {
                varSize += sizeof(double);
            }
        }
    }
}

void mapEvents()
{
    for (int i = 0;; i++) {
        if (WriteEvents[i].name == NULL) {
            break;
        }

        if (SimConnect_MapClientEventToSimEvent(hSimConnect, WriteEvents[i].id, WriteEvents[i].name) != 0) {
            printf("Map event failed: %s\n", WriteEvents[i].name);
        }
    }
}

void init()
{
    addReadDefs();
    mapEvents();

    // Start requesting data
    if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME, 0, 0, 0, 0) < 0) {
        printf("Failed to start requesting data\n");
    }

#ifdef jetbridgeFallback
    if (jetbridgeClient != 0) {
        delete jetbridgeClient;
    }
    jetbridgeClient = new jetbridge::Client(hSimConnect);
#endif
}

void cleanUp()
{
    if (hSimConnect) {
        if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_NEVER, 0, 0, 0, 0) < 0) {
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
    printf("Instrument Data Link %s Copyright (c) 2021 Scott Vincent\n", versionString);

    // Yield so server can start
    Sleep(100);

    printf("Searching for local MS FS2020...\n");
    simVars.connected = 0;

#ifdef jetbridgeFallback
    std::thread jetbridgeThread(pollJetbridge);
#endif

    HRESULT result;

    int loopMillis = 10;
    int retryDelay = 0;
    while (!quit)
    {
        if (simVars.connected) {
            result = SimConnect_CallDispatch(hSimConnect, MyDispatchProc, NULL);
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
                init();
                simVars.connected = 1;
            }
            else {
                retryDelay = 200;
            }
        }

        Sleep(loopMillis);
    }

#ifdef jetbridgeFallback
    // Wait for thread to exit
    jetbridgeThread.join();
#endif

    cleanUp();
    return 0;
}

void processRequest()
{
    if (recvBuffer.requestedSize == writeDataSize) {
        // This is a write
        if (!simVars.connected) {
            return;
        }

        if (recvBuffer.writeData.eventId >= VJOY_BUTTONS && recvBuffer.writeData.eventId <= VJOY_BUTTONS_END) {
#ifdef vJoyFallback
            if (!vJoyInitialised) {
                vJoyInit();
            }
            vJoyButtonPress(recvBuffer.writeData.eventId);
#else
            printf("vJoy button event ignored - vJoyFallback is not enabled\n");
#endif
            return;
        }

#ifdef jetbridgeFallback
        jetbridgeButtonPress(recvBuffer.writeData.eventId, recvBuffer.writeData.value);
#endif

        if (SimConnect_TransmitClientEvent(hSimConnect, 0, recvBuffer.writeData.eventId, (DWORD)recvBuffer.writeData.value, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
            printf("Failed to transmit event: %d\n", recvBuffer.writeData.eventId);
        }

        if (recvBuffer.writeData.eventId == KEY_COM1_STBY_RADIO_SET || recvBuffer.writeData.eventId == KEY_COM2_STBY_RADIO_SET) {
            // Check for extra .5 on value
            int val = (int)(recvBuffer.writeData.value * 10 + 0.01);
            if (val % 10 >= 4) {
                if (recvBuffer.writeData.eventId == KEY_COM1_STBY_RADIO_SET) {
                    recvBuffer.writeData.eventId = KEY_COM1_RADIO_FRACT_INC;
                }
                else {
                    recvBuffer.writeData.eventId = KEY_COM2_RADIO_FRACT_INC;
                }
                if (SimConnect_TransmitClientEvent(hSimConnect, 0, recvBuffer.writeData.eventId, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
                    printf("Failed to transmit event: %d\n", recvBuffer.writeData.eventId);
                }
            }
        }
    }
    else if (recvBuffer.requestedSize == instrumentsDataSize) {
        // Send instrument data to the client that polled us
        bytes = sendto(sockfd, (char*)&simVars, instrumentsDataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
        if (active != 1) {
            printf("Instrument panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
            active = 1;
        }
    }
    else if (recvBuffer.requestedSize == autopilotDataSize) {
        // Send autopilot data to the client that polled us
        bytes = sendto(sockfd, (char*)&simVars, autopilotDataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
        if (!autopilotPanelConnected) {
            printf("Autopilot panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
            autopilotPanelConnected = true;
        }
    }
    else if (recvBuffer.requestedSize == radioDataSize) {
        // Send radio data to the client that polled us
        bytes = sendto(sockfd, (char*)&simVars, radioDataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
        if (!radioPanelConnected) {
            printf("Radio panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
            radioPanelConnected = true;
        }
    }
    else if (recvBuffer.requestedSize == lightsDataSize) {
        // Send power/lights data to the client that polled us
        bytes = sendto(sockfd, (char*)&simVars, lightsDataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
        if (!lightsPanelConnected) {
            printf("Power/Lights panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
            lightsPanelConnected = true;
        }
    }
    else {
        // Data size mismatch
        bytes = sendto(sockfd, (char*)&instrumentsDataSize, sizeof(long), 0, (SOCKADDR*)&senderAddr, addrSize);
        printf("Client at %s requested %ld bytes instead of %ld bytes\n",
            inet_ntoa(senderAddr.sin_addr), recvBuffer.requestedSize, instrumentsDataSize);
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

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    while (!quit) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Wait for instrument panel to poll (non-blocking, 1 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, (char*)&recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&senderAddr, &addrSize);
            if (bytes > 0) {
                processRequest();
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

#ifdef vJoyFallback
void vJoyInit()
{
    printf("Initialising vJoy Interface...\n");

    if (!vJoyEnabled())
    {
        printf("Failed - Make sure that vJoy is installed and enabled\n");
        return;
    }

    // Get the status of the vJoy device before trying to acquire it
    VjdStat status = GetVJDStatus(vJoyDeviceId);

    switch (status)
    {
    case VJD_STAT_BUSY:
        printf("Failed - vJoy device %d is already owned by another program", vJoyDeviceId);
        return;
    case VJD_STAT_MISS:
        printf("Failed - vJoy device %d is not installed or disabled. Run %s to configure it.\n", vJoyDeviceId, VJOY_CONFIG_EXE);
        return;
    case VJD_STAT_OWN:
        printf("vJoy device %d is already owned by this program\n", vJoyDeviceId);
        break;
    case VJD_STAT_FREE:
        // printf("vJoy device %d is available\n", vJoyDeviceId);
        break;
    default:
        printf("Failed - vJoy device %d general error\n", vJoyDeviceId);
        return;
    };

    // Acquire the vJoy device
    if (!AcquireVJD(vJoyDeviceId))
    {
        printf("Failed - Cannot acquire vJoy device %d\n", vJoyDeviceId);
        return;
    }

    // Get the number of buttons that are configured for this joystick
    vJoyConfiguredButtons = GetVJDButtonNumber(vJoyDeviceId);
    int dataLinkConfiguredButtons = (VJOY_BUTTONS_END - 1) - VJOY_BUTTONS;
    if (vJoyConfiguredButtons < dataLinkConfiguredButtons) {
        printf("WARNING - Data link has %d vJoy buttons configured but vJoy device %d only has %d buttons configured. Run %s to configure more buttons.\n",
            dataLinkConfiguredButtons, vJoyDeviceId, vJoyConfiguredButtons, VJOY_CONFIG_EXE);
    }

    printf("Success - Acquired vJoy device %d\n", vJoyDeviceId);

    ResetButtons(vJoyDeviceId);
    vJoyInitialised = true;
}

void vJoyButtonPress(int eventId)
{
    if (eventId == VJOY_BUTTONS || eventId == VJOY_BUTTONS_END) {
        printf("Dummy vJoy button event VJOY_BUTTONS/VJOY_BUTTONS_END gnored\n");
        return;
    }

    int button = eventId - VJOY_BUTTONS;

    if (!vJoyInitialised) {
        printf("Ignored vJoy button %d event - vJoy is not initialised\n", button);
        return;
    }

    if (button > vJoyConfiguredButtons) {
        printf("Ignored vJoy button %d event - vJoy device %d does not have that many buttons configured. Run %s to configure more.\n",
            button, vJoyDeviceId, VJOY_CONFIG_EXE);
        return;
    }

    // Press and release joystick button
    SetBtn(true, vJoyDeviceId, button);
    Sleep(40);
    SetBtn(false, vJoyDeviceId, button);
}
#endif // vJoyFallback
