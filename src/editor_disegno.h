// Editor di disegno e animazione (personaggi, oggetti, caselle).
// Matita, gomma, secchiello, linea, rettangolo, ellisse, contagocce, sposta, specchio;
// colori con selettore HSV e tavolozza; animazioni con piu' fotogrammi, velocita',
// anteprima, "buccia di cipolla", annulla e ripeti, importazione di PNG e fogli di sprite.
// Si disegna a risoluzione piena dentro un rettangolo dato dall'officina.
#pragma once
#include "comune.h"
#include "progetto_gioco.h"
#include <string>
#include <vector>

class EditorDisegno {
public:
    // Apre un disegno. 'id' distingue i disegni: se cambia, la storia (annulla) riparte.
    void Apri(Disegno* d, const std::string& id);
    void Chiudi() { d = nullptr; id.clear(); }
    // Disegna e gestisce l'editor dentro r. Restituisce true se il disegno e' cambiato.
    bool Frame(Rectangle r, float dt);
    // Importa un'immagine come fotogrammi dell'animazione aperta (un foglio di sprite si divide da solo).
    bool Importa(const std::string& file, std::string& messaggio);
    bool EsportaPng(const std::string& file) const;
    int Animazione() const { return anim; }
    const std::string& Id() const { return id; }

private:
    enum Attrezzo { Matita, Gomma, Secchio, Linea, Rettangolo, Ellisse, Contagocce, Sposta, NumAttrezzi };
    struct Stato { int w, h; std::vector<::Animazione> animazioni; };

    Disegno* d = nullptr;
    std::string id;
    int anim = 0, fot = 0;
    int attrezzo = Matita, pennello = 1;
    bool pieno = false, specchio = false, cipolla = true, griglia = true, riproduci = true;
    Color colore = Hex(0x2a2a3a), secondo = BLANK;
    float hue = 0, sat = 0, val = 0.2f;
    std::vector<Color> tavolozza;
    std::vector<Stato> annulla, ripeti;
    bool tratto = false, tastoDestro = false;
    int x0 = 0, y0 = 0, ux = -1, uy = -1;
    Fotogramma primaDelTratto;
    float zoom = 8, panX = 0, panY = 0;
    bool trascina = false;
    Vector2 trascinaDa{ 0, 0 }, panDa{ 0, 0 };
    float tAnteprima = 0, scorriFotogrammi = 0, scorriAnimazioni = 0;
    Texture2D tela{};
    int telaW = 0, telaH = 0;
    bool cambiato = false;
    std::string nomeBozza;
    int nuovaW = 32, nuovaH = 32;
    bool adattaVista = true;
    Rectangle rTela{ 0, 0, 1, 1 };

    Fotogramma& F();
    void Ricorda();
    void Annulla();
    void Ripeti();
    void AggiornaColoreDaHsv();
    void ColoreScelto(Color c);
    void RaccogliTavolozza();
    void Punto(Fotogramma& f, int x, int y, Color c);
    void LineaPixel(Fotogramma& f, int ax, int ay, int bx, int by, Color c);
    void Forma(Fotogramma& f, int ax, int ay, int bx, int by, Color c);
    void Riempi(Fotogramma& f, int x, int y, Color c);
    void Tela(Rectangle r, float dt);
    void Attrezzi(Rectangle r);
    void Colori(Rectangle r);
    void Fotogrammi(Rectangle r);
    void Animazioni(Rectangle r);
    void Intestazione(Rectangle r);
};
