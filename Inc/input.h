#ifndef __INPUT_H__
#define __INPUT_H__

#define INPUT_FIELD_MAX         11
#define INPUT_FORMAT_MAX        30

typedef struct
{
    byte x, y;
    enum { INPUT_DIGIT=0, INPUT_INTEGER, INPUT_FLOAT, INPUT_OPTION } type;
    char* format;
    union {
        char digit_str[INPUT_FORMAT_MAX+1];
        struct { int cur, min, max, step; } ivalue;
        struct { float cur, min, max, step; } fvalue;
        struct { int cur, min, max, option_str_max_len; } ovalue;
    } data;
} TInputField;

typedef struct
{
    byte field_cnt;
    signed char focus_fid;
    signed char cursor_pos;
    TInputField field[INPUT_FIELD_MAX];
} TInput;

extern TInput Input;

void Input_Init(void);
int Input_Add_DigitType(int x, int y, char* format, char* init);
int Input_Add_IntegerType(int x, int y, char* format, int init, int min, int max, int step);
int Input_Add_OptionType(int x, int y, char** options, int init, int min, int max, int option_str_max_len);
int Input_Print(int fid, bool show_cursor);
int Input_SetField(int fid);
int Input_MoveByField(int step);
int Input_MoveByCursor(int step);
int Input_ChangeValue(int step);
int Input_SetValueInt(int fid, int value, char* format, bool draw);
int Input_SetValueFloat(int fid, float value, int over_dot_max_len, bool over_dot_fill_zero, int under_dot_max_len, bool draw);
int Input_SetValueDigit(int fid, char* init, bool draw);
int Input_GetValue(int fid, int* ret_i, float* ret_f);
int Input_GetValueStr(int fid, char** str);

#endif
