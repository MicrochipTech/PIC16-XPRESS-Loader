/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

 Low Voltage Programming Interface

  Bit-Banged implementation of the PIC18F (290K) LVP protocol
  Based on the PIC18'K42 specification

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

// device specific parameters (DS40001836A)
#define ROW_SIZE     32      // width of a flash row in words
#define CFG_ADDRESS 0x300000 // address of config words area
#define UID_ADDRESS 0x200000 // address of UID words area
#define EE_ADDRESS  0x310000 // address of data EE
#define DIA_ADDRESS 0x3F0000 // Device Information Area
#define CAL_ADDRESS 0x3F0024 // Calibration area
#define REV_ID      0x3FFFFC // silicon revision ID
#define DEV_ID      0x3FFFFE // product ID
#define DEV_ERSIZ   0x3FFF00 // row size (words)
#define DEV_URSIZ   0x3FFF04 // number of user rows
#define DEV_EESIZ   0x3FFF06 // data EE size (bytes)
#define CFG_NUM      5       // number of config words

#define WRITE_TIME   3       // mem write time ms
#define CFG_TIME     6       // cfg write time ms
#define BULK_TIME   26       // bulk erase time ms

/****************************************************************************/
// internal state
uint16_t row[ ROW_SIZE];     // buffer containing row being formed
uint32_t row_address = -1;   // destination address of current row

// ICSP commands
#define  CMD_LOAD_ADDR        0x80
#define  CMD_INC_ADDR         0xF8
#define  CMD_LATCH_DATA       0x00
#define  CMD_LATCH_DATA_IA    0x02
#define  CMD_READ_DATA        0xFC
#define  CMD_READ_DATA_IA     0xFE
#define  CMD_BEGIN_PROG       0xE0
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
    while(count-- > 1){             // load n-1 latches
        ICSP_sendCmd(CMD_LATCH_DATA_IA);
        ICSP_sendData(*buffer++);
    }
    ICSP_sendCmd(CMD_LATCH_DATA);   // load last latch (n-1)
    ICSP_sendData(*buffer++);
    ICSP_sendCmd(CMD_BEGIN_PROG);
    __delay_ms(WRITE_TIME);
    ICSP_sendCmd(CMD_INC_ADDR);     // increment address only after prog. command!
}

void ICSP_cfgWrite(uint16_t *buffer, uint8_t count)
{
    ICSP_addressLoad(CFG_ADDRESS);
    while(count-- > 0){
        ICSP_sendCmd(CMD_LATCH_DATA);
        ICSP_sendData(*buffer++);
        ICSP_sendCmd(CMD_BEGIN_PROG);
        __delay_ms(CFG_TIME);
        ICSP_sendCmd(CMD_INC_ADDR);
    }
}

/****************************************************************************/

void LVP_enter(void)
{
#ifdef UART_SHARED
    UART_disable();                 // release shared I/Os
#endif
    ICSP_control();                 // configure ICSP I/Os
    ICSP_signature();               // enter LVP mode
}

void LVP_exit(void)
{
#ifdef UART_SHARED
    UART_enable();
#endif
    ICSP_release();                 // release ICSP-DAT and ICSP-CLK
    row_address = -1;
}

bool LVP_inProgress(void)
{
    return (ICSP_TRIS_nMCLR == OUTPUT_PIN);
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
        // Ignore for now..
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
    uint8_t  index = (address & 0x3e) >> 1;
    uint32_t new_row = (address & 0xfffffffc0) >> 1;
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

