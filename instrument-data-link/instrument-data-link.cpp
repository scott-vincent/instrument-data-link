/*
 * Flight Simulator Instrument Data Link
 * Copyright (c) 2024 Scott Vincent
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include "simvarDefs.h"
#include "LVars-A310.h"
#include "LVars-Fbw.h"
#include "LVars-Kodiak100.h"
#include "jetbridge.h"
#include "vjoy.h"
#include "SimConnect.h"

 // Data will be served on this port
const int Port = 52020;

// Change the next line to false if you always want to send
// full data across the network rather than deltas.
const bool UseDeltas = true;
const int MaxDataSize = 8192;

// Uncomment the next line to show network data usage.
// This should be a lot lower when using deltas.
//#define SHOW_NETWORK_USAGE

#ifdef SHOW_NETWORK_USAGE
ULONGLONG networkStart = 0;
long networkIn;
long networkOut;
#endif

// Comment the following line out if you don't have any Raspberry Pi Pico USB devices
#define PICO_USB

#ifdef PICO_USB
#include <Mmsystem.h>
#include <joystickapi.h>

void picoInit();
void picoRefresh();
void switchboxRefresh();
void g1000Refresh();
void g1000Encoder(int, int, int);
void g1000EncoderPush(int);
void g1000SoftkeyPush(int);
void g1000ButtonPush(int);

int switchboxId = -1;
int g1000Id = -1;
int joystickRetry = 5;
bool switchboxZeroed = false;
double switchboxZeroPos[4];
bool g1000Zeroed = false;
double g1000ZeroPos[3];
bool modeChange = false;
double g1000EncoderVal[3];
int g1000ButtonVal[20];
time_t clrPress = 0;
JOYCAPSA joyCaps;
JOYINFOEX joyInfo;
HWND pfdWin = NULL;
HWND mfdWin = NULL;
WINDOWINFO pfdInfo;
WINDOWINFO mfdInfo;
bool g1000IsPrimary = false;
#endif

enum FLIGHT_PHASE {
    GROUND,
    TAKEOFF,
    CLIMB,
    CRUISE,
    DESCENT,
    APPROACH,
    GO_AROUND
};

bool quit = false;
bool initiatedPushback = false;
bool completedTakeOff = false;
bool hasFlown = false;
int onStandState = 0;
double skytrackState = 0;
bool isA310 = false;
bool isFbw = false;
bool isA320 = false;
bool isA380 = false;
bool is747 = false;
bool isK100 = false;
bool isPA28 = false;
bool isAirliner = false;
bool isNewAircraft = false;
char prevAircraft[32] = "\0";
double lastHeading = 0;
int seatBeltsReplicateDelay = 0;
int fixedPushback = -1;
LVars_A310 a310Vars;
LVars_FBW fbwVars;
HANDLE hSimConnect = NULL;
extern const char* versionString;
extern const char* SimVarDefs[][2];
extern WriteEvent WriteEvents[];

SimVars simVars;
double *varsStart;
int varsSize;

// Some panels request less data to save bandwidth
long writeDataSize = sizeof(WriteData);
long instrumentsDataSize = sizeof(SimVars);
long autopilotDataSize = (long)((LONG_PTR)(&simVars.autothrottleActive) + (long)sizeof(double) - (LONG_PTR)&simVars);
long radioDataSize = (long)((LONG_PTR)(&simVars.transponderCode) + (long)sizeof(double) - (LONG_PTR)&simVars);
long lightsDataSize = (long)((LONG_PTR)(&simVars.apuPercentRpm) + (long)sizeof(double) - (LONG_PTR)&simVars);

char* deltaData;
long deltaSize;
char* prevInstrumentsData;
char* prevAutopilotData;
char* prevRadioData;
char* prevLightsData;

int active = -1;
int bytes;
bool autopilotPanelConnected = false;
bool radioPanelConnected = false;
bool lightsPanelConnected = false;
SOCKET sockfd;
sockaddr_in senderAddr;
int addrSize = sizeof(senderAddr);
Request request;
SOCKET posSockfd;
sockaddr_in posSendAddr;
PosData posData;
int posDataSize = sizeof(PosData);
int posSkip = 0;
const int deltaDoubleSize = sizeof(DeltaDouble);
const int deltaStringSize = sizeof(DeltaString);

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
void pollJetbridge()
{
    // Use low frequency for jetbridge as vars not critical
    int loopMillis = 100;

    while (!quit)
    {
        if (simVars.connected && isA310) {
            readA310Jetbridge();
            Sleep(loopMillis);
        }
        else if (simVars.connected && isFbw) {
            readFbwJetbridge();
            Sleep(loopMillis);
        }
        else {
            Sleep(500);
        }
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
            printf("SimConnect Start event\n");
            fflush(stdout);
            break;
        }

        case SIM_STOP:
        {
            printf("SimConnect Stop event\n");
            fflush(stdout);
            break;
        }

        default:
        {
            printf("SimConnect unknown event id: %ld\n", evt->uEventID);
            fflush(stdout);
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
            int dataSize = pObjData->dwSize - ((int)(&pObjData->dwData) - (int)pData);
            if (dataSize != varsSize) {
                printf("Error: SimConnect expected %d bytes but received %d bytes\n", varsSize, dataSize);
                fflush(stdout);
            }
            else {
                memcpy(varsStart, &pObjData->dwData, varsSize);
            }

            // Populate internal variables
            simVars.skytrackState = skytrackState;
            isA310 = false;
            isFbw = false;
            isA320 = false;
            isA380 = false;
            is747 = false;
            isK100 = false;
            isPA28 = false;
            isAirliner = false;
            isNewAircraft = false;

            if (strcmp(simVars.aircraft, prevAircraft) != 0) {
                strcpy(prevAircraft, simVars.aircraft);
                isNewAircraft = true;
            }

            char* pos = strchr(simVars.aircraft, '3');
            if (pos && *(pos - 1) == 'A') {
                if (*(pos + 1) == '1') {
                    isA310 = true;
                    isAirliner = true;
                }
                else if (*(pos + 1) == '2') {
                    isFbw = true;
                    isA320 = true;
                    isAirliner = true;
                }
                else if (*(pos + 1) == '8') {
                    isFbw = true;
                    isA380 = true;
                    isAirliner = true;
                }
            }

            if (strncmp(simVars.aircraft, "Salty", 5) == 0 || strncmp(simVars.aircraft, "Boeing 747-8", 12) == 0) {
                is747 = true;
                isAirliner = true;
            }
            else if (strncmp(simVars.aircraft, "Kodiak 100", 10) == 0) {
                isK100 = true;
            }
            else if (strncmp(simVars.aircraft, "Just Flight PA28", 16) == 0) {
                isPA28 = true;
            }
            else if (strncmp(simVars.aircraft, "Airbus", 6) == 0 || strncmp(simVars.aircraft, "Boeing", 6) == 0) {
                isAirliner = true;
            }

            if (abs(simVars.hiHeading - lastHeading) > 10) {
                // Fix gyro if aircraft heading changes abruptly
                //SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_HEADING_GYRO_SET, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                writeJetbridgeVar(KEY_HEADING_GYRO_SET, 1);
            }
            lastHeading = simVars.hiHeading;

            if (simVars.connected && isA310) {
                // Map A310 vars to real vars
                simVars.apuStartSwitch = a310Vars.apuStart;
                if (a310Vars.apuStartAvail) {
                    simVars.apuPercentRpm = 100;
                }
                else {
                    simVars.apuPercentRpm = 0;
                }

                if (seatBeltsReplicateDelay > 0) {
                    seatBeltsReplicateDelay--;
                }
                else if (simVars.seatBeltsSwitch != a310Vars.seatbeltsSwitch) {
                    // Replicate lvar value back to standard SDK variable to make PACX work correctly
                    //SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                    writeJetbridgeVar(KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE, 1);
                    seatBeltsReplicateDelay = 10;
                }
                simVars.seatBeltsSwitch = a310Vars.seatbeltsSwitch;
                simVars.jbPitchTrim = a310Vars.pitchTrim1 + a310Vars.pitchTrim2;
                simVars.autopilotAirspeed = a310Vars.autopilotAirspeed;
                simVars.autopilotMach = a310Vars.autopilotAirspeed;
                simVars.autopilotHeading = a310Vars.autopilotHeading;
                simVars.autopilotAltitude = a310Vars.autopilotAltitude;
                simVars.autopilotVerticalSpeed = a310Vars.autopilotVerticalSpeed;
                simVars.tfAutoBrake = simVars.jbAutobrake + 1;
                simVars.flightDirectorActive = a310Vars.flightDirector;
                simVars.autopilotEngaged = a310Vars.autopilot;
                simVars.autothrottleActive = a310Vars.autothrottle;
                simVars.autopilotApproachHold = a310Vars.localiser;
                simVars.autopilotGlideslopeHold = a310Vars.approach;
                if (a310Vars.profile) {
                    simVars.jbManagedSpeed = 1;
                }
                else {
                    simVars.jbManagedSpeed = 0;
                }
                if (a310Vars.altHold || a310Vars.levelChange || a310Vars.profile) {
                    simVars.jbManagedAltitude = 0;  // For A310, managedAltitude == Selected VS
                }
                else {
                    simVars.jbManagedAltitude = 1;  // For A310, managedAltitude == Selected VS
                }
                if (a310Vars.gearHandle == 0 && simVars.gearLeftPos == 0 && simVars.gearCentrePos == 0 && simVars.gearRightPos == 0) {
                    // After gear up set handle to neutral position
                    writeJetbridgeVar(A310_GEAR_HANDLE, 1);
                }
                simVars.nav1Freq = a310Vars.ilsFrequency / 100;
                simVars.nav1Standby = a310Vars.ilsFrequency / 100;
                simVars.vor1Obs = a310Vars.ilsCourse;
            }
            else if (simVars.connected && isFbw) {
                // Map FBW vars to real vars
                simVars.apuStartSwitch = fbwVars.apuStart;
                if (fbwVars.apuStartAvail) {
                    simVars.apuPercentRpm = 100;
                }
                else {
                    simVars.apuPercentRpm = 0;
                }
                simVars.tfFlapsIndex = fbwVars.flapsIndex;
                simVars.parkingBrakeOn = fbwVars.parkBrakePos;
                simVars.tfSpoilersPosition = fbwVars.spoilersHandlePos;
                simVars.brakeLeftPedal = fbwVars.leftBrakePedal;
                simVars.brakeRightPedal = fbwVars.rightBrakePedal;
                simVars.rudderPosition = fbwVars.rudderPedalPos / 100.0;
                simVars.autopilotEngaged = (fbwVars.autopilot1 == 0 && fbwVars.autopilot2 == 0) ? 0 : 1;
                if (fbwVars.autothrust == 0) {
                    simVars.autothrottleActive = 0;
                }
                else {
                    simVars.autothrottleActive = 1;
                }
                simVars.transponderState = fbwVars.xpndrMode;
                simVars.autopilotHeading = fbwVars.autopilotHeading;
                simVars.autopilotAltitude = simVars.autopilotAltitude3;
                simVars.autopilotVerticalSpeed = fbwVars.autopilotVerticalSpeed;
                if (simVars.jbVerticalMode == 14) {
                    // V/S mode engaged
                    simVars.autopilotVerticalHold = 1;
                }
                else if (simVars.jbVerticalMode == 15) {
                    // FPA mode engaged
                    simVars.autopilotVerticalHold = -1;
                    simVars.autopilotVerticalSpeed = fbwVars.autopilotFpa;
                }
                else {
                    simVars.autopilotVerticalHold = 0;
                }
                simVars.autopilotApproachHold = simVars.jbLocMode;
                simVars.autopilotGlideslopeHold = simVars.jbApprMode;
                simVars.tfAutoBrake = simVars.jbAutobrake + 1;
                simVars.exhaustGasTemp1 = fbwVars.engineEgt1;
                simVars.exhaustGasTemp2 = fbwVars.engineEgt2;
                simVars.engineFuelFlow1 = fbwVars.engineFuelFlow1;
                simVars.engineFuelFlow2 = fbwVars.engineFuelFlow2;
            }
            else if (is747) {
                // Map Salty 747 vars to real vars
                simVars.autopilotAltitude = simVars.autopilotAltitude3;

                // Slot index 1 = Selected, 2 = Managed
                simVars.autopilotHeadingLock = simVars.autopilotHeadingSlotIndex == 1;
                simVars.autopilotVerticalHold = simVars.autopilotVsSlotIndex == 1;

                // B747 Bug - Fix initial autopilot altitude
                if (simVars.altAboveGround < 50 && simVars.autopilotAltitude > 49900) {
                    // Set autopilot altitude to 5000
                    //SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_AP_ALT_VAR_SET_ENGLISH, 5000, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                    writeJetbridgeVar(KEY_AP_ALT_VAR_SET_ENGLISH, 5000);
                }

                // B747 Bug - Fix master battery
                if (simVars.batteryLoad > 1.2) {
                    if (simVars.elecBat1 == 0) {
                        simVars.elecBat1 = 1;
                        simVars.elecBat2 = 1;
                        printf("Batteries on, load: %f\n", simVars.batteryLoad);
                        fflush(stdout);
                    }
                }
                else if (simVars.batteryLoad > 0) {
                    // Ignore fake load 0.0 setting
                    if (simVars.elecBat1 == 1) {
                        simVars.elecBat1 = 0;
                        simVars.elecBat2 = 0;
                        printf("Batteries off, load: %f\n", simVars.batteryLoad);
                        fflush(stdout);
                    }
                }
            }
            else {
                simVars.elecBat1 = simVars.dcVolts > 0;
            }

            if (simVars.connected && isAirliner && simVars.engineFuelFlow1 > 0 && simVars.suctionPressure < 0.001) {
                // Stop Annunciator reporting a vacuum fault on airliners
                simVars.suctionPressure = 5;
            }

            if (simVars.altAboveGround > 50) {
                hasFlown = true;
                simVars.landingRate = -999;

                if (initiatedPushback) {
                    // Reset ground state
                    initiatedPushback = false;
                    onStandState = 0;
                }

                if (!completedTakeOff && simVars.altAltitude > 10000) {
                    completedTakeOff = true;
                }
            }
            else if (completedTakeOff && simVars.elecBat1 == 0) {
                printf("Reset flight (Battery off)\n");
                fflush(stdout);
                completedTakeOff = false;
            }
#ifdef PICO_USB
            // Populate simvars for Pico USB devices
            picoRefresh();

            if (isNewAircraft) {
                simVars.sbMode = 0;     // Default to autopilot on switchbox
            }
#endif

            if (fixedPushback != -1) {
                // Pushback goes wrong sometimes (pushbackState == 4)
                fixedPushback++;
                if (fixedPushback == 20) {
                    if (simVars.pushbackState < 4) {
                        fixedPushback = -1;
                    }
                    else {
                        printf("Extra start pushback\n");
                        //SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_TOGGLE_PUSHBACK, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                        writeJetbridgeVar(KEY_TOGGLE_PUSHBACK, 0);
                    }
                }
                else if (fixedPushback == 40) {
                    fixedPushback = -1;
                    if (simVars.pushbackState < 3) {
                        printf("Extra stop pushback\n");
                        //SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_TOGGLE_PUSHBACK, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                        writeJetbridgeVar(KEY_TOGGLE_PUSHBACK, 0);
                    }
                }
            }

            if (!simVars.connected || simVars.elecBat1 == 0) {
                hasFlown = false;
                simVars.landingRate = -999;
            }

            // Record landing rate. TouchdownVs isn't accurate so use actual VS instead.
            if (hasFlown && simVars.onGround && simVars.landingRate == -999) {
                simVars.landingRate = abs(simVars.vsiVerticalSpeed);
                if (simVars.landingRate != 0) {
                    printf("Landing Rate: %d FPM\n", (int)((simVars.landingRate * 60) + 0.5));
                }
            }

            //// For testing only - Leave commented out
            //if (displayDelay > 0) {
            //    displayDelay--;
            //}
            //else {
            //    //printf("Aircraft: %s   Cruise Speed: %f\n", simVars.aircraft, simVars.cruiseSpeed);
            //    displayDelay = 60;
            //}

            break;
        }
        default:
        {
            printf("SimConnect unknown request id: %ld\n", pObjData->dwRequestID);
            fflush(stdout);
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
            if (isA310) {
                updateA310FromJetbridge(packet->data);
            }
            else if (isFbw) {
                updateFbwFromJetbridge(packet->data);
            }
        }
        break;
    }
#endif

    case SIMCONNECT_RECV_ID_QUIT:
    {
        // Comment out next line to stay running when FS2020 quits
        //quit = true;
        printf("SimConnect Quit\n");
        fflush(stdout);
        break;
    }
    }
}

void addReadDefs()
{
    varsStart = (double*)&simVars + 1;
    varsSize = 0;
    bool foundInternal = false;

    for (int i = 0;; i++) {
        if (SimVarDefs[i][0] == NULL) {
            break;
        }

        if (_stricmp(SimVarDefs[i][1], "internal") == 0) {
            foundInternal = true;
        }
        else if (foundInternal) {
            printf("ERROR: Internal variables must come last. Cannot add: %s\n", SimVarDefs[i][0]);
        }
        else if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
            // Add string
            SIMCONNECT_DATATYPE dataType;
            int dataLen;

            if (strcmp(SimVarDefs[i][1], "string32") == 0) {
                dataType = SIMCONNECT_DATATYPE_STRING32;
                dataLen = 32;
            }
            else {
                printf("Unsupported string type: %s\n", SimVarDefs[i][1]);
                dataType = SIMCONNECT_DATATYPE_STRING32;
                dataLen = 32;
            }

            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], NULL, dataType) != 0) {
                printf("Data def failed: %s (string)\n", SimVarDefs[i][0]);
            }
            else {
                varsSize += dataLen;
            }
        }
        else if (_stricmp(SimVarDefs[i][1], "jetbridge") == 0) {
            // SimConnect variables start after all Jetbridge variables
            varsStart++;
        }
        else {
            // Add double (float64)
            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], SimVarDefs[i][1]) != 0) {
                printf("Data def failed: %s, %s\n", SimVarDefs[i][0], SimVarDefs[i][1]);
            }
            else {
                varsSize += sizeof(double);
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
    if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME, 0, 0, 0, 0) != 0) {
        printf("Failed to start requesting data\n");
    }

#ifdef jetbridgeFallback
    jetbridgeInit(hSimConnect);
#endif
}

void cleanUp()
{
    if (hSimConnect) {
        if (SimConnect_RequestDataOnSimObject(hSimConnect, REQ_ID, DEF_READ_ALL, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_NEVER, 0, 0, 0, 0) != 0) {
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
    printf("Instrument Data Link %s Copyright (c) 2024 Scott Vincent\n", versionString);

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

void addDeltaDouble(long offset, double newVal)
{
    DeltaDouble deltaDouble;
    deltaDouble.offset = offset;
    deltaDouble.data = newVal;

    memcpy(deltaData + deltaSize, &deltaDouble, deltaDoubleSize);
    deltaSize += deltaDoubleSize;
}

void addDeltaString(long offset, char *newVal)
{
    DeltaString deltaString;
    deltaString.offset = 0x10000 | offset;  // Set high bit so we know it is a string
    strncpy(deltaString.data, newVal, 32);  // Only support string32

    memcpy(deltaData + deltaSize, &deltaString, deltaStringSize);
    deltaSize += deltaStringSize;
}

/// <summary>
/// Send the full set of data if this a new connection or we
/// don't want to use deltas.
/// </summary>
void sendFull(char* prevSimVars, long dataSize)
{
    bytes = sendto(sockfd, (char*)&simVars, dataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
#ifdef SHOW_NETWORK_USAGE
    networkOut += bytes;
#endif

    // Update prev data
    memcpy(prevSimVars, &simVars, dataSize);
}

/// <summary>
/// If this a new connection send all the data otherwise only send
/// the delta, i.e. data that has changed since we last sent it.
/// This should reduce network bandwidth usage hugely.
/// </summary>
void sendDelta(char* prevSimVars, long dataSize)
{
    // Initialise delta data
    deltaSize = 0;

    // Always send 'connected' var
    addDeltaDouble(0, simVars.connected);
    long offset = sizeof(double);

    // Add all vars that have changed to delta data
    for (int i = 0;; i++) {
        if (SimVarDefs[i][0] == NULL) {
            break;
        }

        char* oldVarPtr = prevSimVars + offset;
        char* newVarPtr = (char*)&simVars + offset;

        if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
            // Has string changed?
            if (strncmp(oldVarPtr, newVarPtr, 32) != 0) {
                addDeltaString(offset, newVarPtr);
                memcpy(oldVarPtr, newVarPtr, 32);
            }

            offset += 32;
        }
        else {
            // Has double changed?
            double* oldVar = (double*)oldVarPtr;
            double* newVar = (double*)newVarPtr;
            if (*oldVar != *newVar) {
                addDeltaDouble(offset, *newVar);
                memcpy(oldVarPtr, newVarPtr, sizeof(double));
            }

            offset += 8;
        }

        // Next variable
        if (offset >= dataSize) {
            break;
        }
    }

    if (deltaSize < dataSize) {
        // Send delta data
        bytes = sendto(sockfd, (char*)deltaData, deltaSize, 0, (SOCKADDR*)&senderAddr, addrSize);
    }
    else {
        // Send full data
        bytes = sendto(sockfd, (char*)&simVars, dataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
    }
#ifdef SHOW_NETWORK_USAGE
    networkOut += bytes;
#endif
}

/// <summary>
/// If an event button is pressed return either EVENT_NONE or the event (sound)
/// that should be played depending on current aircraft state.
/// </summary>
EVENT_ID getCustomEvent(int eventNum)
{
    EVENT_ID event = EVENT_NONE;
    bool isClimbing = simVars.vsiVerticalSpeed > 3;
    bool isDescending = simVars.vsiVerticalSpeed < -3;

    FLIGHT_PHASE phase = GROUND;
    if (simVars.altAboveGround > 50) {
        if (simVars.altAltitude < 10000) {
            if (!completedTakeOff) {
                phase = TAKEOFF;
            }
            else if (isClimbing) {
                phase = GO_AROUND;
            }
            else {
                phase = APPROACH;
            }
        }
        else if (isClimbing) {
            phase = CLIMB;
        }
        else if (isDescending) {
            phase = DESCENT;
        }
        else {
            phase = CRUISE;
        }
    }

    switch (eventNum) {
    case 1:
        // Event button 1 pressed
        switch (phase) {
            case GROUND:
                if (completedTakeOff) {
                    // Landed
                    if (simVars.parkingBrakeOn) {
                        // Arrived at stand
                        return EVENT_DOORS_FOR_DISEMBARK;
                    }
                    else {
                        // Taxi in
                        return EVENT_DOORS_TO_MANUAL;
                    }
                }
                else if (simVars.pushbackState < 3) {
                    // Pushing back
                    return EVENT_DOORS_TO_AUTO;
                }
                else if (initiatedPushback) {
                    // Completed pushback
                    return EVENT_SEATS_FOR_TAKEOFF;
                }
                else if (simVars.parkingBrakeOn) {
                    // Still on stand
                    onStandState++;
                    switch (onStandState) {
                    case 1:
                        return EVENT_DOORS_FOR_BOARDING;
                    case 2:
                        return EVENT_WELCOME_ON_BOARD;
                    case 3:
                        return EVENT_BOARDING_COMPLETE;
                    }
                }
                return EVENT_NONE;
            case TAKEOFF:
                return EVENT_NONE;
            case CLIMB:
                return EVENT_START_SERVING;
            case CRUISE:
                return EVENT_START_SERVING;
            case DESCENT:
                return EVENT_NONE;
            case APPROACH:
                if (simVars.altAboveGround > 4000) {
                    return EVENT_LANDING_PREPARE_CABIN;
                }
                else {
                    return EVENT_SEATS_FOR_LANDING;
                }
            case GO_AROUND:
                return EVENT_NONE;
        }

    case 2:
        // Event button 2 pressed
        switch (phase) {
        case GROUND:
            if (completedTakeOff) {
                // Landed
                if (simVars.parkingBrakeOn) {
                    // Arrived at stand
                    printf("Reset flight (Captain goodbye)\n");
                    fflush(stdout);
                    completedTakeOff = false;
                    return EVENT_DISEMBARK;
                }
                else {
                    // Taxi in
                    return EVENT_REMAIN_SEATED;
                }
            }
            else {
                return simVars.pushbackState < 3 ? EVENT_PUSHBACK_STOP : EVENT_PUSHBACK_START;
            }
        case TAKEOFF:
            // If not reached 10000ft but descending and button 2 pressed just assume short flight
            if (!completedTakeOff && isDescending) {
                completedTakeOff = true;
                return EVENT_FINAL_DESCENT;
            }
            return EVENT_NONE;
        case CLIMB:
            if (simVars.seatBeltsSwitch == 1) {
                return EVENT_TURBULENCE;
            }
            else {
                return EVENT_NONE;
            }
        case CRUISE:
            if (simVars.seatBeltsSwitch == 1) {
                return EVENT_TURBULENCE;
            }
            else {
                return EVENT_REACHED_CRUISE;
            }
        case DESCENT:
            if (simVars.seatBeltsSwitch == 1) {
                return EVENT_TURBULENCE;
            }
            else {
                return EVENT_REACHED_TOD;
            }
        case APPROACH:
            if (simVars.altAboveGround > 4000) {
                return EVENT_FINAL_DESCENT;
            }
        case GO_AROUND:
            return EVENT_GO_AROUND;
        }
    }

    return EVENT_NONE;
}

void processRequest(int bytes)
{
    //// For testing only - Leave commented out
    //if (request.requestedSize == writeDataSize) {
    //    printf("Received %d bytes from %s - Write Request event: %s\n", bytes, inet_ntoa(senderAddr.sin_addr), WriteEvents[request.writeData.eventId].name);
    //}
    //else {
    //    // To  test you can send from client with this command: echo - e '\x1\x0\x0\x0' | ncat -u 192.168.1.80 52020
    //    printf("Received %d bytes from %s - Requesting %d bytes\n", bytes, inet_ntoa(senderAddr.sin_addr), request.requestedSize);
    //}

    if (request.requestedSize == writeDataSize) {
         // This is a write
        if (request.writeData.eventId == KEY_ENG_CRANK) {
            if (isA310) {
                // 1 = Start A, 3 = Off
                int value = 3;
                if (request.writeData.value == 1) {
                    value = 1;
                }
                writeJetbridgeVar(A310_ENG_IGNITION, value);
            }
            return;
        }

        if (!simVars.connected) {
            return;
        }

        //// For testing only - Leave commented out
        //if (request.writeData.eventId == KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE) {
        //    request.writeData.eventId = KEY_FLAPS_INCR;
        //    request.writeData.value = 0;
        //    printf("Intercepted event - Changed to: %d = %f\n", request.writeData.eventId, request.writeData.value);
        //    printf("Flaps: %f\n", simVars.tfFlapsIndex);
        //    writeJetbridgeVar("K:FLAPS HANDLE INDEX, number", simVars.tfFlapsIndex + 1.0f);
        //}
        //else {
        //    printf("Unintercepted event: %d (%d) = %f\n", request.writeData.eventId, KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE, request.writeData.value);
        //}

        if (request.writeData.eventId >= VJOY_BUTTONS && request.writeData.eventId <= VJOY_BUTTONS_END) {
            // Override vJoy anti ice buttons for A310
            if (isA310) {
                if (request.writeData.eventId == VJOY_BUTTON_13) {
                    // Anti ice on
                    writeJetbridgeVar(A310_ENG1_ANTI_ICE, 1);
                    writeJetbridgeVar(A310_ENG2_ANTI_ICE, 1);
                    writeJetbridgeVar(A310_WING_ANTI_ICE, 1);
                    return;
                }
                else if (request.writeData.eventId == VJOY_BUTTON_12) {
                    // Anti ice off
                    writeJetbridgeVar(A310_ENG1_ANTI_ICE, 0);
                    writeJetbridgeVar(A310_ENG2_ANTI_ICE, 0);
                    writeJetbridgeVar(A310_WING_ANTI_ICE, 0);
                    return;
                }
            }

#ifdef vJoyFallback
            vJoyButtonPress(request.writeData.eventId);
#else
            printf("vJoy button event ignored - vJoyFallback is not enabled\n");
#endif
            return;
        }

#ifdef jetbridgeFallback
        if (isA310 && jetbridgeA310ButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
        else if (isFbw && jetbridgeFbwButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
        else if (isK100 && jetbridgeK100ButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
        else if (isPA28 && jetbridgePA28ButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
        else if (jetbridgeMiscButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
#endif

        // Process custom events
        if (request.writeData.eventId == KEY_CHECK_EVENT) {
            int eventNum = (int)(request.writeData.value);
            // Ignore event 1 in GA aircraft (button used for Engine Primer instead)
            if (eventNum == 1 && !isAirliner) {
                return;
            }
            else if (isA310 && a310Vars.engineIgnition < 2) {
                // If engine ignition is on then event keys start engines instead
                if (eventNum == 1) {
                    writeJetbridgeVar(A310_ENG1_STARTER, 1);
                }
                else {
                    writeJetbridgeVar(A310_ENG2_STARTER, 1);
                }
                return;
            }
            EVENT_ID event = getCustomEvent(eventNum);
            sendto(sockfd, (char*)&event, sizeof(int), 0, (SOCKADDR*)&senderAddr, addrSize);
            if (event == EVENT_PUSHBACK_START || event == EVENT_PUSHBACK_STOP) {
                // Don't return (need to trigger the pushback)
                request.writeData.eventId = KEY_TOGGLE_PUSHBACK;
                if (event == EVENT_PUSHBACK_START) {
                    initiatedPushback = true;
                    fixedPushback = -1;
                }
                else {
                    fixedPushback = 0;
                }
            }
            else {
                return;
            }
        }
        else if (request.writeData.eventId == KEY_SKYTRACK_STATE) {
            skytrackState = request.writeData.value;
            return;
        }

        if (request.writeData.eventId == EVENT_RESET_DRONE_FOV) {
            writeJetbridgeVar(DRONE_CAMERA_FOV, 50);
            return;
        }

        if (request.writeData.eventId == KEY_TOGGLE_RAMPTRUCK) {
            printf("Ramp truck requested\n");
        }

        //if (SimConnect_TransmitClientEvent(hSimConnect, 0, request.writeData.eventId, (DWORD)request.writeData.value, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
        //    printf("Failed to transmit event: %d\n", request.writeData.eventId);
        //}
        writeJetbridgeVar(request.writeData.eventId, request.writeData.value);
    }
    else if (request.requestedSize == instrumentsDataSize) {
        // Send instrument data to the client that polled us
        if (active != 1 || request.wantFullData || !UseDeltas) {
            if (active != 1) {
                printf("Instrument panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                active = 1;
            }
            sendFull(prevInstrumentsData, instrumentsDataSize);
        }
        else {
            sendDelta(prevInstrumentsData, instrumentsDataSize);
        }
    }
    else if (request.requestedSize == autopilotDataSize) {
        // Send autopilot data to the client that polled us
        if (!autopilotPanelConnected || request.wantFullData || !UseDeltas) {
            if (!autopilotPanelConnected) {
                printf("Autopilot panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                autopilotPanelConnected = true;
            }
            sendFull(prevAutopilotData, autopilotDataSize);
        }
        else {
            sendDelta(prevAutopilotData, autopilotDataSize);
        }
    }
    else if (request.requestedSize == radioDataSize) {
        // Send radio data to the client that polled us
        if (!radioPanelConnected || request.wantFullData || !UseDeltas) {
            if (!radioPanelConnected) {
                printf("Radio panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                radioPanelConnected = true;
            }
            sendFull(prevRadioData, radioDataSize);
        }
        else {
            sendDelta(prevRadioData, radioDataSize);
        }
    }
    else if (request.requestedSize == lightsDataSize) {
        // Send power/lights data to the client that polled us
        if (!lightsPanelConnected || request.wantFullData || !UseDeltas) {
            if (!lightsPanelConnected) {
                printf("Power/Lights panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                lightsPanelConnected = true;
            }
            sendFull(prevLightsData, lightsDataSize);
        }
        else {
            sendDelta(prevLightsData, lightsDataSize);
        }
    }
    else {
        // Data size mismatch
        bytes = sendto(sockfd, (char*)&instrumentsDataSize, 4, 0, (SOCKADDR*)&senderAddr, addrSize);
#ifdef SHOW_NETWORK_USAGE
        networkOut += bytes;
#endif
        printf("Client at %s requested %ld bytes instead of %ld bytes\n",
            inet_ntoa(senderAddr.sin_addr), request.requestedSize, instrumentsDataSize);
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

    deltaData = (char*)malloc(MaxDataSize);
    prevInstrumentsData = (char*)malloc(MaxDataSize);
    prevAutopilotData = (char*)malloc(MaxDataSize);
    prevRadioData = (char*)malloc(MaxDataSize);
    prevLightsData = (char*)malloc(MaxDataSize);

    printf("Server listening on port %d\n", Port);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    while (!quit) {
        picoRefresh();      // DELETE ME
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);

        // Wait for instrument panel to poll (non-blocking, 0.5 second timeout)
        int sel = select(FD_SETSIZE, &fds, 0, 0, &timeout);
        if (sel > 0) {
            bytes = recvfrom(sockfd, (char*)&request, sizeof(request), 0, (SOCKADDR*)&senderAddr, &addrSize);

#ifdef SHOW_NETWORK_USAGE
            networkIn += bytes;
#endif
            if (bytes > 3) {
                processRequest(bytes);
            }
            else {
                if (bytes == -1) {
                    int error = WSAGetLastError();
                    if (error == 10040) {
                        printf("Received more than %ld bytes from %s (WSAError = %d)\n", sizeof(request), inet_ntoa(senderAddr.sin_addr), error);
                    }
                    else {
                        printf("Received from %s but WSAError = %d\n", inet_ntoa(senderAddr.sin_addr), error);
                    }
                }
                else {
                    printf("Received %d bytes from %s - Not a valid request\n", bytes, inet_ntoa(senderAddr.sin_addr));
                }
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

#ifdef SHOW_NETWORK_USAGE
        ULONGLONG now = GetTickCount64();
        double elapsedMillis = now - networkStart;
        if (elapsedMillis > 2000) {
            if (networkStart > 0) {
                double kbInPerSec = networkIn / elapsedMillis;
                double kbOutPerSec = networkOut / elapsedMillis;
                printf("Network Usage: In = %.2f KB/s, Out = %.2f KB/s\n", kbInPerSec, kbOutPerSec);
            }
            networkStart = now;
            networkIn = 0;
            networkOut = 0;
        }
#endif
    }

    free(deltaData);
    free(prevInstrumentsData);
    free(prevAutopilotData);
    free(prevRadioData);
    free(prevLightsData);

    closesocket(sockfd);
    printf("Server stopped\n");
}

#ifdef PICO_USB
/// <summary>
/// Raspberry Pi Pico Switchbox (4 encoders + 4 buttons) appears as a USB joystick with 4 axes and 9 buttons.
/// Raspberry Pi Pico Garmin G1000 appears as another USB joystick with 3 axes and 21 buttons.
/// </summary>
void picoInit()
{
    // Initialise simvars
    simVars.sbMode = 0;     // Default to autopilot on switchbox
    for (int i = 0; i < 7; i++) {
        if (i < 4) {
            simVars.sbEncoder[i] = 0;
        }
        simVars.sbButton[i] = 0;
    }

    simVars.sbParkBrake = -1;

    JOYCAPSA joyCaps;
    int joystickId;

    for (joystickId = 0; joystickId < 16; joystickId++) {
        MMRESULT res = joyGetDevCaps(joystickId, &joyCaps, sizeof(joyCaps));
        if (res != JOYERR_NOERROR) {
            continue;
        }

        // Pico Switchbox has 4 axes and 10 buttons (includes 4 buttons for clickable encoders + park brake button + a dummy refresh button)
        if (switchboxId < 0 && joyCaps.wNumAxes == 4 && joyCaps.wNumButtons == 10) {
            switchboxId = joystickId;
            printf("Found Pico Switchbox joystick id %d\n", switchboxId);
        }

        // Pico G1000 has 3 axes and 21 buttons
        if (g1000Id < 0 && joyCaps.wNumAxes == 3 && joyCaps.wNumButtons == 21) {
            g1000Id = joystickId;
            printf("Found Pico G1000 joystick id %d\n", g1000Id);
        }
    }

    if (switchboxId >= 0 and g1000Id >= 0) {
        joystickRetry = 0;
    }
    else if (joystickRetry > 0) {
        joystickRetry--;
    }
    else {
        if (switchboxId < 0) {
            printf("No Pico Switchbox connected\n");
        }

        if (g1000Id < 0) {
            printf("No Pico G1000 connected\n");
        }
    }
}

void picoRefresh()
{
    if (joystickRetry > 0) {
        picoInit();
    }

    if (switchboxId >= 0) {
        switchboxRefresh();
    }

    if (g1000Id >= 0) {
        g1000Refresh();
    }
}

void switchboxRefresh()
{
    JOYINFOEX joyInfo;
    joyInfo.dwSize = sizeof(joyInfo);
    joyInfo.dwFlags = JOY_RETURNALL;
    joyInfo.dwButtons = 0xffffffffl;

    MMRESULT res = joyGetPosEx(switchboxId, &joyInfo);
    if (res != JOYERR_NOERROR) {
        return;
    }

    // First set of data with button 9 pressed (refresh button) will zeroise all axes
    if (!switchboxZeroed && joyInfo.dwButtons == (1 << 9)) {
        switchboxZeroPos[0] = joyInfo.dwXpos;
        switchboxZeroPos[1] = joyInfo.dwYpos;
        switchboxZeroPos[2] = joyInfo.dwZpos;
        switchboxZeroPos[3] = joyInfo.dwRpos;
        switchboxZeroed = true;
    }

    if (switchboxZeroed) {
        // Set simvars
        simVars.sbEncoder[0] = joyInfo.dwXpos - switchboxZeroPos[0];
        simVars.sbEncoder[1] = joyInfo.dwYpos - switchboxZeroPos[1];
        simVars.sbEncoder[2] = joyInfo.dwZpos - switchboxZeroPos[2];
        simVars.sbEncoder[3] = joyInfo.dwRpos - switchboxZeroPos[3];

        //printf("Switchbox Axes 0=%.0f, 1=%.0f, 2=%.0f, 3=%.0f\n", simVars.sbEncoder[0], simVars.sbEncoder[1], simVars.sbEncoder[2], simVars.sbEncoder[3]);

        // Buttons need to be set to 2 if pressed or 1 if released
        int mask = 1;
        for (int i = 0; i < 7; i++) {
            simVars.sbButton[i] = 1 + ((joyInfo.dwButtons & mask) > 0);
            mask = mask << 1;

            //if (simVars.sbButton[i] == 2) {
            //    printf("Pico Switchbox button %d pressed\n", i);
            //}
        }

        // Mode button
        double button7 = (joyInfo.dwButtons & (1 << 7)) > 0;
        //if (button7) {
        //    printf("Pico Switchbox mode button pressed\n");
        //}

        // Park Brake
        simVars.sbParkBrake = (joyInfo.dwButtons & (1 << 8)) > 0;
        //if (simVars.sbParkBrake) {
        //    printf("Pico Switchbox Park Brake on\n");
        //}

        // Check for mode button press (plays a sound when switched)
        if (button7 && !modeChange) {
            modeChange = true;
            if (simVars.sbMode > 3) {
                simVars.sbMode = 1;
            }
            else {
                simVars.sbMode++;
            }

            char soundFile[50];
            switch (int(simVars.sbMode)) {
                case 2: strcpy(soundFile, "Sounds\\Switchbox Radio.wav"); break;
                case 3: strcpy(soundFile, "Sounds\\Switchbox Instruments.wav"); break;
                case 4: strcpy(soundFile, "Sounds\\Switchbox Navigation.wav"); break;
                default: strcpy(soundFile, "Sounds\\Switchbox Autopilot.wav"); break;
            }

            PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC);
        }

        // Check for mode button release
        if (modeChange && !button7) {
            modeChange = false;
        }
    }
}

void g1000Refresh()
{
    JOYINFOEX joyInfo;
    joyInfo.dwSize = sizeof(joyInfo);
    joyInfo.dwFlags = JOY_RETURNALL;
    joyInfo.dwButtons = 0xffffffffl;

    MMRESULT res = joyGetPosEx(g1000Id, &joyInfo);
    if (res != JOYERR_NOERROR) {
        return;
    }

    // First set of data with button 20 pressed (refresh button) will zeroise all axes
    if (!g1000Zeroed && joyInfo.dwButtons == (1 << 20)) {
        g1000ZeroPos[0] = joyInfo.dwXpos;
        g1000ZeroPos[1] = joyInfo.dwYpos;
        g1000ZeroPos[2] = joyInfo.dwZpos;
        g1000Zeroed = true;

        g1000EncoderVal[0] = 0;
        g1000EncoderVal[1] = 0;
        g1000EncoderVal[2] = 0;

        for (int i = 0; i < 20; i++) {
            g1000ButtonVal[i] = 1;
        }
    }

    if (g1000Zeroed) {
        double lower = joyInfo.dwXpos - g1000ZeroPos[0];
        double upper = joyInfo.dwYpos - g1000ZeroPos[1];
        double zoom = joyInfo.dwZpos - g1000ZeroPos[2];

        //printf("G1000 Axes 0=%.0f, 1=%.0f, 2=%.0f\n", lower, upper, zoom);

        // Passes 0 if no change, > 0 if turned clockwise and < 0 if turned anti-clockwise
        g1000Encoder(lower - g1000EncoderVal[0], upper - g1000EncoderVal[1], zoom - g1000EncoderVal[2]);
        
        g1000EncoderVal[0] = lower;
        g1000EncoderVal[1] = upper;
        g1000EncoderVal[2] = zoom;

        int mask = 1;
        for (int i = 0; i < 20; i++) {
            // Buttons need to be set to 2 if pressed or 1 if released
            int buttonVal = 1 + ((joyInfo.dwButtons & mask) > 0);
            mask = mask << 1;

            // Has button state changed?
            if (buttonVal == g1000ButtonVal[i]) {
                // Check for CLR long press
                if (i == 18 && buttonVal == 2 && clrPress > 0) {
                    time_t now;
                    time(&now);
                    if (now - clrPress > 1) {
                        writeJetbridgeHvar(WriteEvents[HVAR_G1000_PFD_CLR_LONG].name);
                        clrPress = 0;
                    }
                }
                continue;
            }

            g1000ButtonVal[i] = buttonVal;

            // Has button been pressed?
            if (buttonVal == 2) {
                if (i < 2) {
                    // Encoder click x 2
                    g1000EncoderPush(i);
                }
                else if (i < 14) {
                    // Softkey x 12
                    g1000SoftkeyPush(i - 2);
                }
                else {
                    // Button x 6
                    g1000ButtonPush(i - 14);
                    // If CLR button save time pressed
                    if (i == 18) {
                        time(&clrPress);
                    }
                    else {
                        clrPress = 0;
                    }
                }
            }
        }
    }
}

void g1000Encoder(int lowerDiff, int upperDiff, int zoomDiff)
{
    EVENT_ID eventId;

    if (lowerDiff != 0) {
        if (lowerDiff > 0) {
            if (g1000IsPrimary) eventId = HVAR_G1000_PFD_LOWER_INC; else eventId = HVAR_G1000_MFD_LOWER_INC;
        }
        else {
            if (g1000IsPrimary) eventId = HVAR_G1000_PFD_LOWER_DEC; else eventId = HVAR_G1000_MFD_LOWER_DEC;
        }
        writeJetbridgeHvar(WriteEvents[eventId].name);
    }

    if (upperDiff != 0) {
        if (upperDiff > 0) {
            if (g1000IsPrimary) eventId = HVAR_G1000_PFD_UPPER_INC; else eventId = HVAR_G1000_MFD_UPPER_INC;
        }
        else {
            if (g1000IsPrimary) eventId = HVAR_G1000_PFD_UPPER_DEC; else eventId = HVAR_G1000_MFD_UPPER_DEC;
        }
        writeJetbridgeHvar(WriteEvents[eventId].name);
    }

    if (zoomDiff != 0) {
        if (zoomDiff > 0) {
            eventId = HVAR_G1000_MFD_RANGE_DEC;
        }
        else {
            eventId = HVAR_G1000_MFD_RANGE_INC;
        }
        writeJetbridgeHvar(WriteEvents[eventId].name);
    }
}

void g1000SwapWindows()
{
    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
    {
        if (!IsWindowVisible(hwnd))
            continue;

        char title[256];
        int len = GetWindowText(hwnd, title, 256);
        if (len == 0)
            continue;

        if (strcmp(title, "AS1000_PFD") == 0) {
            pfdInfo.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwnd, &pfdInfo)) {
                pfdWin = hwnd;
            }
        }
        else if (strcmp(title, "AS1000_MFD") == 0) {
            mfdInfo.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwnd, &mfdInfo)) {
                mfdWin = hwnd;
            }
        }
    }

    if (!pfdWin || !mfdWin)
        return;

    WINDOWINFO tempInfo;
    memcpy(&tempInfo, &pfdInfo, sizeof(WINDOWINFO));
    memcpy(&pfdInfo, &mfdInfo, sizeof(WINDOWINFO));
    memcpy(&mfdInfo, &tempInfo, sizeof(WINDOWINFO));

    UINT flags = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER;
    SetWindowPos(pfdWin, NULL, pfdInfo.rcWindow.left, pfdInfo.rcWindow.top, pfdInfo.rcWindow.right - pfdInfo.rcWindow.left, pfdInfo.rcWindow.bottom - pfdInfo.rcWindow.top, flags);
    SetWindowPos(mfdWin, NULL, mfdInfo.rcWindow.left, mfdInfo.rcWindow.top, mfdInfo.rcWindow.right - mfdInfo.rcWindow.left, mfdInfo.rcWindow.bottom - mfdInfo.rcWindow.top, flags);

    g1000IsPrimary = pfdInfo.rcWindow.top > -100;
}

void g1000EncoderPush(int num)
{
    EVENT_ID eventId;
    if (num == 0) {
        if (g1000IsPrimary) eventId = HVAR_G1000_PFD_PUSH; else eventId = HVAR_G1000_MFD_PUSH;
        writeJetbridgeHvar(WriteEvents[eventId].name);
    }
    else {
        g1000SwapWindows();
    }
}

void g1000SoftkeyPush(int num)
{
    int eventNum;
    if (g1000IsPrimary) eventNum = HVAR_G1000_PFD_SOFTKEY_1 + num; else eventNum = HVAR_G1000_MFD_SOFTKEY_1 + num;
    writeJetbridgeHvar(WriteEvents[eventNum].name);
}

void g1000ButtonPush(int num)
{
    EVENT_ID eventId;
    if (g1000IsPrimary) {
        switch (num) {
        case 0: eventId = HVAR_G1000_PFD_DIRECTTO; break;
        case 1: eventId = HVAR_G1000_PFD_MENU; break;
        case 2: eventId = HVAR_G1000_PFD_FPL; break;
        case 3: eventId = HVAR_G1000_PFD_PROC; break;
        case 4: eventId = HVAR_G1000_PFD_CLR; break;
        default: eventId = HVAR_G1000_PFD_ENT; break;
        }
    }
    else {
        switch (num) {
        case 0: eventId = HVAR_G1000_MFD_DIRECTTO; break;
        case 1: eventId = HVAR_G1000_MFD_MENU; break;
        case 2: eventId = HVAR_G1000_MFD_FPL; break;
        case 3: eventId = HVAR_G1000_MFD_PROC; break;
        case 4: eventId = HVAR_G1000_MFD_CLR; break;
        default: eventId = HVAR_G1000_MFD_ENT; break;
        }
    }
    writeJetbridgeHvar(WriteEvents[eventId].name);
}
#endif // PICO_USB
