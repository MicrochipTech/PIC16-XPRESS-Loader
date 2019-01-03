/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

MSD Direct Programming (ICSP) Interface

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

#include <string.h>
#include <fileio_config.h>
#include <fileio.h>
#include <direct.h>
#include "files.h"
#include "lvp.h"

#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Global Variables
 *****************************************************************************/
static FILEIO_MEDIA_INFORMATION mediaInformation;
bool ParseHex(char c);

/******************************************************************************
 * Function:        uint8_t MediaDetect(void* config)
 * PreCondition:    InitIO() function has been executed.
 * Input:           void
 * Output:          true   - Card detected
 *                  false   - No card detected
 *****************************************************************************/
uint8_t DIRECT_MediaDetect(void* config)
{
    return true;
}//end MediaDetect

/******************************************************************************
 * Function:        uint16_t SectorSizeRead(void)
 * PreCondition:    MediaInitialize() is complete
 * Input:           void
 * Output:          uint16_t - size of the sectors for this physical media.
 *****************************************************************************/
uint16_t DIRECT_SectorSizeRead(void* config)
{
    return FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
}

/******************************************************************************
 * Function:        uint32_t ReadCapacity(void)
 * PreCondition:    MediaInitialize() is complete
 * Input:           void
 * Output:          uint32_t - size of the "disk" - 1 (in terms of sector count).
 *                  Ex: In other words, this function returns the last valid
 *                  LBA address that may be read/written to.
 *****************************************************************************/
uint32_t DIRECT_CapacityRead(void* config)
{
    return ((uint32_t)DRV_TOTAL_DISK_SIZE - 1);
}

/******************************************************************************
 * Function:        uint8_t MediaInitialize(void)
 * Input:           None
 * Output:          Returns a pointer to a MEDIA_INFORMATION structure
 * Overview:        MediaInitialize initializes the media card and supporting variables.
 *****************************************************************************/
FILEIO_MEDIA_INFORMATION * DIRECT_MediaInitialize(void* config)
{
    mediaInformation.validityFlags.bits.sectorSize = true;
    mediaInformation.sectorSize = FILEIO_CONFIG_MEDIA_SECTOR_SIZE;
    mediaInformation.errorCode = MEDIA_NO_ERROR;
    return &mediaInformation;
}

/******************************************************************************
 * Function:   uint8_t SectorRead(uint32_t sector_addr, uint8_t *buffer, seg)
 * Input:      sector_addr - Sector address, each sector contains 512-byte
 *             buffer      - Buffer where data will be stored
 *             seg         - 64-byte segment of a sector
 * Output:     Returns true if read successful, false otherwise
 *****************************************************************************/
uint8_t DIRECT_SectorRead(void* config, uint32_t sector_addr, uint8_t* buffer, uint8_t seg)
{
    // Read a sector worth of data, and copy it to the specified RAM "buffer"
    if      ( 0 == sector_addr)     MasterBootRecordGet( buffer, seg);
    else if ( 1 == sector_addr)     VolumeBootRecordGet( buffer, seg);
    else if ( 2 == sector_addr)     {
        FATRecordGet( buffer, seg);
    }
    else if ( 3 == sector_addr)     {
        RootRecordGet( buffer, seg);
    }
    else {
        memset(buffer, '\0', MSD_IN_EP_SIZE); // empty buffer
        if ( 4 == sector_addr) {
            // Service README.HTM
            if ( seg < ( (readme_size() + 63) % 64) )
                strncpy( (void*)buffer,
                         (void*)&readme[seg*64],
                         64);  // at most 64 bytes at a time
        }
    }
	return true;
}//end SectorRead

/******************************************************************************
 * Function:        uint8_t SectorWrite(uint32_t sector_addr, uint8_t *buffer, uint8_t seg)
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be read from
 *                  seg         - 64-byte segment of a 512 sector
 * Output:          Returns true if write successful, false otherwise
 *****************************************************************************/
uint8_t DIRECT_SectorWrite(void* config, uint32_t sector_addr, uint8_t* buffer, uint8_t seg)
{
    if ((sector_addr < 2) || (sector_addr >= DRV_TOTAL_DISK_SIZE))
    {
        return false;
    }
    if ( 2 == sector_addr) {            // updating the FAT table - RAM
        FATRecordSet( buffer, seg);     // update the RAM (fabricated) image
        return true;
    }
    if ( 3 == sector_addr) {            // update of the root directory
        RootRecordSet( buffer, seg);
        return true;
    }

    // all remaining data sectors are parsed and programmed directly into the device
    uint8_t i=0;
    while ((i++ < 64) && ParseHex(*buffer++));

    return true;
} // SectorWrite

