//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                                 SMTP.C                                   //
//              SMTP Engine for Microchip TCP/IP Stack API                  //
//
// Define STACK_USE_SMTP to TRUE before including stacktsk.h in your code
// to enable this SMTP API.  Once enabled, you can use these functions:
//
// SMTPConnectAndStart(IP_ADDR ip, int16 port, char *from, char *to, char *subject)
//    Will open the IP address and TCP port (which should be your SMTP
//    server), and start the engine that will initiate SMTP connection.
//    The SMTP engine will then send the mail to: and rcpt from: command,
//    and create basic E-Mail headers.  Will return TRUE if successfully
//    started the engine, will return FALSE if a previous engine is still
//    running.
//    Once an engine is running, it will stop running once
//    SMTPIsPutReady() returns TRUE -OR- SMTPLastError() returns a
//    non-zero number.
//
// SMTPConnect(IP_ADDR ip, int16 port)
//    Unlike the full SMTPConnectAndStart(), this simply opens the TCP socket and
//    performs a EHLO command (it does not send mail to: and rcpt from:
//    commands.  Use SMTPStart() to do this).  See the other SMTPConnect() 
//    for documentation.
//
// SMTPStart(char *from, char *to, char *subject)
//    If you use the short SMTPConnect() routine, this will tell the engine
//    to start sending the from, to and subject fields of the e-mail.
//    from, to and subject must be valid until SMTPIsPutReady() returns TRUE.
//    You shouldn't call this until SMTPIsStartReady() is TRUE.
//
// SMTPIsPutReady()
//    After a succesfull SMTPConnect(), the SMTP engine will be sending
//    SMTP commands.  You cannot start sending the body of the e-mail
//    until the SMTP engine has got the SMTP server in a state that is
//    ready for the body of the e-mail.  SMTPIsPutReady() returns TRUE
//    if the SMTP engine and the SMTP server is ready.
//
// SMTPLastError()
//    If there was an error with the SMTP, this will return non-zero.
//    Once this returns non-zero then you can try again by calling
//    SMTPConnect().
//
// SMTPPut(char c)
//    Puts this char into the body of the e-mail.  SMTPIsPutReady() must
//    return TRUE before this is called.
//
// SMTPFinish()
//    Finishes off the e-mail.  Once the e-mail is finished poll 
//    SMTPIsStartReady() to determine when it is ready for the next e-mail.
//
// SMTPDisconnect()
// SMTPDisconnectNoFlush()
//    Close the e-mail socket.  If SMTPDisconnect() is used, it will first 
//    finish sending the e-mail; if SMTPDisconnectNoFlush() is useed, then 
//    it will just close the socket.  SMTPIsPutReady() must return TRUE before 
//    you call this.  After calling this, wait until SMTPIsFree() returns TRUE 
//    and use SMTPLastError() to see if the email was sent sucessfully.
//
// SMTPIsFree()
//    Will return TRUE if the SMTP engine is free for another connection.  If
//    the SMTP has crashed at any time (perhaps due to a timeout error to a
//    command) then it will close the socket and this will return TRUE.
//
// SMTPIsStartReady()
//    Will return TRUE if the SMTP engine is ready for another SMTPStart().
//    You will want to poll this after you call SMTPFinish() to determine when
//    it is ready to send the next e-mail.
//
// int16 = SMTPLastResultCode()
//    Each command sent to the SMTP server is responded with a numeric result
//    code.  This function returns the last result code read by the SMTP
//    engine.  Will return -1 if the engine hasn't been used yet or no 
//    result code has be read yet.
//
// NOTE: You *MUST* use the SMTP server for your ISP.  If you do not know it
//  then ask your ISP.  The reason for this is that because of the war on
//  spam almost all SMTP servers block access to clients who aren't on their
//  network.
//
// NOTE: The SMTP engine can only handle one socket at a time.  Therefore you
//  cannot call a SMTPConnect() until the previous SMTPConnect() has been
//  disconnected.
//
// NOTE: Due to the war on spam many internet service providers are placing
//  restrictions upon SMTP servers.  Such restrictions may be authentication,
//  sender-id, message-id and max message-per-minute rate.  This engine
//  deals with none of those restrictions.  It's very likely in the future
//  that it will be impossible for a PIC to have the resources to send e-mail
//  using SMTP.
//
// NOTE: If you are using Ethernet, you will have to enable ARP!!!
//
///////////////////////////////////////////////////////////////////////////////

