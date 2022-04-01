//////////////////////////////////////////////////////////////////////////////
///
///                              TFTP.C
///
/// Implements a TFTP server.  Only allows file transfers to the server.
/// Transfers from the server to the client are not allowed.
///
/// Based loosely on the CCS implementation of the Microchip FTP server.
/// **** CONFIGURATION ****
///
/// STACK_USE_TFTP - Define this to be true before you include stacktsk.c
///        in your application.  Defining this to be true will cause
///        the stack to include the TFTP portion and execute the init
///        and process any TFTP tasks.
///
/// TFTP_PORT - The TCP/IP port the TFTP server will listen to for TFTP
///        connections.  Port 69 is almost exclusively used for TFTP traffic
///
/// TFTP_NUM_SOCKETS - Number of sockets the stack will open for the
///        TFTP server.  The more sockets you use the more RAM 
///        is used to hold buffers and state configuration.
///
/// TFTP_MAX_HEADER_LENGTH - Maximum length of the TFTP header portion of the
///        incoming packet.  This does not include the data in a TFTP data packet
///
/// TFTP_MAX_DATA_LENGTH - Defines the length of the data portion of a complete
///        TFTP data packet.  This should generally not be modified unless you
///        really know what you are doing.  TFTP uses this length to determine
///        when to terminate a file transfer connection. 
///        TFTP specification says that if a TFTP datagram is less than 516 bytes
///        in length, then it is the last packet of a file transfer.
///
///
/// **** HOW IT WORKS ****
/// TFTP is a very simple (trivial) form of the File Transfer Protocol (FTP).  
/// TFTP is designed to operate on top of the UDP protocol (although it isn't
/// required to do so).  Only requests for file writing from the client 
/// to the server are granted by the server.  Write requests will automatically
/// overwrite an existing file on the server's MMC (no append).  In the case
/// that a file does not exist, one will be created.  Specify the path and
/// file name in the destination field of your TFTP client.  Filenames without
/// a path will be stored in the root directory of the MMC.  Keep filenames
/// to the DOS-style (8.3) format.  All files should be sent in octec (binary,
/// image) mode (-i option in Windows' command-line TFTP program).
///
///
////////////////////////////////////////////////////////////////////////////////
///
/// * Author         Date              Comment
/// *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// * Nick LaBonte   Feb 15 2007       Initial version
/// * Nick LaBonte   Feb 22 2007       MPFS support, cleanup
///
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2007 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////
#ifndef THIS_IS_TFTP_C
#define THIS_IS_TFTP_C
#include "tcpip/tftp.h"

static TICKTYPE             TFTPlastActivity;   // Timeout keeper.

UDP_SOCKET       TFTPSocket;
SM_TFTP          smTFTP;
char             TFTPString[TFTP_MAX_HEADER_LENGTH];
char             TFTPbuf[TFTP_BUFFER_SIZE];
int16            last_block;
int16            last_sent;
int1             bad_block=0;   

int16            last_mem_begin;
int32            end_of_mpfs;
int32            TFTPaddy;
//int16           MPFS_Size=0;

TFTP_EC          TFTPec;

FILE             TFTPfstream;
char             TFTPfilename[15];

int1             TFTPconnected = 0;
int1             received_final_packet=0;

void execute_tftp_command(void);
void send_data(void);


/*********************************************************************
 * Function:        void TFTPInit(void)
 *
 * PreCondition:    UDP module is already initialized.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Opens a UDP socket for use by TFTP
 *
 * Note:
 ********************************************************************/
void TFTPInit(void)
{
    TFTPSocket       = UDPOpen(TFTP_PORT, NULL, INVALID_UDP_SOCKET);
    smTFTP           = SM_TFTP_WAIT;
}


/*********************************************************************
 * Function:        void TFTPTask(void)
 *
 * PreCondition:    TFTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened TFTP connections are served.
 *
 * Side Effects:    None
 *
 * Overview:
 *
 * Note:            This function acts as a task (similar to one in
 *                  RTOS).  This function performs its task in
 *                  co-operative manner.  Main application must call
 *                  this function repeatdly to ensure all open
 *                  or new connections are served on time.
 ********************************************************************/
