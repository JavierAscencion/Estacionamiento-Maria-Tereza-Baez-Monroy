/*********************************************************************
 *
 *                  MAC Module for Microchip TCP/IP Stack
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        MAC.c
 * Dependencies:    string.h
 *                  stacktsk.h
 *                  helpers.h
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
 * Nilesh Rajbharti     4/27/00  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    02/04/04 MacIsTxReady() doesn't check PTX bit until after first transmission
 * Nilesh Rajbharti     3/21/03 Fixed MACIsTxReady() bug where upon
 *                              certain bus collision, this would always
 *                              return FLASE causing Stack to not handle
 *                              any incoming packets at all.
 * Darren Rook (CCS)    06/28/04 Switched to 2.20 MacIsTxReady()
 * Darren Rook (CCS)    06/28/04 Added 2.20 MACGetOffset()
 * Darren Rook (CCS)    06/29/04 TxBuffers[] no longer static
 * R. Shelquist (CCS)	09/24/04 MacPutArray() is faster
 * Darren Rook (CCS)    10/24/06 In synch with Microchip's V3.75 stack
 ********************************************************************/
#define THIS_IS_MAC_LAYER

#define debug_mac

#include <string.h>
#include "tcpip/stacktsk.h"
#include "tcpip/helpers.h"
#include "tcpip/mac.h"

#define DelayMs(x)   delay_ms(x)

#if STACK_USE_SLIP
#error Unexpected module is detected.
#error Do not include MAC.C if STACK_USE_SLIP is set to TRUE
#endif

//Set the Data bus to Write or Read
#define SET_NIC_READ()      (NIC_DATA_TRIS = 0xff)
#define SET_NIC_WRITE()     (NIC_DATA_TRIS = 0x00)

/*
 * Hardware interface to NIC.
 */
#define NIC_DATAPORT        (0x10)
#define NIC_RESET           (0x1f)

/* Ethernet definitions.. */
#define MINFRAME  60
#define MINFRAMEC 64
#define CRCLEN    4
#define MAXFRAME  1514
#define MAXFRAMEC 1518

/* 8390 Network Interface Controller (NIC) page0 register offsets */
#define CMDR    0x00            /* command register for read & write */
#define PSTART  0x01            /* page start register for write */
#define PSTOP   0x02            /* page stop register for write */
#define BNRY    0x03            /* boundary reg for rd and wr */
#define TPSR    0x04            /* tx start page start reg for wr */
#define TXSTAT  TPSR
#define TBCR0   0x05            /* tx byte count 0 reg for wr */
#define TBCR1   0x06            /* tx byte count 1 reg for wr */
#define ISR     0x07            /* interrupt status reg for rd and wr */
#define RSAR0   0x08            /* low byte of remote start addr */
#define RSAR1   0x09            /* hi byte of remote start addr */
#define RBCR0   0x0A            /* remote byte count reg 0 for wr */
#define RBCR1   0x0B            /* remote byte count reg 1 for wr */
#define RCR     0x0C            /* rx configuration reg for wr */
#define TCR     0x0D            /* tx configuration reg for wr */
#define DCR     0x0E            /* data configuration reg for wr */
#define IMR     0x0F            /* interrupt mask reg for wr */

/* NIC page 1 register offsets */
#define PAR0    0x01            /* physical addr reg 0 for rd and wr */
#define CURRP   0x07            /* current page reg for rd and wr */
#define MAR0    0x08            /* multicast addr reg 0 for rd and WR */

/* NIC page 3 register offsets */
#define RTL9346CR 0x01          /* RTL 9346 command reg */
#define RTL3    0x06            /* RTL config reg 3 */


#define NIC_PAGE_SIZE       (256)

#define NIC_DATAPORT        0x10
#define NIC_RESET           0x1f

