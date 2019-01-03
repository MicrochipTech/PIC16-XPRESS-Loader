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
#define CFG_ADDRESS 0x015780    // address of config words area
#define DEV_ID      0xFF0000    // product ID
#define REV_ID      0xFF0002    // silicon revision ID
#define UID_ADDRESS 0x800F00    // address of UID words area

#define ROW_SIZE     64         // width of a flash row in words
#define CFG_NUM      12         // number of config words

#define WRITE_TIME   1       // mem write time ms
#define CFG_TIME     1       // cfg write time ms
#define BULK_TIME   30       // bulk erase time ms

/****************************************************************************/
// internal state
uint16_t row[ROW_SIZE];         // buffer containing row being formed
uint32_t row_address;           // destination address of current row

// ICSP serial commands
#define SIX                 0
#define REGOUT              1

// ICSP commands (via the Programming Executive)
#define CMD_SCHECK          0x0000
#define CMD_READ_DATA       0x2000
#define CMD_PROG_2W         0x3000
#define CMD_PROG_PAGE       0x50C3
#define CMD_BULK_ERASE      0x7001      // BULK ERASE + length (1)

void ICSP_slaveReset(void) {
    ICSP_nMCLR = SLAVE_RESET;
    ICSP_TRIS_nMCLR = OUTPUT_PIN;
}

void ICSP_slaveRun(void) {
    ICSP_TRIS_nMCLR = INPUT_PIN;
    ICSP_nMCLR = SLAVE_RESET;
}

void ICSP_init(void ) {
    RCSTAbits.SPEN = 0;         // disable UART, control I/O
    __delay_us(1);
    ICSP_TRIS_DAT = INPUT_PIN;
    ICSP_CLK = 0;
    ICSP_TRIS_CLK = OUTPUT_PIN;
}

void ICSP_release(void) {
    ICSP_TRIS_DAT  = INPUT_PIN;
    ICSP_TRIS_CLK  = INPUT_PIN;
    UART_init();
    ICSP_slaveRun();
}

void sendControlCode(uint8_t type) {
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for(uint8_t i = 0; i < 4; i++)
    {
        if (type & 0x01)
            ICSP_DAT = 1;
        else
            ICSP_DAT = 0;

        ICSP_CLK = 1;
        type >>= 1;
        __delay_us(1);
        ICSP_CLK = 0;
        __delay_us(1);
    }
    __delay_us(1);
}


void ICSP_sendWord( uint16_t w) {
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for( uint8_t i=0; i<16; i++) {
        if ((w & 0x8000) > 0) // Msb first
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

void sendData24( uint24_t data) {
    uint8_t i;
    ICSP_TRIS_DAT = OUTPUT_PIN;
    for(i = 0; i < 24; i++){
        if (data & 0x000001)
            ICSP_DAT = 1;     // Lsb first
        else
            ICSP_DAT = 0;     // Lsb first

        ICSP_CLK = 1;
        data >>= 1;
        __delay_us(1);
        ICSP_CLK = 0;
        __delay_us(1);
    }
    __delay_us(1);
}

sendSIXCmd(uint24_t data) {
    sendControlCode(SIX);
    sendData24(data);
}

void ICSP_signature(void) {
    ICSP_slaveReset();          // MCLR output => Vil (GND)
    __delay_us(1);              // > P6 (100ns))
    ICSP_slaveRun();
    __delay_us(200);            // < P21 (500us) short pulse
    ICSP_slaveReset();
    __delay_ms(2);              // > P18 (1ms)
    ICSP_sendWord(0x4D43);
    ICSP_sendWord(0x4851);
    __delay_us(1);              // > P19 (20ns)
    ICSP_slaveRun();            // release MCLR
    __delay_ms(55);             // > P7(50ms) + P1 (500us)*5
}

void ICSP_exitResetVector(void) {
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
    sendSIXCmd(0x040200);       // goto 200
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
}

void ICSP_unlockWR(void) {
    sendSIXCmd(0x200551);       // mov  #55, W1
    sendSIXCmd(0x883971);       // mov  W1, NVKEY
    sendSIXCmd(0x200AA1);       // mov  #AA, W1
    sendSIXCmd(0x883971);       // mov  W1, NVKEY
    sendSIXCmd(0xA8E729);       // bset NVCOM, #WR
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop]
}

uint16_t ICSP_getWord( void)
{
    uint16_t w = 0;
    ICSP_TRIS_DAT = INPUT_PIN;  // PGD input
    for( uint8_t i=0; i<16; i++) {
        ICSP_CLK = 1;
        w >>= 1;                // shift right
        __delay_us(1);
        w |= (ICSP_DAT_IN ? 0x8000 : 0);       // read port
        ICSP_CLK = 0;
        __delay_us(1);
    }
    return w;
}

uint16_t readVISI(void)
{
    sendControlCode(REGOUT);
    for(uint8_t i=0; i<8; i++){
        ICSP_CLK = 1;
        __delay_us(1);
        ICSP_CLK = 0;
        __delay_us(1);
    }
    return ICSP_getWord();
}

void ICSP_bulkErase(void) {
    ICSP_exitResetVector();
    sendSIXCmd(0x2400EA);       // mov  0x400E, W10
    sendSIXCmd(0x88394A);       // mov  W10, NVMCON
    sendSIXCmd(0);              // nop
    sendSIXCmd(0);              // nop
    ICSP_unlockWR();
    __delay_ms( BULK_TIME);
}

void ICSP_addressLoad(uint24_t address) {
    uint24_t destAddressHigh, destAddressLow;

    destAddressHigh = (address >> 16) & 0xff;
    destAddressLow  = address & 0xffff;

    // Step 1: Exit the Reset vector.
    ICSP_exitResetVector();

    // Step 2: Set the TBLPG register for writing to the latches  (@FA0000)
    sendSIXCmd(0x200FAC);   // MOV #0xFA, W12
    sendSIXCmd(0x883B0A);   // MOV W12, TBLPG

    // Step 3: set NVMADR, NVMADRU to point to the destination
    sendSIXCmd(0x200003 + (destAddressLow) << 4);  // MOV #<DestinationAddress15:0>, W3
    sendSIXCmd(0x200004 + (destAddressHigh)<< 4);  // MOV #<DestinationAddress23:16>, W4
    sendSIXCmd(0x883953);                          // MOV W3, NVMADR
    sendSIXCmd(0x883964);                          // MOV W4, NVMADRU

    // Step 4: Set the NVMCON to program 2 instruction words.
    sendSIXCmd(0x24001A);   // MOV #0x4001, W10
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0x883B0A);   // MOV W10, NVMCON
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0);          // NOP
}

