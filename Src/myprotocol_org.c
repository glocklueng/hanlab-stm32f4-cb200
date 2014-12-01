#define TRACE_EN		0
#include "main.h"
#include "myprotocol.h"

//#define __CREATE_WASTE_TASK__

//----------------------------------------------------------------------------
// User Defined Macro
//----------------------------------------------------------------------------
osThreadDefine(SEND_THREAD, MsgSendTask, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);
osThreadDefine(RECV_THREAD, MsgRecvTask, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE);

#define SOX     0xA0
#define EOX     0xC0
#define ESC     0xE0

#define MSG_TYPE_DEBUG          0x40
#define MSG_TYPE_LOG            0x20
#define MSG_TYPE_DISPLAY        0x60

#define MSG_TYPE_CONTROL        0x1C
#define MSG_TYPE_ACK            0x1C
#define MSG_TYPE_NAK            0x18
#define MSG_TYPE_REQ_CONN       0x14
#define MSG_TYPE_CONN           0x10

#define MSG_TX_CTRL_Q_SIZE       10
#define MSG_TX_DATA_Q_SIZE       30
#define MSG_TX_DATA_Q_SIZE_MAX   127

#define MSG_RX_DISP_BUF_SIZE     200
#define MSG_RX_LOG_BUF_SIZE      500
#define MSG_RX_DEBUG_BUF_SIZE    500

#define GetMsgType(type_and_size)   ((type_and_size)&0x60 ? (type_and_size)&0x60 : (type_and_size)&0x1C)
#define GetMsgSize(type_and_size)   ((type_and_size)&0x60 ? (type_and_size)&0x1F : (type_and_size)&0x03)

#define SCI1TXBYTE(ch)              if ( (ch)==SOX || (ch)==EOX || (ch)==ESC ) { SCI_Putc(SCI_PORT, ESC); SCI_Putc(SCI_PORT, (ch)^0x80); } else { SCI_Putc(SCI_PORT, ch); }

#define MSG_FLAG_CONNECT                   0x1

#define MSG_FLAG_TX_DATA_Q_IN_USE          0x2
#define MSG_FLAG_TX_CTRL_Q_IN_USE          0x4
#define MSG_FLAG_TX_DATA_Q_IS_FULL         0x8
#define MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS   0x10
#define MSG_FLAG_TX_CTRL_Q_IS_FULL         0x20
#define MSG_FLAG_TX_CTRL_Q_IS_EMPTY        0x40

#define MSG_FLAG_RX_DISP_IS_EMPTY          0x80
#define MSG_FLAG_RX_DEBUG_IS_EMPTY         0x100
#define MSG_FLAG_RX_LOG_IS_EMPTY           0x200
#define MSG_FLAG_RX_DISP_IS_FULL           0x400
#define MSG_FLAG_RX_DEBUG_IS_FULL          0x800
#define MSG_FLAG_RX_LOG_IS_FULL            0x1000
#define MSG_FLAG_RX_DISP_IN_USE            0x2000
#define MSG_FLAG_RX_DEBUG_IN_USE           0x4000
#define MSG_FLAG_RX_LOG_IN_USE             0x8000

//----------------------------------------------------------------------------
// User Defined Type
//----------------------------------------------------------------------------
struct DataMessage
{
    unsigned char type_and_size;
    unsigned char seq;
    unsigned char data[MSG_MAX];
    unsigned char crc8;
};

struct CtrlMessage
{
    unsigned char type_and_size;
    unsigned char seq;
    unsigned char crc8;
};

//----------------------------------------------------------------------------
// Global Variable
//----------------------------------------------------------------------------
static bool volatile MsgConnect = FALSE;
static bool volatile MsgProtocolInError = FALSE;

static unsigned char volatile MsgTxFront = 0; // 전송완료가 확인된 경우 이동한다.
static unsigned char volatile MsgTxRear = 0;
static unsigned char volatile MsgTxPosition = 0; // Front와 유사하지만 전송하자마자 이동한다.
static struct DataMessage MsgTxQueue[MSG_TX_DATA_Q_SIZE];

static unsigned char volatile MsgTxCtrlFront = 0;
static unsigned char volatile MsgTxCtrlRear = 0;
static struct CtrlMessage MsgTxCtrlQueue[MSG_TX_CTRL_Q_SIZE];

static unsigned char volatile MsgRxDispBuf[MSG_RX_DISP_BUF_SIZE];
static unsigned char volatile MsgRxLogBuf[MSG_RX_LOG_BUF_SIZE];
static unsigned char volatile MsgRxDebugBuf[MSG_RX_DEBUG_BUF_SIZE];

static unsigned short volatile MsgRxDispFront=0;
static unsigned short volatile MsgRxDispRear=0;
static unsigned short volatile MsgRxLogFront=0;
static unsigned short volatile MsgRxLogRear=0;
static unsigned short volatile MsgRxDebugFront=0;
static unsigned short volatile MsgRxDebugRear=0;

static OS_FLAG_GRP *MsgFlag;

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void MsgFlush(int type);
bool MsgServiceIsConnected(void);
bool MsgServiceWaitConnected(int wait_tick);
bool MsgServiceIsEnabled(void);
void MsgServiceStart(void);
void MsgServiceEnd(void);
#ifdef __CREATE_WASTE_TASK__
    static void MsgWasteTask1(void* p_arg);
    static void MsgWasteTask2(void* p_arg);
    static void MsgWasteTask3(void* p_arg);
#endif
static void MsgSendTask(void* p_arg);
    static void MsgInit(void);
static void MsgRecvTask(void* p_arg);
    static void MsgSendNAK(int last_recv_seq, int occur_step);
int MsgSend(int type, void* sendbuf, int bufsize);
int MsgRecv(int type, void* recvbuf, int bufsize, int wait_tick, bool wait_full);

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
#ifdef __CREATE_WASTE_TASK__
void MsgWasteTask1(void* p_arg)
{
    int size;
    unsigned char buf[MSG_MAX];

    FSTART();

    while ( 1 )
    {
        if ( OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ )
            break;

        size = MsgRecv(MSG_TYPE_DEBUG, buf, MSG_MAX, 1000, FALSE);

        OSTimeDly(1);

        if ( size < 0 )
            OSTimeDly(1);
    }

    FEND();
    OSTaskDel(OS_PRIO_SELF);
}

