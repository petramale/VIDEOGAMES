// STELLARIA: l'Invasione dei Prismi - sparatutto a ondate (genere "invasori dallo spazio").
// Gioco originale di Mario Petramale - Elvantis. 30 ondate in 6 cieli, barriere a mattoncini che si
// consumano a ogni colpo, disco bonus, combo di colore, potenziamenti, eventi che cambiano ogni partita
// e alieni che imparano dove ti metti di solito.
#include "arcade.h"
#include "flusso.h"
#include "json.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <fstream>

using nlohmann::json;

namespace arcade {
namespace {
using namespace arte;

constexpr float TAU = 6.2831853f;
constexpr int RIGHE = 5, COLONNE = 11;
constexpr float PASSO_X = 112, PASSO_Y = 76;
constexpr float Y_GIOCATORE = 975;
constexpr int CB = 8;                       // lato di un mattoncino delle barriere
constexpr int BW = 24, BH = 17;             // barriera: 24 x 17 mattoncini
constexpr float Y_BARRIERE = 790;
constexpr int ONDATE = 30;

const char* NOMI_CIELI[6] = { "Nebulosa Rosa", "Aurora Boreale", "Tempesta Ionica", "Anelli di Saturnia", "Corona del Sole", "Orizzonte Oscuro" };
// colori delle cinque specie di alieni (chiaro, scuro)
const unsigned COL_ALIENI[5][2] = { { 0xff4fd8, 0x8a0b72 }, { 0xff9a3c, 0x8a3a07 }, { 0x3cf0ff, 0x075a7a }, { 0x8dff4a, 0x2e6e08 }, { 0xffe34a, 0x7a5a05 } };
const unsigned COL_BARRIERE[6] = { 0x3cff9a, 0xffc84a, 0x6ae0ff, 0x7affc8, 0x7ad0ff, 0xff7ad8 };   // una tinta per cielo
const int PUNTI_SPECIE[5] = { 400, 300, 200, 200, 100 };

std::string HexS(unsigned c) { char b[8]; std::snprintf(b, sizeof b, "#%06x", c); return b; }

struct Alieno { float ox, oy; int specie; int vita; bool vivo; float lampo = 0; };
struct Colpo { Vector2 p; float vy; int tipo; bool amico; bool laser = false; float t = 0; };
struct Capsula { Vector2 p; int tipo; };   // 0 triplo, 1 laser, 2 scudo, 3 rapido, 4 vita
struct Meteora { Vector2 p, v; float r, rot; int vita; };

// risorse condivise da tutte le partite (gioco e vetrina del menu): si dipingono una volta sola
Texture2D cieli[6]{}, alieniTex[5][2]{}, cannone{}, disco{}, capsTex[5]{}, meteoraTex{};
Brano brani[7];

class Stellaria : public GiocoArcade {
public:
    const char* Nome() const override { return "stellaria"; }
    std::string Titolo() const override { return "Stellaria: l'Invasione dei Prismi"; }
    std::string Tappa() const override { return "Ondata " + std::to_string(ondata) + " - " + NOMI_CIELI[Cielo()]; }
    std::vector<std::string> Aiuto() const override {
        return { "Frecce, A/D, levetta o mouse: muovi il cannone", "Spazio, J, clic sinistro o pulsante A: spara",
                 "Colpisci alieni dello stesso colore di fila per la COMBO", "Le capsule danno colpo triplo, laser, scudo, fuoco rapido o una vita" };
    }
    Brano MusicaMenu() const override { return brani[6]; }

    // --- stato della partita ---
    Caso caso;
    uint64_t seme = 1;
    Regista regista;
    int ondata = 1, vite = 3, prossimaVita = 20000, record = 0;
    std::vector<Alieno> alieni;
    float formX = 0, formY = 0, dir = 1, tPasso = 0, intervallo = 0.6f;
    int fotogramma = 0, nota = 0;
    float discesaIngresso = 0;            // gli alieni entrano dall'alto
    std::vector<Colpo> colpi;
    std::vector<Capsula> capsule;
    std::vector<Meteora> meteore;
    unsigned char barriere[4][BH][BW];
    float px = 960, tRicarica = 0, tInvulnerabile = 0, tMorto = 0;
    int arma = 0; float tArma = 0;        // 0 normale, 1 triplo, 2 laser, 3 rapido
    bool scudo = false;
    float tDisco = 12, discoX = -999, discoDir = 1; bool discoVivo = false; float tSirena = 0;
    int comboSpecie = -1, combo = 0; float tCombo = 0;
    float tOndata = 0, tScritta = 0, tFineOndata = -1;
    int evento = 0;                       // 0 niente, 1 meteore, 2 corazzati, 3 disco doppio, 4 alieni veloci
    float tMeteore = 0;
    std::string scritta, sottoScritta;
    float tScosse = 0;
    Particelle part;
    struct Testo { Vector2 p; std::string t; float vita; Color c; };
    std::vector<Testo> testi;


    int Cielo() const { return std::min(5, (ondata - 1) / 5); }

    void Prepara() override {
        PreparaGrafica();
        PreparaSuoni();
        PreparaMusica();
    }

