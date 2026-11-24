// Pennello: disegnatore vettoriale con antialias (vedi pennello.h).
#include "pennello.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace pennello {

static float Lim01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

Colore Esa(unsigned rgb, float alfa) { return { ((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, alfa }; }
Colore Esa(const char* hex, float alfa) {
    if (!hex) return { 0, 0, 0, alfa };
    if (*hex == '#') hex++;
    unsigned v = 0; int n = 0;
    for (const char* p = hex; *p && n < 6; p++, n++) {
        char ch = *p; int d = (ch >= '0' && ch <= '9') ? ch - '0' : (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10 : 0;
        v = v * 16 + d;
    }
    if (n == 3) { unsigned r = (v >> 8) & 15, g = (v >> 4) & 15, b = v & 15; v = (r * 17 << 16) | (g * 17 << 8) | (b * 17); }
    return Esa(v, alfa);
}
Colore Schiarisci(Colore c, float k) {
    float t = k > 0 ? 1.f : 0.f, a = std::fabs(k);
    return { c.r + (t - c.r) * a, c.g + (t - c.g) * a, c.b + (t - c.b) * a, c.a };
}
Colore Mescola(Colore a, Colore b, float k) { return { a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k, a.a + (b.a - a.a) * k }; }
Colore ConAlfa(Colore c, float a) { c.a = a; return c; }

float Matrice::Scala() const { return std::sqrt(std::fabs(a * d - b * c)); }
static Matrice Inversa(const Matrice& m) {
    float det = m.a * m.d - m.b * m.c; if (std::fabs(det) < 1e-12f) det = 1e-12f;
    Matrice r; r.a = m.d / det; r.b = -m.b / det; r.c = -m.c / det; r.d = m.a / det;
    r.e = -(r.a * m.e + r.c * m.f); r.f = -(r.b * m.e + r.d * m.f); return r;
}

Vernice Lineare(float x0, float y0, float x1, float y1, std::initializer_list<Colore> colori) {
    Vernice v; v.tipo = 1; v.x0 = x0; v.y0 = y0; v.x1 = x1; v.y1 = y1;
    int n = (int)colori.size(), i = 0;
    for (const Colore& c : colori) v.fermate.push_back({ n > 1 ? (float)i++ / (n - 1) : 0.f, c });
    return v;
}
Vernice Radiale(float x, float y, float r0, float r1, std::initializer_list<Colore> colori) {
    Vernice v; v.tipo = 2; v.x0 = x; v.y0 = y; v.r0 = r0; v.r1 = r1;
    int n = (int)colori.size(), i = 0;
    for (const Colore& c : colori) v.fermate.push_back({ n > 1 ? (float)i++ / (n - 1) : 0.f, c });
    return v;
}

Tela::Tela(int larghezza, int altezza) : w(std::max(1, larghezza)), h(std::max(1, altezza)), px((size_t)w * h * 4, 0.f) {}

void Tela::Salva() { pila.push_back({ matrice, alfaGlobale, modo, spessore, riempimento, tratto, bagliore, coloreBagliore, ritaglio, haRitaglio }); }
void Tela::Ripristina() {
    if (pila.empty()) return;
    Stato& s = pila.back();
    matrice = s.m; alfaGlobale = s.alfa; modo = s.modo; spessore = s.spessore; riempimento = s.riemp; tratto = s.tratto;
    bagliore = s.bagliore; coloreBagliore = s.cb; ritaglio.swap(s.ritaglio); haRitaglio = s.haRitaglio;
    pila.pop_back();
}
void Tela::Trasla(float x, float y) { matrice.e += matrice.a * x + matrice.c * y; matrice.f += matrice.b * x + matrice.d * y; }
void Tela::Scala(float sx, float sy) { matrice.a *= sx; matrice.b *= sx; matrice.c *= sy; matrice.d *= sy; }
void Tela::Ruota(float ang) {
    float cs = std::cos(ang), sn = std::sin(ang);
    Matrice m = matrice;
    matrice.a = m.a * cs + m.c * sn; matrice.b = m.b * cs + m.d * sn;
    matrice.c = -m.a * sn + m.c * cs; matrice.d = -m.b * sn + m.d * cs;
}
void Tela::Imposta(float a, float b, float c, float d, float e, float f) { matrice = { a, b, c, d, e, f }; }

// --- tracciati ---------------------------------------------------------------------------------
void Tela::Inizia() { sotto.clear(); chiuso.clear(); haUltimo = false; }
void Tela::Aggiungi(Vector2 p) {
    if (sotto.empty()) { sotto.push_back({}); chiuso.push_back(false); inizioSotto = p; }
    sotto.back().push_back(p); ultimo = p; haUltimo = true;
}
void Tela::Muovi(float x, float y) {
    Vector2 p = matrice.Applica(x, y);
    sotto.push_back({ p }); chiuso.push_back(false); inizioSotto = p; ultimo = p; haUltimo = true;
}
void Tela::Linea(float x, float y) { Vector2 p = matrice.Applica(x, y); if (!haUltimo) { Muovi(x, y); return; } Aggiungi(p); }
void Tela::Quadratica(float cx, float cy, float x, float y) {
    if (!haUltimo) Muovi(cx, cy);
    Vector2 p0 = ultimo, p1 = matrice.Applica(cx, cy), p2 = matrice.Applica(x, y);
    float len = std::hypot(p1.x - p0.x, p1.y - p0.y) + std::hypot(p2.x - p1.x, p2.y - p1.y);
    int n = std::max(2, std::min(64, (int)(len / 3)));
    for (int i = 1; i <= n; i++) {
        float t = (float)i / n, u = 1 - t;
        Aggiungi({ u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x, u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y });
    }
}
void Tela::Bezier(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    if (!haUltimo) Muovi(c1x, c1y);
    Vector2 p0 = ultimo, p1 = matrice.Applica(c1x, c1y), p2 = matrice.Applica(c2x, c2y), p3 = matrice.Applica(x, y);
    float len = std::hypot(p1.x - p0.x, p1.y - p0.y) + std::hypot(p2.x - p1.x, p2.y - p1.y) + std::hypot(p3.x - p2.x, p3.y - p2.y);
    int n = std::max(2, std::min(80, (int)(len / 3)));
    for (int i = 1; i <= n; i++) {
        float t = (float)i / n, u = 1 - t;
        float a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
        Aggiungi({ a * p0.x + b * p1.x + c * p2.x + d * p3.x, a * p0.y + b * p1.y + c * p2.y + d * p3.y });
    }
}
void Tela::Ellisse(float x, float y, float rx, float ry, float rot, float a0, float a1, bool antiorario) {
    rx = std::max(0.05f, std::fabs(rx)); ry = std::max(0.05f, std::fabs(ry));
    float d;
    if (!antiorario) { d = a1 - a0; if (d >= PI2) d = PI2; else { d = std::fmod(d, PI2); if (d < 0) d += PI2; } }
    else { d = a0 - a1; if (d >= PI2) d = PI2; else { d = std::fmod(d, PI2); if (d < 0) d += PI2; } d = -d; }
    float rpx = std::max(rx, ry) * matrice.Scala();
    int n = std::max(6, std::min(160, (int)(std::fabs(d) * std::max(2.f, rpx) * 0.9f)));
    float cr = std::cos(rot), sr = std::sin(rot);
    for (int i = 0; i <= n; i++) {
        float a = a0 + d * i / n;
        float ex = std::cos(a) * rx, ey = std::sin(a) * ry;
        Vector2 p = matrice.Applica(x + ex * cr - ey * sr, y + ex * sr + ey * cr);
        if (i == 0 && !haUltimo) { sotto.push_back({ p }); chiuso.push_back(false); inizioSotto = p; ultimo = p; haUltimo = true; }
        else Aggiungi(p);
    }
}
void Tela::Arco(float x, float y, float r, float a0, float a1, bool antiorario) { Ellisse(x, y, r, r, 0, a0, a1, antiorario); }
void Tela::Rettangolo(float x, float y, float l, float a) { Muovi(x, y); Linea(x + l, y); Linea(x + l, y + a); Linea(x, y + a); Chiudi(); }
void Tela::Chiudi() {
    if (sotto.empty()) return;
    chiuso.back() = true;
    Vector2 s = sotto.back().front();
    sotto.push_back({ s }); chiuso.push_back(false); inizioSotto = s; ultimo = s; haUltimo = true;
}

// --- copertura con antialias (5 sottorighe per pixel, copertura orizzontale esatta) -----------
void Tela::Copertura(const std::vector<std::vector<Vector2>>& poli, int& bx, int& by, int& bw, int& bh, std::vector<float>& cop) const {
    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    struct Lato { float x0, y0, x1, y1; int dir; };
    std::vector<Lato> lati;
    for (const auto& p : poli) {
        if (p.size() < 2) continue;
        for (size_t i = 0; i < p.size(); i++) {
            Vector2 a = p[i], b = p[(i + 1) % p.size()];
            minx = std::min(minx, a.x); maxx = std::max(maxx, a.x); miny = std::min(miny, a.y); maxy = std::max(maxy, a.y);
            if (a.y == b.y) continue;
            if (a.y < b.y) lati.push_back({ a.x, a.y, b.x, b.y, 1 }); else lati.push_back({ b.x, b.y, a.x, a.y, -1 });
        }
    }
    bx = std::max(0, (int)std::floor(minx)); by = std::max(0, (int)std::floor(miny));
    int ex = std::min(w, (int)std::ceil(maxx) + 1), ey = std::min(h, (int)std::ceil(maxy) + 1);
    bw = ex - bx; bh = ey - by;
    if (bw <= 0 || bh <= 0 || lati.empty()) { bw = bh = 0; cop.clear(); return; }
    cop.assign((size_t)bw * bh, 0.f);
    std::sort(lati.begin(), lati.end(), [](const Lato& a, const Lato& b) { return a.y0 < b.y0; });
    const int SUB = 5; const float peso = 1.f / SUB;
    std::vector<std::pair<float, int>> inc;
    std::vector<int> attivi; size_t prossimo = 0;
    for (int y = by; y < ey; y++) {
        float* riga = &cop[(size_t)(y - by) * bw];
        for (int s = 0; s < SUB; s++) {
            float sy = y + (s + 0.5f) / SUB;
            while (prossimo < lati.size() && lati[prossimo].y0 <= sy) attivi.push_back((int)prossimo++);
            inc.clear();
            for (size_t k = 0; k < attivi.size();) {
                const Lato& l = lati[attivi[k]];
                if (l.y1 <= sy) { attivi[k] = attivi.back(); attivi.pop_back(); continue; }
                if (l.y0 <= sy) inc.push_back({ l.x0 + (sy - l.y0) * (l.x1 - l.x0) / (l.y1 - l.y0), l.dir });
                k++;
            }
            if (inc.size() < 2) continue;
            std::sort(inc.begin(), inc.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first < b.first; });
            int avv = 0;
            for (size_t k = 0; k + 1 < inc.size(); k++) {
                avv += inc[k].second;
                if (avv == 0) continue;
                float xa = std::max((float)bx, inc[k].first), xb = std::min((float)ex, inc[k + 1].first);
                if (xb <= xa) continue;
                int ia = (int)xa, ib = (int)xb;
                if (ia == ib) { riga[ia - bx] += (xb - xa) * peso; continue; }
                riga[ia - bx] += (ia + 1 - xa) * peso;
                for (int i = ia + 1; i < ib; i++) riga[i - bx] += peso;
                if (ib < ex) riga[ib - bx] += (xb - ib) * peso;
            }
        }
    }
    for (float& v : cop) v = Lim01(v);
    if (haRitaglio)
        for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) cop[(size_t)y * bw + x] *= ritaglio[(size_t)(y + by) * w + x + bx];
}

Colore Tela::Campiona(const Vernice& v, float x, float y, const Matrice& inv) const {
    if (v.tipo == 0 || v.fermate.empty()) return v.pieno;
    Vector2 u = inv.Applica(x, y);
    float t;
    if (v.tipo == 1) {
        float dx = v.x1 - v.x0, dy = v.y1 - v.y0, l2 = dx * dx + dy * dy;
        t = l2 > 0 ? ((u.x - v.x0) * dx + (u.y - v.y0) * dy) / l2 : 0;
    } else {
        float d = std::hypot(u.x - v.x0, u.y - v.y0);
        t = v.r1 > v.r0 ? (d - v.r0) / (v.r1 - v.r0) : 0;
    }
    t = Lim01(t);
    const auto& f = v.fermate;
    if (t <= f.front().first) return f.front().second;
    for (size_t i = 1; i < f.size(); i++)
        if (t <= f[i].first) {
            float k = f[i].first > f[i - 1].first ? (t - f[i - 1].first) / (f[i].first - f[i - 1].first) : 1;
            Colore a = f[i - 1].second, b = f[i].second;
            // interpolazione in premoltiplicato (come i browser)
            float aa = a.a + (b.a - a.a) * k;
            if (aa <= 0) return { 0, 0, 0, 0 };
            return { (a.r * a.a + (b.r * b.a - a.r * a.a) * k) / aa, (a.g * a.a + (b.g * b.a - a.g * a.a) * k) / aa,
                     (a.b * a.a + (b.b * b.a - a.b * a.a) * k) / aa, aa };
        }
    return f.back().second;
}

void Tela::Componi(const std::vector<float>& cop, int bx, int by, int bw, int bh, const Vernice& v) {
    Matrice inv = Inversa(matrice);
    for (int y = 0; y < bh; y++)
        for (int x = 0; x < bw; x++) {
            float c = cop[(size_t)y * bw + x];
            if (c <= 0.0005f) continue;
            Colore col = Campiona(v, bx + x + 0.5f, by + y + 0.5f, inv);
            float a = Lim01(col.a * c * alfaGlobale);
            if (a <= 0) continue;
            float* d = &px[((size_t)(by + y) * w + bx + x) * 4];
            if (modo == Modo::Sopra) { float k = 1 - a; d[0] = col.r * a + d[0] * k; d[1] = col.g * a + d[1] * k; d[2] = col.b * a + d[2] * k; d[3] = a + d[3] * k; }
            else if (modo == Modo::Somma) { d[0] = std::min(1.f, d[0] + col.r * a); d[1] = std::min(1.f, d[1] + col.g * a); d[2] = std::min(1.f, d[2] + col.b * a); d[3] = std::min(1.f, d[3] + a); }
            else { float k = 1 - a; d[0] *= k; d[1] *= k; d[2] *= k; d[3] *= k; }
        }
}

static void SfocaCanale(std::vector<float>& v, int bw, int bh, int stride, int canali, int r) {
    if (r < 1) return;
    std::vector<float> tmp(std::max(bw, bh));
    for (int pass = 0; pass < 3; pass++) {
        for (int ch = 0; ch < canali; ch++) {
            for (int y = 0; y < bh; y++) {   // orizzontale
                float somma = 0; int n = 2 * r + 1;
                for (int x = -r; x <= r; x++) somma += v[((size_t)y * stride + std::min(bw - 1, std::max(0, x))) * canali + ch];
                for (int x = 0; x < bw; x++) {
                    tmp[x] = somma / n;
                    int xa = std::max(0, x - r), xb = std::min(bw - 1, x + r + 1);
                    somma += v[((size_t)y * stride + xb) * canali + ch] - v[((size_t)y * stride + xa) * canali + ch];
                }
                for (int x = 0; x < bw; x++) v[((size_t)y * stride + x) * canali + ch] = tmp[x];
            }
            for (int x = 0; x < bw; x++) {   // verticale
                float somma = 0; int n = 2 * r + 1;
                for (int y = -r; y <= r; y++) somma += v[((size_t)std::min(bh - 1, std::max(0, y)) * stride + x) * canali + ch];
                for (int y = 0; y < bh; y++) {
                    tmp[y] = somma / n;
                    int ya = std::max(0, y - r), yb = std::min(bh - 1, y + r + 1);
                    somma += v[((size_t)yb * stride + x) * canali + ch] - v[((size_t)ya * stride + x) * canali + ch];
                }
                for (int y = 0; y < bh; y++) v[((size_t)y * stride + x) * canali + ch] = tmp[y];
            }
        }
    }
}

void Tela::ComponiBagliore(const std::vector<float>& cop, int bx, int by, int bw, int bh) {
    int r = std::max(1, (int)(bagliore * 0.5f * 0.6f));
    int m = r * 3;
    int nx = std::max(0, bx - m), ny = std::max(0, by - m), ex = std::min(w, bx + bw + m), ey = std::min(h, by + bh + m);
    int nw = ex - nx, nh = ey - ny;
    if (nw <= 0 || nh <= 0) return;
    std::vector<float> g((size_t)nw * nh, 0.f);
    for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) g[(size_t)(y + by - ny) * nw + x + bx - nx] = cop[(size_t)y * bw + x];
    SfocaCanale(g, nw, nh, nw, 1, r);
    Vernice v(coloreBagliore);
    Modo vecchio = modo; if (modo == Modo::Cancella) return;
    Componi(g, nx, ny, nw, nh, v);
    modo = vecchio;
}

