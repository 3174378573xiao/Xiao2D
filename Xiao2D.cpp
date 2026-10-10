#include "Xiao2D.h"
namespace Xiao2D
{

    // ========= Point运算符与工具实现 =========
    Point Point::operator+(const Point &other) const
    {
        return Point{x + other.x, y + other.y};
    }
    Point Point::operator-(const Point &other) const
    {
        return Point{x - other.x, y - other.y};
    }
    Point Point::operator*(float s) const
    {
        return Point{x * s, y * s};
    }
    Point Point::operator/(float s) const
    {
        return Point{x / s, y / s};
    }
    float Point::length() const
    {
        return std::sqrt(x * x + y * y);
    }
    Point Point::normalize() const
    {
        float len = length();
        if (len < 1e-6f)
            return Point{0, 0};
        return *this / len;
    }
    Point Point::perpendicularCCW() const
    {
        // 向量(x,y)逆时针垂直 = (-y, x)
        return Point{-y, x};
    }
    Point Point::perpendicularCW() const
    {
        // 顺时针垂直 = (y, -x)
        return Point{y, -x};
    }

    //===== 多边形三角剖分几何辅助实现 =====
    float cross(const Point &a, const Point &b, const Point &c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    bool pointInTriangle(const Point &p, const Point &a, const Point &b, const Point &c)
    {
        float c1 = cross(a, b, p);
        float c2 = cross(b, c, p);
        float c3 = cross(c, a, p);
        bool pos = (c1 >= -1e-6f) && (c2 >= -1e-6f) && (c3 >= -1e-6f);
        bool neg = (c1 <= 1e-6f) && (c2 <= 1e-6f) && (c3 <= 1e-6f);
        return pos || neg;
    }

    bool isConvex(const Point &a, const Point &b, const Point &c)
    {
        // 逆时针多边形，cross>0代表b是凸顶点
        return cross(a, b, c) > 1e-6f;
    }

    bool earClipTriangulate(const Point poly[], size_t length, std::vector<size_t> &outTris)
    {
        outTris.clear();
        if (length < 3)
            return false;

        // 维护剩余顶点索引列表
        std::vector<size_t> idxList;
        for (size_t i = 0; i < length; i++)
            idxList.push_back(i);

        size_t count = idxList.size();
        while (count > 3)
        {
            bool earFound = false;
            for (size_t i = 0; i < count; i++)
            {
                size_t iPrev = (i == 0) ? count - 1 : i - 1;
                size_t iCurr = i;
                size_t iNext = (i + 1) % count;

                Point a = poly[idxList[iPrev]];
                Point b = poly[idxList[iCurr]];
                Point c = poly[idxList[iNext]];

                // 耳朵条件1：当前顶点是凸顶点
                if (!isConvex(a, b, c))
                    continue;

                // 耳朵条件2：三角形abc内部不包含其他任何剩余顶点
                bool hasPointInside = false;
                for (size_t j = 0; j < count; j++)
                {
                    if (j == iPrev || j == iCurr || j == iNext)
                        continue;
                    Point p = poly[idxList[j]];
                    if (pointInTriangle(p, a, b, c))
                    {
                        hasPointInside = true;
                        break;
                    }
                }
                if (hasPointInside)
                    continue;

                // ✔ 找到耳朵，输出三角形(a,b,c)
                outTris.push_back(idxList[iPrev]);
                outTris.push_back(idxList[iCurr]);
                outTris.push_back(idxList[iNext]);

                // 裁剪耳朵，移除当前耳朵顶点
                idxList.erase(idxList.begin() + i);
                count--;
                earFound = true;
                break;
            }
            if (!earFound)
            {
                // 找不到耳朵：多边形自交或者退化，剖分失败
                return false;
            }
        }
        // 剩下最后3个顶点输出最后一个三角形
        outTris.push_back(idxList[0]);
        outTris.push_back(idxList[1]);
        outTris.push_back(idxList[2]);
        return true;
    }

    // ========= Rect两种构造实现 =========
    Rect::Rect(Point topLeft, float w, float h)
    {
        v[0] = topLeft;
        v[1] = Point{topLeft.x + w, topLeft.y};
        v[2] = Point{topLeft.x + w, topLeft.y + h};
        v[3] = Point{topLeft.x, topLeft.y + h};
    }

    Rect::Rect(Point midTop, Point midBot, float width)
    {
        Point dirLine = midBot - midTop;
        Point perp = dirLine.perpendicularCCW().normalize();
        float halfW = width * 0.5f;
        // 上边两个角
        v[0] = midTop + perp * halfW;
        v[1] = midTop - perp * halfW;
        // 下边两个角
        v[2] = midBot - perp * halfW;
        v[3] = midBot + perp * halfW;
    }

    //=====内部辅助：生成圆环顶点索引=====
    static void buildRingGeometry(Point center, float outerR, float innerR, int segCount, Color color,
                                  std::vector<SDL_Vertex> &outVerts, std::vector<int> &outIndices)
    {
        outVerts.clear();
        outIndices.clear();
        if (outerR <= 0 || innerR < 0 || innerR >= outerR || segCount < 3)
            return;

        for (int i = 0; i <= segCount; i++)
        {
            float ang = 2.f * std::numbers::pi * i / segCount;
            float cx = center.x;
            float cy = center.y;

            SDL_Vertex vOut{};
            vOut.position.x = cx + outerR * std::cos(ang);
            vOut.position.y = cy + outerR * std::sin(ang);
            vOut.color = color.toSDL_FColor();
            outVerts.push_back(vOut);

            SDL_Vertex vIn{};
            vIn.position.x = cx + innerR * std::cos(ang);
            vIn.position.y = cy + innerR * std::sin(ang);
            vIn.color = color.toSDL_FColor();
            outVerts.push_back(vIn);
        }

        for (int i = 0; i < segCount; i++)
        {
            int o0 = i * 2;
            int i0 = i * 2 + 1;
            int o1 = (i + 1) * 2;
            int i1 = (i + 1) * 2 + 1;

            outIndices.push_back(o0);
            outIndices.push_back(i0);
            outIndices.push_back(o1);

            outIndices.push_back(o1);
            outIndices.push_back(i0);
            outIndices.push_back(i1);
        }
    }

    // ========= Window =========
    Window::Window() : Window("none", 800, 600) {}

    Window::Window(const std::string _title, int _width, int _height) : title(_title), width(_width), height(_height) {}

    void Window::create()
    {

        // 启动后再创建窗口
        if (!SDL_CreateWindowAndRenderer(title.c_str(), width, height, SDL_WINDOW_RESIZABLE, &this->_window, &this->_renderer))
        {
            std::cout << "创建窗口渲染器失败: " << SDL_GetError() << std::endl;
            _window = nullptr;
            _renderer = nullptr;
            return;
        }
        // 开启alpha透明混合，RenderGeometry生效
        SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
    }

    void Window::destroy()
    {
        if (_renderer)
        {
            SDL_DestroyRenderer(_renderer);
            _renderer = nullptr;
        }
        if (_window)
        {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }
    }

    // 内部辅助：提交顶点数组
    void Window::submitGeometry(const std::vector<SDL_Vertex> &verts)
    {
        if (!_renderer)
            return;
        if (verts.size() % 3 != 0)
            return;
        SDL_RenderGeometry(_renderer, nullptr, verts.data(), (int)verts.size(), nullptr, 0);
    }

    void Window::eraseAll(const DrawPen &drawPen)
    {
        SDL_SetRenderDrawColorFloat(
            this->_renderer,
            drawPen.eraseColor.r,
            drawPen.eraseColor.g,
            drawPen.eraseColor.b,
            drawPen.eraseColor.a);
        SDL_RenderClear(this->_renderer);
    }

    void Window::draw(Point point, const DrawPen &drawPen)
    {
        DrawPen tmpPen = drawPen;
        tmpPen.isFilledWithBorder = false;
        tmpPen.circlePrecision = tmpPen.pointPrecision;
        tmpPen.fillColor = tmpPen.lineColor;
        draw(Circle{point, tmpPen.lineWidth}, tmpPen);
    }

    void Window::draw(Circle circle, const DrawPen &drawPen)
    {
        if (_renderer == nullptr)
            return;
        int N = drawPen.circlePrecision;
        if (N < 3)
            return;
        float r = circle.r;
        if (r <= 0)
            return;

        // 1.绘制单层实心填充圆
        {
            DrawPen tmpPen = drawPen;
            tmpPen.isFilledWithBorder = false;
            std::vector<SDL_Vertex> vertices(N + 1);
            vertices[0].position.x = circle.point.x;
            vertices[0].position.y = circle.point.y;
            vertices[0].color = tmpPen.fillColor.toSDL_FColor();
            for (int i = 1; i <= N; i++)
            {
                float angle = 2 * std::numbers::pi * i / N;
                vertices[i].position.x = circle.point.x + r * std::cos(angle);
                vertices[i].position.y = circle.point.y + r * std::sin(angle);
                vertices[i].color = tmpPen.fillColor.toSDL_FColor();
            }
            std::vector<int> indices(3 * N);
            for (int i = 0; i < N; i++)
            {
                indices[3 * i + 0] = 0;
                indices[3 * i + 1] = i + 1;
                indices[3 * i + 2] = i + 2;
            }
            indices[3 * N - 1] = 1;
            SDL_RenderGeometry(_renderer, nullptr,
                               vertices.data(), (int)vertices.size(),
                               indices.data(), (int)indices.size());
        }

        // 2.开启描边：绘制独立圆环Ring，修复半透明颜色叠加bug
        if (drawPen.isFilledWithBorder)
        {
            DrawPen ringPen = drawPen;
            ringPen.isFilledWithBorder = false;
            ringPen.fillColor = drawPen.lineColor;

            Ring strokeRing{};
            strokeRing.point = circle.point;
            strokeRing.outerR = r;
            strokeRing.innerR = std::max(0.f, r - drawPen.lineWidth);
            draw(strokeRing, ringPen);
        }
    }

    void Window::draw(Ring ring, const DrawPen &drawPen)
    {
        if (!_renderer)
            return;

        int seg = drawPen.circlePrecision;
        if (seg < 3)
            return;

        float outerR = ring.outerR;
        float innerR = ring.innerR;

        if (outerR <= 0 || innerR < 0 || innerR >= outerR)
            return;

        // ========== 1. 绘制圆环本体填充 ==========
        {
            DrawPen tmpPen = drawPen;
            tmpPen.isFilledWithBorder = false;

            std::vector<SDL_Vertex> verts;
            std::vector<int> idx;
            buildRingGeometry(ring.point, outerR, innerR, seg, tmpPen.fillColor, verts, idx);

            SDL_RenderGeometry(_renderer, nullptr,
                               verts.data(), (int)verts.size(),
                               idx.data(), (int)idx.size());
        }

        // ========== 2. 开启描边：外圈 + 内圈 ==========
        if (drawPen.isFilledWithBorder)
        {
            DrawPen strokePen = drawPen;
            strokePen.isFilledWithBorder = false;
            strokePen.fillColor = drawPen.lineColor;

            // ---- 外圈描边 ----
            Ring outerStrokeRing{};
            outerStrokeRing.point = ring.point;
            outerStrokeRing.outerR = outerR;
            outerStrokeRing.innerR = std::max(0.f, outerR - drawPen.lineWidth);
            draw(outerStrokeRing, strokePen);

            // ---- 内圈描边 ----
            Ring innerStrokeRing{};
            innerStrokeRing.point = ring.point;
            innerStrokeRing.outerR = innerR;
            innerStrokeRing.innerR = std::max(0.f, innerR - drawPen.lineWidth);
            draw(innerStrokeRing, strokePen);
        }
    }

    /// 绘制线段：中间斜矩形 + 两端圆形端点（圆角线帽）
    void Window::draw(Line line, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        float lw = pen.lineWidth;
        if (lw <= 0)
            return;
        Point p1 = line.p1;
        Point p2 = line.p2;
        Point dir = p2 - p1;
        float len = dir.length();
        if (len < 1e-6f)
            return;
        std::vector<SDL_Vertex> verts;
        auto addV = [&](Point pt, Color c)
        {
            SDL_Vertex v{};
            v.position.x = pt.x;
            v.position.y = pt.y;
            v.color = c.toSDL_FColor();
            verts.push_back(v);
        };
        // --------中间斜矩形主体---------
        Point n = dir.perpendicularCCW().normalize();
        Point offset = n * (lw * 0.5f);
        Point A = p1 + offset;
        Point B = p1 - offset;
        Point C = p2 - offset;
        Point D = p2 + offset;
        Color lineCol = pen.lineColor;
        addV(A, lineCol);
        addV(B, lineCol);
        addV(D, lineCol);
        addV(B, lineCol);
        addV(C, lineCol);
        addV(D, lineCol);
        submitGeometry(verts);
        verts.clear();

        // --------两端圆形线帽（两个小圆）---------
        DrawPen capPen = pen;
        capPen.isFilledWithBorder = false;
        capPen.fillColor = pen.lineColor;
        capPen.circlePrecision = pen.pointPrecision;
        draw(Circle{p1, lw * 0.5f}, capPen);
        draw(Circle{p2, lw * 0.5f}, capPen);
    }

    /// 绘制矩形（支持轴对齐/斜平行四边形），支持填充+描边
    void Window::draw(const Rect &rect, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        std::vector<SDL_Vertex> verts;
        auto addV = [&](Point pt, Color c)
        {
            SDL_Vertex v{};
            v.position.x = pt.x;
            v.position.y = pt.y;
            v.color = c.toSDL_FColor();
            verts.push_back(v);
        };
        // =========填充部分=========
        if (pen.isFilledWithBorder)
        {
            addV(rect.v[0], pen.fillColor);
            addV(rect.v[1], pen.fillColor);
            addV(rect.v[2], pen.fillColor);

            addV(rect.v[0], pen.fillColor);
            addV(rect.v[2], pen.fillColor);
            addV(rect.v[3], pen.fillColor);
            submitGeometry(verts);
            verts.clear();
        }
        // =========描边：四条线段=========
        DrawPen borderPen = pen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = pen.lineColor;
        borderPen.lineWidth = pen.lineWidth;

        draw(Line{rect.v[0], rect.v[1]}, borderPen);
        draw(Line{rect.v[1], rect.v[2]}, borderPen);
        draw(Line{rect.v[2], rect.v[3]}, borderPen);
        draw(Line{rect.v[3], rect.v[0]}, borderPen);
    }

    //==== draw重载：不传画笔，使用窗口默认drawPen ====
    void Window::draw(Point point)
    {
        draw(point, this->drawPen);
    }

    void Window::draw(Circle circle)
    {
        draw(circle, this->drawPen);
    }

    void Window::draw(Ring ring)
    {
        draw(ring, this->drawPen);
    }

    void Window::draw(Line line)
    {
        draw(line, this->drawPen);
    }

    void Window::draw(const Rect &rect)
    {
        draw(rect, this->drawPen);
    }

    void Window::draw(const Point points[], size_t length)
    {
        draw(points, length, this->drawPen);
    }

    void Window::draw(const Point points[], size_t length, const DrawPen &drawPen)
    {
        if (!_renderer)
            return;
        if (length < 3 || points == nullptr)
            return;

        // ---------- 填充绘制：耳切剖分 ----------
        if (drawPen.isFilledWithBorder)
        {
            DrawPen tmpPen = drawPen;
            tmpPen.isFilledWithBorder = false;
            std::vector<size_t> triIndices;
            bool ok = earClipTriangulate(points, length, triIndices);
            if (ok)
            {
                std::vector<SDL_Vertex> verts;
                auto addV = [&](Point pt, Color c)
                {
                    SDL_Vertex v{};
                    v.position.x = pt.x;
                    v.position.y = pt.y;
                    v.color = c.toSDL_FColor();
                    verts.push_back(v);
                };
                // triIndices每3个为一个三角形索引
                for (size_t t = 0; t < triIndices.size(); t += 3)
                {
                    size_t i0 = triIndices[t];
                    size_t i1 = triIndices[t + 1];
                    size_t i2 = triIndices[t + 2];
                    addV(points[i0], tmpPen.fillColor);
                    addV(points[i1], tmpPen.fillColor);
                    addV(points[i2], tmpPen.fillColor);
                }
                submitGeometry(verts);
            }
        }

        // ---------- 描边线框绘制：闭合多边形，复用Line（自带圆角线帽） ----------
        DrawPen borderPen = drawPen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = drawPen.lineColor;

        for (size_t i = 0; i < length; i++)
        {
            size_t j = (i + 1) % length;
            draw(Line{points[i], points[j]}, borderPen);
        }
    }

    std::vector<Window *> App::wins;
    Uint64 App::perfFreq = 0;
    Uint64 App::lastPerfCount = 0;
    float App::deltaTime = 0.0f;
    float App::fps = 0.0f;

    Window *App::findWindowByID(Uint32 windowId)
    {
        for (auto w : wins)
        {
            if (w && w->_window)
            {
                auto id = SDL_GetWindowID(w->_window);
                if (id == windowId)
                    return w;
            }
        }
        return nullptr;
    }

    void App::addWindow(Window *win)
    {
        if (!win)
            return;
        win->create(); // 调用Window创建窗口渲染器
        wins.push_back(win);
    }

    void App::addWindow(Window *wins[], int winNum)
    {
        for (int i = 0; i < winNum; i++)
        {
            addWindow(wins[i]);
        }
    }

    void App::quit()
    {
        // 销毁所有窗口
        for (auto w : wins)
        {
            if (w)
            {
                w->destroy();
            }
        }
        wins.clear();
        SDL_Quit();
    }

    float App::getDeltaTime()
    {
        return App::deltaTime;
    }

    float App::getFPS()
    {
        return App::fps;
    }

    void App::run()
    {
        bool running = true;
        SDL_Event event;

        // 初始化高精度计时器
        perfFreq = SDL_GetPerformanceFrequency();
        lastPerfCount = SDL_GetPerformanceCounter();

        const float maxFrameTime = 1.0f / 60.0f; // 目标60fps，最小帧间隔
        const float maxDeltaTime = 0.1f;         // 最大deltaTime上限，防止卡顿跳变（最小10fps）

        while (running)
        {
            // ========= 计算deltaTime =========
            Uint64 now = SDL_GetPerformanceCounter();
            deltaTime = (float)(now - lastPerfCount) / (float)perfFreq;
            lastPerfCount = now;

            // 钳位，避免卡顿后deltaTime爆炸
            if (deltaTime > maxDeltaTime)
                deltaTime = maxDeltaTime;

            // 计算瞬时FPS
            if (deltaTime > 1e-6f)
            {
                fps = 1.0f / deltaTime;
            }

            // ========= 事件循环 =========
            while (SDL_PollEvent(&event))
            {
                // 全局退出
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                    break;
                }

                // 窗口关闭请求
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                {
                    Uint32 closeId = event.window.windowID;
                    Window *win = findWindowByID(closeId);
                    if (win)
                    {
                        win->destroy();
                    }
                }

                // 将事件分发到对应窗口的eventCallback
                if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
                {
                    Uint32 evtWinId = event.window.windowID;
                    Window *win = findWindowByID(evtWinId);
                    if (win && win->eventCallback)
                    {
                        win->eventCallback(&event);
                    }
                }
                else
                {
                    // 键盘、鼠标等非窗口事件：广播给所有存活窗口
                    for (auto w : wins)
                    {
                        if (w && w->_window && w->eventCallback)
                        {
                            w->eventCallback(&event);
                        }
                    }
                }
            }

            // 清理已经destroy的窗口，从vector擦除
            std::vector<Window *> aliveWins;
            for (auto w : wins)
            {
                if (w && w->_window != nullptr)
                {
                    aliveWins.push_back(w);
                }
            }
            wins.swap(aliveWins);

            // 全部窗口关闭 → 退出主循环
            if (wins.empty())
            {
                running = false;
                break;
            }

            // ========= 每一帧更新 + 渲染每个窗口 =========
            for (auto w : wins)
            {
                if (!w || !w->_renderer)
                    continue;

                // 1.清空画布
                w->eraseAll(w->drawPen);

                // 2.执行用户的更新绘图回调，传入deltaTime
                if (w->updateCallback)
                {
                    w->updateCallback(deltaTime);
                }

                // 3.提交渲染（每个窗口独立Present）
                SDL_RenderPresent(w->_renderer);
            }

            // 帧率节流：如果本帧耗时小于目标帧时间，则sleep剩余时间
            float frameElapsed = (float)(SDL_GetPerformanceCounter() - lastPerfCount) / perfFreq;
            if (frameElapsed < maxFrameTime)
            {
                Uint32 sleepMs = (Uint32)((maxFrameTime - frameElapsed) * 1000.0f);
                if (sleepMs > 0)
                {
                    SDL_Delay(sleepMs);
                }
            }
        }

        quit();
    }
} // namespace Xiao2D