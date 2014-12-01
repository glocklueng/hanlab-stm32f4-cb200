#include "main.h"
#include "trace.h"
#include "menu.h"

//----------------------------------------------------------------------------
// Global Variable
//----------------------------------------------------------------------------
TMenu Menu;

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void Menu_Init(int start_y);
int Menu_Add(char* title);
int Menu_Add_InputDigit(char* title, int x, char* format, char* init);
int Menu_Add_InputInteger(char* title, int x, char* format, int init, int min, int max, int step);
int Menu_Add_InputOption(char* title, int x, char** options, int init, int min, int max, int option_str_max_len);
int Menu_Print(int mid);
int Menu_Move(int step);
int Menu_InputSwitch(void);

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
void Menu_Init(int start_y)
{
    Input_Init();
    memset(&Menu, 0, sizeof(Menu));
    Menu.start_y = start_y;
}

int Menu_Add(char* title)
{
    if ( title == NULL )
        return -1;

    Menu.item[Menu.menu_cnt].title = title;
    Menu.item[Menu.menu_cnt].field_id = -1;
    Menu.menu_cnt++;
    return Menu.menu_cnt-1;
}

int Menu_Add_InputDigit(char* title, int x, char* format, char* init)
{
    int fid;

    if ( title == NULL )
        return -1;

    fid = Input_Add_DigitType(x, Menu.start_y+Menu.menu_cnt, format, init);
    if ( fid < 0 ) return -1;

    Menu.item[Menu.menu_cnt].title = title;
    Menu.item[Menu.menu_cnt].field_id =  fid;
    Menu.menu_cnt++;

    return Menu.menu_cnt-1;
}

int Menu_Add_InputInteger(char* title, int x, char* format, int init, int min, int max, int step)
{
    int fid;

    if ( title == NULL )
        return -1;

    fid = Input_Add_IntegerType(x, Menu.start_y+Menu.menu_cnt, format, init, min, max, step);
    if ( fid < 0 ) return -1;

    Menu.item[Menu.menu_cnt].title = title;
    Menu.item[Menu.menu_cnt].field_id =  fid;
    Menu.menu_cnt++;

    return Menu.menu_cnt-1;
}

int Menu_Add_InputOption(char* title, int x, char** options, int init, int min, int max, int option_str_max_len)
{
    int fid;

    if ( title == NULL )
        return -1;

    fid = Input_Add_OptionType(x, Menu.start_y+Menu.menu_cnt, options, init, min, max, option_str_max_len);
    if ( fid < 0 ) return -1;

    Menu.item[Menu.menu_cnt].title = title;
    Menu.item[Menu.menu_cnt].field_id =  fid;
    Menu.menu_cnt++;

    return Menu.menu_cnt-1;
}

int Menu_Print(int mid)
{
    int i;

    if ( mid < 0 )
    {
        for ( i=0 ; i<Menu.menu_cnt ; i++ )
            Menu_Print(i);

        LCD_TextUpdate();
    }
    else
    {
        int y = Menu.start_y + mid;

        FillLine2(y, y, (mid == Menu.focus_mid ? 1 : 0));
        LCD_PRINT(0, y, Menu.item[mid].title, 0);

        if ( Menu.item[mid].field_id >= 0 )
        {
            Input_Print(Menu.item[mid].field_id, FALSE);
        }
    }

    return 0;
}

int Menu_Move(int step)
{
    int old_focus_mid = Menu.focus_mid;

    if ( step == 0 )
        return -1;

    if ( Menu.menu_cnt == 1 )
        return -1;

    Menu.focus_mid += step;
    while ( Menu.focus_mid >= Menu.menu_cnt )
        Menu.focus_mid -= Menu.menu_cnt;
    while ( Menu.focus_mid < 0 )
        Menu.focus_mid += Menu.menu_cnt;

    if ( old_focus_mid != Menu.focus_mid )
    {
        Menu_Print(old_focus_mid);
        Menu_Print(Menu.focus_mid);
        LCD_TextUpdate();
    }

    return 0;
}

int Menu_InputSwitch(void)
{
    if ( Menu.input_active )
    {
        Menu.input_active = FALSE;
        CURSOROFF();
        Input_Print(Menu.item[Menu.focus_mid].field_id, FALSE);
    }
    else if ( Menu.item[Menu.focus_mid].field_id >= 0 )
    {
        Menu.input_active = TRUE;
        Input_SetField(Menu.item[Menu.focus_mid].field_id);
    }

    LCD_TextUpdate();
    return 0;
}
