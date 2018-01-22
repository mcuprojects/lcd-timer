#include "STC12C5620AD.H"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef uint8 bool;
#define false 0
#define true 1

#define CPU_FREQ 11059200
#define T1MS (65536 - CPU_FREQ/12/1000)

#define LCD_PORT P1
#define LCD_RS P2_0
#define LCD_RW P3_7
#define LCD_EN P2_1
//#define LCD_RS P3_5
//#define LCD_EN P3_4
#include "lcd.h"

#include "rtc.h"

#define LED_CONST P3_0
#define LED_BLINK P3_1
#define BUZZER P3_4
#define RELAY P3_5
//#define BUZZER P2_0
//#define RELAY P2_1

#define EN 0
#define RU 1
#define LANG RU

#if LANG == RU
    #include "font_ru.h"
#endif

#define MUTE

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
    uint8 m;
    uint8 s;
} timer_t;

const uint8 code days_in_month[12] = {31, 29, 31, 20, 31, 30, 31, 31, 30, 31, 30, 31};

#define EEPROM_MAGIC 0xDE

uint16 timer_scaler = 0;
uint16 delay_timer = 0;
uint16 beep_timer = 0;
bool scan_keyboard_request = 0;
bool dec_timer_request = 0;
bool read_date_request = 0;
bool backup_timer_enable = 0;
bool timer_enable = 0;
uint8 cursor_pos = 0;

state_t state = UNDEFINED;
key_t key_pressed = NONE;
key_t prev_key = NONE;

date_time_t idata date;
timer_t idata timer;
timer_t idata old_timer;
uint16 idata timer_offset;

date_time_t idata new_date;
timer_t idata new_timer;

void delay_ms(uint16 ms)
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
    lcd_Write_Char(0);
    lcd_Write_Char(1);
    lcd_Write_Char(2);
    lcd_Write_Char(3);
    lcd_Write_Char(0);
    lcd_Write_Char(7);
    lcd_Write_Char(255);
    lcd_Write_String(" MEDICAL ");
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_Char(4);
    lcd_Write_Char(5);
    lcd_Write_Char(6);
    lcd_Write_Char(4);
    lcd_Write_Char(4);
    lcd_Write_Char(255);
    lcd_Write_Char(255);
    lcd_Write_String(" F O R T ");
}

void display_error()
{
    lcd_Clear();
    lcd_Cursor_Off();

#if LANG == RU
    lcd_Load_Custom_Symbol(0, ru_sh);
    lcd_Load_Custom_Symbol(1, ru_i);
    lcd_Load_Custom_Symbol(2, ru_b);
    lcd_Load_Custom_Symbol(3, ru_k);
    lcd_Set_Cursor_Pos(0, 5);
    lcd_Write_Char('O');
    lcd_Write_Char(0);
    lcd_Write_Char(1);
    lcd_Write_Char(2);
    lcd_Write_Char(3);
    lcd_Write_Char('a');
    lcd_Write_Char('!');
#else
    lcd_Set_Cursor_Pos(0, 0);
    if (state == TIMER_EDIT)
        lcd_Write_String("Timer set error!");
    if (state == DATE_EDIT)
        lcd_Write_String("Clock set error!");
#endif
}

bool valid_date(date_time_t *d)
{
    uint8 year = d->y10 * 10 + d->y1;
    uint8 month = d->m10 * 10 + d->m1;
    uint8 day = d->d10 * 10 + d->d1;
    uint8 hour = d->h10 * 10 + d->h1;
    uint8 minute = d->mm10 * 10 + d->mm1;
    uint8 second = d->s10 * 10 + d->s1;
    uint8 wday = d->wday;

    if (year > 99)
        return false;
    if ((month == 0) || (month > 12))
        return false;
    if ((day == 0) || (day > days_in_month[month - 1]))
        return false;
    if (((year % 4) != 0) && (month == 2) && (day > 28))
        return false;
    if (hour > 23)
        return false;
    if (minute > 59)
        return false;
    if (second > 59)
        return false;
    if (wday > 6)
        return false;

    return true;
}

bool valid_timer(timer_t *t)
{
    if (t->m < 30)
        return (t->s < 60);
    else if (t->m == 30)
        return (t->s == 0);
    else
        return false;
}

