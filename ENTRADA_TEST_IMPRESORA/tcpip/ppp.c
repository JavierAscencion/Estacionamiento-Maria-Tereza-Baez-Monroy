///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                              PPP.C                                ////
////                                                                   ////
//// Hardware layer functions for adding PPP to Microchip's TCP/IP     ////
//// stack.  Many of these functions are called automatically by       ////
//// Microchip's TCP/IP stack.                                         ////
////                                                                   ////
//// Here are important functions required to create a PPP connection  ////
//// before running Microchip's TCP/IP stack:                          ////
////                                                                   ////
////   ppp_connect(username, password, phonenumber)                    ////
////     Initialize a PPP connection to ISP.  Will dial up to ISP      ////
////     and start PPP state machine.  username, password and          ////
////     phonenumbers are pointers to global variables that contain    ////
////     the ISP's phone number and login information.  Will return    ////
////     a MODEM_RESP that contains modem's last response to dial      ////
////     command.                                                      ////
////                                                                   ////
////   ppp_handle()                                                    ////
////     Normally this function is called by StackTask() to handle     ////
////     the PPP state machine.  However, after using a ppp_connect()  ////
////     you must continously call ppp_handle() until                  ////
////     the PPP has sucesfully connected.                             ////
////                                                                   ////
////   ppp_disconnect()                                                ////
////     Closes the PPP connection and hangs up the phone.             ////
////                                                                   ////
////   ppp_is_connected()                                              ////
////     Returns TRUE if the PPP layer is connected.                   ////
////                                                                   ////
////   ppp_is_connecting()                                             ////
////     Returns TRUE if the PPP layer is still attempting to connect. ////
////     Eventually PPP machine will timeout (and hangup) or connect,  ////
////     and when it does connect ppp_is_connected() will return TRUE. ////
////                                                                   ////
////   An example code that connects:                                  ////
////                                                                   ////
////    StackInit();                                                   ////
////    if (ppp_connect(&username,&password,&phonenumber)==MODEM_OK) { ////
////      while(!ppp_is_connected()) {ppp_handle();}                   ////
////    }                                                              ////
////    /* After connected, use Stack */                               ////
////    ppp_disconnect();                                              ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Nov 21, 2006: AppConfig.PrimaryDNSServer is updated            ////
////                                                                   ////
////    2004 - 2006:  Various bug fixes.                               ////
////                                                                   ////
////    May 20, 2004: ppp_disconnect() and TERMINATE state wait for    ////
////                  ack time changed from 10s to 2s.                 ////
////                                                                   ////
////    Apr 26, 2004: LCP will negotiate MRU.  Set PPP_LCP_USE_MRU to  ////
////                   to true to enable this option.  Server's MRU    ////
////                   will be saved to global var ppp_server_mru.     ////
////                   PPP stack will not use server MRU at this time. ////
////                  Async-Map cleaned up.  Still doesn't work right. ////
////                                                                   ////
////    Apr 22, 2004: Added ppp_isp_ask_pap to keep track if we should ////
////                  we should be asking for PAP in LCP.              ////
////                                                                   ////
////    Apr 06, 2004: ppp_ip_recv() will now read and save entire IP   ////
////                  packet and save to ppp_ip_rx_buffer[] array.     ////
////                  Some work done on advanced PPP functions, but    ////
////                  since they are still a little buggy those        ////
////                  options are turned off.                          ////
////                                                                   ////
////    Apr 02, 2004: ppp_get_frame() makes sure the first START it    ////
////                   got wasn't actually the end of the last packet. ////
////                                                                   ////
////    Jan 09, 2004: Initial Public Release                           ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
//// Note: Sorry for the mess.  Hopefully someday I get time to        ////
////       rewrite this.                                               ////
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2004 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#define debug_ppp

#include <tcpip/modem.h>
#include <tcpip/ppp.h>
#include <tcpip/pppwrap.h>
#include <tcpip/tcp.h>

#define ppp_is_connecting() _ppp_is_connecting
#define ppp_is_connected() _ppp_is_connected
#define MACIsLinked()   _ppp_is_connected

//send data to modem, but append PPP escape code (if needed) and calculate checksum
void ppp_putc(char c)
{
    if (c==PPP_END || c==PPP_ESC)
    {
       ppp_putc_escape(c);
    }

  #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
      else if (c<0x20)
     {
         if (ppp_async_uplink.flags.use)
       {
            if (bit_test(ppp_async_uplink.map,c))
         {
            ppp_putc_escape(c);
         }
         else
            fputc(c,MODEM);
         }
         else
       {
            ppp_putc_escape(c);
         }
      }
  #ELSE
    else if (c<0x20)
   {
       ppp_putc_escape(c);
    }
  #ENDIF

    else {
        fputc(c,MODEM);
    }


    tx_cs = ppp_cs(tx_cs, c);
    //fprintf(MODEM,"*%LX*",tx_cs);
}

//gets data from modem buffer, but checks for escape chars and calculates checksum
char ppp_getc(void) {
   char c;

   c=modem_getb();

   if (c==PPP_ESC) {
      c=modem_getb();
      c^=0x20;
   }

   rx_cs = ppp_cs(rx_cs,c);

   if (ip_data_remaining)
      ip_data_remaining--;

   return(c);
}


int32 ppp_get_ip(void) {
   int8 a,b,c,d;
   int32 ret;

   a=ppp_getc();
   b=ppp_getc();
   c=ppp_getc();
   d=ppp_getc();
   ret=make32(d,c,b,a);

   return(ret);
}

void ppp_put_ip(int32 ip) {
   int8 a,b,c,d;
   a=make8(ip,3);
   b=make8(ip,2);
   c=make8(ip,1);
   d=make8(ip,0);

   ppp_putc(d);
   ppp_putc(c);
   ppp_putc(b);
   ppp_putc(a);
}

