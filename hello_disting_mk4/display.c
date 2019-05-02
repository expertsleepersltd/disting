
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "constants.h"
#include "settings.h"
#include "presets.h"
#include "algorithmNames.h"

static const short font[95][3] = {
#include "font.h"
};

const short* getFont( int c )
{
    c -= 32;
    if ( ( c < 0 ) || ( c >= 95 ) )
        c = 0;
    return font[ c ];
}

static const int dimPatterns[16] = { 0x00010001, 0x01010101, 0x08410841, 0x11111111, 
                                     0x24912491, 0x52495249, 0x55495549, 0x55555555,
                                     0x556D556D, 0x5B6D5B6D, 0xB6DDB6DD, 0xDDDDDDDD,
                                     0xEF7DEF7D, 0xFDFDFDFD, 0xFFFDFFFD, 0xFFFFFFFF };

static char const * const kVersionString = "v1.0.0";

static const short matrixRows[7] = { BIT_10, BIT_7, BIT_8, BIT_9, BIT_0, BIT_4, BIT_1 };
static const short matrixCols[5] = { BIT_10, BIT_13, BIT_2, BIT_3, BIT_4 };

short columns[7] = { 0 };
int dimPattern = 0;
#define resetDimPattern() { dimPattern = dimPatterns[ settings.m_brightness-1 ]; }

enum {
    kMenuNone = 0,
    kMenuLeft = 1,
    kMenuRight = 2,
    kMenuRightSelected = kMenuRight+1,
    kMenuChooseAlgorithm = 4,
    kMenuChooseAlgorithmSelected = kMenuChooseAlgorithm+1,
    kMenuEditSetting = 8,
    kMenuEditSettingSelected = kMenuEditSetting+1,
    kMenuChoosePreset = 10,
    kMenuChoosePresetSelected = kMenuChoosePreset+1,
};

enum {
    kMenuLAlgorithm = 0,
    kMenuSave,
    kMenuLoad,
    kMenuTestSD,
    kMenuSettings,
    kNumMenuL,
};

char const * const menuLStrings[kNumMenuL] = {
    "Algorithm",
    "Save",
    "Load",
    "Test SD",
    "Settings",
};

enum { 
    kSettingBrightness,
    kSettingEnableRecall,
    kSettingMidiChIn,
    kSettingMidiChOut,
    kSettingMidiThru,
    kSettingMidiPgmOption,
    kSettingContinuousFilename,
    kSettingScrollSpeed,
    kSettingSDSampleRate,
    kNumSettings,
};

char const * const settingsStrings[] = {
    "Brightness",
    "Recall enable",
    "In MIDI ch",
    "Out MIDI ch",
    "Thru MIDI",
    "Pgm Chng Alg",
    "Cont filenames",
    "Scroll speed",
    "WAV sample rate",
};

unsigned int lastSlowTime = 0;

char activeMenu = 0;
char menuL = -1;
char menuR = -1;
BYTE numMenuR = 0;
BYTE menuFlash = 0;
char displayMode = kDisplayModeAlgorithm;
short menuValue = 0, menuMin = 0, menuMax = 0;
char displayScroll = 0;
BYTE displayMessagePos = 0;
char editingSetting = 0;
short scrollSpeed = 0;
short scrollCountdown = 0;
const char* displayMessage = 0;
char const * const * menuRStrings = 0;
char menuAlgorithmName[2+1+kMenuAlgorithmNameMaxChars+1+1] = { 0 };

BYTE pushButton = 0;
BYTE turnedWhilePressed = 0;

void clearColumns()
{
    *(int*)&columns[0] = 0;
    *(int*)&columns[2] = 0;
    *(int*)&columns[4] = 0;
    columns[6] = 0;
}

void setColumnsFromChars( const char* c )
{
    const short* f;
    f = font[ c[0]-32 ];
    columns[0] = f[0];            columns[1] = f[1];            columns[2] = f[2];
    columns[3] = 0;
    f = font[ c[1]-32 ];
    columns[4] = f[0];            columns[5] = f[1];            columns[6] = f[2];
}