void sendSIXMov( uint16_t word, uint8_t reg) {
    uint24_t lit = word;
    // format mov command as  op
    sendSIXCmd( 0x200000 + (lit << 4) + reg);
}

void ICSP_rowWrite(uint16_t *buffer, uint8_t count) {
    uint16_t LSW0, MSB0;
    uint16_t LSW1, MSB1;
    uint16_t LSW2, MSB2;
    uint16_t LSW3, MSB3;

    // Step 5: init W7 to point to first latch
    sendSIXCmd(0xEB0380);       // CLR W7
    sendSIXCmd(0x000000);       // NOP

    for(uint8_t i=count/4; i>0; i--)     // load 2 latches, 4 * 16-bit words
    {
        LSW0 = *buffer++;       MSB0 = *buffer++;
        LSW1 = *buffer++;       MSB1 = *buffer++;

        sendSIXMov(LSW0, 0);     // MOV #<LSW0>, W0
        sendSIXMov( ((MSB1 & 0xFF) << 8) + (MSB0 & 0xFF), 1); // MOV #<MSB1:MSB0>, W1
        sendSIXMov(LSW1, 2);     // MOV #<LSW1>, W2

        sendSIXCmd(0xEB0300);   // CLR W6
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0xBB0BB6);   // TBLWTL[W6++], [W7]
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0xBBDBB6);   // TBLWTH.B[W6++], [W7++]
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0xBBEBB6);   // TBLWTH.B[W6++], [++W7]
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0xBB1BB6);   // TBLWTL[W6++], [W7++]
        sendSIXCmd(0);          // NOP
        sendSIXCmd(0);          // NOP
    }
    // Step 7. Initiate write cycle
    ICSP_unlockWR();
    __delay_ms( WRITE_TIME);
}

//void ICSP_cfgWrite(uint16_t *buffer, uint8_t count)
//{
//    uint16_t LSW0, MSB0;
//    uint16_t LSW1, MSB1;
//
//    while( count > 0)
//    {
//        LSW0 = *buffer++;       MSB0 = *buffer++;
//        LSW1 = *buffer++;       MSB1 = *buffer++;
//
//        sendSIXMov(LSW0, 0);    // MOV #<LSW0>, W0
//        sendSIXMov( ((MSB1 & 0xFF) << 8) + (MSB0 & 0xFF), 1); // MOV #<MSB1:MSB0>, W1
//        sendSIXMov(LSW1, 2);    // MOV #<LSW1>, W2
//
//        sendSIXCmd(0xEB0300);   // CLR W6
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0xBB0B00);   // TBLWTL W0, [W6]
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0xBB9B01);   // TBLWTH W1, [W6++]
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0xBB0B02);   // TBLWTL W2, [W6]
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0xBB9B03);   // TBLWTH W3, [W6++]
//        sendSIXCmd(0);          // NOP
//        sendSIXCmd(0);          // NOP
//    }
//
//    // Step 7: Initiate the write cycle.
//     ICSP_unlockWR();
//    __delay_ms( CFG_TIME);
//
//    // Step 8: Wait for the Configuration Register Write operation to complete and make sure WR bit is clear.
//
//    //    sendSIXCmd(0x803B00);  // MOV NVMCON, W0
//    //    sendSIXCmd(0x883C20);  // MOV W0, VISI
//    //    sendSIXCmd(0x000000);  // NOP
//    //
//    //    //TODO: Implement the regout command for instruction below
//    //    //<VISI>  // Clock out contents of VISI register.
//    //
//    //    sendSIXCmd(0x040200);  // GOTO 0x200
//    //    sendSIXCmd(0x000000);  // NOP
//    //
//    //    //Repeat until the WR bit is clear.
//}

