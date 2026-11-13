// Funzioni e costanti condivise da tutto lo studio.
#pragma once
#include "raylib.h"
#include <cmath>
#include <string>
#include <vector>

// Risoluzione virtuale "pixel art": tutto viene disegnato qui e poi ingrandito.
constexpr int VW = 480;
constexpr int VH = 270;
constexpr int TILE = 16;

inline Color Hex(unsigned int v, unsigned char a = 255) {
    return Color{ (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v, a };
}

inline float Lerp1(float a, float b, float t) { return a + (b - a) * t; }
inline float Limita(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline Color Mescola(Color a, Color b, float t) {
    t = Limita(t, 0.0f, 1.0f);
    return Color{ (unsigned char)Lerp1(a.r, b.r, t), (unsigned char)Lerp1(a.g, b.g, t),
                  (unsigned char)Lerp1(a.b, b.b, t), (unsigned char)Lerp1(a.a, b.a, t) };
}

inline Color ConAlfa(Color c, float a) { c.a = (unsigned char)(Limita(a, 0, 1) * 255); return c; }

// Rumore deterministico: stessi numeri a ogni avvio, per paesaggi sempre uguali.
inline float Hash1(int n) {
    unsigned int x = (unsigned int)n * 374761393u + 668265263u;
    x = (x ^ (x >> 13)) * 1274126177u;
    return ((x ^ (x >> 16)) & 0xFFFFFF) / 16777215.0f;
}

inline float Rumore1(float x) {
    int i = (int)std::floor(x);
    float f = x - i;
    float u = f * f * (3 - 2 * f);
    return Lerp1(Hash1(i), Hash1(i + 1), u);
}

inline float Casuale(float lo, float hi) { return lo + (hi - lo) * (GetRandomValue(0, 10000) / 10000.0f); }

// Testo con ombra, leggibile su qualsiasi sfondo.
inline void Scritta(const char* testo, int x, int y, int dim, Color c) {
    DrawText(testo, x + 1, y + 1, dim, Hex(0x000000, 160));
    DrawText(testo, x, y, dim, c);
}

inline void ScrittaCentrata(const char* testo, int cx, int y, int dim, Color c) {
    Scritta(testo, cx - MeasureText(testo, dim) / 2, y, dim, c);
}

// Testo che va a capo da solo entro una larghezza massima. Restituisce le righe usate.
inline int ScrittaACapo(const std::string& testo, int x, int y, int larghezza, int dim, Color c, int interlinea = 12) {
    std::string riga, parola;
    int n = 0;
    auto Stampa = [&]() { DrawText(riga.c_str(), x, y + n * interlinea, dim, c); n++; riga.clear(); };
    for (size_t i = 0; i <= testo.size(); i++) {
        char ch = i < testo.size() ? testo[i] : ' ';
        if (ch != ' ') { parola += ch; continue; }
        std::string prova = riga.empty() ? parola : riga + " " + parola;
        if (!riga.empty() && MeasureText(prova.c_str(), dim) > larghezza) { Stampa(); riga = parola; }
        else riga = prova;
        parola.clear();
    }
    if (!riga.empty()) Stampa();
    return n;
}

// Pannello semitrasparente con bordo, usato da menu e schede.
inline void Pannello(Rectangle r, Color fondo, Color bordo) {
    DrawRectangleRec(r, fondo);
    DrawRectangleLinesEx(r, 1, bordo);
}

// Risoluzione alta: il gioco si disegna in una texture grande VW*gScalaHD x VH*gScalaHD.
// Le coordinate restano quelle virtuali (480x270): ogni telecamera si moltiplica per gScalaHD.
extern float gScalaHD;
inline Camera2D CameraHD(Vector2 offset, Vector2 target, float zoom) {
    Camera2D c{};
    c.offset = { offset.x * gScalaHD, offset.y * gScalaHD };
    c.target = target;
    c.zoom = zoom * gScalaHD;
    return c;
}
inline void BaseHD() { BeginMode2D(CameraHD({ 0, 0 }, { 0, 0 }, 1)); }
inline void FineCameraHD() { EndMode2D(); BaseHD(); }
