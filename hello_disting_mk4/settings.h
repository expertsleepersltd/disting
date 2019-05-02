#ifndef _SETTINGS_H
#define _SETTINGS_H

enum { kNumEditableSettings = 9 };

typedef struct Settings {

    UINT32          m_version;

    BYTE            m_favourites[16];

    union {
        struct {
            BYTE    m_brightness;
            BYTE    m_enableRecall;
            BYTE    m_midiChIn;
            BYTE    m_midiChOut;
            BYTE    m_midiThru;
            BYTE    m_midiPgmOption;
            BYTE    m_continuousFilename;
            BYTE    m_scrollSpeed;
            BYTE    m_sdSampleRate;
        };
        BYTE        m_editable[kNumEditableSettings];
    };

} Settings;

extern Settings settings;

extern const char editingSettingRanges[][3];

#endif /* _SETTINGS_H */
