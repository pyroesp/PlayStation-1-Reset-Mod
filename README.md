# PlayStation 1 Reset Mod  

PlayStation 1 Controller Combination Reset mod, with Arduino Nano.  
**TESTED**, still, I'm not responsible if your PS1 blows up.  

 * PB5 - SCK (input, connect to clock)
 * PB4 - MISO (output, do not connect)
 * PB3 - MOSI (input, connect to controller DATA)
 * PB2 - /SS (input, connect to select)
 * PB1 - playstation reset (output, connect to reset of parallel port (pin 2))

You also need power and GND. For power use the 3.5V.  

Resetting the PlayStation with the GUNCON does not currently work.  
I have only tested this with the classic controller, not the analog ones, but I'm pretty sure those will work fine.

Youtube:
--------
https://youtu.be/H-n-7R_S09U

4 controllers programmed:  
------------------------
#define ID_DIG_CTRL 0x5A41 // digital  
#define ID_ANP_CTRL 0x5A73 // analog/pad  
#define ID_ANS_CTRL 0x5A53 // analog/stick  
#define ID_GUNCON_CTRL 0x5A63 // light gun  

2 different combos programmed:
------------------------------
Select + Start + L2 + R2 for digital and analog controllers.  

A + B + Trigger for GUNCON controller.  
I don't think this combination is ever needed, but I might add the X/Y position to this too.  
I might make it so it only resets if the GUNCON is not pointed towards the TV. I have to test this before making the change.

PlayStation information:
------------------------
All PlayStation related information I found came from NO$PSX on problemkaputt.de.  
There's a ton of super cool info on there.

License:
---------  
Attribution-ShareAlike 4.0 International, see license file for more info
