// Importazione dei file MIDI (.mid, .midi): le note diventano un brano del rullo di pianoforte,
// modificabile nota per nota e suonato dal sintetizzatore dello studio (anche negli eseguibili).
#pragma once
#include "progetto_gioco.h"
#include <string>

// Legge un file MIDI standard (formato 0 o 1). Fino a 6 tracce: la batteria (canale 10) e le
// 5 parti con piu' note; ogni strumento General MIDI diventa l'onda piu' simile.
bool BranoDaMidi(const std::string& dati, Brano& b, std::string& errore);
