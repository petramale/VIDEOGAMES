// Sprite in "pixel art testuale": una palette e righe di caratteri.
// E' il formato che Claude usa per disegnare personaggi e oggetti (categoria 7),
// ed e' comodo anche da scrivere a mano.
//
// Esempio:
// { "nome": "fungo", "palette": { "r": "#d8344a", "w": "#ffffff" },
//   "fotogrammi": [ ["..rr..", ".rwwr.", "rrrrrr"] ], "fps": 4 }
#pragma once
#include "comune.h"

struct SpritePixel {
    std::string nome;
    std::vector<Texture2D> fotogrammi;
    float fps = 4;
    int larghezza = 0, altezza = 0;

    bool CaricaDaJson(const std::string& testo, std::string& errore);
    void Scarica();
    void Disegna(Vector2 centro, float scala, float tempo, bool specchia = false) const;
};
