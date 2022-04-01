/*********************************************************************
 *
 *                  DHCP Defs for Microchip TCP/IP Stack
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        DHCP.h
 * Dependencies:    StackTsk.h
 *                  UDP.h
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
 * Nilesh Rajbharti     3/21/01  Original        (Rev 1.0)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    06/29/04 Added DHCPIsDisabled() macro, returns true if DHCP is disabled
 * Darren Rook (CCS)    07/13/06 Changed DHCPIsDisabled() to use AppConfig instead of smDHCPState
 * Darren Rook (CCS)    08/25/06 In synch with Microchip's V3.60 stack (no changes)
 * Darren Rook (CCS)    10/24/06 In synch with Microchips's V3.75 stack
 ********************************************************************/
#ifndef DHCP_H
#define DHCP_H

#include "tcpip/stacktsk.h"


typedef enum _SM_DHCP
{
	SM_DHCP_INIT_FIRST_TIME,
    SM_DHCP_INIT,
    SM_DHCP_RESET_WAIT,
    SM_DHCP_BROADCAST,
    SM_DHCP_DISCOVER,
    SM_DHCP_REQUEST,
    SM_DHCP_BIND,
    SM_DHCP_BOUND,
    SM_DHCP_DISABLED,
} SM_DHCP;

SM_DHCP smDHCPState = SM_DHCP_INIT_FIRST_TIME;

/*********************************************************************
 * Macro:           void DHCPDisable(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Puts DHCPTask into unhandled state "SM_DHCP_DISABLED"
 *                  and hence DHCP is effictively disabled.
 *
 * Note:            This macro should be called before DHCPTask is called
 *                  or else a UDP port will be kept open and there will
 *                  be no task to process it.
 ********************************************************************/
#define DHCPDisable()       (smDHCPState = SM_DHCP_DISABLED)

#define DHCPIsDisabled()      (smDHCPState == SM_DHCP_DISABLED)


/*********************************************************************
 * Function:        void DHCPTask(void)
 *
 * PreCondition:    DHCPInit() is already called AND
 *                  IPGetHeader() is called with
 *                  IPFrameType == IP_PROT_UDP
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Fetches pending UDP packet from MAC receive buffer
 *                  and dispatches it appropriate UDP socket.
 *                  If not UDP socket is matched, UDP packet is
 *                  silently discarded.
 *
 * Note:            Caller must make sure that MAC receive buffer
 *                  access pointer is set to begining of UDP packet.
 *                  Required steps before calling this function is:
 *
 *                  If ( MACIsRxReady() )
 *                  {
 *                      MACGetHeader()
 *                      If MACFrameType == IP
 *                          IPGetHeader()
 *                          if ( IPFrameType == IP_PROT_UDP )
 *                              Call DHCPTask()
 *                  ...
 ********************************************************************/
void DHCPTask(void);

/*********************************************************************
 * Macro:           BOOL DHCPIsBound(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE if DHCP is bound to given configuration
 *                  FALSE if DHCP has yet to be bound.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:
 ********************************************************************/
#define DHCPIsBound()       (DHCPState.bits.bIsBound)

typedef union _DHCP_STATE
{
    struct
    {
        unsigned char bIsBound : 1;
        unsigned char bOfferReceived : 1;
    } bits;
    BYTE Val;
} DHCP_STATE;


static DHCP_STATE DHCPState;

/*********************************************************************
 * Macro:           void DHCPReset(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Closes any previously opened DHCP socket
 *                  and resets DHCP state machine so that on next
 *                  call to DHCPTask will result in new DHCP request.
 *
 * Note:            None
 ********************************************************************/
void DHCPReset(void);


#endif
