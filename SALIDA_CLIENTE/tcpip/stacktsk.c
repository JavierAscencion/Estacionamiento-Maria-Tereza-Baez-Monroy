/*********************************************************************
 *
 *               Microchip TCP/IP Stack FSM Implementation on PIC18
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        StackTsk.c
 * Dependencies:    StackTsk.H
 *                  ARPTsk.h
 *                  MAC.h
 *                  IP.h
 *                  ICMP.h
 *                  Tcp.h
 *                  http.h
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
 * Nilesh Rajbharti     8/14/01  Original (Rev. 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Nilesh Rajbharti     12/5/02     Modified UDPProcess() and TCPProcess()
 *                                  to include localIP as third param.
 *                                  This was done to allow these functions
 *                                  to calculate checksum correctly.
 * Nilesh Rajbharti     7/26/04     Added code in StackTask() to not
 *                                  clear statically IP address if link is
 *                                  removed and DHCP module is disabled
 *                                  at runtime.
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    06/11/04 A break; added to StackTask() after handling an ARP, else it would goto IP handler.
 * Darren Rook (CCS)    06/28/04 Added 2.20 improvement that resets DHCP after unlink of ethernet
 * Darren Rook (CCS)    06/29/04 A fix for 2.20 improvement (see above) if DHCP was dynamically disabled
 * Darren Rook (CCS)    06/29/04 smStack no longer static
 * Darren Rook (CCS)    04/08/05 Added http.c and http.h.
 * Darren Rook (CCS)    04/08/05 Task() and Init() execute any needed HTTP code
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack
 * Darren Rook (CCS)    10/25/06 In synch with Microchip's V3.75 stack
 ********************************************************************/

#define STACK_USE_FTP_SERVER STACK_USE_FTP

#define STACK_INCLUDE
#include "tcpip/stacktsk.h"
#include <string.h>
#include <stdlib.h>
#include "tcpip/helpers.c"
#include "tcpip/tick.c"

//#define debug_stack
//#define debug_stack  debug_printf
#define debug_stack(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)

#if STACK_USE_MAC
   #include "tcpip/mac.h"
   #if STACK_USE_MCPENC
    #include "tcpip/enc28j60.c"
   #elif STACK_USE_MCPINC
    #include "tcpip/eth97j60.c"
   #else
    #include "tcpip/rtl8019AS.c"
   #endif
#endif

#if STACK_USE_PPP
   #include "tcpip/modem.c"
   #include "tcpip/ppp.c"
   #include "tcpip/pppwrap.c"
#endif

#if STACK_USE_SLIP
   #include "tcpip/slip.c"
#ENDIF

#if STACK_USE_UDP
   #include "tcpip/udp.h"
#endif

#if STACK_USE_DHCP
   #include "tcpip/dhcp.h"
#endif

#if STACK_USE_SMTP
   #include "tcpip/smtp.h"
#endif

#if STACK_USE_BASE64
   #include "tcpip/base64.h"
#endif

#if STACK_USE_HTTP
   #include "tcpip/http.h"
#endif

#if STACK_USE_FAT
   #include "fat/fat_pic.c"
#endif

#if STACK_USE_MPFS
   #include "mpfs/mpfs.h"
#endif

#if STACK_USE_HTTP2
   #include "tcpip/http2.h"
#endif

#if STACK_USE_FTP
   #include "tcpip/ftp.h"
#endif

#if STACK_USE_TFTP
   #include "tcpip/tftp.h"
#endif

#if STACK_USE_DNS
   #include "tcpip/dns.h"
#endif

#if STACK_USE_SNMP
   #include "tcpip/snmp.h"
#endif

/*
#if STACK_USE_MPFS
   #include "tcpip/mpfs.h"
#endif
*/

#include "tcpip/ip.c"

#if STACK_USE_TCP
   #include "tcpip/tcp.c"
#endif

#if STACK_USE_ICMP
   #include "tcpip/icmp.c"
#endif

#if STACK_USE_UDP
   #include "tcpip/udp.c"
#endif

#if STACK_USE_DHCP
   #include "tcpip/dhcp.c"
#endif

#if STACK_USE_TELNET
   #include "tcpip/telnet2.c"
#endif

#if STACK_USE_ARP
   #include "tcpip/arptsk.c"
   #include "tcpip/arp.c"
#endif

#if STACK_USE_MPFS
   #include "mpfs/mpfs.c"
#endif

#if STACK_USE_HTTP
   #include "tcpip/http.c"
#endif

#if STACK_USE_HTTP2
   #include "tcpip/http2.c"
#endif

#if STACK_USE_FTP
   #include "tcpip/ftp.c"
#endif

#if STACK_USE_TFTP
   #include "tcpip/tftp.c"
#endif

#if STACK_USE_SMTP
   #include "tcpip/smtp.c"
#endif

#if STACK_USE_BASE64
   #include "tcpip/base64.c"
#endif

#if STACK_USE_DNS
   #include "tcpip/dns.c"
#endif

#if STACK_USE_ANNOUNCE
   #include "tcpip/announce.h"
   #include "tcpip/announce.c"
