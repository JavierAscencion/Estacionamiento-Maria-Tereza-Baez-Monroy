//////////////////////////////////////////////////////////////////////////////
///
///                              HTTP2.C
///
/// Simple webserver for the Microchip TCP/IP stack. Using web pages
/// stored on a MultiMediaCard.
/// NOTE: THIS IS A DIFFERENT HTTP.C THAN WHAT MICROCHIP PROVIDES
///
/// **** CONFIGURATION ****
///
/// STACK_USE_HTTP - Define this to be true before you include stacktsk.c
///        in your application.  Defining this to be true will cause
///        the stack to include the HTTP portion and execute the init
///        and process any HTTP tasks.
///
///
/// STACK_USE_HTTP2 - Same as STACK_USE_HTTP except this will cause
///         web pages to be loaded from files stored on the MMC, not
///        stored in program memory.
///
/// HTTP_USE_AUTHENTICATION - If set to TRUE (default is FALSE), you can
///      have some websites password protected.  In your file system (FAT
///      or MPFS) create a new file at the root called 'htaccess.txt'.  The
///      format of this file should be as follows:
///         user|password|file1|file2|file3
///      You can password protect entire directories (if your file system 
///      provides directory support).  To password protect the entire file
///      system then use / as your filename.  Even though you can provide a
///      user name and password into htaccess.txt file, the HTTP stack will also
///      call http_check_authentication() to verify the username and password.
///      It will check both locations, and if the username/password from the
///      file matches the user input, or http_check_authentication() returns
///      TRUE then authentication is granted.  The reason it checks both is in
///      case you want ot password protect a file in the field without having
///      to re-program the firmware, you just have to modify htaccess.txt to
///      add authentication.  If you want to have the HTTP stack ignore the
///      username/password from htaccess.txt then leave those fields blank.
///
///      If authentication fails the webserver will display the error401.htm
///      page.
///
/// HTTP_PORT - The TCP/IP port the HTTP server will listen to for HTTP
///        connections.
///
/// HTTP_NUM_SOCKETS - Number of sockets the stack will open for the
///        HTTP server.  You probably will be fine with just 1.  The
///        more sockets you use the more RAM is used to hold buffers
///        and state configuration.
///
/// HTTP_GET_PARAM_MAX_SIZE - This defines the maximum size of several
///        buffers.  This limits the size of your GET or POST requests
///        and all CGI POST data:
///            If using GET, then max amount of cgi data is this value
///            minus everything else on the initial GET command (which
///            also includes the filename).  I believe the max specified
///            by W3C is 255.
///            If using POST, then this is the maximum size for one
///            key=value pair (including the '=' sign).
///        These values do not inlude any escape characters.
///
///
/// HTTP_USE_DOUBLE_ESCAPE - If your HTTP pages need more escape
///      chars, set this to TRUE.  When set to TRUE your HTTP pages have
///      two escape characters after the %.  An example of each:
///         If FALSE: %A
///         If TRUE: %0A
///
///
/// **** HOW IT WORKS ****
///
/// The TCP/IP stack will open sockets to the desired ports.  It will
/// then listen for GET or POST requests.  When it gets a GET or POST
/// request it passes the page request to the callback function
/// http_get_page() which then returns 0 if the page doesn't exist, or
/// a pointer to the constant memory area that holds the page in program
/// memory.  If it was a POST request it waits until the HTTP header is
/// done and then saves the POST data into a buffer, and passes the
/// buffer to the callback function http_exec_cgi().  http_exec_cgi() will
/// parse the CGI post data and act upon it.  When done, the HTTP
/// server then responds by sending the page.  If the page is to have
/// variable data, it can be represented by an escape code - %0 or %1
/// for example.  When the HTTP stack sees such an escape code it calls
/// the callback function http_format_char() to format the escape code
/// into the needed variable data (such as ADC readings).  After the
/// HTTP stack is done sending the request it will close the port.
/// If the page didn't exist it will send a 404 File
/// not found error.  If there was a problem/timeout parsing the request
/// the HTTP stack will send a 500 Internal Server Error response.
///
/// **** CALL BACK FUNCTIONS ****
///
/// Your main application must provide the following callback functions to
/// fill application dependent needs:
///
/// http_exec_cgi(char* file, char *key, char *val);
///    A call-back function provided by your application that processes incoming
///    CGI commands.  key and val are the incoming key=val CGI pairs.  file is
///    the file name.
///
/// int http_format_char(char* file, char id, char *str, int8 max_ret);
///    Given an escaped character in the HTML file, convert to
///    variable data.  id is the escaped character, *str is where to save the
///    result.  max_ret is the maximum amount of bytes you can save to *str.
///    Returns the number of bytes written to *str.  file is
///    the file name.
///
/// int1 http_check_authentication(char *fileName, char *user, char *pwd);
///   If someone has tried to access a password protected file, the http server
///   will call this function so the application can determine if the user has
///   access.  fileName is the requested file, user is the username the user
///   entered, and pwd is the password the user entered.  The function should
///   return TRUE if access is granted, FALSE if not.  This function is only
///   needed if HTTP_USE_AUTHENTICATION is defined as TRUE.
///
/// **** LIMITATIONS ****
///
/// When creating web pages with forms, keep your form names (keys) simple
/// because the HTTP stack does not format the escape characters.  For example,
/// when sending "Pass+Word" the HTTP client will parse it out as "Pass%2bWord".
/// The HTTP stack will correctly parse out the escape chars when retrieving
/// the value, but not the key.  Therefore keep your keys simple.
///
///
/// **** FILE SUPPORT ***
/// Two file systems are supported--FAT on an MMC and Microchip's MPFS on the
/// external eeprom chip.
/// 
/// Three files must be supplied on the MMC or MPFS image.  Error404.htm will
/// be called when the file requested in the URL does not exist.Error500.htm will
/// be called on an internal server error or timeout.  Internet Exploder requires
/// that custom error pages be greater than 512 bytes in length or the browser
/// will insert its own error page in its place.  Index.htm will be called
/// when no file is explicitly specified in the URL. Other files may be included
/// on the mmc or MPFS image.  File types that are supported are .htm, .html,
/// .xml, .txt, .jpg, .png, and .gif. Filenames should be in DOS 8.3 format. 
///
////////////////////////////////////////////////////////////////////////////////
///
/// * Author         Date           Comment
/// *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// * Darren Rook    Apr 12 2007    Added HTTP_USE_AUTHENTICATION.
///                                 If file system is missing, will display a
///                                 500 error file from memory.
///                                 If client gets root (/) and index.htm is
///                                 missing, show 404 page.
///                                 Fixed bug in TCPPutFileParseConst() where
///                                 it was checking for EOF after reading a 
///                                 char, it should check for EOF before reading
///                                 from stream.
///
/// * Nick LaBonte   Feb 22 2007    Added support for MPFS
///
/// * Nick LaBonte   Jan 2007       Added MMC file support, removed HTTP_USE_CHUNKS
///
/// * Darren Rook    Oct 05 2006    Bug fix involving POST
///
/// * Darren Rook    Summer 2006    Large parts of this rewritten to fix
///                                 many bugs.  The biggest bug is that
///                                 it can send pages and receive CGI that
///                                 is larger than 1 TCP packet.  Also
///                                 added the HTTP_USE_CHUNKS,
///                                 HTTP_USE_DOUBLE_ESCAPE and
///                                 HTTP_USE_CONTENT_TYPE parameters.
///
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2006 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef STACK_USE_MPFS
#ifndef STACK_USE_FAT
#error Must define a file system to use
#endif
#endif

