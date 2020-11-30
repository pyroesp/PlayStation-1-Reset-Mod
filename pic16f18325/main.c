/*
 * File:   main.c
 * Author: pyroesp
 * 
 * PlayStation 1 Reset Mod for PIC16F18325
 *
 * Created on September 21, 2019, 5:33 PM
 
 Connection:
    RA2 = SS
    RC0 = CLK
    RC1 = DATA
    RC2 = CMD
    SDO1,2 = unassigned
    RC5 = RESET
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

/* Uncomment the define below to have UART TX debugging on RC3 */
//#define DEBUG

#ifdef DEBUG
#define REBOOT_DELAY 2 // s
#else
#define REBOOT_DELAY 20 // s
#endif

#define SHORT_DELAY 500 // ms
#define LONG_DELAY 2 // s

/* Reset port */
#define PS1_LAT_IO LATC // IO port reg used for RESET
#define PS1_TRIS_IO TRISC // IO dir reg used for RESET
#define PS1_RESET 5 // Only IO that outputs a logic 0

/* Controller ID */
#define ID_DIG_CTRL 0x5A41 // digital
#define ID_ANP_CTRL 0x5A73 // analog/pad
#define ID_ANS_CTRL 0x5A53 // analog/stick
#define ID_DS2_CTRL 0x5A79 // dualshock 2
#define ID_GUNCON_CTRL 0x5A63 // light gun

/* PlayStation Commands */
#define CMD_SEL_CTRL_1 0x01 // select controller 1
#define CMD_SEL_MEMC_1 0x81 // select memory card 1
#define CMD_READ_SW 0x42 // read switch status from controller

/* PlayStation Communication Buff Size */
#define PS1_CTRL_BUFF_SIZE 9 // max size of buffer needed for a controller

/* Key Combo */
#define KEY_COMBO_CTRL 0xFCF6       // select-start-L2-R2   1111 1100 1111 0110
#define KEY_COMBO_GUNCON 0x9FF7     // A-trigger-B          1001 1111 1111 0111
#define KEY_COMBO_XSTATION 0xBCFE   // select-cross-L2-R2   1011 1100 1111 1110

/*
Keys : 
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
*/


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

/* Global variables */
volatile union PS1_Cmd cmd;
volatile union PS1_Ctrl_Data data;
volatile uint8_t cmd_cnt, data_cnt;

/* Function prototype */
void reverse_byte(uint8_t *b);
void clear_buff(uint8_t *p, uint8_t s);
void __delay_s(uint8_t s);

void reset_short(void);
void reset_long(void);

#ifdef DEBUG
void UART_init(void);
void UART_sendByte(uint8_t c);
void UART_print(uint8_t *str);
void UART_printHex(uint8_t b);
#else
#define UART_init(a)
#define UART_sendByte(a) 
#define UART_print(a) 
#define UART_printHex(a) 
#endif


/* Interrupt */
void __interrupt() _spi_int(void) {    
    // SPI1 interrupt
    if (PIR1bits.SSP1IF){
        if (data_cnt < PS1_CTRL_BUFF_SIZE){
            data.buff[data_cnt] = SSP1BUF;
            data_cnt++;
        }
        PIR1bits.SSP1IF = 0; // clear SPI1 flag
    }
    // SPI2 interrupt
    if (PIR2bits.SSP2IF){
        if (cmd_cnt < PS1_CTRL_BUFF_SIZE){
            cmd.buff[cmd_cnt] = SSP2BUF;
            cmd_cnt++;
        }
        PIR2bits.SSP2IF = 0; // clear SPI2 flag
    }
}

/* Main */
void main(void){
    uint8_t i;
    
    // SETUP I/O
    ANSELA = 0; // port A is digital IO
    LATA = 0; // clear latch A
    LATC = 0; // clear latch C
    ANSELC = 0; // port C is digital IO
    
    PS1_LAT_IO &= ~_BV(PS1_RESET); // clear reset output 
    PS1_TRIS_IO |= _BV(PS1_RESET); // set reset pin to input

    TRISAbits.TRISA2 = 1; // SS set to input
    TRISCbits.TRISC0 = 1; // SCK set to input
    TRISCbits.TRISC1 = 1; // SDI1 set to input
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
    
    // DEBUG
    UART_init();
    UART_print((uint8_t*)"PlayStation 1 mod:\n\r");
    
    // SETUP variables and arrays
    data_cnt = 0;
    cmd_cnt = 0;
    
    clear_buff((uint8_t*)cmd.buff, PS1_CTRL_BUFF_SIZE);
    clear_buff((uint8_t*)data.buff, PS1_CTRL_BUFF_SIZE);
    
    // delay before enabling interrupts
    __delay_s(REBOOT_DELAY); // wait 20 sec before next reset
    
    // SETUP INTERRUPTS
    PIR1bits.SSP1IF = 0; // clear SPI1 flag
    PIR2bits.SSP2IF = 0; // clear SPI2 flag
    PIE1bits.SSP1IE = 1; // enable MSSP interrupt (SPI1)
    PIE2bits.SSP2IE = 1; // enable MSSP interrupt (SPI2)
       
    INTCONbits.PEIE = 1; // peripheral interrupt enable
    INTCONbits.GIE = 1; // global interrupt enable
       
    // MAIN LOOP
    for(;;){
        if (data_cnt >= PS1_CTRL_BUFF_SIZE || cmd_cnt >= PS1_CTRL_BUFF_SIZE){
            INTCONbits.PEIE = 0; // peripheral interrupt disable
            UART_print((uint8_t*)"\n\rCMD: ");
            for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
                reverse_byte((uint8_t*)&cmd.buff[i]);
                UART_printHex((uint8_t)cmd.buff[i]);
                UART_print((uint8_t*)" ");
            }
            UART_print((uint8_t*)"\n\rDATA: ");
            for (i = 0; i < PS1_CTRL_BUFF_SIZE; i++){
                reverse_byte((uint8_t*)&data.buff[i]);
                UART_printHex((uint8_t)data.buff[i]);
                UART_print((uint8_t*)" ");
            }
            UART_print((uint8_t*)"\n\rData done converting");
            
            // Check first command for device selected
            if (cmd.device_select == CMD_SEL_CTRL_1 && cmd.command == CMD_READ_SW){
                // Check ID
                switch(data.id){
                    case ID_GUNCON_CTRL:
                        // Check switch combo
                        switch(data.switches){
                            case KEY_COMBO_GUNCON:
                                reset_long();
                                break;
                        }
                        break;
                    case ID_DIG_CTRL:
                    case ID_ANS_CTRL:
                    case ID_ANP_CTRL:
                    case ID_DS2_CTRL:
                        // Check switch combo
                        switch(data.switches){
                            case KEY_COMBO_CTRL:
                                reset_short();
                                break;
                            case KEY_COMBO_XSTATION:
                                reset_long();
                                break;
                        }
                        break;
                }
            }

            clear_buff((uint8_t*)data.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
            clear_buff((uint8_t*)cmd.buff, PS1_CTRL_BUFF_SIZE); // clear controller stuff
    
            data_cnt = 0; // reset counters
            cmd_cnt = 0; // reset counters
            
            PIR1bits.SSP1IF = 0; // clear SPI1 flag
            PIR2bits.SSP2IF = 0; // clear SPI2 flag
            INTCONbits.PEIE = 1; // peripheral interrupt enable
        }
    }
    
    for(;;);
}