/* NIC RAM definitions */
#define RAMPAGES 0x20           /* Total number of 256-byte RAM pages */
#define TXSTART  0x40           /* Tx buffer start page - NE2000 mode */
#define TXPAGES (MAC_TX_BUFFER_COUNT * (MAC_TX_BUFFER_SIZE/NIC_PAGE_SIZE))
#define RXSTART  (TXSTART+TXPAGES)      /* Rx buffer start page */
#define RXSTOP   (TXSTART+RAMPAGES-1)   /* Last Rx buffer page */
#define DCRVAL   0x48           /* Value for data config reg */
                                /* 8-bit DMA, big-endian, 1 DMA, Normal */

#define RXPAGES (RXSTOP - RXSTART)


typedef struct _IEEE_HEADER
{
    MAC_ADDR        DestMACAddr;
    MAC_ADDR        SourceMACAddr;
    WORD_VAL        Len;
    BYTE            LSAPControl[3];
    BYTE            OUI[3];
    WORD_VAL        Protocol;
} IEEE_HEADER;

#define ETHER_IP        (0x00)
#define ETHER_ARP       (0x06)


typedef struct _DATA_BUFFER
{
    BYTE Index;
    BOOL bFree;
} DATA_BUFFER;


static DATA_BUFFER TxBuffers[(TXPAGES*NIC_PAGE_SIZE)/MAC_TX_BUFFER_SIZE];

#define MAX_DATA_BUFFERS        (sizeof(TxBuffers)/sizeof(TxBuffers[0]))

typedef struct _ETHER_HEADER
{
    MAC_ADDR        DestMACAddr;
    MAC_ADDR        SourceMACAddr;
    WORD_VAL        Type;
} ETHER_HEADER;


typedef struct _NE_RCR
{
     int1 PRX;
     int1 CRC;
     int1 FAE;
     int1 FO;
     int1 MPA;
     int1 PHY;
     int1 DIS;
     int1 DFR;
} NE_RCR;



typedef struct _NE_PREAMBLE
{
    NE_RCR Status;
    BYTE NextPacketPointer;
    WORD ReceivedBytes;

    MAC_ADDR        DestMACAddr;
    MAC_ADDR        SourceMACAddr;
    WORD_VAL        Type;
} NE_PREAMBLE;


BYTE NICReadPtr;                // Next page that will be used by NIC to load new packet.
BYTE NICCurrentRdPtr;           // Page that is being read...
BYTE NICCurrentTxBuffer;
//// the following 2 moved to mac.h

static void NICReset(void);
static void NICPut(BYTE reg, BYTE val);
static BYTE NICGet(BYTE reg);
static void NICSetAddr(WORD addr);

void    MACInit(void)
{
    BYTE i;

    // On Init, all transmit buffers are free.
    for ( i = 0; i < MAX_DATA_BUFFERS; i++ )
    {
        TxBuffers[i].Index = TXSTART + (i * (MAC_TX_BUFFER_SIZE/NIC_PAGE_SIZE));
        TxBuffers[i].bFree = TRUE;
    }
    NICCurrentTxBuffer = 0;

    NICReset();
    delay_ms(2);
    NICPut(NIC_RESET, NICGet(NIC_RESET));
    delay_ms(2);  // mimimum Delay of 1.6 ms

    // Continue only if reset state is entered.
    if ( (NICGet(ISR) & 0x80) != 0 )
    {
        // Select Page 0
        NICPut(CMDR, 0x21);
        delay_ms(2);

        // Initialize Data Configuration Register
        NICPut(DCR, DCRVAL);

        // Clear Remote Byte Count Registers
        NICPut(RBCR0, 0);
        NICPut(RBCR1, 0);

        // Initialize Receive Configuration Register
        NICPut(RCR, 0x04);

        // Place NIC in LOOPBACK mode 1
        NICPut(TCR, 0x02);

        // Initialize Transmit buffer queue
        NICPut(TPSR, TxBuffers[NICCurrentTxBuffer].Index);

        // Initialize Receive Buffer Ring
        NICPut(PSTART, RXSTART);
        NICPut(PSTOP, RXSTOP);
        NICPut(BNRY, (BYTE)(RXSTOP-1));

        // Initialize Interrupt Mask Register
        // Clear all status bits
        NICPut(ISR, 0xff);
        // No interrupt enabled.
        NICPut(IMR, 0x00);

        // Select Page 1
        NICPut(CMDR, 0x61);

        // Initialize Physical Address Registers
        NICPut(PAR0+0, AppConfig.MyMACAddr.v[0]);
        NICPut(PAR0+1, AppConfig.MyMACAddr.v[1]);
        NICPut(PAR0+2, AppConfig.MyMACAddr.v[2]);
        NICPut(PAR0+3, AppConfig.MyMACAddr.v[3]);
        NICPut(PAR0+4, AppConfig.MyMACAddr.v[4]);
        NICPut(PAR0+5, AppConfig.MyMACAddr.v[5]);

        // Initialize Multicast registers
        for ( i = 0; i < 8; i++ )
            NICPut(MAR0+i, 0xff);

        // Initialize CURRent pointer
        NICPut(CURRP, RXSTART);

        // Remember current receive page
        NICReadPtr = RXSTART;

        // Page 0, Abort Remote DMA and Activate the transmitter.
        NICPut(CMDR, 0x22);

        // Set Normal Mode
        NICPut(TCR, 0x00);

    }

}



