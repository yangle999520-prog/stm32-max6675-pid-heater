#include "pid.h"
#define DT 0.25f
void PID_Reset(PID_t *p) { p->Out=0; p->Error0=0; p->Error1=0; p->ErrorInt=0; }
void PID_Init(PID_t *p) { p->Actual=0; PID_Reset(p); }
void PID_Update(PID_t *p)
{
 float integral, output;
 p->Error1=p->Error0;
 p->Error0=p->Target-p->Actual;
 integral=p->ErrorInt+p->Error0*DT;
 output=p->Kp*p->Error0+p->Ki*integral+p->Kd*(p->Error0-p->Error1)/DT;
 /* Pause integration when saturated in the same direction as error. */
 if (!((output>p->OutMax && p->Error0>0) || (output<p->OutMin && p->Error0<0))) p->ErrorInt=integral;
 output=p->Kp*p->Error0+p->Ki*p->ErrorInt+p->Kd*(p->Error0-p->Error1)/DT;
 if (output>p->OutMax) output=p->OutMax;
 if (output<p->OutMin) output=p->OutMin;
 p->Out=output;
}
