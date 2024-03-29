/*********************************************************************
 *
 *                  ICMP Module for Microchip TCP/IP Stack
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        ICMP.C
 * Dependencies:    ICMP.h
 *                  string.h
 *                  StackTsk.h
 *                  Helpers.h
 *                  IP.h
 *                  MAC.h
 * Processor:       PIC18
 * Complier:        CCS PCH 3.181 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the �Company�) for its PICmicro� Microcontroller is intended and
 * supplied to you, the Company�s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date     Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     4/30/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    06/29/04 SwapICMPPacket() no longer static
 * Howard Schlunder      9/9/04   Added ENC28J60 DMA checksum support
 * Howard Schlunder      1/5/06   Increased DMA checksum efficiency
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack
 * Darren Rook (CCS)    10/24/06 In synch with Microchip's V3.75 stack
 ********************************************************************/

#include <string.h>
#include "tcpip/stacktsk.h"
#include "tcpip/helpers.h"
#include "tcpip/icmp.h"
#include "tcpip/ip.h"

//#define debug_icmp
//#define debug_icmp   debug_printf
#define debug_icmp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)

//#define MAX_ICMP_DATA       32 //moved to icmp.h

/*
 * ICMP packet definition
 */
typedef struct _ICMP_PACKET
{
    BYTE    Type;
    BYTE    Code;
    WORD    Checksum;
    WORD    Identifier;
    WORD    SequenceNumber;
    BYTE    Data[MAX_ICMP_DATA];
} ICMP_PACKET;
#define ICMP_HEADER_SIZE    (sizeof(ICMP_PACKET) - MAX_ICMP_DATA)

static void SwapICMPPacket(ICMP_PACKET* p);


/*********************************************************************
 * Function:        BOOL ICMPGet(ICMP_CODE *code,
 *                              BYTE *data,
 *                              BYTE *len,
 *                              WORD *id,
 *                              WORD *seq)
 *
 * PreCondition:    MAC buffer contains ICMP type packet.
 *
 * Input:           code    - Buffer to hold ICMP code value
 *                  data    - Buffer to hold ICMP data
 *                  len     - Buffer to hold ICMP data length
 *                  id      - Buffer to hold ICMP id
 *                  seq     - Buffer to hold ICMP seq
 *
 * Output:          TRUE if valid ICMP packet was received
 *                  FALSE otherwise.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL ICMPGet(ICMP_CODE *code,
             BYTE *data,
             BYTE *len,
             WORD *id,
             WORD *seq)
{
    ICMP_PACKET packet;
    WORD CalcChecksum;
    WORD ReceivedChecksum;
#if !defined(MCHP_MAC)
    WORD checksums[2];
#endif

    debug_icmp(debug_putc, "\r\nICMP GET ");

    // Obtain the ICMP Header
    MACGetArray((BYTE*)&packet, ICMP_HEADER_SIZE);


#if defined(MCHP_MAC)
   // Calculate the checksum using the Microchip MAC's DMA module
   // The checksum data includes the precomputed checksum in the
   // header, so a valid packet will always have a checksum of
   // 0x0000 if the packet is not disturbed.
   ReceivedChecksum = 0x0000;
   CalcChecksum = MACCalcRxChecksum(0+sizeof(IP_HEADER), *len);
#endif

   // Obtain the ICMP data payload
    *len -= ICMP_HEADER_SIZE;
    MACGetArray(data, *len);


#if !defined(MCHP_MAC)
   // Calculte the checksum in local memory without hardware help
    ReceivedChecksum = packet.Checksum;
    packet.Checksum = 0;

    checksums[0] = ~CalcIPChecksum((BYTE*)&packet, ICMP_HEADER_SIZE);
    checksums[1] = ~CalcIPChecksum(data, *len);

    CalcChecksum = CalcIPChecksum((BYTE*)checksums, 2 * sizeof(WORD));
#endif

    SwapICMPPacket(&packet);

    *code = packet.Type;
    *id = packet.Identifier;
    *seq = packet.SequenceNumber;

    debug_icmp(debug_putc, "%U", CalcChecksum == ReceivedChecksum);

    return ( CalcChecksum == ReceivedChecksum );
}

/*********************************************************************
 * Function:        void ICMPPut(NODE_INFO *remote,
 *                               ICMP_CODE code,
 *                               BYTE *data,
 *                               BYTE len,
 *                               WORD id,
 *                               WORD seq)
 *
 * PreCondition:    ICMPIsTxReady() == TRUE
 *
 * Input:           remote      - Remote node info
 *                  code        - ICMP_ECHO_REPLY or ICMP_ECHO_REQUEST
 *                  data        - Data bytes
 *                  len         - Number of bytes to send
 *                  id          - ICMP identifier
 *                  seq         - ICMP sequence number
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Note:            A ICMP packet is created and put on MAC.
 *
 ********************************************************************/
void ICMPPut(NODE_INFO *remote,
             ICMP_CODE code,
             BYTE *data,
             BYTE len,
             WORD id,
             WORD seq)
{
    ICMP_PACKET   packet;
    WORD ICMPLen;
   BUFFER MyTxBuffer;
   MyTxBuffer = MACGetTxBuffer(TRUE);

   // Abort if there is no where in the Ethernet controller to
   // store this packet.
   if(MyTxBuffer == INVALID_BUFFER)
      return;

   IPSetTxBuffer(MyTxBuffer, 0);


   ICMPLen = ICMP_HEADER_SIZE + (WORD)len;

    packet.Code             = 0;
    packet.Type             = code;
    packet.Checksum         = 0;
    packet.Identifier       = id;
    packet.SequenceNumber   = seq;

    memcpy((void*)packet.Data, (void*)data, len);

    SwapICMPPacket(&packet);

#if !defined(MCHP_MAC)
    packet.Checksum         = CalcIPChecksum((BYTE*)&packet,
                                    ICMPLen);
#endif

    IPPutHeader(remote,
                IP_PROT_ICMP,
                (WORD)(ICMP_HEADER_SIZE + len));

    IPPutArray((BYTE*)&packet, ICMPLen);

#if defined(MCHP_MAC)
    // Calculate and write the ICMP checksum using the Microchip MAC's DMA
   packet.Checksum = MACCalcTxChecksum(sizeof(IP_HEADER), ICMPLen);
   IPSetTxBuffer(MyTxBuffer, 2);
   MACPutArray((BYTE*)&packet.Checksum, 2);
#endif


    MACFlush();
}

/*********************************************************************
 * Function:        void SwapICMPPacket(ICMP_PACKET* p)
 *
 * PreCondition:    None
 *
 * Input:           p - ICMP packet header
 *
 * Output:          ICMP packet is swapped
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void SwapICMPPacket(ICMP_PACKET* p)
{
    p->Identifier           = swaps(p->Identifier);
    p->SequenceNumber       = swaps(p->SequenceNumber);
    p->Checksum             = swaps(p->Checksum);
}