BOOL MACIsTxReady(BOOL HighPriority)
{
    // NICCurrentTxBuffer always points to free buffer, if there is any.
    // If there is none, NICCurrentTxBuffer will be a in 'Use' state.
    //return TxBuffers[NICCurrentTxBuffer].bFree;
    // Check to see if previous transmission was successful or not.
    return !(NICGet(CMDR) & 0x04);
}

BUFFER MACGetTxBuffer(BOOL HighPriority)
{
	return 0;
}


void    MACPut(BYTE val)
{
    NICPut(RBCR0, 1);
    NICPut(RBCR1, 0);
    NICPut(CMDR, 0x12);
    NICPut(NIC_DATAPORT, val);
}

void    MACPutArray(BYTE *val, WORD len)
{
    WORD_VAL t;
    t.Val = len + (len & 1);

   debug_mac("\r\nMACPUTARRAY %LX: ",len);

    NICPut(ISR, 0x40);
    NICPut(RBCR0, t.v[0]);
    NICPut(RBCR1, t.v[1]);
    NICPut(CMDR, 0x12);

 // index bug corrected RJS 8/3/04
    while ( t.Val-- > 0 ) {
        NICPut(NIC_DATAPORT, *val++);
        debug_mac("%X ",*(val-1));
    }

    // Make sure that DMA is complete.
    len = 255;
    while( len && (NICGet(ISR) & 0x40) == 0 )
        len--;
}

BYTE    MACGet(void)
{
    NICPut(RBCR0, 1);
    NICPut(RBCR1, 0);
    NICPut(CMDR, 0x0a);
    return NICGet(NIC_DATAPORT);
}


WORD    MACGetArray(BYTE *val, WORD len)
{
    WORD_VAL t;

    t.Val = len;

    NICPut(ISR, 0x40);
    NICPut(RBCR0, t.v[0]);
    NICPut(RBCR1, t.v[1]);
    NICPut(CMDR, 0x0a);

    debug_mac("\r\nMACGETARRAY: ");

    while( len-- > 0 )
    {
        *val++ = NICGet(NIC_DATAPORT);
        debug_mac("%X ",*(val-1));
    }

    return t.Val;
}

void MACReserveTxBuffer(BUFFER buff)
{
    TxBuffers[buff].bFree = FALSE;
}


void MACDiscardTx(BUFFER buff)
{
    TxBuffers[buff].bFree = TRUE;
    NICCurrentTxBuffer = buff;
}

void MACDiscardRx(void)
{
    BYTE newBoundary;

    newBoundary = NICReadPtr - 1;
    if ( newBoundary < RXSTART )
        newBoundary = RXSTOP - 1;
    NICPut(CMDR, 0x20);     // Select PAGE 0
    NICPut(BNRY, newBoundary);
}


