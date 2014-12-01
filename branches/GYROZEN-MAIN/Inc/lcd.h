#ifndef __LCD_H__
#define __LCD_H__

#define LCD_HEIGHT          64
#define LCD_WIDTH           128

#define LCD_TEXT_HEIGHT     10
#define LCD_TEXT_WIDTH      21

#define ONLY_CHAR           0
#define INVERT_CHAR         1
#define INVERT_SPACE        2
#define CLEAR_SPACE         3

void LCD_Init(void);
void LCD_Update(void);
void LCD_ClearScreen(void);
void LCD_GraphicPutc(int px, int py, char ch, int print_mode);
void LCD_GraphicPutc2(int px, int py, char ch, int FONT_WIDTH, bool invert);
void LCD_GraphicPrintf(int px, int py, int print_mode, char* fmt, ...);
void LCD_GraphicPrintf2(int px, int py, int print_mode, int font_width, char* fmt, ...);
void LCD_FillRect(int px, int py, int w, int h, bool invert);
void LCD_InvertRect(int px, int py, int w, int h);

void LCD_TextUpdate(void);
void LCD_Scroll(int start_line, int end_line, int scr_line);
void LCD_Printf(int line, int col, bool attr, char* fmt, ...);
void LCD_ScrollPrintf(bool update, char* fmt, ...);
void LCD_PRINT(int col, int line, char* str, int attr);
void CURSOR8(int col, int line);
void CURSOR2(int col, int line);
void CURSOROFF(void);
void FillLine2(int start_line, int end_line, int attr);

#endif
