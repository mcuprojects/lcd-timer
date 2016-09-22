/* After is STC additional SFR or change */

/* sfr  AUXR  = 0x8e; */
/* sfr  IPH   = 0xb7; */

/* Watchdog Timer Register */
sfr at (0xe1) WDT_CONTR;

/* ISP_IAP_EEPROM Register */
sfr at (0xe2) ISP_DATA;
sfr at (0xe3) ISP_ADDRH;
sfr at (0xe4) ISP_ADDRL;
sfr at (0xe5) ISP_CMD;
sfr at (0xe6) ISP_TRIG;
sfr at (0xe7) ISP_CONTR;

/* System Clock Divider */
sfr at (0xc7) CLK_DIV;

/* I_O Port Mode Set Register */
sfr at (0x93) P0M0;
sfr at (0x94) P0M1;
sfr at (0x91) P1M0;
sfr at (0x92) P1M1;
sfr at (0x95) P2M0;
sfr at (0x96) P2M1;
sfr at (0xb1) P3M0;
sfr at (0xb2) P3M1;

/* SPI Register */
sfr at (0x84) SPSTAT;
sfr at (0x85) SPCTL;
sfr at (0x86) SPDAT;

/* ADC Register */
sfr at (0xc5) ADC_CONTR;
sfr at (0xc6) ADC_DATA;
sfr at (0xbe) ADC_LOW2;

/* PCA SFR */
sfr at (0xD9) CMOD;
sfr at (0xDA) CCAPM0;
sfr at (0xDB) CCAPM1;
sfr at (0xDC) CCAPM2;
sfr at (0xDD) CCAPM3;
sfr at (0xDE) CCAPM4;
sfr at (0xDF) CCAPM5;

sfr at (0xE9) CL;
sfr at (0xEA) CCAP0L;
sfr at (0xEB) CCAP1L;
sfr at (0xEC) CCAP2L;
sfr at (0xED) CCAP3L;
sfr at (0xEE) CCAP4L;
sfr at (0xEF) CCAP5L;

sfr at (0xF9) CH;
sfr at (0xFA) CCAP0H;
sfr at (0xFB) CCAP1H;
sfr at (0xFC) CCAP2H;
sfr at (0xFD) CCAP3H;
sfr at (0xFE) CCAP4H;
sfr at (0xFF) CCAP5H;

sfr at (0xF2) PCA_PWM0;
sfr at (0xF3) PCA_PWM1;
sfr at (0xF4) PCA_PWM2;
sfr at (0xF5) PCA_PWM3;
sfr at (0xF6) PCA_PWM4;
sfr at (0xF7) PCA_PWM5;

/*  CCON  */
sfr at (0xD8) CCON;
sbit at (0xDF) CF;
sbit at (0xDE) CR;
sbit at (0xDD) CCF5;
sbit at (0xDC) CCF4;
sbit at (0xDB) CCF3;
sbit at (0xDA) CCF2;
sbit at (0xD9) CCF1;
sbit at (0xD8) CCF0;


/* Above is STC additional SFR or change */

/*--------------------------------------------------------------------------
REG51F.H

Header file for 8xC31/51, 80C51Fx, 80C51Rx+
Copyright (c) 1988-1999 Keil Elektronik GmbH and Keil Software, Inc.
All rights reserved.

Modification according to DataSheet from April 1999
 - SFR's AUXR and AUXR1 added for 80C51Rx+ derivatives
--------------------------------------------------------------------------*/

/*  BYTE Registers  */
sfr at (0xE0) ACC;
sfr at (0xF0) B;
sfr at (0x81) SP;
sfr at (0x82) DPL;
sfr at (0x83) DPH;
sfr at (0x87) PCON;
sfr at (0x89) TMOD;
sfr at (0x8A) TL0;
sfr at (0x8B) TL1;
sfr at (0x8C) TH0;
sfr at (0x8D) TH1;
sfr at (0x99) SBUF;

/*  GPIO pins  */
/*  P0  */
sfr at (0x80) P0;
sbit at (0x80) P0_0;
sbit at (0x81) P0_1;
sbit at (0x82) P0_2;
sbit at (0x83) P0_3;
sbit at (0x84) P0_4;
sbit at (0x85) P0_5;
sbit at (0x86) P0_6;
sbit at (0x87) P0_7;

