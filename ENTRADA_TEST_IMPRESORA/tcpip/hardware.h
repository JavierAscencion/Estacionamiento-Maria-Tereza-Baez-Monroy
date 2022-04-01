///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                         HARDWARE.H                                ////
////                                                                   ////
//// Hardware I/O definitions and TCP/IP stack configuration settings. ////
////                                                                   ////
//// These values will probably change with each application.          ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
//// RELEASE HISTORY:                                                  ////
////                                                                   ////
////    Jan 15, 2004: MODEM_RESPONSE_TIMEOUT and MODEM_CONNECT_TIMEOUT ////
////                  moved to here.                                   ////
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

#IFNDEF ___TCPIP_STACK_CONFIGURATION
#define ___TCPIP_STACK_CONFIGURATION


#ifndef STACK_USE_CCS_PICNET
#define STACK_USE_CCS_PICNET  FALSE
#endif

#ifndef STACK_USE_CCS_PICENS
#define STACK_USE_CCS_PICENS  FALSE
#endif

/*
#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#if STACK_USE_CCS_PICNET || STACK_USE_CCS_PICEEC || STACK_USE_CCS_EWL3V
 #use fast_io(F)
#endif


//// VARIOUS MODEM SETTINGS.
   #DEFINE  MODEM_DCD         PIN_G3
   #DEFINE  MODEM_RESET       PIN_G4
   #define  MODEM_TX          PIN_G1
   #define  MODEM_RX          PIN_G2
   #DEFINE  MODEM_INIT_STR    "ATM1L3&K0"   //speaker on, volume high, no hw flow control
   #DEFINE  MODEM_DIAL_STR    "ATDT"
   #DEFINE  MODEM_BAUD_RATE   115200

   #DEFINE  MODEM_RESPONSE_TIMEOUT  2000     //time to wait for a response to an AT command (in ms)
   #DEFINE  MODEM_CONNECT_TIMEOUT   120000    //time to wait for modem to make a connection (in ms)


//// VARIOUS MAC/NIC SETTINGS.
      #define PIN_ENC_MAC_SO  PIN_C4   // PIC <<<< ENC
      #define PIN_ENC_MAC_SI  PIN_C5   // PIC >>>> ENC
      #define PIN_ENC_MAC_CLK PIN_C3
      #define PIN_ENC_MAC_CS  PIN_C0
      #define PIN_ENC_MAC_RST PIN_C1
      #define PIN_ENC_MAC_INT PIN_C6
      #define PIN_ENC_MAC_WOL PIN_C7
      #define ENC_MAC_USE_SPI TRUE      //due to an errata in the ENC28J60, you should always use HW SPI to assure that SPI clock is over 8MHz!
      #define mac_enc_spi_tris_init()//  TRISB=(TRISB | 0b11); TRISC = (TRISC & 0b11010111) | 0x10; TRISD=TRISD & 0xF4//was 0xFC
     
   #if STACK_USE_CCS_PICNET
      //Latch and Directional control SFR locations for the 3 control pins
      #define NIC_RESET_LAT       LATE_RE7
      #define NIC_IOW_LAT         LATE_RE6
      #define NIC_IOR_LAT         LATE_RE5
      #define NIC_RESET_TRIS      TRISE_RE7
      #define NIC_IOW_TRIS        TRISE_RE6
      #define NIC_IOR_TRIS        TRISE_RE5

      //Latch and Directional control SFR locations for the 5bit address port
      #define NIC_ADDR_LAT        LATE
      #define NIC_ADDR_TRIS       TRISE

      //Latch, Directional and I/O SFR locations for the 8bit data port
      #define NIC_DATA_IO         PORTF
      #define NIC_DATA_LAT        LATF
      #define NIC_DATA_TRIS       TRISF

      //This macro takes an address and properly outputs it on the latch register, and sets proper pins to output.
      //Leaves other pins alone.
      #define WRITE_NIC_ADDR(a)   NIC_ADDR_LAT = (NIC_ADDR_LAT & 0xE0)|a; \
                                NIC_ADDR_TRIS = NIC_ADDR_TRIS & 0xE0
   #elif STACK_USE_MCPENC
      #define PIN_ENC_MAC_SO  PIN_C4   // PIC <<<< ENC
      #define PIN_ENC_MAC_SI  PIN_C5   // PIC >>>> ENC
      #define PIN_ENC_MAC_CLK PIN_C3
      #define PIN_ENC_MAC_CS  PIN_D1
      #define PIN_ENC_MAC_RST PIN_D0
      #define PIN_ENC_MAC_INT PIN_B0
      #define PIN_ENC_MAC_WOL PIN_B1
      #define ENC_MAC_USE_SPI TRUE      //due to an errata in the ENC28J60, you should always use HW SPI to assure that SPI clock is over 8MHz!
      #define mac_enc_spi_tris_init()  TRISB=(TRISB | 0b11); TRISC = (TRISC & 0b11010111) | 0x10; TRISD=TRISD & 0xF4//was 0xFC
   #elif STACK_USE_CCS_EWL5V
      #define PIN_ENC_MAC_SO  PIN_C4   // PIC <<<< ENC
      #define PIN_ENC_MAC_SI  PIN_C5   // PIC >>>> ENC
      #define PIN_ENC_MAC_CLK PIN_C3
      #define PIN_ENC_MAC_CS  PIN_A4
      #define PIN_ENC_MAC_RST PIN_B5
      #define PIN_ENC_MAC_INT PIN_B2
      #define PIN_ENC_MAC_WOL PIN_B3
      #define ENC_MAC_USE_SPI TRUE      //due to an errata in the ENC28J60, you should always use HW SPI to assure that SPI clock is over 8MHz!
      #define mac_enc_spi_tris_init()  TRISB=(TRISB | 0b00001100)&0b11011111; TRISC = (TRISC & 0b11010111) | 0x10; TRISA=TRISA & 0b11101111
   #else
      #error Please define your MAC/NIC I/O settings
   #endif
*/


