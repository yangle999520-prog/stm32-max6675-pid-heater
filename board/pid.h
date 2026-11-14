#ifndef __PID_H
#define __PID_H
/* Kp: counts/C; Ki: counts/(C*s); Kd: counts*s/C. */
typedef struct {
		float Target, Actual, Out;
		float Kp, Ki, Kd;
		float Error0, Error1, ErrorInt;
		float OutMax, OutMin;
} PID_t;
void PID_Init(PID_t *p);
void PID_Reset(PID_t *p);
void PID_Update(PID_t *p); /* Once per valid 250 ms sample. */
#endif
