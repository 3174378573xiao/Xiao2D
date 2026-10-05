#include "XUI.h"
#include <iostream>
#include <cmath>

int main()
{
    if (XUI::App::init() != 0)
    {
        std::cerr << "XUI App初始化失败\n";
        return -1;
    }

    // ========== 窗口1：几何图形演示窗口 ==========
    XUI::Window winGeo("几何测试窗口", 900, 700);

    // 动画变量
    float rotateAngle = 0.0f;
    float circleOffsetX = 0.0f;

    winGeo.updateCallback = [&](float dt)
    {
        rotateAngle += 1.2f * dt;
        circleOffsetX = std::sin(rotateAngle) * 180.0f;

        // ---------------- 1.绘制轴对齐矩形 ----------------
        XUI::DrawPen penRect;
        penRect
            .setFillColor({0.15f, 0.45f, 0.70f, 1.0f})
            .setLineColor({1.0f, 1.0f, 1.0f, 1.0f})
            .setLineWidth(3.0f)
            .setFilledWithBorder(true);

        XUI::Rect rectA = XUI::Rect::CreateWithTLWH({40, 40}, 220, 140);
        winGeo.draw(rectA, penRect);

        // ---------------- 2.斜矩形（平行四边形） ----------------
        XUI::Point midTop{480, 110};
        XUI::Point midBot{620, 240};
        XUI::Rect rectSlant = XUI::Rect::CreateFromTwoMidPoints(midTop, midBot, 160);
        XUI::DrawPen penSlant;
        penSlant
            .setFillColor({0.7f, 0.2f, 0.3f, 0.85f})
            .setLineColor({1,1,0,1})
            .setLineWidth(2.5f);
        winGeo.draw(rectSlant, penSlant);

        // ----------------3.实心圆（带动画偏移）----------------
        XUI::DrawPen penCircle;
        penCircle
            .setFillColor({0.9f,0.6f,0.1f,1})
            .setLineColor({1,1,1,1})
            .setLineWidth(4.f)
            .setCirclePrecision(60);
        XUI::Circle cir{{350 + circleOffsetX, 380}, 75};
        winGeo.draw(cir, penCircle);

        // ----------------4.圆环Ring ----------------
        XUI::DrawPen penRing;
        penRing
            .setFillColor({0.2f,0.8f,0.3f,0.75f})
            .setLineColor({1,1,1,1})
            .setLineWidth(2.f);
        XUI::Ring ring;
        ring.point = {720, 400};
        ring.outerR = 90;
        ring.innerR = 55;
        winGeo.draw(ring, penRing);

        // ----------------5.粗线条Line（圆角线帽）----------------
        XUI::DrawPen penLine;
        penLine
            .setLineColor({0.9,0.2,0.9,1})
            .setLineWidth(12.f);
        XUI::Point p1{60, 520};
        XUI::Point p2{820, 520};
        XUI::Line line{p1,p2};
        winGeo.draw(line, penLine);

        // ----------------6.绘制点（内部小圆）----------------
        XUI::DrawPen penPoint;
        penPoint
            .setLineColor({1,0,0,1})
            .setLineWidth(10.f);
        winGeo.draw({120,620}, penPoint);
        winGeo.draw({220,620}, penPoint);
        winGeo.draw({320,620}, penPoint);

        // 打印FPS到控制台
        static int printCounter = 0;
        printCounter++;
        if(printCounter >=30)
        {
            printCounter =0;
            std::cout << "[GeoWin] FPS:" << XUI::App::getFPS()
                      << "  dt=" << XUI::App::getDeltaTime() << "\n";
        }
    };

    // 窗口1事件回调
    winGeo.eventCallback = [](SDL_Event* ev)
    {
        if(ev->type == SDL_EVENT_KEY_DOWN)
        {
            std::cout << "[窗口1] 按键按下 keycode=" << ev->key.key << "\n";
        }
        if(ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            std::cout << "[窗口1]鼠标点击 x:" << ev->button.x << " y:" << ev->button.y << "\n";
        }
    };

    // ========== 窗口2：简单动画窗口（独立渲染，独立事件） ==========
    XUI::Window winAnim("第二个测试窗口",600,450);
    float ballX = 300;
    float ballVx = 130.0f;

    winAnim.updateCallback = [&](float dt)
    {
        ballX += ballVx * dt;
        if(ballX >530 || ballX <70)
        {
            ballVx *= -1;
        }
        XUI::Circle ball{{ballX,220},50};
        winAnim.draw(ball); // 使用窗口默认drawPen
    };

    winAnim.eventCallback = [](SDL_Event* ev)
    {
        if(ev->type == SDL_EVENT_KEY_DOWN && ev->key.key == SDLK_ESCAPE)
        {
            std::cout << "[窗口2] 按下ESC，本窗口即将关闭\n";
        }
    };

    // 将两个窗口交给App托管
    XUI::App::addWindow(&winGeo);
    XUI::App::addWindow(&winAnim);

    std::cout << "==== XUI测试程序启动 ====\n";
    std::cout << "功能：多窗口、圆/圆环/线条/斜矩形、动画、事件、FPS统计\n";
    std::cout << "关闭全部窗口程序退出\n";

    // 启动主循环（阻塞）
    XUI::App::run();

    std::cout << "程序正常退出\n";
    return 0;
}