/******************************************************************************
 * Function:        uint8_t WriteProtectState(void)
 * Output:          uint8_t    - Returns always false (never protected)
 *****************************************************************************/
uint8_t DIRECT_WriteProtectStateGet(void* config)
{
    return false;
}

/*******************************************************************************
 Direct Hex File Parsing and Programming State Machine

 This is a simple state machine that parses an input stream to detect and decode
 the INTEL Hex file format produced by the MPLAB XC8 compiler
 Bytes are assembled in Words
 Words are assembled in Rows (currently supporting fixed size of 32-words)
 Rows are aligned (normalized) and written directly to the target using LVP ICSP
 Special treatment is reserved for words written to 'configuration' addresses
 ******************************************************************************/
bool isDigit( char * c){
    if (*c < '0') return false;
    *c -= '0';
    if (*c > 9)  {
        *c &= 0xdf;         // reduce to upper case
        *c-=7;              // A-F
    }
    if (*c > 0xf)  return false;
    return true;
}

// the actual state machine - Hex Machina
enum hexstate { SOL, BYTE_COUNT, ADDRESS, RECORD_TYPE, DATA, CHKSUM};

uint8_t  data[16]@0x3E0;    // manually allocated to ensure optimal RAM usage!

/**
 * Parser, main state machine decoding engine
 *
 * @param c     input character
 * @return      true = success, false = decoding failure/invalid file contents
 */
bool ParseHex(char c)
{
    static enum hexstate state = SOL;
    static uint8_t  bc;
    static uint8_t  data_count;
    static uint32_t address;
    static uint32_t ext_address = 0;
    static uint8_t  checksum;
    static uint8_t  record_type;
    static uint8_t  data_byte;
    static uint8_t  data_index;

    switch( state){
        case SOL:
            if (c == '\r') break;
            if (c == '\n') break;
            if (c != ':') return false;
            state = BYTE_COUNT;
            bc = 0;
            address = 0;
            checksum = 0;
            break;
        case BYTE_COUNT:
            if ( isDigit( &c) == false) { state = SOL; return false; }
            bc++;
            if (bc == 1)
                data_count = c;
            if (bc == 2 )  {
                data_count = (data_count << 4) + c;
                checksum += data_count;
                bc = 0;
                if (data_count > 16) { state = SOL; return false; }
                state = ADDRESS;
            }
            break;
        case ADDRESS:
            if ( isDigit( &c) == false) { state = SOL; return false;}
            bc++;
            if (bc == 1)
                address = c;
            else  {
                address = (address << 4) + (uint32_t)c;
                if (bc == 4) {
                    checksum += (address>>8) + address;
                    bc = 0;
                    state = RECORD_TYPE;
                }
            }
            break;
        case RECORD_TYPE:
            if ( isDigit( &c) == false) { state = SOL; return false;}
            bc++;
            if (bc == 1)
                if (c != 0) { state = SOL; return false; }
            if (bc == 2)  {
                record_type = c;
                checksum += c;
                bc = 0;
                state = DATA;  // default
                data_index = 0;
                memset(data, 0xff, sizeof(data));
                if (record_type == 0) break; // data record
                if (record_type == 1) { state = CHKSUM; break; }  // EOF record
                if (record_type == 4) break; // extended address record
                state = SOL;
                return false;
            }
            break;
        case DATA:
            if ( isDigit( &c) == false) { state = SOL; return false;}
            bc++;
            if (bc == 1)
                data[data_index] = (c<<4);
            if (bc == 2)  {
                bc = 0;
                data[data_index] += c;
                checksum +=  data[data_index];
                data_index++;
                if (data_index == data_count) {
                    state = CHKSUM;
                }
            }
            break;
        case CHKSUM:
            if ( isDigit( &c) == false) { state = SOL; return false;}
            bc++;
            if (bc == 1)
                checksum += (c<<4);
            if (bc == 2)  {
                bc = 0;
                checksum += c;
                if (checksum != 0) {
                    state = SOL;
                    return false;
                }
                // chksum is good
                state = SOL;
                if (record_type == 0)
                    LVP_packRow( ext_address + address, data, data_count);
                else if (record_type == 4)
                    ext_address = ((uint32_t)(data[0]) << 24) + ((uint32_t)(data[1]) << 16);
                else if (record_type == 1) {
                    LVP_programLastRow();
                    ext_address = 0;
                }
                else return false;
            }
            break;
        default:
            break;
    }
    return true;
}
