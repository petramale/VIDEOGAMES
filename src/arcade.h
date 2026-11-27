// Giochi arcade dello Studio: quattro giochi completi scritti in C++ che usano lo stesso flusso
// dei giochi creati (intro Elvantis, menu 1/2 giocatori, opzioni, crediti, salvataggi):
//
//   stellaria     Stellaria: l'Invasione dei Prismi      sparatutto a ondate (30 ondate, 6 cieli)
//   sciame        Sciame di Orione                       sparatutto a formazioni (15 livelli, boss ogni 3)
//   polvere       Polvere da Sparo: Commando del West    run-and-gun in pixel art (10 missioni con boss)
//   querciaverde  Il Regno di Querciaverde               avventura isometrica dipinta (12 capitoli)
//
// Ogni gioco disegna la sua grafica con il Pennello (vettoriale, alta risoluzione) o in pixel art,
// compone la sua musica e i suoi effetti, ha un "regista" che si adatta a come gioca il giocatore
// (IA adattiva) e un seme che rende ogni partita diversa. Lo stato si salva in JSON.
#pragma once
#include "comune.h"
#include "arte.h"
#include "platform_tre.h"
#include "progetto_gioco.h"
#include "json.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace arcade {

using arte::K;
using arte::Tela;
using piatt::CreaBrano;
using piatt::TracciaTesto;
using piatt::Effetto;

// --- Comandi del giocatore (tastiera, mouse, joypad) ------------------------------------------
struct Pad {
    float x = 0, y = 0;                 // -1..1
    bool fuoco = false, fuocoP = false; // Spazio / A / clic sinistro (P = appena premuto)
    bool bomba = false, bombaP = false; // Z, X, Ctrl / B / clic destro
    bool salto = false, saltoP = false; // Su / Spazio nei giochi a piattaforme (Polvere)
    bool su = false, suP = false, giu = false, giuP = false, sx = false, sxP = false, dx = false, dxP = false;
    bool conferma = false;              // Invio
    Vector2 mouse{ 0, 0 };              // nelle coordinate virtuali del gioco
    bool clic = false, clicP = false, clicDxP = false, mouseMosso = false;
    float rotella = 0;
};

// Legge tastiera, mouse e joypad; il mouse in coordinate del gioco (area = dove si vede il gioco sullo schermo).
Pad LeggiPad(Rectangle area, int L, int A);

// --- Numeri casuali con stato salvabile (xorshift64*) -----------------------------------------
struct Caso {
    uint64_t s = 88172645463325252ull;
    void Semina(uint64_t v) { s = v ? v : 0x9e3779b97f4a7c15ull; for (int i = 0; i < 4; i++) Nuovo(); }
    uint64_t Nuovo() { s ^= s >> 12; s ^= s << 25; s ^= s >> 27; return s * 2685821657736338717ull; }
    float F() { return (Nuovo() >> 40) / 16777216.0f; }            // 0..1
    float F(float a, float b) { return a + (b - a) * F(); }
    int I(int a, int b) { return a + (int)(F() * (b - a + 1) * 0.99999f); }   // a..b compresi
    bool P(float p) { return F() < p; }
};
// Caso "visivo" (scintille, polvere): non influenza la partita, quindi non va salvato.
float Vis();
float Vis(float a, float b);

// --- Il "regista": IA adattiva comune ----------------------------------------------------------
// Osserva come gioca la persona e regola la sfida (0.6 facile .. 1.6 difficile) e le abitudini
// che i nemici imparano (dove sta di solito, quanto spara, quanto rischia).
struct Regista {
    float sfida = 1.0f;
    float colpiSparati = 0, colpiAndatiASegno = 0;
    float vitePerse = 0, tappeFinite = 0, tempoTappe = 0;
    float abitX = 0.5f, abitY = 0.5f;   // dove sta di solito (0..1)
    float fuocoMedio = 0.5f;            // quanto tiene premuto il fuoco
    float rischio = 0.5f;               // quanto si avvicina ai pericoli
    void Osserva(float dt, float xNorm, float yNorm, bool spara);
    void Sparo() { colpiSparati++; }
    void Bersaglio() { colpiAndatiASegno++; }
    void VitaPersa();
    void TappaFinita(float secondi, float atteso);
    float Precisione() const { return colpiSparati > 5 ? colpiAndatiASegno / colpiSparati : 0.5f; }
    std::string Json() const;
    void DaJson(const std::string& j);
};

// --- Un gioco arcade -------------------------------------------------------------------------
class GiocoArcade {
public:
    virtual ~GiocoArcade() {}
    virtual const char* Nome() const = 0;            // "stellaria"
    virtual std::string Titolo() const = 0;
    virtual int L() const { return 1920; }           // risoluzione virtuale
    virtual int A() const { return 1080; }
    virtual bool PixelArt() const { return false; }  // true: ingrandimento a pixel netti
    virtual void Prepara() = 0;                      // grafica e suoni (una volta, dopo InitWindow)
    virtual void Nuova(uint64_t seme, int giocatore) = 0;   // nuova partita
    virtual void Aggiorna(float dt, const Pad& p) = 0;
    virtual void Disegna() = 0;                      // dentro la texture di L() x A()
    virtual Pad Automatico() = 0;                    // il "pilota automatico" della vetrina dei menu
    virtual std::string Stato() const = 0;           // JSON dello stato (per i salvataggi)
    virtual bool Ripristina(const std::string& json, std::string& errore) = 0;
    virtual std::string Tappa() const = 0;           // "Ondata 7", "Missione 3"...
    virtual std::vector<std::string> Aiuto() const = 0;   // righe con i comandi
    virtual Brano MusicaMenu() const = 0;
    virtual bool SalvaQuandoVuoi() const { return false; }   // true: si salva in ogni momento (avventura)

