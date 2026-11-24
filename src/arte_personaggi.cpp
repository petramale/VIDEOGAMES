// Personaggi dettagliati disegnati con il Pennello: eroi, nemici e boss dei giochi d'esempio.
#include "arte.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace arte {

using pennello::Colore;
static const float TAU = 6.28318530718f;
static const char* OUT = "#1d1426";

// --- colori ------------------------------------------------------------------------------------
Colore Col(const std::string& s) {
    if (s.compare(0, 5, "rgba(") == 0) {
        float r = 0, g = 0, b = 0, a = 1;
        std::sscanf(s.c_str() + 5, "%f,%f,%f,%f", &r, &g, &b, &a);
        return { r / 255.f, g / 255.f, b / 255.f, a };
    }
    return pennello::Esa(s.c_str());
}
static std::string Hex(Colore c) {
    char b[8]; auto q = [](float v) { int i = (int)std::lround(v * 255); return i < 0 ? 0 : (i > 255 ? 255 : i); };
    std::snprintf(b, sizeof b, "#%02x%02x%02x", q(c.r), q(c.g), q(c.b)); return b;
}
std::string sh(const std::string& h, float k) { return Hex(pennello::Schiarisci(Col(h), k)); }
std::string rgba(const std::string& h, float a) {
    Colore c = Col(h); char b[64];
    std::snprintf(b, sizeof b, "rgba(%d,%d,%d,%.3f)", (int)std::lround(c.r * 255), (int)std::lround(c.g * 255), (int)std::lround(c.b * 255), a);
    return b;
}
std::string mixc(const std::string& a, const std::string& b, float k) { return Hex(pennello::Mescola(Col(a), Col(b), k)); }
Vernice V(const std::string& s) { return Vernice(Col(s)); }
Vernice lg(float x0, float y0, float x1, float y1, std::initializer_list<std::string> c) {
    Vernice v; v.tipo = 1; v.x0 = x0; v.y0 = y0; v.x1 = x1; v.y1 = y1; int n = (int)c.size(), i = 0;
    for (auto& s : c) v.fermate.push_back({ n > 1 ? (float)i++ / (n - 1) : 0.f, Col(s) });
    return v;
}
Vernice lgs(float x0, float y0, float x1, float y1, std::initializer_list<std::pair<float, std::string>> f) {
    Vernice v; v.tipo = 1; v.x0 = x0; v.y0 = y0; v.x1 = x1; v.y1 = y1; for (auto& p : f) v.fermate.push_back({ p.first, Col(p.second) }); return v;
}
Vernice rg(float x, float y, float r0, float r1, std::initializer_list<std::string> c) {
    Vernice v; v.tipo = 2; v.x0 = x; v.y0 = y; v.r0 = r0; v.r1 = r1; int n = (int)c.size(), i = 0;
    for (auto& s : c) v.fermate.push_back({ n > 1 ? (float)i++ / (n - 1) : 0.f, Col(s) });
    return v;
}
Vernice rgs(float x, float y, float r0, float r1, std::initializer_list<std::pair<float, std::string>> f) {
    Vernice v; v.tipo = 2; v.x0 = x; v.y0 = y; v.r0 = r0; v.r1 = r1; for (auto& p : f) v.fermate.push_back({ p.first, Col(p.second) }); return v;
}
void K::glow(float blur, const std::string& col) { t.bagliore = blur; t.coloreBagliore = Col(col); }

// --- aiuti di disegno --------------------------------------------------------------------------
static void ell(K& c, float x, float y, float rx, float ry, float rot = 0) { c.beginPath(); c.ellipse(x, y, std::max(0.1f, rx), std::max(0.1f, ry), rot, 0, TAU); }
static void fillOut(K& c, const Vernice& f, float lw = 3) { c.fillStyle(f); c.fill(); c.lineWidth(lw); c.strokeStyle(V(OUT)); c.stroke(); }
static void star(K& c, float x, float y, float r) {
    c.beginPath(); for (int i = 0; i < 10; i++) { float a = -TAU / 4 + i * TAU / 10, rr = i % 2 ? r * 0.45f : r; c.lineTo(x + std::cos(a) * rr, y + std::sin(a) * rr); }
    c.closePath(); c.fill();
}
struct Punti { float kx, ky, fx, fy; };
static Punti limb(K& c, float x, float y, float a1, float l1, float a2, float l2, float w, const std::string& col, const std::string& hi = "") {
    float kx = x + std::sin(a1) * l1, ky = y + std::cos(a1) * l1, fx = kx + std::sin(a1 + a2) * l2, fy = ky + std::cos(a1 + a2) * l2;
    c.beginPath(); c.moveTo(x, y); c.lineTo(kx, ky); c.lineTo(fx, fy);
    c.strokeStyle(V(OUT)); c.lineWidth(w + 5); c.stroke(); c.strokeStyle(V(col)); c.lineWidth(w); c.stroke();
    if (!hi.empty()) { c.strokeStyle(V(hi)); c.lineWidth(w * 0.3f); c.beginPath(); c.moveTo(x - w * 0.15f, y); c.lineTo(kx - w * 0.15f, ky); c.stroke(); }
    return { kx, ky, fx, fy };
}
static void shoe(K& c, float x, float y, const std::string& col) {
    c.beginPath(); c.moveTo(x - 8, y - 2); c.quadraticCurveTo(x - 8, y - 9, x, y - 9); c.quadraticCurveTo(x + 12, y - 8, x + 14, y); c.lineTo(x - 8, y + 1); c.closePath();
    fillOut(c, lg(0, y - 9, 0, y + 1, { sh(col, 0.3f), col }), 2.5f); c.fillStyle(V(rgba("#ffffff", 0.3f))); c.fillRect(x - 6, y - 1, 19, 2);
}
static void eye(K& c, float x, float y, float s, const std::string& col, bool lash = false, bool chiuso = false) {
    if (chiuso) {
        c.strokeStyle(V(OUT)); c.lineWidth(2.6f); c.beginPath(); c.moveTo(x - 5 * s, y - 4 * s); c.lineTo(x + 4 * s, y); c.lineTo(x - 5 * s, y + 4 * s); c.stroke(); return;
    }
    ell(c, x, y, 6 * s, 8.5f * s); fillOut(c, V("#ffffff"), 2);
    c.save(); ell(c, x, y, 6 * s, 8.5f * s); c.clip();
    c.fillStyle(lg(0, y - 6 * s, 0, y + 8 * s, { sh(col, -0.35f), col, sh(col, 0.45f) })); ell(c, x + 1.6f * s, y + 1 * s, 4.7f * s, 6.6f * s); c.fill();
    c.fillStyle(V("#120a18")); ell(c, x + 2 * s, y + 1.5f * s, 2.4f * s, 3.6f * s); c.fill();
    c.fillStyle(V("rgba(0,0,0,0.18)")); c.fillRect(x - 7 * s, y - 9 * s, 14 * s, 4 * s); c.restore();
    c.fillStyle(V("#ffffff")); ell(c, x + 3.4f * s, y - 2.6f * s, 2 * s, 2.2f * s); c.fill(); ell(c, x + 0.4f * s, y + 4 * s, 1 * s, 1 * s); c.fill();
    if (lash) { c.strokeStyle(V(OUT)); c.lineWidth(2.4f); c.beginPath(); c.arc(x, y + 1, 7.5f * s, -2.6f, -0.5f); c.stroke(); c.beginPath(); c.moveTo(x + 6 * s, y - 5 * s); c.lineTo(x + 9 * s, y - 7 * s); c.stroke(); }
}
static void lantern(K& c, float x, float y, float t) {
    c.strokeStyle(V("#4a3a2a")); c.lineWidth(2.5f); c.beginPath(); c.moveTo(x, y - 6); c.lineTo(x, y + 4); c.stroke();
    c.save(); c.lighter(); float fl = 0.85f + std::sin(t * 9) * 0.08f + std::sin(t * 23) * 0.05f;
    c.fillStyle(rg(x, y + 18, 2, 60 * fl, { "rgba(255,220,120,0.55)", "rgba(255,170,60,0.18)", "rgba(255,150,40,0)" })); c.fillRect(x - 70, y - 50, 140, 140); c.restore();
    c.fillStyle(V("#3a2a1e")); c.fillRect(x - 9, y + 3, 18, 5); c.fillRect(x - 9, y + 30, 18, 5);
    c.strokeStyle(V(OUT)); c.lineWidth(2); c.strokeRect(x - 9, y + 3, 18, 5); c.strokeRect(x - 9, y + 30, 18, 5);
    c.fillStyle(rg(x, y + 19, 1, 12, { "#fffbe0", "#ffd25a", "#ff9a2a" })); c.fillRect(x - 7, y + 8, 14, 22);
    c.strokeStyle(V("#3a2a1e")); c.lineWidth(2); c.strokeRect(x - 7, y + 8, 14, 22); c.beginPath(); c.moveTo(x, y + 8); c.lineTo(x, y + 30); c.stroke();
    c.fillStyle(V("#ffffff")); ell(c, x, y + 20 + std::sin(t * 12), 2.5f, 4); c.fill();
}

