/*
 * File:   main.c
 * Author: pyroesp
 * 
 * Playstation 1 Reset Mod for PIC16F18325
 *
 * Created on September 21, 2019, 5:33 PM
 */

// CONFIG1
#pragma config FEXTOSC = OFF     // FEXTOSC External Oscillator mode Selection bits (HS (crystal oscillator) above 4 MHz)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with 2x PLL (32MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR/VPP pin function is MCLR; Weak pull-up enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bits (WDT disabled; SWDTEN is ignored)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will not cause a Reset)
#pragma config DEBUG = OFF      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits (Write protection off)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/VPP must be used for programming.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ 32000000

#include <xc.h>
#include <stdint.h>

#define _BV(b) (1<<(b))

#define REBOOT_DELAY 2 // 30 seconds

/*
    RA2 = SS
    RC0 = CLK
    RC1 = DATA
    RC2 = CMD
    SDO1,2 = unassigned
 */

#define PS1_LAT_IO LATC // IO port reg used for RESET
#define PS1_TRIS_IO TRISC // IO dir reg used for RESET
#define PS1_RESET 5 // Only IO that outputs a logic 0

/* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital   0101 1010 0100 0001 | 1000 0010 0101 1010  825A
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
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
void __delay_s(uint8_t s);

volatile union PS1_Cmd cmd;
volatile union PS1_Ctrl_Data data;
volatile uint8_t cmd_cnt, data_cnt;

// SPI interrupt cmd
void __interrupt() _spi_int(void) {
    LATAbits.LATA5 ^= 1; // toggle PIN A5
    
    // SPI1 interrupt
    if (PIR1bits.SSP1IF){
        cmd.buff[cmd_cnt] = SSP1BUF;
        cmd_cnt++;
        PIR1bits.SSP1IF = 0; // clear SPI1 flag
    }
    
    // SPI2 interrupt
    if (PIR2bits.SSP2IF){
        data.buff[data_cnt] = SSP2BUF;
        data_cnt++;
        PIR2bits.SSP2IF = 0; // clear SPI2 flag
    }
}

void UART_sendByte(uint8_t c){
    TX1REG = c;
    while(!TX1STAbits.TRMT);
}

void print(uint8_t *str){
    uint8_t i;
    for (i = 0; str[i] != 0 && i < 255; i++)
        UART_sendByte(str[i]);
}

void printHex(uint8_t b){
    uint8_t low_nibble = b & 0x0F;
    uint8_t high_nibble = (b >> 4) & 0x0F;
    
    print((uint8_t*)"0x");
    UART_sendByte(high_nibble >= 10 ? (high_nibble - 10 + 'A') : high_nibble + '0');
    UART_sendByte(low_nibble >= 10 ? (low_nibble - 10 + 'A') : low_nibble + '0');
}

void reverseByte(uint8_t *b) {
   *b = (*b & 0xF0) >> 4 | (*b & 0x0F) << 4;
   *b = (*b & 0xCC) >> 2 | (*b & 0x33) << 2;
   *b = (*b & 0xAA) >> 1 | (*b & 0x55) << 1;
}

void main(void){    
    // SETUP I/O
    LATC = 0;
    LATA = 0;
    
    PS1_LAT_IO &= ~_BV(PS1_RESET); // clear reset output 
    PS1_TRIS_IO |= _BV(PS1_RESET); // set reset pin to input

    ANSELAbits.ANSA2 = 0; // SS set to digital I/O
    TRISAbits.TRISA2 = 1; // SS set to input
    ANSELCbits.ANSC0 = 0; // SCK set to digital I/O
    TRISCbits.TRISC0 = 1; // SCK set to input
    ANSELCbits.ANSC1 = 0; // SDI1 set to digital I/O
    TRISCbits.TRISC1 = 1; // SDI1 set to input
    ANSELCbits.ANSC2 = 0; // SDI2 set to digital I/O
    TRISCbits.TRISC2 = 1; // SDI2 set to input
    
    // SETUP SPI1 & SPI2 I/O
    SSP1SSPPSbits.SSP1SSPPS = 0x02; // SS1 = RA2 
    SSP2SSPPSbits.SSP2SSPPS = 0x02; // SS2 = RA2 
    
    SSP1CLKPPSbits.SSP1CLKPPS = 0x10; // SCK1 = RC0
    SSP2CLKPPSbits.SSP2CLKPPS = 0x10; // SCK2 = RC0
    
    SSP1DATPPSbits.SSP1DATPPS = 0x11; // SDI1 = RC1 
    SSP2DATPPSbits.SSP2DATPPS = 0x12; // SDI2 = RC2 
    
    // SETUP SPI1 & SPI2
    SSP1STATbits.SMP = 0; // slave mode SPI1
    SSP1STATbits.CKE = 0; // transmit from idle to active
    SSP1CON1bits.CKP = 1; // clock idle high
    SSP1CON1bits.SSPM = 0x04; // slave mode: clk = sck pin; SS enabled
    SSP1CON3bits.BOEN = 1; // ignore BF flag
    SSP1CON1bits.SSPEN = 1; // enable SPI1
    
    SSP2STATbits.SMP = 0; // slave mode SPI2
    SSP2STATbits.CKE = 0; // transmit from idle to active
    SSP2CON1bits.CKP = 1; // clock idle = high
    SSP2CON1bits.SSPM = 0x04; // slave mode: clk = sck pin; SS enabled
    SSP2CON3bits.BOEN = 1; // ignore BF flag
    SSP2CON1bits.SSPEN = 1; // enable SPI2
    
    SSP1BUF = 0xFF;
    SSP2BUF = 0xFF;
    
    // SETUP UART
    ANSELCbits.ANSC3 = 0; // TX set to digital I/O
    LATCbits.LATC3 = 0; // set output 0
    TRISCbits.TRISC3 = 0; // TX set to output
    RC3PPSbits.RC3PPS = 0x14; // TX = RC3
    
    SP1BRGL = 51; // 9615 baud
    SP1BRGH = 0; // 9615 baud
    // BRGH = 1 ; SPBRG = 16 -> 117.64k baud
    
    TX1STAbits.SYNC = 0; // Asynch mode
    RC1STAbits.SPEN = 1; // Serial Port enable bit
    TX1STAbits.BRGH = 0; // low speed
    BAUD1CONbits.BRG16 = 0; // 8 bit baud rate
    BAUD1CONbits.SCKP = 0; // Idle state for TX high level
    CLKRCONbits.CLKRDIV = 0; // Fosc
    CLKRCONbits.CLKRDC = 2; // 50% duty cyle
    CLKRCONbits.CLKREN = 1; // Enable CLK reference
    
    TX1STAbits.TXEN = 1; // Enable transmitter    
    
    // DEBUG setup here
    ANSELAbits.ANSA5 = 0; // set pin to digital I/O
    TRISAbits.TRISA5 = 0; // set pin to output
    LATAbits.LATA5 = 1; // set output high
    
    ANSELAbits.ANSA4 = 0; // set pin to digital I/O
    TRISAbits.TRISA4 = 0; // set pin to output
    LATAbits.LATA4 = 1; // set output high 
    
    // SETUP variables and arrays
	data_cnt = 0;
	cmd_cnt = 0;
	clear_buff((uint8_t*)cmd.buff, PS1_CTRL_BUFF_SIZE);
	clear_buff((uint8_t*)data.buff, PS1_CTRL_BUFF_SIZE);
	
    // delay before enabling interrupts
	__delay_s(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
    
    // SETUP INTERRUPTS
    PIR1bits.SSP1IF = 0; // clear SPI1 flag
    PIR2bits.SSP2IF = 0; // clear SPI2 flag
    PIE1bits.SSP1IE = 1; // enable MSSP interrupt (SPI1)
    PIE2bits.SSP2IE = 1; // enable MSSP interrupt (SPI2)
       
    INTCONbits.PEIE = 1; // peripheral interrupt enable
    INTCONbits.GIE = 1; // global interrupt enable
    
    uint8_t i;
       
	// MAIN LOOP
    for(;;){
        LATAbits.LATA4 ^= 1; // toggle output
        if (data_cnt >= PS1_CTRL_BUFF_SIZE || cmd_cnt >= PS1_CTRL_BUFF_SIZE){
            INTCONbits.PEIE = 0; // global interrupt disable

            print((uint8_t*)"\n\rCMD: ");
            for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
                reverseByte((uint8_t*)&cmd.buff[i]);
                printHex((uint8_t)cmd.buff[i]);
                print((uint8_t*)" ");
            }
            print((uint8_t*)"\n\rDATA: ");
            for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
                reverseByte((uint8_t*)&data.buff[i]);
                printHex((uint8_t)data.buff[i]);
                print((uint8_t*)" ");
            }
            print((uint8_t*)"\n\rData done converting");
            
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
                    print((uint8_t*)"\n\rPlayStation Reset successful\n\r");
                    // change RESET from input to output, logic low (PORT is already 0)
                    PS1_TRIS_IO &= ~_BV(PS1_RESET);
                    __delay_ms(100); // hold reset for 100ms
                    // change RESET back to input
                    PS1_TRIS_IO |= _BV(PS1_RESET);
                    __delay_s(REBOOT_DELAY); // wait 30 sec before next reset, this is to prevent multiple reset one after the other
                }
            }

            clear_buff((uint8_t*)data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
            clear_buff((uint8_t*)cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
	
            data_cnt = 0;
            cmd_cnt = 0;
	
            __delay_s(1);
            
            PIR1bits.SSP1IF = 0; // clear SPI1 flag
            PIR2bits.SSP2IF = 0; // clear SPI2 flag
            INTCONbits.PEIE = 1; // global interrupt enable
        }
    }
    
    for(;;);
    return;
}

void clear_buff(uint8_t *p, uint8_t s){
    uint8_t i;
    for (i = 0; i < s; i++)
        p[i] = 0;
}

void __delay_s(uint8_t s){
    for (; s > 0; s--)
        __delay_ms(1000);
}