void MsgWasteTask2(void* p_arg)
{
    int size;
    unsigned char buf[MSG_MAX];

    FSTART();

    while ( 1 )
    {
        if ( OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ )
            break;

        size = MsgRecv(MSG_TYPE_LOG, buf, MSG_MAX, 1000, FALSE);

        OSTimeDly(1);

        if ( size < 0 )
            OSTimeDly(1);
    }

    FEND();
    OSTaskDel(OS_PRIO_SELF);
}

void MsgWasteTask3(void* p_arg)
{
    int size;
    unsigned char buf[MSG_MAX];

    FSTART();

    while ( 1 )
    {
        if ( OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ )
            break;

        size = MsgRecv(MSG_TYPE_DISPLAY, buf, MSG_MAX, 1000, FALSE);

        OSTimeDly(1);

        if ( size < 0 )
            OSTimeDly(1);
    }

    FEND();
    OSTaskDel(OS_PRIO_SELF);
}
#endif

void MsgFlush(int type)
{
    switch ( type )
    {
        case MSG_TYPE_DEBUG:
		    MsgRxDebugFront = MsgRxDebugRear = 0;
			break;
        case MSG_TYPE_DISPLAY:
		    MsgRxDispFront = MsgRxDispRear = 0;
			break;
        case MSG_TYPE_LOG:
		    MsgRxLogFront = MsgRxLogRear = 0;
			break;
	}
}

bool MsgServiceIsConnected(void)
{
    return MsgConnect;
}

bool MsgServiceIsEnabled(void)
{
    return (MsgConnect==TRUE && MsgProtocolInError==FALSE);
}

bool MsgServiceWaitConnected(int wait_tick)
{
    unsigned char err;
    if ( wait_tick != 0 )
        OSFlagPend(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_WAIT_SET_ALL, wait_tick<0 ? 0 : wait_tick, &err);
    return MsgConnect;
}

void MsgServiceStart(void)
{
    OS_TCB tcb;
    unsigned char ret1, ret2;

    ret1 = OSTaskQuery(TASK_PRIO_MSG_SEND, &tcb);
    ret2 = OSTaskQuery(TASK_PRIO_MSG_RECV, &tcb);

    if ( ret1 == OS_PRIO_ERR && ret2 == OS_PRIO_ERR )
    {
        DPUTS("Msg Service Start!!");

        OSTaskCreate(MsgSendTask,
                (void *)0,
                (OS_STK *)&TaskStk[0][TASK_STK_SIZE-1],
                TASK_PRIO_MSG_SEND);

        OSTaskCreate(MsgRecvTask,
                (void *)0,
                (OS_STK *)&TaskStk[1][TASK_STK_SIZE-1],
                TASK_PRIO_MSG_RECV);

#ifdef __CREATE_WASTE_TASK__
        OSTaskCreate(MsgWasteTask1,
                (void *)0,
                (OS_STK *)&TaskStk[2][TASK_STK_SIZE-1],
                TASK_PRIO_MSG_WASTE_1);

        OSTaskCreate(MsgWasteTask2,
                (void *)0,
                (OS_STK *)&TaskStk[3][TASK_STK_SIZE-1],
                TASK_PRIO_MSG_WASTE_2);

        OSTaskCreate(MsgWasteTask3,
                (void *)0,
                (OS_STK *)&TaskStk[4][TASK_STK_SIZE-1],
                TASK_PRIO_MSG_WASTE_3);
#endif
    }
}

void MsgServiceEnd(void)
{
    OS_TCB tcb;
    unsigned char ret1, ret2;

    printf("FUNCTION START: %s()\r\n", __FUNCTION__);

#ifdef __CREATE_WASTE_TASK__
    OSTaskDelReq(TASK_PRIO_MSG_WASTE_1);
    OSTaskDelReq(TASK_PRIO_MSG_WASTE_2);
    OSTaskDelReq(TASK_PRIO_MSG_WASTE_3);

    OSTimeDlyResume(TASK_PRIO_MSG_WASTE_1);
    OSTimeDlyResume(TASK_PRIO_MSG_WASTE_2);
    OSTimeDlyResume(TASK_PRIO_MSG_WASTE_3);
#endif

    OSTaskDelReq(TASK_PRIO_MSG_SEND);
    OSTaskDelReq(TASK_PRIO_MSG_RECV);

    OSTimeDlyResume(TASK_PRIO_MSG_SEND);
    OSTimeDlyResume(TASK_PRIO_MSG_RECV);

    do {
        ret1 = OSTaskQuery(TASK_PRIO_MSG_SEND, &tcb);
        ret2 = OSTaskQuery(TASK_PRIO_MSG_RECV, &tcb);
        OSTimeDly(1);
    } while ( ret1 != OS_PRIO_ERR || ret2 != OS_PRIO_ERR );

    printf("FUNCTION END: %s()\r\n", __FUNCTION__);
}

