/*********************************************************************
 *
 *             FTP ServerModule for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        FTP.c
 * Dependencies:    StackTsk.h
 *                  TCP.h
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F
 * Complier:        Microchip C18 v3.02 or higher
 *               Microchip C30 v2.01 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * This software is owned by Microchip Technology Inc. ("Microchip") 
 * and is supplied to you for use exclusively as described in the 
 * associated software agreement.  This software is protected by 
 * software and other intellectual property laws.  Any use in 
 * violation of the software license may subject the user to criminal 
 * sanctions as well as civil liability.  Copyright 2006 Microchip
 * Technology Inc.  All rights reserved.
 *
 * This software is provided "AS IS."  MICROCHIP DISCLAIMS ALL 
 * WARRANTIES, EXPRESS, IMPLIED, STATUTORY OR OTHERWISE, NOT LIMITED 
 * TO MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND 
 * INFRINGEMENT.  Microchip shall in no event be liable for special, 
 * incidental, or consequential damages.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     4/23/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     11/13/02 Fixed FTPServer()
 * Howard Schlunder     07/10/06 Added hash printing to FTP client
 * Howard Schlunder     07/20/06 Added FTP_RESP_DATA_NO_SOCKET error message
 * Nick LaBonte         02/14/07 CCS Port with FAT support
 * Nick LaBonte         02/22/07 MPFS support re-implemented; cleanup
  ********************************************************************/
#define THIS_IS_FTP

#include <string.h>
#include <stdlib.h>
#include "tcpip/ftp.h"

#if STACK_USE_FTP_SERVER

#define FTP_BUFFER_SIZE 128

#if STACK_USE_MPFS
   #define FILE   MPFS
   #define fatclose(x)     MPFSPutEnd(); MPFSClose();
   #define fatputc(x,y)    MPFSPut(x)
#endif

#if STACK_USE_MPFS
   #define FTP_DELETE_ENABLED 0
#else
   #define FTP_DELETE_ENABLED 0
#endif

#if !STACK_USE_HTTP2
   int1 FTPWriteMMC = 0;
#endif

#define FTP_COMMAND_PORT                (21)
#define FTP_DATA_PORT                   (20)
#define FTP_TIMEOUT                     (TICKTYPE)((TICKTYPE)180 * TICK_SECOND)
#define MAX_FTP_ARGS                    (7)
#define MAX_FTP_CMD_STRING_LEN          (31)

typedef enum _SM_FTP
{
    SM_FTP_NOT_CONNECTED,
    SM_FTP_CONNECTED,
    SM_FTP_USER_NAME,
    SM_FTP_USER_PASS,
    SM_FTP_RESPOND
} SM_FTP;

typedef enum _SM_FTP_CMD
{
    SM_FTP_CMD_IDLE,
    SM_FTP_CMD_WAIT,
    SM_FTP_CMD_RECEIVE,
    SM_FTP_CMD_WAIT_FOR_DISCONNECT
} SM_FTP_CMD;

typedef enum _FTP_COMMAND
{
    FTP_CMD_USER,
    FTP_CMD_PASS,
    FTP_CMD_QUIT,
    FTP_CMD_STOR,
    FTP_CMD_PORT,
    FTP_CMD_ABORT,
#if FTP_DELETE_ENABLED
    FTP_CMD_DELE,
#endif
    FTP_CMD_UNKNOWN,
    FTP_CMD_NONE,
} FTP_COMMAND;

char  FTP_CMD_STR_USER[] = "USER",
      FTP_CMD_STR_PASS[] = "PASS",
      FTP_CMD_STR_QUIT[] = "QUIT",
      FTP_CMD_STR_STOR[] = "STOR",
      FTP_CMD_STR_PORT[] = "PORT",
      FTP_CMD_STR_ABOR[] = "ABOR";
#if FTP_DELETE_ENABLED
char  FTP_CMD_STR_DELE[] = "DELE";
#endif      

typedef enum _FTP_RESPONSE
{
   FTP_RESP_BANNER = 0,
   FTP_RESP_USER_OK,
   FTP_RESP_PASS_OK,
   FTP_RESP_QUIT_OK,
   FTP_RESP_STOR_OK,
   FTP_RESP_UNKNOWN,
   FTP_RESP_LOGIN,
   FTP_RESP_DATA_OPEN,
   FTP_RESP_DATA_READY,
   FTP_RESP_BAD_FILE,
   FTP_RESP_DATA_CLOSE,
   FTP_RESP_DATA_NO_SOCKET,
   FTP_RESP_OK,

   FTP_RESP_NONE                       // This must always be the last
                                       // There is no corresponding string.
} FTP_RESPONSE;


