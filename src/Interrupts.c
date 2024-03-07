//=========================================================
// src/Interrupts.c: generated by Hardware Configurator
//
// This file will be regenerated when saving a document.
// leave the sections inside the "$[...]" comment tags alone
// or they will be overwritten!
//=========================================================

// USER INCLUDES
#include <SI_EFM8SB1_Register_Enums.h>
#include "EFM8SB1_SMBus_Master_Multibyte.h"

SI_SBIT (P05, SFR_P0, 5);// Pin 0.5 for SHDN (20V stage) enable/disable

// NT3H I2C parameters
#define MEMA_read  0x01     // Memory address to read
#define MEMA_write  0x02    // Memory address to write
bool SA_sent = 0;
bool SA_read_sent = 0;
bool MEMA_sent = 0;
bool Read_Init = 0;

extern uint16_t pulseCounter;
extern uint16_t T_on;
extern uint16_t T_on_double;
extern bool On;
extern volatile uint8_t set_stim_off = 0;
extern volatile bool channel_set;

extern void
T0_Waitus (uit16_t);
extern void
Polarity (uint8_t);
extern void
Pulse_On (void);
extern void
Pulse_Off (void);

//-----------------------------------------------------------------------------
// TIMER2_ISR
//-----------------------------------------------------------------------------
//
// TIMER2 ISR Content goes here. Remember to clear flag bits:
// TMR2CN0::TF2H (Timer # High Byte Overflow Flag)
// TMR2CN0::TF2L (Timer # Low Byte Overflow Flag)
//
//-----------------------------------------------------------------------------
SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
  {
    pulseCounter++;
    if (pulseCounter <= T_on)
      {
        Polarity(0); // start shunted
        Polarity(1);// forward polarity
        Pulse_On();// pulse on
        T0_Waitus(1);
        Pulse_Off();// pulse off
        Polarity(0);// shunt
        Polarity(2);// reverse
        Pulse_On();// pulse on
        T0_Waitus(1);
        Pulse_Off();// pulse off
        Polarity(0);// shunted
        // next interrupt is the next pulse
      }
    else if ((pulseCounter <= T_on_double) && (!set_stim_off))
      {
        Pulse_Off();
        Polarity(0);// shunted
        set_stim_off = 1;
        P05 = 0;
      }
    else if (pulseCounter > T_on_double)
      {
        pulseCounter = 0;
        set_stim_off = 0;
        P05 = On;
        channel_set = 0;
      }
    TMR2CN0_TF2H = 0; // clear overflow flag
  }