void setColumnsFromSelector()
{
    const short* f;
    int d;
    d = selector >> 3;
    f = font[ ('A'-32)+d ];
    columns[0] = f[0];            columns[1] = f[1];            columns[2] = f[2];
    columns[3] = 0;
    d = selector & 7;
    f = font[ ('1'-32)+d ];
    columns[4] = f[0];            columns[5] = f[1];            columns[6] = f[2];
}

void updateMenuDisplay()
{
    if ( activeMenu <= 0 )
        return;

    if ( !menuFlash )
        dimPattern = 0;
    else
        resetDimPattern();

    char chars[2] = { 0, 0 };
    
    switch ( displayMode )
    {
        default:
        case kDisplayModeMenu:
            break;
        case kDisplayModeMenuValue:
            setColumnsFromValue( menuValue, 0 );
            break;
    }
}

void setMenuString( const char* msg )
{
    scrollMessageContinuously( msg );
    displayMode = kDisplayModeMenu;
}

void setChooseAlgorithmString()
{
    menuAlgorithmName[0] = 'A' + ( menuValue >> 3 );
    menuAlgorithmName[1] = '1' + ( menuValue & 0x7 );
    menuAlgorithmName[2] = ' ';
    
    int sel = menuValue;
    if ( sel >= 112 )
        sel = settings.m_favourites[ sel - 112 ];
    if ( sel >= kNumAlgorithmNames )
    {
        menuAlgorithmName[3] = 0;
    }
    else
    {
        strcpy( menuAlgorithmName+3, algorithmNames[sel] );
    }
    
    setMenuString( menuAlgorithmName );
}

void encoderPressed()
{
    // encoder pressed now
    lastSlowTime = slowTime;
    // menu -1 is change current parameter
    activeMenu = -1;
    turnedWhilePressed = 0;
}

void activateMenu()
{
    activeMenu = kMenuLeft;
    menuL = 1;
    menuR = 0;
    menuFlash = 1;
    pushButton = 0;
    setMenuString( menuLStrings[ 0 ] );
}

void deactivateMenu();

void deactivateMenuOnSPress()
{
    unsigned int thisSlowTime;

    // timer 2 for encoder read
    // ( 48MHz / 256 ) / 750 = 250
    // ( 50MHz / 64 ) / 3125 = 250
    OpenTimer2( T2_ON | T2_SOURCE_INT | T2_PS_1_256, 750 );
    INTEnable( INT_T2, INT_ENABLED );

    // wait for push button release
    while ( 1 )
    {
        if ( ( thisSlowTime = slowTime ) != lastSlowTime )
        {
            pushButton = !PORTCbits.RC2;
            if ( !pushButton )
                break;
        }
        lastSlowTime = thisSlowTime;
    }
    // wait for one clock
    lastSlowTime = slowTime;
    while ( slowTime == lastSlowTime )
        ;
    deactivateMenu();
}

void deactivateMenu()
{
    activeMenu = 0;
    lastSlowTime = 0;
    CloseTimer2();
    if ( turnedWhilePressed )
        showPot = kTimeToShowParamNumber/2;
    else
        showSelector();
    resetDimPattern();
}