void default_date(date_time_t *d)
{
//    d->y1000 = 2;
//    d->y100 =  0;
    d->y10 =   1;
    d->y1 =    6;
    d->m10 =   0;
    d->m1 =    1;
    d->d10 =   0;
    d->d1 =    1;
    d->h10 =  1;
    d->h1 =   2;
    d->mm10 = 0;
    d->mm1 =  0;
    d->s10 =  0;
    d->s1 =   0;
    d->wday =  0;
}

void default_timer(timer_t *t)
{
    t->m = 0;
    t->s = 30;
}

void update_icon()
{
    const char code play[8] = {0x00, 0x10, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x10};
    const char code pause[8] = {0x00, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00};
    const char code stop[8] = {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
    
    if (state == RUNNING) {
        lcd_Load_Custom_Symbol(7, play);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Char(7);
    } else if (state == PAUSED) {
        lcd_Load_Custom_Symbol(7, pause);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Char(7);
    } else if (state == STOPPED) {
        lcd_Load_Custom_Symbol(7, stop);
        lcd_Set_Cursor_Pos(1, 14);
        lcd_Write_Char(7);
    }
}

void update_date()
{
    if ((state == STOPPED) || (state == RUNNING) || (state == PAUSED)) {
        lcd_Set_Cursor_Pos(0, 0);
        lcd_Write_Char('0' + date.m10);
        lcd_Write_Char('0' + date.m1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + date.d10);
        lcd_Write_Char('0' + date.d1);
    } else if (state == DATE_SHOW) {
        lcd_Set_Cursor_Pos(0, 0);
//        lcd_Write_Char('0' + date.y1000);
//        lcd_Write_Char('0' + date.y100);
        lcd_Write_Char('2');
        lcd_Write_Char('0');
        lcd_Write_Char('0' + date.y10);
        lcd_Write_Char('0' + date.y1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + date.m10);
        lcd_Write_Char('0' + date.m1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + date.d10);
        lcd_Write_Char('0' + date.d1);
    } else if (state == DATE_EDIT) {
        lcd_Set_Cursor_Pos(0, 0);
//        lcd_Write_Char('0' + new_date.y1000);
//        lcd_Write_Char('0' + new_date.y100);
        lcd_Write_Char('2');
        lcd_Write_Char('0');
        lcd_Write_Char('0' + new_date.y10);
        lcd_Write_Char('0' + new_date.y1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + new_date.m10);
        lcd_Write_Char('0' + new_date.m1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + new_date.d10);
        lcd_Write_Char('0' + new_date.d1);
    } else
        return;
}

void update_week_day()
{
    uint8 wday;

    if (state == DATE_SHOW)
        wday = date.wday;
    else if (state == DATE_EDIT)
        wday = new_date.wday;
    else
        return;

#if LANG == RU
    lcd_Load_Custom_Symbol(0, ru_bcap);
    lcd_Load_Custom_Symbol(1, ru_pcap);
    lcd_Load_Custom_Symbol(2, ru_chcap);
    lcd_Set_Cursor_Pos(0, 13);
    if (wday == 0) {
        lcd_Write_Char(1);
        lcd_Write_Char('H');
    } else if (wday == 1) {
        lcd_Write_Char('B');
        lcd_Write_Char('T');
    } else if (wday == 2) {
        lcd_Write_Char('C');
        lcd_Write_Char('P');
    } else if (wday == 3) {
        lcd_Write_Char(2);
        lcd_Write_Char('T');
    } else if (wday == 4) {
        lcd_Write_Char(1);
        lcd_Write_Char('T');
    } else if (wday == 5) {
        lcd_Write_Char('C');
        lcd_Write_Char(0);
    } else if (wday == 6) {
        lcd_Write_Char('B');
        lcd_Write_Char('C');
    }
#else
    lcd_Set_Cursor_Pos(0, 13);
    if (wday == 0)
        lcd_Write_String("MON");
    else if (wday == 1)
        lcd_Write_String("TUE");
    else if (wday == 2)
        lcd_Write_String("WED");
    else if (wday == 3)
        lcd_Write_String("THU");
    else if (wday == 4)
        lcd_Write_String("FRI");
    else if (wday == 5)
        lcd_Write_String("SAT");
    else if (wday == 6)
        lcd_Write_String("SUN");
#endif
}

void update_time()
{
    if ((state == STOPPED) || (state == RUNNING) || (state == PAUSED))
        lcd_Set_Cursor_Pos(0, 8);
    else if ((state == DATE_SHOW) || (state == DATE_EDIT))
        lcd_Set_Cursor_Pos(1, 4);
    else
        return;
    
    if (state == DATE_EDIT) {
        lcd_Write_Char('0' + new_date.h10);
        lcd_Write_Char('0' + new_date.h1);
        lcd_Write_Char(':');
        lcd_Write_Char('0' + new_date.mm10);
        lcd_Write_Char('0' + new_date.mm1);
        lcd_Write_Char(':');
        lcd_Write_Char('0' + new_date.s10);
        lcd_Write_Char('0' + new_date.s1);
    } else {
        lcd_Write_Char('0' + date.h10);
        lcd_Write_Char('0' + date.h1);
        lcd_Write_Char(':');
        lcd_Write_Char('0' + date.mm10);
        lcd_Write_Char('0' + date.mm1);
        lcd_Write_Char(':');
        lcd_Write_Char('0' + date.s10);
        lcd_Write_Char('0' + date.s1);
    }
}

void update_timer()
{
    if ((state == STOPPED) || (state == RUNNING) || (state == PAUSED))
#if LANG == RU
        lcd_Set_Cursor_Pos(1, 7);
#else
        lcd_Set_Cursor_Pos(1, 6);
#endif
    else if (state == TIMER_EDIT)
        lcd_Set_Cursor_Pos(1, 5);
    else
        return;
    
    if (state == TIMER_EDIT) {
        lcd_Write_Char('0' + (new_timer.m / 10));
        lcd_Write_Char('0' + (new_timer.m % 10));
        lcd_Write_Char(':');
        lcd_Write_Char('0' + (new_timer.s / 10));
        lcd_Write_Char('0' + (new_timer.s % 10));
    } else {
        lcd_Write_Char('0' + (timer.m / 10));
        lcd_Write_Char('0' + (timer.m % 10));
        lcd_Write_Char(':');
        lcd_Write_Char('0' + (timer.s / 10));
        lcd_Write_Char('0' + (timer.s % 10));
    }
}

void update_cursor()
{
    if (state == DATE_EDIT) {
        if (cursor_pos == 0)
            lcd_Set_Cursor_Pos(0, 2);
        else if (cursor_pos == 1)
            lcd_Set_Cursor_Pos(0, 3);
        else if (cursor_pos == 2)
            lcd_Set_Cursor_Pos(0, 5);
        else if (cursor_pos == 3)
            lcd_Set_Cursor_Pos(0, 6);
        else if (cursor_pos == 4)
            lcd_Set_Cursor_Pos(0, 8);
        else if (cursor_pos == 5)
            lcd_Set_Cursor_Pos(0, 9);
        else if (cursor_pos == 6)
            lcd_Set_Cursor_Pos(0, 13);
        else if (cursor_pos == 7)
            lcd_Set_Cursor_Pos(1, 4);
        else if (cursor_pos == 8)
            lcd_Set_Cursor_Pos(1, 5);
        else if (cursor_pos == 9)
            lcd_Set_Cursor_Pos(1, 7);
        else if (cursor_pos == 10)
            lcd_Set_Cursor_Pos(1, 8);
        else if (cursor_pos == 11)
            lcd_Set_Cursor_Pos(1, 10);
        else if (cursor_pos == 12)
            lcd_Set_Cursor_Pos(1, 11);
        lcd_Cursor_On();
    } else if (state == TIMER_EDIT) {
        if (cursor_pos == 0)
           lcd_Set_Cursor_Pos(1, 5);
        else if (cursor_pos == 1)
            lcd_Set_Cursor_Pos(1, 6);
        else if (cursor_pos == 2)
            lcd_Set_Cursor_Pos(1, 8);
        else if (cursor_pos == 3)
            lcd_Set_Cursor_Pos(1, 9);
        lcd_Cursor_On();
    } else {
        lcd_Cursor_Off();
    }
}

void show_main_screen()
{
    lcd_Clear();
    lcd_Cursor_Off();

#if LANG == RU
    lcd_Load_Custom_Symbol(0, ru_ii);
    lcd_Load_Custom_Symbol(1, ru_m);
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_String("Ta");
    lcd_Write_Char(0);
    lcd_Write_Char(1);
    lcd_Write_String("ep:");
#else
    lcd_Set_Cursor_Pos(1, 0);
    lcd_Write_String("Timer:");
#endif
    update_date();
    update_time();
    update_timer();
    update_icon();
}

void show_date_screen()
{
    lcd_Clear();
    update_date();
    update_week_day();
    update_time();
}

void show_date_edit_screen()
{
    lcd_Clear();
    memcpy(&new_date, &date, sizeof(date_time_t));
    update_date();
    update_week_day();
    update_time();

    cursor_pos = 0;
    update_cursor();
}

void show_timer_edit_screen()
{
    lcd_Clear();

#if LANG == RU
    lcd_Load_Custom_Symbol(0, ru_ucap);
    lcd_Load_Custom_Symbol(1, ru_t);
    lcd_Load_Custom_Symbol(2, ru_ii);
    lcd_Load_Custom_Symbol(3, ru_m);
    lcd_Set_Cursor_Pos(0, 2);
    lcd_Write_Char(0);
    lcd_Write_Char('c');
    lcd_Write_Char(1);
    lcd_Write_Char('.');
    lcd_Write_Char(' ');
    lcd_Write_Char(1);
    lcd_Write_Char('a');
    lcd_Write_Char(2);
    lcd_Write_Char(3);
    lcd_Write_String("ep:");
#else
    lcd_Set_Cursor_Pos(0, 3);
    lcd_Write_String("Set timer:");
#endif

    memcpy(&new_timer, &timer, sizeof(timer_t));
    update_timer();
    
    cursor_pos = 0;
    update_cursor();
}

void date_cursor_move(key_t btn)
{
    if (btn == UP)
        cursor_pos = (cursor_pos + 12) % 13;    // -1
    if (btn == DOWN)
        cursor_pos = (cursor_pos + 1) % 13;     // +1

    update_cursor();
}

void timer_cursor_move(key_t btn)
{
    if (btn == UP)
        cursor_pos = (cursor_pos + 3) % 4;      // -1
    if (btn == DOWN)
        cursor_pos = (cursor_pos + 1) % 4;      // +1

    update_cursor();
}

void date_cursor_edit(key_t btn)
{
    if (btn == RIGHT) {                         // +1
        if (cursor_pos == 0) {          // yyYy-mm-dd hh:mm:ss w
            new_date.y10 = (new_date.y10 + 1) % 10;
        } else if (cursor_pos == 1) {   // yyyY-mm-dd hh:mm:ss w
            new_date.y1 = (new_date.y1 + 1) % 10;
        } else if (cursor_pos == 2) {   // yyyy-Mm-dd hh:mm:ss w
            new_date.m10 = (new_date.m10 + 1) % 2;
        } else if (cursor_pos == 3) {   // yyyy-mM-dd hh:mm:ss w
            if (new_date.m10 < 1)
                new_date.m1 = (new_date.m1 + 1) % 10;
            else
                new_date.m1 = (new_date.m1 + 1) % 3;
        } else if (cursor_pos == 4) {   // yyyy-mm-Dd hh:mm:ss w
            if (new_date.d10 < (days_in_month[new_date.m10 * 10 + new_date.m1 - 1] / 10))
                new_date.d10 = (new_date.d10 + 1) % 4;
            else
                new_date.d10 = 0;
        } else if (cursor_pos == 5) {   // yyyy-mm-dD hh:mm:ss w
            if (new_date.d1 < days_in_month[new_date.m10 * 10 + new_date.m1 - 1])
                new_date.d1 = (new_date.d1 + 1) % 10;
            else
                new_date.d1 = 0;
        } else if (cursor_pos == 6) {   // yyyy-mm-dd hh:mm:ss W
            new_date.wday = (new_date.wday + 1) % 7;
        } else if (cursor_pos == 7) {   // yyyy-mm-dd Hh:mm:ss w
            new_date.h10 = (new_date.h10 + 1) % 3;
        } else if (cursor_pos == 8) {   // yyyy-mm-dd hH:mm:ss w
            if (new_date.h10 < 2)
                new_date.h1 = (new_date.h1 + 1) % 10;
            else
                new_date.h1 = (new_date.h1 + 1) % 4;
        } else if (cursor_pos == 9) {   // yyyy-mm-dd hh:Mm:ss w
            new_date.mm10 = (new_date.mm10 + 1) % 6;
        } else if (cursor_pos == 10) {  // yyyy-mm-dd hh:mM:ss w
            new_date.mm1 = (new_date.mm1 + 1) % 10;
        } else if (cursor_pos == 11) {  // yyyy-mm-dd hh:mm:Ss w
            new_date.s10 = (new_date.s10 + 1) % 6;
        } else if (cursor_pos == 12) {  // yyyy-mm-dd hh:mm:sS w
            new_date.s1 = (new_date.s1 + 1) % 10;
        }
    } else if (btn == LEFT) {                   // -1
        if (cursor_pos == 0) {          // yyYy-mm-dd hh:mm:ss w
            new_date.y10 = (new_date.y10 + 9) % 10;
        } else if (cursor_pos == 1) {   // yyyY-mm-dd hh:mm:ss w
            new_date.y1 = (new_date.y1 + 9) % 10;
        } else if (cursor_pos == 2) {   // yyyy-Mm-dd hh:mm:ss w
            new_date.m10 = (new_date.m10 + 3) % 2;
        } else if (cursor_pos == 3) {   // yyyy-mM-dd hh:mm:ss w
            if (new_date.m10 < 1)
                new_date.m1 = (new_date.m1 + 9) % 10;
            else
                new_date.m1 = (new_date.m1 + 2) % 3;
        } else if (cursor_pos == 4) {   // yyyy-mm-Dd hh:mm:ss w
            if (new_date.d10 > 0)
                new_date.d10--;
            else
                new_date.d10 = days_in_month[new_date.m10 * 10 + new_date.m1 - 1] / 10;
        } else if (cursor_pos == 5) {   // yyyy-mm-dD hh:mm:ss w
            if (new_date.d1 > 0)
                new_date.d1--;
            else
                new_date.d1 = days_in_month[new_date.m10 * 10 + new_date.m1 - 1] % 10;
        } else if (cursor_pos == 6) {   // yyyy-mm-dd hh:mm:ss W
            new_date.wday = (new_date.wday + 6) % 7;
        } else if (cursor_pos == 7) {   // yyyy-mm-dd Hh:mm:ss w
            new_date.h10 = (new_date.h10 + 2) % 3;
        } else if (cursor_pos == 8) {   // yyyy-mm-dd hH:mm:ss w
            if (new_date.h10 < 2)
                new_date.h1 = (new_date.h1 + 9) % 10;
            else
                new_date.h1 = (new_date.h1 + 3) % 4;
        } else if (cursor_pos == 9) {   // yyyy-mm-dd hh:Mm:ss w
            new_date.mm10 = (new_date.mm10 + 5) % 6;
        } else if (cursor_pos == 10) {  // yyyy-mm-dd hh:mM:ss w
            new_date.mm1 = (new_date.mm1 + 9) % 10;
        } else if (cursor_pos == 11) {  // yyyy-mm-dd hh:mm:Ss w
            new_date.s10 = (new_date.s10 + 5) % 6;
        } else if (cursor_pos == 12) {  // yyyy-mm-dd hh:mm:sS w
            new_date.s1 = (new_date.s1 + 9) % 10;
        }
    }

    lcd_Cursor_Off();
    if (cursor_pos < 6)
        update_date();
    else if (cursor_pos == 6)
        update_week_day();
    else
        update_time();
    update_cursor();
}

void timer_cursor_edit(key_t btn)
{
    uint8 m10 = new_timer.m / 10;
    uint8 m1 = new_timer.m % 10;
    uint8 s10 = new_timer.s / 10;
    uint8 s1 = new_timer.s % 10;

    if (btn == RIGHT) {                         // +1
        if (cursor_pos == 0) {          // Mm:ss
            m10 = (m10 + 1) % 4;
        } else if (cursor_pos == 1) {   // mM:ss
            if (m10 < 3)
                m1 = (m1 + 1) % 10;
            else
                m1 = 0;
        } else if (cursor_pos == 2) {   // mm:Ss
            s10 = (s10 + 1) % 6;
        } else if (cursor_pos == 3) {   // mm:sS
            s1 = (s1 + 1) % 10;
        }
    } else if (btn == LEFT) {                   // -1
        if (cursor_pos == 0) {          // Mm:ss
            m10 = (m10 + 3) % 4;
        } else if (cursor_pos == 1) {   // mM:ss
            if (m10 < 3)
                m1 = (m1 + 9) % 10;
            else
                m1 = 0;
        } else if (cursor_pos == 2) {   // mm:Ss
            s10 = (s10 + 5) % 6;
        } else if (cursor_pos == 3) {   // mm:sS
            s1 = (s1 + 9) % 10;
        }
    }

    new_timer.m = m10 * 10 + m1;
    new_timer.s = s10 * 10 + s1;

    lcd_Cursor_Off();
    update_timer();
    update_cursor();
}

void save_date()
{
    if (valid_date(&new_date)) {
        memcpy(&date, &new_date, sizeof(date_time_t));
        rtc_Write_Burst(&new_date);
    } else {
        display_error();
        delay_ms(1000);
    }

    state = STOPPED;
    show_main_screen();
}

void save_timer()
{
    ISP_CONTR = 0;
    ISP_CMD = 0;
    ISP_TRIG = 0;
    ISP_ADDRH = 0;
    ISP_ADDRL = 0;

    if (valid_timer(&new_timer)) {
        memcpy(&timer, &new_timer, sizeof(timer_t));

        rtc_Write_Direct(0xC0, EEPROM_MAGIC);
        delay_ms(1);
        rtc_Write_Direct(0xC2, timer.m);
        delay_ms(1);
        rtc_Write_Direct(0xC4, timer.s);
    } else {
        display_error();
        delay_ms(1000);
    }

    state = STOPPED;
    show_main_screen();
}

void load_timer(timer_t *t)
{
    uint8 magic;

    rtc_Write_Direct(DS_CTRL, 0x00);
    magic = rtc_Read_Direct(0xC0);
    if (magic == EEPROM_MAGIC) {
        rtc_Write_Direct(DS_CTRL, 0x00);
        t->m = rtc_Read_Direct(0xC2);
        rtc_Write_Direct(DS_CTRL, 0x00);
        t->s = rtc_Read_Direct(0xC4);
    } else {
        default_timer(t);
    }
}

void beep_ms(uint16 ms)
{
#ifndef MUTE
    BUZZER = 1;
    beep_timer = ms;
#endif
    (void) ms;
}

void beep_short()
{
    beep_ms(25);
}

void beep_long()
{
    beep_ms(250);
}

void start_timer()
{
    //backup_timer_enable = 1;
    timer_enable = 1;

    timer_offset = timer_scaler;

    RELAY = 0;
}

void stop_timer()
{
    backup_timer_enable = 0;
    timer_enable = 0;

    RELAY = 1;
}

void dec_timer()
{
    if (timer.s)
        timer.s--;
    else {
        timer.s = 59;
        timer.m--;
    }
    
    if ((timer.m == 0) && (timer.s == 0)) {
        state = STOPPED;
        update_icon();
        stop_timer();
    }
}

void scan_keyboard()
{
    prev_key = key_pressed;

    P2_7 = 1;
    P2_6 = 1;
    P2_5 = 1;
    P2_4 = 1;
    P2_3 = 1;
    P2_2 = 1;

    key_pressed = NONE;
    
    P2_4 = 0;
    if (P2_7 == 0)
        key_pressed = UP;
    P2_4 = 1;

    P2_3 = 0;
    if (P2_7 == 0)
        key_pressed = RIGHT;
    P2_3 = 1;

    P2_2 = 0;
    if (P2_7 == 0)
        key_pressed = LEFT;
    P2_2 = 1;

    P2_4 = 0;
    if (P2_6 == 0)
        key_pressed = START;
    P2_4 = 1;

    P2_3 = 0;
    if (P2_6 == 0)
        key_pressed = OK;
    P2_3 = 1;

    P2_2 = 0;
    if (P2_6 == 0)
        key_pressed = DOWN;
    P2_2 = 1;

    P2_2 = 0;
    if (P2_5 == 0)
        key_pressed = STOP;
    P2_2 = 1;
}

void on_button_pressed(key_t btn)
{
    switch (state) {
    case STOPPED:
        if (btn == LEFT) {
            state = TIMER_EDIT;
            show_timer_edit_screen();
        } else if (btn == RIGHT) {
            state = DATE_SHOW;
            show_date_screen();
        } else if (btn == START) {
            if ((timer.m == 0) && (timer.s == 0)) {
                timer.m = old_timer.m;
                timer.s = old_timer.s;
                update_timer();
            } else {
                state = RUNNING;
                update_icon();
                start_timer();
            }
        } else if (btn == STOP) {
            timer.m = 0;
            timer.s = 0;
            update_timer();

            rtc_Init();
        }
        break;
    case RUNNING:
        if (btn == STOP) {
            state = PAUSED;
            update_icon();
            stop_timer();
        }
        break;
    case PAUSED:
        if (btn == STOP) {
            state = STOPPED;
            update_icon();
            stop_timer();
        } else if (btn == START) {
            state = RUNNING;
            update_icon();
            start_timer();
        }
        break;
    case TIMER_EDIT:
        if (btn == STOP) {
            state = STOPPED;
            show_main_screen();
        } else if ((btn == UP) || (btn == DOWN)) {
            timer_cursor_move(btn);
        } else if ((btn == RIGHT) || (btn == LEFT)) {
            timer_cursor_edit(btn);
        } else if (btn == OK) {
            save_timer();
        }
        break;
    case DATE_SHOW:
        if (btn == OK) {
            state = DATE_EDIT;
            show_date_edit_screen();
        } else {
            state = STOPPED;
            show_main_screen();
        }
        break;
    case DATE_EDIT:
        if (btn == STOP) {
            state = STOPPED;
            show_main_screen();
        } else if ((btn == UP) || (btn == DOWN)) {
            date_cursor_move(btn);
        } else if ((btn == RIGHT) || (btn == LEFT)) {
            date_cursor_edit(btn);
        } else if (btn == OK) {
            save_date();
        }
        break;
    default:
        break;
    }
}

void main()
{
    static date_time_t tmp_date;
    uint8 a, b, c;

    TMOD = 0x01;
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    TR0 = 1;
    ET0 = 1;
    EA = 1;
    
    lcd_Init();
    rtc_Init();

    //delay_ms(15);
    //rtc_Write_Direct(0xC0, 0xee);
//    delay_ms(30);
//    a = 3;
//    a = rtc_Read_Direct(0xC0);
    
    display_logo();
    delay_ms(2000);

    LED_CONST = 0;

    P3M1 |= (1 << 4);
    BUZZER = 0;
    

    default_timer(&timer);
    load_timer(&timer);
    default_timer(&old_timer);

    rtc_Read_Burst(&date);
    if (!valid_date(&date))
        default_date(&date);
    
    state = STOPPED;
    show_main_screen();

    //b = rtc_Read_Direct(0xC2);
    //c = rtc_Read_Direct(0xC4);
//    b = 1;
//    c = 2;
    //lcd_Clear();
//    lcd_Set_Cursor_Pos(1, 0);
//    lcd_printf("0x%02x 0x%02x 0x%02x", a, b, c);
    
    while (1) {
        if (scan_keyboard_request) {
            scan_keyboard_request = 0;
            scan_keyboard();
            
            if ((key_pressed != NONE) && (prev_key == NONE))
                beep_short();

            if ((key_pressed == NONE) && (prev_key != NONE))
                on_button_pressed(prev_key);
        }
        
        if (dec_timer_request) {
            dec_timer_request = 0;
            dec_timer();
            update_timer();
        }

        if ((state == STOPPED) || \
            (state == RUNNING) || \
            (state == PAUSED) || \
            (state == DATE_SHOW)) {
            if (read_date_request) {
                read_date_request = 0;
                rtc_Write_Direct(DS_CTRL, 0x00);
                rtc_Read_Burst(&tmp_date);
                if (valid_date(&tmp_date))
                    memcpy(&date, &tmp_date, sizeof(date_time_t));

                update_date();
                update_time();
            }
        }
    }
}

void timer0_ISR() interrupt 1
{
    TL0 = T1MS;
    TH0 = T1MS >> 8;

    if (timer_enable) {
        if (timer_scaler == timer_offset)
            dec_timer_request = 1;
    }
    
    if (timer_scaler++ >= 1000) {
        timer_scaler = 0;
        LED_BLINK = !LED_BLINK;
        
        if (backup_timer_enable)
            dec_timer_request = 1;
    }

    if ((timer_scaler & 0x00FF) == 0)
        read_date_request = 1;
    
    if (delay_timer)
        delay_timer--;
    
    if (beep_timer)
        beep_timer--;
    else
        BUZZER = 0;

    scan_keyboard_request = 1;
}
