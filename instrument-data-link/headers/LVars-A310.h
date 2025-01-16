#ifndef _LVARS_A310_H_
#define _LVARS_A310_H_

#include <stdio.h>

// LVars for iniBuilds Airbus A310
const char A310_APU_MASTER_SW[] = "L:A310_apu_master_switch, bool";
const char A310_APU_START[] = "L:A310_apu_start_button, bool";
const char A310_APU_START_AVAIL[] = "L:A310_apu_available, bool";
const char A310_APU_BLEED[] = "L:A310_apu_bleed, bool";
const char A310_ELEC_BAT1[] = "L:A310_bat1_on, bool";
const char A310_ELEC_BAT2[] = "L:A310_bat2_on, bool";
const char A310_ELEC_BAT3[] = "L:A310_bat3_on, bool";
const char A310_SEATBELTS_SWITCH[] = "L:A310_seatbelts_switch, bool";
const char A310_BEACON_LIGHTS[] = "L:A310_BEACON_LIGHT_SWITCH, number";
const char A310_LANDING_LIGHTS_L[] = "L:A310_LANDING_LIGHT_L_SWITCH, number";
const char A310_LANDING_LIGHTS_R[] = "L:A310_LANDING_LIGHT_R_SWITCH, number";
const char A310_TAXI_LIGHTS[] = "L:A310_TAXI_LIGHTS_SWITCH, number";
const char A310_TURNOFF_LIGHTS_L[] = "L:A310_RWY_TURNOFF_L_SWITCH, number";
const char A310_TURNOFF_LIGHTS_R[] = "L:A310_RWY_TURNOFF_R_SWITCH, number";
const char A310_NAV_LIGHTS[] = "L:A310_NAV_LOGO_LIGHT_SWITCH, number";
const char A310_STROBES[] = "L:A310_POTENTIOMETER_24, number";
const char A310_PITCH_TRIM1[] = "L:A310_PITCH_TRIM1, number";
const char A310_PITCH_TRIM2[] = "L:A310_PITCH_TRIM2, number";
const char A310_TCAS_MODE[] = "L:A310_TCAS_MODE_PEDESTAL, number";
const char A310_XPNDR_HIGH_INC[] = "L:A310_TRANSPONDER_OUTER_KNOB_TURNED_CLOCKWISE, number";
const char A310_XPNDR_HIGH_DEC[] = "L:A310_TRANSPONDER_OUTER_KNOB_TURNED_ANTICLOCKWISE, number";
const char A310_XPNDR_LOW_INC[] = "L:A310_TRANSPONDER_INNER_KNOB_TURNED_CLOCKWISE, number";
const char A310_XPNDR_LOW_DEC[] = "L:A310_TRANSPONDER_INNER_KNOB_TURNED_ANTICLOCKWISE, number";
const char A310_AP_AIRSPEED[] = "L:A310_AIRSPEED_DIAL, number";
const char A310_AP_IS_MACH[] = "L:A310_AIRSPEED_IS_MACH, number";
const char A310_AP_HEADING[] = "L:A310_HEADING_DIAL, number";
const char A310_AP_ALTITUDE[] = "L:A310_ALTITUDE_DIAL, number";
const char A310_AP_VERTICALSPEED[] = "L:A310_VVI_DIAL, number";
const char A310_AUTOBRAKE[] = "L:A310_AUTOBRAKE_LEVEL, Number";
const char A310_GEAR_HANDLE[] = "L:A310_GEAR_HANDLE_STATUS, Number";
const char A310_FLIGHTDIRECTOR[] = "L:A310_FPV_ON, Number";
const char A310_AUTOPILOT[] = "L:A310_AP_ON, Number";
const char A310_AUTOPILOT_ON[] = "L:A310_AP1_BUTTON, Number";
const char A310_AUTOPILOT_OFF[] = "L:A310_AP_DISCONNECT_BUTTON, Number";
const char A310_AUTOTHROTTLE[] = "L:A310_AT_ON, Number";
const char A310_LOCALISER[] = "L:A310_LOCALIZER_BUTTON, Number";
const char A310_LOCALISER_TOGGLE[] = "L:AP6_BUTTON, Number";
const char A310_APPROACH[] = "L:A310_APPROACH_BUTTON, Number";
const char A310_APPROACH_TOGGLE[] = "L:AP7_BUTTON, Number";
const char A310_ENG_IGNITION[] = "L:A310_ENG_IGNITION_SWITCH, Number";
const char A310_ENG1_STARTER[] = "L:A310_ENG1_STARTER, Number";
const char A310_ENG2_STARTER[] = "L:A310_ENG2_STARTER, Number";
const char A310_PITCH_MODE[] = "L:A310_PITCH_MODE, Number";
const char A310_ALT_HOLD_TOGGLE[] = "L:AP9_BUTTON, Number";
const char A310_LEVEL_CHANGE_TOGGLE[] = "L:AP2_BUTTON, Number";
const char A310_PROFILE_TOGGLE[] = "L:AP3_BUTTON, Number";
const char A310_HEADING_MODE[] = "L:A310_MCU_HDG_SEL_LIGHT, Number";
const char A310_SELECTED_HEADING[] = "L:A310_FCU_SELECTED_HEADING_BUTTON, Number";
const char A310_MANAGED_HEADING[] = "L:A310_FCU_MANAGED_HEADING_BUTTON, Number";
const char A310_ILS_FREQUENCY[] = "L:A310_ILS_FREQUENCY, Number";
const char A310_ILS_SET_FREQUENCY_MHZ[] = "L:A310_ILS_FREQUENCY_MHZ, Number";
const char A310_ILS_SET_FREQUENCY_KHZ[] = "L:A310_ILS_FREQUENCY_KHZ, Number";
const char A310_ILS_COURSE[] = "L:A310_ILS_COURSE, Number";
const char A310_ENG1_ANTI_ICE[] = "L:A310_ENG1_ANTI_ICE, Number";
const char A310_ENG2_ANTI_ICE[] = "L:A310_ENG2_ANTI_ICE, Number";
const char A310_WING_ANTI_ICE[] = "L:A310_WING_ANTI_ICE, Number";


struct LVars_A310
{
    double apuStart = 0;
    double apuStartAvail = 0;
    double seatbeltsSwitch;
    double beaconLights = 0;
    double landingLights = 2;
    double taxiLights = 2;
    double turnoffLights = 0;
    double navLights = 2;
    double strobes = 2;
    double pitchTrim1 = 0;
    double pitchTrim2 = 0;
    double autopilotAirspeed = 0;
    double autopilotHeading = 0;
    double autopilotAltitude = 0;
    double autopilotVerticalSpeed = 0;
    double flightDirector = 0;
    double autopilot = 0;
    double autothrottle = 0;
    double localiser = 0;
    double approach = 0;
    double engineIgnition = 0;
    double pitchMode = 0;
    bool altHold = false;
    bool levelChange = false;
    bool profile = false;
    double gearHandle = 1;
    double ilsFrequency = 0;
    double ilsCourse = 0;
};

#endif // _LVARS_A310_H_
