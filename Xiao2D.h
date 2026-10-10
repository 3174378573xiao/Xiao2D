#pragma once
/**
 * @file Xiao2D.h
 * @brief 基于SDL3的简易2D绘图UI库，提供点、圆、圆环、线段、斜矩形等几何绘制能力
 * @author Xiao2D
 * @version 0.1
 * @details
 * 底层依赖 SDL3 RenderGeometry 硬件三角形渲染；
 * 支持填充、描边、可设置线宽；支持旋转斜矩形；
 * 所有坐标均为窗口像素浮点数，Y轴向下。
 * DrawPen支持链式调用设置属性。
 *
 * <b>内部实现概要：</b>
 * - 全部图形均通过构造SDL_Vertex三角形顶点+索引交由SDL_RenderGeometry硬件渲染，不使用SDL传统像素绘制API；
 * - 圆形/圆环通过多边形分段逼近；粗线段由斜矩形主体+两端圆形线帽组合绘制；
 * - 描边不使用轮廓算法，而是通过生成向内偏移的几何体（圆环/多段线）实现，规避半透明颜色叠加bug；
 * - App主循环统一管理事件分发、deltaTime计时、多窗口生命周期；每个窗口独立Renderer、独立Present。
 * @note 需要C++20标准，依赖SDL3
 */
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <thread>
#include <numbers>
#include <cmath>
#include <vector>
#include <functional>
/**
 * @namespace Xiao2D
 * @brief Xiao2D库顶层命名空间，全部库接口放置于此
 *
 * @par 内部实现说明
 * 几何计算全部在CPU完成顶点构造，提交给SDL3硬件渲染；
 * Window对象由App静态容器统一管理生命周期，禁止手动delete窗口。
 */
