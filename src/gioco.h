// Categorie 3 e 5: oggetti interattivi e logica di gioco.
// Fisica a piattaforme (gravita', salto variabile, "coyote time"), collisioni con
// tile e oggetti, nemici con intelligenza semplice, telecamera che scorre con
// anticipo, punti di controllo, vittoria e sconfitta.
#pragma once
#include "comune.h"
#include "sfondo.h"
#include "personaggio.h"
#include "livello.h"
#include "particelle.h"

struct Comandi {
    bool sinistra = false, destra = false, salto = false, saltoPremuto = false;
    static Comandi DaTastiera();
};

class Gioco {
public:
    bool Carica(const std::string& testoLivello, std::string& errore);
    void Aggiorna(float dt, const Comandi& c);
    void Disegna();

    Sfondo sfondo;
    Eroe eroe;
    bool mostraEtichette = false;   // nomi degli oggetti (vetrina della categoria 3)
    bool mostraCollisioni = false;  // riquadri di collisione (categoria 5)
    bool cicloGiorno = true;        // l'ora del giorno avanza lentamente
    bool mostraHud = true;
    std::string titolo = "La valle delle gemme";

    int monete = 0, gemme = 0, vite = 3;
    float tempo = 0;
    bool vinto = false, finito = false;

    float CameraX() const { return camX; }

private:
    void Fisica(float dt, const Comandi& c);
    void MuoviConCollisioni(Vector2& pos, Vector2& vel, float w, float hgt, float dt, bool& aTerra, bool attraversa);
    void AggiornaOggetti(float dt);
    void Ferisci(float spintaX);
    void Rinasci();
    void DisegnaOggetto(const Oggetto& o) const;
    void DisegnaHud() const;

    Livello livello;
    std::string sorgente;
    std::vector<Oggetto> oggetti;
    Particelle particelle;
    Vector2 checkpoint{ 0, 0 };
    float camX = 0, camY = 0, scossa = 0;
    float coyote = 0, bufferSalto = 0, tFine = 0, tPasso = 0;
    bool saltoTagliato = false;
    float tempoVisivo = 0;
};
