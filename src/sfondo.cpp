#include "sfondo.h"
#include "immagini.h"

const std::vector<TemaSfondo>& Sfondo::Temi() {
    static const std::vector<TemaSfondo> temi = {
        { "Valle verde",     { 0x8fa6c9, 0x6b86b3, 0x3f6b5a, 0x2d4f3c }, 0x1e3a2b, 1.0f },
        { "Deserto rosso",   { 0xd9a38a, 0xc47a5a, 0x9c4f3a, 0x6e3428 }, 0x4a2a20, 7.0f },
        { "Ghiacci eterni",  { 0xc9dcf0, 0xa3bfe0, 0x7d9ccc, 0x5a77a8 }, 0x2c3e66, 3.0f },
        { "Terre vulcaniche",{ 0x7a5a6e, 0x5c3f55, 0x3d2a3d, 0x2a1c2a }, 0x150d15, 11.0f },
    };
    return temi;
}

float Sfondo::Notte() const {
    // Coseno dell'ora: massimo a mezzanotte, zero di giorno.
    float c = std::cos(ora * 2 * PI);
    return Limita(c * 1.6f + 0.2f, 0, 1);
}

void Sfondo::Cielo(Color& alto, Color& basso) const {
    if (cieloPersonale) {
        float n = Notte() * 0.7f;
        alto = Mescola(cieloAlto, Hex(0x05081c), n);
        basso = Mescola(cieloBasso, Hex(0x1b1f4d), n);
        return;
    }
    struct Tappa { float t; unsigned int a, b; };
    static const Tappa tappe[] = {
        { 0.00f, 0x05081c, 0x1b1f4d }, { 0.20f, 0x1a1f5c, 0x5b3d7a }, { 0.27f, 0x3a4fa0, 0xff9a6b },
        { 0.35f, 0x3f8ff0, 0xb9e3ff }, { 0.65f, 0x3f8ff0, 0xb9e3ff }, { 0.73f, 0x4a3a8c, 0xff6a4d },
        { 0.80f, 0x1e1a50, 0x6b2f6b }, { 1.00f, 0x05081c, 0x1b1f4d } };
    for (int i = 0; i < 7; i++) {
        if (ora >= tappe[i].t && ora <= tappe[i + 1].t) {
            float k = (ora - tappe[i].t) / (tappe[i + 1].t - tappe[i].t);
            alto = Mescola(Hex(tappe[i].a), Hex(tappe[i + 1].a), k);
            basso = Mescola(Hex(tappe[i].b), Hex(tappe[i + 1].b), k);
            return;
        }
    }
    alto = Hex(0x05081c); basso = Hex(0x1b1f4d);
}

Color Sfondo::ColoreOrizzonte() const { Color a, b; Cielo(a, b); return b; }

void Sfondo::Montagne(float camX, int strato, Color colore, float fattore, float base, float ampiezza, float scala) const {
    const TemaSfondo& t = Temi()[tema];
    float off = camX * fattore;
    for (int x = 0; x < VW; x++) {
        float wx = (x + off) / scala + t.seme * 37 + strato * 100;
        float h = Rumore1(wx) * 0.6f + Rumore1(wx * 2.3f) * 0.28f + Rumore1(wx * 5.1f) * 0.12f;
        int y = (int)(base - h * ampiezza);
        DrawRectangle(x, y, 1, VH - y, colore);
        // Riflesso della luce sulle creste.
        DrawRectangle(x, y, 1, 1, Mescola(colore, WHITE, 0.18f));
    }
}

void Sfondo::Nuvole(float camX) const {
    Color c = Mescola(Hex(0xffffff), ColoreOrizzonte(), 0.35f + Notte() * 0.4f);
    for (int i = 0; i < 9; i++) {
        float velocita = 4 + Hash1(i * 7) * 8;
        float x = std::fmod(Hash1(i) * 900 + tempo * velocita - camX * 0.08f, 640.0f);
        if (x < 0) x += 640;
        x -= 80;
        float y = 20 + Hash1(i * 3) * 70;
        float s = 0.7f + Hash1(i * 5) * 0.8f;
        Color ombra = Mescola(c, Hex(0x5a6a9a), 0.25f);
        for (int k = 0; k < 5; k++) {
            float dx = (k - 2) * 9 * s, dy = -std::fabs((float)k - 2) * -2 * s;
            DrawCircleV({ x + dx, y + dy + 2 }, (9 - std::fabs((float)k - 2) * 2) * s, ombra);
        }
        for (int k = 0; k < 5; k++) {
            float dx = (k - 2) * 9 * s, dy = -std::fabs((float)k - 2) * -2 * s;
            DrawCircleV({ x + dx, y + dy }, (9 - std::fabs((float)k - 2) * 2) * s, c);
        }
    }
}