WORD MACGetFreeRxSize(void)
{
    BYTE NICWritePtr;
    BYTE temp;
    WORD_VAL tempVal;

    NICPut(CMDR, 0x60);
    NICWritePtr = NICGet(CURRP);
    NICPut(CMDR, 0x20);

    if ( NICWritePtr < NICCurrentRdPtr )
        temp = (RXSTOP - NICCurrentRdPtr) + NICWritePtr;
    else
        temp = NICWritePtr - NICCurrentRdPtr;

    temp = RXPAGES - temp;
    tempVal.v[1] = temp;
    tempVal.v[0] = 0;
    return tempVal.Val;
}

BOOL MACIsLinked(void)
{
    BYTE_VAL temp;

    // Select Page 3
    NICPut(CMDR, 0xe0);

    // Read CONFIG0.
    temp.Val = NICGet(0x03);

    // Reset to page 0.
    NICPut(CMDR, 0x20);

    // Bit 2 "BNC" will be '0' if LINK is established.
    return (temp.bits.b2 == 0);
}


BOOL MACGetHeader(MAC_ADDR *remote, BYTE* type)
{
   int i;
    NE_PREAMBLE     header;
    BYTE NICWritePtr;
    WORD_VAL temp;

    *type = MAC_UNKNOWN;

    // Reset NIC if overrun has occured.
    if ( NICGet(ISR) & 0x10 )
    {
#if 1
        NICPut(CMDR, 0x21);
		for(i = 0; i < 10; i++)
		{
			DelayMs(20);
		}
        NICPut(RBCR0, 0);
        NICPut(RBCR1, 0);
        NICPut(TCR, 0x02);
        NICPut(CMDR, 0x20);
        MACDiscardRx();
        NICPut(ISR, 0xff);
        NICPut(TCR, 0x00);
        return FALSE;
#else
        MACInit();
        return FALSE;
#endif
    }

    NICPut(CMDR, 0x60);
    NICWritePtr = NICGet(CURRP);
    NICPut(CMDR, 0x20);

    if ( NICWritePtr != NICReadPtr )
    {
        temp.v[1] = NICReadPtr;
        temp.v[0] = 0;
        NICSetAddr(temp.Val);

        //MACGetArray((int8*)&header, sizeof(header));

        debug_mac("\r\n***************************************\r\n");

         MACGetArray(&header, sizeof(NE_PREAMBLE));

        debug_mac("\r\n\r\nGOT HDR = ST:%X NPP:%X LEN:%LU",
          (int8)header.Status,header.NextPacketPointer,header.ReceivedBytes);
        debug_mac("\r\n  DEST: %X.%X.%X.%X.%X.%X  SRC: %X.%X.%X.%X.%X.%X  TYPE:%LX",
            header.DestMACAddr.v[0],header.DestMACAddr.v[1],header.DestMACAddr.v[2],
            header.DestMACAddr.v[3],header.DestMACAddr.v[4],header.DestMACAddr.v[5],
            header.SourceMACAddr.v[0],header.SourceMACAddr.v[1],header.SourceMACAddr.v[2],
            header.SourceMACAddr.v[3],header.SourceMACAddr.v[4],header.SourceMACAddr.v[5],
            header.Type.Val);


        // Validate packet length and status.
        if ( header.Status.PRX && (header.ReceivedBytes >= MINFRAMEC) && (header.ReceivedBytes <= MAXFRAMEC) )
        {
            debug_mac(" VALID");
            header.Type.Val = swaps(header.Type.Val);

            //memcpy((char*)remote->v, (char*)header.SourceMACAddr.v, sizeof(*remote));
            memcpy(&remote->v[0], &header.SourceMACAddr.v[0], sizeof(MAC_ADDR));

            if ( (header.Type.v[1] == 0x08) && ((header.Type.v[0] == ETHER_IP) || (header.Type.v[0] == ETHER_ARP)) )
                *type = header.Type.v[0];

        }

        NICCurrentRdPtr = NICReadPtr;
        NICReadPtr = header.NextPacketPointer;

        return TRUE;
    }
    return FALSE;

}



