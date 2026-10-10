#pragma once
/**
 * @file Xiao2D.h
 * @brief 基于SDL3的简易2D绘图库，重构版：统一Area图形基类，支持变换、点包含检测，圆/椭圆/圆环/矩形/扇形/多边形
 * @author Xiao2D
 * @version 0.2
 * @details
 * 底层依赖 SDL3 RenderGeometry 硬件三角形渲染；
 * 全部图形继承Area基类，内置位置、旋转、缩放变换；支持虚函数contains()点碰撞检测；
 * 所有坐标窗口像素浮点数，Y轴向下；rotation单位为弧度，正值顺时针旋转。
 * DrawPen支持链式调用设置属性。
 *
 * <b>内部实现概要：</b>
 * - 每个图形保存局部几何参数；绘制时先生成局部顶点，通过矩阵变换到世界窗口坐标提交SDL；
 * - 点包含检测contains：将世界点逆变换回局部坐标系，执行局部几何判断；
 * - 圆形/椭圆/扇形/圆环通过多边形分段逼近；描边使用偏移几何体规避半透明叠加bug；
 * - 凹多边形使用Ear‑Clipping耳切剖分；App管理多窗口生命周期、事件循环、deltaTime计时。
 * @note 需要C++20标准，依赖SDL3
 */
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <thread>
#include <cmath>
#include <vector>
#include <functional>
#include <numbers>

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
     *
     * @par 内部实现
     * draw系列函数会拷贝DrawPen副本，绘图过程不会修改外部传入画笔实例；
     * isFilledWithBorder控制图形是否同时绘制填充几何体+描边几何体。
     */
    class DrawPen
    {
    public:
        Color lineColor;         ///< 描边/线条颜色
        float lineWidth;         ///< 线宽（像素）
        Color fillColor;         ///< 内部填充颜色
        bool isFilledWithBorder; ///< true:绘制填充+描边；false:仅绘制描边
        Color eraseColor;        ///< 画布擦除背景颜色
        int pointPrecision;      ///< 点(小圆)分段数量
        int circlePrecision;     ///< 圆、椭圆、扇形、圆环的多边形逼近分段数
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
        DrawPen &setLineColor(Color c) { lineColor = c; return *this; }
        DrawPen &setLineWidth(float w) { lineWidth = w; return *this; }
        DrawPen &setFillColor(Color c) { fillColor = c; return *this; }
        DrawPen &setFilledWithBorder(bool enable) { isFilledWithBorder = enable; return *this; }
        DrawPen &setEraseColor(Color c) { eraseColor = c; return *this; }
        DrawPen &setPointPrecision(int prec) { pointPrecision = prec; return *this; }
        DrawPen &setCirclePrecision(int prec) { circlePrecision = prec; return *this; }
    };

    /**
     * @brief 二维浮点点/向量类，窗口坐标系Y轴向下
     *
     * @par 内部实现
     * 全部运算const成员，返回新Point对象；提供旋转变换工具函数。
     */
    class Point
    {
    public:
        float x; ///< X向右增大
        float y; ///< Y向下增大
        Point() : x(0), y(0) {}
        Point(float x_, float y_) : x(x_), y(y_) {}

        Point operator+(const Point &other) const;
        Point operator-(const Point &other) const;
        Point operator*(float s) const;
        Point operator/(float s) const;

        float length() const;
        Point normalize() const;
        Point perpendicularCCW() const;
        Point perpendicularCW() const;

        /**
         * @brief 绕原点旋转当前点（窗口Y向下，angle>0顺时针）
         * @param angle 旋转弧度
         * @return Point 旋转后点
         */
        Point rotate(float angle) const;
        /**
         * @brief 绕指定中心点旋转
         * @param pivot 旋转支点
         * @param angle 弧度，>0顺时针
         * @return Point
         */
        Point rotateAround(const Point& pivot, float angle) const;
    };

    //===== 几何工具函数 =====
    float cross(const Point& a, const Point& b, const Point& c);
    bool pointInTriangle(const Point& p, const Point& a, const Point& b, const Point& c);
    bool isConvex(const Point& a, const Point& b, const Point& c);
    bool earClipTriangulate(const Point poly[], size_t length, std::vector<size_t>& outTris);

    /**
     * @brief Area：所有可绘制2D图形的抽象基类
     *
     * @par 内部实现
     * 存储位置pos、旋转rotation(弧度，顺时针)、scale缩放；
     * 虚函数contains()：输入世界坐标点，判断点是否在图形内部；
     * 派生类实现virtual void onDraw()生成局部顶点；外部调用Window::draw(const Area&)触发多态绘制。
     * 变换流程：局部点 → 缩放 → 旋转 → 平移pos，得到世界窗口坐标。
     */
    class Area
    {
    public:
        Point pos;          ///< 图形基准位置（世界坐标）
        float rotation;     ///< 旋转角度，**弧度**，窗口Y向下，正值顺时针旋转
        Point scale;        ///< X/Y缩放，{1,1}原始大小

        Area() : pos{0,0}, rotation(0.f), scale{1.f,1.f} {}
        Area(Point p) : pos(p), rotation(0.f), scale{1.f,1.f} {}

        virtual ~Area() = default;

        /**
         * @brief 判断【世界窗口坐标点p】是否落在图形内部
         * @param p 世界窗口像素坐标
         * @return true点在内部；false外部
         *
         * @par 内部实现
         * 将世界坐标做逆变换（-平移 → -旋转 → 逆缩放）得到局部坐标，调用局部几何检测。
         */
        virtual bool contains(const Point& p) const = 0;

        /**
         * @brief 将局部坐标点变换为世界窗口坐标（scale→rotate→translate）
         * @param local 局部坐标系点
         * @return Point 世界坐标
         */
        Point localToWorld(const Point& local) const;

        /**
         * @brief 将世界窗口坐标逆变换回局部坐标系
         * @param world 世界点
         * @return Point 局部点
         */
        Point worldToLocal(const Point& world) const;
    };

    /**
     * @brief 实心圆，继承Area；pos=圆心
     *
     * @par 内部实现
     * r为局部坐标系半径；绘制生成扇形三角扇；描边生成薄圆环几何体；contains检测局部点距离小于r。
     */
    class Circle : public Area
    {
    public:
        float r; ///< 局部坐标系半径
        Circle() : Area(), r(10.f) {}
        Circle(Point center, float radius) : Area(center), r(radius) {}
        bool contains(const Point& p) const override;
    };

    /**
     * @brief 圆环(空心环)，继承Area；pos=圆心
     *
     * @par 内部实现
     * outerR/innerR局部半径；buildRingGeometry生成内外圈顶点；contains：点距离介于innerR~outerR之间。
     */
    class Ring : public Area
    {
    public:
        float outerR;
        float innerR;
        Ring() : Area(), outerR(30.f), innerR(15.f) {}
        Ring(Point center, float or_, float ir_) : Area(center), outerR(or_), innerR(ir_) {}
        bool contains(const Point& p) const override;
    };

    /**
     * @brief 轴对齐椭圆，继承Area；pos=椭圆圆心
     *
     * @par 内部实现
     * rx、ry局部半长/半短轴；参数方程生成局部椭圆顶点；contains使用椭圆隐式方程做局部检测。
     */
    class Ellipse : public Area
    {
    public:
        float rx; ///< 局部X半长轴
        float ry; ///< 局部Y半短轴
        Ellipse() : Area(), rx(40.f), ry(25.f) {}
        Ellipse(Point center, float rx_, float ry_) : Area(center), rx(rx_), ry(ry_) {}
        bool contains(const Point& p) const override;
    };

    /**
     * @brief 扇形（饼图扇区）继承Area；pos=扇形圆心
     * @details startAng、endAng为局部坐标系弧度；从startAng扫到endAng；支持大于2PI
     *
     * @par 内部实现
     * 生成由圆心+多段圆弧顶点组成扇形三角扇；contains：点距离小于半径，且角度落在扇区角度区间。
     */
    class Sector : public Area
    {
    public:
        float radius;       ///< 局部半径
        float startAng;     ///< 起始弧度（局部坐标系）
        float endAng;       ///< 结束弧度（局部坐标系）
        Sector() : Area(), radius(30.f), startAng(0), endAng(std::numbers::pi/2.f) {}
        Sector(Point c, float r, float s, float e) : Area(c), radius(r), startAng(s), endAng(e) {}
        bool contains(const Point& p) const override;
        
    };

    /**
     * @brief 轴对齐矩形Rect，继承Area；pos=左上角局部原点
     * @details width、height局部宽高；旋转缩放全部由基类Area的rotation/scale控制
     * @note 已删除旧的midTop/midBot斜向构造方式，倾斜完全依靠基类rotation
     *
     * @par 内部实现
     * 局部4个顶点(0,0),(w,0),(w,h),(0,h)；变换后得到世界顶点；contains局部坐标0<=x<=w,0<=y<=h。
     */
    class Rect : public Area
    {
    public:
        float width;
        float height;
        Rect() : Area(), width(80.f), height(60.f) {}
        Rect(Point topLeft, float w, float h) : Area(topLeft), width(w), height(h) {}
        bool contains(const Point& p) const override;
        /**
         * @brief 获取矩形在局部坐标系下4个逆时针顶点
         * @param out 输出4个局部点
         */
        void getLocalVertices(Point out[4]) const;
    };

    /**
     * @brief 简单多边形（凸/凹不自交）继承Area
     * @details localPoints存储**局部坐标系顶点**；pos为图形基准点；旋转缩放由Area控制
     *
     * @par 内部实现
     * 绘制：局部顶点变换到世界，耳切剖分；contains：世界点逆变换回局部，射线法判断点在多边形内。
     */
    class Polygon : public Area
    {
    public:
        std::vector<Point> localPoints; ///< 局部坐标系顶点，顺序凸/凹不自交
        Polygon() : Area() {}
        explicit Polygon(std::vector<Point> pts) : Area(), localPoints(std::move(pts)) {}
        bool contains(const Point& p) const override;
    };


    
    class Window;
    /**
     * @brief Xiao2D窗口类，封装SDL窗口渲染器
     *
     * @par 内部实现
     * 支持多态 draw(const Area& area, const DrawPen& pen)；内部完成Area的局部→世界顶点变换。
     */
    class Window
    {
    public:
        friend class App;
        Window();
        Window(const std::string title, int width, int height);

        void eraseAll(const DrawPen &drawPen);

        //===== 多态绘制Area派生对象（圆、椭圆、矩形、扇形、圆环、多边形） =====
        void draw(const Area& area, const DrawPen &drawPen);
        void draw(const Area& area);

        //===== 单独几何体绘制（旧接口保留，主要供内部调用；优先使用Area多态draw） =====
        void draw(Point point, const DrawPen &drawPen);
        void draw(Point point);

    public:
        SDL_Window *_window{nullptr};
        SDL_Renderer *_renderer{nullptr};
        DrawPen drawPen;
        std::function<void(float deltaTime)> updateCallback;
        std::function<void(SDL_Event *event)> eventCallback;

    private:
        std::string title;
        int width{800}, height{600};
        void create();
        void destroy();
        void submitGeometry(const std::vector<SDL_Vertex> &verts);

        //==== 内部：接收【已经变换完成的世界坐标顶点】，做填充/描边绘制逻辑 =====
        void drawCircleWorld(float rWorld, Point centerWorld, const DrawPen& pen);
        void drawEllipseWorld(Point centerWorld, float rxWorld, float ryWorld, const DrawPen& pen);
        void drawRingWorld(Point centerWorld, float outerRWorld, float innerRWorld, const DrawPen& pen);
        void drawSectorWorld(Point centerWorld, float radiusWorld, float startAng, float endAng, const DrawPen& pen);
        void drawPolygonWorld(const Point worldPts[], size_t count, const DrawPen& pen);
        void drawRectWorld(const Point worldVerts[4], const DrawPen& pen);
    };

    class App
    {
    public:
        static int init();
        static void addWindow(Window *win);
        static void addWindow(Window *wins[], int winNum);
        static void run();
        static void quit();
        static float getDeltaTime();
        static float getFPS();
    private:
        static std::vector<Window *> wins;
        static Uint64 perfFreq;
        static Uint64 lastPerfCount;
        static float deltaTime;
        static float fps;
        static Window *findWindowByID(Uint32 windowId);
    };
}