// --- capelli e cappelli ------------------------------------------------------------------------
static void hair(K& c, const Umanoide& o, float x, float y, float r, float t) {
    if (o.hairStyle.empty() || o.hair.empty()) return;
    const std::string& col = o.hair;
    Vernice g = lg(x - r, y - r, x + r, y + r * 0.5f, { sh(col, 0.3f), col, sh(col, -0.35f) });
    c.beginPath();
    if (o.hairStyle == "bob") {
        c.moveTo(x - 32, y + 20); c.quadraticCurveTo(x - 38, y - 36, x + 2, y - 34); c.quadraticCurveTo(x + 36, y - 34, x + 33, y - 6);
        c.lineTo(x + 28, y - 12); c.lineTo(x + 24, y - 4); c.lineTo(x + 19, y - 16); c.lineTo(x + 12, y - 8); c.lineTo(x + 6, y - 18); c.lineTo(x - 2, y - 10); c.lineTo(x - 8, y - 14); c.lineTo(x - 12, y + 6); c.lineTo(x - 18, y + 26); c.closePath();
    } else if (o.hairStyle == "pony") {
        c.moveTo(x - 30, y + 8); c.quadraticCurveTo(x - 34, y - 34, x + 2, y - 33); c.quadraticCurveTo(x + 34, y - 32, x + 32, y - 8);
        c.lineTo(x + 24, y - 14); c.lineTo(x + 16, y - 8); c.lineTo(x + 10, y - 18); c.lineTo(x, y - 14); c.lineTo(x - 12, y - 6); c.lineTo(x - 18, y + 10); c.closePath();
    } else if (o.hairStyle == "cap") {
        c.moveTo(x - 30, y + 6); c.lineTo(x - 34, y - 8); c.lineTo(x - 26, y - 14); c.lineTo(x - 20, y - 2); c.lineTo(x - 14, y - 16); c.lineTo(x - 6, y - 6); c.lineTo(x - 10, y + 10); c.closePath();
    } else if (o.hairStyle == "spiky") {
        c.moveTo(x - 30, y + 4);
        for (int i = 0; i < 8; i++) { float a = TAU / 2 * 1.05f + i * 0.17f; c.lineTo(x + std::cos(a) * r * 1.35f, y + std::sin(a) * r * 1.35f); c.lineTo(x + std::cos(a + 0.085f) * r * 0.95f, y + std::sin(a + 0.085f) * r * 0.95f); }
        c.lineTo(x + 30, y - 8); c.lineTo(x + 20, y - 14); c.lineTo(x + 4, y - 10); c.lineTo(x - 14, y); c.closePath();
    } else return;
    c.fillStyle(g); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
    c.strokeStyle(V(rgba("#ffffff", 0.45f))); c.lineWidth(3); c.beginPath(); c.arc(x - 2, y - 6, r * 0.95f, -2.5f, -1.7f); c.stroke();
    if (o.hairStyle == "cap") {
        c.beginPath(); c.moveTo(x - 31, y - 6); c.bezierCurveTo(x - 34, y - 46, x + 30, y - 50, x + 32, y - 10); c.closePath();
        fillOut(c, lg(0, y - 48, 0, y - 6, { sh(o.capc, 0.3f), o.capc, sh(o.capc, -0.3f) }), 3);
        c.beginPath(); c.moveTo(x + 18, y - 12); c.quadraticCurveTo(x + 44, y - 16, x + 54, y - 6); c.quadraticCurveTo(x + 38, y - 4, x + 18, y - 6); c.closePath(); fillOut(c, V(sh(o.capc, -0.3f)), 2.5f);
        c.fillStyle(V("#f2c94a")); c.fillRect(x - 30, y - 12, 62, 5); c.fillStyle(V("#ffffff")); c.fillRect(x - 2, y - 34, 14, 10);
        c.strokeStyle(V("#c83a3a")); c.lineWidth(1.5f); c.beginPath(); c.moveTo(x - 2, y - 34); c.lineTo(x + 5, y - 28); c.lineTo(x + 12, y - 34); c.stroke();
        c.fillStyle(V("#ffffff")); c.beginPath(); c.moveTo(x - 20, y - 32); c.quadraticCurveTo(x - 40, y - 50 + std::sin(t * 6) * 3, x - 46, y - 30); c.quadraticCurveTo(x - 34, y - 34, x - 22, y - 26); c.fill();
        c.strokeStyle(V(OUT)); c.lineWidth(2); c.stroke();
    }
}
static void hat(K& c, const Umanoide& o, float x, float y, float t) {
    if (o.hat == "wizard") {
        c.beginPath(); c.moveTo(x - 40, y - 18); c.quadraticCurveTo(x, y - 30, x + 44, y - 16); c.lineTo(x + 20, y - 24); c.quadraticCurveTo(x + 14, y - 80, x - 24 + std::sin(t * 2) * 4, y - 110); c.quadraticCurveTo(x - 8, y - 70, x - 18, y - 24); c.closePath();
        fillOut(c, lg(x - 30, 0, x + 30, 0, { sh(o.hatc, 0.25f), o.hatc, sh(o.hatc, -0.4f) }), 3);
        c.fillStyle(V("#f2c94a")); star(c, x, y - 56, 7); star(c, x - 10, y - 80, 4); c.fillStyle(V(o.trim.empty() ? "#f2c94a" : o.trim)); c.fillRect(x - 20, y - 30, 40, 6);
    } else if (o.hat == "cowboy") {
        ell(c, x + 2, y - 20, 44, 9, -0.05f); fillOut(c, V(sh(o.hatc, -0.15f)), 3);
        c.beginPath(); c.moveTo(x - 22, y - 22); c.quadraticCurveTo(x - 24, y - 56, x - 4, y - 52); c.lineTo(x + 2, y - 46); c.lineTo(x + 8, y - 52); c.quadraticCurveTo(x + 28, y - 56, x + 26, y - 22); c.closePath();
        fillOut(c, lg(x - 22, 0, x + 26, 0, { sh(o.hatc, 0.25f), o.hatc, sh(o.hatc, -0.3f) }), 3); c.fillStyle(V("#3a2014")); c.fillRect(x - 22, y - 30, 48, 6);
    } else if (o.hat == "bandana") {
        c.beginPath(); c.moveTo(x - 31, y - 4); c.bezierCurveTo(x - 34, y - 46, x + 32, y - 48, x + 32, y - 8); c.quadraticCurveTo(x, y - 18, x - 31, y - 4);
        fillOut(c, lg(0, y - 44, 0, y - 6, { sh(o.hatc, 0.3f), o.hatc }), 3);
        c.fillStyle(V("#ffffff")); for (int i = 0; i < 6; i++) { ell(c, x - 18 + i * 8, y - 26 - (i % 2) * 6, 2, 2); c.fill(); }
        c.beginPath(); c.moveTo(x - 28, y - 10); c.lineTo(x - 48, y - 2 + std::sin(t * 6) * 4); c.lineTo(x - 40, y + 6); c.closePath(); fillOut(c, V(o.hatc), 2);
    } else if (o.hat == "shako") {
        c.beginPath(); c.moveTo(x - 22, y - 22); c.lineTo(x - 20, y - 72); c.lineTo(x + 22, y - 72); c.lineTo(x + 24, y - 22); c.closePath();
        fillOut(c, lg(x - 22, 0, x + 24, 0, { sh(o.hatc, 0.3f), o.hatc, sh(o.hatc, -0.35f) }), 3);
        c.fillStyle(V("#f2c94a")); c.fillRect(x - 22, y - 30, 46, 6); ell(c, x + 1, y - 50, 7, 7); c.fill();
        c.fillStyle(V("#ff4a4a")); ell(c, x + 1, y - 82, 7, 11); c.fill(); ell(c, x + 8, y - 16, 22, 5); c.fillStyle(V("#222222")); ell(c, x + 12, y - 18, 20, 4); c.fill();
    }
}

