///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                             PPP.H                                 ////
////                                                                   ////
//// Definitions and prototypes used with PPP.C                        ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
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

#IFNDEF __TCPIP_STACK_PPP_MODULE_H
#DEFINE __TCPIP_STACK_PPP_MODULE_H

#define PPP_LCP_MAX_TERMINATE_RETRIES  5

#define PPP_KEEPALIVE_RATE 20 //in seconds

///END USER CONFIG


///DEFINES

typedef struct __PPP_OPTION_FRAME_HEADER
{
   int16 protocol;
   int8  code;
   int8  id;
   int16 len;
} PPP_OPTION_FRAME_HEADER;

int8 ppp_ip_rx_buffer[MAC_RX_BUFFER_SIZE];

#define PPP_COMP_PROT_LCP     0xC021
#define PPP_COMP_PROT_IPCP    0x8021
#define PPP_COMP_PROT_CCP     0x80FD
#define PPP_COMP_PROT_PAP     0xC023

#define PPP_COMP_PROT_IP      0x0021

#define PPP_OPTION_LCP_MRU          1  //max size of ppp info field
#define PPP_OPTION_LCP_ASYNC_MAP    2  //control chars that must be escaped
#define PPP_OPTION_LCP_AUTH         3  //choice of protocol for authentication
#define PPP_OPTION_LCP_MAGIC        5  //random number for loop-back detection
#define PPP_OPTION_LCP_PCOMP        7  //protocol field compression
#define PPP_OPTION_LCP_ACOMP        8  //address and field compression

#define PPP_OPTION_IPCP_IPCOMP   2  //ip header compression (we dont do this)
#define PPP_OPTION_IPCP_IP       3  //ip adress
#define PPP_OPTION_IPCP_DNS      0x81  //primary dns

#define PPP_CODE_REQ       1
#define PPP_CODE_ACK       2
#define PPP_CODE_NAK       3
#define PPP_CODE_REJ       4
#define PPP_CODE_TERM_REQ  5
#define PPP_CODE_TERM_ACK  6

#define PPP_END         0x7E
#define PPP_START       0x7E
#define PPP_ESC         0x7D

#IFNDEF PPP_LOGIN_BEFORE_AUTH
   #DEFINE PPP_LOGIN_BEFORE_AUTH FALSE
#ENDIF

#ifndef PPP_LCP_USE_PAP
   #define PPP_LCP_USE_PAP     TRUE
   #define PPP_LCP_PAP_WE_ASK  FALSE
   #define PPP_LCP_NAK_AUTH   TRUE
#endif

//request and use ASYNC map pic->server
#ifndef PPP_LCP_USE_ASYNC_MAP_UPLINK
   #define PPP_LCP_USE_ASYNC_MAP_UPLINK   FALSE //MAY BE BROKEN!!!!
#endif

//ack and use ASYNC map pic<-server
#ifndef PPP_LCP_USE_ASYNC_MAP_DOWNLINK
   #define PPP_LCP_USE_ASYNC_MAP_DOWNLINK   FALSE
#endif

#ifndef PPP_LCP_USE_PROTOCOL_COMPRESSION
   #define PPP_LCP_USE_PROTOCOL_COMPRESSION FALSE
#endif

#ifndef PPP_LCP_USE_MRU
   #define PPP_LCP_USE_MRU FALSE //this is buggy, do not use
#endif

#ifndef PPP_LCP_USE_ADDRESS_COMPRESSION
   #define PPP_LCP_USE_ADDRESS_COMPRESSION FALSE
#endif

#ifndef PPP_IPCP_FORCE_AFTER_AUTH
   #define PPP_IPCP_FORCE_AFTER_AUTH   TRUE
#endif

#ifndef PPP_LCP_ANSWER_MAGIC_NUMBER
   #define PPP_LCP_ANSWER_MAGIC_NUMBER   FALSE
#endif


