/*
 * File:   pinout.h
 *
 * pinout assignements for XPRESS series
 */

#ifndef PINOUT_H
#define	PINOUT_H

#define _XTAL_FREQ          48000000L

#define INPUT_PIN           1
#define OUTPUT_PIN          0
#define DIGITAL_PIN         0
#define ANALOG_PIN          1

#define SLAVE_RUN           INPUT_PIN   // pull up
#define SLAVE_RESET         OUTPUT_PIN  // drive low

// LED pin mapping
#define LED_GREEN_LAT       LATCbits.LATC2
#define LED_RED_LAT         LATCbits.LATC3
#define LED_GREEN_TRIS      TRISCbits.TRISC2
#define LED_RED_TRIS        TRISCbits.TRISC3
#define LED_ON              1
#define LED_OFF             0

// ICSP pin mapping
#define ICSP_DAT_DIGITAL()                        // this pins is always digital
#define ICSP_TRIS_DAT       TRISCbits.TRISC4
#define ICSP_DAT            LATCbits.LATC4
#define ICSP_DAT_IN         PORTCbits.RC4
#define ICSP_TRIS_CLK       TRISCbits.TRISC5
#define ICSP_CLK            LATCbits.LATC5
#define ICSP_TRIS_nMCLR     TRISAbits.TRISA4
#define ICSP_nMCLR          LATAbits.LATA4

#define BTN_PORT            PORTAbits.RA5
#define BUTTON_PRESSED      0

#define UART_TRISTx         TRISCbits.TRISC4
#define UART_TRISRx         TRISCbits.TRISC5
#define UART_Tx             PORTCbits.RC4
#define UART_Rx             PORTCbits.RC5

// Use following only for Hardware Flow Control
//#define UART_DTS          PORTAbits.RA3
//#define UART_DTR          LATCbits.LATC3
//#define UART_RTS          LATBbits.LATB4
//#define UART_CTS          PORTBbits.RB6

//#define mInitRTSPin() {TRISBbits.TRISB4 = 0;}   //Configure RTS as a digital output.
//#define mInitCTSPin() {TRISBbits.TRISB6 = 1;}   //Configure CTS as a digital input.  (Make sure pin is digital if ANxx functions is present on the pin)
//#define mInitDTSPin() {}//{TRISAbits.TRISA3 = 1;}   //Configure DTS as a digital input.  (Make sure pin is digital if ANxx functions is present on the pin)
//#define mInitDTRPin() {TRISCbits.TRISC3 = 0;}   //Configure DTR as a digital output.

/******** USB stack hardware selection options *********************/

//#define USE_SELF_POWER_SENSE_IO
#if defined(USE_SELF_POWER_SENSE_IO)
#define tris_self_power     TRISAbits.TRISA2    // Input
#define self_power          PORTAbits.RA2
#else
#define self_power          1
#endif

//#define USE_USB_BUS_SENSE_IO		//JP1 must be in R-U position to use this feature on this board
#if defined(USE_USB_BUS_SENSE_IO)
#define tris_usb_bus_sense  TRISBbits.TRISB5    // Input
#define USB_BUS_SENSE       PORTBbits.RB5
#else
#define USB_BUS_SENSE       1
#endif


#endif	/* PINOUT_H */