    // ======================= GRAFICA (Pennello) =======================
    void PreparaGrafica() {
        if (cannone.id) return;
        const unsigned nebula[6][4] = {
            { 0x07021a, 0x1a0b3d, 0x3b0d4f, 0xff3cb4 }, { 0x021018, 0x063040, 0x0b4a4a, 0x3cffb0 }, { 0x040414, 0x101040, 0x2a1050, 0x8a6aff },
            { 0x0a0610, 0x2a1820, 0x4a2a20, 0xffb06a }, { 0x120400, 0x401008, 0x6a2a08, 0xffd040 }, { 0x000000, 0x08040e, 0x140820, 0xb04aff } };
        for (int k = 0; k < 6; k++) {
            cieli[k] = Dipingi(1280, 720, [&](K& c) {
                Caso r; r.Semina(100 + k);
                c.fillStyle(lg(0, 0, 0, 720, { HexS(nebula[k][0]), HexS(nebula[k][1]), HexS(nebula[k][2]) }));
                c.fillRect(0, 0, 1280, 720);
                // nebulose morbide
                for (int i = 0; i < 7; i++) {
                    float x = r.F(0, 1280), y = r.F(0, 600), rad = r.F(160, 420);
                    unsigned col = i % 2 ? nebula[k][3] : 0x4a8aff;
                    c.lighter();
                    c.fillStyle(rgs(x, y, 0, rad, { { 0, rgba(HexS(col), 0.20f) }, { 0.5f, rgba(HexS(col), 0.08f) }, { 1, rgba(HexS(col), 0) } }));
                    c.beginPath(); c.arc(x, y, rad, 0, TAU); c.fill();
                    c.normal();
                }
                // filamenti di polvere
                for (int i = 0; i < 160; i++) {
                    float x = r.F(0, 1280), y = r.F(0, 620);
                    c.fillStyle(V(rgba(HexS(nebula[k][3]), r.F(0.02f, 0.06f))));
                    c.beginPath(); c.ellipse(x, y, r.F(20, 90), r.F(4, 14), r.F(-0.6f, 0.6f), 0, TAU); c.fill();
                }
                // stelle fisse
                for (int i = 0; i < 420; i++) {
                    float x = r.F(0, 1280), y = r.F(0, 700), rr = r.F(0.4f, 1.6f);
                    c.fillStyle(V(rgba("#ffffff", r.F(0.3f, 0.95f)))); c.beginPath(); c.arc(x, y, rr, 0, TAU); c.fill();
                }
                // un pianeta diverso per ogni cielo
                float qx = k % 2 ? 1080 : 200, qy = 120 + k * 8, qr = 60 + k * 10;
                const char* pc[6][3] = { { "#ffb06b", "#d6406a", "#3a0d3e" }, { "#b0fff0", "#3aa0a0", "#0a3040" }, { "#c8b0ff", "#6a4ad0", "#1a1040" },
                                         { "#ffe0a0", "#c08040", "#40200a" }, { "#fff6b0", "#ff8a20", "#802000" }, { "#2a1040", "#0a0010", "#000000" } };
                if (k == 5) {   // buco nero con disco luminoso
                    c.lighter();
                    c.fillStyle(rgs(qx, qy + 40, 0, 260, { { 0, "rgba(255,160,255,0.35)" }, { 1, "rgba(120,40,200,0)" } }));
                    c.beginPath(); c.ellipse(qx, qy + 40, 260, 60, -0.2f, 0, TAU); c.fill();
                    c.normal();
                    c.fillStyle(V("#000000")); c.beginPath(); c.arc(qx, qy + 40, 70, 0, TAU); c.fill();
                } else {
                    c.fillStyle(rgs(qx - qr * 0.3f, qy - qr * 0.3f, qr * 0.1f, qr * 1.1f, { { 0, pc[k][0] }, { 0.6f, pc[k][1] }, { 1, pc[k][2] } }));
                    c.beginPath(); c.arc(qx, qy, qr, 0, TAU); c.fill();
                    if (k == 3 || k == 0) { c.strokeStyle(V(rgba("#ffd0a0", 0.6f))); c.lineWidth(5); c.beginPath(); c.ellipse(qx, qy, qr * 1.8f, qr * 0.35f, -0.25f, 0, TAU); c.stroke(); }
                }
            });
        }
        // alieni: cinque specie, due fotogrammi, disegnati in grande e rimpiccioliti (bordi netti)
        for (int sp = 0; sp < 5; sp++)
            for (int f = 0; f < 2; f++)
                alieniTex[sp][f] = Dipingi(176, 136, [&](K& c) { c.translate(88, 70); c.scale(2, 2); DisegnaAlieno(c, sp, f); });
        cannone = Dipingi(260, 200, [&](K& c) { c.translate(130, 110); c.scale(1.8f, 1.8f); DisegnaCannone(c); });
        disco = Dipingi(320, 140, [&](K& c) { c.translate(160, 80); c.scale(1.2f, 1.2f); DisegnaDisco(c); });
        const char* capCol[5] = { "#ffe14a", "#ff5a5a", "#5ac8ff", "#8aff6a", "#ff7ad0" };
        const char* capLet[5] = { "3", "L", "S", "R", "+" };
        for (int i = 0; i < 5; i++)
            capsTex[i] = Dipingi(96, 96, [&](K& c) {
                c.glow(14, capCol[i]);
                c.fillStyle(lg(0, 20, 0, 76, { "#ffffff", capCol[i], sh(capCol[i], -0.5f) }));
                c.beginPath(); c.ellipse(48, 48, 30, 26, 0, 0, TAU); c.fill(); c.noGlow();
                c.strokeStyle(V("#ffffff")); c.lineWidth(3); c.stroke();
                (void)capLet;
            });
        meteoraTex = Dipingi(128, 128, [&](K& c) {
            c.fillStyle(rg(54, 54, 4, 56, { "#c8a080", "#7a5a48", "#3a2820" }));
            c.beginPath();
            for (int i = 0; i < 12; i++) { float a = i * TAU / 12, rr = 46 + std::sin(i * 2.3f) * 8; c.lineTo(64 + std::cos(a) * rr, 64 + std::sin(a) * rr); }
            c.closePath(); c.fill();
            for (int i = 0; i < 6; i++) { c.fillStyle(V("rgba(40,25,15,0.45)")); c.beginPath(); c.arc(50 + std::cos(i * 1.7f) * 22, 60 + std::sin(i * 2.1f) * 20, 6 + i % 3 * 3, 0, TAU); c.fill(); }
        });
    }

    static void DisegnaAlieno(K& c, int sp, int f) {
        std::string a = HexS(COL_ALIENI[sp][0]), b = HexS(COL_ALIENI[sp][1]);
        // sagome spigolose senza gambe; il fotogramma 1 apre le "ali" o le punte
        static const float P[5][16][2] = {
            { { -12, -30 }, { 12, -30 }, { 30, -12 }, { 30, 8 }, { 18, 8 }, { 18, 22 }, { 6, 22 }, { 6, 10 }, { -6, 10 }, { -6, 22 }, { -18, 22 }, { -18, 8 }, { -30, 8 }, { -30, -12 }, { -12, -30 }, { -12, -30 } },
            { { 0, -32 }, { 34, -6 }, { 34, 10 }, { 22, 24 }, { 10, 10 }, { -10, 10 }, { -22, 24 }, { -34, 10 }, { -34, -6 }, { 0, -32 }, { 0, -32 }, { 0, -32 }, { 0, -32 }, { 0, -32 }, { 0, -32 }, { 0, -32 } },
            { { -24, -26 }, { 24, -26 }, { 38, -8 }, { 38, 6 }, { 26, 6 }, { 32, 22 }, { 16, 22 }, { 10, 8 }, { -10, 8 }, { -16, 22 }, { -32, 22 }, { -26, 6 }, { -38, 6 }, { -38, -8 }, { -24, -26 }, { -24, -26 } },
            { { -8, -32 }, { 8, -32 }, { 8, -18 }, { 34, -18 }, { 34, 4 }, { 22, 22 }, { -22, 22 }, { -34, 4 }, { -34, -18 }, { -8, -18 }, { -8, -32 }, { -8, -32 }, { -8, -32 }, { -8, -32 }, { -8, -32 }, { -8, -32 } },
            { { 0, -34 }, { 16, -14 }, { 36, -14 }, { 22, 4 }, { 28, 26 }, { 0, 12 }, { -28, 26 }, { -22, 4 }, { -36, -14 }, { -16, -14 }, { 0, -34 }, { 0, -34 }, { 0, -34 }, { 0, -34 }, { 0, -34 }, { 0, -34 } } };
        float apri = f ? 1.12f : 1.0f;
        c.glow(16, rgba(a, 0.8f));
        c.fillStyle(lg(0, -32, 0, 28, { "#ffffff", a, b }));
        c.beginPath();
        for (int i = 0; i < 16; i++) c.lineTo(P[sp][i][0] * (std::fabs(P[sp][i][1]) > 10 ? apri : 1), P[sp][i][1]);
        c.closePath(); c.fill(); c.noGlow();
        c.strokeStyle(V(rgba("#ffffff", 0.8f))); c.lineWidth(1.6f); c.stroke();
        // sfaccettature del cristallo
        c.fillStyle(V(rgba("#ffffff", 0.35f)));
        c.beginPath(); c.moveTo(-20, -20); c.lineTo(-4, -24); c.lineTo(-16, -12); c.closePath(); c.fill();
        c.fillStyle(V(rgba(b, 0.5f)));
        c.beginPath(); c.moveTo(10, 4); c.lineTo(26, -6); c.lineTo(24, 6); c.closePath(); c.fill();
        // occhi quadrati che si spostano
        float o = f ? 2 : -2;
        c.fillStyle(V("#120420")); c.fillRect(-17 + o, -12, 11, 11); c.fillRect(6 + o, -12, 11, 11);
        c.fillStyle(V("#ffffff")); c.fillRect(-14 + o, -10, 4, 4); c.fillRect(9 + o, -10, 4, 4);
    }
    static void DisegnaCannone(K& c) {
        c.glow(22, "rgba(80,255,160,0.8)");
        c.fillStyle(lg(0, -56, 0, 30, { "#eaffef", "#5cff9a", "#0a7a4a" }));
        c.beginPath();
        const float P[][2] = { { -4, -56 }, { 4, -56 }, { 6, -40 }, { 14, -40 }, { 14, -22 }, { 50, -14 }, { 62, -4 }, { 62, 28 }, { -62, 28 }, { -62, -4 }, { -50, -14 }, { -14, -22 }, { -14, -40 }, { -6, -40 } };
        for (auto& p : P) c.lineTo(p[0], p[1]);
        c.closePath(); c.fill(); c.noGlow();
        c.strokeStyle(V("#eafff2")); c.lineWidth(2.5f); c.stroke();
        c.fillStyle(V("rgba(255,255,255,0.35)")); c.fillRect(-56, -6, 112, 6);
        c.fillStyle(V("#0b3a2a")); c.fillRect(-46, 8, 92, 10);
        for (int i = 0; i < 6; i++) { c.fillStyle(V(i % 2 ? "#77ffff" : "#ffe14a")); c.fillRect(-42 + i * 15, 10, 9, 6); }
        c.fillStyle(V("#ffffff")); c.fillRect(-3, -62, 6, 10);
    }
    static void DisegnaDisco(K& c) {
        c.glow(30, "rgba(255,60,60,0.7)");
        c.fillStyle(lg(0, -40, 0, 0, { "#bff6ff", "#3a8ab0" }));
        c.beginPath(); c.ellipse(0, -14, 46, 34, 0, 3.14159f, 0); c.fill();
        c.fillStyle(lg(0, -14, 0, 24, { "#ff8a8a", "#e0202a", "#5a0010" }));
        c.beginPath(); c.moveTo(-120, 4); c.lineTo(-70, -16); c.lineTo(70, -16); c.lineTo(120, 4); c.lineTo(80, 22); c.lineTo(-80, 22); c.closePath(); c.fill(); c.noGlow();
        c.strokeStyle(V("#ffd0d0")); c.lineWidth(2); c.stroke();
        c.fillStyle(V("rgba(255,255,255,0.5)")); c.beginPath(); c.ellipse(-14, -28, 14, 7, -0.3f, 0, TAU); c.fill();
    }

