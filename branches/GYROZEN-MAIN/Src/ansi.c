#include "main.h"
#include "ansi.h"

void ANSI_SetAttr(int attr)
{
    char str[10];

    sprintf(str, "\x1b[%im", attr);
    DEBUGprintf(str);
}

void ANSI_SetColor(int fore_color, int back_color)
{
    char str[10];

    sprintf(str, "\x1b[%im", fore_color);
    DEBUGprintf(str);

    sprintf(str, "\x1b[%im", back_color);
    DEBUGprintf(str);
}

void ANSI_SetCursor(int row, int col)
{
    char str[10];
    sprintf(str, "\x1b[%i;%iH", row, col);
    DEBUGprintf(str);
}

void ANSI_MoveCursor(int row, int col)
{
    char str[10];

    if ( row )
    {
        if ( row > 0 ) sprintf(str, "\x1b[%iA", row);
        else if ( row < 0 ) sprintf(str, "\x1b[%iB", -row);
        DEBUGprintf(str);
    }

    if ( col )
    {
        if ( col > 0 ) sprintf(str, "\x1b[%iC", col);
        else if ( col < 0 ) sprintf(str, "\x1b[%iD", -col);
        DEBUGprintf(str);
    }
}

void ANSI_ClearScreen(bool all)
{
    if ( all ) DEBUGprintf("\x1b[2J");
    else DEBUGprintf("\x1b[1J");
}

void ANSI_ClearLine(bool before_cursor, bool after_cursor)
{
    if ( before_cursor && after_cursor )
        DEBUGprintf("\x1b[2K");
    else if ( before_cursor )
        DEBUGprintf("\x1b[1K");
    else if ( after_cursor )
        DEBUGprintf("\x1b[K");
}
