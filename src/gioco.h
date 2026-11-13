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
#include "mondo.h"

struct Comandi {
    float asse = 0;                 // -1 sinistra .. 1 destra (anche analogico)
    float asseY = 0;                // -1 su .. 1 giu' (labirinto, puzzle, sparatutto)
    bool salto = false, saltoPremuto = false;   // nello sparatutto e' il fuoco
    bool mouseGuida = false;        // mouse: cammina verso il puntatore
    float mouseX = 0, mouseY = 0;   // posizione del puntatore sullo schermo virtuale
    bool annulla = false;           // puzzle: annulla l'ultima mossa
    static Comandi DaPeriferiche(); // tastiera + mouse + joypad, secondo la mappatura
};

struct Proiettile {
    Vector2 pos, vel;
    bool amico;                     // sparato dall'eroe
    float vita;
    Color colore;
};

// Disegna un oggetto del livello (usato anche dalla tavolozza dell'editor).
void DisegnaOggetto(const Oggetto& o, float camX, float camY, float tempo, bool bandieraVinta);
Rectangle BoxOggetto(const Oggetto& o);

class Gioco {
public:
    // Accetta il JSON del nuovo formato e il vecchio testo a caratteri.
    bool Carica(const std::string& testoLivello, std::string& errore);
    bool CaricaMondo(const Mondo& m, std::string& errore);
    void Aggiorna(float dt, const Comandi& c);
    void Disegna();
    void Ricomincia() { std::string e; CaricaMondo(Mondo(mondo), e); }

    Sfondo sfondo;
    Eroe eroe;
    ParametriFisica fisica;
    bool mostraEtichette = false;   // nomi degli oggetti (vetrina della categoria 3)
    bool mostraCollisioni = false;  // riquadri di collisione (categoria 5)
    bool cicloGiorno = true;        // l'ora del giorno avanza lentamente
    bool mostraHud = true;
    std::string titolo = "La valle delle gemme";
    std::string crediti;            // riga mostrata alla vittoria (giochi creati)
    bool mostraEsito = true;        // scritte "livello completato" e "game over" (la Partita usa le sue)
    bool mostraFps = false;         // debug: FPS e contatori
    int lingua = 0;                 // 0 italiano, 1 inglese (testi dei cartelli)

    Modello modello = Modello::Platform;
    float zoomBase = 1;
    int musica = 0;
    std::vector<TestoCartello> testi;

    int monete = 0, gemme = 0, vite = 3, punteggio = 0;
    int energia = 3, energiaMax = 3;
    int mosse = 0;                  // puzzle
    float tempo = 0;
    bool vinto = false, finito = false;

    float CameraX() const { return camX; }
    float CameraY() const { return camY; }
    bool VistaAlto() const { return vistaAlto; }
    const Mondo& MondoAttuale() const { return mondo; }
    int MappaAttuale() const { return mappaAttuale; }
    Telecamera telecamera;          // della mappa attuale (si puo' cambiare al volo)
    int GemmeTotali() const { return gemmeTotali; }
    bool UscitaAperta() const { return modello != Modello::Labirinto || gemme >= gemmeTotali; }

private:
    void Fisica(float dt, const Comandi& c);
    void FisicaLabirinto(float dt, const Comandi& c);
    void FisicaPuzzle(float dt, const Comandi& c);
    void FisicaSparatutto(float dt, const Comandi& c);
    void AggiornaProiettili(float dt);
    void AggiornaBoss(Oggetto& o, float dt, Rectangle box);
    void AggiornaTelecamera(float dt, bool subito = false);
    bool EntraMappa(int indice, Vector2 arrivo, std::string& errore);
    void VaiAMappa(int indice, Vector2 arrivo);
    bool Laterale() const { return !vistaAlto && modello != Modello::Sparatutto; }
    void DisegnaDecorazioni(float cx, float cy, bool primoPiano) const;
    void Spara(Vector2 da, Vector2 verso, float velocita, bool amico, Color c);
    void Colpito(Oggetto& o, int danno);       // un nemico preso da un colpo o schiacciato
    bool CassaIn(int tx, int ty) const;
    void DisegnaNave(Vector2 p) const;
    void DisegnaCartello() const;
    void MuoviConCollisioni(Vector2& pos, Vector2& vel, float w, float hgt, float dt, bool& aTerra, bool attraversa);
    void AggiornaOggetti(float dt);
    void Ferisci(float spintaX);
    void Rinasci();
    void DisegnaOggettoConInfo(const Oggetto& o) const;
    void DisegnaHud() const;

    Livello livello;
    Mondo mondo;
    int mappaAttuale = 0, mappaVerso = -1, mappaDopoVittoria = -1;
    Vector2 arrivoVerso{ -1, -1 };
    float tTransizione = 0;         // dissolvenza tra una mappa e l'altra
    bool portaBloccata = false;     // appena arrivati su una porta: non si riparte subito
    bool vistaAlto = false;
    std::vector<Oggetto> oggetti;
    std::vector<Proiettile> proiettili;
    std::vector<std::vector<Oggetto>> storiaPuzzle; // per annullare le mosse
    std::vector<Vector2> storiaEroe;
    Particelle particelle;
    int gemmeTotali = 0, bossIndice = -1, cartello = -1;
    float zoom = 1, tSparo = 0, tPassoPuzzle = 0, tFineScorrimento = 0;
    bool fineScorrimento = false;
    Vector2 checkpoint{ 0, 0 };
    float camX = 0, camY = 0, scossa = 0;
    float coyote = 0, bufferSalto = 0, tFine = 0, tPasso = 0;
    bool saltoTagliato = false;
    float tempoVisivo = 0;
};
