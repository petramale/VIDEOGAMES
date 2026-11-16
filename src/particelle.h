// Sistema di particelle: polvere, scintille, esplosioni, fuochi d'artificio.
#pragma once
#include "comune.h"

struct Particella {
    Vector2 pos, vel;
    float vita, durata, dimensione, gravita;
    Color colore;
    bool luce; // disegnata in modo additivo per un effetto "bagliore"
};

class Particelle {
public:
    void Emetti(Vector2 pos, int quante, Color colore, float velocita, float durata,
                float gravita = 200.0f, float dimensione = 2.0f, bool luce = false);
    void Polvere(Vector2 pos, int quante);
    void Scintille(Vector2 pos, Color colore);
    void Esplosione(Vector2 pos, Color colore);
    void Aggiorna(float dt);
    void Disegna() const;
    void Svuota() { lista.clear(); }
private:
    std::vector<Particella> lista;
};
