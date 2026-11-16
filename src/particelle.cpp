#include "particelle.h"

void Particelle::Emetti(Vector2 pos, int quante, Color colore, float velocita, float durata,
                        float gravita, float dimensione, bool luce) {
    for (int i = 0; i < quante; i++) {
        float a = Casuale(0, 2 * PI);
        float v = Casuale(0.3f, 1.0f) * velocita;
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
        p.vel.x *= 1.0f - 1.5f * dt;
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
        DrawRectangleV({ std::round(p.pos.x - d / 2), std::round(p.pos.y - d / 2) }, { d, d }, ConAlfa(p.colore, t));
    }
}
