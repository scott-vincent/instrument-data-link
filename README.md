# MICROSOFT FLIGHT SIMULATOR 2020 - INSTRUMENT DATA LINK

![Screenshot](Screenshot.jpg)

This is the companion program to

  https://github.com/scott-vincent/instrument-panel
  
  https://github.com/scott-vincent/autopilot-panel
  
  https://github.com/scott-vincent/radio-panel
  
  https://github.com/scott-vincent/power-lights-panel

This program must run on the same host as MS FS2020 and uses the Microsoft SimConnect SDK to connect to FS2020 and collect all the data required by the instrument panel.

The instrument panel connects to this program over your network and receives the data at regular intervals so that the instruments can be drawn with up-to-date values.

# vJoy

This is an alternative way for a panel to send an event to FS2020 when the event in SimConnect is not yet working correctly (due to SDK bugs). The panel simulates a joystick button press and the correct action must be configured in the Controls section of FS2020.

Use of vJoy is optional and is currently only used by the Power/Lights panel. If you want to use vJoy you must install the vJoy Virtual Joystick driver from here:

http://vjoystick.sourceforge.net/site/index.php/download-a-install/download

After the vJoy driver is installed you will see a new joystick in the Controls section of FS2020 called vJoy.

# Donate

If you find this project useful, would like to see it developed further or would just like to buy the author a beer, please consider a small donation.

[<img src="donate.svg" width="210" height="40">](https://paypal.me/scottvincent2020)
