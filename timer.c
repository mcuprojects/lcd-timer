#include "STC12C5620AD.H"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define CPU_FREQ 11059200
#define T1MS (65536 - CPU_FREQ/12/1000)

#define EN 0
#define RU 1
#define LANG RU

#if LANG == RU
    #include "font_ru.h"
#endif

#define LCD_PORT P1
#define LCD_RS P2_0
#define LCD_RW P3_7
#define LCD_EN P2_1
#include "lcd.h"

#define LED_BLINKER P3_1
#define BUZZER P3_4

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef uint8 bool;

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

typedef struct {
    uint8 m;
    uint8 s;
} timer_t;

uint16 timer_scaler = 0;
uint16 delay_timer = 0;
uint16 beep_timer = 0;
bool scan_keyboard_request = 0;
bool dec_timer_request = 0;
bool backup_timer_enable = 0;
uint8 cursor_pos = 0;

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
        lcd_Write_String(" Date set error!");
#endif
}

void default_date(date_time_t *d)
{
    d->y1000 = 2;
    d->y100 =  0;
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
    } else if ((state == DATE_SHOW) || (state == DATE_EDIT)) {
        lcd_Set_Cursor_Pos(0, 0);
        lcd_Write_Char('0' + date.y1000);
        lcd_Write_Char('0' + date.y100);
        lcd_Write_Char('0' + date.y10);
        lcd_Write_Char('0' + date.y1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + date.m10);
        lcd_Write_Char('0' + date.m1);
        lcd_Write_Char('-');
        lcd_Write_Char('0' + date.d10);
        lcd_Write_Char('0' + date.d1);
    } else
        return;
}

void update_week_day()
{
    if ((state == DATE_SHOW) || (state == DATE_EDIT)) {
#if LANG == RU
        lcd_Load_Custom_Symbol(0, ru_bcap);
        lcd_Load_Custom_Symbol(1, ru_pcap);
        lcd_Load_Custom_Symbol(2, ru_chcap);
        lcd_Set_Cursor_Pos(0, 13);
        if (date.wday == 0) {
            lcd_Write_Char(1);
            lcd_Write_Char('H');
        } else if (date.wday == 1) {
            lcd_Write_Char('B');
            lcd_Write_Char('T');
        } else if (date.wday == 2) {
            lcd_Write_Char('C');
            lcd_Write_Char('P');
        } else if (date.wday == 3) {
            lcd_Write_Char(2);
            lcd_Write_Char('T');
        } else if (date.wday == 4) {
            lcd_Write_Char(1);
            lcd_Write_Char('T');
        } else if (date.wday == 5) {
            lcd_Write_Char('C');
            lcd_Write_Char(0);
        } else if (date.wday == 6) {
            lcd_Write_Char('B');
            lcd_Write_Char('C');
        }
#else
        lcd_Set_Cursor_Pos(0, 13);
        if (date.wday == 0)
            lcd_Write_String("MON");
        else if (date.wday == 1)
            lcd_Write_String("TUE");
        else if (date.wday == 2)
            lcd_Write_String("WED");
        else if (date.wday == 3)
            lcd_Write_String("THU");
        else if (date.wday == 4)
            lcd_Write_String("FRI");
        else if (date.wday == 5)
            lcd_Write_String("SAT");
        else if (date.wday == 6)
            lcd_Write_String("SUN");
#endif
    }
}

void update_time()
{
    if ((state == STOPPED) || (state == RUNNING) || (state == PAUSED))
        lcd_Set_Cursor_Pos(0, 8);
    else if ((state == DATE_SHOW) || (state == DATE_EDIT))
        lcd_Set_Cursor_Pos(1, 4);
    else
        return;
    
    lcd_Write_Char('0' + date.h10);
    lcd_Write_Char('0' + date.h1);
    lcd_Write_Char(':');
    lcd_Write_Char('0' + date.mm10);
    lcd_Write_Char('0' + date.mm1);
    lcd_Write_Char(':');
    lcd_Write_Char('0' + date.s10);
    lcd_Write_Char('0' + date.s1);
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
    
    lcd_printf("%02bd:%02bd", timer.m, timer.s);
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
        else if (cursor_pos == 3);
            lcd_Set_Cursor_Pos(1, 9);
        lcd_Cursor_On();
    } else
        lcd_Cursor_Off();
}

void show_main_screen()
{
    lcd_Clear();

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
    update_date();
    update_time();
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

    update_timer();
    
    cursor_pos = 0;
    update_cursor();
}

void timer_cursor_move(key_t btn)
{
    if (state == DATE_EDIT) {
        if (btn == UP)              // +1
            cursor_pos = (cursor_pos + 14) % 13;
        if (btn == DOWN)            // -1
            cursor_pos = (cursor_pos + 1) % 13;
    } else if (state == TIMER_EDIT) {
        if (btn == UP)              // +1
            cursor_pos = (cursor_pos + 5) % 4;
        if (btn == DOWN)            // -1
            cursor_pos = (cursor_pos + 1) % 4;
    }
}

void timer_cursor_edit(key_t btn)
{
    (void) btn;
}

void beep_ms(uint16 ms)
{
    BUZZER = 1;
    beep_timer = ms;
}

void beep_short()
{
    beep_ms(25);
}

void beep_long()
{
    beep_ms(500);
}

void start_timer()
{
    backup_timer_enable = 1;
}

void stop_timer()
{
    backup_timer_enable = 0;
}

void dec_timer()
{
    if (timer.s)
        timer.s--;
    else {
        timer.s = 0;
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
            state = RUNNING;
            update_icon();
            start_timer();
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
        } else if (btn == RIGHT) {
            timer_cursor_move(btn);
            update_cursor();
        } else if (btn == LEFT) {
            timer_cursor_move(btn);
            update_cursor();
        }
        break;
    case DATE_SHOW:
        if (btn == STOP) {
            state = STOPPED;
            show_main_screen();
        } else if (btn == OK) {
            state = DATE_EDIT;
            show_date_edit_screen();
        }
        break;
    case DATE_EDIT:
        if (btn == STOP) {
            state = STOPPED;
            show_main_screen();
        }
        break;
    default:
        break;
    }
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
    
    default_date(&date);
    default_timer(&timer);
    default_timer(&old_timer);
    
    state = STOPPED;
    show_main_screen();
    
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
    }
}

void timer0_ISR() interrupt 1
{
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    
    if (timer_scaler++ >= 1000) {
        timer_scaler = 0;
        LED_BLINKER = !LED_BLINKER;
        
        if (backup_timer_enable)
            dec_timer_request = 1;
    }
    
    if (delay_timer)
        delay_timer--;
    
    if (beep_timer)
        beep_timer--;
    else
        BUZZER = 0;

    scan_keyboard_request = 1;
}
