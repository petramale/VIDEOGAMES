// Sistema di particelle: polvere, scintille, esplosioni, fuochi d'artificio.
#pragma once
#include "comune.h"

struct Particella {
    Vector2 pos, vel;
    float vita, durata, dimensione, gravita;
    Color colore;
    bool luce; // disegnata in modo additivo per un effetto "bagliore"
    int forma = 0;   // 0 quadratino, 1 tondo morbido, 2 stellina, 3 cuore, 4 foglia, 5 bolla
    float attrito = 1.5f;
};

class Particelle {
public:
    void Emetti(Vector2 pos, int quante, Color colore, float velocita, float durata,
                float gravita = 200.0f, float dimensione = 2.0f, bool luce = false);
    void Polvere(Vector2 pos, int quante);
    void Scintille(Vector2 pos, Color colore);
    void Esplosione(Vector2 pos, Color colore);
    void Aggiungi(const Particella& p) { if (lista.size() < 3000) lista.push_back(p); }
    // Gli effetti speciali della Logica e le scie (vedi NomiEffettoSpeciale e NomiScia): scala 1 = normale.
    void Effetto(int tipo, Vector2 pos, Color colore, float scala = 1);
    void Scia(int tipo, Vector2 pos, Color colore, float verso);
    void Aggiorna(float dt);
    void Disegna() const;
    void Svuota() { lista.clear(); }
private:
    std::vector<Particella> lista;
};
