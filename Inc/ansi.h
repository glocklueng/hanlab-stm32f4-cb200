#ifndef __ANSI_H__
#define __ANSI_H__

#define ANSI_ATTR_NORMAL            0
#define ANSI_ATTR_BOLD              1
#define ANSI_ATTR_UNDERSCORE        2
#define ANSI_ATTR_BLINK             3
#define ANSI_ATTR_REVERSE           4
#define ANSI_ATTR_CONCEALED         5

#define ANSI_FORECOLOR_BLACK        30
#define ANSI_FORECOLOR_RED          31
#define ANSI_FORECOLOR_GREEN        32
#define ANSI_FORECOLOR_YELLOW       33
#define ANSI_FORECOLOR_BLUE         34
#define ANSI_FORECOLOR_MAGENTA      35
#define ANSI_FORECOLOR_CYAN         36
#define ANSI_FORECOLOR_WHITE        37

#define ANSI_BACKCOLOR_BLACK        40
#define ANSI_BACKCOLOR_RED          41
#define ANSI_BACKCOLOR_GREEN        42
#define ANSI_BACKCOLOR_YELLOW       43
#define ANSI_BACKCOLOR_BLUE         44
#define ANSI_BACKCOLOR_MAGENTA      45
#define ANSI_BACKCOLOR_CYAN         46
#define ANSI_BACKCOLOR_WHITE        47

#define ANSI_ATTR(attr)             "\x1b[" #attr "m"
#define ANSI_OFF                    "\x1b[0m"
#define ANSI_BOLD                   "\x1b[1m"
#define ANSI_REVERSE                "\x1b[4m"
#define ANSI_SET(row,col)           "\x1b[" #row ";" #col "H"
#define ANSI_UP(row)                "\x1b[" #row "A"
#define ANSI_DOWN(row)              "\x1b[" #row "B"
#define ANSI_RIGHT(col)             "\x1b[" #col "C"
#define ANSI_LEFT(col)              "\x1b[" #col "D"
#define ANSI_CLRSCR                 "\x1b[2J"
#define ANSI_CLRSCR_BEFORE          "\x1b[1J"
#define ANSI_CLRLINE                "\x1b[2K"
#define ANSI_CLRLINE_BEFORE         "\x1b[1K"
#define ANSI_CLRLINE_AFTER          "\x1b[K"

void ANSI_SetAttr(int attr);
void ANSI_SetColor(int fore_color, int back_color);
void ANSI_SetCursor(int row, int col);
void ANSI_MoveCursor(int row, int col);
void ANSI_ClearScreen(bool all);
void ANSI_ClearLine(bool before_cursor, bool after_cursor);

#endif
