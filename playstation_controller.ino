/*
 * Arduino nano reset combo mod POC
 * --------------------------------
 * 
 * PB5 - SCK (input, connect to clock)
 * PB4 - MISO (output, do not connect)
 * PB3 - MOSI (input, connect to controller DATA)
 * PB2 - /SS (input, connect to select)
 * PB1 - playstation reset (output), connect to high side of reset switch
 * 
 */

 /* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_GUNCON_CTRL 0x5A63 // light gun

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

/* PlayStation Controller Union */
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
      //light gun only (scanlines since vsync
      uint16_t y_pos; // if x_pos = 0x0001 && y_pos = 0x000A => not aimed at screen
      struct{
        uint8_t adc2; // left joy X
        uint8_t adc3; // left joy Y
      };
    };
  };
};

uint8_t data_in[32];
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
  // Set reset pin to input for tristate
  pinMode(9, INPUT);
  digitalWrite(9, LOW);
  
  delay(30000); // wait 30 seconds after power up
  Serial.begin(115200);
  Serial.println("PlayStation 1 Controller Reset Mod");
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

  // Clear buffer
  ctrl_Clear(&c);
  // Clear vars
  pos = 0;
  start_reading = 0;
  process_data = 0;
  new_data = 0;
}

ISR (SPI_STC_vect){
  data = SPDR;
  data_in[pos] = data;
  pos++;
}

void loop() {
  while (PINB & 0x04); // wait while there's no data incoming
  while (!(PINB & 0x04)); // wait while data being read
  /* Process Data */
  uint8_t max_data = (data_in[1] & 0x0F) << 1 + 1; // get # halfword to bytes + 1 byte for the 0x5A from ID
  uint8_t i, j;
  // Copy data to structure
  for (i = 1, j = 0; i <= max_data; i++, j++)
    c.buff[j] = data_in[i];
  pos = 0;

  // Check ID
  uint16_t key_combo;
  switch(c.id){
    case ID_GUNCON_CTRL:
      Serial.println("GUNCON found");
      key_combo = KEY_COMBO_GUNCON;
      break;
    case ID_DIG_CTRL:
    case ID_ANP_CTRL:
    case ID_ANS_CTRL:
      Serial.println("Controller found");
      key_combo = KEY_COMBO_CTRL;
      break;
    default:
      Serial.println("Wrong data or unsupported device");
      Serial.println(c.id, HEX);
      break;
  }

  // Check switch combo
  if (0 == (c.switches ^ key_combo)){
    process_data = 0;
    // TODO: does resetting the playstation cut the power too ?
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);
    Serial.println("PlayStation Resetting");
    delay(100); // hold reset for 100ms
    pinMode(9, INPUT);
    digitalWrite(9, LOW);
    delay(30000); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
    // for some reason, during boot sequence,
    
    // Output can sink 20mA
    // reset switch has a 13.3k pull up to 3.5V, 
    // when reset is pressed 260µA is drawn from the 3.5V through the resistor.
  }
  
  ctrl_Clear(&c); // clear controller stuff
}
