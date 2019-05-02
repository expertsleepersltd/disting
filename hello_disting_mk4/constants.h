
#ifndef CONSTANTS_H
#define	CONSTANTS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <peripheral/nvm.h>
#undef PAGE_SIZE
#undef BYTE_PAGE_SIZE
#undef ROW_SIZE
#undef BYTE_ROW_SIZE
#undef NUM_ROWS_PAGE

#define PAGE_SIZE               256        // # of 32-bit Instructions per Page
#define BYTE_PAGE_SIZE          (4 * PAGE_SIZE) // Page size in Bytes
#define ROW_SIZE                32         // # of 32-bit Instructions per Row
#define BYTE_ROW_SIZE           (4 * ROW_SIZE) // # Row size in Bytes
#define NUM_ROWS_PAGE           8              //Number of Rows per Page

#define STATIC_ASSERT(X) ({ extern int __attribute__((error("assertion failure: '" #X "' not true"))) compile_time_check(); ((X)?0:compile_time_check()),0; })

extern char pageBuffer[ BYTE_PAGE_SIZE ];
    
//#define DEBUG_SD_TIMING

typedef long long int64_t;
typedef unsigned long long uint64_t;

extern int pot;
extern BYTE potMIDIOverride;

extern void ConfigureCodec(void);

extern void readCalibration(void);

typedef void (algorithm)(void);

// algorithms
extern void sumDifference(void);

// display
#define ALL_COLUMNS ( BIT_2 | BIT_3 | BIT_4 | BIT_10 | BIT_13 )    
#define ALL_ROWS    ( BIT_0 | BIT_1 | BIT_4 | BIT_7 | BIT_8 | BIT_9 | BIT_10 )
extern char activeMenu;
extern short columns[];
extern char displayMode;
extern void clearColumns(void);
extern void showSelector(void);
extern void showSelectorFirstTime(void);
extern void showValue( unsigned int value, int flags );
extern void setColumnsFromValue( unsigned int value, int flags );
extern void showVersion( int value );
extern void startupSequence(void);
extern void updateDisplay( int dt );
extern void scrollMessageOnce( const char* msg );
extern void scrollMessageContinuously( const char* msg );
extern void scrollMessageOnceAndWait( const char* msg );
extern void scrollFilename( const char* msg );
extern void openTimerForScroll(void);
#define closeScrollTimer CloseTimer1
extern void setColumnsFromChars( const char* c );
extern int loopEndMenuProcess(void);

enum {
    kDisplayModeAlgorithm,
    kDisplayModeMenuValue,
    kDisplayModeMenu,
    kDisplayModeScrollOnce,
    kDisplayModeScroll,
};

#define SYS_FREQ 	(48000000L)

#ifdef __32MX170F256D__
#define CALIBRATION_FLASH_ADDR      0xBD03FE00
#define PRESETS_FLASH_ADDR          0xBD03F800
#define SETTINGS_FLASH_ADDR         0xBD03F400
#endif

#define SAMPLE_RATE     75000
#define REFDIV		1
#define REFTRIM		128
#define SPI_SRC_DIV     4

#define kTimeToShowPot ( SAMPLE_RATE * 2 )
#define kTimeToShowParamNumber ( SAMPLE_RATE / 2 )

extern volatile unsigned int time;
extern volatile unsigned int slowTime;
extern volatile int inL, inR;
extern volatile int outL, outR;

extern int A[2], D[2];
extern int64_t Br[2], Er[2];

extern BYTE selector;
extern BYTE actuallyRunningSelector;
extern BYTE numParameters;
extern char parameters[];
extern char const* parameterRanges;
extern BYTE currentParameter;
extern int pot;
extern int showPot;
extern BYTE preventAutoSave;
extern BYTE retIsFromSave;
extern short scrollCountdown;

#define parameterMin( p )   ( parameterRanges[ 3 * (p) + 0 ] )
#define parameterMax( p )   ( parameterRanges[ 3 * (p) + 1 ] )

extern unsigned int lastSlowTime;
extern BYTE lastEncA;
extern void encoderPressed(void);
extern int processMenu(void);
extern void handleEncoderTurn( int encB );
extern void setParameterRanges( int num, char const* ranges );

extern void LoadFavourites(void);
extern void ReadFavouritesFromFlash(void);
extern void SaveFavouritesToFlash(void);
extern void WriteSettingsPage( const char* buffer );
extern void enableRecallAccordingToSetting(void);

extern BYTE sdActive;          // sd card algorithm active
extern BYTE EnterSDMode(void);
extern void LeaveSDMode(void);

extern int ProcessMIDIIn( BYTE b );

extern int Recall_ProcessMIDI( BYTE b );

#define SLOW_RATE (600)

#define kSlowTimeRatio (SAMPLE_RATE/SLOW_RATE)

#define DECLARATIONS()              \
    int slowTimeCountdown = kSlowTimeRatio;    \
    unsigned int thisTime, lastTime = time;

#define IDLEWAIT()                  \
        while ( ( thisTime = time ) == lastTime )  \
            ;

#define IDLE()                      \
        PORTCCLR = BIT_9;           \
        IDLEWAIT()                  \
        PORTCSET = BIT_9;

#define LOOP_END_UPDATE_DISPLAY_COUNT (1)

#define LOOP_END_NOMIDI()        \
        if ( U2STAbits.URXDA )      \
        {                           \
            BYTE data = U2RXREG;    \
            if ( Recall_ProcessMIDI( data ) )   \
                break;                          \
        }                           \
        updateDisplay( LOOP_END_UPDATE_DISPLAY_COUNT );         \
        lastTime = thisTime;        \
        if ( --slowTimeCountdown == 0 )             \
        {                                           \
            slowTimeCountdown = kSlowTimeRatio;     \
            slowTime++;                             \
        }                                           \
        if ( loopEndMenuProcess() )                 \
            break;

#define LOOP_END_MIDIHANDLER()   \
        if ( U1STAbits.URXDA )      \
        {                           \
            BYTE data = U1RXREG;    \
            if ( ProcessMIDIIn( data ) ) \
                break;                      \
        }                           \
        LOOP_END_NOMIDI()

#define LOOP_END() LOOP_END_MIDIHANDLER()

#define CLAMP( x )                  \
        if ( x < -0x800000 )        \
            x = -0x800000;          \
        else if ( x > 0x7fffff )    \
            x = 0x7fffff;

#define SHOW_POT_HANDLING_DT( dt )         \
        if ( showPot > 0 )          \
        {                           \
            showPot -= dt;           \
            if ( showPot <= 0 )     \
                showSelector();     \
        }

#define SHOW_POT_HANDLING()         \
        SHOW_POT_HANDLING_DT( 1 )

#define SHOW_POT_HANDLING_PARAM_NAME_DT( dt )         \
        if ( showPot > 0 )          \
        {                           \
            showPot -= dt;           \
            if ( showPot <= 0 )     \
            {                           \
                if ( activeMenu <= 0 )  \
                    scrollMessageOnce( paramNames[currentParameter] );  \
            }                           \
        }

#define SHOW_POT_HANDLING_PARAM_NAME()         \
        SHOW_POT_HANDLING_PARAM_NAME_DT( 1 )

#ifdef	__cplusplus
}
#endif

#endif	/* CONSTANTS_H */
