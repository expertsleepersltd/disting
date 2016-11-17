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

#ifndef GLOBAL_H
#define	GLOBAL_H

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

#define kMajorVersion 3
#define kMinorVersion 0

typedef long long int64_t;
typedef unsigned long long uint64_t;

extern int selector;
extern int pot;

extern void delayLoop( int count );

extern void showValue( int value, int negative );

extern void ConfigureCodec();
extern void ErrorHalt();

// display
extern int activeMenu;
extern void showSelector();
extern void showValue( int value, int negative );
extern void showVersion( int value );
extern void startupSequence();

#define SYS_FREQ 	(48000000L)

#ifdef __32MX170F256B__
#define CALIBRATION_FLASH_ADDR      0xBD03FE00
#define CURRENT_ALG_FLASH_ADDR      0xBD03F800
#define SETTINGS_FLASH_ADDR         0xBD03F400
#endif

#define SAMPLE_RATE     75000
#define RODIV		1
#define REFTRIM		128
#define SPI_SRC_DIV     4

#define kTimeToShowPot ( SAMPLE_RATE * 2 )

#define kMinimumTapCount ( ( SAMPLE_RATE * 10 ) / 1000 )

extern volatile unsigned int time;
extern volatile unsigned int slowTime;
extern volatile int inL, inR;
extern volatile int outL, outR;

extern int selector;
extern int parameters[];
extern int currentParameter;
extern int pot;
extern int showPot;

extern BYTE sdActive;       // sd card algorithm active
extern unsigned int setB15; // BIT_15 if we're allowed to use B15 for IO else 0
#define sdPresent (!setB15) // sd card detected at power up

extern unsigned int lastSlowTime;
extern int lastEncA;
extern void encoderPressed();
extern int processMenu();
extern int handleEncoderTurn( int encB );
extern int handleSelectorTurn( int encB );

#define SLOW_RATE (600)

#define kSlowTimeRatio (SAMPLE_RATE/SLOW_RATE)

#define DECLARATIONS()              \
    unsigned int slowTimeCountdown = kSlowTimeRatio;    \
    unsigned int thisTime, lastTime = time;

#define IDLEWAIT()                  \
        while ( ( thisTime = time ) == lastTime )  \
            ;

#define IDLE()                      \
        PORTACLR = BIT_0;           \
        IDLEWAIT()                  \
        PORTASET = BIT_0;

#define LOOP_END()                  \
        lastTime = thisTime;        \
        if ( --slowTimeCountdown == 0 )             \
        {                                           \
            slowTimeCountdown = kSlowTimeRatio;     \
            slowTime++;                             \
        }                                           \
        {                           \
            int ret = 0;            \
            /* menu active? */      \
            if ( activeMenu )           \
            {                           \
                ret += processMenu();   \
            }                           \
            /* encoder button */        \
            else if ( !PORTAbits.RA4 )  \
            {                           \
                encoderPressed();       \
            }                           \
            else                        \
            {                                       \
                /* encoder turned? */               \
                unsigned int thisSlowTime;          \
                if ( ( thisSlowTime = slowTime ) != lastSlowTime )  \
                {                                       \
                    int encA = PORTBbits.RB12;          \
                    int encB = PORTAbits.RA3;           \
                    int stepped = 0;                    \
                    if ( !encB )                        \
                    {                                   \
                        if ( !encA && lastEncA )        \
                        {                               \
                            stepped = 1;                \
                            encB = 1;                   \
                        }                               \
                        else if ( encA && !lastEncA )   \
                            stepped = 1;                \
                    }                                   \
                    if ( stepped )                      \
                    {                                   \
                        ret += handleEncoderTurn( encB );     \
                    }                                   \
                    lastEncA = encA;                    \
                }                                       \
                lastSlowTime = thisSlowTime;            \
            }                       \
            pot = ReadADC10(0); /* Read the result of pot conversion */ \
            ConvertADC10();     /* start the next conversion */         \
            if ( ret )              \
                break;              \
        }

#define CLAMP( x )                  \
        if ( x < -0x800000 )        \
            x = -0x800000;          \
        else if ( x > 0x7fffff )    \
            x = 0x7fffff;

#define SHOW_POT_HANDLING()         \
        if ( showPot > 0 )          \
        {                           \
            showPot -= 1;           \
            if ( showPot == 0 )     \
                showSelector();     \
        }

#define TAP_DECLARATIONS()          \
    int inTapTrigger = 0;           \
    int count = 0, upCount = 0;

#define TAP_HANDLING_START()        \
        if ( inTapTrigger )         \
        {                                       \
            /* minimum count to avoid bounce */ \
            if ( count >= kMinimumTapCount )    \
            {                                   \
                if ( PORTBbits.RB3 )            \
                {                               \
                    inTapTrigger = 0;           \
                    upCount = 0;                \
                }                               \
            }                                   \
        }                                       \
        else                                    \
        {                                       \
            upCount += 1;                                                   \
            if ( ( upCount >= kMinimumTapCount ) && !PORTBbits.RB3 )        \
            {                                                               \
                inTapTrigger = 1;
                
#define TAP_HANDLING_END()              \
                count = 0;              \
            }                           \
        }

#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */
