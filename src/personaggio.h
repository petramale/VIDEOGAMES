// Categoria 2: personaggi con animazioni.
// L'eroe e' disegnato dal codice pezzo per pezzo (testa, mantello, braccia, gambe),
// quindi ogni animazione e' fluida e i colori si possono cambiare a piacere.
#pragma once
#include "comune.h"

enum class StatoEroe { Fermo, Corsa, Salto, Caduta, Atterraggio, Vittoria, Colpito };

const char* NomeStato(StatoEroe s);

struct AspettoEroe {
    Color pelle    = Hex(0xf2c39b);
    Color capelli  = Hex(0x6b3b1f);
    Color maglia   = Hex(0x3f7de0);
    Color mantello = Hex(0xd8344a);
    Color pantaloni= Hex(0x2a2f4a);
    Color scarpe   = Hex(0x5a3020);
};

struct Eroe {
    Vector2 pos{ 0, 0 };    // piedi del personaggio, in pixel del mondo
    Vector2 vel{ 0, 0 };
    int dir = 1;            // 1 destra, -1 sinistra
    bool aTerra = false;
    StatoEroe stato = StatoEroe::Fermo;
    float tStato = 0;       // tempo trascorso nello stato attuale
    float fase = 0;         // fase del ciclo di corsa
    float schiaccia = 0;    // deformazione elastica (atterraggi e salti)
    float lampeggio = 0;    // invulnerabilita' dopo un colpo
    AspettoEroe aspetto;

    static constexpr float LARGHEZZA = 10;
    static constexpr float ALTEZZA = 20;

    Rectangle Box() const { return { pos.x - LARGHEZZA / 2, pos.y - ALTEZZA, LARGHEZZA, ALTEZZA }; }
    void CambiaStato(StatoEroe s);
    void AggiornaAnimazione(float dt);   // da chiamare dopo la fisica
    void Disegna(float tempo) const;
};