const char FTP_RESPONSES[13][30]=
{
   "220 Ready\r\n",
   "331 Password required\r\n",
   "230 Logged in\r\n",
   "221 Bye\r\n",
   "500 \r\n",
   "502 Not Implemented\r\n",
   "530 Login Required\r\n",
   "150 Transferring data...\r\n",
   "125 Done\r\n",
   "550 Error: Can't Open File\r\n",
   "\r\n226 Transfer Complete\r\n",
   "425 Can't Open Connection\r\n",
   "200 OK\r\n"
};

/*
const char FTP_RESP_BANNER_STR[]="220 Ready\r\n",
      FTP_RESP_USER_OK_STR[]="331 Password required\r\n",
      FTP_RESP_PASS_OK_STR[]="230 Logged in\r\n",
      FTP_RESP_QUIT_OK_STR[]="221 Bye\r\n",
      FTP_RESP_STOR_OK_STR[]="500 \r\n",
      FTP_RESP_UNKNOWN_STR[]="502 Not Implemented\r\n",
      FTP_RESP_LOGIN_STR[]="530 Login Required\r\n",
      FTP_RESP_DATA_OPEN_STR[]="150 Transferring data...\r\n",
      FTP_RESP_DATA_READY_STR[]="125 Done\r\n",
#if FTP_DELETE_ENABLED
      FTP_RESP_BAD_FILE_STR[]="550 Error: Can't Open File\r\n",
#endif
      FTP_RESP_DATA_CLOSE_STR[]="\r\n226 Transfer Complete\r\n",
      FTP_RESP_DATA_NO_SOCKET_STR[]="425 Can't Open Connection\r\n",
      FTP_RESP_OK_STR[]="200 OK\r\n";
*/

union
{
    struct
    {
        unsigned char bUserSupplied : 1;
        unsigned char bLoggedIn: 1;
    } Bits;
    BYTE Val;
} FTPFlags;


TCP_SOCKET       FTPSocket;      // Main ftp command socket.
TCP_SOCKET       FTPDataSocket;  // ftp data socket.
WORD_VAL         FTPDataPort;    // ftp data port number as supplied by client

SM_FTP           smFTP;          // ftp server FSM state
SM_FTP_CMD       smFTPCommand;   // ftp command FSM state

FTP_COMMAND      FTPCommand;
FTP_RESPONSE     FTPResponse;

char             FTPUser[FTP_USER_NAME_LEN];
char             FTPString[MAX_FTP_CMD_STRING_LEN+2];
BYTE             FTPStringLen;
char             *FTP_argv[MAX_FTP_ARGS];    // Parameters for a ftp command
BYTE             FTP_argc;       // Total number of params for a ftp command
TICKTYPE         lastActivity;   // Timeout keeper.
int32            FTPaddy;


#if STACK_USE_FAT
FILE             fstream;
#elif STACK_USE_MPFS
MPFS             FTPFileHandle;
#endif

// Private helper functions.
void ParseFTPString(void);
FTP_COMMAND ParseFTPCommand(char *cmd);
void ParseFTPString(void);
BOOL ExecuteFTPCommand(FTP_COMMAND cmd);
BOOL PutFile(void);
BOOL Quit(void);

//uncomment to disable file writing to MMC
#define FTP_PUT_ENABLED    1

void FTPPutChar(char c)
{
   TCPPut(FTPSocket, c);
}


/*********************************************************************
 * Function:        void FTPInit(void)
 *
 * PreCondition:    TCP module is already initialized.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Initializes internal variables of FTP
 *
 * Note:
 ********************************************************************/
void FTPInit(void)
{
    FTPSocket       = TCPListen(FTP_COMMAND_PORT);
    smFTP           = SM_FTP_NOT_CONNECTED;
    FTPStringLen    = 0;
    FTPFlags.Val    = 0;
    FTPDataPort.Val = FTP_DATA_PORT;
}