int16 ppp_getw(void) {
   int8 lsb,msb;
   int16 ret;

   msb=ppp_getc();
   lsb=ppp_getc();
   ret=make16(msb,lsb);

   return(ret);
}

int32 ppp_get32(void) {
   union {
      int8 b[4];
      int32 f;
   } val;
   val.b[3]=ppp_getc();
   val.b[2]=ppp_getc();
   val.b[1]=ppp_getc();
   val.b[0]=ppp_getc();
   return(val.f);
}

void ppp_put32(int32 f) {
   ppp_putc(make8(f,3));
   ppp_putc(make8(f,2));
   ppp_putc(make8(f,1));
   ppp_putc(make8(f,0));
}

void ppp_putw(int16 w) {
   int8 l,m;

   l=make8(w,0);
   m=make8(w,1);

   ppp_putc(m);
   ppp_putc(l);
}

//functions for the user
void ppp_init(void) {
   //int i;

   MACInit();
   timer_init();
   modem_init();

   _ppp_is_connected=FALSE;
   _ppp_is_connecting=FALSE;

   ppp_state=PPP_STATE_INIT;

#IF PPP_LCP_USE_PAP
   (int8)ppp_option_status=0;
 #if PPP_LCP_PAP_WE_ASK
   ppp_option_status.ask=TRUE;
 #endif
#ENDIF

#IF PPP_LCP_USE_ASYNC_MAP_UPLINK
   ppp_async_uplink.map=0;
   //bit_set(ppp_async_uplink.map,17);
   //bit_set(ppp_async_uplink.map,19);
   ppp_async_uplink.flags.use=FALSE; //will be set to true if they ack our request
   ppp_async_uplink.flags.attempt=TRUE;
#ENDIF

#IF PPP_LCP_USE_ASYNC_MAP_DOWNLINK
   ppp_async_downlink.map=0;
   //bit_set(ppp_async_downlink.map,17);
   //bit_set(ppp_async_downlink.map,19);
   ppp_async_downlink.flags.use=FALSE; //will be set to true if we ack their request
   ppp_async_downlink.flags.attempt=TRUE;
#ENDIF


#if PPP_LCP_USE_PROTOCOL_COMPRESSION
   ppp_attempt_protocol_compression=TRUE;
   ppp_use_protocol_compression=FALSE; //will be set true after he acks our request
#ENDIF

#if PPP_LCP_USE_MRU
   ppp_attempt_send_mru=TRUE;
#endif

#if PPP_LCP_USE_ADDRESS_COMPRESSION
   ppp_attempt_address_compression=TRUE;
   ppp_use_address_compression=FALSE;  //will be set true after he acks our request
#ENDIF


   ppp_server_ip.Val=0;
   MY_IP.Val=0;
   AppConfig.PrimaryDNSServer.Val=0;
   intermediate_dns.Val=0;
   intermediate_ip.Val=0;
   timer_set_s(0);
}

void ppp_data_packet_begin(int16 protocol) {
      tx_cs=0Xffff;
      fputc(PPP_START,MODEM);

      #if PPP_LCP_USE_ADDRESS_COMPRESSION
      if ( (!ppp_use_address_compression) || (protocol==PPP_COMP_PROT_LCP) ) {
         ppp_putc(0xFF);
         ppp_putc(0x03);
      }
      #else
         ppp_putc(0xFF);
         ppp_putc(0x03);
      #endif

     #IF PPP_LCP_USE_PROTOCOL_COMPRESSION
      if ((protocol==PPP_COMP_PROT_IP)&&(ppp_use_protocol_compression)) {
       ppp_putc(0x21);
      }
      else {
       ppp_putw(protocol);
      }
     #ELSE
      ppp_putw(protocol);
     #endif
}


void ppp_option_packet_begin(int16 protocol, int8 code, int8 id, int16 len) {
      ppp_data_packet_begin(protocol);
      ppp_putc(code);
      ppp_putc(id);
      ppp_putw(len+4);
}

void debug_display_ppp_frame(PPP_OPTION_FRAME_HEADER *frame) {
   debug_ppp("\r\nNew PPP Frame %LU %LU %U:  PROT=%LX ", modem_next, modem_last, modem_overrun, frame->protocol);
   if (frame->protocol != PPP_COMP_PROT_IP) {
       debug_ppp("CODE=%X  ID=%X  LEN=%LX",frame->code, frame->id, frame->len);
   }
}

//return true if this is the start of a new frame
int1 ppp_get_frame(PPP_OPTION_FRAME_HEADER *frame) {
   int1 ret=FALSE;
   char c,d;
   static char lastc;

   if (modem_kbhit) {
      c=modem_getb();
      if (c==PPP_START) {
         rx_cs=0xFFFF;

         c=modem_getb();

         //make sure the first START we got wasn't actually the end of the last packet
         if (c==PPP_START) {
            c=ppp_getc();
         }
         else {
            if (c==PPP_ESC) {
               c=modem_getb();
               c^=0x20;
            }
            rx_cs = ppp_cs(rx_cs,c);
         }

         //debug(debug_putc,"%X",c);
         if (c==0xFF) { //strip addr, ctrl.  put first byte of protocol in c
            ppp_getc();
            c=ppp_getc();
         }
         else {
            //possible bug: if you turn on address and control field compression the isp may not send a 0xFF
            return(FALSE);
         }

         if (bit_test(c,0)) {frame->protocol=c;}
         else {
            d=ppp_getc();
            frame->protocol=make16(c,d);
         }

         if (frame->protocol != PPP_COMP_PROT_IP) {
            frame->code=ppp_getc();

            frame->id=ppp_getc();

            frame->len=ppp_getw();
         }
         ret=TRUE;
      }
      else if ((lastc=='N')&&(c=='O')) { //NO CARRIER from modem means we got hung up
         debug_ppp("\r\nNO CARRIER HANGUP");
         modem_disconnect();
         StackInit();   //ppp_init();
         return(FALSE);
      }
      else {
         //debug_putc('%');
      }
      lastc=c;
   }

   return(ret);
}

