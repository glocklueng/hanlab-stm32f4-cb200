#ifndef __ROTOR_COMM_H__
#define __ROTOR_COMM_H__

struct ROTOR_INTERFACE
{
    unsigned char product_type;
    unsigned char version;
    struct GUID guid;
    int weight[3];
    int default_pos[3];

    int move_result[3];

    int result;
    enum { ROTOR_READY=0, ROTOR_FINISHED=0, ROTOR_POWERCHECK, ROTOR_QUERY, ROTOR_MOVE, ROTOR_POWERCHECK_FOR_RETRY, ROTOR_MOVE_RETRY, ROTOR_MOVE_RESULT } status;
};

extern struct ROTOR_INTERFACE gRotor;
extern int gRotorResult[3];

void RB_Process(void);
void RB_CanRecvHandler(CanRxMsgTypeDef* msg);
void RB_PowerOnAndCheck(int retry_cnt);
void RB_Move(int retry_cnt, int bal1_mm, int bal2_mm, int bal3_mm);
void RB_SetWeightDefaultPosition(int balancer1_mm, int balancer2_mm, int balancer3_mm);
void RB_SetBalancerWeight(int balancer1_gram, int balancer2_gram, int balancer3_gram);
void RB_GetInformation(void);
void RB_Cancel(void);

#endif
