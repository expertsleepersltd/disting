/* 
The MIT License (MIT)

Copyright (c) 2016 Expert Sleepers Ltd

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

#define SYS_FREQ 	(48000000L)

// Configuration Bit settings
// SYSCLK = 48 MHz (8MHz Crystal / FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 48 MHz (SYSCLK / FPBDIV)
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// Other options are don't care
#pragma config FPLLMUL = MUL_24, FPLLIDIV = DIV_2, FPLLODIV = DIV_2
#pragma config POSCMOD = OFF, FNOSC = FRCPLL, FPBDIV = DIV_1, FSOSCEN = OFF

#pragma config JTAGEN = OFF

// WDT OFF, non-windowed, 1:1024 postscale
#pragma config FWDTEN = OFF, WINDIS = OFF, WDTPS = PS1024

#pragma config CP = OFF, PWP = OFF, BWP = OFF

const int magic __attribute__((address(0xBD009370))) = 0xbadabeef;

volatile unsigned int time = 0;
volatile unsigned int slowTime = 0;
volatile int inL = 0, inR = 0;
volatile int outL = 0, outR = 0;

int selector = 0;
#define kMaxParameters 4
int numParameters = 0;
int parameters[kMaxParameters] = { 0 };
int parameterMins[kMaxParameters] = { 0 };
int parameterMaxs[kMaxParameters] = { 0 };
int currentParameter = 0;
// measured pot range is 0x5B to 0x3B1, 91 to 945
int pot = 0;
int looper = 0;
int lastEncA = 1;
int showPot = 0;

int readParameters[kMaxParameters] = { 0 };
BYTE useReadParameter = 0;

unsigned int setB15 = 0;    // BIT_15 if we're allowed to use B15 for IO else 0
BYTE sdActive = 0;          // sd card algorithm active

void delayLoop( int count )
{
    count *= 5;
    int i;
    for ( i=0; i<count; ++i )
        looper++;
}

int currentSelectorSlot;

void storeSelector()
{
    STATIC_ASSERT( ( sizeof(int) * PAGE_SIZE ) == BYTE_PAGE_SIZE );

    int* ptr = (int*)CURRENT_ALG_FLASH_ADDR;

    if ( currentSelectorSlot >= ( PAGE_SIZE - 1 ) )
    {
        NVMErasePage( (void*)CURRENT_ALG_FLASH_ADDR );
        currentSelectorSlot = 0;
    }
    else
    {
        NVMWriteWord( &ptr[ currentSelectorSlot ], 0 );
        currentSelectorSlot += 1;
    }

    char c = parameters[0];
    BYTE b0 = 0x3F & *(BYTE*)&c;
    c = parameters[1];
    BYTE b1 = 0x3F & *(BYTE*)&c;
    c = parameters[2];
    BYTE b2 = 0x3F & *(BYTE*)&c;
    c = parameters[3];
    BYTE b3 = 0x3F & *(BYTE*)&c;
    int w = ( selector + 1 );
    if ( w > 254 )
        w = 69 + ( w - 255 );
    w = w | ( b0 << 8 ) | ( b1 << 14 ) | ( b2 << 20 ) | ( b3 << 26 );
    NVMWriteWord( &ptr[ currentSelectorSlot ], w );
}

int extractParam( unsigned int r, int shift )
{
    int c = ( r >> shift ) & 0x3F;
    return ( ( c << 26 ) >> 26 );
}

void readSelector()
{
    unsigned int* ptr = (unsigned int*)CURRENT_ALG_FLASH_ADDR;
    selector = 0;
    currentSelectorSlot = PAGE_SIZE;
    int i;
    for ( i=0; i<PAGE_SIZE; ++i )
    {
        unsigned int r = ptr[i];
        unsigned int v = r & 0xff;
        if ( v >= 1 && v <= 254 )
        {
            if ( v >= 69 && v <= 70 )
                v = 255 + ( v - 69 );
            selector = v - 1;
            readParameters[0] = extractParam( r, 8 );
            readParameters[1] = extractParam( r, 14 );
            readParameters[2] = extractParam( r, 20 );
            readParameters[3] = extractParam( r, 26 );
            useReadParameter = 1;
            currentSelectorSlot = i;
        }
    }
}

int handleEncoderTurn( int encB )
{
    int p = parameters[currentParameter];
    if ( !encB )
    {
        p -= 1;
        if ( p < parameterMins[currentParameter] )
            p = parameterMins[currentParameter];
    }
    else
    {
        p += 1;
        if ( p > parameterMaxs[currentParameter] )
            p = parameterMaxs[currentParameter];
    }
    if ( p != parameters[currentParameter] )
    {
        parameters[currentParameter] = p;
        storeSelector();
        showPot = kTimeToShowPot;
        if ( p >= 0 )
            showValue( p, 0 );
        else
            showValue( -p, 1 );
    }

    return 0;
}

void setParameterRanges( int num, const int* ranges )
{
    showPot = 0;
    currentParameter = 0;

    numParameters = num;

    int i;

    for ( i=0; i<num; ++i )
    {
        int mn = *ranges++;
        int mx = *ranges++;

        parameterMins[i] = mn;
        parameterMaxs[i] = mx;
    }

    if ( useReadParameter )
    {
        for ( i=0; i<num; ++i )
        {
            int p = readParameters[i];
            if ( p >= parameterMins[i] && p <= parameterMaxs[i] )
                parameters[i] = p;
        }
    }
    
    for ( i=0; i<num; ++i )
    {
        if ( parameters[i] < parameterMins[i] )
            parameters[i] = parameterMins[i];
        else if ( parameters[i] > parameterMaxs[i] )
            parameters[i] = parameterMaxs[i];
    }

    // unless it's the first algorithm after power up,
    // store the new selector value now we've validated the parameters
    if ( useReadParameter )
    {
        useReadParameter = 0;
    }
    else
    {
        storeSelector();
    }
}

int handleSelectorTurn( int encB )
{
    int ret = 0;
    int sel = selector & 0xf;
    int lastSel = sel;
    if ( !encB )
    {
        sel -= 1;
        if ( sel < 0 )
            sel = 0;
    }
    else
    {
        sel += 1;
        if ( sel > 15 )
            sel = 15;
    }
    if ( sel != lastSel )
    {
        selector = ( selector & 0xf0 ) | sel;
        ret = 1;
    }
    return ret;
}

/*
 * The archetypal algorithm loop.
 */
