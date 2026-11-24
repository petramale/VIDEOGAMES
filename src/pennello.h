// Pennello: un piccolo disegnatore vettoriale in C++ (stile "canvas" di un browser).
// Serve a creare grafica dettagliata e liscia (personaggi, sfondi, caselle) direttamente
// dentro lo Studio, senza programmi esterni. Disegna tracciati con curve, archi ed ellissi,
// riempiti con colori pieni o sfumature (lineari e radiali), contorni con estremi arrotondati,
// trasparenza, ritagli, bagliori e sfocature, con bordi antialias. Il risultato e' una
// Image di raylib, pronta da trasformare in fotogrammi o immagini della libreria del gioco.
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace pennello {

struct Colore { float r = 0, g = 0, b = 0, a = 1; };   // 0..1, non premoltiplicato
Colore Esa(const char* hex, float alfa = 1);           // "#rrggbb"
Colore Esa(unsigned rgb, float alfa = 1);              // 0xrrggbb
Colore Schiarisci(Colore c, float k);                  // k>0 verso il bianco, k<0 verso il nero
Colore Mescola(Colore a, Colore b, float k);
Colore ConAlfa(Colore c, float a);

struct Matrice { float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;   // x' = a*x + c*y + e, y' = b*x + d*y + f
    Vector2 Applica(float x, float y) const { return { a * x + c * y + e, b * x + d * y + f }; }
    float Scala() const; };

// Una sfumatura (o un colore pieno se 'tipo' == 0).
struct Vernice {
    int tipo = 0;                          // 0 pieno, 1 lineare, 2 radiale
    Colore pieno;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0, r0 = 0, r1 = 1;   // coordinate dell'utente
    std::vector<std::pair<float, Colore>> fermate;
    Vernice() = default;
    Vernice(Colore c) : pieno(c) {}
    Vernice(const char* hex) : pieno(Esa(hex)) {}
    Vernice& Ferma(float t, Colore c) { fermate.push_back({ t, c }); return *this; }
    Vernice& Ferma(float t, const char* hex) { return Ferma(t, Esa(hex)); }
};
Vernice Lineare(float x0, float y0, float x1, float y1, std::initializer_list<Colore> colori);
Vernice Radiale(float x, float y, float r0, float r1, std::initializer_list<Colore> colori);

enum class Modo { Sopra, Somma, Cancella };   // source-over, lighter, destination-out

class Tela {
public:
    Tela(int larghezza, int altezza);
    int L() const { return w; }
    int A() const { return h; }

    // Stato (salvato e ripristinato con Salva/Ripristina)
    void Salva();
    void Ripristina();
    void Trasla(float x, float y);
    void Scala(float sx, float sy);
    void Ruota(float a);
    void Imposta(float a, float b, float c, float d, float e, float f);
    Matrice matrice;
    float alfaGlobale = 1;
    Modo modo = Modo::Sopra;
    float spessore = 1;
    Vernice riempimento{ Colore{ 0, 0, 0, 1 } };
    Vernice tratto{ Colore{ 0, 0, 0, 1 } };
    float bagliore = 0;            // come shadowBlur, con l'ombra centrata (bagliore)
    Colore coloreBagliore{ 0, 0, 0, 0 };

    // Tracciati (in coordinate dell'utente, trasformati subito)
    void Inizia();
    void Muovi(float x, float y);
    void Linea(float x, float y);
    void Quadratica(float cx, float cy, float x, float y);
    void Bezier(float c1x, float c1y, float c2x, float c2y, float x, float y);
    void Arco(float x, float y, float r, float a0, float a1, bool antiorario = false);
    void Ellisse(float x, float y, float rx, float ry, float rot, float a0, float a1, bool antiorario = false);
    void Rettangolo(float x, float y, float l, float a);
    void Chiudi();

    void Riempi();
    void Contorna();
    void Ritaglia();               // interseca il ritaglio con il tracciato attuale
    void RiempiRettangolo(float x, float y, float l, float a);
    void Pulisci();

    // Effetti sull'intera tela
    void Sfoca(float raggio);
    void Disegna(const Tela& altra, float x, float y);   // incolla (con alfa e modo attuali, senza trasformazione)

    Image Immagine() const;        // RGBA8 non premoltiplicato
    Image Parte(int x, int y, int l, int a) const;

private:
    struct Stato { Matrice m; float alfa; Modo modo; float spessore; Vernice riemp, tratto; float bagliore; Colore cb; std::vector<float> ritaglio; bool haRitaglio; };
    int w, h;
    std::vector<float> px;         // RGBA premoltiplicato
    std::vector<std::vector<Vector2>> sotto;   // sottotracciati (pixel)
    std::vector<bool> chiuso;
    std::vector<Stato> pila;
    std::vector<float> ritaglio;   // copertura 0..1 per pixel (vuoto = nessun ritaglio)
    bool haRitaglio = false;
    Vector2 ultimo{ 0, 0 }, inizioSotto{ 0, 0 };
    bool haUltimo = false;

    void Aggiungi(Vector2 p);
    // copertura del tracciato (regola nonzero) dentro il riquadro bx,by,bw,bh
    void Copertura(const std::vector<std::vector<Vector2>>& poli, int& bx, int& by, int& bw, int& bh, std::vector<float>& cop) const;
    void Componi(const std::vector<float>& cop, int bx, int by, int bw, int bh, const Vernice& v);
    void ComponiBagliore(const std::vector<float>& cop, int bx, int by, int bw, int bh);
    Colore Campiona(const Vernice& v, float x, float y, const Matrice& inv) const;
    std::vector<std::vector<Vector2>> PoligoniTratto() const;
};

constexpr float PI2 = 6.28318530718f;

}  // namespace pennello