/*********************************************************************
 * Function:        void FTPTask(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened FTP connections are served.
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
BOOL FTPTask(void)
{
    BYTE v;
    TICKTYPE currentTick;
    char pMsg[30];
    int8 i;

    if ( !TCPIsConnected(FTPSocket) )
    {
        FTPStringLen    = 0;
        FTPCommand      = FTP_CMD_NONE;
        smFTP           = SM_FTP_NOT_CONNECTED;
        FTPFlags.Val    = 0;
        smFTPCommand    = SM_FTP_CMD_IDLE;
        return TRUE;
    }

    if ( TCPIsGetReady(FTPSocket) )
    {
        lastActivity    = TickGet();

        while( TCPGet(FTPSocket, &v ) )
        {
            FTPString[FTPStringLen++]   = v;
            if ( FTPStringLen == MAX_FTP_CMD_STRING_LEN )
                FTPStringLen            = 0;
        }
        TCPDiscard(FTPSocket);


        if ( v == '\n' )
        {
            FTPString[FTPStringLen]     = '\0';
            FTPStringLen                = 0;
            ParseFTPString();
            FTPCommand                  = ParseFTPCommand(FTP_argv[0]);
        }
        
    }
    else if ( smFTP != SM_FTP_NOT_CONNECTED )
    {
        currentTick = TickGet();
        currentTick = TickGetDiff(currentTick, lastActivity);
        if ( currentTick >= FTP_TIMEOUT )
        {
            lastActivity                = TickGet();
            FTPCommand                  = FTP_CMD_QUIT;
            smFTP                       = SM_FTP_CONNECTED;
        }
    }

    switch(smFTP)
    {
    case SM_FTP_NOT_CONNECTED:
        FTPResponse = FTP_RESP_BANNER;
        lastActivity = TickGet();
        /* No break - Continue... */

    case SM_FTP_RESPOND:
SM_FTP_RESPOND_Label:
        if(!TCPIsPutReady(FTPSocket))
      {
         return TRUE;
      }
      else
      {
         i=0;
         if (FTPResponse >= FTP_RESP_NONE)
            FTPResponse = FTP_RESP_UNKNOWN;
         while ((v=FTP_RESPONSES[FTPResponse][i++])!=0)
         {
            FTPPutChar(v);
         }
         TCPFlush(FTPSocket);
         FTPResponse = FTP_RESP_NONE;
         smFTP = SM_FTP_CONNECTED;
      }
        // No break - this will speed up little bit

    case SM_FTP_CONNECTED:
        if ( FTPCommand != FTP_CMD_NONE )
        {
            if ( ExecuteFTPCommand(FTPCommand) )
            {
                if ( FTPResponse != FTP_RESP_NONE )
                    smFTP = SM_FTP_RESPOND;
                else if ( FTPCommand == FTP_CMD_QUIT )
                    smFTP = SM_FTP_NOT_CONNECTED;

                FTPCommand = FTP_CMD_NONE;
                smFTPCommand = SM_FTP_CMD_IDLE;
            }
            else if ( FTPResponse != FTP_RESP_NONE )
            {
                smFTP = SM_FTP_RESPOND;
                goto SM_FTP_RESPOND_Label;
            }
        }
        break;


    }

    return TRUE;
}

static BOOL ExecuteFTPCommand(FTP_COMMAND cmd)
{
    switch(cmd)
    {
    case FTP_CMD_USER:
        FTPFlags.Bits.bUserSupplied = TRUE;
        FTPFlags.Bits.bLoggedIn = FALSE;
        FTPResponse = FTP_RESP_USER_OK;
        strncpy(FTPUser, FTP_argv[1], sizeof(FTPUser));
        break;

    case FTP_CMD_PASS:
        if ( !FTPFlags.Bits.bUserSupplied )
            FTPResponse = FTP_RESP_LOGIN;
        else
        {
            if ( FTPVerify(FTPUser, FTP_argv[1]) )
            {
                FTPFlags.Bits.bLoggedIn = TRUE;
                FTPResponse = FTP_RESP_PASS_OK;
            }
            else
                FTPResponse = FTP_RESP_LOGIN;
        }
        break;

    case FTP_CMD_QUIT:
        return Quit();

    case FTP_CMD_PORT:
        FTPDataPort.v[1] = (BYTE)atoi(FTP_argv[5]);
        FTPDataPort.v[0] = (BYTE)atoi(FTP_argv[6]);
        FTPResponse = FTP_RESP_OK;
        break;

    case FTP_CMD_STOR:
      
        return PutFile();
#if FTP_DELETE_ENABLED
    case FTP_CMD_DELE:
        return DeleteFile();
#endif
    case FTP_CMD_ABORT:
        FTPResponse = FTP_RESP_OK;
        if ( FTPDataSocket != INVALID_SOCKET )
            TCPDisconnect(FTPDataSocket);
        break;

    default:
        FTPResponse = FTP_RESP_UNKNOWN;
        break;
    }
    return TRUE;
}

