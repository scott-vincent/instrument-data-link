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

// Change the next line to false if you always want to send
// full data across the network rather than deltas.
const bool UseDeltas = true;

// Uncomment the next line to show network data usage.
// This should be a lot lower when using deltas.
//#define SHOW_NETWORK_USAGE

#ifdef SHOW_NETWORK_USAGE
ULONGLONG networkStart = 0;
long networkIn;
long networkOut;
#endif

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

// SimConnect doesn't currently support reading local (lvar)
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

const char JETBRIDGE_APU_MASTER_SW[] = "L:A32NX_OVHD_APU_MASTER_SW_PB_IS_ON, bool";
const char JETBRIDGE_APU_START[] = "L:A32NX_OVHD_APU_START_PB_IS_ON, bool";
const char JETBRIDGE_APU_START_AVAIL[] = "L:A32NX_OVHD_APU_START_PB_IS_AVAILABLE, bool";
const char JETBRIDGE_APU_BLEED[] = "L:A32NX_OVHD_PNEU_APU_BLEED_PB_IS_ON, bool";
const char JETBRIDGE_ELEC_BAT1[] = "L:A32NX_OVHD_ELEC_BAT_1_PB_IS_AUTO, bool";
const char JETBRIDGE_ELEC_BAT2[] = "L:A32NX_OVHD_ELEC_BAT_2_PB_IS_AUTO, bool";
const char JETBRIDGE_PARK_BRAKE_POS[] = "L:A32NX_PARK_BRAKE_LEVER_POS, bool";
const char JETBRIDGE_AUTOPILOT[] = "L:A32NX_AUTOPILOT_1_ACTIVE, bool";
const char JETBRIDGE_AUTOTHRUST[] = "L:A32NX_AUTOTHRUST_STATUS, enum";
const char JETBRIDGE_AUTOPILOT_HDG[] = "L:A32NX_AUTOPILOT_HEADING_SELECTED, degrees";
const char JETBRIDGE_AUTOPILOT_VS[] = "L:A32NX_AUTOPILOT_VS_SELECTED, feetperminute";
const char JETBRIDGE_MANAGED_SPEED[] = "L:A32NX_FCU_SPD_MANAGED_DOT, bool";
const char JETBRIDGE_MANAGED_HEADING[] = "L:A32NX_FCU_HDG_MANAGED_DOT, bool";
const char JETBRIDGE_MANAGED_ALTITUDE[] = "L:A32NX_FCU_ALT_MANAGED, bool";
const char JETBRIDGE_LATERAL_MODE[] = "L:A32NX_FMA_LATERAL_MODE, enum";
const char JETBRIDGE_VERTICAL_MODE[] = "L:A32NX_FMA_VERTICAL_MODE, enum";
const char JETBRIDGE_LOC_MODE[] = "L:A32NX_FCU_LOC_MODE_ACTIVE, bool";
const char JETBRIDGE_APPR_MODE[] = "L:A32NX_FCU_APPR_MODE_ACTIVE, bool";
const char JETBRIDGE_AUTOTHRUST_MODE[] = "L:A32NX_AUTOTHRUST_MODE, enum";
const char JETBRIDGE_AUTOBRAKE[] = "L:A32NX_AUTOBRAKES_ARMED_MODE, bool";
const char JETBRIDGE_LEFT_BRAKEPEDAL[] = "L:A32NX_LEFT_BRAKE_PEDAL_INPUT, percent";
const char JETBRIDGE_RIGHT_BRAKEPEDAL[] = "L:A32NX_RIGHT_BRAKE_PEDAL_INPUT, percent";
const char JETBRIDGE_ENGINE_EGT[] = "L:A32NX_ENGINE_EGT:1, number";
const char JETBRIDGE_ENGINE_FUEL_FLOW[] = "L:A32NX_ENGINE_FF:1, number";
const char JETBRIDGE_FLAPS_INDEX[] = "L:A32NX_FLAPS_HANDLE_INDEX, number";