    // ======================= SUONI =======================
    void PreparaSuoni() {
        CreaSuono("st_sparo", 0.22f, [](float t) { float f = 1500 * std::exp(-t * 14) + 260; return 0.45f * Onda(0, f * t * 1.0f) * Inviluppo(t, 0.002f, 0.22f, 1.5f) + 0.15f * Rumore() * Inviluppo(t, 0, 0.03f); });
        CreaSuono("st_laser", 0.5f, [](float t) { return 0.35f * Onda(2, (900 + 300 * std::sin(t * 60)) * t) * Inviluppo(t, 0.01f, 0.5f, 1); });
        CreaSuono("st_alieno", 0.45f, [](float t) { static float y = 0; float n = Rumore(); y += (n - y) * (0.6f - t); float f = 420 * std::exp(-t * 6); return 0.55f * y * Inviluppo(t, 0, 0.45f, 2) + 0.3f * Onda(0, f * t) * Inviluppo(t, 0, 0.18f, 2); });
        CreaSuono("st_esplosione", 1.4f, [](float t) { static float y = 0; float n = Rumore(); y += (n - y) * 0.18f; return 0.9f * y * Inviluppo(t, 0.005f, 1.4f, 2.2f) + 0.4f * Onda(3, (90 * std::exp(-t * 2)) * t) * Inviluppo(t, 0, 0.8f, 2); });
        const float passi[4] = { 98, 87.3f, 77.8f, 73.4f };
        for (int i = 0; i < 4; i++) {
            float f = passi[i];
            CreaSuono("st_passo" + std::to_string(i), 0.16f, [f](float t) { return 0.75f * (0.7f * Onda(0, f * t) + 0.3f * Onda(1, f * 2 * t)) * Inviluppo(t, 0.003f, 0.16f, 1.6f); });
        }
        CreaSuono("st_sirena", 0.5f, [](float t) { float f = 760 + 240 * std::sin(t * TAU * 8); return 0.25f * Onda(1, f * t) * Inviluppo(t, 0.02f, 0.5f, 0.3f); });
        CreaSuono("st_disco", 0.9f, [](float t) { int n = (int)(t * 14); static const float sc[] = { 523, 659, 784, 1047, 1319, 1568, 2093 }; return 0.35f * Onda(0, sc[n % 7] * t) * Inviluppo(t, 0, 0.9f, 1); });
        CreaSuono("st_capsula", 0.5f, [](float t) { int n = (int)(t * 16); float f = 440 * std::pow(2.0f, n / 12.0f * 3); return 0.3f * Onda(0, f * t) * Inviluppo(t, 0, 0.5f, 1); });
        CreaSuono("st_mattone", 0.08f, [](float t) { return 0.35f * Rumore() * Inviluppo(t, 0, 0.08f, 3); });
        CreaSuono("st_combo", 0.25f, [](float t) { return 0.3f * (Onda(3, 1760 * t) + 0.5f * Onda(3, 2637 * t)) * Inviluppo(t, 0, 0.25f, 2); });
        CreaSuono("st_vita", 0.9f, [](float t) { static const float m[] = { 784, 988, 1175, 1568, 1175, 1568 }; int n = std::min(5, (int)(t * 7)); return 0.3f * Onda(0, m[n] * t) * Inviluppo(t, 0, 0.9f, 0.6f); });
        CreaSuono("st_ondata", 1.2f, [](float t) { static const float m[] = { 392, 523, 659, 784 }; int n = std::min(3, (int)(t * 5)); return 0.3f * (Onda(2, m[n] * t) * 0.5f + Onda(0, m[n] * 0.5f * t) * 0.3f) * Inviluppo(t, 0, 1.2f, 0.7f); });
        CreaSuono("st_scudo", 0.4f, [](float t) { return 0.3f * Onda(3, (300 + 900 * t) * t) * Inviluppo(t, 0, 0.4f, 1); });
        CreaSuono("st_colpito", 0.2f, [](float t) { return 0.3f * Onda(0, 200 * t) * Inviluppo(t, 0, 0.2f, 2); });
    }
    void PreparaMusica() {
        brani[0] = CreaBrano("Stellaria - Nebulosa Rosa", 112, 8, {
            { "Sintetizzatore", 0, 0.32f, "E5:1 G5:1 B5:2 A5:1 G5:1 E5:2 D5:1 E5:1 G5:2 F#5:2 B4:2 E5:1 G5:1 B5:2 C6:2 B5:1 A5:1 G5:2 A5:1 G5:1 F#5:4" },
            { "Basso", 2, 0.4f, "E2:1 E3:1 E2:1 E3:1 C2:1 C3:1 C2:1 C3:1 D2:1 D3:1 D2:1 D3:1 B1:1 B2:1 B1:1 B2:1" },
            { "Batteria", 4, 0.45f, "K...S...K.K.S...", true } });
        brani[1] = CreaBrano("Stellaria - Aurora Boreale", 96, 8, {
            { "Campane", 3, 0.38f, "A5:2 E5:2 C#6:2 B5:2 A5:3 E5:1 F#5:4 D6:2 C#6:2 B5:2 A5:2 G#5:4 E5:4" },
            { "Pad", 1, 0.25f, "A3:8 F#3:8 D3:8 E3:8" },
            { "Battito", 4, 0.3f, "K.......K..hS...", true } });
        brani[2] = CreaBrano("Stellaria - Tempesta Ionica", 132, 8, {
            { "Guida", 2, 0.28f, "D5:1 D5:1 F5:1 D5:1 G5:1 D5:1 A5:1 G5:1 F5:1 D5:1 C5:1 D5:1 A4:2 C5:2 D5:1 D5:1 F5:1 D5:1 A5:1 D5:1 C6:1 A5:1 G5:2 F5:2 E5:4" },
            { "Basso", 0, 0.38f, "D2:1 D2:1 D3:1 D2:1 Bb1:1 Bb1:1 Bb2:1 Bb1:1 C2:1 C2:1 C3:1 C2:1 A1:1 A1:1 A2:1 A1:1" },
            { "Batteria", 4, 0.5f, "K.hhS.hhK.KhS.hh", true } });
        brani[3] = CreaBrano("Stellaria - Anelli di Saturnia", 104, 8, {
            { "Flauto", 3, 0.36f, "G5:3 A5:1 B5:2 D6:2 C6:2 B5:2 A5:4 E5:2 G5:2 A5:2 B5:2 G5:4 D5:4" },
            { "Arpeggio", 1, 0.2f, "G3:1 B3:1 D4:1 G4:1 E3:1 G3:1 B3:1 E4:1 C3:1 E3:1 G3:1 C4:1 D3:1 F#3:1 A3:1 D4:1" },
            { "Basso", 3, 0.45f, "G1:4 E1:4 C2:4 D2:4" },
            { "Spazzole", 4, 0.3f, "K..h..h.K..hS.h.", true } });
        brani[4] = CreaBrano("Stellaria - Corona del Sole", 140, 8, {
            { "Ottoni", 2, 0.3f, "C5:2 E5:1 G5:1 C6:2 B5:1 A5:1 G5:2 E5:2 F5:2 A5:1 C6:1 D6:2 C6:1 B5:1 C6:4 G5:4" },
            { "Basso", 0, 0.4f, "C2:1 C3:1 C2:1 C3:1 A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 G1:1 G2:1 G1:1 G2:1" },
            { "Batteria", 4, 0.5f, "K.h.S.hhK.h.S.ho", true } });
        brani[5] = CreaBrano("Stellaria - Orizzonte Oscuro", 88, 8, {
            { "Organo", 2, 0.26f, "B4:2 D5:2 F#5:2 G5:2 F#5:4 D5:2 B4:2 C5:2 E5:2 G5:2 A5:2 G5:8" },
            { "Basso", 3, 0.5f, "B1:8 G1:8 C2:8 F#1:8" },
            { "Tamburo", 4, 0.4f, "K.......S..K....", true } });
        brani[6] = CreaBrano("Stellaria - Tema", 120, 8, {
            { "Melodia", 0, 0.32f, "A4:1 C5:1 E5:1 A5:1 G5:2 E5:2 F5:1 E5:1 D5:1 C5:1 D5:2 E5:2 A4:1 C5:1 E5:1 A5:1 B5:2 C6:2 B5:1 A5:1 G5:1 E5:1 A5:4" },
            { "Basso", 2, 0.4f, "A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 G1:1 G2:1 G1:1 G2:1 E1:1 E2:1 E1:1 E2:1" },
            { "Arpeggio", 1, 0.16f, "A4:1 C5:1 E5:1 C5:1 F4:1 A4:1 C5:1 A4:1 G4:1 B4:1 D5:1 B4:1 E4:1 G#4:1 B4:1 G#4:1" },
            { "Batteria", 4, 0.45f, "K...S...K.K.S..h", true } });
    }

