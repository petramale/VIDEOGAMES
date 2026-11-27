// Salvataggio e caricamento delle partite (pilastro 3).
//
// Un salvataggio e' un file di testo: una riga di intestazione, una riga con il codice di controllo
// (CRC-32) e la lunghezza, poi il JSON con lo stato completo della partita:
//
//     ELVANTIS-SALVATAGGIO 1
//     CRC32 7a3f09c1 BYTE 18342
//     { "formato": "elvantis-salvataggio", "versione": 1, "gioco": {...}, "destino": {...}, "partita": {...}, ... }
//
// - Scrittura atomica: si scrive un file .tmp e lo si rinomina solo alla fine; il salvataggio
//   precedente diventa la copia di riserva .bak.
// - Lettura: se il file e' troncato o modificato (CRC diverso) si prova la copia di riserva.
// - Versioni: i salvataggi vecchi passano per una catena di migrazioni; quelli di una versione
//   piu' nuova del gioco non si caricano (sarebbe un ripristino incoerente).
#pragma once
#include <cstdint>
#include <string>

constexpr int VERSIONE_SALVATAGGIO = 1;   // versione dei dati (sale quando cambia la struttura)

uint32_t Crc32(const std::string& dati);
std::string ImbustaSalvataggio(const std::string& json);
bool SbustaSalvataggio(const std::string& contenuto, std::string& json, std::string& errore);
// Porta il JSON di un salvataggio alla versione attuale. false se e' troppo nuovo o non e' un salvataggio.
bool MigraSalvataggio(std::string& json, std::string& errore, int* versioneLetta = nullptr);
// Scrive in modo atomico (file .tmp, poi rinomina; il vecchio diventa .bak).
bool ScriviAtomico(const std::string& percorso, const std::string& contenuto, std::string& errore);

struct InfoSalvataggio {
    bool esiste = false, valido = false, daRiserva = false;
    std::string errore;          // perche' non e' valido
    std::string titolo, mappa, data;
    int punteggio = 0, versione = 0, giocatori = 1;
    float tempo = 0;
    uint64_t impronta = 0;
};

// Legge e controlla un salvataggio (o la sua riserva); 'json' esce gia' migrato alla versione attuale.
bool LeggiSalvataggio(const std::string& percorso, std::string& json, InfoSalvataggio& info);
InfoSalvataggio InfoDiSalvataggio(const std::string& percorso);

// Il JSON completo: intestazione per l'elenco degli slot + lo stato del gioco (Gioco::StatoJson).
struct MetaSalvataggio {
    std::string titolo, mappa;
    uint64_t impronta = 0;
    int punteggio = 0, giocatori = 1, turno = 0, punti[2] = { 0, 0 };
    float tempo = 0;
};
std::string ComponiSalvataggio(const std::string& statoGioco, const MetaSalvataggio& meta);
// Legge l'intestazione (giocatori, turno, punti) da un JSON di salvataggio.
MetaSalvataggio MetaDi(const std::string& json);
std::string DataOra();   // "27/11/2026 08:15"
