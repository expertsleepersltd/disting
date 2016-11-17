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

// LEDs:
//
// B0   B1
// A2   B15
// B10  B11
// B6   B7
//
// When using SD:
// A2 is Dout from SD       if sdPresent, stays as input
// B15 is clk               if sdPresent, keep low
// B6 is Din to SD          if sdPresent, use as normal

enum {
    kMenuNone = 0,
    kMenuLeft = 1,
    kMenuRight = 2,
    kMenuRightSelected = kMenuRight+1,
    kMenuChooseAlgorithm = 4,
    kMenuChooseAlgorithmSelected = kMenuChooseAlgorithm+1,
    kMenuChooseBank = 6,
    kMenuChooseBankSelected = kMenuChooseBank+1,
};

enum {
    kDisplayModeMenu,
    kDisplayMode1To16,
};

int activeMenu = 0;
int menuL = -1;
int menuR = -1;
int menuFlash = 0;
int displayMode = kDisplayModeMenu;
int menuValue, menuMin, menuMax;

unsigned int lastSlowTime;
int pushButton;
int turnedWhilePressed;

void updateDisplayLeft()
{
    if ( activeMenu <= 0 )
        return;

    int val;
    switch ( displayMode )
    {
        default:
        case kDisplayModeMenu:
            val = menuFlash ? ( menuL - 1 ) : -1;
            break;
        case kDisplayMode1To16:
            val = menuFlash ? ( menuValue >> 2 ) : -1;
            break;
    }
    switch ( val )
    {
        default:
            PORTACLR = BIT_2;
            PORTBCLR = BIT_0 | BIT_10 | BIT_6;
            break;
        case 0:
            PORTACLR = BIT_2;
            PORTBCLR = BIT_10 | BIT_6;
            PORTBSET = BIT_0;
            break;
        case 1:
            PORTBCLR = BIT_0 | BIT_10 | BIT_6;
            PORTASET = BIT_2;
            break;
        case 2:
            PORTACLR = BIT_2;
            PORTBCLR = BIT_0 | BIT_6;
            PORTBSET = BIT_10;
            break;
        case 3:
            PORTACLR = BIT_2;
            PORTBCLR = BIT_0 | BIT_10;
            PORTBSET = BIT_6;
            break;
    }
}

void updateDisplayRight()
{
    if ( activeMenu <= 0 )
        return;

    int val;
    switch ( displayMode )
    {
        default:
        case kDisplayModeMenu:
            val = menuFlash ? ( menuR - 1 ) : -1;
            break;
        case kDisplayMode1To16:
            val = menuFlash ? ( menuValue & 3 ) : -1;
            break;
    }
    switch ( val )
    {
        default:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_1;
            break;
        case 0:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7;
            PORTBSET = BIT_1;
            break;
        case 1:
            PORTBCLR = BIT_11 | BIT_7 | BIT_1;
            PORTBSET = setB15;
            break;
        case 2:
            PORTBCLR = BIT_15 | BIT_7 | BIT_1;
            PORTBSET = BIT_11;
            break;
        case 3:
            PORTBCLR = BIT_15 | BIT_11 | BIT_1;
            PORTBSET = BIT_7;
            break;
    }
}

void encoderPressed()
{
    // encoder pressed now
    lastSlowTime = slowTime;
    // menu -1 is hot swap of algorithm
    activeMenu = -1;
    turnedWhilePressed = 0;
    displayMode = kDisplayModeMenu;
}

void activateMenu()
{
    activeMenu = kMenuLeft;
    menuL = 1;
    menuR = 0;
    menuFlash = 1;
    pushButton = 0;
}

void deactivateMenu()
{
    unsigned int thisSlowTime;

    // timer 2 for encoder read
    // ( 48MHz / 256 ) / 750 = 250
    OpenTimer2( T2_ON | T2_SOURCE_INT | T2_PS_1_256, 750 );
    INTEnable( INT_T2, INT_ENABLED );

    // wait for push button release
    while ( 1 )
    {
        if ( ( thisSlowTime = slowTime ) != lastSlowTime )
        {
            pushButton = !PORTAbits.RA4;
            if ( !pushButton )
                break;
        }
        lastSlowTime = thisSlowTime;
    }
    // wait for one clock
    lastSlowTime = slowTime;
    while ( slowTime == lastSlowTime )
        ;
    activeMenu = 0;
    lastSlowTime = 0;
    CloseTimer2();
    showSelector();
}

