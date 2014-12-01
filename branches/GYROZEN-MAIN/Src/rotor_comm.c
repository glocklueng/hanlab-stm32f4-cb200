/*
    0x640 파라미터 전송
    0x641 명령 전송
    0x642 명령 수행 결과
    0x643 스텝 모터 파워 변경 (결과 없음)
    0x644 무게추 기본 위치 저장 (결과 0x644)
    0x645 무게추 기본 위치 확인 (결과 0x645)
    0x646 DIAG 결과
    0x647 GUID 확인 1 (결과 0x647)
    0x648 GUID 확인 2 (결과 0x648)
    0x649 무게추 무게 저장 (결과 0x649)
    0x64A 무게추 무게 확인 (결과 0x64A)
    0x64B 제품타입 및 버젼 확인 (결과 0x64B)
    0x64F 자동응답
    0x650 디버그 데이터
*/

#include "main.h"
#include "rotor_comm.h"

#define RB_CMD_BAL1_INIT_SENSOR             0x8
#define RB_CMD_BAL1_RETURN_ORG              0x4
#define RB_CMD_BAL1_MOVE                    0x2
#define RB_CMD_BAL1_MEASURE_RANGE           0x1
#define RB_CMD_BAL1_RET_ORG_MOVE            0x6

#define RB_CMD_BAL2_INIT_SENSOR             0x800
#define RB_CMD_BAL2_RETURN_ORG              0x400
#define RB_CMD_BAL2_MOVE                    0x200
#define RB_CMD_BAL2_MEASURE_RANGE           0x100
#define RB_CMD_BAL2_RET_ORG_MOVE            0x600

#define RB_CMD_BAL3_INIT_SENSOR             0x80000
#define RB_CMD_BAL3_RETURN_ORG              0x40000
#define RB_CMD_BAL3_MOVE                    0x20000
#define RB_CMD_BAL3_MEASURE_RANGE           0x10000
#define RB_CMD_BAL3_RET_ORG_MOVE            0x60000

#define RB_CMD_GET_STATUS                   0x1000000
#define RB_CMD_DIAG                         0x2000000
#define RB_CMD_DEL_SYS_ERR                  0x40000000
#define RB_CMD_DEL_BAL_ERR                  0x80000000

//----------------------------------------------------------------------------
// Global Valiable
//----------------------------------------------------------------------------
struct ROTOR_INTERFACE gRotor = { 0, };
int gRotorResult[3] = { 0, };

static int OrgMoveDistance[3];
static unsigned long CommandSendTime;
static signed char PowerCheckRetryCount;
static signed char MoveRetryCount;

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void RB_Process(void);
void RB_CanRecvHandler(CanRxMsgTypeDef* msg);

void RB_Cancel(void);
void RB_PowerOnAndCheck(int retry_cnt);
    static void RB_PowerOnAndCheckForRetry(int retry_cnt);
void RB_GetInformation(void);
void RB_SetMotorPower(unsigned char level);
void RB_SetWeightDefaultPosition(int balancer1_mm, int balancer2_mm, int balancer3_mm);
void RB_SetBalancerWeight(int balancer1_gram, int balancer2_gram, int balancer3_gram);