namespace Xiao2D
{
    /**
     * @brief 颜色类，RGBA浮点颜色，值域0.0 ~ 1.0
     *
     * @par 内部实现
     * 直接存储浮点RGBA，与SDL_FColor内存布局对齐，toSDL_FColor直接转换返回，无颜色空间转换。
     */
    class Color
    {
    public:
        float r; ///< 红色通道 [0.0,1.0]
        float g; ///< 绿色通道 [0.0,1.0]
        float b; ///< 蓝色通道 [0.0,1.0]
        float a; ///< Alpha透明度 [0.0完全透明，1.0不透明]
        /**
         * @brief 转换为SDL3 SDL_FColor结构体
         * @return SDL_FColor SDL内部颜色结构体
         * @note const：不修改Color对象，可被const对象调用
         *
         * @par 内部实现
         * 直接聚合初始化返回SDL_FColor，字段一一对应。
         */
        SDL_FColor toSDL_FColor() const { return {r, g, b, a}; }
    };
    /**
     * @brief 绘图画笔参数，统一控制图形填充、描边、精度配置
     * @details 支持链式调用 setLineColor / setLineWidth ...
     * 同时保留聚合初始化语法兼容旧代码
     *
     * @par 内部实现
     * draw系列函数会拷贝DrawPen副本，绘图过程不会修改外部传入画笔实例；
     * isFilledWithBorder控制图形是否同时绘制填充几何体+描边几何体。
     */
    class DrawPen
    {
    public:
        Color lineColor;         ///< 描边/线条颜色
        float lineWidth;         ///< 线宽（像素），描边几何体偏移距离以此数值计算
        Color fillColor;         ///< 内部填充颜色
        bool isFilledWithBorder; ///< true:绘制填充+描边；false:仅绘制描边
        Color eraseColor;        ///< 画布擦除背景颜色，Window::eraseAll使用该颜色清屏
        int pointPrecision;      ///< 点图形（小圆）分段数量，点本质是小圆
        int circlePrecision;     ///< 圆形绘制分段数量，越大越圆滑，控制圆/圆环多边形逼近精度
        /// @brief 默认构造，给合理默认绘图参数
        DrawPen()
            : lineColor{1, 1, 1, 1},
              lineWidth(2.0f),
              fillColor{1, 1, 1, 1},
              isFilledWithBorder(true),
              eraseColor{0.05f, 0.05f, 0.08f, 1.0f},
              pointPrecision(16),
              circlePrecision(40)
        {
        }
        /**
         * @brief 设置线条颜色，链式调用
         * @param c 颜色
         * @return DrawPen& 返回自身引用用于链式
         *
         * @par 内部实现
         * 修改成员变量，返回*this支持连续链式调用。
         */
        DrawPen &setLineColor(Color c)
        {
            lineColor = c;
            return *this;
        }
        /**
         * @brief 设置线宽
         * @param w 像素宽度
         * @return DrawPen&
         */
        DrawPen &setLineWidth(float w)
        {
            lineWidth = w;
            return *this;
        }
        /**
         * @brief 设置填充颜色
         * @param c 填充色
         * @return DrawPen&
         */
        DrawPen &setFillColor(Color c)
        {
            fillColor = c;
            return *this;
        }
        /**
         * @brief 设置是否同时绘制填充+描边
         * @param enable true填充+描边；false仅描边
         * @return DrawPen&
         */
        DrawPen &setFilledWithBorder(bool enable)
        {
            isFilledWithBorder = enable;
            return *this;
        }
        /**
         * @brief 设置画布擦除背景色
         * @param c 颜色
         * @return DrawPen&
         */
        DrawPen &setEraseColor(Color c)
        {
            eraseColor = c;
            return *this;
        }
        /**
         * @brief 设置点的圆弧分段精度
         * @param prec 分段数
         * @return DrawPen&
         */
        DrawPen &setPointPrecision(int prec)
        {
            pointPrecision = prec;
            return *this;
        }
        /**
         * @brief 设置圆形圆弧分段精度
         * @param prec 分段数
         * @return DrawPen&
         */
        DrawPen &setCirclePrecision(int prec)
        {
            circlePrecision = prec;
            return *this;
        }
    };
    /**
     * @brief 二维浮点点/向量类，提供向量运算符与几何工具函数
     *
     * @par 内部实现
     * 全部运算为const成员，不修改自身，返回新Point对象；
     * 坐标系：窗口Y轴向下；perpendicularCCW/perpendicularCW按该坐标系计算垂直向量。
     */
    class Point
    {
    public:
        float x; ///< X坐标，向右增大
        float y; ///< Y坐标，向下增大（窗口坐标系）
        /**
         * @brief 默认构造，初始化为(0,0)
         */
        Point() : x(0), y(0) {}
        /**
         * @brief 构造指定x,y点
         * @param x_ x坐标
         * @param y_ y坐标
         */
        Point(float x_, float y_) : x(x_), y(y_) {}
        /**
         * @brief 向量加法
         * @param other 另一个点/向量
         * @return Point 结果向量
         */
        Point operator+(const Point &other) const;
        /**
         * @brief 向量减法
         * @param other 被减向量
         * @return Point 结果向量
         */
        Point operator-(const Point &other) const;
        /**
         * @brief 向量数乘
         * @param s 缩放系数
         * @return Point 缩放后向量
         */
        Point operator*(float s) const;
        /**
         * @brief 向量数除
         * @param s 除数，不能为0
         * @return Point 缩放后向量
         */
        Point operator/(float s) const;
        /**
         * @brief 获取向量模长（长度）
         * @return float 向量长度
         *
         * @par 内部实现
         * 使用std::sqrt计算欧几里得范数。
         */
        float length() const;
        /**
         * @brief 返回归一化单位向量；零向量返回(0,0)
         * @return Point 单位向量
         *
         * @par 内部实现
         * 先调用length获取模，模小于1e‑6判定零向量避免除零。
         */
        Point normalize() const;
        /**
         * @brief 获取当前向量【逆时针90度】垂直法线
         * @return Point 垂直向量
         * @note 窗口坐标系Y向下
         *
         * @par 内部实现
         * 返回(-y, x)，注意Y向下，数学坐标系逆时针与屏幕坐标系行为不同。
         */
        Point perpendicularCCW() const;
        /**
         * @brief 获取当前向量【顺时针90度】垂直法线
         * @return Point 垂直向量
         *
         * @par 内部实现
         * 返回(y, -x)。
         */
        Point perpendicularCW() const;
    };