void doAlgorithm0()
{
    static const int ranges[] = { 0, 1, 0, 8 };
    setParameterRanges( 2, ranges );

    // setup
    DECLARATIONS();
    TAP_DECLARATIONS();

    while ( 1 )
    {
        // wait for new audio frame
        IDLE();

        // read the inputs
        int vL = inL;
        int vR = inR;

        SHOW_POT_HANDLING()

        count += 1;
        TAP_HANDLING_START()
                currentParameter = ( currentParameter + 1 ) & 1;
                showPot = kTimeToShowPot;
                showValue( currentParameter, 0 );
        TAP_HANDLING_END()

        // do the processing
        int vOutL = vL + vR;
        int vOutR = vL - vR;
        CLAMP( vOutL );
        CLAMP( vOutR );

        // write the outputs
        outL = vOutL;
        outR = vOutR;

        // loop end processing
        // (including reading the ADC channels)
        LOOP_END();
    }
}

/*
 *
 */
int main(int argc, char** argv) {
    UINT spi_con1 = 0, spi_con2 = 0;

    SYSTEMConfig( SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE );
    INTEnableSystemMultiVectoredInt();

    // outputs
    PORTSetPinsDigitalOut( IOPORT_A, BIT_0 | BIT_1 | BIT_2 );
    PORTSetPinsDigitalOut( IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_14 | BIT_15 );
    // inputs
    PORTSetPinsDigitalIn( IOPORT_A, BIT_3 | BIT_4 );
    PORTSetPinsDigitalIn( IOPORT_B, BIT_3 | BIT_4 | BIT_5 | BIT_12 );
    // pot
    mPORTBSetPinsAnalogIn( ENABLE_AN11_ANA );       // B13

    // enable the pullups on the encoder & switches
    CNPUA = CNA3_PULLUP_ENABLE | CNA4_PULLUP_ENABLE;
    CNPUB = CNB12_PULLUP_ENABLE;

    // disable the watchdog timer
    WDTCONCLR = BIT_15;

    startupSequence();

    ConfigureCodec();

    // reconfigure I2C ports
    PORTSetPinsDigitalIn( IOPORT_B, BIT_8 | BIT_9 );

    int j;
    for ( j=0; j<4; ++j )
    {
        int i;
        PORTBINV = BIT_1;
        delayLoop( 500000 );
    }

    // read the stored last selector value
    readSelector();

    // SD card installed?
    setB15 =  BIT_15;

    // 48MHz
    // Tpb = 20.83333ns
    // 80000 Hz <=> 12500 ns
    //
    // Tad = 2 * ( ADCS + 1 ) * Tpb
    // conversion time = 12 Tad
    // Tad must be > 83.33ns
    //
    // 48Mhz
    // so ADCS must be >= 2
    //
    // 2 * ( 2+1 ) * 12 * 20.83ns = 1500 ns

    CloseADC10();   // Ensure the ADC is off before setting the configuration
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 );
    OpenADC10( ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_MANUAL | ADC_AUTO_SAMPLING_ON,
               ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF,
               ADC_CONV_CLK_PB | 2, // last number is ADCS
               ENABLE_AN11_ANA,
               SKIP_SCAN_ALL );
    EnableADC10(); // Enable the ADC

    // set the SAMP bit to start acquisition
    // from here on, this will be done automatically (ADC_AUTO_SAMPLING_ON)
    AcquireADC10();

    // timer 2 for encoder read
    CloseTimer2();
    INTSetVectorPriority( INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1 );
    INTSetVectorSubPriority( INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0 );

    /*------------------------------------------------
    Function	PPS	Pin#	Test Point#	: Comment
    __________________________________________________________________________
    REFOCLK(o)	RPB2	( 6)	TP36		: Reference Clock Output
    SCK1(o)	-	(25)	TP14            : SPI-I2S SCLK Output
    SS(o)	RPB4	(11)	TP33            : SPI-I2S LRCK Output
    SDO(o)	RPA1	( 3)	TP20            : SPI-I2S SDO Output
    SDI(i)	RPB5	(14)	-               : SPI-I2S SDI Input */
    PPSOutput(3, RPB2, REFCLKO); //REFCLK0: RPB2 - Out
    PPSInput(2, SDI1, RPB5); //SDI: RPB5 - In
    PPSOutput(2, RPA1, SDO1); //SDO: RPA1 - Out
    PPSOutput(1, RPB4, SS1); //SS: RPB4 - Out

    //Configure Reference Clock Output to 12.288MHz.
    mOSCREFOTRIMSet(REFTRIM);
    OSCREFConfig(OSC_REFOCON_SYSCLK, //SYSCLK clock output used as REFCLKO source
            OSC_REFOCON_OE | OSC_REFOCON_ON, //Enable and turn on the REFCLKO
            RODIV);

    //Configure SPI in I2S mode with 24-bit stereo audio.
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

    //Configure and turn on the SPI1 module.
    SpiChnOpenEx(SPI_CHANNEL1, spi_con1, spi_con2, SPI_SRC_DIV);

    //Enable SPI1 interrupt.
    IPC7bits.SPI1IP = 3;
    IPC7bits.SPI1IS = 1;
    IEC1bits.SPI1TXIE = 1;
