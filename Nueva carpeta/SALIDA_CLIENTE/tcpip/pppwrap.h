///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                          PPPWRAP.H                                ////
////                                                                   ////
//// Definitions and prototypes used by PPPWRAP.C                      ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Apr 06, 2004: Functions use the new ppp_ip_rx_buffer[] array   ////
////                  as the buffer that holds the IP packet.  Doesn't ////
////                  read IP packet straight from modem buffer.       ////
////                                                                   ////
////    Jan 09, 2004: Initial Public Release                           ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2004 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef __TCPIP_STACK_PPPWRAP_MODULE_H
#define __TCPIP_STACK_PPPWRAP_MODULE_H

#define MAC_IP 0

#define MAC_ARP   6  //wont be used, but keep it defined

#define INVALID_BUFFER 0xFF


typedef struct __PPP_TX_BUFFER {
   //int1 txen;
   int16 txpos; //when writing to buffer (storing), use this index
   int16 rxpos; //when reading from buffer (transmitting), use this index
   int16 txlen;   //amount of bytes in this buffer
   int8 data[MAC_TX_BUFFER_SIZE];
} PPP_TX_BUFFER;

//use modem.c's buffer routines for reading/writing to the buffer
typedef struct __PPP_RX_BUFFER {
   int16 len;  //length of IP packet.  hopefully it doesn't go over MAC_RX_BUFFER_SIZE
   int16 index;
} PPP_RX_BUFFER;

typedef enum __PPP_BUFFER_CHOICE  {PPP_RX_ACTIVE, PPP_TX_ACTIVE} PPP_BUFFER_CHOICE;

///mac

BOOL    MACGetHeader(MAC_ADDR *remote, int8* type);
WORD    MACGetArray(BYTE *val, WORD len);
void    MACDiscardRx(void);
void    MACDiscardTx(BUFFER buff);
void    MACPutHeader(MAC_ADDR *remote,
                     BYTE type,
                     WORD dataLen);
void    MACSetRxBuffer(WORD offset);
void    MACSetTxBuffer(BUFFER buff, WORD offset);
void    MACReserveTxBuffer(BUFFER buff);
BOOL    MACIsTxReady(BOOL HighPriority);
WORD   MACGetFreeRxSize(void);
void    MACFlush(void);
BYTE    MACGet(void);
void    MACPut(int8 val);
void    MACPutArray(BYTE *val, WORD len);
void    MACInit(void);
BUFFER   MACGetTxBuffer(BOOL HighPriority);


///timer
int8 event_second_count;
void timer_init(void);
int1 timer_event(void);
void timer_set_s(int s);

int1 _timer_enabled=0;
#define timer_disable()    _timer_enabled=0
#define timer_enable()     _timer_enabled=1


#endif