#if STACK_USE_MPFS&&(HTTP_NUM_SOCKETS>1)
#error Only 1 socket allowed with MPFS.  See http2.h.
#endif

char http_404_error[]="/error404.htm";
char http_500_error[]="/error500.htm";
char html_index_page[]="/index.htm";

//this will be displayed if the http_500_error[] file cannot be found on the
//file system.  useful if the file system has crashed.
const char http_500_file[]="<html><body><h1>500 Error</h1><hr><p>Internal server error.</p></body></html>";

/*
Very similar to standard strncpy(), but it adds a null termination on n+1, and n
is the entire size of the string including null termination.
*/
void _strncpy(char *s1, char *s2, size_t n)
{
   strncpy(s1,s2,n-1);
   s1[n-1]=0;
}

static int1 FTPWriteMMC = 0;

//key=val pair string, & delimited
void http_parse_cgi_str(char* file, char *cgistr);

int8 http_socket[HTTP_NUM_SOCKETS]={INVALID_SOCKET};

enum {
   HTTP_DISABLED = 0xFF,
   HTTP_IGNORE,
   HTTP_LISTEN_WAIT,
   HTTP_CONNECTED,
   HTTP_GET_HEADERS,
   HTTP_CHECK_AUTHENTICATION,
   HTTP_GET_POST,
   HTTP_GET_POST_CONTINUE,
   HTTP_SEND_RESPONSE,
   HTTP_SEND_RESPONSE_CONTINUE,
   HTTP_CLOSE,
   HTTP_CLOSE_WAITING,
   HTTP_CLOSED
} http_state[HTTP_NUM_SOCKETS]={HTTP_IGNORE};

//the fat seek functions didn't work very well, so this takes care
//of moving forward or backward within the file stream
//also define out some functions that aren't needed with FAT
#if STACK_USE_FAT
void myfatseek(FILE* stream, signed int offset){
   fatpos_t pos;
   fatgetpos(stream, &pos);
   pos+=offset;
   fatsetpos(stream, &pos);
}
#define MPFSGetEnd(x);
#define MPFSGetBegin(x);
#endif

//Macros needed for MPFS support
#if STACK_USE_MPFS
   #define moderb          (NULL)
   #define GOODEC          (0)
   #define EOF             (-1)
   #define fatpos_t        MPFS
   #define FILE            MPFS
   #define fatgetc(x)      MPFSGet()
   #define fateof(x)       MPFSIsEOF()
   void fatclose(FILE *f)     {MPFSGetEnd(f); MPFSClose();}
   #define myfatseek(x,y)  myMPFSSeek(x,y)

   //these flags are provided for compatablity with the FAT library.  the
   //only option that really is valid is READ!
   typedef enum
   {
      Closed = 0x00,
      Read = 0x01,
      Write = 0x02,
      Append = 0x04,
      Binary = 0x08,
      EOF_Reached = 0x10,
      Read_Error = 0x20,
      Write_Error = 0x40,
      File_Not_Found = 0x80
   } ioflags;
   
   signed int fatopen(char *file, ioflags iomode, FILE *retHandle)
   {
      MPFS handle;
      signed int ret=GOODEC;
      
      handle=MPFSOpen(file);
      if (handle == MPFS_INVALID)
         ret=EOF;
      else
         MPFSGetBegin(handle);
      
      *retHandle=handle;
      return(ret);
   }
#endif

int FileExists(char *file)
{
   int ret=FALSE;
   FILE handle;
 
 #if STACK_USE_MPFS
   handle=MPFSOpen(file);
   if (handle != MPFS_INVALID)
   {
      ret=TRUE;
      MPFSGetEnd(&handle); 
      MPFSClose();
   }
 #endif
 
 #if STACK_USE_FAT
   if (fatopen(file, Read|Binary, &handle)==GOODEC)
   {
      ret=TRUE;
      fatclose(&handle);
   }
 #endif
 
   return(ret);
}

#if HTTP_USE_AUTHENTICATION
   char http_401_error[]="/error401.htm";