#include <tcpip/smtp.h>
#include <tcpip/base64.h>

#define debug_smtp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)

typedef enum {
   SMTP_STATE_IDLE=0, SMTP_STATE_START_FULL=1, SMTP_STATE_START_SIMPLE=2, 
   SMTP_STATE_START=3, SMTP_STATE_ARP_REQ=4, SMTP_STATE_ARP_WAIT=5, 
   SMTP_STATE_CONNECT=6, SMTP_STATE_CONNECT_WAIT=7, SMTP_STATE_EHLO=8,
   SMTP_STATE_POST_EHLO=9, SMTP_STATE_START_AUTH_LOGIN=10, 
   SMTP_STATE_AUTH_LOGIN_SEND_USERNAME=11,
   SMTP_STATE_AUTH_LOGIN_USERNAME_CONTINUE=12, 
   SMTP_STATE_AUTH_LOGIN_SEND_PASSWORD=13, 
   SMTP_STATE_AUTH_LOGIN_PASSWORD_CONTINUE=14,
   SMTP_STATE_AUTH_PLAIN_SEND=15, SMTP_STATE_AUTH_PLAIN_SEND_CONTINUE=16,
   SMTP_STATE_START_AUTH_PLAIN=17,
   SMTP_STATE_MAIL_FROM=18, SMTP_STATE_RCPT_TO=19, SMTP_STATE_DATA_START=20,
   SMTP_STATE_DO_CMD=21, SMTP_STATE_DO_CMD_GET_RESP=22, 
   SMTP_STATE_PUT_HEADER=23, SMTP_STATE_PUT_START=24, SMTP_STATE_PUT_BODY=25, 
   SMTP_STATE_FLUSH_EMAIL=26, SMTP_STATE_FLUSH_AND_CLOSE_EMAIL=27, 
   SMTP_STATE_FLUSH_EMAIL_CHECK=28, SMTP_STATE_FLUSH_EMAIL_DELAY=29, 
   SMTP_STATE_FLUSH_EMAIL_WAIT=30, SMTP_STATE_CLOSE=31, 
   SMTP_STATE_FORCE_CLOSE=32, SMTP_STATE_RESET=33
} SMTP_STATE;

SMTP_STATE smtp_state;


TCP_SOCKET smtp_socket=INVALID_SOCKET;
NODE_INFO smtp_remote;
char *smtp_engine_from;
char *smtp_engine_to;
char *smtp_engine_subject;
SMTP_EC smtp_last_error=0;
int16 smtp_engine_port;
int16 g_LastSMTPResultCode = -1;

#if defined(__ESMTP)
 int1 g_AUTHIsPlain;
 int1 g_AUTHIsLogin;
 char *g_SMTPAuthInfo[2];  //[0] holds user name, [1] holds password
#endif

#inline int16 SMTPLastResultCode(void)
{
   return(g_LastSMTPResultCode);
}

int8 SMTPIsFree(void) 
{
   return(smtp_state==SMTP_STATE_IDLE);
}