#endif

#if STACK_USE_SNMP
   #include "tcpip/snmp.c"
#endif

//#define MAX_ICMP_DATA_LEN   64 //moved to icmp.h

/*
 * Stack FSM states.
 */
typedef enum _SM_STACK
{
    SM_STACK_IDLE=0,
    SM_STACK_MAC,
    SM_STACK_IP,
    SM_STACK_ICMP,
    SM_STACK_ICMP_REPLY,
    SM_STACK_ARP,
    SM_STACK_TCP,
    SM_STACK_UDP
} SM_STACK;

SM_STACK smStack;

NODE_INFO remoteNode;


/*********************************************************************
 * Function:        void StackInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Stack and its componets are initialized
 *
 * Side Effects:    None
 *
 * Note:            This function must be called before any of the
 *                  stack or its component routines are used.
 *
 ********************************************************************/
void StackInit(void)
{
    smStack                     = SM_STACK_IDLE;

#if STACK_USE_IP_GLEANING || STACK_USE_DHCP
    /*
     * If DHCP or IP Gleaning is enabled,
     * startup in Config Mode.
     */
    AppConfig.Flags.bInConfigMode = TRUE;
#endif

   TickInit();

#if STACK_USE_MAC
    MACInit();
#endif

#if STACK_USE_ARP
    ARPInit();
#endif

#if STACK_USE_UDP
    UDPInit();
#endif

#if STACK_USE_TCP
    TCPInit();
#endif

#if STACK_USE_DHCP
   DHCPReset();
#endif

#if STACK_USE_PPP
   ppp_init();
#endif

#if STACK_USE_TELNET
   TelnetInit();
#endif

#if STACK_USE_SMTP
   SMTPInit();
#endif

#if STACK_USE_HTTP||STACK_USE_HTTP2
   HTTP_Init();
#endif

#if STACK_USE_FTP
   FTPInit();
#endif

#if STACK_USE_TFTP
   TFTPInit();
#endif
}

/*
#if STACK_USE_MCPENC
#if MAC_TX_BUFFER_COUNT > 1
 #define DebugDisplayVals() debug_stack("\r\nTXB=%U TXB0.ADD=0x%LX TXB0.FR=%U NPL=0x%LX CPL=0x%LX",NICCurrentTxBuffer, TxBuffers[0].StartAddress, TxBuffers[0].bFree, NextPacketLocation.Val, CurrentPacketLocation.Val)
#else
 #define DebugDisplayVals() debug_stack("\r\nTXB=%U NPL=0x%LX CPL=0x%LX", NICCurrentTxBuffer, NextPacketLocation.Val, CurrentPacketLocation.Val)
#endif
#else
 #define DebugDisplayVals()
#endif
*/

#define DebugDisplayVals()

/*********************************************************************
 * Function:        void StackTask(void)
 *
 * PreCondition:    StackInit() is already called.
 *
 * Input:           None
 *
 * Output:          Stack FSM is executed.
 *
 * Side Effects:    None
 *
 * Note:            This FSM checks for new incoming packets,
 *                  and routes it to appropriate stack components.
 *                  It also performs timed operations.
 *
 *                  This function must be called periodically to
 *                  ensure timely responses.
 *
 ********************************************************************/
