// POLVERE DA SPARO: COMMANDO DEL WEST - run and gun in pixel art (384x216 ingrandito 5 volte).
// Gioco originale di Mario Petramale - Elvantis. Corri e spara nel Far West a vapore: dinamite,
// armi raccolte dagli ostaggi liberati e dalle casse, il carro armato a vapore "Bisonte" da guidare,
// banditi-robot, cavalieri meccanici, girocotteri e 10 missioni con un boss ciascuna.
// Tutto il contenuto (missioni, ambienti, nemici, boss, armi, musica, suoni) sta nei dati del progetto.
#include "arcade.h"
#include "flusso.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

using nlohmann::json;

namespace arcade {
namespace {

constexpr int LW = 384, LH = 216;
constexpr float TERRA = 184;          // y dei piedi sul terreno
constexpr float PIg = 3.14159265f;

Color C(unsigned v, int a = 255) { return Hex(v, (unsigned char)a); }
Color Sc(Color c, float k) {
    auto f = [&](unsigned char v) { return (unsigned char)std::max(0.0f, std::min(255.0f, v * k)); };
    return Color{ f(c.r), f(c.g), f(c.b), c.a };
}
int Ar(float v) { return (int)std::floor(v + 0.5f); }

// --- Tela a pixel: le stesse operazioni dell'anteprima approvata (parti ombreggiate e contorno) ---
struct Pix {
    int w, h;
    std::vector<Color> d;
    Pix(int w_, int h_) : w(w_), h(h_), d((size_t)w_ * h_, Color{ 0, 0, 0, 0 }) {}
    bool Op(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h && d[(size_t)y * w + x].a > 100; }
    void P(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= w || y >= h || c.a == 0) return;
        Color& o = d[(size_t)y * w + x];
        if (c.a == 255 || o.a == 0) { o = c; return; }
        float a = c.a / 255.0f;
        o.r = (unsigned char)(o.r + (c.r - o.r) * a); o.g = (unsigned char)(o.g + (c.g - o.g) * a); o.b = (unsigned char)(o.b + (c.b - o.b) * a);
        o.a = std::max(o.a, c.a);
    }
    void R(float x, float y, float ww, float hh, Color c) {
        int x0 = Ar(x), y0 = Ar(y), x1 = x0 + Ar(ww), y1 = y0 + Ar(hh);
        for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) P(xx, yy, c);
    }
    void Disc(float cx, float cy, int r, Color c) {
        for (int y = -r; y <= r; y++) { int xx = (int)std::floor(std::sqrt((float)(r * r - y * y)) + 0.3f); R(cx - xx, cy + y, xx * 2 + 1, 1, c); }
    }
    void Ell(float cx, float cy, int rx, int ry, Color c) {
        if (ry <= 0) { R(cx - rx, cy, rx * 2 + 1, 1, c); return; }
        for (int y = -ry; y <= ry; y++) { int xx = (int)std::floor(rx * std::sqrt(std::max(0.0f, 1 - (float)(y * y) / (ry * ry))) + 0.3f); R(cx - xx, cy + y, xx * 2 + 1, 1, c); }
    }
    void Poly(std::vector<Vector2> p, Color c) {
        if (p.size() < 3) return;
        float y0 = 1e9f, y1 = -1e9f;
        for (auto& q : p) { y0 = std::min(y0, q.y); y1 = std::max(y1, q.y); }
        for (int y = (int)std::floor(y0); y <= (int)std::ceil(y1); y++) {
            float sy = y + 0.5f;
            std::vector<float> xs;
            for (size_t i = 0; i < p.size(); i++) {
                Vector2 a = p[i], b = p[(i + 1) % p.size()];
                if ((a.y <= sy && b.y > sy) || (b.y <= sy && a.y > sy)) xs.push_back(a.x + (sy - a.y) / (b.y - a.y) * (b.x - a.x));
            }
            std::sort(xs.begin(), xs.end());
            for (size_t i = 0; i + 1 < xs.size(); i += 2)
                for (int x = (int)std::ceil(xs[i] - 0.5f); x + 0.5f <= xs[i + 1]; x++) P(x, y, c);
        }
    }
    void Line(float x0, float y0, float x1, float y1, Color c, int wd = 1) {
        int n = (int)std::max(std::fabs(x1 - x0), std::fabs(y1 - y0));
        for (int i = 0; i <= n; i++) { float t = n ? (float)i / n : 0; R(x0 + (x1 - x0) * t - (wd - 1) / 2.0f, y0 + (y1 - y0) * t - (wd - 1) / 2.0f, wd, wd, c); }
    }
    // Una "parte" disegnata a parte e ombreggiata (luce da sinistra in alto), poi incollata.
    void Part(const std::function<void(Pix&)>& fn, float hi = 0.32f, float lo = 0.34f, bool rnd = false) {
        Pix t(w, h);
        fn(t);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                Color c = t.d[(size_t)y * w + x];
                if (c.a <= 100) continue;
                float k = 1;
                if (!t.Op(x, y - 1) || !t.Op(x - 1, y)) k = 1 + hi; else if (!t.Op(x, y - 2) || !t.Op(x - 2, y)) k = 1 + hi * 0.4f;
                if (!t.Op(x, y + 1) || !t.Op(x + 1, y)) k = 1 - lo; else if (!t.Op(x, y + 2) || !t.Op(x + 2, y)) k = 1 - lo * 0.5f;
                else if (rnd && !t.Op(x + 3, y)) k = 1 - lo * 0.25f;
                auto f = [&](unsigned char v) { return (unsigned char)std::min(255.0f, v * k + (k > 1 ? 14 : 0)); };
                P(x, y, Color{ f(c.r), f(c.g), f(c.b), 255 });
            }
    }
    void Contorno(Color col = Color{ 28, 16, 12, 255 }) {
        std::vector<Color> o = d;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                if (Op(x, y)) continue;
                if (Op(x - 1, y) || Op(x + 1, y) || Op(x, y - 1) || Op(x, y + 1)) o[(size_t)y * w + x] = col;
            }
        d = o;
    }
    // grana arcade: colori a gradini con un leggero retino ordinato
    void Grana(float forza = 7) {
        static const int BM[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                Color& c = d[(size_t)y * w + x];
                if (!c.a) continue;
                float t = (BM[y % 4][x % 4] / 16.0f - 0.5f) * forza;
                auto f = [&](unsigned char v) { return (unsigned char)std::max(0.0f, std::min(255.0f, std::round((v + t) / 8) * 8)); };
                c.r = f(c.r); c.g = f(c.g); c.b = f(c.b);
            }
    }
    Texture2D Tex() const {
        Image im{ (void*)d.data(), w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        Texture2D t = LoadTextureFromImage(im);
        SetTextureFilter(t, TEXTURE_FILTER_POINT);
        return t;
    }
};

struct Spr { Texture2D t{}; int ax = 0, ay = 0; };
Spr FaiSpr(int w, int h, int ax, int ay, const std::function<void(Pix&)>& f, bool contorno = true, bool grana = true) {
    Pix p(w, h);
    f(p);
    if (contorno) p.Contorno();
    if (grana) p.Grana(5);
    Spr s; s.t = p.Tex(); s.ax = ax; s.ay = ay;
    return s;
}
void DisegnaSpr(const Spr& s, float x, float y, bool flip = false, Color tinta = WHITE, float rot = 0) {
    if (!s.t.id) return;
    Rectangle src = { 0, 0, (float)(flip ? -s.t.width : s.t.width), (float)s.t.height };
    if (rot == 0) {
        float px = flip ? std::round(x - (s.t.width - s.ax)) : std::round(x - s.ax);
        DrawTextureRec(s.t, src, { px, std::round(y - s.ay) }, tinta);
    } else DrawTexturePro(s.t, src, { std::round(x), std::round(y), (float)s.t.width, (float)s.t.height }, { (float)(flip ? s.t.width - s.ax : s.ax), (float)s.ay }, rot, tinta);
}
void Libera(Spr& s) { if (s.t.id) UnloadTexture(s.t); s.t = Texture2D{}; }

// ============================== DISEGNI ==============================
const Color PELLE = C(0xf2b888), STIVALE = C(0x4a2e1a), ACCIAIO = C(0x80808e);

// L'eroe: gambe 0 fermo, 1-4 corsa, 5 salto; mira 0 avanti, 1 su, 2 giu'; grande = arma pesante.
void Eroe(Pix& c, float x, float y, Color giacca, Color cappello, Color calzoni, int gambe, int mira, bool grande) {
    const float piedi[6][4] = { { -6, 0, 6, 0 }, { -10, -1, 9, 0 }, { -3, -4, 2, 0 }, { 9, 0, -10, -2 }, { 2, 0, -3, -5 }, { -9, -6, 5, -5 } };
    auto Gamba = [&](float hx, float fx, float fy, Color col) {
        c.Part([&](Pix& t) { t.Poly({ { x + hx - 3, y - 15 }, { x + hx + 3, y - 15 }, { x + fx + 2, y + fy - 3 }, { x + fx - 3, y + fy - 1 } }, col); });
        c.Part([&](Pix& t) { t.R(x + fx - 3, y + fy - 3, 8, 4, STIVALE); });
    };
    Gamba(-3, piedi[gambe][0], piedi[gambe][1], Sc(calzoni, 0.9f));
    Gamba(3, piedi[gambe][2], piedi[gambe][3], calzoni);
    float b = (gambe == 2 || gambe == 4) ? -1 : 0;   // il busto sobbalza nella corsa
    y += b;
    c.Part([&](Pix& t) { t.Poly({ { x - 8, y - 30 }, { x + 7, y - 30 }, { x + 8, y - 14 }, { x - 8, y - 14 } }, giacca); });
    c.Part([&](Pix& t) { t.R(x - 8, y - 17, 16, 3, C(0x4a2a14)); }, 0.2f);
    c.R(x - 1, y - 17, 3, 3, C(0xe8c050));
    for (int k = 0; k < 6; k++) c.R(x - 6 + k * 2, y - 29 + k * 2, 2, 2, k % 2 ? C(0xf0d060) : C(0xb08a30));
    c.Part([&](Pix& t) { t.Ell(x + 1, y - 37, 8, 8, PELLE); }, 0.32f, 0.34f, true);
    c.R(x - 6, y - 34, 14, 4, C(0xc8302a)); c.R(x - 6, y - 34, 14, 1, C(0xee5a48));
    c.R(x + 3, y - 40, 2, 3, C(0x2a1a10)); c.R(x + 3, y - 40, 1, 1, WHITE); c.R(x - 1, y - 40, 1, 2, C(0x2a1a10)); c.R(x + 1, y - 43, 4, 1, C(0x6a3a20));
    c.Part([&](Pix& t) { t.R(x - 11, y - 46, 24, 3, cappello); t.Poly({ { x - 6, y - 46 }, { x + 7, y - 46 }, { x + 5, y - 53 }, { x - 4, y - 53 } }, cappello); });
    c.R(x - 6, y - 48, 13, 1, C(0x2a1a10));
    if (mira == 0) {
        c.Part([&](Pix& t) { t.R(x, y - 27, 8, 5, giacca); });
        c.Part([&](Pix& t) { t.Ell(x + 9, y - 23, 3, 3, PELLE); });
        if (grande) {
            c.Part([&](Pix& t) { t.R(x + 3, y - 28, 24, 6, ACCIAIO); t.R(x + 26, y - 27, 6, 4, C(0x5a5a68)); t.R(x + 8, y - 22, 6, 5, C(0x6a4a2a)); t.Ell(x + 12, y - 31, 4, 3, C(0xa07030)); }, 0.4f);
            c.R(x + 12, y - 28, 10, 1, C(0xd0d0e0));
            for (int k = 0; k < 3; k++) c.R(x + 14 + k * 4, y - 26, 1, 3, C(0x3a3a48));
        } else {
            c.Part([&](Pix& t) { t.R(x + 7, y - 28, 11, 4, C(0x6a6a78)); t.R(x + 8, y - 25, 3, 4, C(0x5a3a20)); }, 0.4f);
            c.R(x + 7, y - 28, 11, 1, C(0xb0b0c0));
        }
    } else if (mira == 1) {
        c.Part([&](Pix& t) { t.R(x + 2, y - 35, 5, 9, giacca); });
        c.Part([&](Pix& t) { t.Ell(x + 5, y - 37, 3, 3, PELLE); });
        if (grande) {
            c.Part([&](Pix& t) { t.R(x + 2, y - 60, 6, 24, ACCIAIO); t.R(x + 3, y - 63, 4, 4, C(0x5a5a68)); t.R(x - 3, y - 42, 5, 6, C(0x6a4a2a)); t.Ell(x + 11, y - 46, 3, 4, C(0xa07030)); }, 0.4f);
            c.R(x + 2, y - 58, 1, 10, C(0xd0d0e0));
        } else c.Part([&](Pix& t) { t.R(x + 3, y - 52, 4, 13, C(0x6a6a78)); t.R(x, y - 41, 4, 3, C(0x5a3a20)); }, 0.4f);
    } else {
        c.Part([&](Pix& t) { t.R(x + 1, y - 27, 5, 9, giacca); });
        c.Part([&](Pix& t) { t.Ell(x + 3, y - 17, 3, 3, PELLE); });
        if (grande) c.Part([&](Pix& t) { t.R(x + 1, y - 20, 6, 23, ACCIAIO); t.R(x + 7, y - 16, 5, 5, C(0x6a4a2a)); t.Ell(x - 3, y - 12, 3, 4, C(0xa07030)); }, 0.4f);
        else c.Part([&](Pix& t) { t.R(x + 2, y - 16, 4, 13, C(0x6a6a78)); }, 0.4f);
    }
}
// punto da cui escono i colpi dell'eroe (rispetto ai piedi, guardando a destra)
Vector2 Canna(int mira, bool grande) {
    if (mira == 1) return grande ? Vector2{ 5, -63 } : Vector2{ 5, -53 };
    if (mira == 2) return grande ? Vector2{ 4, 4 } : Vector2{ 4, -3 };
    return grande ? Vector2{ 33, -25 } : Vector2{ 19, -26 };
}

// Il bandito guarda a SINISTRA. posa: 0-3 cammina, 4 spara, 5 dinamite, 6 scappa
void Bandito(Pix& c, float x, float y, Color giacca, Color cappello, int posa) {
    auto L = [&](float px, float ww) { return x - px - ww; };
    float d1 = 0, d2 = 0, s1 = 0, s2 = 0;
    if (posa < 4) { const float o[4] = { 0, 2, 0, -2 }; d1 = o[posa]; d2 = -o[posa]; s1 = posa == 1 ? -1 : 0; s2 = posa == 3 ? -1 : 0; }
    c.Part([&](Pix& t) { t.R(L(-5, 4) - d1, y - 12 + s1, 4, 10, C(0x5c4c3a)); t.R(L(1, 4) - d2, y - 12 + s2, 4, 10, C(0x5c4c3a)); });
    c.Part([&](Pix& t) { t.R(L(-6, 6) - d1, y - 3 + s1, 6, 3, C(0x2a1a10)); t.R(L(1, 6) - d2, y - 3 + s2, 6, 3, C(0x2a1a10)); });
    c.Part([&](Pix& t) { t.Poly({ { L(-8, 0), y - 29 }, { L(8, 0), y - 29 }, { L(10, 0), y - 9 }, { L(-10, 0), y - 9 } }, giacca); });
    c.R(L(0, 1), y - 28, 1, 16, Color{ 0, 0, 0, 77 });
    c.Part([&](Pix& t) { t.Ell(x, y - 35, 7, 7, C(0xe2a878)); }, 0.32f, 0.34f, true);
    c.R(L(-7, 14), y - 35, 14, 5, C(0x3a4c70)); c.R(L(-7, 14), y - 35, 14, 1, C(0x5a6c90)); c.R(L(2, 2), y - 38, 2, 2, C(0x1a1010));
    c.Part([&](Pix& t) { t.R(L(-11, 22), y - 42, 22, 3, cappello); t.R(L(-6, 12), y - 49, 12, 7, cappello); });
    c.R(L(-6, 12), y - 44, 12, 1, C(0x8a3020));
    if (posa == 4) {
        c.Part([&](Pix& t) { t.R(L(2, 16), y - 26, 16, 4, C(0x3e3e4c)); t.Ell(x - 4, y - 23, 3, 3, C(0xe2a878)); });
        c.R(L(4, 12), y - 26, 12, 1, C(0x8a8a98));
    } else if (posa == 5) {
        c.Part([&](Pix& t) { t.R(L(5, 4), y - 46, 4, 18, giacca); });
        c.Part([&](Pix& t) { t.R(L(4, 6), y - 50, 6, 4, C(0xc83028)); });
        c.R(L(6, 1), y - 53, 1, 3, C(0x3a2a1a));
    } else if (posa == 6) {
        c.Part([&](Pix& t) { t.R(L(-12, 4), y - 42, 4, 12, C(0xe2a878)); t.R(L(8, 4), y - 44, 4, 14, C(0xe2a878)); });
    } else {
        c.Part([&](Pix& t) { t.R(L(-2, 5), y - 27 + (posa % 2), 5, 11, giacca); });
        c.Part([&](Pix& t) { t.R(L(4, 8), y - 20, 8, 3, C(0x3e3e4c)); });
    }
}
// Bandito-robot a vapore (guarda a sinistra). posa 0-3 cammina, 4 soffia vapore
void Robot(Pix& c, float x, float y, Color corpo, Color cappello, int posa) {
    const float o[4] = { 0, 2, 0, -2 };
    float d = posa < 4 ? o[posa] : 0;
    c.Part([&](Pix& t) { t.R(x - 6 - d, y - 14, 5, 12, C(0x50505a)); t.R(x + 2 + d, y - 14, 5, 12, C(0x50505a)); });
    c.Part([&](Pix& t) { t.R(x - 9 - d, y - 4, 8, 4, C(0x3a3a44)); t.R(x + 1 + d, y - 4, 8, 4, C(0x3a3a44)); });
    c.Part([&](Pix& t) { t.Ell(x, y - 24, 12, 11, corpo); }, 0.35f, 0.4f, true);
    for (int k = 0; k < 4; k++) c.R(x - 8 + k * 5, y - 28, 1, 1, C(0xe0d8c0));
    c.R(x - 10, y - 20, 20, 1, Sc(corpo, 0.6f));
    c.Part([&](Pix& t) { t.R(x - 7, y - 44, 14, 10, Sc(corpo, 1.1f)); }, 0.35f, 0.4f);
    c.R(x - 6, y - 40, 4, 3, C(0xff3a2a)); c.R(x - 6, y - 40, 2, 1, C(0xffd0a0));
    c.Part([&](Pix& t) { t.R(x - 11, y - 46, 22, 3, cappello); t.R(x - 4, y - 56, 8, 10, cappello); });
    c.R(x - 4, y - 49, 8, 1, C(0xc09040));
    c.Part([&](Pix& t) { t.R(x - 22, y - 28, 12, 6, C(0x6a6a74)); t.Ell(x - 22, y - 25, 3, 4, C(0xa07030)); }, 0.4f);
    if (posa == 4) for (int k = 0; k < 5; k++) c.Disc(x - 26 - k * 3, y - 25 - k, 1 + k / 2, Color{ 240, 240, 240, 180 });
}
// Cavaliere su cavallo meccanico (guarda a sinistra), f = 0/1 galoppo
void Cavaliere(Pix& c, float x, float y, Color giacca, Color cappello, int f) {
    Color met = C(0x7a6a5a), sc = C(0x4a3e34);
    float a = f ? 5 : -5;
    for (int k = 0; k < 2; k++) {
        float lx = k ? x + 12 : x - 12;
        c.Part([&](Pix& t) { t.Line(lx, y - 16, lx + (k ? a : -a), y - 2, sc, 3); t.R(lx + (k ? a : -a) - 2, y - 3, 5, 3, C(0x2a2a2a)); });
    }
    c.Part([&](Pix& t) { t.Ell(x, y - 21, 17, 7, met); }, 0.35f, 0.35f, true);
    for (int k = 0; k < 5; k++) c.R(x - 12 + k * 6, y - 22, 1, 1, C(0xe0d0b0));
    c.Part([&](Pix& t) { t.Poly({ { x - 14, y - 24 }, { x - 20, y - 38 }, { x - 28, y - 36 }, { x - 30, y - 30 }, { x - 22, y - 28 }, { x - 18, y - 20 } }, met); });
    c.R(x - 25, y - 35, 2, 2, C(0xff4020));
    c.Part([&](Pix& t) { t.R(x + 14, y - 30, 4, 10, C(0x3a3a40)); });
    c.Disc(x + 16, y - 33 - f, 2, Color{ 230, 230, 230, 170 });
    Bandito(c, x + 2, y - 18, giacca, cappello, 4);
}
// Girocottero dei banditi (guarda a sinistra), f = elica
void Girocottero(Pix& c, float x, float y, Color corpo, int f) {
    c.Part([&](Pix& t) { t.Ell(x - 4, y, 17, 8, corpo); t.R(x + 12, y - 2, 18, 3, Sc(corpo, 0.85f)); t.R(x + 28, y - 7, 3, 10, Sc(corpo, 0.7f)); }, 0.32f, 0.34f, true);
    c.Part([&](Pix& t) { t.Ell(x - 12, y - 1, 5, 4, C(0xa8d4e4)); }, 0.5f);
    c.R(x - 2, y - 12, 2, 4, C(0x3a3a2a));
    if (f) c.R(x - 28, y - 14, 54, 2, C(0x2a2a22)); else c.R(x - 14, y - 14, 26, 2, C(0x2a2a22));
    c.R(x - 8, y + 8, 1, 5, C(0x3a3a2a)); c.R(x + 6, y + 8, 1, 5, C(0x3a3a2a)); c.R(x - 14, y + 12, 26, 2, C(0x3a3a2a));
    c.R(x - 2, y - 1, 2, 2, C(0xe0c050));
}
// Prigioniero: posa 0/1 legato al palo (si dimena), 2/3 libero che corre, 4 saluta
void Ostaggio(Pix& c, float x, float y, int posa) {
    bool legato = posa < 2;
    if (legato) c.Part([&](Pix& t) { t.R(x - 2, y - 48, 4, 48, C(0x6e4a2a)); });
    float dx = legato ? (posa ? 1 : 0) : 0;
    if (legato || posa == 4) c.Part([&](Pix& t) { t.R(x - 6, y - 12, 5, 10, C(0x6a6a8c)); t.R(x + 1, y - 12, 5, 10, C(0x6a6a8c)); });
    else { float o = posa == 2 ? 3 : -3; c.Part([&](Pix& t) { t.R(x - 6 + o, y - 12, 5, 10, C(0x6a6a8c)); t.R(x + 1 - o, y - 12, 5, 10, C(0x6a6a8c)); }); }
    c.R(x - 7, y - 3, 6, 3, C(0x4a3020)); c.R(x + 1, y - 3, 6, 3, C(0x4a3020));
    c.Part([&](Pix& t) { t.Poly({ { x - 7 + dx, y - 28 }, { x + 7 + dx, y - 28 }, { x + 8, y - 12 }, { x - 8, y - 12 } }, C(0xece0c4)); });
    c.Part([&](Pix& t) { t.Ell(x + dx, y - 34, 6, 6, PELLE); }, 0.32f, 0.34f, true);
    c.Part([&](Pix& t) { t.R(x - 7 + dx, y - 40, 14, 3, C(0xc8a060)); t.R(x - 4 + dx, y - 44, 8, 4, C(0xc8a060)); });
    c.R(x - 3 + dx, y - 35, 2, 2, C(0x2a1a10)); c.R(x + 2 + dx, y - 35, 2, 2, C(0x2a1a10)); c.R(x - 2 + dx, y - 31, 4, 1, C(0x8a3a2a));
    if (legato) { for (float yy : { y - 24.0f, y - 18.0f }) c.R(x - 9, yy, 18, 2, C(0xa87838)); }
    else if (posa == 4) c.Part([&](Pix& t) { t.R(x + 6, y - 46, 3, 14, PELLE); });
    else c.Part([&](Pix& t) { t.R(x - 10, y - 30, 3, 10, PELLE); t.R(x + 7, y - 30, 3, 10, PELLE); });
}
// Il carro armato a vapore "Bisonte" (guarda a destra), f = cingoli, conPilota
void Bisonte(Pix& c, float x, float y, int f, bool conPilota, Color cappello) {
    Color scafo = C(0x7a6a3a), ottone = C(0xc09040);
    c.Part([&](Pix& t) { t.Ell(x, y - 7, 22, 7, C(0x3a342c)); }, 0.2f, 0.3f, true);
    for (int k = 0; k < 5; k++) { c.Disc(x - 16 + k * 8, y - 7, 3, C(0x6a6458)); c.R(x - 17 + k * 8 + (f ? 1 : 0), y - 8, 2, 2, C(0x2a2620)); }
    for (int k = 0; k < 11; k++) c.R(x - 21 + k * 4 + (f ? 2 : 0), y - 1, 2, 1, C(0x1a1814));
    c.Part([&](Pix& t) { t.Poly({ { x - 22, y - 14 }, { x + 20, y - 14 }, { x + 24, y - 20 }, { x + 14, y - 26 }, { x - 18, y - 26 }, { x - 24, y - 20 } }, scafo); });
    for (int k = 0; k < 6; k++) c.R(x - 16 + k * 6, y - 17, 1, 1, C(0xe0d0a0));
    c.R(x - 20, y - 15, 40, 1, Sc(scafo, 0.6f));
    if (conPilota) {
        c.Part([&](Pix& t) { t.Ell(x - 6, y - 32, 4, 4, PELLE); }, 0.3f, 0.3f, true);
        c.Part([&](Pix& t) { t.R(x - 12, y - 36, 13, 2, cappello); t.R(x - 9, y - 40, 7, 4, cappello); });
    }
    c.Part([&](Pix& t) { t.Ell(x + 2, y - 29, 10, 6, ottone); }, 0.4f, 0.35f, true);
    c.Part([&](Pix& t) { t.R(x + 10, y - 32, 20, 5, C(0x5a5a64)); t.R(x + 28, y - 33, 4, 7, C(0x44444e)); }, 0.4f);
    c.Part([&](Pix& t) { t.R(x - 20, y - 40, 5, 14, C(0x3a3a40)); t.R(x - 21, y - 41, 7, 2, C(0x2a2a30)); });
}
void Cassa(Pix& c, float x, float y) {
    c.Part([&](Pix& t) { t.R(x, y, 15, 13, C(0xa0703c)); });
    c.R(x + 1, y + 6, 13, 1, C(0x5a3a1a)); c.Line(x + 1, y + 1, x + 13, y + 11, C(0x6a4424)); c.R(x + 5, y + 2, 5, 2, C(0x3a2a1a));
}
void Barile(Pix& c, float x, float y, bool polvere) {
    c.Part([&](Pix& t) { t.R(x, y, 10, 14, polvere ? C(0xb03028) : C(0x8a5a2a)); t.Ell(x + 5, y, 5, 2, polvere ? C(0x80201a) : C(0x6a4220)); }, 0.32f, 0.34f, true);
    c.R(x, y + 3, 10, 1, C(0x3a3a40)); c.R(x, y + 10, 10, 1, C(0x3a3a40));
    if (polvere) { c.R(x + 3, y + 5, 4, 4, C(0xf0e0b0)); c.R(x + 4, y + 6, 2, 2, C(0x2a1a10)); }
}
// Le icone delle armi raccolte (cassetta con la lettera)
void Icona(Pix& c, float x, float y, Color col) {
    c.Part([&](Pix& t) { t.R(x, y, 14, 12, col); }, 0.4f, 0.4f);
    c.R(x + 2, y + 2, 10, 8, Sc(col, 0.55f));
}

