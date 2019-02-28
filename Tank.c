#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "resource.h"
#include "Tank.h"
#include "Gdi.h"

#define TankScreenX         (SCREEN_X)//屏幕长度
#define TankScreenY         (SCREEN_Y)//屏幕宽度
#define TankPntSize         (MAX_PNT_SIZE)//像素点缩放比例

#define TankDbgTextSize     (20)

#define TankPointXY         (TankPntSize)//像素点缩放比例
#define TankMaxX            ((TankScreenX) / (TankPointXY))//缩放后的x轴坐标最大值
#define TankMaxY            ((TankScreenY) / (TankPointXY))//缩放后的y轴坐标最大值

#define TANK_SHAPE_NUM_MAX      (4u)//总共四种坦克形状，对应上右下左四个方向
#define TANK_SHAPE_PNT_MAX      (6u)//每个坦克用六个坐标点表示

#define TANK_ENMY_WAR_NUM_MAX   (((TankMaxX * TankMaxY) / 6) - 1)//敌军参战的坦克最大数量
#define TANK_ENMY_WAR_NUM_MIN   (5u)//敌军参战的坦克最小数量(最初)
#define TANK_MYSELF_WAR_NUM_MAX (1u)//我军参战的坦克最大数量
#define TANK_MYSELF_WAR_NUM_MIN (1u)//我军参战的坦克最小数量(最初)
#define TANK_WAR_NUM_MAX        (TANK_ENMY_WAR_NUM_MAX + TANK_MYSELF_WAR_NUM_MAX)//坦克最大数量
#define TANK_BOMB_NUM_MAX       (TANK_WAR_NUM_MAX * 3)//炮弹最大数量
#define TANK_ALL_NUM_MIN        (TANK_ENMY_WAR_NUM_MIN + TANK_MYSELF_WAR_NUM_MIN)
#define TANK_ALL_NUM_MAX        (TANK_ENMY_WAR_NUM_MAX + TANK_MYSELF_WAR_NUM_MAX)
#define TANK_ENMY_NUM_INIT      (TANK_ALL_NUM_MIN)//初始坦克数量

#define TANK_SPEED_MAX          (5u)
#define TANK_TIMER_MIN          (0u)
#define TANK_TIMER_MAX          (100u)

#define TANK_TIMER_FIRE_MIN     (TANK_TIMER_MIN)
#define TANK_TIMER_FIRE_MAX     (TANK_TIMER_MAX)

#define TANK_TIMER_EQUIP_MIN    (TANK_TIMER_MAX)
#define TANK_TIMER_EQUIP_MAX    (TANK_TIMER_MAX*10)

#define TANK_WEAPON_LIFE_MAX    (5u)//每个武器最大使用次数

#define TANK_OF_MYSELF          (TANK_WAR_BOX[0])

#define COPY_POINT(pdpnt, pspnt)                \
do                                              \
{                                               \
    (pdpnt)->x = (pspnt)->x;                    \
    (pdpnt)->y = (pspnt)->y;                    \
    (pdpnt)->col = (pspnt)->col;                \
}while(0)

#define COPY_TANK(pdtank, pstank)                       \
do                                                      \
{                                                       \
    COPY_POINT(&((pdtank)->pnt), &((pstank)->pnt));     \
    (pdtank)->dir = (pstank)->dir;                      \
    (pdtank)->pr = (pstank)->pr;                        \
    (pdtank)->lf = (pstank)->lf;                        \
    (pdtank)->mv = (pstank)->mv;                        \
}while(0)

typedef enum property_e
{
    PR_MIN = 0,
    PR_NULL = 0,
    PR_WALL, //墙
    PR_WEAPON, //武器
    PR_LIFE, //装备
    PR_BOMB, //炮弹
    PR_MYSELF, //自己
    PR_ENMY, //敌人
    PR_TBOMB,
    PR_MAX
} property_t; //属性

typedef enum weapon_e
{
    WP_MIN = 0,
    WP_NONE,//普通武器
    WP_MAX
}weapon_t;

typedef enum life_e
{
    LF_MIN = 0,
    LF_DIE = 0,//死亡
    LF_BURN,//燃烧
    LF_LIVE,//存活
    LF_MAX
} life_t;

typedef enum move_e
{
    MV_MIN = 0,
    MV_STOP = 0,//停止
    MV_MOVE,//移动
    MV_MAX
}move_t;

typedef struct point_s
{
    uint32_t x;
    uint32_t y;
    uint32_t col;
} point_t;//坐标点

typedef struct equip_s
{
    int8        valid;
    point_t     pnt;//装备的坐标点
    property_t  pr;//装备属性(武器还是生命) 
    int32       tmr;//装备存活定时器，到期后装备消失
} equip_t;

typedef struct tank_s
{
    int8        valid;
    dir_t       dir;//坦克方向，同时也是其在TANK_SHAPE_BOX中的索引
    point_t     pnt;//坦克左上角的坐标，其它点的相对坐标在TANK_SHAPE_BOX中
    property_t  pr;//属性
    int32       lf;//生命
    int32       wep;//武器
    int32       mv;//移动(步数)
    int32       fire;//开火倒计时
    int32       kill;//击杀的敌军数量
    int32       bomb;//消耗弹药数量
} tank_t;

typedef struct bomb_s
{
    int8        valid;
    dir_t       dir;//炮弹飞行方向
    point_t     pnt;//炮弹的坐标点
    property_t  pr;//炮弹属性(敌人的炮弹还是自己的炮弹)
    tank_t*     ptank;//这颗炮弹是哪辆坦克发射的
} bomb_t;

typedef struct tank_pr_s
{
    int32 cur_tank_num;//当前坦克数量
    int32 war_tank_num;//参战斗坦克数量
    //int32 kill_tank_num;//阵亡(杀敌)坦克数
    //int32 cur_bomb_num;//当前炮弹数量
    int32 create_equp_tmr;//装备产生定时器，到期后产生一个装备(武器或生命)
    int32 speed;
    int32 super;//无敌模式
    int32 debug;//调试信息
}tank_pr_t;

typedef struct warmap_s
{
    int32 col;
    int32 pr;
}warmap_t;

