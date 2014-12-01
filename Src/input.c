#include "main.h"
#include "input.h"

//----------------------------------------------------------------------------
// Global Variable
//----------------------------------------------------------------------------
TInput Input;

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void Input_Init(void);
    int CheckInputInit(void);
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

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
void Input_Init(void)
{
    memset(&Input, 0, sizeof(Input));
}

int CheckInputInit(void)
{
    int i;

    if ( Input.field_cnt > INPUT_FIELD_MAX )
        RETURN(-1);

    if ( Input.focus_fid == 0 && Input.field_cnt == 0 )
        ;
    else if ( Input.focus_fid >= Input.field_cnt || Input.focus_fid < 0 )
        RETURN(-2);

    if ( Input.cursor_pos >= INPUT_FORMAT_MAX || Input.cursor_pos < 0 )
        RETURN(-3);

    for ( i=0 ; i<Input.field_cnt ; i++ )
    {
        TInputField* field = &Input.field[i];

        if ( field->format == NULL )
            RETURN(-4);

        if ( field->type!=INPUT_OPTION && strlen(field->format)<1 )
            RETURN(-5);

        switch ( field->type )
        {
            case INPUT_DIGIT:
                if ( strlen(field->data.digit_str) > INPUT_FORMAT_MAX )
                    RETURN(-6);
                break;

            case INPUT_INTEGER:
                if ( field->data.ivalue.min > field->data.ivalue.max ||
                     field->data.ivalue.cur > field->data.ivalue.max ||
                     field->data.ivalue.cur < field->data.ivalue.min ||
                     field->data.ivalue.step <= 0 )
                    RETURN(-7);
                break;

            case INPUT_FLOAT:
                if ( field->data.fvalue.min > field->data.fvalue.max ||
                     field->data.fvalue.cur > field->data.fvalue.max ||
                     field->data.fvalue.cur < field->data.fvalue.min ||
                     field->data.fvalue.step <= 0 )
                    RETURN(-8);
                break;

            case INPUT_OPTION:
                if ( field->data.ovalue.min > field->data.ovalue.max ||
                     field->data.ovalue.cur > field->data.ovalue.max ||
                     field->data.ovalue.cur < field->data.ovalue.min ||
                     field->data.ovalue.option_str_max_len <= 0 )
                    RETURN(-9);
                break;

        }
    }

    if ( Input.field[Input.focus_fid].type!=INPUT_OPTION && Input.cursor_pos>=strlen(Input.field[Input.focus_fid].format) )
        RETURN(-10);

    return 0;
}

int Input_Add_DigitType(int x, int y, char* format, char* init)
{
    TInputField* field;

    if ( CheckInputInit() != 0 )
        Input_Init();
    else if ( Input.field_cnt >= INPUT_FIELD_MAX )
        return -1;
    else if ( x<0 || y<0 || format==NULL || init==NULL )
        return -1;

    field = &Input.field[Input.field_cnt];
    field->type = INPUT_DIGIT;
    field->x = x;
    field->y = y;
    field->format = format;
    strcpy(field->data.digit_str, init);
    Input.field_cnt++;

    return Input.field_cnt-1;
}

int Input_Add_IntegerType(int x, int y, char* format, int init, int min, int max, int step)
{
    TInputField* field;

    if ( CheckInputInit() != 0 )
        Input_Init();
    else if ( Input.field_cnt >= INPUT_FIELD_MAX )
        return -1;
    else if ( x<0 || y<0 || format==NULL )
        return -1;
    else if ( step <= 0 || min > max )
        return -1;
    else if ( init > max )
        init = max;
    else if ( init < min )
        init = min;

    field = &Input.field[Input.field_cnt];
    field->type = INPUT_INTEGER;
    field->x = x;
    field->y = y;
    field->format = format;
    field->data.ivalue.cur = init;
    field->data.ivalue.min = min;
    field->data.ivalue.max = max;
    field->data.ivalue.step = step;
    Input.field_cnt++;

    return Input.field_cnt-1;
}

int Input_Add_FloatType(int x, int y, char* format, float init, float min, float max, float step)
{
    TInputField* field;

    if ( CheckInputInit() != 0 )
        Input_Init();
    else if ( Input.field_cnt >= INPUT_FIELD_MAX )
        return -1;
    else if ( x<0 || y<0 || format==NULL )
        return -1;
    else if ( step <= 0 || min > max )
        return -1;
    else if ( init > max )
        init = max;
    else if ( init < min )
        init = min;

    field = &Input.field[Input.field_cnt];
    field->type = INPUT_FLOAT;
    field->x = x;
    field->y = y;
    field->format = format;
    field->data.fvalue.cur = init;
    field->data.fvalue.min = min;
    field->data.fvalue.max = max;
    field->data.fvalue.step = step;
    Input.field_cnt++;

    return Input.field_cnt-1;
}

