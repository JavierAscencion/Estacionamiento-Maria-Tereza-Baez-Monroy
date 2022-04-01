/*********************************************************************
 *
 *                  Helper Functions for Microchip TCP/IP Stack
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        Helpers.C
 * Dependencies:    compiler.h
 *                  helpers.h
 * Processor:       PIC18
 * Complier:        CCS PCH 3.181 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date     Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     5/17/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     6/25/02  Rewritten CalcIPChecksum() to avoid
 *                               multi-byte shift operation.
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 * Darren Rook (CCS)    05/24/04 swaps() and swapl() optimized
 * Howard Schlunder      2/9/05   Added hexatob(), btohexa_high(), and
 *                        btohexa_low()
 * Darren Rook (CCS)    07/13/06 In synch with Microchip's V3.02 stack
 * Darren Rook (CCS)    10/24/06 In synch with Microchip's V3.75 stack 
 ********************************************************************/

#include "tcpip/helpers.h"
#include "tcpip/mac.h"

/*********************************************************************
* Function:        BYTE hexatob(WORD_VAL AsciiChars)
*
* PreCondition:    None
*
* Input:           Two ascii bytes; each ranged '0'-'9', 'A'-'F', or
*                  'a'-'f'
*
* Output:          The resulting packed byte: 0x00-0xFF
*
* Side Effects:    None
*
* Overview:        None
*
* Note:         None
********************************************************************/
BYTE hexatob(WORD_VAL AsciiChars)
{
   // Convert lowercase to uppercase
   if(AsciiChars.v[1] > 'F')
      AsciiChars.v[1] -= 'a'-'A';
   if(AsciiChars.v[0] > 'F')
      AsciiChars.v[0] -= 'a'-'A';

   // Convert 0-9, A-F to 0x0-0xF
   if(AsciiChars.v[1] > '9')
      AsciiChars.v[1] -= 'A' - 10;
   else
      AsciiChars.v[1] -= '0';

   if(AsciiChars.v[0] > '9')
      AsciiChars.v[0] -= 'A' - 10;
   else
      AsciiChars.v[0] -= '0';

   // Concatenate
   return (AsciiChars.v[1]<<4) |  AsciiChars.v[0];
}

/*********************************************************************
 * Function:        BYTE btohexa_high(BYTE b)
 *
 * PreCondition:    None
 *
 * Input:           One byte ranged 0x00-0xFF
 *
 * Output:          An ascii byte (always uppercase) between '0'-'9'
 *               or 'A'-'F' that corresponds to the upper 4 bits of
 *               the input byte.
 *               ex: b = 0xAE, btohexa_high() returns 'A'
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:         None
 ********************************************************************/
BYTE btohexa_high(BYTE b)
{
   b >>= 4;
   return (b>0x9) ? b+'A'-10:b+'0';
}

/*********************************************************************
 * Function:        BYTE btohexa_low(BYTE b)
 *
 * PreCondition:    None
 *
 * Input:           One byte ranged 0x00-0xFF
 *
 * Output:          An ascii byte (always uppercase) between '0'-'9'
 *               or 'A'-'F' that corresponds to the lower 4 bits of
 *               the input byte.
 *               ex: b = 0xAE, btohexa_low() returns 'E'
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:         None
 ********************************************************************/
BYTE btohexa_low(BYTE b)
{
   b &= 0x0F;
   return (b>9) ? b+'A'-10:b+'0';
}

WORD swaps(WORD_VAL v)
{
    WORD_VAL new;

    new.v[0]=v.v[1];
    new.v[1]=v.v[0];

    return(new.Val);
}

DWORD swapl(DWORD_VAL v)
{
    DWORD_VAL new;

    new.v[0]=v.v[3];
    new.v[1]=v.v[2];
    new.v[2]=v.v[1];
    new.v[3]=v.v[0];

    return(new.Val);
}

WORD CalcIPChecksum(BYTE* buffer, WORD count)
{
   WORD i;
   WORD *val;

   union
   {
      DWORD Val;
      struct
      {
         WORD_VAL LSB;
         WORD_VAL MSB;
      } words;
   } tempSum, sum;

   sum.Val = 0;

   i = count >> 1;
   val = (WORD *)buffer;

   while( i-- )
      sum.Val += *val++;

   if ( count & 1 )
      sum.Val += *(BYTE *)val;

   tempSum.Val = sum.Val;
   i = tempSum.words.MSB.Val;

   while( i != 0u )
   {
      sum.words.MSB.Val = 0;
      sum.Val = (DWORD)sum.words.LSB.Val + (DWORD)i;
      tempSum.Val = sum.Val;
      i = tempSum.words.MSB.Val;
   }

   return (~sum.words.LSB.Val);
}

/*********************************************************************
 * Function:        WORD CalcIPBufferChecksum(WORD len)
 *
 * PreCondition:    TCPInit() is already called     AND
 *                  MAC buffer pointer set to starting of buffer
 *
 * Input:           len     - Total number of bytes to calculate
 *                          checksum for.
 *
 * Output:          16-bit checksum as defined by rfc 793.
 *
 * Side Effects:    None
 *
 * Overview:        This function performs checksum calculation in
 *                  MAC buffer itself.
 *
 * Note:            None
 ********************************************************************/
#if !defined(MCHP_MAC)
WORD CalcIPBufferChecksum(WORD len)
{
    BOOL lbMSB;
    WORD_VAL checkSum;
    BYTE Checkbyte;

    lbMSB = TRUE;
    checkSum.Val = 0;

    while( len-- )
    {
        Checkbyte = MACGet();

        if ( !lbMSB )
        {
            if ( (checkSum.v[0] = Checkbyte+checkSum.v[0]) < Checkbyte)
            {
                if ( ++checkSum.v[1] == 0 )
                    checkSum.v[0]++;
            }
        }
        else
        {
            if ( (checkSum.v[1] = Checkbyte+checkSum.v[1]) < Checkbyte)
            {
                if ( ++checkSum.v[0] == 0 )
                    checkSum.v[1]++;
            }
        }

        lbMSB = !lbMSB;
    }

    checkSum.v[1] = ~checkSum.v[1];
    checkSum.v[0] = ~checkSum.v[0];
    return checkSum.Val;
}
#endif

/*
char *strupr (char *s)
{
    char c;
    char *t;

    t = s;
    while( (c = *t) )
    {
        if ( (c >= 'a' && c <= 'z') )
            *t -= ('a' - 'A');
    t++;
    }
    return s;
}

void delay_s(int8 s) {
   while(s) {
      restart_wdt();
      delay_ms(1000);
      s--;
   }
}
*/