const point_t TANK_SHAPE_BOX[TANK_SHAPE_NUM_MAX][TANK_SHAPE_PNT_MAX] =
{//不论哪个方向的坦克，其车身点在数组中的位置都是固定的
    { { 1, 0, TRUE }, { 0, 1, TRUE }, { 2, 1, TRUE }, { 1, 1, TRUE }, { 0, 2, TRUE }, { 2, 2, TRUE } }, //上
    { { 2, 1, TRUE }, { 1, 0, TRUE }, { 1, 2, TRUE }, { 1, 1, TRUE }, { 0, 0, TRUE }, { 0, 2, TRUE } }, //右
    { { 1, 2, TRUE }, { 2, 1, TRUE }, { 0, 1, TRUE }, { 1, 1, TRUE }, { 2, 0, TRUE }, { 0, 0, TRUE } }, //下
    { { 0, 1, TRUE }, { 1, 2, TRUE }, { 1, 0, TRUE }, { 1, 1, TRUE }, { 2, 2, TRUE }, { 2, 0, TRUE } }  //左
};

const int32 TANK_PR_COLOUR[PR_MAX] =
{
    GDI_RGB_BACKGROUND,//PR_NULL = 0,//空白，黑色
    GDI_RGB_FOREGROUND,//PR_WALL, //墙，白色
    GDI_RGB_PURPLE,//PR_WEAPON, //武器，紫色
    GDI_RGB_GREEN,//PR_LIFE, //装备，绿色
    GDI_RGB_YELLOW,//PR_BOMB, //炮弹，黄色
    GDI_RGB_RED,//PR_MYSELF, //自己，红色
    GDI_RGB_BLUE,//PR_ENMY, //敌人，蓝色
    GDI_RGB_YELLOW//PR_TBOMB,//烧毁的坦克，黄色
};

static tank_t   TANK_WAR_BOX[TANK_WAR_NUM_MAX] = { 0 };//参战斗坦克(最大)
//static int32    TANK_WAR_BOX_VALID[TANK_WAR_NUM_MAX] = { 0 };
static bomb_t   TANK_BOMB_BOX[TANK_BOMB_NUM_MAX] = { 0 };//同一时刻所有坦克发出三发炮弹
//static int32    TANK_BOMB_BOX_VALID[TANK_BOMB_NUM_MAX] = { 0 };
static warmap_t TANK_WAR_MAP[TankMaxX][TankMaxY] = { 0 };//坦克大战游戏地图,相当于显示缓存

static equip_t TANK_EQUIP = { 0 };//装备
static tank_pr_t TANK_PR = { 0 };//属性

//static uint32 TANK_DEBUG = FALSE;

void tank_debug_out(void)
{
    int32 i = 0;
    int32 kill = 0, life = 0, enemy = 0, bomb = 0, super = 0;
    TCHAR debugStr[200] = { 0 };
    PTCHAR superStr = NULL;

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        if ((TANK_WAR_BOX[i].pr != PR_NULL) &&
            (TANK_WAR_BOX[i].valid != FALSE))
        {
            if (TANK_WAR_BOX[i].pr != PR_MYSELF)
            {
                enemy += 1;
            }
            else
            {
                life = TANK_WAR_BOX[i].lf - LF_BURN;
                life = ((life > 0) ? life : 0);
                kill = TANK_WAR_BOX[i].kill;
                bomb = TANK_WAR_BOX[i].bomb;
            }
        }
    }

#ifdef DEBUG_TEXT_OUT_CHN
    superStr = (TANK_PR.super) ? (TEXT("开启")) : (TEXT("关闭"));
    wsprintf(debugStr, TEXT("生命值[%03d], 消灭敌军[%03d], 敌军数量[%03d], 消耗弹药[%06d], 超级模式[%s]"),
        life, kill, enemy, bomb, superStr);
#else
    superStr = (TANK_PR.super) ? (TEXT("ON")) : (TEXT("OFF"));
    wsprintf(debugStr, TEXT("Kill[%03d], Life[%03d], Enemy[%03d], Bomb[%06d], Super[%s]"),
        kill, life, enemy, bomb, superStr);
#endif
    gdi_textout(0, TankScreenY - TankDbgTextSize - 1, TankDbgTextSize, GDI_RGB_FOREGROUND, 0, debugStr);
}

int32 tank_set_super(tank_t* tank, int32 super)
{
    if (super)
    {
        tank->lf = LF_MAX;
        tank->wep = WP_MAX;
    }

    return RTN_OK;
}

//调用GDI绘制一个形状
int32 tank_draw_point(int32 x, int32 y, int32 col, int32 pr)
{
    //pr属性用于控制显示的形状

    //像素点缩放
    x = ((x < 0) ? 0 : ((x >= TankMaxX) ? (TankMaxX - 1) : x));
    y = ((y < 0) ? 0 : ((y >= TankMaxY) ? (TankMaxY - 1) : y));

    pr = ((pr >= PR_MAX) ? PR_NULL : ((pr <= PR_NULL) ? PR_NULL : pr));

    if (TankPointXY != 1)//画一个矩形代表缩放过后的点
    {
        x = x * TankPointXY;
        y = y * TankPointXY;
        switch (pr)
        {
        case PR_BOMB://炮弹，黄色圆形
            gdi_circle(x + (TankPointXY / 2), y + (TankPointXY / 2), (TankPointXY / 2) - 1, col, 1);
            break;
        case PR_WEAPON://武器，紫色三角形
            gdi_triangle(x, y, x + TankPointXY - 1, y + TankPointXY - 1, col, 1);
            break;
        case PR_LIFE://装备，绿色菱形
            gdi_rhombus(x, y, x + TankPointXY - 1, y + TankPointXY - 1, col, 1);
            break;
        case PR_WALL://墙，白色矩形
        case PR_MYSELF://我军，红色矩形
        case PR_ENMY://敌军，蓝色矩形
            gdi_rectangle(x, y, x + TankPointXY - 1, y + TankPointXY - 1, col, 1);
            break;
        case PR_NULL://空白，黑色矩形
        default:
            col = TANK_PR_COLOUR[PR_NULL];
            gdi_rectangle(x, y, x + TankPointXY - 1, y + TankPointXY - 1, col, 1);
            break;
        }
    }
    else//为了加快速度，宽度为1不缩放，直接画点
    {
        gdi_set_point(x, y, col);
    }

    return RTN_OK;
}

