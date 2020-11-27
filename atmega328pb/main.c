/*
 * Created: 21/08/2019 21:03:29
 * Author : pyroesp
 
 * MCU 328PB : 2xSPI
 *		- 16MHz
 */ 


#include <avr/io.h>
#include <avr/iom328pb.h>
#include <avr/interrupt.h>

#define  F_CPU 16000000L
#include <util/delay.h>

#define REBOOT_DELAY 30000 // 30 seconds

#define PS1_PIN_IO PIND // IO pin reg used for SS, CMD, DATA and RESET
#define PS1_PORT_IO PORTD // IO port reg used for SS, CMD, DATA and RESET
#define PS1_DIR_IO DDRD // IO dir reg used for SS, CMD, DATA and RESET
#define RESET 6 // Only IO that outputs a logic 0

/* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_GUNCON_CTRL 0x5A63 // light gun

/* PlayStation Commands */
#define CMD_SEL_CTRL_1 0x01 // select controller 1
#define CMD_SEL_MEMC_1 0x81 // select memory card 1
#define CMD_READ_SW 0x42 // read switch status from controller

/* PlayStation Buff Size */
#define PS1_CTRL_BUFF_SIZE 9 // max size of buffer needed for a controller

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

void clear_buff(uint8_t *p, uint8_t s);

volatile union PS1_Cmd cmd;
volatile union PS1_Ctrl_Data data;
volatile uint8_t cmd_cnt, data_cnt;

// SPI0 interrupt cmd
ISR(SPI0_STC_vect){
	cmd.buff[cmd_cnt] = SPDR0 ^ 0xFF;
	cmd_cnt++;
}

// SPI1 interrupt data
ISR(SPI1_STC_vect){
	data.buff[data_cnt] = SPDR1 ^ 0xFF;
	data_cnt++;
}

/*
ISR(TIMER1_OVF_vect){
	PORTB ^= _BV(PORTB0);	
	TCNT1 = 65495; // OVF at 65536 (41 clock/8 cycles)
}
*/

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

void USART_Init(unsigned int ubrr){   
	/*Set baud rate */   
	UBRR0 = ubrr;  
	/* Enable receiver and transmitter */   
	UCSR0B = (1<<TXEN0);   
	/* Set frame format: 8data, 1stop bit */ 
	UCSR0C = (3<<UCSZ00);
}

void USART_Transmit(unsigned char data ){   
	/* Wait for empty transmit buffer */   
	while ( !( UCSR0A & (1<<UDRE0)) );   
	/* Put data into buffer, sends the data */   
	UDR0 = data;
}

void USART_print(unsigned char *data){
	int i;
	for (i = 0; data[i] != 0; i++)
		USART_Transmit(data[i]);
}

void USART_printByte(unsigned char byte){
	char map[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	USART_print("0x");
	USART_Transmit(map[byte >> 4]);
	USART_Transmit(map[byte & 0xF]);
}

int main(void){
	int i;
	// PORT REG
	PS1_DIR_IO = 0;
	PS1_PORT_IO = 0;
	
	// Setup IO
	DDRE &= ~(_BV(DDRE3) | _BV(DDRE2)); // MOSI1 & SS1 set to inputs
	DDRC &= ~_BV(DDRC0); // SCK1 set to inputs
	DDRC |= _BV(DDRC1); // MISO1 set as output
	DDRB &= ~(_BV(DDRB2) | _BV(DDRB3) | _BV(DDRB5)); // set MOSI0, SCK0, SS0 to input
	DDRB |= _BV(DDRB4); // set MISO0 to output
	
	DDRB |= _BV(PORTB0);
	
	USART_Init(MYUBRR);
	
	// INTERRUPT REG
	
	// Select timer1 normal mode (timer overflow)
	//TCCR1A = 0;
	//TCCR1B = 2;
	//TIMSK1 |= _BV(TOIE1);
	//TCNT1 = 65495; // OVF at 65536 (41 clock/8 cycles)
	
	SPCR0 |= _BV(SPIE); // enable SPI0 interrupt
	SPCR0 |= _BV(SPE); // enable SPI0
	SPCR0 |= _BV(DORD); // data order LSB
	SPCR0 |= _BV(CPOL); // CLK idle when high
	
	SPCR1 |= _BV(SPIE1); // enable SPI1 interrupt
	SPCR1 |= _BV(SPE1); // enable SPI1
	SPCR1 |= _BV(DORD1); // data order LSB
	SPCR1 |= _BV(CPOL1); // CLK idle when high
	
	sei(); // enable global interrupt
	
	data_cnt = 0;
	cmd_cnt = 0;
	clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE);
	clear_buff(data.buff, PS1_CTRL_BUFF_SIZE);
	
	_delay_ms(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
	
	// MAIN LOOP
    for(;;){		
		if (data_cnt >= PS1_CTRL_BUFF_SIZE || cmd_cnt >= PS1_CTRL_BUFF_SIZE){
			SPCR0 &= ~_BV(SPE); // disable SPI0 
			SPCR1 &= ~_BV(SPE1); // disable SPI1 
			
			USART_print("CMD: ");
			for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
				USART_printByte(cmd.buff[i]);
				USART_print(" ");
			}
			USART_print("\r\n");
			
			USART_print("DATA: ");
			for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
				USART_printByte(data.buff[i]);
				USART_print(" ");
			}
			USART_print("\r\n");
			USART_print("-------------------------------\r\n");
			
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
					USART_print("Resetting PlayStation\r\n");
					USART_print("-------------------------------\r\n");
					_delay_ms(100); // hold reset for 100ms
					// change RESET back to input
					PS1_DIR_IO &= ~_BV(RESET);
					_delay_ms(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
				}
			}

			clear_buff(data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
			clear_buff(cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
	
			data_cnt = 0;
			cmd_cnt = 0;
	
			_delay_ms(50);
			SPCR0 |= _BV(SPE); // enable SPI0 interrupt
			SPCR1 |= _BV(SPE1); // enable SPI1 interrupt
		}
    }
}

void clear_buff(uint8_t *p, uint8_t s){
	uint8_t i;
	for (i = 0; i < s; i++)
		p[i] = 0;
}
