#include "particelle.h"

void Particelle::Emetti(Vector2 pos, int quante, Color colore, float velocita, float durata,
                        float gravita, float dimensione, bool luce) {
    for (int i = 0; i < quante; i++) {
        float a = Casuale(0, 2 * PI);
        float v = Casuale(0.3f, 1.0f) * velocita;
        if (lista.size() >= 3000) return;
        lista.push_back({ pos, { std::cos(a) * v, std::sin(a) * v }, 0, durata * Casuale(0.6f, 1.2f),
                          dimensione, gravita, colore, luce });
    }
}

void Particelle::Polvere(Vector2 pos, int quante) {
    for (int i = 0; i < quante; i++) {
        float dir = (i % 2 == 0) ? -1.0f : 1.0f;
        lista.push_back({ pos, { dir * Casuale(15, 45), Casuale(-25, -5) }, 0, Casuale(0.25f, 0.5f),
                          Casuale(1.5f, 3.0f), 40, Hex(0xe8dcc0, 200), false });
    }
}

void Particelle::Scintille(Vector2 pos, Color colore) {
    Emetti(pos, 10, colore, 70, 0.5f, 60, 1.5f, true);
    Emetti(pos, 4, WHITE, 40, 0.3f, 0, 1.0f, true);
}

void Particelle::Esplosione(Vector2 pos, Color colore) {
    Emetti(pos, 28, colore, 140, 0.8f, 160, 2.5f, true);
    Emetti(pos, 12, Hex(0xfff3b0), 90, 0.5f, 80, 1.5f, true);
}

void Particelle::Aggiorna(float dt) {
    for (auto& p : lista) {
        p.vita += dt;
        p.vel.y += p.gravita * dt;
        p.vel.x *= std::max(0.0f, 1.0f - p.attrito * dt);
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
    for (size_t i = 0; i < lista.size();) {
        if (lista[i].vita >= lista[i].durata) { lista[i] = lista.back(); lista.pop_back(); }
        else i++;
    }
}

void Particelle::Disegna() const {
    for (const auto& p : lista) {
        float t = 1.0f - p.vita / p.durata;
        float d = p.dimensione * (0.4f + 0.6f * t);
        if (p.luce) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV(p.pos, d * 2.2f, ConAlfa(p.colore, 0.25f * t));
            EndBlendMode();
        }
        Color c = ConAlfa(p.colore, t);
        switch (p.forma) {
            case 1: DrawCircleGradient((int)p.pos.x, (int)p.pos.y, d * 1.6f, ConAlfa(p.colore, 0.8f * t), ConAlfa(p.colore, 0)); break;
            case 2: {   // stellina a quattro punte
                float g = p.vita * 4;
                for (int k = 0; k < 2; k++) {
                    float a = g + k * PI / 2;
                    Vector2 v = { std::cos(a) * d * 1.8f, std::sin(a) * d * 1.8f };
                    DrawLineEx({ p.pos.x - v.x, p.pos.y - v.y }, { p.pos.x + v.x, p.pos.y + v.y }, std::max(1.0f, d * 0.45f), c);
                }
                DrawCircleV(p.pos, d * 0.5f, ConAlfa(WHITE, t));
                break;
            }
            case 3: {   // cuore
                float r = d * 0.55f;
                DrawCircleV({ p.pos.x - r * 0.8f, p.pos.y - r * 0.3f }, r, c);
                DrawCircleV({ p.pos.x + r * 0.8f, p.pos.y - r * 0.3f }, r, c);
                DrawTriangle({ p.pos.x - r * 1.75f, p.pos.y }, { p.pos.x, p.pos.y + r * 2.0f }, { p.pos.x + r * 1.75f, p.pos.y }, c);
                break;
            }
            case 4: DrawEllipse((int)p.pos.x, (int)p.pos.y, d * 1.2f, d * 0.6f, c); break;   // foglia
            case 5: DrawCircleLines((int)p.pos.x, (int)p.pos.y, d, c); DrawCircleV({ p.pos.x - d * 0.35f, p.pos.y - d * 0.35f }, std::max(0.6f, d * 0.2f), ConAlfa(WHITE, t)); break;
            default: DrawRectangleV({ std::round(p.pos.x - d / 2), std::round(p.pos.y - d / 2) }, { d, d }, c); break;
        }
    }
}

namespace {
Particella P(Vector2 pos, Vector2 vel, float durata, float dim, float gravita, Color c, bool luce, int forma, float attrito = 1.5f) {
    Particella p{ pos, vel, 0, durata, dim, gravita, c, luce };
    p.forma = forma; p.attrito = attrito;
    return p;
}
Vector2 Dir(float a, float v) { return { std::cos(a) * v, std::sin(a) * v }; }
}