    /**
     * @brief 二维向量叉积 (b‑a) × (c‑a)
     * @param a 起点
     * @param b 向量1终点
     * @param c 向量2终点
     * @return float 叉积，>0逆时针转向；<0顺时针；=0三点共线
     *
     * @par 内部实现
     * cross = (b.x‑a.x)*(c.y‑a.y) − (b.y‑a.y)*(c.x‑a.x)
     */
    float cross(const Point& a, const Point& b, const Point& c);

    /**
     * @brief 判断点p是否在三角形abc内部或边上
     * @param p 待测点
     * @param a 三角形顶点1
     * @param b 三角形顶点2
     * @param c 三角形顶点3
     * @return true点在三角形内/边上；false在外
     */
    bool pointInTriangle(const Point& p, const Point& a, const Point& b, const Point& c);

    /**
     * @brief 判断三点(a,b,c)构成的角是否为凸角（逆时针多边形）
     * @return true凸顶点；false凹顶点
     */
    bool isConvex(const Point& a, const Point& b, const Point& c);

    /**
     * @brief 耳朵裁剪三角剖分，简单不自交多边形，输出三角形顶点索引
     * @param poly 输入多边形顶点数组
     * @param length 顶点数量 >=3
     * @param outTris [out]输出三角形索引，每3个int为一组三角形{idx0,idx1,idx2}
     * @return bool true剖分成功；false失败（自交/点数不足）
     *
     * @par 内部实现
     * Ear‑Clipping耳切算法；反复找到耳朵(凸顶点，三角形不含其他顶点)裁剪输出三角形；
     * 仅支持简单无孔洞不自交多边形；输入顶点顺/逆时针均可。
     */
    bool earClipTriangulate(const Point poly[], size_t length, std::vector<size_t>& outTris);

