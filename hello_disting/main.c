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

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "global.h"


// Configuration Bit settings
// SYSCLK = 40 MHz (8MHz internal OSC / FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz (SYSCLK / FPBDIV)
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF
// Other options are don't care
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_2, FWDTEN = OFF
#pragma config POSCMOD = OFF, FNOSC = FRCPLL, FPBDIV = DIV_1, FSOSCEN = OFF

#pragma config JTAGEN = OFF

// 'time' counter. updates once per audio frame.
volatile unsigned int time = 0;
// the left and right inputs from the codec
volatile int inL = 0, inR = 0;
// data to be sent to the codec
int outL = 0, outR = 0;

// the 'selector' pot value (the top one)
int selector = 0x20;
// the Z pot value
int pot = 0;

// dummy variable for delayLoop()
int looper = 0;

/*
 * Wait a while.
 */
void delayLoop( int count )
{
    // once code optimisation is enabled,
    // put this line in to retain similar timing
//    count *= 5;
    //
    
    int i;
    for ( i=0; i<count; ++i )
        looper++;
}

/*
 * Read the ADC channels (the selector pot and the Z pot).
 * Returns '1' if the algorithm processing loop should end.
 */
int readADC()
{
    int ret = 0;

    // Determine which buffer is idle and create an offset
    unsigned int offset = 8 * ((~ReadActiveBufferADC10() & 0x01));

    // Read the result of conversion from the idle buffer
    int s = ReadADC10(offset);
    if ( ( s | selector ) & 0x20 )
    {
        int lastSelector = selector;
        selector = s >> 6;
        selector = selector < 15 ? selector : 15;
        if ( selector != lastSelector )
        {
            // new algorithm chosen - exit
            ret = 1;
        }
    }

    // Read the result of pot conversion from the idle buffer
    pot = ReadADC10(offset + 1);

    // button pressed?
    if ( !PORTAbits.RA4 )
    {
        // do nothing in this project,
        // but this is where some reaction to the button could go
        //
        ret = 1;
    }

    mAD1ClearIntFlag();

    return ret;
}

/*
 * The archetypal algorithm loop.
 */
void doAlgorithm0()
{
    // setup
    DECLARATIONS();

    while ( 1 )
    {
        // wait for new audio frame
        IDLE();

        // read the inputs
        int vL = inL;
        int vR = inR;

        // do the processing
        int vOutL = vL + vR;
        int vOutR = vL - vR;

        // write the outputs
        outL = vOutL;
        outR = vOutR;

        // loop end processing
        // (including reading the ADC channels)
        LOOP_END();
    }
}

/*
 * Flash the LEDs at startup.
 */
void startupSequence()
{
    int sequence[8][2] = {
    { BIT_3, 0 },
    { 0, BIT_15 },
    { 0, BIT_11 },
    { 0, BIT_7 },
    { 0, BIT_4 },
    { 0, BIT_12 },
    { 0, BIT_10 },
    { 0, BIT_6 },
    };
    PORTA = 0;
    PORTB = 0;
    int apins = BIT_2 | BIT_3;
    int bpins = BIT_4 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_12 | BIT_15;
    int j;
    for ( j=0; j<8; ++j )
    {
        int i;
        PORTASET = sequence[j][0];
        PORTBSET = sequence[j][1];
        delayLoop( 500000 );
        PORTACLR = sequence[j][0];
        PORTBCLR = sequence[j][1];
    }
    PORTASET = apins;
    PORTBSET = bpins;
    delayLoop( 1000000 );

    PORTACLR = apins;
    PORTBCLR = bpins;
}

/*
 * Main program entry point.
 */