void    MACPutHeader(MAC_ADDR *remote,
                     BYTE type,
                     WORD dataLen)
{
    WORD_VAL mytemp;
    BYTE etherType;

    NICPut(ISR, 0x0a);

    mytemp.v[1] = TxBuffers[NICCurrentTxBuffer].Index;
    mytemp.v[0] = 0;

    NICSetAddr(mytemp.Val);

    MACPutArray((BYTE*)remote, sizeof(*remote));

   debug_mac("\r\n\r\nMACPUTHEADER: DA:%X.%X.%X.%X.%X.%X T=%X L=%LX",
      remote->v[0],remote->v[1],remote->v[2],remote->v[3],remote->v[4],remote->v[5],type,dataLen);

    MACPut(AppConfig.MyMACAddr.v[0]);
    MACPut(AppConfig.MyMACAddr.v[1]);
    MACPut(AppConfig.MyMACAddr.v[2]);
    MACPut(AppConfig.MyMACAddr.v[3]);
    MACPut(AppConfig.MyMACAddr.v[4]);
    MACPut(AppConfig.MyMACAddr.v[5]);

    if ( type == MAC_IP )
        etherType = ETHER_IP;
    else
        etherType = ETHER_ARP;

    MACPut(0x08);
    MACPut(etherType);

    dataLen += (WORD)sizeof(ETHER_HEADER);
    if ( dataLen < MINFRAME ) // 64 )      // NKR 4/23/02
        dataLen = 64; // MINFRAME;
    mytemp.Val = dataLen;

    NICPut(TBCR0, mytemp.v[0]);
    NICPut(TBCR1, mytemp.v[1]);

}

void MACFlush(void)
{
    BYTE i;

    NICPut(TPSR, TxBuffers[NICCurrentTxBuffer].Index);

    NICPut(CMDR, 0x24);

    // After every transmission, adjust transmit pointer to
    // next free transmit buffer.
    for ( i = 0; i < MAX_DATA_BUFFERS; i++ )
    {
        if ( TxBuffers[i].bFree )
        {
            NICCurrentTxBuffer = i;
            return;
        }
    }
}

static void NICReset(void)
{
    SET_NIC_READ();
    WRITE_NIC_ADDR(0);

    NIC_IOW_LAT = 1;
    NIC_IOR_LAT = 1;
    NIC_RESET_LAT = 1;

    NIC_RESET_TRIS=0;
    NIC_IOW_TRIS=0;
    NIC_IOR_TRIS=0;

    // Reset pulse must be at least 800 ns.
    delay_us(10);

    NIC_RESET_LAT = 0;
}

static void NICPut(BYTE reg, BYTE val)
{
    WRITE_NIC_ADDR(reg);
    NIC_DATA_LAT = val;
    SET_NIC_WRITE();
    NIC_IOW_LAT = 0;
    NIC_IOW_LAT = 1;
    SET_NIC_READ();
}

static BYTE NICGet(BYTE reg)
{
    BYTE val;

    SET_NIC_READ();
    WRITE_NIC_ADDR(reg);
    NIC_IOR_LAT = 0;
    val = NIC_DATA_IO;
    NIC_IOR_LAT = 1;
    return val;
}


static void NICSetAddr(WORD addr)
{
    WORD_VAL t;

    t.Val = addr;
    NICPut(ISR, 0x40);
    NICPut(RSAR0, t.v[0]);
    NICPut(RSAR1, t.v[1]);
}

void MACSetRxBuffer(WORD offset)
{
    WORD_VAL t;

    t.v[1] = NICCurrentRdPtr;
    t.v[0] = sizeof(NE_PREAMBLE);

    t.Val += offset;

    NICSetAddr(t.Val);
}

void MACSetTxBuffer(BUFFER buff, WORD offset)
{
    WORD_VAL t;

    NICCurrentTxBuffer = buff;
    t.v[1] = TxBuffers[NICCurrentTxBuffer].Index;
    t.v[0] = sizeof(ETHER_HEADER);

    t.Val += offset;

    NICSetAddr(t.Val);
}

WORD MACGetOffset(void)
{
    WORD_VAL t;

    t.v[1] = NICGet(RSAR1);
    t.v[0] = NICGet(RSAR0);

    return t.Val;
}
