
#ifndef PRAGMA_CONFIGS_H
#define	PRAGMA_CONFIGS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define SYS_FREQ 	(48000000L)

// Configuration Bit settings
// SYSCLK = 48 MHz (8MHz Crystal / FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 48 MHz (SYSCLK / FPBDIV)
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// Other options are don't care
#pragma config FPLLMUL = MUL_24, FPLLIDIV = DIV_2, FPLLODIV = DIV_2
#pragma config POSCMOD = XT, FNOSC = PRIPLL, FPBDIV = DIV_1, FSOSCEN = OFF

#pragma config JTAGEN = OFF

// WDT OFF, non-windowed, 1:1024 postscale
#pragma config FWDTEN = OFF, WINDIS = OFF, WDTPS = PS1024

#pragma config CP = OFF, PWP = OFF, BWP = OFF

#ifdef	__cplusplus
}
#endif

#endif	/* PRAGMA_CONFIGS_H */
