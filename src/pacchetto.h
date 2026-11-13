// Costruttore dell'eseguibile (senza compilatore).
// Un gioco e' una copia dello studio con i dati del gioco attaccati in coda:
//   [eseguibile][dati][dimensione dati: 8 byte][firma "VGPAK001"]
// All'avvio l'eseguibile controlla la propria coda: se trova la firma, avvia il gioco.
#pragma once
#include <string>
#include <utility>
#include <vector>

// Un file incluso nel gioco (musica, effetto sonoro...): nome e contenuto binario.
using FileIncluso = std::pair<std::string, std::string>;

struct DatiGioco {
    std::string titolo;
    std::string livello;               // testo del progetto (impostazioni + mappa)
    std::vector<FileIncluso> file;     // es. "musica.ogg", "effetto_Salto.wav"
};

std::string SerializzaGioco(const DatiGioco& g);
bool DeserializzaGioco(const std::string& dati, DatiGioco& g);

bool LeggiPacchettoIncluso(std::string& dati);
bool CreaEseguibile(const std::string& destinazione, const std::string& dati, std::string& errore);
