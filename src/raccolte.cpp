#include "raccolte.h"
#include <algorithm>
#include <functional>
#include <map>

namespace {

// --- La tela: forme ombreggiate, contorno, antialiasing per lo stile disegnato -----------
//
// Le misure sono in "unita'": un'unita' e' mezzo pixel del mondo. In pixel art fine
// un'unita' e' un pixel del disegno (densita' 2); nello stile disegnato sono 2 pixel
// (densita' 4) e si disegna a doppia risoluzione per poi rimpicciolire (bordi morbidi).

constexpr unsigned SCURO = 0x1c1430, CHIARO = 0xfff4d8;

struct Tela {
    int W = 0, H = 0, ss = 1;
    float k = 1;
    bool hd = false;
    std::vector<Color> px;

    Tela(int w0, int h0, StileGrafico s) {
        hd = s == StileGrafico::Disegnato;
        ss = hd ? 2 : 1;
        k = (hd ? 2.0f : 1.0f) * ss;
        W = (int)(w0 * k); H = (int)(h0 * k);
        px.assign((size_t)W * H, BLANK);
    }

    Color Tono(Color c, float luce) const {
        if (!hd) luce = luce > 0.38f ? 0.55f : (luce < -0.4f ? -0.6f : 0.0f);
        luce = Limita(luce, -1, 1);
        Color r = luce >= 0 ? Mescola(c, Hex(CHIARO), luce * 0.42f) : Mescola(c, Hex(SCURO), -luce * 0.5f);
        r.a = c.a;
        return r;
    }

    void Punto(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= W || y >= H || c.a == 0) return;
        Color& d = px[(size_t)y * W + x];
        if (c.a == 255 || d.a == 0) { d = c; return; }
        float a = c.a / 255.0f;
        Color m = Mescola(d, c, a);
        m.a = (unsigned char)std::max<int>(d.a, c.a);
        d = m;
    }

    template <typename F>
    void Area(float x0, float y0, float x1, float y1, F f) {
        int a = std::max(0, (int)std::floor(x0 * k)), b = std::min(W - 1, (int)std::ceil(x1 * k));
        int c = std::max(0, (int)std::floor(y0 * k)), d = std::min(H - 1, (int)std::ceil(y1 * k));
        for (int y = c; y <= d; y++)
            for (int x = a; x <= b; x++) f(x, y, (x + 0.5f) / k, (y + 0.5f) / k);
    }

    void Ellisse(float cx, float cy, float rx, float ry, Color c, float ombra = 1) {
        if (rx <= 0 || ry <= 0) return;
        Area(cx - rx, cy - ry, cx + rx, cy + ry, [&](int x, int y, float fx, float fy) {
            float u = (fx - cx) / rx, v = (fy - cy) / ry;
            if (u * u + v * v > 1) return;
            Punto(x, y, Tono(c, -(u * 0.55f + v * 0.75f) * ombra));
        });
    }

    void Rett(float x, float y, float w, float h, Color c, float ombra = 1, float raggio = 0) {
        Area(x, y, x + w, y + h, [&](int px_, int py_, float fx, float fy) {
            if (fx < x || fy < y || fx > x + w || fy > y + h) return;
            if (raggio > 0) {
                float qx = std::max(std::max(x + raggio - fx, fx - (x + w - raggio)), 0.0f);
                float qy = std::max(std::max(y + raggio - fy, fy - (y + h - raggio)), 0.0f);
                if (qx * qx + qy * qy > raggio * raggio) return;
            }
            float u = (fx - (x + w / 2)) / (w / 2), v = (fy - (y + h / 2)) / (h / 2);
            Punto(px_, py_, Tono(c, -(u * 0.35f + v * 0.65f) * ombra));
        });
    }

    void Poligono(const std::vector<Vector2>& p, Color c, float ombra = 1) {
        if (p.size() < 3) return;
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
        for (auto& q : p) { x0 = std::min(x0, q.x); y0 = std::min(y0, q.y); x1 = std::max(x1, q.x); y1 = std::max(y1, q.y); }
        float cx = (x0 + x1) / 2, cy = (y0 + y1) / 2, hw = std::max(0.5f, (x1 - x0) / 2), hh = std::max(0.5f, (y1 - y0) / 2);
        Area(x0, y0, x1, y1, [&](int x, int y, float fx, float fy) {
            bool dentro = false;
            for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
                if (((p[i].y > fy) != (p[j].y > fy)) && (fx < (p[j].x - p[i].x) * (fy - p[i].y) / (p[j].y - p[i].y) + p[i].x)) dentro = !dentro;
            if (!dentro) return;
            float u = (fx - cx) / hw, v = (fy - cy) / hh;
            Punto(x, y, Tono(c, -(u * 0.4f + v * 0.6f) * ombra));
        });
    }

    void Linea(Vector2 a, Vector2 b, float sp, Color c, float ombra = 1) {
        float r = sp / 2;
        Area(std::min(a.x, b.x) - r, std::min(a.y, b.y) - r, std::max(a.x, b.x) + r, std::max(a.y, b.y) + r, [&](int x, int y, float fx, float fy) {
            float dx = b.x - a.x, dy = b.y - a.y, l2 = dx * dx + dy * dy;
            float t = l2 > 0 ? Limita(((fx - a.x) * dx + (fy - a.y) * dy) / l2, 0, 1) : 0;
            float qx = a.x + dx * t - fx, qy = a.y + dy * t - fy;
            float d = std::sqrt(qx * qx + qy * qy);
            if (d > r) return;
            // luce dal lato in alto a sinistra della linea
            float nx = -qx / std::max(0.01f, r), ny = -qy / std::max(0.01f, r);
            Punto(x, y, Tono(c, (nx * 0.5f + ny * 0.7f) * -ombra * 0.8f));
        });
    }

    void Occhio(float cx, float cy, float r, Color iride = Hex(0x1a1426), float guardaX = 0.3f) {
        Ellisse(cx, cy, r, r * 1.15f, Hex(0xffffff), 0.2f);
        Ellisse(cx + guardaX * r * 0.5f, cy + r * 0.1f, r * 0.6f, r * 0.75f, iride, 0);
        if (r * k >= 2.5f) Ellisse(cx + guardaX * r * 0.5f - r * 0.2f, cy - r * 0.25f, r * 0.22f, r * 0.22f, Hex(0xffffff), 0);
    }

    Fotogramma Finisci(bool contorno = true) {
        if (contorno) {
            std::vector<Color> out = px;
            int raggio = ss;
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) {
                    if (px[(size_t)y * W + x].a > 40) continue;
                    Color vicino = BLANK;
                    for (int dy = -raggio; dy <= raggio && vicino.a == 0; dy++)
                        for (int dx = -raggio; dx <= raggio; dx++) {
                            if ((dx == 0 && dy == 0) || std::abs(dx) + std::abs(dy) > raggio) continue;
                            int nx = x + dx, ny = y + dy;
                            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                            Color n = px[(size_t)ny * W + nx];
                            if (n.a > 100) { vicino = n; break; }
                        }
                    if (vicino.a) { Color o = Mescola(vicino, Hex(0x120a18), 0.78f); o.a = 255; out[(size_t)y * W + x] = o; }
                }
            px = out;
        }
        Fotogramma f;
        f.Nuovo(W / ss, H / ss);
        for (int y = 0; y < f.h; y++)
            for (int x = 0; x < f.w; x++) {
                float r = 0, g = 0, b = 0, a = 0;
                for (int sy = 0; sy < ss; sy++)
                    for (int sx = 0; sx < ss; sx++) {
                        Color c = px[(size_t)(y * ss + sy) * W + x * ss + sx];
                        float al = c.a / 255.0f;
                        r += c.r * al; g += c.g * al; b += c.b * al; a += al;
                    }
                if (a <= 0) continue;
                float n = (float)(ss * ss);
                f.Metti(x, y, Color{ (unsigned char)(r / a), (unsigned char)(g / a), (unsigned char)(b / a), (unsigned char)(a / n * 255) });
            }
        return f;
    }
};

Disegno NuovoDisegno(int w0, int h0, StileGrafico s) {
    Disegno d;
    bool hd = s == StileGrafico::Disegnato;
    d.w = w0 * (hd ? 2 : 1); d.h = h0 * (hd ? 2 : 1);
    d.densita = hd ? 4.0f : 2.0f;
    d.liscio = hd;
    return d;
}

// Aggiunge un'animazione: 'disegna(tela, fotogramma)' per ogni fotogramma.
void Anima(Disegno& d, int w0, int h0, StileGrafico s, const std::string& nome, int n, float fps, bool ripeti,
           const std::function<void(Tela&, int)>& disegna, bool contorno = true) {
    Animazione a;
    a.nome = nome; a.fps = fps; a.ripeti = ripeti;
    for (int i = 0; i < n; i++) {
        Tela t(w0, h0, s);
        disegna(t, i);
        a.fotogrammi.push_back(t.Finisci(contorno));
    }
    d.animazioni.push_back(a);
}

Vector2 Ruota(Vector2 o, float l, float ang) { return { o.x + std::sin(ang) * l, o.y + std::cos(ang) * l }; }

// --- Personaggi umanoidi -----------------------------------------------------------------

struct Umano {
    Color pelle = Hex(0xf2c39b), capelli = Hex(0x6b3b1f), maglia = Hex(0x3f7de0), pantaloni = Hex(0x2a2f4a), scarpe = Hex(0x5a3020);
    Color mantello = BLANK, elmo = BLANK, arma = Hex(0xc8d0e0), cintura = Hex(0x5a3a20), occhi = Hex(0x1a1426);
    int armaTipo = 0;          // 0 niente, 1 spada, 2 bastone magico, 3 arco, 4 pistola, 5 lancia
    int capelliTipo = 0;       // 0 corti, 1 lunghi, 2 cresta, 3 calvo, 4 cappello a punta, 5 bandana
    Color cappello = BLANK;
    float robusto = 1;         // larghezza del corpo
    bool scheletro = false, barba = false, mostro = false;
};