///ppp_handle() is the ppp state machine that must be called often.  it will return a number if there is an IP packet
//to be read.  the state machine happens like this:
// 1. INIT -   If modem is connected, send out LCP config requests every second.
//             Wait for ACK to this message, as well as wait for server to send us a message which we must ACK
// 2. AUTH -   After ACKing their message and server ACKs our LCP config message, send out PAP message
//             every second.  If we are not using PAP we will skip this.
// 3. WAIT_FOR_IPCP -   Wait until we ACK server's IPCP config request
// 4. FIND_IP  -  Send out IPCP config req with IP and DNS of 0 to suggest to PPP server we dont know our IP, every second
// 5. ECHO_IP or ECHO_BOTH -  Server sent us a NAK with our new IP and maybe DNS.
//                            Send out IPCP config requests every second with this new IP and maybe DNS.
// 6. RUNNING - PPP server ackd our request to use the IP that they sent us before.
// 7. TERMINATE_CLIENT_REQUEST - We tell the server we want to terminate / close the connection
// 8. TERMINATE_CLIENT_WAIT - Wait a short time for server to ack our terminate request
typedef enum __PPP_STATES {PPP_STATE_INIT, SERVER_ACKED_LCP, CLIENT_ACKED_LCP, PPP_AUTH, WAIT_FOR_IPCP,
 FIND_IP, ECHO_IP, ECHO_DNS, ECHO_BOTH, RUNNING, TERMINATE_CLIENT_REQUEST} PPP_STATES;

//global variables

PPP_STATES ppp_state;

IP_ADDR ppp_server_ip;
IP_ADDR intermediate_dns;
IP_ADDR intermediate_ip;

int16 ip_data_remaining;   //amount of IP packet is left in the RX buffer

int1 _ppp_is_connected;
int1 _ppp_is_connecting;

#IF PPP_LCP_USE_PAP
struct {
   int1 use;
   int1 ask;
   int1 isp_acked;
   int1 isp_rejected;
   int1 we_acked;
} ppp_option_status;

 char * ppp_pap_password;
 char * ppp_pap_username;
#ENDIF

typedef struct {
   int32 map;
   struct {
      int1 use;
      int1 attempt;
      int1 new;
   } flags;
} PPP_ASYNC_CONFIG_STRUCT;

#IF PPP_LCP_USE_ASYNC_MAP_UPLINK
   PPP_ASYNC_CONFIG_STRUCT ppp_async_uplink;
#ENDIF

#IF PPP_LCP_USE_ASYNC_MAP_DOWNLINK
   PPP_ASYNC_CONFIG_STRUCT ppp_async_downlink;
#ENDIF

#if PPP_LCP_USE_PROTOCOL_COMPRESSION
   int1 ppp_attempt_protocol_compression;
   int1 ppp_use_protocol_compression;
#ENDIF

#if PPP_LCP_USE_ADDRESS_COMPRESSION
   int1 ppp_attempt_address_compression;
   int1 ppp_use_address_compression;
#ENDIF

#if PPP_LCP_USE_MRU
   int1  ppp_attempt_send_mru;
   int16 ppp_server_mru;
#endif

int8 ppp_lcp_terminate_retries;

int16 tx_cs;
int16 rx_cs;


/// CODE


//prototypes
void ppp_init(void);
int32 ppp_get_ip(void);
void ppp_put_ip(int32 ip);
int16 ppp_getw(void);
void ppp_putw(int16 w);

void ppp_option_packet_begin(int16 protocol, int8 code, int8 id, int16 len);
void ppp_data_packet_begin(int16 protocol);
int1 ppp_get_frame(PPP_OPTION_FRAME_HEADER *frame);
void ppp_putd(int8 * buff, int16 len);
void ppp_getd(int8 * buff, int16 len);
void ppp_packet_end_tx(void);
int16 ppp_cs(int16 cs, int8 c);
void ppp_putc(char c);
void ppp_putc_escape(char c);
char ppp_getc(void);
void ppp_change_state(PPP_STATES new_state, int8 next_event);
MODEM_RESP ppp_connect(char * username, char * password, char * phonenumber);
void ppp_disconnect(void);
void ppp_lcp_ipcp_recv_req(PPP_OPTION_FRAME_HEADER *frame);
void ppp_lcp_ipcp_recv(PPP_OPTION_FRAME_HEADER *frame);
int16 ppp_ip_recv(void);
void ppp_pap_recv(PPP_OPTION_FRAME_HEADER *frame);
int16 ppp_handle(void);
void ppp_packet_end(void);
void ppp_keepalive(void);

