// File:   myDebug.h
//
// A set of Preprocessor Macros that can be used to "instrument" or
// debug your code with a logic analyzer or oscilloscope. 
// The Macros use the pins on header JB, pins 1 through 8 which are
// connected to the GPIO PORT E.
//
// You can connect your logic analyzer or oscilloscope to these pins
// to see when "events" in your code happen. This timing should can
// be cycle accurate if your device can see 10 nsec transitions (note 
// that the PIC32 clock for our projects is running at 80MHz).
//  
// You can use the macros a couple of different ways.
//
// Way 1: Turn the event signal on at the beginning of an event
// and off at the end of the event to see when the event occurs and
// to measure the time the event takes. For example to measure the 
// the a function call takes, you could do the following:
//     DBG_ON(MASK_DBG1);
//     myFunctionCall();
//     DBG_OFF(MASK_DBG1);
// This would show when the instruction right before the function call
// occurred and the instruction right after the function call returned.
//
// Way 2: You can just mark events. (NB: This approach will not work for
// an oscilloscope.) For example, I could define events in this file like:
//   #define ISR_TIMER3_EVENT 0x23
//   #define ISR_SPI1_ENABLE 0x24
// And just do:
//   DBG_ON(ISR_TIMER3_EVENT);
// and
//   DBG_ON(ISR_SPI1_ENABLE);
// at the appropriate points in your code. On the logic analyzer you would
// be able to see exactly when (and the order) of these events. (You normally
// do this by reading these lines as a digital bus and decoding the bus state
// as a hex value so that you can easily read the event numbers).
//
// 9/20/2016 PEP


#ifndef MYDEBUG_H
#define	MYDEBUG_H

#ifdef	__cplusplus
extern "C" {
#endif

// intrumentation for the logic analyzer (or oscilliscope)
#define MASK_DBG0  0x01
#define MASK_DBG1  0x02
#define MASK_DBG2  0x04
#define MASK_DBG3  0x08
#define MASK_DBG4  0x10
#define MASK_DBG5  0x20
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0xFF

#ifdef	__cplusplus
}
#endif

#endif	/* MYDEBUG_H */