/*  P1  */
sfr at (0x90) P1;
sbit at (0x90) P1_0;
sbit at (0x91) P1_1;
sbit at (0x92) P1_2;
sbit at (0x93) P1_3;
sbit at (0x94) P1_4;
sbit at (0x95) P1_5;
sbit at (0x96) P1_6;
sbit at (0x97) P1_7;

#define T2EX P1_1
#define T2 P1_0

/*  P2  */
sfr at (0xA0) P2;
sbit at (0xA0) P2_0;
sbit at (0xA1) P2_1;
sbit at (0xA2) P2_2;
sbit at (0xA3) P2_3;
sbit at (0xA4) P2_4;
sbit at (0xA5) P2_5;
sbit at (0xA6) P2_6;
sbit at (0xA7) P2_7;

/*  P3  */
sfr at (0xB0) P3;
sbit at (0xB0) P3_0;
sbit at (0xB1) P3_1;
sbit at (0xB2) P3_2;
sbit at (0xB3) P3_3;
sbit at (0xB4) P3_4;
sbit at (0xB5) P3_5;
sbit at (0xB6) P3_6;
sbit at (0xB7) P3_7;

#define RD P3_7
#define WR P3_6
#define T1 P3_5
#define T0 P3_4
#define INT1 P3_3
#define INT0 P3_2
#define TXD P3_1
#define RXD P3_0

#define CEX3 P2_4
#define CEX2 P2_0
#define CEX1 P3_5
#define CEX0 P3_7
#define ECI P3_4

/*  80C51Fx/Rx Extensions  */
sfr at (0x8E) AUXR;
/* sfr at (0xA2) AUXR1; */
sfr at (0xA9) SADDR;
sfr at (0xB7) IPH;
sfr at (0xB9) SADEN;
sfr at (0xC9) T2MOD;
sfr at (0xCA) RCAP2L;
sfr at (0xCB) RCAP2H;
sfr at (0xCC) TL2;
sfr at (0xCD) TH2;


/*  BIT Registers  */
/*  PSW   */
sfr at (0xD0) PSW;
sbit at (0xD7) CY;
sbit at (0xD6) AC;
sbit at (0xD5) F0;
sbit at (0xD4) RS1;
sbit at (0xD3) RS0;
sbit at (0xD2) OV;
sbit at (0xD0) P;

/*  TCON  */
sfr at (0x88) TCON;
sbit at (0x8F) TF1;
sbit at (0x8E) TR1;
sbit at (0x8D) TF0;
sbit at (0x8C) TR0;
sbit at (0x8B) IE1;
sbit at (0x8A) IT1;
sbit at (0x89) IE0;
sbit at (0x88) IT0;

/*  SCON  */
sfr at (0x98) SCON;
sbit at (0x9F) SM0; // alternatively "FE"
sbit at (0x9F) FE;
sbit at (0x9E) SM1;
sbit at (0x9D) SM2;
sbit at (0x9C) REN;
sbit at (0x9B) TB8;
sbit at (0x9A) RB8;
sbit at (0x99) TI;
sbit at (0x98) RI;

/*  T2CON  */
sfr at (0xC8) T2CON;
sbit at (0xCF) TF2;
sbit at (0xCE) EXF2;
sbit at (0xCD) RCLK;
sbit at (0xCC) TCLK;
sbit at (0xCB) EXEN2;
sbit at (0xCA) TR2;
sbit at (0xC9) C_T2;
sbit at (0xC8) CP_RL2;

/*  IE   */
sfr at (0xA8) IE;
sbit at (0xAF) EA;
sbit at (0xAE) EPCA_LVD;
sbit at (0xAD) EADC_SPI;
sbit at (0xAC) ES;
sbit at (0xAB) ET1;
sbit at (0xAA) EX1;
sbit at (0xA9) ET0;
sbit at (0xA8) EX0;

/*  IP   */
sfr at (0xB8) IP;
sbit at (0xBE) PPCA_LVD;
sbit at (0xBD) PADC_SPI;
sbit at (0xBC) PS;
sbit at (0xBB) PT1;
sbit at (0xBA) PX1;
sbit at (0xB9) PT0;
sbit at (0xB8) PX0;