// --- umanoide ---------------------------------------------------------------------------------
void DisegnaUmanoide(K& c, float x, float y, float s, const Umanoide& o, const Posa& P) {
    const float t = P.t;
    c.save(); c.translate(x, y); c.scale(s * (o.flip ? -1 : 1), s * (o.upside ? -1 : 1));
    bool run = P.a == Anim::Corsa; float p = P.ph;
    float bob = run ? -std::fabs(std::sin(p)) * 5 : std::sin(t * 2.2f) * 1.3f;
    float legF = 0.1f, legB = -0.12f, knF = -0.05f, knB = -0.05f, armF = 0.18f + std::sin(t * 2.2f) * 0.04f, armB = -0.15f, lean = 0;
    bool occhiChiusi = false, sorriso = o.grin;
    switch (P.a) {
    case Anim::Corsa:
        legF = std::sin(p) * 0.8f; legB = -std::sin(p) * 0.8f;
        knF = -0.15f - 0.95f * std::max(0.f, std::sin(p + 1.8f)); knB = -0.15f - 0.95f * std::max(0.f, std::sin(p + 1.8f + TAU / 2));
        armF = -std::sin(p) * 0.95f; armB = std::sin(p) * 0.95f; lean = 0.06f; break;
    case Anim::Salto: legF = 0.7f; knF = -1.3f; legB = -0.35f; knB = -0.5f; armF = 2.5f; armB = -2.1f; bob = -4; break;
    case Anim::Caduta: legF = 0.25f; knF = -0.35f; legB = -0.3f; knB = -0.25f; armF = 1.9f; armB = -1.9f; break;
    case Anim::Attacco: { float k = std::sin(std::min(1.f, t) * TAU / 2); legF = 0.55f; knF = -0.25f; legB = -0.55f; knB = -0.1f; armF = 0.6f + 1.2f * k; armB = -0.6f; lean = 0.12f * k; break; }
    case Anim::Colpito: legF = 0.25f; legB = -0.2f; armF = -0.9f; armB = 1.0f; lean = -0.22f; occhiChiusi = true; break;
    case Anim::Vittoria: armF = 2.95f; armB = -0.5f; sorriso = true; bob = -std::fabs(std::sin(t * 4)) * 6; break;
    default: break;
    }
    c.translate(0, bob); c.rotate(lean);
    const std::string skin = o.skin, top = o.top, bot = o.bottom.empty() ? sh(o.top, -0.3f) : o.bottom;
    if (!o.cloak.empty()) {
        float wv = std::sin(t * 5) * 6;
        c.beginPath(); c.moveTo(-14, -98); c.quadraticCurveTo(-40, -70 + wv, -52 - (run ? 14.f : 0.f), -30 + wv); c.quadraticCurveTo(-30, -26, -16, -40); c.lineTo(14, -96); c.closePath();
        c.fillStyle(lg(-50, -90, 10, -30, { sh(o.cloak, 0.15f), o.cloak, sh(o.cloak, -0.45f) })); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
    }
    if (o.hairStyle == "pony") {
        float sw = std::sin(t * 4 + p) * 5;
        c.beginPath(); c.moveTo(-22, -146); c.quadraticCurveTo(-58, -150 + sw, -60, -112 + sw); c.quadraticCurveTo(-50, -96 + sw, -44, -110 + sw); c.quadraticCurveTo(-40, -128, -20, -126); c.closePath();
        c.fillStyle(lg(-60, -150, -20, -100, { sh(o.hair, 0.25f), o.hair, sh(o.hair, -0.35f) })); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
        std::string ac = o.accent.empty() ? "#e84a4a" : o.accent; ell(c, -24, -138, 5, 7); fillOut(c, V(ac), 2);
    }
    if (o.hairStyle == "bob") {
        c.beginPath(); c.moveTo(-30, -150); c.quadraticCurveTo(-44, -120, -34, -98); c.lineTo(-10, -100); c.lineTo(-6, -150); c.closePath();
        c.fillStyle(V(sh(o.hair, -0.25f))); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
    }
    // braccio e gamba dietro
    {
        Punti a = limb(c, -9, -92, armB, 19, 0.6f, 17, 10, sh(top, -0.3f));
        ell(c, a.fx, a.fy, 6, 6); fillOut(c, V(sh(skin, -0.2f)), 2.5f);
    }
    if (!o.robe) { Punti lb = limb(c, -5, -54, legB, 26, knB, 24, 13, sh(bot, -0.3f)); shoe(c, lb.fx, lb.fy, sh(o.shoes, -0.25f)); }
    if (o.robe) {
        c.beginPath(); c.moveTo(-18, -98); c.lineTo(18, -98); c.lineTo(30, -4); c.quadraticCurveTo(0, 4 + std::sin(t * 4) * 2, -30, -4); c.closePath();
        c.fillStyle(lg(-30, 0, 30, 0, { sh(top, 0.2f), top, sh(top, -0.4f) })); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
        c.fillStyle(V(o.trim.empty() ? "#f2c94a" : o.trim)); c.fillRect(-28, -12, 56, 5);
        c.fillStyle(V(rgba("#ffffff", 0.85f))); for (int i = 0; i < 4; i++) star(c, -14 + i * 10, -40 - i * 12 + (i % 2) * 8, 3.5f);
    }
    c.beginPath(); c.moveTo(-17, -99); c.quadraticCurveTo(0, -104, 17, -99); c.lineTo(15, -54); c.quadraticCurveTo(0, -50, -15, -54); c.closePath();
    c.fillStyle(lg(-17, 0, 17, 0, { sh(top, 0.22f), top, sh(top, -0.3f) })); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
    if (!o.overall.empty()) {
        c.fillStyle(lg(-15, 0, 15, 0, { sh(o.overall, 0.2f), o.overall, sh(o.overall, -0.3f) }));
        c.beginPath(); c.moveTo(-12, -80); c.lineTo(12, -80); c.lineTo(15, -54); c.lineTo(-15, -54); c.closePath(); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(2); c.stroke();
        c.strokeStyle(V(o.overall)); c.lineWidth(4); c.beginPath(); c.moveTo(-9, -80); c.lineTo(-11, -99); c.moveTo(9, -80); c.lineTo(11, -99); c.stroke();
        c.fillStyle(V("#f2c94a")); ell(c, -9, -81, 2.5f, 2.5f); c.fill(); ell(c, 9, -81, 2.5f, 2.5f); c.fill(); c.fillStyle(V(sh(o.overall, -0.2f))); c.fillRect(-6, -74, 12, 9);
    }
    if (!o.collar.empty()) {
        c.beginPath(); c.moveTo(-12, -100); c.lineTo(0, -88); c.lineTo(12, -100); c.lineTo(6, -102); c.lineTo(0, -95); c.lineTo(-6, -102); c.closePath();
        c.fillStyle(V(o.collar)); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(2); c.stroke();
    }
    if (!o.belt.empty()) {
        c.fillStyle(V(o.belt)); c.fillRect(-15.5f, -62, 31, 7); c.strokeStyle(V(OUT)); c.lineWidth(2); c.strokeRect(-15.5f, -62, 31, 7);
        c.fillStyle(V("#f2c94a")); c.fillRect(-4, -63, 8, 9); c.strokeRect(-4, -63, 8, 9);
    }
    if (!o.emblem.empty()) { c.fillStyle(V(o.emblem)); star(c, 4, -80, 5); }
    if (o.bag) {
        c.strokeStyle(V("#6a3a1a")); c.lineWidth(4); c.beginPath(); c.moveTo(-14, -98); c.lineTo(14, -58); c.stroke();
        c.fillStyle(lg(0, -66, 0, -44, { "#c88a4a", "#8a5428" })); c.beginPath(); c.rect(4, -66, 22, 20); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(2.5f); c.stroke();
        c.fillStyle(V("#ffffff")); c.fillRect(9, -60, 12, 8); c.strokeStyle(V("#c83a3a")); c.lineWidth(1); c.beginPath(); c.moveTo(9, -60); c.lineTo(15, -55); c.lineTo(21, -60); c.stroke();
    }
    if (o.bandage) { c.strokeStyle(V(rgba("#a89a78", 0.8f))); c.lineWidth(1.5f); for (int k = 0; k < 6; k++) { c.beginPath(); c.moveTo(-16, -96 + k * 8); c.lineTo(16, -92 + k * 8); c.stroke(); } }
    if (!o.robe) { Punti lf = limb(c, 5, -54, legF, 26, knF, 24, 13, bot, rgba("#ffffff", 0.15f)); shoe(c, lf.fx, lf.fy, o.shoes); }
    if (!o.scarf.empty()) {
        float wv = std::sin(t * 7) * 5;
        c.fillStyle(V(o.scarf)); c.beginPath(); c.moveTo(-6, -100); c.quadraticCurveTo(-26, -96 + wv, -44, -90 - wv); c.lineTo(-40, -82 - wv); c.quadraticCurveTo(-24, -88 + wv, -6, -92); c.closePath(); c.fill();
        c.strokeStyle(V(OUT)); c.lineWidth(2.5f); c.stroke(); ell(c, 0, -100, 16, 6); fillOut(c, V(o.scarf), 2.5f);
    }
    // braccio davanti con l'oggetto in mano
    Punti af = limb(c, 9, -92, armF, 19, 0.75f, 17, 10.5f, top, rgba("#ffffff", 0.2f));
    if (o.item == "lantern") lantern(c, af.fx + 2, af.fy + 4, t);
    else if (o.item == "wrench") {
        c.save(); c.translate(af.fx, af.fy); c.rotate(-0.6f + armF);
        c.fillStyle(lg(-3, 0, 3, 0, { "#e8eef8", "#8a94a8" })); c.fillRect(-3, -30, 6, 30); c.strokeStyle(V(OUT)); c.lineWidth(2); c.strokeRect(-3, -30, 6, 30);
        ell(c, 0, -34, 9, 8); fillOut(c, V("#c8d0e0"), 2); c.fillStyle(V("#5a4a6a")); c.fillRect(-3, -44, 6, 12); c.restore();
    } else if (o.item == "staff") {
        c.strokeStyle(V("#6a3a1a")); c.lineWidth(5); c.beginPath(); c.moveTo(af.fx, af.fy + 40); c.lineTo(af.fx + 4, af.fy - 80); c.stroke();
        c.save(); c.glow(20, o.orb); ell(c, af.fx + 4, af.fy - 88, 10, 10); fillOut(c, rg(af.fx + 1, af.fy - 91, 1, 10, { "#ffffff", o.orb }), 2.5f); c.restore();
    } else if (o.item == "sword") {
        c.save(); c.translate(af.fx, af.fy); c.rotate(0.4f);
        c.fillStyle(lg(-3, 0, 3, 0, { "#f4f8ff", "#8a94b8" })); c.beginPath(); c.moveTo(-4, -4); c.lineTo(-4, -62); c.lineTo(0, -70); c.lineTo(4, -62); c.lineTo(4, -4); c.fill();
        c.strokeStyle(V(OUT)); c.lineWidth(2); c.stroke(); c.fillStyle(V("#f2c94a")); c.fillRect(-11, -6, 22, 5); c.restore();
    } else if (o.item == "pistol") {
        c.fillStyle(V("#5a5a6a")); c.fillRect(af.fx - 2, af.fy - 4, 20, 7); c.strokeStyle(V(OUT)); c.lineWidth(2); c.strokeRect(af.fx - 2, af.fy - 4, 20, 7);
    }
    ell(c, af.fx, af.fy, 6.5f, 6.5f); fillOut(c, rg(af.fx - 2, af.fy - 2, 1, 7, { sh(skin, 0.2f), skin }), 2.5f);
    // testa
    const float hcx = 2, hcy = -130, hr = 30;
    if (!o.helmet.empty()) {
        ell(c, hcx, hcy, hr, hr * 0.98f); fillOut(c, rg(hcx - 10, hcy - 12, 4, hr, { sh(o.helmet, 0.5f), o.helmet, sh(o.helmet, -0.45f) }), 3);
        c.fillStyle(V("#0c0812")); c.beginPath(); c.rect(hcx - 4, hcy - 6, 34, 9); c.fill();
        std::string gl = o.glow.empty() ? "#ff3a3a" : o.glow;
        c.save(); c.glow(14, gl); c.fillStyle(V(gl)); ell(c, hcx + 8, hcy - 2, 4, 2.5f); c.fill(); ell(c, hcx + 22, hcy - 2, 4, 2.5f); c.fill(); c.restore();
        c.strokeStyle(V(rgba("#ffffff", 0.5f))); c.lineWidth(3); c.beginPath(); c.arc(hcx, hcy, hr - 6, -2.6f, -1.6f); c.stroke();
        c.fillStyle(V(sh(o.helmet, -0.2f))); c.fillRect(hcx - 2, hcy - hr - 2, 6, hr + 8);
        if (!o.plume.empty()) {
            c.fillStyle(V(o.plume)); c.beginPath(); c.moveTo(hcx, hcy - hr); c.quadraticCurveTo(hcx - 30, hcy - hr - 30, hcx - 50, hcy - hr + 10 + std::sin(t * 4) * 4);
            c.quadraticCurveTo(hcx - 24, hcy - hr - 10, hcx + 4, hcy - hr + 4); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(2); c.stroke();
        }
    } else {
        ell(c, hcx - 24, hcy + 4, 7, 9); fillOut(c, V(sh(skin, -0.1f)), 2.5f);
        ell(c, hcx, hcy, hr, hr * 0.96f); fillOut(c, rg(hcx - 8, hcy - 10, 4, hr * 1.1f, { sh(skin, 0.25f), skin, sh(skin, -0.2f) }), 3);
        if (o.face == "skull") {
            c.fillStyle(V("#1a1220")); ell(c, hcx + 8, hcy - 2, 7, 8); c.fill(); ell(c, hcx + 24, hcy - 2, 6, 8); c.fill();
            c.save(); c.glow(10, "#7af0ff"); c.fillStyle(V("#9af8ff")); ell(c, hcx + 9, hcy - 1, 2.2f, 2.2f); c.fill(); ell(c, hcx + 24, hcy - 1, 2, 2); c.fill(); c.restore();
            c.fillStyle(V("#1a1220")); c.beginPath(); c.moveTo(hcx + 16, hcy + 6); c.lineTo(hcx + 13, hcy + 12); c.lineTo(hcx + 19, hcy + 12); c.fill();
            c.fillStyle(V("#ffffff")); c.fillRect(hcx + 4, hcy + 17, 24, 7); c.strokeStyle(V(OUT)); c.lineWidth(1.5f); c.strokeRect(hcx + 4, hcy + 17, 24, 7);
            for (int k = 1; k < 5; k++) { c.beginPath(); c.moveTo(hcx + 4 + k * 4.8f, hcy + 17); c.lineTo(hcx + 4 + k * 4.8f, hcy + 24); c.stroke(); }
        } else if (o.face == "mummy") {
            c.strokeStyle(V("#b8ab88")); c.lineWidth(2);
            for (int k = 0; k < 7; k++) { c.beginPath(); c.moveTo(hcx - 28, hcy - 22 + k * 8); c.lineTo(hcx + 30, hcy - 18 + k * 8 + (k % 2) * 4); c.stroke(); }
            c.fillStyle(V("#1a1018")); c.fillRect(hcx + 4, hcy - 6, 26, 9);
            c.save(); c.glow(12, "#ff3a2a"); c.fillStyle(V("#ffd04a")); ell(c, hcx + 12, hcy - 2, 3, 3); c.fill(); ell(c, hcx + 24, hcy - 2, 3, 3); c.fill(); c.restore();
        } else {
            eye(c, hcx + 7, hcy - 3, 1, o.eye, o.lash, occhiChiusi); eye(c, hcx + 23, hcy - 3, 0.92f, o.eye, o.lash, occhiChiusi);
            c.fillStyle(V(rgba("#ff6a8a", 0.35f))); ell(c, hcx - 2, hcy + 10, 6, 3.5f); c.fill(); ell(c, hcx + 29, hcy + 10, 4, 3); c.fill();
            c.strokeStyle(V(OUT)); c.lineWidth(2.4f); c.beginPath();
            if (P.a == Anim::Colpito) { ell(c, hcx + 17, hcy + 13, 4, 5); c.fillStyle(V("#8a2a3a")); c.fill(); c.stroke(); }
            else if (sorriso) { c.moveTo(hcx + 10, hcy + 13); c.quadraticCurveTo(hcx + 17, hcy + 22, hcx + 25, hcy + 12); c.closePath(); c.fillStyle(V("#8a2a3a")); c.fill(); c.stroke(); }
            else { c.arc(hcx + 16, hcy + 11, 5, 0.3f, 2.6f); c.stroke(); }
            if (o.brows) { c.strokeStyle(V(o.hair)); c.lineWidth(3); c.beginPath(); c.moveTo(hcx + 1, hcy - 16); c.lineTo(hcx + 12, hcy - 14); c.moveTo(hcx + 19, hcy - 14); c.lineTo(hcx + 29, hcy - 17); c.stroke(); }
        }
        hair(c, o, hcx, hcy, hr, t);
    }
    if (o.beard) {
        c.beginPath(); c.moveTo(hcx - 14, hcy + 10); c.quadraticCurveTo(hcx - 6, hcy + 70, hcx + 14, hcy + 62 + std::sin(t * 3) * 2); c.quadraticCurveTo(hcx + 34, hcy + 40, hcx + 30, hcy + 10); c.quadraticCurveTo(hcx + 10, hcy + 20, hcx - 14, hcy + 10);
        fillOut(c, lg(0, hcy, 0, hcy + 60, { "#ffffff", "#c8c8d8" }), 2.5f);
    }
    if (!o.hat.empty()) hat(c, o, hcx, hcy, t);
    if (o.crown) {
        c.beginPath(); c.moveTo(hcx - 22, hcy - 22); c.lineTo(hcx - 24, hcy - 48); c.lineTo(hcx - 12, hcy - 36); c.lineTo(hcx + 2, hcy - 56); c.lineTo(hcx + 14, hcy - 36); c.lineTo(hcx + 28, hcy - 48); c.lineTo(hcx + 26, hcy - 22); c.closePath();
        fillOut(c, lg(0, hcy - 56, 0, hcy - 20, { "#fff2a8", "#f2b81a", "#a8700a" }), 2.5f);
        c.fillStyle(V("#c83aff")); ell(c, hcx + 2, hcy - 32, 4, 4); c.fill(); c.fillStyle(V("#ff3a5a")); ell(c, hcx - 14, hcy - 30, 3, 3); c.fill(); ell(c, hcx + 18, hcy - 30, 3, 3); c.fill();
    }
    if (o.goggles) {
        for (float gx : { hcx + 4, hcx + 22 }) {
            ell(c, gx, hcy - 26, 8, 7); fillOut(c, V("#8a5a2a"), 2.5f); ell(c, gx, hcy - 26, 5.5f, 4.5f);
            c.fillStyle(lg(gx - 5, hcy - 30, gx + 5, hcy - 22, { "#d8f8ff", "#4ab8e8" })); c.fill(); c.fillStyle(V("#ffffff")); c.fillRect(gx - 3, hcy - 29, 2.5f, 2);
        }
        c.strokeStyle(V("#5a3a1a")); c.lineWidth(3); c.beginPath(); c.moveTo(hcx - 26, hcy - 22); c.lineTo(hcx - 4, hcy - 26); c.stroke();
    }
    c.restore();
}