/*
src holds a string in base64 (null terminated), this will convert that string 
to ascii and save to dest (null terminated).  if src is NULL, then it will use
dest for the source and save the result over source.
*/
void Base64ToString(char *dest, char *src)
{
   int32 j;
   int8 i,scr;
   
   if (!src)
      src=dest;
   
   while(*src)
   {
      j=0;
      for (i=0;i<4;i++)
      {
         scr=*src;
         //printf("'%c'->",scr);
         if (scr)
            src++;
         if ((scr>='A')&&(scr<='Z'))
            scr-='A';
         else if ((scr>='a')&&(scr<='z'))
            scr+=26-'a';
         else if ((scr>='0')&&(scr<='9'))
            scr+=52-'0';
         else if (scr==' ')
            scr=62;
         else if (scr=='/')
            scr=63;
         else
            scr=0;
            
         j *= (int32)64; //bit shift left 6 times
         j &= (int32)0xFFFFFFC0;
         j |= scr;
         //printf("%U [%LX]\r\n", scr,j);
      }
      dest[0]=make8(j,2);
      dest[1]=make8(j,1);
      dest[2]=make8(j,0);
      //printf("APPEND: '%c%c%c'\r\n", dest[0],dest[1],dest[2]);
      dest+=3;
   }
   *dest=0;
}

/*
Reads the htaccess file (already opened in fHandle), saving max chars to
*result.  If it reads a '|' then it returns FALSE, if it hits EOF then it 
returns TRUE.  If result is NULL then it won't save, it will just point the 
file to the next element.
*/
static int HTTPParseHtaccess(FILE *pFHandle, char *result, int max)
{
   char c;
   int ret=FALSE;
   
   max--;   //save one for null termination
   
   do
   {
      if (fateof(pFHandle))
      {
         ret=TRUE;
         break;
      }
      c = fatgetc(pFHandle);
      if (result && max && (c!='|') && (c>=' '))
      {
         *result++ = c;
         max--;
      }
   } while(c!='|');
   
   if (result)
      *result = 0;
   
   return(ret);
}   

/*
Upon request of a file, this function should be called before serving the file
to the user.  It checks to see if the file requires authentication, and if it
does it sees if the user provided username/password combination passes.  *page
contains the requested page, *user contains the username/password combination
from the HTTP header (in base64).  If the user hasn't provided a 
username/password then user will be set to NULL.  This function will return 
TRUE if authentication fails, in which case the server will respond with 
Error 401.
If you are using MPFS, it must be free at this time and be able to open a file
else the results will be TRUE.  
If the htaccess format is not valid this will always return TRUE.
If it cannot find htaccess file it will assume that no file requires 
authentication.
*/
int HTTPRequiresAuthentication(char *page, char *user)
{
   static char htaccess[]="/htaccess.txt";
   static char token[]=":";
   char *pwd;
   FILE fHandle;
   char userFromFile[30], pwdFromFile[30], fileToCheck[30];
   int eof,i;
   int ret=TRUE;


   //printf("\r\nCHECK FILE '%s'\r\n", page);
  
  #if STACK_USE_MPFS
   if (MPFSIsInUse())
   {
      //printf("\r\nMPFSINUSE\r\n");
      return(TRUE);
   }
  #endif
   
   if (fatopen(htaccess, Read|Binary, &fHandle) == EOF)
   {
      //printf("\r\nNOHTACCESS '%s'\r\n", htaccess);
      return(FALSE);
   }
   
   if (HTTPParseHtaccess(&fHandle, userFromFile, sizeof(userFromFile)))
      goto __HTTPRequiresAuthentication_Cleanup;
   if (HTTPParseHtaccess(&fHandle, pwdFromFile, sizeof(pwdFromFile)))
      goto __HTTPRequiresAuthentication_Cleanup;
  
   while(TRUE)
   {
      //printf("\r\nFINDING FILE\r\n");
      eof = HTTPParseHtaccess(&fHandle, fileToCheck, sizeof(fileToCheck));
     
      
     #if STACK_USE_MPFS
      strupr(fileToCheck);
     #endif
      
      //strip any any whitespace chars at the end of the file
      i=strlen(fileToCheck);
      while(i--)
      {
         if (fileToCheck[i] <= ' ')
            fileToCheck[i]=0;
         else
            break;
      }

      //printf("\r\nFILE (%U) = '%s'\r\n", eof, fileToCheck);
      if (
            (strcmp(fileToCheck, page)==0)   //file is a perfect match
                     ||
            (  //check for subdirectory
               (fileToCheck[i]=='/') &&   //if file ends in /, it is a directory
               (strstr(page, fileToCheck)==page)
            )
         )
      {
         //printf("\r\nCONVERTING 64: '%s'->", user);
         Base64ToString(user, NULL);         
         //printf("'%s'\r\n", user);
         user=strtok(user,token);
         pwd=strtok(0,token);
         return
            (
               !(
                  (
                     strlen(userFromFile) && 
                     strlen(pwdFromFile) && 
                     (stricmp(userFromFile,user)==0) &&
                     (stricmp(pwdFromFile,pwd)==0)
                  ) ||
                  http_check_authentication(page,user,pwd)
                )
            );
         //if (user && pwd && (strcmp(user,validUser)==0) && (strcmp(pwd,validPwd)==0))
         //   ret=FALSE;
         break;
      }
      
      if (eof)
      {
         ret=FALSE;
         break;
      }      
   }

__HTTPRequiresAuthentication_Cleanup:
   fatclose(&fHandle);
   return(ret);
}
#endif //HTTP_USE_AUTHENTICATION

//strips out any escape characters that the HTTP client may have inserted.
// (+ is replaced with space)
// (%xx is replaced with character representation of xx)
void http_escape_chars(char *str)
{
   char new[3];
   char c;
   char val;

   new[2]=0;

   while((c=*str) != 0)
   {
      if (c=='+')
         *str++=' ';
      else if (c=='%')
      {
         memcpy(new, str + 1, 2);
         val = strtoul(new, 0, 16);
         *str++ = val;
         memmove(str, str + 2, strlen(str) - 1);
      }
      else
         str++;
   }
}