int menuTick()
{
    int ret = 0;
    int newSelector;

    // read encoder
    int encA = PORTBbits.RB12;
    int encB = PORTAbits.RA3;
    int stepped = 0;
    if ( !encB )
    {
        if ( !encA && lastEncA )
        {
            stepped = 1;
            encB = 1;
        }
        else if ( encA && !lastEncA )
            stepped = 1;
    }
    if ( stepped )
    {
        switch ( activeMenu )
        {
            default:
            case 0:
                break;
            case kMenuLeft:
                // select left menu
                menuL -= 1;
                menuL = ( menuL + ( encB ? 1 : -1 ) ) & 3;
                menuL += 1;
                break;
            case kMenuRight:
                // select right menu
                menuR -= 1;
                menuR = ( menuR + ( encB ? 1 : -1 ) ) & 3;
                menuR += 1;
                break;
            case kMenuChooseAlgorithm:
            case kMenuChooseBank:
                menuValue = menuValue + ( encB ? 1 : -1 );
                if ( menuValue > menuMax )
                    menuValue = menuMax;
                else if ( menuValue < menuMin )
                    menuValue = menuMin;
                break;
        }
        
        updateDisplayLeft();
        updateDisplayRight();
    }
    lastEncA = encA;

    // read button
    int lastButton = pushButton;
    pushButton = !PORTAbits.RA4;
    if ( pushButton && !lastButton )
    {
        // advance through menus
        activeMenu++;
        switch ( activeMenu )
        {
            case kMenuRight:
#ifdef TWO_LAYER_MENUS
                // advance to right menu
                menuR = 1;
                break;
#else
                activeMenu = kMenuRight+1;
#endif
            case kMenuRight+1:
                // right menu selected
                switch ( menuL )
                {
                    default:
                        break;
#ifdef TWO_LAYER_MENUS
                    case 1:
                        switch ( menuR )
                        {
                            default:
                                break;
#endif
                            case 1:
                                activeMenu = kMenuChooseAlgorithm;
                                displayMode = kDisplayMode1To16;
                                menuValue = selector & 0xf;
                                menuMin = 0;
                                menuMax = 15;
                                break;
                            case 2:
                                activeMenu = kMenuChooseBank;
                                displayMode = kDisplayMode1To16;
                                menuValue = selector >> 4;
                                menuMin = 0;
                                menuMax = 15;
                                break;
#ifdef TWO_LAYER_MENUS
                        }
                        break;
#endif
                    case 4:
#ifdef TWO_LAYER_MENUS
                        switch ( menuR )
                        {
                            default:
                                break;
                            case 4:
#endif
//                                calibrate();
                                ret = 1;
                                break;
#ifdef TWO_LAYER_MENUS
                        }
                        break;
#endif
                }
                if ( activeMenu == kMenuRight+1 )
                    deactivateMenu();
                break;
            case kMenuChooseAlgorithm+1:
                newSelector = ( selector & 0xf0 ) | menuValue;
                if ( newSelector != selector )
                {
                    selector = newSelector;
                    ret = 1;
                }
                deactivateMenu();
                break;
            case kMenuChooseBank+1:
                newSelector = ( selector & 0xf ) | ( menuValue << 4 );
                if ( !sdPresent && ( menuValue == 4 ) )
                    newSelector = selector;
                if ( newSelector != selector )
                {
                    selector = newSelector;
                    ret = 1;
                }
                deactivateMenu();
                break;
            default:
                deactivateMenu();
                break;
        }

        updateDisplayLeft();
        updateDisplayRight();
    }

    // menu flash
    static unsigned int flashTimer = 0;
    int flash;
    switch ( displayMode )
    {
        default:
        case kDisplayModeMenu:
            if ( ( ++flashTimer & 0x1f ) == 0 )
            {
                menuFlash = 1 - menuFlash;
                updateDisplayLeft();
                updateDisplayRight();
            }
            break;
        case kDisplayMode1To16:
            ++flashTimer;
            flash = ( flashTimer >> 5 ) & 0x3;
            flash = ( flash == 0 ? 0 : 1 );
            if ( flash != menuFlash )
            {
                menuFlash = flash;
                updateDisplayLeft();
                updateDisplayRight();
            }
            break;
    }

    return ret;
}

