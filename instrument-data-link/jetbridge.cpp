#include "jetbridge.h"

#ifdef jetbridgeFallback

#include "jetbridge\client.h"
#include "simvarDefs.h"
#include "LVars-A310.h"
#include "LVars-A32NX.h"
#include "LVars-Kodiak100.h"

extern SimVars simVars;
extern LVars_A310 a310Vars;
extern LVars_A320 a320Vars;

jetbridge::Client* jetbridgeClient = 0;


void jetbridgeInit(HANDLE hSimConnect)
{
    if (jetbridgeClient != 0) {
        delete jetbridgeClient;
    }

    jetbridgeClient = new jetbridge::Client(hSimConnect);
}

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

void updateA310FromJetbridge(const char* data)
{
    if (strncmp(&data[1], A310_APU_MASTER_SW, sizeof(A310_APU_MASTER_SW) - 1) == 0) {
        simVars.apuMasterSw = atof(&data[sizeof(A310_APU_MASTER_SW) + 1]);
    }
    else if (strncmp(&data[1], A310_APU_START, sizeof(A310_APU_START) - 1) == 0) {
        a310Vars.apuStart = atof(&data[sizeof(A310_APU_START) + 1]);
    }
    else if (strncmp(&data[1], A310_APU_START_AVAIL, sizeof(A310_APU_START_AVAIL) - 1) == 0) {
        a310Vars.apuStartAvail = atof(&data[sizeof(A310_APU_START_AVAIL) + 1]);
    }
    else if (strncmp(&data[1], A310_APU_BLEED, sizeof(A310_APU_BLEED) - 1) == 0) {
        simVars.apuBleed = atof(&data[sizeof(A310_APU_BLEED) + 1]);
    }
    else if (strncmp(&data[1], A310_ELEC_BAT1, sizeof(A310_ELEC_BAT1) - 1) == 0) {
        simVars.elecBat1 = atof(&data[sizeof(A310_ELEC_BAT1) + 1]);
    }
    else if (strncmp(&data[1], A310_ELEC_BAT2, sizeof(A310_ELEC_BAT2) - 1) == 0) {
        simVars.elecBat2 = atof(&data[sizeof(A310_ELEC_BAT2) + 1]);
    }
    else if (strncmp(&data[1], A310_SEATBELTS_SWITCH, sizeof(A310_SEATBELTS_SWITCH) - 1) == 0) {
        a310Vars.seatbeltsSwitch = atof(&data[sizeof(A310_SEATBELTS_SWITCH) + 1]);
    }
    else if (strncmp(&data[1], A310_BEACON_LIGHTS, sizeof(A310_BEACON_LIGHTS) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_BEACON_LIGHTS) + 1]) != a310Vars.beaconLights) {
            writeJetbridgeVar(A310_BEACON_LIGHTS, a310Vars.beaconLights);
        }
    }
    else if (strncmp(&data[1], A310_LANDING_LIGHTS_L, sizeof(A310_LANDING_LIGHTS_L) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_LANDING_LIGHTS_L) + 1]) != a310Vars.landingLights) {
            writeJetbridgeVar(A310_LANDING_LIGHTS_L, a310Vars.landingLights);
            writeJetbridgeVar(A310_LANDING_LIGHTS_R, a310Vars.landingLights);
        }
    }
    else if (strncmp(&data[1], A310_TAXI_LIGHTS, sizeof(A310_TAXI_LIGHTS) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_TAXI_LIGHTS) + 1]) != a310Vars.taxiLights) {
            writeJetbridgeVar(A310_TAXI_LIGHTS, a310Vars.taxiLights);
        }
    }
    else if (strncmp(&data[1], A310_TURNOFF_LIGHTS_L, sizeof(A310_TURNOFF_LIGHTS_L) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_TURNOFF_LIGHTS_L) + 1]) != a310Vars.turnoffLights) {
            writeJetbridgeVar(A310_TURNOFF_LIGHTS_L, a310Vars.turnoffLights);
            writeJetbridgeVar(A310_TURNOFF_LIGHTS_R, a310Vars.turnoffLights);
        }
    }
    else if (strncmp(&data[1], A310_NAV_LIGHTS, sizeof(A310_NAV_LIGHTS) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_NAV_LIGHTS) + 1]) != a310Vars.navLights) {
            writeJetbridgeVar(A310_NAV_LIGHTS, a310Vars.navLights);
        }
    }
    else if (strncmp(&data[1], A310_STROBES, sizeof(A310_STROBES) - 1) == 0) {
        // Force to required value (doesn't always stick)
        if (atof(&data[sizeof(A310_STROBES) + 1]) != a310Vars.strobes) {
            writeJetbridgeVar(A310_STROBES, a310Vars.strobes);
        }
    }
    else if (strncmp(&data[1], A310_PITCH_TRIM1, sizeof(A310_PITCH_TRIM1) - 1) == 0) {
        a310Vars.pitchTrim1 = atof(&data[sizeof(A310_PITCH_TRIM1) + 1]);
    }
    else if (strncmp(&data[1], A310_PITCH_TRIM2, sizeof(A310_PITCH_TRIM2) - 1) == 0) {
        a310Vars.pitchTrim2 = atof(&data[sizeof(A310_PITCH_TRIM2) + 1]);
    }
    else if (strncmp(&data[1], A310_TCAS_MODE, sizeof(A310_TCAS_MODE) - 1) == 0) {
        simVars.jbTcasMode = atof(&data[sizeof(A310_TCAS_MODE) + 1]);
    }
    else if (strncmp(&data[1], A310_AP_AIRSPEED, sizeof(A310_AP_AIRSPEED) - 1) == 0) {
        a310Vars.autopilotAirspeed = atof(&data[sizeof(A310_AP_AIRSPEED) + 1]);
    }
    else if (strncmp(&data[1], A310_AP_IS_MACH, sizeof(A310_AP_IS_MACH) - 1) == 0) {
        simVars.jbShowMach = atof(&data[sizeof(A310_AP_IS_MACH) + 1]);
    }
    else if (strncmp(&data[1], A310_AP_HEADING, sizeof(A310_AP_HEADING) - 1) == 0) {
        a310Vars.autopilotHeading = atof(&data[sizeof(A310_AP_HEADING) + 1]);
    }
    else if (strncmp(&data[1], A310_AP_ALTITUDE, sizeof(A310_AP_ALTITUDE) - 1) == 0) {
        a310Vars.autopilotAltitude = atof(&data[sizeof(A310_AP_ALTITUDE) + 1]);
    }
    else if (strncmp(&data[1], A310_AP_VERTICALSPEED, sizeof(A310_AP_VERTICALSPEED) - 1) == 0) {
        a310Vars.autopilotVerticalSpeed = atof(&data[sizeof(A310_AP_VERTICALSPEED) + 1]);
    }
    else if (strncmp(&data[1], A310_AUTOBRAKE, sizeof(A310_AUTOBRAKE) - 1) == 0) {
        simVars.jbAutobrake = atof(&data[sizeof(A310_AUTOBRAKE) + 1]);
    }
    else if (strncmp(&data[1], A310_FLIGHTDIRECTOR, sizeof(A310_FLIGHTDIRECTOR) - 1) == 0) {
        a310Vars.flightDirector = atof(&data[sizeof(A310_FLIGHTDIRECTOR) + 1]);
    }
    else if (strncmp(&data[1], A310_AUTOPILOT, sizeof(A310_AUTOPILOT) - 1) == 0) {
        a310Vars.autopilot = atof(&data[sizeof(A310_AUTOPILOT) + 1]);
    }
    else if (strncmp(&data[1], A310_AUTOTHROTTLE, sizeof(A310_AUTOTHROTTLE) - 1) == 0) {
        a310Vars.autothrottle = atof(&data[sizeof(A310_AUTOTHROTTLE) + 1]);
    }
    else if (strncmp(&data[1], A310_LOCALISER, sizeof(A310_LOCALISER) - 1) == 0) {
        a310Vars.localiser = atof(&data[sizeof(A310_LOCALISER) + 1]);
    }
    else if (strncmp(&data[1], A310_APPROACH, sizeof(A310_APPROACH) - 1) == 0) {
        a310Vars.approach = atof(&data[sizeof(A310_APPROACH) + 1]);
    }
    else if (strncmp(&data[1], A310_ENG_IGNITION, sizeof(A310_ENG_IGNITION) - 1) == 0) {
        // 1 = Start A, 3 = Off
        a310Vars.engineIgnition = atof(&data[sizeof(A310_ENG_IGNITION) + 1]);
    }
    else if (strncmp(&data[1], A310_HEADING_MODE, sizeof(A310_HEADING_MODE) - 1) == 0) {
        simVars.jbManagedHeading = 1 - atof(&data[sizeof(A310_HEADING_MODE) + 1]);
    }
    else if (strncmp(&data[1], A310_PITCH_MODE, sizeof(A310_PITCH_MODE) - 1) == 0) {
        // 7 & 8 = ALL OFF, 6 & 9 = Alt Hold, 2 & 4 = LVL/CH, 3 & 5 & 15 & 20 & 26 = PROFILE
        a310Vars.pitchMode = atof(&data[sizeof(A310_PITCH_MODE) + 1]);
        a310Vars.altHold = (a310Vars.pitchMode == 6 || a310Vars.pitchMode == 9);
        a310Vars.levelChange = (a310Vars.pitchMode == 2 || a310Vars.pitchMode == 4);
        a310Vars.profile = (!a310Vars.altHold && !a310Vars.levelChange && a310Vars.pitchMode != 7 && a310Vars.pitchMode != 8);
    }
    else if (strncmp(&data[1], A310_GEAR_HANDLE, sizeof(A310_GEAR_HANDLE) - 1) == 0) {
        a310Vars.gearHandle = atof(&data[sizeof(A310_GEAR_HANDLE) + 1]);
    }
    else if (strncmp(&data[1], A310_ILS_FREQUENCY, sizeof(A310_ILS_FREQUENCY) - 1) == 0) {
       a310Vars.ilsFrequency = atof(&data[sizeof(A310_ILS_FREQUENCY) + 1]);
    }
    else if (strncmp(&data[1], A310_ILS_COURSE, sizeof(A310_ILS_COURSE) - 1) == 0) {
        a310Vars.ilsCourse = atof(&data[sizeof(A310_ILS_COURSE) + 1]);
    }
}

