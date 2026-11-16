// Periferiche di gioco: tastiera, mouse e joystick/joypad.
// Ogni azione (sinistra, salto, conferma...) puo' essere legata a due tasti e a un
// pulsante del joypad. La mappatura si cambia dallo studio e viene salvata in
// comandi.cfg accanto all'eseguibile: vale sia per lo studio sia per i giochi creati.
#pragma once
#include "comune.h"

enum class Azione { Sinistra, Destra, Su, Giu, Salto, Conferma, Indietro, Pausa, Totale };
constexpr int NUM_AZIONI = (int)Azione::Totale;

const char* NomeAzione(Azione a);
const char* NomeTasto(int tasto);
const char* NomePulsante(int pulsante);

struct Mappatura {
    int tasti[NUM_AZIONI][2];
    int pulsanti[NUM_AZIONI];
    bool mouseNelGioco = true;   // tieni premuto il sinistro per camminare verso il puntatore, destro per saltare
    float zonaMorta = 0.35f;     // levette analogiche: sotto questo valore non si muove nulla
    static Mappatura Predefinita();
};

enum class Periferica { Tastiera, Mouse, Joypad };

class Periferiche {
public:
    Mappatura m = Mappatura::Predefinita();

    void Carica();
    void Salva() const;
    // Da chiamare una volta per fotogramma. 'scala' e 'margine' servono a riportare
    // il mouse dalla finestra alla risoluzione virtuale del gioco.
    void Aggiorna(float scala, Vector2 margine);

    bool Tenuto(Azione a) const { return tenuto[(int)a]; }
    bool Premuto(Azione a) const { return tenuto[(int)a] && !precedente[(int)a]; }
    float Asse() const; // -1..1 orizzontale, con levetta analogica

    int Joypad() const { return joypad; }            // -1 se nessuno collegato
    const char* NomeJoypad() const;
    Vector2 Mouse() const { return mouse; }          // in coordinate virtuali
    bool MouseMosso() const { return mouseMosso; }
    Periferica Ultima() const { return ultima; }
    float LevettaX() const { return levettaX; }
    float LevettaY() const { return levettaY; }

private:
    bool tenuto[NUM_AZIONI] = {};
    bool precedente[NUM_AZIONI] = {};
    int joypad = -1;
    Vector2 mouse{ 0, 0 };
    bool mouseMosso = false;
    float levettaX = 0, levettaY = 0;
    Periferica ultima = Periferica::Tastiera;
};

extern Periferiche gInput;