//清空地图中的数据
int32 tank_clear_warmap(void)
{
    int32 x = 0, y = 0;
    for (x = 0; x < TankMaxX; x++)
    {
        for (y = 0; y < TankMaxY; y++)
        {
            TANK_WAR_MAP[x][y].col = TANK_PR_COLOUR[PR_NULL];
            TANK_WAR_MAP[x][y].pr = PR_NULL;
        }
    }

    return RTN_OK;
}

//把地图绘制到屏幕窗口上
int32 tank_update_warmap(void)
{
    int32 x = 0, y = 0;

    TCHAR debugStr[200] = { 0 };

    gdi_clear(TANK_PR_COLOUR[PR_NULL]);

    for (x = 0; x < TankMaxX; x++)
    {
        for (y = 0; y < TankMaxY; y++)
        {
            tank_draw_point(x, y, TANK_WAR_MAP[x][y].col, TANK_WAR_MAP[x][y].pr);
        }
    }

    if (TANK_PR.debug)
    {
        tank_debug_out();
    }

    gdi_update();
    return RTN_OK;
}

//往地图上添加一个形状
int32 tank_set_warmap(point_t *point, int32 pr)
{
    //pr属性用于控制显示的形状
    int32 x = 0, y = 0;

    if (point == NULL)
        return RTN_ERR;

    x = point->x;
    y = point->y;

    x = ((x < 0) ? 0 : ((x >= TankMaxX) ? (TankMaxX - 1) : x));
    y = ((y < 0) ? 0 : ((y >= TankMaxY) ? (TankMaxY - 1) : y));

    pr = ((pr >= PR_MAX) ? PR_NULL : ((pr <= PR_NULL) ? PR_NULL : pr));

    //TANK_WAR_MAP[x][y].col = TANK_PR_COLOUR[pr];;
    TANK_WAR_MAP[x][y].col = point->col;
    TANK_WAR_MAP[x][y].pr = pr;

    return RTN_OK;
}

//从地图上获取一个形状
int32 tank_get_warmap(point_t *point)
{
    int32 x = 0, y = 0;
    int32 col = 0, pr = 0;
    int32 ret = PR_NULL;

    if (point == RTN_NULL)
    {
        return RTN_ERR;
    }

    x = point->x;
    y = point->y;

    x = ((x < 0) ? 0 : ((x >= TankMaxX) ? (TankMaxX - 1) : x));
    y = ((y < 0) ? 0 : ((y >= TankMaxY) ? (TankMaxY - 1) : y));

    return TANK_WAR_MAP[x][y].pr;
}

//rand for [min, max)
int32 tank_get_rand(int32 min, int32 max)
{
    static int32 seed = 0;
    if (seed == 0)
    {
        seed = GetTickCount();
        srand(seed);
    }

    return (min + (rand() % max));
}


//获取一个随机的坐标点
void *tank_get_randpnt(point_t *t_point)
{
    if (t_point == RTN_NULL)
    {
        return RTN_NULL;
    }

    t_point->x = tank_get_rand(0, TankMaxX - 3);
    t_point->y = tank_get_rand(0, TankMaxY - 3);

    return t_point;
}


//根据方向获取下一个点的坐标
point_t* tank_get_nextpnt(dir_t dir, point_t* t_point)
{
    int32 dx = 0, dy = 0;

    if (t_point == NULL)
    {
        return RTN_NULL;
    }

    dir = (dir <= DIR_MIN) ? DIR_MIN : ((dir >= DIR_MAX) ? (DIR_MAX - 1) : dir);

    switch (dir)
    {
    case DIR_UP:
        dx = 0; dy = -1;
        break;
    case DIR_DOWN:
        dx = 0; dy = 1;
        break;
    case DIR_LEFT:
        dx = -1; dy = 0;
        break;
    case DIR_RIGHT:
        dx = 1; dy = 0;
        break;
    default:
        break;
    }

    t_point->x += dx;
    t_point->y += dy;

    return t_point;
}

//制造一辆坦克
tank_t* tank_create_atank(tank_t* tank, int32 pr)
{
    point_t pnt = { 0 };
    int32 n = TankMaxX * TankMaxY;
    int32 i = 0;

    if (tank == RTN_NULL)
    {
        return RTN_NULL;
    }

    memset(tank, 0, sizeof(tank_t));
    tank->valid = TRUE;
    tank->pr = PR_NULL;
    tank->dir = tank_get_rand(DIR_MIN, DIR_MAX);//获取一个随机的方向
    tank->wep = WP_NONE;
    tank->lf = LF_LIVE;
    tank->kill = 0;
    tank->bomb = 0;

    if (pr == PR_MYSELF)//我军
    {
        tank->mv = MV_STOP;//默认不可移动,（手动操控）
        tank->fire = 1;
        tank->pr = PR_MYSELF;
    }
    else//敌军
    {
        tank->mv = tank_get_rand(MV_MOVE, min(TankMaxX, TankMaxY));//产生一个随机的移动步数
        tank->fire = tank_get_rand(TANK_TIMER_FIRE_MIN, TANK_TIMER_FIRE_MAX);
        tank->pr = PR_ENMY;
    }

    while (n--)//寻找可以放置坦克的随机点
    {
        tank_get_randpnt(&(tank->pnt));//生成一个随机点

        for (i = 0; i < TANK_SHAPE_PNT_MAX; i++)
        {
            pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][i].x;
            pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][i].y;
            if (tank_get_warmap(&pnt) != PR_NULL)
            {
                break;
            }
        }

        if (i >= TANK_SHAPE_PNT_MAX)
        {
            tank->pnt.col = TANK_PR_COLOUR[tank->pr];
            return tank;//该位置可以放下一坦克
        }
    }

    memset(tank, 0, sizeof(tank_t));
    tank->pr = PR_NULL;
    tank->valid = FALSE;

    return RTN_NULL;
}

