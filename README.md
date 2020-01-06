# PlayStation 1 Reset Mod  

PlayStation 1 controller combination reset mod.  
This mod when installed, allows you to reset your PlayStation through a button combination on the controller port 1.  

**Checkout the Wiki for more information.**

Disclaimer
----------  
I, or anyone contributing to this mod, am not responsible to whatever happens to your PlayStation 1 after installing or removing it.  
You do this at your own risk.  
If you don't know what you're doing, go to someone that does.   

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

See also [an 8 part video playlist](https://www.youtube.com/playlist?list=PLGaX4WJGgdHiliTw9mCHme-9vNLV6fG6E) where I go from the Arduino Nano to a PIC16F18325 and a schematic with PCB.  

Three controllers programmed  
----------------------------
Digital, Analog and GUNCON controllers have been programmed.  

/* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_GUNCON_CTRL 0x5A63 // light gun

Two different combos programmed, one for the normal controllers and one for the GUNCON.

License
-------
Attribution-ShareAlike 4.0 International, see license file for more info