void RB_Move(int retry_cnt, int bal1_mm, int bal2_mm, int bal3_mm);
    static void RB_SendCommand(int cmd);
    static void RB_InputParameter(int balancer1_mm, int balancer2_mm, int balancer3_mm);
    static void RB_GetData(CanRxMsgTypeDef* msg, int* array);

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
void RB_Process(void)
{
    if ( gRotor.status != ROTOR_READY && gCancel )
    {
        RotorPower(0);
        SlipRingPower(0);

        gRotor.status = ROTOR_FINISHED;
        gRotor.result = ERROR_USER_CANCEL;
    }
    else if ( (gRotor.status == ROTOR_POWERCHECK || gRotor.status == ROTOR_POWERCHECK_FOR_RETRY) && TIME_FROM(CommandSendTime) > 1 )
    {
        RotorPower(0);
        SlipRingPower(0);

        if ( PowerCheckRetryCount > 0 )
        {
            PowerCheckRetryCount--;
            Delay_ms(200);
            if ( gRotor.status == ROTOR_POWERCHECK )
                RB_PowerOnAndCheck(-1);
            else
                RB_PowerOnAndCheckForRetry(-1);
        }
        else
        {
            gRotor.status = ROTOR_READY;
            gRotor.result = ERROR_ROTOR_POWER;
        }
    }
    else if ( gRotor.status == ROTOR_QUERY && TIME_FROM(CommandSendTime) > 1 )
    {
        gRotor.status = ROTOR_READY;
        gRotor.result = ERROR_ROTOR_NOT_RESPONDING;
    }
    else if ( gRotor.status == ROTOR_MOVE && TIME_FROM(CommandSendTime) > 15 )
    {
        RotorPower(0);
        SlipRingPower(0);

        if ( MoveRetryCount > 0 )
        {
            MoveRetryCount--;
            Delay_ms(200);
            RB_PowerOnAndCheckForRetry(-1);
        }
        else
        {
            gRotor.status = ROTOR_READY;
            gRotor.result = ERROR_ROTOR_TIMEOUT;
        }
    }
    else if ( gRotor.status == ROTOR_MOVE_RETRY )
    {
        RB_Move(-1, OrgMoveDistance[0], OrgMoveDistance[1], OrgMoveDistance[2]);
    }
    else if ( gRotor.status == ROTOR_MOVE_RESULT )
    {
        gRotorResult[0] = gRotor.move_result[0];
        gRotorResult[1] = gRotor.move_result[1];
        gRotorResult[2] = gRotor.move_result[2];

        WI(gRotorResult[0]);
        WI(gRotorResult[1]);
        WI(gRotorResult[2]);

        if ( gRotor.move_result[0]<0 || gRotor.move_result[1]<0 || gRotor.move_result[2]<0 )
        {
            if ( MoveRetryCount > 0 )
            {
                MoveRetryCount--;
                RB_Move(-1, OrgMoveDistance[0], OrgMoveDistance[1], OrgMoveDistance[2]);
            }
            else
            {
                RotorPower(0);
                SlipRingPower(0);

                gRotor.status = ROTOR_READY;
                gRotor.result = ERROR_ROTOR_RET_ORG;
            }
        }
        else if ( (gRotor.move_result[0]<OrgMoveDistance[0]-50 || gRotor.move_result[0]>OrgMoveDistance[0]+50) ||
                  (gRotor.move_result[1]<OrgMoveDistance[1]-50 || gRotor.move_result[1]>OrgMoveDistance[1]+50) ||
                  (gRotor.move_result[2]<OrgMoveDistance[2]-50 || gRotor.move_result[2]>OrgMoveDistance[2]+50) )
        {
            if ( MoveRetryCount > 0 )
            {
                MoveRetryCount--;
                RB_Move(-1, OrgMoveDistance[0], OrgMoveDistance[1], OrgMoveDistance[2]);
            }
            else
            {
                RotorPower(0);
                SlipRingPower(0);

                gRotor.status = ROTOR_READY;
                gRotor.result = ERROR_ROTOR_JAM;
            }
        }
        else
        {
            RotorPower(0);
            SlipRingPower(0);

            gRotor.status = ROTOR_READY;
            gRotor.result = SUCCESS;
        }
    }
}