// --- I dieci boss (guardano a sinistra). f = fotogramma 0/1 ---
struct Misura { int w, h; };
const Misura MISURE_BOSS[10] = { { 120, 80 }, { 70, 100 }, { 110, 64 }, { 130, 90 }, { 130, 70 }, { 120, 80 }, { 120, 64 }, { 110, 90 }, { 130, 80 }, { 90, 120 } };
void DisegnaBoss(Pix& c, int aspetto, Color col, int f) {
    float x = c.w / 2.0f, y = (float)c.h - 2;   // in basso al centro (la tela ha un margine intorno)
    Color scuro = Sc(col, 0.55f), chiaro = Sc(col, 1.3f), rosso = C(0xc03028), ottone = C(0xc09040), ferro = C(0x4a4a52);
    auto Ruota = [&](float cx, float cy, int r) {
        c.Part([&](Pix& t) { t.Disc(cx, cy, r, C(0x2e2e2c)); }, 0.3f, 0.3f, true);
        c.Disc(cx, cy, std::max(1, r / 2), C(0x7a7a70));
        float a = f * 0.8f;
        c.R(cx + std::cos(a) * r * 0.6f - 1, cy + std::sin(a) * r * 0.6f - 1, 2, 2, rosso);
    };
    auto Fumaiolo = [&](float fx, float fy, int w, int h) {
        c.Part([&](Pix& t) { t.R(fx - w / 2.0f, fy - h, w, h, ferro); t.R(fx - w / 2.0f - 2, fy - h - 3, w + 4, 4, Sc(ferro, 0.7f)); });
    };
    switch (aspetto) {
        case 0: {   // Treno Corazzato
            c.Part([&](Pix& t) { t.R(x - 50, y - 50, 100, 38, col); t.R(x - 30, y - 64, 40, 14, scuro); }, 0.25f, 0.3f);
            for (int k = 0; k < 10; k++) c.R(x - 46 + k * 10, y - 48, 1, 34, Sc(col, 0.7f));
            for (int k = 0; k < 9; k++) { c.R(x - 45 + k * 11, y - 49, 1, 1, C(0xc0b8a0)); c.R(x - 45 + k * 11, y - 15, 1, 1, C(0xc0b8a0)); }
            c.R(x - 30, y - 36, 50, 5, rosso); c.R(x - 29, y - 36, 48, 1, C(0xe86048));
            c.Part([&](Pix& t) { t.Poly({ { x - 50, y - 14 }, { x - 62, y - 2 }, { x - 50, y - 2 } }, ferro); });
            c.Part([&](Pix& t) { t.R(x - 70, y - 60, 42, 6, ferro); }, 0.3f);
            Fumaiolo(x + 34, y - 50, 9, 18);
            for (int k = 0; k < 5; k++) Ruota(x - 40 + k * 20, y - 8, 7);
            c.R(x - 20, y - 58, 6, 5, C(0xffd040));
            break;
        }
        case 1: {   // Sceriffo d'Acciaio
            float g = f ? 3 : -3;
            c.Part([&](Pix& t) { t.R(x - 18 + g, y - 34, 12, 32, ferro); t.R(x + 6 - g, y - 34, 12, 32, ferro); });
            c.Part([&](Pix& t) { t.R(x - 22 + g, y - 6, 18, 6, C(0x2a2a30)); t.R(x + 4 - g, y - 6, 18, 6, C(0x2a2a30)); });
            c.Part([&](Pix& t) { t.Ell(x, y - 54, 24, 22, col); }, 0.35f, 0.4f, true);
            c.Part([&](Pix& t) { t.Poly({ { x - 4, y - 66 }, { x + 1, y - 58 }, { x + 9, y - 58 }, { x + 3, y - 52 }, { x + 6, y - 44 }, { x - 2, y - 49 }, { x - 9, y - 44 }, { x - 7, y - 52 }, { x - 13, y - 58 }, { x - 6, y - 58 } }, ottone); }, 0.45f);
            c.Part([&](Pix& t) { t.R(x - 12, y - 84, 22, 14, chiaro); }, 0.35f, 0.4f);
            c.R(x - 10, y - 80, 6, 4, C(0xff3a2a)); c.R(x + 1, y - 80, 6, 4, C(0xff3a2a));
            c.Part([&](Pix& t) { t.R(x - 30, y - 88, 58, 5, C(0x3a2a20)); t.R(x - 14, y - 99, 26, 12, C(0x3a2a20)); });
            c.R(x - 14, y - 90, 26, 2, rosso);
            c.Part([&](Pix& t) { t.R(x - 34, y - 62, 10, 22, scuro); t.R(x - 34, y - 46, 6, 6, ferro); }, 0.35f);
            for (int k = 0; k < 3; k++) c.Part([&](Pix& t) { t.R(x - 38 + k * 2, y - 40 + k * 3, 10, 2, C(0x5a5a64)); });
            c.Part([&](Pix& t) { t.R(x + 22, y - 62, 10, 22, scuro); });
            break;
        }
        case 2: {   // Talpa Trivella
            c.Part([&](Pix& t) { t.Ell(x + 10, y - 10, 38, 8, C(0x3a342c)); }, 0.2f, 0.3f, true);
            for (int k = 0; k < 7; k++) { c.Disc(x - 22 + k * 11, y - 10, 4, C(0x6a6458)); }
            c.Part([&](Pix& t) { t.R(x - 20, y - 46, 66, 30, col); }, 0.3f, 0.35f);
            for (int k = 0; k < 6; k++) c.R(x - 16 + k * 11, y - 44, 1, 26, Sc(col, 0.7f));
            c.Part([&](Pix& t) { t.Poly({ { x - 20, y - 50 }, { x - 20, y - 12 }, { x - 54, y - 31 } }, ottone); }, 0.45f, 0.4f);
            for (int k = 0; k < 5; k++) c.Line(x - 22 - k * 6, y - 48 + k * 4 + (f ? 2 : 0), x - 22 - k * 6, y - 14 - k * 4 - (f ? 2 : 0), Sc(ottone, 0.6f));
            c.Part([&](Pix& t) { t.R(x + 8, y - 60, 26, 14, scuro); }, 0.3f);
            c.R(x + 12, y - 56, 8, 6, C(0x9ad0e0));
            Fumaiolo(x + 38, y - 46, 6, 12);
            break;
        }
        case 3: {   // Battello a Ruota da Guerra
            c.Part([&](Pix& t) { t.Poly({ { x - 64, y - 30 }, { x + 60, y - 30 }, { x + 54, y - 6 }, { x - 52, y - 6 } }, col); }, 0.3f, 0.35f);
            c.R(x - 60, y - 22, 112, 2, rosso);
            c.Part([&](Pix& t) { t.R(x - 40, y - 52, 70, 22, C(0xe0d0b0)); t.R(x - 34, y - 64, 50, 12, C(0xd0c0a0)); }, 0.25f, 0.3f);
            for (int k = 0; k < 6; k++) c.R(x - 36 + k * 11, y - 46, 6, 8, C(0x4a3a3a));
            Fumaiolo(x - 18, y - 64, 7, 22); Fumaiolo(x + 2, y - 64, 7, 22);
            c.Part([&](Pix& t) { t.Disc(x + 46, y - 30, 22, C(0x8a5a2a)); }, 0.3f, 0.35f, true);
            for (int k = 0; k < 8; k++) { float a = k * PIg / 4 + f * 0.4f; c.Line(x + 46, y - 30, x + 46 + std::cos(a) * 21, y - 30 + std::sin(a) * 21, C(0x4a3018), 2); }
            c.Disc(x + 46, y - 30, 4, ferro);
            c.Part([&](Pix& t) { t.R(x - 62, y - 40, 22, 5, ferro); t.R(x - 44, y - 38, 6, 8, scuro); }, 0.3f);
            break;
        }
        case 4: {   // Dirigibile Bandito
            c.Part([&](Pix& t) { t.Ell(x, y - 48, 62, 20, col); }, 0.3f, 0.35f, true);
            for (int k = 0; k < 7; k++) c.R(x - 48 + k * 16, y - 66, 1, 36, Sc(col, 0.75f));
            c.Part([&](Pix& t) { t.Poly({ { x + 52, y - 52 }, { x + 66, y - 66 }, { x + 66, y - 30 }, { x + 52, y - 44 } }, scuro); });
            c.Part([&](Pix& t) { t.R(x - 26, y - 24, 52, 14, C(0x6a4a2a)); }, 0.3f);
            for (int k = 0; k < 5; k++) c.R(x - 22 + k * 10, y - 20, 5, 4, C(0xffd070));
            c.R(x - 20, y - 29, 1, 6, C(0x2a1a10)); c.R(x + 20, y - 29, 1, 6, C(0x2a1a10));
            c.Part([&](Pix& t) { t.R(x + 26, y - 20, 6, 6, ferro); });
            if (f) c.R(x + 32, y - 26, 2, 18, C(0x2a2a22)); else c.R(x + 30, y - 18, 8, 2, C(0x2a2a22));
            c.R(x - 8, y - 10, 16, 4, C(0x3a3a40));
            c.R(x - 40, y - 56, 34, 7, C(0xf0e0c0)); c.R(x - 37, y - 54, 28, 3, rosso);
            break;
        }
        case 5: {   // Cannone Gigante
            for (int k = 0; k < 2; k++) Ruota(x - 14 + k * 34, y - 14, 14);
            c.Part([&](Pix& t) { t.Poly({ { x - 30, y - 22 }, { x + 40, y - 22 }, { x + 52, y - 6 }, { x - 30, y - 6 } }, C(0x6a4a2a)); });
            c.Part([&](Pix& t) { t.Poly({ { x + 10, y - 50 }, { x + 30, y - 36 }, { x - 52, y - 66 }, { x - 58, y - 54 } }, col); }, 0.4f, 0.4f);
            c.Part([&](Pix& t) { t.Ell(x - 56, y - 60, 5, 8, scuro); });
            c.Disc(x - 57, y - 60, 3, C(0x101010));
            for (int k = 0; k < 4; k++) c.R(x - 40 + k * 16, y - 60 + k * 6, 3, 9, ottone);
            c.Part([&](Pix& t) { t.R(x + 22, y - 56, 30, 34, C(0x8a6a4a)); t.Poly({ { x + 18, y - 56 }, { x + 37, y - 70 }, { x + 56, y - 56 } }, C(0x6a4430)); });
            c.R(x + 30, y - 46, 10, 8, C(0x2a1a14));
            break;
        }
        case 6: {   // Scorpione Meccanico
            for (int k = 0; k < 4; k++) {
                float lx = x - 30 + k * 18, a = ((k + f) % 2) ? 3 : -3;
                c.Part([&](Pix& t) { t.Line(lx, y - 20, lx - 6 + a, y - 2, ferro, 3); });
            }
            c.Part([&](Pix& t) { t.Ell(x, y - 24, 36, 10, col); }, 0.35f, 0.4f, true);
            for (int k = 0; k < 5; k++) c.R(x - 28 + k * 12, y - 30, 1, 14, scuro);
            for (int k = 0; k < 5; k++) c.Part([&](Pix& t) { t.Disc(x + 34 + k * 4, y - 30 - k * 8, 6 - k / 2, k % 2 ? col : chiaro); }, 0.35f, 0.35f, true);
            c.Part([&](Pix& t) { t.Poly({ { x + 48, y - 64 }, { x + 30, y - 58 }, { x + 34, y - 70 } }, ottone); }, 0.45f);
            c.Part([&](Pix& t) { t.Poly({ { x - 34, y - 26 }, { x - 52, y - 34 + (f ? 2 : 0) }, { x - 58, y - 26 }, { x - 50, y - 20 } }, chiaro); });
            c.Part([&](Pix& t) { t.Poly({ { x - 34, y - 18 }, { x - 52, y - 14 }, { x - 58, y - 6 + (f ? -2 : 0) }, { x - 46, y - 8 } }, chiaro); });
            c.R(x - 30, y - 30, 3, 3, C(0xff3020)); c.R(x - 24, y - 31, 3, 3, C(0xff3020));
            break;
        }
        case 7: {   // Mammut a Vapore
            float g = f ? 3 : -3;
            for (int k = 0; k < 4; k++) { float lx = x - 30 + k * 18 + (k % 2 ? g : -g); c.Part([&](Pix& t) { t.R(lx - 6, y - 28, 12, 28, Sc(col, 0.8f)); t.R(lx - 7, y - 4, 14, 4, C(0x2a2a30)); }); }
            c.Part([&](Pix& t) { t.Ell(x + 4, y - 46, 42, 24, col); }, 0.35f, 0.4f, true);
            for (int k = 0; k < 6; k++) c.R(x - 28 + k * 12, y - 66, 1, 40, scuro);
            c.Part([&](Pix& t) { t.Ell(x - 36, y - 54, 16, 16, chiaro); }, 0.35f, 0.4f, true);
            c.Part([&](Pix& t) { t.R(x - 54, y - 50, 8, 34, ferro); t.R(x - 56, y - 18, 10, 4, C(0x3a3a40)); });
            c.Part([&](Pix& t) { t.Poly({ { x - 44, y - 42 }, { x - 64, y - 36 }, { x - 68, y - 44 }, { x - 50, y - 46 } }, C(0xf0e8d0)); }, 0.4f);
            c.R(x - 42, y - 60, 4, 4, C(0xff3a20));
            Fumaiolo(x + 18, y - 68, 8, 16);
            c.Part([&](Pix& t) { t.R(x - 6, y - 80, 22, 12, C(0x6a4a2a)); }, 0.3f);
            break;
        }
        case 8: {   // Locomotiva Drago
            c.Part([&](Pix& t) { t.R(x - 40, y - 52, 96, 38, col); }, 0.3f, 0.35f);
            for (int k = 0; k < 8; k++) c.Part([&](Pix& t) { t.Poly({ { x - 34 + k * 12, y - 52 }, { x - 28 + k * 12, y - 62 }, { x - 22 + k * 12, y - 52 } }, ottone); }, 0.4f);
            c.Part([&](Pix& t) { t.Poly({ { x - 40, y - 50 }, { x - 64, y - 46 }, { x - 66, y - 36 }, { x - 44, y - 34 } }, chiaro); }, 0.4f);
            c.Part([&](Pix& t) { t.Poly({ { x - 40, y - 32 }, { x - 62, y - 30 + (f ? 4 : 0) }, { x - 58, y - 22 }, { x - 40, y - 22 } }, chiaro); }, 0.4f);
            for (int k = 0; k < 4; k++) c.R(x - 62 + k * 5, y - 34, 2, 3, C(0xffffff));
            c.R(x - 50, y - 46, 4, 4, C(0xffd020));
            c.R(x - 36, y - 30, 88, 3, rosso);
            Fumaiolo(x + 40, y - 52, 9, 16);
            for (int k = 0; k < 5; k++) Ruota(x - 32 + k * 20, y - 9, 8);
            break;
        }
        default: {  // Robot Gigante del Barone Ruggine
            float g = f ? 3 : -3;
            c.Part([&](Pix& t) { t.R(x - 20 + g, y - 44, 15, 42, ferro); t.R(x + 5 - g, y - 44, 15, 42, ferro); });
            c.Part([&](Pix& t) { t.R(x - 24 + g, y - 7, 22, 7, C(0x2a2a30)); t.R(x + 2 - g, y - 7, 22, 7, C(0x2a2a30)); });
            c.Part([&](Pix& t) { t.R(x - 28, y - 86, 56, 44, col); }, 0.35f, 0.4f);
            for (int k = 0; k < 5; k++) c.R(x - 24 + k * 12, y - 84, 1, 40, scuro);
            c.R(x - 20, y - 60, 40, 4, rosso);
            c.Part([&](Pix& t) { t.Ell(x, y - 98, 16, 12, C(0x9ad0e0)); }, 0.5f, 0.3f, true);
            c.Part([&](Pix& t) { t.Ell(x, y - 96, 5, 5, PELLE); });
            c.Part([&](Pix& t) { t.R(x - 7, y - 104, 14, 3, C(0x2a2028)); t.R(x - 4, y - 112, 8, 8, C(0x2a2028)); });
            c.R(x - 4, y - 96, 2, 1, C(0x2a1a10)); c.R(x - 5, y - 92, 10, 2, C(0x6a4a30));
            c.Part([&](Pix& t) { t.R(x - 44, y - 82, 16, 14, scuro); t.R(x - 44, y - 68, 10, 18, ferro); }, 0.35f);
            for (int k = 0; k < 3; k++) c.Part([&](Pix& t) { t.R(x - 54, y - 54 + k * 4, 16, 2, C(0x5a5a64)); });
            c.Part([&](Pix& t) { t.R(x + 28, y - 82, 16, 14, scuro); t.R(x + 32, y - 68, 10, 20, ferro); }, 0.35f);
            Fumaiolo(x + 16, y - 86, 6, 12); Fumaiolo(x - 16, y - 86, 6, 12);
            break;
        }
    }
}

// --- Gli ambienti: cielo e sfondo lontano, strato medio con la scena, suolo ripetuto ---
const char* NOMI_SCENE[10] = { "Canyon", "Citta'", "Miniera", "Fiume", "Ponte", "Forte", "Deserto", "Neve", "Ferrovia nel cielo", "Fortezza di ferro" };
constexpr int LS = 768;   // larghezza degli strati che si ripetono
constexpr int Y_SUOLO = 148;

struct Rnd { uint32_t s; float F() { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.0f; } float F(float a, float b) { return a + (b - a) * F(); } int I(int a, int b) { return a + (int)(F() * (b - a + 1) * 0.9999f); } };

Color Mix(Color a, Color b, float t) { return Mescola(a, b, Limita(t, 0, 1)); }

