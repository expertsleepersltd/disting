
#ifndef _FS_DEF_

#include "HardwareProfile.h"

// The FS_MAX_FILES_OPEN #define is only applicable when Dynamic
// memeory allocation is not used (FS_DYNAMIC_MEM not defined).
// Defines how many concurent open files can exist at the same time.
// Takes up static memory. If you do not need to open more than one
// file at the same time, then you should set this to 1 to reduce
// memory usage
#define FS_MAX_FILES_OPEN 	2
/************************************************************************/

// long file names
#define SUPPORT_LFN

// The size of a sector
// Must be 512, 1024, 2048, or 4096
// 512 bytes is the value used by most cards
#define MEDIA_SECTOR_SIZE 		512
/************************************************************************/

// Determines processor type
#define USE_PIC32

// Defines the device type
#include "MDD File System/SD-SPI.h"

/*************************************************************************/
/*Compiler options to enable/Disable Features based on user's application*/
/*************************************************************************/

// Uncomment this to use the FindFirst, FindNext, and FindPrev
//#define ALLOW_FILESEARCH
/************************************************************************/
/************************************************************************/

// Comment this line out if you don't intend to write data to the card
#define ALLOW_WRITES
/************************************************************************/

// Comment this line out if you don't intend to format your card
// Writes must be enabled to use the format function
//#define ALLOW_FORMATS
/************************************************************************/

// Uncomment this definition if you're using directories
// Writes must be enabled to use directories
#define ALLOW_DIRS
/************************************************************************/

// Allows the use of FSfopenpgm, FSremovepgm, etc with PIC18
//#define ALLOW_PGMFUNCTIONS
/************************************************************************/

// Allows the use of the FSfprintf function
// Writes must be enabled to use the FSprintf function
//#define ALLOW_FSFPRINTF
/************************************************************************/

// If FAT32 support required then uncomment the following
#define SUPPORT_FAT32
/* **********************************************************************/


// Select how you want the timestamps to be updated
// Use the Real-time clock peripheral to set the clock
// You must configure the RTC in your application code
//#define USEREALTIMECLOCK
// The user will update the timing variables manually using the SetClockVars function
// The user should set the clock before they create a file or directory (Create time),
// and before they close a file (last access time, last modified time)
#define USERDEFINEDCLOCK
// Just increment the time- this will not produce accurate times and dates
//#define INCREMENTTIMESTAMP

// Warnings
#ifndef USEREALTIMECLOCK
    #ifndef USERDEFINEDCLOCK
        #ifndef INCREMENTTIMESTAMP
            #error Please enable USEREALTIMECLOCK, USERDEFINEDCLOCK, or INCREMENTTIMESTAMP
        #endif
    #endif
#endif


// Function definitions
#define MDD_MediaInitialize     MDD_SDSPI_MediaInitialize
#define MDD_MediaDetect         MDD_SDSPI_MediaDetect
#define MDD_SectorRead          MDD_SDSPI_SectorRead
#define MDD_SectorWrite         MDD_SDSPI_SectorWrite
#define MDD_InitIO              MDD_SDSPI_InitIO
#define MDD_ShutdownMedia       MDD_SDSPI_MediaReset
#define MDD_WriteProtectState   MDD_SDSPI_WriteProtectState
#define MDD_ReadCapacity        MDD_SDSPI_ReadCapacity
#define MDD_ReadSectorSize      MDD_SDSPI_ReadSectorSize

#endif
