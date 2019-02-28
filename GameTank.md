# 坦克大战-C语言-详注版

## 概述
本文详述了C语言版坦克大战游戏的原理以及实现方法,对游戏代码进行了详细的分析和注释,通过本文能够让大家对WIN32编程框架有一个大致了解,对C语言运用有一定提高,同时也能给大家提供一个C语言小游戏编程的思路,也能完全够通过自己的实力去编写一个属于自己的游戏.

> 游戏体验
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20180905135655.gif)

> 视频版:[坦克大战-C语言版-GameTank](https://v.youku.com/v_show/id_XMzgxMTc1MjI0OA==.html)


## 代码框架
坦克大战游戏代码框架如下
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20180905131328.png)
在 ***main.c*** 中,创建应用窗口,并初始化一些系统资源,然后初始化gdi,初始化坦克大战游戏.
在 ***gdi.c*** 中,对系统的HDC及绘图接口进行了一次封装,使用WIN32系统提供的绘图接口来实现我们自己的图形绘制API.这里使用到了双缓冲技术,下文会简单介绍.
在 ***Tank.c*** 中,实现了坦克大战的游戏逻辑,包括贴图系统,地图系统,坦克制造,炮弹制造,装备生成,坦克移动,炮弹移动,以及坦克被炮弹击中,坦克吃到装备,等等.
下面将对代码进行详细的分析.

## 代码主函数
在 ***main.c*** 中,创建应用窗口,并初始化一些系统资源,然后初始化gdi,初始化坦克大战游戏.
> main.c
```c
//主函数
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    //注册窗口类,创建窗口,初始化等: 
    MyRegisterClass(hInstance);
    InitInstance(hInstance, nCmdShow);

    //显示初始化及游戏初始化
    gdi_init(hWnd);
    tank_init();

    //创建坦克大战运行线程
    hTankRunT = CreateThread(NULL, 0, TankRun, NULL, 0, &dwTankRunTId);

    //快捷键
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMETANK));

    // 主消息循环: 
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            //TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

//目的: 注册窗口类。
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    ......
    wcex.lpfnWndProc = WndProc;//绑定窗口消息处理函数
	......

    return RegisterClassEx(&wcex);//注册窗口类
}

//目的: 保存实例句柄并创建主窗口
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 将实例句柄存储在全局变量中
    ......
    hWnd = CreateWindow(......);//无窗口创建数据

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//目的:    处理主窗口的消息。
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	......
    switch (message)
    {
    case WM_COMMAND:
    {
        // 分析菜单选择: 
        switch (wmId)
        {
        case IDM_ABOUT: //关于
            GAME_CTRL.run = FALSE;//点击对话框的时候暂停
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_README: //说明
            GAME_CTRL.run = FALSE;//点击对话框的时候暂停
            DialogBox(hInst, MAKEINTRESOURCE(IDD_READMEBOX), hWnd, Readme);
            break;
        case IDM_EXIT://退出
            DestroyWindow(hWnd);
            break;
        ......
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此处添加使用 hdc 的任何绘图代码...
        gdi_update();
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        gdi_dinit();
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_UP://上
            GAME_CTRL.dir = DIR_UP;
            break;
        case VK_DOWN://下
            GAME_CTRL.dir = DIR_DOWN;
            break;
        case VK_LEFT://左
            GAME_CTRL.dir = DIR_LEFT;
            break;
        case VK_RIGHT://右
            GAME_CTRL.dir = DIR_RIGHT;
            break;
        case VK_RETURN://回车键开火
            GAME_CTRL.fire = TRUE;
            break;
        ......
        default:
            break;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//游戏运行线程
DWORD WINAPI TankRun(LPVOID lpProgram)
{
    while (TRUE)
    {
        if (GAME_CTRL.run)//暂停游戏
        {
            if (GAME_CTRL.auto_fire != FALSE)
            {
                // GameTankDir = ((i++) % DIR_MAX);
                GAME_CTRL.fire = TRUE;
            }

            if (tank_run(&(GAME_CTRL.dir), &(GAME_CTRL.fire)/*, GAME_CTRL.difficult*/) != RTN_OK)
            {
                //DEBUG_LOG("ERR");
                MessageBox(/*NULL*/hWnd, TEXT("你输了!"), TEXT("你输了!"), MB_OK);
                GameCtrlInit();
                tank_init();
            }
            Sleep(1 + GAME_CTRL.speed);
        }
    }
    return 0;
}
```