void updateA320FromJetbridge(const char* data)
{
    if (strncmp(&data[1], A32NX_APU_MASTER_SW, sizeof(A32NX_APU_MASTER_SW) - 1) == 0) {
        simVars.apuMasterSw = atof(&data[sizeof(A32NX_APU_MASTER_SW) + 1]);
    }
    else if (strncmp(&data[1], A32NX_APU_START, sizeof(A32NX_APU_START) - 1) == 0) {
        a320Vars.apuStart = atof(&data[sizeof(A32NX_APU_START) + 1]);
    }
    else if (strncmp(&data[1], A32NX_APU_START_AVAIL, sizeof(A32NX_APU_START_AVAIL) - 1) == 0) {
        a320Vars.apuStartAvail = atof(&data[sizeof(A32NX_APU_START_AVAIL) + 1]);
    }
    else if (strncmp(&data[1], A32NX_APU_BLEED, sizeof(A32NX_APU_BLEED) - 1) == 0) {
        simVars.apuBleed = atof(&data[sizeof(A32NX_APU_BLEED) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ELEC_BAT1, sizeof(A32NX_ELEC_BAT1) - 1) == 0) {
        simVars.elecBat1 = atof(&data[sizeof(A32NX_ELEC_BAT1) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ELEC_BAT2, sizeof(A32NX_ELEC_BAT2) - 1) == 0) {
        simVars.elecBat2 = atof(&data[sizeof(A32NX_ELEC_BAT2) + 1]);
    }
    else if (strncmp(&data[1], A32NX_FLAPS_INDEX, sizeof(A32NX_FLAPS_INDEX) - 1) == 0) {
        a320Vars.flapsIndex = atof(&data[sizeof(A32NX_FLAPS_INDEX) + 1]);
    }
    else if (strncmp(&data[1], A32NX_PARK_BRAKE_POS, sizeof(A32NX_PARK_BRAKE_POS) - 1) == 0) {
        a320Vars.parkBrakePos = atof(&data[sizeof(A32NX_PARK_BRAKE_POS) + 1]);
    }
    else if (strncmp(&data[1], A32NX_XPNDR_MODE, sizeof(A32NX_XPNDR_MODE) - 1) == 0) {
        a320Vars.xpndrMode = atof(&data[sizeof(A32NX_XPNDR_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOPILOT_1, sizeof(A32NX_AUTOPILOT_1) - 1) == 0) {
        a320Vars.autopilot1 = atof(&data[sizeof(A32NX_AUTOPILOT_1) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOPILOT_2, sizeof(A32NX_AUTOPILOT_2) - 1) == 0) {
        a320Vars.autopilot2 = atof(&data[sizeof(A32NX_AUTOPILOT_2) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOTHRUST, sizeof(A32NX_AUTOTHRUST) - 1) == 0) {
        a320Vars.autothrust = atof(&data[sizeof(A32NX_AUTOTHRUST) + 1]);
    }
    else if (strncmp(&data[1], A32NX_TCAS_MODE, sizeof(A32NX_TCAS_MODE) - 1) == 0) {
        simVars.jbTcasMode = atof(&data[sizeof(A32NX_TCAS_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOPILOT_HDG, sizeof(A32NX_AUTOPILOT_HDG) - 1) == 0) {
        a320Vars.autopilotHeading = atof(&data[sizeof(A32NX_AUTOPILOT_HDG) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOPILOT_VS, sizeof(A32NX_AUTOPILOT_VS) - 1) == 0) {
        a320Vars.autopilotVerticalSpeed = atof(&data[sizeof(A32NX_AUTOPILOT_VS) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOPILOT_FPA, sizeof(A32NX_AUTOPILOT_FPA) - 1) == 0) {
        a320Vars.autopilotFpa = atof(&data[sizeof(A32NX_AUTOPILOT_FPA) + 1]);
    }
    else if (strncmp(&data[1], A32NX_MANAGED_SPEED, sizeof(A32NX_MANAGED_SPEED) - 1) == 0) {
        simVars.jbManagedSpeed = atof(&data[sizeof(A32NX_MANAGED_SPEED) + 1]);
    }
    else if (strncmp(&data[1], A32NX_MANAGED_HEADING, sizeof(A32NX_MANAGED_HEADING) - 1) == 0) {
        simVars.jbManagedHeading = atof(&data[sizeof(A32NX_MANAGED_HEADING) + 1]);
    }
    else if (strncmp(&data[1], A32NX_MANAGED_ALTITUDE, sizeof(A32NX_MANAGED_ALTITUDE) - 1) == 0) {
        simVars.jbManagedAltitude = atof(&data[sizeof(A32NX_MANAGED_ALTITUDE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_LATERAL_MODE, sizeof(A32NX_LATERAL_MODE) - 1) == 0) {
        simVars.jbLateralMode = atof(&data[sizeof(A32NX_LATERAL_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_VERTICAL_MODE, sizeof(A32NX_VERTICAL_MODE) - 1) == 0) {
        simVars.jbVerticalMode = atof(&data[sizeof(A32NX_VERTICAL_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_LOC_MODE, sizeof(A32NX_LOC_MODE) - 1) == 0) {
        simVars.jbLocMode = atof(&data[sizeof(A32NX_LOC_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_APPR_MODE, sizeof(A32NX_APPR_MODE) - 1) == 0) {
        simVars.jbApprMode = atof(&data[sizeof(A32NX_APPR_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOTHRUST_MODE, sizeof(A32NX_AUTOTHRUST_MODE) - 1) == 0) {
        simVars.jbAutothrustMode = atof(&data[sizeof(A32NX_AUTOTHRUST_MODE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_AUTOBRAKE, sizeof(A32NX_AUTOBRAKE) - 1) == 0) {
        simVars.jbAutobrake = atof(&data[sizeof(A32NX_AUTOBRAKE) + 1]);
    }
    else if (strncmp(&data[1], A32NX_LEFT_BRAKEPEDAL, sizeof(A32NX_LEFT_BRAKEPEDAL) - 1) == 0) {
        a320Vars.leftBrakePedal = atof(&data[sizeof(A32NX_LEFT_BRAKEPEDAL) + 1]);
    }
    else if (strncmp(&data[1], A32NX_SPOILERS_HANDLE_POS, sizeof(A32NX_SPOILERS_HANDLE_POS) - 1) == 0) {
        a320Vars.spoilersHandlePos = atof(&data[sizeof(A32NX_SPOILERS_HANDLE_POS) + 1]);
    }
    else if (strncmp(&data[1], A32NX_RIGHT_BRAKEPEDAL, sizeof(A32NX_RIGHT_BRAKEPEDAL) - 1) == 0) {
        a320Vars.rightBrakePedal = atof(&data[sizeof(A32NX_RIGHT_BRAKEPEDAL) + 1]);
    }
    else if (strncmp(&data[1], A32NX_RUDDER_PEDAL_POS, sizeof(A32NX_RUDDER_PEDAL_POS) - 1) == 0) {
        a320Vars.rudderPedalPos = atof(&data[sizeof(A32NX_RUDDER_PEDAL_POS) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ENGINE_EGT1, sizeof(A32NX_ENGINE_EGT1) - 1) == 0) {
        a320Vars.engineEgt1 = atof(&data[sizeof(A32NX_ENGINE_EGT1) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ENGINE_EGT2, sizeof(A32NX_ENGINE_EGT2) - 1) == 0) {
        a320Vars.engineEgt2 = atof(&data[sizeof(A32NX_ENGINE_EGT2) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ENGINE_FUEL_FLOW1, sizeof(A32NX_ENGINE_FUEL_FLOW1) - 1) == 0) {
        a320Vars.engineFuelFlow1 = atof(&data[sizeof(A32NX_ENGINE_FUEL_FLOW1) + 1]);
    }
    else if (strncmp(&data[1], A32NX_ENGINE_FUEL_FLOW2, sizeof(A32NX_ENGINE_FUEL_FLOW2) - 1) == 0) {
        a320Vars.engineFuelFlow2 = atof(&data[sizeof(A32NX_ENGINE_FUEL_FLOW2) + 1]);
    }
}

bool jetbridgeA310ButtonPress(int eventId, double value)
{
    switch (eventId) {
    case KEY_APU_OFF_SWITCH:
        writeJetbridgeVar(A310_APU_MASTER_SW, value);
        return true;
    case KEY_APU_STARTER:
        writeJetbridgeVar(A310_APU_START, value);
        return true;
    case KEY_BLEED_AIR_SOURCE_CONTROL_SET:
        writeJetbridgeVar(A310_APU_BLEED, value);
        return true;
    case KEY_ELEC_BAT1:
        writeJetbridgeVar(A310_ELEC_BAT1, value);
        return true;
    case KEY_ELEC_BAT2:
        writeJetbridgeVar(A310_ELEC_BAT2, value);
        writeJetbridgeVar(A310_ELEC_BAT3, value);
        return true;
    case KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE:
        // Toggle
        writeJetbridgeVar(A310_SEATBELTS_SWITCH, 1 - a310Vars.seatbeltsSwitch);
        return true;
    case KEY_BEACON_LIGHTS_SET:
        // Off = 0, On = 1
        a310Vars.beaconLights = value;
        return true;
    case KEY_LANDING_LIGHTS_SET:
        // On = 0, Off = 2
        a310Vars.landingLights = 2 - value * 2;
        return true;
    case KEY_TAXI_LIGHTS_SET:
        // T.O. = 0, Taxi = 1, Off = 2
        a310Vars.taxiLights = 2 - value * 2;
        // Off = 0, On = 1
        a310Vars.turnoffLights = value;
        return true;
    case KEY_NAV_LIGHTS_SET:
        // On = 0, Off = 2
        a310Vars.navLights = 2 - value * 2;
        return true;
    case KEY_STROBES_SET:
        // On = 0, Off = 2
        a310Vars.strobes = 2 - value * 2;
        return true;
    case KEY_XPNDR_HIGH_SET:
        if (value == 1) {
            writeJetbridgeVar(A310_XPNDR_HIGH_INC, 1);
        }
        else {
            writeJetbridgeVar(A310_XPNDR_HIGH_DEC, 1);
        }
        return true;
    case KEY_XPNDR_LOW_SET:
        if (value == 1) {
            writeJetbridgeVar(A310_XPNDR_LOW_INC, 1);
        }
        else {
            writeJetbridgeVar(A310_XPNDR_LOW_DEC, 1);
        }
        return true;
    case KEY_XPNDR_STATE:
        // Set TCAS mode to standby or TA/RA
        writeJetbridgeVar(A310_TCAS_MODE, value);
        return true;
    case KEY_AP_SPD_VAR_SET:
        writeJetbridgeVar(A310_AP_AIRSPEED, value);
        return true;
    case KEY_AP_AIRSPEED_ON:
    case KEY_AP_MACH_OFF:
        writeJetbridgeVar(A310_AP_IS_MACH, 0);
        return true;
    case KEY_AP_AIRSPEED_OFF:
    case KEY_AP_MACH_ON:
        writeJetbridgeVar(A310_AP_IS_MACH, 1);
        return true;
    case KEY_HEADING_BUG_SET:
        writeJetbridgeVar(A310_AP_HEADING, value);
        return true;
    case KEY_AP_ALT_VAR_SET_ENGLISH:
        writeJetbridgeVar(A310_AP_ALTITUDE, value);
        return true;
    case KEY_AP_VS_VAR_SET_ENGLISH:
        writeJetbridgeVar(A310_AP_VERTICALSPEED, value);
        return true;
    case KEY_AUTOBRAKE:
        writeJetbridgeVar(A310_AUTOBRAKE, value);
        return true;
    case KEY_GEAR_SET:
        // 0 = Up, 1 = Neutral, 2 = Down
        writeJetbridgeVar(A310_GEAR_HANDLE, value * 2);
        return true;
    case KEY_TOGGLE_FLIGHT_DIRECTOR:
        writeJetbridgeVar(A310_FLIGHTDIRECTOR, value);
        return true;
    case KEY_AP_MASTER:
        if (value == 0) {
            writeJetbridgeVar(A310_AUTOPILOT_OFF, 1);
        }
        else {
            writeJetbridgeVar(A310_AUTOPILOT_ON, 1);
        }
        return true;
    case KEY_AUTO_THROTTLE_ARM:
        writeJetbridgeVar(A310_AUTOTHROTTLE, value);
        return true;
    case KEY_AP_LOC_HOLD:
        writeJetbridgeVar(A310_LOCALISER_TOGGLE, 1);
        return true;
    case KEY_AP_APR_HOLD_ON:
    case KEY_AP_APR_HOLD_OFF:
        writeJetbridgeVar(A310_APPROACH_TOGGLE, 1);
        return true;
    case KEY_AP_SPEED_SLOT_INDEX_SET:
        // Turn PROFILE on/off
        if (a310Vars.profile != value - 1) {
            writeJetbridgeVar(A310_PROFILE_TOGGLE, 1);
        }
        return true;
    case KEY_AP_VS_SLOT_INDEX_SET:
    case KEY_AP_VS_SET:
        // Turn VS on (by turning everything else off)
        if (a310Vars.altHold) {
            // ALT HOLD off
            writeJetbridgeVar(A310_ALT_HOLD_TOGGLE, 1);
        }
        if (a310Vars.levelChange) {
            // LVL/CH off
            writeJetbridgeVar(A310_LEVEL_CHANGE_TOGGLE, 1);
        }
        if (a310Vars.profile) {
            // PROFILE off
            writeJetbridgeVar(A310_PROFILE_TOGGLE, 1);
        }
        return true;
    case KEY_AP_HEADING_SLOT_INDEX_SET:
        if (value == 1) {
            writeJetbridgeVar(A310_SELECTED_HEADING, 1);
        }
        else if (value == 2) {
            writeJetbridgeVar(A310_MANAGED_HEADING, 1);
        }
        return true;
    case KEY_NAV1_STBY_SET:
    {
        int mhz = int(value);
        int khz = int((0.005 + value - mhz) * 100);
        writeJetbridgeVar(A310_ILS_SET_FREQUENCY_MHZ, mhz);
        writeJetbridgeVar(A310_ILS_SET_FREQUENCY_KHZ, khz);
        return true;
    }
    case KEY_VOR1_SET:
        writeJetbridgeVar(A310_ILS_COURSE, value);
        return true;
    }

    return false;
}

bool jetbridgeA320ButtonPress(int eventId, double value)
{
    switch (eventId) {
    case KEY_APU_OFF_SWITCH:
        writeJetbridgeVar(A32NX_APU_MASTER_SW, value);
        return true;
    case KEY_APU_STARTER:
        writeJetbridgeVar(A32NX_APU_START, value);
        return true;
    case KEY_BLEED_AIR_SOURCE_CONTROL_SET:
        writeJetbridgeVar(A32NX_APU_BLEED, value);
        return true;
    case KEY_ELEC_BAT1:
        writeJetbridgeVar(A32NX_ELEC_BAT1, value);
        return true;
    case KEY_ELEC_BAT2:
        writeJetbridgeVar(A32NX_ELEC_BAT2, value);
        return true;
    case KEY_AUTOBRAKE:
        writeJetbridgeVar(A32NX_AUTOBRAKE, value);
        return true;
    case KEY_XPNDR_STATE:
        // Set transponder to standby or auto
        writeJetbridgeVar(A32NX_XPNDR_MODE, value);
        return true;
    }

    return false;
}

bool jetbridgeK100ButtonPress(int eventId, double value)
{
    switch (eventId) {
    case KEY_TANK_SELECT_1:
        writeJetbridgeVar(K100_TANK_SELECTOR_1, value);
        return true;
    case KEY_TANK_SELECT_2:
        writeJetbridgeVar(K100_TANK_SELECTOR_2, value);
        return true;
    case KEY_LANDING_LIGHTS_SET:
        // 0 = off, 2 = on
        writeJetbridgeVar(K100_LANDING_LIGHT, value * 2);
        return true;
    }

    return false;
}

void readA310Jetbridge()
{
    readJetbridgeVar(A310_APU_MASTER_SW);
    readJetbridgeVar(A310_APU_START);
    readJetbridgeVar(A310_APU_START_AVAIL);
    readJetbridgeVar(A310_APU_BLEED);
    readJetbridgeVar(A310_ELEC_BAT1);
    readJetbridgeVar(A310_ELEC_BAT2);
    readJetbridgeVar(A310_SEATBELTS_SWITCH);
    readJetbridgeVar(A310_BEACON_LIGHTS);
    readJetbridgeVar(A310_LANDING_LIGHTS_L);
    readJetbridgeVar(A310_TAXI_LIGHTS);
    readJetbridgeVar(A310_TURNOFF_LIGHTS_L);
    readJetbridgeVar(A310_NAV_LIGHTS);
    readJetbridgeVar(A310_STROBES);
    readJetbridgeVar(A310_PITCH_TRIM1);
    readJetbridgeVar(A310_PITCH_TRIM2);
    readJetbridgeVar(A310_TCAS_MODE);
    readJetbridgeVar(A310_AP_AIRSPEED);
    readJetbridgeVar(A310_AP_IS_MACH);
    readJetbridgeVar(A310_AP_HEADING);
    readJetbridgeVar(A310_AP_ALTITUDE);
    readJetbridgeVar(A310_AP_VERTICALSPEED);
    readJetbridgeVar(A310_AUTOBRAKE);
    readJetbridgeVar(A310_FLIGHTDIRECTOR);
    readJetbridgeVar(A310_AUTOPILOT);
    readJetbridgeVar(A310_AUTOTHROTTLE);
    readJetbridgeVar(A310_LOCALISER);
    readJetbridgeVar(A310_APPROACH);
    readJetbridgeVar(A310_ENG_IGNITION);
    readJetbridgeVar(A310_HEADING_MODE);
    readJetbridgeVar(A310_PITCH_MODE);
    readJetbridgeVar(A310_GEAR_HANDLE);
    readJetbridgeVar(A310_ILS_FREQUENCY);
    readJetbridgeVar(A310_ILS_COURSE);
}

void readA320Jetbridge()
{
    readJetbridgeVar(A32NX_APU_MASTER_SW);
    readJetbridgeVar(A32NX_APU_START);
    readJetbridgeVar(A32NX_APU_START_AVAIL);
    readJetbridgeVar(A32NX_APU_BLEED);
    readJetbridgeVar(A32NX_ELEC_BAT1);
    readJetbridgeVar(A32NX_ELEC_BAT2);
    readJetbridgeVar(A32NX_FLAPS_INDEX);
    readJetbridgeVar(A32NX_PARK_BRAKE_POS);
    readJetbridgeVar(A32NX_SPOILERS_HANDLE_POS);
    readJetbridgeVar(A32NX_XPNDR_MODE);
    readJetbridgeVar(A32NX_AUTOPILOT_1);
    readJetbridgeVar(A32NX_AUTOPILOT_2);
    readJetbridgeVar(A32NX_AUTOTHRUST);
    readJetbridgeVar(A32NX_TCAS_MODE);
    readJetbridgeVar(A32NX_MANAGED_SPEED);
    readJetbridgeVar(A32NX_MANAGED_HEADING);
    readJetbridgeVar(A32NX_MANAGED_ALTITUDE);
    readJetbridgeVar(A32NX_LATERAL_MODE);
    readJetbridgeVar(A32NX_VERTICAL_MODE);
    readJetbridgeVar(A32NX_LOC_MODE);
    readJetbridgeVar(A32NX_APPR_MODE);
    readJetbridgeVar(A32NX_AUTOTHRUST_MODE);
    readJetbridgeVar(A32NX_AUTOBRAKE);
    readJetbridgeVar(A32NX_LEFT_BRAKEPEDAL);
    readJetbridgeVar(A32NX_RIGHT_BRAKEPEDAL);
    readJetbridgeVar(A32NX_RUDDER_PEDAL_POS);
    readJetbridgeVar(A32NX_AUTOPILOT_HDG);
    readJetbridgeVar(A32NX_AUTOPILOT_VS);
    readJetbridgeVar(A32NX_AUTOPILOT_FPA);
    readJetbridgeVar(A32NX_ENGINE_EGT1);
    readJetbridgeVar(A32NX_ENGINE_EGT2);
    readJetbridgeVar(A32NX_ENGINE_FUEL_FLOW1);
    readJetbridgeVar(A32NX_ENGINE_FUEL_FLOW2);
}

#endif  // jetbridgeFallback