static BOOL Quit(void)
{
    switch(smFTPCommand)
    {
    case SM_FTP_CMD_IDLE:
#if FTP_PUT_ENABLED
        if ( smFTPCommand == SM_FTP_CMD_RECEIVE ){
            fatclose(&fstream);
            FTPWriteMMC=0;
        }
#endif

        if ( FTPDataSocket != INVALID_SOCKET )
        {
#if FTP_PUT_ENABLED
            fatclose(&fstream);
#endif
            FTPWriteMMC=0;
            TCPDisconnect(FTPDataSocket);
            smFTPCommand = SM_FTP_CMD_WAIT;
        }
        else
            goto Quit_Done;
        break;

    case SM_FTP_CMD_WAIT:
        if ( !TCPIsConnected(FTPDataSocket) )
        {
Quit_Done:
            FTPResponse = FTP_RESP_QUIT_OK;
            smFTPCommand = SM_FTP_CMD_WAIT_FOR_DISCONNECT;
        }
        break;

    case SM_FTP_CMD_WAIT_FOR_DISCONNECT:
        if ( TCPIsPutReady(FTPSocket) )
        {
            if ( TCPIsConnected(FTPSocket) )
                TCPDisconnect(FTPSocket);
        }
        break;

    }
    return FALSE;
}


static BOOL PutFile(void)
{
    BYTE v;
    char buf[FTP_BUFFER_SIZE];
    int8 i;
    char b[4];
//FTP_argv[1] holds filename
char filename[20];
sprintf(filename, "/%s", FTP_argv[1]);

    switch(smFTPCommand)
    {
    case SM_FTP_CMD_IDLE:
        if ( !FTPFlags.Bits.bLoggedIn )
        {
            FTPResponse     = FTP_RESP_LOGIN;
            return TRUE;
        }
        else
        {
            FTPResponse     = FTP_RESP_DATA_OPEN;
            FTPDataSocket   = TCPConnect(&REMOTE_HOST(FTPSocket), FTPDataPort.Val);

         // Make sure that a valid socket was available and returned
         // If not, return with an error
         if(FTPDataSocket != INVALID_SOCKET)
         {
               smFTPCommand = SM_FTP_CMD_WAIT;
         }
         else
         {
               FTPResponse = FTP_RESP_DATA_NO_SOCKET;
               return TRUE;
         }
        }
        break;

    case SM_FTP_CMD_WAIT:
        if ( TCPIsConnected(FTPDataSocket) )
        {

#if FTP_PUT_ENABLED
   #if STACK_USE_FAT
         if(fatopen(filename, Write, &fstream)==EOF){
            mk_file(filename);
            fatopen(filename, Write, &fstream);
         }
   #elif STACK_USE_MPFS
         FTPFileHandle   = MPFSFormat();
         FTPaddy = MPFS_Start;
   #endif//STACK_USE_FAT            
#endif
         FTPWriteMMC=1;
         smFTPCommand = SM_FTP_CMD_RECEIVE;
        }
        break;

    case SM_FTP_CMD_RECEIVE:
        if ( TCPIsGetReady(FTPDataSocket) )
        {
            // Reload timeout timer.
            lastActivity    = TickGet();
#if STACK_USE_MPFS
            MPFSPutBegin(FTPFileHandle);
#endif
            i=0;
            while( TCPGet(FTPDataSocket, &v) )
            {
               buf[i]=v;
               ++i;
               if(i==FTP_BUFFER_SIZE){
#if STACK_USE_FAT
                  fatput(buf, FTP_BUFFER_SIZE, &fstream);
#elif STACK_USE_MPFS
                  write_ext_eeprom_buf(FTPaddy, buf, FTP_BUFFER_SIZE);
#endif
                  FTPaddy+=FTP_BUFFER_SIZE;
                  i=0;
               }
            }
            if(i){
#if STACK_USE_FAT
               fatput(buf, i, &fstream);
#elif STACK_USE_MPFS
               write_ext_eeprom_buf(FTPaddy, buf, i);
#endif
               FTPaddy+=i;
            }               
            TCPDiscard(FTPDataSocket);
            FTPWriteMMC=1;
        }
        else if ( !TCPIsConnected(FTPDataSocket) )
        {
#if FTP_PUT_ENABLED
         #if STACK_USE_FAT
            fatclose(&fstream);
         #else
            b[0]=make8(FTPaddy,3);
            b[1]=make8(FTPaddy,2);
            b[2]=make8(FTPaddy,1);
            b[3]=make8(FTPaddy,0);
            write_ext_eeprom_buf(END_OF_MPFS_POINTER, b,4);
            MPFSPutEnd();
            MPFSClose();
         #endif
#endif
            FTPWriteMMC=0;
            TCPDisconnect(FTPDataSocket);
            FTPDataSocket   = INVALID_SOCKET;
            FTPResponse     = FTP_RESP_DATA_CLOSE;
            return TRUE;
        }
    }
    return FALSE;
}

