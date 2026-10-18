#pragma once
/**
 * @file Xiao2D.h
 * @brief 基于SDL3的简易2D绘图库，重构版：统一Area图形基类，支持变换、点碰撞检测，圆/椭圆/圆环/矩形/扇形/多边形
 * @author Xiao2D
 * @version 0.3
 * @details
 * 底层依赖 SDL3 RenderGeometry 硬件三角形渲染；
 * 全部图形继承Area基类，内置位置、旋转、缩放变换；支持虚函数contains()点碰撞检测；
 * 绘图逻辑全部下沉到各个Area子类drawOnWindow，Window仅负责底层渲染提交与事件循环。
 * 所有坐标窗口像素浮点数，Y轴向下；rotation单位为弧度，正值顺时针旋转。
 * DrawPen支持链式调用设置属性。
 * @note 需要C++20标准，依赖SDL3
 */
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <numbers>

namespace Xiao2D
{
    class Window;
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

        SDL_FColor toSDL_FColor() const { return {r, g, b, a}; }

        // ========== 常用预定义颜色【static const 常量】 头文件声明 ==========
        static const Color Black;
        static const Color White;
        static const Color MidGray;
        static const Color DarkGray;
        static const Color LiteGray;

        static const Color Red;
        static const Color Green;
        static const Color Blue;

        static const Color Yellow;
        static const Color PinkRed;
        static const Color SeaBlue;

        static const Color Orange;
        static const Color LiteRed;
        static const Color Vio;

        static const Color Clear;

        /// 辅助工具：从0‑255整数RGBA构造颜色
        static constexpr Color fromRGBA(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        {
            return {
                static_cast<float>(r_) / 255.0f,
                static_cast<float>(g_) / 255.0f,
                static_cast<float>(b_) / 255.0f,
                static_cast<float>(a_) / 255.0f};
        }
    };

    /**
     * @brief 绘图画笔参数，统一控制图形填充、描边、精度配置
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

        std::string fontPath; ///< 字体文件路径
        int fontSize;         ///< 字体大小

        static std::vector<std::pair<std::string, TTF_Font *>> fontsLoadList;
        static bool loadFont(const std::string &fontPath, int fontSize);
        static TTF_Font *getFont(const std::string &fontPath);
        static void freeAllFonts()
        {
            for (auto &pair : fontsLoadList)
            {
                if (pair.second)
                {
                    TTF_CloseFont(pair.second);
                    pair.second = nullptr;
                }
            }
        }

        DrawPen()
            : lineColor{1, 1, 1, 1},
              lineWidth(2.0f),
              fillColor{1, 1, 1, 1},
              isFilledWithBorder(true),
              eraseColor{0.05f, 0.05f, 0.08f, 1.0f},
              pointPrecision(16),
              circlePrecision(40),
              fontPath("c:\\WINDOWS\\Fonts\\CONSOLA.TTF"),
              fontSize(40)
        {
        }

        DrawPen &setLineColor(Color c)
        {
            lineColor = c;
            return *this;
        }
        DrawPen &setLineWidth(float w)
        {
            lineWidth = w;
            return *this;
        }
        DrawPen &setFillColor(Color c)
        {
            fillColor = c;
            return *this;
        }
        DrawPen &setFilledWithBorder(bool enable)
        {
            isFilledWithBorder = enable;
            return *this;
        }
        DrawPen &setEraseColor(Color c)
        {
            eraseColor = c;
            return *this;
        }
        DrawPen &setPointPrecision(int prec)
        {
            pointPrecision = prec;
            return *this;
        }
        DrawPen &setCirclePrecision(int prec)
        {
            circlePrecision = prec;
            return *this;
        }

        DrawPen &setFontPath(const std::string &path)
        {
            fontPath = path;
            return *this;
        }

        DrawPen &setFontSize(int size)
        {
            fontSize = size;
            return *this;
        }
    };

    /**
     * @brief 二维浮点点/向量类，窗口坐标系Y轴向下
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
        Point rotateAround(const Point &pivot, float angle) const;
    };

    //===== 几何工具函数 =====
    float cross(const Point &a, const Point &b, const Point &c);
    bool pointInTriangle(const Point &p, const Point &a, const Point &b, const Point &c);
    bool isConvex(const Point &a, const Point &b, const Point &c);
    bool earClipTriangulate(const Point poly[], size_t length, std::vector<size_t> &outTris);

    /**
     * @brief Area：所有可绘制2D图形的抽象基类
     */
    class Area
    {
    public:
        Point pos;      ///< 图形基准位置（世界坐标）
        float rotation; ///< 旋转角度，**弧度**，窗口Y向下，正值顺时针旋转
        Point scale;    ///< X/Y缩放，{1,1}原始大小