// Posa: 0 fermo, 1 corsa, 2 salto, 3 caduta, 4 colpito, 5 vittoria, 6 attacco
void DisegnaUmano(Tela& t, const Umano& u, int posa, float fase) {
    const float cx = 16, piedi = 43;
    float respiro = posa == 0 ? std::sin(fase * 2 * PI) * 0.5f : 0;
    float salto = posa == 5 ? -std::fabs(std::sin(fase * PI)) * 3 : 0;
    float inclina = posa == 4 ? -1.5f : (posa == 1 ? 0.8f : 0);
    float anca = 31 + salto, spalla = 22 + respiro * 0.5f + salto;
    float gambaL = 11.5f;
    float aG1 = 0, aG2 = 0, aB1 = 0, aB2 = 0;   // angoli: gambe e braccia (0 = giu')
    switch (posa) {
        case 1: aG1 = std::sin(fase * 2 * PI) * 0.75f; aG2 = -aG1; aB1 = -aG1 * 0.9f; aB2 = aG1 * 0.9f; anca -= std::fabs(std::cos(fase * 2 * PI)) * 0.8f; spalla = anca - 9; break;
        case 2: aG1 = 0.9f; aG2 = -0.35f; aB1 = 2.4f; aB2 = -2.0f; break;
        case 3: aG1 = 0.25f; aG2 = -0.25f; aB1 = 2.7f; aB2 = -2.7f; break;
        case 4: aG1 = -0.4f; aG2 = 0.3f; aB1 = -1.2f; aB2 = -1.6f; break;
        case 5: aG1 = 0.15f; aG2 = -0.15f; aB1 = 2.9f; aB2 = -2.9f; break;
        case 6: aG1 = 0.35f; aG2 = -0.35f; aB1 = -0.3f; aB2 = Lerp1(-2.6f, 1.4f, fase); break;
        default: aB1 = 0.12f; aB2 = -0.12f; break;
    }
    float largo = 5.2f * u.robusto;
    Color pelle = u.scheletro ? Hex(0xe8e4d8) : u.pelle;
    Color maglia = u.scheletro ? BLANK : u.maglia;
    // Mantello dietro a tutto
    if (u.mantello.a) {
        float onda = std::sin(fase * 2 * PI) * 1.5f + (posa == 1 ? 3 : 0) + (posa == 2 || posa == 3 ? 2 : 0);
        t.Poligono({ { cx - largo + 1 + inclina, spalla }, { cx + largo - 2 + inclina, spalla }, { cx - largo - 2 - onda, piedi - 4 + salto },
                     { cx - largo - 6 - onda * 1.4f, piedi - 6 + salto } }, Mescola(u.mantello, Hex(SCURO), 0.15f), 0.6f);
    }
    // Braccio e gamba dietro
    Vector2 sp2 = { cx - largo + 1.5f + inclina, spalla + 1.5f }, an2 = { cx - 2, anca };
    Vector2 m2 = Ruota(sp2, 8.5f, aB1);
    Vector2 p2 = Ruota(an2, gambaL, aG2);
    t.Linea(an2, p2, 4.4f * u.robusto, Mescola(u.scheletro ? Hex(0xd8d4c8) : u.pantaloni, Hex(SCURO), 0.2f));
    t.Ellisse(p2.x + 1.2f, p2.y - 0.6f, 3, 1.8f, Mescola(u.scarpe, Hex(SCURO), 0.2f));
    t.Linea(sp2, m2, 3.6f, Mescola(u.scheletro ? Hex(0xd8d4c8) : maglia.a ? maglia : pelle, Hex(SCURO), 0.2f));
    t.Ellisse(m2.x, m2.y, 1.9f, 1.9f, Mescola(pelle, Hex(SCURO), 0.15f));
    // Torso
    if (u.scheletro) {
        t.Rett(cx - 1 + inclina, spalla, 2.5f, anca - spalla, Hex(0xe8e4d8));
        for (int i = 0; i < 3; i++) t.Rett(cx - largo + 1.5f + inclina, spalla + 1.5f + i * 2.6f, largo * 2 - 3, 1.4f, Hex(0xe8e4d8), 0.5f, 0.6f);
        t.Ellisse(cx + inclina * 0.5f, anca, 4, 2, Hex(0xd8d4c8));
    } else {
        t.Rett(cx - largo + inclina, spalla - 0.5f, largo * 2, anca - spalla + 2.5f, maglia, 1, 2.5f);
        t.Rett(cx - largo + inclina, anca - 1.2f, largo * 2, 2.2f, u.cintura, 0.5f);
        t.Rett(cx - 1 + inclina, anca - 1.2f, 2, 2.2f, Hex(0xffd23f), 0.3f);
    }
    // Gamba davanti
    Vector2 an1 = { cx + 1.8f, anca };
    Vector2 p1 = Ruota(an1, gambaL, aG1);
    t.Linea(an1, p1, 4.4f * u.robusto, u.scheletro ? Hex(0xe8e4d8) : u.pantaloni);
    t.Ellisse(p1.x + 1.4f, p1.y - 0.6f, 3.2f, 1.9f, u.scarpe);
    // Testa
    float hx = cx + 1 + inclina * 1.3f, hy = spalla - 7.5f;
    if (u.mostro) {
        t.Ellisse(hx, hy + 0.5f, 7.6f, 7, pelle);
        t.Poligono({ { hx - 5, hy - 5 }, { hx - 3, hy - 11 }, { hx - 1.5f, hy - 6 } }, Hex(0xf4ecd8));
        t.Poligono({ { hx + 3, hy - 6 }, { hx + 5.5f, hy - 11 }, { hx + 6, hy - 4.5f } }, Hex(0xf4ecd8));
    } else t.Ellisse(hx, hy, 7, 7.2f, pelle);
    if (u.scheletro) {
        t.Ellisse(hx + 1.2f, hy - 0.5f, 1.9f, 2.2f, Hex(0x1a1426), 0);
        t.Ellisse(hx + 5, hy - 0.5f, 1.6f, 2.0f, Hex(0x1a1426), 0);
        for (int i = 0; i < 3; i++) t.Rett(hx + 0.5f + i * 1.6f, hy + 4, 0.9f, 1.6f, Hex(0x3a3440), 0);
    } else {
        // capelli
        switch (u.capelliTipo) {
            case 0: t.Ellisse(hx - 1, hy - 3.2f, 7.2f, 4.6f, u.capelli); t.Ellisse(hx - 5, hy - 0.5f, 2.6f, 4, u.capelli); break;
            case 1: t.Ellisse(hx - 1, hy - 3.2f, 7.4f, 4.8f, u.capelli); t.Rett(hx - 8, hy - 2, 5, 11, u.capelli, 1, 2); break;
            case 2: t.Poligono({ { hx - 6, hy - 3 }, { hx - 4, hy - 11 }, { hx - 1, hy - 5 }, { hx + 1, hy - 12 }, { hx + 3, hy - 5 }, { hx + 6, hy - 9 }, { hx + 6, hy - 3 } }, u.capelli); break;
            case 4: {
                Color c = u.cappello.a ? u.cappello : Hex(0x3a3aa8);
                t.Ellisse(hx, hy - 4.5f, 10, 2, c);
                t.Poligono({ { hx - 6, hy - 5 }, { hx + 5, hy - 5 }, { hx - 4, hy - 17 } }, c);
                t.Ellisse(hx - 3.5f, hy - 10, 0.9f, 0.9f, Hex(0xffd23f), 0);
                break;
            }
            case 5: {
                Color c = u.cappello.a ? u.cappello : Hex(0xd8344a);
                t.Ellisse(hx - 0.5f, hy - 3.5f, 7.4f, 4.2f, c);
                t.Poligono({ { hx - 7, hy - 3 }, { hx - 11, hy + 1 }, { hx - 9, hy - 5 } }, c);
                break;
            }
            default: break;
        }
        if (u.barba) t.Poligono({ { hx - 4, hy + 2 }, { hx + 6.5f, hy + 2 }, { hx + 4, hy + 8 }, { hx - 1, hy + 9 } }, u.capelli);
        // occhi (guarda a destra)
        bool chiusi = posa == 4;
        if (chiusi) { t.Rett(hx + 1, hy, 2.4f, 0.8f, u.occhi, 0); t.Rett(hx + 4.5f, hy, 2.2f, 0.8f, u.occhi, 0); }
        else {
            t.Rett(hx + 1.4f, hy - 1.6f, 1.7f, 3, u.occhi, 0, 0.6f);
            t.Rett(hx + 4.8f, hy - 1.6f, 1.6f, 3, u.occhi, 0, 0.6f);
            if (t.k >= 2) { t.Rett(hx + 1.6f, hy - 1.4f, 0.7f, 0.8f, WHITE, 0); t.Rett(hx + 5, hy - 1.4f, 0.7f, 0.8f, WHITE, 0); }
        }
        if (!u.mostro) t.Ellisse(hx + 6.5f, hy + 2.5f, 1.4f, 0.9f, Mescola(pelle, Hex(0xd8344a), 0.35f), 0);
        else t.Rett(hx + 1, hy + 3.5f, 5.5f, 1.1f, Hex(0x2a1420), 0);
    }
    if (u.elmo.a) {
        t.Ellisse(hx - 0.5f, hy - 2.5f, 7.8f, 6.2f, u.elmo);
        t.Rett(hx - 7.5f, hy - 2.5f, 3, 9, u.elmo, 1, 1);
        t.Rett(hx + 0.5f, hy - 2.8f, 7, 1.4f, Mescola(u.elmo, Hex(SCURO), 0.5f), 0);
        t.Poligono({ { hx - 2, hy - 8 }, { hx + 1, hy - 14 }, { hx + 2.5f, hy - 8 } }, Hex(0xd8344a));
    }
    // Braccio davanti e arma
    Vector2 sp1 = { cx + largo - 1.5f + inclina, spalla + 1.5f };
    Vector2 m1 = Ruota(sp1, 8.5f, aB2);
    if (u.armaTipo) {
        float ang = aB2 + (posa == 6 ? 0.6f : 0.9f);
        Vector2 punta = Ruota(m1, u.armaTipo == 5 ? 16 : 11, PI - ang);
        switch (u.armaTipo) {
            case 1: t.Linea(m1, punta, 2.2f, u.arma); t.Linea(Ruota(m1, 2, -ang + PI / 2), Ruota(m1, 2, -ang - PI / 2), 1.6f, Hex(0x8a5a20)); break;
            case 2: t.Linea(Ruota(m1, 6, -ang), punta, 1.8f, Hex(0x8a5a20)); t.Ellisse(punta.x, punta.y, 2.4f, 2.4f, u.arma); break;
            case 3: t.Linea(Ruota(m1, 7, PI - ang + 0.6f), Ruota(m1, 7, PI - ang - 0.6f), 1.2f, Hex(0x8a5a20)); break;
            case 4: t.Rett(m1.x - 0.5f, m1.y - 2.5f, 6, 2.6f, Hex(0x3a3e52), 1, 0.6f); break;
            case 5: t.Linea(Ruota(m1, 6, -ang), punta, 1.5f, Hex(0x8a5a20)); t.Poligono({ punta, Ruota(punta, 3, PI - ang + 0.5f), Ruota(punta, 3, PI - ang - 0.5f) }, u.arma); break;
        }
    }
    t.Linea(sp1, m1, 3.6f, u.scheletro ? Hex(0xe8e4d8) : maglia.a ? maglia : pelle);
    t.Ellisse(m1.x, m1.y, 2, 2, pelle);
}

PersonaggioDef CreaUmano(const std::string& nome, const Umano& u, StileGrafico s, TipoComportamento tipo) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(32, 44, s);
    auto Posa = [&](const char* n, int posa, int fot, float fps, bool ripeti = true) {
        Anima(p.disegno, 32, 44, s, n, fot, fps, ripeti, [&](Tela& t, int i) { DisegnaUmano(t, u, posa, fot > 1 ? (float)i / fot : 0.5f); });
    };
    Posa("fermo", 0, 4, 5);
    Posa("corsa", 1, 6, 12);
    Posa("salto", 2, 1, 1);
    Posa("caduta", 3, 1, 1);
    Posa("colpito", 4, 1, 1);
    Posa("vittoria", 5, 4, 6);
    if (u.armaTipo) Posa("attacco", 6, 3, 12, false);
    p.comportamento.tipo = tipo;
    if (tipo == TipoComportamento::Giocatore) { p.comportamento.velocita = 115; p.comportamento.salto = 305; p.comportamento.energia = 3; }
    if (tipo == TipoComportamento::Parla) { p.comportamento.velocita = 0; p.comportamento.calpestabile = false; p.comportamento.danno = 0; }
    return p;
}

// --- Creature ----------------------------------------------------------------------------

PersonaggioDef CreaSlime(const std::string& nome, Color c, StileGrafico s) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(32, 24, s);
    auto Corpo = [c](Tela& t, float sq, bool colpito) {
        float rx = 12.5f * (1 + sq * 0.18f), ry = 9.5f * (1 - sq * 0.18f);
        float cy = 23 - ry;
        Color corpo = ConAlfa(c, 0.92f);
        t.Ellisse(16, cy + 1, rx, ry + 1, corpo);
        t.Rett(16 - rx * 0.92f, cy + 1, rx * 1.84f, 23 - cy - 0.5f, corpo, 0.6f, 2);
        t.Ellisse(11, cy - ry * 0.35f, 3, 2, ConAlfa(WHITE, 0.55f), 0);
        if (colpito) { t.Rett(17, cy - 1, 3, 1, Hex(0x1a1426), 0); t.Rett(22, cy - 1, 3, 1, Hex(0x1a1426), 0); }
        else { t.Occhio(18.5f, cy - 0.5f, 2.2f); t.Occhio(23.5f, cy - 0.5f, 2.0f); }
        t.Ellisse(21.5f, cy + 4, 2, 1, Mescola(c, Hex(SCURO), 0.6f), 0);
    };
    Anima(p.disegno, 32, 24, s, "fermo", 4, 6, true, [&](Tela& t, int i) { Corpo(t, std::sin(i * PI / 2) * 0.5f, false); });
    Anima(p.disegno, 32, 24, s, "corsa", 4, 9, true, [&](Tela& t, int i) { Corpo(t, std::sin(i * PI / 2), false); });
    Anima(p.disegno, 32, 24, s, "colpito", 1, 1, true, [&](Tela& t, int) { Corpo(t, 1, true); });
    p.comportamento.tipo = TipoComportamento::Pattuglia;
    p.comportamento.velocita = 28;
    return p;
}

// Creatura volante: pipistrello (0), uccello (1), drago (2), ape (3).
PersonaggioDef CreaVolante(const std::string& nome, int specie, Color corpo, Color ali, StileGrafico s) {
    PersonaggioDef p;
    p.nome = nome;
    int W = specie == 2 ? 56 : 32, H = specie == 2 ? 44 : 28;
    p.disegno = NuovoDisegno(W, H, s);
    auto Disegna = [=](Tela& t, float batti, int posa) {
        float cx = W / 2.0f, cy = H / 2.0f + 2;
        float su = std::sin(batti * 2 * PI);
        if (specie == 2) {   // drago
            Color pancia = Mescola(corpo, Hex(0xffe0a0), 0.55f);
            t.Poligono({ { cx - 6, cy + 2 }, { cx - 22, cy + 6 - su }, { cx - 27, cy + 1 }, { cx - 20, cy + 1 }, { cx - 8, cy - 3 } }, corpo);   // coda
            t.Poligono({ { cx - 27, cy + 1 }, { cx - 25, cy - 4 }, { cx - 22, cy + 2 } }, Hex(0xd8344a));
            t.Poligono({ { cx - 2, cy - 4 }, { cx - 14, cy - 18 - su * 6 }, { cx - 6, cy - 14 - su * 5 }, { cx + 2, cy - 20 - su * 5 }, { cx + 6, cy - 5 } }, ali, 0.6f);
            t.Ellisse(cx, cy, 11, 8, corpo);
            t.Ellisse(cx + 1, cy + 3, 7, 4.5f, pancia, 0.5f);
            t.Linea({ cx - 4, cy + 6 }, { cx - 5, cy + 12 }, 3.2f, corpo);
            t.Linea({ cx + 5, cy + 6 }, { cx + 6, cy + 12 }, 3.2f, corpo);
            t.Linea({ cx + 7, cy - 3 }, { cx + 13, cy - 9 }, 6, corpo);
            float bocca = posa == 6 ? 2.5f : 0;
            t.Ellisse(cx + 17, cy - 11, 7, 5.5f, corpo);
            t.Ellisse(cx + 22, cy - 9 + bocca * 0.4f, 4.5f, 2.8f + bocca * 0.3f, corpo);
            if (bocca > 0) t.Ellisse(cx + 24, cy - 7.5f, 2.5f, 1.4f, Hex(0x5a1020), 0);
            t.Poligono({ { cx + 13, cy - 14 }, { cx + 10, cy - 22 }, { cx + 16, cy - 15 } }, Hex(0xf4ecd8));
            t.Poligono({ { cx + 17, cy - 15.5f }, { cx + 17, cy - 23 }, { cx + 20, cy - 15 } }, Hex(0xf4ecd8));
            t.Poligono({ { cx + 4, cy - 7 }, { cx + 7, cy - 11 }, { cx + 8, cy - 6 } }, Hex(0xd8344a));
            t.Occhio(cx + 19, cy - 13, 2.2f, posa == 4 ? Hex(0x1a1426) : Hex(0xd8a020));
            t.Rett(cx + 25, cy - 11, 1, 1, Hex(0x1a1426), 0);
            t.Poligono({ { cx - 1, cy - 3 }, { cx - 10, cy - 16 + su * 6 }, { cx - 3, cy - 13 + su * 5 }, { cx + 4, cy - 18 + su * 5 }, { cx + 7, cy - 3 } }, Mescola(ali, Hex(CHIARO), 0.15f), 0.6f);
            return;
        }
        if (specie == 3) {   // ape
            t.Ellisse(cx - 4, cy - 6 - su * 2, 5, 3.5f, ConAlfa(Hex(0xe8f4ff), 0.7f), 0.2f);
            t.Ellisse(cx - 1, cy, 8, 6, corpo);
            for (int i = 0; i < 3; i++) t.Rett(cx - 6 + i * 4, cy - 5, 1.6f, 10, Hex(0x2a2030), 0.3f);
            t.Poligono({ { cx - 9, cy }, { cx - 13, cy + 1 }, { cx - 9, cy + 2 } }, Hex(0x2a2030));
            t.Ellisse(cx + 7, cy - 1, 4.5f, 4.5f, Hex(0x2a2030));
            t.Occhio(cx + 8.5f, cy - 2, 1.8f);
            t.Ellisse(cx - 2, cy - 7 - su * 2, 5, 3.2f, ConAlfa(Hex(0xe8f4ff), 0.8f), 0.2f);
            return;
        }
        // pipistrello e uccello
        if (specie == 0) {
            t.Poligono({ { cx - 3, cy - 1 }, { cx - 15, cy - 8 + su * 9 }, { cx - 12, cy + 1 + su * 4 }, { cx - 8, cy - 1 + su * 3 }, { cx - 5, cy + 3 } }, ali, 0.5f);
            t.Poligono({ { cx + 3, cy - 1 }, { cx + 15, cy - 8 + su * 9 }, { cx + 12, cy + 1 + su * 4 }, { cx + 8, cy - 1 + su * 3 }, { cx + 5, cy + 3 } }, ali, 0.5f);
            t.Ellisse(cx, cy, 6, 6, corpo);
            t.Poligono({ { cx - 5, cy - 3 }, { cx - 4, cy - 10 }, { cx - 1, cy - 5 } }, corpo);
            t.Poligono({ { cx + 5, cy - 3 }, { cx + 4, cy - 10 }, { cx + 1, cy - 5 } }, corpo);
            t.Occhio(cx - 2, cy - 1, 1.6f, Hex(0xd8344a), 0);
            t.Occhio(cx + 2.5f, cy - 1, 1.6f, Hex(0xd8344a), 0);
            t.Poligono({ { cx - 1.5f, cy + 3 }, { cx - 0.8f, cy + 5 }, { cx, cy + 3 } }, WHITE, 0);
            t.Poligono({ { cx + 1, cy + 3 }, { cx + 1.8f, cy + 5 }, { cx + 2.5f, cy + 3 } }, WHITE, 0);
        } else {
            t.Poligono({ { cx - 9, cy }, { cx - 15, cy - 3 }, { cx - 14, cy + 3 } }, ali);
            t.Ellisse(cx, cy, 9, 6, corpo);
            t.Ellisse(cx + 1, cy + 2, 6, 3.5f, Mescola(corpo, WHITE, 0.5f), 0.4f);
            t.Ellisse(cx + 8, cy - 4, 5, 4.5f, corpo);
            t.Poligono({ { cx + 12, cy - 5 }, { cx + 17, cy - 3 }, { cx + 12, cy - 2 } }, Hex(0xf0a020));
            t.Occhio(cx + 9.5f, cy - 5, 1.6f);
            t.Poligono({ { cx - 3, cy - 2 }, { cx - 9, cy - 10 + su * 9 }, { cx + 4, cy - 3 } }, ali, 0.5f);
        }
    };
    Anima(p.disegno, W, H, s, "fermo", 4, 10, true, [&](Tela& t, int i) { Disegna(t, i / 4.0f, 0); });
    Anima(p.disegno, W, H, s, "corsa", 4, 12, true, [&](Tela& t, int i) { Disegna(t, i / 4.0f, 1); });
    Anima(p.disegno, W, H, s, "colpito", 1, 1, true, [&](Tela& t, int) { Disegna(t, 0.25f, 4); });
    if (specie == 2) Anima(p.disegno, W, H, s, "attacco", 2, 6, false, [&](Tela& t, int i) { Disegna(t, i / 2.0f, 6); });
    p.comportamento.tipo = TipoComportamento::Vola;
    p.comportamento.gravita = false;
    p.comportamento.velocita = 35;
    if (specie == 2) {
        p.comportamento.tipo = TipoComportamento::Boss;
        p.comportamento.energia = 30; p.comportamento.danno = 1; p.comportamento.vista = 220; p.comportamento.attaccoOgni = 1.6f;
        p.comportamento.calpestabile = true; p.comportamento.punti = 2000; p.comportamento.velocita = 45;
    }
    return p;
}

