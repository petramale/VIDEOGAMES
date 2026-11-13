// Il progetto di un gioco: livello, titolo, scenario, aspetto dell'eroe e fisica.
// Si salva come testo: una prima riga di impostazioni che inizia con ';' e poi la mappa.
//   ;tema=1 ora=0.35 gravita=900 salto=305 velocita=115 molla=470 maglia=#3f7de0 ... titolo=Il mio gioco
#pragma once
#include "comune.h"
#include "personaggio.h"
#include "pacchetto.h"

struct ParametriFisica {
    float gravita = 900;     // pixel/s^2
    float salto = 305;       // velocita' iniziale del salto
    float velocita = 115;    // velocita' massima di corsa
    float molla = 470;       // spinta della molla
};

// Tipo di gioco: cambia regole, comandi e visuale.
enum class Modello { Platform, Sparatutto, Labirinto, Puzzle, Totale };
const char* NomeModello(Modello m);
const char* ChiaveModello(Modello m);   // "platform", "sparatutto"...

// Testo di un cartello (1..9) in due lingue.
struct TestoCartello { int numero = 1; std::string it, en; };

std::string ColoreInTesto(Color c);
bool TestoInColore(const std::string& s, Color& c);

struct Progetto {
    std::string titolo = "Il mio gioco";
    int tema = 0;
    float ora = 0.35f;
    ParametriFisica fisica;
    AspettoEroe eroe;
    std::vector<std::string> righe;   // la mappa, tutte le righe lunghe uguali
    std::vector<FileIncluso> file;    // musiche ed effetti caricati da file ("musica.ogg", "effetto_Salto.wav")
    Modello modello = Modello::Platform;
    float zoom = 1.0f;                // ingrandimento della telecamera (1 = normale)
    int musica = 0;                   // brano generato (vedi Audio::Brani)
    std::vector<TestoCartello> testi;

    static constexpr int ALTEZZA = 15;

    void Nuovo(int larghezza = 80);
    bool DaTesto(const std::string& testo, std::string& errore);
    bool LeggiImpostazioni(const std::string& testo);   // solo la riga ';' (false se manca)
    std::string Testo() const;
    std::string Json() const;                            // stato completo, per Claude
    int Larghezza() const { return righe.empty() ? 0 : (int)righe[0].size(); }
    void Ridimensiona(int larghezza);
    const TestoCartello* Testo(int numero) const;
};

// Un gioco con piu' livelli e' un unico testo: i livelli sono separati da una riga ";;".
std::vector<std::string> DividiLivelli(const std::string& testoGioco);
std::string UnisciLivelli(const std::vector<std::string>& livelli);