void http_parse_cgi_string(char* file, char *ptr)
{
   char *pKey, *pValue, c;

   pKey=ptr;

   while(TRUE)
   {
      c = *ptr;
      if ((c=='&') || (c==0))
      {
         *ptr=0;
         http_escape_chars(pKey);
         http_escape_chars(pValue);
         http_exec_cgi(file, pKey, pValue);
         pKey=ptr+1;
         if (c==0)
            break;
      }
      else if (c=='=')
      {
         *ptr=0;
         pValue=ptr+1;
      }
      ptr++;
   }
}

int8 _httpPutcSocket;

#define tcp_http_tx_left()  TCPPutAvailable(_httpPutcSocket)

void set_tcp_http_putc(int8 newSocket)
{
   _httpPutcSocket=newSocket;
}

int tcp_http_putc(char c)
{
   //putc(c);
   return(TCPPut(_httpPutcSocket,c));
}
FILE lastHTTPPutConstPos[HTTP_NUM_SOCKETS];
char * lastHTTPPutVarPos[HTTP_NUM_SOCKETS];
static int1 is_image;

typedef enum
{
   TCP_PUT_CONST_EC_FINISH = 0,
   TCP_PUT_CONST_EC_CONTINUE,
   TCP_PUT_CONST_EC_ESCAPE
} TCP_PUT_CONST_EC;

//////////////////////////////////////////////////////////////////////////////
//
// TCPPutFileVarChunk(ptr)
//
// send a chunked response from ram
//
// ptr - data to send (well be sent as an http/1.1 chunk).  It will be updated
//          with the continue position before exit.
//
// Returns a status code:
//    TCP_PUT_CONST_EC_FINISH - The whole chunk was completed
//    TCP_PUT_CONST_EC_CONTINUE - The chunk needs to be continued
//
//////////////////////////////////////////////////////////////////////////////
//TCP_PUT_CONST_EC TCPPutFileVarChunk(char **retPtr)
//because of a compiler error this had to be a double pointer an int16, when
//it should be a char.

TCP_PUT_CONST_EC TCPPutFileVarChunk(int16 **retPtr)
{
   int16 txLeft, n;
   char *ptr, ec;

   ptr=*retPtr;

   n=strlen(ptr);
   
   if (!n){
      return(TCP_PUT_CONST_EC_FINISH);
   }

   txLeft = tcp_http_tx_left();
  
   if (n > txLeft)
      ec = TCP_PUT_CONST_EC_CONTINUE;   
   else
   {
      txLeft = n;
      ec = TCP_PUT_CONST_EC_FINISH;
   }

   //TCPPutFileChunkStart(txLeft);

   while (txLeft--)
      tcp_http_putc(*(ptr++));

   //TCPPutFileChunkStop();

   *retPtr=ptr;

   return(ec);
}


//////////////////////////////////////////////////////////////////////////////
//
// TCPPutFileParseConst(fstream, n, doSend)
//
// Reads file from mmc.  Stops when it reaches an escape sequence,
// n chars or an end of file
//
// fstream - file sream to read (will be sent as an http/1.1 chunk).  The stream
//        will be updated for the next call.
//
// n - max number of chars to read from file.  will save the total number
//    of chars passed to this pointer.
//
// doSend - if TRUE, then send data to TCP socket
//
// returns the last char read
//
//////////////////////////////////////////////////////////////////////////////
char TCPPutFileParseConst(FILE *fstream, int16 *n, int8 doSend)
{
   char stopC, checkC;
   int16 fileSize = 1, max;
   int1 premature = TRUE;
   max = *n;
   MPFSGetBegin(*fstream);
   while (TRUE)
   {
      if(fateof(fstream))//EOF
      {
         premature = FALSE;
         break;   
      }
      stopC= fatgetc(fstream);
      if (stopC == '%'&&!is_image)
      {
         checkC= fatgetc(fstream);
         if (checkC == '%')
         {
            if (fileSize < max)
            {
               if (doSend)
                  tcp_http_putc('%');
               fileSize++;
            }
            else
               break;
         }
         else
            break;   //ESCAPE
      }
      else
      {
        #if defined(STACK_USE_MPFS) && STACK_USE_MPFS
         if (fateof(fstream))
         {
            premature = FALSE;
            break;
         }
        #endif
         if (((fileSize) < max))
         {
            //if (doSend&&!fateof(fstream))
            if (doSend)
            {
               tcp_http_putc(stopC);
            }
            fileSize++;
         }
         else
            break;
      }
      /*
      if(fateof(fstream))//EOF
      {
         premature = FALSE;
         break;   
      }
      */
   }
   
   MPFSGetEnd(fstream);
   if (premature==TRUE){
      myfatseek(fstream, -1);  //addy--;
   }
 
   *n = fileSize;
   MPFSGetEnd(fstream);
   return(stopC);
}

//////////////////////////////////////////////////////////////////////////////
//
// TCPPutFileConstChunk(*addy)
//
// send a chunked response from mmc
//
// addy - data to send (well be sent as an http/1.1 chunk).  This address
//        will be updated for the next call.
//
// returns a response:
//    TCP_PUT_CONST_EC_CONTINUE: we ran out of space to put more data, continue
//                            from this position on next call.
//    TCP_PUT_CONST_EC_ESCAPE:   an escape character
//    TCP_PUT_CONST_EC_FINISH:   we finished reading the chunk, end of file.
//
//////////////////////////////////////////////////////////////////////////////
TCP_PUT_CONST_EC TCPPutFileConstChunk(FILE *retAddy)
{
   char stopC;
   int16 txLeft;
   TCP_PUT_CONST_EC ec;
   MPFSGetBegin(*retAddy);
   txLeft = tcp_http_tx_left();

   if (!txLeft)
      return(TCP_PUT_CONST_EC_CONTINUE);
   
   stopC=TCPPutFileParseConst(retAddy, &txLeft, TRUE);
   if (stopC == '%'&&!is_image)
   {
      ec = TCP_PUT_CONST_EC_ESCAPE;
   }
   else
   {
      ec = TCP_PUT_CONST_EC_CONTINUE;
   }
   
   if(fateof(retAddy))//EOF
   {
      ec = TCP_PUT_CONST_EC_FINISH;
   }
   MPFSGetEnd(retAddy);
   return(ec);
}