    // --- Dati modificabili nello Studio --------------------------------------------------------
    // Ogni gioco descrive le sue parti modificabili (schema) e i valori di partenza; lo Studio le mostra
    // nella sezione del gioco arcade e le salva nel progetto .gioco. Il gioco le legge in Configura().
    virtual nlohmann::json Schema() const = 0;
    virtual nlohmann::json Predefiniti() const = 0;
    virtual std::vector<Brano> BraniPredefiniti() const = 0;   // la musica che va nel progetto
    virtual int Tappe() const = 0;                     // quanti livelli/ondate/capitoli ci sono nei dati
    virtual void IniziaDa(int tappa) = 0;              // dopo Nuova(): salta a quella tappa (prove nello Studio)
    // Usa i dati del progetto (quelli mancanti restano predefiniti). p puo' essere nullo.
    void Configura(const nlohmann::json& dati, const ProgettoGioco* p);
    nlohmann::json cfg;
    const ProgettoGioco* progetto = nullptr;
    // Il brano con quel nome: dal progetto (scritto nella sezione Audio) o, se manca, quello predefinito.
    Brano BranoDi(const std::string& nome) const;
    bool finito = false, vinto = false;
    int punteggio = 0;
    float tempo = 0;
    int puntiDiControllo = 0;   // cresce a ogni tappa: il flusso fa il salvataggio automatico
    int giocatore = 0;          // 0 o 1 (partite a turni)
    int altroPunteggio = -1;    // punteggio dell'altro giocatore (per l'HUD), -1 = partita da solo
};

std::unique_ptr<GiocoArcade> Crea(const std::string& nome);
// Il progetto .gioco d'esempio di un gioco arcade: dati predefiniti, musica e titolo, pronto per lo Studio.
ProgettoGioco ProgettoArcade(const std::string& nome);
// Fonde i dati salvati con quelli predefiniti (le chiavi mancanti prendono il valore predefinito).
nlohmann::json Unisci(const nlohmann::json& predefinito, const nlohmann::json& dati);
// Gli effetti sonori sostituiti nel progetto: evento del gioco -> effetto della sezione Audio.
void ImpostaSuoni(const nlohmann::json& suoni, const ProgettoGioco* p);
const std::vector<std::string>& Nomi();
std::string TitoloDi(const std::string& nome);
// Il testo del pacchetto: {"formato":"studio-videogiochi/arcade-1","gioco":"stellaria",...}
std::string Pacchetto(const std::string& nome);

// --- Aiuti di disegno ------------------------------------------------------------------------
// Disegna con il Pennello (API in stile canvas) in un'immagine e ne fa una texture liscia.
Texture2D Dipingi(int l, int a, const std::function<void(K&)>& f, bool mipmap = true);
Image DipingiImmagine(int l, int a, const std::function<void(K&)>& f);
// Disegna una texture centrata in (x, y), con scala, rotazione e colore.
void Sprite(const Texture2D& t, float x, float y, float scala = 1, float rot = 0, Color c = WHITE, bool flipX = false);
void Bagliore(float x, float y, float r, Color c);   // alone additivo
// Scritte: Outfit (Ui) o pixel (per la pixel art).
void Scritta(const std::string& t, float x, float y, int dim, Color c, int allinea = 0, bool ombra = true);   // allinea: -1 sx, 0 centro, 1 dx
void ScrittaPixel(const std::string& t, float x, float y, int scala, Color c, int allinea = -1);
int LarghezzaPixel(const std::string& t, int scala);

// --- Particelle (solo grafica) ------------------------------------------------------------------
struct Particella { Vector2 p, v; float vita, durata, r; Color c; float attrito = 0.98f, gravita = 0; bool additiva = true; int forma = 0; };
struct Particelle {
    std::vector<Particella> v;
    void Aggiungi(const Particella& p) { if (v.size() < 4000) v.push_back(p); }
    void Esplosione(Vector2 c, Color col, int n, float vel, float raggio, float durata = 0.7f);
    void Aggiorna(float dt);
    void Disegna() const;
};

// --- Suoni --------------------------------------------------------------------------------------
// Effetti sintetizzati dal codice: f(t) restituisce il campione (-1..1) all'istante t (secondi).
// Ogni effetto ha 4 voci, cosi' gli spari ravvicinati non si interrompono a vicenda.
void CreaSuono(const std::string& nome, float durata, const std::function<float(float t)>& f);
void Suona(const std::string& nome, float volume = 1, float tono = 1);
// Aiuti per costruire i suoni
float Onda(int tipo, float fase);                  // 0 quadra, 1 triangolo, 2 sega, 3 seno (fase in giri)
float Rumore();                                    // -1..1
float Inviluppo(float t, float attacco, float durata, float curva = 2);   // 0..1
extern bool gSilenzio;              // true mentre gioca la vetrina del menu: niente suoni ne' musica
void SuonaMusica(const Brano& b);   // cambia il brano di sottofondo solo se e' diverso
void FermaMusica();

// --- JSON minimo per i salvataggi ----------------------------------------------------------------
std::string Num(double v);   // numero esatto (17 cifre se serve)
// Letture comode dai dati del progetto (con il valore predefinito se manca o e' sbagliato).
Color ColoreDi(const nlohmann::json& j, const char* chiave, Color predefinito = WHITE);
std::string Esa(Color c);    // "#rrggbb"
float NumDi(const nlohmann::json& j, const char* chiave, float predefinito);
int IntDi(const nlohmann::json& j, const char* chiave, int predefinito);
std::string TestoDi(const nlohmann::json& j, const char* chiave, const std::string& predefinito = "");

}  // namespace arcade
