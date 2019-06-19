# PlayStation 1 Reset Mod  

PlayStation 1 Controller Combo Reset mod, **proof of concept** with arduino nano.  
**UNTESTED**, I'm not responsible if your PS1 blows up.  

 * PB5 - SCK (input, connect to clock)
 * PB4 - MISO (output, do not connect)
 * PB3 - MOSI (input, connect to controller DATA)
 * PB2 - /SS (input, connect to select)
 * PB1 - playstation reset (output), connect to high side of reset switch

4 controllers programmed:  
------------------------
#define ID_DIG_CTRL 0x5A41 // digital  
#define ID_ANP_CTRL 0x5A73 // analog/pad  
#define ID_ANS_CTRL 0x5A53 // analog/stick  
#define ID_GUNCON_CTRL 0x5A63 // light gun  
