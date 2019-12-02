/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

 bsp.c   // board support package for clicker-2 FK40

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************/

#include <bsp.h>
#include <xc.h>
#include "pinout.h"

#ifdef UART_SHARED
    #warning "This configuration shares the UART with the ICSP pins"
#else
#warning "This configuration does not share the UART with ICSP pins"
#endif

void BSP_init(void)
{
    // init I/Os
    LED_GREEN_TRIS = OUTPUT_PIN;
    LED_RED_TRIS   = OUTPUT_PIN;
    ICSP_DAT_DIGITAL();
    ICSP_TRIS_DAT = INPUT_PIN; // *should* be default ...

    // init UART
    RCSTA = 0x90;       	// SP enable, continuous RX enable
    TXSTA = 0x24;       	// TX enable BRGH=1
    SPBRG = 0xE2;
    SPBRGH = 0x04;      	// 48MHz -> 9600 baud
    BAUDCON = 0x08;     	// BRG16 = 1
    char c = RCREG;         // read
}

bool BUTTON_isPressed(void)
{
    return  (BTN_PORT == BUTTON_PRESSED) ? true : false;
}

void LED_set(COLOR c)
{
    switch(c){
        case RED:
            LED_GREEN_LAT = LED_OFF;
            LED_RED_LAT   = LED_ON;
            break;
        case GREEN:
            LED_GREEN_LAT = LED_ON;
            LED_RED_LAT   = LED_OFF;
            break;
        case ORANGE:
            LED_GREEN_LAT = LED_ON;
            LED_RED_LAT   = LED_ON;
            break;
        default:
        case BLACK:
            LED_GREEN_LAT = LED_OFF;
            LED_RED_LAT   = LED_OFF;
            break;
    }
}

void UART_baudrateSet(uint32_t dwBaud)
{
    uint32_t dwDivider = ((_XTAL_FREQ/4) / dwBaud) - 1;
    SPBRG = (uint8_t) dwDivider;
    SPBRGH = (uint8_t)((uint16_t) (dwDivider >> 8));
}

void UART_enable(void)
{
    RCSTAbits.SPEN = 1;         // enable UART, control I/Os
}

void UART_disable(void)
{
    RCSTAbits.SPEN = 0;         // disable UART, control I/Os
}

char UART_getch(void)
{
	char  c;

	if (RCSTAbits.OERR)  // in case of overrun error
	{                    // we should never see an overrun error, but if we do,
		RCSTAbits.CREN = 0;  // reset the port
		c = RCREG;
		RCSTAbits.CREN = 1;  // and keep going.
	}
	else
    {
		c = RCREG;
    }
	return c;
}

void UART_putch(char c)
{
    while(!UART_TxRdy());
    TXREG = c;
}
