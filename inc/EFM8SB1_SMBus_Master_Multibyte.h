/*
 * SB1_SMBus_Master_Multibyte.h
 *
 *  Created on: Jun 2, 2014
 *      Author: jiguo1
 */

#ifndef EFM8SB1_SMBUS_MASTER_MULTIBYTE_MAIN_H_
#define EFM8SB1_SMBUS_MASTER_MULTIBYTE_MAIN_H_

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

#define  SYSCLK         12250000       // System clock frequency in Hz

#define  SMB_FREQUENCY  15950          // Target SCL clock rate
                                       // This example supports between 10kHz
                                       // and 100kHz

#define  WRITE          0x00           // SMBus WRITE command
#define  READ           0x01           // SMBus READ command


// Device addresses (7 bits, lsb is a don't care)
#define  SLAVE_ADDR     0xAA           // Device address for slave target

// Status vector - top 4 bits only
#define  SMB_MTSTA      0xE0           // (MT) start transmitted
#define  SMB_MTDB       0xC0           // (MT) data byte transmitted
#define  SMB_MRDB       0x80           // (MR) data byte received
// End status vector definition

#define  NUM_BYTES_WR   16              // Number of bytes to write
                                       // Master -> Slave
#define  NUM_BYTES_RD   16              // Number of bytes to read
                                       // Master <- Slave

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

// Global holder for SMBus data
// All receive data is written here
extern uint8_t SMB_DATA_IN[NUM_BYTES_RD];

// Global holder for SMBus data.
// All transmit data is read from here
extern uint8_t SMB_DATA_OUT[NUM_BYTES_WR];


extern uint8_t TARGET;                             // Target SMBus slave address

extern volatile bool SMB_BUSY;                          // Software flag to indicate when the
                                       // SMB_Read() or SMB_Write() functions
                                       // have claimed the SMBus

extern volatile bool SMB_RW;                            // Software flag to indicate the
                                       // direction of the current transfer

extern uint16_t NUM_ERRORS;                        // Counter for the number of errors.
extern uint8_t i;

//SI_SBIT (LED0, SFR_P1, 0);          // LED0==LED_ON means ON

#endif /* EFM8SB1_SMBUS_MASTER_MULTIBYTE_MAIN_H_ */