## GDI绘图
在 ***gdi.c*** 中,对系统的HDC及绘图接口进行了一次封装,使用WIN32系统提供的绘图接口来实现我们自己的图形绘制API.这里使用到了双缓冲技术.
### 双缓冲技术
坦克大战的每一次运行,坦克和炮弹的每一次移动,我们都要不断的清除屏幕上原来的内容,然后重新绘制新的内容,假如我们每一次更改显示内容都直接调用WIN32的api实时的刷新到屏幕上,一旦游戏运行速度很快的时候,就会造成屏幕上的内容一直在闪烁,非常影响游戏体验.
双缓冲技术主要是为了解决坦克大战实时刷新造成的屏幕闪烁问题.其原理就是,加入地图上共有10辆坦在战斗,每辆坦克都在移动,每移动一步我们都需要重新计算并显示10辆坦克的位置,但是我们不必在计算每一辆坦克的时候都把他刷新到屏幕上,而是先把这辆坦克绘制到内存上,等到所有的坦克都计算并完成移动之后,再同一把内存上的内容刷新到屏幕上,这样做就大大减少了刷新屏幕的次数,也就可以避免实时刷新造成的屏幕闪烁问题.
更详细的介绍,请参考下面这篇文章:

> [双缓冲技术讲解](https://blog.csdn.net/jxw167/article/details/72157154)

> gdi.c

```c
#include "Gdi.h"

HPEN		hGdiPen = NULL;	        //画笔
HBRUSH		hGdiBrush = NULL;	    //画刷
HDC			mGdiHdc;	            //内存设备(双缓冲技术)
HDC         hGdiHdc;                //硬件设备
HWND        hGdiWnd;                //窗口
RECT        hGdiWndRect;            //窗口客户区大小

HBITMAP     mGdiBmp;
HBITMAP     mGdiBmpOld;

#define maxX                SCREEN_X
#define maxY                SCREEN_Y

static void _gdi_clr_pencol(HPEN _hGdiPen)
{
    DeleteObject(_hGdiPen);//释放资源
    SelectObject(mGdiHdc, hGdiPen);//恢复初始画刷
}

static HPEN _gdi_set_pencol(int32 color)
{
    HPEN _hGdiPen;
    COLORREF color_t = (COLORREF)color;
    _hGdiPen = CreatePen(PS_SOLID, 1, color_t);//画笔
    hGdiPen = SelectObject(mGdiHdc, _hGdiPen);//为缓存DC选择画笔
    return _hGdiPen;
}

static void _gdi_clr_brushcol(HBRUSH _hGdiBrush)
{
    DeleteObject(_hGdiBrush);//释放资源
    SelectObject(mGdiHdc, hGdiBrush);//恢复初始画刷
}

static HBRUSH _gdi_set_brushcol(int32 color)
{
    HBRUSH _hGdiBrush;

    COLORREF color_t = (COLORREF)color;
    _hGdiBrush = CreateSolidBrush(color_t);//画刷

    hGdiBrush = SelectObject(mGdiHdc, _hGdiBrush);//为缓存DC选择画刷
    return _hGdiBrush;
}

/*
 * gdi_clear:
 *	Clear the display to the given colour.
 *******************************************************************************
 */
void gdi_clear(int32 colour)
{
    gdi_rectangle(0, 0, maxX, maxY, colour, TRUE);

}

 /*
  * gdi_set_point:
  *	Plot a pixel.
  *******************************************************************************
  */
void gdi_set_point(int32 x, int32 y, int32 colour)
{
    x = ((x < 0) ? 0 : ((x > (maxX - 1)) ? (maxX - 1) : x));
    y = ((y < 0) ? 0 : ((y > (maxY - 1)) ? (maxY - 1) : y));

    HPEN hPen = _gdi_set_pencol(colour);
    SetPixel(mGdiHdc, x, y, colour);
    _gdi_clr_pencol(hPen);
}

/*
 * gdi_get_point:
 *	Plot a pixel.
 *******************************************************************************
 */
int32 gdi_get_point(int32 x, int32 y)
{
    x = ((x < 0) ? 0 : ((x > (maxX - 1)) ? (maxX - 1) : x));
    y = ((y < 0) ? 0 : ((y > (maxY - 1)) ? (maxY - 1) : y));

    COLORREF col = GetPixel(mGdiHdc, x, y);
    return (int32)col;
}

......

/*
* gdi_triangle:
*	A triangle is a spoilt days fishing
*******************************************************************************
*/
void gdi_triangle(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled)
{
    HPEN _hPen;
    HBRUSH _hBrush;
    POINT triangle[3] = { 0 };
    int32 halfx = 0;

    halfx = ((x2 - x1 + 1) / 2);

    triangle[0].x = x1 + halfx;
    triangle[0].y = y1;

    triangle[1].x = x1;
    triangle[1].y = y2;

    triangle[2].x = x2;
    triangle[2].y = y2;

    if (filled)
    {
        _hPen = _gdi_set_pencol(colour);
        _hBrush = _gdi_set_brushcol(colour);
        Polygon(mGdiHdc, triangle, 3);
        _gdi_clr_pencol(_hPen);
        _gdi_clr_brushcol(_hBrush);
    }
    else
    {
        _hPen = _gdi_set_pencol(colour);
        Polygon(mGdiHdc, triangle, 3);
        _gdi_clr_pencol(_hPen);
    }
}

......

/*
 * gdi_init:
 *	Initialise the display and GPIO.
 *******************************************************************************
 */
int32 gdi_init(HWND hWnd)
{
    int32 hGdiWndWidth = 0;//窗口客户区宽度
    int32 hGdiWndHeight = 0;//窗口客户区高度
    hGdiWnd = hWnd;
    hGdiHdc = GetDC(hGdiWnd);                     //获取硬件设备
    mGdiHdc = CreateCompatibleDC(hGdiHdc);        //创建软件设备,双缓冲技术
    GetClientRect(hGdiWnd, &hGdiWndRect);
    hGdiWndWidth = hGdiWndRect.right - hGdiWndRect.left;
    hGdiWndHeight = hGdiWndRect.bottom - hGdiWndRect.top;

	//双缓冲技术核心:先创建一个软件绘图设备HDC,为这个软件HDC选择一个内存画布,
	//所有的绘图都通过这个软件HDC来完成,都被绘制到了这个内存画布之上
    //当所有的绘图工作都完成之后,就通过BitBlt把内存画布上的内容拷贝到硬件绘图设备HDC上,
	//这样就完成了显示(gdi_update)
    mGdiBmp = CreateCompatibleBitmap(hGdiHdc, hGdiWndWidth, hGdiWndHeight);//创建BMP画布
    mGdiBmpOld = SelectObject(mGdiHdc, mGdiBmp);//为软件HDC设备选择BMP画布

    return OK;
}

int32 gdi_dinit(void)
{
    DeleteObject(mGdiBmp);//删除BMP画布
    DeleteObject(mGdiHdc);//删除软件设备
    DeleteDC(hGdiHdc);//删除硬件设备

    return OK;
}

int32 gdi_update(void)
{
    int32 hGdiWndWidth = 0;//窗口客户区宽度
    int32 hGdiWndHeight = 0;//窗口客户区高度
    hGdiWndWidth = hGdiWndRect.right - hGdiWndRect.left;
    hGdiWndHeight = hGdiWndRect.bottom - hGdiWndRect.top;
    //把软件设备上的内容拷贝到硬件设备上
    BitBlt(hGdiHdc, 0, 0, hGdiWndWidth, hGdiWndHeight, mGdiHdc, 0, 0, SRCCOPY);

    return OK;
}

```


## 坦克大战
### 坦克系统
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20180905142211.png)
如上图所示,每个坦克由6个小方块组成,把这6个小方块按顺序标号.每个坦克有上下左右四个方向.为了简化程序操作,把这四个方向的坦克放在一个(二维)数组 ***TANK_SHAPE_BOX*** 中,即TANK_SHAPE_BOX[4][6],4表示共有四种形状的坦克(四个方向),6表示每种坦克由6个小方块. 数组的每个元素保存的是这6个小方块的相对坐标.坦克的每个方块的实际坐标可以通过坦克左上角的坐标与这6个相对坐标计算得到.

