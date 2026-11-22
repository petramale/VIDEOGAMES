// Categoria 6: gestione delle librerie.
// Elenco delle librerie open-source incluse nel progetto, con versione, licenza e uso.
// Lo stesso elenco e' in librerie/librerie.json, per chi lavora sui file.
#pragma once
#include <vector>

struct InfoLibreria {
    const char* nome;
    const char* versione;
    const char* licenza;
    const char* uso;
    const char* cartella;
};

const std::vector<InfoLibreria>& Librerie();