void Ambiente(int scena, Color cA, Color cB, Color roc, Color ter, Pix& lon, Pix& med, Pix& suo) {
    Rnd r{ (uint32_t)(1234 + scena * 77) };
    auto W = [&](Pix& p, float x, float y, float w, float h, Color c) { for (int k = -1; k <= 1; k++) p.R(x + k * LS, y, w, h, c); };
    auto WP = [&](Pix& p, std::vector<Vector2> pts, Color c) { for (int k = -1; k <= 1; k++) { auto q = pts; for (auto& v : q) v.x += k * LS; p.Poly(q, c); } };
    auto WD = [&](Pix& p, float x, float y, int rr, Color c) { for (int k = -1; k <= 1; k++) p.Disc(x + k * LS, y, rr, c); };
    auto WPart = [&](Pix& p, const std::function<void(Pix&, float)>& f, float hi = 0.25f, float lo = 0.3f) { p.Part([&](Pix& t) { for (int k = -1; k <= 1; k++) f(t, (float)k * LS); }, hi, lo); };
    bool caverna = scena == 2;
    // cielo
    for (int y = 0; y < LH; y++) lon.R(0, y, LS, 1, Mix(cA, cB, y / 140.0f));
    if (caverna) {
        for (int i = 0; i < 70; i++) { float x = r.F(0, LS), y = r.F(0, 140); WD(lon, x, y, r.I(6, 18), Sc(cA, r.F(0.75f, 1.2f))); }
        for (int i = 0; i < 5; i++) { float x = r.F(0, LS); for (int y = 0; y < 150; y++) W(lon, x + y * 0.3f, y, 14, 1, Color{ 255, 230, 170, (unsigned char)(26 - y / 8) }); }
    } else {
        // nuvole a bande
        for (int i = 0; i < 12; i++) {
            float x = r.F(0, LS), y = r.F(14, 64), L = r.F(40, 90);
            for (int k = 0; k < 4; k++) { W(lon, x + k * 6, y + k * 2, L - k * 10, 2, Color{ 240, 232, 215, (unsigned char)(140 - k * 25) }); W(lon, x + k * 6 + 4, y + k * 2 + 2, L - k * 14, 1, Color{ 190, 170, 150, 90 }); }
        }
        if (scena == 6) { lon.Disc(300, 40, 14, C(0xfff0c0)); lon.Disc(300 + LS / 2, 40, 0, C(0xfff0c0)); }
    }
    // rilievi lontani
    Color lontane = Mix(roc, cB, 0.35f);
    if (scena == 7) lontane = Mix(C(0xe8eef4), cB, 0.2f);
    if (scena == 8) {
        // mare di nuvole sotto l'orizzonte
        for (int i = 0; i < 60; i++) WD(lon, r.F(0, LS), r.F(120, 150), r.I(8, 20), Mix(C(0xffffff), cB, r.F(0, 0.4f)));
        lon.R(0, 140, LS, 76, Mix(C(0xf4f0ff), cB, 0.3f));
    } else if (scena == 9) {
        for (int i = 0; i < 12; i++) { float x = r.F(0, LS), w = r.F(20, 50), h = r.F(30, 80); W(lon, x, 140 - h, w, h, Sc(lontane, 0.6f)); W(lon, x + w * 0.3f, 140 - h - 20, 5, 20, Sc(lontane, 0.5f)); }
        for (int i = 0; i < 30; i++) WD(lon, r.F(0, LS), r.F(20, 90), r.I(4, 9), Color{ 60, 50, 50, 90 });
    } else if (!caverna) {
        for (int i = 0; i < 4; i++) {
            float x = i * 192 + r.F(0, 60), w = r.F(90, 150), top = r.F(52, 80);
            WP(lon, { { x, 130 }, { x + 8, top + 4 }, { x + 18, top }, { x + w - 16, top - 2 }, { x + w - 6, top + 8 }, { x + w + 12, 130 } }, Sc(lontane, r.F(0.95f, 1.05f)));
            if (scena != 7) for (int k = 0; k < 8; k++) W(lon, x + 10, top + 6 + k * 6, w - 10, 1, Color{ 130, 90, 70, 90 });
            else W(lon, x + 8, top, w - 16, 5, C(0xffffff));
        }
        for (int i = 0; i < 300; i++) lon.R(r.F(0, LS), r.F(56, 128), 1, 1, r.F() < 0.5f ? Color{ 255, 230, 200, 46 } : Color{ 90, 60, 50, 46 });
    }
    // colline medie (strato medio, piu' vicino)
    Color colline = Mix(roc, ter, 0.5f);
    if (scena == 7) colline = C(0xdfe8f0);
    if (!caverna && scena != 8) {
        std::vector<Vector2> pts{ { 0, 150 } };
        for (int x = 0; x <= LS; x += 48) pts.push_back({ (float)x, 106 + 8 * std::sin(x * 0.02f + scena) + 5 * std::sin(x * 0.07f) });
        pts.push_back({ (float)LS, 150 });
        med.Poly(pts, colline);
        for (int i = 0; i < 400; i++) med.R(r.F(0, LS), r.F(112, 150), r.F(1, 3), 1, Sc(colline, r.F(0.85f, 1.15f)));
    }
    Color legno = C(0x8a6640), legnoS = C(0x4a3020);
    auto Trave = [&](float x, float y, float w, float h) { W(med, x, y, w, h, C(0x6e4a2a)); W(med, x, y, 1, h, C(0xa07448)); W(med, x + w - 1, y, 1, h, C(0x3a2414)); };
    switch (scena) {
        case 0:   // canyon: pilastri di roccia e ponte di corda
            for (int i = 0; i < 4; i++) {
                float x = 40 + i * 190 + r.F(0, 40), w = r.F(26, 44), top = r.F(40, 70);
                WPart(med, [&](Pix& t, float o) { t.Poly({ { x + o, 150 }, { x + 4 + o, top }, { x + w - 4 + o, top - 3 }, { x + w + o, 150 } }, roc); });
                for (int k = 0; k < 14; k++) W(med, x + 3, top + 4 + k * 7, w - 4, 1, Color{ 110, 60, 45, 110 });
                if (i % 2 == 0) for (int k = 0; k < 14; k++) W(med, x + w + k * 7, top + 10 + std::sin(k / 13.0f * PIg) * 9, 5, 2, legno);
            }
            break;
        case 1:   // citta': saloon, banca, emporio, torre dell'acqua
            for (int i = 0; i < 3; i++) {
                float x = 30 + i * 250, w = r.F(80, 100), h = r.F(46, 62), y = 152 - h;
                Color muro = i == 0 ? C(0xa8805c) : i == 1 ? C(0x9a7a64) : C(0xb08a62);
                WPart(med, [&](Pix& t, float o) { t.R(x + o, y, w, h, muro); t.Poly({ { x - 4 + o, y }, { x + w / 2 + o, y - 12 }, { x + w + 4 + o, y } }, C(0x7a5438)); }, 0.2f, 0.25f);
                for (int k = 0; k < h / 4; k++) W(med, x, y + 2 + k * 4, w, 1, Color{ 60, 40, 25, 110 });
                for (int k = 0; k < 3; k++) { W(med, x + 8 + k * 28, y + 8, 12, 14, C(0x2a1a14)); W(med, x + 8 + k * 28, y + 8, 12, 1, C(0xd0a878)); W(med, x + 13 + k * 28, y + 8, 1, 14, C(0x6a4a30)); }
                W(med, x + w / 2 - 8, y + h - 26, 16, 26, C(0x2a1a14));
                W(med, x - 4, y + 28, w + 8, 3, C(0x6a4628));
                W(med, x + 10, y - 8, w - 20, 9, C(0xd8c08a));
                for (int k = 0; k < 6; k++) W(med, x + 16 + k * ((w - 32) / 6), y - 5, 3, 3, C(0x7a2a1a));
            }
            WPart(med, [&](Pix& t, float o) { t.Ell(220 + o, 60, 15, 13, C(0x8a5e38)); t.Poly({ { 205 + o, 56 }, { 220 + o, 44 }, { 235 + o, 56 } }, C(0x6a4426)); }, 0.3f, 0.3f);
            for (float x0 : { 210.0f, 230.0f }) for (int k = -1; k <= 1; k++) med.Line(x0 + k * LS, 72, x0 + (x0 < 220 ? -6 : 6) + k * LS, 150, legnoS, 2);
            break;
        case 2:   // miniera: travi di sostegno e lanterne
            for (int i = 0; i < 6; i++) {
                float x = i * 128.0f + 20;
                Trave(x, 40, 5, 112); Trave(x + 60, 40, 5, 112); W(med, x - 4, 36, 74, 6, C(0x7a5434)); W(med, x - 4, 36, 74, 1, C(0xb08050));
                W(med, x + 30, 44, 4, 6, C(0x3a3a40)); WD(med, x + 32, 53, 3, C(0xffd070));
                for (int y = 50; y < 70; y++) W(med, x + 32 - (y - 50), y, (y - 50) * 2, 1, Color{ 255, 210, 120, 18 });
            }
            for (int i = 0; i < 40; i++) WD(med, r.F(0, LS), r.F(0, 30), r.I(4, 10), Sc(cA, 0.8f));
            break;
        case 3:   // fiume con battelli lontani e pontili
            W(med, 0, 128, LS, 22, Mix(C(0x3a7aa0), cB, 0.3f));
            for (int i = 0; i < 80; i++) W(med, r.F(0, LS), r.F(129, 148), r.F(4, 14), 1, Color{ 220, 240, 255, 110 });
            for (int i = 0; i < 2; i++) {
                float x = 100 + i * 380;
                WPart(med, [&](Pix& t, float o) { t.R(x + o, 116, 50, 10, C(0xe0d0b0)); t.R(x + 6 + o, 106, 30, 10, C(0xd0c0a0)); t.R(x + 14 + o, 92, 4, 14, C(0x3a3a40)); t.R(x + 24 + o, 92, 4, 14, C(0x3a3a40)); t.Poly({ { x - 6 + o, 126 }, { x + 56 + o, 126 }, { x + 50 + o, 132 }, { x + o, 132 } }, C(0x6a3a28)); });
            }
            for (int i = 0; i < 4; i++) { float x = 40 + i * 190; for (int k = 0; k < 6; k++) W(med, x + k * 8, 140, 6, 3, legno); W(med, x, 143, 2, 8, legnoS); W(med, x + 40, 143, 2, 8, legnoS); }
            break;
        case 4:   // ponte: piloni del grande ponte di legno sull'abisso
            for (int i = 0; i < 6; i++) {
                float x = i * 128.0f;
                Trave(x + 10, 90, 4, 62); Trave(x + 60, 90, 4, 62);
                for (int k = -1; k <= 1; k++) { med.Line(x + 12 + k * LS, 96, x + 62 + k * LS, 150, legnoS, 2); med.Line(x + 62 + k * LS, 96, x + 12 + k * LS, 150, legnoS, 2); }
                W(med, x, 86, 128, 5, C(0x7a5434));
            }
            break;
        case 5:   // forte: palizzata con torri e bandiere
            for (int x = 0; x < LS; x += 7) { float h = 40 + (x / 7 % 3) * 2; WPart(med, [&](Pix& t, float o) { t.R(x + o, 150 - h, 6, h, C(0x8a5e38)); t.Poly({ { x + o, 150 - h }, { x + 3 + o, 145 - h }, { x + 6 + o, 150 - h } }, C(0x8a5e38)); }, 0.25f, 0.3f); }
            for (int i = 0; i < 2; i++) {
                float x = 120 + i * 384;
                WPart(med, [&](Pix& t, float o) { t.R(x + o, 60, 40, 90, C(0x7a5434)); t.Poly({ { x - 6 + o, 60 }, { x + 20 + o, 42 }, { x + 46 + o, 60 } }, C(0x5a3a22)); });
                W(med, x + 14, 80, 12, 10, C(0x2a1a14)); W(med, x + 19, 20, 1, 24, C(0x3a2a1a)); W(med, x + 20, 20, 14, 8, C(0xc03028)); W(med, x + 20, 24, 14, 1, C(0xf0e0c0));
            }
            break;
        case 6:   // deserto: dune, cactus, teschio
            for (int i = 0; i < 5; i++) { float x = i * 160 + r.F(0, 40); WP(med, { { x, 152 }, { x + 50, 122 + r.F(0, 10) }, { x + 120, 152 } }, Sc(ter, 1.08f)); }
            for (int i = 0; i < 5; i++) {
                float x = 60 + i * 150 + r.F(0, 30), h = r.F(20, 34);
                WPart(med, [&](Pix& t, float o) { t.R(x + o, 150 - h, 6, h, C(0x4e7a3e)); t.R(x - 8 + o, 150 - h + 8, 8, 4, C(0x4e7a3e)); t.R(x - 8 + o, 150 - h + 2, 4, 8, C(0x4e7a3e)); t.R(x + 6 + o, 150 - h + 12, 7, 4, C(0x4e7a3e)); t.R(x + 9 + o, 150 - h + 5, 4, 9, C(0x4e7a3e)); }, 0.3f, 0.3f);
            }
            WPart(med, [&](Pix& t, float o) { t.Ell(500 + o, 146, 7, 5, C(0xf0e8d8)); }, 0.3f, 0.3f);
            W(med, 496, 145, 2, 2, C(0x2a1a10)); W(med, 501, 145, 2, 2, C(0x2a1a10));
            break;
        case 7:   // neve: abeti innevati
            for (int i = 0; i < 12; i++) {
                float x = i * 64 + r.F(0, 30), h = r.F(30, 50);
                WPart(med, [&](Pix& t, float o) { t.Poly({ { x + o, 150 - h }, { x - 14 + o, 150 }, { x + 14 + o, 150 } }, C(0x2e5a44)); }, 0.3f, 0.35f);
                for (int k = 0; k < 3; k++) WP(med, { { x, 150 - h + k * 10 }, { x - 5 - k * 3, 150 - h + 6 + k * 10 }, { x + 5 + k * 3, 150 - h + 6 + k * 10 } }, C(0xf4f8ff));
                W(med, x - 1, 150, 3, 2, C(0x4a3020));
            }
            break;
        case 8:   // ferrovia nel cielo: isole volanti e tralicci
            for (int i = 0; i < 4; i++) {
                float x = i * 192 + r.F(0, 40), y = r.F(60, 100), w = r.F(30, 60);
                WPart(med, [&](Pix& t, float o) { t.Poly({ { x + o, y }, { x + w + o, y }, { x + w * 0.6f + o, y + 26 }, { x + w * 0.3f + o, y + 20 } }, roc); }, 0.3f, 0.35f);
                W(med, x, y - 2, w, 3, C(0x5a9a4a));
            }
            for (int i = 0; i < 6; i++) { float x = i * 128.0f + 30; Trave(x, 150, 4, 66); for (int k = -1; k <= 1; k++) med.Line(x + 2 + k * LS, 160, x + 64 + k * LS, 216, legnoS, 2); }
            break;
        case 9:   // fortezza di ferro: mura, tubi e ingranaggi
            for (int i = 0; i < 4; i++) {
                float x = i * 192.0f;
                WPart(med, [&](Pix& t, float o) { t.R(x + o, 70, 150, 80, C(0x5a5a62)); }, 0.2f, 0.25f);
                for (int k = 0; k < 10; k++) W(med, x + 4 + k * 15, 74, 1, 72, C(0x3a3a42));
                for (int k = 0; k < 9; k++) { W(med, x + 8 + k * 16, 76, 1, 1, C(0xc0c0c8)); W(med, x + 8 + k * 16, 144, 1, 1, C(0xc0c0c8)); }
                W(med, x, 96, 150, 5, C(0x8a5a30)); W(med, x, 96, 150, 1, C(0xc08a50));
                WD(med, x + 110, 120, 14, C(0x6a6a72)); WD(med, x + 110, 120, 5, C(0x3a3a42));
                W(med, x + 30, 40, 10, 30, C(0x3a3a42));
                for (int k = 0; k < 5; k++) WD(med, x + 35 + k * 3, 34 - k * 6, 4 + k, Color{ 70, 60, 60, 120 });
            }
            break;
    }
    // suolo (si ripete ogni 384 pixel)
    int sh = LH - Y_SUOLO;
    for (int y = 0; y < sh; y++) { float t = y / (float)sh; suo.R(0, y, LW, 1, Mix(ter, Sc(ter, 0.45f), t)); }
    suo.R(0, 0, LW, 2, Sc(ter, 1.2f));
    Rnd q{ (uint32_t)(99 + scena) };
    for (int i = 0; i < 900; i++) suo.R(q.F(0, LW), q.F(2, (float)sh), q.F(1, 3), 1, Sc(ter, q.F(0.6f, 1.2f)));
    if (scena == 2 || scena == 8 || scena == 1) {   // binari
        int yb = (int)(TERRA - Y_SUOLO) + 2;
        for (int x = 0; x < LW; x += 10) { suo.R(x, yb, 7, 4, C(0x4a2e18)); suo.R(x, yb, 7, 1, C(0x7a5434)); }
        suo.R(0, yb - 3, LW, 2, C(0xa8a8b4)); suo.R(0, yb - 1, LW, 1, C(0x4a4a58));
    }
    if (scena == 4) { for (int x = 0; x < LW; x += 8) { suo.R(x, 0, 7, 10, C(0x8a6640)); suo.R(x, 0, 7, 1, C(0xb89060)); } suo.R(0, 10, LW, 3, C(0x3a2414)); }
    if (scena == 7) { suo.R(0, 0, LW, 4, C(0xf4f8ff)); for (int x = 0; x < LW; x += 3) suo.R(x, 4, 2, q.I(0, 3), C(0xe0e8f4)); }
    if (scena == 9) { for (int x = 0; x < LW; x += 32) { suo.R(x, 0, 1, sh, C(0x2a2a30)); suo.R(x + 4, 4, 1, 1, C(0xc0c0c8)); } }
    if (scena == 3) for (int i = 0; i < 40; i++) suo.R(q.F(0, LW), 0, 1, 2, C(0x6a8a3a));
}

// Oggetti del mondo disegnati una volta
void Impalcatura(Pix& c, float x, float y) {   // piedi in (x, y), piattaforma 74 pixel sopra
    auto Tr = [&](float tx, float ty, float w, float h) { c.R(tx, ty, w, h, C(0x6e4a2a)); c.R(tx, ty, 1, h, C(0xa07448)); c.R(tx + w - 1, ty, 1, h, C(0x3a2414)); };
    Tr(x - 22, y - 74, 4, 74); Tr(x + 18, y - 74, 4, 74); Tr(x - 1, y - 74, 3, 74);
    for (int k = 0; k < 3; k++) { c.Line(x - 19, y - 62 + k * 20, x + 19, y - 44 + k * 20, C(0x5a3a20), 2); c.Line(x + 19, y - 62 + k * 20, x - 19, y - 44 + k * 20, C(0x5a3a20), 2); }
    c.R(x - 28, y - 80, 56, 7, C(0x8a6640)); for (int k = 0; k < 8; k++) c.R(x - 27 + k * 7, y - 80, 1, 7, C(0x4a3020)); c.R(x - 28, y - 80, 56, 1, C(0xb89060));
}
void Sacchi(Pix& c, float x, float y) {
    for (int k = 0; k < 4; k++) c.Part([&](Pix& t) { t.Ell(x - 15 + k * 10, y - 4, 6, 4, C(0xbca476)); }, 0.25f);
    for (int k = 0; k < 3; k++) c.Part([&](Pix& t) { t.Ell(x - 10 + k * 10, y - 10, 6, 4, C(0xc8b080)); }, 0.25f);
}
void Roccia(Pix& c, float x, float y, int r) {
    c.Part([&](Pix& t) { t.Ell(x, y, r + 3, r, C(0x8a7866)); t.Ell(x + r * 0.6f, y + 1, (int)(r * 0.7f), (int)(r * 0.7f), C(0x7a6858)); }, 0.32f, 0.34f, true);
}

// ============================== IL GIOCO ==============================
const char* NOMI_ASPETTI[10] = { "Treno Corazzato", "Sceriffo d'Acciaio", "Talpa Trivella", "Battello a Ruota", "Dirigibile", "Cannone Gigante",
                                 "Scorpione Meccanico", "Mammut a Vapore", "Locomotiva Drago", "Robot Gigante" };
const char* NOMI_ARMI[5] = { "PISTOLA", "MITRAGLIATRICE", "PALLETTONI", "FIAMME", "RAZZI" };
const char* SIGLE_ARMI[5] = { "", "H", "S", "F", "R" };

std::string Hx(unsigned c) { char b[8]; std::snprintf(b, sizeof b, "#%06x", c); return b; }

struct Ogg { int tipo; float x, y; float hp; bool vivo = true; int premio = 0; };   // 0 cassa, 1 barile, 2 barile di polvere, 3 impalcatura, 4 sacchi, 5 roccia
struct Piatt { float x0, x1, y; bool solida; int ogg; };
struct Prig { float x, y; int stato = 0; float t = 0; int dono = 0; bool sopra = false; };   // 0 legato, 1 corre, 2 dona, 3 se ne va
struct Nemico {
    int tipo, comp; float x, y, vx = 0, vy = 0, hp; int dir = -1; bool aTerra = true;
    int stato = 0; float t = 0, tFuoco = 1, tAnim = 0, yPiatt = TERRA, lampo = 0; float baseY = 0;
};
struct Proiettile { float x, y, vx, vy; int tipo; float vita, danno; bool nemico; float r = 2; float t = 0; };
struct Premio { float x, y, vy = -90; int tipo; float t = 0; };   // 1-4 armi, 5 bombe, 6 oro, 7 vita
struct Boom { float x, y, t = 0, durata, r; unsigned seme; };
struct Fumo { float x, y, vx, vy, vita, durata; int r; Color c; };
struct Arrivo { float x; int nemico, quanti; bool alto = false, fatto = false; };
struct TestoVolante { float x, y; std::string t; float vita; Color c; };
struct BossP {
    bool vivo = false; int indice = 0, aspetto = 0, comp = 0; float x = 0, y = TERRA, hp = 1, hpMax = 1, t = 0, tA = 2, tB = 3, tC = 6, vx = 0, vy = 0;
    float lampo = 0, tMorte = 0, ingresso = 0, yOff = 0, tF = 0; int fase = 0, f = 0; bool sotto = false; float bersX = 0, tFiamma = 0;
};

class Polvere : public GiocoArcade {
public:
    const char* Nome() const override { return "polvere"; }
    std::string Titolo() const override { return "Polvere da Sparo: Commando del West"; }
    int L() const override { return LW; }
    int A() const override { return LH; }
    bool PixelArt() const override { return true; }
    std::string Tappa() const override { return "Missione " + std::to_string(missione + 1) + " - " + TestoDi(Mis(), "nome", "Missione"); }
    std::vector<std::string> Aiuto() const override {
        return { "Frecce, A/D o levetta: corri; Su: mira in alto; Giu' in volo: spara in basso",
                 "Spazio, J, clic o A: spara   C, L, Maiuscolo, X o Y: salta",
                 "Z, X, K, clic destro o B: lancia la dinamite (dal carro: cannonata)",
                 "Libera gli ostaggi per avere armi; sali sul carro Bisonte toccandolo (Giu'+salto per scendere)" };
    }
    Brano MusicaMenu() const override { return BranoDi(TestoDi(Gen(), "musicaMenu", "Polvere - Tema")); }

    // --- dati del progetto ---
    static const json& Elem(const json& lista, int k) { static const json vuoto = json::object(); return lista.is_array() && k >= 0 && k < (int)lista.size() ? lista[k] : vuoto; }
    int NMis() const { return cfg["missioni"].is_array() && !cfg["missioni"].empty() ? (int)cfg["missioni"].size() : 1; }
    const json& Mis() const { return Elem(cfg["missioni"], missione); }
    const json& Gen() const { return cfg["generale"]; }
    const json& Armi() const { return cfg["armi"]; }
    const json& Amb(int k) const { return Elem(cfg["ambienti"], k); }
    const json& NemJ(int k) const { return Elem(cfg["nemici"], k); }
    const json& BossJ(int k) const { return Elem(cfg["boss"], k); }
    int IndAmb() const { int a = IntDi(Mis(), "ambiente", missione % 10); return std::max(0, std::min(a, (int)cfg["ambienti"].size() - 1)); }
    int NNem() const { return cfg["nemici"].is_array() ? (int)cfg["nemici"].size() : 0; }

