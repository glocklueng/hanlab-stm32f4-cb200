#ifndef __TRACE_H__
#define __TRACE_H__

//-----------------------------------------
// TRACE ENABLE (1:ENABLE  0:DISABLE)
//-----------------------------------------
#ifndef TRACE_EN
#define TRACE_EN		1
#endif

//#define __TRACE_NOT_MIX_UNDER_MULTITASKING__

#if TRACE_EN != 0 && TRACE_EN != 1
#error TRACE_EN != 0 && TRACE_EN != 1
#endif

#if TRACE_EN == 0

#include <stdio.h>

#define REPORT(s)
#define LOG(s)
#define WARNING(s)
#define ERROR(s)
#define DPUTS(s)

#define TRACEF(param)
#define LOGF(param)
#define WARNINGF(param)
#define ERRORF(param)
#define DPRINTF(param)

#define RETURN(ercd)			    return (ercd)
#define RETURNDESC(ercd, err_str)   return (ercd)
#define RETURNI(ercd)			    return (ercd)
#define RETURNX(ercd)               return (ercd)
#define RETURNS(ercd)			    return (ercd)

#define FSTART()
#define FEND()
#define CHK()

#define VERIFY(cond)                cond
#define VERIFY_EQ(cond1,cond2)      cond1
#define VERIFY_NEQ(cond1,cond2)     cond1
#undef  ASSERT
#define ASSERT(cond)

#define WATCHS(vn,v)
#define WATCHI(vn,v)
#define WATCHF(vn,v)
#define WATCHX(vn,v)
#define WATCHB(vn,v)
#define WATCHC(vn,v)

#define WS(v)
#define WI(v)
#define WF(v)
#define WX(v)
#define WB(v)
#define WC(v)

#define RESULTS(vn,v)   v
#define RESULTI(vn,v)   v
#define RESULTX(vn,v)   v
#define RESULTB(vn,v)   v
#define RESULTC(vn,v)   v

#define RS(v)           v
#define RI(v)           v
#define RX(v)           v
#define RB(v)           v
#define RC(v)           v

#else							// TRACE_EN == 0

#define _DEBUG
#include <stdio.h>

#define MYTRACE_LOW(s)				do { DEBUGprintf_NewLine s; } while(0)
#define MYTRACE_HIGH(s)             do { DEBUGprintf_NewLine s; } while(0)

#ifdef  __TRACE_NOT_MIX_UNDER_MULTITASKING__
#undef  MYTRACE_LOW
#undef  MYTRACE_HIGH
#define MYTRACE_LOW(s)				do { osSemaphoreWait(UART_PRINTF_SEMAPHOR, 100);  DEBUGprintf_NewLine s; osSemaphoreRelease(UART_PRINTF_SEMAPHOR); } while(0)
#define MYTRACE_HIGH(s)             do { osSemaphoreWait(UART_PRINTF_SEMAPHOR, 100);  DEBUGprintf_NewLine s; osSemaphoreRelease(UART_PRINTF_SEMAPHOR); } while(0)
#endif

#define REPORT(s)					MYTRACE_LOW(("%s", (s)))
#define LOG(s)                    	MYTRACE_LOW(("%s", (s)))
#define WARNING(s)                	MYTRACE_HIGH(("%s", (s)))
#define ERROR(s)                  	MYTRACE_HIGH(("%s", (s)))
#define DPUTS(s)					MYTRACE_LOW(("%s", (s)))

#define TRACEF(param)				MYTRACE_LOW(param)
#define LOGF(param)					MYTRACE_LOW(param)
#define WARNINGF(param)				MYTRACE_HIGH(param)
#define ERRORF(param)				MYTRACE_HIGH(param)
#define DPRINTF(param)				MYTRACE_LOW(param)

#define RETURN(ercd)			    do { MYTRACE_LOW(("%s():%i RETURN: " #ercd, __FUNCTION__, __LINE__)); return (ercd); } while(0)
#define RETURNDESC(ercd, err_str)	do { MYTRACE_LOW(("%s():%i RETURN: " err_str, __FUNCTION__, __LINE__)); return (ercd); } while(0)
#define RETURNI(ercd)               do { int tempercd = (int)(ercd); MYTRACE_LOW(("%s():%i RETURN: %i", __FUNCTION__, __LINE__, tempercd)); return tempercd; } while(0)
#define RETURNX(ercd)               do { int tempercd = (int)(ercd); MYTRACE_LOW(("%s():%i RETURN: 0x%X", __FUNCTION__, __LINE__, tempercd)); return tempercd; } while(0)
#define RETURNS(ercd)               do { char* tempercd = (char*)(ercd); MYTRACE_LOW(("%s():%i RETURN: \"%s\"", __FUNCTION__, __LINE__, tempercd)); return tempercd; } while(0)