//ppp_packet_end

#ENDIF

/// FROM RFC 1700, HERE ARE ALL THE PROTOCOL FIELDS
/*
0001            Padding Protocol
0003 to 001f    reserved (transparency inefficient)
0021            Internet Protocol
0023            OSI Network Layer
0025            Xerox NS IDP
0027            DECnet Phase IV
0029            Appletalk
002b            Novell IPX
002d            Van Jacobson Compressed TCP/IP
002f            Van Jacobson Uncompressed TCP/IP
0031            Bridging PDU
0033            Stream Protocol (ST-II)
0035            Banyan Vines
0037            reserved (until 1993)
0039            AppleTalk EDDP
003b            AppleTalk SmartBuffered
003d            Multi-Link
003f            NETBIOS Framing
0041            Cisco Systems
0043            Ascom Timeplex
0045            Fujitsu Link Backup and Load Balancing (LBLB)
0047            DCA Remote Lan
0049            Serial Data Transport Protocol (PPP-SDTP)
004b            SNA over 802.2
004d            SNA
004f            IP6 Header Compression
006f            Stampede Bridging
007d            reserved (Control Escape)             [RFC 1661std51]
007f            reserved (compression inefficient)    [RFC 1662std51]
00cf            reserved (PPP NLPID)
00fb            compression on single link in multilink group
00fd            1st choice compression

00ff            reserved (compression inefficient)

0201            802.1d Hello Packets
0203            IBM Source Routing BPDU
0205            DEC LANBridge100 Spanning Tree
0231            Luxcom
0233            Sigma Network Systems

8001-801f       Not Used - reserved                   [RFC 1661std51]
8021            Internet Protocol Control Protocol
8023            OSI Network Layer Control Protocol
8025            Xerox NS IDP Control Protocol
8027            DECnet Phase IV Control Protocol
8029            Appletalk Control Protocol
802b            Novell IPX Control Protocol
802d            reserved
802f            reserved
8031            Bridging NCP
8033            Stream Protocol Control Protocol
8035            Banyan Vines Control Protocol
8037            reserved till 1993
8039            reserved
803b            reserved
803d            Multi-Link Control Protocol
803f            NETBIOS Framing Control Protocol
807d            Not Used - reserved                   [RFC 1661std51]
8041            Cisco Systems Control Protocol
8043            Ascom Timeplex
8045            Fujitsu LBLB Control Protocol
8047            DCA Remote Lan Network Control Protocol (RLNCP)
8049            Serial Data Control Protocol (PPP-SDCP)
804b            SNA over 802.2 Control Protocol
804d            SNA Control Protocol
804f            IP6 Header Compression Control Protocol
006f            Stampede Bridging Control Protocol
80cf            Not Used - reserved                   [RFC 1661std51]
80fb            compression on single link in multilink group control
80fd            Compression Control Protocol
80ff            Not Used - reserved                   [RFC 1661std51]

c021            Link Control Protocol
c023            Password Authentication Protocol
c025            Link Quality Report
c027            Shiva Password Authentication Protocol
c029            CallBack Control Protocol (CBCP)
c081            Container Control Protocol                  [KEN]
c223            Challenge Handshake Authentication Protocol
c281            Proprietary Authentication Protocol         [KEN]

c26f            Stampede Bridging Authorization Protocol
c481            Proprietary Node ID Authentication Protocol [KEN]
*/
