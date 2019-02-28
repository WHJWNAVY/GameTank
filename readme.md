# 坦克大战-C语言版

> # 游戏体验
>
> ![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20180905135655.gif)

> # 视频版
>
> [坦克大战-C语言版-GameTank](https://v.youku.com/v_show/id_XMzgxMTc1MjI0OA==.html)

## 代码框架

坦克大战游戏代码框架如下
![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20180905131328.png)
在 ***main.c*** 中,创建应用窗口,并初始化一些系统资源,然后初始化gdi,初始化坦克大战游戏.
在 ***gdi.c*** 中,对系统的HDC及绘图接口进行了一次封装,使用WIN32系统提供的绘图接口来实现我们自己的图形绘制API.这里使用到了双缓冲技术,下文会简单介绍.
在 ***Tank.c*** 中,实现了坦克大战的游戏逻辑,包括贴图系统,地图系统,坦克制造,炮弹制造,装备生成,坦克移动,炮弹移动,以及坦克被炮弹击中,坦克吃到装备,等等.
下面将对代码进行详细的分析.


## 运行截图

![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20181015114935.png)

![](https://raw.githubusercontent.com/WHJWNAVY/myImage/master/PicGo/20181015114759.png)
