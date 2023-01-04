/*
 * Flight Simulator Instrument Data Link
 * Copyright (c) 2023 Scott Vincent
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include "simvarDefs.h"
#include "LVars-A310.h"
#include "LVars-A32NX.h"
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

// Comment the following line out if you don't want to reduce rudder sensitivity
#define RUDDER_SENSITIVITY

#ifdef RUDDER_SENSITIVITY
#include <Mmsystem.h>
#include <joystickapi.h>

void joystickInit();
void joystickRefresh();

int joystickId = -1;
int joystickRetry = 5;
JOYINFOEX joyInfo;
double rudderSensitivity = 1;
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
bool stoppedPushback = false;
bool completedTakeOff = false;
bool hasFlown = false;
int onStandState = 0;
double skytrackState = 0;
bool isA310 = false;
bool isA320 = false;
bool is747 = false;
bool isK100 = false;
bool isAirliner = false;
double lastHeading = 0;
int lastSoftkey = 0;
int lastG1000Key = 0;
time_t lastG1000Press = 0;
int seatBeltsReplicateDelay = 0;
LVars_A310 a310Vars;
LVars_A320 a320Vars;
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
time_t lastPushbackAdjust = 0;

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
        else if (simVars.connected && isA320) {
            readA320Jetbridge();
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
            isA320 = false;
            is747 = false;
            isK100 = false;
            isAirliner = false;

            if (strncmp(simVars.aircraft, "A310", 4) == 0 || strncmp(simVars.aircraft, "Airbus A310", 11) == 0) {
                isA310 = true;
                isAirliner = true;
            }
            else if (strncmp(simVars.aircraft, "FBW", 3) == 0 || strncmp(simVars.aircraft, "Airbus A320", 11) == 0) {
                isA320 = true;
                isAirliner = true;
            }
            else if (strncmp(simVars.aircraft, "Salty", 5) == 0 || strncmp(simVars.aircraft, "Boeing 747-8", 12) == 0) {
                is747 = true;
                isAirliner = true;
            }
            else if (strncmp(simVars.aircraft, "Kodiak 100", 10) == 0) {
                isK100 = true;
            }
            else if (strncmp(simVars.aircraft, "Airbus", 6) == 0 || strncmp(simVars.aircraft, "Boeing", 6) == 0) {
                isAirliner = true;
            }

            if (abs(simVars.hiHeading - lastHeading) > 10) {
                // Fix gyro if aircraft heading changes abruptly
                SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_HEADING_GYRO_SET, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
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
                simVars.suctionPressure = 5;
                if (seatBeltsReplicateDelay > 0) {
                    seatBeltsReplicateDelay--;
                }
                else if (simVars.seatBeltsSwitch != a310Vars.seatbeltsSwitch) {
                    // Replicate lvar value back to standard SDK variable to make PACX work correctly
                    SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
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
            else if (simVars.connected && isA320) {
                // Map A32NX vars to real vars
                simVars.apuStartSwitch = a320Vars.apuStart;
                if (a320Vars.apuStartAvail) {
                    simVars.apuPercentRpm = 100;
                }
                else {
                    simVars.apuPercentRpm = 0;
                }
                simVars.tfFlapsIndex = a320Vars.flapsIndex;
                simVars.parkingBrakeOn = a320Vars.parkBrakePos;
                simVars.tfSpoilersPosition = a320Vars.spoilersHandlePos;
                simVars.brakeLeftPedal = a320Vars.leftBrakePedal;
                simVars.brakeRightPedal = a320Vars.rightBrakePedal;
                simVars.rudderPosition = a320Vars.rudderPedalPos / 100.0;
                simVars.autopilotEngaged = (a320Vars.autopilot1 == 0 && a320Vars.autopilot2 == 0) ? 0 : 1;
                if (a320Vars.autothrust == 0) {
                    simVars.autothrottleActive = 0;
                }
                else {
                    simVars.autothrottleActive = 1;
                }
                simVars.transponderState = a320Vars.xpndrMode;
                simVars.autopilotHeading = a320Vars.autopilotHeading;
                simVars.autopilotVerticalSpeed = a320Vars.autopilotVerticalSpeed;
                if (simVars.jbVerticalMode == 14) {
                    // V/S mode engaged
                    simVars.autopilotVerticalHold = 1;
                }
                else if (simVars.jbVerticalMode == 15) {
                    // FPA mode engaged
                    simVars.autopilotVerticalHold = -1;
                    simVars.autopilotVerticalSpeed = a320Vars.autopilotFpa;
                }
                else {
                    simVars.autopilotVerticalHold = 0;
                }
                simVars.autopilotApproachHold = simVars.jbLocMode;
                simVars.autopilotGlideslopeHold = simVars.jbApprMode;
                simVars.tfAutoBrake = simVars.jbAutobrake + 1;
                simVars.exhaustGasTemp1 = a320Vars.engineEgt1;
                simVars.exhaustGasTemp2 = a320Vars.engineEgt2;
                simVars.engineFuelFlow1 = a320Vars.engineFuelFlow1;
                simVars.engineFuelFlow2 = a320Vars.engineFuelFlow2;
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
                    SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_AP_ALT_VAR_SET_ENGLISH, 5000, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
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
            else if (isA320) {
                updateA320FromJetbridge(packet->data);
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
    printf("Instrument Data Link %s Copyright (c) 2023 Scott Vincent\n", versionString);

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

void processG1000Events()
{
    time_t now;
    time(&now);
    if (now - lastG1000Press > 5) {
        // Timeout the previous press
        lastSoftkey = 0;
    }

    if (request.writeData.eventId >= KEY_G1000_PFD_SOFTKEY_1 && request.writeData.eventId <= KEY_G1000_PFD_SOFTKEY_12) {
        if (lastSoftkey == KEY_G1000_PFD_SOFTKEY_11 && request.writeData.eventId == KEY_G1000_PFD_SOFTKEY_11 && (lastG1000Key == KEY_G1000_MFD_RANGE_INC || lastG1000Key == KEY_G1000_MFD_RANGE_DEC)) {
            // NRST -> Enter + DirectTo
            SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_G1000_PFD_ENT, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            request.writeData.eventId = KEY_G1000_PFD_DIRECTTO;
        }
        else if (lastSoftkey == KEY_G1000_PFD_DIRECTTO) {
            if (request.writeData.eventId == KEY_G1000_PFD_SOFTKEY_11) {
                // NRST -> Enter (after DirectTo)
                request.writeData.eventId = KEY_G1000_PFD_ENT;
            }
            else {
                // Cancel DirectTo (back to NRST)
                SimConnect_TransmitClientEvent(hSimConnect, 0, KEY_G1000_PFD_DIRECTTO, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                request.writeData.eventId = KEY_G1000_PFD_SOFTKEY_11;
            }
        }
        lastSoftkey = request.writeData.eventId;
        lastG1000Key = request.writeData.eventId;
    }
    else if (request.writeData.eventId == KEY_G1000_MFD_RANGE_INC) {
        lastG1000Key = request.writeData.eventId;
        if (lastSoftkey == KEY_G1000_PFD_SOFTKEY_11) {
            // RANGE_INC -> FMS_DEC
            request.writeData.eventId = KEY_G1000_PFD_LOWER_DEC;
        }
    }
    else if (request.writeData.eventId == KEY_G1000_MFD_RANGE_DEC) {
        lastG1000Key = request.writeData.eventId;
        if (lastSoftkey == KEY_G1000_PFD_SOFTKEY_11) {
            // RANGE_DEC -> FMS_INC
            request.writeData.eventId = KEY_G1000_PFD_LOWER_INC;
        }
    }

    time(&lastG1000Press);
}

void processRequest()
{
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
            else {
                rudderSensitivity = request.writeData.value + 1;
            }
            return;
        }

        if (!simVars.connected) {
            return;
        }

        //// For testing only - Leave commented out
        //if (request.writeData.eventId == KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE) {
        //    request.writeData.eventId = KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE;
        //    request.writeData.value = 1;
        //    printf("Intercepted event - Changed to: %d = %f\n", request.writeData.eventId, request.writeData.value);
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
        else if (isA320 && jetbridgeA320ButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
        else if (isK100 && jetbridgeK100ButtonPress(request.writeData.eventId, request.writeData.value)) {
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
                    stoppedPushback = false;
                }
                else {
                    stoppedPushback = true;
                }
                time(&lastPushbackAdjust);
            }
            else {
                return;
            }
        }
        else if (request.writeData.eventId == KEY_SKYTRACK_STATE) {
            skytrackState = request.writeData.value;
            return;
        }

        if (request.writeData.eventId >= KEY_G1000_PFD_SOFTKEY_1 && request.writeData.eventId <= KEY_G1000_END) {
            processG1000Events();
        }

        if (SimConnect_TransmitClientEvent(hSimConnect, 0, request.writeData.eventId, (DWORD)request.writeData.value, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
            printf("Failed to transmit event: %d\n", request.writeData.eventId);
        }

        if (request.writeData.eventId == KEY_COM1_STBY_RADIO_SET || request.writeData.eventId == KEY_COM2_STBY_RADIO_SET) {
            // Check for extra .5 on value
            int val = (int)(request.writeData.value * 10 + 0.01);
            if (val % 10 >= 4) {
                if (request.writeData.eventId == KEY_COM1_STBY_RADIO_SET) {
                    request.writeData.eventId = KEY_COM1_RADIO_FRACT_INC;
                }
                else {
                    request.writeData.eventId = KEY_COM2_RADIO_FRACT_INC;
                }
                if (SimConnect_TransmitClientEvent(hSimConnect, 0, request.writeData.eventId, 0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY) != 0) {
                    printf("Failed to transmit event: %d\n", request.writeData.eventId);
                }
            }
        }
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
        bytes = sendto(sockfd, (char*)&instrumentsDataSize, sizeof(long), 0, (SOCKADDR*)&senderAddr, addrSize);
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

#ifdef RUDDER_SENSITIVITY
        joystickRefresh();
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

#ifdef RUDDER_SENSITIVITY
void joystickInit()
{
    joyInfo.dwSize = sizeof(joyInfo);
    joyInfo.dwFlags = JOY_RETURNU | JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ;

    for (joystickId = 0; joystickId < 16; joystickId++) {
        MMRESULT res = joyGetPosEx(joystickId, &joyInfo);
        if (res != JOYERR_NOERROR) {
            continue;
        }

        // Rudder pedal axis will be centred (and throttles won't be)
        if (joyInfo.dwUpos > 30000 && joyInfo.dwUpos < 36000 && (joyInfo.dwXpos != 32767 || joyInfo.dwYpos != 32767 || joyInfo.dwZpos != 32767)) {
            break;
        }

        //printf("Joystick %d (%d, %d, %d, %d) does not have rudder pedal input\n", joystickId, joyInfo.dwUpos, joyInfo.dwXpos, joyInfo.dwYpos, joyInfo.dwZpos);
    }

    joyInfo.dwFlags = JOY_RETURNU;

    if (joystickId < 16) {
        printf("Rudder pedal input found on joystick %d\n", joystickId);
    }
    else {
        if (joystickRetry > 0) {
            joystickRetry--;
            joystickId = -1;
        }
        else {
            joystickId = 1;
            printf("Defaulting to joystick %d for rudder pedal input\n", joystickId);
        }
    }
}

/// <summary>
/// Reads the Rudder Pedal axis and sets vJoy axis 0 with reduced sensitivity.
/// Sensitivity is reduced to one third until the extremes where it is linearly mapped back to the full value.
/// To use: In FS2020, map vJoy axis 0 instead of the real rudder pedal axis.
/// </summary>
void joystickRefresh()
{
    if (joystickId < 0 || joystickId > 15) {
        if (joystickId == -1) {
            joystickInit();
            vJoyInit();
        }
        return;
    }

    MMRESULT res = joyGetPosEx(joystickId, &joyInfo);
    if (res != JOYERR_NOERROR) {
        return;
    }

    const int centre = 32767;

    int rudderPos = joyInfo.dwUpos;
    if (rudderSensitivity > 1) {
        // Reduce sensitivity
        rudderPos = centre + ((rudderPos - centre) / rudderSensitivity);
    }

#ifdef vJoyFallback
    // printf("rudder: %d adjusted to %d\n", joyInfo.dwUpos, rudderPos);
    vJoySetAxis(rudderPos);
#else
    printf("rudder not adjusted as no vJoy\n");
    joystickId = 16;
#endif
}
#endif // RUDDER_SENSITIVTY