// 0이상: 실제로 수신한 바이트의 개수
// -1: 파라미터 에러
// -2: 연결되지 않음
// -3: 타임아웃
int MsgRecv(int type, void* recvbuf, int bufsize, int wait_tick, bool wait_full)
{
    unsigned char err;
    int rx_q_size, readsize;
    unsigned char* buf = (unsigned char*) recvbuf;

    if ( wait_tick != 0 )
        OSFlagPend(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_WAIT_SET_ALL, wait_tick<0 ? 0 : wait_tick, &err);

    if ( !MsgConnect )
        return -2;

    if ( buf==NULL || bufsize<=0 )
        return -1;

    switch ( type )
    {
        case MSG_TYPE_DEBUG:
            if ( wait_tick <= 0 )
            {
                if ( wait_full )
                {
                    if ( bufsize > CQ_USED(MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear) )
                        return 0;
                }
                else
                {
                    if ( CQ_ISEMPTY(MsgRxDebugFront, MsgRxDebugRear) )
                        return 0;
                }
            }
            else
            {
                if ( wait_full )
                {
                    while ( wait_tick-- )
                    {
                        if ( !MsgConnect )
                            return -2;
                        else if ( bufsize > CQ_USED(MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear) )
                            OSTimeDly(1);
                        else
                            break;
                    }

                    if ( wait_tick == 0 )
                        return 0;
                }
                else
                {
                    OSFlagPend(MsgFlag, MSG_FLAG_CONNECT|MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_WAIT_CLR_ANY, wait_tick, &err);
                    if ( !MsgConnect ) return -2;
                    else if ( err != OS_NO_ERR ) return 0;
                }
            }
            OSFlagPend(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE, OS_FLAG_WAIT_CLR_ALL, 100, &err);
            if ( err != OS_NO_ERR ) return -3;
            OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE, OS_FLAG_SET, &err);
            {
                for ( readsize=0 ; readsize<bufsize && CQ_ISNOTEMPTY(MsgRxDebugFront, MsgRxDebugRear) ; readsize++ )
                    CQ_GET(MsgRxDebugBuf, MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, buf[readsize]);

                rx_q_size = CQ_USED(MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear);

                if ( rx_q_size > MSG_RX_DEBUG_BUF_SIZE-MSG_MAX-1 ) // full
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL, OS_FLAG_SET, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_CLR, &err);
                }
                else if ( rx_q_size == 0 ) // empty
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL, OS_FLAG_CLR, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_SET, &err);
                }
                else
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL|MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_CLR, &err);
                }
            }
            OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE, OS_FLAG_CLR, &err);
            if ( readsize > 0 ) DPRINTF(("\t<<--: DEBUG size:%i", readsize));
            break;

        case MSG_TYPE_LOG:
            if ( wait_tick <= 0 )
            {
                if ( wait_full )
                {
                    if ( bufsize > CQ_USED(MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear) )
                        return 0;
                }
                else
                {
                    if ( CQ_ISEMPTY(MsgRxLogFront, MsgRxLogRear) )
                        return 0;
                }
            }
            else
            {
                if ( wait_full )
                {
                    while ( wait_tick-- )
                    {
                        if ( !MsgConnect )
                            return -2;
                        else if ( bufsize > CQ_USED(MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear) )
                            OSTimeDly(1);
                        else
                            break;
                    }

                    if ( wait_tick == 0 )
                        return 0;
                }
                else
                {
                    OSFlagPend(MsgFlag, MSG_FLAG_CONNECT|MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_WAIT_CLR_ANY, wait_tick, &err);
                    if ( !MsgConnect ) return -2;
                    else if ( err != OS_NO_ERR ) return 0;
                }
            }
            OSFlagPend(MsgFlag, MSG_FLAG_RX_LOG_IN_USE, OS_FLAG_WAIT_CLR_ALL, 100, &err);
            if ( err != OS_NO_ERR ) return -3;
            OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IN_USE, OS_FLAG_SET, &err);
            {
                for ( readsize=0 ; readsize<bufsize && CQ_ISNOTEMPTY(MsgRxLogFront, MsgRxLogRear) ; readsize++ )
                    CQ_GET(MsgRxLogBuf, MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, buf[readsize]);

                rx_q_size = CQ_USED(MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear);

                if ( rx_q_size > MSG_RX_LOG_BUF_SIZE-MSG_MAX-1 ) // full
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL, OS_FLAG_SET, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_CLR, &err);
                }
                else if ( rx_q_size == 0 ) // empty
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL, OS_FLAG_CLR, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_SET, &err);
                }
                else
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL|MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_CLR, &err);
                }
            }
            OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IN_USE, OS_FLAG_CLR, &err);
            if ( readsize > 0 ) DPRINTF(("\t<<--: LOG size:%i", readsize));
            break;

        case MSG_TYPE_DISPLAY:
            if ( wait_tick <= 0 )
            {
                if ( wait_full )
                {
                    if ( bufsize > CQ_USED(MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear) )
                        return 0;
                }
                else
                {
                    if ( CQ_ISEMPTY(MsgRxDispFront, MsgRxDispRear) )
                        return 0;
                }
            }
            else
            {
                if ( wait_full )
                {
                    while ( wait_tick-- )
                    {
                        if ( !MsgConnect )
                            return -2;
                        else if ( bufsize > CQ_USED(MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear) )
                            OSTimeDly(1);
                        else
                            break;
                    }

                    if ( wait_tick == 0 )
                        return 0;
                }
                else
                {
                    OSFlagPend(MsgFlag, MSG_FLAG_CONNECT|MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_WAIT_CLR_ANY, wait_tick, &err);
                    if ( !MsgConnect ) return -2;
                    else if ( err != OS_NO_ERR ) return 0;
                }
            }
            OSFlagPend(MsgFlag, MSG_FLAG_RX_DISP_IN_USE, OS_FLAG_WAIT_CLR_ALL, 100, &err);
            if ( err != OS_NO_ERR ) return -3;
            OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IN_USE, OS_FLAG_SET, &err);
            {
                for ( readsize=0 ; readsize<bufsize && CQ_ISNOTEMPTY(MsgRxDispFront, MsgRxDispRear) ; readsize++ )
                    CQ_GET(MsgRxDispBuf, MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, buf[readsize]);

                rx_q_size = CQ_USED(MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear);

                if ( rx_q_size > MSG_RX_DISP_BUF_SIZE-MSG_MAX-1 ) // full
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL, OS_FLAG_SET, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_CLR, &err);
                }
                else if ( rx_q_size == 0 ) // empty
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL, OS_FLAG_CLR, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_SET, &err);
                }
                else
                {
                    OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL|MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_CLR, &err);
                }
            }
            OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IN_USE, OS_FLAG_CLR, &err);
            if ( readsize > 0 ) DPRINTF(("\t<<--: DISPLAY size:%i", readsize));
            break;

        default:
            return -1;
    }

    return readsize;
}