PersonaggioDef CreaFantasma(const std::string& nome, Color c, StileGrafico s) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(28, 32, s);
    auto D = [c](Tela& t, float f, bool colpito) {
        Color b = ConAlfa(c, 0.85f);
        float y0 = 4 + std::sin(f * 2 * PI) * 1.2f;
        t.Ellisse(14, y0 + 10, 10, 10, b);
        t.Rett(4, y0 + 10, 20, 12, b, 0.6f);
        for (int i = 0; i < 4; i++) {
            float ox = 4 + i * 5 + 2.5f;
            t.Ellisse(ox, y0 + 22 + std::sin(f * 2 * PI + i) * 1.2f, 2.6f, 2.6f, b);
        }
        if (colpito) { t.Rett(13, y0 + 9, 3, 1, Hex(0x1a1426), 0); t.Rett(19, y0 + 9, 3, 1, Hex(0x1a1426), 0); }
        else { t.Ellisse(15, y0 + 9, 2, 2.8f, Hex(0x1a1426), 0); t.Ellisse(20.5f, y0 + 9, 1.8f, 2.6f, Hex(0x1a1426), 0); }
        t.Ellisse(18, y0 + 15, 2, 1.6f, Hex(0x1a1426), 0);
    };
    Anima(p.disegno, 28, 32, s, "fermo", 4, 6, true, [&](Tela& t, int i) { D(t, i / 4.0f, false); });
    Anima(p.disegno, 28, 32, s, "colpito", 1, 1, true, [&](Tela& t, int) { D(t, 0, true); });
    p.comportamento.tipo = TipoComportamento::Insegue;
    p.comportamento.gravita = false;
    p.comportamento.velocita = 26;
    p.comportamento.calpestabile = false;
    return p;
}

PersonaggioDef CreaRobot(const std::string& nome, Color c, Color luce, StileGrafico s, TipoComportamento tipo) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(32, 40, s);
    auto D = [=](Tela& t, float f, int posa) {
        float b = posa == 1 ? std::fabs(std::sin(f * 2 * PI)) * 1.2f : 0;
        t.Linea({ 17, 9 - b }, { 17, 3 - b }, 1.2f, Hex(0x5a6070));
        t.Ellisse(17, 3 - b, 1.8f, 1.8f, (int)(f * 4) % 2 ? luce : Mescola(luce, Hex(SCURO), 0.5f), 0);
        t.Rett(8, 8 - b, 18, 12, c, 1, 2.5f);
        t.Rett(13, 11 - b, 11, 4.5f, Hex(0x1a1426), 0.2f, 1.5f);
        t.Rett(15 + (posa == 4 ? 0 : 3), 12 - b, 4, 2.5f, luce, 0, 1);
        t.Rett(7, 21 - b, 20, 11, Mescola(c, Hex(SCURO), 0.15f), 1, 2);
        t.Ellisse(17, 26 - b, 2.5f, 2.5f, luce, 0.2f);
        float g = posa == 1 ? std::sin(f * 2 * PI) * 2 : 0;
        t.Rett(9 + g, 32, 6, 8, Hex(0x4a5060), 1, 1);
        t.Rett(19 - g, 32, 6, 8, Hex(0x4a5060), 1, 1);
        t.Linea({ 25, 23 - b }, { 30, 28 - b + (posa == 6 ? -6 : 0) }, 3, Hex(0x5a6070));
    };
    Anima(p.disegno, 32, 40, s, "fermo", 4, 4, true, [&](Tela& t, int i) { D(t, i / 4.0f, 0); });
    Anima(p.disegno, 32, 40, s, "corsa", 4, 8, true, [&](Tela& t, int i) { D(t, i / 4.0f, 1); });
    Anima(p.disegno, 32, 40, s, "colpito", 1, 1, true, [&](Tela& t, int) { D(t, 0, 4); });
    Anima(p.disegno, 32, 40, s, "attacco", 2, 6, false, [&](Tela& t, int i) { D(t, i / 2.0f, 6); });
    p.comportamento.tipo = tipo;
    p.comportamento.velocita = 32;
    p.comportamento.energia = 2;
    return p;
}

PersonaggioDef CreaNave(const std::string& nome, Color c, Color vetro, StileGrafico s, bool nemica) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(40, 24, s);
    auto D = [=](Tela& t, int i) {
        float fiamma = 4 + (i % 3) * 2.5f;
        t.Poligono({ { 6, 9 }, { 6 - fiamma, 12 }, { 6, 15 } }, Hex(0xffa020), 0);
        t.Poligono({ { 6, 10.5f }, { 6 - fiamma * 0.5f, 12 }, { 6, 13.5f } }, Hex(0xfff4b0), 0);
        t.Poligono({ { 12, 10 }, { 4, 2 }, { 9, 2 }, { 22, 10 } }, Mescola(c, Hex(SCURO), 0.2f));
        t.Poligono({ { 12, 14 }, { 4, 22 }, { 9, 22 }, { 22, 14 } }, Mescola(c, Hex(SCURO), 0.2f));
        t.Poligono({ { 5, 8 }, { 26, 7 }, { 38, 12 }, { 26, 17 }, { 5, 16 } }, c);
        t.Ellisse(25, 11, 5, 3, vetro, 0.8f);
        t.Rett(8, 11.2f, 12, 1.4f, nemica ? Hex(0xd8344a) : Hex(0xffd23f), 0);
    };
    Anima(p.disegno, 40, 24, s, "fermo", 3, 14, true, D);
    p.comportamento.tipo = nemica ? TipoComportamento::Spara : TipoComportamento::Giocatore;
    p.comportamento.gravita = false;
    p.comportamento.velocita = nemica ? 50 : 120;
    return p;
}

PersonaggioDef CreaCarro(const std::string& nome, Color c, StileGrafico s) {
    PersonaggioDef p;
    p.nome = nome;
    p.disegno = NuovoDisegno(44, 30, s);
    auto D = [=](Tela& t, int i, bool spara) {
        t.Rett(3, 20, 38, 9, Hex(0x3a3e48), 1, 4);
        for (int k = 0; k < 5; k++) t.Ellisse(8 + k * 7 + (i % 2) * 0.8f, 24.5f, 2.8f, 2.8f, Hex(0x6a7080), 0.8f);
        t.Rett(5, 13, 34, 8, c, 1, 2);
        t.Rett(13, 6, 16, 9, Mescola(c, WHITE, 0.08f), 1, 3);
        t.Rett(28, 8.5f, spara ? 11 : 14, 3, Mescola(c, Hex(SCURO), 0.25f), 1, 1);
        if (spara) t.Ellisse(42, 10, 2.5f, 2.5f, Hex(0xffd040), 0);
        t.Ellisse(17, 6, 3, 1.5f, Hex(0x2a2e38), 0);
    };
    Anima(p.disegno, 44, 30, s, "fermo", 2, 6, true, [&](Tela& t, int i) { D(t, i, false); });
    Anima(p.disegno, 44, 30, s, "attacco", 2, 6, false, [&](Tela& t, int i) { D(t, i, i == 0); });
    p.comportamento.tipo = TipoComportamento::Spara;
    p.comportamento.velocita = 20;
    p.comportamento.energia = 4;
    return p;
}

// Animali e mostri a terra: pesce (0), ragno (1), pianta carnivora (2), fungo (3), granchio (4), lumaca (5), dinosauro (6)
PersonaggioDef CreaBestia(const std::string& nome, int specie, Color c, Color c2, StileGrafico s) {
    PersonaggioDef p;
    p.nome = nome;
    int W = 32, H = specie == 2 ? 36 : (specie == 6 ? 40 : 26);
    p.disegno = NuovoDisegno(W, H, s);
    auto D = [=](Tela& t, float f, int posa) {
        float o = std::sin(f * 2 * PI);
        switch (specie) {
            case 0:
                t.Poligono({ { 6, 13 }, { 0, 6 + o * 2 }, { 1, 20 - o * 2 } }, c2);
                t.Ellisse(16, 13, 11, 8, c);
                t.Poligono({ { 12, 6 }, { 18, 1 }, { 20, 7 } }, c2);
                t.Ellisse(17, 16, 7, 3.5f, Mescola(c, WHITE, 0.4f), 0.3f);
                t.Occhio(23, 11, 2.4f);
                t.Rett(25, 15, 2.5f, 1, Hex(0x2a1420), 0);
                break;
            case 1:
                for (int i = 0; i < 4; i++) {
                    float a = 0.5f + i * 0.3f + o * 0.15f * (i % 2 ? 1 : -1);
                    t.Linea({ 16, 16 }, { 16 - std::cos(a) * 14, 26 - std::sin(a) * 2 - (i % 2) * 3 }, 1.6f, Mescola(c, Hex(SCURO), 0.3f));
                    t.Linea({ 16, 16 }, { 16 + std::cos(a) * 14, 26 - std::sin(a) * 2 - ((i + 1) % 2) * 3 }, 1.6f, Mescola(c, Hex(SCURO), 0.3f));
                }
                t.Ellisse(13, 13, 8, 7, c);
                t.Ellisse(22, 16, 5, 4.5f, c);
                t.Occhio(22, 15, 1.3f, Hex(0xd8344a)); t.Occhio(25, 15, 1.3f, Hex(0xd8344a));
                t.Ellisse(12, 11, 3, 2, c2, 0);
                break;
            case 2: {
                float bocca = posa == 6 ? 4 : 1.5f + o;
                t.Rett(9, 28, 14, 8, Hex(0xb8603a), 1, 1);
                t.Linea({ 16, 28 }, { 16 + o, 16 }, 2.6f, Hex(0x3a8a3a));
                t.Ellisse(10, 24, 5, 2, Hex(0x5ac85a)); t.Ellisse(22, 22, 5, 2, Hex(0x5ac85a));
                t.Ellisse(17 + o, 11 - bocca * 0.5f, 9, 6, c);
                t.Ellisse(17 + o, 13 + bocca * 0.5f, 8, 5, c);
                t.Ellisse(20 + o, 12, 4.5f, bocca, Hex(0x5a1020), 0);
                for (int i = 0; i < 3; i++) t.Poligono({ { 17.5f + o + i * 2.2f, 12 - bocca }, { 18.5f + o + i * 2.2f, 12 - bocca + 2 }, { 19.5f + o + i * 2.2f, 12 - bocca } }, WHITE, 0);
                for (int i = 0; i < 4; i++) t.Ellisse(12 + o + i * 3, 7 - bocca * 0.5f, 1.2f, 1.2f, c2, 0);
                break;
            }
            case 3: {
                float sq = posa == 1 ? std::fabs(o) * 1.5f : 0;
                t.Ellisse(12 - o * 2, 24, 3.5f, 2, Hex(0x5a3020)); t.Ellisse(21 + o * 2, 24, 3.5f, 2, Hex(0x5a3020));
                t.Rett(9, 13 - sq, 15, 11, Hex(0xf4e8d0), 1, 4);
                t.Ellisse(16, 11 - sq, 14, 9, c);
                t.Ellisse(10, 8 - sq, 2.5f, 2, c2, 0); t.Ellisse(18, 6 - sq, 3, 2.2f, c2, 0); t.Ellisse(24, 10 - sq, 2, 1.6f, c2, 0);
                t.Occhio(17, 17 - sq, 1.6f); t.Occhio(21.5f, 17 - sq, 1.5f);
                break;
            }
            case 4:
                for (int i = 0; i < 3; i++) {
                    t.Linea({ 10, 18 }, { 4.0f - i, 25 - i * 2 + o * (i % 2) }, 1.6f, Mescola(c, Hex(SCURO), 0.2f));
                    t.Linea({ 22, 18 }, { 28.0f + i, 25 - i * 2 - o * (i % 2) }, 1.6f, Mescola(c, Hex(SCURO), 0.2f));
                }
                t.Ellisse(16, 17, 11, 7, c);
                t.Linea({ 7, 13 }, { 3, 6 }, 2, c); t.Ellisse(3, 5, 3.5f, 3, c);
                t.Linea({ 25, 13 }, { 29, 6 }, 2, c); t.Ellisse(29, 5, 3.5f, 3, c);
                t.Linea({ 13, 12 }, { 12, 8 }, 1, c); t.Linea({ 19, 12 }, { 20, 8 }, 1, c);
                t.Occhio(12, 7, 1.6f); t.Occhio(20, 7, 1.6f);
                break;
            case 5:
                t.Ellisse(18, 21, 13, 4, c, 0.6f);
                t.Linea({ 28, 20 }, { 29 + o, 12 }, 4.5f, c);
                t.Linea({ 28, 12 }, { 27 + o, 7 }, 1, c); t.Linea({ 30, 12 }, { 32, 7 }, 1, c);
                t.Occhio(27 + o, 6, 1.4f); t.Occhio(31.5f, 6.5f, 1.3f);
                t.Ellisse(14, 13, 9, 9, c2);
                t.Ellisse(14, 13, 5, 5, Mescola(c2, Hex(SCURO), 0.25f), 0.3f);
                t.Ellisse(14, 13, 2, 2, Mescola(c2, Hex(CHIARO), 0.2f), 0);
                break;
            default: {   // dinosauro
                float g = posa == 1 ? o * 0.6f : 0;
                t.Poligono({ { 8, 22 }, { 0, 30 + o }, { 10, 27 } }, c);
                t.Linea({ 12, 28 }, Ruota({ 12, 28 }, 10, g), 4.5f, Mescola(c, Hex(SCURO), 0.2f));
                t.Ellisse(15, 23, 10, 8, c);
                t.Ellisse(17, 26, 6, 4, c2, 0.4f);
                t.Linea({ 18, 28 }, Ruota({ 18, 28 }, 10, -g), 4.5f, c);
                t.Linea({ 20, 19 }, { 24, 11 }, 6, c);
                t.Ellisse(25, 9, 7, 5.5f, c);
                t.Ellisse(29, 11.5f, 4, 2.5f, c);
                t.Rett(26, 13, 6, 1, Hex(0x2a1420), 0);
                t.Occhio(26, 7, 1.7f);
                t.Linea({ 23, 19 }, { 27, 22 }, 1.6f, c);
                for (int i = 0; i < 4; i++) t.Poligono({ { 9.0f + i * 4, 16 - i * 0.6f }, { 11.0f + i * 4, 12 - i * 0.6f }, { 13.0f + i * 4, 16 - i * 0.6f } }, c2, 0.4f);
            }
        }
    };
    Anima(p.disegno, W, H, s, "fermo", 4, 5, true, [&](Tela& t, int i) { D(t, i / 4.0f, 0); });
    Anima(p.disegno, W, H, s, "corsa", 4, 9, true, [&](Tela& t, int i) { D(t, i / 4.0f, 1); });
    Anima(p.disegno, W, H, s, "colpito", 1, 1, true, [&](Tela& t, int) { D(t, 0, 4); });
    if (specie == 2) Anima(p.disegno, W, H, s, "attacco", 2, 6, false, [&](Tela& t, int i) { D(t, i / 2.0f, 6); });
    p.comportamento.tipo = specie == 2 ? TipoComportamento::Spara : TipoComportamento::Pattuglia;
    p.comportamento.velocita = specie == 5 ? 12 : (specie == 6 ? 45 : 30);
    if (specie == 0) { p.comportamento.tipo = TipoComportamento::Vola; p.comportamento.gravita = false; }
    if (specie == 2) { p.comportamento.velocita = 0; p.comportamento.vista = 150; }
    if (specie == 6) { p.comportamento.tipo = TipoComportamento::Insegue; p.comportamento.energia = 3; }
    return p;
}