// --- robot ----------------------------------------------------------------------------------------
void DisegnaRobot(K& c, float x, float y, float s, const Robot& o, const Posa& P) {
    const float t = P.t; bool run = P.a == Anim::Corsa; float ph = P.ph;
    c.save(); c.translate(x, y); c.scale(s * (o.flip ? -1 : 1), s);
    float bob = run ? -std::fabs(std::sin(ph)) * 4 : std::sin(t * 2) * 1.5f;
    if (P.a == Anim::Salto) bob = -6; if (P.a == Anim::Vittoria) bob = -std::fabs(std::sin(t * 4)) * 8;
    c.translate(0, bob); if (P.a == Anim::Colpito) c.rotate(-0.18f);
    const std::string m = o.metal, m2 = sh(m, -0.35f);
    float lf = run ? std::sin(ph) * 0.6f : 0.1f, lb = -lf;
    if (P.a == Anim::Salto) { lf = 0.5f; lb = -0.3f; } else if (P.a == Anim::Caduta) { lf = 0.2f; lb = -0.25f; }
    struct G { float lx, a; std::string col; };
    for (const G& g : { G{ -12, lb, m2 }, G{ 12, lf, m } }) {
        float fx = g.lx + std::sin(g.a) * 40, fy = -44 + std::cos(g.a) * 40;
        c.strokeStyle(V(OUT)); c.lineWidth(15); c.beginPath(); c.moveTo(g.lx, -44); c.lineTo(fx, fy); c.stroke();
        c.strokeStyle(lg(g.lx - 6, 0, g.lx + 6, 0, { sh(g.col, 0.4f), g.col, sh(g.col, -0.3f) })); c.lineWidth(10); c.stroke();
        c.beginPath(); c.moveTo(fx - 14, fy + 2); c.lineTo(fx - 10, fy - 8); c.lineTo(fx + 14, fy - 8); c.lineTo(fx + 18, fy + 2); c.closePath(); fillOut(c, V(sh(g.col, -0.2f)), 2.5f);
    }
    float braccioB = P.a == Anim::Vittoria ? -2.6f : (P.a == Anim::Salto ? -2.2f : 0.3f);
    c.strokeStyle(V(OUT)); c.lineWidth(13); c.beginPath(); c.moveTo(-30, -96); c.lineTo(-30 - std::sin(-braccioB) * 32, -96 + std::cos(braccioB) * 32); c.stroke();
    c.strokeStyle(V(m2)); c.lineWidth(8); c.stroke(); ell(c, -30 - std::sin(-braccioB) * 34, -96 + std::cos(braccioB) * 34, 9, 9); fillOut(c, V(m2), 2.5f);
    c.beginPath(); c.moveTo(-32, -112); c.quadraticCurveTo(-40, -80, -30, -46); c.lineTo(30, -46); c.quadraticCurveTo(40, -80, 32, -112); c.closePath();
    fillOut(c, lg(-38, 0, 38, 0, { sh(m, 0.45f), m, sh(m, -0.45f) }), 3.5f);
    c.fillStyle(V(rgba("#ffffff", 0.5f))); c.fillRect(-26, -106, 6, 52);
    c.fillStyle(V(sh(m, -0.25f))); for (float yy : { -100.f, -58.f }) for (int xx = -24; xx <= 24; xx += 12) { ell(c, (float)xx, yy, 2.2f, 2.2f); c.fill(); }
    ell(c, 0, -80, 13, 13); fillOut(c, V("#2a2a3a"), 3);
    c.save(); c.glow(18, o.core); float pul = 0.75f + 0.25f * std::sin(t * 4); ell(c, 0, -80, 8 * pul + 2, 8 * pul + 2); c.fillStyle(rg(-2, -82, 1, 10, { "#ffffff", o.core })); c.fill(); c.restore();
    c.strokeStyle(V(OUT)); c.lineWidth(1.5f); for (int k = 0; k < 4; k++) { c.beginPath(); c.moveTo(-10 + k * 7.f, -66); c.lineTo(-10 + k * 7.f, -56); c.stroke(); }
    c.beginPath(); c.moveTo(-26, -114); c.lineTo(-24, -150); c.quadraticCurveTo(0, -160, 24, -150); c.lineTo(26, -114); c.closePath();
    fillOut(c, lg(-26, 0, 26, 0, { sh(m, 0.4f), m, sh(m, -0.4f) }), 3.5f);
    c.fillStyle(V("#16161f")); c.beginPath(); c.rect(-18, -144, 40, 20); c.fill();
    c.save(); c.glow(16, o.eye); c.fillStyle(V(o.eye));
    if (P.a == Anim::Colpito) { c.strokeStyle(V(o.eye)); c.lineWidth(3); for (float ex : { -4.f, 14.f }) { c.beginPath(); c.moveTo(ex - 4, -138); c.lineTo(ex + 4, -130); c.moveTo(ex + 4, -138); c.lineTo(ex - 4, -130); c.stroke(); } }
    else if (P.a == Anim::Vittoria) { c.strokeStyle(V(o.eye)); c.lineWidth(3); for (float ex : { -4.f, 14.f }) { c.beginPath(); c.arc(ex, -130, 5, 3.6f, 5.8f); c.stroke(); } }
    else { ell(c, -4, -134, 5, 7); c.fill(); ell(c, 14, -134, 5, 7); c.fill(); }
    c.restore();
    c.fillStyle(V(rgba("#ffffff", 0.15f))); c.fillRect(-18, -144, 40, 5);
    c.strokeStyle(V("#555a6a")); c.lineWidth(3); c.beginPath(); c.moveTo(0, -156); c.lineTo(0, -176); c.stroke();
    c.save(); c.glow(14, o.ant); ell(c, 0, -180, 5, 5); fillOut(c, V(o.ant), 2); c.restore();
    ell(c, -28, -132, 6, 10); fillOut(c, V(m2), 2.5f); ell(c, 28, -132, 6, 10); fillOut(c, V(m2), 2.5f);
    // braccio a molla (si allunga nell'attacco)
    float ax = 36, ay = -96; float allunga = P.a == Anim::Attacco ? 1.f + 1.6f * std::sin(std::min(1.f, t) * TAU / 2) : 1.f;
    float dirx = P.a == Anim::Attacco ? 1.f : 0.2f, diry = P.a == Anim::Attacco ? 0.15f : 1.f;
    if (P.a == Anim::Vittoria || P.a == Anim::Salto) { dirx = 0.2f; diry = -1; }
    float nl = std::hypot(dirx, diry); dirx /= nl; diry /= nl;
    float lungh = 36 * allunga;
    c.strokeStyle(V(sh(m, -0.1f))); c.lineWidth(3); c.beginPath();
    for (int k = 0; k <= 8; k++) { float d = lungh * k / 8, off = (k % 2 ? -6.f : 6.f); c.lineTo(ax + dirx * d - diry * off, ay + diry * d + dirx * off); }
    c.stroke();
    float px = ax + dirx * lungh, py = ay + diry * lungh;
    c.save(); c.translate(px, py); c.rotate(std::atan2(diry, dirx));
    c.beginPath(); c.moveTo(-2, -12); c.lineTo(20, -14); c.lineTo(24, 12); c.lineTo(0, 14); c.closePath(); fillOut(c, lg(0, -14, 24, 14, { sh(o.fist, 0.3f), o.fist, sh(o.fist, -0.35f) }), 3);
    c.fillStyle(V(rgba("#ffffff", 0.4f))); c.fillRect(2, -9, 14, 3); c.restore();
    c.restore();
}