    /**
     * @brief 圆形几何对象（实心圆）
     *
     * @par 内部实现
     * 仅存储圆心+半径，不保存顶点；绘制时实时生成扇形三角带顶点交给SDL_RenderGeometry。
     */
    class Circle
    {
    public:
        Point point; ///< 圆心坐标
        float r;     ///< 圆半径（像素）
    };
    /**
     * @brief 圆环几何对象，外半径outerR，内半径innerR，中间镂空
     *
     * @par 内部实现
     * 仅存储几何参数；绘制调用内部buildRingGeometry生成内外圈交替顶点+索引绘制三角带。
     */
    class Ring
    {
    public:
        Point point;  ///< 圆环圆心
        float outerR; ///< 外圆半径
        float innerR; ///< 内圆半径，必须小于outerR
    };
    /**
     * @brief 线段几何对象；带圆形线帽，由两点定义
     *
     * @par 内部实现
     * draw时生成斜矩形作为线段主体，再额外绘制两个小圆作为两端圆角线帽。
     */
    class Line
    {
    public:
        Point p1; ///< 线段起点
        Point p2; ///< 线段终点
        /**
         * @brief 构造线段
         * @param a 起点
         * @param b 终点
         */
        Line(Point a, Point b) : p1(a), p2(b) {}
    };
    /**
     * @brief 四边形矩形对象；支持轴对齐矩形 / 斜平行四边形
     * @details 内部存储逆时针顺序4个顶点v[0],v[1],v[2],v[3]
     *
     * @par 内部实现
     * 不存储宽高，全部依靠4个顶点表达；两个静态工厂函数计算生成顶点数组；
     * 绘制填充时拆分为两个三角形；描边拆解为4条Line绘制。
     */
    class Rect
    {
    public:
        Point v[4]; ///< 4个顶点，逆时针顺序
        /**
         * @brief 创建【轴对齐矩形】：左上角+宽高
         * @param topLeft 左上角点
         * @param w 宽度（像素）
         * @param h 高度（像素）
         * @return Rect 轴对齐矩形对象
         *
         * @par 内部实现
         * 根据左上角、宽高直接计算4个逆时针顶点。
         */
        Rect(Point topLeft, float w, float h);
        /**
         * @brief 创建斜矩形（平行四边形），上下边中点 + 矩形宽度
         * @param midTop 上边线段中点
         * @param midBot 下边线段中点；两点连线确定矩形倾斜方向
         * @param width 矩形横向边总宽度
         * @return Rect 斜平行四边形
         * @note midTop与midBot不要重合，否则几何异常
         *
         * @par 内部实现
         * 计算两点方向向量，取逆时针垂直法线，沿法线正负偏移半宽得到四个角点。
         */
        Rect(Point midTop, Point midBot, float width);
    };
    /**
     * @brief Xiao2D窗口类，封装SDL窗口与渲染器，提供所有draw绘图接口
     * @warning 所有接口必须在主线程调用
     *
     * @par 内部实现
     * - create()内部调用SDL_CreateWindowAndRenderer创建窗口渲染器，开启SDL_BLENDMODE_BLEND透明混合；
     * - 所有draw重载均构造SDL_Vertex顶点，调用submitGeometry转发SDL_RenderGeometry；
     * - updateCallback每帧触发；eventCallback接收窗口/键鼠事件；
     * - 窗口生命周期交由App静态容器管理，不要手动delete。
     */
    class Window
    {
    public:
        friend class App;
        /**
         * @brief 默认构造，标题none，大小800x600
         *
         * @par 内部实现
         * 仅初始化成员，不会真正创建SDL窗口；窗口在App::addWindow调用create()才生成。
         */
        Window();
        /**
         * @brief 创建窗口与渲染器
         * @param title 窗口标题，UTF‑8编码
         * @param width 窗口像素宽
         * @param height 窗口像素高
         * @note 创建失败内部指针置空，后续draw调用直接安全返回；自动开启alpha混合
         *
         * @par 内部实现
         * 构造仅保存标题宽高；SDL资源延迟到create()执行。
         */
        Window(const std::string title, int width, int height);
        /**
         * @brief 使用画笔的eraseColor清空画布背景
         * @param drawPen 画笔，读取eraseColor作为背景
         *
         * @par 内部实现
         * 调用SDL_SetRenderDrawColorFloat设置清屏颜色，SDL_RenderClear执行清屏。
         */
        void eraseAll(const DrawPen &drawPen);
        /**
         * @brief 绘制点（内部实现为小圆），使用窗口默认drawPen画笔
         * @param point 点中心位置
         */
        void draw(Point point);
        /**
         * @brief 绘制点（内部实现为小圆）
         * @param point 点中心位置
         * @param drawPen 画笔参数，lineWidth控制点大小
         * @note 函数内部拷贝画笔副本，不会修改外部传入DrawPen
         *
         * @par 内部实现
         * 将点转为极小Circle，复用圆形绘制逻辑；pointPrecision作为圆分段精度。
         */
        void draw(Point point, const DrawPen &drawPen);
        /**
         * @brief 绘制实心圆形，使用窗口默认drawPen画笔
         * @param circle 圆形对象
         */
        void draw(Circle circle);
        /**
         * @brief 绘制实心圆形，支持填充+描边
         * @param circle 圆形对象
         * @param drawPen 画笔
         * @note 修复半透明填充叠加bug，描边使用真实圆环几何体；内部拷贝画笔副本
         *
         * @par 内部实现
         * 1.填充：扇形三角扇，圆心为第一个顶点，外围圆周顶点构成三角形扇；
         * 2.描边：不绘制轮廓线，构造一个薄Ring圆环几何体作为描边，规避半透明叠加瑕疵。
         */
        void draw(Circle circle, const DrawPen &drawPen);
        /**
         * @brief 绘制圆环(环形)，使用窗口默认drawPen画笔
         * @param ring 圆环对象 outerR>innerR
         */
        void draw(Ring ring);
        /**
         * @brief 绘制圆环(环形)，支持填充环本体 + 描边
         * @param ring 圆环对象 outerR>innerR
         * @param drawPen 画笔
         * @note isFilledWithBorder=true：绘制环本体填充，再绘制环外圈+内圈描边
         *
         * @par 内部实现
         * 调用静态辅助buildRingGeometry生成内外圈顶点索引；描边生成内外两层薄圆环叠加。
         */
        void draw(Ring ring, const DrawPen &drawPen);
        /**
         * @brief 绘制粗线段，两端带圆形圆角线帽，使用窗口默认drawPen画笔
         * @param line 线段对象
         */
        void draw(Line line);
        /**
         * @brief 绘制粗线段，两端带圆形圆角线帽
         * @param line 线段对象
         * @param pen 画笔，lineWidth控制线条粗细
         * @note 函数内部拷贝画笔副本，不会修改外部传入DrawPen
         *
         * @par 内部实现
         * 计算垂直法线生成斜矩形4顶点构成线段主体；再在两个端点绘制半径=半线宽的小圆作为圆角线帽。
         */
        void draw(Line line, const DrawPen &pen);
        /**
         * @brief 绘制四边形（轴对齐矩形 / 斜平行四边形），使用窗口默认drawPen画笔
         * @param rect Rect对象，4个逆时针顶点
         */
        void draw(const Rect &rect);
        /**
         * @brief 绘制四边形（轴对齐矩形 / 斜平行四边形）
         * @param rect Rect对象，4个逆时针顶点
         * @param pen 画笔，控制填充、描边、线宽
         * @note 函数内部拷贝画笔副本，不会修改外部传入DrawPen
         *
         * @par 内部实现
         * 填充拆分为2个三角形提交顶点；描边拆解为4条Line依次绘制四边。
         */
        void draw(const Rect &rect, const DrawPen &pen);