// 0: 성공
// -1: 파라미터 에러
// -2: 연결되지 않음
// -3: 타임아웃
int MsgSend(int type, void* sendbuf, int bufsize)
{
    unsigned char err;

    if ( type <= MSG_TYPE_CONTROL )
    {
        struct CtrlMessage ctrl;

        ctrl.type_and_size = type;
        ctrl.seq = bufsize;
        ctrl.crc8 = crc8(&ctrl.type_and_size, 2);

        // 연결이 불안한 경우 재전송을 0.5초 간격으로 12번 하기 때문에 최소 6초 이상 기다려준다.
        // 처리시간 및 응답이 오기까지의 시간을 고려하여 10초로 한다.
        OSFlagPend(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE|MSG_FLAG_TX_CTRL_Q_IS_FULL, OS_FLAG_WAIT_CLR_ALL, 1000, &err);
        if ( err != OS_NO_ERR ) RETURN(-3);

        if ( CQ_ISFULL(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear) )
            RETURN(-4);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_SET, &err);

        CQ_PUT(MsgTxCtrlQueue, MSG_TX_CTRL_Q_SIZE, MsgTxCtrlRear, ctrl);

        int tx_q_size = CQ_USED(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear);
        if ( tx_q_size == MSG_TX_CTRL_Q_SIZE-1 )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_FULL, OS_FLAG_SET, &err);
        else if ( tx_q_size == 1 )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_EMPTY, OS_FLAG_CLR, &err);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_CLR, &err);
    }
    else
    {
        struct DataMessage temp;

        if ( !MsgConnect )
            RETURN(-2);

        if ( (bufsize <= 0) || bufsize>MSG_MAX )
            RETURN(-1);

        temp.type_and_size = type | bufsize;
        temp.seq = MsgTxRear;
        if ( sendbuf ) memcpy(temp.data, sendbuf, bufsize);
        temp.crc8 = crc8(&temp.type_and_size, bufsize+2);

        // 연결이 불안한 경우 재전송을 0.5초 간격으로 12번 하기 때문에 최소 6초 이상 기다려준다.
        // 처리시간 및 응답이 오기까지의 시간을 고려하여 10초로 한다.
        OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE|MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_WAIT_CLR_ALL, 1000, &err);
        if ( !MsgConnect ) RETURN(-2);
        else if ( err != OS_NO_ERR ) RETURN(-3);

        if ( CQ_ISFULL(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear) )
            RETURN(-4);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_SET, &err);

        CQ_PUT(MsgTxQueue, MSG_TX_DATA_Q_SIZE, MsgTxRear, temp);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);

        if ( CQ_ISFULL(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear) )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_SET, &err);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_CLR, &err);
    }

    return 0;
}

int MsgSendNoWait(int type, void* sendbuf, int bufsize)
{
    unsigned char err;

    if ( type <= MSG_TYPE_CONTROL )
    {
        struct CtrlMessage ctrl;

        ctrl.type_and_size = type;
        ctrl.seq = bufsize;
        ctrl.crc8 = crc8(&ctrl.type_and_size, 2);

        OSFlagPend(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 100, &err);
        if ( err != OS_NO_ERR ) RETURN(-3);

        if ( CQ_ISFULL(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear) )
            RETURN(-4);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_SET, &err);

        CQ_PUT(MsgTxCtrlQueue, MSG_TX_CTRL_Q_SIZE, MsgTxCtrlRear, ctrl);

        int tx_q_size = CQ_USED(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear);
        if ( tx_q_size == MSG_TX_CTRL_Q_SIZE-1 )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_FULL, OS_FLAG_SET, &err);
        else if ( tx_q_size == 1 )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_EMPTY, OS_FLAG_CLR, &err);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_CLR, &err);
    }
    else
    {
        struct DataMessage temp;

        if ( !MsgConnect )
            RETURN(-2);

        if ( (bufsize <= 0) || bufsize>MSG_MAX )
            RETURN(-1);

        temp.type_and_size = type | bufsize;
        temp.seq = MsgTxRear;
        if ( sendbuf ) memcpy(temp.data, sendbuf, bufsize);
        temp.crc8 = crc8(&temp.type_and_size, bufsize+2);

        OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 100, &err);
        if ( !MsgConnect ) RETURN(-2);
        else if ( err != OS_NO_ERR ) RETURN(-3);

        if ( CQ_ISFULL(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear) )
            RETURN(-4);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_SET, &err);

        CQ_PUT(MsgTxQueue, MSG_TX_DATA_Q_SIZE, MsgTxRear, temp);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);

        if ( CQ_ISFULL(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear) )
            OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_SET, &err);

        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_CLR, &err);
    }

    return 0;
}

void MsgSendNAK(int last_recv_seq, int occur_step)
{
	DPRINTF(("MsgSendNAK(%i) at step %i", last_recv_seq, occur_step));

    if ( MsgConnect )
    {
        if ( last_recv_seq >= 0 )
            MsgSend(MSG_TYPE_NAK, NULL, (last_recv_seq+1)%MSG_TX_DATA_Q_SIZE);
        else
            MsgSend(MSG_TYPE_NAK, NULL, MSG_TX_DATA_Q_SIZE_MAX);
    }
}