//在地图上绘制坦克
int32 tank_draw_atank(tank_t* tank)
{
    int32 i = 0;
    point_t pnt = { 0 };
    int32 pr = 0;

    if (tank == NULL)
    {
        return RTN_ERR;
    }

    if (tank->valid == FALSE)
    {
        return RTN_ERR;
    }

    for (i = 0; i < TANK_SHAPE_PNT_MAX; i++)
    {
        pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][i].x;
        pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][i].y;
        pnt.col = tank->pnt.col;
        tank_set_warmap(&pnt, tank->pr);
    }

    return RTN_OK;
}

//检查坦克是否能够继续移动
int32 tank_check_atank(tank_t* tank)
{
    int32 i = 0, pr = 0;
    point_t pnt = { 0 };
    point_t* ppnt = NULL;

    if (tank == NULL)
    {
        return FALSE;
    }

    if ((tank->pr == PR_NULL) || (tank->valid == FALSE))
    {
        return FALSE;
    }

    //坦克不能越界
    if ((tank->pnt.x < 0) || (tank->pnt.x >= TankMaxX) ||
        (tank->pnt.y < 0) || (tank->pnt.y >= TankMaxY))
    {
        return FALSE;
    }

    for (i = 0; i < /*TANK_SHAPE_PNT_MAX*/3; i++)//只需要检查前三个点能否移动
    {
        pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][i].x;
        pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][i].y;

        ppnt = tank_get_nextpnt(tank->dir, &pnt);

        if (ppnt == RTN_NULL)
        {
            return FALSE;
        }

        //坦克不能越界
        if ((ppnt->x < 0) || (ppnt->x >= TankMaxX) ||
            (ppnt->y < 0) || (ppnt->y >= TankMaxY))
        {
            return FALSE;
        }

        //坦克的下一个位置必须不是坦克或墙
        pr = tank_get_warmap(ppnt);
        if ((pr == PR_WALL) || (pr == PR_MYSELF) || (pr == PR_ENMY))
        {
            return FALSE;
        }
    }

    return TRUE;
}

#if 0
//在地图上搜索坦克
tank_t* tank_search_atank_inmap(point_t* point, tank_t* tank)
{
    int32 i = 0;
    point_t pnt = { 0 };
    int32 pr = 0, dr = 0, j = 0;

    if (point == NULL)
    {
        return RTN_NULL;
    }

    pr = tank_get_warmap(point);

    if ((pr != PR_ENMY) && (pr != PR_MYSELF))
    {
        memset(tank, 0, sizeof(tank_t));
        tank->pr = PR_NULL;
        return RTN_NULL;//这个点不存在坦克
    }

    tank->pr = pr;

    //假如这个点在坦克上
    for (dr = DIR_MIN; dr < DIR_MAX; dr++)
    {//分别假设坦克的方向为四个方向
        for (i = 0; i < TANK_SHAPE_PNT_MAX; i++)
        {//分别假设这个点在坦克的六个点中的任意一个点上

            //计算出坦克基点的坐标
            tank->dir = dr;
            tank->pnt.x = point->x - TANK_SHAPE_BOX[tank->dir][i].x;
            tank->pnt.y = point->y - TANK_SHAPE_BOX[tank->dir][i].y;

            if ((tank->pnt.x < 0) || (tank->pnt.x >= TankMaxX) ||
                (tank->pnt.y < 0) || (tank->pnt.y >= TankMaxY))
            {
                //基点坐标在界外
                memset(tank, 0, sizeof(tank_t));
                tank->pr = PR_NULL;
                return RTN_NULL;
            }

            for (j = 0; j < TANK_SHAPE_PNT_MAX; j++)
            {//然后搜索其他点的位置是否正确
                pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][j].x;
                pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][j].y;
                if (tank_get_warmap(&pnt) != pr)
                {
                    break;
                }
            }

            if (j >= TANK_SHAPE_PNT_MAX)
            {
                //可能会出现某个点上存在多辆坦克（比如三辆以上的坦克挤在一起）
                //这种情况不考虑
                return tank;//搜索到一辆坦克
            }
        }
    }

    memset(tank, 0, sizeof(tank_t));
    tank->pr = PR_NULL;
    return RTN_NULL;
}

//在兵工厂中搜索坦克(根据坐标)
tank_t* tank_search_atank_inbox(point_t* point)
{
    tank_t* tank = { 0 };
    point_t pnt = { 0 };
    //int32 pr = 0;
    int32 i = 0, j = 0;

    if (point == NULL)
    {
        return RTN_NULL;
    }

    if ((point->x < 0) || (point->x >= TankMaxX) ||
        (point->y < 0) || (point->y >= TankMaxY))
    {
        return RTN_NULL;
    }

    //pr = tank_get_warmap(point);

    //if ((pr != PR_ENMY) && (pr != PR_MYSELF))
    //if (pr == PR_NULL)
    //{
    //    return RTN_NULL;//这个点不存在坦克
    //}

    //如果这个点存在坦克，就在box中搜索坦克信息
    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        //if (TANK_WAR_BOX_VALID[i] == 0)//过滤无效的元素
        if (TANK_WAR_BOX[i].pr == PR_NULL)//过滤无效的元素
        {
            continue;
        }

        tank = &(TANK_WAR_BOX[i]);
        for (j = 0; j < TANK_SHAPE_PNT_MAX; j++)
        {
            pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][j].x;
            pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][j].y;
            if ((point->x == pnt.x) && (point->y == pnt.y))
            {
                return tank;
            }
    }
}

    return RTN_NULL;
}
#endif