    // ======================= PARTITA =======================
    void Nuova(uint64_t s, int g) override {
        seme = s; giocatore = g;
        caso.Semina(s ^ 0x5157ull);
        regista = Regista{};
        ondata = 1; vite = 3; prossimaVita = 20000; punteggio = 0; tempo = 0;
        finito = vinto = false; puntiDiControllo = 0;
        arma = 0; tArma = 0; scudo = false; px = 960; tMorto = 0; tInvulnerabile = 0;
        colpi.clear(); capsule.clear(); meteore.clear(); part.v.clear(); testi.clear();
        LeggiRecord();
        RicostruisciBarriere();
        IniziaOndata();
    }
    void LeggiRecord() {
        std::ifstream f(FileAccanto(".record"));
        record = 0;
        if (f) f >> record;
    }
    void ScriviRecord() {
        if (punteggio <= record) return;
        record = punteggio;
        std::ofstream f(FileAccanto(".record"));
        if (f) f << record;
    }
    void RicostruisciBarriere() {
        for (int b = 0; b < 4; b++)
            for (int j = 0; j < BH; j++)
                for (int i = 0; i < BW; i++) {
                    bool pieno = true;
                    if (j < 5 && (i < 5 - j || i > BW - 6 + j)) pieno = false;          // angoli smussati in alto
                    if (j >= BH - 6 && i >= 7 && i < BW - 7) {                            // arco in basso
                        int d = std::min(i - 7, BW - 8 - i);
                        if (j - (BH - 6) >= 2 - d) pieno = false;
                    }
                    barriere[b][j][i] = pieno ? 3 : 0;   // 3 = intatto, 2 e 1 = incrinato
                }
    }
    float BarrieraX(int b) const { return 330 + b * 420 - BW * CB / 2.0f; }

