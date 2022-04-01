/*********************************************************************
 *
 *                  Microchip TCP/IP Stack Definations for PIC18
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        StackTsk.h
 * Dependencies:    compiler.h
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
 * Nilesh Rajbharti     8/10/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    04/08/05 Added http.c and http.h.
 * Darren Rook (CCS)    09/01/06 Removed APP_CONFIG.Flags.bIsDHCPEnabled
 * Nilesh Rajbharti     8/7/03  Rev 2.21 - TFTP Client addition
 * Howard Schlunder      9/30/04   Added MCHP_MAC, MAC_POWER_ON_TEST,
                         EEPROM_BUFFER_SIZE, USE_LCD
 * Howard Schlunder      8/09/06   Removed MCHP_MAC, added STACK_USE_NBNS,
 *                        STACK_USE_DNS, and STACK_USE_GENERIC_TCP_EXAMPLE
 * Darren Rook (CCS)    10/25/06 In synch with Microchip's V3.75 stack
 ********************************************************************/
#ifndef STACK_TSK_H
#define STACK_TSK_H

#case

#ifndef OUTPUT_DRIVE_MACRO
 #if defined(__PCH__)
   #define OUTPUT_DRIVE_MACRO(x) bit_clear(*(x/8+18),x%8)
 #else
   #define OUTPUT_DRIVE_MACRO(x) bit_clear(*(x/8+0x80),x%8)
 #endif
#endif

#ifndef OUTPUT_FLOAT_MACRO
 #if defined(__PCH__)
   #define OUTPUT_FLOAT_MACRO(x) bit_set(*(x/8+18),x%8)
 #else
   #define OUTPUT_FLOAT_MACRO(x) bit_set(*(x/8+0x80),x%8)
 #endif
#endif


#include "tcpip/pic18.h"

/*
 * This value is specific to the Microchip Ethernet controllers.
 * If a different Ethernet controller is used, this define is not
 * used.  If a Microchip controller is used and a self memory test
 * should be done when the MACInit() function is called,
 * uncomment this define.  The test requires ~512 bytes of
 * program memory.
 */
//#define MAC_POWER_ON_TEST


/*
 * This value is specific to the Microchip Ethernet controllers.
 * If a different Ethernet controller is used, this define is not
 * used.  Ideally, when MAC_FILTER_BROADCASTS is defined, all
 * broadcast packets that are received would be discarded by the
 * hardware, except for ARP requests for our IP address.  This could
 * be accomplished by filtering all broadcasts, but allowing the ARPs
 * using the patter match filter.  The code for this feature has been
 * partially implemented, but it is not complete nor tested, so this
 * option should remain unused in this stack version.
 */
//#define MAC_FILTER_BROADCASTS

/*
 * Number of bytes to be reserved before MPFS storage is to start.
 *
 * These bytes host application configurations such as IP Address,
 * MAC Address, and any other required variables.
 *
 * After making any change to this variable, MPFS.exe must be
 * executed with correct block size.
 * See MPFS.exe help message by executing MPFS /?
 */
#ifndef MPFS_START_POSITION
#define MPFS_START_POSITION   548
#endif

#define END_OF_MPFS_POINTER             (MPFS_START_POSITION)
#define MPFS_RESERVE_BLOCK              (END_OF_MPFS_POINTER+4)

/*
 * Modules to include in this project
 * For demo purpose only, each sample project defines one or more
 * of following defines in compiler command-line options. (See
 * each MPLAB Project Node Properties under "Project->Edit Project" menu.
 * In real applcation, user may want to define them here.
 */
#ifndef STACK_USE_MAC
   #define  STACK_USE_MAC  TRUE
#endif

#ifndef STACK_USE_MCPENC
   #define STACK_USE_MCPENC FALSE
#endif

#ifndef STACK_USE_MCPINC
   #define STACK_USE_MCPINC FALSE
#endif

//using MCPENC chip requires MAC
#if STACK_USE_MCPENC
   #undef STACK_USE_MAC
   #define STACK_USE_MAC TRUE
#endif

//using MCPENC chip requires MAC
#if STACK_USE_MCPINC
   #undef STACK_USE_MAC
   #define STACK_USE_MAC TRUE