int TCPPutFileConstGetEscape(FILE* fstream)
{
#if HTTP_USE_DOUBLE_ESCAPE
   char str[3];
   int ret;
   MPFSGetBegin(*fstream);
   str[0]=fatgetc(fstream);
   str[1]=fatgetc(fstream);
   myfatseek(fstream, -2);

   str[2] = 0;
   ret = strtol(str, 0, 16);
#else
   char ret;
   MPFSGetBegin(*fstream);
   ret=fatgetc(fstream);
   MPFSGetEnd(fstream);
   myfatseek(fstream, -1);//"replace" that char to the stream
#endif
   MPFSGetEnd(fstream);
   return(ret);
}

int1 tcp_http_put_file(int8 which, int16 errorCode, FILE* MMCfile, char* fname)
{
   static char str[40];
   int8 socket;
   char ec;
   int escaped;
   static char extHTML[]="html";
   static char extHTM[]="htm";
   static char extTXT[]="txt";
   static char extXML[]="xml";
   static char extGIF[]="gif";
   static char extJPG[]="jpg";
   static char extPNG[]="png";
   static char extICO[]="ico";
   char extension[5];
   
   fatpos_t position;   

   static enum
   {
      HTTP_PUT_FILE_INIT = 0,
      HTTP_PUT_FILE_CONTINUE,
      HTTP_PUT_FILE_CHUNK_END,
      HTTP_PUT_FILE_DONE
   } status;
   socket=http_socket[which];

  MPFSGetBegin(*MMCfile); 
   set_tcp_http_putc(socket);
#if STACK_USE_FAT
   fatgetpos(MMCfile, &position);
#elif STACK_USE_MPFS
   position = MPFSTell()-MPFSFileTell();
#endif
  // printf("position: %lx\r\n", position);
   //printf("\r\n** PUT FILE W%U/S%U '%s' EC=%LU MRK=%LX POS=%LX\r\n", which, socket, fname, errorCode, *MMCfile, position);
   if(position == 0)
   {
      lastHTTPPutVarPos[which] = 0;
      lastHTTPPutConstPos[which] = *MMCfile;
      status = HTTP_PUT_FILE_CONTINUE;

    #if HTTP_USE_CHUNKS
      printf(tcp_http_putc,"HTTP/1.1 %LU ", errorCode);
    #else
      printf(tcp_http_putc,"HTTP/1.0 %LU ", errorCode);
    #endif

      switch(errorCode)
      {
         case 200:
            printf(tcp_http_putc,"OK");
            break;
         case 401:
            printf(tcp_http_putc,"Authorization Required");
            break;
         case 404:
            printf(tcp_http_putc,"Not Found");
            break;
         case 500:
            printf(tcp_http_putc,"Server Error");
            break;
         default:
            break;
      }

     #if HTTP_USE_AUTHENTICATION
      if (errorCode==401)
         printf(tcp_http_putc, "\r\nWWW-Authenticate: Basic realm=\"Authorization Required\"");
     #endif

      printf(tcp_http_putc, "\r\nContent-Type: ");
      
      if(strlen(strchr(fname,'.'))<6){
         strncpy (extension, (strchr(fname,  '.'))+1,  4);
      }
      if(stricmp(extension, extHTML)==0||stricmp(extension, extHTM)==0){
         printf(tcp_http_putc,"text/html");
         is_image=0;
      }
      else if(stricmp(extension,extXML)==0){
         printf(tcp_http_putc,"text/xml");
         is_image=0;
      }
      else if(stricmp(extension,extTXT)==0){
         printf(tcp_http_putc,"text/plain");
         is_image=0;
      }
      else if(stricmp(extension,extGIF)==0){
         printf(tcp_http_putc,"image/gif");
         is_image=1;
      }
      else if(stricmp(extension,extJPG)==0){
         printf(tcp_http_putc,"image/jpeg");
         is_image=1;
      }
      else if(stricmp(extension,extPNG)==0){
         printf(tcp_http_putc,"image/png");
         is_image=1;
      }
      else if(stricmp(extension,extICO)==0){
         printf(tcp_http_putc,"image/x-icon");
         is_image=1;
      }
      else{ 
         printf(tcp_http_putc,"text/plain");
         is_image=0;
      }
      printf(tcp_http_putc, "\r\n");
      
      if (is_image)
         printf(tcp_http_putc, "Last-Modified: Tue, 15 Nov 1994 12:45:26 GMT\r\n");
         
      printf(tcp_http_putc, "\r\n");
   }
   
   if ((errorCode==500) && (!FileExists(http_500_error)))
   {
      printf(tcp_http_putc, "%s", http_500_file);
      TCPFlush(socket);
      return(TRUE);
   }

   if (lastHTTPPutVarPos[which]!=0)
   {
      TCPPutFileVarChunk(&lastHTTPPutVarPos[which]);
      lastHTTPPutVarPos[which] = 0;
   }

   if (status == HTTP_PUT_FILE_CONTINUE)
   {
      do {
         ec = TCPPutFileConstChunk(&lastHTTPPutConstPos[which]);

         if (ec == TCP_PUT_CONST_EC_ESCAPE)//&is_image
         {
            escaped = TCPPutFileConstGetEscape(&lastHTTPPutConstPos[which]);////
           #if HTTP_USE_DOUBLE_ESCAPE
               myfatseek(&lastHTTPPutConstPos[which], 2);
           #else
               myfatseek(&lastHTTPPutConstPos[which], 1);
           #endif
            http_format_char(fname, escaped, &str[0], sizeof(str)-1);
            lastHTTPPutVarPos[which] = &str[0];
            ec = TCPPutFileVarChunk(&lastHTTPPutVarPos[which]);
            if (ec == TCP_PUT_CONST_EC_FINISH)
               lastHTTPPutVarPos[which] = 0;
            else
               break;
         }
         else
         {
            if (ec == TCP_PUT_CONST_EC_FINISH)
               status = HTTP_PUT_FILE_CHUNK_END;
            break;
         }
      } while (TRUE);
   }

   if (status == HTTP_PUT_FILE_CHUNK_END)
   {
     #if HTTP_USE_CHUNKS
      if (tcp_http_tx_left() > 8)
      {
         TCPPutFileChunkStart(0);
         TCPPutFileChunkStop();
         status = HTTP_PUT_FILE_DONE;
      }
     #else
      status = HTTP_PUT_FILE_DONE;
     #endif
   }

#if STACK_USE_FAT
   memcpy(MMCfile, &lastHTTPPutConstPos[which], sizeof(lastHTTPPutConstPos[which]));
#else
   MPFSGetEnd(MMCfile);
#endif
   //printf("\r\nFLUSH W%u/S%u/T%u", which,socket,_httpPutcSocket);
   ec=TCPFlush(socket);
   //printf(" RETURN=%U\r\n", ec);
   return(status == HTTP_PUT_FILE_DONE);
}

