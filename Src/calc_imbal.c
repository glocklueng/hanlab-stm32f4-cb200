#include "main.h"
#include <math.h>
#include "calc_imbal.h"

#define PI        (3.141596)
#define TO_DEGREE (180/PI)
#define TO_RADIAN (PI/180)

//----------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------
double gWeight_Mass[3] = { SWING_ROTOR_BALANCER_WEIGHT, SWING_ROTOR_BALANCER_WEIGHT, SWING_ROTOR_BALANCER_WEIGHT }; // g

static double VibAmount; // mG
static double VibPhase; // degree

static double VibProportionConstant = 0.00008; // mG/g.mm
static double VibPhaseConstant = -143; // degree

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
static const double Weight_Degree[3] = { 0 , 120, 240 }; // degree

//----------------------------------------------------------------------------
// Function Declaration
//----------------------------------------------------------------------------
void SetBalancerWeight(float w1, float w2, float w3);
int  GetCompensationConstant(int weight_idx, double move_mm, double first_vib, double first_phase, double vib, double phase, double* vib_const, double* phase_const);
void SetCompensationConstant(double vib_const_mg_per_g_mm, double phase_const_degree);
int  GetCompensationResult(double vib_mg, double phase_degree, int* count1_move_mm, int* count2_move_mm, int* count3_move_mm);
int  GetCompensationRawResult(double vib_mg, double phase_degree, double* count1_amount, double* count2_amount, double* count3_amount);
    double Weight1_CompensationRange(void); // mm
    double Weight2_CompensationRange(void); // mm
    double Weight3_CompensationRange(void); // mm
        double Weight1_CompensationAmount(void); // g.mm
        double Weight2_CompensationAmount(void); // g.mm
        double Weight3_CompensationAmount(void); // g.mm
            int MoveWeight1(void);
            int MoveWeight2(void);
            int MoveWeight3(void);
            double CompensationAmount(void); // g.mm
            double CompensationDegree(void); // degree
                double ImbalanceAmount(void); // g.mm
                double ImbalancePhase(void); // degree

//----------------------------------------------------------------------------
// Function Body
//----------------------------------------------------------------------------
void SetBalancerWeight(float w1, float w2, float w3)
{
    gWeight_Mass[0] = w1;
    gWeight_Mass[1] = w2;
    gWeight_Mass[2] = w3;
}

int GetCompensationConstant(int weight_idx, double move_mm, double first_vib, double first_phase, double vib, double phase, double* vib_const, double* phase_const)
{
    if ( !vib_const || !phase_const )
        return -1;

    if ( weight_idx>2 || weight_idx<0 || move_mm<0 || first_vib<0 || first_phase<-180 || first_phase>180 || vib<0 || phase<-180 || phase>180 )
        return -2;

    double cos_temp = vib*cos(phase*TO_RADIAN)-first_vib*cos(first_phase*TO_RADIAN);
    double sin_temp = vib*sin(phase*TO_RADIAN)-first_vib*sin(first_phase*TO_RADIAN);

    *vib_const = sqrt(cos_temp*cos_temp+sin_temp*sin_temp) / (gWeight_Mass[weight_idx] * move_mm);

    if ( cos_temp < 0 )
    {
        if ( sin_temp >= 0 )
        {
            *phase_const = ((Weight_Degree[weight_idx]*TO_RADIAN) - atan(sin_temp/cos_temp) + PI) * 180 / PI;
        }
        else
        {
            *phase_const = ((Weight_Degree[weight_idx]*TO_RADIAN) - atan(sin_temp/cos_temp) - PI) * 180 / PI;
        }
    }
    else
    {
        *phase_const = (Weight_Degree[weight_idx]*TO_RADIAN - atan(sin_temp/cos_temp)) * 180 / PI;
    }

    return 0;
}

void SetCompensationConstant(double vib_const_mg_per_g_mm, double phase_const_degree)
{
    VibProportionConstant = vib_const_mg_per_g_mm;
    VibPhaseConstant = phase_const_degree;
}

int GetCompensationResult(double vib_mg, double phase_degree, int* count1_move_mm, int* count2_move_mm, int* count3_move_mm)
{
    if ( !count1_move_mm || !count2_move_mm || !count3_move_mm )
        return -1;

    if ( vib_mg<0 || phase_degree<-180 || phase_degree>180 )
        return -2;

    VibAmount = vib_mg;
    VibPhase = phase_degree;

    *count1_move_mm = (int)(Weight1_CompensationRange()*100);
    *count2_move_mm = (int)(Weight2_CompensationRange()*100);
    *count3_move_mm = (int)(Weight3_CompensationRange()*100);

    if ( *count1_move_mm<0 || *count2_move_mm<0 || *count3_move_mm<0 )
        return -3;

    return 0;
}

int GetCompensationRawResult(double vib_mg, double phase_degree, double* count1_amount, double* count2_amount, double* count3_amount)
{
    if ( !count1_amount || !count2_amount || !count3_amount )
        return -1;

    if ( vib_mg<0 || phase_degree<-180 || phase_degree>180 )
        return -2;

    VibAmount = vib_mg;
    VibPhase = phase_degree;

    *count1_amount = Weight1_CompensationAmount();
    *count2_amount = Weight2_CompensationAmount();
    *count3_amount = Weight3_CompensationAmount();

    return 0;
}