int Input_Add_OptionType(int x, int y, char** options, int init, int min, int max, int option_str_max_len)
{
    TInputField* field;

    if ( CheckInputInit() != 0 )
        Input_Init();
    else if ( Input.field_cnt >= INPUT_FIELD_MAX )
        return -1;
    else if ( x<0 || y<0 || options==NULL )
        return -1;
    else if ( option_str_max_len <= 0 || min > max )
        return -1;
    else if ( init > max )
        init = max;
    else if ( init < min )
        init = min;

    field = &Input.field[Input.field_cnt];
    field->type = INPUT_OPTION;
    field->x = x;
    field->y = y;
    field->format = (char*) options;
    field->data.ovalue.cur = init;
    field->data.ovalue.min = min;
    field->data.ovalue.max = max;
    field->data.ovalue.option_str_max_len = option_str_max_len;
    Input.field_cnt++;

    return Input.field_cnt-1;
}

int Input_Print(int fid, bool show_cursor)
{
    int i;
    char str[INPUT_FORMAT_MAX+1];

    if ( CheckInputInit() != 0 )
        return -1;

    if ( Input.field_cnt == 0 || fid >= Input.field_cnt )
        return -1;

    if ( fid < 0 )
    {
        for (i=0; i<Input.field_cnt; i++)
            Input_Print(i, show_cursor);

        LCD_TextUpdate();
    }
    else if ( fid < Input.field_cnt )
    {
        TInputField* field = &Input.field[fid];

        switch ( field->type )
        {
            case INPUT_OPTION:
                {
                    char** options = (char**)field->format;
                    memset(str, ' ', INPUT_FORMAT_MAX);
                    str[field->data.ovalue.option_str_max_len] = 0;
                    LCD_PRINT(field->x, field->y, str, 0);
                    LCD_PRINT(field->x, field->y, options[field->data.ovalue.cur - field->data.ovalue.min], 0);
                }
                break;
            case INPUT_DIGIT:
                LCD_PRINT(field->x, field->y, field->data.digit_str, 0);
                break;
            case INPUT_INTEGER:
                sprintf(str, field->format, field->data.ivalue.cur);
                LCD_PRINT(field->x, field->y, str, 0);
                break;
            case INPUT_FLOAT:
                sprintf(str, field->format, field->data.fvalue.cur);
                LCD_PRINT(field->x, field->y, str, 0);
                break;
        }

        if ( show_cursor && fid == Input.focus_fid )
        {
            if ( field->type == INPUT_DIGIT )
                CURSOR8(field->x+Input.cursor_pos, field->y);
            else
                CURSOR2(field->x+Input.cursor_pos, field->y);
        }
    }

    return 0;
}

int Input_SetField(int fid)
{
    int old_focus = Input.focus_fid;

    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    Input.focus_fid = fid;
    Input.cursor_pos = 0;

    if ( old_focus != Input.focus_fid )
        Input_Print(old_focus, FALSE);

    Input.cursor_pos = 0;
    Input_Print(Input.focus_fid, TRUE);
    LCD_TextUpdate();

    return 0;
}

int Input_MoveByField(int step)
{
    int i, old_focus = Input.focus_fid;

    if ( CheckInputInit() != 0 )
        return -1;

    if ( step == 0 || Input.field_cnt <= 1 )
        return -1;

    if ( step > 0 )
    {
        for ( i=0 ; i<step ; i++ )
        {
            Input.focus_fid++;
            if ( Input.focus_fid >= Input.field_cnt )
                Input.focus_fid = 0;
        }
    }
    else if ( step < 0 )
    {
        step *= -1;
        for ( i=0 ; i<step ; i++ )
        {
            Input.focus_fid--;
            if ( Input.focus_fid < 0 )
                Input.focus_fid = Input.field_cnt - 1;
        }
    }

    if ( old_focus != Input.focus_fid )
        Input_Print(old_focus, FALSE);

    Input.cursor_pos = 0;
    Input_Print(Input.focus_fid, TRUE);
    LCD_TextUpdate();

    return 0;
}