// --- pinguino, lucciola ------------------------------------------------------------------------
void DisegnaPinguino(K& c, float x, float y, float s, const std::string& sciarpa, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    float fl = std::sin(t * 10) * 0.5f; if (P.a == Anim::Corsa) fl = std::sin(P.ph) * 0.7f;
    if (P.a == Anim::Corsa) c.translate(0, -std::fabs(std::sin(P.ph)) * 5);
    if (P.a == Anim::Colpito) c.rotate(-0.2f);
    c.save(); c.translate(-26, -70); c.rotate(-0.6f + fl - (P.a == Anim::Salto || P.a == Anim::Vittoria ? 1.4f : 0)); ell(c, 0, 18, 10, 26); fillOut(c, V("#1e2236"), 3); c.restore();
    ell(c, 0, -62, 34, 58); fillOut(c, rg(-12, -90, 5, 70, { "#4a5070", "#1e2236" }), 3.5f);
    ell(c, 6, -52, 22, 42); c.fillStyle(lg(0, -90, 0, -10, { "#ffffff", "#d8e0f0" })); c.fill();
    bool ch = P.a == Anim::Colpito;
    eye(c, 0, -92, 0.9f, "#2a5ad8", false, ch); eye(c, 16, -92, 0.85f, "#2a5ad8", false, ch);
    c.beginPath(); c.moveTo(4, -82); c.lineTo(30, -76); c.lineTo(6, -70); c.closePath(); fillOut(c, lg(0, -82, 0, -70, { "#ffd04a", "#f28a1a" }), 2.5f);
    c.strokeStyle(V("#5a3a1a")); c.lineWidth(4); c.beginPath(); c.moveTo(-32, -102); c.quadraticCurveTo(0, -116, 32, -104); c.stroke();
    for (float gx : { -2.f, 18.f }) { ell(c, gx, -108, 9, 8); fillOut(c, V("#8a5a2a"), 2.5f); ell(c, gx, -108, 6, 5); c.fillStyle(lg(0, -113, 0, -103, { "#d8f8ff", "#4ab8e8" })); c.fill(); }
    float wv = std::sin(t * 7) * 6;
    c.beginPath(); c.moveTo(-20, -56); c.quadraticCurveTo(-46, -50 + wv, -66, -58 - wv); c.lineTo(-62, -48 - wv); c.quadraticCurveTo(-44, -42 + wv, -18, -46); c.closePath(); fillOut(c, V(sciarpa), 2.5f);
    ell(c, 0, -52, 28, 8); fillOut(c, V(sciarpa), 2.5f);
    c.save(); c.translate(26, -70); c.rotate(0.5f - fl + (P.a == Anim::Attacco ? -1.4f : 0) + (P.a == Anim::Salto || P.a == Anim::Vittoria ? -1.6f : 0)); ell(c, 0, 18, 9, 24); fillOut(c, V("#1e2236"), 3); c.restore();
    if (P.a == Anim::Attacco) { ell(c, 50, -86, 9, 9); fillOut(c, rg(47, -89, 1, 9, { "#ffffff", "#bfe6ff" }), 2); }
    float pf = P.a == Anim::Corsa ? std::sin(P.ph) * 5 : 0;
    ell(c, -14 - pf, -3, 13, 5); fillOut(c, V("#f29a1a"), 2.5f); ell(c, 12 + pf, -3, 13, 5); fillOut(c, V("#f29a1a"), 2.5f);
    c.restore();
}
void DisegnaLucciola(K& c, float x, float y, float s, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    c.lighter(); c.fillStyle(rg(0, 8, 0, 60, { "rgba(220,255,140,0.6)", "rgba(180,255,120,0.15)", "rgba(180,255,120,0)" })); c.fillRect(-60, -52, 120, 120); c.normal();
    float fl = std::sin(t * 30) * 0.5f;
    for (int sd : { -1, 1 }) { c.save(); c.rotate(sd * (0.5f + fl)); ell(c, sd * 14.f, -12, 14, 7, sd * 0.3f); c.fillStyle(V("rgba(220,240,255,0.55)")); c.fill(); c.strokeStyle(V("rgba(255,255,255,0.8)")); c.lineWidth(1.5f); c.stroke(); c.restore(); }
    ell(c, 0, 10, 10, 12); fillOut(c, rg(0, 12, 1, 12, { "#ffffe0", "#d8ff6a", "#8ad82a" }), 2.5f);
    ell(c, 0, -6, 11, 10); fillOut(c, rg(-3, -9, 1, 11, { "#7a6aa8", "#3a2a5a" }), 2.5f);
    bool ch = P.a == Anim::Colpito;
    eye(c, -2, -8, 0.55f, "#5a3ad8", false, ch); eye(c, 7, -8, 0.5f, "#5a3ad8", false, ch);
    c.strokeStyle(V(OUT)); c.lineWidth(1.5f); c.beginPath(); c.moveTo(-3, -15); c.quadraticCurveTo(-8, -28, -14, -26); c.moveTo(4, -15); c.quadraticCurveTo(8, -28, 14, -27); c.stroke();
    c.fillStyle(V("#ffe96a")); ell(c, -14, -26, 2.5f, 2.5f); c.fill(); ell(c, 14, -27, 2.5f, 2.5f); c.fill();
    c.restore();
}