#if defined(__ESMTP)
int8 SMTPConnect(IP_ADDR *ip, int16 port, char *username, char *pwd) 
#else
int8 SMTPConnect(IP_ADDR *ip, int16 port) 
#endif
{
   if (SMTPIsFree()) 
   {
      smtp_state=SMTP_STATE_START_SIMPLE;
      memcpy(&smtp_remote.IPAddr, ip, sizeof(IP_ADDR));
      smtp_engine_port=port;
      smtp_last_error=0;
     #if defined(__ESMTP)
      g_SMTPAuthInfo[0]=username;
      g_SMTPAuthInfo[1]=pwd;
      g_AUTHIsPlain=FALSE;
      g_AUTHIsLogin=FALSE;
     #endif
      return(TRUE);
   }
   smtp_last_error=SMTP_EC_FINISH_PREVIOUS;
   return(FALSE);
}

//user functions
#if defined(__ESMTP)
int8 SMTPConnectAndStart(IP_ADDR *ip, int16 port, char *username, char *pwd, 
                  char *from, char *to, char *subject)
#else
int8 SMTPConnectAndStart(IP_ADDR *ip, int16 port, char *from, char *to, char *subject)
#endif
{
   if (
       #if defined(__ESMTP)
        SMTPConnect(ip, port, username, pwd)
       #else
        SMTPConnect(ip, port)
       #endif
      )
   {
      smtp_state=SMTP_STATE_START_FULL;
      smtp_engine_from=from;
      smtp_engine_to=to;
      smtp_engine_subject=subject;
      return(TRUE);
   }
           
   smtp_last_error=SMTP_EC_FINISH_PREVIOUS;
   return(FALSE);
}

int8 SMTPIsStartReady(void)
{
   return(smtp_state==SMTP_STATE_PUT_START);
}

int8 SMTPStart(char *from, char *to, char *subject) 
{
   if (SMTPIsStartReady()) 
   {
      smtp_state=SMTP_STATE_MAIL_FROM;
      smtp_engine_from=from;
      smtp_engine_to=to;
      smtp_engine_subject=subject;
      smtp_last_error=0;
      return(TRUE);
   }
   smtp_last_error=SMTP_EC_FINISH_PREVIOUS;
   return(FALSE);
}


int8 SMTPIsPutReady(void) {
   return((smtp_state==SMTP_STATE_PUT_BODY)&&(TCPIsPutReady(smtp_socket)));
}

int1 SMTPPut(char c) {
   if (SMTPIsPutReady() && TCPIsPutReady(smtp_socket)) {
      return(TCPPut(smtp_socket,c));
   }
   return(FALSE);
}

void SMTPFinish(void)
{
   smtp_state = SMTP_STATE_FLUSH_EMAIL;
}

void _SMTPDisconnect(int8 flush)
{
   if (smtp_state==SMTP_STATE_PUT_BODY)
   {
      if (flush)
         smtp_state = SMTP_STATE_FLUSH_AND_CLOSE_EMAIL;
      else
         smtp_state = SMTP_STATE_CLOSE;
   }
   else if (smtp_socket!=INVALID_SOCKET)
      smtp_state=SMTP_STATE_CLOSE;
   else
      SMTPInit();
}

void SMTPDisconnect(void) {_SMTPDisconnect(TRUE);}
void SMTPDisconnectNoFlush(void) {_SMTPDisconnect(FALSE);}

SMTP_EC SMTPLastError(void) {
   return(smtp_last_error);
}

//stack functions
void SMTPInit(void) 
{
   if (smtp_socket!=INVALID_SOCKET)
   {
      TCPDisconnect(smtp_socket);
   }
   smtp_socket=INVALID_SOCKET;
   smtp_state=SMTP_STATE_IDLE;
}

void SMTPError(SMTP_EC ec) 
{
   smtp_last_error=ec;
   smtp_state=SMTP_STATE_RESET;
}

void SMTPPutCmd(char c) 
{
   TCPPut(smtp_socket,c);
}

#inline
static SMTP_STATE SMTPPostHeaderState(int1 fullConnect)
{
   if (fullConnect)
      return(SMTP_STATE_MAIL_FROM);
   else
      return(SMTP_STATE_PUT_START);
}

