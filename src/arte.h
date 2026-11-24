// Arte vettoriale dettagliata per i giochi d'esempio (e per lo Studio): personaggi animati,
// paesaggi a strati, materiali del terreno. Tutto e' disegnato con il Pennello (pennello.h)
// e trasformato in fotogrammi e immagini della libreria del gioco.
#pragma once
#include "pennello.h"
#include "progetto_gioco.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace arte {

using pennello::Tela;
using pennello::Vernice;

// Un "contesto" in stile canvas sopra la Tela: rende il disegno facile da leggere e scrivere.
struct K {
    Tela& t;
    explicit K(Tela& tela) : t(tela) {}
    void save() { t.Salva(); }
    void restore() { t.Ripristina(); }
    void translate(float x, float y) { t.Trasla(x, y); }
    void scale(float x, float y) { t.Scala(x, y); }
    void rotate(float a) { t.Ruota(a); }
    void beginPath() { t.Inizia(); }
    void moveTo(float x, float y) { t.Muovi(x, y); }
    void lineTo(float x, float y) { t.Linea(x, y); }
    void quadraticCurveTo(float a, float b, float x, float y) { t.Quadratica(a, b, x, y); }
    void bezierCurveTo(float a, float b, float c, float d, float x, float y) { t.Bezier(a, b, c, d, x, y); }
    void arc(float x, float y, float r, float a0, float a1, bool ccw = false) { t.Arco(x, y, r, a0, a1, ccw); }
    void ellipse(float x, float y, float rx, float ry, float rot, float a0, float a1, bool ccw = false) { t.Ellisse(x, y, rx, ry, rot, a0, a1, ccw); }
    void rect(float x, float y, float w, float h) { t.Rettangolo(x, y, w, h); }
    void closePath() { t.Chiudi(); }
    void fill() { t.Riempi(); }
    void stroke() { t.Contorna(); }
    void clip() { t.Ritaglia(); }
    void fillRect(float x, float y, float w, float h) { t.RiempiRettangolo(x, y, w, h); }
    void strokeRect(float x, float y, float w, float h) { t.Inizia(); t.Rettangolo(x, y, w, h); t.Contorna(); }
    void fillStyle(const Vernice& v) { t.riempimento = v; }
    void strokeStyle(const Vernice& v) { t.tratto = v; }
    void lineWidth(float w) { t.spessore = w; }
    void alpha(float a) { t.alfaGlobale = a; }
    void lighter() { t.modo = pennello::Modo::Somma; }
    void erase() { t.modo = pennello::Modo::Cancella; }
    void normal() { t.modo = pennello::Modo::Sopra; }
    void glow(float blur, const std::string& col);
    void noGlow() { t.bagliore = 0; }
};

// Colori come testo, come nei browser: "#rrggbb", "#rgb", "rgba(r,g,b,a)".
pennello::Colore Col(const std::string& s);
std::string sh(const std::string& hex, float k);         // schiarisce (k>0) o scurisce (k<0)
std::string rgba(const std::string& hex, float a);
std::string mixc(const std::string& a, const std::string& b, float k);
Vernice V(const std::string& s);
Vernice lg(float x0, float y0, float x1, float y1, std::initializer_list<std::string> colori);
Vernice lgs(float x0, float y0, float x1, float y1, std::initializer_list<std::pair<float, std::string>> fermate);
Vernice rg(float x, float y, float r0, float r1, std::initializer_list<std::string> colori);
Vernice rgs(float x, float y, float r0, float r1, std::initializer_list<std::pair<float, std::string>> fermate);

// --- Posa: quale animazione e in che istante -------------------------------------------------
enum class Anim { Fermo, Corsa, Salto, Caduta, Attacco, Colpito, Vittoria };
struct Posa { Anim a = Anim::Fermo; float t = 0; float ph = 0; };

// Un personaggio disegnabile: piedi in (x, y), scala s.
using DisegnaPersonaggio = std::function<void(K& c, float x, float y, float s, const Posa& p)>;

// Specifica di un personaggio umanoide (eroi, mummie, pirati, cavalieri, maghi...).
struct Umanoide {
    std::string skin = "#f6cfa8", hair, hairStyle, eye = "#3a7ad8", top = "#3a7ad8", bottom, shoes = "#3a2a2a";
    std::string cloak, belt, collar, overall, scarf, emblem, accent, capc, helmet, plume, glow, hat, hatc, trim, orb, item, face;
    bool lash = false, grin = false, brows = false, goggles = false, bag = false, robe = false, beard = false, crown = false, bandage = false;
    bool flip = false, upside = false;
};
void DisegnaUmanoide(K& c, float x, float y, float s, const Umanoide& o, const Posa& p);

struct Robot { std::string metal = "#9aa4bc", core = "#5af0ff", eye = "#5af0ff", ant = "#ff6a3a", fist = "#f28a2a"; bool flip = false; };
void DisegnaRobot(K& c, float x, float y, float s, const Robot& o, const Posa& p);
void DisegnaPinguino(K& c, float x, float y, float s, const std::string& sciarpa, const Posa& p);
void DisegnaLucciola(K& c, float x, float y, float s, const Posa& p);
void DisegnaOmbra(K& c, float x, float y, float s, const std::string& col, const std::string& aura, const std::string& occhi, const Posa& p);
void DisegnaPipistrello(K& c, float x, float y, float s, const std::string& ali, const std::string& corpo, const std::string& occhi, const Posa& p);
void DisegnaGolem(K& c, float x, float y, float s, const std::string& chiaro, const std::string& medio, const std::string& scuro, const std::string& luce, const Posa& p);
struct Dino { std::string c = "#4aa85a", belly = "#e8e0a0", stripe = "#2a6a3a"; bool flip = false; };
void DisegnaDino(K& c, float x, float y, float s, const Dino& o, const Posa& p);
void DisegnaDrone(K& c, float x, float y, float s, const std::string& col, const std::string& occhio, const Posa& p);
void DisegnaMedusa(K& c, float x, float y, float s, const std::string& col, const Posa& p);
void DisegnaApe(K& c, float x, float y, float s, const Posa& p);
void DisegnaMech(K& c, float x, float y, float s, const std::string& col, const Posa& p);

// Crea un Disegno con le animazioni standard del motore (fermo, corsa, salto, caduta, attacco,
// colpito, vittoria). 'altezza' = altezza del personaggio in pixel del mondo; il disegno ha
// densita' 'dens' (pixel del disegno per pixel del mondo). 'alto' e 'largo' = misure del
// personaggio nelle unita' della funzione di disegno (umanoide: circa 170 x 120).
struct OpzioniDisegno {
    float altezzaMondo = 40;       // pixel del mondo
    float dens = 4;
    bool liscio = true;
    float alto = 175, largo = 130; // ingombro nelle unita' della funzione (piedi in basso al centro)
    float sopra = 0;               // spazio extra sopra (oggetti sollevati, volo)
    std::vector<Anim> animazioni = { Anim::Fermo, Anim::Corsa, Anim::Salto, Anim::Caduta, Anim::Attacco, Anim::Colpito, Anim::Vittoria };
    int fotogrammiCorsa = 8, fotogrammiFermo = 4;
};
Disegno CreaDisegno(const DisegnaPersonaggio& f, const OpzioniDisegno& o);

}  // namespace arte
