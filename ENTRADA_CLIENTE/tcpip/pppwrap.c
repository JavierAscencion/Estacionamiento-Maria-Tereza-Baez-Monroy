///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                          PPPWRAP.C                                ////
////                                                                   ////
//// A wrapper for CCS's PPP functions (PPP.C) to make it compatable   ////
//// with Microchip's TCP/IP stack.  Makes PPP.C look like MAC.C.      ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Apr 06, 2004: Functions use the new ppp_ip_rx_buffer[] array   ////
////                  as the buffer that holds the IP packet.  Doesn't ////
////                  read IP packet straight from modem buffer.       ////
////                                                                   ////
////    Apr 01, 2004: MacGetArray() properly returns number of bytes   ////
////                      read.                                        ////
////                  MacFlush() no longer uses Transmit buffer.       ////
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

#include <tcpip/pppwrap.h>

PPP_TX_BUFFER ppp_tx_buff;
PPP_RX_BUFFER ppp_rx_buff;

PPP_BUFFER_CHOICE ppp_active_buff;
#locate ppp_active_buff=0x20
///mac


//ppp_init() will call this
void    MACInit(void) {
   ppp_tx_buff.txlen=0;
}


/// mac rx

//checks the mac rx buffer, returns remote host and packet information if true
//(in ppp case,  remote will not be valid but type will be MAC_IP and return TRUE if new packet)
int1    MACGetHeader(MAC_ADDR *remote, int8* type) {
   ppp_rx_buff.len = ppp_handle();  //this calls CCS's PPP Handler. MUST BE CALLED AT REGULAR INTERVALS
   if (ppp_rx_buff.len) {
      ppp_rx_buff.index=0;
      ppp_active_buff = PPP_RX_ACTIVE;
      *type = MAC_IP;
      return(TRUE);
   }
   return(FALSE);
}

//gets next byte from rx or tx buffer
int8    MACGet(void) {
   if (ppp_active_buff == PPP_RX_ACTIVE) {
      return(ppp_ip_rx_buffer[ppp_rx_buff.index++]);  //get from rx buffer.  nic_getc() automatically wraps around buffer
   }
   else {
      return(ppp_tx_buff.data[ppp_tx_buff.txpos++]);
   }
}

//fetches an array of bytes from the active tx or rx buffer.
//returns number of bytes read.  (NOT SURE WHY MICROCHIP DOES THIS)
WORD    MACGetArray(BYTE *val, WORD len) {
   int16 ret;
   char c;
   ret=len;

   debug_ppp("\r\nMAC GET ARRAY L=%LU:\r\n",len);

   while (len) {
      c = MACGet();
      debug_ppp("%X ",c);
      *val = c;
      val++;
      len--;
   }

   debug_ppp("\r\n");

   return(ret);
}

//discard active rx buffer and mark it as free
void    MACDiscardRx(void) {
   debug_ppp("\r\nDISCARD RX BUFFER ");
   ppp_rx_buff.index=0;
   ppp_rx_buff.len=0;
}

//set access location for the active rx buffer
void    MACSetRxBuffer(WORD offset) {
   ppp_active_buff = PPP_RX_ACTIVE;

   //modem_buffer_goforward(offset, &modem_last); //WRONG
   ppp_rx_buff.index=offset;

   debug_ppp("\r\nSET RX BUFFER %LU ",offset);
}


//return total free rx buffer size avail for future data packets
WORD    MACGetFreeRxSize(void) {
   return(MAC_RX_BUFFER_SIZE);
}

//return true if one mac tx buffer is empty (in ppp case, if it's not transmitting and enough space left in buffer)
BOOL    MACIsTxReady(BOOL HighPriority) {
   return (ppp_tx_buff.txlen < MAC_TX_BUFFER_SIZE) ;
}

//loads MAC addr and puts in tx buffer. (in ppp case, just reset the ppp tx buffer for a new packet)
void    MACPutHeader(MAC_ADDR *remote, BYTE type, WORD dataLen) {
   ppp_tx_buff.rxpos = 0;
   ppp_tx_buff.txlen = dataLen;
   MACSetTxBuffer(0,0);
}

//put char into the tx or rx buffer (in ppp case, just the tx buffer unless we write directly to modem)
void    MACPut(int8 val) {
   if ( ppp_active_buff == PPP_TX_ACTIVE ) {
      ppp_tx_buff.data[ppp_tx_buff.txpos]=val;
      ppp_tx_buff.txpos++;
   }
   else {
      ppp_ip_rx_buffer[ppp_rx_buff.index++]=val;
   }
}


//write an array of characters to buffer
void    MACPutArray(BYTE *val, WORD len) {
   while (len--) {
      MACPut(*val);
      val++;
   }
}

//marks active tx buffer for transmission
//(in PPP case, send char then enable TBE interrupt. TBE interrupt then spits out rest of buffer)
void    MACFlush(void) {
   if (ppp_tx_buff.txlen) {
      ppp_tx_buff.rxpos=0;

      ppp_data_packet_begin(PPP_COMP_PROT_IP);

      debug_ppp("\r\nMAC FLUSH L=%LU\r\n",ppp_tx_buff.txlen);

      while (ppp_tx_buff.rxpos < ppp_tx_buff.txlen) {
         debug_ppp("%X ",ppp_tx_buff.data[ppp_tx_buff.rxpos]);
         ppp_putc(ppp_tx_buff.data[ppp_tx_buff.rxpos]);
         ppp_tx_buff.rxpos++;
      }

      debug_ppp("\r\nDONE MAC FLUSH ");

      ppp_tx_buff.txlen=0;

      ppp_packet_end_tx();
   }
}

//discard given tx buffer and mark it as free.
//(in ppp case, there is only 1 buffer, so ignore the parameter).
void    MACDiscardTx(BUFFER buff) {
      ppp_tx_buff.txlen = 0;
      ppp_tx_buff.txpos = 0;
      ppp_tx_buff.rxpos = 0;
      debug_ppp("\r\nDISCARD TX BUFFER ");
}

//sets access location for a given tx buffer, and makes tx buffer active
void    MACSetTxBuffer(BUFFER buff, WORD offset) {
   ppp_tx_buff.txpos = offset;
   ppp_active_buff = PPP_TX_ACTIVE;
   debug_ppp("\r\nSET TX BUFFER %LU ",offset);
}


//mark a tx buffer as reserved.
//(in ppp case, ignore)
void    MACReserveTxBuffer(BUFFER buff) {
}

BUFFER   MACGetTxBuffer(BOOL HighPriority)
{
   return(0);
}


///timer
//the following functions are used by the CCS PPP driver.
//these functions use MICROCHIP modified timer routines that are already in the stack

void timer_init(void) {
   //timer should have been initialized by TickInit(), called by StackInit().
   //this init, however, is just for the PPP timing functions below
   timer_disable();
}

int1 timer_event(void) {
   if ((_timer_enabled)&&(ppp_second_counter >= event_second_count))  {
      return(1);
   }
   return(0);
}

void timer_set_s(int s) {
   ppp_second_counter=0;
   ppp_second_counter_intermediate=0;
   event_second_count=s;
   timer_enable();
}

