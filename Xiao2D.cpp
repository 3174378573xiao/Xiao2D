#include "Xiao2D.h"
namespace Xiao2D
{
    //======== Point实现 ========
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
            return {0, 0};
        return *this / len;
    }
    Point Point::perpendicularCCW() const
    {
        return Point{-y, x};
    }
    Point Point::perpendicularCW() const
    {
        return Point{y, -x};
    }
    Point Point::rotate(float angle) const
    {
        // Y向下：顺时针旋转矩阵
        float c = std::cos(angle);
        float s = std::sin(angle);
        return Point{x * c + y * s, -x * s + y * c};
    }
    Point Point::rotateAround(const Point &pivot, float angle) const
    {
        return (*this - pivot).rotate(angle) + pivot;
    }

    //==== 几何工具函数 ====
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
        return cross(a, b, c) > 1e-6f;
    }
    bool earClipTriangulate(const Point poly[], size_t length, std::vector<size_t> &outTris)
    {
        outTris.clear();
        if (length < 3)
            return false;
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
                if (!isConvex(a, b, c))
                    continue;
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
                outTris.push_back(idxList[iPrev]);
                outTris.push_back(idxList[iCurr]);
                outTris.push_back(idxList[iNext]);
                idxList.erase(idxList.begin() + i);
                count--;
                earFound = true;
                break;
            }
            if (!earFound)
                return false;
        }
        outTris.push_back(idxList[0]);
        outTris.push_back(idxList[1]);
        outTris.push_back(idxList[2]);
        return true;
    }

    //======== Area基类变换 ========
    Point Area::localToWorld(const Point &local) const
    {
        Point p{local.x * scale.x, local.y * scale.y};
        p = p.rotate(rotation);
        p = p + pos;
        return p;
    }
    Point Area::worldToLocal(const Point &world) const
    {
        Point p = world - pos;
        p = p.rotate(-rotation);
        p.x /= scale.x;
        p.y /= scale.y;
        return p;
    }

    //======== 派生类 contains 实现 ========
    bool Circle::contains(const Point &p) const
    {
        Point l = worldToLocal(p);
        return l.length() <= r + 1e-6f;
    }
    bool Ring::contains(const Point &p) const
    {
        Point l = worldToLocal(p);
        float d = l.length();
        return d >= innerR - 1e-6f && d <= outerR + 1e-6f;
    }
    bool Ellipse::contains(const Point &p) const
    {
        Point l = worldToLocal(p);
        float x = l.x / rx;
        float y = l.y / ry;
        return x * x + y * y <= 1.0f + 1e-6f;
    }
    bool Sector::contains(const Point &p) const
    {
        Point l = worldToLocal(p);
        float d = l.length();
        if (d > radius + 1e-6f)
            return false;
        float ang = std::atan2(l.y, l.x);
        auto wrap = [](float a) -> float
        {
            while (a < 0)
                a += 2.f * std::numbers::pi;
            while (a >= 2.f * std::numbers::pi)
                a -= 2.f * std::numbers::pi;
            return a;
        };
        float a0 = wrap(startAng);
        float a1 = wrap(endAng);
        float ap = wrap(ang);
        if (a0 <= a1)
            return ap >= a0 - 1e-6f && ap <= a1 + 1e-6f;
        else
            return ap >= a0 - 1e-6f || ap <= a1 + 1e-6f;
    }
    bool Rect::contains(const Point &p) const
    {
        Point l = worldToLocal(p);
        return l.x >= -1e-6f && l.x <= width + 1e-6f && l.y >= -1e-6f && l.y <= height + 1e-6f;
    }
    void Rect::getLocalVertices(Point out[4]) const
    {
        out[0] = {0, 0};
        out[1] = {width, 0};
        out[2] = {width, height};
        out[3] = {0, height};
    }
    bool Polygon::contains(const Point &p) const
    {
        if (localPoints.size() < 3)
            return false;
        Point lp = worldToLocal(p);
        // 射线法
        bool inside = false;
        size_t n = localPoints.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            const Point &vi = localPoints[i];
            const Point &vj = localPoints[j];
            if (((vi.y > lp.y) != (vj.y > lp.y)))
            {
                float xIntersect = ((lp.y - vi.y) * (vj.x - vi.x)) / (vj.y - vi.y) + vi.x;
                if (lp.x <= xIntersect + 1e-6f)
                    inside = !inside;
            }
        }
        return inside;
    }

    //==== 内部辅助生成圆环顶点 ====
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

    //======== Window实现 ========
    Window::Window() : Window("none", 800, 600) {}
    Window::Window(const std::string _title, int _width, int _height)
        : title(_title), width(_width), height(_height) {}

    void Window::create()
    {
        if (!SDL_CreateWindowAndRenderer(title.c_str(), width, height, SDL_WINDOW_RESIZABLE, &_window, &_renderer))
        {
            std::cout << "创建窗口渲染器失败: " << SDL_GetError() << std::endl;
            _window = nullptr;
            _renderer = nullptr;
            return;
        }
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
        SDL_SetRenderDrawColorFloat(_renderer,
                                    drawPen.eraseColor.r, drawPen.eraseColor.g, drawPen.eraseColor.b, drawPen.eraseColor.a);
        SDL_RenderClear(_renderer);
    }

    void Window::drawCircleWorld(float rWorld, Point centerWorld, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || rWorld <= 0)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v(N + 1);
            v[0].position.x = centerWorld.x;
            v[0].position.y = centerWorld.y;
            v[0].color = tmp.fillColor.toSDL_FColor();
            for (int i = 1; i <= N; i++)
            {
                float a = 2.f * std::numbers::pi * i / N;
                v[i].position.x = centerWorld.x + rWorld * std::cos(a);
                v[i].position.y = centerWorld.y + rWorld * std::sin(a);
                v[i].color = tmp.fillColor.toSDL_FColor();
            }
            std::vector<int> idx(3 * N);
            for (int i = 0; i < N; i++)
            {
                idx[3 * i + 0] = 0;
                idx[3 * i + 1] = i + 1;
                idx[3 * i + 2] = i + 2;
            }
            idx[3 * N - 1] = 1;
            SDL_RenderGeometry(_renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
            // 描边
            DrawPen ringPen = pen;
            ringPen.isFilledWithBorder = false;
            ringPen.fillColor = pen.lineColor;
            float innerR = std::max(0.f, rWorld - pen.lineWidth);
            std::vector<SDL_Vertex> rv;
            std::vector<int> ridx;
            buildRingGeometry(centerWorld, rWorld, innerR, N, ringPen.fillColor, rv, ridx);
            SDL_RenderGeometry(_renderer, nullptr, rv.data(), (int)rv.size(), ridx.data(), (int)ridx.size());
        }
    }

    void Window::drawEllipseWorld(Point centerWorld, float rxWorld, float ryWorld, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || rxWorld <= 0 || ryWorld <= 0)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v(N + 1);
            v[0].position = {centerWorld.x, centerWorld.y};
            v[0].color = tmp.fillColor.toSDL_FColor();
            for (int i = 1; i <= N; i++)
            {
                float ang = 2.f * std::numbers::pi * i / N;
                float c = std::cos(ang), s = std::sin(ang);
                v[i].position.x = centerWorld.x + rxWorld * c;
                v[i].position.y = centerWorld.y + ryWorld * s;
                v[i].color = tmp.fillColor.toSDL_FColor();
            }
            std::vector<int> idx(3 * N);
            for (int i = 0; i < N; i++)
            {
                idx[3 * i + 0] = 0;
                idx[3 * i + 1] = i + 1;
                idx[3 * i + 2] = i + 2;
            }
            idx[3 * N - 1] = 1;
            SDL_RenderGeometry(_renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());

            // 椭圆描边简易薄环
            DrawPen ringPen = pen;
            ringPen.isFilledWithBorder = false;
            ringPen.fillColor = pen.lineColor;
            float lw = pen.lineWidth;
            float rxIn = std::max(0.f, rxWorld - lw);
            float ryIn = std::max(0.f, ryWorld - lw);
            std::vector<SDL_Vertex> verts;
            std::vector<int> strokeIdx; // ← 修复：改名，不再和上面idx重名
            for (int i = 0; i <= N; i++)
            {
                float ang = 2.f * std::numbers::pi * i / N;
                float c = std::cos(ang), s = std::sin(ang);
                SDL_Vertex vo{};
                vo.position = {centerWorld.x + rxWorld * c, centerWorld.y + ryWorld * s};
                vo.color = ringPen.fillColor.toSDL_FColor();
                verts.push_back(vo);
                SDL_Vertex vi{};
                vi.position = {centerWorld.x + rxIn * c, centerWorld.y + ryIn * s};
                vi.color = ringPen.fillColor.toSDL_FColor();
                verts.push_back(vi);
            }
            for (int i = 0; i < N; i++)
            {
                int o0 = i * 2, i0 = i * 2 + 1, o1 = (i + 1) * 2, i1 = (i + 1) * 2 + 1;
                strokeIdx.push_back(o0);
                strokeIdx.push_back(i0);
                strokeIdx.push_back(o1);
                strokeIdx.push_back(o1);
                strokeIdx.push_back(i0);
                strokeIdx.push_back(i1);
            }
            SDL_RenderGeometry(_renderer, nullptr, verts.data(), (int)verts.size(), strokeIdx.data(), (int)strokeIdx.size());
        }
    }

    void Window::drawRingWorld(Point centerWorld, float outerRWorld, float innerRWorld, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        int seg = pen.circlePrecision;
        if (seg < 3 || outerRWorld <= 0 || innerRWorld < 0 || innerRWorld >= outerRWorld)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v;
            std::vector<int> idx;
            buildRingGeometry(centerWorld, outerRWorld, innerRWorld, seg, tmp.fillColor, v, idx);
            SDL_RenderGeometry(_renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
            // 外圈描边
            DrawPen strokePen = pen;
            strokePen.isFilledWithBorder = false;
            strokePen.fillColor = pen.lineColor;
            float oIn = std::max(0.f, outerRWorld - pen.lineWidth);
            std::vector<SDL_Vertex> vo;
            std::vector<int> idxo;
            buildRingGeometry(centerWorld, outerRWorld, oIn, seg, strokePen.fillColor, vo, idxo);
            SDL_RenderGeometry(_renderer, nullptr, vo.data(), (int)vo.size(), idxo.data(), (int)idxo.size());
            // 内圈描边
            float iIn = std::max(0.f, innerRWorld - pen.lineWidth);
            std::vector<SDL_Vertex> vi;
            std::vector<int> idxi;
            buildRingGeometry(centerWorld, innerRWorld, iIn, seg, strokePen.fillColor, vi, idxi);
            SDL_RenderGeometry(_renderer, nullptr, vi.data(), (int)vi.size(), idxi.data(), (int)idxi.size());
        }
    }

    void Window::drawSectorWorld(Point centerWorld, float radiusWorld, float startAng, float endAng, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || radiusWorld <= 0)
            return;
        float span = endAng - startAng;
        if (std::fabs(span) < 1e-6f)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> verts;
            verts.push_back({{centerWorld.x, centerWorld.y}, tmp.fillColor.toSDL_FColor(), {0, 0}});
            int steps = std::max(3, (int)(std::fabs(span) / (2.f * std::numbers::pi) * N));
            for (int i = 0; i <= steps; i++)
            {
                float a = startAng + span * (float)i / steps;
                SDL_Vertex v{};
                v.position.x = centerWorld.x + radiusWorld * std::cos(a);
                v.position.y = centerWorld.y + radiusWorld * std::sin(a);
                v.color = tmp.fillColor.toSDL_FColor();
                verts.push_back(v);
            }
            std::vector<int> idx;
            for (int i = 1; i < (int)verts.size() - 1; i++)
            {
                idx.push_back(0);
                idx.push_back(i);
                idx.push_back(i + 1);
            }
            SDL_RenderGeometry(_renderer, nullptr, verts.data(), (int)verts.size(), idx.data(), (int)idx.size());
            // 简易描边：扇形圆弧外边缘 + 两条半径边；此处省略复杂扇形描边，仅填充生效；可扩展
        }
    }

    void Window::drawPolygonWorld(const Point worldPts[], size_t count, const DrawPen &pen)
    {
        if (!_renderer)
            return;
        if (count < 3)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<size_t> tris;
            bool ok = earClipTriangulate(worldPts, count, tris);
            if (ok)
            {
                std::vector<SDL_Vertex> v;
                auto add = [&](Point p)
                {
                    SDL_Vertex sv{};
                    sv.position = {p.x, p.y};
                    sv.color = tmp.fillColor.toSDL_FColor();
                    v.push_back(sv);
                };
                for (size_t t = 0; t < tris.size(); t += 3)
                {
                    add(worldPts[tris[t]]);
                    add(worldPts[tris[t + 1]]);
                    add(worldPts[tris[t + 2]]);
                }
                submitGeometry(v);
            }
        }
        // 描边线段
        DrawPen borderPen = pen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = pen.lineColor;
        for (size_t i = 0; i < count; i++)
        {
            size_t j = (i + 1) % count;
            Point p1 = worldPts[i], p2 = worldPts[j];
            // 复用线段绘制逻辑（内联简化，可迁移旧Line draw）
            float lw = borderPen.lineWidth;
            if (lw <= 0)
                continue;
            Point dir = p2 - p1;
            float len = dir.length();
            if (len < 1e-6f)
                continue;
            std::vector<SDL_Vertex> segVerts;
            auto addV = [&](Point pt)
            {
                SDL_Vertex sv{};
                sv.position = {pt.x, pt.y};
                sv.color = borderPen.fillColor.toSDL_FColor();
                segVerts.push_back(sv);
            };
            Point n = dir.perpendicularCCW().normalize();
            Point off = n * (lw * 0.5f);
            Point A = p1 + off, B = p1 - off, C = p2 - off, D = p2 + off;
            addV(A);
            addV(B);
            addV(D);
            addV(B);
            addV(C);
            addV(D);
            submitGeometry(segVerts);
        }
    }

    void Window::drawRectWorld(const Point worldVerts[4], const DrawPen &pen)
    {
        if (!_renderer)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v;
            auto add = [&](Point p)
            {
                SDL_Vertex sv{};
                sv.position = {p.x, p.y};
                sv.color = tmp.fillColor.toSDL_FColor();
                v.push_back(sv);
            };
            add(worldVerts[0]);
            add(worldVerts[1]);
            add(worldVerts[2]);
            add(worldVerts[0]);
            add(worldVerts[2]);
            add(worldVerts[3]);
            submitGeometry(v);
        }
        DrawPen borderPen = pen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = pen.lineColor;
        for (int i = 0; i < 4; i++)
        {
            int j = (i + 1) % 4;
            Point p1 = worldVerts[i], p2 = worldVerts[j];
            float lw = borderPen.lineWidth;
            if (lw <= 0)
                continue;
            Point dir = p2 - p1;
            float len = dir.length();
            if (len < 1e-6f)
                continue;
            std::vector<SDL_Vertex> segVerts;
            auto addV = [&](Point pt)
            {
                SDL_Vertex sv{};
                sv.position = {pt.x, pt.y};
                sv.color = borderPen.fillColor.toSDL_FColor();
                segVerts.push_back(sv);
            };
            Point n = dir.perpendicularCCW().normalize();
            Point off = n * (lw * 0.5f);
            Point A = p1 + off, B = p1 - off, C = p2 - off, D = p2 + off;
            addV(A);
            addV(B);
            addV(D);
            addV(B);
            addV(C);
            addV(D);
            submitGeometry(segVerts);
        }
    }

    //==== 多态入口 draw(const Area&) ====
    void Window::draw(const Area &area, const DrawPen &drawPen)
    {
        if (!_renderer)
            return;
        if (const Circle *c = dynamic_cast<const Circle *>(&area))
        {
            float rW = c->r * std::fabs(c->scale.x);
            drawCircleWorld(rW, c->pos, drawPen);
        }
        else if (const Ring *rg = dynamic_cast<const Ring *>(&area))
        {
            float oR = rg->outerR * std::fabs(rg->scale.x);
            float iR = rg->innerR * std::fabs(rg->scale.x);
            drawRingWorld(rg->pos, oR, iR, drawPen);
        }
        else if (const Ellipse *e = dynamic_cast<const Ellipse *>(&area))
        {
            float rxW = e->rx * std::fabs(e->scale.x);
            float ryW = e->ry * std::fabs(e->scale.y);
            drawEllipseWorld(e->pos, rxW, ryW, drawPen);
        }
        else if (const Sector *s = dynamic_cast<const Sector *>(&area))
        {
            float rW = s->radius * std::fabs(s->scale.x);
            drawSectorWorld(s->pos, rW, s->startAng + s->rotation, s->endAng + s->rotation, drawPen);
        }
        else if (const Rect *rt = dynamic_cast<const Rect *>(&area))
        {
            Point local[4];
            rt->getLocalVertices(local);
            Point world[4];
            for (int i = 0; i < 4; i++)
                world[i] = rt->localToWorld(local[i]);
            drawRectWorld(world, drawPen);
        }
        else if (const Polygon *poly = dynamic_cast<const Polygon *>(&area))
        {
            if (poly->localPoints.size() < 3)
                return;
            std::vector<Point> worldPts;
            for (auto &lp : poly->localPoints)
                worldPts.push_back(poly->localToWorld(lp));
            drawPolygonWorld(worldPts.data(), worldPts.size(), drawPen);
        }
    }
    void Window::draw(const Area &area)
    {
        draw(area, this->drawPen);
    }

    void Window::draw(Point point, const DrawPen &drawPen)
    {
        DrawPen tmp = drawPen;
        tmp.isFilledWithBorder = false;
        tmp.circlePrecision = tmp.pointPrecision;
        float r = tmp.lineWidth;
        drawCircleWorld(r, point, tmp);
    }
    void Window::draw(Point point)
    {
        draw(point, drawPen);
    }

    //==== App静态成员定义 ====
    std::vector<Window *> App::wins;
    Uint64 App::perfFreq = 0;
    Uint64 App::lastPerfCount = 0;
    float App::deltaTime = 0.0f;
    float App::fps = 0.0f;

    int App::init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cout << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return -1;
        }
        return 0;
    }
    void App::addWindow(Window *win)
    {
        if (!win)
            return;
        win->create();
        wins.push_back(win);
    }
    void App::addWindow(Window *wins[], int winNum)
    {
        for (int i = 0; i < winNum; i++)
            addWindow(wins[i]);
    }
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
    void App::quit()
    {
        for (auto w : wins)
            if (w)
                w->destroy();
        wins.clear();
        SDL_Quit();
    }
    float App::getDeltaTime() { return deltaTime; }
    float App::getFPS() { return fps; }

    void App::run()
    {
        bool running = true;
        SDL_Event event{};
        perfFreq = SDL_GetPerformanceFrequency();
        lastPerfCount = SDL_GetPerformanceCounter();
        const float maxFrameTime = 1.f / 60.f;
        const float maxDeltaTime = 0.1f;
        while (running)
        {
            Uint64 now = SDL_GetPerformanceCounter();
            deltaTime = (float)(now - lastPerfCount) / (float)perfFreq;
            lastPerfCount = now;
            if (deltaTime > maxDeltaTime)
                deltaTime = maxDeltaTime;
            if (deltaTime > 1e-6f)
                fps = 1.f / deltaTime;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                    break;
                }
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                {
                    auto win = findWindowByID(event.window.windowID);
                    if (win)
                        win->destroy();
                }
                if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
                {
                    auto win = findWindowByID(event.window.windowID);
                    if (win && win->eventCallback)
                        win->eventCallback(&event);
                }
                else
                {
                    for (auto w : wins)
                        if (w && w->_window && w->eventCallback)
                            w->eventCallback(&event);
                }
            }
            std::vector<Window *> alive;
            for (auto w : wins)
                if (w && w->_window)
                    alive.push_back(w);
            wins.swap(alive);
            if (wins.empty())
            {
                running = false;
                break;
            }

            for (auto w : wins)
            {
                if (!w || !w->_renderer)
                    continue;
                w->eraseAll(w->drawPen);
                if (w->updateCallback)
                    w->updateCallback(deltaTime);
                SDL_RenderPresent(w->_renderer);
            }
            float elapsed = (float)(SDL_GetPerformanceCounter() - lastPerfCount) / (float)perfFreq;
            if (elapsed < maxFrameTime)
            {
                Uint32 ms = (Uint32)((maxFrameTime - elapsed) * 1000.f);
                if (ms > 0)
                    SDL_Delay(ms);
            }
        }
        quit();
    }
}
