
#ifndef _LVARS_FBW_H_
#define _LVARS_FBW_H_

#include <stdio.h>

// LVars for FBW Airbus A320 & A380
const char A32NX_APU_MASTER_SW[] = "L:A32NX_OVHD_APU_MASTER_SW_PB_IS_ON, bool";
const char A32NX_APU_START[] = "L:A32NX_OVHD_APU_START_PB_IS_ON, bool";
const char A32NX_APU_START_AVAIL[] = "L:A32NX_OVHD_APU_START_PB_IS_AVAILABLE, bool";
const char A32NX_APU_BLEED[] = "L:A32NX_OVHD_PNEU_APU_BLEED_PB_IS_ON, bool";
const char A32NX_ELEC_BAT1[] = "L:A32NX_OVHD_ELEC_BAT_1_PB_IS_AUTO, bool";
const char A32NX_ELEC_BAT2[] = "L:A32NX_OVHD_ELEC_BAT_2_PB_IS_AUTO, bool";
const char A32NX_ELEC_BAT_ESS[] = "L:A32NX_OVHD_ELEC_BAT_ESS_PB_IS_AUTO, bool";
const char A32NX_ELEC_BAT_APU[] = "L:A32NX_OVHD_ELEC_BAT_APU_PB_IS_AUTO, bool";
const char A32NX_PARK_BRAKE_POS[] = "L:A32NX_PARK_BRAKE_LEVER_POS, bool";
const char A32NX_XPNDR_MODE[] = "L:A32NX_TRANSPONDER_MODE, enum";
const char A32NX_AUTOPILOT_1[] = "L:A32NX_AUTOPILOT_1_ACTIVE, bool";
const char A32NX_AUTOPILOT_2[] = "L:A32NX_AUTOPILOT_2_ACTIVE, bool";
const char A32NX_AUTOTHRUST[] = "L:A32NX_AUTOTHRUST_STATUS, enum";
const char A32NX_TCAS_MODE[] = "L:A32NX_TCAS_MODE, enum";
const char A32NX_TCAS_SWITCH[] = "L:A32NX_SWITCH_TCAS_Position, enum";
const char A32NX_AUTOPILOT_HDG[] = "L:A32NX_AUTOPILOT_HEADING_SELECTED, degrees";
const char A32NX_AUTOPILOT_VS[] = "L:A32NX_AUTOPILOT_VS_SELECTED, feetperminute";
const char A32NX_AUTOPILOT_FPA[] = "L:A32NX_AUTOPILOT_FPA_SELECTED, degrees";
const char A32NX_MANAGED_SPEED[] = "L:A32NX_FCU_SPD_MANAGED_DASHES, bool";
const char A32NX_MANAGED_HEADING[] = "L:A32NX_FCU_HDG_MANAGED_DASHES, bool";
const char A32NX_MANAGED_ALTITUDE[] = "L:A32NX_FCU_ALT_MANAGED, bool";
const char A32NX_FCU_ALT_SET[] = "K:A32NX.FCU_ALT_SET, feet";
const char A32NX_FCU_ALT_INCREMENT_SET[] = "K:A32NX.FCU_ALT_INCREMENT_SET, feet";
const char A32NX_LATERAL_MODE[] = "L:A32NX_FMA_LATERAL_MODE, enum";
const char A32NX_VERTICAL_MODE[] = "L:A32NX_FMA_VERTICAL_MODE, enum";
const char A32NX_LOC_MODE[] = "L:A32NX_FCU_LOC_MODE_ACTIVE, bool";
const char A32NX_APPR_MODE[] = "L:A32NX_FCU_APPR_MODE_ACTIVE, bool";
const char A32NX_AUTOTHRUST_MODE[] = "L:A32NX_AUTOTHRUST_MODE, enum";
const char A32NX_AUTOBRAKE[] = "L:A32NX_AUTOBRAKES_ARMED_MODE, bool";
const char A32NX_LEFT_BRAKEPEDAL[] = "L:A32NX_LEFT_BRAKE_PEDAL_INPUT, percent";
const char A32NX_RIGHT_BRAKEPEDAL[] = "L:A32NX_RIGHT_BRAKE_PEDAL_INPUT, percent";
const char A32NX_RUDDER_PEDAL_POS[] = "L:A32NX_RUDDER_PEDAL_POSITION, number";
const char A32NX_ENGINE_EGT1[] = "L:A32NX_ENGINE_EGT:1, number";
const char A32NX_ENGINE_EGT2[] = "L:A32NX_ENGINE_EGT:2, number";
const char A32NX_ENGINE_FUEL_FLOW1[] = "L:A32NX_ENGINE_FF:1, number";
const char A32NX_ENGINE_FUEL_FLOW2[] = "L:A32NX_ENGINE_FF:2, number";
const char A32NX_FLAPS_INDEX[] = "L:A32NX_FLAPS_HANDLE_INDEX, number";
const char A32NX_SPOILERS_HANDLE_POS[] = "L:A32NX_SPOILERS_HANDLE_POSITION, number";

struct LVars_FBW
{
    double apuStart = 0;
    double apuStartAvail = 0;
    double flapsIndex = 0;
    double parkBrakePos = 0;
    double spoilersHandlePos = 0;
    double xpndrMode = 0;
    double autopilot1 = 0;
    double autopilot2 = 0;
    double autothrust = 0;
    double autopilotHeading;
    double autopilotVerticalSpeed;
    double autopilotFpa;
    double leftBrakePedal = 0;
    double rightBrakePedal = 0;
    double rudderPedalPos = 0;
    double engineEgt1 = 0;
    double engineEgt2 = 0;
    double engineFuelFlow1 = 0;
    double engineFuelFlow2 = 0;
};

#endif // _LVARS_FBW_H_
