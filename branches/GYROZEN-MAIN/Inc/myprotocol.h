#ifndef __MYPROTOCOL_H__
#define __MYPROTOCOL_H__

#define __DATA1_STREAM__
#define __DATA2_STREAM__
#define __DATA3_STREAM__

#define MSG_TYPE_DATA1      0x20
#define MSG_TYPE_DATA2      0x40
#define MSG_TYPE_DATA3      0x60

#define MSG_TYPE_DEBUG      MSG_TYPE_DATA1
#define MSG_TYPE_LOG        MSG_TYPE_DATA2
#define MSG_TYPE_DISPLAY    MSG_TYPE_DATA3

#define DATA_MSG_MAX_LEN    31

struct RxMsg
{
    unsigned char data[DATA_MSG_MAX_LEN];
};

void MsgInit(void);
    bool MsgServiceIsConnected(void);

void MsgRecvHandler(void);
void MsgSendIrqHandler(void);
    void MsgSendHandler(void);

int MsgSend(int type, void* sendbuf, int bufsize);
int MsgRecv(int data_channel, void* recvbuf, int bufsize);
struct RxMsg* MsgRecvPacket(int data_channel);

#endif

