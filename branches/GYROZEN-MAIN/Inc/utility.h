#ifndef __UTILITY_H__
#define __UTILITY_H__

#define ARRAYSIZE(array_name)   (sizeof(array_name) / sizeof(*(array_name)))

#define TIME_FROM(t1)       ((t1)<=gSeconds ? gSeconds-(t1) : 0xffffffffUL-(t1)+gSeconds+1)
#define TICK_FROM(t1)       ((t1)<=gTick ? gTick-(t1) : 0xffffffffUL-(t1)+gTick+1)
#define TICK_FROM_EX(t1,t2) ((t1)<=((t2)=gTick) ? (t2)-(t1) : 0xffffffffUL-(t1)+(t2)+1)
#define TIME_BETWEEN(t1,t2) ((t1)<=(t2) ? (t2)-(t1) : 0xffffffffUL-(t1)+(t2)+1)

#define SWAP2(p)    { unsigned char* tempp=(unsigned char*)(p); unsigned char tempch=tempp[1]; tempp[1]=tempp[0]; tempp[0]=tempch; }
#define SWAP4(p)    { unsigned char* tempp=(unsigned char*)(p); unsigned char tempch=tempp[3]; tempp[3]=tempp[0]; tempp[0]=tempch; tempch=tempp[1]; tempp[1]=tempp[2]; tempp[2]=tempch; }

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
int mygets(char *s, int max);

#endif