void TFTPTask(void)
{  

   BYTE b[4];
   //int16 i=0;
   TICKTYPE currentTick;
    
   switch(smTFTP){
   
   case SM_TFTP_WAIT:
      if(UDPIsGetReady(TFTPSocket)){
         TFTPconnected = 1;
         TFTPlastActivity  = TickGet();
         execute_tftp_command();
         UDPDiscard();
      }else if(TFTPconnected){
         currentTick = TickGet();
         currentTick = TickGetDiff(currentTick, TFTPlastActivity);
         if ( currentTick >= TFTP_TIMEOUT )
         {
             TFTPlastActivity                = TickGet();
             if(received_final_packet){
                smTFTP                      = SM_TFTP_END;
             }
#if STACK_USE_MPFS             
             else{
               send_data();
             }
#endif             
         }
      }
      
      break;
   
   case SM_TFTP_ACK:
      while(!UDPIsPutReady(TFTPSocket));
         if(!bad_block){
            UDPPut(0);
            UDPPut(TFTP_OPCODE_ACK);
            UDPPut(make8(last_block,1));
            UDPPut(make8(last_block,0));
            UDPFlush();         
         } 
         if(received_final_packet){
      #if TFTP_WRITE_ENABLED
         #if STACK_USE_FAT
            fatclose(&TFTPfstream);
         #else
            b[0]=make8(TFTPaddy,3);
            b[1]=make8(TFTPaddy,2);
            b[2]=make8(TFTPaddy,1);
            b[3]=make8(TFTPaddy,0);
            write_ext_eeprom_buf(END_OF_MPFS_POINTER, b,4);
            MPFSPutEnd();
            MPFSClose();
         #endif
      #endif
            FTPWriteMMC=0;
            smTFTP = SM_TFTP_END;
         }else{
            smTFTP = SM_TFTP_WAIT;

         }
     // }
      break;
      
   case SM_TFTP_ERROR:
      while(!UDPIsPutReady(TFTPSocket));
      
      UDPPut(0);
      UDPPut(TFTP_OPCODE_ERROR);
      UDPPut(0);
      UDPPut(TFTPec);
   
      switch(TFTPec){
      case TFTP_EC_GENERIC:
         printf(UDPPut, "Undefined Error");
         break;
         
      case TFTP_EC_BAD_CMD:
         printf(UDPPut, "Command Not Supported");
         break;
         
      case TFTP_EC_ACCESS:
         printf(UDPPut, "Memory Access Violation");
         break;
         }
      
      UDPPut(0);
      UDPFlush();
      smTFTP = SM_TFTP_END;
   //no break needed
   case SM_TFTP_END:
      received_final_packet = 0;
      TFTPconnected=0;
      last_block=0;
      last_sent=0;
      smTFTP = SM_TFTP_WAIT;
   }//switch(SM_TFTP)

}//TFTPTask()


