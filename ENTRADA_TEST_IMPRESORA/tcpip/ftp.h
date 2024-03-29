/*********************************************************************
 *
 *              FTP Server Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ftp.h
 * Dependencies:    StackTsk.h
 *                  tcp.h
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
 * Nick LaBotne         2/14/07  CCS Port
 ********************************************************************/

#ifndef FTP_H
#define FTP_H


#define FTP_USER_NAME_LEN       (10)


/*********************************************************************
 * Function:        BOOL FTPVerify(char *login, char *password)
 *
 * PreCondition:    None
 *
 * Input:           login       - Telnet User login name
 *                  password    - Telnet User password
 *
 * Output:          TRUE if login and password verfies
 *                  FALSE if login and password does not match.
 *
 * Side Effects:    None
 *
 * Overview:        Compare given login and password with internal
 *                  values and return TRUE or FALSE.
 *
 * Note:            This is a callback from Telnet Server to
 *                  user application.  User application must
 *                  implement this function in his/her source file
 *                  return correct response based on internal
 *                  login and password information.
 ********************************************************************/

BOOL FTPVerify(char *login, char *password);



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
 * Overview:        Initializes internal variables of Telnet
 *
 * Note:
 ********************************************************************/
void FTPInit(void);


/*********************************************************************
 * Function:        void FTPTask(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened Telnet connections are served.
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
BOOL FTPTask(void);


BOOL DeleteFile(void);

extern void lcd_putc( char c);


#endif
