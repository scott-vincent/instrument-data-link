#ifndef _VJOY_H_
#define _VJOY_H_

#include <windows.h>
#include <stdio.h>

// Some SimConnect events don't work with certain aircraft
// so use vJoy to simulate joystick button presses instead.
// To use vJoy you must install the device driver from here:
//    http://vjoystick.sourceforge.net/site/index.php/download-a-install/download
//
// Comment the following line out if you don't want to use vJoy.
#define vJoyFallback

#ifdef vJoyFallback

void vJoyInit();
void vJoyButtonPress(int button);
void vJoySetAxis(int value);

#endif

#endif // _VJOY_H_