        /**
         * @brief 绘制多边形，使用窗口默认drawPen画笔
         * @param points 多边形顶点数组，可以凸/凹，必须是简单不自交多边形
         * @param length 顶点数组元素个数，至少3个点才会渲染
         * @note 不支持带孔洞、自交多边形；顶点顺序顺时针/逆时针均可
         */
        void draw(const Point points[],size_t length);

        /**
         * @brief 绘制多边形，支持凹多边形填充 + 描边
         * @param points 多边形顶点数组，简单不自交多边形，凸/凹均可
         * @param length 顶点数量，必须>=3才绘制
         * @param drawPen 绘图画笔
         * @note 函数内部拷贝画笔副本，不会修改外部DrawPen；不支持孔洞、自交多边形
         *
         * @par 内部实现
         * 填充：调用earClipTriangulate耳切法把多边形拆成若干三角形，批量提交SDL顶点；
         * 描边：循环绘制相邻顶点线段，闭合首尾，复用Line绘制，自带圆角线帽；
         * isFilledWithBorder=true同时绘制填充+描边；false仅绘制描边线框。
         */
        void draw(const Point points[],size_t length, const DrawPen &drawPen);
    public:
        SDL_Window *_window;                                 ///< SDL窗口裸指针，高级用户可访问，不要手动销毁，生命周期由App管理
        SDL_Renderer *_renderer;                             ///< SDL渲染器裸指针，高级用户可访问，不要手动销毁
        DrawPen drawPen;                                     ///< 窗口默认画笔；无参draw()重载全部使用该画笔
        std::function<void(float deltaTime)> updateCallback; ///< 每帧更新绘图回调，参数deltaTime：帧间隔(秒)；每一帧清屏后调用
        std::function<void(SDL_Event *event)> eventCallback; ///< SDL事件回调；窗口事件给到对应窗口，键鼠事件广播全部存活窗口
    private:
        /**
         * @brief 内部工具：把顶点数组提交给SDL_RenderGeometry
         * @param verts 顶点数组，数量必须为3倍数
         * @attention 库内部私有，外部不可调用
         *
         * @par 内部实现
         * 简单校验非空+顶点是3倍数，直接调用SDL_RenderGeometry，不使用索引参数。
         */
        void submitGeometry(const std::vector<SDL_Vertex> &verts);
        std::string title; ///< 窗口标题，构造时传入
        int width, height; ///< 窗口宽高，构造时传入
        /**
         * @brief 内部：真正创建SDL窗口与渲染器资源
         * @par 内部实现
         * 被App::addWindow调用；失败置空_window/_renderer。
         */
        void create();
        /**
         * @brief 内部：销毁SDL窗口渲染器资源，置空指针
         * @par 内部实现
         * 安全判空，调用SDL_DestroyRenderer / SDL_DestroyWindow。
         */
        void destroy();
    };
    /**
     * @brief App全局应用单例管理类，管理多窗口、事件循环、计时、主循环
     *
     * @par 内部实现
     * 全部成员静态；内部维护Window*容器wins；run()为阻塞式主循环；
     * 使用SDL高性能性能计数器计算deltaTime；自动清理已destroy的窗口；
     * 事件分发：窗口事件定向对应窗口，其余事件广播全部存活窗口；
     * 循环末尾做帧率sleep节流；所有窗口关闭自动退出run循环。
     */
    class App
    {
    public:
        /**
         * @brief 初始化SDL视频子系统
         * @return int 0成功，‑1失败
         *
         * @par 内部实现
         * 调用SDL_Init(SDL_INIT_VIDEO)。
         */
        static int init()
        {
            if (!SDL_Init(SDL_INIT_VIDEO))
            {
                std::cout << "SDL初始化失败: " << SDL_GetError() << std::endl;
                return -1;
            }
            return 0;
        }
        /**
         * @brief 添加窗口到App管理，内部调用Window::create创建SDL资源
         * @param win Window对象指针
         */
        static void addWindow(Window *win);
        /**
         * @brief 批量添加多个窗口
         * @param wins[] 窗口指针数组
         * @param winNum 窗口数量
         */
        static void addWindow(Window *wins[], int winNum);
        /**
         * @brief 阻塞式运行主循环，事件处理、更新、渲染、计时全部在这里
         *
         * @par 内部实现
         * 1.初始化性能计数器；
         * 2.循环PollEvent处理事件、分发回调；
         * 3.过滤已经destroy的窗口；无存活窗口退出循环；
         * 4.每个存活窗口执行eraseAll → updateCallback → SDL_RenderPresent；
         * 5.帧末尾sleep节流；退出前调用quit销毁全部SDL资源。
         */
        static void run();
        // 新增：退出，销毁全部窗口
        /**
         * @brief 销毁全部窗口、调用SDL_Quit清理SDL
         *
         * @par 内部实现
         * 遍历wins调用destroy，清空容器，执行SDL_Quit。
         */
        static void quit();
        // 新增帧率工具接口
        /**
         * @brief 获取上一帧帧间隔，单位秒
         * @return float deltaTime 帧时间，已经做上限钳位防止卡顿爆炸
         */
        static float getDeltaTime();
        /**
         * @brief 获取瞬时FPS（1.0/deltaTime）
         * @return float 当前帧率
         */
        static float getFPS();
    private:
        static std::vector<Window *> wins; ///< 被App管理的窗口指针集合；run循环内会过滤掉已destroy窗口
        // 时间统计变量
        static Uint64 perfFreq;      ///< SDL高性能计数器频率，每秒钟计数次数，run开头初始化
        static Uint64 lastPerfCount; ///< 上一循环的性能计数器计数值，用于计算帧间隔
        static float deltaTime;      ///< 当前帧间隔，单位秒，做maxDeltaTime钳位
        static float fps;            ///< 瞬时帧率，由1/deltaTime算出
        // 辅助：通过windowID查找Window*
        /**
         * @brief 根据SDL窗口ID查找对应的Xiao2D::Window指针
         * @param windowId SDL窗口ID
         * @return Window* 找不到返回nullptr
         *
         * @par 内部实现
         * 遍历wins容器，调用SDL_GetWindowID对比ID。
         */
        static Window *findWindowByID(Uint32 windowId);
    };
};
