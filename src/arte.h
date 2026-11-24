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

// =====================================================================================
// Paesaggi: primitive per gli strati dello sfondo (tutte in coordinate "logiche": lo schermo
// e' 1280x720, uno strato ripetibile e' largo LS = 1920). La Tela puo' avere una risoluzione
// diversa: chi chiama imposta la scala con K::scale prima di disegnare.
// =====================================================================================
namespace arte {
constexpr float LS = 1920, HS = 720;

struct Caso {   // numeri pseudo-casuali ripetibili
    unsigned s;
    explicit Caso(unsigned seme) : s(seme * 2654435761u + 12345u) { if (!s) s = 1; }
    float operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s % 1000003u) / 1000003.f; }
};
std::function<float(float)> RumorePeriodico(unsigned seme, int n, float caduta);
void Ripeti(float x, float w, const std::function<void(float)>& f);   // disegna anche a x-LS / x+LS ai bordi

struct Cresta {   // montagne e colline (ridge)
    unsigned seme = 1; float base = 400, amp = 150; int k = 36; float fall = 1.05f; bool punte = false, soffitto = false;
    std::string top, bot, light, shade, haze, strata; float hazeA = 0.55f, neve = 0, facet = 1; bool lava = false;
};
void DisegnaCresta(K& c, const Cresta& o);
void RiempiSotto(K& c, float y, const std::string& col, const std::string& col2 = "");
struct ColoriAlbero { std::string trunk, dark, mid, light, rim, spec, fruit; bool snow = false; };
void Pino(K& c, float x, float y, float h, const ColoriAlbero& o);
void AlberoTondo(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned seme);
void Palma(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned seme);
void Felce(K& c, float x, float y, float h, const ColoriAlbero& o, unsigned seme);
void Cactus(K& c, float x, float y, float h, const ColoriAlbero& o);
struct ColoriFungo { std::string stalk, stalkL, cap, dot; };
void Fungo(K& c, float x, float y, float h, const ColoriFungo& o, unsigned seme);
struct ColoriFiore { std::string stem, leaf, p1, p2; };
void Fiore(K& c, float x, float y, float h, const ColoriFiore& o, unsigned seme);
struct ColoriCristallo { std::string dark, mid, light, glow; };
void Cristalli(K& c, float x, float y, float h, const ColoriCristallo& o, unsigned seme);
struct ColoriEdificio { std::string wall, dark, win, win2, roof, roofc, neon, frame; float lit = 0.3f; bool chim = false, brick = false; float ww = 8, wh = 12, gx = 16, gy = 22; };
void Edificio(K& c, float x, float y, float w, float h, const ColoriEdificio& o, unsigned seme);
struct ColoriCastello { std::string stone, roof, flag, win, banner; };
void Castello(K& c, float x, float y, float s, const ColoriCastello& o);
void Piramide(K& c, float x, float y, float w, const std::string& chiaro, const std::string& scuro);
void Colonna(K& c, float x, float y, float h, float w, const std::string& col, const std::string& fascia);
void Ingranaggio(K& c, float x, float y, float r, const std::string& col, const std::string& buco, float rot);
void Tubi(K& c, float y0, const std::string& col, unsigned seme);
void Ciminiera(K& c, float x, float y, float h, float w, const std::string& col, const std::string& fascia);
void Mesa(K& c, float x, float y, float w, float h, const std::string& chiaro, const std::string& medio, const std::string& scuro, const std::string& fascia, const std::string& cima);
void Isola(K& c, float x, float y, float w, const std::string& terra, const std::string& erba, const ColoriAlbero& albero, unsigned seme);
void Corallo(K& c, float x, float y, float h, const std::string& col, unsigned seme);
void Alga(K& c, float x, float y, float h, const std::string& col, unsigned seme);
void Cubo(K& c, float x, float y, float s, const std::string& col);
struct ColoriNuvola { std::string base, light, shadow; };
void Nuvola(K& c, float x, float y, float w, const ColoriNuvola& o, unsigned seme);
void Nave(K& c, float x, float y, float s);
// fila di elementi lungo lo strato (alberi, funghi...): f(x, base, altezza, seme)
void Fila(unsigned seme, float base, float hmin, float hmax, float passo, const std::function<void(float, float, float, unsigned)>& f);

// Cielo fisso (immagine 1280x720 logici): sfumatura, sole/luna/eclissi, stelle, aurora.
struct Cielo {
    std::vector<std::string> fermate;
    bool stelle = false, aurora = false, raggi = false;
    float sole[3] = { 0, 0, 0 }; std::string coloreSole;
    float luna[3] = { 0, 0, 0 }; std::string coloreLuna;
    float eclissi[3] = { 0, 0, 0 };
    std::vector<std::pair<float, ColoriNuvola>> nuvole;   // y, colori (sparse sul cielo)
};
void DisegnaCielo(K& c, const Cielo& s);

// Materiali del terreno: disegnati in una striscia (x0..x0+w, da y0 in giu').
struct Materiale {
    std::string kind = "grass", body = "#7a4a2a", top = "#6aa83a", glow, moss, stripe1, stripe2, neon;
    std::vector<std::string> blades, flowers, cols;
};
void DisegnaMateriale(K& c, float x0, float w, float y0, float h, const Materiale& m, unsigned seme);
// Primo piano scuro e sfocato (foglie, rocce, spine, pali)
void PrimoPiano(K& c, const std::string& col, const std::string& tipo);
}  // namespace arte