// --- nemici ----------------------------------------------------------------------------------------
void DisegnaOmbra(K& c, float x, float y, float s, const std::string& col, const std::string& aura, const std::string& occhi, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    if (P.a == Anim::Colpito) c.scale(1.15f, 0.85f);
    c.save(); c.glow(30, aura);
    c.beginPath(); c.moveTo(-34, -30); c.bezierCurveTo(-40, -96, 40, -96, 34, -30);
    for (int i = 0; i <= 6; i++) { float k = i / 6.f; c.lineTo(34 - 68 * k, -14 + std::sin(t * 6 + i * 1.7f) * 8 + (i % 2) * 14); }
    c.closePath(); c.fillStyle(rg(-10, -60, 4, 60, { sh(col, 0.3f), col, sh(col, -0.5f) })); c.fill(); c.restore();
    c.strokeStyle(V(rgba(aura, 0.8f))); c.lineWidth(2); c.stroke();
    c.save(); c.glow(16, occhi); c.fillStyle(V(occhi));
    c.beginPath(); c.moveTo(-18, -58); c.lineTo(-2, -52); c.lineTo(-16, -46); c.closePath(); c.fill();
    c.beginPath(); c.moveTo(18, -58); c.lineTo(4, -52); c.lineTo(16, -46); c.closePath(); c.fill(); c.restore();
    c.fillStyle(V("#0a0510")); c.beginPath();
    if (P.a == Anim::Attacco) { ell(c, 0, -34, 10, 8); c.fill(); }
    else { c.moveTo(-12, -36); c.quadraticCurveTo(0, -26, 12, -36); c.quadraticCurveTo(0, -32, -12, -36); c.fill(); }
    c.restore();
}
void DisegnaPipistrello(K& c, float x, float y, float s, const std::string& ali, const std::string& corpo, const std::string& occhi, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    float f = std::sin(t * 14); if (P.a == Anim::Colpito) c.rotate(0.4f);
    for (int sd : { -1, 1 }) {
        c.save(); c.scale((float)sd, 1); c.rotate(f * 0.35f);
        c.beginPath(); c.moveTo(8, -6); c.quadraticCurveTo(40, -40, 70, -24); c.lineTo(62, -10); c.quadraticCurveTo(54, -2, 48, -6); c.quadraticCurveTo(40, 4, 32, -2); c.quadraticCurveTo(22, 8, 10, 4); c.closePath();
        fillOut(c, lg(8, -30, 70, 0, { sh(ali, 0.3f), ali, sh(ali, -0.4f) }), 3);
        c.strokeStyle(V(rgba(sh(ali, -0.5f), 0.8f))); c.lineWidth(1.8f); c.beginPath(); c.moveTo(12, -6); c.lineTo(62, -20); c.moveTo(12, -4); c.lineTo(48, -6); c.moveTo(12, -2); c.lineTo(32, 0); c.stroke();
        c.restore();
    }
    ell(c, 0, -4, 18, 20); fillOut(c, rg(-5, -10, 2, 22, { sh(corpo, 0.35f), corpo, sh(corpo, -0.4f) }), 3);
    for (int sd : { -1, 1 }) { c.beginPath(); c.moveTo(sd * 6.f, -20); c.lineTo(sd * 16.f, -38); c.lineTo(sd * 16.f, -16); c.closePath(); fillOut(c, V(corpo), 2.5f); }
    c.save(); c.glow(10, occhi); c.fillStyle(V(occhi)); ell(c, -7, -8, 4.5f, 5); c.fill(); ell(c, 7, -8, 4.5f, 5); c.fill(); c.restore();
    c.fillStyle(V("#ffffff")); ell(c, -6, -10, 1.5f, 1.5f); c.fill(); ell(c, 8, -10, 1.5f, 1.5f); c.fill();
    c.fillStyle(V("#ffffff")); c.beginPath(); c.moveTo(-5, 2); c.lineTo(-3, 9); c.lineTo(-1, 2); c.moveTo(1, 2); c.lineTo(3, 9); c.lineTo(5, 2); c.fill();
    c.restore();
}
void DisegnaGolem(K& c, float x, float y, float s, const std::string& L, const std::string& M, const std::string& D, const std::string& G, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    if (P.a == Anim::Corsa) c.translate(0, -std::fabs(std::sin(P.ph)) * 4);
    if (P.a == Anim::Colpito) c.rotate(-0.12f);
    auto poly = [&](std::initializer_list<std::pair<float, float>> pts, const std::string& col) {
        c.beginPath(); bool primo = true; for (auto& p : pts) { if (primo) c.moveTo(p.first, p.second); else c.lineTo(p.first, p.second); primo = false; } c.closePath(); fillOut(c, V(col), 2.5f);
    };
    float pa = P.a == Anim::Corsa ? std::sin(P.ph) * 6 : 0;
    poly({ { -30 + pa, 0 }, { -26, -34 }, { -10, -30 }, { -12 + pa, 0 } }, D); poly({ { 12 - pa, 0 }, { 10, -30 }, { 26, -34 }, { 30 - pa, 0 } }, M);
    poly({ { -40, -36 }, { -30, -96 }, { 0, -110 }, { 34, -92 }, { 42, -36 }, { 0, -24 } }, M);
    poly({ { -40, -36 }, { -30, -96 }, { 0, -110 }, { 0, -24 } }, L); poly({ { 0, -110 }, { 34, -92 }, { 42, -36 }, { 0, -24 } }, D);
    float br = P.a == Anim::Attacco ? -30.f : 0.f;
    poly({ { -44, -88 }, { -64, -60 }, { -56, -30 }, { -38, -50 } }, L); poly({ { 44, -88 }, { 66 - br * 0.3f, -58 + br }, { 58 - br * 0.2f, -28 + br }, { 38, -50 } }, D);
    poly({ { -18, -110 }, { -10, -140 }, { 14, -142 }, { 22, -110 }, { 0, -102 } }, M); poly({ { -18, -110 }, { -10, -140 }, { 2, -141 }, { 0, -102 } }, L);
    c.save(); c.glow(18, G); c.fillStyle(V(G)); ell(c, -6, -124, 3.5f, 3.5f); c.fill(); ell(c, 10, -124, 3.5f, 3.5f); c.fill();
    float p = 0.8f + 0.2f * std::sin(t * 3); c.beginPath(); c.moveTo(0, -82); c.lineTo(10 * p, -68); c.lineTo(0, -54); c.lineTo(-10 * p, -68); c.closePath(); c.fill(); c.restore();
    c.fillStyle(V(rgba("#ffffff", 0.7f))); c.beginPath(); c.moveTo(-26, -90); c.lineTo(-20, -90); c.lineTo(-30, -50); c.lineTo(-34, -52); c.fill();
    c.restore();
}
void DisegnaDino(K& c, float x, float y, float s, const Dino& o, const Posa& P) {
    const float t = P.t; bool run = P.a == Anim::Corsa; float p = P.ph;
    c.save(); c.translate(x, y); c.scale(s * (o.flip ? -1 : 1), s);
    float bob = run ? -std::fabs(std::sin(p)) * 3 : std::sin(t * 2) * 1; c.translate(0, bob);
    if (P.a == Anim::Colpito) c.rotate(-0.15f);
    float legA = run ? std::sin(p) * 0.6f : 0.1f; if (P.a == Anim::Salto) legA = 0.5f;
    limb(c, -10, -50, -legA, 24, 0.6f, 22, 13, sh(o.c, -0.35f));
    c.beginPath(); c.moveTo(-20, -62); c.quadraticCurveTo(-70, -70, -96, -50 + std::sin(t * 4) * 6); c.quadraticCurveTo(-64, -50, -16, -40); c.closePath();
    fillOut(c, lg(0, -70, 0, -40, { sh(o.c, 0.2f), o.c, sh(o.c, -0.3f) }), 3);
    ell(c, 0, -58, 30, 24, -0.2f); fillOut(c, rg(-6, -70, 4, 36, { sh(o.c, 0.3f), o.c, sh(o.c, -0.35f) }), 3);
    c.save(); ell(c, 0, -58, 30, 24, -0.2f); c.clip(); c.fillStyle(V(o.belly)); ell(c, 10, -44, 22, 14, -0.3f); c.fill();
    c.fillStyle(V(o.stripe)); for (int k = 0; k < 4; k++) { c.beginPath(); c.moveTo(-24 + k * 12.f, -84); c.lineTo(-18 + k * 12.f, -60); c.lineTo(-14 + k * 12.f, -84); c.fill(); } c.restore();
    c.beginPath(); c.moveTo(14, -72); c.quadraticCurveTo(20, -96, 30, -104); c.lineTo(40, -86); c.quadraticCurveTo(30, -74, 26, -56); c.closePath(); fillOut(c, V(o.c), 3);
    float jaw = P.a == Anim::Attacco ? 0.45f : std::max(0.f, std::sin(t * 3)) * 0.25f;
    c.save(); c.translate(30, -104);
    c.beginPath(); c.moveTo(-8, -4); c.quadraticCurveTo(0, -22, 26, -16); c.quadraticCurveTo(46, -12, 46, 0); c.lineTo(10, 4); c.closePath(); fillOut(c, rg(10, -14, 2, 30, { sh(o.c, 0.35f), o.c }), 3);
    c.save(); c.rotate(jaw); c.beginPath(); c.moveTo(4, 2); c.lineTo(44, 2); c.quadraticCurveTo(40, 14, 10, 14); c.closePath(); fillOut(c, V(sh(o.c, -0.15f)), 3);
    c.fillStyle(V("#ffffff")); for (int k = 0; k < 5; k++) { c.beginPath(); c.moveTo(14 + k * 6.f, 2); c.lineTo(17 + k * 6.f, -3); c.lineTo(20 + k * 6.f, 2); c.fill(); } c.restore();
    c.fillStyle(V("#ffffff")); for (int k = 0; k < 5; k++) { c.beginPath(); c.moveTo(14 + k * 6.f, 0); c.lineTo(17 + k * 6.f, 6); c.lineTo(20 + k * 6.f, 0); c.fill(); }
    ell(c, 18, -10, 6, 6); fillOut(c, V("#fff6c0"), 2);
    if (P.a == Anim::Colpito) { c.strokeStyle(V(OUT)); c.lineWidth(2.5f); c.beginPath(); c.moveTo(14, -14); c.lineTo(22, -6); c.moveTo(22, -14); c.lineTo(14, -6); c.stroke(); }
    else { c.fillStyle(V("#1a0a0a")); ell(c, 20, -10, 2, 4.5f); c.fill(); }
    c.strokeStyle(V(OUT)); c.lineWidth(3); c.beginPath(); c.moveTo(10, -18); c.lineTo(26, -14); c.stroke();
    c.fillStyle(V("#1a1018")); ell(c, 40, -8, 1.6f, 1.2f); c.fill(); c.restore();
    c.strokeStyle(V(OUT)); c.lineWidth(7); c.beginPath(); c.moveTo(22, -66); c.lineTo(34, -58); c.lineTo(38, -64); c.stroke(); c.strokeStyle(V(o.c)); c.lineWidth(4); c.stroke();
    limb(c, 8, -46, legA, 24, 0.5f, 22, 14, o.c, rgba("#ffffff", 0.2f));
    c.restore();
}
void DisegnaDrone(K& c, float x, float y, float s, const std::string& col, const std::string& oc, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    if (P.a == Anim::Colpito) c.rotate(0.3f);
    for (int sd : { -1, 1 }) {
        c.strokeStyle(V("#3a3a4a")); c.lineWidth(4); c.beginPath(); c.moveTo(sd * 20.f, -50); c.lineTo(sd * 40.f, -66); c.stroke();
        ell(c, sd * 40.f, -70, 22 * std::max(0.2f, std::fabs(std::sin(t * 40 + sd))), 3); c.fillStyle(V("rgba(200,220,255,0.6)")); c.fill();
    }
    ell(c, 0, -44, 34, 18); fillOut(c, lg(0, -62, 0, -26, { sh(col, 0.45f), col, sh(col, -0.45f) }), 3);
    c.fillStyle(V(sh(col, -0.3f))); c.fillRect(-34, -46, 68, 4);
    ell(c, 0, -38, 12, 12); fillOut(c, V("#14141c"), 2.5f);
    c.save(); c.glow(18, oc); c.fillStyle(V(oc)); ell(c, 3, -37, 6, 6); c.fill(); c.restore();
    c.fillStyle(V("#ffffff")); ell(c, 5, -40, 2, 2); c.fill();
    if (P.a == Anim::Attacco) { c.save(); c.lighter(); c.strokeStyle(V(rgba(oc, 0.6f))); c.lineWidth(4); c.beginPath(); c.moveTo(4, -30); c.lineTo(40, 30); c.stroke(); c.restore(); }
    c.restore();
}
void DisegnaMedusa(K& c, float x, float y, float s, const std::string& col, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    c.strokeStyle(V(rgba(col, 0.7f))); c.lineWidth(3);
    for (int i = 0; i < 6; i++) { c.beginPath(); c.moveTo(-20 + i * 8.f, -40); for (int k = 1; k < 10; k++) c.lineTo(-20 + i * 8 + std::sin(t * 4 + k * 0.8f + i) * 5, -40 + k * 7.f); c.stroke(); }
    c.save(); c.glow(20, col);
    float sq = P.a == Anim::Attacco ? 0.85f : 1.f;
    c.beginPath(); c.moveTo(-30 * sq, -40); c.bezierCurveTo(-34 * sq, -90, 34 * sq, -90, 30 * sq, -40); c.quadraticCurveTo(0, -34, -30 * sq, -40);
    c.fillStyle(rg(-8, -70, 2, 40, { "rgba(255,255,255,0.9)", rgba(col, 0.75f), rgba(sh(col, -0.3f), 0.6f) })); c.fill(); c.restore();
    c.strokeStyle(V(rgba("#ffffff", 0.6f))); c.lineWidth(2); c.beginPath(); c.arc(0, -48, 22, -2.6f, -1.8f); c.stroke();
    bool ch = P.a == Anim::Colpito;
    eye(c, -8, -56, 0.7f, "#5a2a8a", false, ch); eye(c, 8, -56, 0.65f, "#5a2a8a", false, ch);
    if (P.a == Anim::Attacco) { c.save(); c.glow(12, "#fff6a0"); c.strokeStyle(V("#fff6a0")); c.lineWidth(2.5f); c.beginPath(); c.moveTo(-40, -60); c.lineTo(-30, -50); c.lineTo(-42, -44); c.lineTo(-32, -34); c.moveTo(40, -60); c.lineTo(30, -50); c.lineTo(42, -44); c.lineTo(32, -34); c.stroke(); c.restore(); }
    c.restore();
}
void DisegnaApe(K& c, float x, float y, float s, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    if (P.a == Anim::Colpito) c.rotate(0.35f);
    float f = std::sin(t * 40);
    for (int sd : { -1, 1 }) { ell(c, sd * -4.f - 6, -58 - f * 4, 18, 9, sd * 0.6f - 0.3f); c.fillStyle(V("rgba(230,245,255,0.6)")); c.fill(); c.strokeStyle(V("rgba(255,255,255,0.9)")); c.lineWidth(1.5f); c.stroke(); }
    ell(c, -8, -38, 26, 20); fillOut(c, V("#f2c23a"), 3);
    c.save(); ell(c, -8, -38, 26, 20); c.clip(); c.fillStyle(V("#2a1a10")); for (int k = 0; k < 3; k++) c.fillRect(-24 + k * 13.f, -60, 6, 44); c.restore();
    c.fillStyle(V("#2a1a10")); c.beginPath(); c.moveTo(-32, -38); c.lineTo(-44, -36); c.lineTo(-32, -32); c.fill();
    ell(c, 20, -46, 15, 14); fillOut(c, rg(16, -50, 2, 16, { "#5a4a3a", "#2a1a10" }), 3);
    eye(c, 22, -48, 0.7f, "#2a1a10", false, P.a == Anim::Colpito);
    c.strokeStyle(V(OUT)); c.lineWidth(2); c.beginPath(); c.moveTo(18, -58); c.quadraticCurveTo(14, -74, 8, -74); c.moveTo(26, -58); c.quadraticCurveTo(30, -74, 36, -72); c.stroke();
    float tr = P.a == Anim::Attacco ? 1.4f : 1.f;
    c.beginPath(); c.moveTo(32, -40); c.lineTo(32 + 24 * tr, -48 - 4 * tr); c.lineTo(32 + 26 * tr, -32 + 4 * tr); c.closePath(); fillOut(c, lg(32, -48, 58, -32, { "#fff2a8", "#d8a01a" }), 2);
    c.restore();
}
void DisegnaMech(K& c, float x, float y, float s, const std::string& m, const Posa& P) {
    const float t = P.t; c.save(); c.translate(x, y); c.scale(s, s);
    if (P.a == Anim::Corsa) c.translate(0, -std::fabs(std::sin(P.ph)) * 4);
    if (P.a == Anim::Colpito) c.rotate(-0.08f);
    for (int sd : { -1, 1 }) {
        c.beginPath(); c.moveTo(sd * 30.f, -70); c.lineTo(sd * 44.f, -30); c.lineTo(sd * 36.f, 0); c.lineTo(sd * 60.f, 0); c.lineTo(sd * 58.f, -30); c.lineTo(sd * 46.f, -72); c.closePath();
        fillOut(c, lg(sd * 30.f, 0, sd * 60.f, 0, { sh(m, 0.3f), m, sh(m, -0.4f) }), 3);
    }
    c.beginPath(); c.moveTo(-70, -150); c.lineTo(70, -150); c.lineTo(60, -64); c.lineTo(-60, -64); c.closePath(); fillOut(c, lg(-70, 0, 70, 0, { sh(m, 0.4f), m, sh(m, -0.45f) }), 3.5f);
    c.fillStyle(V(sh(m, -0.3f))); for (int k = -60; k <= 60; k += 20) { ell(c, (float)k, -144, 3, 3); c.fill(); }
    c.save(); c.translate(0, -104);
    c.fillStyle(V("#f2c94a")); c.beginPath(); c.moveTo(-16, -24); c.lineTo(16, -24); c.lineTo(2, 0); c.lineTo(16, 24); c.lineTo(-16, 24); c.lineTo(-2, 0); c.closePath(); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(2.5f); c.stroke();
    float k = std::fmod(t * 0.3f, 1.f);
    c.fillStyle(V("#ffe8a0")); c.beginPath(); c.moveTo(-10, -20 + k * 16); c.lineTo(10, -20 + k * 16); c.lineTo(1, -2); c.closePath(); c.fill();
    c.beginPath(); c.moveTo(-10 * k, 20 - k * 12); c.lineTo(10 * k, 20 - k * 12); c.lineTo(12, 22); c.lineTo(-12, 22); c.closePath(); c.fill(); c.restore();
    for (int sd : { -1, 1 }) {
        c.save(); c.translate(sd * 74.f, -138); float rot = sd * (0.3f + std::sin(t * 2) * 0.15f); if (P.a == Anim::Attacco && sd > 0) rot = -1.3f; c.rotate(rot);
        c.fillStyle(lg(-12, 0, 12, 0, { sh(m, 0.3f), m, sh(m, -0.4f) })); c.beginPath(); c.rect(-12, 0, 24, 64); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
        c.beginPath(); c.moveTo(-18, 64); c.lineTo(18, 64); c.lineTo(14, 84); c.lineTo(-14, 84); c.closePath(); fillOut(c, V("#f24a4a"), 3); c.restore();
    }
    c.beginPath(); c.arc(0, -150, 40, TAU / 2, TAU); c.closePath(); c.fillStyle(V("rgba(170,230,255,0.35)")); c.fill(); c.strokeStyle(V(OUT)); c.lineWidth(3); c.stroke();
    // testa del dottore
    c.save(); c.translate(0, -168); c.scale(0.9f, 0.9f);
    ell(c, 0, 0, 26, 25); fillOut(c, rg(-8, -8, 3, 30, { sh("#f2c39a", 0.25f), "#f2c39a", sh("#f2c39a", -0.2f) }), 3);
    c.fillStyle(V("#e8e8f0")); c.beginPath(); c.moveTo(-26, 4); c.quadraticCurveTo(-34, -18, -18, -16); c.lineTo(-22, -4); c.closePath(); c.fill(); c.beginPath(); c.moveTo(26, 4); c.quadraticCurveTo(34, -18, 18, -16); c.lineTo(22, -4); c.closePath(); c.fill();
    eye(c, -9, -2, 0.8f, "#5a3a8a", false, P.a == Anim::Colpito); eye(c, 9, -2, 0.8f, "#5a3a8a", false, P.a == Anim::Colpito);
    c.strokeStyle(V("#c8a03a")); c.lineWidth(2.5f); ell(c, 9, -2, 8, 8); c.stroke();
    c.strokeStyle(V("#e8e8f0")); c.lineWidth(4); c.beginPath(); c.moveTo(-17, -14); c.lineTo(-3, -9); c.moveTo(3, -9); c.lineTo(17, -14); c.stroke();
    c.fillStyle(V("#e8e8f0")); c.beginPath(); c.moveTo(-12, 10); c.quadraticCurveTo(0, 4, 12, 10); c.quadraticCurveTo(14, 16, 8, 14); c.quadraticCurveTo(0, 10, -8, 14); c.quadraticCurveTo(-14, 16, -12, 10); c.fill();
    c.strokeStyle(V(OUT)); c.lineWidth(1.5f); c.stroke(); c.restore();
    c.strokeStyle(V("rgba(255,255,255,0.7)")); c.lineWidth(4); c.beginPath(); c.arc(0, -150, 34, -2.7f, -2.1f); c.stroke();
    c.save(); c.glow(16, "#ff4a4a"); c.fillStyle(V("#ff4a4a")); for (int sd : { -1, 1 }) { ell(c, sd * 48.f, -120, 6, 6); c.fill(); } c.restore();
    c.restore();
}