//制造一颗炮弹
bomb_t* tank_create_abomb(tank_t* tank, bomb_t* bomb, int32* bnum)
{
    int32 i = 0;
    point_t pnt = { 0 };
    point_t* ppnt = NULL;
    if ((tank == NULL) || (bomb == NULL) || (bnum == NULL))
    {
        return RTN_NULL;
    }

    if (tank->pr == PR_NULL)
    {
        return RTN_NULL;
    }

    if (tank->wep > WP_NONE)//加强版武器，每次发射三颗炮弹
    {
        //tank->wep -= 1;//武器使用一次
        tank->bomb += 3;
        for (i = 0; i < 3; i++)
        {
            *bnum = i + 1;
            pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][i].x;
            pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][i].y;
            ppnt = tank_get_nextpnt(tank->dir, &pnt);
            if (ppnt == RTN_NULL)
            {
                *bnum = 0;
                memset(&(bomb[i]), 0, sizeof(bomb_t));
                bomb[i].valid = FALSE;
                bomb[i].pr = PR_NULL;
                return RTN_NULL;
            }

            bomb[i].valid = TRUE;
            bomb[i].dir = tank->dir;
            bomb[i].pr = tank->pr;
            bomb[i].ptank = tank;
            ppnt->col = TANK_PR_COLOUR[PR_BOMB];
            COPY_POINT(&(bomb[i].pnt), ppnt);
        }
    }
    else//普通武器，每次发射一颗炮弹
    {
        tank->bomb += 1;
        *bnum = 1;
        pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][0].x;
        pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][0].y;
        ppnt = tank_get_nextpnt(tank->dir, &pnt);
        if (ppnt == RTN_NULL)
        {
            *bnum = 0;
            memset(&(bomb[0]), 0, sizeof(bomb_t));
            bomb[0].valid = FALSE;
            bomb[0].pr = PR_NULL;
            return RTN_NULL;
        }

        bomb[0].valid = TRUE;
        bomb[0].dir = tank->dir;
        bomb[0].pr = tank->pr;
        bomb[0].ptank = tank;
        ppnt->col = TANK_PR_COLOUR[PR_BOMB];
        COPY_POINT(&(bomb[0].pnt), ppnt);
    }

    return bomb;
}


//在地图上绘制炮弹
int32 tank_draw_abomb(bomb_t* bomb)
{
    int32 pr = 0;

    if (bomb == NULL)
    {
        return RTN_ERR;
    }

    if (bomb->valid == FALSE)
    {
        return RTN_ERR;
    }

    //炮弹也分为敌方炮弹和我方炮弹,用pr区分
    //但是不管敌方炮弹还是我方炮弹,显示的形状都
    //是一样的,都是(黄色)圆形,这里要注意区分
    //pr用于控制显示的形状
    pr = ((bomb->pr != PR_NULL) ? PR_BOMB : PR_NULL);
    tank_set_warmap(&(bomb->pnt), pr);

    return RTN_OK;
}

//检查炮弹能否继续移动
int32 tank_check_abomb(bomb_t* bomb)
{
    //int32 i = 0;
    //point_t pnt = { 0 };
    //point_t* ppnt = NULL;

    if (bomb == NULL)
    {
        return FALSE;
    }

    if ((bomb->pr == PR_NULL) || (bomb->valid == FALSE))
    {
        return FALSE;
    }

    if ((bomb->pnt.x < 0) || (bomb->pnt.x >= TankMaxX) ||
        (bomb->pnt.y < 0) || (bomb->pnt.y >= TankMaxY))
    {
        return FALSE;
    }

#if 0
    pnt.x = bomb->pnt.x;
    pnt.y = bomb->pnt.y;

    ppnt = tank_get_nextpnt(bomb->dir, &pnt);

    if (ppnt == RTN_NULL)
    {
        return FALSE;
    }

    if ((ppnt->x < 0) || (ppnt->x >= TankMaxX) ||
        (ppnt->y < 0) || (ppnt->y >= TankMaxY))
    {
        return FALSE;
    }
#endif

    return TRUE;
}

//在弹药库中查照炮弹(根据坐标)
bomb_t* tank_search_abomb_inbox(point_t* point)
{
    int32 i = 0;
    if (point == NULL)
    {
        return RTN_NULL;
    }

    if ((point->x < 0) || (point->x >= TankMaxX) ||
        (point->y < 0) || (point->y >= TankMaxY))
    {
        return RTN_NULL;
    }

    for (i = 0; i < TANK_BOMB_NUM_MAX; i++)
    {
        if ((TANK_BOMB_BOX[i].pr == PR_NULL) ||
            (TANK_BOMB_BOX[i].valid == FALSE))//过滤无效的元素
        {
            continue;
        }

        if ((point->x == TANK_BOMB_BOX[i].pnt.x) &&
            (point->y == TANK_BOMB_BOX[i].pnt.y))
        {
            return &(TANK_BOMB_BOX[i]);
        }
    }

    return RTN_NULL;
}

//创建一个装备
equip_t* tank_create_aequip(equip_t* equip)
{
    point_t pnt = { 0 };
    int32 n = TankMaxX * TankMaxY;
    int32 i = 0;

    if (equip == RTN_NULL)
    {
        return RTN_NULL;
    }

    equip->tmr = tank_get_rand(TANK_TIMER_EQUIP_MIN, TANK_TIMER_EQUIP_MAX);
    equip->pr = tank_get_rand(TANK_TIMER_MIN, TANK_TIMER_MAX);
    equip->valid = TRUE;
    if ((equip->pr % 2) == 0)
    {
        equip->pr = PR_LIFE;
    }
    else
    {
        equip->pr = PR_WEAPON;
    }

    while (n--)//寻找可以放置装备的随机点
    {
        tank_get_randpnt(&(equip->pnt));//生成一个随机点

        pnt.x = equip->pnt.x;
        pnt.y = equip->pnt.y;
        if (tank_get_warmap(&pnt) == PR_NULL)
        {
            equip->pnt.col = TANK_PR_COLOUR[equip->pr];
            return equip;
        }
    }

    memset(equip, 0, sizeof(equip_t));
    equip->valid = FALSE;
    equip->pr = PR_NULL;

    return RTN_NULL;
}

//在地图上绘制装备
uint32 tank_draw_aequip(equip_t* equip)
{
    point_t pnt = { 0 };
    int32 n = TankMaxX * TankMaxY;
    int32 i = 0;

    if (equip == RTN_NULL)
    {
        return RTN_ERR;
    }

    if (equip->valid == FALSE)
    {
        return RTN_ERR;
    }

    tank_set_warmap(&(equip->pnt), equip->pr);

    return RTN_OK;
}

