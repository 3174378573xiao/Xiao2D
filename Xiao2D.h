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
 */
namespace Xiao2D
{

    /**
     * @brief 颜色类，RGBA浮点颜色，值域0.0 ~ 1.0
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
         */
        SDL_FColor toSDL_FColor() const { return {r, g, b, a}; }
    };

    /**
     * @brief 绘图画笔参数，统一控制图形填充、描边、精度配置
     * @details 支持链式调用 setLineColor / setLineWidth ...
     * 同时保留聚合初始化语法兼容旧代码
     */
    class DrawPen
    {
    public:
        Color lineColor;         ///< 描边/线条颜色
        float lineWidth;         ///< 线宽（像素）
        Color fillColor;         ///< 内部填充颜色
        bool isFilledWithBorder; ///< true:绘制填充+描边；false:仅绘制描边
        Color eraseColor;        ///< 画布擦除背景颜色
        int pointPrecision;      ///< 点图形（小圆）分段数量
        int circlePrecision;     ///< 圆形绘制分段数量，越大越圆滑

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
         * @param c 背景色
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
         */
        float length() const;

        /**
         * @brief 返回归一化单位向量；零向量返回(0,0)
         * @return Point 单位向量
         */
        Point normalize() const;

        /**
         * @brief 获取当前向量【逆时针90度】垂直法线
         * @return Point 垂直向量
         * @note 窗口坐标系Y向下
         */
        Point perpendicularCCW() const;

        /**
         * @brief 获取当前向量【顺时针90度】垂直法线
         * @return Point 垂直向量
         */
        Point perpendicularCW() const;
    };

    /**
     * @brief 圆形几何对象（实心圆）
     */
    class Circle
    {
    public:
        Point point; ///< 圆心坐标
        float r;     ///< 圆半径（像素）
    };

    /**
     * @brief 圆环几何对象，外半径outerR，内半径innerR，中间镂空
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
         */
        static Rect CreateWithTLWH(Point topLeft, float w, float h);

        /**
         * @brief 创建斜矩形（平行四边形），上下边中点 + 矩形宽度
         * @param midTop 上边线段中点
         * @param midBot 下边线段中点；两点连线确定矩形倾斜方向
         * @param width 矩形横向边总宽度
         * @return Rect 斜平行四边形
         * @note midTop与midBot不要重合，否则几何异常
         */
        static Rect CreateFromTwoMidPoints(Point midTop, Point midBot, float width);
    };

    /**
     * @brief Xiao2D窗口类，封装SDL窗口与渲染器，提供所有draw绘图接口
     * @warning 所有接口必须在主线程调用
     */
    class Window
    {
    public:
        friend class App;

        /**
         * @brief 默认构造，标题none，大小800x600
         */
        Window();

        /**
         * @brief 创建窗口与渲染器
         * @param title 窗口标题，UTF‑8编码
         * @param width 窗口像素宽
         * @param height 窗口像素高
         * @note 创建失败内部指针置空，后续draw调用直接安全返回；自动开启alpha混合
         */
        Window(const std::string title, int width, int height);

        /**
         * @brief 使用画笔的eraseColor清空画布背景
         * @param drawPen 画笔，读取eraseColor作为背景
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
         */
        void draw(const Rect &rect, const DrawPen &pen);

    public:
        SDL_Window *_window;                                 ///< SDL窗口裸指针，高级用户可访问，不要手动销毁
        SDL_Renderer *_renderer;                             ///< SDL渲染器裸指针，高级用户可访问，不要手动销毁
        DrawPen drawPen;                                     ///< 窗口默认画笔
        std::function<void(float deltaTime)> updateCallback; ///< 每帧更新绘图回调，参数deltaTime：帧间隔(秒)
        std::function<void(SDL_Event *event)> eventCallback; ///< SDL事件回调

    private:
        /**
         * @brief 内部工具：把顶点数组提交给SDL_RenderGeometry
         * @param verts 顶点数组，数量必须为3倍数
         * @attention 库内部私有，外部不可调用
         */
        void submitGeometry(const std::vector<SDL_Vertex> &verts);

        std::string title;
        int width, height;

        void create();

        void destroy();
    };

    class App
    {
    public:
        static int init()
        {
            if (!SDL_Init(SDL_INIT_VIDEO))
            {
                std::cout << "SDL初始化失败: " << SDL_GetError() << std::endl;
                return -1;
            }
            return 0;
        }
        // 再run调用前或run循环中都能调用
        static void addWindow(Window *win);
        static void addWindow(Window *wins[], int winNum);
        static void run();
        // 新增：退出，销毁全部窗口
        static void quit();

        // 新增帧率工具接口
        static float getDeltaTime();
        static float getFPS();

    private:
        static std::vector<Window *> wins;
        // 时间统计变量
        static Uint64 perfFreq;      // 性能计数器频率
        static Uint64 lastPerfCount; // 上一帧计数
        static float deltaTime;      // 当前帧间隔 秒
        static float fps;            // 当前fps

        // 辅助：通过windowID查找Window*
        static Window *findWindowByID(Uint32 windowId);
    };

};
