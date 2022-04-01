/*********************************************************************
 *
 *                  Tick Manager for PIC18
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        Tick.h
 * Dependencies:    None
 * Processor:       PIC18
 * Complier:        CCS PCH 3.181 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date     Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     6/28/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * R. Shelquist (CCS)   09/24/04 TickGetDiff fixed so it isn't off by 1 if a<b
 ********************************************************************/

#ifndef TICK_H
#define TICK_H

typedef int16 TICKTYPE;

/*
 * This value is used by TCP to implement timeout actions.
 */
#define TICKS_PER_SECOND               12 // 10 original value (MiE)

#if (TICKS_PER_SECOND < 10 || TICKS_PER_SECOND > 255)
#error Invalid TICKS_PER_SECONDS specified.
#endif

//16 is the prescalar
#define CYCLES_PER_TICKS_PER_SECOND getenv("CLOCK")/(4 * 16 * TICKS_PER_SECOND)

#if (CYCLES_PER_TICKS_PER_SECOND > 0xFFFF)
 #error Reduce Clock Speed or Increase TICKS_PER_SECOND
#endif

#if (CYCLES_PER_TICKS_PER_SECOND <= 100)
 #error Timer0 not accurate enough.  Raise Clock Speed or Reduce TICKS_PER_SECOND
#endif


#define TICK_COUNTER 0xFFFF-CYCLES_PER_TICKS_PER_SECOND

/*
#if (getenv("CLOCK")==20000000)
   #define TICK_COUNTER 34250
#elif (getenv("CLOCK")==40000000)
   #define TICK_COUNTER 3035
#elif (getenv("CLOCK")==19600000)
   #define TICK_COUNTER 34910
#else
 #error CALCULATE THIS CLOCK SPEED (INCREMENT TICKCOUNT EVERY 100MS).  SEE TickInit()
#endif
*/

#define TICK_SECOND  TICKS_PER_SECOND


#define TickGetDiff(a, b)       ((TICKTYPE)(a < b) ? (((TICKTYPE)0xffff - b) + a + 1) : (a - b))


/*********************************************************************
 * Function:        void TickInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Tick manager is initialized.
 *
 * Side Effects:    None
 *
 * Overview:        Initializes Timer0 as a tick counter.
 *
 * Note:            None
 ********************************************************************/
void TickInit(void);



/*********************************************************************
 * Function:        TICK TickGet(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Current second value is given
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
TICKTYPE TickGet(void);


/*********************************************************************
 * Function:        void TickUpdate(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Internal Tick and Seconds count are updated.
 *
 * Note:            None
 *
 * CCS NOTE:        CCS has removed TickUpdate and replaced it with an interrupt.
 *                  You do not have to call TickUpdate() at a regular interval.
 ********************************************************************/
//void TickUpdate(void);



#endif
