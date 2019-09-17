/* 
 * File:   myBoardConfigFall2016.h
 * Author: plassmann
 *
 * Created on September 19, 2016, 5:20 PM
 */

#ifndef MYBOARDCONFIGFALL2016_H
#define	MYBOARDCONFIGFALL2016_H

#ifdef	__cplusplus
extern "C" {
#endif

// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	    // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	    // PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	    // PLL output divider
#pragma config FPBDIV       = DIV_8	    // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	    // Secondary oscillator enable


#ifdef	__cplusplus
}
#endif

#endif	/* MYBOARDCONFIGFALL2016_H */

