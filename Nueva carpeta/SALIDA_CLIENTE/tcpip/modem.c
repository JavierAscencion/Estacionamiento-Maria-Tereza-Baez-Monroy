///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                            MODEM.C                                ////
////                                                                   ////
//// Driver used by the PPP portion of the TCP/IP stack to talk to the ////
//// modem.  The user does not need to call any of these functions,    ////
//// the PPP portion of the TCP/IP stack will.                         ////
////                                                                   ////
//// This driver requires the modem to be attached to a hardware UART. ////
////                                                                   ////
//// The global variable connected_baudrate can be used to see what    ////
//// baudrate you are currently connected at.  It will be 0 when       ////
//// the modem is not connected.                                       ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Sometime in 2004-2005?: Added _modem_getb_timeout              ////
////                                                                   ////
////    Apr 06, 2004: Modem buffer size isn't tied to MAC_RX_SIZE      ////
////                  anymore.                                         ////
////                  ISR doesn't strip out PPP escape codes anymore.  ////
////                  PPP doesn't use modem buffer for the IP packet   ////
////                  buffer anymore.                                  ////
////                                                                   ////
////    Mar 31, 2004: Will not send hangup command before a dial.      ////
////                  When you dial, if a timeout happens waiting      ////
////                   for a CARRIER or NO CARRIER, gives a            ////
////                   different error.                                ////
////    Jan 09, 2004: Initial Public Release.                          ////
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

#include <tcpip/modem.h>
#include <tcpip/ppp.h>

int1 _modem_getb_timeout=0;

#IF MODEM_RX==PIN_G2
#int_rda2
#elif MODEM_RX==PIN_C7
#int_rda
#else
#error Modem ISR was written for hardware UART.  Change your I/O mappings or rewrite code to use software UART.
#endif
void rda_isr(void) {
   char c;

   c=fgetc(MODEM);
   _modem_getb_timeout=0;

   modem_buffer[modem_next++]=c;
   if (modem_next >= MODEM_BUFFER_SIZE) {modem_next=0;}
   if (modem_next == modem_last) {modem_overrun=1;}
}


//gets a character from receive buffer
char modem_getb(void) {
   char c;
   int16 count=0;
      
   if (_modem_getb_timeout) {return(0);}  ///not proud of this hack

   while (!modem_kbhit && (count!=0xFFFF)) {
      delay_us(35);
      count++;
   }

   //not proud of this hack:
   if (count==0xFFFF) {_modem_getb_timeout=1; return(0);}   

   c=modem_buffer[modem_last++];
   if (modem_last >= MODEM_BUFFER_SIZE) {modem_last=0;}

 #IFDEF DEBUG_LED2
   if (!modem_kbhit) {DEBUG_LED_LOW(DEBUG_LED2);}
 #ENDIF

   return(c);
}

void modem_flush(void) {
   modem_last = modem_next;
}


///end buffer

void modem_init(void) {

   output_high(MODEM_RESET);
   delay_ms(100);
   output_low(MODEM_RESET);
   delay_ms(100);
   output_high(MODEM_RESET);
   delay_ms(100);

   modem_next=0;
   modem_last=0;
   connected_baudrate=0;

   while(kbhit(MODEM)) {fgetc(MODEM);} //clear overrun

#IF MODEM_RX==PIN_G2
   enable_interrupts(INT_RDA2);
#ELSE
   enable_interrupts(INT_RDA);
#ENDIF
   enable_interrupts(GLOBAL);
}

/********************************************************************
//
// function: modem_at_command
//
// description: listens to and figures out the modem's AT response to AT commands we sent.
//
// input:  str (AT command to send)
//         to_base (time to wait, in ms, for a response from the modem)
// output: s7600_ec (error code type which gives a reason for error)
// calls: s7600_Serial_kbhit(), s7600_Serial_Read(), s7600_DMA_Read()
//
//*******************************************************************/
MODEM_RESP modem_at_command(char * str, int32 to)
{
   int1 done=0;
   int1 first=1;
   int8 char1=0,char2=0,c;
   MODEM_RESP resp=MODEM_NO_RESPONSE;
   char baud[8];  int i;
   int len;

   to*=10;  //convert 1ms to 100us

   modem_flush();
   fprintf(MODEM,"%s\r",str);

   len=strlen(str)+1;

   while (to && (!done)) {
      if (modem_kbhit) {
         c=modem_getb();
         if (len) {len--;} //pull off local echo
         else if (first) {
            char1=c;
            if ((char1 <= 'Z') && (char1 >= 'A')) {first=0;}
         }
         else if (!first) {
            char2=c;
            done=1;
         }
      }
      else {
         delay_us(100);
         to--;
      }
   }

   if      ((done) && (char1 == 'O') && (char2 == 'K')) {resp=MODEM_OK;}
   else if ((done) && (char1 == 'C') && (char2 == 'O')) { //CONNECT 21600
      resp=MODEM_CONNECTED;
      done=0;
      for (i=0;i<6;i++) {char1=modem_getb();}
      i=0;
      do {
         char1=modem_getb();
         if ((char1>='0')&&(char1<='9')) {baud[i++]=char1;}
         else {done=1;}
      } while (!done);
      baud[i]=0;
      connected_baudrate=atoi32(baud);
   }
   else if ((done) && (char1 == 'B') && (char2 == 'U')) {resp=MODEM_BUSY;}
   else if ((done) && (char1 == 'N') && (char2 == 'O')) {
      modem_getb();
      char1=modem_getb();
      if (char1 == 'C') {resp=MODEM_NO_CARRIER;}
      else if (char1 == 'D') {resp=MODEM_NO_DIALTONE;}
   }

   return(resp);
}

/********************************************************************
//
// function: modem_connect
//
// description: dials a phone number and makes a modem connection. (does not establish a PPP connection)
//
// input: phone_number (an array of char that contains the phone number to dial, null terminated)
// output: s7600_ec (error code type which gives a reason for error)
// calls: s7600_Serial_Write(), modem_response(), s7600_DMA_Write(), s7600_DMA_Read()
//
//*******************************************************************/

MODEM_RESP modem_connect(char * phone_number)
{
   char ATstr[30];

   MODEM_RESP resp;

   connected_baudrate=0;

   //check to see if we are online first
   //modem_disconnect();  //just in case

   sprintf(ATstr,"%s",MODEM_INIT_STR);
   resp=modem_at_command(ATstr, MODEM_RESPONSE_TIMEOUT);

   if (resp==MODEM_OK) {
      sprintf(ATstr,"%s%s",MODEM_DIAL_STR,phone_number);
      resp=modem_at_command(ATstr, MODEM_CONNECT_TIMEOUT);
      if (resp==MODEM_NO_RESPONSE) {
         resp=MODEM_DIALUP_TIMEOUT;
      }
   }

   return(resp);
}


/********************************************************************
//
// function: modem_disconnect
//
// description: hangs up the modem
//
// input: none
// output: s7600_ec (error code type which gives a reason for error)
// calls: s7600_DMA_Write()
//
//*******************************************************************/

MODEM_RESP modem_disconnect(void)
{
   char ATH[]="ATH";
   MODEM_RESP resp;

   delay_s(2);
   fprintf(MODEM,"+++");
   delay_s(2);

   resp=modem_at_command(ATH, MODEM_RESPONSE_TIMEOUT);

   if ((resp==MODEM_OK)||(resp==MODEM_NO_CARRIER)) {resp=MODEM_OK;}

   return(resp);
}

