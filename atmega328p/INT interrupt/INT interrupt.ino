/*
 * Arduino nano reset combo mod POC v2
 * -----------------------------------
 * 
 * IO are setup on PORTB, but can be modified. See PS1_X_IO defines.
 * PB5 - SCK (input, connect to PS1 clock)
 * PB4 - CMD (input, connect to PS1 TX)
 * PB3 - DATA (input, connect to PS1 RX)
 * PB2 - /SS (input, connect to controller 1 select)
 * PB1 - playstation reset (output, connect to reset of parallel port (pin 2))
 * 
 */

#define DEBUG

#if defined(DEBUG)
  #define REBOOT_DELAY 1000
#else
  #define REBOOT_DELAY 1
#endif
 
#define PS1_PIN_IO PIND // IO pin reg used for SS, CMD, DATA and RESET
#define PS1_PORT_IO PORTD // IO port reg used for SS, CMD, DATA and RESET
#define PS1_DIR_IO DDRD // IO dir reg used for SS, CMD, DATA and RESET
#define CLK 2
#define SS 3
#define CMD 4
#define DATA 5
#define RESET 6 // ONly IO that outputs a logic 0

 /* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_GUNCON_CTRL 0x5A63 // light gun

/* PlayStation Commands */
#define CMD_SEL_CTRL_1 0x01 // select controller 1
#define CMD_SEL_MEMC_1 0x81 // select memory card 1
#define CMD_READ_SW 0x42 // read switch status from controller

/* PlayStation Buff Size */
#define PS1_CTRL_BUFF_SIZE 9 // max size of buffer needed for a controller
#define PORT_BUFF_SIZE 72 // (8 * PS1_CTRL_BUFF_SIZE)

/* Key Combo */
#define KEY_COMBO_CTRL 0xFCF6 // select-start-L2-R2 1111 1100 1111 0110
#define KEY_COMBO_GUNCON 0x9FF7 // A-trigger-B 1001 1111 1111 0111 

/* Key stuff */
#define KEY_PRESSED 0
#define KEY_RELEASED 1

enum Keys{
  SELECT, // 0
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

// read 8 bytes -> 1 byte corresponding to bit b
uint8_t convert_buff_to_byte(uint8_t *p, uint8_t b){
  uint8_t i, retval;
  for (i = 0, retval = 0; i < 8; i++){
    if (p[i] & b)
      retval |= _BV(i);
  }
  return retval;
}

uint8_t port[PORT_BUFF_SIZE]; // port buffer
uint8_t bit_cnt; // bit counter

union PS1_Ctrl_Data data;
union PS1_Cmd cmd;

void setup() {
  // Set PORT to inputs and no pull-ups
  PS1_PORT_IO = 0;
  PS1_DIR_IO = 0;

  // ISR setup stuff

  EICRA |= 0x03; // ISC01-ISC00: The rising edge of INT0 generates an interrupt request. -> CLK
  EICRA |= 0x08; // ISC01-ISC00: The falling edge of INT1 generates an interrupt request. -> SS
  EIMSK |= _BV(INT1); // INT1: External Interrupt Request 1 enabled and INT0 disabled
  // Select timer1 normal mode (timer overflow)
  TCCR1A = 0;
  TCCR1B = 2;
  
  SREG |= 0x80; //  I: Global Interrupt Enable

  // -------
  
  DDRB |= _BV(5);
  PORTB |= _BV(5);
  delay(1000); // LED on for 1000ms
  PORTB &= ~_BV(5);
  
  delay(REBOOT_DELAY); // wait 30 seconds after power up
#if defined(DEBUG)
  Serial.begin(115200);
  Serial.println("PlayStation reset mod");
#endif

  // Clear buffer
  clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE);
  clear_buff(data.buff, PS1_CTRL_BUFF_SIZE);
  clear_buff(port, PORT_BUFF_SIZE);
  // Clear vars
  bit_cnt = 0;
}


// ISR read on rising edge of PD2 using INT0
ISR(INT0_vect){
  port[bit_cnt] = PS1_PIN_IO; // read port
  bit_cnt++;
}

// ISR read on rising edge of PD3 using INT1
ISR(INT1_vect){
  TCNT1 = 65495; // OVF at 65536 (41 clock / 8 cycles)
  TIMSK1 |= _BV(TOIE1);
}

// TMR1 interrupt is triggered when TMR1 overflows
// Check if SS is low for long enough to start reading data, see extra SS pulse after controller data transfer
ISR(TIMER1_OVF_vect){
  if (!(PS1_PIN_IO & _BV(SS))){ // check if SS is still low
    TIMSK1 &= ~_BV(TOIE1); // disable timer interrupt
    EIMSK |= _BV(INT0); // enables external interrupt INT0
  }
}

void loop(){
  if (bit_cnt >= PORT_BUFF_SIZE){
    EIMSK &= ~_BV(INT0); // disable clk interrupt
    EIMSK &= ~_BV(INT1); // disable ss interrupt

    bit_cnt = 0;

    // Convert PORT data to bytes
    uint8_t i;
    for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
      cmd.buff[i] = convert_buff_to_byte(&port[i*8], _BV(CMD));
      data.buff[i] = convert_buff_to_byte(&port[i*8], _BV(DATA));
    }

#if defined(DEBUG)
    Serial.print("PORT BUFF: ");
    for (i = 0; i < PORT_BUFF_SIZE; i++){
      Serial.print("0x");
      Serial.print(port[i], HEX);
      Serial.print(", ");
    }
    Serial.print("\nCMD: ");
    for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
      Serial.print(cmd.buff[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
      Serial.print("DATA: ");
    for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
      Serial.print(data.buff[i], HEX);
      Serial.print(" ");
    }
    Serial.println("");
    Serial.println("Data done converting");
#endif

    uint16_t key_combo = 0;
    // Check first command for device selected
    if (cmd.device_select == CMD_SEL_CTRL_1 && cmd.command == CMD_READ_SW){
      // Check ID
      switch(data.id){
        case ID_GUNCON_CTRL:
#if defined(DEBUG)
          Serial.println("GUNCON found");
          Serial.print("Switches = 0x");
          Serial.println(data.switches, HEX);
#endif
          key_combo = KEY_COMBO_GUNCON;
          break;
        case ID_DIG_CTRL:
        case ID_ANP_CTRL:
        case ID_ANS_CTRL:
#if defined(DEBUG)
          Serial.println("Controller found");
          Serial.print("Switches = 0x");
          Serial.println(data.switches, HEX);
#endif
          key_combo = KEY_COMBO_CTRL;
          break;
        default:
#if defined(DEBUG)
          Serial.print("Wrong data or unsupported device: ");
          Serial.println(data.id, HEX);
#endif
          key_combo = 0;
          break;
      }
  
      // Check switch combo
      if (key_combo != 0 && 0 == (data.switches ^ key_combo)){
        // change RESET from input to output, logic low (PORT is already 0)
        PS1_DIR_IO |= _BV(RESET);
        DDRB |= _BV(5);
        PORTB |= _BV(5);
#if defined(DEBUG)
        Serial.println("PlayStation Resetting");
#endif
        delay(1000); // hold reset for 100ms
        // change RESET back to input
        PS1_DIR_IO &= ~_BV(RESET);
        PORTB &= ~_BV(5);
        delay(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
      }
    }

    clear_buff(data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
    clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
    clear_buff(port, PORT_BUFF_SIZE); // clear PORT to remove all previous data and not get stuck in a reset loop if the controller is disconnected after a reset
    
    delay(50);

    EIMSK |= _BV(INT1); // enable ss interrupt
  } 
}
