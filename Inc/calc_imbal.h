#ifndef __CALC_IMBAL_H__
#define __CALC_IMBAL_H__

#define SWING_ROTOR_BALANCER_WEIGHT     285.7

extern double gWeight_Mass[3]; // g

void SetBalancerWeight(float w1, float w2, float w3);
int  GetCompensationConstant(int weight_idx, double move_mm, double first_vib, double first_phase, double vib, double phase, double* vib_const, double* phase_const);
void SetCompensationConstant(double vib_const_mg_per_g_mm, double phase_const_degree);
int  GetCompensationResult(double vib_mg, double phase_degree, int* count1_move_mm, int* count2_move_mm, int* count3_move_mm);
int  GetCompensationRawResult(double vib_mg, double phase_degree, double* count1_amout, double* count2_amout, double* count3_amout);

#endif


