/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

 CDC Device Implementation

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

/** INCLUDES *******************************************************/
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "system.h"
#include "usb.h"
#include "usb_device_cdc.h"
#include "app_device_cdc.h"
#include "usb_config.h"

/** VARIABLES ******************************************************/

static uint8_t USBOutBuffer[CDC_DATA_OUT_EP_SIZE]@0x3A0;           // bank7
static uint8_t SerialBuffer[CDC_DATA_IN_EP_SIZE]@0x420;            // bank8

uint8_t     NextUSBOut;
uint8_t     LastSerialOut;  // Number of characters in the buffer
uint8_t     SerialCp;       // current position within the buffer
bool        SerialBuffer_Rdy = false;

/*********************************************************************
* Function: void APP_DeviceCDCEmulatorInitialize(void);
* Overview: Initializes the interface
********************************************************************/
void APP_DeviceCDCEmulatorInitialize()
{
    CDCInitEP();
    line_coding.bCharFormat = 0;
    line_coding.bDataBits = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate = 9600;

// 	 Initialize the arrays
   uint8_t i;
   for (i=0; i<sizeof(USBOutBuffer); i++){
      USBOutBuffer[i] = 0;
   }

	NextUSBOut = 0;
	LastSerialOut = 0;
}

/*********************************************************************
* Function: void APP_DeviceCDCEmulatorTasks(void);
*
* PreCondition: APP_DeviceCDCEmulatorInitialize() and APP_DeviceCDCEmulatorStart()
********************************************************************/
void APP_DeviceCDCEmulatorTasks()
{

    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;

	if (!SerialBuffer_Rdy)  // only check for new USB buffer if the old Serial buffer is
	{						  // empty.  This will cause additional USB packets to be NAK'd
		LastSerialOut = getsUSBUSART(SerialBuffer, 64); // until the buffer is free.
		if(LastSerialOut > 0)
		{
			SerialBuffer_Rdy = true;    // signal buffer full
			SerialCp = 0;               // reset the current position
		}
	}

//    Check if one or more bytes are waiting in the physical UART transmit
//    queue.  If so, send it out the UART TX pin.
	if(SerialBuffer_Rdy && UART_TxRdy())
	{
    	#if defined(USB_CDC_SUPPORT_HARDWARE_FLOW_CONTROL)
        	// make sure the receiving UART device is ready to receive data before
        	// actually sending it.
        	if(UART_CTS == USB_CDC_CTS_ACTIVE_LEVEL)
        	{
        		UART_putc(SerialBuffer[SerialCp]);
        		++SerialCp;
        		if (SerialCp == LastSerialOut)
        			SerialBuffer_Rdy = false;
    	    }
	    #else
	        //Hardware flow control not being used.  Just send the data.
    		UART_putch(SerialBuffer[SerialCp]);
    		++SerialCp;
    		if (SerialCp == LastSerialOut)
    			SerialBuffer_Rdy = false;
	    #endif
	}

    //Check if we received a character over the physical UART, and we need
    //to buffer it up for eventual transmission to the USB host.
	if (UART_RxRdy() && (NextUSBOut < (CDC_DATA_OUT_EP_SIZE - 1)))
    {
		USBOutBuffer[NextUSBOut] = UART_getch();
		++NextUSBOut;
		USBOutBuffer[NextUSBOut] = 0;
	}

	#if defined(USB_CDC_SUPPORT_HARDWARE_FLOW_CONTROL)
    	//Drive RTS pin, to let UART device attached know if it is allowed to
    	//send more data or not.  If the receive buffer is almost full, we
    	//deassert RTS.
    	if(NextUSBOut <= (CDC_DATA_OUT_EP_SIZE - 5u))
    	{
            UART_RTS = USB_CDC_RTS_ACTIVE_LEVEL;
        }
        else
        {
        	UART_RTS = (USB_CDC_RTS_ACTIVE_LEVEL ^ 1);
        }
    #endif

    //Check if any bytes are waiting in the queue to send to the USB host.
    //If any bytes are waiting, and the endpoint is available, prepare to
    //send the USB packet to the host.
	if((USBUSARTIsTxTrfReady()) && (NextUSBOut > 0))
	{
		putUSBUSART(&USBOutBuffer[0], NextUSBOut);
		NextUSBOut = 0;
	}

    CDCTxService();
}

/******************************************************************************
 * Function:        void APP_mySetLineCodingHandler(void)
 * PreCondition:    USB_CDC_SET_LINE_CODING_HANDLER is defined
 * Overview:        This function gets called when a SetLineCoding command
 *                  is sent on the bus.  This function will evaluate the request
 *                  and determine if the application should update the baudrate
 *                  or not.
 *****************************************************************************/
#if defined(USB_CDC_SET_LINE_CODING_HANDLER)
void APP_SetLineCodingHandler(void)
{
    //If the request is not in a valid range
    //if(cdc_notice.GetLineCoding.dwDTERate > 115200)
    //{
        //NOTE: There are two ways that an unsupported baud rate could be
        //handled.  The first is just to ignore the request and don't change
        //the values.  That is what is currently implemented in this function.
        //The second possible method is to stall the STATUS stage of the request.
        //STALLing the STATUS stage will cause an exception to be thrown in the
        //requesting application.  Some programs, like HyperTerminal, handle the
        //exception properly and give a pop-up box indicating that the request
        //settings are not valid.  Any application that does not handle the
        //exception correctly will likely crash when this requiest fails.  For
        //the sake of example the code required to STALL the status stage of the
        //request is provided below.  It has been left out so that this demo
        //does not cause applications without the required exception handling
        //to crash.
        //---------------------------------------
        //USBStallEndpoint(0,1);
    //}
    //else
    //{
    //Update the baudrate info in the CDC driver
    CDCSetBaudRate(cdc_notice.GetLineCoding.dwDTERate);
    UART_baudrateSet(line_coding.dwDTERate); // !!!
}
#endif
