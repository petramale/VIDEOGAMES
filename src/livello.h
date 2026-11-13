// Mappa a tile del livello, letta da un semplice testo:
//   #  terra con erba      =  pietra          -  piattaforma attraversabile dal basso
//   ^  spine               P  partenza        F  bandiera d'arrivo
//   o  moneta   g  gemma   m  molla   c  forziere   k  lanterna (punto di controllo)
//   e  slime (nemico)      b  pipistrello (nemico)
// Una riga che inizia con ';' contiene le impostazioni del progetto (vedi progetto.h).
#pragma once
#include "comune.h"

enum class TipoOggetto { Moneta, Gemma, Molla, Forziere, Lanterna, Slime, Pipistrello, Bandiera };

struct Oggetto {
    TipoOggetto tipo;
    Vector2 pos;            // centro alla base (piedi) per nemici, centro per gli altri
    Vector2 vel{ 0, 0 };
    Vector2 origine{ 0, 0 };
    bool attivo = true;
    float t = 0;            // tempo di animazione
    float stato = 0;        // significato diverso per ogni tipo (apertura forziere, compressione molla...)
    int dir = -1;
    bool fisica = false;    // le monete uscite dal forziere cadono
};

const char* NomeOggetto(TipoOggetto t);

class Livello {
public:
    bool Carica(const std::string& testo, std::string& errore);
    char Cella(int x, int y) const;
    bool Solido(int x, int y) const { char c = Cella(x, y); return c == '#' || c == '='; }
    bool Piattaforma(int x, int y) const { return Cella(x, y) == '-'; }
    bool Spina(int x, int y) const { return Cella(x, y) == '^'; }
    int Larghezza() const { return w; }
    int Altezza() const { return h; }
    Vector2 Partenza() const { return partenza; }
    const std::vector<Oggetto>& Oggetti() const { return oggetti; }
    void Disegna(float camX, float camY, float tempo, float notte) const;

private:
    int w = 0, h = 0;
    std::vector<char> celle;
    std::vector<Oggetto> oggetti;
    Vector2 partenza{ 32, 200 };
    void DisegnaTile(int tx, int ty, float sx, float sy, float tempo, float notte) const;
};

// Livelli inclusi nello studio (funzionano anche nell'eseguibile, senza file esterni).
const char* LivelloAvventura();
const char* LivelloVetrinaOggetti();