void StackTask(void)
{
    static WORD dataCount;

#if STACK_USE_ICMP
    static BYTE data[MAX_ICMP_DATA_LEN];
    static WORD ICMPId;
    static WORD ICMPSeq;
#endif
    IP_ADDR tempLocalIP;


    union
    {
        BYTE MACFrameType;
        BYTE IPFrameType;
#if STACK_USE_ICMP
        ICMP_CODE ICMPCode;
#endif
    } type;


    BOOL lbContinue=FALSE;

    do
    {
        lbContinue = FALSE;
        switch(smStack)
        {
        case SM_STACK_IDLE:
        case SM_STACK_MAC:
            if ( !MACGetHeader(&remoteNode.MACAddr, &type.MACFrameType) )
            {
                #if STACK_USE_DHCP
                    // Normally, an application would not include  DHCP module
                    // if it is not enabled. But in case some one wants to disable
                    // DHCP module at run-time, remember to not clear our IP
                    // address if link is removed.
                    if ( !DHCPIsDisabled() )
                    {
                        if ( !MACIsLinked() )
                        {
                            AppConfig.MyIPAddr.Val = 0x00000000ul;
                            AppConfig.Flags.bInConfigMode = TRUE;
                            DHCPReset();
                        }
                    }
                #endif
                break;
            }

            debug_stack(debug_putc, "\r\nMAC GET ");

            lbContinue = TRUE;
            if ( type.MACFrameType == MAC_IP ) {
                smStack = SM_STACK_IP;
                debug_stack(debug_putc, "IP ");
            }
            else if ( type.MACFrameType == MAC_ARP ) {
                smStack = SM_STACK_ARP;
                debug_stack(debug_putc, "ARP ");
            }
            else {
                MACDiscardRx();
                debug_stack(debug_putc, "DISCARD");
            }
            break;

        case SM_STACK_ARP:
#if STACK_USE_ARP
            if ( ARPProcess() ) {
                smStack = SM_STACK_IDLE;
            }
#else
            smStack = SM_STACK_IDLE;
#endif
            break;

        case SM_STACK_IP:
            if ( IPGetHeader(&tempLocalIP,
                             &remoteNode,
                             &type.IPFrameType,
                             &dataCount) )
            {
                lbContinue = TRUE;
                if ( type.IPFrameType == IP_PROT_ICMP )
                {
                    smStack = SM_STACK_ICMP;
                    debug_stack(debug_putc, "ICMP ");
#if STACK_USE_IP_GLEANING
                    if(AppConfig.Flags.bInConfigMode && !DHCPIsDisabled())
                    {
                        /*
                         * Accoriding to "IP Gleaning" procedure,
                         * when we receive an ICMP packet with a valid
                         * IP address while we are still in configuration
                         * mode, accept that address as ours and conclude
                         * configuration mode.
                         */
                        if( tempLocalIP.Val != 0xffffffff )
                        {
                            AppConfig.Flags.bInConfigMode = FALSE;
                            AppConfig.MyIPAddr = tempLocalIP;
                            myDHCPBindCount--;
                        }
                    }
#endif
                }

#if STACK_USE_TCP
                else if ( type.IPFrameType == IP_PROT_TCP ) {
                    smStack = SM_STACK_TCP;
                    debug_stack(debug_putc, "TCP ");
                }
#endif

#if STACK_USE_UDP
                else if ( type.IPFrameType == IP_PROT_UDP ) {
                    smStack = SM_STACK_UDP;
                    debug_stack(debug_putc, "UDP ");
                }
#endif

                else
                {
                    lbContinue = FALSE;
                    MACDiscardRx();

                    smStack = SM_STACK_IDLE;
                    debug_stack(debug_putc, "UNKOWN-IP1 ");
                }
            }
            else
            {
                MACDiscardRx();
                smStack = SM_STACK_IDLE;
                debug_stack(debug_putc, "UNKOWN-IP2 ");
            }
            break;

#if STACK_USE_UDP
        case SM_STACK_UDP:
            if ( UDPProcess(&remoteNode, &tempLocalIP, dataCount) )
            {
                debug_stack(debug_putc, "\r\nUDP PROCESSED");
                DebugDisplayVals();
                smStack = SM_STACK_IDLE;
            }
            break;
#endif

#if STACK_USE_TCP
        case SM_STACK_TCP:
            if ( TCPProcess(&remoteNode, &tempLocalIP, dataCount) )
            {
                debug_stack(debug_putc, "\r\nTCP PROCESSED");
                DebugDisplayVals();
                smStack = SM_STACK_IDLE;
            }
            break;
#endif

        case SM_STACK_ICMP:
            smStack = SM_STACK_IDLE;

#if STACK_USE_ICMP
            //if ( dataCount <= (MAX_ICMP_DATA_LEN+9) )
            if ( dataCount <= (MAX_ICMP_DATA_LEN+8) )
            {
                if ( ICMPGet(&type.ICMPCode,
                             data,
                             (BYTE*)&dataCount,
                             &ICMPId,
                             &ICMPSeq) )
                {
                    if ( type.ICMPCode == ICMP_ECHO_REQUEST )
                    {
                        debug_stack(debug_putc, "\r\nICMP PROCESSED");
                        lbContinue = TRUE;
                        smStack = SM_STACK_ICMP_REPLY;
                    }
                }
            }
            DebugDisplayVals();
#endif
            MACDiscardRx();
            break;

#if STACK_USE_ICMP
        case SM_STACK_ICMP_REPLY:
            if ( ICMPIsTxReady() )
            {
                ICMPPut(&remoteNode,
                        ICMP_ECHO_REPLY,
                        data,
                        (BYTE)dataCount,
                        ICMPId,
                        ICMPSeq);

                debug_stack(debug_putc, "\r\nICMP REPLIED");
                DebugDisplayVals();
                smStack = SM_STACK_IDLE;
            }
            break;
#endif

        }

    } while( lbContinue );

#if STACK_USE_SMTP
   SMTPTask();
#endif

#if STACK_USE_ANNOUNCE
   AnnounceTask();
#endif

#if STACK_USE_TCP
    // Perform timed TCP FSM.
    TCPTick();
#endif

#if STACK_USE_TELNET
   TelnetTask();
#endif

#if STACK_USE_HTTP
   HTTP_Task();
#endif

#if STACK_USE_HTTP2
   HTTP_Task();
#endif

#IF STACK_USE_FTP
   FTPTask();   
#ENDIF   

#if STACK_USE_TFTP
   TFTPTask();
#endif


#if STACK_USE_DHCP
    /*
     * DHCP must be called all the time even after IP configuration is
     * discovered.
     * DHCP has to account lease expiration time and renew the configuration
     * time.
     */
    DHCPTask();

    if ( DHCPIsBound() )
        AppConfig.Flags.bInConfigMode = FALSE;

#endif
}
