#include "Xiao2D.h"
namespace Xiao2D
{
    //==== Color静态常量定义 ====
    const Color Color::Black = {0.0f, 0.0f, 0.0f, 1.0f};
    const Color Color::White = {1.0f, 1.0f, 1.0f, 1.0f};
    const Color Color::MidGray = {0.5f, 0.5f, 0.5f, 1.0f};
    const Color Color::DarkGray = {0.25f, 0.25f, 0.25f, 1.0f};
    const Color Color::LiteGray = {0.75f, 0.75f, 0.75f, 1.0f};
    const Color Color::Red = {1.0f, 0.0f, 0.0f, 1.0f};
    const Color Color::Green = {0.0f, 1.0f, 0.0f, 1.0f};
    const Color Color::Blue = {0.0f, 0.0f, 1.0f, 1.0f};
    const Color Color::Yellow = {1.0f, 1.0f, 0.0f, 1.0f};
    const Color Color::PinkRed = {1.0f, 0.0f, 1.0f, 1.0f};
    const Color Color::SeaBlue = {0.0f, 1.0f, 1.0f, 1.0f};
    const Color Color::Orange = {1.0f, 0.647f, 0.0f, 1.0f};
    const Color Color::LiteRed = {1.0f, 0.412f, 0.706f, 1.0f};
    const Color Color::Vio = {0.5f, 0.0f, 0.5f, 1.0f};
    const Color Color::Clear = {0.0f, 0.0f, 0.0f, 0.0f};
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

