/*********************************************************************
 *
 *                  PIC18 SFR Definitions
 *                 (Modified to work with CCS PCH, by CCS)
 *
 *********************************************************************
 * FileName:        pic18.h
 * Dependencies:    None
 * Processor:       PIC18
 * Complier:        CCS PCH 3.181 or later
 *
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
 *
 * Author               Date     Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     11/14/01 Original        (Rev 1.0)
 * Nilesh Rajbharti     2/9/02   Cleanup
 * Nilesh Rajbharti     5/22/02  Rev 2.0 (See version.log for detail)
 * Darren Rook (CCS)    01/09/04 Initial CCS Public Release
 ********************************************************************/
#ifndef COMPILER_H
#define COMPILER_H

#if !defined(__PCH__)
#error "This version only works with CCS PCH or PCWH"
#endif

// ** I/O PORT REGISTERS

#byte PORTA    =  0xF80
#byte PORTB    =  0xF81
#byte PORTC    =  0xF82
#byte PORTD    =  0xF83
#byte PORTE    =  0xF84
#byte PORTF    =  0xF85

#bit PORTA_RA5 =  0xF80.5
#bit PORTA_RA4 =  0xF80.4
#bit PORTA_RA3 =  0xF80.3
#bit PORTA_RA2 =  0xF80.2
#bit PORTA_RA1 =  0xF80.1
#bit PORTA_RA0 =  0xF80.0

#bit PORTB_RB7 =  0xF81.7
#bit PORTB_RB6 =  0xF81.6
#bit PORTB_RB5 =  0xF81.5
#bit PORTB_RB4 =  0xF81.4
#bit PORTB_RB3 =  0xF81.3
#bit PORTB_RB2 =  0xF81.2
#bit PORTB_RB1 =  0xF81.1
#bit PORTB_RB0 =  0xF81.0

#bit PORTC_RC7 =  0xF82.7
#bit PORTC_RC6 =  0xF82.6
#bit PORTC_RC5 =  0xF82.5
#bit PORTC_RC4 =  0xF82.4
#bit PORTC_RC3 =  0xF82.3
#bit PORTC_RC2 =  0xF82.2
#bit PORTC_RC1 =  0xF82.1
#bit PORTC_RC0 =  0xF82.0

#bit PORTD_RD7 =  0xF83.7
#bit PORTD_RD6 =  0xF83.6
#bit PORTD_RD5 =  0xF83.5
#bit PORTD_RD4 =  0xF83.4
#bit PORTD_RD3 =  0xF83.3
#bit PORTD_RD2 =  0xF83.2
#bit PORTD_RD1 =  0xF83.1
#bit PORTD_RD0 =  0xF83.0

#bit PORTE_RE7 =  0xF84.7
#bit PORTE_RE6 =  0xF84.6
#bit PORTE_RE5 =  0xF84.5
#bit PORTE_RE4 =  0xF84.4
#bit PORTE_RE3 =  0xF84.3
#bit PORTE_RE2 =  0xF84.2
#bit PORTE_RE1 =  0xF84.1
#bit PORTE_RE0 =  0xF84.0

#bit PORTF_RF7 =  0xF85.7
#bit PORTF_RF6 =  0xF85.6
#bit PORTF_RF5 =  0xF85.5
#bit PORTF_RF4 =  0xF85.4
#bit PORTF_RF3 =  0xF85.3
#bit PORTF_RF2 =  0xF85.2
#bit PORTF_RF1 =  0xF85.1
#bit PORTF_RF0 =  0xF85.0


// *** TRIS REGISTERS

#byte TRISA    =  0xF92
#byte TRISB    =  0xF93
#byte TRISC    =  0xF94
#byte TRISD    =  0xF95
#byte TRISE    =  0xF96
#byte TRISF    =  0xF97

#bit TRISA_RA7 =  0xF92.7
#bit TRISA_RA6 =  0xF92.6
#bit TRISA_RA5 =  0xF92.5
#bit TRISA_RA4 =  0xF92.4
#bit TRISA_RA3 =  0xF92.3
#bit TRISA_RA2 =  0xF92.2
#bit TRISA_RA1 =  0xF92.1
#bit TRISA_RA0 =  0xF92.0

#bit TRISB_RB7 =  0xF93.7
#bit TRISB_RB6 =  0xF93.6
#bit TRISB_RB5 =  0xF93.5
#bit TRISB_RB4 =  0xF93.4
#bit TRISB_RB3 =  0xF93.3
#bit TRISB_RB2 =  0xF93.2
#bit TRISB_RB1 =  0xF93.1
#bit TRISB_RB0 =  0xF93.0

#bit TRISC_RC7 =  0xF94.7
#bit TRISC_RC6 =  0xF94.6
#bit TRISC_RC5 =  0xF94.5
#bit TRISC_RC4 =  0xF94.4
#bit TRISC_RC3 =  0xF94.3
#bit TRISC_RC2 =  0xF94.2
#bit TRISC_RC1 =  0xF94.1
#bit TRISC_RC0 =  0xF94.0