void ppp_putd(int8 * buff, int16 len) {
   while(len--) {
      ppp_putc(*buff);
      buff++;
   }
}

void ppp_packet_end_tx(void) {
   int8 l,m;

   l=make8(tx_cs,0);
   m=make8(tx_cs,1);

   //dont use ppp_putw() because CS is backwards endian.
   ppp_putc(~l);  //lsb
   ppp_putc(~m);  //msb

   fputc(PPP_END,MODEM);
}

void ppp_getd(int8 * buff, int16 len) {
   while(len--) {
      *buff=ppp_getc();
      buff++;
   }
}


//if you want to do this as a constant table, use this
   const int16 ppp_cs_tab[256] = {
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,   //00-07
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,   //08-0F
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,   //10-17
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,   //18-1f
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,   //20-27
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,   //28-2f
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,   //30-37
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,   //38-3f
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,   //40-47
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,   //48-4f
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,   //50-57
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,   //58-5f
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,   //60-67
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,   //68-6f
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,   //70-77
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,   //78-7f
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,   //80-87
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,   //88-8f
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,   //90-97
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,   //98-9f
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,   //a0-a7
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,   //a8-af
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,   //b0-b7
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,   //b8-bf
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,   //c0-c7
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,   //c8-cf
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,   //d0-d7
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,   //d8-df
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,   //e0-e7
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,   //e8-ef
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,   //f0-f7
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78    //f8-ff
   };

//if you want to use this as a function, use this:
/*
int16 ppp_cs_tab(int8 d) {
   int8 i;
   int16 v;

   v=d;
   for (i = 8; i--; ) {
       v = v & 1 ? (v >> 1) ^ (int16)0x8408 : v >> 1;
   }
   return(v);
}
*/

int16 ppp_cs(int16 cs, int8 c) {
   cs = (cs >> 8) ^ ppp_cs_tab[(cs ^ c) & 0xff];
   return(cs);
}

void ppp_putc_escape(char c) {
        fputc(PPP_ESC,MODEM);
        fputc(c ^ 0x20,MODEM);
}

void ppp_change_state(PPP_STATES new_state, int8 next_event) {
   debug_ppp("\r\nNew State: %X->%X (T=%U)",ppp_state, new_state, next_event);
   ppp_state=new_state;
   timer_set_s(next_event);
}

MODEM_RESP ppp_connect(char * username, char * password, char * phonenumber) {
   MODEM_RESP resp;

  #IF PPP_LCP_USE_PAP
   ppp_pap_username=username;
   ppp_pap_password=password;
  #ENDIF

   resp=modem_connect(phonenumber);

   if (resp==MODEM_CONNECTED) {
      _ppp_is_connecting=TRUE;
   #IF PPP_LOGIN_BEFORE_AUTH
      delay_ms(4000);
      fprintf(MODEM,"%s\n",username);
      delay_ms(2000);
      modem_flush();
      fprintf(MODEM,"%s\n",password);
   #ENDIF
   }

   timer_set_s(0);
   return(resp);
}

//does not automatically disconnect.
//will put the ppp module in a state where it will attempt to disconnect a few times.
//after a few fails it will force a disconnect.
void ppp_disconnect(void) {
   ppp_lcp_terminate_retries=0;
   ppp_change_state(TERMINATE_CLIENT_REQUEST,0);
}