jetbridge::Client* jetbridgeClient = 0;
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
double highestTouchdownVs = -999;
int onStandState = 0;
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

char deltaData[2048];
long deltaSize;
char* prevInstrumentsData = NULL;
char* prevAutopilotData = NULL;
char* prevRadioData = NULL;
char* prevLightsData = NULL;
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
void readJetbridgeVar(const char* var)
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
    if (strncmp(&data[1], JETBRIDGE_APU_MASTER_SW, sizeof(JETBRIDGE_APU_MASTER_SW) - 1) == 0) {
        simVars.apuMasterSw = atof(&data[sizeof(JETBRIDGE_APU_MASTER_SW) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_START, sizeof(JETBRIDGE_APU_START) - 1) == 0) {
        simVars.jbApuStart = atof(&data[sizeof(JETBRIDGE_APU_START) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_START_AVAIL, sizeof(JETBRIDGE_APU_START_AVAIL) - 1) == 0) {
        simVars.jbApuStartAvail = atof(&data[sizeof(JETBRIDGE_APU_START_AVAIL) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_APU_BLEED, sizeof(JETBRIDGE_APU_BLEED) - 1) == 0) {
        simVars.apuBleed = atof(&data[sizeof(JETBRIDGE_APU_BLEED) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_ELEC_BAT1, sizeof(JETBRIDGE_ELEC_BAT1) - 1) == 0) {
        simVars.elecBat1 = atof(&data[sizeof(JETBRIDGE_ELEC_BAT1) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_ELEC_BAT2, sizeof(JETBRIDGE_ELEC_BAT2) - 1) == 0) {
        simVars.elecBat2 = atof(&data[sizeof(JETBRIDGE_ELEC_BAT2) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_FLAPS_INDEX, sizeof(JETBRIDGE_FLAPS_INDEX) - 1) == 0) {
        simVars.jbFlapsIndex = atof(&data[sizeof(JETBRIDGE_FLAPS_INDEX) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_PARK_BRAKE_POS, sizeof(JETBRIDGE_PARK_BRAKE_POS) - 1) == 0) {
        simVars.jbParkBrakePos = atof(&data[sizeof(JETBRIDGE_PARK_BRAKE_POS) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOPILOT, sizeof(JETBRIDGE_AUTOPILOT) - 1) == 0) {
        simVars.jbAutopilot = atof(&data[sizeof(JETBRIDGE_AUTOPILOT) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOTHRUST, sizeof(JETBRIDGE_AUTOTHRUST) - 1) == 0) {
        simVars.jbAutothrust = atof(&data[sizeof(JETBRIDGE_AUTOTHRUST) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOPILOT_HDG, sizeof(JETBRIDGE_AUTOPILOT_HDG) - 1) == 0) {
        simVars.jbAutopilotHeading = atof(&data[sizeof(JETBRIDGE_AUTOPILOT_HDG) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOPILOT_VS, sizeof(JETBRIDGE_AUTOPILOT_VS) - 1) == 0) {
        simVars.jbAutopilotVerticalSpeed = atof(&data[sizeof(JETBRIDGE_AUTOPILOT_VS) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_MANAGED_SPEED, sizeof(JETBRIDGE_MANAGED_SPEED) - 1) == 0) {
        simVars.jbManagedSpeed = atof(&data[sizeof(JETBRIDGE_MANAGED_SPEED) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_MANAGED_HEADING, sizeof(JETBRIDGE_MANAGED_HEADING) - 1) == 0) {
        simVars.jbManagedHeading = atof(&data[sizeof(JETBRIDGE_MANAGED_HEADING) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_MANAGED_ALTITUDE, sizeof(JETBRIDGE_MANAGED_ALTITUDE) - 1) == 0) {
        simVars.jbManagedAltitude = atof(&data[sizeof(JETBRIDGE_MANAGED_ALTITUDE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_LATERAL_MODE, sizeof(JETBRIDGE_LATERAL_MODE) - 1) == 0) {
        simVars.jbLateralMode = atof(&data[sizeof(JETBRIDGE_LATERAL_MODE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_VERTICAL_MODE, sizeof(JETBRIDGE_VERTICAL_MODE) - 1) == 0) {
        simVars.jbVerticalMode = atof(&data[sizeof(JETBRIDGE_VERTICAL_MODE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_LOC_MODE, sizeof(JETBRIDGE_LOC_MODE) - 1) == 0) {
        simVars.jbLocMode = atof(&data[sizeof(JETBRIDGE_LOC_MODE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_APPR_MODE, sizeof(JETBRIDGE_APPR_MODE) - 1) == 0) {
        simVars.jbApprMode = atof(&data[sizeof(JETBRIDGE_APPR_MODE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOTHRUST_MODE, sizeof(JETBRIDGE_AUTOTHRUST_MODE) - 1) == 0) {
        simVars.jbAutothrustMode = atof(&data[sizeof(JETBRIDGE_AUTOTHRUST_MODE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_AUTOBRAKE, sizeof(JETBRIDGE_AUTOBRAKE) - 1) == 0) {
        simVars.jbAutobrake = atof(&data[sizeof(JETBRIDGE_AUTOBRAKE) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_LEFT_BRAKEPEDAL, sizeof(JETBRIDGE_LEFT_BRAKEPEDAL) - 1) == 0) {
        simVars.jbLeftBrakePedal = atof(&data[sizeof(JETBRIDGE_LEFT_BRAKEPEDAL) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_RIGHT_BRAKEPEDAL, sizeof(JETBRIDGE_RIGHT_BRAKEPEDAL) - 1) == 0) {
        simVars.jbRightBrakePedal = atof(&data[sizeof(JETBRIDGE_RIGHT_BRAKEPEDAL) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_ENGINE_EGT, sizeof(JETBRIDGE_ENGINE_EGT) - 1) == 0) {
        simVars.jbEngineEgt = atof(&data[sizeof(JETBRIDGE_ENGINE_EGT) + 1]);
    }
    else if (strncmp(&data[1], JETBRIDGE_ENGINE_FUEL_FLOW, sizeof(JETBRIDGE_ENGINE_FUEL_FLOW) - 1) == 0) {
        simVars.jbEngineFuelFlow = atof(&data[sizeof(JETBRIDGE_ENGINE_FUEL_FLOW) + 1]);
    }
}

bool jetbridgeButtonPress(int eventId, double value)
{
    if (strncmp(simVars.aircraft, "FBW", 3) != 0) {
        return false;
    }

    switch (eventId) {
    case KEY_APU_OFF_SWITCH:
        writeJetbridgeVar(JETBRIDGE_APU_MASTER_SW, value);
        return true;
    case KEY_APU_STARTER:
        writeJetbridgeVar(JETBRIDGE_APU_START, value);
        return true;
    case KEY_BLEED_AIR_SOURCE_CONTROL_SET:
        writeJetbridgeVar(JETBRIDGE_APU_BLEED, value);
        return true;
    case KEY_ELEC_BAT1:
        writeJetbridgeVar(JETBRIDGE_ELEC_BAT1, value);
        return true;
    case KEY_ELEC_BAT2:
        writeJetbridgeVar(JETBRIDGE_ELEC_BAT2, value);
        return true;
    case KEY_AUTOBRAKE:
        writeJetbridgeVar(JETBRIDGE_AUTOBRAKE, value);
        return true;
    }

    return false;
}

void pollJetbridge()
{
    // Use low frequency for jetbridge as vars not critical
    int loopMillis = 100;

    while (!quit)
    {
        if (simVars.connected && strncmp(simVars.aircraft, "FBW", 3) == 0) {
            readJetbridgeVar(JETBRIDGE_APU_MASTER_SW);
            readJetbridgeVar(JETBRIDGE_APU_START);
            readJetbridgeVar(JETBRIDGE_APU_START_AVAIL);
            readJetbridgeVar(JETBRIDGE_APU_BLEED);
            readJetbridgeVar(JETBRIDGE_ELEC_BAT1);
            readJetbridgeVar(JETBRIDGE_ELEC_BAT2);
            readJetbridgeVar(JETBRIDGE_FLAPS_INDEX);
            readJetbridgeVar(JETBRIDGE_PARK_BRAKE_POS);
            readJetbridgeVar(JETBRIDGE_AUTOPILOT);
            readJetbridgeVar(JETBRIDGE_AUTOTHRUST);
            readJetbridgeVar(JETBRIDGE_MANAGED_SPEED);
            readJetbridgeVar(JETBRIDGE_MANAGED_HEADING);
            readJetbridgeVar(JETBRIDGE_MANAGED_ALTITUDE);
            readJetbridgeVar(JETBRIDGE_LATERAL_MODE);
            readJetbridgeVar(JETBRIDGE_VERTICAL_MODE);
            readJetbridgeVar(JETBRIDGE_LOC_MODE);
            readJetbridgeVar(JETBRIDGE_APPR_MODE);
            readJetbridgeVar(JETBRIDGE_AUTOTHRUST_MODE);
            readJetbridgeVar(JETBRIDGE_AUTOBRAKE);
            readJetbridgeVar(JETBRIDGE_LEFT_BRAKEPEDAL);
            readJetbridgeVar(JETBRIDGE_RIGHT_BRAKEPEDAL);
            readJetbridgeVar(JETBRIDGE_AUTOPILOT_HDG);
            readJetbridgeVar(JETBRIDGE_AUTOPILOT_VS);
            readJetbridgeVar(JETBRIDGE_ENGINE_EGT);
            readJetbridgeVar(JETBRIDGE_ENGINE_FUEL_FLOW);
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
            if (pObjData->dwSize < varsSize) {
                printf("Error: SimConnect expected %d bytes but received %d bytes\n",
                    varsSize, pObjData->dwSize);
                fflush(stdout);
            }
            else {
                memcpy(varsStart, &pObjData->dwData, varsSize);
            }

            if (strncmp(simVars.aircraft, "Airbus", 6) == 0) {
                // Some liveries don't have FBW prefix
                strcpy(simVars.aircraft, "FBW Airbus A320 Neo");
            }

            if (strncmp(simVars.aircraft, "FBW", 3) == 0) {
                // Map A32NX vars to real vars
                simVars.apuStartSwitch = simVars.jbApuStart;
                if (simVars.jbApuStartAvail) {
                    simVars.apuPercentRpm = 100;
                }
                else {
                    simVars.apuPercentRpm = 0;
                }
                simVars.tfFlapsIndex = simVars.jbFlapsIndex;
                simVars.parkingBrakeOn = simVars.jbParkBrakePos;
                simVars.brakePedal = (simVars.jbLeftBrakePedal + simVars.jbRightBrakePedal) / 2.0;
                simVars.autopilotEngaged = simVars.jbAutopilot;
                if (simVars.jbAutothrust == 0) {
                    simVars.autothrottleActive = 0;
                }
                else {
                    simVars.autothrottleActive = 1;
                }
                simVars.autopilotHeading = simVars.jbAutopilotHeading;
                simVars.autopilotVerticalSpeed = simVars.jbAutopilotVerticalSpeed;
                if (simVars.jbVerticalMode == 14) {
                    simVars.autopilotVerticalHold = 1;
                }
                else {
                    simVars.autopilotVerticalHold = 0;
                }
                simVars.autopilotApproachHold = simVars.jbLocMode;
                simVars.autopilotGlideslopeHold = simVars.jbApprMode;
                simVars.tfAutoBrake = simVars.jbAutobrake + 1;
                simVars.exhaustGasTemp = simVars.jbEngineEgt;
                simVars.engineFuelFlow = simVars.jbEngineFuelFlow;
            }
            else if (strncmp(simVars.aircraft, "Boeing", 6) == 0) {
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

            if (simVars.altAboveGround > 50) {
                if (simVars.altAboveGround > 200) {
                    hasFlown = true;
                    highestTouchdownVs = -999;
                }

                if (initiatedPushback) {
                    // Reset ground state
                    initiatedPushback = false;
                    onStandState = 0;
                }

                if (!completedTakeOff && simVars.altAltitude > 10000) {
                    completedTakeOff = true;
                }
            }
            else if (completedTakeOff && (simVars.elecBat1 == 0 || simVars.elecBat2 == 0)) {
                printf("Reset flight (Battery off)\n");
                fflush(stdout);
                completedTakeOff = false;
            }

            // Record highest landing rate (plane may bounce)
            if (hasFlown && simVars.onGround) {
                if (highestTouchdownVs < simVars.touchdownVs) {
                    highestTouchdownVs = simVars.touchdownVs;
                    printf("Touchdown Vertical Speed: %d FPM\n", (int)((highestTouchdownVs * 60) + 0.5));
                }
            }

            // Landing rate for instrument panel to display where -999 suppresses display
            simVars.touchdownVs = highestTouchdownVs;

            // For testing only - Leave commented out
            if (displayDelay > 0) {
                displayDelay--;
            }
            else {
                //printf("Aircraft: %s   Cruise Speed: %f\n", simVars.aircraft, simVars.cruiseSpeed);
                displayDelay = 60;
            }

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
            updateVarFromJetbridge(packet->data);
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

    for (int i = 0;; i++) {
        if (SimVarDefs[i][0] == NULL) {
            break;
        }

        if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
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

            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], NULL, dataType) < 0) {
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
            if (SimConnect_AddToDataDefinition(hSimConnect, DEF_READ_ALL, SimVarDefs[i][0], SimVarDefs[i][1]) < 0) {
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

    if (prevInstrumentsData != NULL) {
        delete [] prevInstrumentsData;
    }

    if (prevAutopilotData != NULL) {
        delete [] prevAutopilotData;
    }

    if (prevRadioData != NULL) {
        delete [] prevRadioData;
    }

    if (prevLightsData != NULL) {
        delete [] prevLightsData;
    }

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
    deltaString.offset = 2048 + offset;     // Add 2048 so we know it is a string
    strncpy(deltaString.data, newVal, 32);  // Only support string32

    memcpy(deltaData + deltaSize, &deltaString, deltaStringSize);
    deltaSize += deltaStringSize;
}

/// <summary>
/// If this a new connection send all the data otherwise only send
/// the delta, i.e. data that has changed since we last sent it.
/// This should reduce network bandwidth usage hugely.
/// </summary>
void sendDelta(char** prevSimVars, long dataSize)
{
    if (*prevSimVars == NULL || !UseDeltas) {
        // No prev data so send full rather than delta
        bytes = sendto(sockfd, (char*)&simVars, dataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
#ifdef SHOW_NETWORK_USAGE
        networkOut += bytes;
#endif

        // Update prev data
        *prevSimVars = new char[dataSize];
        memcpy(*prevSimVars, &simVars, dataSize);
        return;
    }

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

        char* oldVarPtr = *prevSimVars + offset;
        char* newVarPtr = (char*)&simVars + offset;

        if (_strnicmp(SimVarDefs[i][1], "string", 6) == 0) {
            // Has string changed?
            if (strncmp(oldVarPtr, newVarPtr, 32) != 0) {
                addDeltaString(offset, newVarPtr);
            }

            offset += 32;
        }
        else {
            // Has double changed?
            double* oldVar = (double*)oldVarPtr;
            double* newVar = (double*)newVarPtr;
            if (*oldVar != *newVar) {
                addDeltaDouble(offset, *newVar);
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
        bytes = sendto(sockfd, (char*)&deltaData, deltaSize, 0, (SOCKADDR*)&senderAddr, addrSize);
    }
    else {
        // Send full data
        bytes = sendto(sockfd, (char*)&simVars, dataSize, 0, (SOCKADDR*)&senderAddr, addrSize);
    }
#ifdef SHOW_NETWORK_USAGE
    networkOut += bytes;
#endif

    // Update prev data
    memcpy(*prevSimVars, &simVars, dataSize);
}

/// <summary>
/// If an event button is pressed return either EVENT_NONE or the event (sound)
/// that should be played depending on current aircraft state.
/// </summary>
EVENT_ID getCustomEvent(int eventNum)
{
    EVENT_ID event = EVENT_NONE;

    FLIGHT_PHASE phase = GROUND;
    if (simVars.altAboveGround > 50) {
        bool isClimbing = simVars.vsiVerticalSpeed > 3;
        bool isDescending = simVars.vsiVerticalSpeed < -3;

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
                if (simVars.altAltitude > 5000) {
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
                return EVENT_FINAL_DESCENT;
            case GO_AROUND:
                return EVENT_GO_AROUND;
        }
    }

    return EVENT_NONE;
}

void processRequest()
{
    if (request.requestedSize == writeDataSize) {
         // This is a write
        if (!simVars.connected) {
            return;
        }

        //// For testing only - Leave commented out
        //if (request.writeData.eventId == KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE) {
        //    request.writeData.eventId = A32NX_FCU_SPD_PULL;
        //    request.writeData.value = 1;
        //    printf("Intercepted event - Changed to: %d = %f\n", request.writeData.eventId, request.writeData.value);
        //}
        //else {
        //    printf("Unintercepted event: %d = %f\n", request.writeData.eventId, request.writeData.value);
        //}

        if (request.writeData.eventId >= VJOY_BUTTONS && request.writeData.eventId <= VJOY_BUTTONS_END) {
#ifdef vJoyFallback
            if (!vJoyInitialised) {
                vJoyInit();
            }
            vJoyButtonPress(request.writeData.eventId);
#else
            printf("vJoy button event ignored - vJoyFallback is not enabled\n");
#endif
            return;
        }

#ifdef jetbridgeFallback
        if (jetbridgeButtonPress(request.writeData.eventId, request.writeData.value)) {
            return;
        }
#endif

        // Process custom events
        if (request.writeData.eventId == KEY_CHECK_EVENT) {
            int eventNum = (int)(request.writeData.value);
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
        if (active != 1 || request.wantFullData) {
            if (active != 1) {
                printf("Instrument panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                active = 1;
            }
            if (prevInstrumentsData != NULL) {
                delete [] prevInstrumentsData;
                prevInstrumentsData = NULL;
            }
        }
        sendDelta(&prevInstrumentsData, instrumentsDataSize);
    }
    else if (request.requestedSize == autopilotDataSize) {
        // Send autopilot data to the client that polled us
        if (!autopilotPanelConnected || request.wantFullData) {
            if (!autopilotPanelConnected) {
                printf("Autopilot panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                autopilotPanelConnected = true;
            }
            if (prevAutopilotData != NULL) {
                delete [] prevAutopilotData;
                prevAutopilotData = NULL;
            }
        }
        sendDelta(&prevAutopilotData, autopilotDataSize);
    }
    else if (request.requestedSize == radioDataSize) {
        // Send radio data to the client that polled us
        if (!radioPanelConnected || request.wantFullData) {
            if (!radioPanelConnected) {
                printf("Radio panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                radioPanelConnected = true;
            }
            if (prevRadioData != NULL) {
                delete [] prevRadioData;
                prevRadioData = NULL;
            }
        }
        sendDelta(&prevRadioData, radioDataSize);
    }
    else if (request.requestedSize == lightsDataSize) {
        // Send power/lights data to the client that polled us
        if (!lightsPanelConnected || request.wantFullData) {
            if (!lightsPanelConnected) {
                printf("Power/Lights panel connected from %s\n", inet_ntoa(senderAddr.sin_addr));
                lightsPanelConnected = true;
            }
            if (prevLightsData != NULL) {
                delete [] prevLightsData;
                prevLightsData = NULL;
            }
        }
        sendDelta(&prevLightsData, lightsDataSize);
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