void Tela::Riempi() {
    int bx, by, bw, bh; std::vector<float> cop;
    Copertura(sotto, bx, by, bw, bh, cop);
    if (bw <= 0) return;
    if (bagliore > 0 && coloreBagliore.a > 0) ComponiBagliore(cop, bx, by, bw, bh);
    Componi(cop, bx, by, bw, bh, riempimento);
}

static float Area(const std::vector<Vector2>& p) { float a = 0; for (size_t i = 0; i < p.size(); i++) { const Vector2& u = p[i]; const Vector2& v = p[(i + 1) % p.size()]; a += u.x * v.y - v.x * u.y; } return a * 0.5f; }

std::vector<std::vector<Vector2>> Tela::PoligoniTratto() const {
    std::vector<std::vector<Vector2>> out;
    float r = std::max(0.3f, spessore * matrice.Scala() * 0.5f);
    int nc = std::max(8, std::min(48, (int)(r * 2)));
    auto cerchio = [&](Vector2 c) { std::vector<Vector2> p; for (int i = 0; i < nc; i++) { float a = PI2 * i / nc; p.push_back({ c.x + std::cos(a) * r, c.y + std::sin(a) * r }); } out.push_back(p); };
    for (size_t s = 0; s < sotto.size(); s++) {
        std::vector<Vector2> pts = sotto[s];
        if (pts.empty()) continue;
        if (chiuso[s] && pts.size() > 1) pts.push_back(pts.front());
        if (pts.size() == 1) continue;
        for (size_t i = 0; i + 1 < pts.size(); i++) {
            Vector2 a = pts[i], b = pts[i + 1];
            float dx = b.x - a.x, dy = b.y - a.y, l = std::hypot(dx, dy);
            if (l < 1e-4f) continue;
            float nx = -dy / l * r, ny = dx / l * r;
            std::vector<Vector2> q = { { a.x + nx, a.y + ny }, { b.x + nx, b.y + ny }, { b.x - nx, b.y - ny }, { a.x - nx, a.y - ny } };
            if (Area(q) < 0) std::reverse(q.begin(), q.end());
            out.push_back(q);
        }
        // giunture e estremi arrotondati
        for (size_t i = 0; i < pts.size(); i++) {
            if (r < 0.9f && i != 0 && i + 1 != pts.size()) continue;
            if (i > 0 && i + 1 < pts.size()) {   // salta i punti quasi allineati (curve gia' spezzate)
                Vector2 a = pts[i - 1], b = pts[i], c = pts[i + 1];
                float cr = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
                float la = std::hypot(b.x - a.x, b.y - a.y), lc = std::hypot(c.x - b.x, c.y - b.y);
                if (la > 0 && lc > 0 && std::fabs(cr) / (la * lc) < 0.02f && ((b.x - a.x) * (c.x - b.x) + (b.y - a.y) * (c.y - b.y)) > 0) continue;
            }
            cerchio(pts[i]);
        }
    }
    for (auto& p : out) if (Area(p) < 0) std::reverse(p.begin(), p.end());
    return out;
}

