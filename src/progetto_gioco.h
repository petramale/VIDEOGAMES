// Il gioco come progetto unico (file .gioco): il mondo con le sue mappe e la libreria
// del gioco: personaggi e oggetti disegnati e animati, scenari, caselle, brani,
// effetti sonori, regole della logica e variabili. Tutto si salva in un solo file JSON
// (le immagini dentro, in PNG codificato base64) e finisce dentro l'eseguibile.
#pragma once
#include "comune.h"
#include "mondo.h"
#include <map>

enum class StileGrafico { PixelFine, Disegnato };
const char* NomeStile(StileGrafico s);

// --- Disegni animati --------------------------------------------------------------------

struct Fotogramma {
    int w = 32, h = 32;
    std::vector<Color> px;            // w*h colori RGBA, riga per riga
    void Nuovo(int larghezza, int altezza) { w = larghezza; h = altezza; px.assign((size_t)w * h, BLANK); }
    Color Prendi(int x, int y) const { return (x >= 0 && y >= 0 && x < w && y < h) ? px[(size_t)y * w + x] : BLANK; }
    void Metti(int x, int y, Color c) { if (x >= 0 && y >= 0 && x < w && y < h) px[(size_t)y * w + x] = c; }
    bool Vuoto() const;
};

struct Animazione {
    std::string nome = "fermo";
    std::vector<Fotogramma> fotogrammi;
    float fps = 8;
    bool ripeti = true;
};

// Un disegno con piu' animazioni (personaggi, oggetti, caselle). 'densita' = pixel del disegno
// per pixel del mondo: 2 vuol dire che un personaggio largo 32 pixel occupa 16 pixel di mondo
// (una casella) ma si vede con il doppio dei dettagli.
struct Disegno {
    int w = 32, h = 32;
    float densita = 2;
    bool liscio = false;              // grafica disegnata HD: filtro morbido invece dei pixel netti
    std::vector<Animazione> animazioni;

    void Nuovo(int larghezza, int altezza, float dens);
    int TrovaAnimazione(const std::string& nome) const;   // -1 se non c'e'
    // Fotogramma da mostrare dopo 't' secondi dall'inizio dell'animazione.
    int FotogrammaAl(int anim, float t) const;
    float LarghezzaMondo() const { return w / densita; }
    float AltezzaMondo() const { return h / densita; }
    // Texture pronta da disegnare (creata la prima volta, rifatta dopo Invalida()).
    Texture2D Texture(int anim, int fot) const;
    void Invalida() const;
    // Disegna centrato in orizzontale con i piedi in 'base' (coordinate del mondo).
    void DisegnaBase(int anim, float t, Vector2 base, bool specchia, Color tinta = WHITE, float scala = 1) const;
    void DisegnaIn(Rectangle r, int anim, float t, Color tinta = WHITE) const;   // dentro un riquadro (icone)
    void Ridimensiona(int nw, int nh);
    ~Disegno();
    Disegno() = default;
    Disegno(const Disegno& d);
    Disegno& operator=(const Disegno& d);

private:
    mutable std::vector<std::vector<Texture2D>> cache;
    mutable int versione = 0;
};

// --- Personaggi e oggetti della libreria --------------------------------------------

enum class TipoComportamento { Giocatore, Fermo, Pattuglia, Insegue, Vola, Spara, Boss, Parla, Totale };
const char* NomeComportamento(TipoComportamento t);
const char* ChiaveComportamento(TipoComportamento t);

struct Comportamento {
    TipoComportamento tipo = TipoComportamento::Pattuglia;
    float velocita = 40;      // pixel al secondo
    float salto = 0;          // spinta del salto (0 = non salta)
    int energia = 1;          // colpi per sconfiggerlo
    int danno = 1;            // cuori tolti all'eroe
    float vista = 120;        // distanza a cui nota l'eroe (insegue, spara, boss)
    float attaccoOgni = 2;    // secondi fra un attacco e l'altro
    std::string proiettile;   // nome di un oggetto della libreria usato come proiettile (vuoto = palla di fuoco)
    bool gravita = true;
    bool calpestabile = true; // saltandogli sopra lo si colpisce
    std::string dialogo;      // "Parla": cosa dice
    int punti = 100;          // punti quando e' sconfitto
};

struct PersonaggioDef {
    std::string nome;
    std::string autore;       // "" oppure "Claude" o il nome della raccolta
    Disegno disegno;
    Comportamento comportamento;
    std::map<std::string, std::string> suoni;   // evento -> effetto ("salto", "colpito", "attacco", "sconfitto")
};

enum class FunzioneOggetto { Decorazione, Raccogli, Cura, Chiave, Porta, Molla, Cassa, Interruttore, PiattaformaMobile,
                             Pericolo, Arrivo, Proiettile, Totale };
const char* NomeFunzione(FunzioneOggetto f);
const char* ChiaveFunzione(FunzioneOggetto f);

struct OggettoDef {
    std::string nome;
    std::string autore;
    Disegno disegno;
    FunzioneOggetto funzione = FunzioneOggetto::Raccogli;
    int valore = 10;              // punti, cuori, spinta della molla...
    std::string richiede;         // porte: oggetto da avere (chiave); vuoto = sempre aperta
    bool solido = false;
    bool gravita = false;
    float velocita = 40;          // piattaforme mobili e proiettili
    float distanza = 64;          // piattaforme mobili: quanto si spostano
    std::string suono;            // effetto quando si attiva
    std::string variabile;        // raccogli: variabile da aumentare (oltre ai punti)
};

