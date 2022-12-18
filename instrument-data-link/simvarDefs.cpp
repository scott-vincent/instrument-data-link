#include <stdio.h>
#include "simvarDefs.h"

const char* versionString = "v1.7.2";

const char* SimVarDefs[][2] = {
    // Vars for Jetbridge (must come first)
    { "Apu Master Sw", "jetbridge" },
    { "Apu Bleed", "jetbridge" },
    { "Elec Bat1", "jetbridge" },
    { "Elec Bat2", "jetbridge" },
    { "Autopilot Managed Speed", "jetbridge" },
    { "Autopilot Managed Heading", "jetbridge" },
    { "Autopilot Managed Altitude", "jetbridge" },
    { "Lateral Mode", "jetbridge" },
    { "Vertical Mode", "jetbridge" },
    { "Loc Mode", "jetbridge" },
    { "Appr Mode", "jetbridge" },
    { "Autothrust Mode", "jetbridge" },
    { "Show Mach", "jetbridge" },
    { "Autobrake Armed", "jetbridge" },
    { "Pitch Trim", "jetbridge" },
    { "Tcas Mode", "jetbridge" },

    // Vars required for all panels (screensaver, aircraft identification etc.)
    { "Title", "string32" },
    { "Estimated Cruise Speed", "knots" },
    { "Electrical Main Bus Voltage", "volts" },
    { "Electrical Battery Load", "amperes" },

    // Vars for Power/Lights panel
    { "Light On States", "mask" },
    { "Flaps Num Handle Positions", "number" },
    { "Flaps Handle Index", "number" },
    { "Brake Parking Position", "bool" },
    { "Pushback State", "enum" },
    { "Apu Switch", "bool" },
    { "Apu Pct Rpm", "percent" },

    // Vars for Radio panel
    { "Com Status:1", "enum" },
    { "Com Transmit:1", "bool" },
    { "Com Active Frequency:1", "mhz" },
    { "Com Standby Frequency:1", "mhz" },
    { "Nav Active Frequency:1", "mhz" },
    { "Nav Standby Frequency:1", "mhz" },
    { "Com Status:2", "enum" },
    { "Com Transmit:2", "bool" },
    { "Com Active Frequency:2", "mhz" },
    { "Com Standby Frequency:2", "mhz" },
    { "Nav Active Frequency:2", "mhz" },
    { "Nav Standby Frequency:2", "mhz" },
    { "Com Receive:1", "bool" },
    { "Com Receive:2", "bool" },
    { "Adf Active Frequency:1", "khz" },
    { "Adf Standby Frequency:1", "khz" },
    { "Cabin Seatbelts Alert Switch", "bool" },
    { "Transponder State:1", "enum" },
    { "Transponder Code:1", "bco16" },

    // Vars for Autopilot panel
    { "Indicated Altitude", "feet" },
    { "Airspeed Indicated", "knots" },
    { "Airspeed Mach", "mach" },
    { "Plane Heading Degrees Magnetic", "degrees" },
    { "Vertical Speed", "feet per second" },
    { "Autopilot Available", "bool" },
    { "Autopilot Master", "bool" },
    { "Autopilot Flight Director Active", "bool" },
    { "Autopilot Heading Lock Dir", "degrees" },
    { "Autopilot Heading Lock", "bool" },
    { "Autopilot Heading Slot Index", "number" },
    { "Autopilot Wing Leveler", "bool" },
    { "Autopilot Altitude Lock Var", "feet" },
    { "Autopilot Altitude Lock Var:3", "feet" },
    { "Autopilot Altitude Lock", "bool" },
    { "Autopilot Pitch Hold", "bool" },
    { "Autopilot Vertical Hold Var", "feet/minute" },
    { "Autopilot Vertical Hold", "bool" },
    { "Autopilot VS Slot Index", "number" },
    { "Autopilot Airspeed Hold Var", "knots" },
    { "Autopilot Mach Hold Var", "number" },
    { "Autopilot Airspeed Hold", "bool" },
    { "Autopilot Approach Hold", "bool" },
    { "Autopilot Glideslope Hold", "bool" },
    { "General Eng Throttle Lever Position:1", "percent" },
    { "Autothrottle Active", "bool" },

    // Remaining vars for Instrument panel
    { "Kohlsman Setting Hg", "inHg" },
    { "Attitude Indicator Pitch Degrees", "degrees" },
    { "Attitude Indicator Bank Degrees", "degrees" },
    { "Airspeed True", "knots" },
    { "Airspeed True Calibrate", "degrees" },
    { "Plane Heading Degrees True", "degrees" },
    { "Plane Alt Above Ground", "feet" },
    { "Turn Indicator Rate", "radians per second" },
    { "Turn Coordinator Ball", "position" },
    { "Elevator Trim Position", "degrees" },
    { "Rudder Trim Pct", "percent" },
    { "Spoilers Handle Position", "percent" },
    { "Auto Brake Switch Cb", "number" },
    { "Zulu Time", "seconds" },
    { "Local Time", "seconds" },
    { "Absolute Time", "seconds" },
    { "Ambient Temperature", "celsius" },
    { "Number Of Engines", "number" },
    { "General Eng Rpm:1", "rpm" },
    { "Eng Rpm Animation Percent:1", "percent" },
    { "General Eng Elapsed Time:1", "hours" },
    { "Fuel Total Capacity", "gallons" },
    { "Fuel Total Quantity", "gallons" },
    { "Fuel Tank Left Main Level", "percent" },
    { "Fuel Tank Right Main Level", "percent" },
    { "Nav Obs:1", "degrees" },
    { "Nav Radial Error:1", "degrees" },
    { "Nav Glide Slope Error:1", "degrees" },
    { "Nav ToFrom:1", "enum" },
    { "Nav Gs Flag:1", "bool" },
    { "Nav Obs:2", "degrees" },
    { "Nav Radial Error:2", "degrees" },
    { "Nav ToFrom:2", "enum" },
    { "Nav Has Localizer:1", "bool" },
    { "Nav Localizer:1", "degrees" },
    { "Gps Drives Nav1", "bool" },
    { "Gps Wp Cross Trk", "meters" },
    { "Adf Radial:1", "degrees" },
    { "Adf Card", "degrees" },
    { "Is Gear Retractable", "bool" },
    { "Gear Left Position", "percent" },
    { "Gear Center Position", "percent" },
    { "Gear Right Position", "percent" },
    { "Rudder Position", "position" },
    { "Brake Left Position", "percent" },
    { "Brake Right Position", "percent" },
    { "General Eng Oil Temperature:1", "fahrenheit" },
    { "General Eng Oil Temperature:2", "fahrenheit" },
    { "General Eng Oil Temperature:3", "fahrenheit" },
    { "General Eng Oil Temperature:4", "fahrenheit" },
    { "General Eng Oil Pressure:1", "psi" },
    { "General Eng Oil Pressure:2", "psi" },
    { "General Eng Oil Pressure:3", "psi" },
    { "General Eng Oil Pressure:4", "psi" },
    { "General Eng Exhaust Gas Temperature:1", "celsius" },
    { "General Eng Exhaust Gas Temperature:2", "celsius" },
    { "General Eng Exhaust Gas Temperature:3", "celsius" },
    { "General Eng Exhaust Gas Temperature:4", "celsius" },
    { "Engine Type", "enum" },
    { "Max Rated Engine RPM", "rpm" },
    { "Turb Eng N1:1", "percent" },
    { "Turb Eng N1:2", "percent" },
    { "Turb Eng N1:3", "percent" },
    { "Turb Eng N1:4", "percent" },
    { "Prop RPM:1", "rpm" },
    { "Eng Manifold Pressure:1", "inches of mercury" },
    { "Eng Fuel Flow GPH:1", "gallons per hour" },
    { "Eng Fuel Flow GPH:2", "gallons per hour" },
    { "Eng Fuel Flow GPH:3", "gallons per hour" },
    { "Eng Fuel Flow GPH:4", "gallons per hour" },
    { "Suction Pressure", "inches of mercury" },
    { "Sim On Ground", "bool" },
    { "G Force", "gforce"},
    { "Atc Id", "string32" },
    { "Atc Airline", "string32" },
    { "Atc Flight Number", "string32" },
    { "Atc Heavy", "bool" },
    // Internal variables must come last
    { "Landing Rate", "internal" },
    { "Skytrack State", "internal" },
    { NULL, NULL }
};

