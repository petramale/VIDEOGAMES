// Immagini importate (PNG, JPG...) dentro il gioco: sfondi, strati, decorazioni.
// I file stanno nel pacchetto del gioco; le texture si creano la prima volta che servono.
#pragma once
#include "comune.h"
#include "pacchetto.h"

// Imposta i file da cui leggere le immagini (sostituisce i precedenti e svuota la cache).
void ImpostaFileImmagini(const std::vector<FileIncluso>& file);
// Aggiunge o sostituisce un file (es. un'immagine appena importata nello studio).
void AggiungiFileImmagine(const std::string& nome, const std::string& dati);
// La texture dell'immagine con quel nome; id 0 se non esiste o non si legge.
Texture2D Immagine(const std::string& nome);
void LiberaImmagini();