int processMenu()
{
    int ret = 0;

    unsigned int thisSlowTime;

    if ( ( thisSlowTime = slowTime ) != lastSlowTime )
    {
        if ( !sdActive )
        {
            // click Z to cancel menu
            if ( !PORTBbits.RB3 )
            {
                deactivateMenu();
                return ret;
            }
        }
        
        if ( activeMenu == -1 )
        {
            // encoder released?
            if ( PORTAbits.RA4 )
            {
                if ( turnedWhilePressed )
                    deactivateMenu();
                else
                    activateMenu();
            }
            else
            {
                // read encoder
                int encA = PORTBbits.RB12;
                int encB = PORTAbits.RA3;
                int stepped = 0;
                if ( !encB )
                {
                    if ( !encA && lastEncA )
                    {
                        stepped = 1;
                        encB = 1;
                    }
                    else if ( encA && !lastEncA )
                        stepped = 1;
                }
                if ( stepped )
                {
                    turnedWhilePressed = 1;
                    ret += handleSelectorTurn( encB );
                }
                lastEncA = encA;
            }
        }
        else
        {
            ret += menuTick();
        }
    }
    lastSlowTime = thisSlowTime;

    return ret;
}

void showSelector()
{
    if ( activeMenu > 0 )
        return;
    
    switch ( selector & 0xf )
    {
        default:
        case 0:
            PORTBSET = BIT_0 | BIT_1;
            PORTBCLR = BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 1:
            PORTBSET = BIT_0 | setB15;
            PORTBCLR = BIT_1 | BIT_6 | BIT_7 | BIT_10 | BIT_11;
            PORTACLR = BIT_2;
            break;
        case 2:
            PORTBSET = BIT_0 | BIT_11;
            PORTBCLR = BIT_1 | BIT_6 | BIT_7 | BIT_10 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 3:
            PORTBSET = BIT_0 | BIT_7;
            PORTBCLR = BIT_1 | BIT_6 | BIT_10 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 4:
            PORTBSET = BIT_1;
            PORTBCLR = BIT_0 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_15;
            PORTASET = BIT_2;
            break;
        case 5:
            PORTBSET = setB15;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_7 | BIT_10 | BIT_11;
            PORTASET = BIT_2;
            break;
        case 6:
            PORTBSET = BIT_11;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_7 | BIT_10 | BIT_15;
            PORTASET = BIT_2;
            break;
        case 7:
            PORTBSET = BIT_7;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_10 | BIT_11 | BIT_15;
            PORTASET = BIT_2;
            break;
        case 8:
            PORTBSET = BIT_1 | BIT_10;
            PORTBCLR = BIT_0 | BIT_6 | BIT_7 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 9:
            PORTBSET = BIT_10 | setB15;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_7 | BIT_11;
            PORTACLR = BIT_2;
            break;
        case 10:
            PORTBSET = BIT_10 | BIT_11;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_7 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 11:
            PORTBSET = BIT_10 | BIT_7;
            PORTBCLR = BIT_0 | BIT_1 | BIT_6 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 12:
            PORTBSET = BIT_1 | BIT_6;
            PORTBCLR = BIT_0 | BIT_7 | BIT_10 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 13:
            PORTBSET = BIT_6 | setB15;
            PORTBCLR = BIT_0 | BIT_1 | BIT_7 | BIT_10 | BIT_11;
            PORTACLR = BIT_2;
            break;
        case 14:
            PORTBSET = BIT_6 | BIT_11;
            PORTBCLR = BIT_0 | BIT_1 | BIT_7 | BIT_10 | BIT_15;
            PORTACLR = BIT_2;
            break;
        case 15:
            PORTBSET = BIT_6 | BIT_7;
            PORTBCLR = BIT_0 | BIT_1 | BIT_10 | BIT_11 | BIT_15;
            PORTACLR = BIT_2;
            break;
    }
}