```c
typedef enum property_e
{
    PR_MIN = 0,
    PR_NULL = 0,//无
    PR_WALL, //墙
    PR_WEAPON, //武器
    PR_LIFE, //装备
    PR_BOMB, //炮弹
    PR_MYSELF, //自己
    PR_ENMY, //敌人
    PR_MAX
} property_t; //属性

typedef struct point_s
{
    uint32_t x;
    uint32_t y;
    uint32_t col;
} point_t;//坐标点

typedef struct tank_s
{
    int8        valid;//是否有效
    dir_t       dir;//坦克方向，同时也是其在TANK_SHAPE_BOX中的索引
    point_t     pnt;//坦克左上角的坐标，其它点的相对坐标在TANK_SHAPE_BOX中
    property_t  pr;//属性,只能是 PR_MYSEL, PR_ENMY, PR_NULL 三个之一
    int32       lf;//生命值,正常情况>1, 当为1时表示被击毁(变为黄色,随后死亡), 当为0时表示死亡
    int32       wep;//武器,>0时表示使用超级武器,每次发射3颗炮弹
    int32       mv;//移动(步数)
    int32       fire;//开火倒计时,
    int32       kill;//击杀的敌军数量
} tank_t;

const point_t TANK_SHAPE_BOX[TANK_SHAPE_NUM_MAX][TANK_SHAPE_PNT_MAX]=
{//不论哪个方向的坦克，其车身点在数组中的位置都是固定的
{{1,0,TRUE},{0,1,TRUE},{2,1,TRUE},{1,1,TRUE},{0,2,TRUE},{2,2,TRUE}},//上
{{2,1,TRUE},{1,0,TRUE},{1,2,TRUE},{1,1,TRUE},{0,0,TRUE},{0,2,TRUE}},//右
{{1,2,TRUE},{2,1,TRUE},{0,1,TRUE},{1,1,TRUE},{2,0,TRUE},{0,0,TRUE}},//下
{{0,1,TRUE},{1,2,TRUE},{1,0,TRUE},{1,1,TRUE},{2,2,TRUE},{2,0,TRUE}}//左
};

//参战斗坦克(最大),坦克仓库,保存了所有坦克的信息,包括在地图上的坐标(只需要保存其左上角的坐标),敌我属性,武器属性,生命值,移动等等
static tank_t TANK_WAR_BOX[TANK_WAR_NUM_MAX]={0};

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
```

