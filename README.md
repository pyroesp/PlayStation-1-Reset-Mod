# PlayStation 1 Reset Mod  

PlayStation 1 Controller Combo Reset mod, **proof of concept** with arduino nano.  
**TESTED**, still, I'm not responsible if your PS1 blows up.  

 * PB5 - SCK (input, connect to clock)
 * PB4 - MISO (output, do not connect)
 * PB3 - MOSI (input, connect to controller DATA)
 * PB2 - /SS (input, connect to select)
 * PB1 - playstation reset (output, connect to high side of reset switch)  
 
 The atmega328 can sink 20mA, which is more than enough to pull the high side of the switch down.  
 The switch is being pulled up by a 13.3k resistor to 3.5V.

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