void    showValue( int value, int negative )
{
    if ( activeMenu )
        return;

    switch ( value )
    {
        default:
        case 0:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6 | BIT_1;
            break;
        case 1:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6;
            PORTBSET = BIT_1;
            break;
        case 2:
            PORTBCLR = BIT_11 | BIT_7 | BIT_6 | BIT_1;
            PORTBSET = setB15;
            break;
        case 3:
            PORTBCLR = BIT_11 | BIT_7 | BIT_6;
            PORTBSET = setB15 | BIT_1;
            break;
        case 4:
            PORTBCLR = BIT_15 | BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_11;
            break;
        case 5:
            PORTBCLR = BIT_15 | BIT_7 | BIT_6;
            PORTBSET = BIT_11 | BIT_1;
            break;
        case 6:
            PORTBCLR = BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_11 | setB15;
            break;
        case 7:
            PORTBCLR = BIT_7 | BIT_6;
            PORTBSET = ( BIT_11 | BIT_1 ) | setB15;
            break;
        case 8:
            PORTBCLR = BIT_15 | BIT_11 | BIT_6 | BIT_1;
            PORTBSET = BIT_7;
            break;
        case 9:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6;
            PORTBSET = BIT_7 | BIT_1;
            break;
        case 10:
            PORTBCLR = BIT_11 | BIT_6 | BIT_1;
            PORTBSET = setB15 | BIT_7;
            break;
        case 11:
            PORTBCLR = BIT_11 | BIT_6;
            PORTBSET = setB15 | ( BIT_7 | BIT_1 );
            break;
        case 12:
            PORTBCLR = BIT_15 | BIT_6 | BIT_1;
            PORTBSET = BIT_11 | BIT_7;
            break;
        case 13:
            PORTBCLR = BIT_15 | BIT_6;
            PORTBSET = BIT_11 | BIT_7 | BIT_1;
            break;
        case 14:
            PORTBCLR = BIT_6 | BIT_1;
            PORTBSET = ( BIT_11 | BIT_7 ) | setB15;
            break;
        case 15:
            PORTBCLR = BIT_6;
            PORTBSET = setB15 | ( BIT_11 | BIT_7 | BIT_1 );
            break;
        case 16:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_1;
            PORTBSET = BIT_6;
            break;
        case 17:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7;
            PORTBSET = BIT_1 | BIT_6;
            break;
        case 18:
            PORTBCLR = BIT_11 | BIT_7 | BIT_1;
            PORTBSET = setB15 | BIT_6;
            break;
        case 19:
            PORTBCLR = BIT_11 | BIT_7;
            PORTBSET = setB15 | ( BIT_1 | BIT_6 );
            break;
        case 20:
            PORTBCLR = BIT_15 | BIT_7 | BIT_1;
            PORTBSET = BIT_11 | BIT_6;
            break;
        case 21:
            PORTBCLR = BIT_15 | BIT_7;
            PORTBSET = BIT_11 | BIT_1 | BIT_6;
            break;
        case 22:
            PORTBCLR = BIT_7 | BIT_1;
            PORTBSET = ( BIT_11 | BIT_6 ) | setB15;
            break;
        case 23:
            PORTBCLR = BIT_7;
            PORTBSET = ( BIT_11 | BIT_1 | BIT_6 ) | setB15;
            break;
        case 24:
            PORTBCLR = BIT_15 | BIT_11 | BIT_1;
            PORTBSET = BIT_7 | BIT_6;
            break;
        case 25:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7;
            PORTBSET = BIT_7 | BIT_1 | BIT_6;
            break;
        case 26:
            PORTBCLR = BIT_11 | BIT_1;
            PORTBSET = setB15 | ( BIT_7 | BIT_6 );
            break;
        case 27:
            PORTBCLR = BIT_11;
            PORTBSET = setB15 | ( BIT_7 | BIT_1 | BIT_6 );
            break;
        case 28:
            PORTBCLR = BIT_15 | BIT_1;
            PORTBSET = BIT_11 | BIT_7 | BIT_6;
            break;
        case 29:
            PORTBCLR = BIT_15;
            PORTBSET = BIT_11 | BIT_7 | BIT_1 | BIT_6;
            break;
        case 30:
            PORTBCLR = BIT_1;
            PORTBSET = ( BIT_6 | BIT_11 | BIT_7 ) | setB15;
            break;
        case 31:
            PORTBSET = setB15 | ( BIT_11 | BIT_7 | BIT_1 | BIT_6 );
            break;
    }

    if ( negative )
    {
        PORTBSET = BIT_10 | BIT_0;
        PORTASET = BIT_2;
    }
    else
    {
        PORTBSET = BIT_0;
        PORTBCLR = BIT_10;
        PORTASET = BIT_2;
    }
}