    static std::vector<std::pair<std::string, std::string>> NomiSuoni() {
        return { { "ps_pistola", "Pistola" }, { "ps_mitra", "Mitragliatrice" }, { "ps_pallettoni", "Pallettoni" }, { "ps_fiamme", "Lanciafiamme" },
                 { "ps_razzo", "Razzo" }, { "ps_lancio", "Lancio della dinamite" }, { "ps_esplosione", "Esplosione" }, { "ps_boom", "Grande esplosione" },
                 { "ps_metallo", "Colpo su metallo" }, { "ps_bandito", "Bandito colpito" }, { "ps_morte", "Eroe colpito" }, { "ps_ostaggio", "Ostaggio liberato" },
                 { "ps_arma", "Arma presa" }, { "ps_salto", "Salto" }, { "ps_carro", "Sale sul carro" }, { "ps_cannone", "Cannone del carro" },
                 { "ps_allarme", "Arriva il boss" }, { "ps_missione", "Missione compiuta" }, { "ps_vita", "Vita extra" }, { "ps_nemico", "Sparo dei banditi" } };
    }
    json Predefiniti() const override {
        json j;
        j["generale"] = { { "vite", 3 }, { "bombe", 10 }, { "vitaExtra", 20000 }, { "vitaOgni", 60000 }, { "velocita", 72 }, { "salto", 1.0 },
                          { "giacca1", "#9a7040" }, { "cappello1", "#6e4a28" }, { "calzoni1", "#55668c" },
                          { "giacca2", "#3e6e4e" }, { "cappello2", "#5a3a20" }, { "calzoni2", "#5a5a7a" },
                          { "musicaMenu", "Polvere - Tema" }, { "fine", "IL BARONE RUGGINE E' SCONFITTO!" } };
        j["armi"] = { { "mitragliatrice", 200 }, { "pallettoni", 30 }, { "fiamme", 120 }, { "razzi", 30 }, { "carro", 6 }, { "bombeOstaggio", 10 } };
        struct A { const char* n; int scena; unsigned c1, c2, roc, ter; int meteo; };
        const A amb[10] = { { "Canyon del Serpente", 0, 0x96a8aa, 0xe2c496, 0xb8967c, 0xba8c60, 1 }, { "Citta' Fantasma", 1, 0x8a9aa8, 0xe2c496, 0xae8c74, 0xb08a5c, 1 },
                            { "Miniera d'Oro", 2, 0x2a1e1a, 0x5a4030, 0x6a5040, 0x6a4a32, 4 }, { "Fiume dei Battelli", 3, 0x7ab0d8, 0xe8d8b0, 0xa08870, 0x8a7a4a, 0 },
                            { "Ponte Sospeso", 4, 0x9ab8d0, 0xf0d8a8, 0x9a7a64, 0x6a4a30, 0 }, { "Forte Abbandonato", 5, 0xb0a090, 0xf0c890, 0xa8846a, 0xa07a50, 1 },
                            { "Deserto Rosso", 6, 0xe0a070, 0xffd8a0, 0xc0603a, 0xd0884a, 1 }, { "Montagne di Neve", 7, 0x8aa8c8, 0xe8f0f8, 0x8a8aa0, 0xc8d0dc, 2 },
                            { "Ferrovia Celeste", 8, 0x5a7ad0, 0xf8d0e0, 0x8a7a8a, 0x7a6a6a, 0 }, { "Fortezza del Barone", 9, 0x3a2a2a, 0xa05a3a, 0x5a4a4a, 0x4a4044, 4 } };
        for (auto& a : amb)
            j["ambienti"].push_back({ { "nome", a.n }, { "scena", a.scena }, { "cielo", Hx(a.c1) }, { "orizzonte", Hx(a.c2) }, { "rocce", Hx(a.roc) }, { "terreno", Hx(a.ter) }, { "meteo", a.meteo } });
        struct N { const char* n; int comp; unsigned g, c; int vita; float vel, fuoco; int punti; };
        const N nem[6] = { { "Pistolero", 0, 0x9a3c2c, 0x2c2228, 1, 1, 1, 100 }, { "Dinamitardo", 1, 0x6e5e3e, 0x3a2a20, 1, 1, 1, 150 },
                           { "Cecchino", 2, 0x4e4e70, 0x2c2228, 1, 1, 1, 200 }, { "Bandito-robot", 3, 0x8a8a94, 0x2a2a30, 8, 0.6f, 1, 500 },
                           { "Cavaliere meccanico", 4, 0x7e3c3c, 0x2c2228, 3, 1, 1, 400 }, { "Girocottero", 5, 0x707060, 0x3a3a2a, 10, 1, 1, 1000 } };
        for (auto& n : nem)
            j["nemici"].push_back({ { "nome", n.n }, { "comportamento", n.comp }, { "giacca", Hx(n.g) }, { "cappello", Hx(n.c) }, { "vita", n.vita },
                                    { "velocita", n.vel }, { "fuoco", n.fuoco }, { "punti", n.punti } });
        const int compB[10] = { 0, 2, 3, 4, 1, 4, 2, 2, 0, 2 };
        const unsigned colB[10] = { 0x5e5e54, 0x8a8a94, 0x8a6a3a, 0x7a2a2a, 0xa06a50, 0x4a4a52, 0x9a5a2a, 0x6a6a74, 0x3a6a4a, 0x8a4a2a };
        const char* nomiB[10] = { "Treno Corazzato", "Sceriffo d'Acciaio", "Talpa Trivella", "Battello a Ruota da Guerra", "Dirigibile Bandito", "Cannone Gigante",
                                  "Scorpione Meccanico", "Mammut a Vapore", "Locomotiva Drago", "Robot Gigante del Barone Ruggine" };
        for (int k = 0; k < 10; k++)
            j["boss"].push_back({ { "nome", nomiB[k] }, { "aspetto", k }, { "comportamento", compB[k] }, { "colore", Hx(colB[k]) }, { "vita", 110 + k * 22 },
                                  { "aggressivita", 1.0 }, { "punti", 10000 + k * 5000 }, { "musica", "Polvere - Boss" } });
        struct M { const char* n; int amb, schermi; float dens; int n1, n2, n3, ost; bool carro; const char* mus; };
        const M mis[10] = { { "Canyon del Serpente", 0, 9, 0.8f, 0, 1, -1, 4, false, "Polvere - Canyon" },
                            { "Citta' Fantasma", 1, 10, 0.9f, 0, 2, 1, 5, true, "Polvere - Citta' Fantasma" },
                            { "Miniera d'Oro", 2, 10, 1.0f, 0, 3, 1, 5, false, "Polvere - Miniera" },
                            { "Fiume dei Battelli", 3, 11, 1.0f, 0, 4, 2, 5, true, "Polvere - Frontiera" },
                            { "Ponte Sospeso", 4, 11, 1.1f, 0, 5, 1, 5, false, "Polvere - Frontiera" },
                            { "Forte Abbandonato", 5, 12, 1.1f, 0, 2, 3, 6, true, "Polvere - Canyon" },
                            { "Deserto Rosso", 6, 12, 1.2f, 4, 1, 3, 6, true, "Polvere - Deserto" },
                            { "Montagne di Neve", 7, 13, 1.25f, 0, 3, 5, 6, false, "Polvere - Miniera" },
                            { "Ferrovia Celeste", 8, 13, 1.3f, 4, 5, 3, 6, true, "Polvere - Frontiera" },
                            { "Fortezza del Barone", 9, 14, 1.4f, 3, 2, 4, 7, true, "Polvere - Fortezza" } };
        for (int k = 0; k < 10; k++) {
            auto& m = mis[k];
            j["missioni"].push_back({ { "nome", m.n }, { "ambiente", m.amb }, { "schermi", m.schermi }, { "densita", m.dens }, { "nemico1", m.n1 }, { "nemico2", m.n2 },
                                      { "nemico3", m.n3 }, { "ostaggi", m.ost }, { "carro", m.carro }, { "boss", k }, { "tempo", 60 }, { "musica", m.mus } });
        }
        json su = json::object();
        for (auto& n : NomiSuoni()) su[n.first] = "";
        j["suoni"] = su;
        return j;
    }
    json Schema() const override {
        json voci = json::array();
        auto Cat = [&](const char* chiave, const char* nome, bool lista, const char* aiuto, json campi, int min = 0, int max = 0, const char* elemento = "") {
            json c = { { "chiave", chiave }, { "nome", nome }, { "lista", lista }, { "aiuto", aiuto }, { "campi", campi } };
            if (lista) { c["min"] = min; c["max"] = max; c["elemento"] = elemento; }
            voci.push_back(c);
        };
        auto N = [](const char* k, const char* n, double mi, double ma, double passo, const char* f = "%.0f") { return json{ { "chiave", k }, { "nome", n }, { "tipo", "numero" }, { "min", mi }, { "max", ma }, { "passo", passo }, { "formato", f } }; };
        auto Co = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "colore" } }; };
        auto T = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "testo" } }; };
        auto S = [](const char* k, const char* n, json v) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "scelta" }, { "voci", v } }; };
        auto Rf = [](const char* k, const char* n, const char* da, const char* vuoto = "") { json f = { { "chiave", k }, { "nome", n }, { "tipo", "rif" }, { "da", da } }; if (*vuoto) f["vuoto"] = vuoto; return f; };
        auto B = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "brano" } }; };
        auto V = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "vero" } }; };
        json G = { { "chiave", "grafica" }, { "nome", "Grafica" }, { "tipo", "grafica" } };
        json Sf = { { "chiave", "sfondo" }, { "nome", "Sfondo" }, { "tipo", "sfondo" } };
        json scene = json::array(); for (auto s : NOMI_SCENE) scene.push_back(s);
        json aspetti = json::array(); for (auto s : NOMI_ASPETTI) aspetti.push_back(s);
        Cat("missioni", "Missioni", true, "Ogni missione: l'ambiente, quanto e' lunga (in schermate), quanti banditi, quali tre tipi di nemici, quanti ostaggi da liberare, "
            "se c'e' il carro Bisonte da guidare, il boss finale, il tempo e la musica. Il gioco finisce dopo l'ultima missione.",
            { T("nome", "Nome"), Rf("ambiente", "Ambiente", "ambienti"), N("schermi", "Lunghezza (schermate)", 3, 40, 1), N("densita", "Quanti nemici", 0.2, 3, 0.05, "%.2f"),
              Rf("nemico1", "Nemico 1", "nemici", "(nessuno)"), Rf("nemico2", "Nemico 2", "nemici", "(nessuno)"), Rf("nemico3", "Nemico 3", "nemici", "(nessuno)"),
              N("ostaggi", "Ostaggi", 0, 20, 1), V("carro", "Carro Bisonte"), Rf("boss", "Boss finale", "boss", "(nessuno)"), N("tempo", "Tempo (secondi)", 20, 99, 1),
              B("musica", "Musica") }, 1, 40, "Missione");
        Cat("ambienti", "Ambienti", true, "I luoghi delle missioni: la scena disegnata (canyon, citta', miniera...), i colori del cielo, delle rocce e del terreno e il tempo che fa.",
            { T("nome", "Nome"), Sf, S("scena", "Scena", scene), Co("cielo", "Cielo in alto"), Co("orizzonte", "Cielo all'orizzonte"), Co("rocce", "Rocce e montagne"),
              Co("terreno", "Terreno"), S("meteo", "Meteo", { "Sereno", "Polvere nel vento", "Neve", "Pioggia", "Braci" }) }, 1, 20, "Ambiente");
        Cat("nemici", "Nemici", true, "I tipi di nemici: nome, comportamento (pistolero, dinamitardo, cecchino sulle torri, robot a vapore, cavaliere meccanico, girocottero), "
            "colori, resistenza, velocita', quanto sparano e punti.",
            { T("nome", "Nome"), G, S("comportamento", "Comportamento", { "Pistolero", "Dinamitardo", "Cecchino sulla torre", "Robot a vapore", "Cavaliere meccanico", "Girocottero" }),
              Co("giacca", "Giacca (o corpo)"), Co("cappello", "Cappello"), N("vita", "Colpi per abbatterlo", 1, 40, 1), N("velocita", "Velocita'", 0.3, 2.5, 0.05, "%.2f"),
              N("fuoco", "Quanto spara", 0.2, 3, 0.05, "%.2f"), N("punti", "Punti", 10, 5000, 10) }, 1, 12, "Nemico");
        Cat("boss", "Boss", true, "I boss di fine missione: nome, disegno, comportamento (veicolo sui binari, volante, gigante che cammina, sotterraneo, fortezza fissa), colore, "
            "resistenza, aggressivita', punti e musica.",
            { T("nome", "Nome"), G, S("aspetto", "Disegno", aspetti),
              S("comportamento", "Comportamento", { "Veicolo sui binari (granate, raffiche, rinforzi)", "Volante (bombe e colpi mirati)", "Gigante (pestoni, ventagli, cariche)",
                                                      "Sotterraneo (sbuca sotto di te, pioggia di sassi)", "Fortezza fissa (cannonate, palle che rotolano)" }),
              Co("colore", "Colore"), N("vita", "Resistenza", 20, 1000, 5), N("aggressivita", "Aggressivita'", 0.3, 3, 0.05, "%.2f"), N("punti", "Punti", 0, 100000, 500),
              B("musica", "Musica") }, 1, 20, "Boss");
        Cat("generale", "Eroi e regole", false, "Le regole della partita e i colori dei due eroi (il giocatore 2 gioca a turni).",
            { G, N("vite", "Vite all'inizio", 1, 9, 1), N("bombe", "Dinamite all'inizio", 0, 99, 1), N("vitaExtra", "Prima vita extra a", 0, 300000, 1000),
              N("vitaOgni", "Poi una vita ogni", 1000, 500000, 1000), N("velocita", "Velocita' dell'eroe", 40, 140, 2), N("salto", "Altezza del salto", 0.6, 1.8, 0.05, "%.2f"),
              Co("giacca1", "Giocatore 1: giacca"), Co("cappello1", "Giocatore 1: cappello"), Co("calzoni1", "Giocatore 1: calzoni"),
              Co("giacca2", "Giocatore 2: giacca"), Co("cappello2", "Giocatore 2: cappello"), Co("calzoni2", "Giocatore 2: calzoni"),
              B("musicaMenu", "Musica del menu"), T("fine", "Scritta della vittoria") });
        Cat("armi", "Armi e carro", false, "Quanti colpi danno le armi raccolte (H mitragliatrice, S pallettoni, F fiamme, R razzi), i colpi che regge il carro Bisonte "
            "e quanta dinamite regala un ostaggio.",
            { N("mitragliatrice", "Mitragliatrice (colpi)", 20, 999, 10), N("pallettoni", "Pallettoni (colpi)", 5, 200, 1), N("fiamme", "Fiamme (colpi)", 10, 999, 10),
              N("razzi", "Razzi (colpi)", 5, 200, 1), N("carro", "Colpi che regge il carro", 1, 30, 1), N("bombeOstaggio", "Dinamite regalata", 1, 50, 1) });
        json su = json::array();
        for (auto& n : NomiSuoni()) su.push_back({ { "chiave", n.first }, { "nome", n.second }, { "tipo", "effetto" } });
        Cat("suoni", "Effetti sonori", false, "Ogni suono del gioco: lascia 'predefinito' o scegli un effetto creato nella sezione Audio.", su);
        return voci;
    }
    int Tappe() const override { return NMis(); }
    void IniziaDa(int tappa) override { missione = std::max(0, std::min(NMis() - 1, tappa)); IniziaMissione(false); }

    std::vector<Brano> BraniPredefiniti() const override {
        std::vector<Brano> b;
        b.push_back(CreaBrano("Polvere - Tema", 132, 8, {
            { "Tromba", 2, 0.27f, "E4:1 A4:1 B4:1 C5:2 B4:1 A4:1 E5:4 D5:1 C5:1 B4:2 A4:1 G4:1 A4:4 R:2 E4:1 A4:1 B4:1 C5:2 D5:1 E5:1 G5:3 F5:1 E5:2 D5:1 B4:1 A4:4 R:4" },
            { "Chitarra", 1, 0.2f, "A2:1 E3:1 A3:1 E3:1 A2:1 E3:1 A3:1 E3:1 F2:1 C3:1 F3:1 C3:1 G2:1 D3:1 G3:1 D3:1" },
            { "Basso", 0, 0.3f, "A1:2 E2:2 A1:2 E2:2 F1:2 C2:2 G1:2 D2:2" },
            { "Batteria", 4, 0.42f, "K.hhS.hhK.hhS.hS", true } }));
        b.push_back(CreaBrano("Polvere - Canyon", 150, 8, {
            { "Guida", 2, 0.26f, "E5:1 E5:1 G5:1 E5:1 A5:2 G5:1 E5:1 D5:1 E5:1 B4:2 R:2 E5:1 E5:1 G5:1 A5:1 B5:2 A5:1 G5:1 E5:2 D5:1 B4:1 E5:4" },
            { "Galoppo", 0, 0.3f, "E2:1 E2:0.5 E2:0.5 B2:1 E2:1 E2:1 E2:0.5 E2:0.5 B2:1 E2:1 D2:1 D2:0.5 D2:0.5 A2:1 D2:1 C2:1 C2:0.5 C2:0.5 G2:1 B1:1" },
            { "Armonica", 1, 0.14f, "E4:4 G4:4 D4:4 B3:4" },
            { "Batteria", 4, 0.45f, "K.h.S.hhK.h.S.hS", true } }));
        b.push_back(CreaBrano("Polvere - Citta' Fantasma", 112, 8, {
            { "Fischio", 3, 0.3f, "A5:3 E5:1 A5:2 C6:1 B5:1 A5:4 R:2 G5:1 E5:1 G5:3 D5:1 E5:4 R:4 A5:3 E5:1 A5:2 C6:1 D6:1 E6:4 D6:1 C6:1 B5:1 G5:1 A5:6 R:2" },
            { "Chitarra", 1, 0.2f, "A2:1 E3:1 A3:1 C4:1 E3:1 A3:1 G2:1 D3:1 G3:1 B3:1 D3:1 G3:1 F2:1 C3:1 F3:1 A3:1" },
            { "Campana", 3, 0.1f, "A3:8 R:8 E3:8 R:8" },
            { "Batteria", 4, 0.32f, "K...h.S.K.K.h.S.", true } }));
        b.push_back(CreaBrano("Polvere - Miniera", 128, 8, {
            { "Guida", 0, 0.24f, "D5:1 F5:1 A5:1 D5:1 C5:1 E5:1 G5:1 C5:1 Bb4:1 D5:1 F5:1 Bb4:1 A4:2 C#5:2 D5:1 F5:1 A5:1 D6:1 C6:1 A5:1 G5:1 E5:1 F5:2 E5:1 C#5:1 D5:4" },
            { "Basso", 2, 0.34f, "D2:1 D2:1 A2:1 D2:1 C2:1 C2:1 G2:1 C2:1 Bb1:1 Bb1:1 F2:1 Bb1:1 A1:1 A1:1 E2:1 A1:1" },
            { "Picconi", 4, 0.4f, "K..hS.h.K.hhS..o", true } }));
        b.push_back(CreaBrano("Polvere - Frontiera", 140, 8, {
            { "Tromba", 2, 0.26f, "G4:1 C5:1 E5:1 G5:2 E5:1 C5:1 D5:2 G4:2 R:2 A4:1 B4:1 C5:1 D5:1 E5:1 F5:1 G5:2 E5:2 C5:4 R:2 G5:1 A5:1 G5:1 E5:1 C5:2 D5:4 C5:4" },
            { "Basso", 0, 0.32f, "C2:1 G2:1 C2:1 G2:1 F1:1 C2:1 G1:1 D2:1" },
            { "Chitarra", 1, 0.18f, "C3:1 E3:1 G3:1 E3:1 F3:1 A3:1 G3:1 B3:1" },
            { "Batteria", 4, 0.44f, "K.hKS.h.K.hKS.hh", true } }));
        b.push_back(CreaBrano("Polvere - Deserto", 118, 8, {
            { "Flauto", 3, 0.3f, "E5:2 F5:1 G#5:1 A5:2 G#5:1 F5:1 E5:4 D5:1 E5:1 F5:2 E5:1 D5:1 C5:1 B4:1 C5:4 B4:2 R:2 E5:2 F5:1 G#5:1 B5:2 A5:1 G#5:1 A5:4 G#5:1 F5:1 E5:6 R:2" },
            { "Basso", 2, 0.3f, "E2:2 E2:1 F2:1 E2:2 D2:2 C2:2 C2:1 D2:1 E2:4" },
            { "Tamburi", 4, 0.4f, "K..K..S.K.K...S.", true } }));
        b.push_back(CreaBrano("Polvere - Fortezza", 152, 8, {
            { "Guida", 2, 0.26f, "C5:1 C5:1 Eb5:1 C5:1 G5:1 F5:1 Eb5:1 D5:1 C5:1 C5:1 Eb5:1 G5:1 Ab5:2 G5:2 C5:1 C5:1 Eb5:1 C5:1 G5:1 Ab5:1 Bb5:1 B5:1 C6:4 G5:4" },
            { "Basso", 0, 0.38f, "C2:1 C3:1 C2:1 C3:1 Ab1:1 Ab2:1 Bb1:1 Bb2:1" },
            { "Batteria", 4, 0.52f, "KoK.SoK.KoKKSoKS", true } }));
        b.push_back(CreaBrano("Polvere - Boss", 160, 8, {
            { "Allarme", 2, 0.26f, "E5:1 F5:1 E5:1 B4:1 E5:1 F5:1 G5:2 F5:1 E5:1 D5:1 C5:1 B4:4 E5:1 F5:1 E5:1 B4:1 G5:1 A5:1 Bb5:2 A5:1 G5:1 F5:1 D5:1 E5:4" },
            { "Basso", 0, 0.42f, "E1:1 E2:1 E1:1 E2:1 F1:1 F2:1 E1:1 E2:1" },
            { "Batteria", 4, 0.56f, "KoKoSoKoKoKoSoKS", true } }));
        return b;
    }

    void PreparaSuoni() {
        CreaSuono("ps_pistola", 0.18f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.5f; return (0.5f * y + 0.3f * Onda(0, (900 - 3000 * t) * t)) * Inviluppo(t, 0, 0.18f, 3); });
        CreaSuono("ps_mitra", 0.1f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.6f; return (0.45f * y + 0.25f * Onda(0, 140 * t)) * Inviluppo(t, 0, 0.1f, 2.5f); });
        CreaSuono("ps_pallettoni", 0.45f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.3f; return (0.8f * y + 0.3f * Onda(3, 70 * t)) * Inviluppo(t, 0, 0.45f, 2.5f); });
        CreaSuono("ps_fiamme", 0.12f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.15f; return 0.4f * y * Inviluppo(t, 0.01f, 0.12f, 1); });
        CreaSuono("ps_razzo", 0.5f, [](float t) { static float y = 0; y += (Rumore() - y) * (0.05f + t * 0.5f); return (0.5f * y + 0.15f * Onda(2, (300 + 900 * t) * t)) * Inviluppo(t, 0.01f, 0.5f, 1.4f); });
        CreaSuono("ps_lancio", 0.3f, [](float t) { return 0.2f * Onda(1, (500 + 900 * t) * t) * Inviluppo(t, 0, 0.3f, 1.5f) + 0.1f * Rumore() * Inviluppo(t, 0, 0.1f, 2); });
        CreaSuono("ps_esplosione", 0.8f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.12f; return (1.1f * y + 0.3f * Onda(3, 55 * t)) * Inviluppo(t, 0.002f, 0.8f, 2); });
        CreaSuono("ps_boom", 2.2f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.06f; return (1.5f * y + 0.4f * Onda(3, 40 * std::exp(-t) * t)) * Inviluppo(t, 0.01f, 2.2f, 1.6f); });
        CreaSuono("ps_metallo", 0.14f, [](float t) { return 0.22f * (Onda(0, 1900 * t) * 0.5f + Onda(0, 2830 * t) * 0.4f + Rumore() * 0.3f) * Inviluppo(t, 0, 0.14f, 3); });
        CreaSuono("ps_bandito", 0.45f, [](float t) { return 0.25f * Onda(2, (520 - 500 * t) * t + 6 * std::sin(t * 60) * 0.01f) * Inviluppo(t, 0.01f, 0.45f, 1.2f); });
        CreaSuono("ps_morte", 1.2f, [](float t) {
            static const float m[] = { 659, 622, 587, 554, 523, 494, 466, 440 };
            int n = std::min(7, (int)(t * 7)); static float y = 0; y += (Rumore() - y) * 0.2f;
            return 0.24f * Onda(0, m[n] * t) * Inviluppo(t, 0, 1.2f, 1) + 0.3f * y * Inviluppo(t, 0, 0.3f, 2); });
        CreaSuono("ps_ostaggio", 0.9f, [](float t) { static const float m[] = { 784, 988, 1175, 1568 }; int n = std::min(3, (int)(t * 6)); return 0.22f * Onda(1, m[n] * t) * Inviluppo(t, 0, 0.9f, 0.8f); });
        CreaSuono("ps_arma", 0.8f, [](float t) { static const float m[] = { 523, 659, 784, 1047, 1319 }; int n = std::min(4, (int)(t * 9)); return 0.24f * Onda(0, m[n] * t) * Inviluppo(t, 0, 0.8f, 0.8f); });
        CreaSuono("ps_salto", 0.18f, [](float t) { return 0.16f * Onda(0, (300 + 900 * t) * t) * Inviluppo(t, 0, 0.18f, 2); });
        CreaSuono("ps_carro", 1.0f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.1f; return 0.2f * Onda(3, 1200 * t) * Inviluppo(t, 0.05f, 0.6f, 1) + 0.3f * y * Inviluppo(t, 0, 1.0f, 1); });
        CreaSuono("ps_cannone", 0.6f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.15f; return (1.0f * y + 0.4f * Onda(3, 80 * std::exp(-t * 3) * t)) * Inviluppo(t, 0, 0.6f, 2.2f); });
        CreaSuono("ps_allarme", 2.0f, [](float t) { float f = std::fmod(t, 0.5f) < 0.25f ? 587 : 784; return 0.2f * Onda(2, f * t) * Inviluppo(t, 0.02f, 2.0f, 0.4f); });
        CreaSuono("ps_missione", 2.4f, [](float t) {
            static const float m[] = { 523, 523, 659, 784, 659, 784, 1047, 1047 };
            int n = std::min(7, (int)(t * 4)); float tt = t - n * 0.25f;
            return 0.22f * (Onda(2, m[n] * t) * 0.6f + Onda(1, m[n] * 0.5f * t)) * Inviluppo(tt, 0.005f, n == 7 ? 1.2f : 0.25f, 1.2f); });
        CreaSuono("ps_vita", 1.0f, [](float t) { static const float m[] = { 1047, 1319, 1568, 2093, 1568, 2093 }; int n = std::min(5, (int)(t * 7)); return 0.2f * Onda(0, m[n] * t) * Inviluppo(t, 0, 1.0f, 0.6f); });
        CreaSuono("ps_nemico", 0.16f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.4f; return (0.35f * y + 0.2f * Onda(0, (600 - 2000 * t) * t)) * Inviluppo(t, 0, 0.16f, 3); });
    }

    // ======================= GRAFICA =======================
    static Spr gEroe[2][6][3][2], gNem[12][7], gOst[5], gCarro[2][2], gBossS[20][2];
    static Spr gCassa, gBarile, gBarileP, gImpalc, gSacchi, gRoccia, gIcona;
    static Texture2D gLon[20], gMed[20], gSuo[20];
    static std::string gFirma, gFirmaAmb[20], gFirmaBoss[20];

    void PreparaGrafica() {
        std::string firma = Gen().dump() + cfg["nemici"].dump();
        if (firma != gFirma) {
            bool prima = gFirma.empty();
            gFirma = firma;
            if (!prima) { for (auto& a : gEroe) for (auto& b : a) for (auto& c : b) for (auto& d : c) Libera(d); for (auto& a : gNem) for (auto& b : a) Libera(b); }
            for (int g = 0; g < 2; g++) {
                std::string n = std::to_string(g + 1);
                Color gi = ColoreDi(Gen(), ("giacca" + n).c_str(), g ? C(0x3e6e4e) : C(0x9a7040));
                Color ca = ColoreDi(Gen(), ("cappello" + n).c_str(), g ? C(0x5a3a20) : C(0x6e4a28));
                Color cz = ColoreDi(Gen(), ("calzoni" + n).c_str(), C(0x55668c));
                for (int l = 0; l < 6; l++) for (int m = 0; m < 3; m++) for (int a = 0; a < 2; a++)
                    gEroe[g][l][m][a] = FaiSpr(64, 70, 22, 66, [&](Pix& c) { Eroe(c, 22, 66, gi, ca, cz, l, m, a == 1); });
            }
            for (int k = 0; k < std::min(12, NNem()); k++) {
                const json& e = NemJ(k);
                int comp = IntDi(e, "comportamento", 0);
                Color gi = ColoreDi(e, "giacca", C(0x9a3c2c)), ca = ColoreDi(e, "cappello", C(0x2c2228));
                for (int f = 0; f < 7; f++) {
                    if (comp <= 2) gNem[k][f] = FaiSpr(64, 60, 32, 57, [&](Pix& c) { Bandito(c, 32, 57, gi, ca, f); });
                    else if (comp == 3 && f < 5) gNem[k][f] = FaiSpr(64, 64, 34, 62, [&](Pix& c) { Robot(c, 34, 62, gi, ca, f); });
                    else if (comp == 4 && f < 2) gNem[k][f] = FaiSpr(80, 74, 40, 70, [&](Pix& c) { Cavaliere(c, 40, 70, gi, ca, f); });
                    else if (comp == 5 && f < 2) gNem[k][f] = FaiSpr(76, 40, 38, 18, [&](Pix& c) { Girocottero(c, 38, 18, gi, f); });
                }
            }
            if (prima) {
                for (int f = 0; f < 5; f++) gOst[f] = FaiSpr(40, 58, 20, 55, [&](Pix& c) { Ostaggio(c, 20, 55, f); });
                for (int f = 0; f < 2; f++) for (int p = 0; p < 2; p++) gCarro[f][p] = FaiSpr(70, 50, 30, 47, [&](Pix& c) { Bisonte(c, 30, 47, f, p == 1, C(0x6e4a28)); });
                gCassa = FaiSpr(20, 18, 2, 15, [](Pix& c) { Cassa(c, 2, 2); });
                gBarile = FaiSpr(16, 20, 3, 18, [](Pix& c) { Barile(c, 3, 3, false); });
                gBarileP = FaiSpr(16, 20, 3, 18, [](Pix& c) { Barile(c, 3, 3, true); });
                gImpalc = FaiSpr(64, 86, 32, 84, [](Pix& c) { Impalcatura(c, 32, 84); }, false);
                gSacchi = FaiSpr(40, 20, 20, 18, [](Pix& c) { Sacchi(c, 20, 18); });
                gRoccia = FaiSpr(24, 14, 12, 11, [](Pix& c) { Roccia(c, 12, 7, 4); });
                gIcona = FaiSpr(18, 16, 9, 14, [](Pix& c) { Icona(c, 2, 2, C(0x7a6a5a)); });
            }
        }
    }
    void PreparaAmbiente(int k) {
        if (k < 0 || k >= 20) return;
        std::string f = Amb(k).dump();
        if (f == gFirmaAmb[k] && gLon[k].id) return;
        if (gLon[k].id) { UnloadTexture(gLon[k]); UnloadTexture(gMed[k]); UnloadTexture(gSuo[k]); }
        gFirmaAmb[k] = f;
        const json& a = Amb(k);
        Pix lon(LS, LH), med(LS, LH), suo(LW, LH - Y_SUOLO);
        Ambiente(std::max(0, std::min(9, IntDi(a, "scena", k % 10))), ColoreDi(a, "cielo", C(0x96a8aa)), ColoreDi(a, "orizzonte", C(0xe2c496)),
                 ColoreDi(a, "rocce", C(0xb8967c)), ColoreDi(a, "terreno", C(0xba8c60)), lon, med, suo);
        lon.Grana(); med.Grana(); suo.Grana();
        gLon[k] = lon.Tex(); gMed[k] = med.Tex(); gSuo[k] = suo.Tex();
    }
    void PreparaBoss(int k) {
        if (k < 0 || k >= 20) return;
        const json& b = BossJ(k);
        int asp = std::max(0, std::min(9, IntDi(b, "aspetto", k % 10)));
        Color col = ColoreDi(b, "colore", C(0x5e5e54));
        std::string f = std::to_string(asp) + Esa(col);
        if (f == gFirmaBoss[k] && gBossS[k][0].t.id) return;
        Libera(gBossS[k][0]); Libera(gBossS[k][1]);
        gFirmaBoss[k] = f;
        int W = MISURE_BOSS[asp].w + 80, H = MISURE_BOSS[asp].h + 40;
        for (int fr = 0; fr < 2; fr++) gBossS[k][fr] = FaiSpr(W, H, W / 2, H - 2, [&](Pix& c) { DisegnaBoss(c, asp, col, fr); });
    }
    void Prepara() override { PreparaSuoni(); PreparaGrafica(); }

    // ======================= STATO =======================
    Caso caso;
    uint64_t seme = 1, casoInizio = 0;
    Regista regista;
    int missione = 0, vite = 3, bombe = 10, prossimaVita = 20000, record = 0;
    int arma = 0, munizioni = 0;
    bool inCarro = false; float vitaCarro = 0;
    float ex = 60, ey = TERRA, evx = 0, evy = 0; bool aTerra = true; int dir = 1, mira = 0;
    float tSparo = 0, tInv = 0, tMorto = 0, tAnim = 0, tLancio = 0, tCannone = 0, tDiscesa = 0;
    float camX = 0, lunghezza = 2000; bool alBoss = false, arena = false;
    float timer = 60, tSecondo = 0, tFine = -1, tScritta = 0, tScosse = 0, tMissione = 0, tLampo = 0;
    std::string scritta, sotto;
    int ostaggiLiberati = 0, ostaggiTotali = 0;
    std::vector<Ogg> oggetti;
    std::vector<Piatt> piatt;
    std::vector<Prig> prigionieri;
    std::vector<Nemico> nemici;
    std::vector<Proiettile> colpi;
    std::vector<Premio> premi;
    std::vector<Boom> boom;
    std::vector<Fumo> fumo;
    std::vector<Arrivo> arrivi;
    std::vector<TestoVolante> testi;
    bool carroLibero = false; float carroX = 0, carroY = TERRA;
    BossP boss;
    float autoT = 0; bool autoFuoco = false;

    // ======================= PARTITA =======================
    void Nuova(uint64_t s, int g) override {
        seme = s; giocatore = g;
        caso.Semina(s ^ 0x9071E5ull);
        regista = Regista{};
        missione = 0; vite = IntDi(Gen(), "vite", 3); bombe = IntDi(Gen(), "bombe", 10); prossimaVita = IntDi(Gen(), "vitaExtra", 20000);
        punteggio = 0; tempo = 0; finito = vinto = false; puntiDiControllo = 0;
        arma = 0; munizioni = 0; inCarro = false; vitaCarro = 0; ostaggiLiberati = 0;
        LeggiRecord();
        IniziaMissione(false);
    }
    void LeggiRecord() { std::ifstream f(FileAccanto(".record")); record = 0; if (f) f >> record; }
    void ScriviRecord() { if (punteggio <= record) return; record = punteggio; std::ofstream f(FileAccanto(".record")); if (f) f << record; }
    void Checkpoint() { casoInizio = caso.s; puntiDiControllo++; }
    // le prime missioni sono piu' tranquille, le ultime piu' cattive
    float Crescita() const { return 0.72f + 0.035f * std::min(missione, 12); }
    int IndBoss() const { int b = IntDi(Mis(), "boss", -1); return b >= 0 && b < (int)cfg["boss"].size() ? b : -1; }
    float VitaCarroMax() const { return NumDi(Armi(), "carro", 6); }

    void AggiungiCasse(float x, int n) {
        auto Una = [&](float cx, float cy) {
            oggetti.push_back({ 0, cx, cy, 2 });
            piatt.push_back({ cx - 1, cx + 16, cy - 13, true, (int)oggetti.size() - 1 });
        };
        Una(x, TERRA);
        if (n >= 2) Una(x + 15, TERRA);
        if (n >= 3) Una(x + 7, TERRA - 13);
    }
    void AggiungiImpalcatura(float x) {
        for (auto& o : oggetti) if (o.tipo == 3 && std::fabs(o.x - x) < 70) return;
        oggetti.push_back({ 3, x, TERRA, 999 });
        piatt.push_back({ x - 28, x + 28, TERRA - 80, false, -1 });
    }
    void IniziaMissione(bool alBossOra) {
        Checkpoint();
        alBoss = alBossOra; arena = false;
        tMissione = 0; tFine = -1; timer = (float)IntDi(Mis(), "tempo", 60); tSecondo = 0;
        oggetti.clear(); piatt.clear(); prigionieri.clear(); nemici.clear(); colpi.clear(); premi.clear(); boom.clear(); fumo.clear(); arrivi.clear(); testi.clear();
        boss = BossP{};
        if (!alBossOra) { inCarro = false; vitaCarro = 0; ostaggiLiberati = 0; }
        int sch = std::max(3, std::min(40, IntDi(Mis(), "schermi", 10)));
        lunghezza = (float)sch * LW;
        float dens = std::max(0.2f, NumDi(Mis(), "densita", 1));
        std::vector<int> tipi;
        for (const char* k : { "nemico1", "nemico2", "nemico3" }) { int t = IntDi(Mis(), k, -1); if (t >= 0 && t < NNem() && t < 12) tipi.push_back(t); }
        float x = 300;
        while (x < lunghezza - 520) {
            int r = caso.I(0, 9);
            if (r < 3) AggiungiCasse(x + caso.F(20, 60), caso.I(1, 3));
            else if (r < 5) AggiungiImpalcatura(x + 30);
            else if (r < 6) oggetti.push_back({ caso.P(0.4f) ? 2 : 1, x + caso.F(0, 50), TERRA, 3 });
            else if (r < 7) oggetti.push_back({ 4, x + caso.F(0, 50), TERRA, 999 });
            else oggetti.push_back({ 5, x + caso.F(0, 60), TERRA + caso.F(4, 24), 999 });
            if (!tipi.empty()) {
                int t = tipi[caso.I(0, (int)tipi.size() - 1)];
                int comp = IntDi(NemJ(t), "comportamento", 0);
                int q = comp >= 3 ? 1 : std::max(1, std::min(5, (int)std::round(caso.F(1, 2.6f) * dens)));
                Arrivo a{ x + caso.F(0, 60), t, q };
                if (comp == 2) { AggiungiImpalcatura(a.x); a.alto = true; a.quanti = 1; }
                arrivi.push_back(a);
            }
            x += caso.F(130, 220) / std::max(0.5f, dens);
        }
        int no = std::max(0, std::min(20, IntDi(Mis(), "ostaggi", 4)));
        ostaggiTotali = no;
        for (int i = 0; i < no; i++) prigionieri.push_back({ 220 + (lunghezza - 720) * (i + 0.5f) / no + caso.F(-40, 40), TERRA });
        carroLibero = Mis().value("carro", false) && !inCarro;
        carroX = 520 + caso.F(0, 120); carroY = TERRA;
        if (!alBossOra) {
            camX = 0; ex = 50; ey = -20; evx = evy = 0; aTerra = false; dir = 1; tInv = 2; tMorto = 0;
            scritta = "MISSIONE " + std::to_string(missione + 1); sotto = TestoDi(Mis(), "nome", ""); tScritta = 3.2f;
            SuonaMusica(BranoDi(TestoDi(Mis(), "musica", "Polvere - Canyon")));
        } else InizioArena();
    }
    void InizioArena() {
        arena = true;
        camX = lunghezza - LW;
        if (ex < camX + 20 || !alBoss) { ex = camX + 60; ey = TERRA; evx = evy = 0; aTerra = true; }
        tMorto = 0; tInv = 1.5f;
        timer = (float)IntDi(Mis(), "tempo", 60);
        int bi = IndBoss();
        if (bi < 0) return;
        const json& bj = BossJ(bi);
        boss = BossP{};
        boss.vivo = true; boss.indice = bi;
        boss.aspetto = std::max(0, std::min(9, IntDi(bj, "aspetto", bi % 10)));
        boss.comp = std::max(0, std::min(4, IntDi(bj, "comportamento", 0)));
        boss.hpMax = boss.hp = NumDi(bj, "vita", 150) * (0.85f + 0.15f * regista.sfida);
        float W = (float)MISURE_BOSS[boss.aspetto].w;
        boss.x = lunghezza + W / 2 + 10;
        boss.y = boss.comp == 1 ? 104 : TERRA;
        boss.bersX = boss.comp == 4 ? lunghezza - W * 0.42f : boss.comp == 1 ? lunghezza - LW / 2.0f : lunghezza - W / 2 - 30;
        boss.ingresso = 2.6f;
        if (boss.comp == 3) { boss.x = boss.bersX; boss.sotto = true; boss.yOff = (float)MISURE_BOSS[boss.aspetto].h; boss.ingresso = 0.5f; }
        Suona("ps_allarme");
        scritta = "ATTENZIONE!"; sotto = TestoDi(bj, "nome", NOMI_ASPETTI[boss.aspetto]); tScritta = 3;
        SuonaMusica(BranoDi(TestoDi(bj, "musica", "Polvere - Boss")));
    }
    void InizioBoss() { Checkpoint(); alBoss = true; InizioArena(); }
    void MissioneCompiuta() {
        int bonus = ostaggiLiberati * 1000;
        punteggio += bonus;
        bool ultima = missione + 1 >= NMis();
        scritta = ultima ? TestoDi(Gen(), "fine", "VITTORIA!") : "MISSIONE COMPIUTA!";
        sotto = "OSTAGGI LIBERATI " + std::to_string(ostaggiLiberati) + "/" + std::to_string(ostaggiTotali) + "   BONUS " + std::to_string(bonus);
        tScritta = 4.5f; tFine = 4.8f;
        Suona("ps_missione");
        regista.TappaFinita(tMissione, lunghezza / LW * 9);
        for (auto& c : colpi) if (c.nemico) c.vita = 0;
        for (auto& n : nemici) if (n.stato == 0) { n.stato = 1; n.t = 0; n.vy = -80; }
        ScriviRecord();
    }

    // --- terreno e piattaforme ---
    bool PiattViva(const Piatt& p) const { return p.ogg < 0 || (p.ogg < (int)oggetti.size() && oggetti[p.ogg].vivo); }
    float Atterra(float x, float yPrima, float yOra, bool attraversa) const {
        float best = yOra >= TERRA ? TERRA : -1;
        for (auto& p : piatt) {
            if (!PiattViva(p) || x < p.x0 || x > p.x1 || (attraversa && !p.solida)) continue;
            if (yPrima <= p.y + 0.5f && yOra >= p.y && (best < 0 || p.y < best)) best = p.y;
        }
        return best;
    }
    bool SuPiattaformaSottile(float x, float y) const {
        for (auto& p : piatt) if (PiattViva(p) && !p.solida && x >= p.x0 && x <= p.x1 && std::fabs(y - p.y) < 0.6f) return true;
        return false;
    }

    // --- effetti ---
    void Esplosione(float x, float y, float r, bool grande = false) {
        boom.push_back({ x, y, 0, grande ? 1.1f : 0.6f + r / 80, r, (unsigned)(x * 13 + y * 7 + tempo * 1000) });
        tScosse = std::max(tScosse, grande ? 0.5f : 0.22f);
        Suona(grande ? "ps_boom" : "ps_esplosione");
    }
    void Testo(float x, float y, const std::string& t, Color c = Color{ 255, 230, 140, 255 }) { testi.push_back({ x, y, t, 1.0f, c }); }
    void Punti(int p, float x, float y) { punteggio += p; Testo(x, y, std::to_string(p)); }

    // --- l'eroe ---
    Rectangle ScatolaEroe() const { return inCarro ? Rectangle{ ex - 22, ey - 30, 44, 30 } : Rectangle{ ex - 6, ey - 36, 12, 36 }; }
    void ColpitoEroe() {
        if (tMorto > 0 || tInv > 0 || tFine >= 0) return;
        if (inCarro) {
            vitaCarro -= 1; tInv = 0.5f; Suona("ps_metallo");
            if (vitaCarro <= 0) { Esplosione(ex, ey - 16, 30, true); inCarro = false; evy = -220; aTerra = false; tInv = 2; }
            return;
        }
        tMorto = 1.6f; evy = -120; Suona("ps_morte");
        regista.VitaPersa();
        vite--;
        for (int i = 0; i < 14; i++) fumo.push_back({ ex, ey - 20, Vis(-60, 60), Vis(-90, 10), 0.6f, 0.6f, 1, Color{ 255, 220, 120, 255 } });
    }
    void Rinasci() {
        if (vite <= 0) { finito = true; vinto = false; ScriviRecord(); return; }
        ex = std::max(camX + 50, std::min(ex, camX + LW - 60)); ey = TERRA - 70; evy = 0; evx = 0; aTerra = false;
        tInv = 2.5f; arma = 0; munizioni = 0; bombe = std::max(bombe, IntDi(Gen(), "bombe", 10));
        timer = (float)IntDi(Mis(), "tempo", 60);
    }
    void Colpo(float x, float y, float vx, float vy, int tipo, float vita, float danno, bool nemico, float r = 2) {
        colpi.push_back({ x, y, vx, vy, tipo, vita, danno, nemico, r });
    }
    void Spara(const Pad& pad, float dt) {
        tSparo -= dt; tCannone -= dt; tLancio -= dt;
        float sx = (float)dir;
        if (inCarro) {
            if (pad.fuoco && tSparo <= 0) {
                tSparo = 0.09f;
                if (mira == 1) Colpo(ex + sx * 2, ey - 44, Vis(-8, 8), -330, 1, 1.2f, 1, false);
                else Colpo(ex + sx * 32, ey - 30 + Vis(-2, 2), sx * 330, 0, 1, 1.2f, 1, false);
                Suona("ps_mitra", 0.7f); regista.Sparo();
            }
            if (pad.bombaP && tCannone <= 0) {
                tCannone = 0.7f; Colpo(ex + sx * 32, ey - 30, sx * 230, -70, 5, 3, 10, false, 3);
                Suona("ps_cannone"); tScosse = std::max(tScosse, 0.15f);
                fumo.push_back({ ex + sx * 34, ey - 30, sx * 20, -10, 0.4f, 0.4f, 4, Color{ 230, 230, 230, 200 } });
            }
            return;
        }
        if (arma == 0 && pad.fuocoP) tSparo = std::min(tSparo, 0.0f);
        bool grande = arma > 0;
        if (pad.fuoco && tSparo <= 0 && mira == 0) {   // a bruciapelo: colpo col calcio dell'arma
            for (auto& n : nemici)
                if (n.stato == 0 && n.comp <= 3 && (n.x - ex) * sx > -4 && (n.x - ex) * sx < 24 && std::fabs(n.y - ey) < 20) {
                    DanneggiaNemico(n, 3); tSparo = 0.3f; Suona("ps_metallo", 0.5f, 0.6f);
                    fumo.push_back({ n.x, n.y - 30, sx * 30, -30, 0.3f, 0.3f, 1, Color{ 255, 240, 200, 255 } });
                    break;
                }
        }
        if (pad.fuoco && tSparo <= 0) {
            const float cad[5] = { 0.22f, 0.075f, 0.5f, 0.05f, 0.35f };
            tSparo = cad[arma];
            Vector2 cn = Canna(mira, grande);
            float x0 = ex + cn.x * sx, y0 = ey + cn.y;
            float dx = mira == 0 ? sx : 0, dy = mira == 1 ? -1.0f : mira == 2 ? 1.0f : 0;
            switch (arma) {
                case 0: Colpo(x0, y0, dx * 300, dy * 300, 0, 1.2f, 1, false); Suona("ps_pistola", 0.8f); break;
                case 1: { float j = Vis(-2.5f, 2.5f); Colpo(x0 + (dy != 0 ? j : 0), y0 + (dx != 0 ? j : 0), dx * 330, dy * 330, 1, 1.2f, 1, false); Suona("ps_mitra", 0.8f); break; }
                case 2: for (int k = -2; k <= 2; k++) { float a = std::atan2(dy, dx) + k * 0.12f; Colpo(x0, y0, std::cos(a) * 280, std::sin(a) * 280, 2, 0.22f, 2.5f, false); } Suona("ps_pallettoni"); break;
                case 3: { float a = std::atan2(dy, dx) + Vis(-0.12f, 0.12f); Colpo(x0, y0, std::cos(a) * 170, std::sin(a) * 170, 3, 0.4f, 0.6f, false, 3); if ((int)(tempo * 20) % 3 == 0) Suona("ps_fiamme", 0.7f); break; }
                case 4: Colpo(x0, y0, dx * 120, dy * 120, 4, 2.5f, 6, false, 2); Suona("ps_razzo"); break;
            }
            regista.Sparo();
            if (arma > 0 && --munizioni <= 0) { arma = 0; munizioni = 0; Testo(ex, ey - 60, "PISTOLA", Color{ 200, 220, 255, 255 }); }
        }
        if (pad.bombaP && bombe > 0 && tLancio <= 0) {
            bombe--; tLancio = 0.3f;
            Colpo(ex + sx * 4, ey - 36, sx * 110 + evx * 0.5f, -170, 6, 4, 8, false, 2);
            Suona("ps_lancio");
        }
    }
    void AggiornaEroe(float dt, const Pad& pad) {
        if (tMorto > 0) {
            tMorto -= dt;
            evy += 400 * dt; ey = std::min(TERRA, ey + evy * dt);
            if (tMorto <= 0) Rinasci();
            return;
        }
        tInv = std::max(0.0f, tInv - dt);
        float vel = inCarro ? 58 : NumDi(Gen(), "velocita", 72);
        evx = pad.x * vel;
        if (std::fabs(pad.x) > 0.2f) dir = pad.x > 0 ? 1 : -1;
        mira = (pad.su || pad.y < -0.5f) ? 1 : ((pad.giu || pad.y > 0.5f) && !aTerra && !inCarro ? 2 : 0);
        if (pad.saltoP) {
            if (inCarro && (pad.giu || pad.y > 0.5f)) {
                inCarro = false; carroLibero = true; carroX = ex; carroY = ey; evy = -200; aTerra = false; tInv = std::max(tInv, 0.5f);
            } else if (aTerra) {
                if ((pad.giu || pad.y > 0.5f) && !inCarro && SuPiattaformaSottile(ex, ey)) { tDiscesa = 0.25f; ey += 1; }
                else { evy = -std::sqrt(2 * 520 * (inCarro ? 30 : 46) * NumDi(Gen(), "salto", 1)); aTerra = false; Suona("ps_salto", 0.6f); }
            }
        }
        tDiscesa -= dt;
        float xp = ex;
        ex += evx * dt;
        float mezzo = inCarro ? 20 : 6;
        for (auto& p : piatt)
            if (p.solida && PiattViva(p) && ey > p.y + 2 && ex + mezzo > p.x0 && ex - mezzo < p.x1 && !(xp + mezzo > p.x0 && xp - mezzo < p.x1)) ex = xp;
        float minX = camX + 8, maxX = std::min(camX + LW - 8, lunghezza - 8);
        ex = std::max(minX, std::min(maxX, ex));
        evy += 520 * dt;
        float yp = ey;
        ey += evy * dt;
        aTerra = false;
        float t = Atterra(ex, yp, ey, tDiscesa > 0);
        if (t >= 0 && evy >= 0) { ey = t; evy = 0; aTerra = true; }
        if (std::fabs(evx) > 1) tAnim += dt;
        if (!inCarro && carroLibero && std::fabs(ex - carroX) < 18 && std::fabs(ey - carroY) < 24 && tInv < 0.4f) {
            inCarro = true; vitaCarro = VitaCarroMax(); carroLibero = false; Suona("ps_carro");
            Testo(ex, ey - 50, "BISONTE!", Color{ 255, 200, 90, 255 });
        }
        if (inCarro && aTerra && std::fabs(evx) > 1 && (int)(tempo * 10) % 3 == 0)
            fumo.push_back({ ex - dir * 20, ey - 42, Vis(-6, 6), -24, 0.8f, 0.8f, 2, Color{ 200, 200, 200, 150 } });
        Spara(pad, dt);
    }

    // velocita' per arrivare in (tx, ty) in T secondi con gravita' g
    static Vector2 Lancio(float x0, float y0, float tx, float ty, float T, float g) { return { (tx - x0) / T, (ty - y0) / T - 0.5f * g * T }; }

    // --- nemici ---
    Rectangle ScatolaNemico(const Nemico& n) const {
        switch (n.comp) {
            case 3: return { n.x - 11, n.y - 50, 22, 50 };
            case 4: return { n.x - 22, n.y - 48, 44, 48 };
            case 5: return { n.x - 20, n.y - 9, 40, 18 };
            default: return { n.x - 8, n.y - 46, 16, 46 };
        }
    }
    int Vivi() const { int k = 0; for (auto& n : nemici) if (n.stato == 0) k++; return k; }
    void NuovoNemico(int tipo, float x, bool alto, int i) {
        const json& e = NemJ(tipo);
        Nemico n;
        n.tipo = tipo; n.comp = std::max(0, std::min(5, IntDi(e, "comportamento", 0)));
        n.hp = NumDi(e, "vita", 1);
        n.tFuoco = caso.F(0.8f, 1.8f);
        n.x = x + i * 18; n.y = TERRA;
        if (n.comp == 5) { n.x = camX + LW + 30 + i * 40; n.y = n.baseY = caso.F(34, 56); }
        else if (n.comp == 4) { n.x = camX + LW + 30 + i * 60; n.vx = -150 * NumDi(e, "velocita", 1); }
        else if (alto) { n.y = TERRA - 80; n.x = x; }
        else if (n.x < camX + LW + 10) { if (caso.P(0.18f)) { n.x = camX - 14 - i * 18; n.dir = 1; } else n.x = camX + LW + 12 + i * 18; }
        nemici.push_back(n);
    }
    void DanneggiaNemico(Nemico& n, float d) {
        if (n.stato != 0) return;
        n.hp -= d; n.lampo = 0.08f;
        regista.Bersaglio();
        if (n.comp == 3 || n.comp == 5) Suona("ps_metallo", 0.6f);
        if (n.hp > 0) return;
        n.stato = 1; n.t = 0; n.vy = -90; n.vx = -n.dir * 40;
        Punti(IntDi(NemJ(n.tipo), "punti", 100), n.x, n.y - 50);
        Suona("ps_bandito", 0.8f, Vis(0.85f, 1.2f));
        if (n.comp == 5 || n.comp == 3 || n.comp == 4) {
            Esplosione(n.x, n.y - (n.comp == 5 ? 0 : 20), 20, n.comp == 5);
            if (n.comp == 5) { n.vy = 20; n.vx = -30; }
        }
        // a volte lascia qualcosa
        if (caso.P(n.comp >= 3 ? 0.5f : 0.06f)) premi.push_back({ n.x, n.y - 20, -90, caso.P(0.5f) ? 5 : 6 });
    }
    void AggiornaNemici(float dt) {
        float sfida = regista.sfida;
        for (auto& n : nemici) {
            const json& e = NemJ(n.tipo);
            float vel = NumDi(e, "velocita", 1), fuoco = NumDi(e, "fuoco", 1);
            n.lampo -= dt; n.t += dt;
            if (n.stato == 1) {   // colpito: vola indietro e cade
                n.vy += 400 * dt; n.x += n.vx * dt; n.y += n.vy * dt;
                if (n.comp == 5 && n.y > TERRA - 8) { n.y = TERRA - 8; if (n.t < 5) { Esplosione(n.x, n.y, 26, true); n.t = 5; } }
                else if (n.y >= n.yPiatt && n.comp != 5) { n.y = n.yPiatt; n.vx *= 0.8f; }
                continue;
            }
            float dx = ex - n.x;
            if (n.comp != 4) n.dir = dx < 0 ? -1 : 1;
            bool visibile = n.x > camX - 10 && n.x < camX + LW + 10;
            if (visibile) n.tFuoco -= dt * fuoco * (0.6f + 0.4f * sfida) * Crescita();
            float sx = (float)n.dir;
            switch (n.comp) {
                case 0:   // pistolero
                    if (n.stato == 2) { n.x -= sx * 50 * vel * dt; if (n.t > 0.6f) n.stato = 0; break; }
                    if (std::fabs(dx) > 70 && n.t > 0.3f) { n.x += sx * 34 * vel * dt; n.tAnim += dt; }
                    if (n.tFuoco <= 0 && std::fabs(dx) < 230 && visibile && tMorto <= 0) {
                        n.tFuoco = caso.F(1.4f, 2.6f); n.t = 0;
                        float vy = std::fabs(ey - n.y) > 20 ? (ey - 18 - (n.y - 25)) / std::max(0.4f, std::fabs(dx) / 110) : 0;
                        Colpo(n.x + sx * 19, n.y - 25, sx * 110 * (0.85f + 0.15f * sfida), vy, 10, 3, 1, true, 1);
                        Suona("ps_nemico", 0.5f);
                    }
                    break;
                case 1: {   // dinamitardo
                    float d = std::fabs(dx);
                    if (d < 90) { n.x -= sx * 30 * vel * dt; n.tAnim += dt; }
                    else if (d > 150) { n.x += sx * 30 * vel * dt; n.tAnim += dt; }
                    if (n.tFuoco <= 0 && d < 230 && visibile && tMorto <= 0) {
                        n.tFuoco = caso.F(2.0f, 3.2f); n.t = 0;
                        Vector2 v = Lancio(n.x - sx * 6, n.y - 50, ex + caso.F(-28, 28) + evx * 0.6f, ey - 8, 1.1f, 360);
                        Colpo(n.x - sx * 6, n.y - 50, Limita(v.x, -170, 170), v.y, 11, 6, 1, true, 2);
                        Suona("ps_lancio", 0.6f);
                    }
                    break;
                }
                case 2:   // cecchino
                    if (n.tFuoco <= 0 && visibile && tMorto <= 0) {
                        n.tFuoco = caso.F(2.2f, 3.4f); n.t = 0;
                        float ax = ex - n.x, ay = ey - 18 - (n.y - 25), l = std::max(1.0f, std::sqrt(ax * ax + ay * ay));
                        Colpo(n.x + sx * 19, n.y - 25, ax / l * 115 * (0.85f + 0.15f * sfida), ay / l * 115 * (0.85f + 0.15f * sfida), 10, 3, 1, true, 1);
                        Suona("ps_nemico", 0.5f);
                    }
                    break;
                case 3:   // robot a vapore
                    if (std::fabs(dx) > 40) { n.x += sx * 20 * vel * dt; n.tAnim += dt; }
                    if (n.tFuoco <= 0 && std::fabs(dx) < 60 && tMorto <= 0) {
                        n.tFuoco = 2.2f; n.t = 0;
                        for (int k = 0; k < 4; k++) Colpo(n.x + sx * 24, n.y - 25 + Vis(-3, 3), sx * (60 + k * 22), Vis(-10, 10), 14, 0.55f, 1, true, 3);
                    }
                    break;
                case 4:   // cavaliere meccanico
                    n.x += n.vx * dt; n.dir = n.vx < 0 ? -1 : 1; n.tAnim += dt;
                    if (n.tFuoco <= 0 && std::fabs(dx) < 140 && visibile) {
                        n.tFuoco = 99;
                        Colpo(n.x + n.dir * 20, n.y - 44, n.dir * 150, 0, 10, 3, 1, true, 1);
                        Suona("ps_nemico", 0.5f);
                    }
                    if (CheckCollisionRecs(ScatolaNemico(n), ScatolaEroe())) ColpitoEroe();
                    break;
                case 5: {   // girocottero
                    float bx = ex + 30 * std::sin(n.t * 0.8f);
                    n.x += Limita(bx - n.x, -1, 1) * 50 * vel * dt;
                    n.y = n.baseY + 4 * std::sin(n.t * 2);
                    n.tAnim += dt;
                    if (n.tFuoco <= 0 && std::fabs(dx) < 50 && visibile) { n.tFuoco = caso.F(1.6f, 2.6f); Colpo(n.x, n.y + 10, 0, 20, 12, 6, 1, true, 3); }
                    break;
                }
            }
            if (n.comp <= 3) {   // gravita' (e piattaforme)
                n.vy += 520 * dt;
                float yp = n.y; n.y += n.vy * dt;
                float t = Atterra(n.x, yp, n.y, false);
                if (t >= 0 && n.vy >= 0) { n.y = t; n.vy = 0; n.yPiatt = t; }
                if (n.y > TERRA) n.y = TERRA;
                n.x = std::min(n.x, lunghezza - 6);
            }
            // fugge davanti alla dinamite che sta per scoppiare
            if (n.comp <= 1 && n.stato == 0)
                for (auto& c : colpi) if (!c.nemico && c.tipo == 6 && std::fabs(c.x - n.x) < 36 && c.vy > 0) { n.stato = 2; n.t = 0; break; }
        }
        nemici.erase(std::remove_if(nemici.begin(), nemici.end(), [&](const Nemico& n) {
            return (n.stato == 1 && n.t > 1.1f && n.t < 5) || n.t > 5.6f || n.x < camX - 70 || n.x > camX + LW + 260; }), nemici.end());
    }

    // --- colpi ---
    void Scoppia(Proiettile& c) {
        c.vita = 0;
        float r = c.tipo == 5 ? 28 : c.tipo == 6 ? 24 : c.tipo == 4 ? 16 : 20;
        Esplosione(c.x, c.y, r, c.tipo == 5);
        if (c.nemico) {
            Rectangle h = ScatolaEroe();
            float cx = h.x + h.width / 2, cy = h.y + h.height / 2;
            if ((cx - c.x) * (cx - c.x) + (cy - c.y) * (cy - c.y) < (r + 6) * (r + 6)) ColpitoEroe();
            return;
        }
        for (auto& n : nemici) {
            Rectangle b = ScatolaNemico(n);
            float cx = b.x + b.width / 2, cy = b.y + b.height / 2;
            if ((cx - c.x) * (cx - c.x) + (cy - c.y) * (cy - c.y) < (r + b.width / 2) * (r + b.width / 2)) DanneggiaNemico(n, c.danno);
        }
        if (boss.vivo) { Rectangle b = ScatolaBoss(); if (CheckCollisionCircleRec({ c.x, c.y }, r, b)) ColpisciBoss(c.danno, c.x, c.y); }
        for (int i = 0; i < (int)oggetti.size(); i++)
            if (oggetti[i].vivo && oggetti[i].tipo <= 2 && std::fabs(oggetti[i].x + 7 - c.x) < r + 8 && std::fabs(oggetti[i].y - 7 - c.y) < r + 8) RompiOggetto(i);
        for (auto& p : prigionieri) if (p.stato == 0 && std::fabs(p.x - c.x) < r + 6 && std::fabs(p.y - 20 - c.y) < r + 20) LiberaOstaggio(p);
    }
    void RompiOggetto(int i) {
        Ogg& o = oggetti[i];
        if (!o.vivo) return;
        o.vivo = false;
        if (o.tipo == 0) {
            for (int k = 0; k < 8; k++) fumo.push_back({ o.x + 7, o.y - 7, Vis(-70, 70), Vis(-120, -20), 0.6f, 0.6f, 1, Color{ 160, 112, 60, 255 } });
            Suona("ps_metallo", 0.4f, 0.5f);
            if (caso.P(0.45f)) { float r = caso.F(); premi.push_back({ o.x + 7, o.y - 10, -90, r < 0.2f ? caso.I(1, 4) : r < 0.6f ? 5 : 6 }); }
            for (int j = 0; j < (int)oggetti.size(); j++)   // la cassa sopra cade con lei
                if (j != i && oggetti[j].vivo && oggetti[j].tipo == 0 && std::fabs(oggetti[j].x - o.x - 7) < 9 && oggetti[j].y < o.y - 5) RompiOggetto(j);
        } else if (o.tipo == 2) {
            Proiettile p{ o.x + 5, o.y - 8, 0, 0, 5, 0, 10, false, 2 };
            Scoppia(p);
        } else {
            for (int k = 0; k < 6; k++) fumo.push_back({ o.x + 5, o.y - 7, Vis(-60, 60), Vis(-100, -20), 0.5f, 0.5f, 1, Color{ 138, 90, 42, 255 } });
            Suona("ps_metallo", 0.4f, 0.6f);
        }
    }
    void AggiornaColpi(float dt) {
        Rectangle he = ScatolaEroe();
        size_t nIniziale = colpi.size();
        for (size_t i = 0; i < nIniziale; i++) {
            Proiettile c = colpi[i];
            if (c.vita <= 0) continue;
            c.t += dt; c.vita -= dt;
            float yp = c.y;
            switch (c.tipo) {
                case 3: case 18: c.vx *= 0.97f; c.vy -= 30 * dt; c.r = std::min(7.0f, c.r + 10 * dt); break;
                case 4: {
                    float best = 1e9f, tx = 0, ty = 0;
                    for (auto& n : nemici) if (n.stato == 0) { float d = std::fabs(n.x - c.x) + std::fabs(n.y - 20 - c.y); if (d < best && (n.x - c.x) * c.vx >= -20) { best = d; tx = n.x; ty = n.y - 20; } }
                    if (boss.vivo && !boss.sotto) { Rectangle b = ScatolaBoss(); float d = std::fabs(b.x + b.width / 2 - c.x); if (d < best) { best = d; tx = b.x + b.width / 2; ty = b.y + b.height / 2; } }
                    float sp = std::min(270.0f, std::sqrt(c.vx * c.vx + c.vy * c.vy) + 300 * dt);
                    float a = std::atan2(c.vy, c.vx);
                    if (best < 1e8f) { float ta = std::atan2(ty - c.y, tx - c.x), da = std::remainder(ta - a, 2 * PIg); a += Limita(da, -4 * dt, 4 * dt); }
                    c.vx = std::cos(a) * sp; c.vy = std::sin(a) * sp;
                    if ((int)(c.t * 30) % 2 == 0) fumo.push_back({ c.x, c.y, Vis(-5, 5), Vis(-5, 5), 0.35f, 0.35f, 1, Color{ 210, 210, 210, 160 } });
                    break;
                }
                case 5: case 6: case 11: case 13: case 15: c.vy += 360 * dt; break;
                case 12: c.vy += 300 * dt; break;
                case 14: c.vx *= 0.93f; c.r += 8 * dt; break;
                case 16: break;
                case 17: break;
            }
            c.x += c.vx * dt; c.y += c.vy * dt;
            bool esplosivo = c.tipo == 4 || c.tipo == 5 || c.tipo == 6 || c.tipo == 11 || c.tipo == 12 || c.tipo == 13;
            // contro terreno e piattaforme
            if (c.tipo == 5 || c.tipo == 6 || c.tipo == 11 || c.tipo == 12 || c.tipo == 13 || c.tipo == 15) {
                float t = Atterra(c.x, yp, c.y, false);
                if (t >= 0 && c.vy > 0) {
                    c.y = t;
                    if (c.tipo == 15) { c.vita = 0; for (int k = 0; k < 5; k++) fumo.push_back({ c.x, c.y - 2, Vis(-50, 50), Vis(-90, -20), 0.5f, 0.5f, 1, Color{ 138, 120, 102, 255 } }); Suona("ps_metallo", 0.3f, 0.4f); }
                    else Scoppia(c);
                }
            } else if (c.tipo <= 4 && c.y >= TERRA + 1) { c.vita = 0; fumo.push_back({ c.x, TERRA - 1, 0, -20, 0.25f, 0.25f, 1, Color{ 220, 200, 160, 200 } }); }
            if (c.tipo == 16 || c.tipo == 17) c.y = TERRA - (c.tipo == 16 ? 5 : 6);
            if (c.vita <= 0) { colpi[i] = c; continue; }
            if (c.x < camX - 60 || c.x > camX + LW + 60 || c.y > LH + 20 || c.y < -120) { c.vita = 0; colpi[i] = c; continue; }
            if (c.nemico) {
                Rectangle cc = { c.x - c.r, c.y - (c.tipo == 16 ? 8 : c.r), c.r * 2, c.tipo == 16 ? 12 : c.r * 2 };
                if (CheckCollisionRecs(cc, he) && tMorto <= 0) {
                    if (esplosivo) Scoppia(c); else { c.vita = 0; ColpitoEroe(); }
                }
            } else {
                bool preso = false;
                for (auto& n : nemici) {
                    if (n.stato != 0 || !CheckCollisionPointRec({ c.x, c.y }, ScatolaNemico(n))) continue;
                    if (esplosivo) { Scoppia(c); preso = true; break; }
                    DanneggiaNemico(n, c.danno);
                    fumo.push_back({ c.x, c.y, -c.vx * 0.1f, Vis(-30, 0), 0.2f, 0.2f, 1, n.comp == 3 || n.comp == 5 ? Color{ 255, 240, 160, 255 } : Color{ 200, 40, 30, 255 } });
                    c.vita = 0; preso = true; break;
                }
                if (!preso && boss.vivo && CheckCollisionPointRec({ c.x, c.y }, ScatolaBoss())) {
                    if (esplosivo) Scoppia(c);
                    else { ColpisciBoss(c.danno, c.x, c.y); c.vita = 0; }
                    preso = true;
                }
                if (!preso)
                    for (int k = 0; k < (int)oggetti.size(); k++) {
                        Ogg& o = oggetti[k];
                        if (!o.vivo || o.tipo > 2) continue;
                        float w = o.tipo == 0 ? 15 : 10, h = o.tipo == 0 ? 13 : 14;
                        if (c.x >= o.x && c.x <= o.x + w && c.y >= o.y - h && c.y <= o.y) {
                            if (esplosivo) Scoppia(c); else { c.vita = 0; o.hp -= c.danno; if (o.hp <= 0) RompiOggetto(k); }
                            preso = true; break;
                        }
                    }
                if (!preso)
                    for (auto& p : prigionieri)
                        if (p.stato == 0 && std::fabs(c.x - p.x) < 7 && c.y > p.y - 48 && c.y < p.y) { LiberaOstaggio(p); if (!esplosivo) c.vita = 0; break; }
            }
            colpi[i] = c;
        }
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Proiettile& c) { return c.vita <= 0; }), colpi.end());
    }

    // --- ostaggi e premi ---
    void LiberaOstaggio(Prig& p) {
        if (p.stato != 0) return;
        p.stato = 1; p.t = 0;
        float r = caso.F();
        p.dono = r < 0.32f ? 1 : r < 0.5f ? 2 : r < 0.62f ? 3 : r < 0.74f ? 4 : r < 0.92f ? 5 : 6;
        ostaggiLiberati++;
        Punti(100, p.x, p.y - 50);
        Suona("ps_ostaggio");
    }
    void AggiornaOstaggi(float dt) {
        for (auto& p : prigionieri) {
            p.t += dt;
            if (p.stato == 0) {
                if (!inCarro && tMorto <= 0 && std::fabs(ex - p.x) < 14 && std::fabs(ey - p.y) < 24) LiberaOstaggio(p);
            } else if (p.stato == 1) {
                float d = ex - p.x;
                if (std::fabs(d) < 14 || p.t > 3) { p.stato = 2; p.t = 0; premi.push_back({ p.x, p.y - 30, -100, p.dono }); }
                else p.x += (d > 0 ? 1 : -1) * 70 * dt;
            } else if (p.stato == 2) { if (p.t > 0.8f) { p.stato = 3; p.t = 0; } }
            else p.x -= 64 * dt;
        }
        prigionieri.erase(std::remove_if(prigionieri.begin(), prigionieri.end(), [&](const Prig& p) { return p.stato == 3 && p.x < camX - 30; }), prigionieri.end());
    }
    void AggiornaPremi(float dt) {
        for (auto& q : premi) {
            q.t += dt;
            q.vy += 300 * dt;
            float yp = q.y; q.y += q.vy * dt;
            float t = Atterra(q.x, yp, q.y, false);
            if (t >= 0 && q.vy > 0) { q.y = t; q.vy = 0; }
            if (tMorto <= 0 && std::fabs(ex - q.x) < 16 && q.y > ey - 40 && q.y < ey + 4 && q.t > 0.25f) {
                q.t = 99;
                if (q.tipo >= 1 && q.tipo <= 4) {
                    const char* chiavi[5] = { "", "mitragliatrice", "pallettoni", "fiamme", "razzi" };
                    const int pred[5] = { 0, 200, 30, 120, 30 };
                    int quanti = IntDi(Armi(), chiavi[q.tipo], pred[q.tipo]);
                    if (arma == q.tipo) munizioni += quanti; else { arma = q.tipo; munizioni = quanti; }
                    Testo(ex, ey - 56, NOMI_ARMI[q.tipo], Color{ 140, 230, 255, 255 });
                    Suona("ps_arma");
                } else if (q.tipo == 5) { bombe += IntDi(Armi(), "bombeOstaggio", 10); Testo(ex, ey - 56, "DINAMITE", Color{ 255, 150, 120, 255 }); Suona("ps_arma"); }
                else if (q.tipo == 6) { Punti(1000, ex, ey - 56); Suona("ps_ostaggio", 0.7f, 1.3f); }
                else if (q.tipo == 7) { vite++; Suona("ps_vita"); }
            }
        }
        premi.erase(std::remove_if(premi.begin(), premi.end(), [](const Premio& q) { return q.t > 12; }), premi.end());
    }

    // --- il boss ---
    Rectangle ScatolaBoss() const {
        float W = (float)MISURE_BOSS[boss.aspetto].w, H = (float)MISURE_BOSS[boss.aspetto].h;
        return { boss.x - W * 0.42f, boss.y - H * 0.88f + boss.yOff, W * 0.84f, H * 0.85f };
    }
    void ColpisciBoss(float d, float x, float y) {
        BossP& b = boss;
        if (!b.vivo || b.tMorte > 0) return;
        if (b.ingresso > 0 || b.sotto) { Suona("ps_metallo", 0.4f); return; }
        b.hp -= d; b.lampo = 0.06f;
        regista.Bersaglio();
        if ((int)(tempo * 20) % 4 == 0) Suona("ps_metallo", 0.5f);
        fumo.push_back({ x, y, Vis(-40, 40), Vis(-60, 0), 0.25f, 0.25f, 1, Color{ 255, 230, 150, 255 } });
        if (b.hp <= 0) {
            b.hp = 0; b.tMorte = 3.2f;
            Punti(IntDi(BossJ(b.indice), "punti", 10000), b.x, b.y - 60);
            Suona("ps_boom");
            for (auto& c : colpi) if (c.nemico) c.vita = 0;
            for (auto& n : nemici) if (n.stato == 0) { n.stato = 1; n.t = 0; n.vy = -80; }
        }
    }
    void Rinforzo() {
        if (Vivi() >= 3) return;
        for (int k = 0; k < NNem() && k < 12; k++)
            if (IntDi(NemJ(k), "comportamento", 0) <= 1) { NuovoNemico(k, camX + LW + 12, false, 0); return; }
    }
    void AggiornaBoss(float dt) {
        BossP& b = boss;
        if (!b.vivo) return;
        float W = (float)MISURE_BOSS[b.aspetto].w, H = (float)MISURE_BOSS[b.aspetto].h;
        b.t += dt; b.lampo -= dt;
        bool muove = false;
        if (b.tMorte > 0) {
            b.tMorte -= dt;
            if ((int)(b.tMorte * 12) != (int)((b.tMorte + dt) * 12)) {
                Rectangle r = ScatolaBoss();
                boom.push_back({ r.x + Vis(0, r.width), r.y + Vis(0, r.height), 0, 0.6f, Vis(10, 22), (unsigned)(b.tMorte * 1000) });
                if ((int)(b.tMorte * 12) % 3 == 0) Suona("ps_esplosione", 0.7f);
                tScosse = 0.3f;
            }
            if (b.tMorte <= 0) { Esplosione(b.x, b.y - H / 2, 40, true); b.vivo = false; MissioneCompiuta(); }
            return;
        }
        float aggr = NumDi(BossJ(b.indice), "aggressivita", 1) * (0.8f + 0.2f * regista.sfida) * Crescita();
        if (b.ingresso > 0) {
            b.ingresso -= dt;
            b.x += (b.bersX - b.x) * std::min(1.0f, dt * 2.2f);
            muove = true;
            if (b.comp != 3) { b.tF += dt; if (b.tF > 0.2f) { b.tF = 0; b.f ^= 1; } }
            return;
        }
        float lato0 = camX + W / 2 + 4, lato1 = camX + LW - W / 2 - 4;
        float bocca = b.x - W * 0.48f;
        b.tA -= dt; b.tB -= dt; b.tC -= dt;
        bool meta = b.hp < b.hpMax * 0.5f;
        switch (b.comp) {
            case 0: {   // veicolo sui binari
                if (b.t > b.tF + 4) { b.tF = b.t; b.bersX = caso.F(camX + LW - W / 2 - 110, camX + LW - W / 2 - 10); }
                float d = b.bersX - b.x;
                if (std::fabs(d) > 2) { b.x += (d > 0 ? 1 : -1) * 30 * dt; muove = true; }
                if (b.tA <= 0) {
                    b.tA = 2.6f / aggr;
                    float x0 = b.x - W * 0.3f, y0 = b.y - H * 0.8f;
                    Vector2 v = Lancio(x0, y0, ex + Vis(-10, 10), ey - 6, 1.2f, 360);
                    Colpo(x0, y0, v.x, v.y, 13, 6, 1, true, 3); Suona("ps_cannone", 0.6f);
                }
                if (b.tB <= 0) {
                    b.tB = 3.0f / aggr;
                    if (b.aspetto == 8) b.tFiamma = 1.2f;
                    else { for (int k = 0; k < 3; k++) Colpo(bocca - k * 10, b.y - H * 0.35f + k * 3, -150 * (0.85f + 0.15f * regista.sfida), 0, 10, 3, 1, true, 1); Suona("ps_nemico", 0.6f); }
                }
                if (meta && b.tC <= 0) { b.tC = 6; Rinforzo(); }
                break;
            }
            case 1: {   // volante
                float tx = camX + LW / 2.0f + std::sin(b.t * 0.45f) * 110;
                b.x += (tx - b.x) * std::min(1.0f, dt * 1.5f);
                b.y = (b.hp < b.hpMax * 0.4f ? 126 : 104) + std::sin(b.t * 1.3f) * 4;
                muove = true;
                if (b.tA <= 0) { b.tA = 1.8f / aggr; Colpo(b.x + Vis(-W * 0.2f, W * 0.2f), b.y - 6, 0, 30, 12, 6, 1, true, 3); }
                if (b.tB <= 0) {
                    b.tB = 2.8f / aggr;
                    for (int k = -1; k <= 1; k += 2 + (meta ? 0 : 1)) {
                        float ax = ex - b.x + k * 14, ay = ey - 18 - (b.y - 12), l = std::max(1.0f, std::sqrt(ax * ax + ay * ay));
                        Colpo(b.x, b.y - 12, ax / l * 110, ay / l * 110, 10, 3, 1, true, 1);
                    }
                    Suona("ps_nemico", 0.6f);
                }
                if (meta && b.tC <= 0) { b.tC = 7; Rinforzo(); }
                break;
            }
            case 2: {   // gigante che cammina
                if (b.fase == 0) {
                    float tx = Limita(ex + 70 + W * 0.3f, lato0, lato1);
                    float d = tx - b.x;
                    if (std::fabs(d) > 3) { b.x += (d > 0 ? 1 : -1) * 26 * dt; muove = true; }
                    if (b.tA <= 0) { b.tA = 5.0f / aggr; b.fase = 1; b.vy = -170; }
                    else if (b.tC <= 0) { b.tC = 9; b.fase = 2; b.vx = ex < b.x ? -150.0f : 150.0f; }
                    if (b.tB <= 0) {
                        b.tB = 2.6f / aggr;
                        if (b.aspetto == 6) {
                            float x0 = b.x + W * 0.3f, y0 = b.y - H;
                            Vector2 v = Lancio(x0, y0, ex, ey - 6, 1.1f, 360);
                            Colpo(x0, y0, v.x, v.y, 13, 6, 1, true, 3); Suona("ps_cannone", 0.5f);
                        } else {
                            float x0 = bocca, y0 = b.y - H * 0.55f;
                            float a0 = std::atan2(ey - 18 - y0, ex - x0);
                            for (int k = -1; k <= 1; k++) Colpo(x0, y0, std::cos(a0 + k * 0.2f) * 120, std::sin(a0 + k * 0.2f) * 120, 10, 3, 1, true, 1);
                            Suona("ps_nemico", 0.6f);
                        }
                    }
                } else if (b.fase == 1) {   // pestone
                    b.vy += 420 * dt; b.y += b.vy * dt;
                    if (b.y >= TERRA) {
                        b.y = TERRA; b.fase = 0;
                        Colpo(b.x - W * 0.35f, TERRA - 5, -115, 0, 16, 2.6f, 1, true, 6);
                        Colpo(b.x + W * 0.35f, TERRA - 5, 115, 0, 16, 2.6f, 1, true, 6);
                        tScosse = 0.5f; Suona("ps_esplosione", 0.8f, 0.6f);
                        for (int k = 0; k < 12; k++) fumo.push_back({ b.x + Vis(-W / 2, W / 2), TERRA - 2, Vis(-40, 40), Vis(-50, -10), 0.7f, 0.7f, 3, Color{ 200, 180, 150, 170 } });
                    }
                } else {   // carica
                    b.x += b.vx * dt; muove = true;
                    if (b.x < lato0 || b.x > lato1) { b.x = Limita(b.x, lato0, lato1); b.fase = 0; tScosse = 0.3f; }
                }
                if (meta && b.tC > 0 && b.tC < 0.02f) Rinforzo();
                break;
            }
            case 3: {   // sotterraneo
                if (b.fase == 0) {
                    b.sotto = true; b.yOff = H;
                    if (b.tA <= 0) { b.fase = 1; b.tA = 0.9f; b.x = Limita(ex, lato0, lato1); }
                    if (meta && b.tB <= 0) { b.tB = 0.6f; Colpo(camX + Vis(20, LW - 20), -10, 0, 40, 15, 4, 1, true, 3); }
                } else if (b.fase == 1) {
                    if ((int)(b.t * 30) % 2 == 0) fumo.push_back({ b.x + Vis(-W / 3, W / 3), TERRA - 1, Vis(-30, 30), Vis(-60, -20), 0.5f, 0.5f, 2, Color{ 190, 160, 120, 200 } });
                    tScosse = 0.1f;
                    if (b.tA <= 0) { b.fase = 2; b.tA = 0.35f; Suona("ps_esplosione", 0.7f, 0.7f); }
                } else if (b.fase == 2) {
                    b.sotto = false; b.yOff = H * std::max(0.0f, b.tA / 0.35f);
                    if (b.tA <= 0) { b.fase = 3; b.tA = 3.0f; b.yOff = 0; b.tB = 0.4f; }
                } else if (b.fase == 3) {
                    if (b.tB <= 0) {
                        b.tB = 0.9f / aggr;
                        float x0 = b.x - W * 0.4f, y0 = b.y - H * 0.6f;
                        Vector2 v = Lancio(x0, y0, ex + Vis(-20, 20), ey - 4, 1.0f, 360);
                        Colpo(x0, y0, v.x, v.y, 15, 4, 1, true, 3);
                    }
                    if (b.tA <= 0) { b.fase = 4; b.tA = 0.5f; }
                } else {
                    b.yOff = H * (1 - std::max(0.0f, b.tA / 0.5f));
                    if (b.tA <= 0) { b.fase = 0; b.tA = 1.4f / aggr + 0.3f; b.sotto = true; }
                }
                muove = b.fase == 2 || b.fase == 4;
                break;
            }
            default: {  // fortezza fissa
                b.x += (camX + LW - W * 0.42f - b.x) * std::min(1.0f, dt * 2);
                if (b.aspetto == 3) b.y = TERRA + 2 + std::sin(b.t * 1.6f) * 2;
                muove = b.aspetto == 3;
                if (b.tA <= 0) {
                    b.tA = 2.2f / aggr;
                    float x0 = b.x - W * 0.45f, y0 = b.y - H * 0.7f;
                    Vector2 v = Lancio(x0, y0, ex + Vis(-12, 12), ey - 6, 1.3f, 360);
                    Colpo(x0, y0, v.x, v.y, 13, 6, 1, true, 3); Suona("ps_cannone", 0.6f);
                    fumo.push_back({ x0, y0, -20, -10, 0.5f, 0.5f, 4, Color{ 220, 220, 220, 180 } });
                }
                if (b.tB <= 0) { b.tB = 3.6f / aggr; Colpo(b.x - W * 0.4f, TERRA - 6, -95, 0, 17, 6, 1, true, 6); }
                if (b.tC <= 0) { b.tC = 7; Rinforzo(); }
                break;
            }
        }
        if (b.tFiamma > 0) {
            b.tFiamma -= dt;
            Colpo(bocca - 4, b.y - H * 0.42f, -170 + Vis(-20, 20), Vis(-20, 20), 18, 0.5f, 1, true, 3);
        }
        if (muove) { b.tF += dt; if (b.tF > 0.2f && b.comp != 0) { b.tF = 0; b.f ^= 1; } if (b.comp == 0) b.f = (int)(b.t * 6) % 2; }
        if (!b.sotto && b.tMorte <= 0 && CheckCollisionRecs(ScatolaBoss(), ScatolaEroe())) ColpitoEroe();
    }

    // ======================= AGGIORNAMENTO =======================
    void Aggiorna(float dt, const Pad& pad) override {
        if (finito) return;
        dt = std::min(dt, 1 / 30.0f);
        tempo += dt; tMissione += dt;
        tScritta -= dt; tScosse = std::max(0.0f, tScosse - dt);
        for (auto& b : boom) b.t += dt;
        boom.erase(std::remove_if(boom.begin(), boom.end(), [](const Boom& b) { return b.t > b.durata; }), boom.end());
        for (auto& f : fumo) { f.vita -= dt; f.x += f.vx * dt; f.y += f.vy * dt; f.vy += (f.r <= 1 ? 200 : -10) * dt; }
        fumo.erase(std::remove_if(fumo.begin(), fumo.end(), [](const Fumo& f) { return f.vita <= 0; }), fumo.end());
        for (auto& t : testi) { t.vita -= dt; t.y -= 16 * dt; }
        testi.erase(std::remove_if(testi.begin(), testi.end(), [](const TestoVolante& t) { return t.vita <= 0; }), testi.end());

        if (tFine >= 0) {
            tFine -= dt;
            Pad fermo; fermo.x = pad.x;
            AggiornaEroe(dt, fermo);
            AggiornaPremi(dt);
            if (tFine < 0) {
                missione++;
                if (missione >= NMis()) { missione = NMis() - 1; vinto = finito = true; ScriviRecord(); return; }
                IniziaMissione(false);
            }
            return;
        }
        AggiornaEroe(dt, pad);
        // il tempo della missione
        if (tMorto <= 0 && tScritta < 2.5f) {
            tSecondo += dt;
            if (tSecondo >= 1) { tSecondo -= 1; timer -= 1; if (timer <= 0) { timer = 0; tInv = 0; if (inCarro) { vitaCarro = 0.5f; } ColpitoEroe(); timer = (float)IntDi(Mis(), "tempo", 60); } }
        }
        // arrivano i nemici
        for (auto& a : arrivi)
            if (!a.fatto && camX + LW + 30 > a.x && Vivi() < 9) { a.fatto = true; for (int i = 0; i < a.quanti; i++) NuovoNemico(a.nemico, a.x, a.alto, i); }
        // la telecamera va solo avanti
        if (!arena) {
            camX = std::max(camX, std::min(ex - 150, lunghezza - LW));
            if (camX >= lunghezza - LW - 0.5f) {
                if (IndBoss() >= 0) InizioBoss();
                else if (ex > lunghezza - 30) { arena = true; MissioneCompiuta(); }
            }
        }
        AggiornaNemici(dt);
        AggiornaColpi(dt);
        AggiornaOstaggi(dt);
        AggiornaPremi(dt);
        AggiornaBoss(dt);
        if (punteggio >= prossimaVita) { vite++; prossimaVita += IntDi(Gen(), "vitaOgni", 60000); Suona("ps_vita"); Testo(ex, ey - 60, "1UP!", Color{ 120, 255, 140, 255 }); }
        regista.Osserva(dt, (ex - camX) / LW, ey / LH, pad.fuoco);
    }

    // il pilota automatico della vetrina (e delle prove)
    Pad Automatico() override {
        Pad p;
        autoT += 1 / 60.0f;
        p.x = 1;
        const Nemico* vicino = nullptr; float dv = 1e9f;
        bool sopra = false; float alto = 1e9f;
        for (auto& n : nemici) {
            if (n.stato != 0) continue;
            float d = n.x - ex;
            if (n.comp == 5) { if (std::fabs(d) < 40) sopra = true; continue; }
            if (n.y < ey - 40) { if (std::fabs(d) < 6) sopra = true; else if (d > -60 && d < 120) alto = d; continue; }
            if (d > -30 && d < 240 && std::fabs(d) < dv) { dv = std::fabs(d); vicino = &n; }
        }
        bool bersaglio = vicino != nullptr;
        if (alto < 1e8f && !vicino) p.x = Limita(alto / 8, -1, 1);
        if (vicino && dv < 70 && vicino->comp != 4) p.x = 0;
        if (vicino && vicino->x < ex) p.x = -0.3f;
        for (auto& q : prigionieri) if (q.stato == 0 && q.x > ex && q.x - ex < 140) bersaglio = true;
        for (auto& o : oggetti) if (o.vivo && o.tipo <= 2 && o.x > ex && o.x - ex < 70) bersaglio = true;
        if (arena && boss.vivo) {
            float W = (float)MISURE_BOSS[boss.aspetto].w;
            float want = boss.comp == 1 ? boss.x - W * 0.36f : boss.x - W * 0.5f - 80;
            if (boss.comp == 3) want = boss.sotto ? (ex < camX + LW / 2 ? camX + LW - 60 : camX + 60) : boss.x - W * 0.5f - 60;
            want = Limita(want, camX + 20, camX + LW - 20);
            p.x = Limita((want - ex) / 16, -1, 1);
            bersaglio = boss.ingresso <= 0 && !boss.sotto;
            if (boss.comp == 1 && std::fabs(boss.x - ex) < W * 0.45f) sopra = true;
        }
        for (auto& q : premi) if (std::fabs(q.x - ex) < 80 && q.t < 11) p.x = Limita((q.x - ex) / 10, -1, 1);
        if (sopra) { p.su = true; p.y = -1; bersaglio = true; }
        p.fuoco = bersaglio && (arma > 0 || std::fmod(autoT, 0.16f) < 0.08f);
        p.fuocoP = p.fuoco && !autoFuoco;
        autoFuoco = p.fuoco;
        // salta per schivare
        bool salta = false;
        for (auto& c : colpi) {
            if (!c.nemico) continue;
            float d = c.x - ex;
            bool basso = c.tipo == 16 || c.tipo == 17;
            if ((c.tipo == 10 || c.tipo == 14 || c.tipo == 18 || basso) && ((d > 0 && d < 50 && c.vx < 0) || (d < 0 && d > -50 && c.vx > 0)) && c.y > ey - 40 && c.y <= ey + 2) salta = true;
            // previsione: dove sara' il colpo nei prossimi istanti
            if (c.tipo == 10) {
                Rectangle h = ScatolaEroe(); h.x -= 3; h.width += 6; h.y -= 3; h.height += 6;
                for (float t = 0; t < 0.45f; t += 0.03f)
                    if (CheckCollisionPointRec({ c.x + c.vx * t, c.y + c.vy * t }, h)) {
                        if (std::fabs(c.vy) < std::fabs(c.vx) * 0.6f) salta = true;
                        else p.x = c.vx > 0 ? 1.0f : c.vx < 0 ? -1.0f : (c.x > ex ? -1.0f : 1.0f);
                        break;
                    }
            }
            if ((c.tipo == 11 || c.tipo == 13 || c.tipo == 12) && std::fabs(d) < 30 && c.vy > 0 && c.y > ey - 80) p.x = d > 0 ? -1.0f : 1.0f;
        }
        for (auto& n : nemici) if (n.stato == 0 && n.comp == 4 && n.x - ex > 0 && n.x - ex < 60) salta = true;
        for (auto& q : piatt) if (q.solida && PiattViva(q) && q.x0 - ex > 0 && q.x0 - ex < 14 && ey > q.y) salta = true;
        if (arena && boss.vivo && boss.comp == 2 && boss.fase == 2 && std::fabs(boss.x - ex) < 90) salta = true;
        p.salto = salta; p.saltoP = salta && aTerra;
        p.bombaP = p.bomba = (arena && boss.vivo && boss.ingresso <= 0 && !boss.sotto && std::fmod(autoT, 2.5f) < 1 / 60.0f) ||
                             (Vivi() >= 3 && std::fmod(autoT, 1.5f) < 1 / 60.0f && dv < 120);
        return p;
    }

    // ======================= DISEGNO =======================
    float sx = 0, sy = 0;
    int X(float wx) const { return (int)std::round(wx - camX + sx); }
    void DisegnaBoom(const Boom& b) const {
        float k = b.t / b.durata;
        Rnd r{ b.seme | 1u };
        for (int i = 0; i < 10; i++) {
            float ox = r.F(-b.r, b.r), oy = -b.r * 0.6f - r.F(0, b.r * 1.3f) * k;
            DrawCircle(X(b.x + ox), (int)(b.y + oy * (0.4f + k) + sy), (int)(r.F(3, 7) * (0.6f + k * 0.8f)), Color{ 58, 48, 44, (unsigned char)(220 * (1 - k)) });
        }
        const Color fuoco[4] = { C(0xd04818), C(0xf08828), C(0xffc848), C(0xfff0b8) };
        for (int i = 0; i < 14; i++) {
            float a = r.F(0, 6.28f), d = r.F(0, b.r) * (0.5f + k);
            int rr = (int)(r.F(b.r * 0.25f, b.r * 0.5f) * (1 - k * 0.7f));
            Color c = fuoco[(i + (int)(k * 4)) % 4];
            if (k > 0.6f) c = Mescola(c, C(0x3a2a20), (k - 0.6f) * 2.5f);
            if (rr > 0) DrawCircle(X(b.x + std::cos(a) * d), (int)(b.y + std::sin(a) * d * 0.8f - k * 8 + sy), (float)rr, c);
        }
        if (k < 0.4f) for (int i = 0; i < 10; i++) DrawRectangle(X(b.x + r.F(-1.7f, 1.7f) * b.r * (0.5f + k * 2)), (int)(b.y + r.F(-1.6f, 0.5f) * b.r * (0.5f + k * 2)), 2, 2, C(0x3a2a20));
    }
    void DisegnaColpo(const Proiettile& c) const {
        int x = X(c.x), y = (int)std::round(c.y + sy);
        float s = c.vx >= 0 ? 1.0f : -1.0f;
        switch (c.tipo) {
            case 0: DrawRectangle(x - 2, y, 5, 2, C(0xfff4b0)); DrawRectangle(x - (int)(s * 5) - (s > 0 ? 0 : 2), y, 3, 2, C(0xf0a030)); break;
            case 1:
                if (std::fabs(c.vy) > std::fabs(c.vx)) { DrawRectangle(x, y - 3, 2, 7, C(0xfff4b0)); DrawRectangle(x, y + (c.vy < 0 ? 4 : -6), 2, 3, C(0xf0a030)); }
                else { DrawRectangle(x - 3, y, 7, 2, C(0xfff4b0)); DrawRectangle(s > 0 ? x - 7 : x + 4, y, 4, 2, C(0xf0a030)); }
                break;
            case 2: DrawRectangle(x - 1, y - 1, 2, 2, C(0xffffff)); DrawRectangle(x - (int)s * 2, y - 1, 2, 2, C(0xf0a030)); break;
            case 3: case 18: { float k = c.vita / 0.5f; DrawCircle(x, y, c.r, Color{ 240, 100, 30, (unsigned char)(220 * std::min(1.0f, k * 1.5f)) }); DrawCircle(x, y, c.r * 0.55f, Color{ 255, 220, 90, (unsigned char)(255 * std::min(1.0f, k * 1.5f)) }); break; }
            case 4: { float a = std::atan2(c.vy, c.vx) * RAD2DEG; DrawRectanglePro({ (float)x, (float)y, 7, 3 }, { 3.5f, 1.5f }, a, C(0x9a9aa8)); DrawCircle(x - (int)(std::cos(a * DEG2RAD) * 5), y - (int)(std::sin(a * DEG2RAD) * 5), 2, C(0xffb040)); break; }
            case 5: case 13: DrawCircle(x, y, 3, C(0x2e2e2c)); DrawRectangle(x - 1, y - 2, 1, 1, C(0xa0a0a0)); if (c.tipo == 13) DrawRectangle(x, y - 1, 1, 1, C(0xff4020)); break;
            case 6: case 11: DrawRectanglePro({ (float)x, (float)y, 3, 7 }, { 1.5f, 3.5f }, c.t * 600, C(0xc03028)); if ((int)(c.t * 20) % 2) DrawRectangle(x - 1, y - 5, 2, 2, C(0xffe040)); break;
            case 10: DrawRectangle(x - 1, y - 1, 3, 2, C(0xff7040)); DrawRectangle(x, y - 1, 1, 1, C(0xffe0a0)); break;
            case 12: DrawEllipse(x, y, 3, 5, C(0x3a3a34)); DrawRectangle(x - 3, y - 6, 6, 2, C(0x5a5a50)); break;
            case 14: DrawCircle(x, y, c.r, Color{ 240, 240, 240, (unsigned char)(200 * std::min(1.0f, c.vita * 2)) }); break;
            case 15: DrawCircle(x, y, 3, C(0x8a7866)); DrawRectangle(x - 1, y - 2, 2, 1, C(0xb0a090)); break;
            case 16: for (int k = 0; k < 4; k++) { DrawRectangle(x - 6 + k * 3, y - 2 - (k % 2) * 3, 3, 4 + (k % 2) * 3, C(0xc8a878)); } DrawRectangle(x - 7, y + 2, 14, 2, C(0x8a6a48)); break;
            case 17: DrawCircle(x, y, 6, C(0x2e2e2c)); DrawCircle(x - 2, y - 2, 2, C(0x6a6a6a)); { float a = c.t * -10; DrawLine(x, y, x + (int)(std::cos(a) * 5), y + (int)(std::sin(a) * 5), C(0x101010)); } break;
        }
    }
    void Disegna() override {
        int ak = IndAmb();
        PreparaAmbiente(ak);
        int bi = IndBoss();
        if (bi >= 0) PreparaBoss(bi);
        sx = tScosse > 0 ? std::round(Vis(-2, 2) * std::min(1.0f, tScosse * 3)) : 0;
        sy = tScosse > 0 ? std::round(Vis(-2, 2) * std::min(1.0f, tScosse * 3)) : 0;
        int o1 = (int)std::fmod(camX * 0.12f, (float)LS), o2 = (int)std::fmod(camX * 0.45f, (float)LS), o3 = (int)std::fmod(std::round(camX), (float)LW);
        // grafica propria (Studio): i piedi in 'piedi', alta h
        auto Gr = [&](const json& j, float x, float piedi, float h, const char* anim, float t, bool specchia, Color tinta = WHITE, float rot = 0) {
            return DisegnaGrafica(j, { x, piedi - h / 2 }, h, anim, t, specchia, tinta, rot);
        };
        if (!DisegnaSfondo(Amb(ak), camX)) {
            DrawTexture(gLon[ak], -o1, 0, WHITE); DrawTexture(gLon[ak], -o1 + LS, 0, WHITE);
            DrawTexture(gMed[ak], -o2 + (int)sx, (int)sy, WHITE); DrawTexture(gMed[ak], -o2 + LS + (int)sx, (int)sy, WHITE);
        }
        bool bossSotto = boss.vivo && boss.comp == 3;
        Color tintaBoss = boss.lampo > 0 ? Color{ 255, 150, 150, 255 } : (boss.tMorte > 0 && (int)(tempo * 16) % 2 ? Color{ 255, 200, 160, 255 } : WHITE);
        float hBoss = (float)MISURE_BOSS[std::max(0, std::min(9, boss.aspetto))].h;
        if (bossSotto && !(boss.sotto && boss.fase == 0) && bi >= 0 && !Gr(BossJ(bi), (float)X(boss.x), boss.y + boss.yOff + sy, hBoss, "attacco|fermo", tempo, false, tintaBoss))
            DisegnaSpr(gBossS[bi][boss.f], (float)X(boss.x), boss.y + boss.yOff + sy, false, tintaBoss);
        DrawTexture(gSuo[ak], -o3 + (int)sx, Y_SUOLO + (int)sy, WHITE); DrawTexture(gSuo[ak], -o3 + LW + (int)sx, Y_SUOLO + (int)sy, WHITE);
        // oggetti del mondo
        for (auto& o : oggetti) {
            if (!o.vivo || o.x < camX - 70 || o.x > camX + LW + 70) continue;
            float y = o.y + sy;
            switch (o.tipo) {
                case 0: DisegnaSpr(gCassa, (float)X(o.x), y); break;
                case 1: DisegnaSpr(gBarile, (float)X(o.x), y); break;
                case 2: DisegnaSpr(gBarileP, (float)X(o.x), y); break;
                case 3: DisegnaSpr(gImpalc, (float)X(o.x), y); break;
                case 4: DisegnaSpr(gSacchi, (float)X(o.x), y); break;
                case 5: DisegnaSpr(gRoccia, (float)X(o.x), y); break;
            }
        }
        if (carroLibero && carroX > camX - 40 && carroX < camX + LW + 40) {
            DisegnaSpr(gCarro[0][0], (float)X(carroX), carroY + sy);
            if ((int)(tempo * 3) % 2) ScrittaPixel("SALI!", (float)X(carroX), carroY - 58, 1, C(0xffe080), 0);
        }
        for (auto& p : prigionieri) {
            if (p.x < camX - 30 || p.x > camX + LW + 30) continue;
            int f = p.stato == 0 ? (int)(tempo * 2.5f) % 2 : p.stato == 2 ? 4 : 2 + (int)(tempo * 8) % 2;
            bool flip = p.stato == 3 || (p.stato == 1 && ex < p.x);
            DisegnaSpr(gOst[f], (float)X(p.x), p.y + sy, flip);
            if (p.stato == 0 && (int)(tempo * 1.5f + p.x) % 3 == 0) {
                int bx = X(p.x) - 17, by = (int)(p.y - 70 + sy);
                DrawRectangle(bx, by, 34, 12, C(0xfff8e0)); DrawRectangleLines(bx - 1, by - 1, 36, 14, C(0x2a1a10));
                DrawTriangle({ (float)bx + 12, (float)by + 12 }, { (float)bx + 15, (float)by + 17 }, { (float)bx + 18, (float)by + 12 }, C(0xfff8e0));
                DrawText("AIUTO", bx + 3, by + 1, 10, C(0xb02818));
            }
            if (p.stato == 2) ScrittaPixel("GRAZIE!", (float)X(p.x), p.y - 66 + sy, 1, WHITE, 0);
        }
        for (auto& n : nemici) {
            if (n.x < camX - 60 || n.x > camX + LW + 60 || n.tipo >= 12) continue;
            int f = 0;
            if (n.comp <= 2) {
                if (n.stato == 2) f = 6;
                else if (n.comp == 1 && n.t < 0.4f && n.stato == 0) f = 5;
                else if (n.comp != 1 && n.t < 0.3f && n.stato == 0) f = 4;
                else if (n.stato == 0 && std::fabs(ex - n.x) > 70 && n.comp == 0) f = (int)(n.tAnim * 8) % 4;
                else if (n.comp == 1 && n.stato == 0) f = (int)(n.tAnim * 8) % 4;
                else if (n.comp == 2) f = 4;
            } else if (n.comp == 3) f = n.t < 0.4f ? 4 : (int)(n.tAnim * 6) % 4;
            else if (n.comp == 4) f = (int)(n.tAnim * 8) % 2;
            else f = (int)(tempo * 20) % 2;
            bool flip = n.dir > 0;
            Color tinta = n.lampo > 0 ? Color{ 255, 110, 110, 255 } : WHITE;
            const float hNem[6] = { 46, 46, 46, 56, 64, 30 };
            float hn = hNem[std::max(0, std::min(5, n.comp))];
            float piediN = n.comp == 5 ? n.y + sy + hn / 2 : n.y + sy;
            if (n.stato == 1 && GraficaDi(NemJ(n.tipo))) {
                if ((int)(n.t * 14) % 2 && n.t > 0.6f) continue;
                Gr(NemJ(n.tipo), (float)X(n.x), piediN, hn, "colpito|fermo", n.t, n.dir < 0, tinta, std::min(1.0f, n.t * 3) * 90 * (flip ? -1 : 1));
                continue;
            }
            if (n.stato != 1 && Gr(NemJ(n.tipo), (float)X(n.x), piediN, hn, f == 4 || f == 5 ? "sparo|attacco|fermo" : f < 4 && n.stato == 0 ? "corsa|fermo" : "fermo",
                                   n.tAnim, n.dir < 0, tinta)) continue;
            if (n.stato == 1) {
                if ((int)(n.t * 14) % 2 && n.t > 0.6f) continue;
                if (n.comp == 5) DisegnaSpr(gNem[n.tipo][0], (float)X(n.x), n.y + sy, flip, C(0x8a8080), n.t * 90);
                else DisegnaSpr(gNem[n.tipo][n.comp <= 2 ? 6 : 0], (float)X(n.x), n.y + sy, flip, tinta, std::min(1.0f, n.t * 3) * 90 * (flip ? -1 : 1));
            } else DisegnaSpr(gNem[n.tipo][f], (float)X(n.x), n.y + sy, flip, tinta);
        }
        if (boss.vivo && !bossSotto && bi >= 0 && !Gr(BossJ(bi), (float)X(boss.x), boss.y + boss.yOff + sy, hBoss, "attacco|fermo", tempo, false, tintaBoss))
            DisegnaSpr(gBossS[bi][boss.f], (float)X(boss.x), boss.y + boss.yOff + sy, false, tintaBoss);
        // l'eroe
        bool lampeggia = tInv > 0 && tMorto <= 0 && std::fmod(tempo, 0.16f) < 0.06f;
        if (tMorto > 0) {
            if ((int)(tMorto * 10) % 2 || tMorto > 0.8f) {
                if (!Gr(Gen(), (float)X(ex), ey + sy, 46, "colpito|fermo", tMorto, dir < 0, WHITE, ey >= TERRA - 1 ? -90.0f * dir : 0))
                    DisegnaSpr(gEroe[giocatore][5][0][0], (float)X(ex), ey + sy, dir < 0, WHITE, ey >= TERRA - 1 ? -90.0f * dir : 0);
            }
        } else if (!lampeggia) {
            if (inCarro) DisegnaSpr(gCarro[(int)(tAnim * 10) % 2][1], (float)X(ex), ey + sy, dir < 0);
            else {
                int g = !aTerra ? 5 : std::fabs(evx) > 1 ? 1 + (int)(tAnim * 10) % 4 : 0;
                const float cad[5] = { 0.22f, 0.075f, 0.5f, 0.05f, 0.35f };
                bool sparando = tSparo > cad[arma] - 0.05f;
                const char* an = !aTerra ? "salto|fermo" : sparando ? "sparo|attacco|fermo" : g > 0 ? "corsa|fermo" : "fermo";
                if (!Gr(Gen(), (float)X(ex), ey + sy, 46, an, tAnim, dir < 0))
                    DisegnaSpr(gEroe[giocatore][g][mira][arma > 0 ? 1 : 0], (float)X(ex), ey + sy, dir < 0);
                if (tSparo > cad[arma] - 0.05f && arma != 3) {
                    Vector2 cn = Canna(mira, arma > 0);
                    int fx = X(ex + cn.x * dir), fy = (int)(ey + cn.y + sy);
                    DrawCircle(fx, fy, 3, C(0xffe060)); DrawCircle(fx, fy, 1.5f, C(0xfff8d8));
                }
            }
        }
        for (auto& c : colpi) DisegnaColpo(c);
        for (auto& q : premi) {
            if (q.t > 9 && (int)(q.t * 8) % 2) continue;
            int x = X(q.x), y = (int)(q.y + sy);
            if (q.tipo >= 1 && q.tipo <= 4) { DisegnaSpr(gIcona, (float)x, (float)y); DrawText(SIGLE_ARMI[q.tipo], x - 3, y - 11, 10, C(0xffe060)); }
            else if (q.tipo == 5) { DisegnaSpr(gIcona, (float)x, (float)y); DrawRectangle(x - 4, y - 10, 3, 7, C(0xc03028)); DrawRectangle(x + 1, y - 10, 3, 7, C(0xc03028)); DrawRectangle(x - 1, y - 12, 1, 2, C(0xffe040)); }
            else if (q.tipo == 6) { DrawCircle(x, y - 4, 4, C(0xc08a20)); DrawCircle(x - 1, y - 5, 3, C(0xffd040)); DrawText("$", x - 2, y - 9, 10, C(0x7a4a10)); }
            else ScrittaPixel("1UP", (float)x, (float)y - 10, 1, C(0x80ff90), 0);
        }
        for (auto& b : boom) DisegnaBoom(b);
        for (auto& f : fumo) {
            float k = f.vita / f.durata;
            Color c = f.c; c.a = (unsigned char)(c.a * std::min(1.0f, k * 1.5f));
            if (f.r <= 1) DrawRectangle(X(f.x), (int)(f.y + sy), 2, 2, c);
            else DrawCircle(X(f.x), (int)(f.y + sy), f.r * (1.6f - k * 0.6f), c);
        }
        // meteo
        int meteo = IntDi(Amb(ak), "meteo", 0);
        for (int i = 0; meteo > 0 && i < 70; i++) {
            unsigned h = (unsigned)(i + 1) * 2654435761u;
            float fx = (float)(h % 997) / 997 * (LW + 40), fy = (float)((h >> 10) % 991) / 991 * LH;
            if (meteo == 1) { if (i > 36) break; float x = std::fmod(fx - tempo * 60 - camX * 0.6f + 4000, (float)LW + 40) - 20, y = fy + std::sin(tempo * 2 + i) * 6; DrawRectangle((int)x, (int)y, 2, 1, Color{ 240, 220, 180, 120 }); }
            else if (meteo == 2) { float x = std::fmod(fx + std::sin(tempo + i) * 10 - camX * 0.8f + 4000, (float)LW + 40) - 20, y = std::fmod(fy + tempo * (20 + i % 5 * 6), (float)LH); DrawRectangle((int)x, (int)y, 1 + i % 2, 1 + i % 2, Color{ 255, 255, 255, 220 }); }
            else if (meteo == 3) { float x = std::fmod(fx - camX + 4000 - tempo * 40, (float)LW + 40) - 20, y = std::fmod(fy + tempo * 260, (float)LH); DrawLine((int)x, (int)y, (int)x - 2, (int)y + 6, Color{ 180, 200, 230, 150 }); }
            else { if (i > 30) break; float x = std::fmod(fx - camX * 0.9f + 4000 + std::sin(tempo + i) * 8, (float)LW + 40) - 20, y = LH - std::fmod(fy + tempo * (14 + i % 4 * 5), (float)LH); DrawRectangle((int)x, (int)y, 1, 1, Color{ 255, (unsigned char)(120 + i % 3 * 40), 40, 230 }); }
        }
        for (auto& t : testi) ScrittaPixel(t.t, (float)X(t.x), t.y + sy, 1, ConAlfa(t.c, std::min(1.0f, t.vita * 2)), 0);
        DisegnaHud(bi);
    }
    void DisegnaHud(int bi) {
        char b[32];
        std::string chi = giocatore ? "2UP=" : "1UP=";
        ScrittaPixel(chi + std::to_string(std::max(0, vite - 1)), 4, 3, 1, WHITE);
        std::snprintf(b, sizeof b, "%06d", punteggio); ScrittaPixel(b, 4, 14, 1, WHITE);
        DrawRectangle(70, 3, 78, 24, Color{ 10, 10, 40, 150 }); DrawRectangle(70, 3, 78, 1, C(0x8890c0));
        if (inCarro) {
            ScrittaPixel("CARRO", 74, 4, 1, C(0xf8d048));
            float k = Limita(vitaCarro / VitaCarroMax(), 0, 1);
            DrawRectangle(74, 17, 66, 6, C(0x301010)); DrawRectangle(74, 17, (int)(66 * k), 6, k > 0.34f ? C(0x60d060) : C(0xe04030));
        } else {
            ScrittaPixel("ARMI", 74, 4, 1, C(0xf8d048)); ScrittaPixel("BOMBE", 111, 4, 1, C(0xf8d048));
            if (arma == 0) { DrawCircleLines(79, 20, 3, WHITE); DrawCircleLines(86, 20, 3, WHITE); }
            else { std::snprintf(b, sizeof b, "%s%d", SIGLE_ARMI[arma], munizioni); ScrittaPixel(b, 74, 15, 1, WHITE); }
            ScrittaPixel(std::to_string(bombe), 117, 15, 1, WHITE);
        }
        std::snprintf(b, sizeof b, "%02d", (int)std::ceil(timer));
        ScrittaPixel(b, 192, 2, 2, timer < 10 && (int)(tempo * 4) % 2 ? C(0xff6040) : C(0xf8d048), 0);
        ScrittaPixel("RECORD", 380, 3, 1, C(0xffd27a), 1);
        std::snprintf(b, sizeof b, "%06d", std::max(record, punteggio)); ScrittaPixel(b, 380, 14, 1, WHITE, 1);
        if (altroPunteggio >= 0) { std::snprintf(b, sizeof b, "%s %06d", giocatore ? "1UP" : "2UP", altroPunteggio); ScrittaPixel(b, 380, 25, 1, C(0xb0b8e0), 1); }
        for (int i = 0; i < ostaggiLiberati && i < 20; i++) { DrawRectangle(4 + i * 6, 204, 4, 6, C(0xece0c4)); DrawRectangle(4 + i * 6, 201, 4, 3, C(0xf2b888)); }
        ScrittaPixel("MISSIONE " + std::to_string(missione + 1), 192, 204, 1, ConAlfa(WHITE, 0.8f), 0);
        if (boss.vivo && boss.ingresso <= 0 && boss.tMorte <= 0 && bi >= 0) {
            float k = boss.hp / boss.hpMax;
            DrawRectangle(111, 26, 162, 7, C(0x200808)); DrawRectangle(112, 27, (int)(160 * k), 5, C(0xe03a3a)); DrawRectangle(112, 27, (int)(160 * k), 1, C(0xff9a7a));
        }
        if (tScritta > 0) {
            Color c = ConAlfa(C(0xf8d048), std::min(1.0f, tScritta * 2));
            int scala = LarghezzaPixel(scritta, 2) < 370 ? 2 : 1;
            ScrittaPixel(scritta, 192, 74, scala, c, 0);
            ScrittaPixel(sotto, 192, 100, 1, ConAlfa(WHITE, std::min(1.0f, tScritta * 2)), 0);
        } else if (tMissione > 3.2f && tMissione < 4.2f && !alBoss) ScrittaPixel("VIA!", 192, 80, 2, C(0xf8d048), 0);
    }

    // ======================= SALVATAGGI =======================
    std::string Stato() const override {
        json j;
        j["seme"] = std::to_string(seme); j["casoInizio"] = std::to_string(casoInizio);
        j["missione"] = missione; j["alBoss"] = alBoss; j["vite"] = vite; j["bombe"] = bombe; j["prossimaVita"] = prossimaVita;
        j["punteggio"] = punteggio; j["tempo"] = tempo; j["arma"] = arma; j["munizioni"] = munizioni; j["inCarro"] = inCarro; j["vitaCarro"] = vitaCarro;
        j["ostaggi"] = ostaggiLiberati; j["giocatore"] = giocatore;
        j["regista"] = json::parse(regista.Json());
        return j.dump();
    }
    bool Ripristina(const std::string& t, std::string& errore) override {
        json j = json::parse(t, nullptr, false);
        if (!j.is_object() || !j.contains("missione")) { errore = "dati di Polvere da Sparo non validi"; return false; }
        uint64_t s, c;
        try { s = std::stoull(j.value("seme", std::string("1"))); c = std::stoull(j.value("casoInizio", std::string("1"))); }
        catch (...) { errore = "stato del caso non valido"; return false; }
        bool vecchio = gSilenzio;
        gSilenzio = true;
        Nuova(s, j.value("giocatore", 0));
        gSilenzio = vecchio;
        missione = std::max(0, std::min(NMis() - 1, j.value("missione", 0)));
        vite = std::max(1, j.value("vite", 3)); bombe = j.value("bombe", 10); prossimaVita = j.value("prossimaVita", 20000);
        punteggio = j.value("punteggio", 0); tempo = j.value("tempo", 0.0f);
        arma = std::max(0, std::min(4, j.value("arma", 0))); munizioni = j.value("munizioni", 0);
        inCarro = j.value("inCarro", false); vitaCarro = j.value("vitaCarro", 0.0f); ostaggiLiberati = j.value("ostaggi", 0);
        if (j.contains("regista")) regista.DaJson(j["regista"].dump());
        caso.s = c;
        int p = puntiDiControllo;
        IniziaMissione(j.value("alBoss", false));
        puntiDiControllo = p;
        return true;
    }
};

Spr Polvere::gEroe[2][6][3][2], Polvere::gNem[12][7], Polvere::gOst[5], Polvere::gCarro[2][2], Polvere::gBossS[20][2];
Spr Polvere::gCassa, Polvere::gBarile, Polvere::gBarileP, Polvere::gImpalc, Polvere::gSacchi, Polvere::gRoccia, Polvere::gIcona;
Texture2D Polvere::gLon[20], Polvere::gMed[20], Polvere::gSuo[20];
std::string Polvere::gFirma, Polvere::gFirmaAmb[20], Polvere::gFirmaBoss[20];

}  // namespace

std::unique_ptr<GiocoArcade> CreaPolvere() { return std::unique_ptr<GiocoArcade>(new Polvere()); }

}  // namespace arcade
