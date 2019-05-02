
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>
#include <math.h>

#include "constants.h"
#include "settings.h"
#include "presets.h"

#include "pragma_configs.h"

#include "Compiler.h"
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "MDD File System/FSIO.h"

const int magic __attribute__((address(0xBD008370))) = 0xbadabeef;

int dummy;

volatile unsigned int time = 0;
volatile unsigned int slowTime = 0;
volatile int inL = 0, inR = 0;
volatile int outL = 0, outR = 0;

int A[2] = { 0 }, D[2] = { 0 };
int64_t Br[2] = { 0 }, Er[2] = { 0 };

BYTE currentParameter = 0;
BYTE numParameters = 0;
char parameters[kMaxParameters] = { 0 };
char const* parameterRanges = NULL;
int pot = 0;
int showPot = 0;

BYTE selector = 0;
BYTE actuallyRunningSelector = 0;

BYTE lastEncA = 1;
BYTE preventAutoSave = 0;
BYTE retIsFromSave = 0;
char pendingSave = 0;

BYTE potMIDIOverride = 0;

BYTE sdActive = 0;          // sd card algorithm active

void handleEncoderTurn( int encB )
{
    if ( numParameters == 0 )
        return;
    
    int p = parameters[currentParameter];
    if ( !encB )
    {
        p -= 1;
        if ( p < parameterMin( currentParameter ) )
            p = parameterMin( currentParameter );
    }
    else
    {
        p += 1;
        if ( p > parameterMax( currentParameter ) )
            p = parameterMax( currentParameter );
    }
    if ( p != parameters[currentParameter] )
    {
        parameters[currentParameter] = p;
        showPot = kTimeToShowPot;
        if ( p >= 0 )
            showValue( p, 0 );
        else
            showValue( -p, 1 );
    }
}

void setParameterRanges( int num, char const* ranges )
{
    if ( retIsFromSave )
    {
        retIsFromSave = 0;
        return;
    }
    
    showPot = 0;
    currentParameter = 0;

    numParameters = num;
    parameterRanges = ranges;

    int i;
    for ( i=0; i<num; ++i )
    {
        parameters[i] = ranges[ 3*i + 2 ];
    }
    for ( ; i<kMaxParameters; ++i )
    {
        parameters[i] = 0;
    }
    
    if ( pendingParameterLoad >= 0 )
    {
        loadParametersFromPreset( pendingParameterLoad );
        pendingParameterLoad = -1;
    }

    if ( !preventAutoSave )
        savePreset( 0 );
    preventAutoSave = 0;
}

void enableRecallAccordingToSetting()
{
    if ( settings.m_enableRecall )
    {
        PORTSetPinsDigitalIn( IOPORT_B, BIT_1 );
        UARTEnable( UART2, UART_ENABLE_FLAGS( UART_PERIPHERAL | UART_RX ) );
    }
    else
    {
        UARTEnable( UART2, UART_DISABLE_FLAGS( UART_PERIPHERAL | UART_RX ) );
        PORTSetPinsDigitalOut( IOPORT_B, BIT_1 );
    }
}

BYTE EnterSDMode()
{
    MDD_SDSPI_InitIO();

    return MDD_SDSPI_MediaDetect();
}

void LeaveSDMode()
{
    SpiChnClose( SPI_CHANNEL2 );
}

void testSDCard()
{
    BYTE sdPresent = EnterSDMode();
    if ( !sdPresent )
    {
        scrollMessageOnceAndWait( "No SD card" );
        return;
    }
    
    FSInit();

    FSFILE* F = FSfopen( "test.txt", "r" );
    if ( F != NULL )
    {
        char buff[100];
        memset( buff, 0, sizeof buff );
        FSfread( buff, 1, (sizeof buff)-1, F );
        scrollMessageOnceAndWait( buff );
        
        FSfclose( F );
    }
    
    LeaveSDMode();
}

