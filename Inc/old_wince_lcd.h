#ifndef __OLD_WINCE_LCD_H__
#define __OLD_WINCE_LCD_H__

#define LCD_ATTR_LEFT       1
#define LCD_ATTR_CENTER     2
#define LCD_ATTR_RIGHT      3
#define LCD_ATTR_CLEAR      4

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
void LCD_EXIT(void);

void LCD_TextUpdate(void);
void LCD_ClearScreen(void);


#endif