#define FSTART(fname)				MYTRACE_LOW(("FUNCTION START: %s()", __FUNCTION__))
#define FEND(fname)					MYTRACE_LOW(("FUNCTION END: %s():%i", __FUNCTION__, __LINE__))
#define CHK()                       MYTRACE_LOW(("CHECK: %s():%i", __FUNCTION__, __LINE__))

#define VERIFY(cond)                do { if ( (cond) ) MYTRACE_HIGH(("<[ASSERT! => %s():%i]>]> (" #cond ") == FALSE",__FUNCTION__,__LINE__)); } while(0)
#define VERIFY_EQ(cond1,cond2)      do { if ( (cond1) != (cond2) ) MYTRACE_HIGH(("<[ASSERT! => %s():%i]> (" #cond1 ") != (" #cond2 ")",__FUNCTION__,__LINE__)); } while(0)
#define VERIFY_NEQ(cond1,cond2)     do { if ( (cond1) == (cond2) ) MYTRACE_HIGH(("<[ASSERT! => %s():%i]> (" #cond1 ") == (" #cond2 ")",__FUNCTION__,__LINE__)); } while(0)
#undef  ASSERT
#define ASSERT(cond)                do { if ( !(cond) ) MYTRACE_HIGH(("<[ASSERT! => %s():%i]>]> (" #cond ") == FALSE",__FUNCTION__,__LINE__)); } while(0)

#define WATCHS(vn,v)                MYTRACE_LOW(("%s: %s",(vn),(v)))
#define WATCHI(vn,v)                MYTRACE_LOW(("%s: %i",(vn),(v)))
#define WATCHF(vn,v)                MYTRACE_LOW(("%s: %f",(vn),(v)))
#define WATCHX(vn,v)                MYTRACE_LOW(("%s: 0x%X",(vn),(v)))
#define WATCHB(vn,v)                MYTRACE_LOW(("%s: %s",(vn),((v) ? "TRUE" : "FALSE")))
#define WATCHC(vn,v)                do { char dch = (char)(v); MYTRACE_LOW(("%s: '%c' (0x%02X)",(vn),(dch<32 ? ' ' : dch),(unsigned char)dch)); } while(0)

#define WS(v)                       MYTRACE_LOW((#v ": %s",(v)))
#define WI(v)                       MYTRACE_LOW((#v ": %i",(v)))
#define WF(v)                       MYTRACE_LOW((#v ": %f",(v)))
#define WX(v)                       MYTRACE_LOW((#v ": 0x%X",(v)))
#define WB(v)                       MYTRACE_LOW((#v ": %s",((v) ? "TRUE" : "FALSE")))
#define WC(v)                       do { char dch = (char)(v); MYTRACE_LOW((#v ": '%c' (0x%02X)",(dch<32 ? ' ' : dch),(unsigned char)dch)); } while(0)

#define RESULTS(vn,v)               do { char* dstrv = (char*)(v); MYTRACE_LOW(("%s: %s",(vn),dstrv)); } while(0)
#define RESULTI(vn,v)               do { int dintv = (int)(v); MYTRACE_LOW(("%s: %i",(vn),dintv)); } while(0)
#define RESULTX(vn,v)               do { int dintv = (int)(v); MYTRACE_LOW(("%s: 0x%X",(vn),dintv)); } while(0)
#define RESULTB(vn,v)               do { int dintv = (int)(v); MYTRACE_LOW(("%s: %s",(vn),(dintv ? "TRUE" : "FALSE"))); } while(0)
#define RESULTC(vn,v)               do { char dchv = (char)(v); MYTRACE_LOW(("%s: '%c' (0x%02X)",(vn),(dchv<32 ? ' ' : dchv),(unsigned char)dchv)); } while(0)

#define RS(v)                       do { char* dstrv = (char*)(v); MYTRACE_LOW((#v ": %s",dstrv)); } while(0)
#define RI(v)                       do { int dintv = (int)(v); MYTRACE_LOW((#v ": %i",dintv)); } while(0)
#define RX(v)                       do { int dintv = (int)(v); MYTRACE_LOW((#v ": 0x%X",dintv)); } while(0)
#define RB(v)                       do { int dintv = (int)(v); MYTRACE_LOW((#v ": %s",(dintv ? "TRUE" : "FALSE"))); } while(0)
#define RC(v)                       do { char dchv = (char)(v); MYTRACE_LOW((#v ": '%c' (0x%02X)",(dchv<32 ? ' ' : dchv),(unsigned char)dchv)); } while(0)

#endif							// TRACE_EN == 0

#endif							// _TRACE_H_