void MsgInit(void)
{
    unsigned char err;

    MsgConnect = FALSE;
    MsgProtocolInError = FALSE;
    MsgTxPosition = MsgTxFront = MsgTxRear = 0;
    MsgRxDispFront = MsgRxDispRear = MsgRxLogFront = MsgRxLogRear = MsgRxDebugFront = MsgRxDebugRear = 0;
    MsgTxCtrlFront = MsgTxCtrlRear = 0;

    if ( !MsgFlag )
    {
        MsgFlag = OSFlagCreate(MSG_FLAG_TX_CTRL_Q_IS_EMPTY|MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS|MSG_FLAG_RX_DISP_IS_EMPTY|MSG_FLAG_RX_LOG_IS_EMPTY|MSG_FLAG_RX_DEBUG_IS_EMPTY, &err);
    }
    else
    {
        OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_EMPTY|MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS|MSG_FLAG_RX_DISP_IS_EMPTY|MSG_FLAG_RX_LOG_IS_EMPTY|MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_SET, &err);
        OSFlagPost(MsgFlag, ~(MSG_FLAG_TX_CTRL_Q_IS_EMPTY|MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS|MSG_FLAG_RX_DISP_IS_EMPTY|MSG_FLAG_RX_LOG_IS_EMPTY|MSG_FLAG_RX_DEBUG_IS_EMPTY), OS_FLAG_CLR, &err);
    }
}

