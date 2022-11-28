#ifndef _JETBRIDGE_H_
#define _JETBRIDGE_H_

#include <windows.h>
#include <stdio.h>

// SimConnect doesn't currently support reading local (lvar) variables
// (params for 3rd party aircraft) but Jetbridge allows us to do this.
// To use jetbridge you must copy the Redist\a32nx-jetbridge
// package to your FS2020 Community folder. Source available from:
//    https://github.com/theomessin/jetbridge
//
// Comment the following line out if you don't want to use Jetbridge.
#define jetbridgeFallback

#ifdef jetbridgeFallback

#include "jetbridge\client.h"

void jetbridgeInit(HANDLE hSimConnect);

void readJetbridgeVar(const char* var);
void writeJetbridgeVar(const char* var, double val);

void updateA310FromJetbridge(const char* data);
void updateA320FromJetbridge(const char* data);

bool jetbridgeA310ButtonPress(int eventId, double value);
bool jetbridgeA320ButtonPress(int eventId, double value);
bool jetbridgeK100ButtonPress(int eventId, double value);

void readA310Jetbridge();
void readA320Jetbridge();

#endif

#endif // _JETBRIDGE_H_
