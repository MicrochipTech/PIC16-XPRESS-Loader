/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

System Initialization

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

#include <xc.h>
#include "system.h"
//#include "system_config.h"
#include "usb.h"
#include "fileio.h"
#include "lvp.h"

/** CONFIGURATION Bits **********************************************/
// PIC 16F145x fuse configuration:
// Config word 1 (Oscillator configuration)
    // CONFIG1
    #pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
    #pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
    #pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
    #pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is reserved)
    #pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
    #pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
    #pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
    #pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
    #pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

    // CONFIG2
    #pragma config WRT = BOOT        // Flash Memory Self-Write Protection (Write protection off)
    #pragma config CPUDIV = NOCLKDIV// CPU System Clock Selection Bit (NO CPU system divide)
    #pragma config USBLSCLK = 48MHz // USB Low SPeed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
    #pragma config PLLMULT = 3x     // PLL Multipler Selection Bit (3x Output Frequency Selected)
    #pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Enabled)
    #pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
    #pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
    #pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
    #pragma config LVP = ON         // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

/*********************************************************************
* Function: void SYSTEM_Initialize( SYSTEM_STATE state )
*
* Overview: Initializes the system.
*
* PreCondition: None
*
* Input:  SYSTEM_STATE - the state to initialize the system into
*
* Output: None
*
********************************************************************/
void SYSTEM_init( void )
{
    OSCCONbits.IRCF = 0xf;      // 16 MHz HFINTOSC 3x PLL = 48MHz
    ACTCON = 0x90;              // ACT enabled, FS-USB mode
    BSP_init();
    LED_set(GREEN);
    ICSP_slaveRun();
    USBDeviceInit();	//usb_device.c.  Initializes USB module SFRs and firmware
    					//variables to known states.
}

void interrupt SYS_InterruptHigh(void)
{
    #if defined(USB_INTERRUPT)
        USBDeviceTasks();
    #endif
}