int menuTick()
{
    int ret = 0;
    int newSelector;

    // read encoder
    int encA = PORTCbits.RC0;
    int encB = PORTCbits.RC1;
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
                menuL = ( menuL + ( encB ? 1 : -1 ) );
                if ( menuL < 1 )
                    menuL = kNumMenuL;
                else if ( menuL > kNumMenuL )
                    menuL = 1;
                setMenuString( menuLStrings[ menuL-1 ] );
                break;
            case kMenuRight:
                // select right menu
                menuR = ( menuR + ( encB ? 1 : -1 ) );
                if ( menuR < 1 )
                    menuR = numMenuR;
                else if ( menuR > numMenuR )
                    menuR = 1;
                setMenuString( menuRStrings[ menuR-1 ] );
                break;
            case kMenuChooseAlgorithm:
                menuValue = menuValue + ( encB ? 1 : -1 );
                if ( menuValue > menuMax )
                    menuValue = menuMin;
                else if ( menuValue < menuMin )
                    menuValue = menuMax;
                setChooseAlgorithmString();
                break;
            case kMenuEditSetting:
            case kMenuChoosePreset:
                menuValue = menuValue + ( encB ? 1 : -1 );
                if ( menuValue > menuMax )
                    menuValue = menuMax;
                else if ( menuValue < menuMin )
                    menuValue = menuMin;
                if ( activeMenu == kMenuEditSetting )
                {
                    settings.m_editable[ editingSetting ] = menuValue;
                    resetDimPattern();
                    if ( editingSetting == kSettingEnableRecall )
                        enableRecallAccordingToSetting();
                }
                break;
        }
        
        updateMenuDisplay();
    }
    lastEncA = encA;

    // read button
    int lastButton = pushButton;
    pushButton = !PORTCbits.RC2;
    if ( pushButton && !lastButton )
    {
        // advance through menus
        activeMenu++;
        switch ( activeMenu )
        {
            case kMenuRight:
                if ( menuL-1 == kMenuSettings )
                {
                    // advance to right menu
                    menuR = 1;
                    STATIC_ASSERT( kNumSettings == sizeof settingsStrings/sizeof settingsStrings[0] );
                    numMenuR = kNumSettings;
                    menuRStrings = settingsStrings;
                    setMenuString( menuRStrings[ menuR-1 ] );
                    break;
                }
                activeMenu = kMenuRight+1;
                // fall through
            case kMenuRight+1:
                // right menu selected
                resetDimPattern();
                switch ( menuL-1 )
                {
                    default:
                        break;
                    case kMenuLAlgorithm:
                        activeMenu = kMenuChooseAlgorithm;
                        menuValue = selector;
                        menuMin = 0;
                        menuMax = 127;
                        setChooseAlgorithmString();
                        break;
                    case kMenuSave:
                    case kMenuLoad:
                        activeMenu = kMenuChoosePreset;
                        menuValue = 0;
                        menuMin = 0;
                        menuMax = kNumPresets-1;
                        displayMode = kDisplayModeMenuValue;
                        break;
                    case kMenuTestSD:
                        testSDCard();
                        break;
                    case kMenuSettings:
                        if ( menuR >= 1 && menuR <= kNumSettings )
                        {
                                STATIC_ASSERT( kNumSettings == kNumEditableSettings );
                                displayMode = kDisplayModeMenuValue;
                                editingSetting = menuR - 1;
                                menuValue = settings.m_editable[ editingSetting ];
                                menuMin = editingSettingRanges[ editingSetting ][ 0 ];
                                menuMax = editingSettingRanges[ editingSetting ][ 1 ];
                                activeMenu = kMenuEditSetting;
                        }
                        break;
                }
                if ( activeMenu == kMenuRight+1 )
                    deactivateMenuOnSPress();
                break;
            case kMenuChooseAlgorithm+1:
                newSelector = menuValue;
                if ( newSelector != selector )
                {
                    selector = newSelector;
                    ret = 1;
                }
                deactivateMenuOnSPress();
                break;
            case kMenuEditSetting+1:
                SaveFavouritesToFlash();
                deactivateMenuOnSPress();
                break;
            case kMenuChoosePreset+1:
                deactivateMenuOnSPress();
                if ( menuL-1 == kMenuSave )
                {
                    pendingSave = 1 + menuValue;
                    ret = 1;
                }
                else
                {
                    ret = loadPreset( menuValue );
                }
                break;
            default:
                deactivateMenuOnSPress();
                break;
        }

        updateMenuDisplay();
    }

    // menu flash
    static unsigned int flashTimer = 0;
    int flash;
    switch ( displayMode )
    {
        default:
        case kDisplayModeMenu:
            ++flashTimer;
            flash = ( flashTimer >> 5 ) & 0x3;
            flash = ( flash == 0 ? 0 : 1 );
            if ( flash != menuFlash )
            {
                menuFlash = flash;
                updateMenuDisplay();
            }
            break;
    }

    return ret;
}

