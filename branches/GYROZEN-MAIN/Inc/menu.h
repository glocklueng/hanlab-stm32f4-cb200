#ifndef __MENU_H__
#define __MENU_H__

#define MENU_MAX        15

typedef struct {
    char* title;
    signed char field_id;
} TMenuItem;

typedef struct {
    TMenuItem item[MENU_MAX];
    byte start_y;
    byte menu_cnt;
    signed char focus_mid;
    bool input_active;
} TMenu;

extern TMenu Menu;

void Menu_Init(int start_y);
int Menu_Add(char* title);
int Menu_Add_InputDigit(char* title, int x, char* format, char* init);
int Menu_Add_InputInteger(char* title, int x, char* format, int init, int min, int max, int step);
int Menu_Add_InputOption(char* title, int x, char** options, int init, int min, int max, int option_str_max_len);
int Menu_Print(int mid);
int Menu_Move(int step);
int Menu_InputSwitch(void);

#endif