void MsgRecvTask(void* p_arg)
{
    int ch;
    unsigned char err;
    unsigned char msg_type, msg_size;
    unsigned char buf[MSG_MAX+6];
    unsigned char buf_len = 0;
    bool wait_esc_next_char = FALSE;
    int last_recv_seq = -1;
    enum { OCCUR_ERROR, WAIT_SOX, FIND_SOX_WAIT_TYPE, RECV_DATA_AND_CRC, CRC_OK_WAIT_EOX } msg_state = WAIT_SOX;

    FSTART();

    while ( 1 )
    {
        OSTimeDly(1);

        if ( OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ )
            break;

#if __DEFAULT_DEBUG_PORT__ == 2
        OSFlagPend(gSerialFlag, SCI1_FLAG_RX_READY, OS_FLAG_WAIT_SET_ALL, 100, &err);
#else
        OSFlagPend(gSerialFlag, SCI2_FLAG_RX_READY, OS_FLAG_WAIT_SET_ALL, 100, &err);
#endif

        // find packet
        while ( (ch = SCI_Getc(SCI_PORT, 0)) != EOF )
        {
            //WC(ch);

            if ( ch == SOX )
            {
                if ( msg_state==FIND_SOX_WAIT_TYPE || (msg_state>FIND_SOX_WAIT_TYPE && msg_type>MSG_TYPE_CONTROL) )
                    MsgSendNAK(last_recv_seq, 1);

                buf[0] = ch;
                buf_len = 1;
                msg_state = FIND_SOX_WAIT_TYPE;
                continue;
            }

            switch ( msg_state )
            {
                case OCCUR_ERROR:
                    break;

                case WAIT_SOX:
                    if ( ch != SOX )
                    {
                        msg_state = OCCUR_ERROR;
                        MsgSendNAK(last_recv_seq, 2);
                    }
                    break;

                case FIND_SOX_WAIT_TYPE:
                    msg_type = GetMsgType(ch);
                    msg_size = GetMsgSize(ch);
                    if ( (msg_type>MSG_TYPE_CONTROL && msg_size>0) || (msg_type<=MSG_TYPE_CONTROL && msg_size==0) )
                    {
                        buf[buf_len++] = ch;
                        msg_state = RECV_DATA_AND_CRC;
                        wait_esc_next_char = FALSE;
                    }
                    else
                    {
                        msg_state = OCCUR_ERROR;
                        MsgSendNAK(last_recv_seq, 3);
                    }
                    break;

                case RECV_DATA_AND_CRC:
                    if ( ch == EOX )
                    {
                        msg_state = OCCUR_ERROR;
                        if ( msg_type > MSG_TYPE_CONTROL ) MsgSendNAK(last_recv_seq, 4);
                        break;
                    }
                    else if ( ch == ESC )
                    {
                        wait_esc_next_char = TRUE;
                        break;
                    }
                    else if ( wait_esc_next_char )
                    {
                        ch ^= 0x80;
                    }

                    buf[buf_len++] = ch;
                    wait_esc_next_char = FALSE;

                    if ( buf_len==3 && ch>=MSG_TX_DATA_Q_SIZE ) // sox, cmd, seq까지 수신한 상태
                    {
                        msg_state = OCCUR_ERROR;
                        if ( msg_type > MSG_TYPE_CONTROL ) MsgSendNAK(last_recv_seq, 5);
                        break;
                    }

                    if ( buf_len == msg_size+4 ) // sox, cmd, seq, buf, crc까지 수신한 상태
                    {
                        if ( buf[buf_len-1] != crc8(buf+1, msg_size+2) )
                        {
                            msg_state = OCCUR_ERROR;
                            // 바로 직전 것이 다시 날라온 경우 NAK를 보내지 않는다.
                            // 이것에 대해 NAK를 보내는 경우 NAK가 무한반복된다.
                            if ( msg_type>MSG_TYPE_CONTROL && buf[2]!=last_recv_seq )
                            {
                                MsgSendNAK(last_recv_seq, 6);
								for ( int i=0 ; i<buf_len ; i++ )
								{
									DPRINTF(("buf[%i]: '%c' (0x%02x)\r\n", i, buf[i]<=127 ? buf[i] : ' ', buf[i]));
								}
								WI(crc8(buf+1, msg_size+2));
                            }
                        }
                        else if (msg_type>MSG_TYPE_CONTROL && last_recv_seq>=0 && buf[2]!=(last_recv_seq+1)%MSG_TX_DATA_Q_SIZE)
                        {
                            msg_state = OCCUR_ERROR;
                            // 바로 직전 것이 다시 날라온 경우 NAK를 보내지 않는다.
                            // 이것에 대해 NAK를 보내는 경우 NAK가 무한반복된다.
                            if ( msg_type>MSG_TYPE_CONTROL && buf[2]!=last_recv_seq )
                            {
                                MsgSendNAK(last_recv_seq, 7);
                                WI(buf[2]);
                            }
                        }
                        else
                        {
                            msg_state = CRC_OK_WAIT_EOX;
                        }
                    }
                    break;

                case CRC_OK_WAIT_EOX:
                    if ( ch == EOX )
                    {
                        msg_state = WAIT_SOX;
                        if ( MsgConnect )
                        {
                            if ( msg_type > MSG_TYPE_CONTROL )
                            {
                                last_recv_seq = buf[2];
                                MsgSend(MSG_TYPE_ACK, NULL, last_recv_seq);
                            }
                            goto process_packet;
                        }
                        else if ( msg_type==MSG_TYPE_CONN || msg_type==MSG_TYPE_REQ_CONN ) // Connect 상태가 아닌 경우 Connect 관련 메시지 이외에는 무시한다.
                        {
                            goto process_packet;
                        }
                    }
                    else
                    {
                        msg_state = OCCUR_ERROR;
                        if ( msg_type > MSG_TYPE_CONTROL ) MsgSendNAK(last_recv_seq, 8);
                    }
                    break;
            }
        }

        continue;

process_packet:
        switch ( msg_type )
        {
            case MSG_TYPE_REQ_CONN:
                DPUTS("<<--: REQ_CONN");
                MsgConnect = FALSE;
                OSFlagPost(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_CLR, &err);
                MsgSend(MSG_TYPE_CONN, NULL, 0);
                MsgSend(MSG_TYPE_CONN, NULL, 0);
                break;
            case MSG_TYPE_CONN:
                DPUTS("<<--: CONN");
                if ( MsgConnect == FALSE )
                {
                    last_recv_seq = -1;
                    MsgInit();
                    MsgSend(MSG_TYPE_CONN, NULL, 0);
                    MsgSend(MSG_TYPE_CONN, NULL, 0);
                    MsgConnect = TRUE;
                    OSFlagPost(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_SET, &err);
                }
                break;
            case MSG_TYPE_NAK:
                {
                    OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_SET, &err);
                    if ( buf[2] < MSG_TX_DATA_Q_SIZE )
                    {
                        // 현재 전송큐가 가지고 있는 시퀀스 범위 내라면
                        if ( (MsgTxFront<MsgTxRear && (buf[2]>=MsgTxFront && buf[2]<MsgTxRear)) ||
                             (MsgTxFront>MsgTxRear && (buf[2]>=MsgTxFront || buf[2]<MsgTxRear)) )
                        {
                            // NAK가 왔다는 것은 거부된 바로 직전 시퀀스까지는 송신성공된 것으로 판단하고 ACK처리를 한다.
                            unsigned char guess_ack_seq = buf[2]==0 ? MSG_TX_DATA_Q_SIZE-1 : buf[2]-1;
                            if ( (MsgTxFront<MsgTxRear && (guess_ack_seq>=MsgTxFront && guess_ack_seq<MsgTxRear)) ||
                                 (MsgTxFront>MsgTxRear && (guess_ack_seq>=MsgTxFront || guess_ack_seq<MsgTxRear)) )
                            {
                                MsgTxFront = buf[2];
                            }

                            // NAK된 송신시퀀스부터 재전송한다.
                            MsgTxPosition = buf[2];
                        }
                        // 시퀀스가 범위 밖이고 전송큐에 비지 않았다면
                        else if ( MsgTxFront != MsgTxRear )
                        {
                            DPRINTF(("NAK out of seq (recv_seq:%i front:%i rear:%i)", buf[2], MsgTxFront, MsgTxRear));

                            MsgConnect = FALSE;
                            OSFlagPost(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_CLR, &err);
                            MsgSend(MSG_TYPE_REQ_CONN, NULL, 0);
                        }
                    }
                    else if ( MsgTxFront != MsgTxRear )
                    {
                        MsgTxPosition = MsgTxFront;
                    }

                    int tx_q_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear);
                    int not_tx_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxPosition, MsgTxRear);

                    if ( not_tx_size == 0 )
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_SET, &err);
                    else
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);

                    if ( tx_q_size == MSG_TX_DATA_Q_SIZE-1 )
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_SET, &err);
                    else
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_CLR, &err);

                    OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_CLR, &err);
                }
                DPRINTF(("\t\t\t\t\t\t<<--: NAK seq:%i, q_use:%i/%i", buf[2], CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear), MSG_TX_DATA_Q_SIZE-1));
                break;
            case MSG_TYPE_ACK:
                MsgProtocolInError = FALSE;
                OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_SET, &err);
                if ( (MsgTxFront<MsgTxRear && (buf[2]>=MsgTxFront && buf[2]<MsgTxRear)) ||
                     (MsgTxFront>MsgTxRear && (buf[2]>=MsgTxFront || buf[2]<MsgTxRear)) )
                {
                    MsgTxFront = (buf[2]+1) % MSG_TX_DATA_Q_SIZE;

                    // MsgTxPosition이 시퀀스 범위 밖에 남아있는 경우 처리한다.
                    if ( (MsgTxFront<=MsgTxRear && (MsgTxPosition<MsgTxFront || MsgTxPosition>MsgTxRear)) ||
                         (MsgTxFront>MsgTxRear  && (MsgTxPosition<MsgTxFront && MsgTxPosition>MsgTxRear)) )
                        MsgTxPosition = MsgTxFront;

                    int tx_q_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear);
                    int not_tx_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxPosition, MsgTxRear);

                    if ( not_tx_size == 0 )
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_SET, &err);
                    else
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);

                    if ( tx_q_size == MSG_TX_DATA_Q_SIZE-1 )
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_SET, &err);
                    else
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IS_FULL, OS_FLAG_CLR, &err);
                }
                OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_CLR, &err);
                DPRINTF(("\t\t\t\t\t\t<<--: ACK seq:%i, q_use:%i/%i", buf[2], CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear), MSG_TX_DATA_Q_SIZE-1));
                break;
            case MSG_TYPE_DISPLAY:
                OSFlagPend(MsgFlag, MSG_FLAG_RX_DISP_IN_USE|MSG_FLAG_RX_DISP_IS_FULL, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IN_USE, OS_FLAG_SET, &err);
                {
					if ( CQ_FREE(MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear) < msg_size )
					{
						ERROR("MSG_RX_DISP_BUF overflow");
						MsgRxDispFront = MsgRxDispRear = 0;
					}

                    CQ_PUTBUF(MsgRxDispBuf, MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear, &buf[3], msg_size);

                    int rx_q_size = CQ_USED(MSG_RX_DISP_BUF_SIZE, MsgRxDispFront, MsgRxDispRear);

                    if ( rx_q_size > MSG_RX_DISP_BUF_SIZE-MSG_MAX-1 ) // full
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL, OS_FLAG_SET, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                    else if ( rx_q_size == 0 ) // empty
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL, OS_FLAG_CLR, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_SET, &err);
                    }
                    else
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IS_FULL|MSG_FLAG_RX_DISP_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                }
                OSFlagPost(MsgFlag, MSG_FLAG_RX_DISP_IN_USE, OS_FLAG_CLR, &err);
                DPRINTF(("<<--: DISPLAY q_use:%i/%i", CQ_USED(MSG_RX_DISP_BUF_SIZE,MsgRxDispFront,MsgRxDispRear), MSG_RX_DISP_BUF_SIZE));
                break;
            case MSG_TYPE_LOG:
                OSFlagPend(MsgFlag, MSG_FLAG_RX_LOG_IN_USE|MSG_FLAG_RX_LOG_IS_FULL, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IN_USE, OS_FLAG_SET, &err);
                {
					if ( CQ_FREE(MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear) < msg_size )
					{
						ERROR("MSG_RX_LOG_BUF overflow");
						MsgRxLogFront = MsgRxLogRear = 0;
					}

                    CQ_PUTBUF(MsgRxLogBuf, MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear, &buf[3], msg_size);

                    int rx_q_size = CQ_USED(MSG_RX_LOG_BUF_SIZE, MsgRxLogFront, MsgRxLogRear);

                    if ( rx_q_size > MSG_RX_LOG_BUF_SIZE-MSG_MAX-1 ) // full
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL, OS_FLAG_SET, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                    else if ( rx_q_size == 0 ) // empty
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL, OS_FLAG_CLR, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_SET, &err);
                    }
                    else
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IS_FULL|MSG_FLAG_RX_LOG_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                }
                OSFlagPost(MsgFlag, MSG_FLAG_RX_LOG_IN_USE, OS_FLAG_CLR, &err);
                DPRINTF(("<<--: LOG q_use:%i/%i", CQ_USED(MSG_RX_LOG_BUF_SIZE,MsgRxLogFront,MsgRxLogRear), MSG_RX_LOG_BUF_SIZE));
                break;
            case MSG_TYPE_DEBUG:
                OSFlagPend(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE|MSG_FLAG_RX_DEBUG_IS_FULL, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE, OS_FLAG_SET, &err);
                {
					if ( CQ_FREE(MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear) < msg_size )
					{
						ERROR("MSG_RX_DEBUG_BUF overflow");
						MsgRxDebugFront = MsgRxDebugRear = 0;
					}

                    CQ_PUTBUF(MsgRxDebugBuf, MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear, &buf[3], msg_size);

                    int rx_q_size = CQ_USED(MSG_RX_DEBUG_BUF_SIZE, MsgRxDebugFront, MsgRxDebugRear);

                    if ( rx_q_size > MSG_RX_DEBUG_BUF_SIZE-MSG_MAX-1 ) // full
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL, OS_FLAG_SET, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                    else if ( rx_q_size == 0 ) // empty
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL, OS_FLAG_CLR, &err);
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_SET, &err);
                    }
                    else
                    {
                        OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IS_FULL|MSG_FLAG_RX_DEBUG_IS_EMPTY, OS_FLAG_CLR, &err);
                    }
                }
                OSFlagPost(MsgFlag, MSG_FLAG_RX_DEBUG_IN_USE, OS_FLAG_CLR, &err);
                DPRINTF(("<<--: DEBUG q_use:%i/%i", CQ_USED(MSG_RX_DEBUG_BUF_SIZE,MsgRxDebugFront,MsgRxDebugRear), MSG_RX_DEBUG_BUF_SIZE-1));
                break;
            default:
				DPRINTF(("<<--: UNKNOWN(0x%x) size:%i", msg_type, msg_size));
				break;
        }
    }

    printf("FUNCTION END: %s()\r\n", __FUNCTION__);
    //FEND();
    OSTaskDel(OS_PRIO_SELF);
}

