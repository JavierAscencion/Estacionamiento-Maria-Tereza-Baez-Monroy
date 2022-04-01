///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                            MODEM.H                                ////
////                                                                   ////
//// Defitions and prototypes for MODEM.C                              ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Apr 06, 2004: Modem buffer size isn't tied to MAC_RX_SIZE      ////
////                  anymore.                                         ////
////                  ISR doesn't strip out PPP escape codes anymore.  ////
////                  PPP doesn't use modem buffer for the IP packet   ////
////                  buffer anymore.                                  ////
////                                                                   ////
////    Mar 31, 2004: When you dial, if a timeout happens waiting      ////
////                   for a CARRIER or NO CARRIER, gives a            ////
////                   different error.                                ////
////                                                                   ////
////    Jan 16, 2004: MODEM_RESP struct cleanup (not a bug fix)        ////
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


#IFNDEF __TCPIP_STACK_MODEM_MODULE_H
#DEFINE __TCPIP_STACK_MODEM_MODULE_H


#use rs232(baud=MODEM_BAUD_RATE, xmit=MODEM_TX,rcv=MODEM_RX, STREAM=MODEM, errors)  //enable serial port

#define MODEM_BUFFER_SIZE 300

int16 modem_next=0, modem_last=0;
int8 modem_buffer[MODEM_BUFFER_SIZE];
int1 modem_overrun=0;

#define modem_kbhit   (modem_last!=modem_next)

int16 connected_baudrate;

typedef enum _MODEM_RESP {
   MODEM_OK = 0,
   MODEM_CONNECTED = 1,
   MODEM_BUSY = 2,
   MODEM_NO_RESPONSE = 3,
   MODEM_NO_DIALTONE = 4,
   MODEM_NO_CARRIER = 5,
   MODEM_DIALUP_TIMEOUT = 6
}
MODEM_RESP;


/*
void modem_buffer_goback(int16 x, int16 * ptr);
void modem_buffer_goforward(int16 x, int16 * ptr);
*/

char modem_getb(void);
void modem_flush(void);

void modem_init(void);


MODEM_RESP modem_connect(char * phone_number);
MODEM_RESP modem_disconnect(void);
MODEM_RESP modem_at_command(char * str, int32 to);

#ENDIF
