// Pezzi comuni del flusso dei giochi creati (intro, menu, scritte, logo), usati dal motore
// dei giochi a mappe (main.cpp) e dai giochi arcade (arcade_flusso.cpp).
#pragma once
#include "comune.h"
#include "pacchetto.h"
#include <string>
#include <vector>

extern const char* SVILUPPATORE;
constexpr int BRANO_INTRO = 9;   // la musica fissa dell'intro e del menu dei giochi creati
extern Texture2D gLogo;

void DisegnaLogo(Rectangle r, float alfa = 1);
void CaricaRisorse();
void FinestraGrande();
std::string FileAccanto(const std::string& estensione);
void Scritta2(const std::string& t, float x, float y, int dim, Color c, bool centro = true);
void DisegnaStelle(float t, float alfa);
// Intro con il logo Elvantis; false se la finestra e' stata chiusa.
bool Intro(const std::string& titolo, const std::string& sotto, bool aspettaTasto, float durata);

// Una voce di menu grande, con mouse, tastiera e joypad. Frame() ritorna la voce scelta o -1.
struct MenuGioco {
    int scelta = 0;
    int Frame(const std::vector<std::string>& voci, float cx, float y, float passo, int dim);
};

// I giochi arcade (Stellaria, Sciame di Orione, Polvere da Sparo, Borgo di Querciaverde):
// il loro pacchetto e' un piccolo JSON con "formato": "studio-videogiochi/arcade-1".
bool EArcade(const std::string& testo);
int GiocaArcade(const DatiGioco& dati);