// --- Oggetti ----------------------------------------------------------------------------

OggettoDef Oggetto(const std::string& nome, FunzioneOggetto f, int valore, int W, int H, StileGrafico s) {
    OggettoDef o;
    o.nome = nome; o.funzione = f; o.valore = valore;
    o.disegno = NuovoDisegno(W, H, s);
    return o;
}

OggettoDef CreaMoneta(const std::string& nome, Color c, int valore, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Raccogli, valore, 20, 20, s);
    o.variabile = "monete";
    o.suono = "Moneta";
    Anima(o.disegno, 20, 20, s, "gira", 6, 10, true, [&](Tela& t, int i) {
        float l = std::fabs(std::cos(i * PI / 6));
        float rx = std::max(1.2f, 8 * l);
        t.Ellisse(10, 10, rx, 8.5f, Mescola(c, Hex(SCURO), 0.25f));
        t.Ellisse(10 - (1 - l) * 0.8f, 10, std::max(0.8f, rx - 1.6f), 7, c);
        if (l > 0.5f) { t.Rett(9, 6, 2, 8, Mescola(c, Hex(SCURO), 0.3f), 0); t.Rett(9.2f, 6.2f, 1, 7.6f, Mescola(c, WHITE, 0.5f), 0); }
        t.Ellisse(10 - rx * 0.4f, 6.5f, std::max(0.6f, rx * 0.2f), 1.4f, ConAlfa(WHITE, 0.8f), 0);
    });
    return o;
}

OggettoDef CreaGemma(const std::string& nome, Color c, int valore, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Raccogli, valore, 20, 20, s);
    o.suono = "Gemma";
    Anima(o.disegno, 20, 20, s, "brilla", 4, 6, true, [&](Tela& t, int i) {
        t.Poligono({ { 4, 7 }, { 7, 3 }, { 13, 3 }, { 16, 7 }, { 10, 17 } }, c);
        t.Poligono({ { 4, 7 }, { 16, 7 }, { 10, 17 } }, Mescola(c, Hex(SCURO), 0.2f), 0.4f);
        t.Poligono({ { 7, 3 }, { 10, 7 }, { 13, 3 } }, Mescola(c, WHITE, 0.45f), 0);
        float sx = 6 + i * 2.0f;
        t.Ellisse(sx, 5.5f, 1, 1, WHITE, 0);
        if (i == 2) { t.Rett(14.5f, 1, 0.8f, 4, WHITE, 0); t.Rett(13, 2.6f, 4, 0.8f, WHITE, 0); }
    });
    return o;
}

OggettoDef CreaCuore(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Cura, 1, 20, 18, s);
    o.suono = "Cura";
    Anima(o.disegno, 20, 18, s, "batte", 2, 3, true, [&](Tela& t, int i) {
        float k = i ? 0.9f : 1;
        t.Ellisse(10 - 3.6f * k, 7, 4.4f * k, 4.4f * k, c);
        t.Ellisse(10 + 3.6f * k, 7, 4.4f * k, 4.4f * k, c);
        t.Poligono({ { 10 - 7.9f * k, 8 }, { 10 + 7.9f * k, 8 }, { 10, 8 + 8.5f * k } }, c);
        t.Ellisse(10 - 4.5f * k, 5.5f, 1.6f, 1.2f, ConAlfa(WHITE, 0.8f), 0);
    });
    return o;
}

OggettoDef CreaChiave(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Chiave, 0, 24, 14, s);
    o.suono = "Chiave";
    Anima(o.disegno, 24, 14, s, "fermo", 2, 2, true, [&](Tela& t, int i) {
        t.Ellisse(6, 7, 5, 5, c);
        t.Ellisse(6, 7, 2.2f, 2.2f, BLANK, 0);
        t.Rett(10, 5.8f, 13, 2.6f, c, 1, 1);
        t.Rett(17, 8, 2, 3.5f, c, 0.5f); t.Rett(21, 8, 2, 4.5f, c, 0.5f);
        if (i) t.Ellisse(4, 4, 1, 1, WHITE, 0);
    });
    // il buco dell'anello
    for (auto& a : o.disegno.animazioni)
        for (auto& f : a.fotogrammi) {
            float k = f.w / 24.0f;
            for (int y = 0; y < f.h; y++) for (int x = 0; x < f.w; x++) {
                float dx = (x + 0.5f) / k - 6, dy = (y + 0.5f) / k - 7;
                if (dx * dx + dy * dy < 4.4f) f.Metti(x, y, BLANK);
            }
        }
    return o;
}

OggettoDef CreaPorta(const std::string& nome, Color legno, Color metallo, const std::string& chiave, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Porta, 0, 32, 48, s);
    o.richiede = chiave;
    o.solido = true;
    o.suono = "Porta";
    auto D = [=](Tela& t, float apertura) {
        t.Rett(1, 2, 30, 46, Hex(0x5a5a6a), 1, 3);
        t.Rett(4, 5, 24, 43, Hex(0x14101c), 0, 2);
        float w = 24 * (1 - apertura);
        if (w > 1) {
            t.Rett(4, 5, w, 43, legno, 1, 2);
            for (int i = 1; i < 4; i++) t.Rett(4 + w * i / 4.0f, 6, 0.8f, 41, Mescola(legno, Hex(SCURO), 0.4f), 0);
            t.Rett(4, 13, w, 2.5f, metallo, 0.6f); t.Rett(4, 36, w, 2.5f, metallo, 0.6f);
            if (apertura < 0.3f) { t.Ellisse(4 + w - 4, 27, 1.8f, 1.8f, Hex(0xffd23f)); t.Rett(4 + w - 5, 29, 2, 3, Hex(0x14101c), 0); }
        }
    };
    Anima(o.disegno, 32, 48, s, "chiusa", 1, 1, true, [&](Tela& t, int) { D(t, 0); });
    Anima(o.disegno, 32, 48, s, "aperta", 3, 8, false, [&](Tela& t, int i) { D(t, 0.4f + i * 0.3f); });
    return o;
}

OggettoDef CreaMolla(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Molla, 470, 28, 20, s);
    o.suono = "Molla";
    auto D = [=](Tela& t, float h) {
        t.Rett(2, 16, 24, 4, Hex(0x5a6070), 1, 1);
        int giri = 4;
        for (int i = 0; i < giri; i++) {
            float y = 16 - (i + 0.5f) * h / giri;
            t.Linea({ 6, y + 1.2f }, { 22, y - 1.2f }, 1.8f, Hex(0xc8d0e0));
        }
        t.Rett(1, 15 - h, 26, 3.5f, c, 1, 1.5f);
    };
    Anima(o.disegno, 28, 20, s, "ferma", 1, 1, true, [&](Tela& t, int) { D(t, 9); });
    Anima(o.disegno, 28, 20, s, "scatta", 3, 12, false, [&](Tela& t, int i) { D(t, i == 0 ? 3 : (i == 1 ? 13 : 9)); });
    return o;
}

OggettoDef CreaCassa(const std::string& nome, Color c, StileGrafico s, bool barile = false) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Cassa, 0, 32, 32, s);
    o.solido = true; o.gravita = true;
    Anima(o.disegno, 32, 32, s, "ferma", 1, 1, true, [&](Tela& t, int) {
        if (barile) {
            t.Ellisse(16, 16, 12, 15, c);
            t.Rett(4.5f, 8, 23, 2.2f, Hex(0x5a6070), 0.5f); t.Rett(4.5f, 22, 23, 2.2f, Hex(0x5a6070), 0.5f);
            return;
        }
        t.Rett(1, 1, 30, 30, c, 1, 2);
        t.Rett(4, 4, 24, 24, Mescola(c, Hex(SCURO), 0.15f), 0.5f, 1);
        t.Linea({ 5, 5 }, { 27, 27 }, 3.5f, c); t.Linea({ 27, 5 }, { 5, 27 }, 3.5f, c);
        for (int i = 0; i < 4; i++) t.Ellisse(i % 2 ? 28 : 4, i / 2 ? 28 : 4, 1, 1, Hex(0xc8d0e0), 0);
    });
    return o;
}

OggettoDef CreaLeva(const std::string& nome, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Interruttore, 0, 28, 24, s);
    o.suono = "Leva";
    auto D = [](Tela& t, float a) {
        t.Linea({ 14, 18 }, Ruota({ 14, 18 }, 13, PI + a), 2.4f, Hex(0x8a8a9a));
        Vector2 p = Ruota({ 14, 18 }, 13, PI + a);
        t.Ellisse(p.x, p.y, 3, 3, a > 0 ? Hex(0x5ac85a) : Hex(0xd8344a));
        t.Rett(4, 17, 20, 7, Hex(0x5a5a6a), 1, 2);
    };
    Anima(o.disegno, 28, 24, s, "spenta", 1, 1, true, [&](Tela& t, int) { D(t, -0.7f); });
    Anima(o.disegno, 28, 24, s, "accesa", 1, 1, true, [&](Tela& t, int) { D(t, 0.7f); });
    return o;
}

OggettoDef CreaPiattaforma(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::PiattaformaMobile, 0, 48, 14, s);
    o.solido = true; o.velocita = 30; o.distanza = 64;
    Anima(o.disegno, 48, 14, s, "ferma", 1, 1, true, [&](Tela& t, int) {
        t.Rett(1, 1, 46, 11, c, 1, 2);
        for (int i = 1; i < 4; i++) t.Rett(i * 12.0f, 2, 1, 9, Mescola(c, Hex(SCURO), 0.4f), 0);
        t.Rett(2, 2, 44, 2, Mescola(c, WHITE, 0.3f), 0);
    });
    return o;
}

OggettoDef CreaSpine(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Pericolo, 1, 32, 16, s);
    Anima(o.disegno, 32, 16, s, "ferme", 1, 1, true, [&](Tela& t, int) {
        t.Rett(0, 13, 32, 3, Mescola(c, Hex(SCURO), 0.4f), 0.5f);
        for (int i = 0; i < 4; i++) t.Poligono({ { i * 8.0f + 0.5f, 14 }, { i * 8.0f + 4, 1 }, { i * 8.0f + 7.5f, 14 } }, c);
    });
    return o;
}

OggettoDef CreaBandiera(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Arrivo, 0, 28, 48, s);
    o.suono = "Vittoria";
    Anima(o.disegno, 28, 48, s, "sventola", 4, 7, true, [&](Tela& t, int i) {
        t.Rett(3, 2, 2.5f, 45, Hex(0xc8d0e0), 0.8f);
        t.Ellisse(4.2f, 2.5f, 2.4f, 2.4f, Hex(0xffd23f));
        std::vector<Vector2> p;
        for (int k = 0; k <= 6; k++) { float x = 5.5f + k * 3.5f; p.push_back({ x, 5 + std::sin(i * PI / 2 + k * 0.9f) * 1.6f * k / 6 }); }
        for (int k = 6; k >= 0; k--) { float x = 5.5f + k * 3.5f; p.push_back({ x, 18 + std::sin(i * PI / 2 + k * 0.9f) * 1.6f * k / 6 - k * 0.4f }); }
        t.Poligono(p, c, 0.5f);
        t.Rett(4, 46, 4, 2, Hex(0x5a5a6a), 0);
    });
    return o;
}

OggettoDef CreaForziere(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Raccogli, 100, 32, 26, s);
    o.suono = "Forziere";
    auto D = [=](Tela& t, float a) {
        t.Rett(2, 12, 28, 13, c, 1, 2);
        t.Rett(2, 15, 28, 2.4f, Hex(0xffd23f), 0.5f);
        if (a > 0) {
            t.Ellisse(16, 12, 11, 3, Hex(0xffe060), 0);
            t.Rett(2, 2 - a * 2, 28, 6, Mescola(c, Hex(SCURO), 0.2f), 1, 2);
        } else {
            t.Rett(2, 4, 28, 10, c, 1, 4);
            t.Rett(2, 10, 28, 2.4f, Hex(0xffd23f), 0.5f);
        }
        t.Rett(14, 12, 4, 5, Hex(0xffd23f), 0.5f, 1);
    };
    Anima(o.disegno, 32, 26, s, "chiuso", 1, 1, true, [&](Tela& t, int) { D(t, 0); });
    Anima(o.disegno, 32, 26, s, "aperto", 1, 1, true, [&](Tela& t, int) { D(t, 1); });
    return o;
}

OggettoDef CreaStella(const std::string& nome, Color c, int valore, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Raccogli, valore, 22, 22, s);
    o.suono = "Gemma";
    Anima(o.disegno, 22, 22, s, "brilla", 4, 8, true, [&](Tela& t, int i) {
        std::vector<Vector2> p;
        float rot = i * 0.12f;
        for (int k = 0; k < 10; k++) {
            float r = k % 2 ? 4.2f : 10;
            float a = rot + k * PI / 5 - PI / 2;
            p.push_back({ 11 + std::cos(a) * r, 11.5f + std::sin(a) * r });
        }
        t.Poligono(p, c);
        t.Ellisse(9, 10, 0.9f, 1.6f, Hex(0x1a1426), 0); t.Ellisse(13, 10, 0.9f, 1.6f, Hex(0x1a1426), 0);
    });
    return o;
}