void Sfondo::Foresta(float camX, Color colore, float dy) const {
    float off = camX * 0.55f;
    for (int i = -2; i < 40; i++) {
        int indice = i + (int)std::floor(off / 14);
        float x = indice * 14 - off + Hash1(indice) * 6;
        float h = 26 + Hash1(indice * 13) * 22;
        float base = VH - 34 + dy;
        float oscilla = std::sin(tempo * 1.3f + indice) * 1.2f;
        DrawTriangle({ x + oscilla, base - h }, { x - 8, base }, { x + 8, base }, colore);
        DrawTriangle({ x + oscilla * 0.6f, base - h - 8 }, { x - 6, base - h * 0.5f }, { x + 6, base - h * 0.5f }, colore);
        DrawRectangle((int)x - 1, (int)base, 3, 6, colore);
    }
    DrawRectangle(0, (int)(VH - 30 + dy), VW, 30 + (int)std::fabs(dy), colore);
}

void Sfondo::Astri() const {
    float notte = Notte();
    Color alto, basso;
    Cielo(alto, basso);
    // Stelle che brillano.
    if (notte > 0.01f) {
        for (int i = 0; i < 140; i++) {
            float x = std::fmod(Hash1(i * 11) * VW + VW * 4, (float)VW);
            float y = Hash1(i * 17) * VH * 0.65f;
            float b = 0.5f + 0.5f * std::sin(tempo * (1.5f + Hash1(i) * 3) + i);
            DrawPixelV({ x, y }, ConAlfa(WHITE, notte * (0.35f + 0.65f * b)));
            if (i % 23 == 0) {
                DrawPixelV({ x - 1, y }, ConAlfa(WHITE, notte * b * 0.5f));
                DrawPixelV({ x + 1, y }, ConAlfa(WHITE, notte * b * 0.5f));
                DrawPixelV({ x, y - 1 }, ConAlfa(WHITE, notte * b * 0.5f));
                DrawPixelV({ x, y + 1 }, ConAlfa(WHITE, notte * b * 0.5f));
            }
        }
    }
    // Sole e luna percorrono un arco nel cielo.
    float ang = (ora - 0.25f) * 2 * PI;
    Vector2 sole = { VW * 0.5f - std::cos(ang) * VW * 0.42f, VH * 0.75f - std::sin(ang) * VH * 0.62f };
    Vector2 luna = { VW * 0.5f + std::cos(ang) * VW * 0.42f, VH * 0.75f + std::sin(ang) * VH * 0.62f };
    BeginBlendMode(BLEND_ADDITIVE);
    for (int r = 5; r >= 1; r--) DrawCircleV(sole, 10.0f + r * 7, ConAlfa(Hex(0xffc070), 0.06f));
    DrawCircleV(luna, 22, ConAlfa(Hex(0x9fb4ff), 0.08f * notte));
    EndBlendMode();
    DrawCircleV(sole, 11, Hex(0xfff2c0));
    DrawCircleV(luna, 8, Hex(0xe8ecff));
    DrawCircleV({ luna.x + 3, luna.y - 2 }, 7, alto); // falce di luna
}

void Sfondo::Disegna(float camX, float dy) const {
    const TemaSfondo& t = Temi()[tema];
    Color alto, basso;
    Cielo(alto, basso);
    DrawRectangleGradientV(0, 0, VW, VH, alto, basso);
    float notte = Notte();
    Astri();

    if (generato) {
        // Strati di montagne: piu' sono lontani, piu' si confondono con il cielo (e meno si spostano).
        float nebbia[4] = { 0.62f, 0.45f, 0.25f, 0.12f };
        float fattori[4] = { 0.05f, 0.12f, 0.22f, 0.35f };
        for (int s = 0; s < 4; s++) {
            Color c = Mescola(Hex(t.montagne[s]), basso, nebbia[s]);
            c = Mescola(c, Hex(0x0a0d24), notte * 0.55f);
            if (s == 1) Nuvole(camX);
            Montagne(camX, s, c, fattori[s], VH * (0.55f + s * 0.09f) + dy * (fattori[s] + 0.1f), 70.0f - s * 10, 70.0f + s * 10);
        }
        Color albero = Mescola(Hex(t.alberi), Hex(0x05060f), notte * 0.6f);
        Foresta(camX, albero, dy * 0.55f);
        // Foschia vicino al suolo.
        DrawRectangleGradientV(0, (int)(VH - 90 + dy * 0.55f), VW, 60, ConAlfa(basso, 0), ConAlfa(basso, 0.22f));
    }

    for (auto& s : strati) if (!s.primoPiano) DisegnaStrato(s, camX, dy);

    // Lucciole al tramonto e di notte.
    if (generato && notte > 0.2f) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 26; i++) {
            float x = std::fmod(Hash1(i * 29) * VW + std::sin(tempo * 0.7f + i) * 18 - camX * 0.6f + VW * 8, (float)VW);
            float y = VH - 40 - Hash1(i * 31) * 50 + std::cos(tempo * 0.9f + i * 2) * 8 + dy * 0.55f;
            float b = 0.5f + 0.5f * std::sin(tempo * 3 + i);
            DrawCircleV({ x, y }, 2.5f, ConAlfa(Hex(0xc8ff6a), 0.35f * b * notte));
            DrawPixelV({ x, y }, ConAlfa(Hex(0xf0ffb0), b * notte));
        }
        EndBlendMode();
    }
}

