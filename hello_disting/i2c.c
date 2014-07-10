/*
The MIT License (MIT)

Copyright (c) 2014 Expert Sleepers Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include <plib.h>

#include "global.h"

#define GetSystemClock()           (SYS_FREQ)
#define GetPeripheralClock()       (SYS_FREQ/1)
#define GetInstructionClock()      (SYS_FREQ)
#define I2C_CLOCK_FREQ             10000

// EEPROM Constants
#define EEPROM_I2C_BUS              I2C1

/*******************************************************************************
  Function:
    BOOL StartTransfer( BOOL restart )

  Summary:
    Starts (or restarts) a transfer to/from the EEPROM.

  Description:
    This routine starts (or restarts) a transfer to/from the EEPROM, waiting (in
    a blocking loop) until the start (or re-start) condition has completed.

  Precondition:
    The I2C module must have been initialized.

  Parameters:
    restart - If FALSE, send a "Start" condition
            - If TRUE, send a "Restart" condition

  Returns:
    TRUE    - If successful
    FALSE   - If a collision occured during Start signaling

  Example:
    <code>
    StartTransfer(FALSE);
    </code>

  Remarks:
    This is a blocking routine that waits for the bus to be idle and the Start
    (or Restart) signal to complete.
  *****************************************************************************/

BOOL StartTransfer( BOOL restart )
{
    I2C_STATUS  status;

    // Send the Start (or Restart) signal
    if(restart)
    {
        I2CRepeatStart(EEPROM_I2C_BUS);
    }
    else
    {
        // Wait for the bus to be idle, then start the transfer
        while( !I2CBusIsIdle(EEPROM_I2C_BUS) );

        if(I2CStart(EEPROM_I2C_BUS) != I2C_SUCCESS)
        {
            DBPRINTF("Error: Bus collision during transfer Start\n");
            return FALSE;
        }
    }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(EEPROM_I2C_BUS);

    } while ( !(status & I2C_START) );

    return TRUE;
}


/*******************************************************************************
  Function:
    BOOL TransmitOneByte( UINT8 data )

  Summary:
    This transmits one byte to the EEPROM.

  Description:
    This transmits one byte to the EEPROM, and reports errors for any bus
    collisions.

  Precondition:
    The transfer must have been previously started.

  Parameters:
    data    - Data byte to transmit

  Returns:
    TRUE    - Data was sent successfully
    FALSE   - A bus collision occured

  Example:
    <code>
    TransmitOneByte(0xAA);
    </code>

  Remarks:
    This is a blocking routine that waits for the transmission to complete.
  *****************************************************************************/

BOOL TransmitOneByte( UINT8 data )
{
    // Wait for the transmitter to be ready
    while(!I2CTransmitterIsReady(EEPROM_I2C_BUS));

    // Transmit the byte
    if(I2CSendByte(EEPROM_I2C_BUS, data) == I2C_MASTER_BUS_COLLISION)
    {
        DBPRINTF("Error: I2C Master Bus Collision\n");
        return FALSE;
    }

    // Wait for the transmission to finish
    while(!I2CTransmissionHasCompleted(EEPROM_I2C_BUS));

    return TRUE;
}


/*******************************************************************************
  Function:
    void StopTransfer( void )

  Summary:
    Stops a transfer to/from the EEPROM.

  Description:
    This routine Stops a transfer to/from the EEPROM, waiting (in a
    blocking loop) until the Stop condition has completed.

  Precondition:
    The I2C module must have been initialized & a transfer started.

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    StopTransfer();
    </code>

  Remarks:
    This is a blocking routine that waits for the Stop signal to complete.
  *****************************************************************************/

void StopTransfer( void )
{
    I2C_STATUS  status;

    // Send the Stop signal
    I2CStop(EEPROM_I2C_BUS);

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(EEPROM_I2C_BUS);

    } while ( !(status & I2C_STOP) );
}


void ErrorHalt()
{
    int apins = BIT_2 | BIT_3;
    int bpins = BIT_4 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_12 | BIT_15;
    int j;
    for ( ;; )
    {
        int i;
        PORTAINV = apins;
        PORTBINV = bpins;
        delayLoop( 100000 );
    }
}

void SendPacket( UINT8* i2cData, int DataSz )
{
    int                 Index;
    UINT32              actualClock;
    BOOL                Acknowledged;
    BOOL                Success = TRUE;
    UINT8               i2cbyte;

    // Start the transfer to write data to the EEPROM
    if( !StartTransfer(FALSE) )
    {
        while(1);
    }

    // Transmit all data
    Index = 0;
    while( Success && (Index < DataSz) )
    {
        // Transmit a byte
        if (TransmitOneByte(i2cData[Index]))
        {
            // Advance to the next byte
            Index++;

            // Verify that the byte was acknowledged
            if(!I2CByteWasAcknowledged(EEPROM_I2C_BUS))
            {
                DBPRINTF("Error: Sent byte was not acknowledged\n");
                Success = FALSE;
            }
        }
        else
        {
            Success = FALSE;
        }
    }

    // End the transfer (hang here if an error occured)
    StopTransfer();
    if(!Success)
    {
        ErrorHalt();
    }

    // Wait for EEPROM to complete write process, by polling the ack status.
    Acknowledged = FALSE;
    do
    {
        // Start the transfer to address the EEPROM
        if( !StartTransfer(FALSE) )
        {
            while(1);
        }

        // Transmit just the EEPROM's address
        if (TransmitOneByte(i2cData[0]))
        {
            // Check to see if the byte was acknowledged
            Acknowledged = I2CByteWasAcknowledged(EEPROM_I2C_BUS);
        }
        else
        {
            Success = FALSE;
        }

        // End the transfer (stop here if an error occured)
        StopTransfer();
        if(!Success)
        {
            ErrorHalt();
        }

    } while (Acknowledged != TRUE);

    // End the transfer (stop here if an error occured)
    StopTransfer();
    if(!Success)
    {
        ErrorHalt();
    }
}

/*
 * Set up the I2C interface,
 * configure the codec,
 * and shut down I2C.
 */
void ConfigureCodec()
{
    I2CConfigure( EEPROM_I2C_BUS, 0 );
    I2CSetFrequency( EEPROM_I2C_BUS, GetPeripheralClock(), I2C_CLOCK_FREQ );
    I2CEnable( EEPROM_I2C_BUS, TRUE );
    //
    UINT8               i2cData[10];
    I2C_7_BIT_ADDRESS   SlaveAddress;

    // Initialize the data buffer
    I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, 0x46, I2C_WRITE);
    i2cData[0] = SlaveAddress.byte;
    i2cData[1] = 0x40;              // register 64
    i2cData[2] = 0xC0;              // turn off power save

    SendPacket( i2cData, 3 );

    i2cData[1] = 73;                // register 73
    i2cData[2] = 0x0C;              // inverted phase, no HPF

    SendPacket( i2cData, 3 );

    //
    I2CEnable( EEPROM_I2C_BUS, FALSE );
}
