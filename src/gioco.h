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
#include "progetto.h"

struct Comandi {
    float asse = 0;                 // -1 sinistra .. 1 destra (anche analogico)
    bool salto = false, saltoPremuto = false;
    bool mouseGuida = false;        // mouse: cammina verso il puntatore
    float mouseX = 0;               // posizione del puntatore sullo schermo virtuale
    static Comandi DaPeriferiche(); // tastiera + mouse + joypad, secondo la mappatura
};

// Disegna un oggetto del livello (usato anche dalla tavolozza dell'editor).
void DisegnaOggetto(const Oggetto& o, float camX, float camY, float tempo, bool bandieraVinta);
Rectangle BoxOggetto(const Oggetto& o);

class Gioco {
public:
    bool Carica(const std::string& testoLivello, std::string& errore);
    void Aggiorna(float dt, const Comandi& c);
    void Disegna();

    Sfondo sfondo;
    Eroe eroe;
    ParametriFisica fisica;
    bool mostraEtichette = false;   // nomi degli oggetti (vetrina della categoria 3)
    bool mostraCollisioni = false;  // riquadri di collisione (categoria 5)
    bool cicloGiorno = true;        // l'ora del giorno avanza lentamente
    bool mostraHud = true;
    std::string titolo = "La valle delle gemme";
    std::string crediti;            // riga mostrata alla vittoria (giochi creati)

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
    void DisegnaOggettoConInfo(const Oggetto& o) const;
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
