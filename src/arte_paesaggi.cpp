// Paesaggi dettagliati disegnati con il Pennello: creste illuminate, alberi, palazzi, castelli,
// piramidi, ingranaggi, isole volanti, coralli, cieli con sole, luna, aurora; materiali del terreno.
#include "arte.h"
#include <algorithm>
#include <cmath>

namespace arte {

static const float TAU = 6.28318530718f;
static void ell(K& c, float x, float y, float rx, float ry, float rot = 0) { c.beginPath(); c.ellipse(x, y, std::max(0.1f, rx), std::max(0.1f, ry), rot, 0, TAU); }

std::function<float(float)> RumorePeriodico(unsigned seme, int n, float caduta) {
    Caso r(seme); std::vector<float> a, p; float norm = 0;
    for (int k = 1; k <= n; k++) { float am = 1 / std::pow((float)k, caduta); a.push_back(am); p.push_back(r() * TAU); norm += am; }
    return [a, p, norm](float x) { float s = 0; for (size_t i = 0; i < a.size(); i++) s += a[i] * std::sin((i + 1) * TAU * x / LS + p[i]); return s / norm; };
}
void Ripeti(float x, float w, const std::function<void(float)>& f) { f(x); if (x + w > LS) f(x - LS); if (x - w < 0) f(x + LS); }
void RiempiSotto(K& c, float y, const std::string& col, const std::string& col2) {
    c.fillStyle(lg(0, y, 0, HS, { col, col2.empty() ? sh(col, -0.3f) : col2 })); c.fillRect(0, y, LS, HS - y);
}
void Fila(unsigned seme, float base, float hmin, float hmax, float passo, const std::function<void(float, float, float, unsigned)>& f) {
    Caso r(seme); float x = r() * 60;
    while (x < LS) { float h = hmin + r() * (hmax - hmin); unsigned sd = (unsigned)(r() * 1e6); float b = base + r() * 6 - 3; f(x, b, h, sd); x += passo * (0.6f + r() * 0.8f); }
}

void DisegnaCresta(K& c, const Cresta& o) {
    auto n = RumorePeriodico(o.seme, o.k, o.fall);
    std::vector<float> ys;
    for (float x = 0; x <= LS; x += 2) {
        float v = n(x); if (o.punte) v = 0.5f - std::fabs(v) * 2.2f;
        ys.push_back(o.base - o.amp * std::max(0.f, std::min(1.25f, 0.5f + v * 1.5f)));
    }
    float top = o.base - o.amp * 1.25f;
    c.beginPath();
    if (o.soffitto) { c.moveTo(0, 0); for (size_t i = 0; i < ys.size(); i++) c.lineTo(i * 2.f, HS - ys[i]); c.lineTo(LS, 0); }
    else { c.moveTo(0, HS); for (size_t i = 0; i < ys.size(); i++) c.lineTo(i * 2.f, ys[i]); c.lineTo(LS, HS); }
    c.closePath();
    c.fillStyle(o.soffitto ? lg(0, 0, 0, HS - top, { o.bot, o.top }) : lg(0, top, 0, o.base + 60, { o.top, o.bot })); c.fill();
    if (o.soffitto) return;
    Caso r(o.seme + 3);
    for (size_t i = 0; i + 1 < ys.size(); i++) {
        float x = i * 2.f, sl = ys[i + 1] - ys[i], d = (18 + r() * 40) * o.facet;
        if (sl < -0.25f && !o.light.empty()) { c.fillStyle(V(o.light)); c.fillRect(x, ys[i], 2.05f, d * std::min(1.f, -sl)); }
        else if (sl > 0.25f && !o.shade.empty()) { c.fillStyle(V(o.shade)); c.fillRect(x, ys[i], 2.05f, d * 1.4f * std::min(1.f, sl)); }
        if (o.neve > 0 && ys[i] < o.neve) { float sd = (o.neve - ys[i]) * 0.55f + r() * 6; c.fillStyle(V(sl < 0 ? "#f4fbff" : "#b9d3ea")); c.fillRect(x, ys[i] - 1, 2.05f, sd); }
    }
    if (!o.strata.empty()) {
        c.strokeStyle(V(o.strata)); c.lineWidth(2);
        for (int k = 0; k < 90; k++) { float x = r() * LS; size_t i = std::min(ys.size() - 1, (size_t)(x / 2)); float y = ys[i] + 20 + r() * (o.base - ys[i] + 40); c.beginPath(); c.moveTo(x, y); c.lineTo(x + 20 + r() * 50, y + (r() - 0.5f) * 8); c.stroke(); }
    }
    if (o.lava) {
        for (int k = 0; k < 5; k++) {
            float x = r() * LS; size_t i = std::min(ys.size() - 1, (size_t)(x / 2));
            c.strokeStyle(V("rgba(255,120,40,0.85)")); c.lineWidth(3); c.beginPath(); c.moveTo(x, ys[i] + 4);
            float yy = ys[i] + 4, xx = x; for (int j = 0; j < 10; j++) { xx += (r() - 0.4f) * 10; yy += 12; c.lineTo(xx, yy); } c.stroke();
        }
    }
    if (!o.haze.empty()) { c.fillStyle(lg(0, top, 0, o.base + 40, { rgba(o.haze, 0), rgba(o.haze, o.hazeA) })); c.fillRect(0, top, LS, HS - top); }
}

void Pino(K& c, float x, float y, float h, const ColoriAlbero& o) {
    c.fillStyle(V(o.trunk)); c.fillRect(x - h * 0.03f, y - h * 0.22f, h * 0.06f, h * 0.24f);
    const int tiers = 6;
    for (int i = 0; i < tiers; i++) {
        float ty = y - h * 0.12f - i * h * 0.14f, tw = h * 0.3f * (1 - i / (tiers + 1.2f)), th = h * 0.3f;
        c.beginPath(); c.moveTo(x, ty - th); const int z = 7;
        for (int j = 0; j <= z; j++) { float px = x + tw - 2 * tw * j / z; c.lineTo(px + (j % 2 ? 0.f : 3.f), ty + (j % 2 ? -6.f : 0.f)); }
        c.closePath(); c.fillStyle(V(o.dark)); c.fill();
        c.beginPath(); c.moveTo(x, ty - th); c.lineTo(x - tw * 0.95f, ty - 2); c.lineTo(x - tw * 0.2f, ty - 5); c.closePath(); c.fillStyle(V(o.light)); c.fill();
        if (o.snow) { c.beginPath(); c.moveTo(x, ty - th); c.lineTo(x - tw * 0.5f, ty - th * 0.45f); c.lineTo(x - tw * 0.2f, ty - th * 0.5f); c.lineTo(x + tw * 0.1f, ty - th * 0.35f); c.lineTo(x + tw * 0.45f, ty - th * 0.48f); c.closePath(); c.fillStyle(V("#eef7ff")); c.fill(); }
        if (!o.rim.empty()) { c.strokeStyle(V(o.rim)); c.lineWidth(1.5f); c.beginPath(); c.moveTo(x, ty - th); c.lineTo(x + tw, ty); c.stroke(); }
    }
}
void AlberoTondo(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned seme) {
    Caso r(seme); c.save();
    c.fillStyle(lg(x - h * 0.05f, 0, x + h * 0.05f, 0, { sh(o.trunk, 0.15f), o.trunk, sh(o.trunk, -0.4f) }));
    c.beginPath(); c.moveTo(x - h * 0.06f, y); c.quadraticCurveTo(x - h * 0.03f, y - h * 0.35f, x - h * 0.025f, y - h * 0.6f); c.lineTo(x + h * 0.025f, y - h * 0.6f); c.quadraticCurveTo(x + h * 0.03f, y - h * 0.35f, x + h * 0.07f, y); c.fill();
    c.strokeStyle(V(o.trunk)); c.lineWidth(h * 0.025f); c.beginPath(); c.moveTo(x, y - h * 0.45f); c.lineTo(x - h * 0.18f, y - h * 0.65f); c.moveTo(x, y - h * 0.5f); c.lineTo(x + h * 0.2f, y - h * 0.7f); c.stroke();
    float cx = x, cy = y - h * 0.72f, rx = h * 0.36f, ry = h * 0.3f;
    struct B { float x, y, r; }; std::vector<B> bl;
    for (int i = 0; i < 22; i++) { float a = r() * TAU, d = std::sqrt(r()); bl.push_back({ cx + std::cos(a) * rx * d * 0.8f, cy + std::sin(a) * ry * d * 0.8f, h * (0.09f + r() * 0.08f) }); }
    c.fillStyle(V(o.dark)); for (auto& b : bl) { ell(c, b.x + h * 0.02f, b.y + h * 0.03f, b.r * 1.1f, b.r); c.fill(); }
    c.fillStyle(V(o.mid)); for (auto& b : bl) { ell(c, b.x, b.y, b.r, b.r * 0.9f); c.fill(); }
    c.fillStyle(V(o.light)); for (auto& b : bl) { float k = (b.x - cx) / rx + (b.y - cy) / ry; if (k < -0.15f) { ell(c, b.x - b.r * 0.25f, b.y - b.r * 0.3f, b.r * 0.62f, b.r * 0.5f); c.fill(); } }
    if (!o.spec.empty()) { c.fillStyle(V(o.spec)); for (int i = 0; i < 30; i++) { float a = r() * TAU, d = r(); float px = cx + std::cos(a) * rx * d, py = cy + std::sin(a) * ry * d; if (px + py < cx + cy) c.fillRect(px, py, 3, 2); } }
    if (!o.fruit.empty()) { c.fillStyle(V(o.fruit)); for (int i = 0; i < 7; i++) { float a = r() * TAU, d = r() * 0.8f; ell(c, cx + std::cos(a) * rx * d, cy + std::sin(a) * ry * d, 4, 4); c.fill(); } }
    c.restore();
}
void Palma(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned seme) {
    Caso r(seme); float lean = (r() - 0.5f) * 0.5f; float tx = x + h * lean * 0.5f, ty = y - h;
    for (int i = 0; i < 14; i++) {
        float k = i / 14.f, k2 = (i + 1) / 14.f;
        float ax = x + (tx - x) * k + std::sin(k * 3) * h * 0.05f, ay = y + (ty - y) * k, bx = x + (tx - x) * k2 + std::sin(k2 * 3) * h * 0.05f, by = y + (ty - y) * k2;
        c.strokeStyle(V(i % 2 ? o.trunk : sh(o.trunk, -0.2f))); c.lineWidth(h * 0.06f * (1 - k * 0.4f)); c.beginPath(); c.moveTo(ax, ay); c.lineTo(bx, by); c.stroke();
    }
    for (int i = 0; i < 8; i++) {
        float a = -TAU / 4 + (i - 3.5f) * 0.42f + (r() - 0.5f) * 0.2f, len = h * (0.45f + r() * 0.15f);
        float ex = tx + std::cos(a) * len, ey = ty + std::sin(a) * len * 0.5f + len * 0.35f;
        c.fillStyle(V(i % 2 ? o.dark : o.mid)); c.beginPath(); c.moveTo(tx, ty);
        c.quadraticCurveTo(tx + std::cos(a) * len * 0.6f, ty + std::sin(a) * len * 0.6f - h * 0.12f, ex, ey); c.quadraticCurveTo(tx + std::cos(a) * len * 0.5f, ty + std::sin(a) * len * 0.4f, tx, ty + 6); c.fill();
        c.strokeStyle(V(o.light)); c.lineWidth(2); c.beginPath(); c.moveTo(tx, ty); c.quadraticCurveTo(tx + std::cos(a) * len * 0.6f, ty + std::sin(a) * len * 0.6f - h * 0.12f, ex, ey); c.stroke();
    }
    c.fillStyle(V("#6a4a22")); for (int i = 0; i < 3; i++) { ell(c, tx - 6 + i * 6, ty + 8, 6, 6); c.fill(); }
}
void Felce(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned) {
    for (int i = 0; i < 7; i++) {
        float a = -TAU / 4 + (i - 3) * 0.32f; float ex = x + std::cos(a) * h, ey = y + std::sin(a) * h * 0.95f + std::fabs(i - 3.f) * h * 0.12f;
        c.strokeStyle(V(o.dark)); c.lineWidth(3); c.beginPath(); c.moveTo(x, y); c.quadraticCurveTo(x + std::cos(a) * h * 0.5f, y + std::sin(a) * h * 0.7f, ex, ey); c.stroke();
        for (int j = 1; j < 12; j++) {
            float k = j / 12.f, px = x + (ex - x) * k, py = y + (ey - y) * k - std::sin(k * TAU / 2) * h * 0.08f, lw = h * 0.12f * (1 - k * 0.7f);
            c.fillStyle(V(j % 2 ? o.mid : o.light)); ell(c, px - lw * 0.5f, py - lw * 0.2f, lw * 0.6f, lw * 0.18f, a - 0.9f); c.fill(); ell(c, px + lw * 0.5f, py - lw * 0.2f, lw * 0.6f, lw * 0.18f, a + 0.9f); c.fill();
        }
    }
}
void Cactus(K& c, float x, float y, float h, const ColoriAlbero& o) {
    float w = h * 0.16f;
    auto body = [&](float cx, float cy, float ww, float hh) {
        c.fillStyle(lg(cx - ww / 2, 0, cx + ww / 2, 0, { o.light, o.mid, o.dark })); c.beginPath(); c.moveTo(cx - ww / 2, cy); c.lineTo(cx - ww / 2, cy - hh + ww / 2);
        c.arc(cx, cy - hh + ww / 2, ww / 2, TAU / 2, TAU); c.lineTo(cx + ww / 2, cy); c.fill();
        c.strokeStyle(V(rgba(o.dark, 0.6f))); c.lineWidth(1.5f); for (int k = -1; k <= 1; k++) { c.beginPath(); c.moveTo(cx + k * ww * 0.22f, cy); c.lineTo(cx + k * ww * 0.22f, cy - hh + ww * 0.4f); c.stroke(); }
    };
    body(x, y, w, h);
    c.strokeStyle(V(o.mid)); c.lineWidth(w * 0.7f); c.beginPath(); c.moveTo(x - w * 0.4f, y - h * 0.45f); c.lineTo(x - w * 1.3f, y - h * 0.45f); c.lineTo(x - w * 1.3f, y - h * 0.7f); c.stroke(); body(x - w * 1.3f, y - h * 0.5f, w * 0.7f, h * 0.28f);
    c.beginPath(); c.moveTo(x + w * 0.4f, y - h * 0.6f); c.lineTo(x + w * 1.2f, y - h * 0.6f); c.stroke(); body(x + w * 1.2f, y - h * 0.55f, w * 0.7f, h * 0.25f);
}
void Fungo(K& c, float x, float y, float h, const ColoriFungo& o, unsigned seme) {
    Caso r(seme); float w = h * (0.55f + r() * 0.2f);
    c.fillStyle(lg(x - w * 0.1f, 0, x + w * 0.1f, 0, { o.stalkL, o.stalk, sh(o.stalk, -0.3f) }));
    c.beginPath(); c.moveTo(x - w * 0.12f, y); c.quadraticCurveTo(x - w * 0.06f, y - h * 0.5f, x - w * 0.08f, y - h * 0.9f); c.lineTo(x + w * 0.08f, y - h * 0.9f); c.quadraticCurveTo(x + w * 0.06f, y - h * 0.5f, x + w * 0.14f, y); c.fill();
    c.fillStyle(V(sh(o.stalk, -0.35f))); ell(c, x, y - h * 0.88f, w * 0.48f, h * 0.06f); c.fill();
    c.strokeStyle(V(rgba(sh(o.stalk, -0.5f), 0.5f))); c.lineWidth(1); for (int i = -8; i <= 8; i++) { c.beginPath(); c.moveTo(x, y - h * 0.86f); c.lineTo(x + i * w * 0.055f, y - h * 0.9f); c.stroke(); }
    c.fillStyle(rg(x - w * 0.15f, y - h * 1.08f, w * 0.05f, w * 0.6f, { sh(o.cap, 0.35f), o.cap, sh(o.cap, -0.35f) }));
    c.beginPath(); c.moveTo(x - w * 0.5f, y - h * 0.88f); c.bezierCurveTo(x - w * 0.5f, y - h * 1.25f, x + w * 0.5f, y - h * 1.25f, x + w * 0.5f, y - h * 0.88f); c.quadraticCurveTo(x, y - h * 0.93f, x - w * 0.5f, y - h * 0.88f); c.fill();
    c.fillStyle(V(o.dot)); for (int i = 0; i < 6; i++) { float dx = (r() - 0.5f) * w * 0.75f, dy = -h * (0.95f + r() * 0.18f); if (std::fabs(dx) < w * 0.45f) { ell(c, x + dx, y + dy, w * 0.05f + r() * w * 0.04f, w * 0.04f); c.fill(); } }
    c.fillStyle(V("rgba(255,255,255,0.35)")); ell(c, x - w * 0.2f, y - h * 1.1f, w * 0.12f, h * 0.03f, -0.4f); c.fill();
}
void Fiore(K& c, float x, float y, float h, const ColoriFiore& o, unsigned seme) {
    Caso r(seme);
    c.strokeStyle(V(o.stem)); c.lineWidth(h * 0.035f); c.beginPath(); c.moveTo(x, y); c.bezierCurveTo(x + h * 0.1f, y - h * 0.4f, x - h * 0.1f, y - h * 0.7f, x, y - h); c.stroke();
    c.fillStyle(V(o.leaf)); c.beginPath(); c.moveTo(x + 2, y - h * 0.35f); c.quadraticCurveTo(x + h * 0.25f, y - h * 0.5f, x + h * 0.3f, y - h * 0.3f); c.quadraticCurveTo(x + h * 0.15f, y - h * 0.28f, x + 2, y - h * 0.35f); c.fill();
    std::string pc = r() > 0.5f ? o.p1 : o.p2; float pr = h * 0.13f;
    for (int i = 0; i < 7; i++) { float a = i / 7.f * TAU; c.fillStyle(lg(x, y - h, x + std::cos(a) * pr * 2, y - h + std::sin(a) * pr * 2, { sh(pc, -0.2f), pc, sh(pc, 0.3f) })); ell(c, x + std::cos(a) * pr, y - h + std::sin(a) * pr, pr, pr * 0.55f, a); c.fill(); }
    c.fillStyle(rg(x - 3, y - h - 3, 1, pr * 0.6f, { "#fff6b0", "#f2a81a" })); ell(c, x, y - h, pr * 0.55f, pr * 0.55f); c.fill();
}
void Cristalli(K& c, float x, float y, float h, const ColoriCristallo& o, unsigned seme) {
    Caso r(seme); int n = 3 + (int)(r() * 3);
    for (int i = 0; i < n; i++) {
        float hh = h * (0.45f + r() * 0.6f), w = hh * 0.22f, a = (r() - 0.5f) * 0.7f, bx = x + (r() - 0.5f) * h * 0.4f;
        c.save(); c.translate(bx, y); c.rotate(a);
        c.fillStyle(V(o.dark)); c.beginPath(); c.moveTo(-w, 0); c.lineTo(-w, -hh * 0.75f); c.lineTo(0, -hh); c.lineTo(w, -hh * 0.75f); c.lineTo(w, 0); c.fill();
        c.fillStyle(V(o.mid)); c.beginPath(); c.moveTo(-w, 0); c.lineTo(-w, -hh * 0.75f); c.lineTo(0, -hh); c.lineTo(0, 0); c.fill();
        c.fillStyle(V(o.light)); c.beginPath(); c.moveTo(-w * 0.6f, -hh * 0.1f); c.lineTo(-w * 0.6f, -hh * 0.72f); c.lineTo(-w * 0.15f, -hh * 0.9f); c.lineTo(-w * 0.15f, -hh * 0.2f); c.fill();
        if (!o.glow.empty()) { c.lighter(); c.fillStyle(rg(0, -hh * 0.5f, 0, hh * 0.8f, { rgba(o.glow, 0.35f), rgba(o.glow, 0) })); c.fillRect(-hh, -hh * 1.3f, hh * 2, hh * 1.6f); c.normal(); }
        c.restore();
    }
}
void Edificio(K& c, float x, float y, float w, float h, const ColoriEdificio& o, unsigned seme) {
    Caso r(seme);
    c.fillStyle(lg(x, 0, x + w, 0, { sh(o.wall, 0.12f), o.wall, sh(o.wall, -0.25f) })); c.fillRect(x, y - h, w, h);
    if (o.brick) { c.fillStyle(V(rgba("#000000", 0.12f))); for (float yy = y - h + 6; yy < y; yy += 10) c.fillRect(x, yy, w, 1.5f); }
    if (o.roof == "pitch") {
        c.fillStyle(lg(0, y - h - w * 0.4f, 0, y - h, { sh(o.roofc, 0.15f), o.roofc })); c.beginPath(); c.moveTo(x - 8, y - h + 2); c.lineTo(x + w * 0.5f, y - h - w * 0.42f); c.lineTo(x + w + 8, y - h + 2); c.fill();
        c.strokeStyle(V(rgba("#000000", 0.25f))); c.lineWidth(1.5f);
        for (int k = 1; k < 6; k++) { float yy = y - h + 2 - w * 0.42f * k / 6; c.beginPath(); c.moveTo(x + w * 0.5f - (w * 0.5f + 8) * (1 - k / 6.f), yy); c.lineTo(x + w * 0.5f + (w * 0.5f + 8) * (1 - k / 6.f), yy); c.stroke(); }
    } else if (o.roof == "flat") { c.fillStyle(V(sh(o.wall, -0.3f))); c.fillRect(x - 3, y - h - 6, w + 6, 7); }
    else if (o.roof == "spire") {
        c.fillStyle(V(sh(o.wall, 0.05f))); c.beginPath(); c.moveTo(x + w * 0.2f, y - h); c.lineTo(x + w * 0.5f, y - h - w * 1.2f); c.lineTo(x + w * 0.8f, y - h); c.fill();
        c.fillStyle(V(o.neon.empty() ? "#ff4ad8" : o.neon)); c.fillRect(x + w * 0.5f - 1, y - h - w * 1.2f - 14, 2, 14);
    }
    if (o.chim && r() > 0.4f) { float cx = x + w * 0.68f; c.fillStyle(V(sh(o.wall, -0.1f))); c.fillRect(cx, y - h - w * 0.38f, w * 0.12f, w * 0.3f); c.fillStyle(V(sh(o.wall, -0.35f))); c.fillRect(cx - 2, y - h - w * 0.4f, w * 0.12f + 4, 5); }
    for (float yy = y - h + 14; yy < y - o.wh - 6; yy += o.gy)
        for (float xx = x + 8; xx < x + w - o.ww - 4; xx += o.gx) {
            float v = r(); bool lit = v < o.lit; std::string wc = (!o.win2.empty() && v < o.lit / 2) ? o.win2 : o.win;
            if (lit) { c.fillStyle(V(rgba(wc, 0.25f))); c.fillRect(xx - 4, yy - 4, o.ww + 8, o.wh + 8); }
            c.fillStyle(lit ? lg(0, yy, 0, yy + o.wh, { sh(wc, 0.4f), wc }) : V(o.dark)); c.fillRect(xx, yy, o.ww, o.wh);
            if (!o.frame.empty()) { c.fillStyle(V(o.frame)); c.fillRect(xx + o.ww / 2 - 0.75f, yy, 1.5f, o.wh); c.fillRect(xx, yy + o.wh / 2 - 0.75f, o.ww, 1.5f); c.fillRect(xx - 2, yy + o.wh, o.ww + 4, 2); }
        }
    if (!o.neon.empty() && r() > 0.5f) { c.save(); c.glow(12, o.neon); c.fillStyle(V(o.neon)); c.fillRect(x + 6, y - h * 0.6f, w - 12, 5); c.restore(); }
}
void Castello(K& c, float x, float y, float s, const ColoriCastello& o) {
    auto stone = [&](float x0, float y0, float w, float h) {
        c.fillStyle(lg(x0, 0, x0 + w, 0, { sh(o.stone, 0.15f), o.stone, sh(o.stone, -0.3f) })); c.fillRect(x0, y0, w, h);
        c.fillStyle(V(rgba("#000000", 0.15f)));
        for (float yy = y0 + 8; yy < y0 + h; yy += 9) { c.fillRect(x0, yy, w, 1); for (float xx = x0 + (((int)(yy / 9)) % 2) * 8; xx < x0 + w; xx += 16) c.fillRect(xx, yy - 8, 1, 8); }
    };
    auto crenel = [&](float x0, float y0, float w) { c.fillStyle(V(o.stone)); for (float xx = x0; xx < x0 + w - 4; xx += 12) c.fillRect(xx, y0 - 9, 7, 10); };
    stone(x, y - s * 0.5f, s * 1.4f, s * 0.5f); crenel(x, y - s * 0.5f, s * 1.4f);
    float towers[3][3] = { { x - s * 0.15f, s * 0.95f, s * 0.3f }, { x + s * 0.5f, s * 1.3f, s * 0.38f }, { x + s * 1.25f, s * 0.85f, s * 0.3f } };
    for (auto& tw3 : towers) {
        float tx = tw3[0], th = tw3[1], tw = tw3[2];
        stone(tx, y - th, tw, th);
        c.fillStyle(lg(tx, 0, tx + tw, 0, { sh(o.roof, 0.2f), o.roof, sh(o.roof, -0.3f) })); c.beginPath(); c.moveTo(tx - 6, y - th); c.lineTo(tx + tw / 2, y - th - tw * 1.3f); c.lineTo(tx + tw + 6, y - th); c.fill();
        c.strokeStyle(V("#3a2a2a")); c.lineWidth(2); c.beginPath(); c.moveTo(tx + tw / 2, y - th - tw * 1.3f); c.lineTo(tx + tw / 2, y - th - tw * 1.3f - 22); c.stroke();
        c.fillStyle(V(o.flag)); c.beginPath(); c.moveTo(tx + tw / 2, y - th - tw * 1.3f - 22); c.lineTo(tx + tw / 2 + 18, y - th - tw * 1.3f - 17); c.lineTo(tx + tw / 2, y - th - tw * 1.3f - 12); c.fill();
        for (int k = 0; k < 2; k++) {
            float wy = y - th + 20 + k * th * 0.35f;
            c.fillStyle(V(o.win.empty() ? "#1a1420" : rgba(o.win, 0.3f))); c.fillRect(tx + tw / 2 - 7, wy - 3, 14, 22);
            c.fillStyle(V(o.win.empty() ? "#1a1420" : o.win)); c.beginPath(); c.moveTo(tx + tw / 2 - 4, wy + 16); c.lineTo(tx + tw / 2 - 4, wy + 4); c.arc(tx + tw / 2, wy + 4, 4, TAU / 2, TAU); c.lineTo(tx + tw / 2 + 4, wy + 16); c.fill();
        }
    }
    c.fillStyle(V("#22161a")); c.beginPath(); c.moveTo(x + s * 0.58f, y); c.lineTo(x + s * 0.58f, y - s * 0.22f); c.arc(x + s * 0.69f, y - s * 0.22f, s * 0.11f, TAU / 2, TAU); c.lineTo(x + s * 0.8f, y); c.fill();
    c.strokeStyle(V("#5a4a3a")); c.lineWidth(1.5f); for (int k = 0; k < 5; k++) { c.beginPath(); c.moveTo(x + s * 0.6f + k * s * 0.045f, y - s * 0.3f); c.lineTo(x + s * 0.6f + k * s * 0.045f, y); c.stroke(); }
    c.fillStyle(V(o.banner.empty() ? "#b8323a" : o.banner)); c.fillRect(x + s * 0.22f, y - s * 0.46f, s * 0.1f, s * 0.22f);
    c.fillStyle(V("#f2c94a")); ell(c, x + s * 0.27f, y - s * 0.36f, s * 0.025f, s * 0.025f); c.fill();
}
void Piramide(K& c, float x, float y, float w, const std::string& L, const std::string& D) {
    float h = w * 0.62f;
    c.fillStyle(V(L)); c.beginPath(); c.moveTo(x, y); c.lineTo(x + w * 0.5f, y - h); c.lineTo(x + w * 0.62f, y); c.fill();
    c.fillStyle(V(D)); c.beginPath(); c.moveTo(x + w * 0.5f, y - h); c.lineTo(x + w, y); c.lineTo(x + w * 0.62f, y); c.fill();
    c.strokeStyle(V(rgba("#5a3a10", 0.3f))); c.lineWidth(1.2f);
    for (int k = 1; k < 16; k++) { float yy = y - h * k / 16, half = w * 0.5f * (1 - k / 16.f); c.beginPath(); c.moveTo(x + w * 0.5f - half, yy); c.lineTo(x + w * 0.5f + half, yy); c.stroke(); }
    c.fillStyle(V("#ffe27a")); c.beginPath(); c.moveTo(x + w * 0.5f, y - h); c.lineTo(x + w * 0.5f - w * 0.04f, y - h * 0.94f); c.lineTo(x + w * 0.5f + w * 0.04f, y - h * 0.94f); c.fill();
}
void Colonna(K& c, float x, float y, float h, float w, const std::string& col, const std::string& fascia) {
    c.fillStyle(lg(x, 0, x + w, 0, { sh(col, 0.25f), col, sh(col, -0.35f) })); c.fillRect(x, y - h, w, h);
    c.strokeStyle(V(rgba("#000000", 0.15f))); c.lineWidth(1.5f); for (int k = 1; k < 5; k++) { c.beginPath(); c.moveTo(x + w * k / 5, y - h + 10); c.lineTo(x + w * k / 5, y - 6); c.stroke(); }
    c.fillStyle(V(sh(col, 0.1f))); c.fillRect(x - w * 0.2f, y - h - 10, w * 1.4f, 12); c.fillRect(x - w * 0.15f, y - 8, w * 1.3f, 8);
    if (!fascia.empty()) { c.fillStyle(V(fascia)); c.fillRect(x, y - h * 0.7f, w, 6); }
}
void Ingranaggio(K& c, float x, float y, float R0, const std::string& col, const std::string& buco, float rot) {
    c.save(); c.translate(x, y); c.rotate(rot); c.beginPath();
    int n = std::max(8, (int)std::lround(R0 / 6));
    for (int i = 0; i < n; i++) {
        float a = i / (float)n * TAU, a2 = (i + 0.5f) / n * TAU;
        c.lineTo(std::cos(a - 0.12f) * R0 * 1.12f, std::sin(a - 0.12f) * R0 * 1.12f); c.lineTo(std::cos(a + 0.12f) * R0 * 1.12f, std::sin(a + 0.12f) * R0 * 1.12f);
        c.lineTo(std::cos(a2 - 0.12f) * R0 * 0.92f, std::sin(a2 - 0.12f) * R0 * 0.92f); c.lineTo(std::cos(a2 + 0.12f) * R0 * 0.92f, std::sin(a2 + 0.12f) * R0 * 0.92f);
    }
    c.closePath(); c.fillStyle(lg(-R0, -R0, R0, R0, { sh(col, 0.35f), col, sh(col, -0.4f) })); c.fill();
    c.fillStyle(V(sh(col, -0.25f))); ell(c, 0, 0, R0 * 0.6f, R0 * 0.6f); c.fill();
    c.fillStyle(lg(-R0, -R0, R0, R0, { sh(col, 0.2f), sh(col, -0.2f) })); ell(c, 0, 0, R0 * 0.5f, R0 * 0.5f); c.fill();
    c.fillStyle(V(buco)); for (int i = 0; i < 5; i++) { float a = i / 5.f * TAU; ell(c, std::cos(a) * R0 * 0.3f, std::sin(a) * R0 * 0.3f, R0 * 0.09f, R0 * 0.09f); c.fill(); }
    ell(c, 0, 0, R0 * 0.12f, R0 * 0.12f); c.fill(); c.restore();
}
void Tubi(K& c, float y0, const std::string& col, unsigned seme) {
    Caso r(seme);
    for (int k = 0; k < 6; k++) {
        float yy = y0 + k * 34 + r() * 20, th = 10 + r() * 8;
        c.fillStyle(lg(0, yy, 0, yy + th, { sh(col, 0.3f), col, sh(col, -0.4f) })); c.fillRect(0, yy, LS, th);
        for (float x = r() * 200; x < LS; x += 120 + r() * 200) { c.fillStyle(V(sh(col, -0.2f))); c.fillRect(x, yy - 3, 12, th + 6); c.fillStyle(V(sh(col, 0.3f))); c.fillRect(x + 2, yy - 2, 3, th + 4); }
    }
}
void Ciminiera(K& c, float x, float y, float h, float w, const std::string& col, const std::string& fascia) {
    c.fillStyle(lg(x, 0, x + w, 0, { sh(col, 0.2f), col, sh(col, -0.35f) })); c.fillRect(x, y - h, w, h);
    c.fillStyle(V(fascia)); for (int k = 1; k < 4; k++) c.fillRect(x, y - h + k * h / 4, w, 5);
    c.fillStyle(V(sh(col, -0.2f))); c.fillRect(x - 4, y - h - 6, w + 8, 8);
}
void Mesa(K& c, float x, float y, float w, float h, const std::string& L, const std::string& M, const std::string& D, const std::string& band, const std::string& top) {
    c.beginPath(); c.moveTo(x, y); c.lineTo(x + w * 0.12f, y - h); c.lineTo(x + w * 0.88f, y - h); c.lineTo(x + w, y); c.closePath();
    c.fillStyle(lg(x, 0, x + w, 0, { L, M, D })); c.fill();
    c.save(); c.clip();
    for (int k = 0; k < 8; k++) { c.fillStyle(V(k % 2 ? rgba(band, 0.5f) : rgba("#ffffff", 0.08f))); c.fillRect(x, y - h + k * h / 8 + ((k * 37) % 9), w, h / 16); }
    c.fillStyle(V(rgba("#000000", 0.25f))); c.fillRect(x + w * 0.62f, y - h, w * 0.4f, h); c.restore();
    c.fillStyle(V(top)); c.fillRect(x + w * 0.12f, y - h, w * 0.76f, 4);
}
void Isola(K& c, float x, float y, float w, const std::string& dirt, const std::string& grass, const ColoriAlbero& albero, unsigned seme) {
    Caso r(seme);
    c.fillStyle(lg(0, y, 0, y + w * 0.55f, { dirt, sh(dirt, -0.45f) })); c.beginPath(); c.moveTo(x - w / 2, y);
    for (int i = 0; i <= 8; i++) { float k = i / 8.f; c.lineTo(x - w / 2 + w * k, y + std::sin(k * TAU / 2) * w * (0.3f + 0.25f * r()) + (i % 2) * 8); }
    c.lineTo(x + w / 2, y); c.fill();
    c.strokeStyle(V(sh(dirt, -0.3f))); c.lineWidth(2);
    for (int i = 0; i < 4; i++) { float rx = x + (r() - 0.5f) * w * 0.5f; c.beginPath(); c.moveTo(rx, y + w * 0.25f); c.quadraticCurveTo(rx + 8, y + w * 0.4f, rx - 4, y + w * 0.55f + r() * 20); c.stroke(); }
    c.fillStyle(lg(0, y - 8, 0, y + 10, { sh(grass, 0.25f), grass, sh(grass, -0.3f) })); c.beginPath(); c.ellipse(x, y, w / 2 + 4, 10, 0, 0, TAU); c.fill();
    c.fillStyle(V(sh(grass, 0.3f))); for (int i = 0; i < w / 5; i++) { float gx = x - w / 2 + i * 5; c.fillRect(gx, y - 12 + std::sin((float)i) * 2, 2, 7); }
    if (r() > 0.3f) AlberoTondo(c, x + (r() - 0.5f) * w * 0.3f, y, w * 0.7f, albero, seme + 9);
}
void Corallo(K& c, float x, float y, float h, const std::string& col, unsigned seme) {
    Caso r(seme);
    std::function<void(float, float, float, float, float, int)> br = [&](float x0, float y0, float a, float len, float w, int d) {
        float x1 = x0 + std::cos(a) * len, y1 = y0 + std::sin(a) * len;
        c.strokeStyle(V(d % 2 ? col : sh(col, 0.15f))); c.lineWidth(w); c.beginPath(); c.moveTo(x0, y0); c.lineTo(x1, y1); c.stroke();
        if (d < 4) { br(x1, y1, a - 0.4f - r() * 0.3f, len * 0.72f, w * 0.72f, d + 1); br(x1, y1, a + 0.4f + r() * 0.3f, len * 0.72f, w * 0.72f, d + 1); }
        else { c.fillStyle(V(sh(col, 0.4f))); ell(c, x1, y1, w * 0.8f, w * 0.8f); c.fill(); }
    };
    br(x, y, -TAU / 4, h * 0.35f, h * 0.08f, 0);
}
void Alga(K& c, float x, float y, float h, const std::string& col, unsigned seme) {
    float sd = (float)(seme % 100);
    c.strokeStyle(V(col)); c.lineWidth(5); c.beginPath(); c.moveTo(x, y);
    for (int k = 1; k <= 12; k++) c.lineTo(x + std::sin(k * 0.9f + sd) * 10, y - h * k / 12); c.stroke();
    c.fillStyle(V(sh(col, 0.2f)));
    for (int k = 1; k < 12; k++) { float px = x + std::sin(k * 0.9f + sd) * 10, py = y - h * k / 12; ell(c, px + (k % 2 ? 10.f : -10.f), py, 10, 4, k % 2 ? 0.5f : -0.5f); c.fill(); }
}
void Cubo(K& c, float x, float y, float s, const std::string& col) {
    c.fillStyle(V(sh(col, 0.35f))); c.beginPath(); c.moveTo(x, y - s); c.lineTo(x + s * 0.3f, y - s * 1.25f); c.lineTo(x + s * 1.3f, y - s * 1.25f); c.lineTo(x + s, y - s); c.fill();
    c.fillStyle(V(sh(col, -0.3f))); c.beginPath(); c.moveTo(x + s, y); c.lineTo(x + s, y - s); c.lineTo(x + s * 1.3f, y - s * 1.25f); c.lineTo(x + s * 1.3f, y - s * 0.25f); c.fill();
    c.fillStyle(V(col)); c.fillRect(x, y - s, s, s); c.fillStyle(V("#ffffff")); c.fillRect(x + s * 0.12f, y - s * 0.88f, s * 0.76f, s * 0.76f);
    c.fillStyle(V(col)); c.beginPath(); for (int i = 0; i < 10; i++) { float a = -TAU / 4 + i * TAU / 10, rr = i % 2 ? s * 0.13f : s * 0.3f; c.lineTo(x + s / 2 + std::cos(a) * rr, y - s / 2 + std::sin(a) * rr); } c.closePath(); c.fill();
}
void Nuvola(K& c, float x, float y, float w, const ColoriNuvola& o, unsigned seme) {
    Caso r(seme); struct P { float x, y, r; }; std::vector<P> pts;
    for (int i = 0; i < 14; i++) { float u = r(); float yy = y - std::sin(u * TAU / 2) * w * 0.22f * r() - r() * w * 0.05f; pts.push_back({ x + (u - 0.5f) * w, yy, w * (0.08f + 0.1f * r() * std::sin(u * TAU / 2) + 0.04f) }); }
    c.fillStyle(V(o.shadow)); for (auto& p : pts) { ell(c, p.x + 4, p.y + 8, p.r, p.r * 0.85f); c.fill(); }
    c.fillStyle(V(o.base)); for (auto& p : pts) { ell(c, p.x, p.y, p.r, p.r * 0.85f); c.fill(); }
    c.fillStyle(V(o.light)); for (auto& p : pts) if (p.y < y - w * 0.04f) { ell(c, p.x - p.r * 0.2f, p.y - p.r * 0.25f, p.r * 0.7f, p.r * 0.55f); c.fill(); }
}
void Nave(K& c, float x, float y, float s) {
    c.save(); c.translate(x, y); c.scale(s, s);
    c.fillStyle(lg(0, -40, 0, 20, { "#8a5a32", "#4a2a14" })); c.beginPath(); c.moveTo(-160, -44); c.lineTo(150, -44); c.lineTo(180, -80); c.lineTo(190, -80); c.lineTo(160, 10); c.lineTo(-130, 10); c.lineTo(-170, -70); c.lineTo(-160, -70); c.closePath(); c.fill();
    c.fillStyle(V("#f2c23a")); c.fillRect(-150, -36, 290, 4); c.fillStyle(V("#2a1a0a")); for (int k = 0; k < 7; k++) { ell(c, -110 + k * 38.f, -18, 7, 7); c.fill(); }
    c.fillStyle(V("#3a2414")); c.fillRect(-60, -260, 7, 220); c.fillRect(60, -220, 7, 180); c.fillRect(-140, -140, 5, 100);
    auto sail = [&](float x0, float y0, float w, float h) {
        c.fillStyle(lg(x0, 0, x0 + w, 0, { "#f4ecd8", "#d8ccb0" })); c.beginPath(); c.moveTo(x0, y0); c.quadraticCurveTo(x0 + w / 2, y0 + 12, x0 + w, y0); c.quadraticCurveTo(x0 + w + 14, y0 + h / 2, x0 + w, y0 + h);
        c.quadraticCurveTo(x0 + w / 2, y0 + h + 10, x0, y0 + h); c.quadraticCurveTo(x0 + 14, y0 + h / 2, x0, y0); c.fill(); c.strokeStyle(V("rgba(120,90,60,0.4)")); c.lineWidth(1.5f); c.stroke();
    };
    sail(-110, -250, 110, 90); sail(-100, -150, 96, 80); sail(14, -210, 100, 80); sail(20, -120, 92, 66); sail(-170, -130, 40, 70);
    c.fillStyle(V("#1a1a1a")); c.fillRect(-53, -262, 36, 22); c.fillStyle(V("#ffffff")); ell(c, -35, -253, 5, 5); c.fill();
    c.strokeStyle(V("rgba(60,40,20,0.7)")); c.lineWidth(1.5f); c.beginPath(); c.moveTo(-57, -260); c.lineTo(-170, -46); c.moveTo(-57, -260); c.lineTo(63, -220); c.lineTo(185, -80); c.stroke();
    c.restore();
}

// --- cielo ---------------------------------------------------------------------------------------
void DisegnaCielo(K& c, const Cielo& S) {
    const float W = 1280, H = 720;
    {
        Vernice v; v.tipo = 1; v.x0 = 0; v.y0 = 0; v.x1 = 0; v.y1 = H; int n = (int)S.fermate.size();
        for (int i = 0; i < n; i++) v.fermate.push_back({ n > 1 ? (float)i / (n - 1) : 0.f, Col(S.fermate[i]) });
        c.fillStyle(v); c.fillRect(0, 0, W, H);
    }
    if (S.stelle) {
        Caso r(77);
        for (int i = 0; i < 220; i++) {
            float x = r() * W, y = r() * H * 0.62f, z = r(); float a = (0.45f + 0.55f * r()) * (1 - y / (H * 0.7f));
            c.fillStyle(V(rgba("#fffaeb", a))); float sz = z > 0.93f ? 2.6f : (z > 0.7f ? 1.6f : 1.f); c.fillRect(x, y, sz, sz);
            if (z > 0.97f) { c.fillRect(x - 4, y + 0.8f, 10, 1); c.fillRect(x + 0.8f, y - 4, 1, 10); }
        }
    }
    if (S.aurora) {
        for (int k = 0; k < 4; k++) {
            c.save(); c.lighter(); std::string col = k % 2 ? "#9a5aff" : "#46ffaa";
            for (float x = 0; x < W; x += 6) {
                float y0 = 60 + k * 34 + std::sin(x * 0.004f + k * 1.7f) * 40 + std::sin(x * 0.011f) * 12, h = 120 + std::sin(x * 0.007f + k) * 60;
                c.fillStyle(lgs(0, y0, 0, y0 + h, { { 0, rgba(col, 0) }, { 0.33f, rgba(col, 0.1f) }, { 0.66f, rgba(col, 0.02f) }, { 1, rgba(col, 0) } })); c.fillRect(x, y0, 6.05f, h);
            }
            c.restore();
        }
    }
    if (S.sole[2] > 0) {
        float x = S.sole[0], y = S.sole[1], r = S.sole[2]; const std::string& col = S.coloreSole;
        c.save(); c.lighter(); c.fillStyle(rgs(x, y, 0, r * 9, { { 0, rgba(col, 0.55f) }, { 0.15f, rgba(col, 0.25f) }, { 1, rgba(col, 0) } })); c.fillRect(0, 0, W, H); c.restore();
        c.fillStyle(rg(x - r * 0.2f, y - r * 0.2f, 0, r, { "#fffef6", col })); ell(c, x, y, r, r); c.fill();
        if (S.raggi) {
            c.save(); c.lighter();
            for (int i = 0; i < 9; i++) {
                float a = i / 9.f * TAU / 2 + 0.15f;
                c.fillStyle(lg(x, y, x + std::cos(a) * 900, y + std::sin(a) * 900, { rgba(col, 0.12f), rgba(col, 0) }));
                c.beginPath(); c.moveTo(x, y); c.lineTo(x + std::cos(a - 0.05f) * 1100, y + std::sin(a - 0.05f) * 1100); c.lineTo(x + std::cos(a + 0.05f) * 1100, y + std::sin(a + 0.05f) * 1100); c.fill();
            }
            c.restore();
        }
    }
    if (S.luna[2] > 0) {
        float x = S.luna[0], y = S.luna[1], r = S.luna[2]; const std::string& col = S.coloreLuna;
        c.save(); c.lighter(); c.fillStyle(rg(x, y, r, r * 7, { rgba(col, 0.35f), rgba(col, 0) })); c.fillRect(0, 0, W, H); c.restore();
        c.fillStyle(rg(x - r * 0.4f, y - r * 0.4f, 0, r * 1.2f, { "#ffffff", col })); ell(c, x, y, r, r); c.fill();
        Caso rr(5); c.fillStyle(V(rgba(sh(col, -0.35f), 0.35f)));
        for (int i = 0; i < 6; i++) { float a = rr() * TAU, d = rr() * r * 0.6f; ell(c, x + std::cos(a) * d, y + std::sin(a) * d, r * (0.08f + rr() * 0.14f), r * (0.08f + rr() * 0.12f)); c.fill(); }
    }
    if (S.eclissi[2] > 0) {
        float x = S.eclissi[0], y = S.eclissi[1], r = S.eclissi[2];
        c.save(); c.lighter();
        for (int i = 0; i < 48; i++) {
            float a = i / 48.f * TAU, len = r * (1.4f + 0.5f * std::sin(i * 3.7f));
            c.strokeStyle(V(rgba(i % 3 == 0 ? "#ff965a" : (i % 3 == 1 ? "#ffb45a" : "#ffd25a"), 0.16f))); c.lineWidth(6);
            c.beginPath(); c.moveTo(x + std::cos(a) * r, y + std::sin(a) * r); c.lineTo(x + std::cos(a) * len, y + std::sin(a) * len); c.stroke();
        }
        c.fillStyle(rg(x, y, r * 0.9f, r * 3.2f, { "rgba(255,170,90,0.75)", "rgba(200,60,160,0.25)", "rgba(80,0,120,0)" })); c.fillRect(0, 0, W, H); c.restore();
        c.fillStyle(rg(x + r * 0.2f, y + r * 0.2f, 0, r, { "#14081e", "#05020a" })); ell(c, x, y, r, r); c.fill();
        c.strokeStyle(V("rgba(255,230,190,0.95)")); c.lineWidth(3); c.stroke();
    }
    unsigned sd = 3;
    for (auto& n : S.nuvole) { Caso r(sd); Nuvola(c, r() * W, n.first, 180 + r() * 160, n.second, sd); sd += 7; }
}

// --- materiali -----------------------------------------------------------------------------------
void DisegnaMateriale(K& c, float x0, float w, float y0, float h, const Materiale& m, unsigned seme) {
    Caso r(seme);
    c.save(); c.beginPath(); c.rect(x0, y0 - 40, w, h + 40); c.clip();
    c.fillStyle(lg(0, y0, 0, y0 + h, { m.body, sh(m.body, -0.5f) })); c.fillRect(x0, y0, w, h);
    for (int i = 0; i < w * h / 500; i++) {
        float x = x0 + r() * w, y = y0 + 16 + r() * (h - 10), s = 3 + r() * 10;
        c.fillStyle(V(r() > 0.5f ? rgba(sh(m.body, 0.2f), 0.5f) : rgba(sh(m.body, -0.35f), 0.5f))); ell(c, x, y, s, s * 0.6f, r()); c.fill();
        if (s > 9) { c.fillStyle(V(rgba("#ffffff", 0.12f))); ell(c, x - s * 0.3f, y - s * 0.25f, s * 0.4f, s * 0.2f); c.fill(); }
    }
    const std::string& k = m.kind;
    if (k == "grass" || k == "nightgrass") {
        c.fillStyle(lg(0, y0, 0, y0 + 26, { m.top, sh(m.top, -0.3f) })); c.beginPath(); c.moveTo(x0, y0);
        for (float x = x0; x <= x0 + w; x += 10) c.lineTo(x, y0 + 18 + std::sin(x * 0.13f) * 4 + r() * 6);
        c.lineTo(x0 + w, y0); c.fill();
        c.fillStyle(V(rgba("#000000", 0.25f))); c.fillRect(x0, y0 + 20, w, 3);
        for (float x = x0; x < x0 + w; x += 2.5f) {
            float bh = 6 + r() * 14; const std::string& col = m.blades[(size_t)(r() * m.blades.size()) % m.blades.size()];
            c.fillStyle(V(col)); c.beginPath(); c.moveTo(x, y0 + 4); c.lineTo(x + 1.5f + r() * 2, y0 - bh); c.lineTo(x + 3, y0 + 4); c.fill();
        }
        if (!m.flowers.empty())
            for (int i = 0; i < w / 60; i++) {
                float x = x0 + r() * w; c.strokeStyle(V(sh(m.top, -0.2f))); c.lineWidth(1.5f); c.beginPath(); c.moveTo(x, y0 + 2); c.lineTo(x, y0 - 12); c.stroke();
                c.fillStyle(V(m.flowers[i % m.flowers.size()])); for (int p = 0; p < 5; p++) { ell(c, x + std::cos(p * 1.25f) * 3, y0 - 12 + std::sin(p * 1.25f) * 3, 2.4f, 2.4f); c.fill(); }
                c.fillStyle(V("#fff3a0")); ell(c, x, y0 - 12, 1.5f, 1.5f); c.fill();
            }
    } else if (k == "snow") {
        c.fillStyle(lg(0, y0 - 6, 0, y0 + 30, { "#ffffff", "#dcecf8", "#a9c8e2" })); c.beginPath(); c.moveTo(x0, y0 + 30);
        for (float x = x0; x <= x0 + w; x += 8) c.lineTo(x, y0 - 4 + std::sin(x * 0.05f) * 4 + std::sin(x * 0.17f) * 2);
        c.lineTo(x0 + w, y0 + 30); for (float x = x0 + w; x >= x0; x -= 12) c.lineTo(x, y0 + 26 + r() * 8); c.fill();
        c.fillStyle(V("rgba(255,255,255,0.9)")); for (int i = 0; i < w / 14; i++) c.fillRect(x0 + r() * w, y0 + r() * 20, 1.6f, 1.6f);
        c.fillStyle(V("#d8f0ff")); for (float x = x0 + 6; x < x0 + w; x += 14 + r() * 20) { float ih = 8 + r() * 18; c.beginPath(); c.moveTo(x - 4, y0 + 30); c.lineTo(x, y0 + 30 + ih); c.lineTo(x + 4, y0 + 30); c.fill(); }
    } else if (k == "sand") {
        c.fillStyle(lg(0, y0, 0, y0 + 30, { m.top, sh(m.top, -0.15f) })); c.fillRect(x0, y0, w, 30);
        c.strokeStyle(V(rgba(sh(m.top, -0.3f), 0.5f))); c.lineWidth(1.5f);
        for (int i = 0; i < w / 18; i++) { float x = x0 + r() * w, y = y0 + 6 + r() * 22; c.beginPath(); c.moveTo(x, y); c.quadraticCurveTo(x + 10, y - 3, x + 22, y); c.stroke(); }
        c.fillStyle(V(rgba("#ffffff", 0.5f))); c.fillRect(x0, y0, w, 2);
    } else if (k == "stone") {
        const float bw = 60, bh = 30;
        for (int row = 0; row * bh < h; row++) {
            float yy = y0 + row * bh;
            for (float x = x0 - ((row % 2) * bw / 2); x < x0 + w; x += bw) {
                float v = (r() - 0.5f) * 0.16f; std::string base = sh(m.top, v - row * 0.12f);
                c.fillStyle(lg(0, yy, 0, yy + bh, { sh(base, 0.08f), sh(base, -0.12f) })); c.fillRect(x + 2, yy + 2, bw - 4, bh - 4);
                c.fillStyle(V(rgba("#ffffff", 0.22f))); c.fillRect(x + 2, yy + 2, bw - 4, 3); c.fillRect(x + 2, yy + 2, 3, bh - 4);
                c.fillStyle(V(rgba("#000000", 0.3f))); c.fillRect(x + 2, yy + bh - 5, bw - 4, 3); c.fillRect(x + bw - 5, yy + 2, 3, bh - 4);
                if (r() > 0.8f) { c.strokeStyle(V(rgba("#000000", 0.3f))); c.lineWidth(1); c.beginPath(); c.moveTo(x + 10 + r() * 30, yy + 5); c.lineTo(x + 20 + r() * 20, yy + 14); c.lineTo(x + 14 + r() * 30, yy + 24); c.stroke(); }
                if (!m.moss.empty() && row == 0 && r() > 0.5f) { c.fillStyle(V(m.moss)); ell(c, x + r() * bw, yy + 3, 8 + r() * 10, 4); c.fill(); }
            }
        }
    } else if (k == "metal") {
        for (float x = x0; x < x0 + w; x += 96) {
            c.fillStyle(lg(x, y0, x + 96, y0 + h, { sh(m.top, 0.25f), m.top, sh(m.top, -0.35f) })); c.fillRect(x + 1, y0 + 12, 94, h);
            c.fillStyle(V(rgba("#ffffff", 0.35f))); c.fillRect(x + 1, y0 + 12, 94, 2); c.fillStyle(V(sh(m.top, -0.5f))); c.fillRect(x + 95, y0 + 12, 1, h);
            c.fillStyle(V(sh(m.top, 0.4f))); for (auto p : { std::make_pair(8.f, 22.f), std::make_pair(86.f, 22.f), std::make_pair(8.f, h - 8), std::make_pair(86.f, h - 8) }) { ell(c, x + p.first, y0 + p.second, 3, 3); c.fill(); }
        }
        for (float x = x0; x < x0 + w; x += 24) {
            c.fillStyle(V(m.stripe1)); c.beginPath(); c.moveTo(x, y0 + 12); c.lineTo(x + 12, y0); c.lineTo(x + 24, y0); c.lineTo(x + 12, y0 + 12); c.fill();
            c.fillStyle(V(m.stripe2)); c.beginPath(); c.moveTo(x + 12, y0 + 12); c.lineTo(x + 24, y0); c.lineTo(x + 36, y0); c.lineTo(x + 24, y0 + 12); c.fill();
        }
        if (!m.neon.empty()) { c.save(); c.glow(14, m.neon); c.fillStyle(V(m.neon)); c.fillRect(x0, y0 + 12, w, 3); c.restore(); }
    } else if (k == "wood") {
        for (int row = 0; row * 22 < h; row++) {
            float yy = y0 + row * 22;
            for (float x = x0 - (row % 3) * 40; x < x0 + w; x += 130) {
                std::string base = sh(m.top, (r() - 0.5f) * 0.15f - row * 0.1f);
                c.fillStyle(lg(0, yy, 0, yy + 22, { sh(base, 0.12f), sh(base, -0.15f) })); c.fillRect(x + 1, yy + 1, 128, 20);
                c.strokeStyle(V(rgba(sh(base, -0.4f), 0.6f))); c.lineWidth(1);
                for (int g = 0; g < 3; g++) { float gy = yy + 5 + g * 5 + r() * 2; c.beginPath(); c.moveTo(x + 4, gy); c.bezierCurveTo(x + 40, gy + r() * 3 - 1.5f, x + 80, gy - r() * 3, x + 125, gy); c.stroke(); }
                c.fillStyle(V("#3a3a44")); ell(c, x + 7, yy + 11, 2, 2); c.fill(); ell(c, x + 122, yy + 11, 2, 2); c.fill();
            }
        }
        c.fillStyle(V(rgba("#ffffff", 0.25f))); c.fillRect(x0, y0 + 1, w, 2);
    } else if (k == "roof") {
        for (int row = 0; row * 16 < h; row++) {
            float yy = y0 + row * 16;
            for (float x = x0 - (row % 2) * 14; x < x0 + w; x += 28) {
                c.fillStyle(lg(0, yy, 0, yy + 18, { sh(m.top, 0.15f - row * 0.06f), sh(m.top, -0.25f - row * 0.06f) }));
                c.beginPath(); c.moveTo(x, yy); c.lineTo(x, yy + 10); c.arc(x + 14, yy + 10, 14, TAU / 2, 0, true); c.lineTo(x + 28, yy); c.fill();
                c.strokeStyle(V(rgba("#000000", 0.35f))); c.lineWidth(1.5f); c.beginPath(); c.arc(x + 14, yy + 10, 14, TAU / 2, 0, true); c.stroke();
            }
        }
        c.fillStyle(V(sh(m.top, -0.3f))); c.fillRect(x0, y0 - 4, w, 8); c.fillStyle(V(rgba("#ffffff", 0.3f))); c.fillRect(x0, y0 - 4, w, 2);
    } else if (k == "crystalfloor" || k == "volcanic" || k == "purple") {
        c.fillStyle(lg(0, y0, 0, y0 + 20, { m.top, sh(m.top, -0.3f) })); c.beginPath(); c.moveTo(x0, y0 + 22);
        for (float x = x0; x <= x0 + w; x += 16) c.lineTo(x, y0 + r() * 6); c.lineTo(x0 + w, y0 + 22); c.fill();
        c.fillStyle(V(rgba("#ffffff", 0.25f))); for (float x = x0; x < x0 + w; x += 16) c.fillRect(x, y0 + 2, 10, 2);
        if (k == "crystalfloor")
            for (int i = 0; i < w / 70; i++) {
                float x = x0 + r() * w, y = y0 + 26 + r() * (h - 40);
                c.save(); c.lighter(); c.fillStyle(rg(x, y, 0, 22, { rgba(m.glow, 0.5f), rgba(m.glow, 0) })); c.fillRect(x - 22, y - 22, 44, 44); c.restore();
                c.fillStyle(V(m.glow)); c.beginPath(); c.moveTo(x, y - 9); c.lineTo(x + 5, y); c.lineTo(x, y + 9); c.lineTo(x - 5, y); c.fill();
            }
        if (k == "volcanic") {
            c.save(); c.strokeStyle(V("#ff7a2a")); c.lineWidth(2); c.glow(10, "#ff5a1a");
            for (int i = 0; i < w / 80; i++) { float x = x0 + r() * w, y = y0 + 24; c.beginPath(); c.moveTo(x, y); for (int j = 0; j < 6; j++) { x += (r() - 0.5f) * 20; y += 10 + r() * 8; c.lineTo(x, y); } c.stroke(); }
            c.restore();
        }
        if (k == "purple") for (int row = 1; row * 32 < h; row++) { c.fillStyle(V(rgba("#000000", 0.2f))); c.fillRect(x0, y0 + row * 32, w, 2); }
    } else if (k == "toy") {
        int i = 0;
        for (float x = x0; x < x0 + w; x += 48) {
            const std::string& col = m.cols[(size_t)(i++) % m.cols.size()];
            c.fillStyle(lg(0, y0, 0, y0 + 40, { sh(col, 0.25f), col, sh(col, -0.2f) })); c.fillRect(x + 1, y0, 46, 40);
            c.fillStyle(V(rgba("#ffffff", 0.4f))); c.fillRect(x + 4, y0 + 3, 40, 4);
            c.fillStyle(V(sh(col, 0.15f))); ell(c, x + 14, y0 - 3, 7, 4); c.fill(); ell(c, x + 34, y0 - 3, 7, 4); c.fill();
        }
    }
    c.restore();
}

void PrimoPiano(K& c, const std::string& col, const std::string& tipo) {
    Caso r(99); c.fillStyle(V(col));
    if (tipo == "leaves") {
        for (int i = 0; i < 40; i++) {
            float x = r() * LS, h = 60 + r() * 140, w = 14 + r() * 20, a = (r() - 0.5f) * 40, b = (r() - 0.5f) * 60;
            Ripeti(x, 60, [&](float xx) { c.beginPath(); c.moveTo(xx - w, HS); c.quadraticCurveTo(xx - w * 0.2f + a, HS - h * 0.6f, xx + b, HS - h); c.quadraticCurveTo(xx + w * 0.3f, HS - h * 0.5f, xx + w, HS); c.fill(); });
        }
    } else if (tipo == "rocks" || tipo == "thorns") {
        for (int i = 0; i < 14; i++) {
            float x = r() * LS, w = 60 + r() * 140, h = 30 + r() * 60;
            Ripeti(x, w, [&](float xx) {
                c.beginPath(); c.moveTo(xx - w, HS); c.quadraticCurveTo(xx - w * 0.6f, HS - h, xx, HS - h * 1.1f); c.quadraticCurveTo(xx + w * 0.6f, HS - h, xx + w, HS); c.fill();
                if (tipo == "thorns") for (int k = 0; k < 5; k++) { c.beginPath(); c.moveTo(xx - 20 + k * 10, HS - h); c.lineTo(xx - 14 + k * 10 + (k * 7 % 5 - 2) * 8, HS - h - 40 - k * 6); c.lineTo(xx - 10 + k * 10, HS - h); c.fill(); }
            });
        }
    } else {
        for (int i = 0; i < 8; i++) { float x = r() * LS, hh = 120 + r() * 200; Ripeti(x, 40, [&](float xx) { c.fillRect(xx, HS - hh, 8, 400); c.fillRect(xx - 20, HS - 110, 48, 8); }); }
    }
}

}  // namespace arte
