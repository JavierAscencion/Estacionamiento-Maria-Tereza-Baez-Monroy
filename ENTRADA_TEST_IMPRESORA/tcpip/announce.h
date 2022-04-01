//NOTE: CCS MADE SOME API CHANGES TO THIS ANNOUNCE

/*********************************************************************
 *
 *                  Announce Module Header
 *
 *********************************************************************
 * FileName:        announce.h
 * Dependencies:    None
 * Processor:       PIC18
 * Complier:        MCC18 v1.00.50 or higher
 *                  HITECH PICC-18 V8.10PL1 or higher
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
 * Howard Schlunder     10/7/04  Original
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack
 ********************************************************************/
#ifndef ANNONCE_H
#define ANNONCE_H

/*
There are many configuration options at the top of Announce.c
*/

/*
 InitAnnounce(c)
 
 Adds char c to the announce message.  Will reset position of string if c is
 '\r'.  Ideal usage of this function will be with a printf.  For example:
      printf(InitAnnounce, "\rUNIT-%LU-%LU", SerialNumber, HTTPPort)
*/
void  InitAnnounce(char c);

/*
 AnnounceEnable()
 
 Start announce messages.  From this point on the TCP/IP stack will send an
 announce message on the configured port at the configured interval.
*/
void AnnounceEnable(void);

/*
 AnnounceDisable()
 
 Stop announce messages.
*/
void AnnounceDisable(void);
#endif