//坦克移动
int32 tank_move_atank(dir_t dir)
{
    int32 i = 0;

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        //if (TANK_WAR_BOX_VALID[i] == 0)//过滤无效的元素
        if ((TANK_WAR_BOX[i].pr == PR_NULL) ||
            (TANK_WAR_BOX[i].valid == FALSE))
        {
            continue;
        }

        if (TANK_WAR_BOX[i].pr == PR_ENMY)//敌军
        {
            if (TANK_WAR_BOX[i].mv > MV_STOP)//步数还未用完
            {//继续移动
                TANK_WAR_BOX[i].mv -= 1;
                if (tank_check_atank(&(TANK_WAR_BOX[i])) != FALSE)
                {//还能继续移动
                    tank_get_nextpnt(TANK_WAR_BOX[i].dir, &(TANK_WAR_BOX[i].pnt));//坦克移动一步
                }
                else
                {//不能继续移动，调转方向
                    TANK_WAR_BOX[i].dir = tank_get_rand(DIR_MIN, DIR_MAX);//获取一个随机的方向
                }
            }
            else
            {//调转方向
                TANK_WAR_BOX[i].dir = tank_get_rand(DIR_MIN, DIR_MAX);//获取一个随机的方向
                TANK_WAR_BOX[i].mv = tank_get_rand(MV_MOVE, min(TankMaxX, TankMaxY));//产生一个随机的移动步数
            }
        }
        else if (TANK_WAR_BOX[i].pr == PR_MYSELF)//我军
        {
            if (dir < DIR_MAX)
            {//移动
                if (TANK_WAR_BOX[i].dir == dir)
                {//继续移动
                    if (tank_check_atank(&(TANK_WAR_BOX[i])) != FALSE)
                    {//还能继续移动
                        tank_get_nextpnt(TANK_WAR_BOX[i].dir, &(TANK_WAR_BOX[i].pnt));//坦克移动一步
                    }
                }
                else
                {//调转方向
                    TANK_WAR_BOX[i].dir = dir;
                }
            }
        }
    }

    return RTN_OK;
}


//炮弹移动
int32 tank_move_abomb(void)
{
    int32 i = 0;
    //point_t pnt = { 0 };
    //tank_t* ptank = NULL;

    for (i = 0; i < TANK_BOMB_NUM_MAX; i++)
    {
        //if (TANK_BOMB_BOX_VALID[i] == 0)//过滤无效的元素
        if ((TANK_BOMB_BOX[i].pr == PR_NULL) ||
            (TANK_BOMB_BOX[i].valid == FALSE))
        {
            continue;
        }

        if (tank_check_abomb(&(TANK_BOMB_BOX[i])) == FALSE)
        {//不能继续移动
            TANK_BOMB_BOX[i].pr = PR_NULL;
        }
        else
        {//继续移动
            //COPY_POINT(&pnt, &(TANK_BOMB_BOX[i].pnt));
            //ptank = tank_search_atank_inbox(&pnt);
            //if (ptank != RTN_NULL)
            //{
                //tank_set_warmap(&pnt, ptank->pr);
                //tank_draw_atank(ptank);
            //}
            tank_get_nextpnt(TANK_BOMB_BOX[i].dir, &(TANK_BOMB_BOX[i].pnt));
        }
    }

    return RTN_OK;
}

//坦克大战装备系统
int32 tank_move_equip(void)
{
    equip_t equip = { 0 };
    //产生装备
    if (TANK_PR.create_equp_tmr > 0)
    {
        //TANK_EQUIP.pr = PR_NULL;
        //TANK_EQUIP.valid = FALSE;
        TANK_PR.create_equp_tmr -= 1;
    }
    else
    {
        if (tank_create_aequip(&equip) != RTN_NULL)
        {
            memcpy(&TANK_EQUIP, &equip, sizeof(equip_t));//产生新的
        }

        TANK_PR.create_equp_tmr = tank_get_rand(TANK_TIMER_EQUIP_MIN, TANK_TIMER_EQUIP_MAX);//重启生成定时器
    }

    if ((TANK_EQUIP.pr != PR_NULL) && (TANK_EQUIP.valid != FALSE))
    {
        if (TANK_EQUIP.tmr > 0)
        {
            TANK_EQUIP.tmr -= 1;
        }
        else
        {
            TANK_EQUIP.pr = PR_NULL;
            TANK_EQUIP.valid = FALSE;
            TANK_PR.create_equp_tmr = tank_get_rand(TANK_TIMER_EQUIP_MIN, TANK_TIMER_EQUIP_MAX);//重启生成定时器
        }
    }

    //DEBUG_LOG("TANK_PR.create_equp_tmr[%d], TANK_EQUIP.tmr[%d], TANK_EQUIP.pr[%d]",
    //    TANK_PR.create_equp_tmr, TANK_EQUIP.tmr, TANK_EQUIP.pr);

    return RTN_OK;
}

int32 tank_sound(int32 sid)
{
    if (sid)
    {//start
        PlaySound(MAKEINTRESOURCE(sid), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
    }
    else
    {//stop
        PlaySound(NULL, 0, 0);
    }
    return RTN_OK;
}

//坦克初始化
int32 tank_init(void)
{
    int32 i = 0;
    int32 pr = 0;

    tank_clear_warmap();

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        memset(&(TANK_WAR_BOX[i]), 0, sizeof(tank_t));
        TANK_WAR_BOX[i].pr = PR_NULL;
        //TANK_WAR_BOX_VALID[i] = FALSE;
        TANK_WAR_BOX[i].valid = FALSE;
    }

    for (i = 0; i < TANK_BOMB_NUM_MAX; i++)
    {
        memset(&(TANK_BOMB_BOX[i]), 0, sizeof(bomb_t));
        TANK_BOMB_BOX[i].pr = PR_NULL;
        //TANK_BOMB_BOX_VALID[i] = FALSE;
        TANK_BOMB_BOX[i].valid = FALSE;
    }

    memset(&(TANK_PR), 0, sizeof(tank_pr_t));
    memset(&(TANK_EQUIP), 0, sizeof(equip_t));
    TANK_EQUIP.pr = PR_NULL;
    TANK_EQUIP.valid = FALSE;

    for (i = 0; i < TANK_ENMY_NUM_INIT; i++)
    {
        if (i == 0)
        {
            pr = PR_MYSELF;
        }
        else
        {
            pr = PR_ENMY;
        }

        if (tank_create_atank(&(TANK_WAR_BOX[i]), pr) == RTN_NULL)
        {
            return RTN_ERR;
        }

        TANK_PR.cur_tank_num++;
    }

    TANK_PR.war_tank_num = TANK_PR.cur_tank_num;
    //TANK_PR.cur_bomb_num = 0;
    TANK_PR.create_equp_tmr = tank_get_rand(TANK_TIMER_EQUIP_MIN, TANK_TIMER_EQUIP_MAX);
    TANK_PR.speed = TANK_SPEED_MAX;
    TANK_PR.debug = FALSE;
    TANK_PR.super = FALSE;

    tank_update_warmap();

    tank_sound(IDR_WAVE_START);

    return RTN_OK;
}

