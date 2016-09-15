#include "STC12C5620AD.H"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <intrins.h>

#define LCD_PORT P1
#include "lcd.h"
sbit LCD_RS = P2^0;
sbit LCD_RW = P3^7;
sbit LCD_EN = P2^1;

#define CPU_FREQ 11059200
#define T1MS (65536 - CPU_FREQ/12/1000)

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

void lcd_Printf(const char* fmt, ...)
{
    char buffer[17];
    va_list args;
    
    va_start(args, fmt);
    vsprintf (buffer, fmt, args);
    buffer[16] = '\0';
    lcd_Write_String(buffer);
    va_end (args);
}

void display_logo()
{
    const char code m1[8] = {0x1F, 0x18, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11};
    const char code m2[8] = {0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x07, 0x03, 0x03};
    const char code m3[8] = {0x1F, 0x1F, 0x1E, 0x1E, 0x1C, 0x1C, 0x18, 0x18};
    const char code m4[8] = {0x1F, 0x03, 0x01, 0x01, 0x01, 0x01, 0x11, 0x11};
    const char code m5[8] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F};
    const char code m6[8] = {0x11, 0x18, 0x18, 0x1C, 0x1C, 0x1E, 0x1E, 0x1F};
    const char code m7[8] = {0x11, 0x03, 0x03, 0x07, 0x07, 0x0F, 0x0F, 0x1F};
    const char code f2[8] = {0x1F, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x01};
    
    lcd_Clear();
    lcd_Load_Custom_Symbol(0, m1);
    lcd_Load_Custom_Symbol(1, m2);
    lcd_Load_Custom_Symbol(2, m3);
    lcd_Load_Custom_Symbol(3, m4);
    lcd_Load_Custom_Symbol(4, m5);
    lcd_Load_Custom_Symbol(5, m6);
    lcd_Load_Custom_Symbol(6, m7);
    lcd_Load_Custom_Symbol(7, f2);
    
    lcd_Set_Cursor_Pos(0, 0);
    lcd_Write_Data(0);
    lcd_Write_Data(1);
    lcd_Write_Data(2);
    lcd_Write_Data(3);
    lcd_Write_Data(0);
    lcd_Write_Data(7);
    lcd_Write_Data(255);
    lcd_Write_String(" MEDICAL ");
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_Data(4);
    lcd_Write_Data(5);
    lcd_Write_Data(6);
    lcd_Write_Data(4);
    lcd_Write_Data(4);
    lcd_Write_Data(255);
    lcd_Write_Data(255);
    lcd_Write_String(" F O R T ");
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
    
    lcd_Init();
    
    display_logo();
    
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
