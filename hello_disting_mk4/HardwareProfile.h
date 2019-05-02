
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

//#define DEMO_BOARD USER_DEFINED_BOARD

#if !defined(DEMO_BOARD)
    #if defined(__C30__) || defined __XC16__
        #if defined(__PIC24FJ256GB110__)
            #include "HardwareProfile - PIC24FJ256GB110 PIM.h"
        #elif defined(__PIC24FJ256GB210__)
            #include "HardwareProfile - PIC24FJ256GB210 PIM.h"
        #elif defined(__PIC24FJ256GB106__)
            #include "HardwareProfile - PIC24F Starter Kit.h"
        #elif defined(__PIC24FJ64GB004__)
            #include "HardwareProfile - PIC24FJ64GB004 PIM.h"
        #elif defined(__PIC24FJ256DA210__)
            #include "HardwareProfile - PIC24FJ256DA210 Development Board.h"
        #endif
    #endif
#endif

#if !defined(DEMO_BOARD)
    #warning "Demo board not defined.  Either define DEMO_BOARD for a custom board or select the correct processor for the demo board."
    #include "HardwareProfile - Your Demo Board.h"
#endif

    #if defined (__PIC32MX__)

        // Registers for the SPI module you want to use
        //#define MDD_USE_SPI_1
        #define MDD_USE_SPI_2

        //SPI Configuration
        #define SPI_START_CFG_1     (PRI_PRESCAL_64_1 | SEC_PRESCAL_8_1 | MASTER_ENABLE_ON | SPI_CKE_ON | SPI_SMP_ON)
        #define SPI_START_CFG_2     (SPI_ENABLE)

        // Define the SPI frequency
        #define SPI_FREQUENCY			(24000000)

        #if defined MDD_USE_SPI_1
            // Description: SD-SPI Chip Select Output bit
            #define SD_CS               LATBbits.LATB1
            // Description: SD-SPI Chip Select TRIS bit
            #define SD_CS_TRIS          TRISBbits.TRISB1

            // Description: SD-SPI Card Detect Input bit
            #define SD_CD               PORTFbits.RF0
            // Description: SD-SPI Card Detect TRIS bit
            #define SD_CD_TRIS          TRISFbits.TRISF0

            // Description: SD-SPI Write Protect Check Input bit
            #define SD_WE               PORTFbits.RF1
            // Description: SD-SPI Write Protect Check TRIS bit
            #define SD_WE_TRIS          TRISFbits.TRISF1

            // Description: The main SPI control register
            #define SPICON1             SPI1CON
            // Description: The SPI status register
            #define SPISTAT             SPI1STAT
            // Description: The SPI Buffer
            #define SPIBUF              SPI1BUF
            // Description: The receive buffer full bit in the SPI status register
            #define SPISTAT_RBF         SPI1STATbits.SPIRBF
            // Description: The bitwise define for the SPI control register (i.e. _____bits)
            #define SPICON1bits         SPI1CONbits
            // Description: The bitwise define for the SPI status register (i.e. _____bits)
            #define SPISTATbits         SPI1STATbits
            // Description: The enable bit for the SPI module
            #define SPIENABLE           SPICON1bits.ON
            // Description: The definition for the SPI baud rate generator register (PIC32)
            #define SPIBRG			    SPI1BRG

            // Tris pins for SCK/SDI/SDO lines

            // Tris pins for SCK/SDI/SDO lines
            #if defined(__32MX460F512L__)
            	// Description: The TRIS bit for the SCK pin
            	#define SPICLOCK            TRISDbits.TRISD10
            	// Description: The TRIS bit for the SDI pin
            	#define SPIIN               TRISCbits.TRISC4
            	// Description: The TRIS bit for the SDO pin
            	#define SPIOUT              TRISDbits.TRISD0
            #else	// example: PIC32MX360F512L
            	// Description: The TRIS bit for the SCK pin
            	#define SPICLOCK            TRISFbits.TRISF6
            	// Description: The TRIS bit for the SDI pin
            	#define SPIIN               TRISFbits.TRISF7
            	// Description: The TRIS bit for the SDO pin
            	#define SPIOUT              TRISFbits.TRISF8
            #endif

            //SPI library functions
            #define putcSPI             putcSPI1
            #define getcSPI             getcSPI1
            #define OpenSPI(config1, config2)   OpenSPI1(config1, config2)

        #elif defined MDD_USE_SPI_2
            // Description: SD-SPI Chip Select Output bit
            #define SD_CS               LATBbits.LATB7
            // Description: SD-SPI Chip Select TRIS bit
            #define SD_CS_TRIS          TRISBbits.TRISB7

            // Description: SD-SPI Card Detect Input bit
            // Not used for MicroSD
            #define SD_CD               error
            // Description: SD-SPI Card Detect TRIS bit
            // Not used for MicroSD
            extern int dummy;
            #define SD_CD_TRIS          dummy

            // Description: SD-SPI Write Protect Check Input bit
            // Not used for MicroSD
            #define SD_WE               FALSE
            // Description: SD-SPI Write Protect Check TRIS bit
            // Not used for MicroSD
            #define SD_WE_TRIS          dummy

            // Description: The main SPI control register
            #define SPICON1             SPI2CON
            // Description: The SPI status register
            #define SPISTAT             SPI2STAT
            // Description: The SPI Buffer
            #define SPIBUF              SPI2BUF
            // Description: The receive buffer full bit in the SPI status register
            #define SPISTAT_RBF         SPI2STATbits.SPIRBF
            // Description: The bitwise define for the SPI control register (i.e. _____bits)
            #define SPICON1bits         SPI2CONbits
            // Description: The bitwise define for the SPI status register (i.e. _____bits)
            #define SPISTATbits         SPI2STATbits
            // Description: The enable bit for the SPI module
            #define SPIENABLE           SPI2CONbits.ON
            // Description: The definition for the SPI baud rate generator register (PIC32)
            #define SPIBRG		SPI2BRG

            // Tris pins for SCK/SDI/SDO lines

            // Description: The TRIS bit for the SCK pin
            #define SPICLOCK            TRISBbits.TRISB15
            // Description: The TRIS bit for the SDI pin
            #define SPIIN               TRISBbits.TRISB6
            // Description: The TRIS bit for the SDO pin
            #define SPIOUT              TRISBbits.TRISB5
            //SPI library functions
            #define putcSPI             putcSPI2
            #define getcSPI             getcSPI2
            #define OpenSPI(config1, config2)   OpenSPI2(config1, config2)
        #endif

    #endif

#define GetSystemClock()            (48000000L)
#define GetPeripheralClock()        GetSystemClock()
#define GetInstructionClock()       (GetSystemClock() / 2)

#endif  //HARDWARE_PROFILE_H
