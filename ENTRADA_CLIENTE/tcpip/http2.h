///////////////////////////////////////////////////////////////////////////
///                                                                     ///
///                              HTTP2.H                                ///
///                                                                     ///
/// Simple webserver for the Microchip TCP/IP stack using web pages     /// 
/// on an MultiMediaCard.                                                ///
/// NOTE: THIS IS A DIFFERENT HTTP.H THAN WHAT MICROCHIP PROVIDES       ///
///                                                                     ///
/// See HTTP.C for documenation                                         ///
///                                                                     ///
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2006 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////


#ifndef HTTP_USE_CHUNKS
#define HTTP_USE_CHUNKS   FALSE
#endif

#ifndef HTTP_USE_DOUBLE_ESCAPE
#define HTTP_USE_DOUBLE_ESCAPE   FALSE
#endif

#ifndef HTTP_PORT
#define HTTP_PORT             80
#endif

#ifndef HTTP_NUM_SOCKETS
#define HTTP_NUM_SOCKETS      1
#endif

#ifndef HTTP_GET_PARAM_MAX_SIZE
#define HTTP_GET_PARAM_MAX_SIZE  254
#endif

#ifndef HTTP_USE_AUTHENTICATION
#define  HTTP_USE_AUTHENTICATION FALSE
#endif

void HTTP_Init(void);
void HTTP_Task(void);

//**** CALLBACKS START ******///

/// the following two functions are callbacks and
/// must be written in your main application!!!  see the documentation
/// in http2.c for more help.

int8 http_format_char(char* file, char id, char *str, int8 max_ret);
void http_exec_cgi(char* file, char *key, char *val);

#if HTTP_USE_AUTHENTICATION
 int1 http_check_authentication(char *fileName, char *user, char *pwd);
#endif

//**** CALLBACKS END ******///