//// SET TCP_NO_WAIT_FOR_ACK TO FALSE IF TCP STACK SHOULD WAIT FOR ACK FROM
//// REMOTE HOST BEFORE TRANSMITTING ANOTHER PACKET.  THIS MAY REDUCE THROUGHPUT.
//// DEFAULT VALUE (TRUE) GETS LOADED IN TCP.H IF THIS LINE IS REMOVED.
   #define TCP_NO_WAIT_FOR_ACK   FALSE


///DEFAULT HARDCODED IP ADDRESSES.
///  FUTURE APPLICATIONS MAY WANT TO SAVE THESE TO AN EEPROM.
///  OR USE AUTO IP ASSIGNMENT (DHCP).
///  NO TWO DEVICES ON A NETwORK CAN HAVE THE SAME IP ADDRESS
   #define MY_DEFAULT_IP_ADDR_BYTE1        10   //IP ADDRESS
   #define MY_DEFAULT_IP_ADDR_BYTE2        10   // This unit's IP address.
   #define MY_DEFAULT_IP_ADDR_BYTE3        5
   #define MY_DEFAULT_IP_ADDR_BYTE4        15

   #define MY_DEFAULT_MASK_BYTE1           0xff //NETMASK
   #define MY_DEFAULT_MASK_BYTE2           0xff // Netmask tells the IP / ARP stack which
   #define MY_DEFAULT_MASK_BYTE3           0xff // IP's are on your local network.
   #define MY_DEFAULT_MASK_BYTE4           0x00

   #define MY_DEFAULT_GATE_BYTE1           192  //GATEWAY IP ADDRESS
   #define MY_DEFAULT_GATE_BYTE2           168  // Gateway acts as a conduit between two networks.
   #define MY_DEFAULT_GATE_BYTE3           100
   #define MY_DEFAULT_GATE_BYTE4           1

///DEFAULT HARDCODED MAC ADDRESS.
///  FUTURE APPLICATIONS MAY WANT TO SAVE THIS TO AN EEPROM, OR GENERATE
///  A DYNAMIC ONE BASED UPON UNIT'S SERIAL NUMBER.
///  NO TWO DEVICES ON THE SAME ETHERNET NETWORK CAN HAVE THE SAME MAC ADDRESS.
#define MY_DEFAULT_MAC_BYTE1            0x00
#define MY_DEFAULT_MAC_BYTE2            0x04
#define MY_DEFAULT_MAC_BYTE3            0xa3
#define MY_DEFAULT_MAC_BYTE4            0x00
#define MY_DEFAULT_MAC_BYTE5            0x00
#define MY_DEFAULT_MAC_BYTE6            0x00

///Maximum sockets to be defined.
/// Note that each socket consumes 36 bytes of RAM.
/// If you remove this, a default value will be loaded in stacktsk.h
   #ifndef MAX_SOCKETS
   #define MAX_SOCKETS                     35
   #endif

///Avaialble UDP Socket
/// DCHP takes 1 socket.
/// If you remove this, a default value will be loaded in stacktsk.h
   #ifndef MAX_UDP_SOCKETS
   #define MAX_UDP_SOCKETS                 2
   #endif

///BUFFER SIZE DEFINITIONS
///
/// For SLIP, there can only be one transmit and one receive buffer.
/// Both buffer must fit in one bank.  If bigger buffer is required,
/// you must manually locate tx and rx buffer in different bank
/// or modify your linker script file to support arrays bigger than
/// 256 bytes.
/// I think Microchip needs MAC_RX_BUFFER_SIZE to equal MAC_TX_BUFFER_SIZE
///
/// For PPP, there can only be one transmit and one receive buffer.
/// You can receive messages larger than the receive buffer if your
/// routines are fast enough.  You cannot transmit messages larger
/// than the TX buffer.  The larger the buffer you can make, the better.
/// BUG: MAC_RX_BUFFER_SIZE must equal MAC_TX_BUFFER_SIZE
///
/// For Ethernet, the Ethernet controler has many buffers that are
/// 1k in size.   Only one buffer is used for TX, rest are for RX.
/// Unlike SLIP and PPP, no RAM is used for these buffers.
   #if STACK_USE_MAC
       #define MAC_TX_BUFFER_SIZE          1024 //do not modify this line
       #define MAC_TX_BUFFER_COUNT         1    //do not modify this line
   #elif STACK_USE_PPP
       #define MAC_TX_BUFFER_SIZE          1024
       #define MAC_TX_BUFFER_COUNT         1
   #elif STACK_USE_SLIP
       #define MAC_TX_BUFFER_SIZE          250
       #define MAC_TX_BUFFER_COUNT         1
   #endif

   #define MAC_RX_BUFFER_SIZE              MAC_TX_BUFFER_SIZE  //do not modify this line unless you are certain you know what you're doing

#endif
