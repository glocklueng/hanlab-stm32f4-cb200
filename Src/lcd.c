#include "main.h"
#include "lcd.h"

//-------------------------------------------------------------------------------------

static char LcdTxtBuf[LCD_TEXT_HEIGHT][LCD_TEXT_WIDTH];
static char LcdTxtAttrBuf[LCD_TEXT_HEIGHT][LCD_TEXT_WIDTH];
static enum { CURSOR_OFF, CURSOR_2, CURSOR_8 } CursorMode = CURSOR_OFF;
static int CursorX, CursorY;

//-------------------------------------------------------------------------------------

void LCD_ClearScreen(void);
void LCD_TextUpdate(void);
void LCD_Scroll(int start_line, int end_line, int scr_line);
void LCD_Printf(int line, int col, bool attr, char* fmt, ...);
void LCD_ScrollPrintf(bool update, char* fmt, ...);
void LCD_PRINT(int col, int line, char* str, int attr);
void CURSOR8(int col, int line);
void CURSOR2(int col, int line);
void CURSOROFF(void);
void FillLine2(int start_line, int end_line, int attr);

void FontConvert(void);
void htextxy( char cho, char jung, char jong, char color);
void gputs( int x, int y, char color, char *st);

//-------------------------------------------------------------------------------------

void LCD_Scroll(int start_line, int end_line, int scr_line)
{
    if ( start_line<0 || start_line>=LCD_TEXT_HEIGHT )
        start_line = 0;

    if ( end_line<0 || end_line>=LCD_TEXT_HEIGHT )
        end_line = LCD_TEXT_HEIGHT-1;

    if ( end_line < start_line )
        return;

    if ( scr_line>=end_line-start_line+1 || scr_line<=-(end_line-start_line+1) )
    {
        memset(&LcdTxtBuf[start_line][0], ' ', (end_line-start_line+1)*LCD_TEXT_WIDTH);
    }
    else if ( scr_line == 0 )
    {
        return;
    }
    else if ( scr_line > 0 )
    {
        for ( int i=0 ; i<(end_line-start_line+1-scr_line) ; i++ )
            memcpy(&LcdTxtBuf[start_line+i][0], &LcdTxtBuf[start_line+scr_line+i][0], LCD_TEXT_WIDTH);
    }
    else if ( scr_line < 0 )
    {
        for ( int i=0 ; i<(end_line-start_line+1+scr_line) ; i++ )
            memcpy(&LcdTxtBuf[end_line-i][0], &LcdTxtBuf[end_line+scr_line-i][0], LCD_TEXT_WIDTH);
    }
}

void LCD_PRINT(int col, int line, char* str, int attr)
{
    if ( line<0 || line>=LCD_TEXT_HEIGHT || col<0 || col>=LCD_TEXT_WIDTH )
        return;

    strncpy(&LcdTxtBuf[line][col], str, LCD_TEXT_WIDTH-col);
    //memset(&LcdTxtAttrBuf[line][col], attr, strlen(str) < LCD_TEXT_WIDTH-col ? strlen(str) : LCD_TEXT_WIDTH-col);
}

void LCD_Printf(int line, int col, bool attr, char* fmt, ...)
{
    char buf[80];
    va_list args;

    if ( line<0 || line>=LCD_TEXT_HEIGHT || col<0 || col>=LCD_TEXT_WIDTH )
        return;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    strncpy(&LcdTxtBuf[line][col], buf, LCD_TEXT_WIDTH-col);
    memset(&LcdTxtAttrBuf[line][col], attr, strlen(buf) < LCD_TEXT_WIDTH-col ? strlen(buf) : LCD_TEXT_WIDTH-col);
}

void LCD_ScrollPrintf(bool update, char* fmt, ...)
{
    char buf[80];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    LCD_Scroll(0, LCD_TEXT_HEIGHT, 1);
    strncpy(&LcdTxtBuf[LCD_TEXT_HEIGHT-1][0], buf, LCD_TEXT_WIDTH);

    if ( update )
        LCD_TextUpdate();
}

void LCD_ClearScreen(void)
{
    CURSOROFF();
    memset(LcdTxtBuf, 0, sizeof(LcdTxtBuf));
    memset(LcdTxtAttrBuf, 0, sizeof(LcdTxtAttrBuf));
    LCD_TextUpdate();
}

void LCD_TextUpdate(void)
{
    /*
    for ( int y=0 ; y<LCD_TEXT_HEIGHT ; y++ )
        for ( int x=0 ; x<LCD_TEXT_WIDTH ; x++ )
            if ( LcdTxtBuf[y][x] )
                LCD_GraphicPutc(x*6 + (LcdTxtAttrBuf[y][x] ? 0 : 1), y==0 ? 1 : y*6+4, LcdTxtBuf[y][x], LcdTxtAttrBuf[y][x] ? INVERT_SPACE : ONLY_CHAR);
            else if ( LcdTxtAttrBuf[y][x] )
                LCD_GraphicPutc(x*6 + 1, y==0 ? 1 : y*6+4, ' ', INVERT_SPACE);

    if ( CursorMode == CURSOR_8 )
        LCD_InvertRect(CursorX*6, CursorY==0 ? 0 : CursorY*6+3, 7, 7);
    else if ( CursorMode == CURSOR_2 )
        LCD_InvertRect(CursorX*6, CursorY==0 ? 4 : CursorY*6+7, 7, 2);

    LCD_Update();
    */
}

void FillLine2(int start_line, int end_line, int attr)
{
    for ( int i=0 ; i<end_line-start_line+1 ; i++ )
        memset(&LcdTxtAttrBuf[start_line+i][0], attr, LCD_TEXT_WIDTH);
}

void CURSOR8(int col, int line)
{
    CursorMode = CURSOR_8;
    CursorX = col;
    CursorY = line;
}

void CURSOR2(int col, int line)
{
    CursorMode = CURSOR_2;
    CursorX = col;
    CursorY = line;
}

void CURSOROFF(void)
{
    CursorMode = CURSOR_OFF;
}
