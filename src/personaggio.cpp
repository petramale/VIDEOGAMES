#include "personaggio.h"

const char* NomeStato(StatoEroe s) {
    switch (s) {
        case StatoEroe::Fermo: return "FERMO";
        case StatoEroe::Corsa: return "CORSA";
        case StatoEroe::Salto: return "SALTO";
        case StatoEroe::Caduta: return "CADUTA";
        case StatoEroe::Atterraggio: return "ATTERRAGGIO";
        case StatoEroe::Vittoria: return "VITTORIA";
        case StatoEroe::Colpito: return "COLPITO";
    }
    return "";
}

void Eroe::CambiaStato(StatoEroe s) {
    if (s == stato) return;
    if (s == StatoEroe::Atterraggio) schiaccia = 0.35f;
    if (s == StatoEroe::Salto) schiaccia = -0.3f;
    stato = s;
    tStato = 0;
}

void Eroe::AggiornaAnimazione(float dt) {
    tStato += dt;
    if (lampeggio > 0) lampeggio -= dt;
    schiaccia += (0 - schiaccia) * Limita(dt * 12, 0, 1);
    float v = std::fabs(vel.x);
    fase += dt * (6 + v * 0.09f);
    if (stato == StatoEroe::Atterraggio && tStato > 0.12f) CambiaStato(v > 5 ? StatoEroe::Corsa : StatoEroe::Fermo);
}

static void Rett(float x, float y, float w, float h, Color c) {
    DrawRectangle((int)std::round(x), (int)std::round(y), (int)std::round(w), (int)std::round(h), c);
}

void Eroe::Disegna(float tempo) const {
    if (lampeggio > 0 && ((int)(lampeggio * 20)) % 2 == 0) return;
    const AspettoEroe& a = aspetto;
    Color contorno = Hex(0x1a1426);

    // Deformazione elastica: largo e basso quando atterra, stretto e alto quando salta.
    float sx = 1 + schiaccia * 0.6f, sy = 1 - schiaccia * 0.5f;
    float respiro = 0, gambaA = 0, gambaB = 0, braccio = 0, alzaBraccia = 0;

    switch (stato) {
        case StatoEroe::Fermo: respiro = std::sin(tempo * 3) * 0.6f; braccio = std::sin(tempo * 3) * 0.5f; break;
        case StatoEroe::Corsa: gambaA = std::sin(fase) * 4; gambaB = -gambaA; braccio = -std::sin(fase) * 3;
                               respiro = -std::fabs(std::sin(fase)) * 1.5f; break;
        case StatoEroe::Salto: gambaA = -3; gambaB = 2; braccio = -3; alzaBraccia = 1; break;
        case StatoEroe::Caduta: gambaA = 2; gambaB = -1; braccio = 2; alzaBraccia = 1; break;
        case StatoEroe::Atterraggio: respiro = 2; break;
        case StatoEroe::Vittoria: respiro = -std::fabs(std::sin(tempo * 8)) * 4; alzaBraccia = 2; break;
        case StatoEroe::Colpito: braccio = std::sin(tempo * 30) * 3; break;
    }

    float px = pos.x, py = pos.y;
    float d = (float)dir;
    auto X = [&](float lx) { return px + lx * d * sx; };   // coordinate locali -> mondo
    auto Y = [&](float ly) { return py + (ly + respiro * (ly < -8 ? 1 : 0)) * sy; };

    // Mantello: ondeggia con il movimento.
    float vento = Limita(std::fabs(vel.x) / 120.0f, 0, 1) + (stato == StatoEroe::Caduta ? 0.6f : 0);
    for (int i = 0; i < 6; i++) {
        float k = i / 5.0f;
        float onda = std::sin(tempo * 9 - i * 0.9f) * (1 + vento * 2) * k;
        float lx = -2 - k * (3 + vento * 7);
        float ly = -16 + k * (10 - vento * 6) + onda;
        Rett(X(lx) - 2, Y(ly) - 1, 4 - k, 4, Mescola(a.mantello, Hex(0x5a0f20), k * 0.6f));
    }

    // Gambe (dietro e davanti).
    Rett(X(gambaB) - 2, Y(-6), 3, 6, Mescola(a.pantaloni, BLACK, 0.3f));
    Rett(X(gambaB) - 2, Y(-1), 4, 1, a.scarpe);
    // Busto.
    Rett(std::min(X(-4), X(4)), Y(-14), 8 * sx, 9 * sy, contorno);
    Rett(std::min(X(-3), X(3)), Y(-13), 6 * sx, 7 * sy, a.maglia);
    Rett(std::min(X(-3), X(3)), Y(-8), 6 * sx, 1, Hex(0xf0c040)); // cintura
    Rett(X(gambaA) - 2, Y(-6), 3, 6, a.pantaloni);
    Rett(X(gambaA) - 1 + d, Y(-1), 4, 1, a.scarpe);

    // Braccia.
    float by = -12 - alzaBraccia * 3;
    Rett(X(-3 - braccio * 0.5f) - 1, Y(by), 2, 5, Mescola(a.maglia, BLACK, 0.3f));
    Rett(X(3 + braccio * 0.5f) - 1, Y(by), 2, 5, a.maglia);
    Rett(X(3 + braccio * 0.5f) - 1, Y(by + 5), 2, 1, a.pelle);

    // Testa.
    float hx = std::min(X(-4), X(4)), hy = Y(-22);
    Rett(hx - 1, hy - 1, 10 * sx, 9 * sy, contorno);
    Rett(hx, hy, 8 * sx, 7 * sy, a.pelle);
    Rett(hx, hy - 1, 8 * sx, 3, a.capelli);
    Rett(d > 0 ? hx : hx + 6 * sx, hy + 1, 2, 3, a.capelli);
    // Occhi con battito di ciglia.
    bool chiusi = std::fmod(tempo, 3.2f) < 0.12f || stato == StatoEroe::Colpito;
    float ex = X(2);
    if (chiusi) Rett(ex - 1, hy + 4, 3, 1, contorno);
    else { Rett(ex - 1, hy + 3, 2, 2, WHITE); Rett(ex + (d > 0 ? 0 : -1), hy + 3, 1, 2, contorno); }
    if (stato == StatoEroe::Vittoria) Rett(X(1) - 1, hy + 5, 3, 1, Hex(0x8a2a2a));
}