    // ========== 各个Area子类 drawOnWindow 实现 ==========
    void Circle::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || r <= 0)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v(N + 1);
            Point worldCenter = localToWorld({0, 0});
            v[0].position.x = worldCenter.x;
            v[0].position.y = worldCenter.y;
            v[0].color = tmp.fillColor.toSDL_FColor();
            for (int i = 1; i <= N; i++)
            {
                float a = 2.f * std::numbers::pi * i / N;
                Point localPt{r * std::cos(a), r * std::sin(a)};
                Point worldPt = localToWorld(localPt);
                v[i].position.x = worldPt.x;
                v[i].position.y = worldPt.y;
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
            SDL_RenderGeometry(renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());

            DrawPen ringPen = pen;
            ringPen.isFilledWithBorder = false;
            ringPen.fillColor = pen.lineColor;
            float innerRLocal = std::max(0.f, r - pen.lineWidth / std::fabs(scale.x));

            std::vector<SDL_Vertex> rv;
            std::vector<int> ridx;
            for (int i = 0; i <= N; i++)
            {
                float ang = 2.f * std::numbers::pi * i / N;
                Point outLocal{r * std::cos(ang), r * std::sin(ang)};
                Point inLocal{innerRLocal * std::cos(ang), innerRLocal * std::sin(ang)};
                SDL_Vertex vOut{};
                vOut.position = {localToWorld(outLocal).x, localToWorld(outLocal).y};
                vOut.color = ringPen.fillColor.toSDL_FColor();
                rv.push_back(vOut);

                SDL_Vertex vIn{};
                vIn.position = {localToWorld(inLocal).x, localToWorld(inLocal).y};
                vIn.color = ringPen.fillColor.toSDL_FColor();
                rv.push_back(vIn);
            }
            for (int i = 0; i < N; i++)
            {
                int o0 = i * 2;
                int i0 = i * 2 + 1;
                int o1 = (i + 1) * 2;
                int i1 = (i + 1) * 2 + 1;
                ridx.push_back(o0);
                ridx.push_back(i0);
                ridx.push_back(o1);
                ridx.push_back(o1);
                ridx.push_back(i0);
                ridx.push_back(i1);
            }
            SDL_RenderGeometry(renderer, nullptr, rv.data(), (int)rv.size(), ridx.data(), (int)ridx.size());
        }
    }

    void Ring::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        int seg = pen.circlePrecision;
        if (seg < 3 || outerR <= 0 || innerR < 0 || innerR >= outerR)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v;
            std::vector<int> idx;
            for (int i = 0; i <= seg; i++)
            {
                float ang = 2.f * std::numbers::pi * i / seg;
                Point outLocal{outerR * std::cos(ang), outerR * std::sin(ang)};
                Point inLocal{innerR * std::cos(ang), innerR * std::sin(ang)};
                SDL_Vertex vOut{};
                Point wOut = localToWorld(outLocal);
                vOut.position = {wOut.x, wOut.y};
                vOut.color = tmp.fillColor.toSDL_FColor();
                v.push_back(vOut);

                SDL_Vertex vIn{};
                Point wIn = localToWorld(inLocal);
                vIn.position = {wIn.x, wIn.y};
                vIn.color = tmp.fillColor.toSDL_FColor();
                v.push_back(vIn);
            }
            for (int i = 0; i < seg; i++)
            {
                int o0 = i * 2;
                int i0 = i * 2 + 1;
                int o1 = (i + 1) * 2;
                int i1 = (i + 1) * 2 + 1;
                idx.push_back(o0);
                idx.push_back(i0);
                idx.push_back(o1);
                idx.push_back(o1);
                idx.push_back(i0);
                idx.push_back(i1);
            }
            SDL_RenderGeometry(renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());

            DrawPen strokePen = pen;
            strokePen.isFilledWithBorder = false;
            strokePen.fillColor = pen.lineColor;
            float oInLocal = std::max(0.f, outerR - pen.lineWidth / std::fabs(scale.x));
            std::vector<SDL_Vertex> vo;
            std::vector<int> idxo;
            for (int i = 0; i <= seg; i++)
            {
                float ang = 2.f * std::numbers::pi * i / seg;
                Point oL{float(outerR * cos(ang)), float(outerR * sin(ang))};
                Point iL{float(oInLocal * cos(ang)), float(oInLocal * sin(ang))};
                vo.push_back({{localToWorld(oL).x, localToWorld(oL).y}, strokePen.fillColor.toSDL_FColor(), {0, 0}});
                vo.push_back({{localToWorld(iL).x, localToWorld(iL).y}, strokePen.fillColor.toSDL_FColor(), {0, 0}});
            }
            for (int i = 0; i < seg; i++)
            {
                int o0 = i * 2, i0 = i * 2 + 1, o1 = (i + 1) * 2, i1 = (i + 1) * 2 + 1;
                idxo.push_back(o0);
                idxo.push_back(i0);
                idxo.push_back(o1);
                idxo.push_back(o1);
                idxo.push_back(i0);
                idxo.push_back(i1);
            }
            SDL_RenderGeometry(renderer, nullptr, vo.data(), (int)vo.size(), idxo.data(), (int)idxo.size());

            float iInLocal = std::max(0.f, innerR - pen.lineWidth / std::fabs(scale.x));
            std::vector<SDL_Vertex> vi;
            std::vector<int> idxi;
            for (int i = 0; i <= seg; i++)
            {
                float ang = 2.f * std::numbers::pi * i / seg;
                Point oL{float(innerR * cos(ang)), float(innerR * sin(ang))};
                Point iL{float(iInLocal * cos(ang)), float(iInLocal * sin(ang))};
                vi.push_back({{localToWorld(oL).x, localToWorld(oL).y}, strokePen.fillColor.toSDL_FColor(), {0, 0}});
                vi.push_back({{localToWorld(iL).x, localToWorld(iL).y}, strokePen.fillColor.toSDL_FColor(), {0, 0}});
            }
            for (int i = 0; i < seg; i++)
            {
                int o0 = i * 2, i0 = i * 2 + 1, o1 = (i + 1) * 2, i1 = (i + 1) * 2 + 1;
                idxi.push_back(o0);
                idxi.push_back(i0);
                idxi.push_back(o1);
                idxi.push_back(o1);
                idxi.push_back(i0);
                idxi.push_back(i1);
            }
            SDL_RenderGeometry(renderer, nullptr, vi.data(), (int)vi.size(), idxi.data(), (int)idxi.size());
        }
    }

    void Ellipse::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || rx <= 0 || ry <= 0)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> v(N + 1);
            Point worldCenter = localToWorld({0, 0});
            v[0].position = {worldCenter.x, worldCenter.y};
            v[0].color = tmp.fillColor.toSDL_FColor();
            for (int i = 1; i <= N; i++)
            {
                float ang = 2.f * std::numbers::pi * i / N;
                float c = std::cos(ang), s = std::sin(ang);
                Point localPt{rx * c, ry * s};
                Point worldPt = localToWorld(localPt);
                v[i].position.x = worldPt.x;
                v[i].position.y = worldPt.y;
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
            SDL_RenderGeometry(renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());

            DrawPen ringPen = pen;
            ringPen.isFilledWithBorder = false;
            ringPen.fillColor = pen.lineColor;
            float lw = pen.lineWidth;
            float rxInLocal = std::max(0.f, rx - lw / std::fabs(scale.x));
            float ryInLocal = std::max(0.f, ry - lw / std::fabs(scale.y));
            std::vector<SDL_Vertex> verts;
            std::vector<int> strokeIdx;
            for (int i = 0; i <= N; i++)
            {
                float ang = 2.f * std::numbers::pi * i / N;
                float c = std::cos(ang), s = std::sin(ang);
                Point voLocal{rx * c, ry * s};
                Point viLocal{rxInLocal * c, ryInLocal * s};
                verts.push_back({{localToWorld(voLocal).x, localToWorld(voLocal).y}, ringPen.fillColor.toSDL_FColor(), {0, 0}});
                verts.push_back({{localToWorld(viLocal).x, localToWorld(viLocal).y}, ringPen.fillColor.toSDL_FColor(), {0, 0}});
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
            SDL_RenderGeometry(renderer, nullptr, verts.data(), (int)verts.size(), strokeIdx.data(), (int)strokeIdx.size());
        }
    }

    void Sector::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        int N = pen.circlePrecision;
        if (N < 3 || radius <= 0)
            return;
        float span = endAng - startAng;
        if (std::fabs(span) < 1e-6f)
            return;
        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<SDL_Vertex> verts;
            Point worldCenter = localToWorld({0, 0});
            verts.push_back({{worldCenter.x, worldCenter.y}, tmp.fillColor.toSDL_FColor(), {0, 0}});
            int steps = std::max(3, (int)(std::fabs(span) / (2.f * std::numbers::pi) * N));
            for (int i = 0; i <= steps; i++)
            {
                float a = startAng + span * (float)i / steps;
                Point localPt{radius * std::cos(a), radius * std::sin(a)};
                Point worldPt = localToWorld(localPt);
                SDL_Vertex v{};
                v.position.x = worldPt.x;
                v.position.y = worldPt.y;
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
            SDL_RenderGeometry(renderer, nullptr, verts.data(), (int)verts.size(), idx.data(), (int)idx.size());
        }
    }

    void Rect::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        Point local[4];
        getLocalVertices(local);
        Point worldVerts[4];
        for (int i = 0; i < 4; i++)
        {
            worldVerts[i] = localToWorld(local[i]);
        }
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
            win->submitGeometry(v);
        }
        DrawPen borderPen = pen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = pen.lineColor;
        float scaleAbsX = std::fabs(scale.x);
        for (int i = 0; i < 4; i++)
        {
            int j = (i + 1) % 4;
            Point p1 = worldVerts[i], p2 = worldVerts[j];
            float lw = borderPen.lineWidth;
            if (lw <= 0 || scaleAbsX < 1e-6f)
                continue;
            std::vector<SDL_Vertex> segVerts;
            auto addV = [&](Point pt)
            {
                SDL_Vertex sv{};
                sv.position = {pt.x, pt.y};
                sv.color = borderPen.fillColor.toSDL_FColor();
                segVerts.push_back(sv);
            };
            Point dir = p2 - p1;
            float len = dir.length();
            if (len < 1e-6f)
                continue;
            Point n = dir.perpendicularCCW().normalize();
            Point off = n * (lw * 0.5f / scaleAbsX);
            Point A = p1 + off, B = p1 - off, C = p2 - off, D = p2 + off;
            addV(A);
            addV(B);
            addV(D);
            addV(B);
            addV(C);
            addV(D);
            win->submitGeometry(segVerts);
        }
    }

    void Polygon::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        SDL_Renderer *renderer = win->_renderer;
        if (!renderer)
            return;
        if (localPoints.size() < 3)
            return;
        size_t count = localPoints.size();
        std::vector<Point> worldPts;
        for (auto &lp : localPoints)
        {
            worldPts.push_back(localToWorld(lp));
        }

        if (pen.isFilledWithBorder)
        {
            DrawPen tmp = pen;
            tmp.isFilledWithBorder = false;
            std::vector<size_t> tris;
            bool ok = earClipTriangulate(localPoints.data(), count, tris);
            if (ok)
            {
                std::vector<SDL_Vertex> v;
                auto add = [&](Point localP)
                {
                    Point w = localToWorld(localP);
                    SDL_Vertex sv{};
                    sv.position = {w.x, w.y};
                    sv.color = tmp.fillColor.toSDL_FColor();
                    v.push_back(sv);
                };
                for (size_t t = 0; t < tris.size(); t += 3)
                {
                    add(localPoints[tris[t]]);
                    add(localPoints[tris[t + 1]]);
                    add(localPoints[tris[t + 2]]);
                }
                win->submitGeometry(v);
            }
        }
        DrawPen borderPen = pen;
        borderPen.isFilledWithBorder = false;
        borderPen.fillColor = pen.lineColor;
        float scaleAbsX = std::fabs(scale.x);
        for (size_t i = 0; i < count; i++)
        {
            size_t j = (i + 1) % count;
            Point p1 = worldPts[i], p2 = worldPts[j];
            float lw = borderPen.lineWidth;
            if (lw <= 0 || scaleAbsX < 1e-6f)
                continue;
            std::vector<SDL_Vertex> segVerts;
            auto addV = [&](Point pt)
            {
                SDL_Vertex sv{};
                sv.position = {pt.x, pt.y};
                sv.color = borderPen.fillColor.toSDL_FColor();
                segVerts.push_back(sv);
            };
            Point dir = p2 - p1;
            float len = dir.length();
            if (len < 1e-6f)
                continue;
            Point n = dir.perpendicularCCW().normalize();
            Point off = n * (lw * 0.5f / scaleAbsX);
            Point A = p1 + off, B = p1 - off, C = p2 - off, D = p2 + off;
            addV(A);
            addV(B);
            addV(D);
            addV(B);
            addV(C);
            addV(D);
            win->submitGeometry(segVerts);
        }
    }

    bool Text::contains(const Point &p) const
    {
        // TODO
        return false;
    }
    void Xiao2D::Text::drawOnWindow(const Window *win, const DrawPen &pen) const
    {
        TTF_Font *font = pen.getFont(pen.fontPath);
        if (!font)
        {
            std::cerr << "[Xiao2D::Text::drawOnWindow]无法找到字体: " << pen.fontPath << std::endl;
            std::cerr << "[Xiao2D::Text::drawOnWindow]尝试加载字体: " << pen.fontPath << std::endl;
            if (!pen.loadFont(pen.fontPath, pen.fontSize))
            {
                std::cerr << "[Xiao2D::Text::drawOnWindow]无法加载字体: " << pen.fontPath << std::endl;
                return;
            }
            else
            {
                font = pen.getFont(pen.fontPath);
                if (font)
                {
                    std::cerr << "[Xiao2D::Text::drawOnWindow]字体加载成功: " << pen.fontPath << std::endl;
                }
                else
                {
                    std::cerr << "[Xiao2D::Text::drawOnWindow]字体加载失败: " << pen.fontPath << std::endl;
                    return;
                }
            }
        }
        TTF_Text *textObj = TTF_CreateText(win->_textEngine, font, this->content.c_str(), SDL_strlen(this->content.c_str()));
        if (!textObj)
        {
            std::cerr << "[Xiao2D::Text::drawOnWindow]无法创建文本对象: " << SDL_GetError() << std::endl;
            return;
        }
        TTF_SetTextColorFloat(textObj, pen.lineColor.r, pen.lineColor.g, pen.lineColor.b, pen.lineColor.a);
        TTF_UpdateText(textObj);
        bool drawOk = TTF_DrawRendererText(textObj, this->pos.x, this->pos.y);
        if (!drawOk)
        {
            std::cerr << "[Xiao2D::Text::drawOnWindow]TTF_DrawRendererText失败！err=" << SDL_GetError() << std::endl;
        }
        TTF_DestroyText(textObj);
    }
    std::vector<std::pair<std::string, TTF_Font *>> DrawPen::fontsLoadList;
    bool DrawPen::loadFont(const std::string &fontPath, int fontSize)
    {
        TTF_Font *font = getFont(fontPath);
        if (font)
        {
            if (TTF_SetFontSize(font, fontSize))
                return true;
            else
                std::cerr << "无法设置字体大小: " << fontSize << "，错误: " << SDL_GetError() << std::endl;
        }
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (!font)
        {
            std::cerr << "无法加载字体: " << fontPath << "，错误: " << SDL_GetError() << std::endl;
            return false;
        }
        fontsLoadList.emplace_back(fontPath, font);
        return true;
    }
    TTF_Font *DrawPen::getFont(const std::string &fontPath)
    {
        for (const auto &pair : fontsLoadList)
        {
            if (pair.first == fontPath)
            {
                return pair.second;
            }
        }
        return nullptr;
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
        this->_textEngine = TTF_CreateRendererTextEngine(_renderer);
        if (!this->_textEngine)
        {
            std::cerr << "无法创建文本渲染引擎: " << SDL_GetError() << std::endl;
        }
    }
    void Window::destroy()
    {
        if (_renderer)
        {
            TTF_DestroyRendererTextEngine(this->_textEngine);
            SDL_DestroyRenderer(_renderer);
            _renderer = nullptr;
        }
        if (_window)
        {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }
    }
    void Window::submitGeometry(const std::vector<SDL_Vertex> &verts) const
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
    void Window::draw(const Area &area, const DrawPen &drawPen)
    {
        if (!_renderer)
            return;
        area.drawOnWindow(this, drawPen);
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
        Circle dot(point, r);
        draw(dot, tmp);
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

    Point App::getMouseGlobalPos()
    {
        Point p;
        SDL_GetGlobalMouseState(&p.x, &p.y);
        return p;
    }

    Point App::getMouseWindowPos(const Window *win)
    {
        if(!win){
            std::cerr<<"[Xiao2D::App::getMouseWindowPos]传入参数 Xiao2D::Window *win 为空"<<std::endl;
            return Point{0,0};
        }
        Point pg = getMouseGlobalPos();
        int x, y;
        SDL_GetWindowPosition(win->_window, &x, &y);
        Point pw{float(x), float(y)};
        return pg - pw;
    }

    int App::init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cout << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return -1;
        }
        if (TTF_Init() == -1)
        {
            std::cout << "SDL_ttf初始化失败: " << SDL_GetError() << std::endl;
            SDL_Quit();
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
        DrawPen::freeAllFonts();
        TTF_Quit();
        SDL_Quit();
    }

    float App::getDeltaTime() { return deltaTime; }
    float App::getFPS() { return fps; }

    bool App::running = false;
    void App::run()
    {
        running = true;
        SDL_Event event{};
        perfFreq = SDL_GetPerformanceFrequency();
        lastPerfCount = SDL_GetPerformanceCounter();
        const float maxFrameTime = 1.f / 60.f;
        const float maxDeltaTime = 0.1f;
        while (running)
        {
            //======== 计算deltaTime和FPS ========
            Uint64 now = SDL_GetPerformanceCounter();
            deltaTime = (float)(now - lastPerfCount) / (float)perfFreq;
            lastPerfCount = now;
            if (deltaTime > maxDeltaTime)
                deltaTime = maxDeltaTime;
            if (deltaTime > 1e-6f)
                fps = 1.f / deltaTime;

            //======= 处理事件 ========
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
    void App::stopRun()
    {
        running = false;
    }
}
