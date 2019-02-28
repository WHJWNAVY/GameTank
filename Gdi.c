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
#if 0
    _hGdiPen = CreatePen(PS_SOLID, 1, ((color) ? GDI_RGB_COL : GDI_RGB_BKCOL));//画笔
#else
    COLORREF color_t = (COLORREF)color;
    _hGdiPen = CreatePen(PS_SOLID, 1, color_t);//画笔
#endif
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
#if  0
    _hGdiBrush = CreateSolidBrush(((color) ? GDI_RGB_COL : GDI_RGB_BKCOL));//画刷
#else
    COLORREF color_t = (COLORREF)color;
    _hGdiBrush = CreateSolidBrush(color_t);//画刷
#endif //  0
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
#if 0
    InvalidateRect(hGdiWnd, NULL, TRUE);
#else
    gdi_rectangle(0, 0, maxX, maxY, colour, TRUE);
#endif
}


/*
 * gdi_get_screen_size:
 *	Return the max X & Y screen sizes. Needs to be called again, if you
 *	change screen orientation.
 *******************************************************************************
 */
void gdi_get_screen_size(int32 *x, int32 *y)
{
    /*RECT wndRect;
    GetClientRect(hGdiWnd, &wndRect);*/
    if (x != NULL)
    {
        *x = /*(int32)(wndRect.right - wndRect.left)*/maxX;
    }
    if (y != NULL)
    {
        *y = /*(int32)(wndRect.bottom - wndRect.top)*/maxY;
    }
}


/*
 *******************************************************************************
 * Standard Graphical Functions
 *******************************************************************************
 */

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
#if 0
    SetPixel(mGdiHdc, x, y, ((colour) ? GDI_RGB_FOREGROUND : GDI_RGB_BACKGROUND));
#else
    SetPixel(mGdiHdc, x, y, colour);
#endif
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
#if 0
    return((col == GDI_RGB_BACKGROUND) ? TRUE : FALSE);
#else
    return (int32)col;
#endif
}

/*
 * gdi_line: gdi_lineto:
 *	Classic Bressenham Line code
 *******************************************************************************
 */
void gdi_line(int32 x0, int32 y0, int32 x1, int32 y1, int32 colour)
{
    x0 = ((x0 < 0) ? 0 : ((x0 > (maxX - 1)) ? (maxX - 1) : x0));
    x1 = ((x1 < 0) ? 0 : ((x1 > (maxX - 1)) ? (maxX - 1) : x1));
    y0 = ((y0 < 0) ? 0 : ((y0 > (maxY - 1)) ? (maxY - 1) : y0));
    y1 = ((y1 < 0) ? 0 : ((y1 > (maxY - 1)) ? (maxY - 1) : y1));
    MoveToEx(mGdiHdc, x0, y0, NULL);

    HPEN _hPen = _gdi_set_pencol(colour);
    LineTo(mGdiHdc, x1, y1);
    _gdi_clr_pencol(_hPen);
}

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

/*
* gdi_rhombus:
*	A rhombus is a spoilt days fishing
*******************************************************************************
*/
void gdi_rhombus(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled)
{
    HPEN _hPen;
    HBRUSH _hBrush;
    POINT rhombus[4] = { 0 };
    int32 halfx = 0, halfy = 0;

    halfx = ((x2 - x1 + 1) / 2);
    halfy = ((y2 - y1 + 1) / 2);

    rhombus[0].x = x1 + halfx;
    rhombus[0].y = y1;

    rhombus[1].x = x2;
    rhombus[1].y = y2 - halfy;

    rhombus[2].x = x2 - halfx;
    rhombus[2].y = y2;

    rhombus[3].x = x1;
    rhombus[3].y = y1 + halfx;

    if (filled)
    {
        _hPen = _gdi_set_pencol(colour);
        _hBrush = _gdi_set_brushcol(colour);
        Polygon(mGdiHdc, rhombus, 4);
        _gdi_clr_pencol(_hPen);
        _gdi_clr_brushcol(_hBrush);
    }
    else
    {
        _hPen = _gdi_set_pencol(colour);
        Polygon(mGdiHdc, rhombus, 4);
        _gdi_clr_pencol(_hPen);
    }
}

/*
 * gdi_rectangle:
 *	A rectangle is a spoilt days fishing
 *******************************************************************************
 */