/* Reverse byte order 
 * The playstation send data LSb first,
 * but the PIC uses a shift left register
 * so the LSb from the PS1 becomes the MSb of the PIC
*/
void reverse_byte(uint8_t *b) {
   *b = (*b & 0xF0) >> 4 | (*b & 0x0F) << 4;
   *b = (*b & 0xCC) >> 2 | (*b & 0x33) << 2;
   *b = (*b & 0xAA) >> 1 | (*b & 0x55) << 1;
}

/* Clear buffer of size s, aka fill with zero */
void clear_buff(uint8_t *p, uint8_t s){
    uint8_t i;
    for (i = 0; i < s; i++)
        p[i] = 0;
}

/* delay in seconds */
void __delay_s(uint8_t s){
    for (; s > 0; s--)
        __delay_ms(1000);
}


void reset_short(void){
    UART_print((uint8_t*)"\n\rShort Reset successful\n\r");
    // change RESET pin from input to output, logic low (PORT is already 0)
    PS1_TRIS_IO &= ~_BV(PS1_RESET);
    __delay_ms(SHORT_DELAY); // hold reset for 500ms
    // change RESET back to input
    PS1_TRIS_IO |= _BV(PS1_RESET);
    // hold the mcu from resetting too early, just in case
    __delay_s(REBOOT_DELAY);
}

void reset_long(void){
    UART_print((uint8_t*)"\n\rLong Reset successful\n\r");
    // change RESET pin from input to output, logic low (PORT is already 0)
    PS1_TRIS_IO &= ~_BV(PS1_RESET);
    __delay_s(LONG_DELAY); // hold reset for 2s
    // change RESET back to input
    PS1_TRIS_IO |= _BV(PS1_RESET);
    // hold the mcu from resetting too early, just in case
    __delay_s(REBOOT_DELAY);
}

#ifdef DEBUG
/**********************************************************/
/* UART Debug Functions */

/* Initialize UART with TX on output RC3 */
void UART_init(void){
    // SETUP UART
    ANSELCbits.ANSC3 = 0; // TX set to digital I/O
    LATCbits.LATC3 = 0; // set output 0
    TRISCbits.TRISC3 = 0; // TX set to output
    RC3PPSbits.RC3PPS = 0x14; // TX = RC3

    SP1BRGL = 51; // 9615 baud
    SP1BRGH = 0; // 9615 baud
    
    TX1STAbits.SYNC = 0; // Asynch mode
    RC1STAbits.SPEN = 1; // Serial Port enable bit
    TX1STAbits.BRGH = 0; // low speed
    BAUD1CONbits.BRG16 = 0; // 8 bit baud rate
    BAUD1CONbits.SCKP = 0; // Idle state for TX high level
    CLKRCONbits.CLKRDIV = 0; // Fosc
    CLKRCONbits.CLKRDC = 2; // 50% duty cyle
    CLKRCONbits.CLKREN = 1; // Enable CLK reference
    
    TX1STAbits.TXEN = 1; // Enable transmitter  
}

/* Send byte and wait until transfer finished */
void UART_sendByte(uint8_t c){
    TX1REG = c;
    while(!TX1STAbits.TRMT);
}

/* Send string */
void UART_print(uint8_t *str){
    uint8_t i;
    for (i = 0; str[i] != 0 && i < 255; i++)
        UART_sendByte(str[i]);
}

/* Print hex byte in format 0x%02X */
void UART_printHex(uint8_t b){
    uint8_t low_nibble = b & 0x0F;
    uint8_t high_nibble = (b >> 4) & 0x0F;
    
    UART_print((uint8_t*)"0x");
    UART_sendByte(high_nibble >= 10 ? (high_nibble - 10 + 'A') : high_nibble + '0');
    UART_sendByte(low_nibble >= 10 ? (low_nibble - 10 + 'A') : low_nibble + '0');
}
#endif