### 弹药系统
```
typedef struct bomb_s
{
    int8        valid;
    dir_t       dir;//炮弹飞行方向
    point_t     pnt;//炮弹的坐标点
    property_t  pr;//炮弹属性(敌人的炮弹还是自己的炮弹)
    tank_t*     ptank;//这颗炮弹是哪辆坦克发射的
} bomb_t;

static bomb_t   TANK_BOMB_BOX[TANK_BOMB_NUM_MAX] = { 0 };//同一时刻所有坦克发出的炮弹

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
```
### 装备系统
```c
typedef struct equip_s
{
    int8        valid;//装备是否有效
    point_t     pnt;//装备的坐标点
    property_t  pr;//装备属性(武器还是生命) 
    int32       tmr;//装备存活定时器，到期后装备消失
} equip_t;

static equip_t TANK_EQUIP = { 0 };//坦克的装备

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

//坦克大战装备系统
int32 tank_move_equip(void)
{
    equip_t equip = { 0 };
    //产生装备
    if (TANK_PR.create_equp_tmr > 0)
    {
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

    return RTN_OK;
}
```

### 移动坦克和炮弹
```c
//坦克移动
int32 tank_move_atank(dir_t dir)
{
    int32 i = 0;

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        if ((TANK_WAR_BOX[i].pr == PR_NULL) || //过滤无效的元素
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
            tank_get_nextpnt(TANK_BOMB_BOX[i].dir, &(TANK_BOMB_BOX[i].pnt));
        }
    }

    return RTN_OK;
}
```
### 碰撞检测
```c
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
```
### 清理战场
```c
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
            tank->pr = PR_NULL;}
    }

    if (tank->lf == LF_BURN)
    {//把正在燃烧的坦克标记为炸毁，下次清理
        tank->lf = LF_DIE;
        tank->mv = MV_STOP;//停止移动
        //tank->pr = PR_BOMB;//将其属性改为炮弹，表示即将爆炸，其他坦克撞到它也会失去一颗生命值
        tank->pnt.col = TANK_PR_COLOUR[PR_BOMB];//将其颜色改为和炮弹同色
    }

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

    return RTN_OK;
}
```
### 坦克开火
```c
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
```

### 显示系统
```c
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
```
### 坦克运行
```c
int32 tank_run(dir_t* dir, int32* fire)
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

    for (i = 0; i < TANK_WAR_NUM_MAX; i++)
    {
        //打扫战场
        ret = tank_clean(&(TANK_WAR_BOX[i]));
        if (ret != RTN_OK)
        {//我军被击败,游戏结束
            //DEBUG_LOG("ERR");
            return ret;
        }

        tank_fire(&(TANK_WAR_BOX[i]), fire);//坦克开火
        
        tank_detect(&(TANK_WAR_BOX[i]));//碰撞检测
    }

    //移动坦克,炮弹和装备
    if (speed < TANK_PR.speed)
    {
        speed++;
    }
    else
    {
        speed = 0;
        tank_move_atank(*dir);
        *dir = DIR_MAX;
    }
    tank_move_equip();
    tank_move_abomb();

    tank_draw();//更新图像界面

    return RTN_OK;
}
```
## 程序运行截图
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20181015114935.png)
## 项目文件截图
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20181015114759.png)
