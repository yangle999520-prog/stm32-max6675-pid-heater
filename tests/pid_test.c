#include <assert.h>
#include "pid.h"

int main(void)
{
    PID_t p = {0};
    int i;
    p.Target = 40.0f;
    p.Kp = 5.0f;
    p.Ki = 0.125f;
    p.OutMin = 0.0f;
    p.OutMax = 100.0f;
    PID_Init(&p);
    assert(p.Target == 40.0f);

    p.Actual = 20.0f;
    for (i = 0; i < 100; ++i) PID_Update(&p);
    assert(p.Out == 100.0f);
    assert(p.ErrorInt == 0.0f);

    p.Actual = 39.0f;
    PID_Update(&p);
    assert(p.Out > 0.0f && p.Out < 100.0f);
    assert(p.ErrorInt > 0.0f);

    p.Actual = 50.0f;
    PID_Update(&p);
    assert(p.Out == 0.0f);
    PID_Reset(&p);
    assert(p.Out == 0.0f && p.ErrorInt == 0.0f);
    assert(p.Target == 40.0f && p.Kp == 5.0f);
    return 0;
}