void RB_CanRecvHandler(CanRxMsgTypeDef* msg_obj)
{
    DPRINTF(("0x%x: %x %x %x %x %x %x %x %x\r\n", msg_obj->StdId, msg_obj->Data[0], msg_obj->Data[1], msg_obj->Data[2], msg_obj->Data[3], msg_obj->Data[4], msg_obj->Data[5], msg_obj->Data[6], msg_obj->Data[7]));

    switch ( msg_obj->StdId )
    {
        case 0x642: // 명령 수행 결과
            if ( gRotor.status == ROTOR_MOVE )
                gRotor.status = ROTOR_MOVE_RESULT;
            else if ( gRotor.status == ROTOR_POWERCHECK_FOR_RETRY )
                gRotor.status = ROTOR_MOVE_RETRY;
            else
                gRotor.status = ROTOR_READY;
            RB_GetData(msg_obj, gRotor.move_result);
            break;

        case 0x644: // 무게추 기본 위치 저장 (결과 0x644)
            gRotor.result = SUCCESS;
            gRotor.status = ROTOR_READY;
            break;

        case 0x649: // 무게추 무게 저장 (결과 0x649)
            gRotor.result = SUCCESS;
            gRotor.status = ROTOR_READY;
            break;

        case 0x64B: // 제품타입 및 버젼 확인 (결과 0x64B)
            gRotor.version = msg_obj->Data[0];
            gRotor.product_type = msg_obj->Data[2];
            //WX(gRotor.version);
            //WX(gRotor.product_type);
            CommandSendTime = gSeconds;
            CAN_Send(ROTOR_CAN, 0x64A, NULL, 0, CAN_SEND_NO_WAIT);
            break;

        case 0x64A: // 무게추 무게 확인 (결과 0x64A)
            RB_GetData(msg_obj, gRotor.weight);
            CommandSendTime = gSeconds;
            CAN_Send(ROTOR_CAN, 0x647, NULL, 0, CAN_SEND_NO_WAIT);
            break;

        case 0x647: // GUID 확인 1 (결과 0x647)
            memcpy(&gRotor.guid, msg_obj->Data, 8);
            CommandSendTime = gSeconds;
            CAN_Send(ROTOR_CAN, 0x648, NULL, 0, CAN_SEND_NO_WAIT);
            break;

        case 0x648: // GUID 확인 2 (결과 0x648)
            memcpy(gRotor.guid.data4, msg_obj->Data, 8);
            CommandSendTime = gSeconds;
            CAN_Send(ROTOR_CAN, 0x645, NULL, 0, CAN_SEND_NO_WAIT);
            break;

        case 0x645: // 무게추 기본 위치 확인 (결과 0x645)
            gRotor.result = SUCCESS;
            gRotor.status = ROTOR_READY;
            RB_GetData(msg_obj, gRotor.default_pos);
            break;
    }
}

void RB_SetMotorPower(unsigned char level)
{
    unsigned char param[8];

    gRotor.status = ROTOR_QUERY;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    memset(param, 0, 8);
    param[3] = level;
    CAN_Send(ROTOR_CAN, 0x643, param, 8, CAN_SEND_NO_WAIT);
}

void RB_SetWeightDefaultPosition(int balancer1_mm, int balancer2_mm, int balancer3_mm)
{
    unsigned char param[8];

    gRotor.status = ROTOR_QUERY;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    memset(param, 0, 8);
    *((short*)&param[2]) = balancer1_mm;
    *((short*)&param[0]) = balancer2_mm;
    *((short*)&param[6]) = balancer3_mm;

    CAN_Send(ROTOR_CAN, 0x644, param, 8, CAN_SEND_NO_WAIT);
}

void RB_SetBalancerWeight(int balancer1_gram, int balancer2_gram, int balancer3_gram)
{
    /*
    gSwingRotorBalancingParam.weight_mass[0] = (float)balancer1_gram/10;
    gSwingRotorBalancingParam.weight_mass[1] = (float)balancer2_gram/10;
    gSwingRotorBalancingParam.weight_mass[2] = (float)balancer3_gram/10;
    */

    unsigned char param[8];

    gRotor.status = ROTOR_QUERY;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    memset(param, 0, 8);
    *((short*)&param[2]) = balancer1_gram;
    *((short*)&param[0]) = balancer2_gram;
    *((short*)&param[6]) = balancer3_gram;

    CAN_Send(ROTOR_CAN, 0x649, param, 8, CAN_SEND_NO_WAIT);
}

