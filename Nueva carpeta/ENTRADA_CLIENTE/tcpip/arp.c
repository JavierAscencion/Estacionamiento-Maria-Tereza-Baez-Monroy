/*********************************************************************
 *
 *                  ARP Module for Microchip TCP/IP Stack
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        ARP.c
 * Dependencies:    string.h
 *                  stacktsk.h
 *                  helpers.h
 *                  arp.h
 *                  mac.h
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
 * Nilesh Rajbharti     5/1/01   Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack [no changes]
 * Darren Rook (CCS)    10/24/06 In synch with Microchips's V3.75 stack
 ********************************************************************/
#include <string.h>

#include "tcpip/stacktsk.h"
#include "tcpip/helpers.h"
#include "tcpip/arp.h"
#include "tcpip/mac.h"

//#define debug_arp
//#define debug_arp debug_printf
#define debug_arp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)

// ARP Operation codes.
#define ARP_OPERATION_REQ       0x01u
#define ARP_OPERATION_RESP      0x02u

// ETHERNET packet type as defined by IEEE 802.3
#define HW_ETHERNET             (0x0001u)
#define ARP_IP                  (0x0800u)



// ARP packet
typedef struct _ARP_PACKET
{
    WORD        HardwareType;
    WORD        Protocol;
    BYTE        MACAddrLen;
    BYTE        ProtocolLen;
    WORD        Operation;
    MAC_ADDR    SenderMACAddr;
    IP_ADDR     SenderIPAddr;
    MAC_ADDR    TargetMACAddr;
    IP_ADDR     TargetIPAddr;
} ARP_PACKET;

// Helper function
static void SwapARPPacket(ARP_PACKET *p);


/*********************************************************************
 * Function:        BOOL ARPGet(NODE_INFO* remote, BYTE* opCode)
 *
 * PreCondition:    ARP packet is ready in MAC buffer.
 *
 * Input:           remote  - Remote node info
 *                  opCode  - Buffer to hold ARP op code.
 *
 * Output:          TRUE if a valid ARP packet was received.
 *                  FALSE otherwise.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
int1 ARPGet(NODE_INFO *remote, int8 *opCode)
{
    ARP_PACKET packet;

    //MACGetArray((int8*)&packet, sizeof(packet));
    MACGetArray(&packet, sizeof(ARP_PACKET));

    MACDiscardRx();

    SwapARPPacket(&packet);

   debug_arp(debug_putc, "\r\nARP: HW:%LX PR:%LX ML:%U PL:%U O:%LX TI:%U.%U.%U.%U FI:%U.%U.%U.%U",
      packet.HardwareType, packet.Protocol, packet.MACAddrLen, packet.ProtocolLen,
      packet.Operation, packet.TargetIPAddr.v[0],packet.TargetIPAddr.v[1],
      packet.TargetIPAddr.v[2],packet.TargetIPAddr.v[3],
      packet.SenderIPAddr.v[0],packet.SenderIPAddr.v[1],packet.SenderIPAddr.v[2],packet.SenderIPAddr.v[3]);

    if ( packet.HardwareType != HW_ETHERNET     ||
         packet.MACAddrLen != sizeof(MAC_ADDR)  ||
         packet.ProtocolLen != sizeof(IP_ADDR) )
         return FALSE;

    if ( packet.Operation == ARP_OPERATION_RESP )
        *opCode = ARP_REPLY;
    else if ( packet.Operation == ARP_OPERATION_REQ )
        *opCode = ARP_REQUEST;
    else
    {
        *opCode = ARP_UNKNOWN;
        return FALSE;
    }

    if(packet.TargetIPAddr.Val == AppConfig.MyIPAddr.Val)
    {
        remote->MACAddr     = packet.SenderMACAddr;
        remote->IPAddr      = packet.SenderIPAddr;
        return TRUE;
    }
    else
        return FALSE;
}


/*********************************************************************
 * Function:        void ARPPut(NODE_INFO* more, BYTE opCode)
 *
 * PreCondition:    None
 *
 * Input:           remote  - Remote node info
 *                  opCode  - ARP op code to send
 *
 * Output:          TRUE - The ARP packet was generated properly
 *               FALSE - Unable to allocate a TX buffer
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL ARPPut(NODE_INFO *remote, BYTE opCode)
{
    ARP_PACKET packet;

   BUFFER MyTxBuffer;
   MyTxBuffer = MACGetTxBuffer(TRUE);
   
   // Do not respond if there is no room to generate the ARP reply
   if(MyTxBuffer == INVALID_BUFFER)
      return FALSE;

   MACSetTxBuffer(MyTxBuffer, 0);
   
   
    packet.HardwareType             = HW_ETHERNET;
    packet.Protocol                 = ARP_IP;
    packet.MACAddrLen               = sizeof(MAC_ADDR);
    packet.ProtocolLen              = sizeof(IP_ADDR);

    if ( opCode == ARP_REQUEST )
    {
        packet.Operation            = ARP_OPERATION_REQ;
        packet.TargetMACAddr.v[0]   = 0xff;
        packet.TargetMACAddr.v[1]   = 0xff;
        packet.TargetMACAddr.v[2]   = 0xff;
        packet.TargetMACAddr.v[3]   = 0xff;
        packet.TargetMACAddr.v[4]   = 0xff;
        packet.TargetMACAddr.v[5]   = 0xff;
    }
    else
    {
        packet.Operation            = ARP_OPERATION_RESP;
        packet.TargetMACAddr        = remote->MACAddr;
    }

    packet.SenderMACAddr = AppConfig.MyMACAddr;
    packet.SenderIPAddr  = AppConfig.MyIPAddr;


    // Check to see if target is on same subnet, if not, find Gateway MAC.
    // Once we get Gateway MAC, all access to remote host will go through Gateway.
    if((packet.SenderIPAddr.Val ^ remote->IPAddr.Val) & AppConfig.MyMask.Val)
    {
      packet.TargetIPAddr = AppConfig.MyGateway;
    }
    else
        packet.TargetIPAddr             = remote->IPAddr;

    SwapARPPacket(&packet);

    MACPutHeader(&packet.TargetMACAddr, MAC_ARP, sizeof(packet));

    //MACPutArray((int8*)&packet, sizeof(packet));
    MACPutArray(&packet, sizeof(ARP_PACKET));

    MACFlush();
   
   return TRUE;
}


/*********************************************************************
 * Function:        static void SwapARPPacket(ARP_PACKET* p)
 *
 * PreCondition:    None
 *
 * Input:           p   - ARP packet to be swapped.
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
static void SwapARPPacket(ARP_PACKET *p)
{
    p->HardwareType     = swaps(p->HardwareType);
    p->Protocol         = swaps(p->Protocol);
    p->Operation        = swaps(p->Operation);
}