// Effetti speciali (NomiEffettoSpeciale): scintille, esplosione, grande esplosione, fumo, stelle, cuori, magia,
// fuochi d'artificio, bolle, polvere, fulmine, teletrasporto, anello di luce, pioggia d'oro.
void Particelle::Effetto(int tipo, Vector2 pos, Color col, float k) {
    k = std::max(0.2f, k);
    int n = 0;
    switch (tipo) {
        case 0: Scintille(pos, col); Emetti(pos, (int)(10 * k), col, 110 * k, 0.6f, 120, 1.5f, true); break;
        case 1: Esplosione(pos, col); Emetti(pos, (int)(8 * k), Hex(0x5a5048, 180), 40 * k, 1.0f, -30, 4, false); break;
        case 2:
            Emetti(pos, (int)(60 * k), col, 260 * k, 1.1f, 180, 3.5f, true);
            Emetti(pos, (int)(30 * k), Hex(0xfff3b0), 170 * k, 0.7f, 90, 2.5f, true);
            for (int i = 0; i < (int)(18 * k); i++) Aggiungi(P(pos, Dir(Casuale(0, 2 * PI), Casuale(10, 60) * k), Casuale(1.2f, 2.0f), Casuale(5, 9) * k, -25, Hex(0x5a5048, 170), false, 1, 0.8f));
            break;
        case 3: for (int i = 0; i < (int)(16 * k); i++) Aggiungi(P({ pos.x + Casuale(-8, 8), pos.y }, { Casuale(-15, 15), Casuale(-40, -15) * k }, Casuale(1.2f, 2.2f), Casuale(4, 8) * k, -10, ConAlfa(col, 0.6f), false, 1, 0.6f)); break;
        case 4: for (int i = 0; i < (int)(14 * k); i++) Aggiungi(P(pos, Dir(Casuale(0, 2 * PI), Casuale(30, 110) * k), Casuale(0.6f, 1.2f), Casuale(2, 3.5f), 40, col, true, 2)); break;
        case 5: for (int i = 0; i < (int)(10 * k); i++) Aggiungi(P({ pos.x + Casuale(-10, 10), pos.y }, { Casuale(-25, 25), Casuale(-70, -35) }, Casuale(0.9f, 1.5f), Casuale(3, 5), -10, col, false, 3, 1)); break;
        case 6:   // magia: spirale di stelline e luci
            for (int i = 0; i < (int)(24 * k); i++) { float a = i * 0.55f; Aggiungi(P({ pos.x + std::cos(a) * 6, pos.y + std::sin(a) * 6 }, Dir(a + 1.2f, 55 * k), Casuale(0.7f, 1.3f), Casuale(1.5f, 3), -20, Mescola(col, WHITE, Casuale(0, 0.5f)), true, i % 3 ? 1 : 2, 1.0f)); }
            break;
        case 7: {   // fuochi d'artificio: un razzo che sale e scoppia (lo scoppio e' subito, piu' in alto)
            Color cc[] = { col, Hex(0xff5c8a), Hex(0x5ef0ff), Hex(0xffd23f), Hex(0x8cff6a) };
            for (int b = 0; b < (int)(3 * k); b++) {
                Vector2 c = { pos.x + Casuale(-60, 60), pos.y - Casuale(50, 110) };
                Color cb = cc[GetRandomValue(0, 4)];
                for (int i = 0; i < 26; i++) Aggiungi(P(c, Dir(i * 2 * PI / 26, Casuale(70, 110)), Casuale(0.8f, 1.3f), 2, 60, cb, true, i % 4 ? 0 : 2, 1.2f));
            }
            break;
        }
        case 8: for (int i = 0; i < (int)(12 * k); i++) Aggiungi(P({ pos.x + Casuale(-12, 12), pos.y + Casuale(-6, 6) }, { Casuale(-10, 10), Casuale(-50, -20) }, Casuale(1.0f, 2.0f), Casuale(2, 5), -15, Mescola(col, WHITE, 0.4f), false, 5, 2)); break;
        case 9: Polvere(pos, (int)(14 * k)); break;
        case 10: {   // fulmine: una linea spezzata di scintille dall'alto
            Vector2 a = { pos.x + Casuale(-30, 30), pos.y - 160 * k };
            for (int s = 0; s < 10; s++) {
                Vector2 b = { pos.x + Casuale(-14, 14) * (1 - s / 10.0f), a.y + (pos.y - a.y) / (10 - s) };
                for (int i = 0; i <= 6; i++) Aggiungi(P({ a.x + (b.x - a.x) * i / 6, a.y + (b.y - a.y) * i / 6 }, { 0, 0 }, 0.25f, 2.5f, 0, Mescola(col, WHITE, 0.5f), true, 1, 0));
                a = b;
            }
            Scintille(pos, col);
            break;
        }
        case 11:   // teletrasporto: colonna di luci che sale
            for (int i = 0; i < (int)(30 * k); i++) Aggiungi(P({ pos.x + Casuale(-10, 10), pos.y + Casuale(-20, 10) }, { 0, Casuale(-120, -40) }, Casuale(0.5f, 1.0f), Casuale(1.5f, 3), 0, Mescola(col, WHITE, Casuale(0, 0.6f)), true, i % 4 ? 1 : 2, 0));
            break;
        case 12:   // anello di luce che si allarga
            n = (int)(36 * k);
            for (int i = 0; i < n; i++) Aggiungi(P(pos, Dir(i * 2 * PI / n, 120 * k), 0.6f, 2.5f, 0, col, true, 1, 0.5f));
            break;
        case 13:   // pioggia d'oro: monetine che cadono dall'alto
            for (int i = 0; i < (int)(20 * k); i++) Aggiungi(P({ pos.x + Casuale(-60, 60), pos.y - Casuale(60, 120) }, { Casuale(-10, 10), Casuale(0, 30) }, Casuale(1.0f, 1.6f), Casuale(2, 3), 160, Mescola(col, Hex(0xffd23f), 0.6f), true, i % 3 ? 1 : 2, 1));
            break;
        default: Scintille(pos, col); break;
    }
}