void SMTPTask(void) 
{
   TICKTYPE currTick;
   static TICKTYPE lastTick;
   static int16 smtp_expected_result;

   static char scrmsg[12];
   #DEFINE SMTP_EHLO_MSG   "ehlo me"
   #DEFINE SMTP_DATA_MSG   "data"
   #DEFINE SMTP_MAIL_MSG   "mail from: "
   #DEFINE SMTP_RCPT_MSG   "rcpt to: "
   #DEFINE SMTP_AUTH_LOGIN_MSG   "AUTH LOGIN"
   #DEFINE SMTP_AUTH_PLAIN_MSG   "AUTH PLAIN"

   /*
   static char ehlomsg[]=     "ehlo me";
   static char datamsg[]=     "data";
   static char mailfrommsg[]= "mail from: ";
   static char rcpttomsg[]=   "rcpt to: ";
   static char *cmdptr;
   */

   static char *cmdptr2;
   static SMTP_EC on_err;
   static int8 next_state;
   static int1 fullConnect;
   static int1 flushAndClose;
   int1 lbContinue = FALSE;
   
   currTick=TickGet();

   do 
   {
      lbContinue = FALSE;
      switch(smtp_state) 
      {
         case SMTP_STATE_IDLE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_IDLE");
            break;
   
         case SMTP_STATE_START_SIMPLE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_START_SIMPLE");
            fullConnect = FALSE;
            smtp_state=SMTP_STATE_START;
            break;
            
         case SMTP_STATE_START_FULL:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_START_FULL");
            fullConnect = TRUE;
            smtp_state=SMTP_STATE_START;
            break;
            
         case SMTP_STATE_START:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_START");
            lastTick=currTick;
            smtp_state=SMTP_STATE_ARP_REQ;
   
      #if STACK_USE_ARP
         case SMTP_STATE_ARP_REQ:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_ARP_REQ");
            if (ARPIsTxReady()) 
            {
               ARPResolve(&smtp_remote.IPAddr);
               lastTick=currTick;
               smtp_state=SMTP_STATE_ARP_WAIT;
            }
            else if (TickGetDiff(currTick,lastTick) > (TICKS_PER_SECOND / 2)) 
            {
               SMTPError(SMTP_EC_MAC_TX_FAIL);
            }
            break;
   
         case SMTP_STATE_ARP_WAIT:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_ARP_WAIT");
            if (ARPIsResolved(&smtp_remote.IPAddr, &smtp_remote.MACAddr)) 
            {
               smtp_state=SMTP_STATE_CONNECT;
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * (TICKTYPE)5)) 
            {
               SMTPError(SMTP_EC_ARP_FAIL);
            }
            break;
      #else    
         case SMTP_STATE_ARP_REQ:
         case SMTP_STATE_ARP_WAIT:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_ARP_REQ");
            smtp_state=SMTP_STATE_CONNECT;
      #endif
   
   
         case SMTP_STATE_CONNECT:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_CONNECT");
            smtp_socket=TCPConnect(&smtp_remote, smtp_engine_port);
            if (smtp_socket!=INVALID_SOCKET) {
               lastTick=currTick;
               smtp_state=SMTP_STATE_CONNECT_WAIT;
            }
            else {
               SMTPError(SMTP_EC_INVALID_SOCKET);
            }
            break;
   
         case SMTP_STATE_CONNECT_WAIT:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_CONNECT_WAIT");
            if (TCPIsConnected(smtp_socket)) 
            {
               smtp_state=SMTP_STATE_DO_CMD_GET_RESP;
               smtp_expected_result=220;
               next_state=SMTP_STATE_EHLO;
               on_err=SMTP_EC_BAD_WELCOME;
               lastTick=currTick;
               SMTPReadResultCodeReset();
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 10)) {
               SMTPError(SMTP_EC_CONNECT_FAIL);
            }
            break;
   
         case SMTP_STATE_EHLO:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_EHLO");
            flushAndClose = TRUE;
            sprintf(scrmsg, SMTP_EHLO_MSG);
            cmdptr2=0;
            smtp_expected_result=250;
            on_err=SMTP_EC_BAD_EHLO;
            smtp_state = SMTP_STATE_DO_CMD;
            next_state = SMTP_STATE_POST_EHLO;
            lastTick=currTick;
            break;
         
         case SMTP_STATE_POST_EHLO:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_POST_EHLO");
            lbContinue = TRUE;
           #if defined(__ESMTP)
            if (g_AUTHIsLogin && g_SMTPAuthInfo[0] && g_SMTPAuthInfo[1])
               smtp_state = SMTP_STATE_START_AUTH_LOGIN;
            else if (g_AUTHIsPlain && g_SMTPAuthInfo[0] && g_SMTPAuthInfo[1])
               smtp_state = SMTP_STATE_START_AUTH_PLAIN;
            else
           #endif
               smtp_state = SMTPPostHeaderState(fullConnect);
            break;
   
   #if defined(__ESMTP)
         // --- login ---
         case SMTP_STATE_START_AUTH_LOGIN:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_START_AUTH_LOGIN");
            sprintf(scrmsg, SMTP_AUTH_LOGIN_MSG);
            cmdptr2=NULL;
            smtp_expected_result=334;
            on_err=SMTP_EC_BAD_AUTH_INIT;
            smtp_state=SMTP_STATE_DO_CMD;
            next_state=SMTP_STATE_AUTH_LOGIN_SEND_USERNAME;
            lastTick=currTick;
            break;
   
         case SMTP_STATE_AUTH_LOGIN_SEND_USERNAME:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_LOGIN_SEND_USERNAME");
            lastTick=currTick;
            smtp_state=SMTP_STATE_AUTH_LOGIN_USERNAME_CONTINUE;
         case SMTP_STATE_AUTH_LOGIN_USERNAME_CONTINUE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_LOGIN_USERNAME_CONTINUE");
            TCPDiscard(smtp_socket);
            if (TCPIsPutReady(smtp_socket))
            {
               StringToBase64XMIT(&g_SMTPAuthInfo[0], 1, smtp_socket);
               SMTPPutCmd('\r');
               SMTPPutCmd('\n');
               TCPFlush(smtp_socket);
               SMTPReadResultCodeReset();
               smtp_expected_result = 334;
               on_err = SMTP_EC_BAD_AUTH_USERNAME;
               smtp_state = SMTP_STATE_DO_CMD_GET_RESP;
               next_state = SMTP_STATE_AUTH_LOGIN_SEND_PASSWORD;
               lastTick = currTick;
            }
            else if (TickGetDiff(currTick,lastTick) > (TICKS_PER_SECOND * 2)) 
            {
               SMTPError(SMTP_EC_MAC_TX_FAIL);
            }            
            break;

         case SMTP_STATE_AUTH_LOGIN_SEND_PASSWORD:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_LOGIN_SEND_PASSWORD");
            lastTick=currTick;
            smtp_state=SMTP_STATE_AUTH_LOGIN_PASSWORD_CONTINUE;
         case SMTP_STATE_AUTH_LOGIN_PASSWORD_CONTINUE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_LOGIN_PASSWORD_CONTINUE");
            TCPDiscard(smtp_socket);
            if (TCPIsPutReady(smtp_socket))
            {
               StringToBase64XMIT(&g_SMTPAuthInfo[1], 1, smtp_socket);
               SMTPPutCmd('\r');
               SMTPPutCmd('\n');
               TCPFlush(smtp_socket);
               SMTPReadResultCodeReset();
               smtp_expected_result = 235;
               on_err = SMTP_EC_BAD_AUTH_PASSWORD;
               smtp_state = SMTP_STATE_DO_CMD_GET_RESP;
               next_state = SMTPPostHeaderState(fullConnect);
               lastTick = currTick;
            }
            else if (TickGetDiff(currTick,lastTick) > (TICKS_PER_SECOND * 2)) 
            {
               SMTPError(SMTP_EC_MAC_TX_FAIL);
            }            
            break;
            
         //// --- plain ---
         
         case SMTP_STATE_START_AUTH_PLAIN:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_START_AUTH_PLAIN");
            sprintf(scrmsg, SMTP_AUTH_PLAIN_MSG);
            cmdptr2=NULL;
            smtp_expected_result=334;
            on_err=SMTP_EC_BAD_AUTH_INIT;
            smtp_state=SMTP_STATE_DO_CMD;
            next_state=SMTP_STATE_AUTH_PLAIN_SEND;
            lastTick=currTick;
            break;
         
         case SMTP_STATE_AUTH_PLAIN_SEND:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_PLAIN_SEND");
            lastTick=currTick;
            smtp_state=SMTP_STATE_AUTH_PLAIN_SEND_CONTINUE;
         case SMTP_STATE_AUTH_PLAIN_SEND_CONTINUE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_AUTH_PLAIN_SEND_CONTINUE");
            TCPDiscard(smtp_socket);
            if (TCPIsPutReady(smtp_socket))
            {
               StringToBase64XMIT(&g_SMTPAuthInfo[0], 2, smtp_socket);
               SMTPPutCmd('\r');
               SMTPPutCmd('\n');
               TCPFlush(smtp_socket);
               SMTPReadResultCodeReset();
               smtp_expected_result = 235;
               on_err = SMTP_EC_BAD_AUTH_USERNAME;
               smtp_state = SMTP_STATE_DO_CMD_GET_RESP;
               next_state = SMTPPostHeaderState(fullConnect);
               lastTick = currTick;
            }
            else if (TickGetDiff(currTick,lastTick) > (TICKS_PER_SECOND * 2)) 
            {
               SMTPError(SMTP_EC_MAC_TX_FAIL);
            }
            break;
  #endif
   
         case SMTP_STATE_MAIL_FROM:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_MAIL_FROM");
            sprintf(scrmsg, SMTP_MAIL_MSG);
            cmdptr2=smtp_engine_from;
            smtp_expected_result=250;
            on_err=SMTP_EC_BAD_MAILFROM;
            smtp_state=SMTP_STATE_DO_CMD;
            next_state=SMTP_STATE_RCPT_TO;
            lastTick=currTick;
            break;
   
         case SMTP_STATE_RCPT_TO:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_RCPT_TO");
            sprintf(scrmsg, SMTP_RCPT_MSG);
            cmdptr2=smtp_engine_to;
            smtp_expected_result=250;
            on_err=SMTP_EC_BAD_RCPTTO;
            smtp_state=SMTP_STATE_DO_CMD;
            next_state=SMTP_STATE_DATA_START;
            lastTick=currTick;
            break;
   
         case SMTP_STATE_DATA_START:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_DATA_START");
            sprintf(scrmsg, SMTP_DATA_MSG);
            cmdptr2=0;
            smtp_expected_result=354;
            on_err=SMTP_EC_BAD_RCPTTO;
            smtp_state=SMTP_STATE_DO_CMD;
            next_state=SMTP_STATE_PUT_HEADER;
            lastTick=currTick;
            break;
   
         case SMTP_STATE_DO_CMD:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_DO_CMD");
            TCPDiscard(smtp_socket);
            if (TCPIsPutReady(smtp_socket)) {
              printf(SMTPPutCmd, "%s", scrmsg);
              if (cmdptr2)
                 printf(SMTPPutCmd, "<%s>", cmdptr2);
              SMTPPutCmd('\r');
              SMTPPutCmd('\n');
              TCPFlush(smtp_socket);
              smtp_state=SMTP_STATE_DO_CMD_GET_RESP;
              lastTick=currTick;
              SMTPReadResultCodeReset();
            }
            else if (TickGetDiff(currTick,lastTick) > (TICKS_PER_SECOND / 2)) {
               SMTPError(SMTP_EC_MAC_TX_FAIL);
            }
            break;
   
         case SMTP_STATE_DO_CMD_GET_RESP:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_DO_CMD_GET_RESP");
            if (TCPIsGetReady(smtp_socket)) {
               lastTick=currTick;
               if (SMTPReadResultCode()) {
                  if (g_LastSMTPResultCode==smtp_expected_result) 
                  {
                     lastTick=currTick;
                     smtp_state=next_state;
                  }
                  else {
                     SMTPError(on_err);
                  }
               }
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * (TICKTYPE)10)) {
               SMTPError(on_err);
            }
            break;
   
         case SMTP_STATE_PUT_HEADER:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_PUT_HEADER");
            if (TCPIsPutReady(smtp_socket)) {
               printf(SMTPPutCmd, "To: %s\r\n", smtp_engine_to);
               printf(SMTPPutCmd, "From: %s\r\n", smtp_engine_from);
               printf(SMTPPutCmd, "Subject: %s\r\n\r\n", smtp_engine_subject);
               TCPFlush(smtp_socket);
               smtp_state=SMTP_STATE_PUT_BODY;
               lastTick=currTick;
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 3)) {
               SMTPError(SMTP_EC_PUT_HEADER);
            }
            break;
   
         //sit in an infinite loop here.
         //but do a timeout check:
         case SMTP_STATE_PUT_START: //now the user can start email with SMTPStart()
         case SMTP_STATE_PUT_BODY:  //now the user can add their own contents to the email by using SMTPPut(). 
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_PUT_BODY");
            if (TCPIsGetReady(smtp_socket))
               TCPDiscard(smtp_socket);
            if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 60)) 
            {
               if (smtp_state == SMTP_STATE_PUT_BODY)
                  smtp_state=SMTP_STATE_FLUSH_AND_CLOSE_EMAIL;
               else
                  smtp_state=SMTP_STATE_CLOSE;
            }
            else
               break;
   
         case SMTP_STATE_FLUSH_EMAIL:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_FLUSH_EMAIL");
            flushAndClose = FALSE;
         case SMTP_STATE_FLUSH_AND_CLOSE_EMAIL:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_FLUSH_AND_CLOSE_EMAIL");
            lastTick = currTick;
            smtp_state = SMTP_STATE_FLUSH_EMAIL_WAIT;  //original
            
            //smtp_state = SMTP_STATE_FLUSH_EMAIL_CHECK;
            
            
            
            /* //idea for a delay, prob not going to work
            smtp_state = SMTP_STATE_FLUSH_EMAIL_DELAY;
   
         case SMTP_STATE_FLUSH_EMAIL_DELAY:
            if (TickGetDiff(currTick, lastTick) >= (TICKS_PER_SECOND * 2))
            {
               lastTick = currTick;
               smtp_state = SMTP_STATE_FLUSH_EMAIL_WAIT;
            }
            else
               break;
               */
         
         /*
         case SMTP_STATE_FLUSH_EMAIL_CHECK:
            if (TCPIsPutReady(smtp_socket))
            {
               if (TCPPutAvailable(smtp_socket) < 5)
               {
                  TCPFlush(smtp_socket);
                  lastTick = currTick;
               }
               else
                  lbContinue = TRUE;
               smtp_state = SMTP_STATE_FLUSH_EMAIL_WAIT;
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 3)) 
               smtp_state=SMTP_STATE_FORCE_CLOSE;
            break;
            */

         case SMTP_STATE_FLUSH_EMAIL_WAIT:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_FLUSH_EMAIL_WAIT");
            if (TCPIsPutReady(smtp_socket)) 
            {
               if (TCPPutAvailable(smtp_socket) < 5)
               {
                  TCPFlush(smtp_socket);
               }
               else
               {
                  SMTPPutCmd('\r');
                  SMTPPutCmd('\n');
                  SMTPPutCmd('.');
                  SMTPPutCmd('\r');
                  SMTPPutCmd('\n');
                  TCPFlush(smtp_socket);
      
                  smtp_state=SMTP_STATE_DO_CMD_GET_RESP;
                  smtp_expected_result=250;
                  if (flushAndClose)
                     next_state=SMTP_STATE_CLOSE;
                  else
                     next_state=SMTP_STATE_PUT_START;
                  on_err=SMTP_EC_BODY_NOT_ACCEPTED;
                  lastTick=currTick;
                  SMTPReadResultCodeReset();
               }
            }
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 3)) 
               smtp_state=SMTP_STATE_FORCE_CLOSE;
            break;
   
         case SMTP_STATE_CLOSE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_CLOSE");
            if (TCPIsPutReady(smtp_socket))
               smtp_state=SMTP_STATE_FORCE_CLOSE;
            else if (TickGetDiff(currTick, lastTick) > (TICKS_PER_SECOND * 3))
               smtp_state=SMTP_STATE_FORCE_CLOSE;
            else
               break;
   
         case SMTP_STATE_FORCE_CLOSE:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_FORCE_CLOSE");
            TCPDisconnect(smtp_socket);
   
         case SMTP_STATE_RESET:
            //printf(usb_cdc_putc,"\n\rSMTP_STATE_RESET");
            SMTPInit();
            break;
      }
   } while (lbContinue);
}

