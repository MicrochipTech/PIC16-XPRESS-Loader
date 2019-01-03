/*******************************************************************************
Copyright 2017 Microchip Technology Inc. (www.microchip.com)

 Low Voltage Programming Interface

  Bit-Banged implementation of the PIC18'K40 LVP protocol
  Based on the PIC18F25Q10 specification

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
#include "lvp.h"
#include "bsp.h"
#include <string.h>
#include <stdlib.h>

// device specific parameters (DS40001874E)
#define ROW_SIZE    128      // width of a flash row in words PIC18F67Q10!!
#define INDEX_MASK  (ROW_SIZE-1)<<1
#define ROW_MASK    ~((INDEX_MASK)+1)

#define UID_ADDRESS 0x200000 // address of UID words area
#define CFG_ADDRESS 0x300000 // address of config words area
#define EE_ADDRESS  0x310000 // address of data EEPROM
#define REV_ID      0x3FFFFC // silicon revision ID
#define DEV_ID      0x3FFFFE // product ID
#define CFG_NUM     6        // number of config words

#define WRITE_TIME  50       // mem write time us!
#define CFG_TIME    50       // cfg write time us!
#define BULK_TIME   75       // bulk erase time ms

/****************************************************************************/
// internal state
uint16_t row[ROW_SIZE]@0x4C0;  // buffer containing row being formed in bank9
uint32_t row_address = -1;      // destination address of current row

// ICSP commands
#define  CMD_LOAD_ADDR        0x80
#define  CMD_INC_ADDR         0xF8
#define  CMD_READ_DATA        0xFC
#define  CMD_READ_DATA_IA     0xFE
#define  CMD_PROG_DATA        0xC0
#define  CMD_PROG_DATA_IA     0xE0
#define  CMD_BULK_ERASE       0x18

void ICSP_slaveReset(void){
    ICSP_nMCLR = SLAVE_RESET;
    ICSP_TRIS_nMCLR = OUTPUT_PIN;
}

void ICSP_slaveRun(void){
    ICSP_nMCLR = SLAVE_RESET;
    ICSP_TRIS_nMCLR = INPUT_PIN;
}

void ICSP_control(void )
{
    __delay_us(1);
    ICSP_TRIS_DAT = INPUT_PIN;
    ICSP_CLK = 0;
    ICSP_TRIS_CLK = OUTPUT_PIN;
}

void ICSP_release(void)
{
    ICSP_TRIS_DAT  = INPUT_PIN;
    ICSP_TRIS_CLK  = INPUT_PIN;
    ICSP_slaveRun();
}

void ICSP_sendCmd(uint8_t b)
{
    uint8_t i;
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for( i=0; i<8; i++){        // 8-bit commands
        if ((b & 0x80) > 0)
            ICSP_DAT = 1;       // Msb first
        else
            ICSP_DAT = 0;
        ICSP_CLK = 1;
        b <<= 1;                // shift left
        __delay_us(1);
        ICSP_CLK = 0;
        __delay_us(1);
    }
    __delay_us(1);
}

void ICSP_sendData( uint24_t w)
{
    uint8_t i;

    w = (w <<1) & 0x7ffffe;     // add start and stop bits
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for( i=0; i < 24; i++){
        if ((w & 0x800000) > 0) // Msb first
            ICSP_DAT = 1;
        else
            ICSP_DAT = 0;
        ICSP_CLK = 1;
        w <<= 1;
        __delay_us(1);
        ICSP_CLK = 0;
        __delay_us(1);
    }
}

void ICSP_signature(void){
    ICSP_slaveReset();          // MCLR output => Vil (GND)
    __delay_ms(10);
    ICSP_sendCmd('M');
    ICSP_sendCmd('C');
    ICSP_sendCmd('H');
    ICSP_sendCmd('P');
    __delay_ms(5);
}

uint16_t ICSP_getData( void)
{
    uint8_t i;
    uint24_t w = 0;
    ICSP_TRIS_DAT = INPUT_PIN;
    for( i=0; i < 24; i++){     // 24 bit
        ICSP_CLK = 1;
        w <<= 1;                // shift left
        __delay_us(1);
        w |= ICSP_DAT_IN;       // read port, msb first (loose top byte)
        ICSP_CLK = 0;
        __delay_us(1);
    }
    return (w >> 1) & 0xffff;
}

uint16_t ICSP_read(void)
{
    ICSP_sendCmd(CMD_READ_DATA_IA);
    return ICSP_getData();
}