OggettoDef CreaPozione(const std::string& nome, Color c, StileGrafico s) {
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Cura, 2, 18, 24, s);
    o.suono = "Cura";
    Anima(o.disegno, 18, 24, s, "bolle", 3, 4, true, [&](Tela& t, int i) {
        t.Ellisse(9, 16, 7.5f, 7.5f, ConAlfa(Hex(0xd8f0ff), 0.6f), 0.3f);
        t.Ellisse(9, 17.5f, 6.2f, 5.6f, c);
        t.Rett(6, 3, 6, 6, ConAlfa(Hex(0xd8f0ff), 0.7f), 0.2f);
        t.Rett(5.5f, 1, 7, 3, Hex(0x8a5a30), 1, 1);
        t.Ellisse(7 + i, 16 - i * 1.5f, 1, 1, ConAlfa(WHITE, 0.8f), 0);
        t.Ellisse(6, 13, 1.2f, 2.2f, ConAlfa(WHITE, 0.6f), 0);
    });
    return o;
}

OggettoDef CreaProiettile(const std::string& nome, int tipo, Color c, StileGrafico s) {
    int W = tipo == 0 ? 16 : 20, H = tipo == 0 ? 16 : 8;
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Proiettile, 1, W, H, s);
    o.velocita = tipo == 1 ? 220 : 140;
    o.suono = tipo == 1 ? "Laser" : "Sparo";
    Anima(o.disegno, W, H, s, "vola", 3, 12, true, [&](Tela& t, int i) {
        if (tipo == 0) {
            t.Ellisse(8, 8, 6.5f + i * 0.3f, 6.5f - i * 0.3f, ConAlfa(c, 0.7f), 0);
            t.Ellisse(9, 8, 4.5f, 4.5f, Mescola(c, Hex(0xffe080), 0.5f), 0);
            t.Ellisse(9.5f, 7.5f, 2.2f, 2.2f, Hex(0xfff8e0), 0);
        } else if (tipo == 1) {
            t.Rett(1, 2, 18, 4, ConAlfa(c, 0.6f), 0, 2);
            t.Rett(3, 3, 15, 2, Mescola(c, WHITE, 0.6f), 0, 1);
        } else {
            t.Rett(1, 3.2f, 15, 1.6f, Hex(0x8a5a30), 0.4f);
            t.Poligono({ { 15, 1 }, { 20, 4 }, { 15, 7 } }, Hex(0xc8d0e0));
            t.Poligono({ { 0, 1.5f }, { 4, 4 }, { 0, 6.5f } }, c);
        }
    }, tipo == 2);
    return o;
}

OggettoDef CreaDecorazione(const std::string& nome, int tipo, Color c, StileGrafico s) {
    // 0 albero, 1 cespuglio, 2 roccia, 3 fiore, 4 torcia, 5 cartello, 6 cristallo, 7 lanterna, 8 colonna, 9 palma, 10 cactus, 11 lapide
    static const int dim[][2] = { { 48, 64 }, { 32, 20 }, { 28, 20 }, { 12, 18 }, { 12, 30 }, { 24, 24 }, { 16, 24 }, { 14, 26 }, { 20, 64 }, { 40, 64 }, { 22, 32 }, { 22, 26 } };
    int W = dim[tipo][0], H = dim[tipo][1];
    OggettoDef o = Oggetto(nome, FunzioneOggetto::Decorazione, 0, W, H, s);
    int n = (tipo == 4 || tipo == 7) ? 3 : 1;
    Anima(o.disegno, W, H, s, "ferma", n, 8, true, [&](Tela& t, int i) {
        switch (tipo) {
            case 0:
                t.Rett(20, 36, 8, 28, Hex(0x6a4020), 1, 2);
                t.Ellisse(24, 22, 20, 16, Mescola(c, Hex(SCURO), 0.2f));
                t.Ellisse(17, 26, 12, 10, c); t.Ellisse(30, 18, 12, 11, c); t.Ellisse(22, 12, 11, 9, Mescola(c, Hex(CHIARO), 0.1f));
                break;
            case 1: t.Ellisse(10, 12, 9, 8, c); t.Ellisse(22, 12, 9, 8, c); t.Ellisse(16, 8, 9, 7, Mescola(c, Hex(CHIARO), 0.1f)); t.Rett(2, 14, 28, 6, c, 0.5f, 3); break;
            case 2: t.Poligono({ { 1, 20 }, { 4, 8 }, { 12, 2 }, { 22, 5 }, { 27, 20 } }, c); break;
            case 3: t.Linea({ 6, 18 }, { 6, 8 }, 1.4f, Hex(0x3a8a3a)); for (int k = 0; k < 5; k++) t.Ellisse(6 + std::cos(k * 1.25f) * 3, 6 + std::sin(k * 1.25f) * 3, 2.2f, 2.2f, c, 0.5f); t.Ellisse(6, 6, 1.6f, 1.6f, Hex(0xffd23f), 0); break;
            case 4: t.Rett(4.5f, 12, 3, 18, Hex(0x6a4020), 1, 1); t.Ellisse(6, 8 - i * 0.5f, 3.5f + i * 0.3f, 6, ConAlfa(Hex(0xff8020), 0.8f), 0); t.Ellisse(6, 9, 2, 3.5f - i * 0.4f, Hex(0xffe080), 0); break;
            case 5: t.Rett(10.5f, 12, 3, 12, Hex(0x6a4020), 1); t.Rett(1, 2, 22, 12, c, 1, 1.5f); for (int k = 0; k < 3; k++) t.Rett(4, 4.5f + k * 3, 16 - k * 3, 1, Mescola(c, Hex(SCURO), 0.5f), 0); break;
            case 6: t.Poligono({ { 8, 0 }, { 14, 8 }, { 12, 24 }, { 4, 24 }, { 2, 8 } }, c); t.Poligono({ { 8, 0 }, { 8, 24 }, { 4, 24 }, { 2, 8 } }, Mescola(c, WHITE, 0.3f), 0); break;
            case 7: t.Rett(6, 0, 2, 4, Hex(0x3a3e52), 0); t.Rett(2, 4, 10, 3, Hex(0x3a3e52), 1, 1); t.Rett(3, 7, 8, 14, ConAlfa(Hex(0xffe080), 0.7f + i * 0.1f), 0, 1); t.Ellisse(7, 14, 2, 3, Hex(0xfff8e0), 0); t.Rett(2, 21, 10, 3, Hex(0x3a3e52), 1, 1); break;
            case 8: t.Rett(2, 0, 16, 5, c, 1, 1); t.Rett(4, 5, 12, 54, c, 1); for (int k = 0; k < 3; k++) t.Rett(6 + k * 3.5f, 6, 1, 52, Mescola(c, Hex(SCURO), 0.3f), 0); t.Rett(2, 59, 16, 5, c, 1, 1); break;
            case 9:
                t.Linea({ 20, 64 }, { 22, 20 }, 5, Hex(0x8a6a40));
                for (int k = 0; k < 5; k++) { float a = -2.6f + k * 1.3f; t.Poligono({ { 22, 18 }, { 22 + std::cos(a) * 18, 18 + std::sin(a) * 9 + 6 }, { 22 + std::cos(a) * 9, 18 + std::sin(a) * 4 } }, c, 0.6f); }
                t.Ellisse(20, 22, 2.5f, 2.5f, Hex(0x6a4020)); t.Ellisse(24, 22, 2.5f, 2.5f, Hex(0x6a4020));
                break;
            case 10: t.Rett(7, 2, 8, 30, c, 1, 4); t.Rett(1, 10, 6, 4, c, 1, 2); t.Rett(1, 4, 4, 9, c, 1, 2); t.Rett(15, 14, 6, 4, c, 1, 2); t.Rett(17, 8, 4, 9, c, 1, 2); break;
            default: t.Rett(3, 4, 16, 22, c, 1, 7); t.Rett(9.5f, 8, 3, 10, Mescola(c, Hex(SCURO), 0.4f), 0); t.Rett(7, 10.5f, 8, 3, Mescola(c, Hex(SCURO), 0.4f), 0); break;
        }
    });
    return o;
}

// --- Caselle ----------------------------------------------------------------------------

// Superficie ("#", con l'erba/la neve sopra quando e' esposta) e interno.
CasellaDef CreaTerreno(const std::string& nome, char car, Color sopra, Color terra, Color dettaglio, StileGrafico s, int stile) {
    CasellaDef c;
    c.carattere = car; c.nome = nome;
    c.disegno = NuovoDisegno(32, 32, s);
    auto D = [=](Tela& t, bool superficie) {
        // stile 0 terra con erba, 1 mattoni, 2 legno (piattaforma), 3 metallo, 4 spine, 5 ghiaccio/cristallo
        if (stile == 4) {
            for (int i = 0; i < 4; i++) t.Poligono({ { i * 8.0f + 0.5f, 32 }, { i * 8.0f + 4, 14 }, { i * 8.0f + 7.5f, 32 } }, terra);
            return;
        }
        if (stile == 2) {
            t.Rett(0, 0, 32, 12, terra, 1, 1.5f);
            t.Rett(0, 0, 32, 3, sopra, 0.5f);
            for (int i = 0; i < 2; i++) t.Rett(4 + i * 20.0f, 4, 2, 2, dettaglio, 0, 1);
            t.Rett(14, 1, 0.8f, 10, Mescola(terra, Hex(SCURO), 0.4f), 0);
            return;
        }
        if (stile == 1) {
            t.Rett(0, 0, 32, 32, Mescola(terra, Hex(SCURO), 0.35f), 0);
            for (int r = 0; r < 4; r++)
                for (int k = -1; k < 3; k++) {
                    float x = k * 16 + (r % 2) * 8;
                    t.Rett(x + 0.8f, r * 8 + 0.8f, 14.4f, 6.6f, Mescola(terra, dettaglio, Hash1(r * 7 + k * 3) * 0.3f), 1, 1);
                }
            if (superficie) t.Rett(0, 0, 32, 3, sopra, 0.6f, 1);
            return;
        }
        if (stile == 3) {
            t.Rett(0, 0, 32, 32, terra, 1, 2);
            t.Rett(2, 2, 28, 28, Mescola(terra, Hex(SCURO), 0.15f), 0.4f, 1.5f);
            for (int i = 0; i < 4; i++) t.Ellisse(i % 2 ? 27 : 5, i / 2 ? 27 : 5, 1.4f, 1.4f, dettaglio, 0.6f);
            t.Linea({ 6, 6 }, { 26, 26 }, 1, Mescola(terra, WHITE, 0.15f), 0);
            return;
        }
        // terra, sabbia, neve, ghiaccio
        t.Rett(0, 0, 32, 32, terra, 0.3f);
        for (int i = 0; i < 7; i++) {
            float x = Hash1(i * 13 + (int)car) * 28 + 2, y = Hash1(i * 7 + 3) * 22 + (superficie ? 9 : 2);
            t.Ellisse(x, y, 1.6f + Hash1(i) * 1.5f, 1.2f + Hash1(i + 5), stile == 5 ? Mescola(terra, WHITE, 0.4f) : Mescola(terra, dettaglio, 0.6f), 0.6f);
        }
        if (superficie) {
            t.Rett(0, 0, 32, 7, sopra, 0.6f);
            for (int i = 0; i < 8; i++) t.Ellisse(i * 4 + 2, 7, 2.4f, 1.6f + Hash1(i + 9), sopra, 0.3f);
            t.Rett(0, 0, 32, 1.4f, Mescola(sopra, Hex(CHIARO), 0.45f), 0);
        }
    };
    Anima(c.disegno, 32, 32, s, "superficie", 1, 1, true, [&](Tela& t, int) { D(t, true); }, false);
    Anima(c.disegno, 32, 32, s, "interno", 1, 1, true, [&](Tela& t, int) { D(t, false); }, false);
    return c;
}

// --- Scenari ----------------------------------------------------------------------------

Scenario CreaScenario(const std::string& nome, int tema, float ora, int pioggia, std::vector<StratoSfondo> strati = {}, bool generato = true,
                      Color alto = BLANK, Color basso = BLANK) {
    Scenario s;
    s.nome = nome; s.tema = tema; s.ora = ora; s.pioggia = pioggia; s.strati = strati; s.sfondoGenerato = generato;
    s.cicloGiorno = false;
    if (alto.a) { s.cieloPersonale = true; s.cieloAlto = alto; s.cieloBasso = basso; }
    return s;
}

StratoSfondo Strato(const char* tipo, Color c, float par, float altezza, float ampiezza, float seme, float vento = 0) {
    StratoSfondo s;
    s.tipo = tipo; s.nome = tipo; s.colore = c; s.parallasseX = par; s.parallasseY = par * 0.5f;
    s.altezza = altezza; s.ampiezza = ampiezza; s.seme = seme; s.vento = vento;
    return s;
}

} // namespace

// --- Musica -----------------------------------------------------------------------------------

const std::vector<std::string>& StiliBrano() {
    static const std::vector<std::string> v = { "Allegro", "Epico", "Calmo", "Misterioso", "Spaziale", "Battaglia" };
    return v;
}