void Sfondo::DisegnaPrimoPiano(float camX, float dy) const {
    for (auto& s : strati) if (s.primoPiano) DisegnaStrato(s, camX, dy);
}

// Uno strato personalizzato. La posizione orizzontale si sposta di camX * parallasseX,
// quella verticale scende di dy * parallasseY quando la telecamera sale.
void Sfondo::DisegnaStrato(const StratoSfondo& s, float camX, float dy) const {
    float notte = Notte();
    Color c = Mescola(s.colore, Hex(0x0a0d24), notte * 0.5f);
    float off = camX * s.parallasseX - tempo * s.vento;
    float base = VH * s.altezza + dy * s.parallasseY;
    float sem = s.seme * 53.0f;
    if (s.tipo == "fascia") {
        DrawRectangle(0, (int)base, VW, VH - (int)base + (int)std::fabs(dy) + 1, c);
    } else if (s.tipo == "montagne" || s.tipo == "colline") {
        bool colline = s.tipo == "colline";
        float scala = colline ? 150.0f : 70.0f;
        for (int x = 0; x < VW; x++) {
            float wx = (x + off) / scala + sem;
            float h = colline ? Rumore1(wx) * 0.8f + Rumore1(wx * 1.7f) * 0.2f
                              : Rumore1(wx) * 0.6f + Rumore1(wx * 2.3f) * 0.28f + Rumore1(wx * 5.1f) * 0.12f;
            int y = (int)(base - h * s.ampiezza);
            DrawRectangle(x, y, 1, VH - y + (int)std::fabs(dy) + 1, c);
            DrawRectangle(x, y, 1, 1, Mescola(c, WHITE, colline ? 0.1f : 0.18f));
        }
    } else if (s.tipo == "foresta") {
        float passo = 14;
        for (int i = -2; i < VW / (int)passo + 4; i++) {
            int indice = i + (int)std::floor(off / passo);
            float x = indice * passo - off + Hash1(indice + (int)sem) * 6;
            float h = s.ampiezza * (0.55f + Hash1(indice * 13 + (int)sem) * 0.45f);
            float oscilla = std::sin(tempo * 1.3f + indice) * 1.2f;
            DrawTriangle({ x + oscilla, base - h }, { x - 8, base }, { x + 8, base }, c);
            DrawTriangle({ x + oscilla * 0.6f, base - h - 8 }, { x - 6, base - h * 0.5f }, { x + 6, base - h * 0.5f }, c);
        }
        DrawRectangle(0, (int)base, VW, VH - (int)base + (int)std::fabs(dy) + 1, c);
    } else if (s.tipo == "nuvole") {
        for (int i = 0; i < 9; i++) {
            float x = std::fmod(Hash1(i + (int)sem) * 900 - off * 0.5f + 6400, 640.0f) - 80;
            float y = base + (Hash1(i * 3 + (int)sem) - 0.5f) * s.ampiezza;
            float sc = 0.7f + Hash1(i * 5 + (int)sem) * 0.8f;
            for (int k = 0; k < 5; k++) {
                float ddx = (k - 2) * 9 * sc, ddy = std::fabs((float)k - 2) * 2 * sc;
                DrawCircleV({ x + ddx, y + ddy }, (9 - std::fabs((float)k - 2) * 2) * sc, c);
            }
        }
    } else if (s.tipo == "stelle") {
        for (int i = 0; i < 120; i++) {
            float x = std::fmod(Hash1(i * 11 + (int)sem) * VW * 3 - off + VW * 30, (float)VW);
            float y = Hash1(i * 17 + (int)sem) * VH * s.altezza + dy * s.parallasseY;
            float b = 0.5f + 0.5f * std::sin(tempo * (1.5f + Hash1(i) * 3) + i);
            DrawPixelV({ x, y }, ConAlfa(c, 0.4f + 0.6f * b));
        }
    } else if (s.tipo == "immagine") {
        Texture2D tex = Immagine(s.immagine);
        if (tex.id == 0) return;
        float w = tex.width * s.scala, h = tex.height * s.scala;
        float top = base - h;   // "altezza" indica dove sta il bordo basso dell'immagine
        Color tinta = Mescola(WHITE, Hex(0x0a0d24), notte * 0.5f);
        if (s.ripeti) {
            float x0 = -std::fmod(off, w);
            if (x0 > 0) x0 -= w;
            for (float x = x0; x < VW; x += w)
                DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, { x, top, w, h }, { 0, 0 }, 0, tinta);
        } else {
            DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, { -off, top, w, h }, { 0, 0 }, 0, tinta);
        }
    }
}