        Area() : pos{0, 0}, rotation(0.f), scale{1.f, 1.f} {}
        Area(Point p) : pos(p), rotation(0.f), scale{1.f, 1.f} {}
        virtual ~Area() = default;

        /**
         * @brief 判断【世界窗口坐标点p】是否落在图形内部
         * @param p 世界窗口像素坐标
         * @return true点在内部；false外部
         */
        virtual bool contains(const Point &p) const = 0;

        Point localToWorld(const Point &local) const;
        Point worldToLocal(const Point &world) const;

        /**
         * @brief 多态绘制接口，子类实现，完成自身全部绘制逻辑
         * @param win 目标窗口
         * @param pen 绘图画笔参数
         */
        virtual void drawOnWindow(const Window *win, const DrawPen &pen) const = 0;
    };

    /**
     * @brief 实心圆，继承Area；pos=圆心
     */
    class Circle : public Area
    {
    public:
        float r; ///< 局部坐标系半径
        Circle() : Area(), r(10.f) {}
        Circle(Point center, float radius) : Area(center), r(radius) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief 圆环(空心环)，继承Area；pos=圆心
     */
    class Ring : public Area
    {
    public:
        float outerR;
        float innerR;
        Ring() : Area(), outerR(30.f), innerR(15.f) {}
        Ring(Point center, float or_, float ir_) : Area(center), outerR(or_), innerR(ir_) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief 轴对齐椭圆，继承Area；pos=椭圆圆心
     */
    class Ellipse : public Area
    {
    public:
        float rx; ///< 局部X半长轴
        float ry; ///< 局部Y半短轴
        Ellipse() : Area(), rx(40.f), ry(25.f) {}
        Ellipse(Point center, float rx_, float ry_) : Area(center), rx(rx_), ry(ry_) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief 扇形（饼图扇区）继承Area；pos=扇形圆心
     */
    class Sector : public Area
    {
    public:
        float radius;   ///< 局部半径
        float startAng; ///< 起始弧度（局部坐标系）
        float endAng;   ///< 结束弧度（局部坐标系）
        Sector() : Area(), radius(30.f), startAng(0), endAng(std::numbers::pi / 2.f) {}
        Sector(Point c, float r, float s, float e) : Area(c), radius(r), startAng(s), endAng(e) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief 轴对齐矩形Rect，继承Area；pos=左上角局部原点
     */
    class Rect : public Area
    {
    public:
        float width;
        float height;
        Rect() : Area(), width(80.f), height(60.f) {}
        Rect(Point topLeft, float w, float h) : Area(topLeft), width(w), height(h) {}
        bool contains(const Point &p) const override;
        void getLocalVertices(Point out[4]) const;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief 简单多边形（凸/凹不自交）继承Area
     */
    class Polygon : public Area
    {
    public:
        std::vector<Point> localPoints; ///< 局部坐标系顶点
        Polygon() : Area() {}
        explicit Polygon(std::vector<Point> pts) : Area(), localPoints(std::move(pts)) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    class Text : public Area
    {
    public:
        std::string content;
        Text() : Area(), content("Hello") {}
        Text(Point p, const std::string &text)
            : Area(p), content(text) {}
        bool contains(const Point &p) const override;
        void drawOnWindow(const Window *win, const DrawPen &pen) const override;
    };

    /**
     * @brief Xiao2D窗口类，封装SDL窗口渲染器
     */
    class Window
    {
    public:
        friend class App;
        friend class Text;
        Window();
        Window(const std::string title, int width, int height);
        void eraseAll(const DrawPen &drawPen);

        //===== 多态绘制Area派生对象 =====
        void draw(const Area &area, const DrawPen &drawPen);
        void draw(const Area &area);

        //===== 简单点绘制（不属于Area体系） =====
        void draw(Point point, const DrawPen &drawPen);
        void draw(Point point);

    public:
        SDL_Window *_window{nullptr};
        SDL_Renderer *_renderer{nullptr};
        DrawPen drawPen;
        std::function<void(float deltaTime)> updateCallback;
        std::function<void(SDL_Event *event)> eventCallback;
        void submitGeometry(const std::vector<SDL_Vertex> &verts) const;

