
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <plib.h>

#include "constants.h"
#include "presets.h"

char pageBuffer[ BYTE_PAGE_SIZE ];

enum
{
    kPresetVersion1 = 1,

    kPresetVersionCurrent = kPresetVersion1,
};

char pendingParameterLoad = 0;

void savePreset( unsigned int w )
{
    STATIC_ASSERT( ( sizeof(Preset) * kNumPresets ) == BYTE_PAGE_SIZE );

    if ( w >= kNumPresets )
        return;
    
    Preset* buffer = (Preset*)pageBuffer;
    memcpy( buffer, (void*)PRESETS_FLASH_ADDR, BYTE_PAGE_SIZE );
    
    Preset* preset = buffer + w;
    memset( preset, 0, sizeof(Preset) );

    preset->m_version = kPresetVersionCurrent;
    // preset zero stores actual selector so if using favourites, after power cycle still in favourites
    preset->m_selector = w ? actuallyRunningSelector : selector;
    int i;
    for ( i=0; i<numParameters; ++i )
        preset->m_parameters[i] = parameters[i];

    if ( memcmp( buffer, (void*)PRESETS_FLASH_ADDR, BYTE_PAGE_SIZE ) == 0 )
        return;
    
	DWORD* ptr = (DWORD*)PRESETS_FLASH_ADDR;
	const DWORD* src = (const DWORD*)buffer;

    NVMErasePage( ptr );

    for ( i=0; i<BYTE_PAGE_SIZE/sizeof(DWORD); ++i )
    {
        NVMWriteWord( ptr, *src );
        ptr++;
        src++;
    }
    
    memset( pageBuffer, 0, sizeof pageBuffer );
}

void handlePendingSave()
{
    savePreset( pendingSave-1 );
    retIsFromSave = 1;
    pendingSave = 0;
}

void loadParametersFromPreset( unsigned int w )
{
    const Preset* preset = ( (Preset*)PRESETS_FLASH_ADDR ) + w;
    if ( preset->m_version > kPresetVersionCurrent )
        return;

    int i;
    for ( i=0; i<numParameters; ++i )
    {
        int p = preset->m_parameters[i];
        if ( p < parameterMin( i ) )
            p = parameterMin( i );
        else if ( p > parameterMax( i ) )
            p = parameterMax( i );
        parameters[i] = p;
    }
}

int loadPreset( unsigned int w )
{
    if ( w >= kNumPresets )
        return 0;

    const Preset* preset = ( (Preset*)PRESETS_FLASH_ADDR ) + w;
    if ( preset->m_version > kPresetVersionCurrent )
        return 0;
    unsigned int s = preset->m_selector;
    if ( s >= 128 )
        return 0;

    int ret = ( s != actuallyRunningSelector );
    if ( ret )
    {
        selector = s;
        pendingParameterLoad = w;
    }
    else
        loadParametersFromPreset( w );
    
    return ret;
}

int selectorFromPreset( unsigned int w )
{
    if ( w >= kNumPresets )
        return 0;
    const Preset* preset = ( (Preset*)PRESETS_FLASH_ADDR ) + w;
    if ( preset->m_version > kPresetVersionCurrent )
        return 0;
    unsigned int s = preset->m_selector;
    if ( s >= 128 )
        return 0;
    return s;
}
