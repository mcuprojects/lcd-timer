#include "STC12C5620AD.H"

#include <stdio.h>
#include <stdlib.h>
#include <intrins.h>

#define CPU_FREQ 11059200
#define T1MS (65536-CPU_FREQ/12/1000)

sbit LED_BLINK = P3^1;

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

static uint16 timer_counter = 0;

void delay(unsigned int ms)
{
    unsigned int i;
    while (ms--) {
        for (i = 500; i > 0; i--) {
            _nop_();
        }
    }
}

void scan_keyboard()
{
    
}

void main()
{
    TMOD = 0x01;
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    TR0 = 1;
    ET0 = 1;
    
    EA = 1;
    
    while (1) {
    }
}

void timer0_ISR() interrupt 1
{
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    if (timer_counter++ >= 1000) {
        timer_counter = 0;
        LED_BLINK = !LED_BLINK;
    }
}