WriteEvent WriteEvents[] = {
    { KEY_CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE, "CABIN_SEATBELTS_ALERT_SWITCH_TOGGLE" },
    { KEY_TRUE_AIRSPEED_CAL_SET, "TRUE_AIRSPEED_CAL_SET" },
    { KEY_KOHLSMAN_SET, "KOHLSMAN_SET" },
    { KEY_HEADING_GYRO_SET, "HEADING_GYRO_SET" },
    { KEY_VOR1_SET, "VOR1_SET" },
    { KEY_VOR2_SET, "VOR2_SET" },
    { KEY_ELEV_TRIM_UP, "ELEV_TRIM_UP" },
    { KEY_ELEV_TRIM_DN, "ELEV_TRIM_DN" },
    { KEY_FLAPS_INCR, "FLAPS_INCR" },
    { KEY_FLAPS_DECR, "FLAPS_DECR" },
    { KEY_FLAPS_UP, "FLAPS_UP" },
    { KEY_FLAPS_SET, "FLAPS_SET" },
    { KEY_FLAPS_DOWN, "FLAPS_DOWN" },
    { KEY_SPOILERS_ARM_SET, "SPOILERS_ARM_SET" },
    { KEY_SPOILERS_OFF, "SPOILERS_OFF" },
    { KEY_SPOILERS_SET, "SPOILERS_SET" },
    { KEY_SPOILERS_ON, "SPOILERS_ON" },
    { KEY_GEAR_SET, "GEAR_SET" },
    { KEY_ADF_CARD_SET, "ADF_CARD_SET" },
    { KEY_COM1_VOLUME_SET, "COM1_VOLUME_SET" },
    { KEY_COM1_TRANSMIT_SELECT, "COM1_TRANSMIT_SELECT" },
    { KEY_COM1_RECEIVE_SELECT, "COM1_RECEIVE_SELECT" },
    { KEY_COM1_STBY_RADIO_SET, "COM_STBY_RADIO_SET" },
    { KEY_COM1_RADIO_FRACT_INC, "COM_RADIO_FRACT_INC" },
    { KEY_COM1_RADIO_SWAP, "COM1_RADIO_SWAP" },
    { KEY_COM2_VOLUME_SET, "COM2_VOLUME_SET" },
    { KEY_COM2_TRANSMIT_SELECT, "COM2_TRANSMIT_SELECT" },
    { KEY_COM2_RECEIVE_SELECT, "COM2_RECEIVE_SELECT" },
    { KEY_COM2_STBY_RADIO_SET, "COM2_STBY_RADIO_SET" },
    { KEY_COM2_RADIO_FRACT_INC, "COM2_RADIO_FRACT_INC" },
    { KEY_COM2_RADIO_SWAP, "COM2_RADIO_SWAP" },
    { KEY_NAV1_STBY_SET, "NAV1_STBY_SET" },
    { KEY_NAV1_RADIO_SWAP, "NAV1_RADIO_SWAP" },
    { KEY_NAV2_STBY_SET, "NAV2_STBY_SET" },
    { KEY_NAV2_RADIO_SWAP, "NAV2_RADIO_SWAP" },
    { KEY_ADF_STBY_SET, "ADF_STBY_SET" },
    { KEY_ADF1_RADIO_SWAP, "ADF1_RADIO_SWAP" },
    { KEY_XPNDR_SET, "XPNDR_SET" },
    { KEY_XPNDR_HIGH_SET, "XPNDR_HIGH_SET" },
    { KEY_XPNDR_LOW_SET, "XPNDR_LOW_SET" },
    { KEY_XPNDR_STATE, "XPNDR_STATE" },
    { KEY_AP_MASTER, "AP_MASTER" },
    { KEY_TOGGLE_FLIGHT_DIRECTOR, "TOGGLE_FLIGHT_DIRECTOR" },
    { KEY_AP_SPD_VAR_SET, "AP_SPD_VAR_SET" },
    { KEY_AP_MACH_VAR_SET, "AP_MACH_VAR_SET" },
    { KEY_HEADING_BUG_SET, "HEADING_BUG_SET" },
    { KEY_AP_ALT_VAR_SET_ENGLISH, "AP_ALT_VAR_SET_ENGLISH" },
    { KEY_AP_VS_VAR_SET_ENGLISH, "AP_VS_VAR_SET_ENGLISH" },
    { KEY_AP_AIRSPEED_ON, "AP_AIRSPEED_ON" },
    { KEY_AP_AIRSPEED_OFF, "AP_AIRSPEED_OFF" },
    { KEY_AP_MACH_ON, "AP_MACH_ON" },
    { KEY_AP_MACH_OFF, "AP_MACH_OFF" },
    { KEY_AP_HDG_HOLD_ON, "AP_HDG_HOLD_ON" },
    { KEY_AP_HDG_HOLD_OFF, "AP_HDG_HOLD_OFF" },
    { KEY_AP_ALT_HOLD_ON, "AP_ALT_HOLD_ON" },
    { KEY_AP_ALT_HOLD_OFF, "AP_ALT_HOLD_OFF" },
    { KEY_AP_LOC_HOLD, "AP_LOC_HOLD" },
    { KEY_AP_APR_HOLD_ON, "AP_APR_HOLD_ON" },
    { KEY_AP_APR_HOLD_OFF, "AP_APR_HOLD_OFF" },
    { KEY_AP_PANEL_ALTITUDE_ON, "AP_PANEL_ALTITUDE_ON" },
    { KEY_AUTO_THROTTLE_ARM, "AUTO_THROTTLE_ARM" },
    { KEY_AP_HEADING_SLOT_INDEX_SET, "AP_HEADING_SLOT_INDEX_SET" },
    { KEY_AP_SPEED_SLOT_INDEX_SET, "AP_SPEED_SLOT_INDEX_SET" },
    { KEY_AP_VS_SLOT_INDEX_SET, "AP_VS_SLOT_INDEX_SET" },
    { KEY_AP_VS_SET, "AP_VS_SET" },
    { KEY_TUG_HEADING, "KEY_TUG_HEADING" },
    { KEY_ELEC_BAT1, "ELEC BAT1" },
    { KEY_ELEC_BAT2, "ELEC BAT2" },
    { KEY_TOGGLE_MASTER_BATTERY, "TOGGLE_MASTER_BATTERY" },
    { KEY_TOGGLE_MASTER_ALTERNATOR, "TOGGLE_MASTER_ALTERNATOR" },
    { KEY_TOGGLE_JETWAY, "TOGGLE_JETWAY" },
    { KEY_FUEL_PUMP, "FUEL_PUMP" },
    { KEY_CABIN_LIGHTS_SET, "CABIN_LIGHTS_SET" },
    { KEY_BEACON_LIGHTS_SET, "BEACON_LIGHTS_SET" },
    { KEY_LANDING_LIGHTS_SET, "LANDING_LIGHTS_SET" },
    { KEY_TAXI_LIGHTS_SET, "TAXI_LIGHTS_SET" },
    { KEY_NAV_LIGHTS_SET, "NAV_LIGHTS_SET" },
    { KEY_STROBES_SET, "STROBES_SET" },
    { KEY_PITOT_HEAT_SET, "PITOT_HEAT_SET" },
    { KEY_ANTI_ICE_SET, "ANTI_ICE_SET" },
    { KEY_WINDSHIELD_DEICE_SET, "WINDSHIELD_DEICE_SET" },
    { KEY_AVIONICS_MASTER_SET, "AVIONICS_MASTER_SET" },
    { KEY_APU_OFF_SWITCH, "APU_OFF_SWITCH" },
    { KEY_APU_STARTER, "APU_STARTER" },
    { KEY_BLEED_AIR_SOURCE_CONTROL_SET, "BLEED_AIR_SOURCE_CONTROL_SET" },
    { KEY_TOGGLE_PUSHBACK, "TOGGLE_PUSHBACK" },
    { KEY_PARKING_BRAKE_SET, "PARKING_BRAKE_SET" },
    { KEY_AUTOBRAKE, "AUTOBRAKE" },
    { KEY_TANK_SELECT_1, "TANK_SELECT_1" },
    { KEY_TANK_SELECT_2, "TANK_SELECT_2" },
    { KEY_ENG_CRANK, "ENGINE_CRANK" },
    { KEY_SKYTRACK_STATE, "SKYTRACK_STATE" },
    { KEY_G1000_PFD_SOFTKEY_1, "MobiFlight.AS1000_PFD_SOFTKEYS_1" },
    { KEY_G1000_PFD_SOFTKEY_2, "MobiFlight.AS1000_PFD_SOFTKEYS_2" },
    { KEY_G1000_PFD_SOFTKEY_3, "MobiFlight.AS1000_PFD_SOFTKEYS_3" },
    { KEY_G1000_PFD_SOFTKEY_4, "MobiFlight.AS1000_PFD_SOFTKEYS_4" },
    { KEY_G1000_PFD_SOFTKEY_5, "MobiFlight.AS1000_PFD_SOFTKEYS_5" },
    { KEY_G1000_PFD_SOFTKEY_6, "MobiFlight.AS1000_PFD_SOFTKEYS_6" },
    { KEY_G1000_PFD_SOFTKEY_7, "MobiFlight.AS1000_PFD_SOFTKEYS_7" },
    { KEY_G1000_PFD_SOFTKEY_8, "MobiFlight.AS1000_PFD_SOFTKEYS_8" },
    { KEY_G1000_PFD_SOFTKEY_9, "MobiFlight.AS1000_PFD_SOFTKEYS_9" },
    { KEY_G1000_PFD_SOFTKEY_10, "MobiFlight.AS1000_PFD_SOFTKEYS_10" },
    { KEY_G1000_PFD_SOFTKEY_11, "MobiFlight.AS1000_PFD_SOFTKEYS_11" },
    { KEY_G1000_PFD_SOFTKEY_12, "MobiFlight.AS1000_PFD_SOFTKEYS_12" },
    { KEY_G1000_PFD_DIRECTTO, "MobiFlight.AS1000_PFD_DIRECTTO" },
    { KEY_G1000_PFD_ENT, "MobiFlight.AS1000_PFD_ENT_Push" },
    { KEY_G1000_PFD_CLR_LONG, "MobiFlight.AS1000_PFD_CLR_Long" },
    { KEY_G1000_PFD_CLR, "MobiFlight.AS1000_PFD_CLR" },
    { KEY_G1000_PFD_MENU, "MobiFlight.AS1000_PFD_MENU_Push" },
    { KEY_G1000_PFD_FPL, "MobiFlight.AS1000_PFD_FPL_Push" },
    { KEY_G1000_PFD_PROC, "MobiFlight.AS1000_PFD_PROC_Push" },
    { KEY_G1000_PFD_UPPER_INC, "MobiFlight.AS1000_PFD_FMS_Upper_INC" },
    { KEY_G1000_PFD_UPPER_DEC, "MobiFlight.AS1000_PFD_FMS_Upper_DEC" },
    { KEY_G1000_PFD_PUSH, "MobiFlight.AS1000_PFD_FMS_Upper_PUSH" },
    { KEY_G1000_PFD_LOWER_INC, "MobiFlight.AS1000_PFD_FMS_Lower_INC" },
    { KEY_G1000_PFD_LOWER_DEC, "MobiFlight.AS1000_PFD_FMS_Lower_DEC" },
    { KEY_G1000_MFD_SOFTKEY_1, "MobiFlight.AS1000_MFD_SOFTKEYS_1" },
    { KEY_G1000_MFD_SOFTKEY_2, "MobiFlight.AS1000_MFD_SOFTKEYS_2" },
    { KEY_G1000_MFD_SOFTKEY_3, "MobiFlight.AS1000_MFD_SOFTKEYS_3" },
    { KEY_G1000_MFD_SOFTKEY_4, "MobiFlight.AS1000_MFD_SOFTKEYS_4" },
    { KEY_G1000_MFD_SOFTKEY_5, "MobiFlight.AS1000_MFD_SOFTKEYS_5" },
    { KEY_G1000_MFD_SOFTKEY_6, "MobiFlight.AS1000_MFD_SOFTKEYS_6" },
    { KEY_G1000_MFD_SOFTKEY_7, "MobiFlight.AS1000_MFD_SOFTKEYS_7" },
    { KEY_G1000_MFD_SOFTKEY_8, "MobiFlight.AS1000_MFD_SOFTKEYS_8" },
    { KEY_G1000_MFD_SOFTKEY_9, "MobiFlight.AS1000_MFD_SOFTKEYS_9" },
    { KEY_G1000_MFD_SOFTKEY_10, "MobiFlight.AS1000_MFD_SOFTKEYS_10" },
    { KEY_G1000_MFD_SOFTKEY_11, "MobiFlight.AS1000_MFD_SOFTKEYS_11" },
    { KEY_G1000_MFD_SOFTKEY_12, "MobiFlight.AS1000_MFD_SOFTKEYS_12" },
    { KEY_G1000_MFD_DIRECTTO, "MobiFlight.AS1000_MFD_DIRECTTO" },
    { KEY_G1000_MFD_ENT, "MobiFlight.AS1000_MFD_ENT_Push" },
    { KEY_G1000_MFD_CLR_LONG, "MobiFlight.AS1000_MFD_CLR_Long" },
    { KEY_G1000_MFD_CLR, "MobiFlight.AS1000_MFD_CLR" },
    { KEY_G1000_MFD_MENU, "MobiFlight.AS1000_MFD_MENU_Push" },
    { KEY_G1000_MFD_FPL, "MobiFlight.AS1000_MFD_FPL_Push" },
    { KEY_G1000_MFD_PROC, "MobiFlight.AS1000_MFD_PROC_Push" },
    { KEY_G1000_MFD_UPPER_INC, "MobiFlight.AS1000_MFD_FMS_Upper_INC" },
    { KEY_G1000_MFD_UPPER_DEC, "MobiFlight.AS1000_MFD_FMS_Upper_DEC" },
    { KEY_G1000_MFD_PUSH, "MobiFlight.AS1000_MFD_FMS_Upper_PUSH" },
    { KEY_G1000_MFD_LOWER_INC, "MobiFlight.AS1000_MFD_FMS_Lower_INC" },
    { KEY_G1000_MFD_LOWER_DEC, "MobiFlight.AS1000_MFD_FMS_Lower_DEC" },
    { KEY_G1000_MFD_RANGE_INC, "MobiFlight.AS1000_MFD_RANGE_INC" },
    { KEY_G1000_MFD_RANGE_DEC, "MobiFlight.AS1000_MFD_RANGE_DEC" },
    { KEY_G1000_END, "G1000_KEYS_END"},
    { A32NX_FCU_SPD_PUSH, "A32NX.FCU_SPD_PUSH" },
    { A32NX_FCU_SPD_PULL, "A32NX.FCU_SPD_PULL" },
    { A32NX_FCU_SPD_SET, "A32NX.FCU_SPD_SET" },
    { A32NX_FCU_HDG_PUSH, "A32NX.FCU_HDG_PUSH" },
    { A32NX_FCU_HDG_PULL, "A32NX.FCU_HDG_PULL" },
    { A32NX_FCU_HDG_SET, "A32NX.FCU_HDG_SET" },
    { A32NX_FCU_ALT_PUSH, "A32NX.FCU_ALT_PUSH" },
    { A32NX_FCU_ALT_PULL, "A32NX.FCU_ALT_PULL" },
    { A32NX_FCU_VS_PUSH, "A32NX.FCU_VS_PUSH" },
    { A32NX_FCU_VS_PULL, "A32NX.FCU_VS_PULL" },
    { A32NX_FCU_VS_SET, "A32NX.FCU_VS_SET" },
    { A32NX_FCU_SPD_MACH_TOGGLE_PUSH, "A32NX.FCU_SPD_MACH_TOGGLE_PUSH" },
    { A32NX_FCU_APPR_PUSH, "A32NX.FCU_APPR_PUSH" },
    { A32NX_FCU_TRK_FPA_TOGGLE_PUSH, "A32NX.FCU_TRK_FPA_TOGGLE_PUSH" },
    { KEY_CHECK_EVENT, "CHECK_EVENT" },
    { EVENT_NONE, "EVENT_NONE" },
    { EVENT_DOORS_TO_MANUAL, "EVENT_DOORS_TO_MANUAL" },
    { EVENT_DOORS_FOR_DISEMBARK, "EVENT_DOORS_FOR_DISEMBARK" },
    { EVENT_DOORS_TO_AUTO, "EVENT_DOORS_TO_AUTO" },
    { EVENT_DOORS_FOR_BOARDING, "EVENT_DOORS_FOR_BOARDING" },
    { EVENT_WELCOME_ON_BOARD, "EVENT_WELCOME_ON_BOARD" },
    { EVENT_BOARDING_COMPLETE, "EVENT_BOARDING_COMPLETE" },
    { EVENT_PUSHBACK_START, "EVENT_PUSHBACK_START" },
    { EVENT_PUSHBACK_STOP, "EVENT_PUSHBACK_STOP" },
    { EVENT_SEATS_FOR_TAKEOFF, "EVENT_SEATS_FOR_TAKEOFF" },
    { EVENT_START_SERVING, "EVENT_START_SERVING" },
    { EVENT_FINAL_DESCENT, "EVENT_FINAL_DESCENT" },
    { EVENT_REMAIN_SEATED, "EVENT_REMAIN_SEATED" },
    { EVENT_DISEMBARK, "EVENT_DISEMBARK" },
    { EVENT_TURBULENCE, "EVENT_TURBULENCE" },
    { EVENT_REACHED_CRUISE, "EVENT_REACHED_CRUISE" },
    { EVENT_REACHED_TOD, "EVENT_REACHED_TOD" },
    { EVENT_LANDING_PREPARE_CABIN, "EVENT_LANDING_PREPARE_CABIN" },
    { EVENT_SEATS_FOR_LANDING, "EVENT_SEATS_FOR_LANDING" },
    { EVENT_GO_AROUND, "EVENT_GO_AROUND" },
    { VJOY_BUTTONS, "VJOY_BUTTONS" },
    { VJOY_BUTTON_1, "VJOY_BUTTON_1" },
    { VJOY_BUTTON_2, "VJOY_BUTTON_2" },
    { VJOY_BUTTON_3, "VJOY_BUTTON_3" },
    { VJOY_BUTTON_4, "VJOY_BUTTON_4" },
    { VJOY_BUTTON_5, "VJOY_BUTTON_5" },
    { VJOY_BUTTON_6, "VJOY_BUTTON_6" },
    { VJOY_BUTTON_7, "VJOY_BUTTON_7" },
    { VJOY_BUTTON_8, "VJOY_BUTTON_8" },
    { VJOY_BUTTON_9, "VJOY_BUTTON_9" },
    { VJOY_BUTTON_10, "VJOY_BUTTON_10" },
    { VJOY_BUTTON_11, "VJOY_BUTTON_11" },
    { VJOY_BUTTON_12, "VJOY_BUTTON_12" },
    { VJOY_BUTTON_13, "VJOY_BUTTON_13" },
    { VJOY_BUTTON_14, "VJOY_BUTTON_14" },
    { VJOY_BUTTON_15, "VJOY_BUTTON_15" },
    { VJOY_BUTTON_16, "VJOY_BUTTON_16" },
    { VJOY_BUTTONS_END, "VJOY_BUTTONS_END" },
    { SIM_STOP, NULL }
};