int main(int argc, char** argv)
{
    UINT spi_con1 = 0, spi_con2 = 0;

    // basic system config
    SYSTEMConfig( SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE );
    INTEnableSystemMultiVectoredInt();

    // button on the PCB
    PORTSetPinsDigitalIn(IOPORT_A, BIT_4);

    // LEDs
    PORTSetPinsDigitalOut(IOPORT_A, BIT_2 | BIT_3);
    PORTSetPinsDigitalOut(IOPORT_B, BIT_4 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_12 | BIT_15);

    // flash the LEDs
    startupSequence();

    // setup the codec
    ConfigureCodec();

    // I2C ports are now digital inputs
    PORTSetPinsDigitalIn(IOPORT_B, BIT_8);
    PORTSetPinsDigitalIn(IOPORT_B, BIT_9);

    // more LED flashing now that we're all configured
    int j;
    for ( j=0; j<4; ++j )
    {
        int i;
        PORTAINV = BIT_2 | BIT_3;
        delayLoop( 500000 );
    }

    // configure the ADC
    //
    // Tpb = 25ns
    // sample rate = 1/( div * ( 12 + sample_time ) * Tpb )
    // div = 2 * ( ADCS + 1 )
    // Tad = Tpb * div  must be > 83.33ns
    // 2 * ( 24+1 ) * ( 12 + 28 ) * 25ns = 1/20kHz
    // sampling two channels so halve it

    CloseADC10();   // Ensure the ADC is off before setting the configuration
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN5 |  ADC_CH0_NEG_SAMPLEB_NVREF | ADC_CH0_POS_SAMPLEB_AN11 );
    OpenADC10( ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON,
               ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_ON | ADC_ALT_INPUT_ON,
               ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_28 | 24,
               ENABLE_AN5_ANA | ENABLE_AN11_ANA,
               SKIP_SCAN_ALL );
    EnableADC10(); // Enable the ADC

    // SPI pins
    PORTSetPinsDigitalOut(IOPORT_A, BIT_1);
    PORTSetPinsDigitalOut(IOPORT_B, BIT_7);
    PORTSetPinsDigitalIn(IOPORT_B, BIT_5);
    PORTSetPinsDigitalOut(IOPORT_B, BIT_2);

    PORTSetPinsDigitalOut(IOPORT_B, BIT_15);

    /*------------------------------------------------
    Function	PPS	Pin#	Test Point#	: Comment
    __________________________________________________________________________
    REFOCLK(o)	RPB2	( 6)	TP36		: Reference Clock Output
    SCK1(o)	-	(25)	TP14            : SPI-I2S SCLK Output
    SS(o)	RPA0	( 2)	TP33            : SPI-I2S LRCK Output
    SDO(o)	RPA1	( 3)	TP20            : SPI-I2S SDO Output
    SDI(i)	RPB5	(14)	-               : SPI-I2S SDI Input */
    PPSOutput(3, RPB2, REFCLKO); //REFCLK0: RPB2 - Out
    PPSInput(2, SDI1, RPB5); //SDI: RPB5 - In
    PPSOutput(2, RPA1, SDO1); //SDO: RPA1 - Out
    PPSOutput(1, RPA0, SS1); //SS: RPB4 - Out

    // Configure Reference Clock Output to 12.288MHz.
    mOSCREFOTRIMSet(REFTRIM);
    OSCREFConfig(OSC_REFOCON_SYSCLK, //SYSCLK clock output used as REFCLKO source
            OSC_REFOCON_OE | OSC_REFOCON_ON, //Enable and turn on the REFCLKO
            RODIV);

    // Configure SPI in I2S mode with 24-bit stereo audio.
    spi_con1 = SPI_OPEN_MSTEN | //Master mode enable
            SPI_OPEN_SSEN | //Enable slave select function
            SPI_OPEN_CKP_HIGH | //Clock polarity Idle High Actie Low
            SPI_OPEN_MODE16 | //Data mode: 24b
            SPI_OPEN_MODE32 | //Data mode: 24b
            SPI_OPEN_MCLKSEL | //Clock selected is reference clock
            SPI_OPEN_FSP_HIGH; //Frame Sync Pulse is active high

    spi_con2 = SPI_OPEN2_IGNROV |
            SPI_OPEN2_IGNTUR |
            SPI_OPEN2_AUDEN | //Enable Audio mode
            SPI_OPEN2_AUDMOD_I2S; //Enable I2S mode

    // Configure and turn on the SPI1 module.
    SpiChnOpenEx(SPI_CHANNEL1, spi_con1, spi_con2, SPI_SRC_DIV);

    //Enable SPI1 interrupt.
    IPC7bits.SPI1IP = 3;
    IPC7bits.SPI1IS = 1;
    IEC1bits.SPI1TXIE = 1;

    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI
    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI

    // main loop (never quits)
    for ( ;; )
    {
        // run the processing loop for the chosen algorithm
        switch ( selector )
        {
            case 0:
            default:
                doAlgorithm0();
                break;
        }
    }

    return (EXIT_SUCCESS);
}

/*
 * SPI1 ISR
 * 
 * Handle SPI interrupts.
 * Read new data from the codec;
 * write new data to the codec.
 */
void __ISR(_SPI_1_VECTOR, ipl3) SPI1InterruptHandler(void)
{
    if ( IFS1bits.SPI1TXIF )
    {
        static int toggleData = 0;
        time += toggleData;

        SPI1BUF = toggleData ? outL : outR;
        toggleData = 1 - toggleData;
        IFS1bits.SPI1TXIF = 0;
    }
    if ( IFS1bits.SPI1RXIF )
    {
        static BOOL toggleData = TRUE;

        int raw = SPI1BUF;
        raw = ( raw << 8 ) >> 8;
        if ( toggleData )
            inL = raw;
        else
            inR = raw;
        toggleData = !toggleData;
        IFS1bits.SPI1RXIF = 0;
    }
}
