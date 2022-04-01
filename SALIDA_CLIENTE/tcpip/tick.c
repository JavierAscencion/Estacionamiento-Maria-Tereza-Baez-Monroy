/*********************************************************************
 *
 *                  Tick Manager for PIC18
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        Tick.c
 * Dependencies:    stackTSK.h
 *                  Tick.h
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
 * Darren Rook (CCS)    01/16/04 Intermediate counter vars added
 * R. Shelquist (CCS)   09/23/04 TickGet() temporarily disables Timer0 interrupt
 ********************************************************************/

#define TICK_INCLUDE

#include "tcpip/stacktsk.h"
#include "tcpip/tick.h"


TICKTYPE TickCount = 0;  //increment every 100ms


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
void TickInit(void)
{
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_16);
    set_timer0(TICK_COUNTER);

    enable_interrupts(INT_TIMER0);
    enable_interrupts(GLOBAL);
}


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
TICKTYPE TickGet(void)
{
    int16 ret;
    disable_interrupts(INT_TIMER0);
    ret=TickCount;
    enable_interrupts(INT_TIMER0);
    return ret;
}

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
 ********************************************************************/
int8 second_counter=0;  //increment this every 1s
int8 second_counter_intermediate=0;

#if STACK_USE_PPP
 int8 ppp_second_counter=0;
 int8 ppp_second_counter_intermediate=0;
#endif

#int_timer0
void Tick_Isr(void)
{
        TickCount++;    //increment this every 100ms

        second_counter_intermediate++;
        if (second_counter_intermediate >= TICKS_PER_SECOND) {
            second_counter++; //increment this ever 1s
            second_counter_intermediate=0;
        }


       #if STACK_USE_PPP
        ppp_second_counter_intermediate++;
        if (ppp_second_counter_intermediate >= TICKS_PER_SECOND) {
            ppp_second_counter_intermediate=0;
            ppp_second_counter++;
        }
       #endif


    set_timer0(TICK_COUNTER); //set timer0 to properly interrupt every 100ms
}