int Input_MoveByCursor(int step)
{
    int ret = 0;
    int i, len;
    char fmt_ch;
    int old_cursor_pos = Input.cursor_pos;
    TInputField* field;

    if ( CheckInputInit() != 0 )
        return -1;

    field = &Input.field[Input.focus_fid];

    if ( step == 0 || Input.field_cnt == 0 || field->type != INPUT_DIGIT )
        return -1;

    len = strlen(field->format);

    if ( len <= 1 )
        return -1;

    if ( step > 0 )
    {
        for ( i=0 ; i<step ; i++ )
        {
            Input.cursor_pos++;
            if ( Input.cursor_pos >= len ) Input.cursor_pos = 0;

            for ( fmt_ch = field->format[Input.cursor_pos];
                  !((fmt_ch>='1'&&fmt_ch<='9')||(fmt_ch>='A'&&fmt_ch<='F')) || Input.cursor_pos == old_cursor_pos;
                  fmt_ch = field->format[Input.cursor_pos])
            {
                Input.cursor_pos++;
                if ( Input.cursor_pos >= len )
                {
                    Input.cursor_pos = 0;
                    ret = 1;
                }
            }
        }
    }
    else
    {
        step *= -1;
        for ( i=0 ; i<step ; i++ )
        {
            Input.cursor_pos--;
            if ( Input.cursor_pos < 0 ) Input.cursor_pos = len-1;

            for ( fmt_ch = field->format[Input.cursor_pos];
                  !((fmt_ch>='1'&&fmt_ch<='9')||(fmt_ch>='A'&&fmt_ch<='F')) || Input.cursor_pos == old_cursor_pos;
                  fmt_ch = field->format[Input.cursor_pos])
            {
                Input.cursor_pos--;
                if ( Input.cursor_pos < 0 )
                {
                    Input.cursor_pos = len-1;
                    ret = 1;
                }
            }
        }
    }

    Input_Print(Input.focus_fid, TRUE);
    LCD_TextUpdate();

    return ret;
}

int Input_ChangeValue(int step)
{
    TInputField* field;

    if ( CheckInputInit() != 0 )
        return -1;

    if ( step == 0 || Input.field_cnt == 0 )
        return -1;

    field = &Input.field[Input.focus_fid];

    switch ( field->type )
    {
        case INPUT_DIGIT:
            {
                char new_ch;
                char fmt_ch = field->format[Input.cursor_pos];
                char cur_ch = field->data.digit_str[Input.cursor_pos];
                int fmt_value = fmt_ch>='A' ? fmt_ch-'A'+10 : fmt_ch-'0';
                int cur_value = cur_ch>='A' ? cur_ch-'A'+10 : cur_ch-'0';

                if ( step > 0 )
                {
                    switch ( fmt_ch )
                    {
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case 'A':
                        case 'B':
                        case 'C':
                        case 'D':
                        case 'E':
                        case 'F':
                            if ( cur_value < fmt_value )
                            {
                                cur_value++;
                                if ( cur_value > 9 )
                                    new_ch = cur_value - 10 + 'A';
                                else
                                    new_ch = cur_value + '0';
                            }
                            else
                                new_ch = '0';
                            break;
                    }
                }
                else
                {
                    switch ( fmt_ch )
                    {
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case 'A':
                        case 'B':
                        case 'C':
                        case 'D':
                        case 'E':
                        case 'F':
                            if ( cur_value > 0 )
                            {
                                cur_value--;
                                if ( cur_value > 9 )
                                    new_ch = cur_value - 10 + 'A';
                                else
                                    new_ch = cur_value + '0';
                            }
                            else
                                new_ch = fmt_ch;
                            break;
                    }
                }

                field->data.digit_str[Input.cursor_pos] = new_ch;
            }
            break;

        case INPUT_INTEGER:
            field->data.ivalue.cur += step * field->data.ivalue.step;
            if ( field->data.ivalue.cur > field->data.ivalue.max ) field->data.ivalue.cur = field->data.ivalue.max;
            if ( field->data.ivalue.cur < field->data.ivalue.min ) field->data.ivalue.cur = field->data.ivalue.min;
            break;

        case INPUT_FLOAT:
            field->data.fvalue.cur += step * field->data.fvalue.step;
            if ( field->data.fvalue.cur > field->data.fvalue.max ) field->data.fvalue.cur = field->data.fvalue.max;
            if ( field->data.fvalue.cur < field->data.fvalue.min ) field->data.fvalue.cur = field->data.fvalue.min;
            break;

        case INPUT_OPTION:
            field->data.ovalue.cur += step;
            if ( field->data.ovalue.cur > field->data.ovalue.max ) field->data.ovalue.cur = field->data.ovalue.max;
            if ( field->data.ovalue.cur < field->data.ovalue.min ) field->data.ovalue.cur = field->data.ovalue.min;
            break;
    }

    Input_Print(Input.focus_fid, TRUE);
    LCD_TextUpdate();

    return 0;
}

