/*
 * Arduino nano reset combo mod 
 * ----------------------------
 * 
 * IO are setup on PORTB, but can be modified. See PS1_X_IO defines.
 * PB5 - SCK (input, connect to PS1 clock)
 * PB4 - CMD (input, connect to PS1 TX)
 * PB3 - DATA (input, connect to PS1 RX)
 * PB2 - /SS (input, connect to controller 1 select)
 * PB1 - playstation reset (output, connect to reset of parallel port (pin 2))
 * 
 */

//#define DEBUG

#if defined(DEBUG)
  #define REBOOT_DELAY 1000
#else
  #define REBOOT_DELAY 30000
#endif

 
#define PS1_PIN_IO PINB // IO pin reg used for SS, CMD, DATA and RESET
#define PS1_PORT_IO PORTB // IO port reg used for SS, CMD, DATA and RESET
#define PS1_DIR_IO DDRB // IO dir reg used for SS, CMD, DATA and RESET
#define CLK 5
#define SS 2
#define CMD 4
#define DATA 3
#define RESET 1

 /* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_GUNCON_CTRL 0x5A63 // light gun

/* PlayStation Commands */
#define CMD_SEL_CTRL_1 0x01 // select controller 1
#define CMD_SEL_MEMC_1 0x81 // select memory card 1
#define CMD_READ_SW 0x42 // read switch status from controller

#define PS1_CTRL_BUFF_SIZE 9 // max size of buffer needed for a controller

/* Key Combo */
#define KEY_COMBO_CTRL 0xFCF6 // select-start-L2-R2 1111 1100 1111 0110
#define KEY_COMBO_GUNCON 0x9FF7 // A-trigger-B 1001 1111 1111 0111 

/* Key stuff */
#define KEY_PRESSED 0
#define KEY_RELEASED 1
enum Keys{
  SELECT,
  L3,
  R3,
  START, // light gun A
  UP,
  RIGHT,
  DOWN,
  LEFT,
  L2,
  R2,
  L1,
  R1,
  TRIANGLE,
  CIRCLE, // light gun trigger
  CROSS, // light gun B
  SQUARE
};

/* PlayStation Controller Command Union */
union PS1_Cmd{
  uint8_t buff[PS1_CTRL_BUFF_SIZE];
  struct{
    uint8_t device_select; // 0x01 or 0x81
    uint8_t command; // 0x42 for read switch
    uint8_t unused[PS1_CTRL_BUFF_SIZE-2]; // always 0 for controller
  };
};

/* PlayStation Controller Data Union */
union PS1_Ctrl_Data{
  uint8_t buff[PS1_CTRL_BUFF_SIZE]; // buffer to read data
  struct{
    uint8_t unused; // always 0xFF
    uint16_t id; // 0x5Ayz - y = type; z = # of half word
    uint16_t switches; // controller switches
    union{
      // light gun only (8MHz clock counter since hsync)
      uint16_t x_pos; // if 0x0001 then error, check y_pos
      struct{
        uint8_t adc0; // right joy X
        uint8_t adc1; // right joy Y
      };
    };
    union{
      //light gun only (scanlines since vsync
      uint16_t y_pos; // if x_pos = 0x0001 && y_pos = 0x000A => not aimed at screen
      struct{
        uint8_t adc2; // left joy X
        uint8_t adc3; // left joy Y
      };
    };
  };
};


void clear_buff(uint8_t *p, uint8_t s){
  uint8_t i;
  for (i = 0; i < s; i++)
    p[i] = 0;
}

uint8_t convert_buff_to_byte(uint8_t *p, uint8_t b){
  uint8_t i, retval;
  for (i = 0, retval = 0; i < 8; i++){
    if (p[i] & b)
      retval |= _BV(i);
  }
  return retval;
}

#define PORT_BUFF_SIZE 72 // (8 * PS1_CTRL_BUFF_SIZE)
uint8_t port[PORT_BUFF_SIZE]; // port buffer
uint8_t bit_cnt; // bit counter

union PS1_Ctrl_Data data;
union PS1_Cmd cmd;

void setup() {
  // Set PORT to inputs and no pull-ups
  PS1_PORT_IO = 0;
  PS1_DIR_IO = 0;
  
  delay(REBOOT_DELAY); // wait 30 seconds after power up
  Serial.begin(115200);
  Serial.println("PlayStation reset mod");

  // Clear buffer
  clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE);
  clear_buff(data.buff, PS1_CTRL_BUFF_SIZE);
  clear_buff(port, PORT_BUFF_SIZE);
  // Clear vars
  bit_cnt = 0;
}

void loop() {
  delay(50); // delay so you're not constantly reading data
  while (!(PS1_PIN_IO & _BV(SS))); // if data is currently being sent, wait for it to finish
  
  bit_cnt = 0;
  while (PS1_PIN_IO & _BV(SS)); // wait while there's no data incoming

  uint8_t clk, prev_clk;
  clk = PS1_PIN_IO & _BV(CLK);
  prev_clk = clk;
  while (!(PS1_PIN_IO & _BV(SS))){ // loop while SS is low
    clk = PS1_PIN_IO & _BV(CLK); // read clock input
    if (!prev_clk && clk){ // check for rising edge
      port[bit_cnt] = PS1_PIN_IO; // read port
      ++bit_cnt;
      if (bit_cnt >= PORT_BUFF_SIZE) // if all bits read, exit loop
        break;
    }
    prev_clk = clk;
  }
  while (!(PS1_PIN_IO & _BV(SS))); // if SS is still low after exiting the previous read loop, wait for it to go high
  
  // convert port buffer bits to byte
  uint8_t i, j;
  for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
    cmd.buff[i] = convert_buff_to_byte(&port[i*8], _BV(CMD));
    data.buff[i] = convert_buff_to_byte(&port[i*8], _BV(DATA));
  }
  
  uint16_t key_combo = 0;
  // Check first command for device selected
  if (cmd.device_select == CMD_SEL_CTRL_1 && cmd.command == CMD_READ_SW){
    // Check ID
    switch(data.id){
      case ID_GUNCON_CTRL:
        Serial.println("GUNCON found");
        key_combo = KEY_COMBO_GUNCON;
        Serial.print("Switches = 0x");
        Serial.println(data.switches, HEX);
        break;
      case ID_DIG_CTRL:
      case ID_ANP_CTRL:
      case ID_ANS_CTRL:
        Serial.println("Controller found");
        key_combo = KEY_COMBO_CTRL;
        Serial.print("Switches = 0x");
        Serial.println(data.switches, HEX);
        break;
      default:
        Serial.print("Wrong data or unsupported device: ");
        Serial.println(data.id, HEX);
        key_combo = 0;
        break;
    }

    // Check switch combo
    if (key_combo != 0 && 0 == (data.switches ^ key_combo)){
      // change RESET from input to output, logic low (PORT is already 0)
      PS1_DIR_IO |= _BV(RESET);
      Serial.println("PlayStation Resetting");
      delay(100); // hold reset for 100ms
      // change RESET back to input
      PS1_DIR_IO &= ~_BV(RESET);
      delay(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
    }
    
    clear_buff(data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
    clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
    clear_buff(port, PORT_BUFF_SIZE); // clear PORT to remove all previous data and not get stuck in a reset loop if the controller is disconnected after a reset
  }
}