//坦克移动侦测(碰撞检测)
int32 tank_detect(tank_t* tank)
{
    int32 i = 0, pr = 0;
    point_t pnt = { 0 };
    bomb_t* bomb = NULL;

    if (tank == NULL)
    {
        return RTN_ERR;
    }

    if ((tank->pr == PR_NULL) || (tank->valid == FALSE))
    {
        return RTN_OK;
    }

    for (i = 0; i < TANK_SHAPE_PNT_MAX; i++)
    {
        pnt.x = tank->pnt.x + TANK_SHAPE_BOX[tank->dir][i].x;
        pnt.y = tank->pnt.y + TANK_SHAPE_BOX[tank->dir][i].y;

        pr = tank_get_warmap(&(pnt));
        switch (pr)
        {
        case PR_WEAPON://吃到武器
            tank->wep += TANK_WEAPON_LIFE_MAX;//增加武器可以使用次数
            TANK_EQUIP.tmr = 0;//装备消失
            TANK_EQUIP.pr = PR_NULL;
            tank_sound(IDR_WAVE_WEAPON);
            break;
        case PR_LIFE://吃到装备
            tank->lf += 1;//吃到装备，生命值+1
            TANK_EQUIP.tmr = 0;//装备消失
            TANK_EQUIP.pr = PR_NULL;
            tank_sound(IDR_WAVE_LIFE);
            break;
        case PR_BOMB://吃到炮弹
            bomb = tank_search_abomb_inbox(&(pnt));
            if (bomb == RTN_NULL)
            {
                break;
            }
            if (bomb->pr != tank->pr)//自己的炮弹不能炸自己
            {
                if (tank->lf > LF_LIVE)
                {
                    tank->lf -= 1;//生命值-1
                }
                if (tank->lf = LF_LIVE)
                {
                    tank->lf -= 1;//生命值-1
                    bomb->ptank->kill += 1;//为这颗炮弹的主人增加一次击杀记录
                }
                bomb->pr = PR_NULL;//击中目标的炮弹失效
                tank->wep = WP_NONE;//坦克被击中之后其武器失效
                tank_sound(IDR_WAVE_BOMB);
            }
            break;
        case PR_NULL:
        case PR_WALL:
        case PR_ENMY:
        case PR_MYSELF:
        default:
            break;
        }
    }

    return RTN_OK;
}

int32 tank_count(void)
{
    int32 i = 0;
    TANK_PR.cur_tank_num = 0;
    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        if ((TANK_WAR_BOX[i].pr != PR_NULL) &&
            (TANK_WAR_BOX[i].valid != FALSE))
        {
            TANK_PR.cur_tank_num += 1;//统计坦克的数目
        }
    }

    return RTN_OK;
}

#if 0
int32 tank_cbomb(void)
{
    TANK_PR.cur_bomb_num = 0;//统计炮弹数目
    for (i = 0; i < TANK_BOMB_NUM_MAX; i++)
    {
        if ((TANK_BOMB_BOX[i].pr != PR_NULL) &&
            (TANK_BOMB_BOX[i].valid != FALSE))
        {
            TANK_PR.cur_bomb_num += 1;//统计炮弹数目
        }
    }
}
#endif

//清理战场
int32 tank_clean(tank_t* tank)
{
    int32 i = 0, j = 0, k = 0;

    static int8 war_num_flag = FALSE;

    tank_t ttank = { 0 };
    tank_t* ptank = NULL;

    if (tank == NULL)
    {
        return RTN_ERR;
    }

    if ((tank->pr == PR_NULL) || (tank->valid == FALSE))
    {
        return RTN_OK;
    }

    //打扫战场
    if (tank->lf == LF_DIE)
    {//清理掉已经炸毁的坦克

        if (tank->pr == PR_MYSELF)
        {//我军战败
            tank->pr = PR_NULL;
            //DEBUG_LOG("");
            return RTN_ERR;
        }

        if (tank->pr == PR_ENMY)
        {//敌军损毁一辆坦克
           //销毁一辆损毁的坦克
            tank->pr = PR_NULL;
#if 0
            //增援一辆新坦克
            ptank = tank_create_atank(&ttank, PR_ENMY);
            if (ptank != RTN_NULL)
            {
                //寻找位置，把新坦克插入到队列中
                for (j = 0; j < TANK_WAR_NUM_MAX; j++)
                {
#if 0
                    if ((ptank->pnt.x == tank->pnt.x) &&
                        (ptank->pnt.y == tank->pnt.y))
                    {//新位置不能在刚损毁的那个位置上
                        continue;
                    }

                    if ((TANK_WAR_BOX[j].pr == PR_NULL) ||
                        (TANK_WAR_BOX[j].valid == FALSE))
#else
                    if (TANK_WAR_BOX[j].valid == FALSE)
#endif
                    {
                        memcpy(&(TANK_WAR_BOX[j]), ptank, sizeof(tank_t));
                        break;
                    }
    }
}
#endif
}
    }

    if (tank->lf == LF_BURN)
    {//把正在燃烧的坦克标记为炸毁，下次清理
        tank->lf = LF_DIE;
        tank->mv = MV_STOP;//停止移动
        //tank->pr = PR_BOMB;//将其属性改为炮弹，表示即将爆炸，其他坦克撞到它也会失去一颗生命值
        tank->pnt.col = TANK_PR_COLOUR[PR_BOMB];//将其颜色改为和炮弹同色
    }

#if 1
#if 1
    //我军每击杀5辆坦克敌军坦克数量+1
    if ((tank->pr == PR_MYSELF) && (tank->kill > 0))
    {
        if ((tank->kill % 5) == 0)
        {
            if (war_num_flag == FALSE)
            {
                war_num_flag = TRUE;
                TANK_PR.war_tank_num += 1;
            }
        }
        else
        {
            war_num_flag = FALSE;
        }
    }
#endif

    tank_count();//统计坦克的数目

    //增援新坦克
    k = 0;
    for (i = TANK_PR.cur_tank_num; i < TANK_PR.war_tank_num; i++)
    {
        ptank = tank_create_atank(&ttank, PR_ENMY);
        if (ptank != RTN_NULL)
        {
            //寻找位置，把新坦克插入到队列中
            for (j = k; j < TANK_WAR_NUM_MAX; j++)
            {
                if ((TANK_WAR_BOX[j].pr == PR_NULL) &&
                    (TANK_WAR_BOX[j].valid == FALSE))
                {
                    k = j + 1;
                    memcpy(&(TANK_WAR_BOX[j]), ptank, sizeof(tank_t));
                    break;
                }
            }
        }
    }

#endif
    return RTN_OK;
}