//initializes the HTTP state machine.  called automatically by the TCP/IP stack
void HTTP_Init(void) {
   int8 i;
   //fprintf(USER,"\r\nHTTP OPENING");
   if (HTTP_PORT != 0)
   {
      for (i=0;i<HTTP_NUM_SOCKETS;i++)
      {
         http_socket[i]=TCPListen(HTTP_PORT);
         //fprintf(USER,"\r\nHTTP SOCKET=%X", http_socket[i]);
         if (http_socket[i]!=INVALID_SOCKET)
         {
            http_state[i]=HTTP_LISTEN_WAIT;
         }
      }
   }
   else
   {
      //fprintf(USER,"\r\nHTTP DISABLED");
      for (i=0;i<HTTP_NUM_SOCKETS;i++)
      {
            http_state[i]=HTTP_DISABLED;
      }
   }
}

#DEFINE HTTP_INDEX_PAGE_EC 0x01
#DEFINE HTTP_FILE_PAGE_EC    0x02
#DEFINE HTTP_404_PAGE_EC    0x00
#DEFINE HTTP_401_PAGE_EC    0xFE
#DEFINE HTTP_500_PAGE_EC    0xFF

//Now these are global--not initialized to 0
//save a couple hundred bytes of ROM this way
char HTTPbuffer[HTTP_NUM_SOCKETS][HTTP_GET_PARAM_MAX_SIZE];
FILE http_page_req[HTTP_NUM_SOCKETS];

#if HTTP_USE_AUTHENTICATION
 char http_get_cache[HTTP_NUM_SOCKETS][HTTP_GET_PARAM_MAX_SIZE];
 char http_auth_user[HTTP_NUM_SOCKETS][HTTP_GET_PARAM_MAX_SIZE];
#endif