void Tela::Contorna() {
    auto poli = PoligoniTratto();
    int bx, by, bw, bh; std::vector<float> cop;
    Copertura(poli, bx, by, bw, bh, cop);
    if (bw <= 0) return;
    if (bagliore > 0 && coloreBagliore.a > 0) ComponiBagliore(cop, bx, by, bw, bh);
    Componi(cop, bx, by, bw, bh, tratto);
}

void Tela::Ritaglia() {
    int bx, by, bw, bh; std::vector<float> cop;
    bool aveva = haRitaglio; haRitaglio = false;
    Copertura(sotto, bx, by, bw, bh, cop);
    haRitaglio = aveva;
    std::vector<float> nuovo((size_t)w * h, 0.f);
    for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) {
        size_t i = (size_t)(y + by) * w + x + bx;
        nuovo[i] = cop[(size_t)y * bw + x] * (haRitaglio ? ritaglio[i] : 1.f);
    }
    ritaglio.swap(nuovo); haRitaglio = true;
}

void Tela::RiempiRettangolo(float x, float y, float l, float a) {
    auto s = sotto; auto c = chiuso; Vector2 u = ultimo, iz = inizioSotto; bool hu = haUltimo;
    Inizia(); Rettangolo(x, y, l, a); Riempi();
    sotto = s; chiuso = c; ultimo = u; inizioSotto = iz; haUltimo = hu;
}
void Tela::Pulisci() { std::fill(px.begin(), px.end(), 0.f); }

