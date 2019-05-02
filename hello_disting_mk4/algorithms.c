
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>
#include <math.h>

#include "constants.h"

void sumDifference()
// sum/difference with integer V offsets
{
    static char const * const paramNames[] = {
        "Atten A",
        "Atten B",
    };

    static const char ranges[] = { 0, 32, 32,  0, 32, 32 };
    setParameterRanges( 2, ranges );

    DECLARATIONS();

    int offset = 0;

    while ( 1 )
    {
        IDLE();

        // .19 voltage
        int vL = ( ( inL - A[0] ) * Br[0] ) >> 24;
        int vR = ( ( inR - A[1] ) * Br[1] ) >> 24;

        offset = ( ( pot - 0x200 ) >> 7 ) << 19;

        SHOW_POT_HANDLING_PARAM_NAME()

        int vOutL = vL + vR + offset;
        int vOutR = vL - vR - offset;

        vOutL = ( vOutL * parameters[0] ) >> 5;
        vOutR = ( vOutR * parameters[1] ) >> 5;
        
        int cL = ( ( vOutL - D[0] ) * Er[0] ) >> 24;
        int cR = ( ( vOutR - D[1] ) * Er[1] ) >> 24;
        CLAMP( cL );
        CLAMP( cR );
        outL = cL;
        outR = cR;

        LOOP_END();
    }
}