//execute_tftp_command()
//used by TFTPTask() to parse and act on a received TFTP packet
void execute_tftp_command(void){
   BYTE op;
   char c;
   int16 block;
   int32 j=0;
   int16 k;
   UDPGet(&op);//first byte is 0
   UDPGet(&op);
   
   
   switch (op){
   
   case TFTP_OPCODE_RRQ:
   #if STACK_USE_MPFS 
      if(last_block!=0){
         smTFTP = SM_TFTP_ERROR;
         TFTPec = TFTP_EC_GENERIC;
      }else{
         end_of_mpfs=make32(read_ext_eeprom(END_OF_MPFS_POINTER), 
                            read_ext_eeprom(END_OF_MPFS_POINTER+1),
                            read_ext_eeprom(END_OF_MPFS_POINTER+2),
                            read_ext_eeprom(END_OF_MPFS_POINTER+3));
         last_sent=1;
         last_mem_begin=MPFS_Start;
         send_data();
         FTPWriteMMC=1;
         FTPWriteMMC=1;
      }
  #else
      smTFTP = SM_TFTP_ERROR;
      TFTPec = TFTP_EC_GENERIC;
   
  #endif //fat or mpfs
      
   break;
   
   
   case TFTP_OPCODE_WRQ:
      UDPGet(&c);
      if(c=='/')
         j=0;
      else{
         TFTPfilename[0]='/';
         j=1;
      }

      while(c!=0){
         TFTPfilename[j++]=c;
         UDPGet(&c);
      }
      TFTPfilename[j] = 0;
#if STACK_USE_MPFS      
      TFTPaddy = MPFS_Start;
#endif
#if TFTP_WRITE_ENABLED
#if STACK_USE_FAT
      if(fatopen(TFTPfilename, Write, &TFTPfstream)==EOF){
         mk_file(TFTPfilename);
         fatopen(TFTPfilename, Write, &TFTPfstream);
      }
      
#else //stack_use_MPFS
      TFTPfstream = MPFSFormat();
      MPFSPutBegin(TFTPfstream);
#endif //fat or mpfs
#endif
      FTPWriteMMC=1;
      smTFTP = SM_TFTP_ACK;
      last_block= 0;
      bad_block=0;
   break;

case TFTP_OPCODE_DATA:
      smTFTP = SM_TFTP_ACK;
      UDPGet(&TFTPString[0]);
      UDPGet(&TFTPString[1]);
      block = make16(TFTPString[0], TFTPString[1]);
      if(block==(last_block+1)){
         bad_block=0;
         last_block++;
         k = 0;
         while(TRUE){
            j=0;
            while(UDPGet(&c)){
               TFTPbuf[j++]=c;
               if(j==TFTP_BUFFER_SIZE)
                  break;
            }

            k+=j;
      #if STACK_USE_MPFS
            TFTPaddy+=j;
            write_ext_eeprom_buf((TFTPaddy-j), TFTPbuf, j);
      #elif STACK_USE_FAT
            fatput(TFTPbuf, j, &TFTPfstream);
      #endif
            if(!UDPIsGetReady(TFTPSocket))
               break;
         } 

         FTPWriteMMC=1;
         if(k<TFTP_MAX_DATA_LENGTH){
            received_final_packet= 1;
         }
      }else bad_block=1;
      
   break;
   
   case TFTP_OPCODE_ACK:
   #if STACK_USE_MPFS
      UDPGet(&TFTPString[0]);
      UDPGet(&TFTPString[1]);
         
      block = make16(TFTPString[0], TFTPString[1]);
      if(block==(last_sent)){
         last_sent++;
      }
      if(!received_final_packet){
        send_data(); 
      }else
         smTFTP=SM_TFTP_END;
      
   #else
      smTFTP = SM_TFTP_ERROR;
      TFTPec = TFTP_EC_GENERIC;
   #endif
   break;
   
   case TFTP_OPCODE_ERROR:
      smTFTP = SM_TFTP_END;
   break;
   
   default:
      smTFTP = SM_TFTP_ERROR;
      TFTPec = TFTP_EC_GENERIC;
   
   }
   
}

#if STACK_USE_MPFS
void send_data(void){
   int16  j=0;
   int16  i=0;
   int16  k=0;
   UDPPut(0);
   UDPPut(3);
   UDPPut(make8(last_sent,1));
   UDPPut(make8(last_sent,0));
 
   while(TRUE){
      j= TFTP_BUFFER_SIZE > (end_of_mpfs-last_mem_begin) ? end_of_mpfs-last_mem_begin : TFTP_BUFFER_SIZE;
      j= k+j > TFTP_MAX_DATA_LENGTH ? TFTP_MAX_DATA_LENGTH-k : j;
   #if STACK_USE_MPFS
      read_ext_eeprom_buf(last_mem_begin, TFTPbuf, j);
   #endif

      k+=j;
      last_mem_begin+=j;
      
      for(i=0; i<j; ++i)
         UDPPut(TFTPbuf[i]);
      
      if(k==TFTP_MAX_DATA_LENGTH)
         break;
         
      if(j<TFTP_BUFFER_SIZE)
         break;
   }
   if(k<TFTP_MAX_DATA_LENGTH)
      received_final_packet=1;
   
   UDPFlush();

   smTFTP=SM_TFTP_WAIT;
}
#endif


#endif
