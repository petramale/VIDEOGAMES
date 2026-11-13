// Claude nel nuovo editor: gli si manda lo stato del mondo in JSON e lui risponde
// con un messaggio e un elenco di azioni (cambia la mappa, aggiunge oggetti, strati...).
// Le azioni si applicano a una copia, si controllano e solo allora entrano nel mondo.
#pragma once
#include "mondo.h"
#include <string>
#include <vector>

std::string PromptMondo();
// Stato da mandare a Claude: il mondo intero (compatto) e quale mappa e' aperta.
std::string StatoMondoPerClaude(const Mondo& m, int mappaAperta);

struct EsitoClaude {
    std::string risposta;                 // il messaggio di Claude
    std::vector<std::string> fatte;       // descrizione delle modifiche applicate
    std::vector<std::string> scartate;    // azioni non valide (con il motivo)
    int apriMappa = -1;                   // Claude chiede di mostrare questa mappa
    bool modificato = false;
};

// jsonRisposta: {"risposta": "...", "azioni": [...]}. Non lancia eccezioni.
EsitoClaude ApplicaRispostaClaude(Mondo& mondo, int mappaAperta, const std::string& jsonRisposta);