// --- Scenari e caselle ----------------------------------------------------------------

struct Scenario {
    std::string nome;
    std::string autore;
    int tema = 0;
    float ora = 0.35f;
    bool cicloGiorno = true;
    bool sfondoGenerato = true;
    bool cieloPersonale = false;
    Color cieloAlto = Hex(0x3f8ff0), cieloBasso = Hex(0xb9e3ff);
    std::vector<StratoSfondo> strati;
    int pioggia = 0;               // 0 niente, 1 pioggia, 2 neve
};

// Aspetto personalizzato di un tipo di casella ('#', '=', '-', '^', '.').
struct CasellaDef {
    char carattere = '#';
    std::string nome;
    Disegno disegno;               // 1 animazione, uno o piu' fotogrammi
};

// --- Suoni e musica ------------------------------------------------------------------------

struct NotaBrano { float inizio = 0, durata = 1; int altezza = 60; };   // in sedicesimi; altezza MIDI
struct TracciaBrano {
    std::string nome;
    int strumento = 0;            // 0 quadra, 1 triangolo, 2 dente di sega, 3 seno, 4 rumore/batteria
    float volume = 0.5f;
    bool batteria = false;
    std::vector<NotaBrano> note;
};
struct Brano {
    std::string nome;
    std::string autore;
    float bpm = 120;
    int battute = 8;               // 16 sedicesimi per battuta
    std::vector<TracciaBrano> tracce;
    std::string file;              // brano importato (OGG, MP3, WAV): nome del file, vuoto = note
    std::string dati;              // contenuto del file importato
    void Nuovo();
};

struct EffettoSuono {
    std::string nome;
    std::string autore;
    int onda = 0;                  // come gli strumenti
    float f0 = 440, f1 = 880, durata = 0.2f, volume = 0.5f, duty = 0.5f;
    float vibrato = 0, vibratoHz = 0, rumore = 0;   // rumore: miscela 0-1
    std::string file;              // nome del file importato (WAV, OGG, MP3), vuoto = generato
    std::string dati;              // contenuto del file importato
};

// --- Logica: regole "quando ... allora ..." ---------------------------------------------

struct AzioneRegola {
    std::string tipo;              // vedi TipiAzione()
    std::string testo;             // nome (suono, mappa, oggetto, variabile, messaggio...)
    float numero = 0;
};

struct Regola {
    bool attiva = true;
    std::string chi = "eroe";      // "eroe", "gioco", il nome di un personaggio o di un oggetto
    std::string evento = "tocca";  // vedi TipiEvento()
    std::string conChi;            // con chi/che cosa (per tocca, colpisce...) oppure il tasto
    std::string variabile;         // condizione facoltativa: variabile op valore
    std::string confronto = ">=";
    float valore = 0;
    std::vector<AzioneRegola> azioni;
};

const std::vector<std::string>& TipiEvento();
const std::vector<std::string>& NomiEvento();
const std::vector<std::string>& TipiAzione();
const std::vector<std::string>& NomiAzione();

// --- Il progetto ------------------------------------------------------------------------------

struct ProgettoGioco {
    StileGrafico stile = StileGrafico::PixelFine;
    std::string eroe;              // personaggio della libreria che fa da eroe (vuoto = quello predefinito)
    Mondo mondo;
    std::vector<PersonaggioDef> personaggi;
    std::vector<OggettoDef> oggetti;
    std::vector<Scenario> scenari;
    std::vector<CasellaDef> caselle;
    std::vector<Brano> brani;
    std::vector<EffettoSuono> effetti;
    std::vector<Regola> regole;
    std::map<std::string, float> variabili;   // valori iniziali
    std::map<std::string, std::string> suoniEventi;   // evento del gioco -> effetto ("salto", "moneta"...)
    std::vector<std::pair<std::string, std::string>> immagini;   // immagini importate (nome, dati PNG/JPG)

    std::string Json(bool compatto = false) const;
    bool DaJson(const std::string& testo, std::string& errore);
    // Accetta un .gioco, un mondo JSON o un vecchio livello a caratteri.
    bool Carica(const std::string& testo, std::string& errore);
    std::string Controlla() const;

    PersonaggioDef* Personaggio(const std::string& nome);
    const PersonaggioDef* Personaggio(const std::string& nome) const;
    OggettoDef* Oggetto(const std::string& nome);
    const OggettoDef* Oggetto(const std::string& nome) const;
    const Scenario* TrovaScenario(const std::string& nome) const;
    const CasellaDef* Casella(char c) const;
    const Brano* TrovaBrano(const std::string& nome) const;
    const EffettoSuono* TrovaEffetto(const std::string& nome) const;
    std::string NomeLibero(const std::string& base) const;   // "Drago", "Drago 2"...
};

// Immagine <-> PNG in base64 (per il JSON).
std::string FotogrammaInBase64(const Fotogramma& f);
bool FotogrammaDaBase64(const std::string& testo, Fotogramma& f);
Fotogramma FotogrammaDaImmagine(const Image& img, Rectangle parte);
// Pixel art testuale (formato di Claude): palette {"r":"#d8344a"} e righe di caratteri.
bool FotogrammaDaRighe(const std::vector<std::string>& righe, const std::map<char, Color>& palette, Fotogramma& f, std::string& errore);

// Un nuovo progetto vuoto per un genere, con lo stile scelto.
ProgettoGioco NuovoProgetto(Genere g, StileGrafico stile, const std::string& titolo);