// --- da funzione di disegno a Disegno con animazioni ---------------------------------------------
static const char* NomeAnim(Anim a) {
    switch (a) { case Anim::Fermo: return "fermo"; case Anim::Corsa: return "corsa"; case Anim::Salto: return "salto"; case Anim::Caduta: return "caduta";
                 case Anim::Attacco: return "attacco"; case Anim::Colpito: return "colpito"; case Anim::Vittoria: return "vittoria"; }
    return "fermo";
}
Disegno CreaDisegno(const DisegnaPersonaggio& f, const OpzioniDisegno& o) {
    // scala: l'ingombro 'alto' (+sopra) diventa altezzaMondo*dens pixel del disegno
    float pxAlto = o.altezzaMondo * o.dens;
    float k = pxAlto / o.alto;
    int W = (int)std::ceil(o.largo * k), Hh = (int)std::ceil((o.alto + o.sopra) * k);
    W += W % 2; Hh += 2;
    Disegno d; d.Nuovo(W, Hh, o.dens); d.liscio = o.liscio; d.animazioni.clear();
    for (Anim a : o.animazioni) {
        Animazione an; an.nome = NomeAnim(a);
        int n = 1; float fps = 8; bool ripeti = true;
        if (a == Anim::Corsa) { n = o.fotogrammiCorsa; fps = 14; }
        else if (a == Anim::Fermo) { n = o.fotogrammiFermo; fps = 5; }
        else if (a == Anim::Attacco) { n = 4; fps = 14; ripeti = false; }
        else if (a == Anim::Vittoria) { n = 4; fps = 6; }
        else if (a == Anim::Colpito) { n = 2; fps = 10; }
        else if (a == Anim::Salto || a == Anim::Caduta) { n = 2; fps = 8; }
        an.fps = fps; an.ripeti = ripeti;
        for (int i = 0; i < n; i++) {
            Posa p; p.a = a;
            if (a == Anim::Corsa) { p.ph = TAU * i / n; p.t = (float)i / n * 0.6f; }
            else if (a == Anim::Attacco) p.t = (i + 0.5f) / n;
            else p.t = (float)i / n * (a == Anim::Fermo ? 2.85f : 1.6f);
            Tela tela(W, Hh); K c(tela);
            f(c, W / 2.f, Hh - 1.f, k, p);
            Image img = tela.Immagine();
            Fotogramma fo = FotogrammaDaImmagine(img, { 0, 0, (float)W, (float)Hh });
            UnloadImage(img);
            an.fotogrammi.push_back(fo);
        }
        d.animazioni.push_back(an);
    }
    return d;
}

}  // namespace arte
