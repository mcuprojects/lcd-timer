//LCD Functions Developed by electroSome

//LCD Module Connections
extern bit LCD_RS;
extern bit LCD_RW;
extern bit LCD_EN;
//#define LCD_PORT P1


void lcd_Delay(int a)
{
    //unsigned char j;
    //unsigned char i;
    int i, j;
    for (i = 0; i < a; i++) {
        for(j = 0; j < 100; j++) {
        }
    }
}

void lcd_Cmd(char a)
{ 
    LCD_RS = 0;              // => LCD_RS = 0
    LCD_PORT = a;
    LCD_EN  = 1;             // => E = 1
    lcd_Delay(10);
    LCD_EN  = 0;             // => E = 0
}

void lcd_Write_Data(char c)
{
    LCD_RS = 1;              // => LCD_RS = 1
    LCD_PORT = c;
    LCD_EN  = 1;             // => E = 1
    lcd_Delay(10);
    LCD_EN  = 0;             // => E = 0
}

lcd_Clear()
{
    lcd_Cmd(0x01);
    lcd_Delay(10);
}

void lcd_Init()
{
    LCD_PORT = 0x00;
    LCD_RS = 0;
    LCD_RW = 0;
    lcd_Delay(200);
    lcd_Clear();
    ///////////// Reset process from datasheet /////////
    lcd_Cmd(0x30);
    lcd_Delay(50);
    lcd_Cmd(0x30);
    lcd_Delay(110);
    lcd_Cmd(0x30);
    /////////////////////////////////////////////////////
    lcd_Cmd(0x38);    //function set
    lcd_Cmd(0x0C);    //display on,cursor off,blink off
    lcd_Cmd(0x01);    //clear display
    lcd_Cmd(0x06);    //entry mode, set increment
    lcd_Delay(50);
}

void lcd_Set_Cursor_Pos(char row, char col)
{
    if (row == 0)
        lcd_Cmd(0x80 + col);
    else if (row == 1)
        lcd_Cmd(0xC0 + col);
}

void lcd_Write_String(char *a)
{
    int i;
    for (i = 0; a[i] != '\0'; i++)
        lcd_Write_Data(a[i]);
}

void lcd_Load_Custom_Symbol(const unsigned char addr, const unsigned char *pattern)
{
    unsigned char i;
    lcd_Cmd(0x40 + (addr << 3));
    for (i = 0; i < 8; i++)
        lcd_Write_Data(pattern[i]);
}

void lcd_Cursor_Blink_On()
{
    lcd_Cmd(0x0F);
}

void lcd_Cursor_Blink_Off()
{
    lcd_Cmd(0x0C);
}

void lcd_Shift_Right()
{
    lcd_Cmd(0x1C);
}

void lcd_Shift_Left()
{
    lcd_Cmd(0x18);
}