int Input_SetValueInt(int fid, int value, char* format, bool draw)
{
    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    switch ( Input.field[fid].type )
    {
        case INPUT_DIGIT:
            //memset(Input.field[fid].data.digit_str, 0, INPUT_FORMAT_MAX+1);
            sprintf(Input.field[fid].data.digit_str, format, value);
            break;

        case INPUT_OPTION:
            if ( value >= Input.field[fid].data.ovalue.min && value <= Input.field[fid].data.ovalue.max )
                Input.field[fid].data.ovalue.cur = value;
            else
                return -2;
            break;

        case INPUT_INTEGER:
            if ( value >= Input.field[fid].data.ivalue.min && value <= Input.field[fid].data.ivalue.max )
                Input.field[fid].data.ivalue.cur = value;
            else
                return -2;
            break;

        case INPUT_FLOAT:
            if ( value >= Input.field[fid].data.fvalue.min && value <= Input.field[fid].data.fvalue.max )
                Input.field[fid].data.fvalue.cur = value;
            else
                return -2;
            break;
    }

    if ( draw )
    {
        Input_Print(fid, TRUE);
        LCD_TextUpdate();
    }

    return 0;
}

int Input_SetValueDigit(int fid, char* init, bool draw)
{
    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    if ( Input.field[fid].type != INPUT_DIGIT )
        return -1;

    strcpy(Input.field[fid].data.digit_str, init);

    if ( draw )
    {
        Input_Print(fid, TRUE);
        LCD_TextUpdate();
    }

    return 0;
}

int Input_SetValueFloat(int fid, float value, int over_dot_max_len, bool over_dot_fill_zero, int under_dot_max_len, bool draw)
{
    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    switch ( Input.field[fid].type )
    {
        case INPUT_DIGIT:
            //memset(Input.field[fid].data.digit_str, 0, INPUT_FORMAT_MAX+1);
            float_sprintf(Input.field[fid].data.digit_str, value, over_dot_max_len, over_dot_fill_zero, under_dot_max_len);
            break;

        case INPUT_INTEGER:
        case INPUT_OPTION:
            return -2;

        case INPUT_FLOAT:
            if ( value >= Input.field[fid].data.fvalue.min && value <= Input.field[fid].data.fvalue.max )
                Input.field[fid].data.fvalue.cur = value;
            else
                return -2;
            break;
    }

    if ( draw )
    {
        Input_Print(fid, TRUE);
        LCD_TextUpdate();
    }

    return 0;
}

int Input_GetValueStr(int fid, char** str)
{
    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    if ( str == NULL )
        return -1;

    if ( Input.field[fid].type != INPUT_DIGIT )
        return -1;

    if ( str )
        *str = Input.field[fid].data.digit_str;

    return 0;
}

int Input_GetValue(int fid, int* ret_i, float* ret_f)
{
    if ( CheckInputInit() != 0 )
        return -1;

    if ( fid < 0 || fid >= Input.field_cnt )
        return -1;

    if ( ret_i == NULL && ret_f == NULL )
        return -1;

    switch ( Input.field[fid].type )
    {
        case INPUT_DIGIT:
            if ( ret_i ) *ret_i = int_sscanf(Input.field[fid].data.digit_str);
            if ( ret_f ) *ret_f = float_sscanf(Input.field[fid].data.digit_str);
            break;

        case INPUT_INTEGER:
            if ( ret_i ) *ret_i = Input.field[fid].data.ivalue.cur;
            if ( ret_f ) *ret_f = Input.field[fid].data.ivalue.cur;
            break;

        case INPUT_OPTION:
            if ( ret_i ) *ret_i = Input.field[fid].data.ovalue.cur;
            if ( ret_f ) *ret_f = Input.field[fid].data.ovalue.cur;
            break;

        case INPUT_FLOAT:
            if ( ret_f ) *ret_f = Input.field[fid].data.fvalue.cur;
            else return -2;
            break;
    }

    return 0;
}
