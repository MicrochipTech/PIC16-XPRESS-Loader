/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

 Low Voltage Programming Interface

  Bit-Banged implementation of the PIC16F1 (290K) LVP protocol
  Based on the PIC16F188XX specification

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
#include "leds.h"
#include "uart.h"
#include <string.h>
#include <stdlib.h>

// device specific parameters (DS70005256A)
#define CFG_ADDRESS 0x300000 // address of config words area
#define REV_ID      0x3FFFFC // silicon revision ID
#define DEV_ID      0x3FFFFE // product ID
#define DEV_ERSIZ   0x3FFF00 // row size (words)
#define DEV_URSIZ   0x3FFF04 // number of user rows
#define DEV_EESIZ   0x3FFF06 // data EE size (bytes)

#define ROW_SIZE     128     // width of a flash row in words
#define CFG_NUM      5       // number of config words

#define WRITE_TIME   3       // mem write time ms
#define CFG_TIME     6       // cfg write time ms
#define BULK_TIME   26       // bulk erase time ms

/****************************************************************************/
// internal state
uint24_t row[ ROW_SIZE];     // buffer containing row being formed
uint32_t row_address;        // destination address of current row

// ICSP commands (via the Programming Executive)
#define CMD_SCHECK          0x0000
#define CMD_READ_DATA       0x2000
#define CMD_PROG_2W         0x3000
#define CMD_PROG_ROW        0x50C3      // PROGRAM ROW + length (192+3)
#define CMD_BULK_ERASE      0x7001      // BULK ERASE + length (1)

void ICSP_slaveReset(void){
    ICSP_nMCLR = SLAVE_RESET;
    ICSP_TRIS_nMCLR = OUTPUT_PIN;
}

void ICSP_slaveRun(void){
    ICSP_TRIS_nMCLR = INPUT_PIN;
    ICSP_nMCLR = SLAVE_RESET;
}

void ICSP_init(void )
{
    RCSTAbits.SPEN = 0;         // disable UART, control I/O
    __delay_us(1);
    ICSP_TRIS_DAT = INPUT_PIN;
    ICSP_CLK = 0;
    ICSP_TRIS_CLK = OUTPUT_PIN;
    ICSP_slaveRun();
}

void ICSP_release(void)
{
    ICSP_TRIS_DAT  = INPUT_PIN;
    ICSP_TRIS_CLK  = INPUT_PIN;
    UART_Initialize();
    ICSP_slaveRun();
}

void ICSP_sendWord( uint16_t w)
{
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for( uint8_t i=0; i<16; i++) {
        if ((w & 0x800000) > 0) // Msb first
            ICSP_DAT = 1;
        else
            ICSP_DAT = 0;
        ICSP_CLK = 1;           // rising edge latch
        w <<= 1;
        __delay_us(1);          // > P1A (200ns)
        ICSP_CLK = 0;
        __delay_us(1);          // > P1B (200ns)
    }
}

void ICSP_signature(void){
    ICSP_slaveReset();          // MCLR output => Vil (GND)
    __delay_us(1);              // > P6 (100ns))
    ICSP_slaveRun();
    __delay_us(200);            // < P21 (500us) short pulse
    ICSP_slaveReset();
    __delay_ms(2);              // > P18 (1ms)
    ICSP_sendWord(0x4D43);
    ICSP_sendWord(0x4850);
    __delay_us(1);              // > P19 (20ns)
    ICSP_slaveRun();            // release MCLR
    __delay_ms(55);             // > P7(50ms) + P1 (500us)*5
}

uint16_t ICSP_getWord( void)
{
    uint16_t w = 0;
    ICSP_TRIS_DAT = INPUT_PIN;  // PGD input
    for( uint8_t i=0; i<16; i++) {
        ICSP_CLK = 1;
        w <<= 1;                // shift left
        __delay_us(1);
        w |= ICSP_DAT_IN;       // read port
        ICSP_CLK = 0;
        __delay_us(1);
    }
    return w;
}

void ICSP_wait(void) {
    ICSP_TRIS_DAT = INPUT_PIN;  // PGD input
    __delay_us(20);             // > P8 (12us)
    while( ICSP_DAT_IN == 1 ) {
        // TODO timeout while waiting for command completion
    }
    __delay_us(25);             // > P9B (23us max)
}

uint16_t ICSP_read(void)
{
    ICSP_sendCmd(CMD_READ_DATA_IA);
    return ICSP_getData();
}

bool ICSP_bulkErase(void)
{
    ICSP_sendWord(CMD_BULK_ERASE);
    ICSP_wait();
    uint16_t res = ICSP_getWord();     // expect 0x1700
    uint16_t len = ICSP_getWord();     // expect 2
    return (res == 0x1700) && (len == 2);
}