#endif


#ifndef STACK_USE_PPP
   #define STACK_USE_PPP   FALSE
#endif

#ifndef STACK_USE_SLIP
   #define STACK_USE_SLIP FALSE
#endif

#if (STACK_USE_SLIP + STACK_USE_PPP + STACK_USE_MAC)>1
   #error ONLY SPECIFY ONE MAC DRIVER (SLIP, PPP or ETHERNET)
#endif

#if !(STACK_USE_SLIP || STACK_USE_PPP || STACK_USE_MAC)
   #error PLEASE SPECIFY A MAC DRIVER
#endif

#ifndef STACK_USE_DNS
   #define STACK_USE_DNS   FALSE
#endif

#ifndef STACK_USE_DHCP
   #define STACK_USE_DHCP   FALSE
#endif

#ifndef STACK_USE_UDP
   #define STACK_USE_UDP   FALSE
#endif

#ifndef STACK_USE_ICMP
   #define STACK_USE_ICMP   FALSE
#endif

#ifndef STACK_USE_ARP
   #define   STACK_USE_ARP   FALSE
#endif

#ifndef STACK_USE_TELNET
   #define   STACK_USE_TELNET   FALSE
#endif

#ifndef STACK_USE_HTTP
   #define  STACK_USE_HTTP FALSE
#endif

#ifndef STACK_USE_SMTP
   #define STACK_USE_SMTP  FALSE
#endif

#ifndef STACK_USE_SNMP
   #define STACK_USE_SNMP  FALSE
#endif

#ifndef STACK_USE_ANNOUNCE
   #define STACK_USE_ANNOUNCE FALSE
#endif

#if (STACK_USE_ARP && STACK_USE_PPP)
 #ERROR CANNOT USE ARP WITH PPP
#ENDIF

#ifndef   STACK_USE_TCP
   #define   STACK_USE_TCP   FALSE
#endif

#ifndef STACK_USE_IP_GLEANING
   #define STACK_USE_IP_GLEANING   FALSE
#endif

#ifndef STACK_USE_MPFS
   #define STACK_USE_MPFS   FALSE
#endif

#ifndef STACK_USE_HTTP2
   #define STACK_USE_HTTP2   FALSE
#endif

#ifndef STACK_USE_FAT
   #define STACK_USE_FAT     FALSE
#endif

#ifndef STACK_USE_FTP
   #define STACK_USE_FTP     FALSE
#endif

#ifndef STACK_USE_TFTP
   #define STACK_USE_TFTP     FALSE
#endif

#ifndef STACK_USE_TEMP
   #define STACK_USE_TEMP     FALSE
#endif

/*
 * When SLIP is used, DHCP is not supported.
 */
#if STACK_USE_SLIP
   #undef STACK_USE_DHCP
   #define STACK_USE_DHCP   FALSE
#endif

/*
 * When DHCP is enabled, UDP must also be enabled.
 */
#if STACK_USE_DHCP
    #if defined(STACK_USE_UDP)
       #undef STACK_USE_UDP
    #endif
    #define STACK_USE_UDP TRUE
#endif

/*
 * When IP Gleaning is enabled, ICMP must also be enabled.
 */
#if STACK_USE_IP_GLEANING
    #if defined(STACK_USE_ICMP)
       #undef STACK_USE_ICMP
    #endif
        #define STACK_USE_ICMP   TRUE
#endif

/*
 * This value is for performance enhancing features specific to
 * Microchip Ethernet controllers.  If a non-Microchip Ethernet
 * controller is used, this define must be commented out.  When
 * defined, checksum computations will be offloaded to the hardware.
 */
#if STACK_USE_MCPENC || STACK_USE_MCPINC
 #define MCHP_MAC
#endif


/*
 * DHCP requires unfragmented packet size of at least 328 bytes,
 * and while in SLIP mode, our maximum packet size is less than
 * 255.  Hence disallow DHCP module while SLIP is in use.
 * If required, one can use DHCP while SLIP is in use by modifying
 * C18 linker scipt file such that C18 compiler can allocate
 * a static array larger than 255 bytes.
 * Due to very specific application that would require this,
 * sample stack does not provide such facility.  Interested users
 * must do this on their own.
 */
