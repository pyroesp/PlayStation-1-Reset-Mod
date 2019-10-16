# PlayStation 1 Reset Mod  

PlayStation 1 controller combination reset mod.

Disclaimer
----------  
I'm not responsible if you mess up the mod and blow up your PlayStation 1.  
If you don't know what you're doing, go to someone that does.    

Why
---
Because I have a PSIO from Cybdyn System and there's no way to change games or get back into the PSIO menu system without resetting the PlayStation.  
Also, because I can.

How does it work
----------------
The PlayStation sends commands to the controller and the controller responds with data.  
The command and data is read by the microcontroller and after some checks, it either does nothing or it resets the PlayStation.  
The communication is very similar to SPI, which is why I'm using it in the later versions of the mod.

NOTE: My mod is connected to controller port 1, but it can be adapted to also work with the 2nd port by adding the SS of port 2 to the code.

Versions
--------

### Proof of concept and improvements:
**ATMEGA328P**, aka Arduino Nano - polling (Arduino IDE 1.8.9):  
This is the first working proof of concept that was done on an Arduino Nano.  
The program used a polling method to read data and cmd between PS1 and controller every rising edge of the clock signal.  

**ATMEGA328P**, aka Arduino Nano - INT interrupt (Arduino IDE 1.8.9):  
The code is similar except that instead of a polling loop, the INT interrupt is used on the SS signal and CLK signal.  
I can't actually remember if this worked, I'll have to look at my twitch vod.  

**ATMEGA328PB** - SPI interrupt (Atmel Studio 7.0):
The ATMEGA328PB has two SPI modules. This version uses those SPI modules to read data and command.  
This version is way way more stable than the previous ones.  

\***Note**: The ATMEGA328PB is pin compatible with the ATMEGA328P, which means you can replace the ATMEGA328P on an Arduino Nano, which is what I did.  

### Final version
**PIC16F18325** - SPI interrupt (MPLABX) IDE v5.25:
I looked at the cheapest microchip microcontroller that had two SPI modules. The 16F18325 was the cheapest IIRC.  
I adapted the ATMEGA328PB - SPI interrupt to work with the PIC mcu.  

Tests
----- 
I have successfully tested this with a digital controller, a GUNCON and an analog controller.  

PCB
---
PCB is 14.5mm x 12mm, see images below.  

![pcb front](https://i.imgur.com/OdFXijV.png)

![pcb back](https://i.imgur.com/sjupgW4.png)

![pcb front footprint](https://i.imgur.com/RlzW3SB.png)

Youtube:
--------
Proof of concept of the first version: https://www.youtube.com/watch?v=H-n-7R_S09U  
Final proof of concept of the first version: https://www.youtube.com/watch?v=lb_uCGyv6pY  

See also a 6 part video where I go from the Arduino Nano to a PIC16F18325 and a schematic with PCB.  

3 controllers programmed:  
------------------------
#define ID_DIG_CTRL 0x5A41 // digital  
#define ID_ANP_CTRL 0x5A73 // analog/pad  
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
A pressed button returns 0 and all controllers send a 16 bit value for their switch status.  
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