    void IniziaOndata() {
        casoInizioOndata = caso.s;
        alieni.clear();
        int forma = ondata <= 3 ? 0 : caso.I(0, 4);
        evento = 0;
        if (ondata % 5 == 0) evento = 2;                                  // ogni quinta ondata: alieni corazzati
        else if (ondata > 2 && caso.P(0.35f)) evento = caso.I(1, 4) == 2 ? 1 : caso.I(1, 4);   // gli eventi cambiano a ogni partita
        // specie per riga: l'ordine dei colori cambia con il seme (partite uniche)
        int specie[RIGHE] = { 0, 1, 2, 3, 4 };
        if (ondata > 1 && caso.P(0.5f)) std::swap(specie[caso.I(0, 4)], specie[caso.I(0, 4)]);
        for (int r = 0; r < RIGHE; r++)
            for (int col = 0; col < COLONNE; col++) {
                bool c = true;
                float cx = col - 5, cy = r - 2;
                if (forma == 1) c = std::fabs(cx) <= 5 - std::fabs(cy) * 1.2f;               // rombo
                else if (forma == 2) c = (r + col) % 2 == 0 || r == 4;                      // scacchiera
                else if (forma == 3) c = std::fabs(cx) >= 1;                                 // due blocchi
                else if (forma == 4) c = std::fabs(cx) <= r + 1.5f;                          // freccia
                if (!c) continue;
                Alieno a;
                a.ox = col * PASSO_X; a.oy = r * PASSO_Y;
                a.specie = specie[r];
                a.vita = (evento == 2 && r == 0) ? 3 : 1;
                a.vivo = true;
                alieni.push_back(a);
            }
        formX = 960 - (COLONNE - 1) * PASSO_X / 2;
        float altezza = 190 + std::min(5, (ondata - 1) % 5) * 22.0f;   // dentro un cielo, si parte un po' piu' in basso
        formY = altezza;
        discesaIngresso = 520;
        dir = 1; tPasso = 0; fotogramma = 0; nota = 0;
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Colpo& c) { return !c.amico; }), colpi.end());
        tDisco = caso.F(10, 18) / (evento == 3 ? 2.0f : 1.0f);
        discoVivo = false;
        tOndata = 0; tFineOndata = -1;
        scritta = "ONDATA " + std::to_string(ondata);
        sottoScritta = NOMI_CIELI[Cielo()];
        const char* ev[5] = { "", "Pioggia di meteore!", "Alieni corazzati in prima fila", "Doppio disco bonus", "Alieni frenetici" };
        if (evento) sottoScritta += std::string("  -  ") + ev[evento];
        tScritta = 2.6f;
        Suona("st_ondata", 0.8f);
        SuonaMusica(brani[Cielo()]);
        puntiDiControllo++;   // il flusso salva qui (inizio ondata)
    }

    int VivoContati() const { int n = 0; for (auto& a : alieni) n += a.vivo; return n; }
    Rectangle BoxAlieno(const Alieno& a) const { return { formX + a.ox - 36, formY - discesaIngresso + a.oy - 28, 72, 56 }; }

    void Punti(int p, Vector2 dove, Color c) {
        punteggio += p;
        testi.push_back({ dove, "+" + std::to_string(p), 0.9f, c });
        if (punteggio >= prossimaVita) {
            vite++; prossimaVita += 50000;
            Suona("st_vita");
            testi.push_back({ { 960, 600 }, "VITA EXTRA!", 1.6f, Hex(0x8aff6a) });
        }
    }

    void Scheggia(int b, int ci, int cj, float raggio, bool dalBasso) {
        for (int j = cj - 4; j <= cj + 4; j++)
            for (int i = ci - 4; i <= ci + 4; i++) {
                if (i < 0 || j < 0 || i >= BW || j >= BH || !barriere[b][j][i]) continue;
                float d = std::hypot((float)(i - ci), (float)(j - cj) * 1.2f);
                float r = raggio * Vis(0.6f, 1.25f);
                if (d < r * 0.6f) barriere[b][j][i] = 0;
                else if (d < r) barriere[b][j][i] = (unsigned char)std::max(0, barriere[b][j][i] - (Vis() < 0.6f ? 2 : 1));
                if (!barriere[b][j][i]) {
                    Particella p;
                    p.p = { BarrieraX(b) + i * CB + CB / 2.0f, Y_BARRIERE + j * CB + CB / 2.0f };
                    p.v = { Vis(-120, 120), dalBasso ? Vis(-260, -40) : Vis(40, 260) };
                    p.vita = p.durata = Vis(0.3f, 0.7f); p.r = 3; p.c = Hex(COL_BARRIERE[Cielo()]); p.forma = 1; p.gravita = 500;
                    part.Aggiungi(p);
                }
            }
        Suona("st_mattone", 0.5f, Vis(0.8f, 1.3f));
    }
    // Un colpo che attraversa le barriere: true se ha colpito un mattoncino (e lo ha scheggiato).
    bool ColpoBarriere(Vector2 p, bool dalBasso, float raggio) {
        if (p.y < Y_BARRIERE - 4 || p.y > Y_BARRIERE + BH * CB + 4) return false;
        for (int b = 0; b < 4; b++) {
            float bx = BarrieraX(b);
            int i = (int)std::floor((p.x - bx) / CB), j = (int)std::floor((p.y - Y_BARRIERE) / CB);
            if (i < 0 || i >= BW || j < 0 || j >= BH) continue;
            for (int dx = -1; dx <= 1; dx++) {
                int ii = i + dx;
                if (ii >= 0 && ii < BW && barriere[b][j][ii]) { Scheggia(b, ii, j, raggio, dalBasso); return true; }
            }
        }
        return false;
    }

    void Spara() {
        int maxColpi = arma == 3 ? 4 : 2;
        int miei = 0;
        for (auto& c : colpi) miei += c.amico && !c.laser;
        if (arma == 2) {   // laser: un raggio che attraversa tutto
            bool attivo = false;
            for (auto& c : colpi) if (c.laser) attivo = true;
            if (!attivo) { colpi.push_back({ { px, Y_GIOCATORE - 70 }, 0, 0, true, true, 0 }); Suona("st_laser", 0.6f); regista.Sparo(); }
            return;
        }
        if (miei >= maxColpi || tRicarica > 0) return;
        tRicarica = arma == 3 ? 0.11f : 0.22f;
        colpi.push_back({ { px, Y_GIOCATORE - 62 }, -1500, 0, true });
        if (arma == 1) { colpi.push_back({ { px - 30, Y_GIOCATORE - 40 }, -1450, 1, true }); colpi.push_back({ { px + 30, Y_GIOCATORE - 40 }, -1450, 2, true }); }
        regista.Sparo();
        Suona("st_sparo", 0.55f, Vis(0.97f, 1.03f));
    }

    void ColpisciAlieno(Alieno& a, Vector2 dove) {
        a.vita--;
        a.lampo = 0.12f;
        if (a.vita > 0) { Suona("st_colpito", 0.5f); return; }
        a.vivo = false;
        regista.Bersaglio();
        Color col = Hex(COL_ALIENI[a.specie][0]);
        part.Esplosione(dove, col, 26, 520, 6, 0.6f);
        part.Esplosione(dove, WHITE, 8, 300, 4, 0.3f);
        // combo di colore
        if (a.specie == comboSpecie && tCombo > 0) combo++; else { comboSpecie = a.specie; combo = 1; }
        tCombo = 2.2f;
        int molt = std::min(5, std::max(1, combo - 1));
        Punti(PUNTI_SPECIE[a.specie] / 10 * 10 * molt, { dove.x, dove.y - 30 }, col);
        if (combo >= 3) { Suona("st_combo", 0.6f, 1 + combo * 0.06f); testi.push_back({ { dove.x, dove.y - 70 }, "COMBO x" + std::to_string(molt), 1.0f, col }); }
        Suona("st_alieno", 0.7f, Vis(0.9f, 1.1f));
        // a volte cade una capsula (piu' spesso a chi fa fatica)
        if (caso.P(0.035f / regista.sfida)) capsule.push_back({ dove, caso.I(0, 3) });
    }

    void MuoviFormazione(float dt) {
        int n = VivoContati();
        if (n == 0) return;
        // intervallo tra un passo e l'altro: piu' alieni abbattuti = piu' veloci (come nei classici)
        float base = 0.62f - std::min(0.25f, ondata * 0.012f);
        intervallo = std::max(0.035f, base * std::pow(n / 55.0f, 0.85f) / regista.sfida);
        if (evento == 4) intervallo *= 0.7f;
        tPasso -= dt;
        if (tPasso > 0) return;
        tPasso += intervallo;
        float minX = 1e9f, maxX = -1e9f;
        for (auto& a : alieni) if (a.vivo) { minX = std::min(minX, formX + a.ox); maxX = std::max(maxX, formX + a.ox); }
        bool scendi = (dir > 0 && maxX + 20 > 1920 - 70) || (dir < 0 && minX - 20 < 70);
        if (scendi) { formY += 34; dir = -dir; }
        else formX += dir * 20;
        fotogramma ^= 1;
        Suona("st_passo" + std::to_string(nota), 0.75f);
        nota = (nota + 1) % 4;
    }

    void SparaAlieni(float dt) {
        if (discesaIngresso > 0 || tOndata < 2.0f) return;   // un attimo di respiro a inizio ondata
        int maxColpi = 2 + std::min(4, ondata / 5) + (regista.sfida > 1.2f ? 1 : 0);
        int loro = 0;
        for (auto& c : colpi) loro += !c.amico;
        float freq = (0.9f + ondata * 0.05f) * regista.sfida;
        if (loro >= maxColpi || !caso.P(freq * dt)) return;
        // l'alieno che spara: in fondo alla sua colonna, spesso sopra dove il giocatore sta di solito
        float mira = caso.P(ondata < 3 ? 0.3f : 0.5f) ? px : regista.abitX * 1920;
        if (caso.P(0.3f)) mira = caso.F(100, 1820);
        const Alieno* migliore = nullptr;
        float best = 1e9f;
        for (auto& a : alieni) {
            if (!a.vivo) continue;
            float x = formX + a.ox;
            float d = std::fabs(x - mira) - a.oy * 0.5f;
            if (d < best) { best = d; migliore = &a; }
        }
        if (!migliore) return;
        // il piu' in basso della stessa colonna
        for (auto& a : alieni) if (a.vivo && a.ox == migliore->ox && a.oy > migliore->oy) migliore = &a;
        Colpo c;
        c.p = { formX + migliore->ox, formY + migliore->oy + 30 };
        c.vy = (380 + ondata * 8) * regista.sfida * caso.F(0.9f, 1.15f);
        c.tipo = caso.I(0, 2);
        if (c.tipo == 2) c.vy *= 1.35f;
        c.amico = false;
        colpi.push_back(c);
    }

    void MortoGiocatore() {
        if (scudo) { scudo = false; tInvulnerabile = 1.0f; Suona("st_scudo"); part.Esplosione({ px, Y_GIOCATORE - 20 }, Hex(0x5ac8ff), 30, 400, 5); return; }
        vite--;
        regista.VitaPersa();
        tMorto = 2.2f;
        tScosse = 0.6f;
        arma = 0; tArma = 0;
        part.Esplosione({ px, Y_GIOCATORE - 20 }, Hex(0x5cff9a), 60, 700, 7, 1.0f);
        part.Esplosione({ px, Y_GIOCATORE - 20 }, Hex(0xffe14a), 30, 400, 9, 0.8f);
        Suona("st_esplosione");
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Colpo& c) { return !c.amico; }), colpi.end());
    }

    void Aggiorna(float dt, const Pad& pad) override {
        tempo += dt;
        tOndata += dt;
        part.Aggiorna(dt);
        for (auto& t : testi) { t.vita -= dt; t.p.y -= 40 * dt; }
        testi.erase(std::remove_if(testi.begin(), testi.end(), [](const Testo& t) { return t.vita <= 0; }), testi.end());
        tScritta -= dt; tScosse -= dt; tCombo -= dt;
        if (finito) return;

        if (discesaIngresso > 0) discesaIngresso = std::max(0.0f, discesaIngresso - dt * 600);

        // --- il cannone ---
        if (tMorto > 0) {
            tMorto -= dt;
            if (tMorto <= 0) {
                if (vite <= 0) { finito = true; ScriviRecord(); scritta = "GAME OVER"; sottoScritta = "Punteggio " + std::to_string(punteggio); tScritta = 99; return; }
                tInvulnerabile = 2.0f; px = 960;
            }
        } else {
            float vx = pad.x * 760;
            if (pad.clic && pad.mouse.y > 0) vx = std::max(-760.0f, std::min(760.0f, (pad.mouse.x - px) * 8));
            px = std::max(80.0f, std::min(1840.0f, px + vx * dt));
            tRicarica -= dt;
            tInvulnerabile -= dt;
            if (pad.fuoco) Spara();
            regista.Osserva(dt, px / 1920, 0.5f, pad.fuoco);
            if (tArma > 0) { tArma -= dt; if (tArma <= 0) arma = 0; }
        }

        MuoviFormazione(dt);
        SparaAlieni(dt);
        for (auto& a : alieni) a.lampo -= dt;

        // --- colpi ---
        for (auto& c : colpi) {
            c.t += dt;
            if (c.laser) { c.p.x = px; continue; }
            c.p.y += c.vy * dt;
            if (c.amico && c.tipo == 1) c.p.x -= 260 * dt;
            if (c.amico && c.tipo == 2) c.p.x += 260 * dt;
        }
        for (size_t i = 0; i < colpi.size(); i++) {
            Colpo& c = colpi[i];
            bool via = c.p.y < 70 || c.p.y > 1080;
            if (c.laser) {
                if (c.t > 0.45f || tMorto > 0) via = true;
                // il laser colpisce tutto lungo la colonna
                for (auto& a : alieni) if (a.vivo && std::fabs(formX + a.ox - px) < 40) ColpisciAlieno(a, { formX + a.ox, formY - discesaIngresso + a.oy });
                for (auto& m : meteore) if (std::fabs(m.p.x - px) < m.r) m.vita = 0;
                if (discoVivo && std::fabs(discoX - px) < 110) ColpisciDisco();
            } else if (c.amico) {
                if (ColpoBarriere(c.p, true, 2.2f)) via = true;
                for (auto& a : alieni) {
                    if (via || !a.vivo) continue;
                    if (CheckCollisionPointRec(c.p, BoxAlieno(a))) { ColpisciAlieno(a, { formX + a.ox, formY - discesaIngresso + a.oy }); via = true; }
                }
                if (!via && discoVivo && std::fabs(c.p.x - discoX) < 110 && std::fabs(c.p.y - 150) < 36) { ColpisciDisco(); via = true; }
                for (auto& m : meteore) if (!via && m.vita > 0 && Vector2Distance(c.p, m.p) < m.r) { m.vita--; via = true; if (m.vita <= 0) Punti(150, m.p, Hex(0xffc080)); }
                // un colpo contro un colpo nemico: si annullano
                for (auto& d : colpi) if (!via && !d.amico && d.vy != 0 && std::fabs(d.p.x - c.p.x) < 14 && std::fabs(d.p.y - c.p.y) < 26) { d.vy = 0; d.p.y = 9999; via = true; part.Esplosione(c.p, WHITE, 10, 200, 3, 0.3f); }
            } else {
                if (ColpoBarriere(c.p, false, 2.0f)) via = true;
                else if (tMorto <= 0 && tInvulnerabile <= 0 && std::fabs(c.p.x - px) < 52 && c.p.y > Y_GIOCATORE - 40 && c.p.y < Y_GIOCATORE + 30) { via = true; MortoGiocatore(); }
            }
            if (via) c.p.y = 99999;
        }
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Colpo& c) { return c.p.y > 5000; }), colpi.end());

        // gli alieni che toccano le barriere le cancellano; se arrivano in fondo e' la fine
        for (auto& a : alieni) {
            if (!a.vivo) continue;
            Rectangle r = BoxAlieno(a);
            if (r.y + r.height > Y_BARRIERE)
                for (int b = 0; b < 4; b++)
                    for (int j = 0; j < BH; j++)
                        for (int i = 0; i < BW; i++)
                            if (barriere[b][j][i] && CheckCollisionPointRec({ BarrieraX(b) + i * CB + 4.0f, Y_BARRIERE + j * CB + 4.0f }, r)) barriere[b][j][i] = 0;
            if (r.y + r.height > Y_GIOCATORE - 30 && tMorto <= 0) { vite = 1; MortoGiocatore(); formY -= 200; }
        }

        // --- disco bonus ---
        tDisco -= dt;
        if (!discoVivo && tDisco <= 0 && discesaIngresso <= 0 && VivoContati() > 6) {
            discoVivo = true; discoDir = caso.P(0.5f) ? 1.0f : -1.0f; discoX = discoDir > 0 ? -140.0f : 2060.0f;
            tDisco = caso.F(18, 28) / (evento == 3 ? 2.0f : 1.0f);
        }
        if (discoVivo) {
            discoX += discoDir * 330 * dt;
            tSirena -= dt;
            if (tSirena <= 0) { Suona("st_sirena", 0.45f); tSirena = 0.48f; }
            if (discoX < -200 || discoX > 2120) discoVivo = false;
        }

        // --- capsule ---
        for (auto& k : capsule) k.p.y += 220 * dt;
        for (auto& k : capsule) {
            if (tMorto <= 0 && std::fabs(k.p.x - px) < 70 && std::fabs(k.p.y - Y_GIOCATORE) < 50) {
                k.p.y = 9999;
                Suona("st_capsula");
                const char* nomi[5] = { "COLPO TRIPLO", "LASER", "SCUDO", "FUOCO RAPIDO", "VITA EXTRA" };
                testi.push_back({ { px, Y_GIOCATORE - 110 }, nomi[k.tipo], 1.3f, WHITE });
                if (k.tipo == 0) { arma = 1; tArma = 12; }
                else if (k.tipo == 1) { arma = 2; tArma = 7; }
                else if (k.tipo == 2) scudo = true;
                else if (k.tipo == 3) { arma = 3; tArma = 12; }
                else { vite++; Suona("st_vita"); }
            }
        }
        capsule.erase(std::remove_if(capsule.begin(), capsule.end(), [](const Capsula& k) { return k.p.y > 1100; }), capsule.end());

        // --- meteore (evento) ---
        if (evento == 1 && discesaIngresso <= 0) {
            tMeteore -= dt;
            if (tMeteore <= 0) { tMeteore = caso.F(1.2f, 2.6f) / regista.sfida; meteore.push_back({ { caso.F(100, 1820), -60 }, { caso.F(-80, 80), caso.F(180, 300) }, caso.F(30, 52), 0, 2 }); }
        }
        for (auto& m : meteore) {
            m.p.x += m.v.x * dt; m.p.y += m.v.y * dt; m.rot += dt * 60;
            if (m.vita > 0 && ColpoBarriere(m.p, false, 3.2f)) m.vita = 0;
            if (m.vita > 0 && tMorto <= 0 && tInvulnerabile <= 0 && Vector2Distance(m.p, { px, Y_GIOCATORE - 10 }) < m.r + 40) { m.vita = 0; MortoGiocatore(); }
            if (m.vita <= 0 && m.p.y < 2000) { part.Esplosione(m.p, Hex(0xffa060), 24, 360, 6, 0.6f); Suona("st_alieno", 0.5f, 0.7f); m.p.y = 5000; }
        }
        meteore.erase(std::remove_if(meteore.begin(), meteore.end(), [](const Meteora& m) { return m.p.y > 1150; }), meteore.end());

        // --- fine dell'ondata ---
        if (VivoContati() == 0 && tFineOndata < 0) {
            tFineOndata = 2.2f;
            regista.TappaFinita(tOndata, 45);
            int bonus = 1000 + ondata * 100;
            Punti(bonus, { 960, 520 }, Hex(0xffe14a));
            scritta = "ONDATA SUPERATA!"; sottoScritta = "Bonus " + std::to_string(bonus); tScritta = 2.0f;
        }
        if (tFineOndata > 0) {
            tFineOndata -= dt;
            if (tFineOndata <= 0) {
                if (ondata >= ONDATE) { finito = vinto = true; ScriviRecord(); scritta = "STELLARIA E' SALVA!"; sottoScritta = "Hai respinto tutte le 30 ondate"; tScritta = 99; Suona("st_vita"); return; }
                ondata++;
                if ((ondata - 1) % 5 == 0) RicostruisciBarriere();   // nuovo cielo: barriere nuove
                IniziaOndata();
            }
        }
        if (punteggio > record) record = punteggio;
    }

    void ColpisciDisco() {
        discoVivo = false;
        static const int premi[] = { 500, 800, 1000, 1500, 3000 };
        int p = premi[std::min(4, combo + caso.I(0, 2))];
        Punti(p, { discoX, 150 }, Hex(0xff8a8a));
        part.Esplosione({ discoX, 150 }, Hex(0xff5a5a), 50, 600, 7, 0.9f);
        part.Esplosione({ discoX, 150 }, Hex(0xffe14a), 30, 400, 5, 0.7f);
        Suona("st_disco");
        if (caso.P(0.5f)) capsule.push_back({ { discoX, 170 }, caso.I(0, 4) });
    }

    Pad Automatico() override {
        Pad p;
        // pilota automatico: insegue la colonna di alieni piu' vicina e schiva i colpi
        float bersaglio = px;
        float best = 1e9f;
        float vel = dir * 20 / std::max(0.03f, intervallo);   // dove sara' la formazione quando arriva il colpo
        for (auto& a : alieni) if (a.vivo) {
            float y = formY - discesaIngresso + a.oy;
            float x = formX + a.ox + vel * (Y_GIOCATORE - y) / 1500;
            float d = std::fabs(x - px) - a.oy * 0.3f;
            if (d < best) { best = d; bersaglio = x; }
        }
        if (discoVivo) bersaglio = discoX + discoDir * 330 * (Y_GIOCATORE - 150) / 1500;
        for (auto& k : capsule) if (k.p.y > 700) bersaglio = k.p.x;
        for (auto& c : colpi) if (!c.amico && c.p.y > 650 && std::fabs(c.p.x - px) < 70) bersaglio = px + (c.p.x < px ? 220 : -220);
        float d = bersaglio - px;
        p.x = std::fabs(d) < 10 ? 0 : std::max(-1.0f, std::min(1.0f, d / 60));
        p.fuoco = std::fabs(d) < 28;
        return p;
    }

    // ======================= DISEGNO =======================
    void Disegna() override {
        float sx = tScosse > 0 ? Vis(-8, 8) * tScosse : 0, sy = tScosse > 0 ? Vis(-8, 8) * tScosse : 0;
        DrawTexturePro(cieli[Cielo()], { 0, 0, 1280, 720 }, { sx - 20, sy - 20, 1960, 1120 }, { 0, 0 }, 0, WHITE);
        // stelle che scorrono e brillano
        for (int i = 0; i < 140; i++) {
            unsigned h = (unsigned)i * 2654435761u;
            float x = (h % 1920);
            float y = std::fmod((h >> 11) % 1080 + tempo * (12 + (h >> 5) % 50), 1080.0f);
            float l = 0.4f + 0.6f * (0.5f + 0.5f * std::sin(tempo * 3 + i));
            DrawCircleV({ x, y }, 1 + (h >> 20) % 2, ConAlfa(WHITE, l * 0.8f));
        }
        // griglia luminosa in basso (scorre verso chi gioca)
        Color cg = Cielo() == 1 ? Hex(0x3cffb0) : Cielo() == 4 ? Hex(0xffb040) : Hex(0xff50c8);
        DrawRectangleGradientV(0, 880, 1920, 200, ConAlfa(cg, 0.12f), ConAlfa(Hex(0x14002a), 0.9f));
        for (int i = -30; i <= 30; i++) DrawLineEx({ 960.0f + i * 40, 880 }, { 960.0f + i * 260, 1080 }, 2, ConAlfa(cg, 0.45f));
        for (int k = 0; k < 9; k++) {
            float f = std::fmod(k / 8.0f + tempo * 0.15f, 1.0f);
            float y = 880 + f * f * 200;
            DrawLineEx({ 0, y }, { 1920, y }, 2, ConAlfa(cg, 0.35f + f * 0.3f));
        }
        DrawRectangle(0, 878, 1920, 3, ConAlfa(cg, 0.9f));

        // barriere a mattoncini
        for (int b = 0; b < 4; b++) {
            float bx = BarrieraX(b);
            Bagliore(bx + BW * CB / 2.0f, Y_BARRIERE + BH * CB / 2.0f, 170, ConAlfa(Hex(COL_BARRIERE[Cielo()]), 0.10f));
            for (int j = 0; j < BH; j++)
                for (int i = 0; i < BW; i++) {
                    int v = barriere[b][j][i];
                    if (!v) continue;
                    unsigned hh = (unsigned)(b * 997 + j * 31 + i * 7) * 2654435761u;
                    float l = 0.75f + 0.25f * ((hh >> 12) % 100) / 100.0f;
                    Color c = Hex(COL_BARRIERE[Cielo()]);
                    float scuro = v == 3 ? 1.0f : v == 2 ? 0.78f : 0.5f;
                    l *= scuro;
                    c.r = (unsigned char)(c.r * l); c.g = (unsigned char)(c.g * l); c.b = (unsigned char)(c.b * l);
                    int x = (int)bx + i * CB, y = (int)Y_BARRIERE + j * CB;
                    DrawRectangle(x, y, CB - 1, CB - 1, c);
                    DrawRectangle(x, y, CB - 1, 2, ConAlfa(WHITE, v == 3 ? 0.35f : 0.15f));
                    if (v == 1) DrawRectangle(x + 2, y + 3, 3, 2, ConAlfa(BLACK, 0.5f));
                }
        }

        // alieni
        for (auto& a : alieni) {
            if (!a.vivo) continue;
            float x = formX + a.ox + sx, y = formY - discesaIngresso + a.oy + sy;
            Color glowC = ConAlfa(Hex(COL_ALIENI[a.specie][0]), 0.22f);
            Bagliore(x, y, 70, glowC);
            Sprite(alieniTex[a.specie][fotogramma], x, y, 0.5f, 0, a.lampo > 0 ? Color{ 255, 255, 255, 255 } : WHITE);
            if (a.lampo > 0) Bagliore(x, y, 50, ConAlfa(WHITE, 0.6f));
            if (a.vita > 1) { DrawRingLines({ x, y }, 40, 44, 0, 360, 24, ConAlfa(Hex(0xc0e0ff), 0.6f)); }
        }

        // disco bonus
        if (discoVivo) {
            Bagliore(discoX, 150, 180, ConAlfa(Hex(0xff3a3a), 0.35f));
            Sprite(disco, discoX + sx, 150 + sy, 0.75f);
            for (int i = 0; i < 7; i++) {
                float lx = discoX - 72 + i * 24, l = 0.5f + 0.5f * std::sin(tempo * 12 + i);
                DrawCircleV({ lx, 154 }, 5, ConAlfa(i % 2 ? Hex(0xffe14a) : Hex(0x77ffff), 0.6f + 0.4f * l));
                Bagliore(lx, 154, 16, ConAlfa(i % 2 ? Hex(0xffe14a) : Hex(0x77ffff), 0.5f * l));
            }
        }

        // meteore
        for (auto& m : meteore) { Bagliore(m.p.x, m.p.y - 20, m.r * 2, ConAlfa(Hex(0xff8040), 0.25f)); Sprite(meteoraTex, m.p.x, m.p.y, m.r / 50, m.rot); }

        // capsule
        for (auto& k : capsule) {
            float pul = 1 + 0.08f * std::sin(tempo * 10);
            Sprite(capsTex[k.tipo], k.p.x, k.p.y, 0.8f * pul);
            const char* let[5] = { "3", "L", "S", "R", "+" };
            Scritta(let[k.tipo], k.p.x, k.p.y + 2, 30, Hex(0x102030), 0, false);
        }

        // colpi
        for (auto& c : colpi) {
            if (c.laser) {
                float a = std::max(0.0f, 1 - c.t / 0.45f);
                DrawRectangleGradientH((int)px - 18, 0, 18, (int)Y_GIOCATORE - 70, ConAlfa(Hex(0xff5a5a), 0), ConAlfa(Hex(0xff8a8a), 0.7f * a));
                DrawRectangleGradientH((int)px, 0, 18, (int)Y_GIOCATORE - 70, ConAlfa(Hex(0xff8a8a), 0.7f * a), ConAlfa(Hex(0xff5a5a), 0));
                DrawRectangle((int)px - 4, 0, 8, (int)Y_GIOCATORE - 70, ConAlfa(WHITE, a));
                continue;
            }
            if (c.amico) {
                DrawRectangleGradientV((int)c.p.x - 4, (int)c.p.y, 8, 40, Hex(0xeaffff), ConAlfa(Hex(0x60ffd0), 0));
                Bagliore(c.p.x, c.p.y + 6, 28, ConAlfa(Hex(0x80ffe0), 0.7f));
            } else {
                Color col = c.tipo == 0 ? Hex(0xff6bd6) : c.tipo == 1 ? Hex(0xffa040) : Hex(0xff4040);
                if (c.tipo == 0) {   // zigzag
                    for (int k = 0; k < 5; k++) DrawLineEx({ c.p.x + (k % 2 ? 7.0f : -7.0f) * std::sin(tempo * 30), c.p.y - k * 9.0f }, { c.p.x + ((k + 1) % 2 ? 7.0f : -7.0f) * std::sin(tempo * 30), c.p.y - (k + 1) * 9.0f }, 4, col);
                } else if (c.tipo == 1) {   // a croce
                    DrawRectangle((int)c.p.x - 3, (int)c.p.y - 36, 6, 36, col);
                    float yy = c.p.y - 30 + std::fmod(tempo * 120, 24);
                    DrawRectangle((int)c.p.x - 9, (int)yy, 18, 5, col);
                } else DrawRectangle((int)c.p.x - 2, (int)c.p.y - 44, 4, 44, col);
                Bagliore(c.p.x, c.p.y - 20, 34, ConAlfa(col, 0.6f));
            }
        }

        // cannone del giocatore
        if (tMorto <= 0) {
            bool vis = tInvulnerabile <= 0 || std::fmod(tempo, 0.2f) < 0.12f;
            if (vis) {
                Bagliore(px, Y_GIOCATORE + 10, 130, ConAlfa(Hex(0x40ff90), 0.25f));
                Sprite(cannone, px + sx, Y_GIOCATORE + sy, 0.55f);
                // fiamme dei motori
                for (int s2 = -1; s2 <= 1; s2 += 2) {
                    float h = 26 + 10 * std::sin(tempo * 40 + s2);
                    DrawTriangle({ px + s2 * 21 - 6, Y_GIOCATORE + 20 }, { px + s2 * 21, Y_GIOCATORE + 20 + h }, { px + s2 * 21 + 6, Y_GIOCATORE + 20 }, ConAlfa(Hex(0x9fffd0), 0.8f));
                }
                if (scudo) { DrawCircleLines((int)px, (int)Y_GIOCATORE, 70 + 3 * std::sin(tempo * 8), ConAlfa(Hex(0x5ac8ff), 0.9f)); Bagliore(px, Y_GIOCATORE, 90, ConAlfa(Hex(0x5ac8ff), 0.2f)); }
            }
        }

        part.Disegna();
        for (auto& t : testi) Scritta(t.t, t.p.x, t.p.y, 34, ConAlfa(t.c, std::min(1.0f, t.vita * 2)));

        // HUD
        DrawRectangleGradientV(0, 0, 1920, 72, Hex(0x0a0020, 200), Hex(0x0a0020, 60));
        DrawRectangle(0, 70, 1920, 2, ConAlfa(Hex(0xff5ad8), 0.7f));
        Scritta((giocatore == 0 ? "1UP  " : "2UP  ") + std::to_string(punteggio), 40, 36, 36, WHITE, -1);
        Scritta("RECORD  " + std::to_string(std::max(record, punteggio)), 960, 36, 36, Hex(0xffe38a), 0);
        Scritta("ONDATA " + std::to_string(ondata) + "/" + std::to_string(ONDATE), 1880, 36, 36, Hex(0x77ffff), 1);
        if (altroPunteggio >= 0) Scritta((giocatore == 0 ? "2UP " : "1UP ") + std::to_string(altroPunteggio), 1880, 1040, 26, Hex(0xb0b8e0), 1);
        for (int i = 0; i < std::min(vite - (tMorto > 0 ? 0 : 1), 8); i++) Sprite(cannone, 70 + i * 74.0f, 1040, 0.25f);
        if (arma) {
            const char* nomi[4] = { "", "COLPO TRIPLO", "LASER", "FUOCO RAPIDO" };
            Scritta(nomi[arma], 960, 1040, 28, Hex(0xffe14a), 0);
            DrawRectangle(860, 1060, (int)(200 * tArma / 12), 6, Hex(0xffe14a));
        } else if (tCombo > 0 && combo >= 2) Scritta("COMBO x" + std::to_string(std::min(5, combo - 1)), 960, 1040, 30, Hex(COL_ALIENI[comboSpecie][0]), 0);
        if (tScritta > 0) {
            float a = std::min(1.0f, tScritta * 2);
            Scritta(scritta, 960 + sx, 440, 96, ConAlfa(finito && !vinto ? Hex(0xff6a8a) : Hex(0xffffff), a), 0);
            Scritta(sottoScritta, 960, 530, 40, ConAlfa(Hex(0x9ff6ff), a), 0);
        }
    }

    // ======================= SALVATAGGI =======================
    std::string Stato() const override {
        json j;
        j["seme"] = std::to_string(seme);
        j["caso"] = std::to_string(caso.s);
        j["ondata"] = ondata; j["vite"] = vite; j["prossimaVita"] = prossimaVita; j["punteggio"] = punteggio; j["tempo"] = tempo;
        j["arma"] = arma; j["tArma"] = tArma; j["scudo"] = scudo; j["giocatore"] = giocatore;
        j["regista"] = json::parse(regista.Json());
        std::string b;
        for (int k = 0; k < 4; k++) for (int y = 0; y < BH; y++) for (int x = 0; x < BW; x++) b += (char)('0' + barriere[k][y][x]);
        j["barriere"] = b;
        j["casoInizioOndata"] = std::to_string(casoInizioOndata);
        return j.dump();
    }
    uint64_t casoInizioOndata = 0;
    bool Ripristina(const std::string& t, std::string& errore) override {
        json j = json::parse(t, nullptr, false);
        if (!j.is_object() || !j.contains("ondata")) { errore = "dati di Stellaria non validi"; return false; }
        Nuova(1, j.value("giocatore", 0));
        try {
            caso.s = std::stoull(j.value("casoInizioOndata", j.value("caso", std::string("1"))));
            seme = std::stoull(j.value("seme", std::string("1")));
        } catch (...) { errore = "stato del caso non valido"; return false; }
        ondata = std::max(1, std::min(ONDATE, j.value("ondata", 1)));
        vite = std::max(1, j.value("vite", 3));
        prossimaVita = j.value("prossimaVita", 20000);
        punteggio = j.value("punteggio", 0);
        tempo = j.value("tempo", 0.0f);
        arma = j.value("arma", 0); tArma = j.value("tArma", 0.0f); scudo = j.value("scudo", false);
        if (j.contains("regista")) regista.DaJson(j["regista"].dump());
        std::string b = j.value("barriere", "");
        if (b.size() == (size_t)(4 * BH * BW)) {
            size_t n = 0;
            for (int k = 0; k < 4; k++) for (int y = 0; y < BH; y++) for (int x = 0; x < BW; x++) barriere[k][y][x] = (unsigned char)std::max(0, std::min(3, b[n++] - '0'));
        }
        uint64_t c = caso.s;
        IniziaOndataDaSalvataggio(c);
        return true;
    }
    void IniziaOndataDaSalvataggio(uint64_t c) {
        caso.s = c;
        casoInizioOndata = c;
        int p = puntiDiControllo;
        IniziaOndata();
        puntiDiControllo = p;   // ricaricare non deve rifare subito il salvataggio automatico
    }
};

}  // namespace

std::unique_ptr<GiocoArcade> CreaStellaria() { return std::unique_ptr<GiocoArcade>(new Stellaria()); }

}  // namespace arcade