void RB_GetInformation(void)
{
    gRotor.status = ROTOR_QUERY;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    CAN_Send(ROTOR_CAN, 0x64B, NULL, 0, CAN_SEND_NO_WAIT);
}

void RB_SendCommand(int cmd)
{
    unsigned char param[8];

    memset(param, 0, 8);
    *((int*)&param[0]) = cmd;
    SWAP4(param);

    CAN_Send(ROTOR_CAN, 0x641, param, 8, CAN_SEND_NO_WAIT);
}

void RB_InputParameter(int balancer1_mm, int balancer2_mm, int balancer3_mm)
{
    unsigned char param[8];

    OrgMoveDistance[0] = balancer1_mm;
    OrgMoveDistance[1] = balancer2_mm;
    OrgMoveDistance[2] = balancer3_mm;

    memset(param, 0, 8);
    *((short*)&param[2]) = balancer1_mm * 2;
    *((short*)&param[0]) = balancer2_mm * 2;
    *((short*)&param[6]) = balancer3_mm * 2;

    gRotorResult[0] = balancer1_mm;
    gRotorResult[1] = balancer2_mm;
    gRotorResult[2] = balancer3_mm;

    CAN_Send(ROTOR_CAN, 0x640, param, 8, CAN_SEND_NO_WAIT);
    Delay_ms(50);
}

void RB_GetData(CanRxMsgTypeDef* msg_obj, int* array)
{
    short range;

    /*
    if ( !msg_obj || !array )
        RETURN(ERROR_PARAMETER);
    */

    range = *((short*)&msg_obj->Data[2]);
    array[0] = range;

    range = *((short*)&msg_obj->Data[0]);
    array[1] = range;

    range = *((short*)&msg_obj->Data[6]);
    array[2] = range;
}

void RB_PowerOnAndCheck(int retry_cnt)
{
    SlipRingPower(1);
    Delay_ms(200);
    RotorPower(1);
    Delay_ms(300);

    gRotor.status = ROTOR_POWERCHECK;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    RB_SendCommand(RB_CMD_GET_STATUS);

    if ( retry_cnt >= 0 )
        PowerCheckRetryCount = retry_cnt;
}

void RB_PowerOnAndCheckForRetry(int retry_cnt)
{
    SlipRingPower(1);
    Delay_ms(200);
    RotorPower(1);
    Delay_ms(300);

    gRotor.status = ROTOR_POWERCHECK_FOR_RETRY;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    RB_SendCommand(RB_CMD_GET_STATUS);

    if ( retry_cnt >= 0 )
        PowerCheckRetryCount = retry_cnt;
}

void RB_Move(int retry_cnt, int bal1_mm, int bal2_mm, int bal3_mm)
{
    if ( gRotor.status != ROTOR_READY && gRotor.status != ROTOR_MOVE_RESULT )
        return;

    if ( retry_cnt >= 0 )
        MoveRetryCount = retry_cnt;

    if ( retry_cnt < 0 && MoveRetryCount == 0 )
    {
        RB_SetMotorPower(0x7f);
        Delay_ms(200);
    }

    gRotor.status = ROTOR_MOVE;
    gRotor.result = SUCCESS;
    CommandSendTime = gSeconds;

    RB_InputParameter(bal1_mm, bal2_mm, bal3_mm);
    RB_SendCommand(RB_CMD_GET_STATUS | RB_CMD_BAL1_RET_ORG_MOVE | RB_CMD_BAL2_RET_ORG_MOVE | RB_CMD_BAL3_RET_ORG_MOVE);
}


void RB_Cancel(void)
{
    RotorPower(0);
    SlipRingPower(0);

    gRotor.status = ROTOR_READY;
    gRotor.result = ERROR_USER_CANCEL;
}