void ICSP_bulkErase(void)
{
    ICSP_sendCmd(CMD_LOAD_ADDR);  // enter config area to erase config words too
    ICSP_sendData(CFG_ADDRESS);
    ICSP_sendCmd(CMD_BULK_ERASE);
    __delay_ms(BULK_TIME);
}

void ICSP_skip(uint16_t count)
{
    while(count-- > 0){
        ICSP_sendCmd(CMD_INC_ADDR);     // increment address
    }
}

void ICSP_addressLoad(uint24_t address)
{
    ICSP_sendCmd(CMD_LOAD_ADDR);
    ICSP_sendData(address);
}

void ICSP_rowWrite(uint16_t *buffer, uint8_t count)
{
    while(count-- > 0){
        ICSP_sendCmd(CMD_PROG_DATA);
        ICSP_sendData(*buffer++);
        __delay_us(WRITE_TIME);         // NOTE: micro-seconds for the 340K process
        ICSP_sendCmd(CMD_INC_ADDR);
    }
}

void ICSP_cfgWrite(uint16_t *buffer, uint8_t count)
{
    ICSP_addressLoad(CFG_ADDRESS);
    while(count-- > 0){
        ICSP_sendCmd(CMD_PROG_DATA);
        ICSP_sendData(*buffer++);
        __delay_us(WRITE_TIME);     // NOTE: micro-seconds for the 340K process
        ICSP_sendCmd(CMD_INC_ADDR);
    }
}

/****************************************************************************/

void LVP_enter(void)
{
    ICSP_control();                 // configure I/Os
    ICSP_signature();               // enter LVP mode
}

void LVP_exit(void)
{
    ICSP_release();                 // release ICSP-DAT and ICSP-CLK
    row_address = -1;
}

bool LVP_inProgress(void)
{
    return (ICSP_TRIS_nMCLR == SLAVE_RESET);
}

void LVP_write( void){
    // check for first entry in lvp
    if (!LVP_inProgress()) {
        LVP_enter();
        ICSP_bulkErase();
    }
    if (row_address == -1){
    }
    else if (row_address >= (CFG_ADDRESS >> 1)) {    // use the special cfg word sequence
        ICSP_cfgWrite(row, CFG_NUM);
    }
    else if(row_address >= (UID_ADDRESS >> 1)){
        ICSP_addressLoad(row_address << 1);
        ICSP_rowWrite(row, 8);
    }
    else { // normal row programming sequence
        ICSP_addressLoad(row_address << 1);
        ICSP_rowWrite(row, ROW_SIZE);
    }
}

void LVP_commitRow( void) {
    // latch and program a row, skip if blank
    uint8_t i;
    uint16_t chk = 0xffff;
    for( i=0; i< ROW_SIZE; i++) chk &= row[i];  // blank check
    if (chk != 0xffff) {
        LVP_write();
        memset((void*)row, 0xff, sizeof(row));    // fill buffer with blanks
    }
}

/**
 * Align and pack words in rows, ready for lvp programming
 * @param address       starting address
 * @param data          buffer
 * @param data_count    number of bytes
 */
void LVP_packRow( uint32_t address, uint8_t *data, uint8_t data_count) {
    // copy only the bytes from the current data packet up to the boundary of a row
    uint8_t  index = (address & INDEX_MASK) >> 1;
    uint32_t new_row = (address & ROW_MASK) >> 1;
    if (new_row != row_address) {
        LVP_commitRow();
        row_address = new_row;
    }
    // ensure data is always even (rounding up)
    data_count = (data_count+1) & 0xfe;
    // copy data up to the row boundaries
    while ((data_count > 0) && (index < ROW_SIZE)){
        uint16_t word = *data++;
        word += ((uint16_t)(*data++)<<8);
        row[index++] = word;
        data_count -= 2;
    }
    // if a complete row was filled, proceed to programming
    if (index == ROW_SIZE) {
        LVP_commitRow();
        // next consider the split row scenario
        if (data_count > 0) {   // leftover must spill into next row
            row_address += ROW_SIZE;
            index = 0;
            while (data_count > 0){
                uint16_t word = *data++;
                word += ((uint16_t)(*data++)<<8);
                row[index++] = word;
                data_count -= 2;
            }
        }
    }
}

void LVP_programLastRow( void) {
    LVP_commitRow();
    LVP_exit();
}