void handleTurnWhilePressed( int encB )
{
    if ( numParameters < 2 )
        return;
    
    int c = currentParameter;
    if ( !encB )
        c -= 1;
    else
        c += 1;
    currentParameter = ( c + numParameters ) % numParameters;
    showPot = kTimeToShowParamNumber;
    showValue( currentParameter, 0 );
}

int processMenu()
{
    int ret = 0;

    unsigned int thisSlowTime;

    if ( ( thisSlowTime = slowTime ) != lastSlowTime )
    {
        // click Z to cancel menu
        if ( !PORTCbits.RC4 )
        {
            deactivateMenu();
            return ret;
        }
        
        if ( activeMenu == -1 )
        {
            // encoder released?
            if ( PORTCbits.RC2 )
            {
                if ( turnedWhilePressed )
                    deactivateMenu();
                else
                    activateMenu();
            }
            else
            {
                // read encoder
                int encA = PORTCbits.RC0;
                int encB = PORTCbits.RC1;
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
                    handleTurnWhilePressed( encB );
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
    
    setColumnsFromSelector();
    displayMode = kDisplayModeAlgorithm;
}

void showSelectorFirstTime()
{
    resetDimPattern();
    showSelector();
}

void    showValue( unsigned int value, int flags )
{
    if ( activeMenu > 0 )
        return;

    displayMode = kDisplayModeAlgorithm;
    
    if ( value < 100 )
        setColumnsFromValue( value, flags );
}

const BYTE bcdTable[100] = {
#include "bcdTable.h"
};

void    setColumnsFromValue( unsigned int value, int flags )
{
    const short* f;
    BYTE b = bcdTable[ value ];
    int d0 = b & 0xf;
    int d1 = b >> 4;
    columns[0] = ( flags & 1 ) ? 4 : 0;
    if ( d1 || ( flags & 2 ) )
    {
        f = font[ ('0'-32)+d1 ];
        columns[1] = f[0];      columns[2] = f[1];      columns[3] = f[2];
    }
    else
    {
        columns[1] = ( flags & 1 ) ? 4 : 0;
        columns[2] = 0;         columns[3] = 0;
    }
    f = font[ ('0'-32)+d0 ];
    columns[4] = f[0];          columns[5] = f[1];      columns[6] = f[2];
}

void openTimerForScroll()
{
    if ( IEC1bits.SPI1TXIE )
        return;
    // ( 48MHz / 64 ) / 10 = 75000
    OpenTimer1( T1_ON | T1_SOURCE_INT | T1_PS_1_64, 10 );
    INTEnable( INT_T1, INT_ENABLED );
}

void startupSequence()
{
    openTimerForScroll();

    dimPattern = dimPatterns[ 7 ];
    scrollMessageOnceAndWait( kVersionString );
    
    closeScrollTimer();
}

void loopUntilMessageScrolled()
{
    unsigned int thisTime, lastTime = time;
    while ( displayMode == kDisplayModeScrollOnce )
    {
        while ( ( thisTime = time ) == lastTime )
            ;
        updateDisplay( 1 );
        lastTime = thisTime;
    }
}

void scrollMessageOnce( const char* msg )
{
    displayMode = kDisplayModeScrollOnce;
    displayScroll = -4;
    displayMessage = msg;
    displayMessagePos = 0;
    scrollSpeed = 5500 - ( ( settings.m_scrollSpeed - 16 ) << 7 );
    scrollCountdown = scrollSpeed;
}

void scrollMessageOnceAndWait( const char* msg )
{
    scrollMessageOnce( msg );
    loopUntilMessageScrolled();
}

void scrollMessageContinuously( const char* msg )
{
    displayMode = kDisplayModeScroll;
    displayScroll = -4;
    displayMessage = msg;
    displayMessagePos = 0;
    scrollSpeed = 5500 - ( ( settings.m_scrollSpeed - 16 ) << 7 );
    scrollCountdown = scrollSpeed;
}

void scrollFilename( const char* msg )
{
    showPot = 0;
    if ( settings.m_continuousFilename )
        scrollMessageContinuously( msg );
    else
        scrollMessageOnce( msg );
}

void updateScroll( int once, int dt )
{
    const short* f;
    unsigned char c0 = displayMessage[displayMessagePos+0];
    unsigned char c1 = displayMessage[displayMessagePos+1];
    unsigned char c2 = displayMessage[displayMessagePos+2];
    if ( once )
    {
        if ( c1 == 0 )
            c2 = 0;
    }
    else
    {
        if ( c0 == 0 )
        {
            c1 = displayMessage[0];
            c2 = displayMessage[1];
        }
        else if ( c1 == 0 )
        {
            c2 = displayMessage[0];
        }
    }
    switch ( displayScroll )
    {
        default:
        case 0:
            f = getFont( c0 );
            columns[0] = f[0];            columns[1] = f[1];            columns[2] = f[2];
            columns[3] = 0;
            f = getFont( c1 );
            columns[4] = f[0];            columns[5] = f[1];            columns[6] = f[2];
            break;
        case 1:
            f = getFont( c0 );
            columns[0] = f[1];            columns[1] = f[2];
            columns[2] = 0;
            f = getFont( c1 );
            columns[3] = f[0];            columns[4] = f[1];            columns[5] = f[2];
            columns[6] = 0;
            break;
        case 2:
            f = getFont( c0 );
            columns[0] = f[2];
            columns[1] = 0;
            f = getFont( c1 );
            columns[2] = f[0];            columns[3] = f[1];            columns[4] = f[2];
            columns[5] = 0;
            f = getFont( c2 );
            columns[6] = f[0];
            break;
        case 3:
            columns[0] = 0;
            f = getFont( c1 );
            columns[1] = f[0];            columns[2] = f[1];            columns[3] = f[2];
            columns[4] = 0;
            f = getFont( c2 );
            columns[5] = f[0];            columns[6] = f[1];
            break;
    }
    displayScroll += 1;
    if ( displayScroll >= 4 )
    {
        displayScroll = 0;
        displayMessagePos += 1;
        if ( once )
        {
            if ( c2 == 0 )
            {
                showSelector();
                displayMode = kDisplayModeAlgorithm;
            }
        }
        else
        {
            if ( c0 == 0 )
            {
                displayMessagePos = 0;
            }
        }
    }
}

void updateDisplay( int dt )
{
    static BYTE p = 0;
    
    if ( displayMode >= kDisplayModeMenu )
    {
        scrollCountdown -= dt;
        if ( scrollCountdown <= 0 )
        {
            scrollCountdown = scrollSpeed;
            updateScroll( displayMode == kDisplayModeScrollOnce, dt );
        }
    }
    
    PORTBCLR = ALL_COLUMNS;
    PORTASET = ALL_ROWS;
    PORTBSET = columns[p];
    asm( "rotr %0, %1, 1\n" : "=d" (dimPattern) : "d" (dimPattern) );
    if ( dimPattern & 1 )
        PORTACLR = matrixRows[ p ];
    int np = p + 1;
    if ( np == 7 )
        p = 0;
    else
        p = np;
}

// Timer 1 interrupt handler
void __ISR(_TIMER_1_VECTOR, IPL1SOFT) Timer1Handler(void)
{
    time++;

    // Clear the interrupt flag
    INTClearFlag( INT_T1 );
}

int loopEndMenuProcess()
{
    int ret = 0;
    /* menu active? */
    if ( activeMenu )
    {
        ret += processMenu();
    }
    /* encoder button */
    else if ( !PORTCbits.RC2 )
    {
        encoderPressed();
    }
    else
    {
        /* encoder turned? */               
        unsigned int thisSlowTime;          
        if ( ( thisSlowTime = slowTime ) != lastSlowTime )  
        {                                       
            int encA = PORTCbits.RC0;           
            int encB = PORTCbits.RC1;           
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
                handleEncoderTurn( encB );
            }                                   
            lastEncA = encA;                    
        }                                       
        lastSlowTime = thisSlowTime;            
    }                       

    if ( !potMIDIOverride )
        pot = ReadADC10(0); /* Read the result of pot conversion */
    ConvertADC10();     /* start the next conversion */

    return ret;
}