void Tela::Sfoca(float raggio) { SfocaCanale(px, w, h, w, 4, std::max(1, (int)(raggio * 0.6f))); }

void Tela::Disegna(const Tela& o, float fx, float fy) {
    int ox = (int)std::lround(fx), oy = (int)std::lround(fy);
    for (int y = 0; y < o.h; y++) {
        int ty = y + oy; if (ty < 0 || ty >= h) continue;
        for (int x = 0; x < o.w; x++) {
            int tx = x + ox; if (tx < 0 || tx >= w) continue;
            const float* s = &o.px[((size_t)y * o.w + x) * 4];
            float a = s[3] * alfaGlobale; if (a <= 0) continue;
            float k = alfaGlobale;
            float* d = &px[((size_t)ty * w + tx) * 4];
            if (modo == Modo::Sopra) { float q = 1 - a; for (int c = 0; c < 3; c++) d[c] = s[c] * k + d[c] * q; d[3] = a + d[3] * q; }
            else if (modo == Modo::Somma) { for (int c = 0; c < 3; c++) d[c] = std::min(1.f, d[c] + s[c] * k); d[3] = std::min(1.f, d[3] + a); }
            else { float q = 1 - a; for (int c = 0; c < 4; c++) d[c] *= q; }
        }
    }
}

Image Tela::Parte(int x0, int y0, int l, int a) const {
    Image img = GenImageColor(std::max(1, l), std::max(1, a), BLANK);
    unsigned char* dst = (unsigned char*)img.data;
    for (int y = 0; y < a; y++) for (int x = 0; x < l; x++) {
        int sx = x + x0, sy = y + y0;
        unsigned char* d = &dst[((size_t)y * l + x) * 4];
        if (sx < 0 || sy < 0 || sx >= w || sy >= h) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
        const float* s = &px[((size_t)sy * w + sx) * 4];
        float al = Lim01(s[3]);
        if (al <= 0.002f) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
        for (int c = 0; c < 3; c++) d[c] = (unsigned char)std::lround(Lim01(s[c] / al) * 255);
        d[3] = (unsigned char)std::lround(al * 255);
    }
    return img;
}
Image Tela::Immagine() const { return Parte(0, 0, w, h); }

}  // namespace pennello