void HTTP_Task(void) {
   static char tokens_header[]=" ";
   static char tokens_get[]="?";
   static char http_get_str[]="GET";
   static char http_post_str[]="POST";
   static char http_len_str[]="Content-Length:";
   int8 j =0, l=0;
   
   char index[]="/";
   static char page[20];
   FILE fstream;

   //static char http_keepalive_str[]="keep-alive";
   //static char http_connection_str[]="Connection";



   static int8 i[HTTP_NUM_SOCKETS];
   static enum {HTTP_REQ_GET=1, HTTP_REQ_POST=2, HTTP_REQ_UNKOWN=0} http_cmd[HTTP_NUM_SOCKETS]={0};
   
   static int16 http_error_code[HTTP_NUM_SOCKETS];
   static int16 http_post_len[HTTP_NUM_SOCKETS]={0};
   static int16 http_timer[HTTP_NUM_SOCKETS];
 
 #if HTTP_USE_AUTHENTICATION
   static char http_auth_str[]="Authorization:";
 #endif

   int1 doneSend, postContinue;

   char c, *pKey, *pValue;
   int8 hs, currSocket;

   for (hs=0; hs<HTTP_NUM_SOCKETS; hs++)
   {
      if (http_state[hs]==HTTP_DISABLED)
         return;

      currSocket=http_socket[hs];

      if (!TCPIsConnected(currSocket))
         http_state[hs]=HTTP_LISTEN_WAIT;

      switch(http_state[hs])
      {
         case HTTP_LISTEN_WAIT:
            if (!TCPIsConnected(currSocket))
               break;
//            fprintf(USER,"HTTP %U CONNECTED\r\n", hs);

         //wait until we get '\r\n\r\n', which marks the end of the HTTP request header
         case HTTP_CONNECTED:
//            fprintf(USER,"HTTP %U LISTENING\r\n", hs);
            /*
            printf("\r\nHTTP OPEN %U/%U ", hs, currSocket);
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr)
               printf(" MAC=");
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);;
               printf("\r\n");
               */
               
            HTTPbuffer[hs][0]=0;
            i[hs]=0;
            http_state[hs]=HTTP_GET_HEADERS;
            http_timer[hs]=TickGet();
            http_page_req[hs]=0;
            http_post_len[hs]=0;
           #if HTTP_USE_AUTHENTICATION
            http_auth_user[hs][0]=0;
           #endif
            //http_got_headers[hs]=FALSE;
            //http_isKeepAlive[hs]=FALSE;

         case HTTP_GET_HEADERS:
            if(FTPWriteMMC)
               break;
            postContinue=FALSE;
            while (TCPIsGetReady(currSocket) && TCPGet(currSocket, &c))
            {
               //http_got_headers[hs]=TRUE;
               if ( (c >= 0x20) && (i[hs] < HTTP_GET_PARAM_MAX_SIZE - 2) )
               {
                  HTTPbuffer[hs][i[hs]++]=c;
               }
               if (c=='\n')
               {
                  HTTPbuffer[hs][i[hs]]=0;
                  if (
                       ( ( pKey = strtok(&HTTPbuffer[hs][0], tokens_header) ) !=0 ) &&
                       ( ( pValue = strtok(0, tokens_header) ) !=0 )
                     )
                  {
                     //fprintf(USER,"HTTP %U PAIR %s = %s\r\n", hs, pKey, pValue);
                     if ( (strcmp(pKey, http_get_str)==0) || (strcmp(pKey, http_post_str)==0) )
                     {
                        if (strcmp(pKey, http_get_str)==0){
                           http_cmd[hs]=HTTP_REQ_GET;
                           //printf("\r\nGET ");
                        }
                        else{
                           http_cmd[hs]=HTTP_REQ_POST;
                           //printf("\r\nPOST ");
                        }
                        /*
               printf(" IP=");
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);
               printf(" MAC=");
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr);
               printf(" ");
               */
                                            
                        pValue=strtok(pValue, tokens_get);  //chop the file after a ?
                        http_escape_chars(pValue);

                        if (stricmp(pValue,index)==0)
                        {
                           //special case:index page
                           //printf("INDEX ");
                           _strncpy(
                                 pValue, 
                                 html_index_page, 
                                 (sizeof(HTTPbuffer)/HTTP_NUM_SOCKETS) - (pValue-&HTTPbuffer[hs][0]),
                              );
                        }
                        else
                        {
                           j=strlen(pValue);
                           l=j;
                           while(pValue[--j] == 47&& j!=0){//strip out trailing '/' chars
                              pValue[j] = 0;   //this may have to be changed if
                           }               //we want to process directories
                        }

/*
#if STACK_USE_MPFS//strip out leading '/' char to satisfy MPFS
                        if(l>1){
                           for(j=0; j<=strlen(pValue); ++j){
                              pValue[j]=pValue[j+1]; 
                           }
                        }
#endif
*/
                        //printf("'%s' ", pValue);
                        
                        //TODO: do i need the following line?
                        fatclose(&fstream);

                        if (FileExists(pValue))
                        {
                           //printf("FOUND");
                           _strncpy(page, pValue, sizeof(page));
                              //TODO directory listing?
                           http_error_code[hs]=200;
                           //printf("!\r\n");
                        }
                        else
                        {
                           //printf("MISSING");
                           _strncpy(page, http_404_error, sizeof(page));
                           http_error_code[hs]=404;
                           //printf("!\r\n");
                        }
               /*
               printf(" IP=");
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);
               printf(" MAC=");
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr);
               printf(" ");
               */

//                      fprintf(USER,"HTTP %U PVALUE: %s FILE OPENED: %s, handle:%LX\r\n", hs, pValue, page, fstream);
                        pValue=strtok(0, tokens_get);
                       #if HTTP_USE_AUTHENTICATION
                       //we cant execute this until we authorize
                        _strncpy(&http_get_cache[hs][0],pValue,(sizeof(http_get_cache)/HTTP_NUM_SOCKETS));
                       #else
                        if (pValue && (http_error_code[hs]==200))
                        {
                           http_parse_cgi_string(page, pValue);
                        }
                       #endif
                     }
                     else if (http_cmd[hs] != HTTP_REQ_UNKOWN)   //we processed a GET or POST
                     {
                        //if you want to parse HTTP headers, do it here.
                        //pKey and pVal hold the individual headers.

                        //parse the Content-Length header.
                        if (strcmp(pKey, http_len_str)==0)
                           http_post_len[hs]=atol(pValue);
                      #if HTTP_USE_AUTHENTICATION                        
                        //parse the Authorization header.
                        if (strcmp(pKey, http_auth_str)==0)
                        {
                           _strncpy(http_auth_user, pValue+6,(sizeof(http_auth_user)/HTTP_NUM_SOCKETS));
                           //printf("\r\nAuth Attempt '%s'->'%s'\r\n", pValue, &http_auth_user[hs][0]);
                        }
                      #endif
                     }
                  }
                  else if (i[hs] == 0)
                  {
                     //got a double \r\n
                     //fprintf(USER,"HTTP %U GET HEADER DONE\r\n", hs);
                     if (http_cmd[hs] == HTTP_REQ_POST)
                     {
                      #if HTTP_USE_AUTHENTICATION
                        http_state[hs]=HTTP_CHECK_AUTHENTICATION;
                      #else
                        http_state[hs]=HTTP_GET_POST;
                      #endif
                        postContinue=TRUE;
                     }
                     else
                     {
                      #if HTTP_USE_AUTHENTICATION
                        http_state[hs]=HTTP_CHECK_AUTHENTICATION;
                      #else
                        http_state[hs]=HTTP_SEND_RESPONSE;
                      #endif
                        TCPDiscard(currSocket);
                     }
                     break;   //break out of read from ethernet loop
                  }
                  i[hs]=0;
               }
            }
            if (TickGetDiff(TickGet(),http_timer[hs]) > TICKS_PER_SECOND*20)
            {
               //if (http_got_headers[hs])
               //{
                  debug_printf("HTTP %U GET HEADER TIMEOUT\r\n", hs);
                  strcpy(page, http_500_error);
                  http_error_code[hs]=500;
                  http_state[hs]=HTTP_SEND_RESPONSE;
               //}
               //else
               //{
               //   http_state[hs]=HTTP_CLOSE;
               //}
            }
            if (!postContinue)
               break;

      #if HTTP_USE_AUTHENTICATION
         case HTTP_CHECK_AUTHENTICATION:
            http_state[hs]=HTTP_SEND_RESPONSE;
            postContinue=FALSE;
            if (HTTPRequiresAuthentication(page, &http_auth_user[hs][0]))
            {
               //printf("\r\nAUTH REQUIRED\r\n");
               _strncpy(page, http_401_error, sizeof(page));
               http_error_code[hs]=401;
            }
            else if (http_cmd[hs] == HTTP_REQ_POST)
            {
               http_state[hs]=HTTP_GET_POST;
               postContinue=TRUE;
            }
            if (!postContinue)
               break;
      #endif

         case HTTP_GET_POST:
//            fprintf(USER,"HTTP %U GET POST %LU\r\n", hs, http_post_len[hs]);
            http_state[hs]=HTTP_GET_POST_CONTINUE;
            http_timer[hs]=TickGet();
            i[hs]=0;

         case HTTP_GET_POST_CONTINUE:
            while (
                     TCPIsGetReady(currSocket) &&
                     TCPGet(currSocket, &c) &&
                     (http_post_len[hs] != 0)
                  )
            {
               http_post_len[hs] -= 1;

               if (c!='&')
               {
                  HTTPbuffer[hs][i[hs]++]=c;
               }

               if ( (c=='&') || (http_post_len[hs] == 0) )
               {
                  HTTPbuffer[hs][i[hs]]=0;

                  //fprintf(USER,"%lu - %s\r\n", http_post_len[hs], &HTTPbuffer[hs][0]);

                  http_parse_cgi_string(page, &HTTPbuffer[hs][0]);
                  if (http_post_len[hs] == 0)
                  {
                     http_state[hs]=HTTP_SEND_RESPONSE;
                     TCPDiscard(currSocket);
                     break;
                  }
                  else
                     i[hs]=0;
               }
            }
            if (TickGetDiff(TickGet(),http_timer[hs]) > TICKS_PER_SECOND*20)
            {
               debug_printf("HTTP %U GET POST TIMEOUT\r\n", hs);
               _strncpy(page, http_500_error, sizeof(page));
               http_error_code[hs]=500;
               http_state[hs]=HTTP_SEND_RESPONSE;
            }
            break;

         case HTTP_SEND_RESPONSE:
            //putc('.');
            if(FTPWriteMMC)
               break;
//            fprintf(USER,"HTTP %U SEND RESPONSE\r\n", hs);
            //putc('*');
            if (fatopen(page, Read|Binary, &fstream) == EOF)
            {
               debug_printf("FILE '%s' OPEN FAILURE\r\n", page);
               _strncpy(page, http_500_error, sizeof(page));
               fatopen(page, Read|Binary, &fstream);
               http_error_code[hs]=500;            
            }
            http_page_req[hs]=fstream;
            lastHTTPPutConstPos[hs]=0;
            http_state[hs]=HTTP_SEND_RESPONSE_CONTINUE;

         case HTTP_SEND_RESPONSE_CONTINUE:
            TCPDiscard(currSocket);
            //putc('@');
             if (TCPIsPutReady(currSocket))
            {               
               //printf("\r\nPut Page Segment %U/%U EC=%LU '%s' %LX ", hs, currSocket, http_error_code[hs], page, http_page_req[hs]);
               /*
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);
               printf(" MAC=");
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr);
               printf("\r\n");
               */
               
//               fprintf(USER,"\r\nPUTTING HTTP SEG\r\n");
               doneSend=tcp_http_put_file(hs, http_error_code[hs], &http_page_req[hs], page);
               if (doneSend!=0)
               {
                  //if (http_isKeepAlive[hs])
                  //   http_state[hs]=HTTP_CONNECTED;
                  //else
                  //   http_state[hs]=HTTP_CLOSE;
                  http_state[hs] = HTTP_CLOSE;
                  
                  fatclose(&fstream);
//                  fprintf(USER,"HTTP %U RESPONSE SENT\r\n", hs);
               }
            }
            break;

         case HTTP_CLOSE:
            //since we set connection: close in the header, the client
            //should automatically close.  but after so many seconds we
            //shall kill the connection
            /*
            printf("\r\nClosing HTTP %U/%U.... ", hs, currSocket);
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);
               printf(" MAC=");
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr);
               printf("\r\n");
               */
            
            http_state[hs]=HTTP_CLOSE_WAITING;
            http_timer[hs]=TickGet();
            
         case HTTP_CLOSE_WAITING:
            TCPDiscard(currSocket);
            if (  TCPIsPutReady(currSocket) ||
                  (TickGetDiff(TickGet(),http_timer[hs]) > (TICKS_PER_SECOND*5))
               )
            {
               /*
               if (!TCPIsPutReady(currSocket))
               {
                  printf("\r\nWIN:%LX B:%U PUT:%U\r\n",
                        TCB[currSocket].RemoteWindow,
                        TCB[currSocket].TxBuffer,
                        TCB[currSocket].Flags.bIsPutReady
                     );
               }
               //fprintf(USER,"HTTP %U FORCE CLOSED\r\n", hs);
               printf("\r\nClosed HTTP %U/%U IP=", hs, currSocket);
               ui_disp_mac_user(&TCB[currSocket].remote.MACAddr);
               printf(" MAC=");
               ui_disp_ip_user(&TCB[currSocket].remote.IPAddr);
               printf("\r\n");
               */
               TCPDisconnect(currSocket);
               http_state[hs]=HTTP_CLOSED;
            }
            break;

         case HTTP_CLOSED: //do nothing until socket actually closes
         default:
            break;
      }
   }
}