void    showVersion( int value )
{
    switch ( value )
    {
        default:
        case 0:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6 | BIT_1;
            break;
        case 1:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6;
            PORTBSET = BIT_1;
            break;
        case 2:
            PORTBCLR = BIT_11 | BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_15;
            break;
        case 3:
            PORTBCLR = BIT_11 | BIT_7 | BIT_6;
            PORTBSET = BIT_15 | BIT_1;
            break;
        case 4:
            PORTBCLR = BIT_15 | BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_11;
            break;
        case 5:
            PORTBCLR = BIT_15 | BIT_7 | BIT_6;
            PORTBSET = BIT_11 | BIT_1;
            break;
        case 6:
            PORTBCLR = BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_11 | BIT_15;
            break;
        case 7:
            PORTBCLR = BIT_7 | BIT_6;
            PORTBSET = BIT_11 | BIT_15 | BIT_1;
            break;
        case 8:
            PORTBCLR = BIT_15 | BIT_11 | BIT_6 | BIT_1;
            PORTBSET = BIT_7;
            break;
        case 9:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_6 | BIT_1;
            PORTBSET = BIT_7;
            break;
        case 10:
            PORTBCLR = BIT_11 | BIT_6 | BIT_1;
            PORTBSET = BIT_15 | BIT_7;
            break;
        case 11:
            PORTBCLR = BIT_11 | BIT_6;
            PORTBSET = BIT_15 | BIT_7 | BIT_1;
            break;
        case 12:
            PORTBCLR = BIT_15 | BIT_6 | BIT_1;
            PORTBSET = BIT_11 | BIT_7;
            break;
        case 13:
            PORTBCLR = BIT_15 | BIT_6;
            PORTBSET = BIT_11 | BIT_7 | BIT_1;
            break;
        case 14:
            PORTBCLR = BIT_6 | BIT_1;
            PORTBSET = BIT_11 | BIT_15 | BIT_7;
            break;
        case 15:
            PORTBCLR = BIT_6;
            PORTBSET = BIT_15 | BIT_11 | BIT_7 | BIT_1;
            break;
        case 16:
            PORTBCLR = BIT_15 | BIT_11 | BIT_7 | BIT_1;
            PORTBSET = BIT_6;
            break;
    }

    PORTBSET = BIT_0;
    PORTBCLR = BIT_10;
    PORTACLR = BIT_2;
}

void startupSequence()
{
    int sequence[8][2] = {
    { 0, BIT_1 },
    { 0, BIT_15 },
    { 0, BIT_11 },
    { 0, BIT_7 },
    { 0, BIT_0 },
    { BIT_2, 0 },
    { 0, BIT_10 },
    { 0, BIT_6 },
    };
    PORTA = 0;
    PORTB = 0;
    int apins = BIT_2;
    int bpins = BIT_0 | BIT_1 | BIT_6 | BIT_7 | BIT_10 | BIT_11 | BIT_15;
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

    showVersion( kMajorVersion );
    delayLoop( 1000000 );
    PORTACLR = apins;
    PORTBCLR = bpins;
    delayLoop( 1000000 );
    showVersion( kMinorVersion );
    delayLoop( 1000000 );
}
