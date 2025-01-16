#ifndef _JETBRIDGE_H_
#define _JETBRIDGE_H_

#include <windows.h>
#include <stdio.h>
#include "simvarDefs.h"

// SimConnect doesn't currently support reading local (lvar) variables
// (params for 3rd party aircraft) but Jetbridge allows us to do this.
// To use jetbridge you must copy the Redist\a32nx-jetbridge
// package to your FS2020 Community folder. Source available from:
//    https://github.com/theomessin/jetbridge
//
// Comment the following line out if you don't want to use Jetbridge.
#define jetbridgeFallback

#ifdef jetbridgeFallback

#include "..\jetbridge\Client.h"

const char DRONE_CAMERA_FOV[] = "A:DRONE CAMERA FOV, percent";

void jetbridgeInit(HANDLE hSimConnect);

void readJetbridgeVar(const char* var);
void writeJetbridgeVar(const char* var, double val = 0);
void writeJetbridgeVar(EVENT_ID eventId, double val);
void writeJetbridgeHvar(const char* var);

void updateA310FromJetbridge(const char* data);
void updateFbwFromJetbridge(const char* data);

bool jetbridgeA310ButtonPress(int eventId, double value);
bool jetbridgeFbwButtonPress(int eventId, double value);
bool jetbridgeK100ButtonPress(int eventId, double value);
bool jetbridgePA28ButtonPress(int eventId, double value);
bool jetbridgeMiscButtonPress(int eventId, double value);

void readA310Jetbridge();
void readFbwJetbridge();

#endif

#endif // _JETBRIDGE_H_