/****************************************************************************/

bool lvp = false;

void LVP_enter(void)
{
    LED_on(RED_LED);
    LED_off(GREEN_LED);

    ICSP_init();                // configure I/Os
    ICSP_signature();           // enter LVP mode

    // fill row buffer with blanks
    memset((void*)row, 0xff, sizeof(row));
    row_address = -1;
    lvp = true;
}

void LVP_exit(void)
{
    ICSP_release();             // release ICSP-DAT and ICSP-CLK
    lvp = false;
    LED_off(RED_LED);
    LED_on(GREEN_LED);
}

bool LVP_inProgress(void)
{
    return (lvp);
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
    else if (row_address >= (CFG_ADDRESS)) {    // use the special cfg word sequence
        ICSP_addressLoad(row_address << 1);
//        ICSP_cfgWrite( row, CFG_NUM);
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
    uint8_t  index = (address & 0xfe) >> 1;
    uint32_t new_row = (address & 0xfffffff00) >> 1;
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

void fiveNOP(void) {
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0);          // NOP
    sendSIXCmd(0);          // NOP
}

uint16_t readWord(uint24_t addr) {
    ICSP_exitResetVector();
    sendSIXMov(addr>>16, 0);        // mov (addru), W0
    sendSIXCmd(0x20F887);           // mov #VISI, W7
    sendSIXCmd(0x8802A0);           // mov W0, TBLPAG
    sendSIXMov(addr & 0xffff, 6);   // mov (addrl), W6
    sendSIXCmd(0);                  // nop
    sendSIXCmd(0xBA8B96);           // TBLRDH [w6],[w7]]
    fiveNOP();
//    readVISI();
//    sendSIXCmd(0xBA0B96);           // TBLRDL [w6],[w7]]
//    fiveNOP();
    return readVISI();
}

void catHexWord(char *buffer, uint24_t addr) {
    buffer += strlen(buffer);   // append to buffer
    utoa(buffer, readWord(addr), 16);
    buffer[strlen(buffer)] = ' ';
}

//void catDecWord(char *buffer, uint24_t addr) {
//    buffer += strlen(buffer);   // append to buffer
//    utoa(buffer, readWord(addr), 10);
//    buffer[strlen(buffer)] = ' ';
//}

uint16_t LVP_getInfoSize(void) {
    // a multiple of 64-char segments
    return 1 * 64;
}

void read_DevID(char *buffer) {
//     read the DevID and RevID
    strcat(buffer,  "\nDev ID: ");
    catHexWord(buffer, DEV_ID);
    strcat(buffer, "\n\nRev ID : ");
    catHexWord(buffer, REV_ID);
    strcat(buffer, "\n\nFlash : 128KB");
}

//void read_Config(char *buffer) {
//    // read the CONFIG
//    strcat(buffer, "\nData EE  : ");
//    catDecWord(buffer, DEV_EESIZ);
//    strcat(buffer, "B\n\nConfiguration:\n");
//    catHexWord(buffer, CFG_ADDRESS+0);
//    catHexWord(buffer, CFG_ADDRESS+2);
//    catHexWord(buffer, CFG_ADDRESS+4);
//    catHexWord(buffer, CFG_ADDRESS+6);
//    catHexWord(buffer, CFG_ADDRESS+8);
//}
//
//void read_MUI(char *buffer) {
//    // read the Microchip Unique Identifier
//    strcat(buffer, "\n\nMUI:\n");
//    catHexWord(buffer, UID_ADDRESS+0);
//    catHexWord(buffer, UID_ADDRESS+2);
//    catHexWord(buffer, UID_ADDRESS+4);
//    catHexWord(buffer, UID_ADDRESS+6);
//    catHexWord(buffer, UID_ADDRESS+8);
//    catHexWord(buffer, UID_ADDRESS+10);
//}
//