// MsgSendTask는 MsgRecvTask보다 우선순위가 높아야 한다.
void MsgSendTask(void* p_arg)
{
    unsigned char err;
    int retrans_cnt = 0;
    unsigned int last_send_time = 0;
    unsigned int last_req_conn_time = 0;
    int not_tx_size;
	unsigned long write_cnt;
	unsigned char write_buf[100];

	#define SCI_TxByte(ch)              { write_buf[write_cnt++] = ch; }
	#define HDLC_TxByte(ch)             if ( (ch)==SOX || (ch)==EOX || (ch)==ESC ) { write_buf[write_cnt++] = ESC; write_buf[write_cnt++] = (ch)^0x80; } else { write_buf[write_cnt++] = ch; }

    FSTART();

    MsgInit();
    MsgSend(MSG_TYPE_REQ_CONN, NULL, 0);

    while ( 1 )
    {
        OSTimeDly(1);

        if ( OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ )
            break;

        OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS|MSG_FLAG_TX_CTRL_Q_IS_EMPTY, OS_FLAG_WAIT_CLR_ANY, 100, &err);
#if __DEFAULT_DEBUG_PORT__ == 2
        OSFlagPend(gSerialFlag, SCI1_FLAG_TX_Q_NEARLY_EMPTY, OS_FLAG_WAIT_SET_ALL, 0, &err);
#else
        OSFlagPend(gSerialFlag, SCI2_FLAG_TX_Q_NEARLY_EMPTY, OS_FLAG_WAIT_SET_ALL, 0, &err);
#endif
        if ( MsgTxCtrlFront != MsgTxCtrlRear )
        {
            OSFlagPend(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 0, &err);
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE, OS_FLAG_SET, &err);
            while ( MsgTxCtrlFront != MsgTxCtrlRear )
            {
                struct CtrlMessage ctrl_msg = MsgTxCtrlQueue[MsgTxCtrlFront];
                MsgTxCtrlFront = (MsgTxCtrlFront+1) % MSG_TX_CTRL_Q_SIZE;

                switch ( ctrl_msg.type_and_size )
                {
                    case MSG_TYPE_ACK:
                        DPRINTF(("-->>: ACK seq:%i, q_use:%i/%i", ctrl_msg.seq, CQ_USED(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear), MSG_TX_DATA_Q_SIZE-1));
                        break;
                    case MSG_TYPE_NAK:
                        DPRINTF(("-->>: NAK seq:%i, q_use:%i/%i", ctrl_msg.seq, CQ_USED(MSG_TX_CTRL_Q_SIZE, MsgTxCtrlFront, MsgTxCtrlRear), MSG_TX_DATA_Q_SIZE-1));
                        break;
                    case MSG_TYPE_REQ_CONN:
                        DPRINTF(("-->>: REQ_CONN"));
                        break;
                    case MSG_TYPE_CONN:
                        DPRINTF(("-->>: CONN"));
                        break;
                }

                if ( ctrl_msg.type_and_size==MSG_TYPE_REQ_CONN || ctrl_msg.type_and_size==MSG_TYPE_CONN )
                    last_req_conn_time = OSTimeGet();

                write_cnt = 0;
                SCI_TxByte(SOX);
                SCI_TxByte(ctrl_msg.type_and_size);
                SCI_TxByte(ctrl_msg.seq);
                HDLC_TxByte(ctrl_msg.crc8);
                SCI_TxByte(EOX);

                SCI_Putd(SCI_PORT, write_buf, write_cnt);
            }
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IS_EMPTY, OS_FLAG_SET, &err);
            OSFlagPost(MsgFlag, MSG_FLAG_TX_CTRL_Q_IN_USE|MSG_FLAG_TX_CTRL_Q_IS_FULL, OS_FLAG_CLR, &err);
        }

        if ( MsgConnect )
        {
            int tx_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxPosition);

            if ( MsgTxPosition!=MsgTxRear && tx_size<10 )
            {
                struct DataMessage data_msg;

                // 세마포어에 의해 보호되는 시간을 최대한 짧게 한다.
                {
                    OSFlagPend(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_WAIT_CLR_ALL, 0, &err);
                    OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_SET, &err);

                    data_msg = MsgTxQueue[MsgTxPosition];
                    MsgTxPosition = (MsgTxPosition+1) % MSG_TX_DATA_Q_SIZE;

                    not_tx_size = CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxPosition, MsgTxRear);

                    if ( not_tx_size == 0 )
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_SET, &err);
                    /*
                    else
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);
                    */

                    OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_IN_USE, OS_FLAG_CLR, &err);
                }

                // 실제 전송
                {
                    unsigned char size = GetMsgSize(data_msg.type_and_size);

                    DPRINTF(("\t\t\t\t\t\t-->>: DEBUG id:%i, seq:%i, not_tx:%i, q_use:%i/%i", data_msg.data[0], data_msg.seq, not_tx_size, CQ_USED(MSG_TX_DATA_Q_SIZE, MsgTxFront, MsgTxRear), MSG_TX_DATA_Q_SIZE-1));

                    write_cnt = 0;
                    SCI_TxByte(SOX);
                    SCI_TxByte(data_msg.type_and_size);
                    SCI_TxByte(data_msg.seq);
                    for ( int i=0 ; i<size ; i++ )
                        HDLC_TxByte(data_msg.data[i]);
                    HDLC_TxByte(data_msg.crc8);
                    SCI_TxByte(EOX);

                    SCI_Putd(SCI_PORT, write_buf, write_cnt);
                    last_send_time = OSTimeGet();
                }
            }
            else if ( MsgTxFront != MsgTxRear ) // 모든 데이터를 전송했으나 ACK를 못받은 상황
            {
                unsigned int cur_time = OSTimeGet();
                unsigned int time_diff = cur_time>=last_send_time ? cur_time-last_send_time : 0xffffffffUL-last_send_time+cur_time+1;

                if ( last_send_time>0 && time_diff>=50 ) // 0.5초 이상 대기
                {
                    if ( retrans_cnt++ < 12 ) // 12번 재시도한다
                    {
                        DPRINTF(("\t\t\t\t\t\t-->>: START RETRANS: %i", retrans_cnt));
                        MsgProtocolInError = TRUE;
                        if ( MsgTxPosition != MsgTxFront )
                            MsgTxPosition = MsgTxPosition>0 ? MsgTxPosition-1 : MSG_TX_DATA_Q_SIZE-1;
                        OSFlagPost(MsgFlag, MSG_FLAG_TX_DATA_Q_HAVE_NO_TRANS, OS_FLAG_CLR, &err);
                    }
                    else // 연결이 끊어진 것으로 간주
                    {
                        MsgConnect = FALSE;
                        OSFlagPost(MsgFlag, MSG_FLAG_CONNECT, OS_FLAG_CLR, &err);
                        MsgSend(MSG_TYPE_REQ_CONN, NULL, 0);
                    }
                    continue;
                }
            }
            else
            {
                if ( retrans_cnt > 0 )
                {
                    DPRINTF(("\t\t\t\t\t\t-->>: RESET RETRANS COUNT"));
                    retrans_cnt = 0;
                }
            }
        }
        else // if ( MsgConnect )
             // REQ_CONN을 1초마다 반복해서 보낸다.
        {
            unsigned int cur_time = OSTimeGet();
            unsigned int time_diff = cur_time>=last_req_conn_time ? cur_time-last_req_conn_time : 0xffffffffUL-last_req_conn_time+cur_time+1;

            if ( last_req_conn_time>0 && time_diff>=500 )
                MsgSend(MSG_TYPE_REQ_CONN, NULL, 0);
        }
    }

    MsgInit();
    printf("FUNCTION END: %s()\r\n", __FUNCTION__);
    //FEND();
    OSTaskDel(OS_PRIO_SELF);
}
