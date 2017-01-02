#define DS_SECOND	0x80
#define DS_MINUTE	0x82
#define DS_HOUR		0x84 
#define DS_WEEK		0x8A
#define DS_DAY		0x86
#define DS_MONTH	0x88
#define DS_YEAR		0x8C
#define DS_BURST	0xBE
#define DS_CTRL		0x8E
#define DS_CHARGE	0x90

#define RTC_CE P3_0
#define RTC_IO P3_2
#define RTC_CLK P3_3

#define rtc_io_pin_in() { \
    P3M0 |=  (1 << 2); \
    P3M1 &= ~(1 << 2); \
}

#define rtc_io_pin_out() { \
    P3M0 &= ~(1 << 2); \
    P3M1 |=  (1 << 2); \
}

typedef struct {
//    uint8 y1000;
//    uint8 y100;
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

void rtc_delay()
{
    unsigned char i = 2;

    while (i--);
}

void rtc_write(unsigned char value)
{
    unsigned char i;

    RTC_CLK = 0;
    for (i = 8; i > 0; i--) {
        RTC_IO = (value & 0x01);
        value = value >> 1;
        RTC_CLK = 1;
        rtc_delay();
        RTC_CLK = 0;
    }
}

unsigned char rtc_read()
{
    unsigned char i, value = 0;

    RTC_CLK = 0;
    for (i = 8; i > 0; i--) {
        value = value >> 1;
        if (RTC_IO)
            value += 0x80;
        RTC_CLK = 1;
        rtc_delay();
        RTC_CLK = 0;
    }

    return value;
}

void rtc_Write_Direct(unsigned char addr, unsigned char value)
{
    RTC_CE = 0;
    RTC_CLK = 0;
    RTC_CE = 1;

    rtc_write(addr & ~0x01);
    rtc_write(value);

    RTC_CLK = 0;
    RTC_CE = 0;
}

unsigned char rtc_Read_Direct(unsigned char addr)
{
    unsigned char ret;

    RTC_CE = 0;
    RTC_CLK = 0;
    RTC_CE = 1;

    rtc_write(addr | 0x01);
    ret = rtc_read();

    RTC_CLK = 0;
    RTC_CE = 0;

    return ret;
}

void rtc_Write_Burst(date_time_t *date)
{
    RTC_CE = 0;
    RTC_CLK = 0;
    RTC_CE = 1;

    rtc_write(DS_BURST & ~0x01);
    rtc_write(((date->s10 << 4) | date->s1)   & 0x7F);
    rtc_write(((date->mm10 << 4) | date->mm1) & 0x7F);
    rtc_write(((date->h10 << 4) | date->h1)   & 0x3F);
    rtc_write(((date->d10 << 4) | date->d1)   & 0x3F);
    rtc_write(((date->m10 << 4) | date->m1)   & 0x1F);
    rtc_write(date->wday                      & 0x07);
    rtc_write(((date->y10 << 4) | date->y1)   & 0xFF);
    rtc_write(                                  0x00);

    RTC_CLK = 0;
    RTC_CE = 0;
}

void rtc_Read_Burst(date_time_t *date)
{
    RTC_CE = 0;
    RTC_CLK = 0;
    RTC_CE = 1;

    rtc_write(DS_BURST | 0x01);
    date->s1 = rtc_read()   & 0x7F;
    date->mm1 = rtc_read()  & 0x7F;
    date->h1 = rtc_read()   & 0x3F;
    date->d1 = rtc_read()   & 0x3F;
    date->m1 = rtc_read()   & 0x1F;
    date->wday = rtc_read() & 0x07;
    date->y1 = rtc_read()   & 0xFF;
    rtc_read();

    RTC_CLK = 0;
    RTC_CE = 0;

    date->s10 = date->s1 >> 4;
    date->mm10 = date->mm1 >> 4;
    date->h10 = date->h1 >> 4;
    date->d10 = date->d1 >> 4;
    date->m10 = date->m1 >> 4;
    date->y10 = date->y1 >> 4;

    date->s1 = date->s1 & 0x0F;
    date->mm1 = date->mm1 & 0x0F;
    date->h1 = date->h1 & 0x0F;
    date->d1 = date->d1 & 0x0F;
    date->m1 = date->m1 & 0x0F;
    date->y1 = date->y1 & 0x0F;
}

void rtc_Init()
{
    unsigned char sec;

//    P3M0 = 0;
//    P3M1 |= (1 << 0) | (1 << 3);     // configure CE & CLK as push-pull output

    rtc_Write_Direct(DS_CTRL, 0);

    sec = rtc_Read_Direct(DS_SECOND);
    if (sec & 0x80)
        rtc_Write_Direct(DS_SECOND, 0);
}
