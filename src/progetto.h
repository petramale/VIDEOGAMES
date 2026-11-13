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

    static constexpr int ALTEZZA = 15;

    void Nuovo(int larghezza = 80);
    bool DaTesto(const std::string& testo, std::string& errore);
    bool LeggiImpostazioni(const std::string& testo);   // solo la riga ';' (false se manca)
    std::string Testo() const;
    std::string Json() const;                            // stato completo, per Claude
    int Larghezza() const { return righe.empty() ? 0 : (int)righe[0].size(); }
    void Ridimensiona(int larghezza);
};