void ICSP_rowWrite(uint24_t *buffer, uint8_t count)
{
    uint24_t w1, w2;    
    ICSP_sendWord(CMD_PROG_ROW);    // program a full row of 128 24-bit words
    ICSP_sendWord(0);               // reserved
    ICSP_sendWord(row_address>>16); // MSB destination
    ICSP_sendWord(row_address);     // LSB destination
    while(count > 0) {              
        count -= 2;                 // work on word pairs (2*24 -> 3*16)
        w1 = *buffer++; 
        w2 = *buffer++; 
        ICSP_sendWord(w1);
        ICSP_sendWord( ( (w2>>8) & 0xff00) + (w1>>16));
        ICSP_sendWord(w2);
    }

    ICSP_wait();
    uint16_t res = ICSP_getWord();     // expect 0x1700
    uint16_t len = ICSP_getWord();     // expect 2
    return (res == 0x1700) && (len == 2);
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
    LED_On(RED_LED);
    LED_Off(GREEN_LED);

    ICSP_init();                    // configure I/Os
    ICSP_signature();               // enter LVP mode

    // fill row buffer with blanks
    memset((void*)row, 0xff, sizeof(row));
    row_address = -1;
}

void LVP_exit(void)
{
    ICSP_release();                 // release ICSP-DAT and ICSP-CLK
    LED_Off(RED_LED);
    LED_On(GREEN_LED);
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
        /* do nothing */
    }
    else if (row_address >= (CFG_ADDRESS >> 1)) {    // use the special cfg word sequence
        ICSP_cfgWrite( &row, CFG_NUM);
    }
    else if(row_address >= (UID_ADDRESS >> 1)){
        // Ignore for now..
    }
    else { // normal row programming sequence
        ICSP_rowWrite( row, ROW_SIZE);
    }
}

void LVP_commitRow( void) {
    // latch and program a row, skip if blank
    uint8_t i;
    uint24_t chk = 0xffffff;
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

uint16_t readWord(uint24_t addr) {
    ICSP_addressLoad( addr);
    return ICSP_read();
}

void catHexWord(char *buffer, uint24_t addr) {
    buffer += strlen(buffer);   // append to buffer
    utoa(buffer, readWord(addr), 16);
    buffer[strlen(buffer)] = ' ';
}

void catDecWord(char *buffer, uint24_t addr) {
    buffer += strlen(buffer);   // append to buffer
    utoa(buffer, readWord(addr), 10);
    buffer[strlen(buffer)] = ' ';
}

uint16_t LVP_getInfoSize(void) {
    // a multiple of 64-char segments
    return 4 * 64;
}

void read_DevID(char *buffer) {
    // read the DevID and RevID
    strcat(buffer, "\nDevice ID: ");
    catHexWord(buffer, DEV_ID);
    strcat(buffer, "\n\nRev ID   : ");
    catHexWord(buffer, REV_ID);
    strcat(buffer, "\n\nFlash    : ");
    utoa(&buffer[strlen(buffer)], (readWord(DEV_URSIZ)*ROW_SIZE)/1024,10);
    strcat(buffer, " KB\n");
}

void read_Config(char *buffer) {
    // read the CONFIG
    strcat(buffer, "\nData EE  : ");
    catDecWord(buffer, DEV_EESIZ);
    strcat(buffer, "B\n\nConfiguration:\n");
    catHexWord(buffer, CFG_ADDRESS+0);
    catHexWord(buffer, CFG_ADDRESS+2);
    catHexWord(buffer, CFG_ADDRESS+4);
    catHexWord(buffer, CFG_ADDRESS+6);
    catHexWord(buffer, CFG_ADDRESS+8);
}

void read_MUI(char *buffer) {
    // read the Microchip Unique Identifier
    strcat(buffer, "\n\nMUI:\n");
    catHexWord(buffer, DIA_ADDRESS+0);
    catHexWord(buffer, DIA_ADDRESS+2);
    catHexWord(buffer, DIA_ADDRESS+4);
    catHexWord(buffer, DIA_ADDRESS+6);
    catHexWord(buffer, DIA_ADDRESS+8);
    catHexWord(buffer, DIA_ADDRESS+10);
}

void read_Calibration(char *buffer) {
    // read the DIA-calibration data
    strcat(buffer, "\n\nCalibration:\n");
    catHexWord(buffer, CAL_ADDRESS+2);    // TSLR2
    catHexWord(buffer, CAL_ADDRESS+8);    // TSHR2

    catHexWord(buffer, CAL_ADDRESS+12);   // FVRA 1X
    catHexWord(buffer, CAL_ADDRESS+14);   // FVRA 2X
    catHexWord(buffer, CAL_ADDRESS+16);   // FVRA 4X

    catHexWord(buffer, CAL_ADDRESS+18);   // FVRC 1X
    catHexWord(buffer, CAL_ADDRESS+20);   // FVRC 2X
    catHexWord(buffer, CAL_ADDRESS+22);   // FVRC 4X
}

void LVP_getInfo(char* buffer, uint16_t seg) {
    // read device information, returns a fixed (64 byte) string at a time
    LVP_enter();

    switch( seg) {
        case 0: read_DevID(buffer); break;
        case 1: read_Config(buffer); break;
        case 2: read_MUI(buffer); break;
        case 3: read_Calibration(buffer); break;
        default:    break;
    }

    // padding with spaces (no \0 string terminator!)
    for(uint8_t i=strlen(buffer); i<64; i++)
        buffer[i] = ' ';

    LVP_exit();
}