#if STACK_USE_SLIP
    #if STACK_USE_DHCP
        #error DHCP cannot be used when SLIP is enabled.
    #endif
#endif

#include "tcpip/hardware.h"

#define MY_MAC_BYTE1                    AppConfig.MyMACAddr.v[0]
#define MY_MAC_BYTE2                    AppConfig.MyMACAddr.v[1]
#define MY_MAC_BYTE3                    AppConfig.MyMACAddr.v[2]
#define MY_MAC_BYTE4                    AppConfig.MyMACAddr.v[3]
#define MY_MAC_BYTE5                    AppConfig.MyMACAddr.v[4]
#define MY_MAC_BYTE6                    AppConfig.MyMACAddr.v[5]

/*
 * Subnet mask for this node.
 * Must not be all zero's or else this node will never transmit
 * anything !!
 */
#define MY_MASK_BYTE1                   AppConfig.MyMask.v[0]
#define MY_MASK_BYTE2                   AppConfig.MyMask.v[1]
#define MY_MASK_BYTE3                   AppConfig.MyMask.v[2]
#define MY_MASK_BYTE4                   AppConfig.MyMask.v[3]

/*
 * Hardcoded IP address of this node
 * My IP = 10.10.5.10
 *
 * Gateway = 10.10.5.10
 */

#define MY_IP                           AppConfig.MyIPAddr

#define MY_IP_BYTE1                     AppConfig.MyIPAddr.v[0]
#define MY_IP_BYTE2                     AppConfig.MyIPAddr.v[1]
#define MY_IP_BYTE3                     AppConfig.MyIPAddr.v[2]
#define MY_IP_BYTE4                     AppConfig.MyIPAddr.v[3]

/*
 * Harcoded Gateway address for this node.
 * This should be changed to match actual network environment.
 */
#define MY_GATE_BYTE1                   AppConfig.MyGateway.v[0]
#define MY_GATE_BYTE2                   AppConfig.MyGateway.v[1]
#define MY_GATE_BYTE3                   AppConfig.MyGateway.v[2]
#define MY_GATE_BYTE4                   AppConfig.MyGateway.v[3]

/*
 * Harcoded DNS server for this node.
 */
#define MY_DNS_BYTE1                   AppConfig.PrimaryDNSServer.v[0]
#define MY_DNS_BYTE2                   AppConfig.PrimaryDNSServer.v[1]
#define MY_DNS_BYTE3                   AppConfig.PrimaryDNSServer.v[2]
#define MY_DNS_BYTE4                   AppConfig.PrimaryDNSServer.v[3]


#ifndef MAX_SOCKETS
#define MAX_SOCKETS                     35
#endif

#ifndef MAX_UDP_SOCKETS
#define MAX_UDP_SOCKETS                 2
#endif

#if (MAX_SOCKETS <= 0 || MAX_SOCKETS > 255)
#error Invalid MAX_SOCKETS value specified.
#endif

#if (MAX_UDP_SOCKETS <= 0 || MAX_UDP_SOCKETS > 255 )&&STACK_USE_UDP
#error Invlaid MAX_UDP_SOCKETS value specified
#endif



#if (MAC_TX_BUFFER_SIZE <= 0 || MAC_TX_BUFFER_SIZE > 1500 )
#error Invalid MAC_TX_BUFFER_SIZE value specified.
#endif

#if ( (MAC_TX_BUFFER_SIZE * MAC_TX_BUFFER_COUNT) > (4* 1024) )
#error Not enough room for Receive buffer.
#endif


#if STACK_USE_DHCP
    #if (MAX_UDP_SOCKETS < 1)
        #error Set MAX_UDP_SOCKETS to at least one.
    #endif
#endif

typedef int1 BOOL;

typedef BYTE BUFFER;

typedef int16 WORD;
typedef int32 DWORD;

