# PlayStation 1 Reset Mod  

PlayStation 1 controller combination reset mod.  
This mod when installed, allows you to reset your PlayStation through a button combination on the controller port 1.  

Disclaimer
----------  
I'm not responsible if you mess up the mod and blow up your PlayStation 1.  
If you don't know what you're doing, go to someone that does.   

CAUTION
-------
Do **NOT** reset whilst you're saving data. If you do, you'll most likely corrupt your game save and/or your memory card.  
You have been warned.    

Why
---
Because I have a PSIO from Cybdyn System and there's no way to change games or get back into the PSIO menu system without resetting the PlayStation.  

How does it work
----------------
The PlayStation sends commands to the controller and the controller responds with data.  
The command and data is read by the microcontroller and after some checks to see if the button combo has been pressed, it will resets the PlayStation.  
The communication is almost exactly like SPI, which is why I'm using it in the later versions of the mod.

\***Note:** My mod is connected to controller port 1, but it can be adapted to also work with port 2.

Versions
--------

### Proof of concept and improvements:
**ATMEGA328P**, aka Arduino Nano - Polling (Arduino IDE 1.8.9):  
This is the first working proof of concept that was done on an Arduino Nano.  
The program used a polling method to read data and cmd between PS1 and controller every rising edge of the clock signal.  

**ATMEGA328P**, aka Arduino Nano - INT interrupt (Arduino IDE 1.8.9):  
The code is similar except that instead of a polling loop, the INT interrupt is used on the SS signal and CLK signal.  
I can't actually remember if this ever worked, I'll have to look at my twitch vod.  

**ATMEGA328PB** - SPI interrupt (Atmel Studio 7.0):  
The ATMEGA328PB has two SPI modules. This version uses those SPI modules to read data and command.  
This version is the original way I wanted to program this, but couldn't because I didn't have enough SPI modules, and is much more stable than the previous ones.  

\***Note:** The ATMEGA328PB is pin compatible with the ATMEGA328P, which means you can replace it on an Arduino Nano. That is what I did.  

### Final version
**PIC16F18325** - SPI interrupt (MPLABX) IDE v5.25:  
I looked at the cheapest microchip microcontroller that had two SPI modules. The 16F18325 was the first one that popped up in the search IIRC.  
I adapted the ATMEGA328PB - SPI interrupt to work with the PIC mcu.  

Tests
----- 
I have successfully tested this with a digital controller, a GUNCON and an analog controller.  

Schematic and PCB
-----------------
![schematic](/pictures/mod/schematic.png)  

PCB is 14.5mm x 12mm, see images below.  
![pcb front](/pictures/mod/pcb%20-%20front.png)  
 
![pcb back](/pictures/mod/pcb%20-%20bottom.png)  
You can choose from three different cap sizes: 1206, 0805 and 0603.

Parts list
----------
Only two components needed for this mod: the microcontroller and **one** capacitor.  
\***Note:** Digikey links are for reference. Go to your electronics supplier of choice.  
1x [16F18325; 14-SOIC package](https://www.digikey.be/product-detail/en/microchip-technology/PIC16F18325-I-SL/PIC16F18325-I-SL-ND/5323625)  
1x [0.1µF 16V; 1206](https://www.digikey.be/product-detail/en/w-rth-elektronik/885012208030/732-8097-1-ND/5454724) **or** [0.1µF 16V; 0805](https://www.digikey.be/product-detail/en/w-rth-elektronik/885012207045/732-8045-1-ND/5454672) **or** [0.1µF 16V; 0603](https://www.digikey.be/product-detail/en/samsung-electro-mechanics/CL10B104KO8NNNC/1276-1005-1-ND/3889091) package  

\***Note:** Capacitor smd sizes:
* 1206 = 0.125 in × 0.06 in | 3.2 mm × 1.6 mm  
* 0805 = 0.080 in × 0.05 in | 2.0 mm × 1.25 mm
* 0603 = 0.060 in × 0.03 in | 1.6 mm × 0.8 mm

If you don't have a lot of experience soldering I recommend going for the 1206.

Youtube
-------
Proof of concept of the first version: https://www.youtube.com/watch?v=H-n-7R_S09U  
Final proof of concept of the first version: https://www.youtube.com/watch?v=lb_uCGyv6pY  

See also [a 6 part video playlist](https://www.youtube.com/playlist?list=PLGaX4WJGgdHiliTw9mCHme-9vNLV6fG6E) where I go from the Arduino Nano to a PIC16F18325 and a schematic with PCB.  

Three controllers programmed  
----------------------------
Digital, Analog and GUNCON controllers have been programmed.  

#define ID_DIG_CTRL 0x5A41 // digital  
#define ID_ANS_CTRL 0x5A73 // analog/stick  
#define ID_GUNCON_CTRL 0x5A63 // light gun  

Two different combos programmed
-------------------------------
**Select + Start + L2 + R2** for **digital** and **analog** controllers.  

**A + B + Trigger** for **GUNCON** controller.  

Changing combination
--------------------
Combination is defined in the code:  
/* Key Combo */  
#define KEY_COMBO_CTRL 0xFCF6 // select-start-L2-R2 1111 1100 1111 0110  
#define KEY_COMBO_GUNCON 0x9FF7 // A-trigger-B 1001 1111 1111 0111  

If you want to change your combination, you'll have to change those defines.  
A pressed button returns 0, all controllers send a 16 bit value for their switch status.  
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

PlayStation information
-----------------------
All PlayStation related information I found came from NO$PSX on problemkaputt.de.  
There's a ton of super cool info on there.

License
-------
Attribution-ShareAlike 4.0 International, see license file for more info
