#include "STC12C5620AD.H"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <intrins.h>

#define CPU_FREQ 11059200
#define T1MS (65536 - CPU_FREQ/12/1000)

#define EN 0
#define RU 1
#define LANG RU

#if LANG == RU
    #include "font_ru.h"
#endif

#define LCD_PORT P1
#include "lcd.h"
sbit LCD_RS = P2^0;
sbit LCD_RW = P3^7;
sbit LCD_EN = P2^1;

sbit LED_BLINK = P3^1;

sbit P2_2 = P2^2;
sbit P2_3 = P2^3;
sbit P2_4 = P2^4;
sbit P2_5 = P2^5;
sbit P2_6 = P2^6;
sbit P2_7 = P2^7;

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef enum {
    UNDEFINED,
    RUNNING,
    STOPPED,
    PAUSED,
    TIMER_EDIT,
    DATE_SHOW,
    DATE_EDIT
} state_t;

typedef enum {
    NONE,
    OK,
    START,
    STOP,
    LEFT,
    RIGHT,
    UP,
    DOWN
} key_t;

typedef struct {
    uint8 y1000;
    uint8 y100;
    uint8 y10;
    uint8 y1;
    uint8 m10;
    uint8 m1;
    uint8 d10;
    uint8 d1;
    
    uint8 h10;
    uint8 h1;
    uint8 mm10;
    uint8 mm1;
    uint8 s10;
    uint8 s1;
    
    uint8 wday;
} date_time_t;

typedef struct timer {
    uint8 m;
    uint8 s;
} timer_t;

uint16 timer_scaler = 0;
uint16 delay_timer = 0;
uint8 scan_keyboard_request = 0;

state_t state = UNDEFINED;
key_t key_pressed = NONE;
key_t prev_key = NONE;

date_time_t idata date;
timer_t idata timer;
timer_t idata old_timer;

void delay(uint16 ms)
{
    delay_timer = ms;
    while (delay_timer);
}

void lcd_printf(const char* fmt, ...)
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

void update_icon()
{
    const char code play[8] = {0x00, 0x10, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x10};
    const char code pause[8] = {0x00, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00};
    const char code stop[8] = {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
    
    if (state == RUNNING) {
        lcd_Load_Custom_Symbol(7, play);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Data(7);
    } else if (state == PAUSED) {
        lcd_Load_Custom_Symbol(7, pause);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Data(7);
    } else if (state == STOPPED) {
        lcd_Load_Custom_Symbol(7, stop);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Data(7);
    }
}

void update_date()
{
    lcd_Set_Cursor_Pos(0, 0);
    lcd_Write_Data('0' + date.m10);
    lcd_Write_Data('0' + date.m1);
    lcd_Write_Data('-');
    lcd_Write_Data('0' + date.d10);
    lcd_Write_Data('0' + date.d1);
    lcd_Write_String("   ");
    lcd_Write_Data('0' + date.h10);
    lcd_Write_Data('0' + date.h1);
    lcd_Write_Data(':');
    lcd_Write_Data('0' + date.mm10);
    lcd_Write_Data('0' + date.mm1);
    lcd_Write_Data(':');
    lcd_Write_Data('0' + date.s10);
    lcd_Write_Data('0' + date.s1);
}

void update_timer()
{
#if LANG == RU
    lcd_Set_Cursor_Pos(1, 7);
#else
    lcd_Set_Cursor_Pos(1, 6);
#endif
    lcd_printf("%02bd:%02bd", timer.m, timer.s);
}

void show_main_screen()
{
    lcd_Clear();
#if LANG == RU
    lcd_Load_Custom_Symbol(0, ru_ii);
    lcd_Load_Custom_Symbol(1, ru_m);
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_String("Ta");
    lcd_Write_Data(0);
    lcd_Write_Data(1);
    lcd_Write_String("ep:");
#else
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_String("Timer:");
#endif
    update_date();
    update_timer();
    update_icon();
}

date_time_t default_date()
{
    date_time_t res;
    res.y1000 = 2;
    res.y100 =  0;
    res.y10 =   1;
    res.y1 =    6;
    res.m10 =   0;
    res.m1 =    1;
    res.d10 =   0;
    res.d1 =    1;
    res.h10 =  1;
    res.h1 =   2;
    res.mm10 = 0;
    res.mm1 =  0;
    res.s10 =  0;
    res.s1 =   0;
    res.wday =  0;
    
    return res;
}

timer_t default_timer()
{
    timer_t res;
    res.m = 0;
    res.s = 30;
    
    return res;
}

void scan_keyboard()
{
    prev_key = key_pressed;

    P2_4 = 1;
    P2_3 = 1;
    P2_2 = 1;

    P2_7 = 1;
    P2_6 = 1;
    P2_5 = 1;

    key_pressed = NONE;
    
    P2_4 = 0;
    _nop_();
    if (P2_7 == 0)
        key_pressed = UP;
    P2_4 = 1;

    P2_3 = 0;
    _nop_();
    if (P2_7 == 0)
        key_pressed = RIGHT;
    P2_3 = 1;

    P2_2 = 0;
    _nop_();
    if (P2_7 == 0)
        key_pressed = LEFT;
    P2_2 = 1;

    P2_4 = 0;
    _nop_();
    if (P2_6 == 0)
        key_pressed = START;
    P2_4 = 1;

    P2_3 = 0;
    _nop_();
    if (P2_6 == 0)
        key_pressed = OK;
    P2_3 = 1;

    P2_2 = 0;
    _nop_();
    if (P2_6 == 0)
        key_pressed = DOWN;
    P2_2 = 1;

    P2_2 = 0;
    _nop_();
    if (P2_5 == 0)
        key_pressed = STOP;
    P2_2 = 1;
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
    delay(2000);
    
    state = STOPPED;
    date = default_date();
    timer = default_timer();
    old_timer = default_timer();
    show_main_screen();
    
    while (1) {
        if (scan_keyboard_request) {
            scan_keyboard_request = 0;
            scan_keyboard();
        }
    }
}

void timer0_ISR() interrupt 1
{
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    
    if (timer_scaler++ >= 1000) {
        timer_scaler = 0;
        LED_BLINK = !LED_BLINK;
    }
    
    if (delay_timer)
        delay_timer--;

    scan_keyboard_request = 1;
}
