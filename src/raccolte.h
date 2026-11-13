// Le raccolte pronte (piu' di 20): personaggi, oggetti, caselle, scenari, musiche ed effetti
// gia' fatti, da aggiungere alla libreria del gioco con un clic. Tutto e' generato dal codice
// (nessun file esterno), sia in pixel art fine sia in grafica disegnata ad alta definizione.
#pragma once
#include "progetto_gioco.h"

struct Raccolta {
    std::string nome, descrizione;
    std::vector<PersonaggioDef> personaggi;
    std::vector<OggettoDef> oggetti;
    std::vector<Scenario> scenari;
    std::vector<CasellaDef> caselle;
    std::vector<Brano> brani;
    std::vector<EffettoSuono> effetti;
    int Totale() const { return (int)(personaggi.size() + oggetti.size() + scenari.size() + caselle.size() + brani.size() + effetti.size()); }
};

int NumeroRaccolte();
const char* NomeRaccolta(int i);
const char* DescrizioneRaccolta(int i);
int IndiceRaccolta(const std::string& nome);   // 0 se non c'e'
// Generata la prima volta che serve, poi tenuta in memoria.
const Raccolta& CreaRaccolta(int i, StileGrafico stile);

// Un brano composto automaticamente (usato dalle raccolte e da "Nuovo brano" con uno stile).
//   stile: 0 allegro, 1 epico, 2 calmo, 3 misterioso, 4 spaziale, 5 battaglia
Brano ComponiBrano(const std::string& nome, int seme, int stile, int battute = 8);
const std::vector<std::string>& StiliBrano();
// Effetti sonori pronti.
std::vector<EffettoSuono> EffettiArcade();
EffettoSuono EffettoCasuale(int seme);
