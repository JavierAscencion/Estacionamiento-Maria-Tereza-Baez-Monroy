//NOTE: CCS MADE SOME API CHANGES TO THIS ANNOUNCE

/*********************************************************************
 *
 *                  Announce Module for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        announce.c
 * Dependencies:    UDP.h
 * Processor:       PIC18
 * Complier:        MCC18 v1.00.50 or higher
 *                  HITECH PICC-18 V8.10PL1 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * This software is owned by Microchip Technology Inc. ("Microchip")
 * and is supplied to you for use exclusively as described in the
 * associated software agreement.  This software is protected by
 * software and other intellectual property laws.  Any use in
 * violation of the software license may subject the user to criminal
 * sanctions as well as civil liability.  Copyright 2006 Microchip
 * Technology Inc.  All rights reserved.
 *
 * This software is provided "AS IS."  MICROCHIP DISCLAIMS ALL
 * WARRANTIES, EXPRESS, IMPLIED, STATUTORY OR OTHERWISE, NOT LIMITED
 * TO MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * INFRINGEMENT.  Microchip shall in no event be liable for special,
 * incidental, or consequential damages.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     10/7/04   Original
 * Howard Schlunder      2/9/05   Simplified MAC address to text
 *                        conversion logic
 * Howard Schlunder      2/14/05   Fixed subnet broadcast calculation
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack
 ********************************************************************/

#include "tcpip/UDP.h"
#include "tcpip/Helpers.h"

#ifndef STACK_USE_ANNOUNCE_REPEAT
#define STACK_USE_ANNOUNCE_REPEAT   TRUE
#endif

#ifndef STACK_ANNOUNCE_RATE
#define STACK_ANNOUNCE_RATE   5  //every 5 seconds
#endif

#ifndef ANNOUNCE_STR_MAX_SIZE
#define ANNOUNCE_STR_MAX_SIZE 30
#endif

#ifndef STACK_ANNOUNCE_PORT
#define STACK_ANNOUNCE_PORT   6123
#endif

char g_AnnounceMessage[ANNOUNCE_STR_MAX_SIZE]={0};
int1 g_AnnounceEnabled=FALSE;
TICKTYPE g_AnnounceTick;

#if !STACK_USE_ANNOUNCE_REPEAT
 static int g_AnnounceNum;
#endif

void AnnounceEnable(void)
{
   g_AnnounceEnabled = TRUE;
   g_AnnounceTick = TickGet() - ((TICKTYPE)STACK_ANNOUNCE_RATE*TICKS_PER_SECOND); 
 #if !STACK_USE_ANNOUNCE_REPEAT
   g_AnnounceNum = 3;
 #endif
}

void AnnounceDisable(void)
{
   g_AnnounceEnabled = FALSE;
}

void InitAnnounce(char c)
{
   static int i;
   
   if (c=='\r')
      i = 0;
   else if ( i < (ANNOUNCE_STR_MAX_SIZE-1) )
   {
      g_AnnounceMessage[i++] = c;
      g_AnnounceMessage[i] = 0;
   }
}

/*********************************************************************
 * Function:        int AnnounceIP(void)
 *
 * PreCondition:    Stack is initialized()
 *
 * Input:           None
 *
 * Output:          TRUE if success
 *                  FALSE because the MAC isn't ready to TX -OR- no UDP sockets
 *
 * Side Effects:    None
 *
 * Overview:        AnnounceIP opens a UDP socket and transmits a
 *               broadcast packet to port 30303.  If a computer is
 *               on the same subnet and a utility is looking for
 *               packets on the UDP port, it will receive the
 *               broadcast.  For this application, it is used to
 *               announce the change of this board's IP address.
 *               The messages can be viewed with the MCHPDetect.exe
 *               program.
 *
 * Note:            A UDP socket must be available before this
 *               function is called.  It is freed at the end of
 *               the function.  MAX_UDP_SOCKETS may need to be
 *               increased if other modules use UDP sockets.
 ********************************************************************/
int AnnounceIP(void)
{
   UDP_SOCKET   MySocket;
   NODE_INFO   Remote;

   if (!g_AnnounceEnabled)
      return(TRUE);

   // Make certain the MAC can be written to
   if (!MACIsTxReady(FALSE))
      return(FALSE);

   // Set the socket's destination to be a broadcast over our IP
   // subnet
   // Set the MAC destination to be a broadcast
   Remote.MACAddr.v[0] = 0xFF;
   Remote.MACAddr.v[1] = 0xFF;
   Remote.MACAddr.v[2] = 0xFF;
   Remote.MACAddr.v[3] = 0xFF;
   Remote.MACAddr.v[4] = 0xFF;
   Remote.MACAddr.v[5] = 0xFF;

   // Set the IP subnet's broadcast address
   Remote.IPAddr.Val = (AppConfig.MyIPAddr.Val & AppConfig.MyMask.Val) |
                   ~AppConfig.MyMask.Val;

   // Open a UDP socket for outbound transmission
   MySocket = UDPOpen(STACK_ANNOUNCE_PORT, &Remote, STACK_ANNOUNCE_PORT);

   // Abort operation if no UDP sockets are available
   // If this ever happens, incrementing MAX_UDP_SOCKETS in
   // StackTsk.h may help (at the expense of more global memory
   // resources).
   if( MySocket == INVALID_UDP_SOCKET )
      return(FALSE);

   UDPIsPutReady(MySocket);

   printf(UDPPut, "%s", g_AnnounceMessage);

   // Send the packet
   UDPFlush();

   // Close the socket so it can be used by other modules
   UDPClose(MySocket);

   return(TRUE);
}

/*
 AnnounceTask()
 
 This is called periodically by StackTask(), and will repeat the announce
 message upon it's configured period.  If STACK_USE_ANNOUNCE_REPEAT is set
 to FALSE then it will only send a message 3 times.
*/
void AnnounceTask(void)
{
   TICKTYPE currTick;
   
   currTick = TickGet();
   
 #if !STACK_USE_ANNOUNCE_REPEAT
   if (g_AnnounceNum)
 #endif
   {
      if (TickGetDiff(currTick, g_AnnounceTick) >= ((TICKTYPE)STACK_ANNOUNCE_RATE*TICKS_PER_SECOND))
      {
         if (AnnounceIP())
         {
          #if !STACK_USE_ANNOUNCE_REPEAT
            g_AnnounceNum--;
          #endif
            g_AnnounceTick = currTick;
         }
      }
   }
}