Brano ComponiBrano(const std::string& nome, int seme, int stile, int battute) {
    Brano b;
    b.Nuovo();
    b.nome = nome;
    b.battute = battute;
    unsigned int rs = (unsigned int)seme * 2654435761u + 12345u;
    auto Caso = [&](int n) { rs = rs * 1664525u + 1013904223u; return (int)((rs >> 8) % (unsigned)std::max(1, n)); };
    static const int maggiore[] = { 0, 2, 4, 5, 7, 9, 11 }, minore[] = { 0, 2, 3, 5, 7, 8, 10 };
    bool minor = stile == 1 || stile == 3 || stile == 5 || (stile == 4 && seme % 2);
    const int* scala = minor ? minore : maggiore;
    static const float bpm[] = { 140, 126, 92, 100, 110, 156 };
    b.bpm = bpm[stile % 6] + Caso(8) - 4;
    int radice = 57 + Caso(5);   // La3..Do#4
    static const int progMagg[][4] = { { 0, 4, 5, 3 }, { 0, 3, 4, 3 }, { 0, 5, 3, 4 } };
    static const int progMin[][4] = { { 0, 5, 2, 6 }, { 0, 3, 4, 0 }, { 0, 6, 5, 4 } };
    const int* prog = minor ? progMin[Caso(3)] : progMagg[Caso(3)];
    auto Grado = [&](int g, int ottava) { int o = g / 7 + (g < 0 ? -1 : 0); int gg = ((g % 7) + 7) % 7; return radice + scala[gg] + 12 * (o + ottava); };
    static const int ritmi[][8] = { { 2, 2, 2, 2, 4, 4, 0 }, { 4, 2, 2, 4, 4, 0 }, { 3, 3, 2, 4, 4, 0 }, { 2, 2, 4, 2, 2, 4, 0 }, { 6, 2, 4, 4, 0 }, { 4, 4, 8, 0 } };
    int motivo[2] = { Caso(6), Caso(6) };
    if (stile == 2) { motivo[0] = 5; motivo[1] = 4; }
    TracciaBrano& mel = b.tracce[0];
    TracciaBrano& bas = b.tracce[1];
    TracciaBrano& acc = b.tracce[2];
    TracciaBrano& bat = b.tracce[3];
    mel.strumento = stile == 2 ? 1 : (stile == 4 ? 3 : 0);
    acc.strumento = stile == 1 || stile == 5 ? 2 : 3;
    int nota = 7 + Caso(3) * 2;
    for (int bt = 0; bt < battute; bt++) {
        int accordo = prog[bt % 4];
        float t0 = bt * 16.0f;
        // melodia: forma A A' B A
        int parte = (bt / 2) % 4;
        const int* r = ritmi[motivo[parte == 2 ? 1 : 0]];
        float t = 0;
        for (int i = 0; r[i] && t < 16; i++) {
            int passo = Caso(5) - 2;
            if (i == 0 || r[i] >= 4) {   // sui tempi forti, note dell'accordo
                int target = accordo + 7 + (Caso(3)) * 2;
                passo = target > nota ? std::min(2, target - nota) : std::max(-2, target - nota);
            }
            nota = (int)Limita((float)(nota + passo), 3, 15);
            if (bt == battute - 1 && !r[i + 1]) nota = 7;   // finale sulla tonica
            bool pausa = Caso(10) == 0 && i > 0;
            if (!pausa) mel.note.push_back({ t0 + t, (float)r[i] * 0.9f, Grado(nota, 0) });
            t += r[i];
        }
        // basso
        int passoB = stile == 2 ? 8 : (stile == 5 ? 2 : 4);
        for (int k = 0; k < 16; k += passoB) {
            int g = accordo + ((k / passoB) % 4 == 3 && stile != 2 ? 4 : 0);
            bas.note.push_back({ t0 + k, (float)passoB * 0.85f, Grado(g, -2) });
        }
        // accordi: arpeggio o tenuti
        if (stile == 2 || stile == 3) {
            for (int v = 0; v < 3; v++) acc.note.push_back({ t0, 15.5f, Grado(accordo + v * 2, -1) });
        } else {
            for (int k = 0; k < 16; k += 2) acc.note.push_back({ t0 + k, 1.6f, Grado(accordo + ((k / 2) % 3) * 2, 0) });
        }
        // batteria: 36 cassa, 38 rullante, 42 charleston
        if (stile != 2) {
            for (int k = 0; k < 16; k += 2) bat.note.push_back({ t0 + k, 1, 42 });
            bat.note.push_back({ t0, 1, 36 });
            bat.note.push_back({ t0 + (stile == 5 ? 6.0f : 8.0f), 1, 36 });
            if (stile == 5 || stile == 0) bat.note.push_back({ t0 + 10, 1, 36 });
            bat.note.push_back({ t0 + 4, 1, 38 });
            bat.note.push_back({ t0 + 12, 1, 38 });
            if (bt % 4 == 3) { bat.note.push_back({ t0 + 14, 1, 38 }); bat.note.push_back({ t0 + 15, 1, 38 }); }
        } else {
            for (int k = 0; k < 16; k += 4) bat.note.push_back({ t0 + k, 1, 42 });
        }
    }
    if (stile == 3) { mel.volume = 0.35f; bat.volume = 0.3f; }
    return b;
}

namespace {
EffettoSuono Eff(const char* nome, int onda, float f0, float f1, float durata, float volume, float duty = 0.5f, float vib = 0, float vibHz = 0, float rumore = 0) {
    EffettoSuono e;
    e.nome = nome; e.onda = onda; e.f0 = f0; e.f1 = f1; e.durata = durata; e.volume = volume; e.duty = duty; e.vibrato = vib; e.vibratoHz = vibHz; e.rumore = rumore;
    return e;
}
}

std::vector<EffettoSuono> EffettiArcade() {
    return {
        Eff("Salto", 0, 260, 680, 0.17f, 0.45f, 0.25f),
        Eff("Moneta", 0, 988, 1319, 0.22f, 0.4f, 0.5f),
        Eff("Gemma", 1, 1047, 2093, 0.4f, 0.5f, 0.5f, 0.01f, 9),
        Eff("Colpo", 4, 900, 120, 0.28f, 0.5f),
        Eff("Esplosione", 4, 1500, 40, 0.7f, 0.7f),
        Eff("Sparo", 0, 880, 220, 0.12f, 0.35f, 0.25f, 0, 0, 0.2f),
        Eff("Laser", 2, 1800, 300, 0.18f, 0.35f),
        Eff("Molla", 1, 140, 900, 0.38f, 0.6f, 0.5f, 0.12f, 28),
        Eff("Porta", 2, 120, 80, 0.45f, 0.45f, 0.5f, 0.05f, 12, 0.3f),
        Eff("Chiave", 1, 1568, 2637, 0.25f, 0.45f),
        Eff("Cura", 3, 523, 1046, 0.45f, 0.5f, 0.5f, 0.02f, 7),
        Eff("Ruggito", 2, 180, 60, 0.9f, 0.6f, 0.5f, 0.12f, 18, 0.45f),
        Eff("Potenziamento", 0, 300, 1600, 0.6f, 0.4f, 0.5f, 0.06f, 14),
        Eff("Danno", 0, 400, 90, 0.3f, 0.45f, 0.5f, 0.1f, 30, 0.1f),
        Eff("Passo", 4, 300, 200, 0.04f, 0.2f),
        Eff("Vittoria", 0, 523, 1046, 0.8f, 0.4f, 0.25f, 0.01f, 6),
        Eff("Leva", 0, 600, 300, 0.08f, 0.35f, 0.5f, 0, 0, 0.3f),
        Eff("Forziere", 1, 784, 1568, 0.5f, 0.45f, 0.5f, 0.01f, 7),
        Eff("Menu", 0, 660, 660, 0.05f, 0.25f),
        Eff("Magia", 3, 800, 2400, 0.7f, 0.4f, 0.5f, 0.08f, 10),
    };
}

EffettoSuono EffettoCasuale(int seme) {
    unsigned int r = (unsigned int)seme * 2654435761u + 7u;
    auto C = [&](float lo, float hi) { r = r * 1664525u + 1013904223u; return lo + (hi - lo) * ((r >> 8) % 10000) / 10000.0f; };
    EffettoSuono e = Eff("Effetto", (int)C(0, 4.99f), C(80, 1800), C(80, 2400), C(0.08f, 0.7f), 0.45f, C(0.1f, 0.6f));
    if (C(0, 1) < 0.3f) { e.vibrato = C(0.01f, 0.15f); e.vibratoHz = C(4, 30); }
    if (C(0, 1) < 0.25f) e.rumore = C(0.1f, 0.6f);
    return e;
}

// --- Le raccolte ---------------------------------------------------------------------------

