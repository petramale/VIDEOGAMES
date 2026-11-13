// Categoria 1: sfondi e scenari.
// Paesaggio a parallasse generato al volo: cielo che cambia con l'ora del giorno,
// sole e luna, stelle, montagne a strati con prospettiva atmosferica, nuvole,
// foresta e lucciole. Nessuna immagine esterna: tutto e' disegnato dal codice.
#pragma once
#include "comune.h"

struct TemaSfondo {
    const char* nome;
    unsigned int montagne[4]; // dal piu' lontano al piu' vicino
    unsigned int alberi;
    float seme;
};

class Sfondo {
public:
    float ora = 0.35f;        // 0 = mezzanotte, 0.25 = alba, 0.5 = mezzogiorno, 0.75 = tramonto
    int tema = 0;             // indice in Temi()
    static const std::vector<TemaSfondo>& Temi();

    void Aggiorna(float dt) { tempo += dt; }
    void Disegna(float camX) const;
    Color ColoreOrizzonte() const;
    float Notte() const;      // 0 di giorno, 1 a notte fonda

private:
    float tempo = 0;
    void Cielo(Color& alto, Color& basso) const;
    void Montagne(float camX, int strato, Color colore, float fattore, float base, float ampiezza, float scala) const;
    void Nuvole(float camX) const;
    void Foresta(float camX, Color colore) const;
};