int ProcessMIDIIn( BYTE b )
{
    return 0;
}

int Recall_ProcessMIDI( BYTE b )
{
    return 0;
}

/*
 *
 */
int main(int argc, char** argv) {
    UINT spi_con1 = 0, spi_con2 = 0;
    
    SYSTEMConfig( SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE );
    INTEnableSystemMultiVectoredInt();

    // LED outputs all high
    PORTA = BIT_0 | BIT_1 | BIT_4 | BIT_7 | BIT_8 | BIT_9 | BIT_10;
    // LED outputs all high, SD CS high, PGED1 low
    PORTB = BIT_2 | BIT_3 | BIT_4 | BIT_10 | BIT_13 | BIT_7;
    // test output high
    PORTC = BIT_9;

    // outputs
    PORTSetPinsDigitalOut( IOPORT_A, BIT_0 | BIT_1 | BIT_4 | BIT_7 | BIT_8 | BIT_9 | BIT_10 );
    PORTSetPinsDigitalOut( IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_3 | BIT_4 | BIT_10 | BIT_13 | BIT_7 );
    PORTSetPinsDigitalOut( IOPORT_C, BIT_9 );
    // inputs
    PORTSetPinsDigitalIn( IOPORT_A, BIT_2 | BIT_3 );
    PORTSetPinsDigitalIn( IOPORT_B, BIT_11 | BIT_6 | BIT_5 | BIT_8 | BIT_9 | BIT_14 | BIT_15 );
    PORTSetPinsDigitalIn( IOPORT_C, BIT_0 | BIT_1 | BIT_2 | BIT_4 | BIT_7 | BIT_8 | BIT_6 | BIT_3 | BIT_5 );
    // pot
    ANSELA = 0;
    TRISBSET = BIT_12;
    ANSELB = BIT_12;
    ANSELC = 0;

    // enable the pullups on the encoder & switches
    CNPUC = CNC0_PULLUP_ENABLE | CNC1_PULLUP_ENABLE | CNC2_PULLUP_ENABLE | CNC4_PULLUP_ENABLE;

    // setup PPS for SPI2 lines
    PPSInput( 3, SDI2, RPB6 );
    PPSOutput( 2, RPB5, SDO2 );

    // UART1
    PPSInput( 3, U1RX, RPC3 );
    PPSOutput( 1, RPC5, U1TX );
    UARTConfigure( UART1, 0 );
    UARTSetDataRate( UART1, SYS_FREQ, 31250 );
    UARTEnable( UART1, UART_ENABLE_FLAGS( UART_PERIPHERAL | UART_RX | UART_TX ) );
    ODCCSET = BIT_5;
    
    // UART2
    PPSInput( 2, U2RX, RPB1 );
    UARTConfigure( UART2, UART_INVERT_RECEIVE_POLARITY );
    UARTSetDataRate( UART2, SYS_FREQ, 31250 );
    
    // disable the watchdog timer
    WDTCONCLR = BIT_15;

    // timer 1 for timebase when SPI not running
    INTSetVectorPriority( INT_TIMER_1_VECTOR, INT_PRIORITY_LEVEL_1 );
    INTSetVectorSubPriority( INT_TIMER_1_VECTOR, INT_SUB_PRIORITY_LEVEL_0 );

    PPSOutput(3, RPC6, REFCLKO); //REFCLK0: RPC6 - Out
    PPSInput(2, SDI1, RPB11); //SDI: RPB11 - In
    PPSOutput(2, RPC8, SDO1); //SDO: RPC8 - Out
    PPSOutput(1, RPC7, SS1); //SS: RPC7 - Out

    //Configure Reference Clock Output to 12.288MHz.
    mOSCREFOTRIMSet(REFTRIM);
    OSCREFConfig(OSC_REFOCON_SYSCLK, //SYSCLK clock output used as REFCLKO source
            OSC_REFOCON_OE | OSC_REFOCON_ON, //Enable and turn on the REFCLKO
            REFDIV);

    //Configure SPI in I2S mode with 24-bit stereo audio.
    spi_con1 = SPI_OPEN_MSTEN | //Master mode enable
            SPI_OPEN_SSEN | //Enable slave select function
            SPI_OPEN_CKP_HIGH | //Clock polarity Idle High Active Low
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

    startupSequence();

    ConfigureCodec();

    // reconfigure I2C ports
    PORTSetPinsDigitalIn( IOPORT_B, BIT_8 | BIT_9 );
    
    // read the calibration data
    readCalibration();
    
    // read the favourites
    ReadFavouritesFromFlash();
    enableRecallAccordingToSetting();

    // read the stored last selector value
    selector = selectorFromPreset( 0 );

    CloseADC10();   // Ensure the ADC is off before setting the configuration
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN12 );
    // OpenADC10() but without the ANA setup
    AD1CSSL = ~(SKIP_SCAN_ALL);
    AD1CON3 = ADC_CONV_CLK_PB | 2; // last number is ADCS
    AD1CON2 = ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF;
    AD1CON1 = ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_MANUAL | ADC_AUTO_SAMPLING_ON;
    EnableADC10(); // Enable the ADC

    // set the SAMP bit to start acquisition
    // from here on, this will be done automatically (ADC_AUTO_SAMPLING_ON)
    AcquireADC10();

    // timer 2 for encoder read
    CloseTimer2();
    INTSetVectorPriority( INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1 );
    INTSetVectorSubPriority( INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0 );

    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI
    SpiChnPutC(SPI_CHANNEL1, 0); //Dummy write to start the SPI

    //Enable SPI1 interrupt.
    IPC7bits.SPI1IP = 3;
    IPC7bits.SPI1IS = 1;
    IEC1bits.SPI1TXIE = 1;

    for ( ;; )
    {
        showSelectorFirstTime();
        
        // clear any UART overrun error just in case
        U1STACLR = BIT_1;
        U2STACLR = BIT_1;

        potMIDIOverride = 0;
        
        int sel = selector;
        if ( sel >= 112 )
            sel = settings.m_favourites[ sel - 112 ];
        if ( sel >= 64 && sel < 88 )
        {
            BYTE sdPresent = EnterSDMode();
            if ( sdPresent )
                sdActive = 1;
            else
            {
                scrollMessageOnce( "No SD card" );
                sel = 0;
            }
        }
        actuallyRunningSelector = sel;
        
        static algorithm* const algorithms[] = {
/*  0 -  4 */       sumDifference, sumDifference, sumDifference, sumDifference,
/*  4 -  7 */       sumDifference, sumDifference, sumDifference, sumDifference,
/*  8 - 11 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 12 - 15 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 16 - 19 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 20 - 23 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 24 - 27 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 28 - 31 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 32 - 35 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 36 - 39 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 40 - 43 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 44 - 47 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 48 - 51 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 52 - 55 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 56 - 59 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 60 - 63 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 64 - 67 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 68 - 71 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 72 - 75 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 76 - 79 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 80 - 83 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 84 - 87 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 88 - 91 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 92 - 95 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 96 - 99 */       sumDifference, sumDifference, sumDifference, sumDifference,
/* 100 - 103 */     sumDifference, sumDifference, sumDifference, sumDifference,
/* 104 - 107 */     sumDifference, sumDifference, sumDifference, sumDifference,
        };
        if ( sel >= sizeof algorithms/sizeof algorithms[0] )
            sel = 0;
        algorithms[sel]();

        sdActive = 0;
        LeaveSDMode();
        
        if ( pendingSave )
            handlePendingSave();
    }

    return (EXIT_SUCCESS);
}

/* SPI1 ISR */
void __ISR(_SPI_1_VECTOR, ipl3soft) SPI1InterruptHandler(void)
{
    int toggleData = !PORTCbits.RC7;
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