#if FTP_DELETE_ENABLE

//////FUNCTION/////
BOOL DeleteFile(void)
{
//FTP_argv[1] holds filename
char filename[20];
sprintf(filename, "/%s", FTP_argv[1]);
    if(smFTPCommand==SM_FTP_CMD_IDLE)
    {
        if ( !FTPFlags.Bits.bLoggedIn )
        {
            FTPResponse     = FTP_RESP_LOGIN;
            return TRUE;
        }
        else
        {
            if(fatopen(filename, moderb, &fstream)==EOF){
               FTPResponse     = FTP_RESP_BAD_FILE;
               return TRUE;
            }else
            {
               fatclose(&fstream);
               rm_file(filename==GOODEC);
               FTPWriteMMC=1;
               second_counter=-2;
               FTPResponse     = FTP_RESP_OK;
               return TRUE;
            }
        }
    }
    return FALSE;
}

#endif
//*****FUNCTION*******//
static FTP_COMMAND ParseFTPCommand(char *cmd)
{
    FTP_COMMAND returnval=0;
/*    0:FTP_CMD_STR_USER[] = "USER",
      1:FTP_CMD_STR_PASS[] = "PASS",
      2:FTP_CMD_STR_QUIT[] = "QUIT",
      3:FTP_CMD_STR_STOR[] = "STOR",
      4:FTP_CMD_STR_PORT[] = "PORT",
      5:FTP_CMD_STR_ABOR[] = "ABOR";

    for ( i = 0; i < (FTP_COMMAND)FTP_COMMAND_TABLE_SIZE; i++ )
    {
      if ( !memcmppgm2ram((void*)cmd, (ROM void*)FTPCommandString[i], 4) )
      if(strcmp(cmd, FTPCommandString[i])==0)
         return i;
    }
*/
   if(strcmp(cmd, FTP_CMD_STR_USER)==0)
      returnval=FTP_CMD_USER;
   else if(strcmp(cmd, FTP_CMD_STR_PASS)==0)
      returnval=FTP_CMD_PASS;
   else if(strcmp(cmd, FTP_CMD_STR_QUIT)==0)
      returnval=FTP_CMD_QUIT;
   else if(strcmp(cmd, FTP_CMD_STR_STOR)==0)
      returnval=FTP_CMD_STOR;
   else if(strcmp(cmd, FTP_CMD_STR_PORT)==0)
      returnval=FTP_CMD_PORT;
   else if(strcmp(cmd, FTP_CMD_STR_ABOR)==0)
      returnval=FTP_CMD_ABORT;
#if FTP_DELETE_ENABLED
   else if(strcmp(cmd, FTP_CMD_STR_DELE)==0)
      returnval=FTP_CMD_DELE;
#endif
   else returnval=FTP_CMD_UNKNOWN;
  
    return returnval;
}


static void ParseFTPString(void)
{
    BYTE *p;
    BYTE v;
    enum { SM_FTP_PARSE_PARAM, SM_FTP_PARSE_SPACE } smParseFTP;

    smParseFTP  = SM_FTP_PARSE_PARAM;
    p           = (BYTE*)&FTPString[0];

    // Skip white blanks
    while( *p == ' ' )
        p++;

    FTP_argv[0]  = (char*)p;
    FTP_argc     = 1;

    while( (v = *p) )
    {
        switch(smParseFTP)
        {
        case SM_FTP_PARSE_PARAM:
            if ( v == ' ' || v == ',' )
            {
                *p = '\0';
                smParseFTP = SM_FTP_PARSE_SPACE;
            }
            else if ( v == '\r' || v == '\n' )
                *p = '\0';
            break;

        case SM_FTP_PARSE_SPACE:
            if ( v != ' ' )
            {
                FTP_argv[FTP_argc++] = (char*)p;
                smParseFTP = SM_FTP_PARSE_PARAM;
            }
            break;
        }
        p++;
    }
}

#endif   // #if defined(STACK_USE_FTP_SERVER)
