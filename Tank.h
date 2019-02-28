#ifndef __TANK_H__
#define __TANK_H__

#include "Type.h"

#define RTN_ERR         FALSE//返回错误
#define RTN_OK          TRUE//返回0
#define RTN_NULL        NULL//返回NULL

typedef enum dir_e
{
    DIR_MIN = 0,
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_MAX
} dir_t;//方向

int32 tank_init(void);
int32 tank_run(dir_t* dir, int32* fire, int32 super, int32 debug);

#endif // !__TANK_H__