double ImbalanceAmount(void) // g.mm
{
    return VibAmount / VibProportionConstant;
}

double ImbalancePhase(void) // degree
{
    if ( (VibPhaseConstant+VibPhase) <= -180 )
        return (VibPhaseConstant+VibPhase+360);
    else if( (VibPhaseConstant+VibPhase) > 180 )
        return (VibPhaseConstant+VibPhase-360);
    else
        return (VibPhaseConstant+VibPhase);
}

double CompensationAmount(void) // g.mm
{
    return ImbalanceAmount();
}

double CompensationDegree(void) // degree
{
    if ( (ImbalancePhase()+180) <= -180 )
        return (ImbalancePhase()+540);
    else if ( (ImbalancePhase()+180) > 180 )
        return (ImbalancePhase()-180);
    else
        return (ImbalancePhase()+180);
}

int MoveWeight1(void)
{
    if ( CompensationDegree()>-180 )
        if ( CompensationDegree()>=-120 )
            if ( CompensationDegree()>-120 )
                if ( CompensationDegree()>=0 )
                    if ( CompensationDegree()>0 )
                        if ( CompensationDegree()>=120 )
                            if ( CompensationDegree()>120 )
                                if ( CompensationDegree()>180 )
                                    return -1; // error
                                else return 2;
                            else return 2;
                        else return 1;
                    else return 1;
                else return 1;
            else return 3;
        else return 2;
    else return -1; // error
}

int MoveWeight2(void)
{
    if ( CompensationDegree()>-180 )
        if ( CompensationDegree()>=-120 )
            if ( CompensationDegree()>-120 )
                if ( CompensationDegree()>=0 )
                    if ( CompensationDegree()>0 )
                        if ( CompensationDegree()>=120 )
                            if ( CompensationDegree()>120 )
                                if ( CompensationDegree()>180 )
                                    return -1; // error
                                else return 3;
                            else return 0;
                        else return 2;
                    else return 0;
                else return 3;
            else return 0;
        else return 3;
    else return -1; // error
}


double Weight1_CompensationAmount(void) // g.mm
{
    if ( MoveWeight1()+MoveWeight2() == 1 )
        return CompensationAmount();
    else if ( MoveWeight1()+MoveWeight2() == 2 )
        return 0;
    else if ( MoveWeight1()+MoveWeight2() == 3 )
    {
        if ( MoveWeight1()*MoveWeight2() == 2 )
            return (CompensationAmount()*sin((Weight_Degree[1]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[1]-Weight_Degree[0])*TO_RADIAN));
        else
            return 0;
    }
    else if ( MoveWeight1()+MoveWeight2() == 4 )
        return (CompensationAmount()*sin((Weight_Degree[2]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[2]-Weight_Degree[0])*TO_RADIAN));
    else if ( MoveWeight1()+MoveWeight2() == 5 )
        return 0;
    else
        return -1; // error
}


double Weight2_CompensationAmount(void) // g.mm
{
    if ( MoveWeight1()+MoveWeight2() == 1 )
        return 0;
    else if ( MoveWeight1()+MoveWeight2() == 2 )
        return CompensationAmount();
    else if ( MoveWeight1()+MoveWeight2() == 3 )
    {
        if ( MoveWeight1()*MoveWeight2() == 2 )
            return (CompensationAmount()*sin((Weight_Degree[0]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[0]-Weight_Degree[1])*TO_RADIAN));
        else
            return 0;
    }
    else if ( MoveWeight1()+MoveWeight2() == 4 )
        return 0;
    else if ( MoveWeight1()+MoveWeight2() == 5 )
        return (CompensationAmount()*sin((Weight_Degree[2]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[2]-Weight_Degree[1])*TO_RADIAN));
    else
        return -1; // error
}

double Weight3_CompensationAmount(void) // g.mm
{
    if ( MoveWeight1()+MoveWeight2() == 1 )
        return 0;
    else if ( MoveWeight1()+MoveWeight2() == 2 )
        return 0;
    else if ( MoveWeight1()+MoveWeight2() == 3 )
    {
        if ( MoveWeight1()*MoveWeight2() == 2 )
            return 0;
        else
            return CompensationAmount();
    }
    else if ( MoveWeight1()+MoveWeight2() == 4 )
        return (CompensationAmount()*sin((Weight_Degree[0]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[0]-Weight_Degree[2])*TO_RADIAN));
    else if ( MoveWeight1()+MoveWeight2() == 5 )
        return (CompensationAmount()*sin((Weight_Degree[1]-CompensationDegree())*TO_RADIAN)/sin((Weight_Degree[1]-Weight_Degree[2])*TO_RADIAN));
    else
        return -1; // error
}


double Weight1_CompensationRange(void) // mm
{
    return Weight1_CompensationAmount() / gWeight_Mass[0];
}

double Weight2_CompensationRange(void) // mm
{
    return Weight2_CompensationAmount() / gWeight_Mass[1];
}

double Weight3_CompensationRange(void) // mm
{
    return Weight3_CompensationAmount() / gWeight_Mass[2];
}

