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

#ifndef GLOBAL_H
#define	GLOBAL_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef long long int64_t;
typedef unsigned long long uint64_t;

extern int selector;
extern int pot;

extern void delayLoop( int count );

extern void ConfigureCodec();
extern void ErrorHalt();

#define SYS_FREQ 	(40000000L)

#define SAMPLE_RATE     78125
#define RODIV		0
#define REFTRIM		0
#define SPI_SRC_DIV     8

extern volatile unsigned int time;
extern volatile int inL, inR;
extern int outL, outR;

#define DECLARATIONS()              \
    unsigned int thisTime, lastTime = time;

#define IDLEWAIT()                  \
        while ( ( thisTime = time ) == lastTime )  \
            ;

#define IDLE()                      \
        PORTACLR = BIT_2;           \
        IDLEWAIT()                  \
        PORTASET = BIT_2;

#define LOOP_END()                  \
        lastTime = thisTime;        \
        if ( mAD1GetIntFlag() )     \
        {                           \
            if ( readADC() )        \
                return;             \
        }

#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */
