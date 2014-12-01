#include "main.h"
#include "utility.h"

//---------------------------------------------------------------

char* F2S(double v);
float GetAverageFloat(float* ptr, int cnt, bool degree);
char* Float2Str(char* str, double param);
char* Int2Str(char* str, int param, int len, bool fill_space_with_zero);
char* float_sprintf(char* str, double f, int over_dot_max_len, bool over_dot_fill_zero, int under_dot_max_len);
float float_sscanf(char* fstr);
int int_sscanf(char* istr);
int hex_sscanf(char* xstr);
void Delay_ms(unsigned long time_ms);
void Delay_us(unsigned long time_us);
int RPM2RCF(int rpm);
int RCF2RPM(int rcf);

//---------------------------------------------------------------

char* F2S(double v)
{
    return Float2Str(NULL, v);
}

int RPM2RCF(int rpm)
{
    const double radius_cm = (double)ROTOR_RADIUS / 10;
    double rcf;
    rcf = round(radius_cm * (rpm * rpm) * 0.00001118);
    return rcf;
}

int RCF2RPM(int rcf)
{
    const double radius_cm = (double)ROTOR_RADIUS / 10;
    double rpm;
    rpm = sqrt((double)rcf/(radius_cm * 0.00001118));
    return rpm;
}

void Delay_us(unsigned long time_us)
{
    register unsigned long i;

    for( i=0 ; i<time_us ; i++ )
    {
        // 168Mhz
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
        asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
    }
}

void Delay_ms(unsigned long time_ms)
{
    Delay_us(time_ms * 1000);
}

int int_sscanf(char* istr)
{
    int i;
    char* str;
    char temp[21];

    if ( istr == NULL || *istr == '\0' )
        return 0;

    memset(temp, 0, 21);
    strncpy(temp, istr, 20);

    for ( str=temp ; *str && (*str < '1' || *str > '9') ; str++ )
    {
        *str = ' ';
    }

    if ( *str < '1' || *str > '9' )
        return 0;

    sscanf(temp, "%i", &i);
    return i;
}

int hex_sscanf(char* xstr)
{
    int x;
    char* str;
    char temp[21];

    if ( xstr == NULL || *xstr == '\0' )
        return 0;

    memset(temp, 0, 21);
    strncpy(temp, xstr, 20);

    for ( str=temp ; *str && !((*str>='1'&&*str<='9')||(*str>='a'&&*str<='f')||(*str>='A'&&*str<='F')) ; str++ )
    {
        *str = ' ';
    }

    if ( !((*str>='1'&&*str<='9')||(*str>='a'&&*str<='f')||(*str>='A'&&*str<='F')) )
        return 0;

    sscanf(temp, "%x", &x);
    return x;
}

float float_sscanf(char* fstr)
{
    double ret = 0;
    int i, over_dot, under_dot=0, under_dot_len=0;
    char *str, *under_dot_str = NULL;
    char temp[21];

    if ( fstr == NULL || *fstr == '\0' )
        return 0;

    memset(temp, 0, sizeof(temp));
    strncpy(temp, fstr, sizeof(temp)-1);

    for ( str=temp ; *str ; str++ )
    {
        if ( *str == '.' )
        {
            *str = '\0';
            under_dot_str = str + 1;
            break;
        }
    }

    over_dot = int_sscanf(temp);

    if ( under_dot_str && *under_dot_str )
    {
        under_dot_len = strlen(under_dot_str);
        under_dot = int_sscanf(under_dot_str);
        ret = under_dot;
        for ( i=0 ; i<under_dot_len ; i++ ) ret /= 10;
    }


    ret += over_dot;

    return ret;
}

char* float_sprintf(char* str, double f, int over_dot_max_len, bool over_dot_fill_zero, int under_dot_max_len)
{
    char under_dot_str[20];
    static char temp_fstr[20];

    if ( str == NULL )
        str = temp_fstr;

    Int2Str(str, (int)f, over_dot_max_len, over_dot_fill_zero);

    if ( f < 0 ) f = -f;
    f -= (int)f;
    Float2Str(under_dot_str, f);
    under_dot_str[under_dot_max_len+2] = '\0';
    strcat(str, under_dot_str+1);

    return str;
}

