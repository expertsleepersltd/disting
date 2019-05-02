
#include <plib.h>

#include "constants.h"

int checkValidRange( int v, int min, int max )
{
    if ( v < min || v > max )
        return 0;
    return 1;
}

int checkValidRanges( int zeroIn, int zeroOut, int halfOut, int threeVolt )
{
    if ( !checkValidRange( zeroIn,      -0x100000, 0x100000 ) ||
            !checkValidRange( zeroOut,  -0x100000, 0x100000 ) ||
            !checkValidRange( halfOut,   0x200000, 0x600000 ) ||
            !checkValidRange( threeVolt, 0x100000, 0x380000 ) )
        return 0;
    return 1;
}

void WarnIgnoreCalibration()
{
    openTimerForScroll();
    scrollMessageOnceAndWait( "Uncalibrated" );
    closeScrollTimer();
}

void    readCalibration()
{
    int* ptr = (int*)CALIBRATION_FLASH_ADDR;
    int i;
    for ( i=0; i<2; ++i )
    {
        int zeroIn = *ptr++;
        int zeroOut = *ptr++;
        int halfOut = *ptr++;
        int threeVolt = *ptr++;

        if ( !checkValidRanges( zeroIn, zeroOut, halfOut, threeVolt ) )
        {
            WarnIgnoreCalibration();
            zeroIn = zeroOut = halfOut = threeVolt = 0;
        }

        /*
         * A = zeroIn                                   24 bit
         * B = ( threeVolt - zeroIn )/3                 24 bit
         * D = ( zeroOut - zeroIn )/B                   .19 bits
         * E = ( halfOut - zeroOut )/( B * 0x400000 )
         *
         * voltages have 19 fractional bits
         * 1V = 0x80000
         *
         * Vi = ( ( Ci - A ) * Br ) >> 24
         * Co = ( ( Vo - D ) * Er ) >> 24
         */

        A[i] = zeroIn;
        int B = ( threeVolt - zeroIn ) / 3;
        // avoid a divide by zero
        if ( B == 0 )
            B = 0xCCCCC;
        int64_t Bri = 0x80000000000LL / B;
        Br[i] = Bri;

        D[i] = ( ( zeroOut - zeroIn ) * Bri ) >> 24;
        // 0x400000 << 5 = 0x8000000
        int64_t Bb = ((int64_t)B) << 27;
        int n = halfOut - zeroOut;
        // avoid a divide by zero
        if ( n == 0 )
            n = 0x399999;
        Er[i] = Bb / n;
    }

    /*
     * Worked example
     * Assuming zeroIn == zeroOut == 0
     * Input range ±10V
     * Output range ±9V
     * then
     * threeVolt = 0x7fffff * 3 / 10 = 0x266666
     * halfOut voltage = 4.5
     * halfOut = 0x7fffff * 4.5 / 10 = 3774873 = 0x399999
     *
     * A = 0
     * B = 0x266666/3 = 0xCCCCC
     * Br = 0xA0000A
     * D = 0
     * Bb = 0x33333000000
     * Er = 0xE38E2
     *
     * input of 5V / 0x400000 should give .19 voltage of 0x280000
     * ( ( Ci - A ) * Br ) >> 24 = ( 0x400000 * 0xA0000A ) >> 24 = 0x280002
     *
     * output .19 voltage of 0x280000 should give output of  5V / 0x471C71
     * ( ( Vo - D ) * Er ) >> 19 = ( 0x280000 * 0xE38E2 ) >> 19 = 0x471C6A
     */
}
