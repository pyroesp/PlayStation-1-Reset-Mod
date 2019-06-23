# PlayStation 1 Reset Mod  

PlayStation 1 Controller Combination Reset mod, with an Arduino Nano.  
**TESTED**, still, I'm not responsible if your PS1 blows up.  

Why
---
Because I have a PSIO from Cybdyn System and there's no way to change games or get back into the PSIO menu system without resetting the PlayStation.

Connections
-----------
IO are setup on PORTB, but can be modified. See PS1_X_IO defines in the Arduino code.

 * PB5 - SCK (input, connect to PS1 clock)
 * PB4 - CMD (input, connect to PS1 TX)
 * PB3 - DATA (input, connect to PS1 RX)
 * PB2 - /SS (input, connect to controller 1 select)
 * PB1 - playstation reset (output, connect to reset of parallel port (pin 2))

You also need power and GND. For power use the 3.5V provided by the PlayStation  

Tests
----- 
I have successfully tested this with a digital controller, a GUNCON and an analog controller.  

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

Changing combination:
---------------------
Combination is defined here:  
/* Key Combo */  
#define KEY_COMBO_CTRL 0xFCF6 // select-start-L2-R2 1111 1100 1111 0110  
#define KEY_COMBO_GUNCON 0x9FF7 // A-trigger-B 1001 1111 1111 0111  

If you want to change your combination, you'll have to change those defines.  
A pressed buttons return 0 and all controllers send a 16 bit value for the switch status.  
The switch status bits are defined as follows :  

|  Button  | Bit |
|:--------:|:---:|
|  SELECT  |  0  |
|    L3    |  1  |
|    R3    |  2  |
|   START  |  3  |
|    UP    |  4  |
|   RIGHT  |  5  |
|   DOWN   |  6  |
|   LEFT   |  7  |
|    L2    |  8  |
|    R2    |  9  |
|    L1    |  10 |
|    R1    |  11 |
| TRIANGLE |  12 |
|  CIRCLE  |  13 |
|   CROSS  |  14 |
|  SQUARE  |  15 |  

The GUNCON buttons are:  
* A = START
* Trigger = CIRCLE
* B = CROSS

PlayStation information:
------------------------
All PlayStation related information I found came from NO$PSX on problemkaputt.de.  
There's a ton of super cool info on there.

License:
---------  
Attribution-ShareAlike 4.0 International, see license file for more info
