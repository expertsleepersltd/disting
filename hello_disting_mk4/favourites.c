
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "constants.h"
#include "settings.h"

enum
{
    kSettingsVersion1 = 1,
    kSettingsVersion2,          // remove auto-store
    kSettingsVersion3,          // added continuous filename scroll & scroll speed

    kSettingsVersionCurrent = kSettingsVersion3,
};

// min, max, default
const char editingSettingRanges[][3] = {
    { 1, 16, 8 },      // brightness
    { 0, 1, 0 },       // enable recall
    { 1, 16, 1 },      // MIDI ch in
    { 1, 16, 1 },      // MIDI ch out
    { 0, 1, 0 },       // MIDI thru
    { 0, 1, 0 },       // MIDI program change option
    { 0, 1, 0 },       // continuous filename scroll
    { 0, 32, 16 },     // scroll speed
    { 0, 1, 0 },       // SD playback sample rate
};

Settings settings;

void ResetSettings()
{
    STATIC_ASSERT( sizeof editingSettingRanges/sizeof editingSettingRanges[0] == kNumEditableSettings );

    memset( &settings, 0, sizeof(Settings) );
    int i;
    for ( i=0; i<kNumEditableSettings; ++i )
    {
        settings.m_editable[i] = editingSettingRanges[i][2];
    }
}

void ReadFavouritesFromFlash()
{
    STATIC_ASSERT( sizeof(Settings) <= 0x20 );
    
    memcpy( &settings, (const void*)(SETTINGS_FLASH_ADDR), sizeof(Settings) );

    if ( settings.m_version > kSettingsVersionCurrent )
    {
        ResetSettings();
    }
    
    int i;
    for ( i=0; i<kNumEditableSettings; ++i )
    {
        if ( settings.m_editable[i] < editingSettingRanges[i][0] || settings.m_editable[i] > editingSettingRanges[i][1] )
            settings.m_editable[i] = editingSettingRanges[i][2];
    }
}

void SaveFavouritesToFlash()
{
    settings.m_version = kSettingsVersionCurrent;

    if ( 0 == memcmp( &settings, (const void*)(SETTINGS_FLASH_ADDR), sizeof(Settings) ) )
        return;
    
    char* buffer = pageBuffer;

    memcpy( buffer, (const void*)SETTINGS_FLASH_ADDR, BYTE_PAGE_SIZE );
    memcpy( buffer, &settings, sizeof(Settings) );
    
    WriteSettingsPage( buffer );
}

void WriteSettingsPage( const char* buffer )
{
    NVMErasePage( (void*)SETTINGS_FLASH_ADDR );

    int* ptr = (int*)SETTINGS_FLASH_ADDR;
    const int* src = (const int*)buffer;
    int i;
    for ( i=0; i<PAGE_SIZE; ++i )
        NVMWriteWord( ptr++, *src++ );
}