void gdi_rectangle(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled)
{
    HPEN _hPen;
    HBRUSH _hBrush;
    x1 = ((x1 < 0) ? 0 : ((x1 > (maxX - 1)) ? (maxX - 1) : x1));
    y1 = ((y1 < 0) ? 0 : ((y1 > (maxY - 1)) ? (maxY - 1) : y1));
    x2 = ((x2 < 0) ? 0 : ((x2 > (maxX - 1)) ? (maxX - 1) : x2));
    y2 = ((y2 < 0) ? 0 : ((y2 > (maxY - 1)) ? (maxY - 1) : y2));

    if (filled)
    {
        _hPen = _gdi_set_pencol(colour);
        _hBrush = _gdi_set_brushcol(colour);
        Rectangle(mGdiHdc, x1, y1, x2, y2);
        _gdi_clr_pencol(_hPen);
        _gdi_clr_brushcol(_hBrush);
    }
    else
    {
        _hPen = _gdi_set_pencol(colour);
        Rectangle(mGdiHdc, x1, y1, x2, y2);
        _gdi_clr_pencol(_hPen);
    }
}


/*
* gdi_ellipse:
*      This is the midpoint32 ellipse algorithm.
*******************************************************************************
*/
void gdi_ellipse(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled)
{
    HPEN _hPen;
    HBRUSH _hBrush;
    x1 = ((x1 < 0) ? 0 : ((x1 > (maxX - 1)) ? (maxX - 1) : x1));
    y1 = ((y1 < 0) ? 0 : ((y1 > (maxY - 1)) ? (maxY - 1) : y1));
    x2 = ((x2 < 0) ? 0 : ((x2 > (maxX - 1)) ? (maxX - 1) : x2));
    y2 = ((y2 < 0) ? 0 : ((y2 > (maxY - 1)) ? (maxY - 1) : y2));

    if (filled)
    {
        _hPen = _gdi_set_pencol(colour);
        _hBrush = _gdi_set_brushcol(colour);
        Ellipse(mGdiHdc, x1, y1, x2, y2);
        _gdi_clr_pencol(_hPen);
        _gdi_clr_brushcol(_hBrush);
    }
    else
    {
        _hPen = _gdi_set_pencol(colour);
        Ellipse(mGdiHdc, x1, y1, x2, y2);
        _gdi_clr_pencol(_hPen);
    }
}

/*
 * gdi_circle:
 *      This is the midpoint32 circle algorithm.
 *******************************************************************************
 */
void gdi_circle(int32 x, int32 y, int32 r, int32 colour, int32 filled)
{
    gdi_ellipse(x - r, y - r, x + r, y + r, colour, filled);
}

void gdi_textout(int32 x, int32 y, int32 size, int32 colour, int32 mode, PTCHAR text)
{
    HFONT hFont;
    int32 cWeight = FW_NORMAL;
    DWORD bItalic = FALSE;
    DWORD bUnderline = FALSE;

    x = ((x < 0) ? 0 : ((x > (maxX - 1)) ? (maxX - 1) : x));
    y = ((y < 0) ? 0 : ((y > (maxY - 1)) ? (maxY - 1) : y));

    if (mode != GDI_TMODE_NULL)
    {
        if (mode & GDI_TMODE_BOLD)
        {
            cWeight = FW_BOLD;
        }
        if (mode & GDI_TMODE_ITALIC)
        {
            bItalic = TRUE;
        }
        if (mode & GDI_TMODE_UNDLINE)
        {
            bUnderline = TRUE;
        }
    }

#ifdef DEBUG_TEXT_OUT_CHN
    //创建字体
    hFont = CreateFont(size, 0, 0, 0, cWeight, bItalic, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("微软雅黑"));
#else
    hFont = CreateFont(size, 0, 0, 0, cWeight, bItalic, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));
#endif
    //选择字体
    SelectObject(mGdiHdc, hFont);

    SetTextColor(mGdiHdc, colour);//设置字体颜色
    //SetBkColor(mGdiHdc, GDI_RGB_BACKGROUND);//设置字体背景色
    SetBkMode(mGdiHdc, TRANSPARENT);//透明背景
    SetTextAlign(mGdiHdc, TA_LEFT);//左对齐

    TextOut(mGdiHdc, x, y, text, lstrlen(text));

    DeleteObject(hFont);//删除字体
}

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
    mGdiHdc = CreateCompatibleDC(hGdiHdc);        //创建软件设备
    GetClientRect(hGdiWnd, &hGdiWndRect);
    hGdiWndWidth = hGdiWndRect.right - hGdiWndRect.left;
    hGdiWndHeight = hGdiWndRect.bottom - hGdiWndRect.top;

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
    BitBlt(hGdiHdc, /*hGdiWndRect.left*/0, /*hGdiWndRect.top*/0, hGdiWndWidth, hGdiWndHeight, mGdiHdc, 0, 0, SRCCOPY);

    return OK;
}