//-----------------------------------------------------------------------------
// SMBUS0_ISR
//-----------------------------------------------------------------------------
//
// SMBUS0 ISR Content goes here. Remember to clear flag bits:
// SMB0CN0::SI (SMBus Interrupt Flag)
//
//-----------------------------------------------------------------------------
SI_INTERRUPT (SMBUS0_ISR, SMBUS0_IRQn)
  {
    bool FAIL = 0;                       // Used by the ISR to flag failed
    // transfers

    static uint8_t sent_byte_counter;
    static uint8_t rec_byte_counter;

    if (SMB0CN0_ARBLOST == 0)// Check for errors
      {
        // Normal operation
        switch (SMB0CN0 & 0xF0)// Status vector
          {
            // Master Transmitter/Receiver: START condition transmitted.
            case SMB_MTSTA:
            SMB0DAT = TARGET;// Load address of the target slave
            SMB0DAT &= 0xFE;// Clear the LSB of the address for the
            // R/W bit
            if (SMB_RW == READ && Read_Init)
              { // Prepare the read mode
                SMB0DAT |= (uint8_t) 1;// set 1 as per NT3H manual for DATA IN receive
                SMB0CN0_STA = 0;// Manually clear START bit
                SA_read_sent = 1;
              }
            else
              {
                SMB0DAT |= (uint8_t) 0; // set 0 as per NT3H manual
                SMB0CN0_STA = 0;// Manually clear START bit
              }
            rec_byte_counter = 1; // Reset the counter
            sent_byte_counter = 1;// Reset the counter
            SA_sent = 1;
            break;

            // Master Transmitter: Data byte transmitted
            case SMB_MTDB:
//            if (SMB0CN0_ACK && SA_sent && MEMA_sent)// Slave SMB0CN0_ACK?
            if (SMB0CN0_ACK && SA_sent && MEMA_sent == 0)
              {
                if (SMB_RW == WRITE)
                  {
                    SMB0DAT = MEMA_write;
                    MEMA_sent = 1;
                  }
                else
                  {
                    SMB0DAT = MEMA_read;
                    MEMA_sent = 1;
                    Read_Init = 0;
                  }
                break;
              }
            else if (SMB0CN0_ACK && SMB_RW == WRITE && MEMA_sent) // If this transfer is a WRITE, and memory address has been sent
              {
                if (sent_byte_counter <= NUM_BYTES_WR)
                  {
                    // send data byte
                    SMB0DAT = SMB_DATA_OUT[sent_byte_counter-1];
                    sent_byte_counter++;
                  }
                else // End of writing
                  {
                    SMB0CN0_STO = 1; // Set SMB0CN0_STO to terminate transfer
                    SMB_BUSY = 0;// And free SMBus interface
                    MEMA_sent = 0;
                    SA_sent = 0;
                  }
              }
            else if (SMB0CN0_ACK && SMB_RW == READ && MEMA_sent && Read_Init == 0) // If transfer is read, initiate STOP, then START, then first 7 bits of SA
            // then proceed to receive mode
              {
                SMB0CN0_STO = 1;
                SMB0CN0_ACK = 1;
                Read_Init = 1;
                SMB0CN0_STA = 1;
              }
            else if (SMB0CN0_ACK == 0)                     // If slave NACK,
              {
                SMB0CN0_STO = 1;                // Send STOP condition, followed
                SMB0CN0_STA = 1;// By a START
                NUM_ERRORS++;// Indicate error. Useful for debugging if SMBus is stuck.
              }
            break;

            // Master Receiver: byte received
            case SMB_MRDB:
            if (rec_byte_counter < NUM_BYTES_RD)
              {
                SMB_DATA_IN[rec_byte_counter-1] = SMB0DAT; // Store received
                // byte
                SMB0CN0_ACK = 1;// Send SMB0CN0_ACK to indicate byte received
                rec_byte_counter++;// Increment the byte counter
              }
            else // End of reading
              {
                SMB_DATA_IN[rec_byte_counter-1] = SMB0DAT; // Store received
                // byte
                SMB_BUSY = 0;// Free SMBus interface
                //SMB0CN0_ACK = 0;// Send NACK to indicate last byte
                // of this transfer
                SMB0CN0_ACK = 1;// Send SMB0CN0_ACK to indicate byte received

                SMB0CN0_STO = 1;// Send STOP to terminate transfer
                MEMA_sent = 0;
                SA_sent = 0;
                Read_Init = 0;
                SA_read_sent = 0;
              }
            break;

            default:
            FAIL = 1;                  // Indicate failed transfer
            // and handle at end of ISR
            break;

          } // end switch
      }
    else
      {
        // SMB0CN0_ARBLOST = 1, error occurred... abort transmission
        FAIL = 1;
      } // end SMB0CN0_ARBLOST if

    if (FAIL)// If the transfer failed,
      {
        SMB0CF &= ~0x80;                 // Reset communication
        SMB0CF |= 0x80;
        SMB0CN0_STA = 0;
        SMB0CN0_STO = 0;
        SMB0CN0_ACK = 0;

        SMB_BUSY = 0;// Free SMBus

        FAIL = 0;

        NUM_ERRORS++;// Indicate an error occurred
      }
    SMB0CN0_SI = 0;                             // Clear interrupt flag
  }

//-----------------------------------------------------------------------------
// TIMER0_ISR
//-----------------------------------------------------------------------------
//
// TIMER0 ISR Content goes here. Remember to clear flag bits:
// TCON::TF0 (Timer 0 Overflow Flag)
//
//-----------------------------------------------------------------------------
//SI_INTERRUPT (TIMER0_ISR, TIMER0_IRQn)
//  {
//
//  }