///*** read result code
#if defined(__ESMTP)
char smtp_last_word[20];
int smtp_last_word_idx;
int1 smtp_got_auth_resp;
#endif
int16 smtp_result_code_scratch;
char smtp_result_code_fnnc;   //first non-numeric char

void SMTPReadResultCodeReset(void) {
  #if defined(__ESMTP)
   smtp_last_word_idx=0;
   smtp_got_auth_resp=FALSE;
  #endif
   smtp_result_code_scratch=0;
   smtp_result_code_fnnc=0;
}

int8 SMTPReadResultCode(void) 
{
 #if defined(__ESMTP)
   char scratch[6];
 #endif
   char c;
   debug_smtp(debug_putc, "\r\nGet Result:\r\n");
   while (TCPGet(smtp_socket, &c)) {
      debug_smtp(debug_putc, "%c",c);
      if ( (c>='0') && (c<='9') && (smtp_result_code_fnnc==0) ) {
         smtp_result_code_scratch*=10;
         smtp_result_code_scratch+=c-'0';
      }
      else if (smtp_result_code_fnnc==0) {
         smtp_result_code_fnnc=c;
      }
     #if defined(__ESMTP)
      if (smtp_result_code_fnnc=='-')
      {
         smtp_last_word[smtp_last_word_idx++] = c;
         if (smtp_last_word_idx >= sizeof(smtp_last_word)) 
            smtp_last_word_idx = sizeof(smtp_last_word)-1;
         smtp_last_word[smtp_last_word_idx++] = 0;
         if (c<=' ')
         {
            if (smtp_got_auth_resp)
            {
               sprintf(scratch, "LOGIN");
               if (stricmp(scratch, smtp_last_word_idx))
                  g_AUTHIsLogin=TRUE;   

               sprintf(scratch, "PLAIN");
               if (stricmp(scratch, smtp_last_word_idx))
                  g_AUTHIsPlain=TRUE;     
            }
            else
            {
               sprintf(scratch, "AUTH");
               if (stricmp(scratch, smtp_last_word_idx))
                  smtp_got_auth_resp=TRUE;
            }
            smtp_last_word_idx=0;
         }
      }
     #endif
      if (c==0x0A) {
         if (smtp_result_code_fnnc==' ') {
            debug_smtp(debug_putc, "\r\nResult=%LU",smtp_result_code_scratch);
            g_LastSMTPResultCode = smtp_result_code_scratch;
            TCPDiscard(smtp_socket);
            return(TRUE);
         }
         else {
            debug_smtp(debug_putc, "\r\nContinue");
            SMTPReadResultCodeReset(); //read next line (some commands have multi-line responses)
         }
      }
   }
   return(FALSE);
}

///*** read result code
