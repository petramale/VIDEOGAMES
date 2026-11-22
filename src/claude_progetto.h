// Claude in tutto lo studio: riceve lo stato del gioco (mondo + libreria + regole) e la
// sezione in cui si trova l'utente, e risponde con un messaggio e un elenco di azioni.
// Le azioni sul mondo sono quelle di claude_mondo.h; in piu' ci sono quelle sulla libreria:
// personaggi e oggetti disegnati e animati, scenari, caselle, brani, effetti, regole e variabili.
#pragma once
#include "progetto_gioco.h"
#include <string>
#include <vector>

std::string PromptProgetto();
// sezione: "mondi", "personaggi"...; selezionato: nome dell'elemento aperto (il suo disegno viene mandato per intero).
std::string StatoProgettoPerClaude(const ProgettoGioco& p, int mappaAperta, const std::string& sezione, const std::string& selezionato);

struct EsitoProgetto {
    std::string risposta;
    std::vector<std::string> fatte, scartate;
    int apriMappa = -1;
    bool modificato = false;
    std::string sezione, nome;      // elemento da mostrare (l'ultimo creato o modificato)
};

EsitoProgetto ApplicaRispostaProgetto(ProgettoGioco& p, int mappaAperta, const std::string& rispostaClaude);

// Un disegno in righe di caratteri con la sua tavolozza (come lo scrive Claude).
std::string DisegnoInRighe(const Disegno& d, int maxLato);
// Raddoppia un disegno in pixel art per lo stile disegnato HD (bordi morbidi, densita' doppia).
void DisegnoInHD(Disegno& d);