//坦克开火
int32 tank_fire(tank_t* tank, int32* fire)
{
    int32 m = 0, k = 0, j = 0;

    int32 bnum = 0;
    bomb_t bomb[3] = { 0 };

    if ((tank == NULL) || (fire == NULL))
    {
        return RTN_ERR;
    }

    if ((tank->pr == PR_NULL) || (tank->valid == FALSE))
    {
        return RTN_OK;
    }

    if (tank->pr == PR_MYSELF)
    {
        //我军开火方式由手动控制
        if (*fire)
        {
            *fire = FALSE;
            tank->fire = 0;
        }
        else
        {
            tank->fire = 1;
        }
    }

    //开火倒计时
    if (tank->fire > 0)
    {
        tank->fire -= 1;
    }
    else
    {
        tank->fire = tank_get_rand(TANK_TIMER_FIRE_MIN, TANK_TIMER_FIRE_MAX);
        //坦克发射炮弹(先产生炮弹，放入队列中)
        if (tank_create_abomb(tank, bomb, &bnum) != RTN_NULL)
        {
            m = 0;
            //把炮弹插入到炮弹队列中
            for (j = 0; j < bnum; j++)
            {
                //寻找可以插入炮弹的位置
                for (k = m; k < TANK_BOMB_NUM_MAX; k++)
                {
                    if ((TANK_BOMB_BOX[k].pr == PR_NULL) ||
                        (TANK_BOMB_BOX[k].valid == FALSE))
                    {
                        //插入炮弹
                        m = k + 1;
                        memcpy(&(TANK_BOMB_BOX[k]), &bomb[j], sizeof(bomb_t));
                        break;
                    }
                }
            }

            if (tank->pr == PR_MYSELF)
            {
                tank_sound(IDR_WAVE_FIRE);
            }
        }
    }

    return RTN_OK;
}

void tank_draw(void)
{
    int32 i = 0;

    tank_clear_warmap();

    //绘制坦克
    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        if (TANK_WAR_BOX[i].valid == FALSE)
        {
            continue;
        }

        tank_draw_atank(&(TANK_WAR_BOX[i]));
        if (TANK_WAR_BOX[i].pr == PR_NULL)
        {
            memset(&(TANK_WAR_BOX[i]), 0, sizeof(tank_t));
            TANK_WAR_BOX[i].pr = PR_NULL;
            TANK_WAR_BOX[i].valid = FALSE;
        }
    }

    //绘制装备
    if (TANK_EQUIP.valid != FALSE)
    {
        tank_draw_aequip(&(TANK_EQUIP));
        if (TANK_EQUIP.pr == PR_NULL)
        {
            memset(&(TANK_EQUIP), 0, sizeof(equip_t));
            TANK_EQUIP.pr = PR_NULL;
            TANK_EQUIP.valid = FALSE;
        }
    }


    //绘制炮弹
    for (i = 0; i < TANK_BOMB_NUM_MAX; i++)
    {
        if (TANK_BOMB_BOX[i].valid == FALSE)
        {
            continue;
        }
        tank_draw_abomb(&(TANK_BOMB_BOX[i]));
        if (TANK_BOMB_BOX[i].pr == PR_NULL)
        {
            memset(&(TANK_BOMB_BOX[i]), 0, sizeof(bomb_t));
            TANK_BOMB_BOX[i].pr = PR_NULL;
            TANK_BOMB_BOX[i].valid = FALSE;
        }
    }

    tank_update_warmap();
}

int32 tank_run(dir_t* dir, int32* fire, int32 super, int32 debug)
{
    int32 i = 0, j = 0, k = 0, m = 0;
    equip_t equip = { 0 };
    tank_t tank = { 0 };
    tank_t* ptank = NULL;
    bomb_t bomb[3] = { 0 };
    int32 bnum = 0;
    static int32 speed = 0;
    int32 pr = 0;

    int32 ret = RTN_OK;

    TANK_PR.debug = (debug ? TRUE : FALSE);
    TANK_PR.super = (super ? TRUE : FALSE);

#if 0
    if (tnum != 0)
    {
        tnum = 0;
        tnum = (tnum <= TANK_ALL_NUM_MIN) ? TANK_ALL_NUM_MIN :
            ((tnum_t >= TANK_ALL_NUM_MIN) ? TANK_ALL_NUM_MIN : tnum);

        TANK_PR.war_tank_num = tnum;
    }
#endif

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        //无敌模式
        if (TANK_WAR_BOX[i].pr == PR_MYSELF)
        {
            tank_set_super(&(TANK_WAR_BOX[i]), TANK_PR.super);
        }

        //打扫战场
        ret = tank_clean(&(TANK_WAR_BOX[i]));
        if (ret != RTN_OK)
        {
            //DEBUG_LOG("ERR");
            return ret;
        }

        //坦克开火
        tank_fire(&(TANK_WAR_BOX[i]), fire);

        //碰撞检测
        tank_detect(&(TANK_WAR_BOX[i]));

    }

    //移动坦克,炮弹和装备
    if (speed < TANK_PR.speed)
    {
        speed++;//炮弹速度比坦克快,炮弹移动5补坦克移动一步
    }
    else
    {
        speed = 0;
        tank_move_atank(*dir);
        *dir = DIR_MAX;
    }

    tank_move_equip();
    tank_move_abomb();

    tank_draw();

    return RTN_OK;
    }
