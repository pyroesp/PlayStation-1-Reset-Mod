/*
 * Arduino nano PlayStation 1 reset combo mod
 * ------------------------------------------
 * 
 * PB5 - SCK (input, connect to clock)
 * PB4 - MISO (output, do not connect)
 * PB3 - MOSI (input, connect to controller DATA)
 * PB2 - /SS (input, connect to select)
 * PB1 - playstation reset (output, connect to high side of reset switch)
 * 
 */

 /* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_GUNCON_CTRL 0x5A63 // light gun

/* Key Combo */
#define KEY_COMBO_CTRL 0x6F3F // select-start-L2-R2 0110 1111 0011 1111
#define KEY_COMBO_GUNCON 0xEFF9 // A-trigger-B 1110 1111 1111 1001

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

/* PlayStation Controller Union */
// TODO : Check endianness of union so everything is copied correctly
union PS1_Ctrl{
  uint8_t buff[8]; // buffer to read SPI
  struct{
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
      //light gun only (scanlines since vsync)
      uint16_t y_pos; // if x_pos = 0x0001 && y_pos = 0x000A => not aimed at screen
      struct{
        uint8_t adc2; // left joy X
        uint8_t adc3; // left joy Y
      };
    };
  };
};

uint8_t pos; // buffer position counter
uint8_t data; // SPI input data
uint8_t start_reading; 
uint8_t process_data;
uint8_t new_data;
union PS1_Ctrl c;

void ctrl_Clear(union PS1_Ctrl *p){
  uint8_t i;
  for (i = 0; i < 8; i++)
    p->buff[i] = 0;
}

void setup() {
  delay(30000); // wait 30 seconds after power up
  Serial.begin(9600);
  // Turn on SPI in slave mode
  SPCR |= _BV(SPE);
  // Turn on SPI interrupts
  SPCR |= _BV(SPIE);
  // PlayStation is LSB first, so set DORD bit
  SPCR |= _BV(DORD);
  // PlayStation clock is high when idle, so set CPOL
  SPCR |= _BV(CPOL);
  // PlayStation reads on rising edge, so set CPHA
  SPCR |= _BV(CPHA);
  // Set MISO to output, but do NOT connect this pin, leav it floating or with pull down.
  pinMode(MISO, OUTPUT);
  // Set reset pin to input for tristate
  pinMode(PB1, INPUT);
  digitalWrite(PB1, LOW); // disable pull-ups

  // Clear buffer
  ctrl_Clear(&c);
  // Clear vars
  pos = 0;
  start_reading = 0;
  process_data = 0;
  new_data = 0;
}

/* Interrupt */
ISR (SPI_STC_vect){
  data = SPDR;
  // check incoming byte for start of data if not processing
  if (data == 0x5A && process_data == 0) 
    start_reading = 1;
   // if reading data started, set new_data to 1
  if (start_reading)
    new_data = 1;
}

void loop() {
  // save data only if we started reading and there's new data
  if (start_reading){
    if (new_data){
      c.buff[pos++] = data;
      new_data = 0;
      // read max 8 bytes, check # of halfword and multiply by 2 for # of bytes
      if (pos >= 8 || pos >= ((c.buff[1] & 0x000F) << 1)){ 
        pos = 0;
        start_reading = 0;
        process_data = 1;
      }
    }
  }

  if (process_data){
    uint16_t key_combo;
    switch(c.id){
      case ID_GUNCON_CTRL:
        key_combo = KEY_COMBO_GUNCON;
        break;
      case ID_DIG_CTRL:
      case ID_ANP_CTRL:
      case ID_ANS_CTRL:
      default:
        key_combo = KEY_COMBO_CTRL;
        break;
    }
    if (0 == (c.switches ^ key_combo)){
      // TODO: does resetting the playstation cut the power too ?
      pinMode(PB1, OUTPUT);
      digitalWrite(PB1, LOW);
      delay(100); // hold reset for 100ms
      pinMode(PB1, INPUT);
      digitalWrite(PB1, LOW);
      delay(10000); // wait 10 sec before next reset, this is to prevent continuous reset
      
      // Output can sink 20mA
      // reset switch has a 13.3k pull up to 3.5V, 
      // when reset is pressed 260µA is drawn from the 3.5V through the resistor.
    }
    // Clear buffer when done processing key
    ctrl_Clear(&c);
    process_data = 0; // clear process_data var
  }
}
