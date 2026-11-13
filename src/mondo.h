// Il nuovo formato dei giochi: un MONDO fatto di una o piu' MAPPE collegate da porte.
// Ogni mappa ha dimensioni libere, una vista (laterale con gravita' o dall'alto),
// una telecamera configurabile, un cielo, strati di sfondo in parallasse,
// le caselle del terreno, decorazioni e oggetti posizionati liberamente.
// Si salva in JSON. Il vecchio formato a caratteri si importa con Mondo::Carica.
#pragma once
#include "comune.h"
#include "personaggio.h"
#include "progetto.h"

// I generi di gioco previsti dal progetto (piu' labirinto e puzzle, gia' esistenti).
enum class Genere { Platform, OpenWorld, Sparatutto, Avventura, Labirinto, Puzzle, Totale };
const char* NomeGenere(Genere g);
const char* ChiaveGenere(Genere g);

enum class Vista { Laterale, Alto };

enum class ModoTelecamera { Segui, Stanze, Automatica, Fissa, Totale };
const char* NomeModoTelecamera(ModoTelecamera m);
const char* ChiaveModoTelecamera(ModoTelecamera m);

struct Telecamera {
    ModoTelecamera modo = ModoTelecamera::Segui;
    float zonaMortaX = 16, zonaMortaY = 24;   // l'eroe si muove liberamente in questo riquadro (pixel)
    float anticipo = 40;                      // la telecamera guarda avanti nella direzione di marcia
    float velocita = 40;                      // scorrimento automatico, pixel al secondo
    int dirX = 1, dirY = 0;                   // direzione dello scorrimento automatico
    float zoom = 1;
};

// Uno strato dello sfondo (o del primo piano).
//   tipo: "montagne", "colline", "foresta", "nuvole", "stelle", "fascia" (colore pieno), "immagine"
struct StratoSfondo {
    std::string nome, tipo = "montagne", immagine;
    Color colore = Hex(0x6b86b3);
    float parallasseX = 0.2f, parallasseY = 0.1f;   // 0 = fermo, 1 = si muove con la mappa
    float altezza = 0.6f;                           // posizione verticale sullo schermo (0 alto, 1 basso)
    float ampiezza = 50;                            // altezza delle creste (montagne, colline, foresta)
    float seme = 1;                                 // forme diverse a parita' di tipo
    float scala = 1;                                // immagini: ingrandimento
    float vento = 0;                                // movimento proprio (nuvole, immagini), pixel/s
    bool ripeti = true;                             // immagini: ripetute in orizzontale
    bool primoPiano = false;                        // disegnato davanti a tutto
};

struct Decorazione {
    std::string immagine;
    float x = 0, y = 0;     // angolo in alto a sinistra, pixel della mappa
    float scala = 1;
    bool primoPiano = false;
};

// Un oggetto della mappa. tipo e' il nome dell'oggetto in minuscolo ("moneta", "slime", "porta"...).
// x, y in pixel della mappa: centro per gli oggetti sospesi, base (piedi) per quelli appoggiati.
struct OggettoMappa {
    std::string tipo;
    float x = 0, y = 0;
    int numero = 0;              // cartelli: quale testo; boss: energia
    int mappa = -1;              // porte: mappa di destinazione (-1 = uscita del livello)
    float destX = -1, destY = -1; // porte: dove si arriva (pixel), -1 = partenza della mappa
};

struct Mappa {
    std::string nome = "Mappa";
    int larghezza = 80, altezza = 17;     // in caselle da 16 pixel
    Vista vista = Vista::Laterale;
    int tema = 0;                         // tema dello sfondo generato
    float ora = 0.35f;
    bool cicloGiorno = true;
    int musica = 0;
    bool sfondoGenerato = true;           // false = solo cielo e strati personalizzati
    bool cieloPersonale = false;
    Color cieloAlto = Hex(0x3f8ff0), cieloBasso = Hex(0xb9e3ff);
    std::vector<StratoSfondo> strati;
    Telecamera telecamera;
    std::vector<std::string> caselle;     // 'altezza' righe di 'larghezza' caratteri (# = - ^ .)
    std::vector<Decorazione> decorazioni;
    std::vector<OggettoMappa> oggetti;
    std::vector<TestoCartello> testi;
    Vector2 partenza{ 32, 200 };          // piedi dell'eroe, pixel

    void Nuova(int w, int h, Vista v);
    char Casella(int x, int y) const;
    void ImpostaCasella(int x, int y, char c);
    void Ridimensiona(int w, int h);
};

struct Mondo {
    std::string titolo = "Il mio gioco";
    std::string autori;
    Genere genere = Genere::Platform;
    int giocatoriMax = 1;
    bool schermoDiviso = false;
    ParametriFisica fisica;
    AspettoEroe eroe;
    int mappaIniziale = 0;
    std::vector<Mappa> mappe;

    std::string Json(bool compatto = false) const;
    bool DaJson(const std::string& testo, std::string& errore);
    // Accetta sia il JSON sia il vecchio formato a caratteri (anche con piu' livelli ";;").
    bool Carica(const std::string& testo, std::string& errore);
    // Un livello nel vecchio formato diventa una mappa (con impostazioni, eroe e fisica).
    bool ImportaVecchio(const std::string& testo, std::string& errore);
    std::string Controlla() const;   // vuoto se tutto e' in ordine, altrimenti il problema
};

bool SembraJson(const std::string& testo);

// Mondi d'esempio del nuovo motore (fase 1): mostrano viste, telecamere e mappe collegate.
std::vector<Mondo> MondiDiProva();

// Una sola mappa in JSON (per Claude e per copiare/incollare mappe).
std::string MappaInJson(const Mappa& m);
bool MappaDaJson(const std::string& testo, Mappa& m, std::string& errore);
// Posizione in pixel di un oggetto messo nella casella (cx, cy): base o centro, secondo il tipo.
Vector2 PosizioneInCasella(const std::string& tipo, int cx, int cy, Vista vista);
// Nome di file senza spazi ne' simboli: "Il mio gioco" -> "IlMioGioco".
std::string NomeFileDaTitolo(const std::string& titolo);