char* Int2Str(char* str, int param, int len_max, bool fill_space_with_zero)
{
    int len;
    int temp;
    static char order = 0;
    static char temp_str[6][12];

    if ( str == NULL )
    {
        str = &temp_str[order][0];
        order++;
        if ( order >= 6 ) order = 0;
    }

    for ( len=0, temp=param ; temp ; len++ )
        temp /= 10;

    if ( len == 0 )
        len = 1;

    if ( len >= len_max )
    {
        sprintf(str, "%i", param);
    }
    else
    {
        if ( param >= 0 )
        {
            if ( fill_space_with_zero )
                memset(str, '0', 12);
            else
                memset(str, ' ', 12);

            sprintf(str+len_max-len, "%i", param);
        }
        else
        {
            if ( fill_space_with_zero )
                memset(str, '0', 12);
            else
                memset(str, ' ', 12);

            str[0] = '-';
            sprintf(str+len_max-len+1, "%i", param);
        }
    }

    return str;
}

char* Float2Str(char* str, double param)
{
    #define INT_HEX_PRINT_MAX_LEN       11
    #define FLOAT_UNDER_DOT_LEN			6
    #define FLOAT_PRINT_MAX_LEN			(INT_HEX_PRINT_MAX_LEN+FLOAT_UNDER_DOT_LEN+1)
    #define SWAP4(p)    { unsigned char* tempp=(unsigned char*)(p); unsigned char tempch=tempp[3]; tempp[3]=tempp[0]; tempp[0]=tempch; tempch=tempp[1]; tempp[1]=tempp[2]; tempp[2]=tempch; }

    int len;
    signed char sign = (param >= 0 ? 1 : -1);
    static char temp_str[FLOAT_PRINT_MAX_LEN+1];
    unsigned long value1, value2;

    if ( str == NULL )
        str = temp_str;

    memset(str, ' ', sizeof(temp_str));
    str[sizeof(temp_str)-1] = '\0';

    if ( param < 0 ) param = -param;

    value1 = (unsigned long) param;
    value2 = (param - (unsigned long)param) * 1000000;

    for ( len=0 ; len<FLOAT_UNDER_DOT_LEN ; len++ )
    {
        str[(FLOAT_PRINT_MAX_LEN-1)-len] = (value2 % 10) + '0';
        value2 /= 10;
    }

    str[(FLOAT_PRINT_MAX_LEN-1)-len] = '.';
    len++;

    for ( len=0 ; len<INT_HEX_PRINT_MAX_LEN && (len==0 || value1>0) ; len++ )
    {
        str[(FLOAT_PRINT_MAX_LEN-1)-len-(FLOAT_UNDER_DOT_LEN+1)] = (value1 % 10) + '0';
        value1 /= 10;
    }

    if ( sign < 0 )
    {
        str[(FLOAT_PRINT_MAX_LEN-1)-len-(FLOAT_UNDER_DOT_LEN+1)] = '-';
        len++;
    }

    for ( int i=0 ; i<len+(FLOAT_UNDER_DOT_LEN+1)+1 ; i++ )
         str[i] = str[FLOAT_PRINT_MAX_LEN-len-(FLOAT_UNDER_DOT_LEN+1)+i];

    return str;
}

float GetAverageFloat(float* ptr, int cnt, bool degree)
{
    int i;
    float min=99999999, max=-99999999, average;
    int avg_cnt=0;
    double sum = 0;
    double sum2 = 0;

    if ( cnt<1 || ptr==NULL )
        return 0;

    for ( i=0 ; i<cnt ; i++ )
    {
        if ( degree )
        {
            if ( ptr[i] > 180 )
                ptr[i] -= 360;
            else if ( ptr[i] < -180 )
                ptr[i] += 360;
        }

        if ( ptr[i] > max )
            max = ptr[i];
        if ( ptr[i] < min )
            min = ptr[i];
    }

    if ( degree )
    {
        if ( max>0 && min<0 && (360-(max-min))<180 )
        {
            for ( i=0 ; i<cnt ; i++ )
                if ( ptr[i] < 0 )
                    ptr[i] += 360;
        }
    }

    for ( i=0 ; i<cnt ; i++ )
    {
        sum2 += ptr[i];
        if ( cnt<3 || degree || (ptr[i]>min && ptr[i]<max) )
        {
            avg_cnt++;
            sum += ptr[i];
        }
    }

    if ( avg_cnt == 0 )
        average = sum2 / cnt;
    else
        average = sum / avg_cnt;

    if ( degree )
    {
        if ( average > 180 )
            average -= 360;
        else if ( average < -180 )
            average += 360;
    }

    return average;
}
