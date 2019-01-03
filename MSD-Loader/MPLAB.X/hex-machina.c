/*
 * File: hex-machina.c
 * Author: Lucio Di Jasio
 * Description: A simple state machine to parse an intel hex file as a stream and program directly to the PIC16F1 flash memory.
 */ 
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum hexstate { SOL, BYTE_COUNT, ADDRESS, RECORD_TYPE, DATA, CHKSUM};

#define ROW_SIZE    128     // for pic18FxxQ10
#define INDEX_MASK  (ROW_SIZE-1)<<1
#define ROW_MASK    ~((INDEX_MASK)+1)

uint16_t row[ ROW_SIZE];
uint32_t row_address;
bool lvp;

void initLVP( void){
    memset((void*)row, 0xff, ROW_SIZE*2);    // fill buffer with blanks
    row_address = 0x8000;
    lvp = false;
}

bool isDigit( char * c){
    if (*c < '0') return false;
    *c -= '0'; if (*c > 9) *c-=7;
    if (*c > 0xf)  return false;
    return true;
}

void lvpWrite( void){
    // first check for first entry in lvp 
    if (!lvp) {
        lvp = true;
        puts("LVP entry");
        puts("Bulk erase device!");
    }
    uint8_t i;

    uint16_t temp=0xffff;
    for(i=0; i<ROW_SIZE-1; i++){
        temp &= row[i];
    }
    printf("AND: %04x \n", temp);

    for(i=0; i<ROW_SIZE; i++){
        if ((i & 0xF) == 0) puts("");
        printf("%04x ", row[i]);
    }
}

void writeRow( void) {
    // latch and program a row, skip if blank
    uint8_t i;
    uint16_t chk = 0xffff;
    for( i=0; i< ROW_SIZE; i++) chk &= row[i];  // blank check
    if (chk != 0xffff) { 
        printf("Loading address :%06x \n", row_address<<1);
        lvpWrite();
        puts("");
        memset((void*)row, 0xff, ROW_SIZE*2);    // fill buffer with blanks
    }
}

void ProgramRow( uint32_t address, uint8_t *data, uint8_t data_count) {
    // copy only the bytes from the current data packet up to the boundary of a row 
    uint8_t  index = (address & INDEX_MASK)>>1; 
    uint32_t new_row = (address & ROW_MASK)>>1;
    if (new_row != row_address) {
        writeRow();
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
        writeRow();
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

void ProgramLastRow( void) {
    writeRow();
    puts("ExitLVP");
}

bool ParseHex(char c)
{
    static enum hexstate state = SOL;
    static uint8_t  bc;
    static uint8_t  data_count;
    static uint32_t address;
    static uint32_t ext_address = 0;
    static uint8_t  checksum;
    static uint8_t  record_type;
    static uint8_t  data_byte, data_index, data[16];
    // static int line = 0;
    // putchar(c);
    switch( state){
        case SOL:
            if (c == '\r') break;
            if (c == '\n') break;
            if (c != ':') return false; 
            state = BYTE_COUNT;
            bc = 0;
            address = 0;
            checksum = 0;
            // printf("Line: %d \n", line++);
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
                memset(data, 0xff, 16);
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
                if (checksum != 0) { printf("checksum: %02x\n", checksum); state = SOL; return false; }
                // chksum is good 
                state = SOL; 
                if (record_type == 0) ProgramRow( ext_address + address, data, data_count);
                else if (record_type == 1) ProgramLastRow();
                else if (record_type == 4) ext_address = ((uint32_t)(data[0]) << 24) + ((uint32_t)(data[1]) << 16);
                else return false;
            }
            break;            
        default:
            break;
    }
    return true;
}

int main( int argc, char** argv){
    char c;
    puts(" Parsing HEX input!");
    initLVP();
    while ( ParseHex( getchar()));
    int i = INDEX_MASK;
    printf("index mask: %02x \n", i);
    i = ROW_MASK;
    printf("row mask: %02x \n", i);
}
