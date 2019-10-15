/*
 * ATMEGA168_PS1_RESET_MOD.c
 *
 * Created: 21/08/2019 21:03:29
 * Author : pyroesp
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

#define  F_CPU 20000000L
#include <util/delay.h>

#define REBOOT_DELAY 1000


#define PS1_PIN_IO PIND // IO pin reg used for SS, CMD, DATA and RESET
#define PS1_PORT_IO PORTD // IO port reg used for SS, CMD, DATA and RESET
#define PS1_DIR_IO DDRD // IO dir reg used for SS, CMD, DATA and RESET
#define CLK 2
#define SS 3
#define CMD 4
#define DATA 5
#define RESET 6 // Only IO that outputs a logic 0

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
			//light gun only (scanlines since vsync)
			uint16_t y_pos; // if x_pos = 0x0001 && y_pos = 0x000A => not aimed at screen
			struct{
				uint8_t adc2; // left joy X
				uint8_t adc3; // left joy Y
			};
		};
	};
};

uint8_t convert_buff_to_byte(uint8_t *p, uint8_t b);
void clear_buff(uint8_t *p, uint8_t s);

volatile uint8_t bit_cnt;
volatile uint8_t buff[PORT_BUFF_SIZE];

// ISR triggered by falling edge of SS
ISR(INT1_vect){
	PORTB |= _BV(0);
	TCNT1 = 65495; // OVF at 65536 (41 clock/8 cycles)
	TIMSK1 |= _BV(TOIE1);
}

// TMR1 interrupt is triggered when TMR1 overflows
// Check if SS is low for long enough to start reading data, see extra SS pulse after controller data transfer
ISR(TIMER1_OVF_vect){
	PORTB &= ~_BV(0);
	if (!(PS1_PIN_IO & _BV(SS))){ // check if SS is still low
		PORTB |= _BV(0);
		EIMSK |= _BV(INT0); // enables external interrupt INT0
	}
	TIMSK1 &= ~_BV(TOIE1); // disable TIMER1 OVF interrupt
}

// ISR triggered by rising edge of CLK
ISR(INT0_vect){
	buff[bit_cnt] = PS1_PIN_IO; // read port
	bit_cnt++;
	PORTB &= ~_BV(0);
}

int main(void){
	union PS1_Cmd cmd;
	union PS1_Ctrl_Data data;
	
	// PORT REG
	PS1_DIR_IO = 0;
	PS1_PORT_IO = 0;
	
	DDRB |= _BV(0); // set PB0 to output
	PORTB |= _BV(0); // set PB0 to output
	
	// INTERRUPT REG
	EICRA |= 0x03; // ISC01-ISC00: The rising edge of INT0 generates an interrupt request. -> CLK
	EICRA |= 0x08; // ISC01-ISC00: The falling edge of INT1 generates an interrupt request. -> SS
	EIMSK |= _BV(INT1); // INT1: External Interrupt Request 1 enabled and INT0 disabled
	// Select timer1 normal mode (timer overflow)
	TCCR1A = 0;
	TCCR1B = 2;
	sei(); // enable global interrupt
	
	bit_cnt = 0;
	clear_buff(buff, PORT_BUFF_SIZE);
	clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE);
	clear_buff(data.buff, PS1_CTRL_BUFF_SIZE);
	
	// MAIN LOOP
    for(;;){
		if (bit_cnt >= PORT_BUFF_SIZE){
			EIMSK &= ~_BV(INT0);
			EIMSK &= ~_BV(INT1);

			bit_cnt = 0;

			// Convert PORT data to bytes
			uint8_t i;
			for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
				cmd.buff[i] = convert_buff_to_byte(&buff[i*8], _BV(CMD));
				data.buff[i] = convert_buff_to_byte(&buff[i*8], _BV(DATA));
			}

			uint16_t key_combo = 0;
			// Check first command for device selected
			if (cmd.device_select == CMD_SEL_CTRL_1 && cmd.command == CMD_READ_SW){
				// Check ID
				switch(data.id){
					case ID_GUNCON_CTRL:
					key_combo = KEY_COMBO_GUNCON;
					break;
					case ID_DIG_CTRL:
					case ID_ANP_CTRL:
					case ID_ANS_CTRL:
					key_combo = KEY_COMBO_CTRL;
					break;
					default:
					key_combo = 0;
					break;
				}
		
				// Check switch combo
				if (key_combo != 0 && 0 == (data.switches ^ key_combo)){
					// change RESET from input to output, logic low (PORT is already 0)
					PS1_DIR_IO |= _BV(RESET);
					_delay_ms(1000); // hold reset for 100ms
					// change RESET back to input
					PS1_DIR_IO &= ~_BV(RESET);
					_delay_ms(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
				}
			}

			clear_buff(data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
			clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
			clear_buff(buff, PORT_BUFF_SIZE); // clear PORT to remove all previous data and not get stuck in a reset loop if the controller is disconnected after a reset
	
			_delay_ms(50);

			EIMSK |= _BV(INT1); // enable ss interrupt
		}
    }
}

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