// Scie di personaggi e oggetti (NomiScia): 1 scia di luce, 2 scintille, 3 fumo, 4 stelline, 5 bolle, 6 fiammelle, 7 foglie, 8 neve, 9 cuori.
void Particelle::Scia(int tipo, Vector2 pos, Color c, float verso) {
    switch (tipo) {
        case 1: Aggiungi(P(pos, { -verso * 5, 0 }, 0.45f, 3.5f, 0, c, true, 1, 0)); break;
        case 2: Aggiungi(P(pos, Dir(Casuale(0, 2 * PI), Casuale(10, 40)), Casuale(0.3f, 0.6f), 1.5f, 60, c, true, 0)); break;
        case 3: Aggiungi(P(pos, { Casuale(-6, 6), Casuale(-30, -15) }, Casuale(1.0f, 1.8f), Casuale(4, 7), -8, ConAlfa(c, 0.5f), false, 1, 0.6f)); break;
        case 4: Aggiungi(P({ pos.x + Casuale(-6, 6), pos.y + Casuale(-6, 6) }, { Casuale(-8, 8), Casuale(-12, 4) }, Casuale(0.5f, 0.9f), Casuale(1.8f, 2.8f), 10, c, true, 2)); break;
        case 5: Aggiungi(P({ pos.x + Casuale(-5, 5), pos.y }, { Casuale(-6, 6), Casuale(-35, -20) }, Casuale(0.8f, 1.4f), Casuale(1.5f, 3), -10, Mescola(c, WHITE, 0.4f), false, 5, 2)); break;
        case 6: Aggiungi(P({ pos.x + Casuale(-3, 3), pos.y }, { Casuale(-5, 5), Casuale(-45, -25) }, Casuale(0.3f, 0.6f), Casuale(2, 3.5f), -20, Mescola(c, Hex(0xffe070), Casuale(0, 0.6f)), true, 1, 1)); break;
        case 7: Aggiungi(P({ pos.x + Casuale(-6, 6), pos.y }, { Casuale(-20, 20), Casuale(-10, 5) }, Casuale(1.0f, 1.6f), Casuale(2, 3), 25, c, false, 4, 1)); break;
        case 8: Aggiungi(P({ pos.x + Casuale(-8, 8), pos.y }, { Casuale(-10, 10), Casuale(-5, 5) }, Casuale(0.8f, 1.4f), Casuale(1.2f, 2.2f), 20, Mescola(c, WHITE, 0.7f), true, 1, 1)); break;
        case 9: Aggiungi(P({ pos.x + Casuale(-5, 5), pos.y }, { Casuale(-8, 8), Casuale(-30, -15) }, Casuale(0.7f, 1.1f), Casuale(2.5f, 3.5f), -5, c, false, 3, 1)); break;
        default: break;
    }
}