//    IEC1bits.SPI1RXIE = 1;

    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI
    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI

    // read the pot to start with a valid value
    ConvertADC10();     /* start the next conversion */
    while ( !AD1CON1bits.DONE )
        ;
    pot = ReadADC10(0); /* Read the result of pot conversion */

    for ( ;; )
    {
        showSelector();
        int sel = selector;
        switch ( sel )
        {
            default:
            case 0:
                doAlgorithm0();
                break;
        }
    }

    return (EXIT_SUCCESS);
}

/* SPI1 ISR */
void __ISR(_SPI_1_VECTOR, ipl3) SPI1InterruptHandler(void)
{
    int toggleData = !PORTBbits.RB4;
    if ( IFS1bits.SPI1TXIF )
    {
        time += toggleData;

        SPI1BUF = toggleData ? outR : outL;
        IFS1bits.SPI1TXIF = 0;
    }
    if ( IFS1bits.SPI1RXIF )
    {
        int raw = SPI1BUF;
        raw = ( raw << 8 ) >> 8;
        if ( toggleData )
            inR = raw;
        else
            inL = raw;
        IFS1bits.SPI1RXIF = 0;
    }
}

// Timer 2 interrupt handler
void __ISR(_TIMER_2_VECTOR, IPL1SOFT) Timer2Handler(void)
{
    slowTime++;

    // Clear the interrupt flag
    INTClearFlag( INT_T2 );
}
