/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

 USB Descriptors
 
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
 
/*********************************************************************
 * Descriptor specific type definitions are defined in:
 * usb_device.h
 *
 * Configuration options are defined in:
 * usb_config.h
 ********************************************************************/
#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

/** INCLUDES *******************************************************/
#include "usb.h"
#include "usb_device_msd.h"
#include "usb_device_cdc.h"

/** CONSTANTS ******************************************************/

/* Device Descriptor */
const USB_DEVICE_DESCRIPTOR device_dsc=
{
    0x12,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0xEF,					// Class Code "MISC_DEVICE" (ex: uses IAD descriptor)  
    0x02,                   // Subclass code
    0x01,                   // Protocol code
    USB_EP0_BUFF_SIZE,      // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID  "Microchip Technology Inc"
    0x0057,                 // Product ID RS232 Emulation
    0x0001,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x03,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/* Configuration 1 Descriptor */
const uint8_t configDescriptor1[]={
    /* Configuration Descriptor */
    9,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    98, 0,                  // Total length of data for this cfg
    3,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    2,                      // Configuration string index
    _DEFAULT | _SELF,       // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

//---------------MSD Function 1 Descriptors------------------------
    /* Interface Descriptor */
    9,   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    MSD_INTF_ID,            // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    MSD_INTF,               // Class code
    MSD_INTF_SUBCLASS,      // Subclass code
    MSD_PROTOCOL, 		    // Protocol code
    0,                      // Interface string index
    
    /* Endpoint Descriptor */
    7,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_IN,_BULK,
    MSD_IN_EP_SIZE,0x00,
    0x01,
    
    /* Endpoint Descriptor */
    7,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_OUT,
    _BULK,
    MSD_OUT_EP_SIZE,0x00,
    0x01,
    
    
//---------------IAD Descriptor------------------------------------
    /* Interface Association Descriptor: CDC Function 1*/ 
	0x08,             //sizeof(USB_IAD_DSC), // Size of this descriptor in bytes 
	0x0B,             // Interface assocication descriptor type 
	CDC_COMM_INTF_ID, // The first associated interface 
	2,                // Number of contiguous associated interface 
	COMM_INTF,        // bInterfaceClass of the first interface 
	ABSTRACT_CONTROL_MODEL, // bInterfaceSubclass of the first interface 
	V25TER,           // bInterfaceProtocol of the first interface 
	0,                // Interface string index 						

//---------------CDC Function 1 Descriptors------------------------
							
    /* Interface Descriptor: CDC Function 1, Status (communication) Interface */
    0x09,   //sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    CDC_COMM_INTF_ID,       // Interface Number
    0,                      // Alternate Setting Number
    1,                      // Number of endpoints in this intf
    COMM_INTF,              // Class code
    ABSTRACT_CONTROL_MODEL, // Subclass code
    V25TER,                 // Protocol code
    0,                      // Interface string index

    /* CDC Class-Specific Descriptors */
    //5 bytes: Header Functional Descriptor
    sizeof(USB_CDC_HEADER_FN_DSC), //Size of this descriptor in bytes (5)
    CS_INTERFACE,               //bDescriptorType (class specific)
    DSC_FN_HEADER,              //bDescriptorSubtype (header functional descriptor)
    0x20, 0x01,                 //bcdCDC (CDC spec version this fw complies with: v1.20 [stored in little endian])

    //4 bytes: Abstract Control Management Functional Descriptor
    sizeof(USB_CDC_ACM_FN_DSC), //Size of this descriptor in bytes (4)
    CS_INTERFACE,               //bDescriptorType (class specific)
    DSC_FN_ACM,                 //bDescriptorSubtype (abstract control management)
    USB_CDC_ACM_FN_DSC_VAL,     //bmCapabilities: (see PSTN120.pdf Table 4)

    //5 bytes: Union Functional Descriptor
    sizeof(USB_CDC_UNION_FN_DSC), //Size of this descriptor in bytes (5)
    CS_INTERFACE,                 //bDescriptorType (class specific)
    DSC_FN_UNION,                 //bDescriptorSubtype (union functional)
    CDC_COMM_INTF_ID,             //bControlInterface: Interface number of the communication class interface (1)
    CDC_DATA_INTF_ID,             //bSubordinateInterface0: Data class interface #2 is subordinate to this interface

    //5 bytes: Call Management Functional Descriptor
    sizeof(USB_CDC_CALL_MGT_FN_DSC), //Size of this descriptor in bytes (5)
    CS_INTERFACE,                    //bDescriptorType (class specific)
    DSC_FN_CALL_MGT,                 //bDescriptorSubtype (call management functional)
    0x00,                            //bmCapabilities: device doesn't handle call management
    CDC_DATA_INTF_ID,                //bDataInterface: Data class interface ID used for the optional call management

    /* Endpoint Descriptor */
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    _EP02_IN,                   //EndpointAddress
    _INTERRUPT,                 //Attributes
    CDC_COMM_IN_EP_SIZE,0x00,   //size
    0x02,                       //Interval

    /* Interface Descriptor: CDC Function 1, Data Interface*/
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,      // INTERFACE descriptor type
    CDC_DATA_INTF_ID,       // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    DATA_INTF,              // Class code
    0,                      // Subclass code
    NO_PROTOCOL,            // Protocol code
    0,                      // Interface string index
    
    /* Endpoint Descriptor */
    //sizeof(USB_EP_DSC),DSC_EP,_EP03_OUT,_BULK,CDC_BULK_OUT_EP_SIZE,0x00,
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    _EP03_OUT,            //EndpointAddress
    _BULK,                       //Attributes
    CDC_DATA_OUT_EP_SIZE,0x00,                  //size
    0x00,                       //Interval

    /* Endpoint Descriptor */
    //sizeof(USB_EP_DSC),DSC_EP,_EP03_IN,_BULK,CDC_BULK_IN_EP_SIZE,0x00
    0x07,/*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    _EP03_IN,            //EndpointAddress
    _BULK,                       //Attributes
    CDC_DATA_IN_EP_SIZE,0x00,                  //size
    0x00                       //Interval
};


//Language code(s) supported string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[1];}sd000={
    sizeof(sd000),
    USB_DESCRIPTOR_STRING,
    {0x0409} //0x0409 = Language ID code for US English
};
//Manufacturer string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[25];}sd001={
sizeof(sd001),USB_DESCRIPTOR_STRING,
{'M','i','c','r','o','c','h','i','p',' ',
'T','e','c','h','n','o','l','o','g','y',' ','I','n','c','.'
}};

//Product string descriptor
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[28];}sd002={
sizeof(sd002),USB_DESCRIPTOR_STRING,
{'M','i','c','r','o','c','h','i','p',' ','C','o','m','p','o','s','i','t','e',' ','D','e','v','i','c','e'
}};

//Serial number string descriptor.  Note: This should be unique for each unit 
//built on the assembly line.  Plugging in two units simultaneously with the 
//same serial number into a single machine can cause problems.  Additionally, not 
//all hosts support all character values in the serial number string.  The MSD 
//Bulk Only Transport (BOT) specs v1.0 restrict the serial number to consist only
//of ASCII characters "0" through "9" and capital letters "A" through "F".
const struct{uint8_t bLength;uint8_t bDscType;uint16_t string[12];}sd003={
sizeof(sd003),USB_DESCRIPTOR_STRING,
{'1','2','3','4','5','6','7','8','9','0','9','9'}};


//Array of configuration descriptors
const uint8_t *const USB_CD_Ptr[]=
{
    (const uint8_t *const)&configDescriptor1
};

//Array of string descriptors
const uint8_t *const USB_SD_Ptr[]=
{
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd002,
    (const uint8_t *const)&sd003
};

/** EOF usb_descriptors.c ***************************************************/

#endif