typedef union _BYTE_VAL
{
    BYTE Val;
    struct
    {
        unsigned char b0:1;
        unsigned char b1:1;
        unsigned char b2:1;
        unsigned char b3:1;
        unsigned char b4:1;
        unsigned char b5:1;
        unsigned char b6:1;
        unsigned char b7:1;
    } bits;
} BYTE_VAL;


typedef union _SWORD_VAL
{
    int32 Val;
    struct
    {
        int8 LSB;
        int8 MSB;
        int8 USB;
    } bytes;
} SWORD_VAL;


typedef union _WORD_VAL
{
    int16 Val;
    int8 v[2];
    struct
    {
        int8 LSB;
        int8 MSB;
    } bytes;
} WORD_VAL;

/*
typedef union _DWORD_VAL
{
    int32 Val;
    int8 v[4];
} DWORD_VAL;
*/

typedef union _DWORD_VAL
{
    DWORD Val;
   WORD w[2];
    BYTE v[4];
    struct
    {
        WORD LW;
        WORD HW;
    } word;
    struct
    {
        BYTE LB;
        BYTE HB;
        BYTE UB;
        BYTE MB;
    } byte;
    struct
    {
        unsigned char b0:1;
        unsigned char b1:1;
        unsigned char b2:1;
        unsigned char b3:1;
        unsigned char b4:1;
        unsigned char b5:1;
        unsigned char b6:1;
        unsigned char b7:1;
        unsigned char b8:1;
        unsigned char b9:1;
        unsigned char b10:1;
        unsigned char b11:1;
        unsigned char b12:1;
        unsigned char b13:1;
        unsigned char b14:1;
        unsigned char b15:1;
        unsigned char b16:1;
        unsigned char b17:1;
        unsigned char b18:1;
        unsigned char b19:1;
        unsigned char b20:1;
        unsigned char b21:1;
        unsigned char b22:1;
        unsigned char b23:1;
        unsigned char b24:1;
        unsigned char b25:1;
        unsigned char b26:1;
        unsigned char b27:1;
        unsigned char b28:1;
        unsigned char b29:1;
        unsigned char b30:1;
        unsigned char b31:1;
    } bits;
} DWORD_VAL;


#define LOWER_LSB(a)    (a).v[0]
#define LOWER_MSB(a)   (a).v[1]
#define UPPER_LSB(a)    (a).v[2]
#define UPPER_MSB(a)    (a).v[3]

typedef struct _MAC_ADDR
{
    BYTE v[6];
} MAC_ADDR;

typedef union _IP_ADDR
{
    BYTE        v[4];
    DWORD       Val;
} IP_ADDR;


typedef struct _NODE_INFO
{
    MAC_ADDR    MACAddr;
    IP_ADDR     IPAddr;
} NODE_INFO;

typedef struct _APP_CONFIG
{
    IP_ADDR     MyIPAddr;
    MAC_ADDR    MyMACAddr;
    IP_ADDR     MyMask;
    IP_ADDR     MyGateway;
   IP_ADDR      PrimaryDNSServer;
    struct
    {
        unsigned char bIsDHCPEnabled : 1;
        unsigned char bInConfigMode : 1;
    } Flags;
    WORD_VAL    SerialNumber;
    IP_ADDR     SMTPServerAddr;     // Not used.
    IP_ADDR     TFTPServerAddr;     // Not used.
   BYTE      NetBIOSName[16];
} APP_CONFIG;

/*typedef union _STACK_FLAGS
{
    struct
    {
        int1 bInConfigMode : 1;
    } bits;
    int8 Val;
} STACK_FLAGS;*/

APP_CONFIG AppConfig;

#if STACK_USE_IP_GLEANING || STACK_USE_DHCP
    #define StackIsInConfigMode()   (stackFlags.bits.bInConfigMode)
#else
    #define StackIsInConfigMode()   FALSE
#endif


/*********************************************************************
 * Function:        void StackInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Stack and its componentns are initialized
 *
 * Side Effects:    None
 *
 * Note:            This function must be called before any of the
 *                  stack or its component routines be used.
 *
 ********************************************************************/
void StackInit(void);


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
 *                  This function must be called periodically called
 *                  to make sure that timely response.
 *
 ********************************************************************/
void StackTask(void);


#endif
