
#ifndef _PRESETS_H    /* Guard against multiple inclusion */
#define _PRESETS_H

#define kMaxParameters (8)

#define kNumPresets (64)

typedef struct
{
    BYTE        m_version;
    BYTE        m_selector;
   
    char        m_parameters[kMaxParameters];
    
    BYTE        m_unused[6];
} Preset;

extern char pendingParameterLoad;
extern char pendingSave;

extern void savePreset( unsigned int w );
extern void handlePendingSave();
extern int loadPreset( unsigned int w );
extern int selectorFromPreset( unsigned int w );

#endif /* _PRESETS_H */
