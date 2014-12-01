//----------------------------------------------------------------------------
// 시리얼을 사용해서 WinCE의 텍스트 출력 모듈로 화면을 출력하는 옛날 모듈
//----------------------------------------------------------------------------
#include "main.h"
#include "old_wince_lcd.h"

//----------------------------------------------------------------------------
// Global variable
//----------------------------------------------------------------------------
#define CMD_PRINT			1
#define CMD_PRINT_ATTR		2
#define CMD_CLEAR			3
#define CMD_CLEAR_LINE		4
#define CMD_CLEAR_SCR		5
#define CMD_CURSOR_ON		6
#define CMD_CURSOR_OFF		7
#define CMD_CURSOR_2		8
#define CMD_CURSOR_8		9
#define CMD_CURSOR_MOVE		10
#define CMD_EXIT			99

#define LCD_COLUMN		40
#define LCD_LINE		17

#define TX_BUF_SIZE     64

//----------------------------------------------------------------------------
// Global variable
//----------------------------------------------------------------------------
static char tx_buffer[TX_BUF_SIZE];

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void LCD_EXIT(void);
void LCD_PRINT(const int pos_x, const int pos_y, const char *string, char attribute);
void LCD_STRING(byte x, byte y, char *string, char attribute, int bReverse);
void LCD_PRINT_STATUS(const char *message);
void LCD_STRING_STATUS(const char *message);
void CURSOR8(int x, int y);	// put a block cursor of 1 character size
void CURSOR2(int x, int y);
void CURSOROFF(void);
void FillLine(int n1, int n2);
void FillLine2(int n1, int n2, unsigned char d);
void ClearImage(void);
void ClearMessageWindow(void);
void ClearScreen(bool reverse);
    static void ARM_Disply(void);

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------

void LCD_TextUpdate(void)
{
}

void LCD_ClearScreen(void)
{
    ClearScreen(false);
}

void ARM_Disply(void)
{
    int send=0, len=strlen(tx_buffer);

    while ( len > DATA_MSG_MAX_LEN )
    {
        MsgSend(MSG_TYPE_DISPLAY, tx_buffer+send, DATA_MSG_MAX_LEN);
        send += DATA_MSG_MAX_LEN;
        len -= DATA_MSG_MAX_LEN;
    }

    if ( len > 0 )
        MsgSend(MSG_TYPE_DISPLAY, tx_buffer+send, len);

    /*
    for ( int i=0 ; i<tx_buf_len ; i++ )
    {
        WC(tx_buffer[i]);
    }
    */
}

void LCD_EXIT(void)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_EXIT;
    tx_buffer[2] = 0xff;
    tx_buffer[3] = 0;
	ARM_Disply();
}

void LCD_PRINT(const int x, const int y, const char *string, char attribute)
{
    int len = strlen(string);
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_PRINT;
    switch ( attribute )
    {
        case LCD_ATTR_CENTER:
            tx_buffer[2] = x+len>=LCD_COLUMN ? x+1 : x+1+(LCD_COLUMN-x-len)/2;
            break;
        case LCD_ATTR_RIGHT:
            tx_buffer[2] = x+len>=LCD_COLUMN ? x+1 : LCD_COLUMN-len+1;
            break;
        default:
            tx_buffer[2] = x+1;
            break;
    }
    tx_buffer[3] = y+1;
    strcpy(tx_buffer+4, string);
    tx_buffer[len+4] = 0xff;
    tx_buffer[len+5] = 0x0;
	ARM_Disply();
}

void LCD_STRING(byte x, byte y, char *string, char attribute, int bReverse)
{
    int len = strlen(string);
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_PRINT_ATTR;
    switch ( attribute )
    {
        case LCD_ATTR_CENTER:
            tx_buffer[2] = x+len>=LCD_COLUMN ? x+1 : x+1+(LCD_COLUMN-x-len)/2;
            break;
        case LCD_ATTR_RIGHT:
            tx_buffer[2] = x+len>=LCD_COLUMN ? x+1 : LCD_COLUMN-len+1;
            break;
        default:
            tx_buffer[2] = x+1;
            break;
    }
    tx_buffer[3] = y+1;
    tx_buffer[4] = bReverse?0x2:0x1;
    strcpy(tx_buffer+5, string);
    tx_buffer[len+5] = 0xff;
    tx_buffer[len+6] = 0x0;
	ARM_Disply();
}

void LCD_PRINT_STATUS(const char *message)
{
    int len = strlen(message);
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_PRINT;
    tx_buffer[2] = 1;
    tx_buffer[3] = LCD_LINE;
    tx_buffer[4] = 0x1;
    strcpy(tx_buffer+5, message);
    tx_buffer[len+5] = 0xff;
    tx_buffer[len+6] = 0x0;
	ARM_Disply();
}

void LCD_STRING_STATUS(const char *message)
{
    int len = strlen(message);
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_PRINT;
    tx_buffer[2] = 1;
    tx_buffer[3] = LCD_LINE;
    tx_buffer[4] = 0x1;
    strcpy(tx_buffer+5, message);
    tx_buffer[len+5] = 0xff;
    tx_buffer[len+6] = 0x0;
	ARM_Disply();
}

void CURSOR8(int x, int y)		// put a block cursor of 1 character size
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CURSOR_ON;
    tx_buffer[2] = 0xfe;
    tx_buffer[3] = CMD_CURSOR_8;
    tx_buffer[4] = 0xfe;
    tx_buffer[5] = CMD_CURSOR_MOVE;
    tx_buffer[6] = x+1;
    tx_buffer[7] = y+1;
    tx_buffer[8] = 0xff;
    tx_buffer[9] = 0x0;
	ARM_Disply();
}

void CURSOR2(int x, int y)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CURSOR_ON;
    tx_buffer[2] = 0xfe;
    tx_buffer[3] = CMD_CURSOR_2;
    tx_buffer[4] = 0xfe;
    tx_buffer[5] = CMD_CURSOR_MOVE;
    tx_buffer[6] = x+1;
    tx_buffer[7] = y+1;
    tx_buffer[8] = 0xff;
    tx_buffer[9] = 0x0;
	ARM_Disply();
}

void CURSOROFF(void)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CURSOR_OFF;
    tx_buffer[2] = 0xff;
    tx_buffer[3] = 0x0;
	ARM_Disply();
}

void FillLine(int n1, int n2)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CLEAR_LINE;
    tx_buffer[2] = n1+1;
    tx_buffer[3] = n2+1;
    tx_buffer[4] = 0x1;
    tx_buffer[5] = 0xff;
    tx_buffer[6] = 0x0;
	ARM_Disply();
}

void ClearScreen(bool reverse)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CURSOR_OFF;
    tx_buffer[2] = 0xfe;
    tx_buffer[3] = CMD_CLEAR_SCR;
    tx_buffer[4] = 0x1;
    tx_buffer[5] = 0xff;
    tx_buffer[6] = 0x0;
	ARM_Disply();
}

void FillLine2(int n1, int n2, unsigned char d)
{
    tx_buffer[0] = 0xfe;
    tx_buffer[1] = CMD_CLEAR_LINE;
    tx_buffer[2] = n1+1;
    tx_buffer[3] = n2+1;
    tx_buffer[4] = d?0x2:0x1;
    tx_buffer[5] = 0xff;
    tx_buffer[6] = 0x0;
	ARM_Disply();
}

void ClearImage(void)
{
	FillLine2(2, 13, 0xff);
}

void ClearMessageWindow(void)
{
	FillLine(2, 13);
}
