#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>
#include <regstr.h>
#include "game-controllers.h"

extern int switchboxId;
extern int g1000Id;

Joystick joystick[MaxJoysticks];

static char* getOem(int vid, int pid)
{
    static char oemName[256];
    DWORD len = sizeof(oemName);
    char regKey[256];
    HKEY hKey;
    long res;

    strcpy(oemName, "Unknown");

    sprintf(regKey, "%s\\VID_%04X&PID_%04X", REGSTR_PATH_JOYOEM, vid, pid);
    res = RegOpenKeyExA(HKEY_CURRENT_USER, regKey, 0, KEY_QUERY_VALUE, &hKey);
    if (res == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, REGSTR_VAL_JOYOEMNAME, 0, 0, (LPBYTE)oemName, &len);
        RegCloseKey(hKey);
    }

    return oemName;
}

static void joyInit(int id)
{
    JOYINFOEX joyInfo;
    joyInfo.dwSize = sizeof(joyInfo);
    joyInfo.dwFlags = JOY_RETURNALL;
    joyInfo.dwButtons = 0xffffffffl;

    MMRESULT res = joyGetPosEx(id, &joyInfo);
    if (res != JOYERR_NOERROR) {
        return;
    }

    joystick[id].initialised = true;

    for (int i = 0; i < joystick[id].axisCount; i++) {
        double axisVal;
        switch (i) {
        case 0: axisVal = joyInfo.dwXpos; break;
        case 1: axisVal = joyInfo.dwYpos; break;
        case 2: axisVal = joyInfo.dwZpos; break;
        case 3: axisVal = joyInfo.dwRpos; break;
        }

        joystick[id].axisZero[i] = axisVal;
        joystick[id].axis[i] = 0;

        if (joystick[id].zeroed) {
            //printf("%s axis %d zeroed = %d\n", joystick[id].name, i, joystick[id].axisZero[i]);
        }
    }

    for (int i = 0; i < joystick[id].buttonCount; i++) {
        joystick[id].button[i] = 1;
    }
}

void joyRefresh(int id)
{
    JOYINFOEX joyInfo;
    joyInfo.dwSize = sizeof(joyInfo);
    joyInfo.dwFlags = JOY_RETURNALL;
    joyInfo.dwButtons = 0xffffffffl;

    MMRESULT res = joyGetPosEx(id, &joyInfo);
    if (res != JOYERR_NOERROR) {
        return;
    }

    // Buttons need to be set to 2 if pressed or 1 if released
    int mask = 1;
    for (int i = 0; i < joystick[id].buttonCount; i++) {
        int state = 1 + ((joyInfo.dwButtons & mask) > 0);
        mask = mask << 1;

        if (joystick[id].button[i] != state) {
            if (state == 2) {
                if (!joystick[id].zeroed) {
                    joystick[id].zeroed = true;
                    joyInit(id);
                }
                //printf("%s button %d pressed\n", joystick[id].name, i);
            }
            joystick[id].button[i] = state;
        }
    }

    if (!joystick[id].zeroed) {
        for (int i = 0; i < joystick[id].axisCount; i++) {
            double axisVal;
            switch (i) {
            case 0: axisVal = joyInfo.dwXpos; break;
            case 1: axisVal = joyInfo.dwYpos; break;
            case 2: axisVal = joyInfo.dwZpos; break;
            case 3: axisVal = joyInfo.dwRpos; break;
            }

            if (joystick[id].axisZero[i] != axisVal) {
                joystick[id].zeroed = true;
                joyInit(id);
                break;
            }
        }
    }

    if (joystick[id].zeroed) {
        for (int i = 0; i < joystick[id].axisCount; i++) {
            double axisVal;
            switch (i) {
            case 0: axisVal = joyInfo.dwXpos; break;
            case 1: axisVal = joyInfo.dwYpos; break;
            case 2: axisVal = joyInfo.dwZpos; break;
            case 3: axisVal = joyInfo.dwRpos; break;
            }

            double newVal = axisVal - joystick[id].axisZero[i];
            if (joystick[id].axis[i] != newVal) {
                joystick[id].axis[i] = newVal;
                //printf("%s axis %d = %d\n", joystick[id].name, i, joystick[id].axis[i]);
            }
        }
    }
}

void initJoysticks()
{
    JOYCAPSA joyCaps;
    for (int id = 0; id < MaxJoysticks; id++) {
        joystick[id].initialised = false;

        MMRESULT res = joyGetDevCaps(id, &joyCaps, sizeof(joyCaps));
        if (res != JOYERR_NOERROR) {
            continue;
        }

        joystick[id].mid = joyCaps.wMid;
        joystick[id].pid = joyCaps.wPid;
        strcpy(joystick[id].name, getOem(joystick[id].mid, joystick[id].pid));
        joystick[id].axisCount = joyCaps.wNumAxes;
        joystick[id].buttonCount = joyCaps.wNumButtons;

        // Only interested in Pico devices
        bool isPico = false;
        if (joystick[id].mid == 0x239a && joystick[id].pid == 0x80f4) {
            isPico = true;
            if (joystick[id].axisCount == 3) {
                strcpy(joystick[id].name, "Pico G1000");
                g1000Id = id;
            }
            else {
                strcpy(joystick[id].name, "Pico Switchbox");
                switchboxId = id;
            }
        }

        //printf("%d: %s (%04x, %04x) has %d axes and %d buttons\n",
        //    id, joystick[id].name, joystick[id].mid, joystick[id].pid, joystick[id].axisCount, joystick[id].buttonCount);

        if (!isPico) {
            continue;
        }

        if (joystick[id].axisCount > MaxAxes) {
            printf("  Axes count exceeds maximum so reduced to %d\n", MaxAxes);
            joystick[id].axisCount = MaxAxes;
        }
        if (joystick[id].buttonCount > MaxButtons) {
            printf("  Button count exceeds maximum so reduced to %d\n", MaxButtons);
            joystick[id].buttonCount = MaxButtons;
        }

        joystick[id].zeroed = false;
        joyInit(id);
    }
}


void refreshJoysticks()
{
    for (int id = 0; id < MaxJoysticks; id++) {
        if (joystick[id].initialised) {
            joyRefresh(id);
        }
    }
}