#bit TRISD_RD7 =  0xF95.7
#bit TRISD_RD6 =  0xF95.6
#bit TRISD_RD5 =  0xF95.5
#bit TRISD_RD4 =  0xF95.4
#bit TRISD_RD3 =  0xF95.3
#bit TRISD_RD2 =  0xF95.2
#bit TRISD_RD1 =  0xF95.1
#bit TRISD_RD0 =  0xF95.0

#bit TRISE_RE7 =  0xF96.7
#bit TRISE_RE6 =  0xF96.6
#bit TRISE_RE5 =  0xF96.5
#bit TRISE_RE4 =  0xF96.4
#bit TRISE_RE3 =  0xF96.3
#bit TRISE_RE2 =  0xF96.2
#bit TRISE_RE1 =  0xF96.1
#bit TRISE_RE0 =  0xF96.0

#bit TRISF_RF7 =  0xF97.7
#bit TRISF_RF6 =  0xF97.6
#bit TRISF_RF5 =  0xF97.5
#bit TRISF_RF4 =  0xF97.4
#bit TRISF_RF3 =  0xF97.3
#bit TRISF_RF2 =  0xF97.2
#bit TRISF_RF1 =  0xF97.1
#bit TRISF_RF0 =  0xF97.0


// *** LAT REGISTERS
#byte LATA    =  0xF89
#byte LATB    =  0xF8A
#byte LATC    =  0xF8B
#byte LATD    =  0xF8C
#byte LATE    =  0xF8D
#byte LATF    =  0xF8E

#bit LATA_RA7 =  0xF89.7
#bit LATA_RA6 =  0xF89.6
#bit LATA_RA5 =  0xF89.5
#bit LATA_RA4 =  0xF89.4
#bit LATA_RA3 =  0xF89.3
#bit LATA_RA2 =  0xF89.2
#bit LATA_RA1 =  0xF89.1
#bit LATA_RA0 =  0xF89.0

#bit LATB_RB7 =  0xF8A.7
#bit LATB_RB6 =  0xF8A.6
#bit LATB_RB5 =  0xF8A.5
#bit LATB_RB4 =  0xF8A.4
#bit LATB_RB3 =  0xF8A.3
#bit LATB_RB2 =  0xF8A.2
#bit LATB_RB1 =  0xF8A.1
#bit LATB_RB0 =  0xF8A.0

#bit LATC_RC7 =  0xF8B.7
#bit LATC_RC6 =  0xF8B.6
#bit LATC_RC5 =  0xF8B.5
#bit LATC_RC4 =  0xF8B.4
#bit LATC_RC3 =  0xF8B.3
#bit LATC_RC2 =  0xF8B.2
#bit LATC_RC1 =  0xF8B.1
#bit LATC_RC0 =  0xF8B.0

#bit LATD_RD7 =  0xF8C.7
#bit LATD_RD6 =  0xF8C.6
#bit LATD_RD5 =  0xF8C.5
#bit LATD_RD4 =  0xF8C.4
#bit LATD_RD3 =  0xF8C.3
#bit LATD_RD2 =  0xF8C.2
#bit LATD_RD1 =  0xF8C.1
#bit LATD_RD0 =  0xF8C.0

#bit LATE_RE7 =  0xF8D.7
#bit LATE_RE6 =  0xF8D.6
#bit LATE_RE5 =  0xF8D.5
#bit LATE_RE4 =  0xF8D.4
#bit LATE_RE3 =  0xF8D.3
#bit LATE_RE2 =  0xF8D.2
#bit LATE_RE1 =  0xF8D.1
#bit LATE_RE0 =  0xF8D.0

#bit LATF_RF7 =  0xF8E.7
#bit LATF_RF6 =  0xF8E.6
#bit LATF_RF5 =  0xF8E.5
#bit LATF_RF4 =  0xF8E.4
#bit LATF_RF3 =  0xF8E.3
#bit LATF_RF2 =  0xF8E.2
#bit LATF_RF1 =  0xF8E.1
#bit LATF_RF0 =  0xF8E.0


// ** OTHER SPECIAL FILE REGISTERS USED BY SLIP

#bit  PIE1_TXIE       =  0xF9D.4
#bit  PIE1_RCIE       =  0xF9D.5
#bit  PIR1_TXIF       =  0xF9E.4
#bit  PIR1_RCIF       =  0xF9E.5
#byte TXSTA           =  0xFAC
#byte RCSTA           =  0xFAB
#bit  RCSTA_CREN      =  0xFAB.4
#byte RCREG           =  0xFAE
#byte SPBRG           =  0xFAF
#byte TXREG           =  0xFAD


#endif
