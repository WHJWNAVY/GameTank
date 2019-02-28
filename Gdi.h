#ifndef __GDI_H_
#define __GDI_H_

#include <Windows.h>
#include "type.h"

// Size
#define MAX_PNT_SIZE        (15)
#define MAX_WND_WIDTH       (MAX_PNT_SIZE * 60)
#define MAX_WND_HEIGHT      (MAX_PNT_SIZE * 40)
#define	SCREEN_X		    MAX_WND_WIDTH
#define	SCREEN_Y		    MAX_WND_HEIGHT

#define GDI_RGB_RED         ((int32)(RGB(255, 0, 0)))//红色
#define GDI_RGB_GREEN       ((int32)(RGB(0, 255, 0)))//绿色
#define GDI_RGB_BLUE        ((int32)(RGB(0, 0, 255)))//蓝色
#define GDI_RGB_YELLOW      ((int32)(RGB(255, 255, 0)))//黄色
#define GDI_RGB_PURPLE      ((int32)(RGB(255, 0, 255)))//紫色
#define GDI_RGB_CYAN        ((int32)(RGB(0, 255, 255)))//青色
#define GDI_RGB_BLACK       ((int32)(RGB(0, 0, 0)))//黑色
#define GDI_RGB_WHITE       ((int32)(RGB(255, 255, 255)))//白色
#define GDI_RGB_GARY        ((int32)(RGB(192, 192, 192)))//灰色

#define GDI_RGB_FOREGROUND  (GDI_RGB_WHITE)//前景色
#define GDI_RGB_BACKGROUND  (GDI_RGB_GARY)//背景色

#define GDI_TMODE_NULL      (0x00)
#define GDI_TMODE_BOLD      (0x01)//粗体
#define GDI_TMODE_ITALIC    (0x02)//斜体
#define GDI_TMODE_UNDLINE   (0x04)//下划线

extern void   gdi_get_screen_size(int32 *x, int32 *y);
extern void   gdi_set_point(int32  x, int32  y, int32 colour);
extern int32  gdi_get_point(int32 x, int32 y);
extern void   gdi_line(int32 x0, int32 y0, int32 x1, int32 y1, int32 colour);
extern void   gdi_triangle(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled);
extern void   gdi_rhombus(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled);
extern void   gdi_rectangle(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled);
extern void   gdi_ellipse(int32 x1, int32 y1, int32 x2, int32 y2, int32 colour, int32 filled);
extern void   gdi_circle(int32  x, int32  y, int32  r, int32 colour, int32 filled);
extern void   gdi_textout(int32 x, int32 y, int32 size, int32 colour, int32 mode, PTCHAR text);
extern void   gdi_clear(int32 colour);
extern int32  gdi_init(HWND hWnd);
extern int32  gdi_dinit(void);
extern int32  gdi_update(void);

#endif