namespace {

struct InfoRaccolta { const char* nome; const char* descrizione; };
const InfoRaccolta RACCOLTE[] = {
    { "Base", "L'eroe, uno slime, un pipistrello, monete, gemme, cuori, molle, porte, chiavi, il prato e le musiche di partenza" },
    { "Fantasy", "Cavaliere, maga, elfo, orco e il drago Fiammaros; pozioni, cristalli e un regno incantato" },
    { "Castello", "Scheletro, cavaliere nero, gargoyle; mattoni di pietra, torce, colonne e porte di ferro" },
    { "Spazio", "Astronave, navi nemiche, robot, laser; caselle di metallo e cielo stellato" },
    { "Pirati", "Capitano, pirati, granchi e pesci; barili, forzieri del tesoro e palme" },
    { "Deserto", "Esploratore, scorpione, mummia; sabbia, cactus e un tramonto rosso" },
    { "Ghiaccio", "Eschimese, pinguino, yeti; neve, ghiaccio, cristalli e tormenta" },
    { "Foresta", "Ranger, funghetti, api, ragni; alberi, cespugli, fiori e il bosco" },
    { "Citta'", "Eroina urbana, ladro, robot poliziotto; mattoni, lampioni e cartelli" },
    { "Robot", "Robot amici e nemici, torrette e carri armati; metallo e circuiti" },
    { "Mostri", "Fantasmi, slime giganti, pipistrelli e piante carnivore" },
    { "Mare", "Sub, pesci, granchi, meduse; sabbia, corallo e onde" },
    { "Vulcano", "Golem di lava, draghetti, palle di fuoco; rocce ardenti e cielo scuro" },
    { "Cimitero", "Zombi, fantasmi e scheletri; lapidi, lanterne e notte di luna" },
    { "Giungla", "Esploratrice, dinosauri, scimmie; palme, liane e piante giganti" },
    { "Oriente", "Ninja, samurai, lanterne di carta, piattaforme di bambu'" },
    { "Fattoria", "Contadino, galline, lumache; staccionate, fiori e campi" },
    { "Cielo", "Uccelli, api, nuvole solide e isole sospese" },
    { "Sotterranei", "Lumache, ragni, cristalli; caverne, torce e casse" },
    { "Carri armati", "Carri amici e nemici, mine, proiettili, deserto di battaglia" },
    { "Musiche chiptune", "Dieci brani pronti: allegri, epici, calmi, misteriosi, spaziali e di battaglia" },
    { "Suoni arcade", "Venti effetti sonori: salti, monete, esplosioni, laser, porte, ruggiti, magie" },
};
constexpr int N_RACCOLTE = (int)(sizeof(RACCOLTE) / sizeof(RACCOLTE[0]));

void Autore(Raccolta& r) {
    for (auto& x : r.personaggi) x.autore = r.nome;
    for (auto& x : r.oggetti) x.autore = r.nome;
    for (auto& x : r.scenari) x.autore = r.nome;
    for (auto& x : r.brani) x.autore = r.nome;
    for (auto& x : r.effetti) x.autore = r.nome;
}

Raccolta Genera(int i, StileGrafico s) {
    Raccolta r;
    r.nome = RACCOLTE[i].nome;
    r.descrizione = RACCOLTE[i].descrizione;
    using TC = TipoComportamento;
    auto Nemico = [](PersonaggioDef p, TC t, float vel, int energia = 1) { p.comportamento.tipo = t; p.comportamento.velocita = vel; p.comportamento.energia = energia; return p; };
    std::string nome = r.nome;
    if (nome == "Base") {
        Umano eroe;
        eroe.mantello = Hex(0xd8344a);
        r.personaggi.push_back(CreaUmano("Eroe", eroe, s, TC::Giocatore));
        r.personaggi.push_back(CreaSlime("Slime", Hex(0x5ac85a), s));
        r.personaggi.push_back(CreaVolante("Pipistrello", 0, Hex(0x6a3a8a), Hex(0x4a2a6a), s));
        Umano merc; merc.maglia = Hex(0xc8904a); merc.capelli = Hex(0xd8d8d8); merc.barba = true; merc.cintura = Hex(0x6a3a20);
        PersonaggioDef m = CreaUmano("Mercante", merc, s, TC::Parla);
        m.comportamento.dialogo = "Benvenuto, viaggiatore! Raccogli le monete e trova la bandiera.";
        r.personaggi.push_back(m);
        r.oggetti.push_back(CreaMoneta("Moneta", Hex(0xffc93c), 10, s));
        r.oggetti.push_back(CreaGemma("Gemma", Hex(0x4ad8f0), 50, s));
        r.oggetti.push_back(CreaCuore("Cuore", Hex(0xff3f6a), s));
        r.oggetti.push_back(CreaChiave("Chiave", Hex(0xffc93c), s));
        r.oggetti.push_back(CreaPorta("Porta", Hex(0x8a5a30), Hex(0x5a6070), "Chiave", s));
        r.oggetti.push_back(CreaMolla("Molla", Hex(0xd8344a), s));
        r.oggetti.push_back(CreaCassa("Cassa", Hex(0xb07a40), s));
        r.oggetti.push_back(CreaForziere("Forziere", Hex(0x9a5a2a), s));
        r.oggetti.push_back(CreaBandiera("Bandiera", Hex(0xd8344a), s));
        r.oggetti.push_back(CreaSpine("Spine", Hex(0xc8d0e0), s));
        r.oggetti.push_back(CreaProiettile("Palla di fuoco", 0, Hex(0xff6020), s));
        r.caselle.push_back(CreaTerreno("Prato", '#', Hex(0x5ac85a), Hex(0x8a5a34), Hex(0x6a4024), s, 0));
        r.caselle.push_back(CreaTerreno("Pietra", '=', Hex(0x9aa0b8), Hex(0x7a8098), Hex(0x8a90a8), s, 1));
        r.caselle.push_back(CreaTerreno("Asse di legno", '-', Hex(0xc8904a), Hex(0x9a6a34), Hex(0x5a3a20), s, 2));
        r.caselle.push_back(CreaTerreno("Spine", '^', Hex(0xc8d0e0), Hex(0xc8d0e0), Hex(0x8a90a8), s, 4));
        r.scenari.push_back(CreaScenario("Valle verde", 0, 0.35f, 0));
        r.brani.push_back(ComponiBrano("Prati felici", 7, 0));
        r.effetti = EffettiArcade();
        r.effetti.resize(12);
    } else if (nome == "Fantasy") {
        Umano cav; cav.maglia = Hex(0xb8c0d8); cav.elmo = Hex(0xc8d0e0); cav.mantello = Hex(0x2a5bd8); cav.armaTipo = 1; cav.pantaloni = Hex(0x5a6078);
        r.personaggi.push_back(CreaUmano("Cavaliere", cav, s, TC::Giocatore));
        Umano maga; maga.maglia = Hex(0x7a3ad8); maga.capelli = Hex(0xf08ac8); maga.capelliTipo = 4; maga.cappello = Hex(0x3a2a8a); maga.armaTipo = 2; maga.arma = Hex(0x5ef0ff); maga.mantello = Hex(0x3ad8b0);
        r.personaggi.push_back(CreaUmano("Maga", maga, s, TC::Giocatore));
        Umano elfo; elfo.maglia = Hex(0x3a8a4a); elfo.capelli = Hex(0xf0d890); elfo.capelliTipo = 1; elfo.armaTipo = 3; elfo.mantello = Hex(0x2a5a3a);
        r.personaggi.push_back(CreaUmano("Elfo", elfo, s, TC::Parla));
        Umano orco; orco.pelle = Hex(0x6aa84a); orco.maglia = Hex(0x6a4a2a); orco.capelliTipo = 3; orco.robusto = 1.3f; orco.armaTipo = 1; orco.arma = Hex(0x8a8a9a); orco.mostro = true;
        r.personaggi.push_back(Nemico(CreaUmano("Orco", orco, s, TC::Pattuglia), TC::Insegue, 34, 3));
        PersonaggioDef drago = CreaVolante("Fiammaros", 2, Hex(0xd8344a), Hex(0x8a1a3a), s);
        drago.comportamento.proiettile = "Palla di fuoco";
        drago.suoni["colpito"] = "Ruggito";
        drago.suoni["attacco"] = "Ruggito";
        r.personaggi.push_back(drago);
        r.personaggi.push_back(CreaVolante("Draghetto verde", 2, Hex(0x5ac85a), Hex(0x2a7a4a), s));
        r.personaggi.back().comportamento.tipo = TC::Vola; r.personaggi.back().comportamento.energia = 3;
        r.oggetti.push_back(CreaPozione("Pozione", Hex(0xd83a8a), s));
        r.oggetti.push_back(CreaDecorazione("Cristallo", 6, Hex(0x8a6af0), s));
        r.oggetti.push_back(CreaGemma("Rubino", Hex(0xff3f5a), 100, s));
        r.oggetti.push_back(CreaProiettile("Palla di fuoco", 0, Hex(0xff6020), s));
        r.oggetti.push_back(CreaStella("Stella magica", Hex(0xffd23f), 200, s));
        r.scenari.push_back(CreaScenario("Regno incantato", 0, 0.72f, 0, {}, true));
        r.brani.push_back(ComponiBrano("Tema del drago", 21, 1, 16));
        r.brani.push_back(ComponiBrano("Villaggio degli elfi", 22, 2));
    } else if (nome == "Castello") {
        Umano sch; sch.scheletro = true; sch.armaTipo = 1; sch.arma = Hex(0x8a8a9a);
        r.personaggi.push_back(Nemico(CreaUmano("Scheletro", sch, s, TC::Pattuglia), TC::Pattuglia, 30, 2));
        Umano nero; nero.maglia = Hex(0x2a2a3a); nero.elmo = Hex(0x3a3a4a); nero.mantello = Hex(0x6a1a2a); nero.armaTipo = 1; nero.pantaloni = Hex(0x1a1a24); nero.robusto = 1.15f;
        r.personaggi.push_back(Nemico(CreaUmano("Cavaliere nero", nero, s, TC::Insegue), TC::Insegue, 40, 4));
        PersonaggioDef garg = CreaVolante("Gargoyle", 0, Hex(0x7a8098), Hex(0x5a607c), s);
        garg.comportamento.energia = 2;
        r.personaggi.push_back(garg);
        Umano re; re.maglia = Hex(0xd8344a); re.mantello = Hex(0x7a3ad8); re.capelli = Hex(0xd8d8d8); re.barba = true; re.capelliTipo = 0;
        PersonaggioDef rep = CreaUmano("Re", re, s, TC::Parla); rep.comportamento.dialogo = "Salva il regno dal drago, prode eroe!";
        r.personaggi.push_back(rep);
        r.oggetti.push_back(CreaDecorazione("Torcia", 4, Hex(0xff8020), s));
        r.oggetti.push_back(CreaDecorazione("Colonna", 8, Hex(0x9aa0b8), s));
        r.oggetti.push_back(CreaPorta("Porta di ferro", Hex(0x5a5a6a), Hex(0x3a3a4a), "Chiave d'oro", s));
        r.oggetti.push_back(CreaChiave("Chiave d'oro", Hex(0xffd23f), s));
        r.oggetti.push_back(CreaLeva("Leva", s));
        r.caselle.push_back(CreaTerreno("Mattoni del castello", '#', Hex(0xa8aec4), Hex(0x7a8098), Hex(0x9aa0b8), s, 1));
        r.scenari.push_back(CreaScenario("Castello al tramonto", 3, 0.74f, 0, { Strato("montagne", Hex(0x6a3a8a), 0.15f, 0.62f, 60, 3), Strato("colline", Hex(0x3a2050), 0.35f, 0.8f, 30, 5) }, false,
                                         Hex(0x2a1a5c), Hex(0xf7b267)));
        r.brani.push_back(ComponiBrano("Mura antiche", 31, 3));
    } else if (nome == "Spazio") {
        r.personaggi.push_back(CreaNave("Astronave", Hex(0xc8d0e0), Hex(0x5ef0ff), s, false));
        r.personaggi.push_back(CreaNave("Caccia alieno", Hex(0x8a3ad8), Hex(0x5aff8a), s, true));
        r.personaggi.push_back(CreaNave("Incrociatore", Hex(0xd8344a), Hex(0xffd23f), s, true));
        r.personaggi.back().comportamento.energia = 4;
        Umano astro; astro.maglia = Hex(0xe8ecf4); astro.elmo = Hex(0xe8ecf4); astro.pantaloni = Hex(0xc8d0e0); astro.armaTipo = 4;
        r.personaggi.push_back(CreaUmano("Astronauta", astro, s, TC::Giocatore));
        r.personaggi.push_back(CreaRobot("Robot alieno", Hex(0x5a9a5a), Hex(0xff3f6a), s, TC::Pattuglia));
        r.oggetti.push_back(CreaProiettile("Laser", 1, Hex(0x5ef0ff), s));
        r.oggetti.push_back(CreaProiettile("Laser rosso", 1, Hex(0xff3f5a), s));
        r.oggetti.push_back(CreaStella("Stella", Hex(0xffe060), 50, s));
        r.oggetti.push_back(CreaGemma("Cristallo di energia", Hex(0x5aff8a), 100, s));
        r.caselle.push_back(CreaTerreno("Metallo", '#', Hex(0x9aa0b8), Hex(0x6a7088), Hex(0xc8d0e0), s, 3));
        r.scenari.push_back(CreaScenario("Spazio profondo", 2, 0.0f, 0, { Strato("stelle", Hex(0xffffff), 0.05f, 0.5f, 0, 2), Strato("stelle", Hex(0x9ac8ff), 0.15f, 0.5f, 0, 7) }, false,
                                         Hex(0x05081c), Hex(0x1a1040)));
        r.brani.push_back(ComponiBrano("Orbita", 41, 4));
    } else if (nome == "Pirati") {
        Umano cap; cap.maglia = Hex(0x2a3a6a); cap.capelliTipo = 4; cap.cappello = Hex(0x1a1a24); cap.barba = true; cap.capelli = Hex(0x1a1a24); cap.armaTipo = 1; cap.mantello = Hex(0xd8344a);
        r.personaggi.push_back(CreaUmano("Capitano", cap, s, TC::Giocatore));
        Umano pir; pir.maglia = Hex(0xe8e4d8); pir.capelliTipo = 5; pir.cappello = Hex(0xd8344a); pir.armaTipo = 1; pir.pantaloni = Hex(0x3a3a6a);
        r.personaggi.push_back(Nemico(CreaUmano("Pirata", pir, s, TC::Pattuglia), TC::Pattuglia, 36, 2));
        r.personaggi.push_back(CreaBestia("Granchio", 4, Hex(0xe85a3a), Hex(0xffffff), s));
        r.personaggi.push_back(CreaBestia("Pesce palla", 0, Hex(0xffc93c), Hex(0xe8a020), s));
        r.oggetti.push_back(CreaCassa("Barile", Hex(0x9a6a34), s, true));
        r.oggetti.push_back(CreaForziere("Tesoro", Hex(0x7a4a20), s));
        r.oggetti.push_back(CreaMoneta("Doblone", Hex(0xffd23f), 20, s));
        r.oggetti.push_back(CreaDecorazione("Palma", 9, Hex(0x3aa84a), s));
        r.caselle.push_back(CreaTerreno("Spiaggia", '#', Hex(0xf0dca0), Hex(0xe0c080), Hex(0xc8a060), s, 0));
        r.scenari.push_back(CreaScenario("Isola", 0, 0.45f, 0, { Strato("fascia", Hex(0x2a7ad8), 0.1f, 0.78f, 0, 1), Strato("nuvole", Hex(0xffffff), 0.1f, 0.2f, 0, 3, 6) }, false,
                                         Hex(0x3fa0f0), Hex(0xc9f0ff)));
        r.brani.push_back(ComponiBrano("Rotta per l'isola", 51, 0));
    } else if (nome == "Deserto") {
        Umano esp; esp.maglia = Hex(0xc8a060); esp.capelliTipo = 4; esp.cappello = Hex(0x8a6a40); esp.pantaloni = Hex(0x6a5030);
        r.personaggi.push_back(CreaUmano("Esploratore", esp, s, TC::Giocatore));
        r.personaggi.push_back(CreaBestia("Scorpione", 4, Hex(0x2a2a3a), Hex(0xffd23f), s));
        Umano mum; mum.pelle = Hex(0xe8dcc0); mum.maglia = Hex(0xe8dcc0); mum.pantaloni = Hex(0xd8ccb0); mum.capelliTipo = 3; mum.scarpe = Hex(0xd8ccb0); mum.occhi = Hex(0xd8344a);
        r.personaggi.push_back(Nemico(CreaUmano("Mummia", mum, s, TC::Insegue), TC::Insegue, 18, 3));
        r.oggetti.push_back(CreaDecorazione("Cactus", 10, Hex(0x5a9a4a), s));
        r.oggetti.push_back(CreaGemma("Scarabeo d'oro", Hex(0xffd23f), 80, s));
        r.caselle.push_back(CreaTerreno("Sabbia", '#', Hex(0xf0c878), Hex(0xd8a860), Hex(0xb88848), s, 0));
        r.scenari.push_back(CreaScenario("Dune rosse", 1, 0.62f, 0));
        r.brani.push_back(ComponiBrano("Miraggio", 61, 3));
    } else if (nome == "Ghiaccio") {
        Umano esk; esk.maglia = Hex(0x5a8ad8); esk.capelliTipo = 4; esk.cappello = Hex(0xe8ecf4); esk.pantaloni = Hex(0x3a4a7a);
        r.personaggi.push_back(CreaUmano("Esquimese", esk, s, TC::Giocatore));
        PersonaggioDef ping = CreaSlime("Pinguino", Hex(0x2a2a3a), s);
        ping.comportamento.velocita = 40;
        r.personaggi.push_back(ping);
        Umano yeti; yeti.pelle = Hex(0xb8d0e8); yeti.maglia = Hex(0xe8f0f8); yeti.pantaloni = Hex(0xe8f0f8); yeti.capelliTipo = 3; yeti.robusto = 1.4f; yeti.mostro = true; yeti.scarpe = Hex(0xc8d8e8);
        r.personaggi.push_back(Nemico(CreaUmano("Yeti", yeti, s, TC::Insegue), TC::Insegue, 30, 5));
        r.oggetti.push_back(CreaDecorazione("Cristallo di ghiaccio", 6, Hex(0x9ad8ff), s));
        r.oggetti.push_back(CreaGemma("Fiocco di neve", Hex(0xe8f4ff), 30, s));
        r.caselle.push_back(CreaTerreno("Neve", '#', Hex(0xf4f8ff), Hex(0x9ab8d8), Hex(0x7a98c0), s, 5));
        r.scenari.push_back(CreaScenario("Tormenta", 2, 0.4f, 2));
        r.brani.push_back(ComponiBrano("Aurora", 71, 2));
    } else if (nome == "Foresta") {
        Umano ran; ran.maglia = Hex(0x3a6a3a); ran.capelliTipo = 5; ran.cappello = Hex(0x5a8a3a); ran.armaTipo = 3;
        r.personaggi.push_back(CreaUmano("Ranger", ran, s, TC::Giocatore));
        r.personaggi.push_back(CreaBestia("Funghetto", 3, Hex(0xd8344a), Hex(0xffffff), s));
        r.personaggi.push_back(CreaVolante("Ape", 3, Hex(0xffc93c), Hex(0xe8f4ff), s));
        r.personaggi.push_back(CreaBestia("Ragno", 1, Hex(0x3a2a4a), Hex(0xd8344a), s));
        r.oggetti.push_back(CreaDecorazione("Albero", 0, Hex(0x3a9a4a), s));
        r.oggetti.push_back(CreaDecorazione("Cespuglio", 1, Hex(0x4aa84a), s));
        r.oggetti.push_back(CreaDecorazione("Fiore", 3, Hex(0xff6a9a), s));
        r.oggetti.push_back(CreaMoneta("Ghianda d'oro", Hex(0xd8a040), 15, s));
        r.caselle.push_back(CreaTerreno("Terra del bosco", '#', Hex(0x3a9a3a), Hex(0x6a4a2a), Hex(0x4a3020), s, 0));
        r.scenari.push_back(CreaScenario("Bosco", 0, 0.3f, 1, { Strato("colline", Hex(0x3a6a5a), 0.2f, 0.7f, 40, 2), Strato("foresta", Hex(0x1e3a2b), 0.45f, 0.82f, 30, 4) }, false,
                                         Hex(0x5a8ab0), Hex(0xc8e0d0)));
        r.brani.push_back(ComponiBrano("Sentiero nel bosco", 81, 2));
    } else if (nome == "Citta'") {
        Umano er; er.maglia = Hex(0xffc93c); er.capelli = Hex(0x1a1a24); er.capelliTipo = 1; er.pantaloni = Hex(0x2a4a8a); er.mantello = Hex(0x2a4a8a);
        r.personaggi.push_back(CreaUmano("Eroina", er, s, TC::Giocatore));
        Umano ladro; ladro.maglia = Hex(0x2a2a3a); ladro.capelliTipo = 5; ladro.cappello = Hex(0x1a1a24); ladro.pantaloni = Hex(0x1a1a24);
        r.personaggi.push_back(Nemico(CreaUmano("Ladro", ladro, s, TC::Pattuglia), TC::Pattuglia, 50, 1));
        r.personaggi.push_back(CreaRobot("Robot poliziotto", Hex(0x2a4a8a), Hex(0xff3f5a), s, TC::Insegue));
        r.oggetti.push_back(CreaDecorazione("Lampione", 7, Hex(0x3a3e52), s));
        r.oggetti.push_back(CreaDecorazione("Cartello", 5, Hex(0x2a7ad8), s));
        r.oggetti.push_back(CreaMoneta("Gettone", Hex(0xc8d0e0), 10, s));
        r.caselle.push_back(CreaTerreno("Mattoni rossi", '#', Hex(0x9aa0b8), Hex(0xa84a3a), Hex(0xc86a4a), s, 1));
        r.scenari.push_back(CreaScenario("Citta' di notte", 2, 0.92f, 1, { Strato("colline", Hex(0x2a2a4a), 0.25f, 0.75f, 70, 9) }, true));
        r.brani.push_back(ComponiBrano("Luci della citta'", 91, 0));
    } else if (nome == "Robot") {
        r.personaggi.push_back(CreaRobot("Robottino", Hex(0xc8d0e0), Hex(0x5ef0ff), s, TC::Giocatore));
        r.personaggi.push_back(CreaRobot("Torretta", Hex(0x6a7088), Hex(0xff3f5a), s, TC::Spara));
        r.personaggi.back().comportamento.velocita = 0;
        r.personaggi.push_back(CreaRobot("Drone", Hex(0x3a8a5a), Hex(0xffd23f), s, TC::Vola));
        r.personaggi.back().comportamento.gravita = false;
        r.personaggi.push_back(CreaCarro("Carro corazzato", Hex(0x5a6a4a), s));
        r.oggetti.push_back(CreaGemma("Bullone", Hex(0xc8d0e0), 10, s));
        r.oggetti.push_back(CreaLeva("Interruttore", s));
        r.oggetti.push_back(CreaPiattaforma("Nastro", Hex(0x6a7088), s));
        r.caselle.push_back(CreaTerreno("Circuiti", '#', Hex(0x5aff8a), Hex(0x2a4a3a), Hex(0x3a8a5a), s, 3));
        r.scenari.push_back(CreaScenario("Fabbrica", 2, 0.85f, 0, { Strato("fascia", Hex(0x2a2e3a), 0.2f, 0.75f, 0, 1) }, false, Hex(0x1a2030), Hex(0x4a5070)));
        r.brani.push_back(ComponiBrano("Catena di montaggio", 101, 4));
    } else if (nome == "Mostri") {
        r.personaggi.push_back(CreaFantasma("Fantasma", Hex(0xe8f0ff), s));
        r.personaggi.push_back(CreaSlime("Slime gigante", Hex(0x8a3ad8), s));
        r.personaggi.back().comportamento.energia = 3;
        r.personaggi.push_back(CreaBestia("Pianta carnivora", 2, Hex(0xd8344a), Hex(0xffffff), s));
        r.personaggi.back().comportamento.proiettile = "Seme velenoso";
        r.personaggi.push_back(CreaVolante("Pipistrello vampiro", 0, Hex(0x3a1a2a), Hex(0x8a1a2a), s));
        r.oggetti.push_back(CreaProiettile("Seme velenoso", 0, Hex(0x8aff3a), s));
        r.brani.push_back(ComponiBrano("Notte dei mostri", 111, 3));
    } else if (nome == "Mare") {
        Umano sub; sub.maglia = Hex(0xffc93c); sub.elmo = Hex(0x9ad8ff); sub.pantaloni = Hex(0x2a4a8a);
        r.personaggi.push_back(CreaUmano("Sub", sub, s, TC::Giocatore));
        r.personaggi.push_back(CreaBestia("Pesce", 0, Hex(0x3ac8e8), Hex(0x2a8ab8), s));
        r.personaggi.push_back(CreaFantasma("Medusa", Hex(0xf08ac8), s));
        r.personaggi.back().comportamento.tipo = TC::Vola;
        r.personaggi.push_back(CreaBestia("Granchio rosso", 4, Hex(0xd8344a), Hex(0xffffff), s));
        r.oggetti.push_back(CreaGemma("Perla", Hex(0xf4f0ff), 40, s));
        r.oggetti.push_back(CreaDecorazione("Corallo", 1, Hex(0xff6a7a), s));
        r.caselle.push_back(CreaTerreno("Fondale", '#', Hex(0xe0d0a0), Hex(0xc8b080), Hex(0xa89060), s, 0));
        r.scenari.push_back(CreaScenario("Abisso blu", 2, 0.5f, 0, { Strato("colline", Hex(0x1a4a7a), 0.2f, 0.72f, 40, 3), Strato("colline", Hex(0x123a6a), 0.4f, 0.85f, 25, 6) }, false,
                                         Hex(0x0a2a5a), Hex(0x2a8ac8)));
        r.brani.push_back(ComponiBrano("Onde", 121, 2));
    } else if (nome == "Vulcano") {
        Umano golem; golem.pelle = Hex(0x5a4040); golem.maglia = Hex(0x4a3434); golem.pantaloni = Hex(0x3a2a2a); golem.capelliTipo = 3; golem.robusto = 1.45f; golem.mostro = true; golem.occhi = Hex(0xff8020);
        r.personaggi.push_back(Nemico(CreaUmano("Golem di lava", golem, s, TC::Insegue), TC::Insegue, 22, 6));
        r.personaggi.push_back(CreaVolante("Draghetto di fuoco", 2, Hex(0xff6020), Hex(0xa82a10), s));
        r.personaggi.back().comportamento.tipo = TC::Vola; r.personaggi.back().comportamento.energia = 3;
        r.personaggi.push_back(CreaSlime("Magma", Hex(0xff5a20), s));
        r.oggetti.push_back(CreaProiettile("Lapillo", 0, Hex(0xff3010), s));
        r.oggetti.push_back(CreaGemma("Ossidiana", Hex(0x5a3a6a), 60, s));
        r.caselle.push_back(CreaTerreno("Roccia ardente", '#', Hex(0xff6a20), Hex(0x4a3030), Hex(0x2a1a1a), s, 0));
        r.scenari.push_back(CreaScenario("Terre di fuoco", 3, 0.82f, 0));
        r.brani.push_back(ComponiBrano("Cuore del vulcano", 131, 5));
    } else if (nome == "Cimitero") {
        Umano zomb; zomb.pelle = Hex(0x8ab08a); zomb.maglia = Hex(0x5a5a7a); zomb.pantaloni = Hex(0x3a3a4a); zomb.capelli = Hex(0x3a3a2a); zomb.occhi = Hex(0xd8344a);
        r.personaggi.push_back(Nemico(CreaUmano("Zombi", zomb, s, TC::Pattuglia), TC::Insegue, 16, 2));
        r.personaggi.push_back(CreaFantasma("Spettro", Hex(0xb8c8ff), s));
        Umano sch; sch.scheletro = true;
        r.personaggi.push_back(Nemico(CreaUmano("Scheletrino", sch, s, TC::Pattuglia), TC::Pattuglia, 34, 1));
        r.oggetti.push_back(CreaDecorazione("Lapide", 11, Hex(0x8a90a8), s));
        r.oggetti.push_back(CreaDecorazione("Lanterna", 7, Hex(0x3a3e52), s));
        r.oggetti.push_back(CreaMoneta("Anima", Hex(0x9ad8ff), 20, s));
        r.scenari.push_back(CreaScenario("Notte di luna", 2, 0.0f, 0, { Strato("colline", Hex(0x2a2a4a), 0.2f, 0.75f, 40, 11), Strato("foresta", Hex(0x14141f), 0.4f, 0.86f, 25, 12) }, true));
        r.brani.push_back(ComponiBrano("Sussurri", 141, 3));
    } else if (nome == "Giungla") {
        Umano esp; esp.maglia = Hex(0x8a8a4a); esp.capelli = Hex(0xd8a040); esp.capelliTipo = 1; esp.pelle = Hex(0xd8a080);
        r.personaggi.push_back(CreaUmano("Esploratrice", esp, s, TC::Giocatore));
        r.personaggi.push_back(CreaBestia("Dinosauro", 6, Hex(0x4aa84a), Hex(0xe8d070), s));
        r.personaggi.push_back(CreaBestia("Raptor", 6, Hex(0xc87a3a), Hex(0x5a3a20), s));
        r.personaggi.back().comportamento.velocita = 70;
        r.personaggi.push_back(CreaVolante("Pappagallo", 1, Hex(0xd8344a), Hex(0x2a7ad8), s));
        r.oggetti.push_back(CreaDecorazione("Palma della giungla", 9, Hex(0x2a8a3a), s));
        r.oggetti.push_back(CreaGemma("Idolo di giada", Hex(0x3ad88a), 150, s));
        r.caselle.push_back(CreaTerreno("Terra rossa", '#', Hex(0x3aa83a), Hex(0xa85a3a), Hex(0x7a3a2a), s, 0));
        r.scenari.push_back(CreaScenario("Giungla", 0, 0.4f, 1, { Strato("foresta", Hex(0x2a6a3a), 0.25f, 0.7f, 50, 3), Strato("foresta", Hex(0x1a4a2a), 0.5f, 0.85f, 35, 7) }, false,
                                         Hex(0x7ac8a0), Hex(0xe0f0c0)));
        r.brani.push_back(ComponiBrano("Tamburi", 151, 5));
    } else if (nome == "Oriente") {
        Umano nin; nin.maglia = Hex(0x2a2a3a); nin.capelliTipo = 5; nin.cappello = Hex(0x2a2a3a); nin.pantaloni = Hex(0x1a1a24); nin.armaTipo = 1; nin.mantello = Hex(0x8a1a2a);
        r.personaggi.push_back(CreaUmano("Ninja", nin, s, TC::Giocatore));
        Umano sam; sam.maglia = Hex(0xa82a2a); sam.elmo = Hex(0x2a2a3a); sam.armaTipo = 5; sam.robusto = 1.15f;
        r.personaggi.push_back(Nemico(CreaUmano("Samurai", sam, s, TC::Insegue), TC::Insegue, 38, 4));
        r.oggetti.push_back(CreaDecorazione("Lanterna di carta", 7, Hex(0xd8344a), s));
        r.oggetti.push_back(CreaStella("Shuriken", Hex(0xc8d0e0), 20, s));
        r.caselle.push_back(CreaTerreno("Bambu'", '-', Hex(0xb8d870), Hex(0x8ab84a), Hex(0x5a8a2a), s, 2));
        r.scenari.push_back(CreaScenario("Monte Fuji", 2, 0.28f, 0, { Strato("montagne", Hex(0xc8c0e0), 0.08f, 0.6f, 90, 4), Strato("colline", Hex(0x7a5a8a), 0.3f, 0.8f, 30, 2) }, false,
                                         Hex(0xf0a0b8), Hex(0xfff0e0)));
        r.brani.push_back(ComponiBrano("Fiori di ciliegio", 161, 2));
    } else if (nome == "Fattoria") {
        Umano con; con.maglia = Hex(0xd8344a); con.pantaloni = Hex(0x2a5ab8); con.capelliTipo = 4; con.cappello = Hex(0xe8c860);
        r.personaggi.push_back(CreaUmano("Contadino", con, s, TC::Giocatore));
        r.personaggi.push_back(CreaVolante("Gallina", 1, Hex(0xf4f0e8), Hex(0xe8e0d0), s));
        r.personaggi.back().comportamento.gravita = true; r.personaggi.back().comportamento.tipo = TC::Pattuglia;
        r.personaggi.push_back(CreaBestia("Lumaca", 5, Hex(0xb8c890), Hex(0xd88a4a), s));
        r.oggetti.push_back(CreaDecorazione("Girasole", 3, Hex(0xffd23f), s));
        r.oggetti.push_back(CreaMoneta("Uovo d'oro", Hex(0xffe060), 30, s));
        r.caselle.push_back(CreaTerreno("Campo", '#', Hex(0x7ad85a), Hex(0x9a6a3a), Hex(0x7a4a2a), s, 0));
        r.scenari.push_back(CreaScenario("Campagna", 0, 0.42f, 0, { Strato("colline", Hex(0x8ac86a), 0.2f, 0.72f, 30, 5), Strato("nuvole", WHITE, 0.08f, 0.2f, 0, 2, 4) }, false,
                                         Hex(0x5ab0f0), Hex(0xd8f0ff)));
        r.brani.push_back(ComponiBrano("Mattino in campagna", 171, 0));
    } else if (nome == "Cielo") {
        r.personaggi.push_back(CreaVolante("Uccellino", 1, Hex(0x3a8ad8), Hex(0x2a5ab8), s));
        r.personaggi.push_back(CreaVolante("Calabrone", 3, Hex(0xff9a20), Hex(0xe8f4ff), s));
        r.oggetti.push_back(CreaPiattaforma("Nuvola", Hex(0xf4f8ff), s));
        r.oggetti.push_back(CreaStella("Stella cadente", Hex(0xfff080), 30, s));
        r.caselle.push_back(CreaTerreno("Nuvola solida", '-', Hex(0xffffff), Hex(0xe0ecff), Hex(0xc8d8f0), s, 2));
        r.scenari.push_back(CreaScenario("Sopra le nuvole", 0, 0.5f, 0, { Strato("nuvole", WHITE, 0.1f, 0.3f, 0, 1, 8), Strato("nuvole", Hex(0xe0ecff), 0.3f, 0.75f, 0, 4, 14) }, false,
                                         Hex(0x3a8ae8), Hex(0xd8f0ff)));
        r.brani.push_back(ComponiBrano("Volo libero", 181, 0));
    } else if (nome == "Sotterranei") {
        r.personaggi.push_back(CreaBestia("Lumaca di caverna", 5, Hex(0x8a8aa8), Hex(0x6a5a8a), s));
        r.personaggi.push_back(CreaBestia("Ragno gigante", 1, Hex(0x2a2030), Hex(0x5aff8a), s));
        r.personaggi.back().comportamento.energia = 3;
        r.oggetti.push_back(CreaDecorazione("Cristallo blu", 6, Hex(0x4a8af0), s));
        r.oggetti.push_back(CreaDecorazione("Torcia da muro", 4, Hex(0xff8020), s));
        r.oggetti.push_back(CreaCassa("Cassa vecchia", Hex(0x7a5a30), s));
        r.caselle.push_back(CreaTerreno("Roccia di caverna", '#', Hex(0x6a6a7a), Hex(0x4a4a5a), Hex(0x3a3a48), s, 0));
        r.scenari.push_back(CreaScenario("Caverna", 3, 0.0f, 0, { Strato("montagne", Hex(0x2a2430), 0.2f, 0.4f, 60, 13) }, false, Hex(0x14101c), Hex(0x2a2430)));
        r.brani.push_back(ComponiBrano("Gocce nel buio", 191, 3));
    } else if (nome == "Carri armati") {
        r.personaggi.push_back(CreaCarro("Carro verde", Hex(0x4a8a4a), s));
        r.personaggi.back().comportamento.tipo = TC::Giocatore;
        r.personaggi.push_back(CreaCarro("Carro nemico", Hex(0x8a5a3a), s));
        r.oggetti.push_back(CreaProiettile("Granata", 0, Hex(0xffa020), s));
        r.oggetti.push_back(CreaSpine("Filo spinato", Hex(0x8a8a9a), s));
        r.scenari.push_back(CreaScenario("Campo di battaglia", 1, 0.55f, 0));
        r.brani.push_back(ComponiBrano("Avanzata", 201, 5));
    } else if (nome == "Musiche chiptune") {
        const char* nomi[] = { "Avventura", "Eroi", "Riposo", "Segreti", "Stelle", "Scontro", "Corsa", "Leggenda", "Ninna nanna", "Boss finale" };
        for (int k = 0; k < 10; k++) r.brani.push_back(ComponiBrano(nomi[k], 300 + k * 17, k % 6, k == 1 || k == 9 ? 16 : 8));
    } else if (nome == "Suoni arcade") {
        r.effetti = EffettiArcade();
    }
    if (r.brani.empty() && nome != "Suoni arcade") r.brani.push_back(ComponiBrano(nome + " - tema", i * 31, i % 6));
    Autore(r);
    return r;
}

} // namespace

int NumeroRaccolte() { return N_RACCOLTE; }
const char* NomeRaccolta(int i) { return (i >= 0 && i < N_RACCOLTE) ? RACCOLTE[i].nome : ""; }
const char* DescrizioneRaccolta(int i) { return (i >= 0 && i < N_RACCOLTE) ? RACCOLTE[i].descrizione : ""; }
int IndiceRaccolta(const std::string& nome) {
    for (int i = 0; i < N_RACCOLTE; i++) if (nome == RACCOLTE[i].nome) return i;
    return 0;
}

const Raccolta& CreaRaccolta(int i, StileGrafico s) {
    static std::map<int, Raccolta> cache;
    i = (int)Limita((float)i, 0, (float)N_RACCOLTE - 1);
    int chiave = i * 2 + (s == StileGrafico::Disegnato ? 1 : 0);
    auto it = cache.find(chiave);
    if (it != cache.end()) return it->second;
    return cache[chiave] = Genera(i, s);
}