///LCP: only ACK auth:pap, REJ rest
///IPCP: only ACK IP and DNS REQ, REJ rest
void ppp_lcp_ipcp_recv_req(PPP_OPTION_FRAME_HEADER *frame) {
      #DEFINE PPP_LCP_TX_BUFFER_SIZE 100
      char c;
      int8 tx_buffer[PPP_LCP_TX_BUFFER_SIZE];
      int1 new_reject=0;
      int8 tx=0;
      int8 sub_option_type;
      int8 sub_option_len;
      int8 rejects=0;
      int1 ackd_lcp=0;
      int1 finished_lcp=0; //going from lcp to next state of configuration
      int16 cs;
      int16 option_len;
      int8 option_code;
      int16 old_modem_last;
      //int8 a,b;
      int1 is_nak=FALSE;

     #IF PPP_LCP_USE_PAP
      int16 auth_method;
     #endif

      //TODO: if he sends an LCP config request, should we force state back to step 1?
      /*
      if (frame->protocol==PPP_COMP_PROT_LCP) {
         if ((ppp_state!=INIT)&&(ppp_state!=SERVER_ACKED_LCP)&&(ppp_state!=CLIENT_ACKED_LCP)&&(ppp_state!=RUNNING)&&(ppp_state!=TERMINATE_CLIENT_REQUEST)) {
            ppp_change_state(SERVER_ACKED_LCP,1);
         }
      }
      */

      if ((frame->protocol==PPP_COMP_PROT_IPCP)&&(ppp_state==WAIT_FOR_IPCP)) {
         ppp_change_state(FIND_IP,1);
      }

      option_len=frame->len - 4; //will get modified later
      option_code=frame->code;   //will get modified later

      while (option_len) {
         new_reject=0;
         sub_option_type=ppp_getc();
         sub_option_len=ppp_getc();
         if (sub_option_len)
            option_len-=2;
         else
            option_len=0;

         //accept IP config requests on IPCP (this is the IP of the PPP server, not our IP)
         if ((sub_option_type == PPP_OPTION_IPCP_IP)&&(sub_option_len==6)&&(frame->protocol==PPP_COMP_PROT_IPCP)) {
            if (!rejects && !is_nak) {
               debug_ppp("IPCP-ACK-IP[%X] ", sub_option_type);
               ppp_server_ip.Val=ppp_get_ip();
               option_len-=4;
               tx_buffer[tx++]=sub_option_type;
               tx_buffer[tx++]=6;
               tx_buffer[tx++]=make8(ppp_server_ip.Val,0);
               tx_buffer[tx++]=make8(ppp_server_ip.Val,1);
               tx_buffer[tx++]=make8(ppp_server_ip.Val,2);
               tx_buffer[tx++]=make8(ppp_server_ip.Val,3);
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }

         #IF PPP_LCP_USE_PAP
         //accept auth:ppp requests on LCP (only LCP we like)
         else if ((sub_option_type == PPP_OPTION_LCP_AUTH)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            old_modem_last=modem_last;
            auth_method=ppp_getw();
            if ((auth_method != 0xC023)||(sub_option_len!=4)) {  //PAP
             #if PPP_LCP_NAK_AUTH
               if (!rejects) {
                  debug_ppp("LCP-NAK-PAPorCHAP[%X] ", sub_option_type);
                  if (!is_nak) {tx=0;}
                  is_nak=TRUE;
                  tx_buffer[tx++]=PPP_OPTION_LCP_AUTH;
                  tx_buffer[tx++]=4;
                  tx_buffer[tx++]=0xC0;
                  tx_buffer[tx++]=0x23;
               }
               modem_last=old_modem_last;
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
             #else
               debug_ppp(debug_ppp_putc,"LCP-REJ-AUTH[%X] ");
               new_reject=1;
               modem_last=old_modem_last;
             #endif
            }
            else if (!rejects && !is_nak) {
               debug_ppp("LCP-ACK-PAP[%X] ", sub_option_type);
               option_len-=2;
               ppp_option_status.we_acked=TRUE;
               ppp_option_status.use=TRUE;
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_AUTH;
               tx_buffer[tx++]=4;
               tx_buffer[tx++]=0xC0;
               tx_buffer[tx++]=0x23;
            }
            else {
               modem_last=old_modem_last;
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
         #ENDIF

/*
         #IF PPP_LCP_USE_PAP && PPP_LCP_PAP_OR_CHAP
         //accept auth:ppp requests on LCP (only LCP we like)
         else if ((sub_option_type == PPP_OPTION_LCP_AUTH)&&(sub_option_len==5)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            old_modem_last=modem_last;
            auth[0]=ppp_getc();
            auth[1]=ppp_getc();
            auth[2]=ppp_getc();
            if ((auth[0]!=0xC2)||(auth[1]!=0x23)||(auth[2]!=0x05)) {  //PAP or CHAP
               new_reject=1;
               modem_last=old_modem_last;
            }
            else if (!rejects) {
               debug_ppp(debug_ppp_putc,"LCP-NAK-PAPorCHAP[%X] ", sub_option_type);
               if (!is_nak) {tx=0;}
               is_nak=TRUE;
               option_len-=3;
               tx_buffer[tx++]=PPP_OPTION_LCP_AUTH;
               tx_buffer[tx++]=4;
               tx_buffer[tx++]=0xC0;
               tx_buffer[tx++]=0x23;
            }
            else {
               modem_last=old_modem_last;
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
         #ENDIF
*/

        #IF PPP_LCP_USE_ASYNC_MAP_DOWNLINK
         else if ((sub_option_type == PPP_OPTION_LCP_ASYNC_MAP)&&(sub_option_len==6)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            old_modem_last=modem_last;
            if (!rejects && !is_nak) {
               debug_ppp(debug_ppp_putc,"LCP-ACK-ASYNC[%X] ", sub_option_type);
               ppp_async_downlink.map=ppp_get32();
               option_len-=4;
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_ASYNC_MAP;
               tx_buffer[tx++]=6;
               tx_buffer[tx++]=make32(ppp_async_downlink.map,3);
               tx_buffer[tx++]=make32(ppp_async_downlink.map,2);
               tx_buffer[tx++]=make32(ppp_async_downlink.map,1);
               tx_buffer[tx++]=make32(ppp_async_downlink.map,0);
               ppp_async_downlink.flags.use=TRUE;
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }

            /* this looks very wrong
            req_async_map=ppp_get_map(); //will convert big endian to little endian, which matches our map
            my_async_map=ppp_map_to_int32();
            if (!rejects) {
               modem_last=old_modem_last;  //modem_buffer_goback(4,&modem_last);
               new_reject=1;
            }
            else {
               option_len-=4;
               ackd_lcp=TRUE;
               //ppp_attempt_async_map=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_ASYNC_MAP;
               tx_buffer[tx++]=6;
               tx_buffer[tx++]=make8(req_async_map,0);   //back to big endian
               tx_buffer[tx++]=make8(req_async_map,1);
               tx_buffer[tx++]=make8(req_async_map,2);
               tx_buffer[tx++]=make8(req_async_map,3);

               if (my_async_map!=req_async_map) {
                  ppp_async_map_new=TRUE;
                  for (a=0;a<32;a++) {
                     ppp_async_map[a]=shift_left(&req_async_map,4,0);
                  }
               }
            }
            */
            /* before we rejected if our async maps dont match.
            if ((my_async_map==req_async_map)&&(!rejects)) {
               option_len-=4;
               ackd_lcp=TRUE;
               //ppp_attempt_async_map=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_ASYNC_MAP;
               tx_buffer[tx++]=6;
               tx_buffer[tx++]=make8(req_async_map,0);   //back to big endian
               tx_buffer[tx++]=make8(req_async_map,1);
               tx_buffer[tx++]=make8(req_async_map,2);
               tx_buffer[tx++]=make8(req_async_map,3);
            }
            else {
               modem_last=old_modem_last;  //modem_buffer_goback(4,&modem_last);
               ppp_attempt_async_map=FALSE;
               new_reject=1;
            }
            */
         }
        #endif

        #if PPP_LCP_USE_ADDRESS_COMPRESSION
         else if ((sub_option_type == PPP_OPTION_LCP_ACOMP)&&(sub_option_len==2)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            if (!rejects && !is_nak) {
               ppp_debug(ppp_debug_putc,"LCP-ACK-ADCMP[%X] ", sub_option_type);
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_ACOMP;
               tx_buffer[tx++]=2;
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
        #endif

        #if PPP_LCP_USE_PROTOCOL_COMPRESSION
         else if ((sub_option_type == PPP_OPTION_LCP_PCOMP)&&(sub_option_len==2)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            if (!rejects && !is_nak) {
               ppp_debug(ppp_debug_putc,"LCP-ACK-PCMP[%X] ", sub_option_type);
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_PCOMP;
               tx_buffer[tx++]=2;
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
        #endif

        #if PPP_LCP_ANSWER_MAGIC_NUMBER
         else if ((sub_option_type == PPP_OPTION_LCP_MAGIC)&&(sub_option_len==6)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            if (!rejects && !is_nak) {
               ppp_debug(ppp_debug_putc,"LCP-ACK-MAG[%X] ", sub_option_type);
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_MAGIC;
               tx_buffer[tx++]=6;
               tx_buffer[tx++]=ppp_getc();
               tx_buffer[tx++]=ppp_getc();
               tx_buffer[tx++]=ppp_getc();
               tx_buffer[tx++]=ppp_getc();

               option_len-=4;
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
        #endif

        #if PPP_LCP_USE_MRU
         else if ((sub_option_type == PPP_OPTION_LCP_MRU)&&(sub_option_len==4)&&(frame->protocol==PPP_COMP_PROT_LCP)) {
            if (!rejects && !is_nak) {
               debug_ppp(debug_ppp_putc,"LCP-ACK-MRU[%X] ", sub_option_type);
               ackd_lcp=TRUE;
               tx_buffer[tx++]=PPP_OPTION_LCP_MRU;
               tx_buffer[tx++]=4;

               a=ppp_getc();
               b=ppp_getc();

               tx_buffer[tx++]=a;
               tx_buffer[tx++]=b;

               ppp_server_mru=make16(a,b);

               option_len-=2;
            }
            else {
               sub_option_len-=2;
               option_len-=sub_option_len;
               while(sub_option_len--) {
                  ppp_getc();
               }
            }
         }
        #endif

         else {
            new_reject=1;
         }

         if (new_reject && sub_option_len) {//echo back rejected option, but do not go over our internal tx buffer size
            ackd_lcp=0;
            debug_ppp("REJ[%X %X %X]{", frame->protocol, sub_option_type, sub_option_len);
            if (!rejects) {tx=0;}
            rejects++;
            if ((tx + sub_option_len)<PPP_LCP_TX_BUFFER_SIZE) {
               tx_buffer[tx++]=sub_option_type;
               tx_buffer[tx++]=sub_option_len;
               sub_option_len-=2;
               option_len-=sub_option_len;
               while (sub_option_len--) {
                  c=ppp_getc();
                  debug_ppp("%X ",c);
                  tx_buffer[tx++]=c;
               }
               debug_ppp("}");
            }
            else {
               while (option_len--) {
                  ppp_getc();
               }
            }
         }
         if (is_nak) {
            ackd_lcp=FALSE;
         }
      }
      cs=ppp_getw();
      ppp_getc(); //get end

      //TODO: check cs

      //the tx buffer data buffer has been set.
      //now set the option_code
      if (!rejects && !is_nak) {
         option_code=PPP_CODE_ACK;
         if (ackd_lcp) {
            if (ppp_state == SERVER_ACKED_LCP)    //we acked him, he acked us, goto next stage
           {
               #IF PPP_LCP_USE_PAP
                  if (ppp_option_status.use) {
                     ppp_change_state(PPP_AUTH,1);
                  }
                  else {
                      #IF PPP_IPCP_FORCE_AFTER_AUTH
                        ppp_change_state(FIND_IP,1);
                      #ELSE
                        ppp_change_state(WAIT_FOR_IPCP,1);
                      #ENDIF
                  }
               #ELSE
                #IF PPP_IPCP_FORCE_AFTER_AUTH
                  ppp_change_state(FIND_IP,1);
                #ELSE
                  ppp_change_state(WAIT_FOR_IPCP,1);
                #ENDIF
               #ENDIF

               finished_lcp=TRUE;
            }
            else if (ppp_state == PPP_STATE_INIT) {   //need to wait until he acks us
               ppp_change_state(CLIENT_ACKED_LCP,1);
            }
         }
      }
      else if (!rejects) {
         option_code=PPP_CODE_NAK;
      }
      else {
         option_code=PPP_CODE_REJ;
      }

      //now send the response (ACK,NACK,REJ) to the servers packet (tx buffer is now set)
      ppp_option_packet_begin(frame->protocol,option_code,frame->id,tx);
      ppp_putd(tx_buffer,tx);
      ppp_packet_end_tx();
}


void ppp_lcp_ipcp_recv(PPP_OPTION_FRAME_HEADER *frame) {
   int16 option_len;
   int16 cs;
   int8 sub_option_type, sub_option_len;
   int1 throw_away_rest=1;
   //int8 debug_i;


   option_len=frame->len - 4;   //subtract the 4 bytes for code, id and len

   debug_ppp("\r\nGOT LCP/IPCP %X ", frame->protocol);

   /// *** ANSWER REQUESTS
   if (frame->code == PPP_CODE_REQ) {
      debug_ppp("REQ ");
      ppp_lcp_ipcp_recv_req(frame);
      throw_away_rest=0;
   }

  /// *** ANSWER REJECTS
  //we dont send any IPCP requests that the ISP would reject, so dont worry about IPCP rejects
   else if ((frame->code == PPP_CODE_REJ)&&(frame->protocol == PPP_COMP_PROT_LCP)) {   //find out what he rejected, and disable that feature
         debug_ppp("REJ LCP");
         while(option_len) {
            sub_option_type=ppp_getc();
            sub_option_len=ppp_getc();
            if (sub_option_len) {
               option_len-=sub_option_len;   //we will grab this whole sub_option
            }
            else {
               option_len=0;
            }

            sub_option_len-=2;   //we already got type and len

            #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
            if (sub_option_type == PPP_OPTION_LCP_ASYNC_MAP) {if (!ppp_async_uplink.flags.new) ppp_async_uplink.flags.attempt=FALSE;}
            #ENDIF

            #IF PPP_LCP_USE_ADDRESS_COMPRESSION
            if (sub_option_type == PPP_OPTION_LCP_ACOMP) {ppp_attempt_address_compression=FALSE;}
            #ENDIF

            #IF PPP_LCP_USE_PROTOCOL_COMPRESSION
            if (sub_option_type == PPP_OPTION_LCP_PCOMP) {ppp_attempt_protocol_compression=FALSE;}
            #ENDIF

            #if PPP_LCP_USE_MRU
            if (sub_option_type == PPP_OPTION_LCP_MRU) {ppp_attempt_send_mru=FALSE;}
            #ENDIF

            #if PPP_LCP_PAP_WE_ASK
            if (sub_option_type == PPP_OPTION_LCP_AUTH) {ppp_option_status.use=FALSE; ppp_option_status.ask=FALSE; ppp_option_status.isp_acked=FALSE; ppp_option_status.isp_rejected=TRUE;}
            #endif

            while (sub_option_len--) {ppp_getc();}
         }
   }


   /// *** ANSWER ACKS
   else if (frame->code == PPP_CODE_ACK) {
      if (frame->protocol==PPP_COMP_PROT_LCP) { //find out which options he acked
         debug_ppp("ACK LCP");
         while(option_len) {
            sub_option_type=ppp_getc();
            sub_option_len=ppp_getc();
            if (sub_option_len)
               option_len-=sub_option_len;   //we will grab this whole sub_option
            else
               option_len=0;
            sub_option_len-=2;   //we already got type and len

            #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
            //if (sub_option_type == PPP_OPTION_LCP_ASYNC_MAP) {ppp_async_uplink.attempt=TRUE;}
            #ENDIF

            #IF PPP_LCP_USE_ADDRESS_COMPRESSION
            if (sub_option_type == PPP_OPTION_LCP_ACOMP) {ppp_use_address_compression=TRUE;}
            #ENDIF

            #IF PPP_LCP_USE_PROTOCOL_COMPRESSION
            if (sub_option_type == PPP_OPTION_LCP_PCOMP) {ppp_use_protocol_compression=TRUE;}
            #ENDIF

            #if PPP_LCP_PAP_WE_ASK
            if (sub_option_type == PPP_OPTION_LCP_AUTH) {ppp_option_status.ask=TRUE; ppp_option_status.use=TRUE; ppp_option_status.isp_acked=TRUE; ppp_option_status.isp_rejected=FALSE;}
            #endif

            while (sub_option_len--) {ppp_getc();}
         }

         if (ppp_state==CLIENT_ACKED_LCP) { //he acked us, we acked him, therefore goto next stage
            #IF PPP_LCP_USE_PAP
               if (ppp_option_status.use) {
                  ppp_change_state(PPP_AUTH,1);
               }
               else {
                  ppp_change_state(WAIT_FOR_IPCP,1);
               }
            #ELSE
               ppp_change_state(WAIT_FOR_IPCP,1);
            #ENDIF

            #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
               ppp_async_uplink.flags.use=ppp_async_uplink.flags.attempt;  //start using async map now for txing messages
            #ENDIF
         }
         else if (ppp_state==PPP_STATE_INIT) {   //we need to ack their config message before we can go to next stage
            ppp_change_state(SERVER_ACKED_LCP,1);
         }
      }
      else if (frame->protocol==PPP_COMP_PROT_IPCP) { //we requested our new IP address and he liked it
         debug_ppp("ACK IPCP");
         ppp_change_state(RUNNING,PPP_KEEPALIVE_RATE);
         /*debug_ppp("\r\n");
         for (debug_i=0;debug_i<MAX_SOCKETS;debug_i++) {
            debug_ppp("S%U=%X ",debug_i, TCB[debug_i].smState);
         }*/
         MY_IP.Val=intermediate_ip.Val;
         AppConfig.PrimaryDNSServer.Val=intermediate_dns.Val;
         timer_set_s(PPP_KEEPALIVE_RATE);
         _ppp_is_connecting=FALSE;
         _ppp_is_connected=TRUE;
      }
   }

   /// *** ANSWER NAKs
   else if (frame->code == PPP_CODE_NAK) {
      //if (protocol==PPP_COMP_PROT_LCP) { } //we shouldnt get LCP naks
      if (frame->protocol==PPP_COMP_PROT_IPCP) { //we requested a bogus IP address and he sends a new one
         debug_ppp("NACK IPCP");
         throw_away_rest=0;
         while (option_len) {
            sub_option_type=ppp_getc();
            sub_option_len=ppp_getc();
            option_len-=2;
            if ((sub_option_type == PPP_OPTION_IPCP_IP)&&(sub_option_len==6)) {
               if (ppp_state==ECHO_DNS) {ppp_change_state(ECHO_BOTH,1);} else {ppp_change_state(ECHO_IP,1);}
               intermediate_ip.Val=ppp_get_ip();
               option_len-=4;
            }
            else if ((sub_option_type == PPP_OPTION_IPCP_DNS)&&(sub_option_len==6)) {
               if (ppp_state==ECHO_IP) {ppp_change_state(ECHO_BOTH,1);} else {ppp_change_state(ECHO_DNS,1);}
               intermediate_dns.Val=ppp_get_ip();
               option_len-=4;
            }
            else {   //throw away others (there shouldnt be any others, but just in case)
               if (sub_option_len) {
                  sub_option_len-=2;
                  option_len-=sub_option_len;
                  while (sub_option_len) {ppp_getc();}
               }
               else {
                  option_len=0;
               }
            }
         }
         cs=ppp_getw();
         ppp_getc(); //get end
      }
   }

   //he acks our terminate request.  so terminate
   else if ((frame->code == PPP_CODE_TERM_ACK)&&(frame->protocol == PPP_COMP_PROT_LCP)&&(ppp_state==TERMINATE_CLIENT_REQUEST)) {
      debug_ppp("\r\nACK TO TERM");
      modem_disconnect();
      StackInit();   //ppp_init();
      throw_away_rest=0;
   }

   //he requests a terminate.  ack it, then terminate
   else if ((frame->code == PPP_CODE_TERM_REQ)&&(frame->protocol == PPP_COMP_PROT_LCP)) {
      debug_ppp("\r\nACK TERM");
      ppp_option_packet_begin(PPP_COMP_PROT_LCP, PPP_CODE_TERM_ACK, frame->id, 0);
      ppp_packet_end_tx();

      modem_disconnect();
      StackInit();   //ppp_init();
      throw_away_rest=0;
   }

   if (throw_away_rest) {
      option_len+=3; //add back CS and end flag
      while (option_len--) {ppp_getc();}  //throw away rest
   }
}

//gets len from IP header, then returns buffer back to start of IP header
int16 ppp_ip_recv(void) {
   int16 cs;
   int16 len;
   int16 i=4;

   debug_ppp("\r\nGET PPP IP DATA [%LX] ", TickGet());

   ppp_ip_rx_buffer[0]=ppp_getc(); //get vhl
   ppp_ip_rx_buffer[1]=ppp_getc(); //skip service

   len=ppp_getw();
   ppp_ip_rx_buffer[2]=make8(len,1);   //len (big endian)
   ppp_ip_rx_buffer[3]=make8(len,0);   //len (big endian)

   debug_ppp("%X %X L=%LU ...", ppp_ip_rx_buffer[0], ppp_ip_rx_buffer[1], len);

   while(i<len) {
      ppp_ip_rx_buffer[i]=ppp_getc();
      i++;
   }

   cs=ppp_getw();    //get CS
   i=ppp_getc();  //get ppp end marker

   debug_ppp(" [%LX]", TickGet());

   return(len);
}

#IF PPP_LCP_USE_PAP
void ppp_pap_recv(PPP_OPTION_FRAME_HEADER *frame) {
   int16 option_len;
   option_len=frame->len - 1;   //subtract the 4 bytes for code, id and len, but then add 3 bytes for CS and end flag

   if (frame->code == PPP_CODE_ACK) {
     #if PPP_IPCP_FORCE_AFTER_AUTH
      ppp_change_state(FIND_IP,1);
     #else
      ppp_change_state(WAIT_FOR_IPCP,1);
     #endif
   }

   //TODO: check cs
   while (option_len--) {ppp_getc();}
}
#ENDIF


void ppp_keepalive(void) {
   //i guess just send an empty ip packet
   ppp_data_packet_begin(PPP_COMP_PROT_IP);
   ppp_packet_end_tx();
}


int16 ppp_handle(void) {
   PPP_OPTION_FRAME_HEADER ppp_frame;
   static int16 dcd_count=0;
   char c;

   int8 option_len;
   int8 username_len, password_len;

   static int8 lcp_id=1, pap_id=1, ipcp_id=1;
   ip_data_remaining=0;

  if (ppp_get_frame(&ppp_frame)) {
      debug_ppp("\r\nPPP RX FRAME %LX [%LX]", ppp_frame.protocol, TickGet());
      debug_display_ppp_frame(&ppp_frame);
      switch (ppp_frame.protocol) {
            case PPP_COMP_PROT_LCP :
            case PPP_COMP_PROT_IPCP :           ppp_lcp_ipcp_recv(&ppp_frame);  break;
            case PPP_COMP_PROT_IP :             ip_data_remaining=ppp_ip_recv();    break;
          #IF PPP_LCP_USE_PAP
            case PPP_COMP_PROT_PAP :            ppp_pap_recv(&ppp_frame);   break;
          #ENDIF
            case PPP_COMP_PROT_CCP :
            default:
                  debug_ppp("\r\nDISCARDING... [%LX] ",TickGet());
                  do {
                     //debug_putc('$');
                     c=modem_getb();
                  } while (c!=PPP_END && !_modem_getb_timeout && !input(MODEM_DCD));
                  debug_ppp(" [%LX]", TickGet());
               break;
         }
   }

   if (timer_event() && connected_baudrate) {
      if ((ppp_state==PPP_STATE_INIT)||(ppp_state==CLIENT_ACKED_LCP)) {         //send out LCP config requests every 2 seconds until he acks us

         option_len=0;

        #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
         if (ppp_async_uplink.flags.attempt) {option_len+=6;}
        #ENDIF

         #if PPP_LCP_USE_ADDRESS_COMPRESSION
         if (ppp_attempt_address_compression) {option_len+=2;}
         #ENDIF

         #IF PPP_LCP_USE_PROTOCOL_COMPRESSION
         if (ppp_attempt_protocol_compression) {option_len+=2;}
         #ENDIF

         #if PPP_LCP_USE_MRU
          if (ppp_attempt_send_mru) {
             option_len+=4;
          }
         #endif

         #if PPP_LCP_PAP_WE_ASK
         if (ppp_option_status.ask) {
            option_len+=4;
         }
         #endif

         ppp_option_packet_begin(PPP_COMP_PROT_LCP, PPP_CODE_REQ, lcp_id++, option_len);

         #if PPP_LCP_PAP_WE_ASK
         if (ppp_option_status.ask) {
            ppp_putc(PPP_OPTION_LCP_AUTH);
            ppp_putc(4);
            ppp_putc(0xC0);
            ppp_putc(0x23);
         }
         #endif

         #if PPP_LCP_USE_MRU
         if (ppp_attempt_send_mru) {
            ppp_putc(PPP_OPTION_LCP_MRU);
            ppp_putc(4);   //len
            ppp_putc(make8(MAC_RX_BUFFER_SIZE,1));
            ppp_putc(make8(MAC_RX_BUFFER_SIZE,0));
         }
         #endif

        #IF PPP_LCP_USE_ASYNC_MAP_UPLINK
         if (ppp_async_uplink.flags.attempt) {
            ppp_async_uplink.flags.new=FALSE;
            ppp_putc(PPP_OPTION_LCP_ASYNC_MAP);
            ppp_putc(6);
            ppp_put32(ppp_async_uplink.map);
         }
        #ENDIF

         #IF PPP_LCP_USE_PROTOCOL_COMPRESSION
         if (ppp_attempt_protocol_compression) {
            ppp_putc(PPP_OPTION_LCP_PCOMP);     //type
            ppp_putc(2);   //len
         }
         #ENDIF

         #if PPP_LCP_USE_ADDRESS_COMPRESSION
         if (ppp_attempt_address_compression) {
            ppp_putc(PPP_OPTION_LCP_ACOMP);     //type
            ppp_putc(2);   //len
         }
         #ENDIF

         ppp_packet_end_tx();
         debug_ppp("\r\nSENT LCP");
         timer_set_s(1);
      }
      else if (ppp_state==PPP_AUTH) {
         //send out PAP login
        #IF PPP_LCP_USE_PAP

         username_len=strlen(ppp_pap_username);
         password_len=strlen(ppp_pap_password);

         ppp_option_packet_begin(PPP_COMP_PROT_PAP, PPP_CODE_REQ, pap_id++, 2 + username_len + password_len);
         ppp_putc(username_len);
         ppp_putd(ppp_pap_username,username_len);
         ppp_putc(password_len);
         ppp_putd(ppp_pap_password,password_len);
         ppp_packet_end_tx();
         timer_set_s(1);
         debug_ppp("\r\nSENT AUTH");
        #ELSE
         debug_ppp(debug_ppp_putc,"\r\nSKIP AUTH");
         ppp_change_state(WAIT_FOR_IPCP,1);   //for some reason we got in this state. immediately send us to the next state
        #ENDIF
      }
      else if ((ppp_state==ECHO_IP)||(ppp_state==ECHO_BOTH)||(ppp_state==FIND_IP)) {
         //send out IPCP config request with IP=0.0.0.0 (FIND_IP) or with the IP they sent us (ECHO_IP, ECHO_BOTH)
         ppp_option_packet_begin(PPP_COMP_PROT_IPCP, PPP_CODE_REQ, ipcp_id++, 12);
         ppp_putc(PPP_OPTION_IPCP_IP);     //type
         ppp_putc(6);   //len
         ppp_put_ip(intermediate_ip.Val);
         ppp_putc(PPP_OPTION_IPCP_DNS);     //type
         ppp_putc(6);   //len
         ppp_put_ip(intermediate_dns.Val);
         ppp_packet_end_tx();
         debug_ppp("\r\nSENT IPCP");
         timer_set_s(1);
      }
      else if (ppp_state==RUNNING) {
         //ppp_keepalive();
         timer_set_s(PPP_KEEPALIVE_RATE);
      }
      else if (ppp_state==TERMINATE_CLIENT_REQUEST) {
         if (ppp_lcp_terminate_retries < PPP_LCP_MAX_TERMINATE_RETRIES) {
            ppp_lcp_terminate_retries++;
            ppp_option_packet_begin(PPP_COMP_PROT_IPCP, PPP_CODE_TERM_REQ, lcp_id++, 0);
            ppp_packet_end_tx();
            debug_ppp("\r\nSENT TERMINATE");
            timer_set_s(2);  //wait 2s for him to ack us
         }
         else {
            debug_ppp("\r\nFORCE TERMINATE");
            modem_disconnect();
            StackInit();   //ppp_init();
         }
      }
      else {
         timer_disable();
      }
   }
   if (connected_baudrate && input(MODEM_DCD)) {
      dcd_count++;
      if (dcd_count==0xFFFF) {
         debug_ppp("\r\nLOST DCD");

         StackInit();
      }
   }
   else {
      dcd_count=0;
   }
   return(ip_data_remaining);
}