    private:
        TTF_TextEngine *_textEngine;
        std::string title;
        int width{800}, height{600};
        void create();
        void destroy();
    };
    enum class Key : int
    {
        A = SDL_SCANCODE_A,
        B = SDL_SCANCODE_B,
        C = SDL_SCANCODE_C,
        D = SDL_SCANCODE_D,
        E = SDL_SCANCODE_E,
        F = SDL_SCANCODE_F,
        G = SDL_SCANCODE_G,
        H = SDL_SCANCODE_H,
        I = SDL_SCANCODE_I,
        J = SDL_SCANCODE_J,
        K = SDL_SCANCODE_K,
        L = SDL_SCANCODE_L,
        M = SDL_SCANCODE_M,
        N = SDL_SCANCODE_N,
        O = SDL_SCANCODE_O,
        P = SDL_SCANCODE_P,
        Q = SDL_SCANCODE_Q,
        R = SDL_SCANCODE_R,
        S = SDL_SCANCODE_S,
        T = SDL_SCANCODE_T,
        U = SDL_SCANCODE_U,
        V = SDL_SCANCODE_V,
        W = SDL_SCANCODE_W,
        X = SDL_SCANCODE_X,
        Y = SDL_SCANCODE_Y,
        Z = SDL_SCANCODE_Z,
        Space = SDL_SCANCODE_SPACE,
        Enter = SDL_SCANCODE_RETURN,
        Escape = SDL_SCANCODE_ESCAPE,
        ShiftLeft = SDL_SCANCODE_LSHIFT,
        ShiftRight = SDL_SCANCODE_RSHIFT,
        CtrlLeft = SDL_SCANCODE_LCTRL,
        CtrlRight = SDL_SCANCODE_RCTRL,
        MouseLeft,
        MouseRight,
        MouseMiddle,
    };

    class App
    {

    public:
    
        static bool running;
        static Point getMouseGlobalPos();
        static Point getMouseWindowPos(const Window *win);

        template <typename... Rest>
        static bool isAnyKeyPressed(Key key, Rest... rest);
        inline static bool isAnyKeyPressed() { return false; }

        template <typename... Rest>
        static bool isAllKeyPressed(Key key, Rest... rest);
        inline static bool isAllKeyPressed() { return true; }

        static int init();
        static void addWindow(Window *win);
        static void addWindow(Window *wins[], int winNum);
        static void run();
        static void stopRun();
        static void quit();
        static float getDeltaTime();
        static float getFPS();

    private:
        static void updateMouseState();
        static std::vector<Window *> wins;
        static Uint64 perfFreq;
        static Uint64 lastPerfCount;
        static float deltaTime;
        static float fps;
        static Window *findWindowByID(Uint32 windowId);
    };

    //inline定义
    template <typename... Rest>
    inline bool App::isAnyKeyPressed(Key key, Rest... rest)
    {
        // C++11静态断言，强制所有参数是Key
        static_assert(std::is_same<Key, typename std::decay<decltype(key)>::type>::value, "参数必须为Key");
        if ((key == Key::MouseLeft || key == Key::MouseRight || key == Key::MouseMiddle))
        {
            SDL_MouseButtonFlags nowState = SDL_GetMouseState(nullptr, nullptr);
            SDL_MouseButtonFlags wantKey;
            switch (key)
            {
            case Key::MouseLeft:
                wantKey = SDL_BUTTON_LEFT;
                break;
            case Key::MouseRight:
                wantKey = SDL_BUTTON_RIGHT;
                break;
            case Key::MouseMiddle:
                wantKey = SDL_BUTTON_MIDDLE;
                break;
            default:
                wantKey = 0;
                break;
            }
            if ((nowState & SDL_BUTTON_MASK(wantKey)) != 0)
                return true;
            else
                return isAnyKeyPressed(rest...);
        }
        else
        {
            const bool *keystate = SDL_GetKeyboardState(nullptr);
            if (keystate[static_cast<int>(key)])
                return true;
            else
                return isAnyKeyPressed(rest...);
        }
    }


    template <typename... Rest>
    inline bool App::isAllKeyPressed(Key key, Rest... rest)
    {
        // C++11静态断言，强制所有参数是Key
        static_assert(std::is_same<Key, typename std::decay<decltype(key)>::type>::value, "参数必须为Key");
        if ((key == Key::MouseLeft || key == Key::MouseRight || key == Key::MouseMiddle))
        {
            SDL_MouseButtonFlags nowState = SDL_GetMouseState(nullptr, nullptr);
            SDL_MouseButtonFlags wantKey;
            switch (key)
            {
            case Key::MouseLeft:
                wantKey = SDL_BUTTON_LEFT;
                break;
            case Key::MouseRight:
                wantKey = SDL_BUTTON_RIGHT;
                break;
            case Key::MouseMiddle:
                wantKey = SDL_BUTTON_MIDDLE;
                break;
            default:
                wantKey = 0;
                break;
            }
            if ((nowState & SDL_BUTTON_MASK(wantKey)) != SDL_BUTTON_MASK(wantKey))
                return false;
            else
                return isAllKeyPressed(rest...);
        }
        else
        {
            const bool *keystate = SDL_GetKeyboardState(nullptr);
            if (!keystate[static_cast<int>(key)])
                return false;
            else
                return isAllKeyPressed(rest...);
        }
    }
}
