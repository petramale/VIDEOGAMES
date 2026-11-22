// Mappa a tile del livello, letta da un semplice testo:
//   #  terra con erba      =  pietra          -  piattaforma attraversabile dal basso
//   ^  spine               P  partenza        F  bandiera d'arrivo
//   o  moneta   g  gemma   m  molla   c  forziere   k  lanterna (punto di controllo)
//   e  slime (nemico)      b  pipistrello (nemico)
//   r  fantasma che insegue   s  torretta che spara   B  boss a tre fasi
//   h  cuore (energia)     D  porta d'uscita   1..9  cartello con un testo
//   x  cassa da spingere   .  bersaglio delle casse (modello puzzle)
// Una riga che inizia con ';' contiene le impostazioni del progetto (vedi progetto.h).
#pragma once
#include "comune.h"

enum class TipoOggetto { Moneta, Gemma, Molla, Forziere, Lanterna, Slime, Pipistrello, Bandiera,
                         Cuore, Cartello, Porta, Fantasma, Torretta, Boss, Cassa };

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
    int numero = 0;         // cartelli: quale testo; boss: energia
    float timer = 0, vy = 0; // nemici: tempo fra un attacco e l'altro, velocita' verticale
    bool inAria = false, sconfitto = false;
    int destMappa = -1;     // porte: mappa di destinazione (-1 = uscita del livello)
    Vector2 dest{ -1, -1 }; // porte: punto d'arrivo nella mappa di destinazione
};

// Caratteri della mappa che diventano oggetti.
const char* CaratteriOggetti();

const char* NomeOggetto(TipoOggetto t);
// Vecchio formato: carattere -> oggetto ("appoggiato" = la posizione e' la base, non il centro).
bool OggettoDaCarattere(char c, TipoOggetto& t, bool& appoggiato);
// Nuovo formato: nome in minuscolo ("moneta", "slime"...) <-> tipo.
std::string ChiaveOggetto(TipoOggetto t);
bool TipoDaNome(const std::string& nome, TipoOggetto& t);
constexpr int NUM_TIPI_OGGETTO = 15;

struct Mappa;
struct Disegno;

class Livello {
public:
    // Caselle disegnate nella libreria del gioco (carattere -> disegno); le altre sono quelle del motore.
    std::vector<std::pair<char, const Disegno*>> caselleLibreria;
    bool Carica(const std::string& testo, std::string& errore);
    bool DaMappa(const Mappa& m, std::string& errore);   // nuovo formato
    char Cella(int x, int y) const;
    void ImpostaCella(int x, int y, char c) { if (x >= 0 && y >= 0 && x < w && y < h) celle[(size_t)y * w + x] = c; }
    bool Solido(int x, int y) const { char c = Cella(x, y); return c == '#' || c == '='; }
    bool Piattaforma(int x, int y) const { return Cella(x, y) == '-'; }
    bool Spina(int x, int y) const { return Cella(x, y) == '^'; }
    bool Bersaglio(int x, int y) const { return Cella(x, y) == '.'; }
    int Larghezza() const { return w; }
    int Altezza() const { return h; }
    Vector2 Partenza() const { return partenza; }
    const std::vector<Oggetto>& Oggetti() const { return oggetti; }
    void Disegna(float camX, float camY, float tempo, float notte, bool dallAlto = false, float larghezza = VW, float altezza = VH) const;
    // Pavimento per i modelli visti dall'alto (labirinto e puzzle).
    void DisegnaPavimento(float camX, float camY, int tema, float larghezza = VW, float altezza = VH, bool pulisci = true) const;

private:
    int w = 0, h = 0;
    std::vector<char> celle;
    std::vector<Oggetto> oggetti;
    Vector2 partenza{ 32, 200 };
    bool bordiChiusi = false;   // vista dall'alto: anche sopra e sotto la mappa c'e' il muro
    void DisegnaTile(int tx, int ty, float sx, float sy, float tempo, float notte, bool dallAlto) const;
};

// Livelli inclusi nello studio (funzionano anche nell'eseguibile, senza file esterni).
const char* LivelloAvventura();
const char* LivelloVetrinaOggetti();
