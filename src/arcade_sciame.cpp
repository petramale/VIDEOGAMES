// SCIAME DI ORIONE - sparatutto a formazioni (genere "sciami in picchiata").
// Gioco originale di Mario Petramale - Elvantis. 15 livelli in 5 settori dello spazio, un boss ogni
// 3 livelli, insetti spaziali che entrano in formazione con traiettorie curve e si tuffano sul
// caccia, armi bonus lasciate solo dagli insetti con l'aura dorata, stemmi di livello e un regista
// che impara dove ti metti e quanto sei preciso.
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
constexpr float Y_CACCIA = 960;
constexpr float PASSO_C = 96, PASSO_R = 70;

std::string HexS(unsigned c) { char b[8]; std::snprintf(b, sizeof b, "#%06x", c); return b; }

const char* SETTORI[5] = { "Cintura di Orione", "Nebulosa del Cigno", "Abisso di Cristallo", "Campi di Vega", "Cuore dello Sciame" };
const char* NOMI_BOSS[5] = { "REGINA VESPA DI CRISTALLO", "IDRA MECCANICA", "OCCHIO DELLA NEBULOSA", "CORAZZATA SCARABEO", "IMPERATRICE DELLO SCIAME" };
const char* NOMI_SPECIE[4] = { "scarabeo", "farfalla", "vespa", "libellula" };
// colori delle specie: [specie][variante] = { chiaro, scuro }
const unsigned COL_SP[4][4][2] = {
    { { 0xc06aff, 0x300a5a }, { 0xff5a3c, 0x6a0a1a }, { 0x4affc0, 0x0a4a3a }, { 0xffd23c, 0x6a3a0a } },
    { { 0xffd23c, 0xa0400a }, { 0x3cc8ff, 0x1a3a9a }, { 0xff7ad0, 0x7a0a5a }, { 0x9aff5a, 0x2a6a0a } },
    { { 0xffe94a, 0xc96a00 }, { 0x5fffe0, 0x0a6a7a }, { 0xff8a5a, 0x8a1a0a }, { 0xb08aff, 0x3a1a8a } },
    { { 0x7aff6a, 0x103040 }, { 0xff7ad0, 0x401030 }, { 0x6ad8ff, 0x102a50 }, { 0xffb04a, 0x402010 } } };
const int PUNTI[4][2] = { { 150, 400 }, { 80, 160 }, { 50, 100 }, { 100, 200 } };   // in formazione / in picchiata

// --- percorsi curvi (ingressi e picchiate) -----------------------------------------------------
struct Percorso {
    std::vector<Vector2> p;
    std::vector<float> d;
    void Punto(Vector2 q) {
        d.push_back(p.empty() ? 0 : d.back() + Vector2Distance(p.back(), q));
        p.push_back(q);
    }
    Vector2 Ultimo() const { return p.empty() ? Vector2{ 0, 0 } : p.back(); }
    void Bezier(Vector2 b, Vector2 c, Vector2 e) {
        Vector2 a = Ultimo();
        for (int i = 1; i <= 24; i++) {
            float t = i / 24.0f, u = 1 - t;
            Punto({ u * u * u * a.x + 3 * u * u * t * b.x + 3 * u * t * t * c.x + t * t * t * e.x,
                    u * u * u * a.y + 3 * u * u * t * b.y + 3 * u * t * t * c.y + t * t * t * e.y });
        }
    }
    void Arco(Vector2 centro, float r, float a0, float a1) {
        int n = std::max(8, (int)(std::fabs(a1 - a0) * r / 18));
        for (int i = 0; i <= n; i++) { float a = a0 + (a1 - a0) * i / n; Punto({ centro.x + std::cos(a) * r, centro.y + std::sin(a) * r }); }
    }
    float Lunghezza() const { return d.empty() ? 0 : d.back(); }
    Vector2 A(float s, float* ang = nullptr) const {
        if (p.size() < 2) return Ultimo();
        size_t i = std::upper_bound(d.begin(), d.end(), s) - d.begin();
        if (i == 0) i = 1;
        if (i >= p.size()) i = p.size() - 1;
        float seg = std::max(0.001f, d[i] - d[i - 1]);
        float k = std::max(0.0f, std::min(1.0f, (s - d[i - 1]) / seg));
        if (ang) *ang = std::atan2(p[i].y - p[i - 1].y, p[i].x - p[i - 1].x);
        return { p[i - 1].x + (p[i].x - p[i - 1].x) * k, p[i - 1].y + (p[i].y - p[i - 1].y) * k };
    }
};

struct Insetto {
    int specie = 2, var = 0, riga = 0, col = 0;
    Vector2 p{ 0, 0 };
    float ang = -TAU / 4;              // direzione del muso
    int vita = 1;
    int stato = 0;                     // 0 ingresso, 1 verso il posto, 2 in formazione, 3 picchiata, 4 rientro
    bool vivo = true, aura = false;
    float s = 0, vel = 520, ritardo = 0, tFuoco = 0, lampo = 0;
    int colpiDaSparare = 0;
    Percorso per;
};
struct Proiettile { Vector2 p, v; int tipo; float r; };   // 0 goccia, 1 cristallo, 2 sfera grande, 3 polvere
struct Colpo { Vector2 p; bool laser = false, missile = false; float vx = 0; int bersaglio = -1; float t = 0; };
struct Capsula { Vector2 p; int tipo; };   // 0 laser, 1 missili, 2 scudo, 3 doppio
struct Anello { Vector2 p; float t, durata, r; Color c; };

struct Parte { Vector2 off{ 0, 0 }; float r = 50, hp = 10, hpMax = 10; bool vulnerabile = true; float lampo = 0; };
struct Boss {
    int tipo = -1, indice = 0;
    bool vivo = false;
    Vector2 p{ 960, -300 };
    float t = 0, tAtt = 2, ingresso = 1, aperto = 1, tCiclo = 0, tSpirale = 0, angSpirale = 0, tMorte = 0, tEvoca = 6;
    int att = 0, fase = 0;
    int carica = 0; float tCarica = 0, caricaX = 0;   // la carica della Regina
    int raggio = 0; float tRaggio = 0, raggioX = 0;   // il raggio dell'Occhio
    std::vector<Parte> parti;
    float Hp() const { float h = 0; for (auto& q : parti) h += std::max(0.0f, q.hp); return h; }
    float HpMax() const { float h = 0; for (auto& q : parti) h += q.hpMax; return h; }
};

// risorse condivise (gioco e vetrina del menu)
Texture2D sfondi[5]{}, insTex[4][2][2]{}, cacciaTex{}, capsTex[4]{}, bossTex[5]{}, testaIdra{}, alaRegina{}, irideTex{}, gusciTex{}, cannoneTex{}, alaImp{}, sferaTex{};
std::string gFirma;

class Sciame : public GiocoArcade {
public:
    const char* Nome() const override { return "sciame"; }
    std::string Titolo() const override { return "Sciame di Orione"; }
    std::string Tappa() const override { return "Livello " + std::to_string(livello) + " - " + NomeSettore(Settore()); }
    std::vector<std::string> Aiuto() const override {
        return { "Frecce, A/D, levetta o mouse: muovi il caccia", "Spazio, J, clic sinistro o pulsante A: spara",
                 "Gli insetti con l'aura dorata lasciano armi: Laser, Missili, Scudo, Doppio",
                 "Ogni 3 livelli arriva un boss: colpisci i suoi punti deboli" };
    }
    Brano MusicaMenu() const override { return BranoDi(TestoDi(Gen(), "musicaMenu", "Sciame - Tema")); }

    // --- dati del progetto ---
    static const json& Elem(const json& lista, int k) { static const json vuoto = json::object(); return lista.is_array() && k >= 0 && k < (int)lista.size() ? lista[k] : vuoto; }
    int NLiv() const { return cfg["livelli"].is_array() && !cfg["livelli"].empty() ? (int)cfg["livelli"].size() : 1; }
    const json& Liv() const { return Elem(cfg["livelli"], livello - 1); }
    const json& Gen() const { return cfg["generale"]; }
    const json& SettoreJ(int k) const { return Elem(cfg["settori"], k); }
    const json& SpecieJ(int k) const { return Elem(cfg["specie"], k); }
    const json& BossJ(int k) const { return Elem(cfg["boss"], k); }
    std::string NomeSettore(int k) const { return TestoDi(SettoreJ(k), "nome", SETTORI[std::max(0, std::min(4, k))]); }
    int IndiceBoss() const { int b = IntDi(Liv(), "boss", -1); return b >= 0 && b < (int)cfg["boss"].size() ? b : -1; }
    std::string NomeBoss() const { return TestoDi(BossJ(boss.indice), "nome", NOMI_BOSS[std::max(0, boss.tipo)]); }
    Color ColSp(int sp, int var) const { return ColoreDi(SpecieJ(sp), var ? "chiaro2" : "chiaro", Hex(COL_SP[sp][var][0])); }
    Color ColSpScuro(int sp, int var) const { return ColoreDi(SpecieJ(sp), var ? "scuro2" : "scuro", Hex(COL_SP[sp][var][1])); }
    int PuntiSp(int sp, bool volo) const { return IntDi(SpecieJ(sp), volo ? "puntiVolo" : "punti", PUNTI[sp][volo ? 1 : 0]); }

    static std::vector<std::pair<std::string, std::string>> NomiSuoni() {
        return { { "sc_sparo", "Sparo del caccia" }, { "sc_esplodi", "Insetto abbattuto" }, { "sc_tuffo", "Picchiata" }, { "sc_ingresso", "Inizio livello" },
                 { "sc_allarme", "Arriva il boss" }, { "sc_metallo", "Colpo su corazza" }, { "sc_boom", "Grande esplosione" }, { "sc_capsula", "Arma presa" },
                 { "sc_missile", "Missili" }, { "sc_laser", "Laser" }, { "sc_scudo", "Scudo consumato" }, { "sc_morte", "Caccia distrutto" },
                 { "sc_vita", "Vita extra" }, { "sc_stemma", "Livello superato" }, { "sc_colpito", "Boss colpito" } };
    }
    json Predefiniti() const override {
        json j;
        j["generale"] = { { "vite", 3 }, { "vitaExtra", 30000 }, { "vitaOgni", 100000 }, { "velocitaCaccia", 820 }, { "colpi", 2 },
                          { "aura", 0.11 }, { "musicaMenu", "Sciame - Tema" }, { "fine", "LO SCIAME E' SCONFITTO!" } };
        const char* cieli[5][5] = { { "#000814", "#04213f", "#0a3b5c", "#00b4ff", "#7fe0ff" }, { "#0a0418", "#2a0a40", "#4a1060", "#ff5ad0", "#ffb0f0" },
                                    { "#000a10", "#022a30", "#063a40", "#3cffe0", "#b0fff0" }, { "#0c0800", "#2a1a08", "#4a2a10", "#ffb040", "#ffe0a0" },
                                    { "#0a0000", "#2a0410", "#40081a", "#ff3a3a", "#ffa0a0" } };
        const char* musiche[5] = { "Sciame - Cintura di Orione", "Sciame - Nebulosa del Cigno", "Sciame - Abisso di Cristallo", "Sciame - Campi di Vega", "Sciame - Cuore dello Sciame" };
        for (int k = 0; k < 5; k++)
            j["settori"].push_back({ { "nome", SETTORI[k] }, { "alto", cieli[k][0] }, { "medio", cieli[k][1] }, { "basso", cieli[k][2] }, { "luce", cieli[k][3] },
                                     { "chiaro", cieli[k][4] }, { "musica", musiche[k] } });
        const char* nomi[4] = { "Scarabeo", "Farfalla", "Vespa", "Libellula" };
        for (int k = 0; k < 4; k++)
            j["specie"].push_back({ { "nome", nomi[k] }, { "chiaro", HexS(COL_SP[k][0][0]) }, { "scuro", HexS(COL_SP[k][0][1]) },
                                    { "chiaro2", HexS(COL_SP[k][1][0]) }, { "scuro2", HexS(COL_SP[k][1][1]) }, { "punti", PUNTI[k][0] }, { "puntiVolo", PUNTI[k][1] },
                                    { "vita", k == 0 ? 2 : 1 }, { "velocita", k == 3 ? 1.25 : 1.0 } });
        const char* nb[5] = { "Regina Vespa di Cristallo", "Idra Meccanica", "Occhio della Nebulosa", "Corazzata Scarabeo", "Imperatrice dello Sciame" };
        for (int k = 0; k < 5; k++)
            j["boss"].push_back({ { "nome", nb[k] }, { "comportamento", k }, { "resistenza", 1.0 }, { "aggressivita", 1.0 }, { "punti", 10000 + k * 5000 },
                                  { "musica", "Sciame - Boss" } });
        for (int n = 1; n <= 15; n++) {
            bool b = n % 3 == 0;
            j["livelli"].push_back({ { "settore", std::min(4, (n - 1) / 3) }, { "boss", b ? n / 3 - 1 : -1 },
                                     { "riga1", b ? -1 : 0 }, { "riga2", b ? -1 : 1 }, { "riga3", b ? -1 : (n >= 10 ? 3 : 1) }, { "riga4", 2 },
                                     { "riga5", b ? (n >= 6 ? 3 : 1) : (n >= 4 ? 3 : 2) }, { "ingresso", 0 },
                                     { "picchiate", 1.0 }, { "velocita", 1.0 }, { "fuoco", 1.0 } });
        }
        j["armi"] = { { "laser", 15 }, { "missili", 15 } };
        json su = json::object();
        for (auto& n : NomiSuoni()) su[n.first] = "";
        j["suoni"] = su;
        return j;
    }
    json Schema() const override {
        json voci = json::array();
        auto Cat = [&](const char* chiave, const char* nome, bool lista, const char* aiuto, json campi, int min = 0, int max = 0, const char* elemento = "") {
            json c = { { "chiave", chiave }, { "nome", nome }, { "lista", lista }, { "aiuto", aiuto }, { "campi", campi } };
            if (lista) { c["min"] = min; c["max"] = max; c["elemento"] = elemento; }
            voci.push_back(c);
        };
        auto N = [](const char* k, const char* n, double mi, double ma, double passo, const char* f = "%.0f") { return json{ { "chiave", k }, { "nome", n }, { "tipo", "numero" }, { "min", mi }, { "max", ma }, { "passo", passo }, { "formato", f } }; };
        auto C = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "colore" } }; };
        auto T = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "testo" } }; };
        auto S = [](const char* k, const char* n, json v) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "scelta" }, { "voci", v } }; };
        auto Rf = [](const char* k, const char* n, const char* da, const char* vuoto = "") { json f = { { "chiave", k }, { "nome", n }, { "tipo", "rif" }, { "da", da } }; if (*vuoto) f["vuoto"] = vuoto; return f; };
        auto B = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "brano" } }; };
        json G = { { "chiave", "grafica" }, { "nome", "Grafica" }, { "tipo", "grafica" } };
        json Sf = { { "chiave", "sfondo" }, { "nome", "Sfondo" }, { "tipo", "sfondo" } };
        Cat("livelli", "Livelli", true, "Ogni livello: il settore dello spazio, il boss (se c'e'), quale insetto in ogni riga della formazione, le traiettorie d'ingresso, "
            "quanto spesso si tuffano, velocita' e fuoco. Il gioco finisce dopo l'ultimo livello.",
            { Rf("settore", "Settore", "settori"), Rf("boss", "Boss", "boss", "(nessuno)"),
              Rf("riga1", "Riga 1 (in alto)", "specie", "(vuota)"), Rf("riga2", "Riga 2", "specie", "(vuota)"), Rf("riga3", "Riga 3", "specie", "(vuota)"),
              Rf("riga4", "Riga 4", "specie", "(vuota)"), Rf("riga5", "Riga 5 (in basso)", "specie", "(vuota)"),
              S("ingresso", "Traiettorie d'ingresso", { "A sorpresa", "Giro della morte", "Curva dal basso", "Serpentina", "Spirale", "Picchiata e risalita", "Doppio otto" }),
              N("picchiate", "Quanto spesso si tuffano", 0.2, 3, 0.05, "%.2f"), N("velocita", "Velocita' degli insetti", 0.5, 2, 0.05, "%.2f"),
              N("fuoco", "Quanto sparano", 0.2, 3, 0.05, "%.2f") }, 1, 60, "Livello");
        Cat("specie", "Insetti", true, "Le quattro specie: nome, due coppie di colori (la seconda si vede a righe alterne e quando lo scarabeo e' ferito), punti, colpi per abbatterli e velocita' in picchiata. "
            "Scarabeo: si tuffa con due farfalle di scorta. Farfalla: fa un anello. Libellula: scende a zig-zag.",
            { T("nome", "Nome"), G, C("chiaro", "Colore chiaro"), C("scuro", "Colore scuro"), C("chiaro2", "Secondo colore chiaro"), C("scuro2", "Secondo colore scuro"),
              N("punti", "Punti in formazione", 10, 5000, 10), N("puntiVolo", "Punti in picchiata", 10, 5000, 10), N("vita", "Colpi per abbatterlo", 1, 9, 1),
              N("velocita", "Velocita' in picchiata", 0.5, 2.5, 0.05, "%.2f") }, 4, 4, "Insetto");
        Cat("boss", "Boss", true, "I boss: nome, comportamento (ognuno ha i suoi attacchi e punti deboli), resistenza, aggressivita', punti e musica.",
            { T("nome", "Nome"), G, S("comportamento", "Comportamento", { "Regina Vespa (ventagli, evoca, carica)", "Idra (tre teste, poi il nucleo)",
                                                                         "Occhio (si apre, spirali, raggio)", "Corazzata (guscio che si apre, cannoni)", "Imperatrice (ali, spirali, evoca)" }),
              N("resistenza", "Resistenza", 0.3, 4, 0.05, "%.2f"), N("aggressivita", "Aggressivita'", 0.3, 3, 0.05, "%.2f"), N("punti", "Punti", 0, 100000, 500),
              B("musica", "Musica") }, 1, 20, "Boss");
        Cat("settori", "Settori", true, "Gli sfondi: i colori del cielo, della luce e del pianeta in basso, e la musica del settore.",
            { T("nome", "Nome"), Sf, C("alto", "Cielo in alto"), C("medio", "Cielo a meta'"), C("basso", "Cielo in basso"), C("luce", "Luce e nebulose"),
              C("chiaro", "Pianeta e luna"), B("musica", "Musica") }, 5, 5, "Settore");
        Cat("generale", "Caccia e regole", false, "Le regole della partita e il caccia del giocatore.",
            { G, N("vite", "Vite all'inizio", 1, 9, 1), N("vitaExtra", "Prima vita extra a", 0, 300000, 1000), N("vitaOgni", "Poi una vita ogni", 1000, 500000, 1000),
              N("velocitaCaccia", "Velocita' del caccia", 300, 1500, 20), N("colpi", "Colpi in volo insieme", 1, 6, 1),
              N("aura", "Insetti con l'aura dorata (armi)", 0, 0.6, 0.01, "%.2f"), B("musicaMenu", "Musica del menu"), T("fine", "Scritta della vittoria") });
        Cat("armi", "Armi bonus", false, "Quanti secondi durano Laser e Missili. Scudo e Doppio caccia durano fino al primo colpo preso.",
            { N("laser", "Laser (secondi)", 1, 90, 1), N("missili", "Missili (secondi)", 1, 90, 1) });
        json su = json::array();
        for (auto& n : NomiSuoni()) su.push_back({ { "chiave", n.first }, { "nome", n.second }, { "tipo", "effetto" } });
        Cat("suoni", "Effetti sonori", false, "Ogni suono del gioco: lascia 'predefinito' o scegli un effetto creato nella sezione Audio.", su);
        return voci;
    }
    int Tappe() const override { return NLiv(); }
    void IniziaDa(int tappa) override { livello = std::max(1, std::min(NLiv(), tappa + 1)); IniziaLivello(); }

    // --- stato ---
    Caso caso;
    uint64_t seme = 1, casoInizio = 0;
    Regista regista;
    int livello = 1, vite = 3, prossimaVita = 30000, record = 0;
    std::vector<Insetto> ins;
    std::vector<Proiettile> nemici;
    std::vector<Colpo> colpi;
    std::vector<Capsula> capsule;
    std::vector<Anello> anelli;
    Boss boss;
    float px = 960, tRicarica = 0, tInv = 0, tMorto = 0;
    int arma = 0; float tArma = 0;   // 0 base, 1 laser, 2 missili
    bool scudo = false, doppio = false;
    float tFormazione = 0, respiro = 0, tPicchiata = 3, tLivello = 0, tFine = -1, tMissile = 0;
    int gruppiEntrati = 0; float tGruppo = 0;
    std::vector<std::vector<int>> gruppi;   // indici degli insetti per gruppo d'ingresso
    std::vector<int> modelli;               // modello di traiettoria per gruppo
    std::string scritta, sotto; float tScritta = 0; Color colScritta = WHITE;
    float tScosse = 0;
    Particelle part;
    struct Testo { Vector2 p; std::string t; float vita; Color c; };
    std::vector<Testo> testi;
    float colpiLivello = 0, aSegnoLivello = 0;

    int Settore() const { return std::max(0, std::min(4, IntDi(Liv(), "settore", (livello - 1) / 3))); }
    bool LivelloBoss() const { return IndiceBoss() >= 0; }

    void Prepara() override { PreparaGrafica(); PreparaSuoni(); }

    // ======================= GRAFICA =======================
    static void Scarabeo(K& c, const std::string& c1, const std::string& c2, int f) {
        if (f) {   // ali membranose aperte sotto le elitre
            c.fillStyle(V("rgba(220,240,255,0.55)"));
            for (int k = -1; k <= 1; k += 2) { c.beginPath(); c.ellipse(k * 22.0f, 6, 18, 8, k * 0.5f, 0, TAU); c.fill(); }
        }
        c.glow(10, rgba(c1, 0.6f));
        c.fillStyle(lg(-18, -18, 18, 22, { "#ffffff", c1, c2 }));
        float ap = f ? 0.18f : 0;
        for (int k = -1; k <= 1; k += 2) {   // due elitre
            c.save(); c.rotate(k * ap);
            c.beginPath(); c.ellipse(k * 8.0f, 4, 10, 20, 0, 0, TAU); c.fill();
            c.restore();
        }
        c.noGlow();
        c.strokeStyle(V("#101020")); c.lineWidth(2); c.beginPath(); c.moveTo(0, -14); c.lineTo(0, 23); c.stroke();
        c.fillStyle(V("rgba(255,255,255,0.45)")); c.beginPath(); c.ellipse(-8, -4, 3, 8, 0.2f, 0, TAU); c.fill();
        c.fillStyle(V("#101020")); c.beginPath(); c.ellipse(0, -17, 10, 7, 0, 0, TAU); c.fill();
        c.strokeStyle(V(c1)); c.lineWidth(3);
        for (int k = -1; k <= 1; k += 2) { c.beginPath(); c.moveTo(k * 6.0f, -21); c.quadraticCurveTo(k * 14.0f, -32, k * 21.0f, -27); c.stroke(); }
        c.fillStyle(V("#ff3050")); c.fillRect(-7, -20, 4, 3); c.fillRect(3, -20, 4, 3);
        // zampette corte
        c.strokeStyle(V("#101020")); c.lineWidth(2.5f);
        for (int k = -1; k <= 1; k += 2) for (int j = 0; j < 3; j++) { c.beginPath(); c.moveTo(k * 15.0f, -4.0f + j * 9); c.lineTo(k * (22.0f + (f ? 2 : 0)), -8.0f + j * 10); c.stroke(); }
    }
    static void Farfalla(K& c, const std::string& c1, const std::string& c2, int f) {
        float a = f ? 0.72f : 1.0f;
        c.glow(10, rgba(c1, 0.6f));
        for (int k = -1; k <= 1; k += 2) {
            c.fillStyle(lg(0, -22, 0, 20, { "#ffffff", c1, c2 }));
            c.beginPath(); c.ellipse(k * 17 * a, -8, 17 * a, 13, k * 0.5f, 0, TAU); c.fill();
            c.beginPath(); c.ellipse(k * 13 * a, 11, 12 * a, 9, -k * 0.4f, 0, TAU); c.fill();
        }
        c.noGlow();
        for (int k = -1; k <= 1; k += 2) {
            c.fillStyle(V("rgba(255,255,255,0.7)")); c.beginPath(); c.arc(k * 19 * a, -9, 4.5f, 0, TAU); c.fill();
            c.fillStyle(V(rgba(c2, 0.8f))); c.beginPath(); c.arc(k * 12 * a, 11, 3.5f, 0, TAU); c.fill();
        }
        c.fillStyle(lg(0, -16, 0, 16, { "#5a5a90", "#202040" })); c.beginPath(); c.ellipse(0, 0, 4, 16, 0, 0, TAU); c.fill();
        c.strokeStyle(V("#e0e0ff")); c.lineWidth(2);
        c.beginPath(); c.moveTo(-2, -15); c.lineTo(-9, -27); c.moveTo(2, -15); c.lineTo(9, -27); c.stroke();
        c.fillStyle(V("#ffffff")); c.beginPath(); c.arc(-9, -27, 2.2f, 0, TAU); c.arc(9, -27, 2.2f, 0, TAU); c.fill();
    }
    static void Vespa(K& c, const std::string& c1, const std::string& c2, int f) {
        float w = f ? -0.9f : -0.4f;
        for (int k = -1; k <= 1; k += 2) {
            c.fillStyle(V("rgba(220,250,255,0.7)"));
            c.beginPath(); c.ellipse(k * 21.0f, -8, 20, 9, k * w, 0, TAU); c.fill();
            c.strokeStyle(V("rgba(255,255,255,0.8)")); c.lineWidth(1.2f); c.stroke();
        }
        c.glow(10, rgba(c1, 0.6f));
        c.fillStyle(lg(0, -20, 0, 26, { "#ffffff", c1, c2 })); c.beginPath(); c.ellipse(0, 5, 14, 22, 0, 0, TAU); c.fill(); c.noGlow();
        c.fillStyle(V("#14101c")); c.fillRect(-13, 5, 26, 5); c.fillRect(-11, 15, 22, 4);
        c.fillStyle(V("#14101c")); c.beginPath(); c.moveTo(-4, 25); c.lineTo(4, 25); c.lineTo(0, 33); c.closePath(); c.fill();
        c.fillStyle(V("#ffffff")); c.beginPath(); c.arc(-6, -10, 4.5f, 0, TAU); c.arc(6, -10, 4.5f, 0, TAU); c.fill();
        c.fillStyle(V("#000000")); c.beginPath(); c.arc(-5, -9, 2.2f, 0, TAU); c.arc(7, -9, 2.2f, 0, TAU); c.fill();
    }
    static void Libellula(K& c, const std::string& c1, const std::string&, int f) {
        float w = f ? 0.25f : -0.1f;
        c.fillStyle(V("rgba(220,250,255,0.65)"));
        for (int k = -1; k <= 1; k += 2) {
            c.beginPath(); c.ellipse(k * 21.0f, -8, 21, 5, k * (-0.15f + w), 0, TAU); c.fill();
            c.beginPath(); c.ellipse(k * 18.0f, 2, 18, 4, k * (0.2f - w), 0, TAU); c.fill();
        }
        c.glow(10, rgba(c1, 0.6f));
        c.fillStyle(lg(0, -14, 0, 30, { "#ffffff", c1, "#103040" }));
        c.fillRect(-3, -6, 6, 34);
        c.beginPath(); c.arc(0, -11, 7.5f, 0, TAU); c.fill(); c.noGlow();
        for (int i = 0; i < 4; i++) { c.fillStyle(V("rgba(0,0,0,0.35)")); c.fillRect(-3, 4.0f + i * 6, 6, 2); }
        c.fillStyle(V("#ff4060")); c.beginPath(); c.arc(-3, -13, 2.8f, 0, TAU); c.arc(3, -13, 2.8f, 0, TAU); c.fill();
    }
    static void Caccia(K& c) {
        c.glow(18, "rgba(120,180,255,0.5)");
        c.fillStyle(lg(-60, 0, 60, 0, { "#b8c0d8", "#ffffff", "#b8c0d8" }));
        static const float P[][2] = { { 0, -78 }, { 10, -50 }, { 12, -24 }, { 30, -18 }, { 30, -4 }, { 48, 4 }, { 60, -20 }, { 66, -20 }, { 66, 36 }, { 56, 40 }, { 30, 26 }, { 18, 32 },
                                      { -18, 32 }, { -30, 26 }, { -56, 40 }, { -66, 36 }, { -66, -20 }, { -60, -20 }, { -48, 4 }, { -30, -4 }, { -30, -18 }, { -12, -24 }, { -10, -50 } };
        c.beginPath();
        for (auto& q : P) c.lineTo(q[0], q[1]);
        c.closePath(); c.fill(); c.noGlow();
        c.strokeStyle(V("#1a2040")); c.lineWidth(2.5f); c.stroke();
        c.fillStyle(V("#e02030"));
        for (int s = -1; s <= 1; s += 2) {
            c.beginPath(); c.moveTo(s * 60.0f, -20); c.lineTo(s * 66.0f, -20); c.lineTo(s * 66.0f, 36); c.lineTo(s * 56.0f, 40); c.lineTo(s * 54.0f, 6); c.closePath(); c.fill();
            c.fillRect(s > 0 ? 30.0f : -38.0f, -18, 8, 22);
        }
        c.fillStyle(V("#2a50d0")); c.beginPath(); c.moveTo(-6, -40); c.lineTo(6, -40); c.lineTo(8, 10); c.lineTo(-8, 10); c.closePath(); c.fill();
        c.fillStyle(lg(0, -36, 0, -8, { "#e8f8ff", "#bfe8ff", "#3060c0" })); c.beginPath(); c.ellipse(0, -22, 7, 14, 0, 0, TAU); c.fill();
        c.fillStyle(V("#e02030")); c.beginPath(); c.moveTo(-4, -78); c.lineTo(4, -78); c.lineTo(0, -90); c.closePath(); c.fill();
        c.fillStyle(V("rgba(255,255,255,0.6)")); c.fillRect(-64, -16, 3, 48); c.fillRect(61, -16, 3, 48);
    }

    void PreparaGrafica() {
        std::string firma = cfg["settori"].dump() + cfg["specie"].dump();
        if (cacciaTex.id && firma == gFirma) return;
        bool primaVolta = !cacciaTex.id;
        if (!primaVolta) { for (auto& t : sfondi) UnloadTexture(t); for (auto& a : insTex) for (auto& b : a) for (auto& t : b) UnloadTexture(t); }
        gFirma = firma;
        std::string cieli[5][5];
        for (int k = 0; k < 5; k++) {
            const char* ch[5] = { "alto", "medio", "basso", "luce", "chiaro" };
            for (int i = 0; i < 5; i++) cieli[k][i] = Esa(ColoreDi(SettoreJ(k), ch[i], GRAY));
        }
        for (int k = 0; k < 5; k++)
            sfondi[k] = Dipingi(1280, 720, [&](K& c) {
                Caso r; r.Semina(500 + k);
                c.fillStyle(lgs(0, 0, 0, 720, { { 0, cieli[k][0] }, { 0.6f, cieli[k][1] }, { 1, cieli[k][2] } })); c.fillRect(0, 0, 1280, 720);
                c.lighter();
                for (int i = 0; i < 6; i++) {
                    float x = r.F(0, 1280), y = r.F(0, 560), rad = r.F(180, 460);
                    std::string col = i % 2 ? cieli[k][3] : std::string("#7a40ff");
                    c.fillStyle(rgs(x, y, 0, rad, { { 0, rgba(col, 0.16f) }, { 1, rgba(col, 0) } }));
                    c.beginPath(); c.arc(x, y, rad, 0, TAU); c.fill();
                }
                for (int i = 0; i < 120; i++) {
                    c.fillStyle(V(rgba(cieli[k][4], r.F(0.02f, 0.05f))));
                    c.beginPath(); c.ellipse(r.F(0, 1280), r.F(0, 640), r.F(30, 120), r.F(4, 16), r.F(-0.5f, 0.5f), 0, TAU); c.fill();
                }
                c.normal();
                for (int i = 0; i < 380; i++) { c.fillStyle(V(rgba("#ffffff", r.F(0.25f, 0.9f)))); c.beginPath(); c.arc(r.F(0, 1280), r.F(0, 720), r.F(0.4f, 1.5f), 0, TAU); c.fill(); }
                // pianeta che si vede in basso
                c.fillStyle(rgs(640, 1260, 600, 700, { { 0, sh(cieli[k][3], -0.35f) }, { 0.8f, sh(cieli[k][3], -0.55f) }, { 0.95f, cieli[k][1] }, { 1, "rgba(0,0,0,0)" } }));
                c.beginPath(); c.arc(640, 1260, 700, 0, TAU); c.fill();
                c.lighter();
                c.fillStyle(rgs(640, 580, 0, 600, { { 0, rgba(cieli[k][3], 0.12f) }, { 1, rgba(cieli[k][3], 0) } }));
                c.fillRect(0, 0, 1280, 720);
                c.normal();
                // una luna o un pianeta lontano diverso per settore
                float lx = k % 2 ? 1060 : 220, ly = 150, lr = 40 + k * 8;
                c.fillStyle(rgs(lx - lr * 0.3f, ly - lr * 0.3f, lr * 0.1f, lr * 1.1f, { { 0, "#ffffff" }, { 0.4f, cieli[k][4] }, { 1, cieli[k][1] } }));
                c.beginPath(); c.arc(lx, ly, lr, 0, TAU); c.fill();
            });
        for (int sp = 0; sp < 4; sp++)
            for (int v = 0; v < 2; v++)
                for (int f = 0; f < 2; f++)
                    insTex[sp][v][f] = Dipingi(132, 132, [&](K& c) {
                        c.translate(66, 66); c.scale(2, 2);
                        std::string a = Esa(ColSp(sp, v)), b = Esa(ColSpScuro(sp, v));
                        if (sp == 0) Scarabeo(c, a, b, f); else if (sp == 1) Farfalla(c, a, b, f); else if (sp == 2) Vespa(c, a, b, f); else Libellula(c, a, b, f);
                    });
        if (!primaVolta) return;
        cacciaTex = Dipingi(250, 220, [&](K& c) { c.translate(125, 120); c.scale(1.6f, 1.6f); Caccia(c); });
        const char* capCol[4] = { "#ff5a5a", "#5ac8ff", "#7aff7a", "#ffd23c" };
        for (int i = 0; i < 4; i++)
            capsTex[i] = Dipingi(120, 90, [&](K& c) {
                c.glow(14, capCol[i]);
                c.fillStyle(lg(0, 20, 0, 70, { "#ffffff", capCol[i], sh(capCol[i], -0.4f) }));
                c.beginPath(); c.moveTo(38, 23); c.lineTo(82, 23); c.arc(82, 45, 22, -TAU / 4, TAU / 4); c.lineTo(38, 67); c.arc(38, 45, 22, TAU / 4, TAU * 3 / 4); c.closePath(); c.fill();
                c.noGlow(); c.strokeStyle(V("#ffffff")); c.lineWidth(2.5f); c.stroke();
            });
        sferaTex = Dipingi(64, 64, [&](K& c) {
            c.fillStyle(rgs(26, 26, 2, 30, { { 0, "#ffffff" }, { 0.4f, "#ffb0e0" }, { 1, "#c0207a" } }));
            c.beginPath(); c.arc(32, 32, 28, 0, TAU); c.fill();
        });
        PreparaBoss();
    }

    void PreparaBoss() {
        // 0. Regina Vespa di Cristallo (corpo; le ali si disegnano a parte e battono)
        bossTex[0] = Dipingi(420, 520, [&](K& c) {
            c.translate(210, 230);
            c.glow(30, "rgba(255,60,120,0.5)");
            c.fillStyle(lgs(0, -80, 0, 200, { { 0, "#ffe36a" }, { 0.35f, "#ff8a1a" }, { 0.7f, "#b0105a" }, { 1, "#3a0430" } }));
            c.beginPath(); c.ellipse(0, 40, 70, 130, 0, 0, TAU); c.fill(); c.noGlow();
            c.strokeStyle(V("#2a0024")); c.lineWidth(10);
            for (int k = 0; k < 4; k++) { c.beginPath(); c.ellipse(0, 40.0f + k * 32 - 30, 66.0f - k * 6, 10, 0, 0, TAU / 2); c.stroke(); }
            c.fillStyle(V("rgba(255,255,255,0.3)")); c.beginPath(); c.ellipse(-28, 10, 12, 50, 0.1f, 0, TAU); c.fill();
            c.fillStyle(lgs(0, -150, 0, -40, { { 0, "#ffffff" }, { 0.3f, "#7ff6ff" }, { 1, "#1a5aa0" } }));
            c.beginPath(); c.moveTo(0, -170); c.lineTo(70, -90); c.lineTo(50, -40); c.lineTo(-50, -40); c.lineTo(-70, -90); c.closePath(); c.fill();
            c.strokeStyle(V("#e8ffff")); c.lineWidth(3); c.stroke();
            for (int s = -1; s <= 1; s += 2) {
                c.glow(20, "rgba(255,40,90,0.9)");
                c.fillStyle(V("#ff1f5a")); c.beginPath(); c.ellipse(s * 30.0f, -90, 18, 24, s * 0.4f, 0, TAU); c.fill(); c.noGlow();
                c.fillStyle(V("#ffffff")); c.beginPath(); c.arc(s * 26.0f, -98, 5, 0, TAU); c.fill();
                c.strokeStyle(V("#ffd0ff")); c.lineWidth(5); c.beginPath(); c.moveTo(s * 20.0f, -160); c.quadraticCurveTo(s * 80.0f, -210, s * 110.0f, -196); c.stroke();
                c.glow(14, "rgba(255,200,255,0.9)"); c.fillStyle(V("#ffffff")); c.beginPath(); c.arc(s * 110.0f, -196, 9, 0, TAU); c.fill(); c.noGlow();
            }
            c.fillStyle(V("#ffffee")); c.beginPath(); c.moveTo(-14, 165); c.lineTo(14, 165); c.lineTo(0, 225); c.closePath(); c.fill();
            // corona di cristalli
            for (int i = -2; i <= 2; i++) {
                c.fillStyle(lg(0, -230, 0, -160, { "#ffffff", "#9af6ff" }));
                c.beginPath(); c.moveTo(i * 22.0f - 9, -166); c.lineTo(i * 22.0f, -206.0f + std::abs(i) * 12); c.lineTo(i * 22.0f + 9, -166); c.closePath(); c.fill();
            }
        });
        alaRegina = Dipingi(380, 300, [&](K& c) {
            c.translate(20, 170);
            c.fillStyle(lg(0, -120, 260, 60, { "rgba(255,190,240,0.85)", "rgba(140,40,255,0.35)" }));
            c.beginPath(); c.moveTo(10, -30); c.bezierCurveTo(170, -200, 360, -140, 330, -10); c.bezierCurveTo(300, 60, 160, 40, 10, 10); c.closePath(); c.fill();
            c.strokeStyle(V("rgba(255,220,255,0.9)")); c.lineWidth(3); c.stroke();
            c.strokeStyle(V("rgba(255,255,255,0.35)")); c.lineWidth(2);
            for (int k = 0; k < 5; k++) { c.beginPath(); c.moveTo(20, -10); c.lineTo(110.0f + k * 45, -150.0f + k * 30); c.stroke(); }
            c.fillStyle(lg(30, 40, 220, 120, { "#ff6ab0", "#5a0a6a" }));
            c.beginPath(); c.moveTo(10, 20); c.bezierCurveTo(150, 40, 230, 100, 200, 120); c.bezierCurveTo(130, 110, 60, 80, 10, 40); c.closePath(); c.fill();
        });
        // 1. Idra Meccanica: corpo-fortezza e teste su colli a segmenti
        bossTex[1] = Dipingi(560, 360, [&](K& c) {
            c.translate(280, 170);
            c.glow(24, "rgba(80,200,255,0.5)");
            c.fillStyle(lg(0, -120, 0, 150, { "#d8e4f0", "#7a8aa8", "#2a3048" }));
            c.beginPath(); c.moveTo(-220, -40); c.lineTo(-140, -130); c.lineTo(140, -130); c.lineTo(220, -40); c.lineTo(180, 110); c.lineTo(-180, 110); c.closePath(); c.fill(); c.noGlow();
            c.strokeStyle(V("#1a2030")); c.lineWidth(5); c.stroke();
            for (int i = -3; i <= 3; i++) { c.fillStyle(V("#3a4460")); c.fillRect(i * 50.0f - 14, -110, 28, 18); c.fillStyle(V("rgba(120,220,255,0.8)")); c.fillRect(i * 50.0f - 10, -106, 20, 6); }
            c.fillStyle(rgs(0, 10, 4, 80, { { 0, "#ffffff" }, { 0.3f, "#6af0ff" }, { 1, "#0a3050" } }));
            c.beginPath(); c.arc(0, 10, 70, 0, TAU); c.fill();
            c.strokeStyle(V("#c0d0e0")); c.lineWidth(8); c.beginPath(); c.arc(0, 10, 76, 0, TAU); c.stroke();
            for (int i = 0; i < 12; i++) { float a = i * TAU / 12; c.fillStyle(V("#5a6480")); c.beginPath(); c.arc(std::cos(a) * 92, 10 + std::sin(a) * 92, 7, 0, TAU); c.fill(); }
            for (int s = -1; s <= 1; s += 2) {
                c.fillStyle(lg(0, -20, 0, 80, { "#9aa8c0", "#3a4460" }));
                c.beginPath(); c.moveTo(s * 120.0f, -20); c.lineTo(s * 200.0f, -10); c.lineTo(s * 180.0f, 90); c.lineTo(s * 110.0f, 90); c.closePath(); c.fill();
                c.fillStyle(V("#ff7a3a")); c.fillRect(s * 160.0f - 10, 30, 20, 30);
            }
        });
        testaIdra = Dipingi(200, 200, [&](K& c) {
            c.translate(100, 90);
            c.glow(16, "rgba(255,120,60,0.6)");
            c.fillStyle(lg(0, -60, 0, 70, { "#e8eef8", "#8a9ab8", "#2a3048" }));
            c.beginPath(); c.moveTo(-50, -50); c.lineTo(50, -50); c.lineTo(66, 0); c.lineTo(40, 60); c.lineTo(14, 80); c.lineTo(-14, 80); c.lineTo(-40, 60); c.lineTo(-66, 0); c.closePath(); c.fill(); c.noGlow();
            c.strokeStyle(V("#1a2030")); c.lineWidth(4); c.stroke();
            c.fillStyle(V("#2a0a0a")); c.beginPath(); c.moveTo(-30, 40); c.lineTo(30, 40); c.lineTo(14, 74); c.lineTo(-14, 74); c.closePath(); c.fill();
            for (int i = -2; i <= 2; i++) { c.fillStyle(V("#f0f0f0")); c.beginPath(); c.moveTo(i * 10.0f - 4, 40); c.lineTo(i * 10.0f + 4, 40); c.lineTo(i * 10.0f, 50); c.closePath(); c.fill(); }
            for (int s = -1; s <= 1; s += 2) {
                c.glow(14, "rgba(255,80,40,0.9)"); c.fillStyle(V("#ff5a2a")); c.beginPath(); c.moveTo(s * 14.0f, -6); c.lineTo(s * 46.0f, -16); c.lineTo(s * 40.0f, 6); c.closePath(); c.fill(); c.noGlow();
                c.fillStyle(V("#5a6480")); c.beginPath(); c.moveTo(s * 40.0f, -50); c.lineTo(s * 70.0f, -84); c.lineTo(s * 56.0f, -44); c.closePath(); c.fill();
            }
        });
        // 2. Occhio della Nebulosa: occhio, tentacoli di luce e iride che segue il caccia
        bossTex[2] = Dipingi(640, 520, [&](K& c) {
            c.translate(320, 250);
            c.lighter();
            for (int i = 0; i < 10; i++) {
                float a = TAU / 4 + (i - 4.5f) * 0.32f;
                c.strokeStyle(V(rgba(i % 2 ? "#a06aff" : "#ff6ad8", 0.5f))); c.lineWidth(18.0f - std::abs(i - 4.5f) * 2);
                c.beginPath(); c.moveTo(std::cos(a) * 120, std::sin(a) * 120);
                c.bezierCurveTo(std::cos(a) * 200 + 40 * std::sin(i * 1.7f), std::sin(a) * 200, std::cos(a) * 240 - 50 * std::sin(i * 1.3f), std::sin(a) * 240 + 20, std::cos(a) * 280, std::sin(a) * 250);
                c.stroke();
            }
            c.fillStyle(rgs(0, 0, 60, 260, { { 0, "rgba(255,120,220,0.5)" }, { 1, "rgba(80,0,160,0)" } }));
            c.beginPath(); c.arc(0, 0, 260, 0, TAU); c.fill();
            c.normal();
            c.fillStyle(lg(0, -150, 0, 150, { "#5a2a8a", "#2a0a4a", "#10001a" }));
            c.beginPath(); c.ellipse(0, 0, 175, 140, 0, 0, TAU); c.fill();
            c.fillStyle(rgs(-30, -30, 10, 150, { { 0, "#ffffff" }, { 0.6f, "#f0e0ff" }, { 1, "#a080c0" } }));
            c.beginPath(); c.ellipse(0, 0, 150, 112, 0, 0, TAU); c.fill();
            c.strokeStyle(V("rgba(200,40,80,0.45)")); c.lineWidth(2);
            for (int i = 0; i < 14; i++) { float a = i * TAU / 14; c.beginPath(); c.moveTo(std::cos(a) * 148, std::sin(a) * 110); c.quadraticCurveTo(std::cos(a + 0.2f) * 110, std::sin(a + 0.2f) * 80, std::cos(a) * 80, std::sin(a) * 60); c.stroke(); }
        });
        irideTex = Dipingi(200, 200, [&](K& c) {
            c.fillStyle(rgs(100, 100, 10, 80, { { 0, "#ffffff" }, { 0.25f, "#ffe14a" }, { 0.55f, "#ff4a8a" }, { 0.9f, "#5a0a6a" }, { 1, "#200020" } }));
            c.beginPath(); c.arc(100, 100, 80, 0, TAU); c.fill();
            c.fillStyle(V("#06000a")); c.beginPath(); c.ellipse(100, 100, 16, 44, 0, 0, TAU); c.fill();
            c.fillStyle(V("rgba(255,255,255,0.85)")); c.beginPath(); c.arc(76, 72, 14, 0, TAU); c.fill();
        });
        // 3. Corazzata Scarabeo: corpo con nucleo, gusci che si aprono, cannoni ai lati
        bossTex[3] = Dipingi(620, 420, [&](K& c) {
            c.translate(310, 200);
            c.glow(26, "rgba(120,255,120,0.4)");
            c.fillStyle(lg(0, -150, 0, 170, { "#5a6a3a", "#2a3a1a", "#101a08" }));
            c.beginPath(); c.ellipse(0, 20, 250, 170, 0, 0, TAU); c.fill(); c.noGlow();
            c.fillStyle(rgs(0, 20, 6, 100, { { 0, "#ffffff" }, { 0.25f, "#c0ff6a" }, { 0.6f, "#3aa020" }, { 1, "#0a2a05" } }));
            c.beginPath(); c.arc(0, 20, 95, 0, TAU); c.fill();
            c.strokeStyle(V("#d0ff9a")); c.lineWidth(4);
            for (int i = 0; i < 6; i++) { float a = i * TAU / 6; c.beginPath(); c.moveTo(std::cos(a) * 30, 20 + std::sin(a) * 30); c.lineTo(std::cos(a) * 80, 20 + std::sin(a) * 80); c.stroke(); }
            c.fillStyle(V("#202a10")); c.beginPath(); c.ellipse(0, -150, 70, 40, 0, 0, TAU); c.fill();
            for (int s = -1; s <= 1; s += 2) { c.glow(12, "rgba(255,60,60,0.9)"); c.fillStyle(V("#ff3a3a")); c.fillRect(s * 30.0f - 8, -160, 16, 10); c.noGlow(); }
            c.strokeStyle(V("#c0d080")); c.lineWidth(6);
            for (int s = -1; s <= 1; s += 2) { c.beginPath(); c.moveTo(s * 40.0f, -180); c.quadraticCurveTo(s * 90.0f, -220, s * 140.0f, -190); c.stroke(); }
            for (int s = -1; s <= 1; s += 2) for (int j = 0; j < 3; j++) { c.strokeStyle(V("#1a220a")); c.lineWidth(12); c.beginPath(); c.moveTo(s * 220.0f, -20.0f + j * 60); c.lineTo(s * 290.0f, -40.0f + j * 70); c.stroke(); }
        });
        gusciTex = Dipingi(300, 380, [&](K& c) {   // guscio sinistro (il destro e' specchiato)
            c.translate(290, 190);
            c.glow(14, "rgba(160,255,120,0.4)");
            c.fillStyle(lg(-260, -160, 0, 160, { "#c8ff8a", "#5aa02a", "#1a4a0a" }));
            c.beginPath(); c.moveTo(0, -170); c.bezierCurveTo(-200, -170, -280, -40, -250, 60); c.bezierCurveTo(-220, 150, -100, 180, 0, 170); c.closePath(); c.fill(); c.noGlow();
            c.strokeStyle(V("#0a2a05")); c.lineWidth(5); c.stroke();
            c.fillStyle(V("rgba(255,255,255,0.35)")); c.beginPath(); c.ellipse(-150, -70, 40, 70, 0.4f, 0, TAU); c.fill();
            for (int i = 0; i < 5; i++) { c.fillStyle(V("rgba(10,40,5,0.4)")); c.beginPath(); c.arc(-60.0f - i * 34, -20.0f + (i % 2) * 70, 12, 0, TAU); c.fill(); }
        });
        cannoneTex = Dipingi(140, 160, [&](K& c) {
            c.translate(70, 70);
            c.fillStyle(lg(-40, 0, 40, 0, { "#8a9a6a", "#e0f0c0", "#4a5a2a" })); c.beginPath(); c.arc(0, 0, 44, 0, TAU); c.fill();
            c.strokeStyle(V("#1a220a")); c.lineWidth(4); c.stroke();
            c.fillStyle(lg(-14, 0, 14, 0, { "#3a4a1a", "#c0d090", "#3a4a1a" })); c.fillRect(-14, 10, 28, 70);
            c.glow(12, "rgba(255,200,60,0.9)"); c.fillStyle(V("#ffd23c")); c.beginPath(); c.arc(0, 0, 14, 0, TAU); c.fill(); c.noGlow();
        });
        // 4. Imperatrice dello Sciame: corpo regale e grandi ali di falena
        bossTex[4] = Dipingi(360, 520, [&](K& c) {
            c.translate(180, 240);
            c.glow(30, "rgba(255,200,80,0.5)");
            c.fillStyle(lgs(0, -60, 0, 230, { { 0, "#fff0b0" }, { 0.3f, "#ffb02a" }, { 0.7f, "#a02a6a" }, { 1, "#2a0a2a" } }));
            c.beginPath(); c.ellipse(0, 70, 62, 150, 0, 0, TAU); c.fill(); c.noGlow();
            c.strokeStyle(V("rgba(60,10,40,0.7)")); c.lineWidth(6);
            for (int k = 0; k < 6; k++) { c.beginPath(); c.ellipse(0, 10.0f + k * 34, 58.0f - k * 4, 9, 0, 0, TAU / 2); c.stroke(); }
            c.fillStyle(lg(0, -130, 0, -20, { "#ffffff", "#ffd0f0", "#a04a9a" })); c.beginPath(); c.ellipse(0, -70, 52, 56, 0, 0, TAU); c.fill();
            for (int s = -1; s <= 1; s += 2) {
                c.glow(18, "rgba(80,255,240,0.9)"); c.fillStyle(V("#4affe0")); c.beginPath(); c.ellipse(s * 22.0f, -76, 16, 22, s * 0.3f, 0, TAU); c.fill(); c.noGlow();
                c.strokeStyle(V("#ffe0a0")); c.lineWidth(4); c.beginPath(); c.moveTo(s * 14.0f, -120); c.bezierCurveTo(s * 40.0f, -200, s * 110.0f, -200, s * 130.0f, -170); c.stroke();
                for (int j = 0; j < 6; j++) { c.beginPath(); c.moveTo(s * (30.0f + j * 16), -150.0f - j * 4); c.lineTo(s * (36.0f + j * 16), -170.0f - j * 3); c.stroke(); }
            }
            // corona
            c.fillStyle(lg(0, -170, 0, -115, { "#fffbe0", "#ffc83a" }));
            c.beginPath(); c.moveTo(-40, -118); c.lineTo(-44, -150); c.lineTo(-22, -132); c.lineTo(0, -166); c.lineTo(22, -132); c.lineTo(44, -150); c.lineTo(40, -118); c.closePath(); c.fill();
            c.glow(10, "rgba(255,60,120,0.9)"); c.fillStyle(V("#ff3a7a")); c.beginPath(); c.arc(0, -140, 7, 0, TAU); c.fill(); c.noGlow();
        });
        alaImp = Dipingi(440, 460, [&](K& c) {   // ala sinistra
            c.translate(420, 200);
            c.fillStyle(lgs(-400, -180, 0, 200, { { 0, "rgba(255,230,140,0.95)" }, { 0.5f, "rgba(255,110,60,0.9)" }, { 1, "rgba(120,20,90,0.9)" } }));
            c.beginPath(); c.moveTo(0, -40); c.bezierCurveTo(-120, -200, -380, -200, -400, -80); c.bezierCurveTo(-410, 0, -300, 40, -200, 40);
            c.bezierCurveTo(-320, 120, -300, 240, -180, 240); c.bezierCurveTo(-90, 240, -30, 140, 0, 40); c.closePath(); c.fill();
            c.strokeStyle(V("#3a0a2a")); c.lineWidth(5); c.stroke();
            for (int i = 0; i < 6; i++) { c.strokeStyle(V("rgba(60,10,40,0.5)")); c.lineWidth(3); c.beginPath(); c.moveTo(-10, 0); c.quadraticCurveTo(-140.0f - i * 30, -120.0f + i * 50, -380.0f + i * 40, -110.0f + i * 60); c.stroke(); }
            c.glow(16, "rgba(80,255,240,0.8)");
            c.fillStyle(rgs(-260, -60, 4, 50, { { 0, "#ffffff" }, { 0.4f, "#4affe0" }, { 1, "#0a3a5a" } })); c.beginPath(); c.arc(-260, -60, 46, 0, TAU); c.fill();
            c.fillStyle(rgs(-170, 160, 4, 34, { { 0, "#ffffff" }, { 0.4f, "#ff6ad8" }, { 1, "#3a0a3a" } })); c.beginPath(); c.arc(-170, 160, 32, 0, TAU); c.fill();
            c.noGlow();
        });
    }

    // ======================= SUONI E MUSICA =======================
    void PreparaSuoni() {
        CreaSuono("sc_sparo", 0.2f, [](float t) { float f = 2200 * std::exp(-t * 18) + 500; return 0.32f * Onda(3, f * t) + 0.2f * Onda(0, f * 0.5f * t) * Inviluppo(t, 0.001f, 0.2f, 2); });
        CreaSuono("sc_esplodi", 0.5f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.35f; float f = 700 * std::exp(-t * 9); return (0.6f * y + 0.35f * Onda(1, f * t)) * Inviluppo(t, 0.002f, 0.5f, 2.4f); });
        CreaSuono("sc_tuffo", 1.1f, [](float t) { float f = 1500 - 1000 * t + 40 * std::sin(t * 50); return 0.18f * Onda(3, f * t) * Inviluppo(t, 0.05f, 1.1f, 1); });
        CreaSuono("sc_ingresso", 2.4f, [](float t) {
            static const float m[] = { 523, 659, 784, 1047, 784, 1047, 1319, 1568 };
            int n = std::min(7, (int)(t * 4)); float tt = t - n * 0.25f;
            return 0.22f * (Onda(0, m[n] * t) * 0.6f + Onda(1, m[n] * 0.5f * t)) * Inviluppo(tt, 0.005f, n == 7 ? 1.2f : 0.25f, 1.2f); });
        CreaSuono("sc_allarme", 2.0f, [](float t) { float f = std::fmod(t, 0.5f) < 0.25f ? 660 : 880; return 0.22f * Onda(2, f * t) * Inviluppo(t, 0.02f, 2.0f, 0.4f); });
        CreaSuono("sc_metallo", 0.15f, [](float t) { return 0.25f * (Onda(0, 1800 * t) * 0.5f + Onda(0, 2630 * t) * 0.4f + Rumore() * 0.3f) * Inviluppo(t, 0, 0.15f, 3); });
        CreaSuono("sc_boom", 2.6f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.08f; return (1.4f * y + 0.4f * Onda(3, 60 * std::exp(-t) * t)) * Inviluppo(t, 0.01f, 2.6f, 1.8f); });
        CreaSuono("sc_capsula", 0.6f, [](float t) { int n = (int)(t * 12); float f = 660 * std::pow(2.0f, (n % 6) * 4 / 12.0f); return 0.25f * Onda(1, f * t) * Inviluppo(t, 0, 0.6f, 1); });
        CreaSuono("sc_missile", 0.5f, [](float t) { static float y = 0; y += (Rumore() - y) * (0.1f + t); return 0.4f * y * Inviluppo(t, 0.02f, 0.5f, 1.5f); });
        CreaSuono("sc_laser", 0.3f, [](float t) { return 0.24f * Onda(2, (2400 - 3000 * t) * t) * Inviluppo(t, 0, 0.3f, 1.4f); });
        CreaSuono("sc_scudo", 0.5f, [](float t) { return 0.3f * Onda(3, (400 + 1200 * t) * t) * Inviluppo(t, 0, 0.5f, 1); });
        CreaSuono("sc_morte", 1.6f, [](float t) {
            static float y = 0; y += (Rumore() - y) * 0.2f;
            static const float m[] = { 880, 740, 622, 523, 440, 370, 311, 262 };
            int n = std::min(7, (int)(t * 6));
            return 0.5f * y * Inviluppo(t, 0, 1.0f, 2) + 0.2f * Onda(0, m[n] * t) * Inviluppo(t, 0, 1.6f, 1); });
        CreaSuono("sc_vita", 1.0f, [](float t) { static const float m[] = { 1047, 1319, 1568, 2093, 1568, 2093 }; int n = std::min(5, (int)(t * 7)); return 0.22f * Onda(0, m[n] * t) * Inviluppo(t, 0, 1.0f, 0.6f); });
        CreaSuono("sc_stemma", 1.4f, [](float t) { return 0.2f * (Onda(3, 1568 * t) + 0.6f * Onda(3, 2349 * t) + 0.4f * Onda(3, 3136 * t)) * Inviluppo(t, 0.005f, 1.4f, 2); });
        CreaSuono("sc_colpito", 0.12f, [](float t) { return 0.25f * Onda(0, 300 * t) * Inviluppo(t, 0, 0.12f, 2); });
    }
    std::vector<Brano> BraniPredefiniti() const override {
        std::vector<Brano> b;
        b.push_back(CreaBrano("Sciame - Cintura di Orione", 126, 8, {
            { "Guida", 0, 0.28f, "E5:1 B4:1 E5:1 G5:1 F#5:2 E5:1 D5:1 E5:2 B4:2 C5:1 D5:1 E5:2 G5:1 A5:1 B5:2 A5:1 G5:1 F#5:1 D5:1 E5:4" },
            { "Basso", 2, 0.4f, "E2:1 E2:1 E3:1 E2:1 C2:1 C2:1 C3:1 C2:1 D2:1 D2:1 D3:1 D2:1 B1:1 B1:1 B2:1 B1:1" },
            { "Arpa", 1, 0.16f, "E4:1 G4:1 B4:1 G4:1 C4:1 E4:1 G4:1 E4:1 D4:1 F#4:1 A4:1 F#4:1 B3:1 D#4:1 F#4:1 D#4:1" },
            { "Batteria", 4, 0.45f, "K.h.S.h.K.KhS.h.", true } }));
        b.push_back(CreaBrano("Sciame - Nebulosa del Cigno", 100, 8, {
            { "Campane", 3, 0.34f, "D5:2 F5:2 A5:3 G5:1 F5:2 E5:2 D5:4 C5:2 E5:2 G5:3 F5:1 E5:2 C5:2 D5:4" },
            { "Pad", 1, 0.22f, "D3:8 Bb2:8 C3:8 A2:8" },
            { "Battito", 4, 0.32f, "K...h.S.K.h.S.hh", true } }));
        b.push_back(CreaBrano("Sciame - Abisso di Cristallo", 138, 8, {
            { "Cristallo", 3, 0.3f, "A5:1 E5:1 A5:1 C6:1 B5:1 G5:1 E5:2 F5:1 C5:1 F5:1 A5:1 G5:2 E5:2 A5:1 E5:1 A5:1 C6:1 D6:1 C6:1 B5:2 C6:4 A5:4" },
            { "Basso", 0, 0.36f, "A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 G1:1 G2:1 G1:1 G2:1 E1:1 E2:1 E1:1 E2:1" },
            { "Batteria", 4, 0.5f, "K.hhS.hoK.KhS.hh", true } }));
        b.push_back(CreaBrano("Sciame - Campi di Vega", 112, 8, {
            { "Ottoni", 2, 0.28f, "G4:2 B4:1 D5:1 G5:2 F#5:1 E5:1 D5:2 B4:2 C5:2 E5:1 G5:1 A5:2 G5:1 F#5:1 G5:4 D5:4" },
            { "Basso", 2, 0.4f, "G1:2 G2:2 E1:2 E2:2 C2:2 C3:2 D2:2 D3:2" },
            { "Batteria", 4, 0.45f, "K..hS..hK.KhS.hh", true } }));
        b.push_back(CreaBrano("Sciame - Cuore dello Sciame", 144, 8, {
            { "Guida", 2, 0.27f, "C5:1 Eb5:1 G5:1 C6:1 Bb5:1 G5:1 Ab5:2 F5:1 Ab5:1 C6:1 F6:1 Eb6:1 C6:1 D6:2 C5:1 Eb5:1 G5:1 C6:1 D6:1 Eb6:1 D6:1 B5:1 C6:4" },
            { "Basso", 0, 0.38f, "C2:1 C3:1 C2:1 C3:1 Ab1:1 Ab2:1 Ab1:1 Ab2:1 F1:1 F2:1 F1:1 F2:1 G1:1 G2:1 G1:1 G2:1" },
            { "Batteria", 4, 0.5f, "KhhhShhhKhKhShho", true } }));
        b.push_back(CreaBrano("Sciame - Boss", 150, 8, {
            { "Allarme", 2, 0.26f, "E5:1 E5:1 F5:1 E5:1 G5:1 E5:1 Bb5:2 A5:1 A5:1 Bb5:1 A5:1 C6:1 A5:1 E5:2 E5:1 E5:1 F5:1 E5:1 G5:1 Bb5:1 C6:1 Db6:1 E6:4 E5:4" },
            { "Basso", 0, 0.42f, "E1:1 E2:1 E1:1 E2:1 E1:1 E2:1 F1:1 F2:1 E1:1 E2:1 E1:1 E2:1 G1:1 G2:1 F1:1 F2:1" },
            { "Batteria", 4, 0.55f, "KoKoSoKoKoKoSoKS", true } }));
        b.push_back(CreaBrano("Sciame - Tema", 118, 8, {
            { "Melodia", 0, 0.3f, "G4:1 C5:1 E5:1 G5:1 F5:2 E5:1 D5:1 C5:2 G4:2 A4:1 C5:1 F5:1 A5:1 G5:2 E5:2 F5:1 E5:1 D5:1 B4:1 C5:4" },
            { "Basso", 2, 0.4f, "C2:1 C3:1 C2:1 C3:1 A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 G1:1 G2:1 G1:1 G2:1" },
            { "Arpeggio", 1, 0.15f, "C4:1 E4:1 G4:1 E4:1 A3:1 C4:1 E4:1 C4:1 F3:1 A3:1 C4:1 A3:1 G3:1 B3:1 D4:1 B3:1" },
            { "Batteria", 4, 0.42f, "K...S...K.K.S..h", true } }));
        return b;
    }

    // ======================= PARTITA =======================
    void Nuova(uint64_t s, int g) override {
        seme = s; giocatore = g;
        caso.Semina(s ^ 0x5C1A3Eull);
        regista = Regista{};
        livello = 1; vite = IntDi(Gen(), "vite", 3); prossimaVita = IntDi(Gen(), "vitaExtra", 30000); punteggio = 0; tempo = 0;
        finito = vinto = false; puntiDiControllo = 0;
        arma = 0; tArma = 0; scudo = doppio = false; px = 960; tMorto = 0; tInv = 0;
        colpi.clear(); nemici.clear(); capsule.clear(); part.v.clear(); testi.clear(); anelli.clear();
        LeggiRecord();
        IniziaLivello();
    }
    void LeggiRecord() { std::ifstream f(FileAccanto(".record")); record = 0; if (f) f >> record; }
    void ScriviRecord() { if (punteggio <= record) return; record = punteggio; std::ofstream f(FileAccanto(".record")); if (f) f << record; }

    Vector2 Posto(const Insetto& a) const {
        float b = 1 + 0.07f * std::sin(respiro);   // la formazione "respira"
        float onda = tFormazione < 0 ? 0 : std::sin(tempo * 0.6f) * 40 * (1 - std::min(1.0f, respiro * 0.0f));
        float cx = 960 + (gruppiEntrati < (int)gruppi.size() ? onda : 0);
        return { cx + (a.col - 4.5f) * PASSO_C * b, 170 + a.riga * PASSO_R * b };
    }

    void IniziaLivello() {
        casoInizio = caso.s;
        ins.clear(); nemici.clear(); gruppi.clear(); modelli.clear();
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Colpo& c) { return c.missile; }), colpi.end());
        boss = Boss{};
        int sett = Settore();
        // la formazione: 4 scarabei, 16 farfalle, 20 vespe (o libellule dal livello 4)
        auto Aggiungi = [&](int riga, int col, int sp) {
            Insetto a; a.riga = riga; a.col = col; a.specie = sp;
            a.var = (sett + (sp == 0 ? 0 : riga)) % 2;
            a.vita = std::max(1, IntDi(SpecieJ(sp), "vita", sp == 0 ? 2 : 1));
            a.aura = caso.P(NumDi(Gen(), "aura", 0.11f));
            a.vivo = true; a.stato = 0; a.p = { -999, -999 };
            ins.push_back(a);
            return (int)ins.size() - 1;
        };
        // specie di ogni riga (dai dati del livello; -1 = riga vuota)
        int sr[5];
        const char* chiavi[5] = { "riga1", "riga2", "riga3", "riga4", "riga5" };
        for (int k = 0; k < 5; k++) { sr[k] = IntDi(Liv(), chiavi[k], -1); if (sr[k] >= (int)cfg["specie"].size() || sr[k] > 3) sr[k] = -1; }
        auto Metti = [&](std::vector<int>& gr, int riga, int col) { if (sr[riga] >= 0) gr.push_back(Aggiungi(riga, col, sr[riga])); };
        std::vector<int> g[5];
        if (LivelloBoss()) {
            for (int c = 1; c <= 8; c++) for (int rr = 0; rr < 5; rr++) if (rr >= 3 || c % 2 == 0) Metti(g[c <= 4 ? 0 : 1], rr, c);
            boss.indice = IndiceBoss();
            boss.tipo = std::max(0, std::min(4, IntDi(BossJ(boss.indice), "comportamento", 0)));
            boss.vivo = true; boss.p = { 960, -300 }; boss.ingresso = 1;
            CreaParti();
        } else {
            for (int c = 3; c <= 6; c++) Metti(g[0], 3, c);
            for (int c = 3; c <= 6; c++) Metti(g[0], 1, c);
            for (int c = 3; c <= 6; c++) Metti(g[1], 0, c);
            for (int c : { 1, 2, 7, 8 }) Metti(g[1], 1, c);
            for (int c = 1; c <= 8; c++) Metti(g[2], 2, c);
            for (int c : { 0, 1, 2, 7, 8, 9 }) Metti(g[3], 3, c);
            for (int c : { 0, 1 }) Metti(g[3], 4, c);
            for (int c = 2; c <= 9; c++) Metti(g[4], 4, c);
        }
        int ingresso = IntDi(Liv(), "ingresso", 0);
        for (auto& v : g) if (!v.empty()) { gruppi.push_back(v); int m = caso.I(0, 5); modelli.push_back(ingresso > 0 ? std::min(5, ingresso - 1 + (int)gruppi.size() % 2) : m); }
        gruppiEntrati = 0; tGruppo = 1.2f;
        tFormazione = 0; respiro = 0;
        tPicchiata = 4.0f;
        tLivello = 0; tFine = -1;
        colpiLivello = regista.colpiSparati; aSegnoLivello = regista.colpiAndatiASegno;
        if (LivelloBoss()) {
            scritta = "ATTENZIONE!"; sotto = "Arriva il boss: " + NomeBoss(); colScritta = Hex(0xff5a7a);
            Suona("sc_allarme", 0.8f);
            SuonaMusica(BranoDi(TestoDi(BossJ(boss.indice), "musica", "Sciame - Boss")));
        } else {
            scritta = "LIVELLO " + std::to_string(livello); sotto = NomeSettore(sett); colScritta = WHITE;
            Suona("sc_ingresso", 0.8f);
            SuonaMusica(BranoDi(TestoDi(SettoreJ(sett), "musica")));
        }
        tScritta = 2.8f;
        puntiDiControllo++;
    }

    void CreaParti() {
        float k = (0.85f + 0.3f * regista.sfida) * std::max(0.1f, NumDi(BossJ(boss.indice), "resistenza", 1));
        auto P = [&](float x, float y, float r, float hp, bool vul) { Parte q; q.off = { x, y }; q.r = r; q.hp = q.hpMax = std::round(hp * k); q.vulnerabile = vul; boss.parti.push_back(q); };
        switch (boss.tipo) {
            case 0: P(0, 20, 95, 90, true); break;
            case 1: P(0, 10, 90, 80, false); P(-260, 200, 55, 36, true); P(0, 230, 55, 36, true); P(260, 200, 55, 36, true); break;
            case 2: P(0, 0, 135, 170, true); break;
            case 3: P(0, 20, 90, 120, false); P(-250, 70, 50, 45, true); P(250, 70, 50, 45, true); break;
            default: P(0, 40, 90, 170, false); P(-240, -10, 120, 75, true); P(240, -10, 120, 75, true); break;
        }
    }

    // Traiettorie d'ingresso: sei modelli (anche specchiati) scelti dal seme.
    Percorso PercorsoIngresso(int modello, bool specchio, Vector2 posto) {
        Percorso p;
        auto M = [&](float x, float y) -> Vector2 { return { specchio ? 1920 - x : x, y }; };
        switch (modello) {
            case 0:   // dall'alto al centro, giro della morte in basso
                p.Punto(M(860, -60)); p.Bezier(M(860, 300), M(560, 520), M(560, 700));
                p.Arco(M(720, 700), 160, specchio ? 0 : TAU / 2, specchio ? -TAU / 2 * 1.5f : TAU / 2 + TAU * 0.75f);
                break;
            case 1:   // dal basso a sinistra, curva larga
                p.Punto(M(-60, 980)); p.Bezier(M(400, 900), M(700, 700), M(760, 480));
                p.Arco(M(620, 480), 140, specchio ? TAU / 2 : 0, specchio ? TAU / 2 + TAU * 0.6f : -TAU * 0.6f);
                break;
            case 2:   // dall'angolo in alto, serpentina
                p.Punto(M(200, -60)); p.Bezier(M(200, 400), M(700, 300), M(900, 600)); p.Bezier(M(1100, 900), M(1400, 500), M(1100, 380));
                break;
            case 3:   // dal lato, spirale
                p.Punto(M(-60, 420)); p.Bezier(M(300, 420), M(500, 560), M(700, 560));
                p.Arco(M(700, 440), 120, TAU / 4 * (specchio ? 1 : 1), specchio ? TAU / 4 + TAU * 1.0f : TAU / 4 - TAU * 1.0f);
                break;
            case 4:   // picchiata dall'alto e risalita
                p.Punto(M(1500, -60)); p.Bezier(M(1300, 500), M(900, 900), M(700, 760)); p.Bezier(M(500, 620), M(700, 420), M(900, 440));
                break;
            default:  // doppio otto
                p.Punto(M(-60, 200)); p.Bezier(M(500, 100), M(700, 700), M(960, 640)); p.Bezier(M(1220, 580), M(1100, 300), M(900, 360));
                break;
        }
        (void)posto;
        return p;
    }
    Percorso PercorsoPicchiata(const Insetto& a) {
        Percorso p;
        p.Punto(a.p);
        float lato = a.p.x < 960 ? -1.0f : 1.0f;
        float mira = caso.P(0.55f) ? px : regista.abitX * 1920;
        mira = std::max(120.0f, std::min(1800.0f, mira + caso.F(-120, 120)));
        // mezzo giro verso l'esterno, poi giu' verso il caccia
        p.Arco({ a.p.x + lato * 60, a.p.y }, 60, lato > 0 ? TAU / 2 : 0, lato > 0 ? TAU / 2 + TAU / 2 : -TAU / 2);
        Vector2 q = p.Ultimo();
        if (a.specie == 3) {   // la libellula scende a zig-zag
            float x = q.x, y = q.y;
            for (int i = 0; i < 5; i++) { float nx = mira + (i % 2 ? -1 : 1) * 220.0f, ny = y + 170; p.Bezier({ x, y + 60 }, { nx, ny - 60 }, { nx, ny }); x = nx; y = ny; }
            p.Bezier({ x, y + 80 }, { mira, 1100 }, { mira, 1180 });
        } else if (a.specie == 1 && caso.P(0.5f)) {   // la farfalla fa un anello a meta' strada
            p.Bezier({ q.x, q.y + 200 }, { mira - lato * 200, 520 }, { mira - lato * 100, 620 });
            Vector2 r = p.Ultimo();
            p.Arco({ r.x + lato * 90, r.y }, 90, lato > 0 ? TAU / 2 : 0, lato > 0 ? TAU / 2 - TAU : TAU);
            p.Bezier({ r.x, r.y + 200 }, { mira, 900 }, { mira + lato * 120, 1180 });
        } else {
            p.Bezier({ q.x, q.y + 260 }, { mira - lato * 260, 640 }, { mira, 860 });
            p.Bezier({ mira + lato * 100, 980 }, { mira + lato * 200, 1080 }, { mira + lato * 260, 1180 });
        }
        return p;
    }

    int Vivi() const { int n = 0; for (auto& a : ins) n += a.vivo; return n; }

    void Punti(int p, Vector2 dove, Color c) {
        punteggio += p;
        testi.push_back({ dove, std::to_string(p), 0.8f, c });
        if (punteggio >= prossimaVita) {
            vite++; prossimaVita += std::max(1000, IntDi(Gen(), "vitaOgni", 100000));
            Suona("sc_vita");
            testi.push_back({ { 960, 620 }, "VITA EXTRA!", 1.6f, Hex(0x8aff6a) });
        }
    }

    void Esplosione(Vector2 p, Color c, float forza) {
        part.Esplosione(p, c, (int)(22 * forza), 480 * std::sqrt(forza), 6, 0.6f);
        part.Esplosione(p, WHITE, (int)(8 * forza), 260, 4, 0.3f);
        anelli.push_back({ p, 0, 0.4f, 60 * forza, c });
    }

    void ColpisciInsetto(Insetto& a, int danno) {
        a.vita -= danno; a.lampo = 0.12f;
        if (a.vita > 0) { Suona("sc_metallo", 0.5f); a.var = (a.var + 1) % 2; return; }
        a.vivo = false;
        regista.Bersaglio();
        Color c = ColSp(a.specie, a.var);
        Esplosione(a.p, c, a.specie == 0 ? 1.4f : 1.0f);
        bool inVolo = a.stato == 3;
        int p = PuntiSp(a.specie, inVolo);
        if (a.specie == 0 && inVolo) p *= 2;
        Punti(p, { a.p.x, a.p.y - 30 }, c);
        Suona("sc_esplodi", 0.7f, Vis(0.85f, 1.2f));
        if (a.aura) capsule.push_back({ a.p, caso.I(0, 3) });
    }

    void SparaNemico(Vector2 da, float vel, int tipo, float r, float deviazione = 0) {
        float a = std::atan2(Y_CACCIA - da.y, px - da.x) + deviazione;
        nemici.push_back({ da, { std::cos(a) * vel, std::sin(a) * vel }, tipo, r });
    }
    void Ventaglio(Vector2 da, int n, float centro, float ampiezza, float vel, int tipo, float r) {
        for (int i = 0; i < n; i++) {
            float a = centro + (n == 1 ? 0 : (i / (float)(n - 1) - 0.5f) * ampiezza);
            nemici.push_back({ da, { std::cos(a) * vel, std::sin(a) * vel }, tipo, r });
        }
    }
    float VelProiettile() const { return (430 + livello * 10) * (0.8f + 0.2f * regista.sfida) * std::max(0.5f, std::sqrt(NumDi(Liv(), "fuoco", 1))); }

    void Spara() {
        if (tRicarica > 0) return;
        int miei = 0;
        for (auto& c : colpi) miei += !c.missile;
        int colpiMax = std::max(1, IntDi(Gen(), "colpi", 2));
        int massimo = colpiMax * (doppio ? 2 : 1) + (arma == 1 ? 1 : 0);
        if (miei >= massimo) return;
        tRicarica = 0.12f;
        auto Uno = [&](float x) { Colpo c; c.p = { x, Y_CACCIA - 70 }; c.laser = arma == 1; colpi.push_back(c); };
        if (doppio) { Uno(px - 34); Uno(px + 34); } else Uno(px);
        if (arma == 2 && tMissile <= 0) {
            int missili = 0;
            for (auto& c : colpi) missili += c.missile;
            if (missili < 4) {
                for (int s = -1; s <= 1; s += 2) { Colpo m; m.p = { px + s * 40.0f, Y_CACCIA - 30 }; m.missile = true; m.vx = s * 300.0f; colpi.push_back(m); }
                Suona("sc_missile", 0.5f);
                tMissile = 0.7f;
            }
        }
        regista.Sparo();
        Suona(arma == 1 ? "sc_laser" : "sc_sparo", 0.5f, Vis(0.97f, 1.04f));
    }

    void Colpito() {
        if (tMorto > 0 || tInv > 0) return;
        if (scudo) { scudo = false; tInv = 1.2f; Suona("sc_scudo"); Esplosione({ px, Y_CACCIA }, Hex(0x7aff7a), 1.2f); return; }
        if (doppio) { doppio = false; tInv = 1.5f; Suona("sc_esplodi"); Esplosione({ px + 34, Y_CACCIA }, Hex(0xffffff), 1.5f); testi.push_back({ { px, Y_CACCIA - 100 }, "DOPPIO PERSO", 1.2f, Hex(0xffd23c) }); return; }
        vite--;
        regista.VitaPersa();
        tMorto = 2.4f; tScosse = 0.7f;
        arma = 0; tArma = 0;
        Esplosione({ px, Y_CACCIA }, Hex(0xffffff), 2.5f);
        part.Esplosione({ px, Y_CACCIA }, Hex(0xff4a8a), 50, 700, 7, 1.1f);
        part.Esplosione({ px, Y_CACCIA }, Hex(0x4ad0ff), 40, 600, 6, 1.0f);
        Suona("sc_morte");
        nemici.clear();
        // chi era in picchiata torna al suo posto
        for (auto& a : ins) if (a.vivo && a.stato == 3) { a.stato = 4; a.p.y = std::min(a.p.y, 1100.0f); }
    }

    // ----------------------- insetti -----------------------
    void AggiornaInsetti(float dt) {
        // ingresso dei gruppi
        tGruppo -= dt;
        if (gruppiEntrati < (int)gruppi.size() && tGruppo <= 0 && (!LivelloBoss() || boss.ingresso <= 0.5f)) {
            const auto& g = gruppi[gruppiEntrati];
            int modello = modelli[gruppiEntrati];
            bool due = modello == 0 || modello == 4 || g.size() > 6;
            for (size_t i = 0; i < g.size(); i++) {
                Insetto& a = ins[g[i]];
                bool sp = due ? (i % 2 == 1) : (gruppiEntrati % 2 == 1);
                a.per = PercorsoIngresso(modello, sp, Posto(a));
                a.s = 0; a.stato = 0; a.ritardo = (due ? i / 2 : i) * 0.14f;
                a.vel = 620 + livello * 8;
                a.p = a.per.A(0);
                a.colpiDaSparare = livello >= 2 && caso.P(0.12f + livello * 0.01f) ? 1 : 0;
                a.tFuoco = caso.F(0.4f, 1.5f);
            }
            gruppiEntrati++;
            tGruppo = 2.6f;
        }
        bool tuttiEntrati = gruppiEntrati >= (int)gruppi.size();
        bool fermi = true;
        for (auto& a : ins) if (a.vivo && a.stato < 2) fermi = false;
        if (tuttiEntrati && fermi) { tFormazione += dt; respiro += dt * 1.6f; }

        for (auto& a : ins) {
            if (!a.vivo) continue;
            a.lampo -= dt;
            if (a.stato == 0 || a.stato == 3) {
                if (a.ritardo > 0) { a.ritardo -= dt; if (a.stato == 0) { a.p = { -999, -999 }; continue; } }
                a.s += a.vel * dt;
                float ang;
                a.p = a.per.A(a.s, &ang);
                a.ang = ang;
                // spari durante il volo
                if (a.colpiDaSparare > 0 && a.p.y > 200 && a.p.y < 720 && a.p.x > 0 && a.p.x < 1920) {
                    a.tFuoco -= dt;
                    if (a.tFuoco <= 0 && tMorto <= 0) { SparaNemico({ a.p.x, a.p.y + 20 }, VelProiettile(), 0, 9, caso.F(-0.08f, 0.08f)); a.colpiDaSparare--; a.tFuoco = 0.35f; }
                }
                if (a.s >= a.per.Lunghezza()) {
                    if (a.stato == 0) a.stato = 1;
                    else { a.stato = 4; a.p = { Posto(a).x, -60 }; }
                }
            } else if (a.stato == 1 || a.stato == 4) {
                Vector2 t = Posto(a);
                Vector2 d = Vector2Subtract(t, a.p);
                float l = Vector2Length(d), v = (a.stato == 1 ? 700.0f : 520.0f) * dt;
                if (l <= v) { a.p = t; a.stato = 2; }
                else { a.p = Vector2Add(a.p, Vector2Scale(d, v / l)); float na = std::atan2(d.y, d.x); a.ang += (na - a.ang) * std::min(1.0f, dt * 10); }
            } else {
                a.p = Posto(a);
                float target = -TAU / 4;
                float da = std::remainder(target - a.ang, TAU);
                a.ang += da * std::min(1.0f, dt * 6);
                // dal livello 8 anche la formazione spara ogni tanto
                if (livello >= 8 && tMorto <= 0 && caso.P(0.004f * regista.sfida * dt * 60 / 10)) SparaNemico({ a.p.x, a.p.y + 20 }, VelProiettile() * 0.8f, 0, 8);
            }
            // contatto con il caccia
            if (tMorto <= 0 && tInv <= 0 && (a.stato == 3 || a.stato == 0) && a.p.y > Y_CACCIA - 60 && a.p.y < Y_CACCIA + 50) {
                bool tocca = std::fabs(a.p.x - px) < 58 || (doppio && std::fabs(a.p.x - (px + 34)) < 58);
                if (tocca) { ColpisciInsetto(a, 9); Colpito(); }
            }
        }

        // le picchiate
        if (tuttiEntrati && tMorto <= 0) {
            tPicchiata -= dt;
            int inVolo = 0;
            for (auto& a : ins) inVolo += a.vivo && (a.stato == 3 || a.stato == 4);
            int maxVolo = 2 + livello / 3 + (regista.sfida > 1.2f ? 1 : 0);
            if (tPicchiata <= 0 && inVolo < maxVolo) {
                std::vector<int> pronti;
                for (size_t i = 0; i < ins.size(); i++) if (ins[i].vivo && ins[i].stato == 2) pronti.push_back((int)i);
                if (!pronti.empty()) {
                    int scelto = pronti[caso.I(0, (int)pronti.size() - 1)];
                    LanciaPicchiata(scelto);
                    // lo scarabeo porta con se' due farfalle di scorta
                    if (ins[scelto].specie == 0) {
                        int scorte = 0;
                        for (int i : pronti) if (scorte < 2 && i != scelto && ins[i].specie == 1 && std::abs(ins[i].col - ins[scelto].col) <= 2) {
                            Insetto& e = ins[i]; e.per = ins[scelto].per; e.s = -(++scorte) * 50.0f;
                            for (auto& q : e.per.p) q.x += (scorte == 1 ? -60 : 60);
                            e.stato = 3; e.vel = ins[scelto].vel; e.colpiDaSparare = 1; e.tFuoco = caso.F(0.5f, 1.2f); e.ritardo = 0;
                        }
                    }
                }
                int vivi = Vivi();
                float base = std::max(0.5f, 2.8f - livello * 0.12f) / regista.sfida / std::max(0.2f, NumDi(Liv(), "picchiate", 1));
                if (vivi < 10) base *= 0.6f;   // gli ultimi rimasti diventano nervosi
                tPicchiata = base * caso.F(0.7f, 1.3f);
            }
        }
    }
    void LanciaPicchiata(int i) {
        Insetto& a = ins[i];
        a.per = PercorsoPicchiata(a);
        a.s = 0; a.stato = 3; a.ritardo = 0;
        a.vel = (520 + livello * 14) * (0.85f + 0.15f * regista.sfida) * NumDi(SpecieJ(a.specie), "velocita", a.specie == 3 ? 1.25f : 1.0f) * NumDi(Liv(), "velocita", 1);
        a.colpiDaSparare = std::max(0, std::min(5, (int)std::lround((1 + livello / 5 + (regista.sfida > 1.1f ? 1 : 0)) * NumDi(Liv(), "fuoco", 1))));
        a.tFuoco = caso.F(0.3f, 0.9f);
        Suona("sc_tuffo", 0.35f, Vis(0.9f, 1.15f));
    }

    // ----------------------- boss -----------------------
    void AggiornaBoss(float dt) {
        Boss& b = boss;
        if (!b.vivo) return;
        b.t += dt;
        for (auto& q : b.parti) q.lampo -= dt;
        if (b.tMorte > 0) {
            b.tMorte -= dt;
            if (Vis() < dt * 14) { Vector2 q = { b.p.x + Vis(-220, 220), b.p.y + Vis(-160, 160) }; Esplosione(q, Vis() < 0.5f ? Hex(0xffd23c) : Hex(0xff5a8a), Vis(1, 2.2f)); Suona("sc_esplodi", 0.6f, Vis(0.6f, 1)); }
            if (b.tMorte <= 0) {
                b.vivo = false;
                for (int k = 0; k < 6; k++) Esplosione({ b.p.x + Vis(-150, 150), b.p.y + Vis(-120, 120) }, WHITE, 3);
                tScosse = 1.0f;
            }
            return;
        }
        if (b.ingresso > 0) {   // entra dall'alto
            b.ingresso = std::max(0.0f, b.ingresso - dt * 0.45f);
            b.p = { 960, 300 - b.ingresso * 620 };
            return;
        }
        float rabbia = 1 + (1 - b.Hp() / b.HpMax()) * 0.8f;   // piu' e' ferito, piu' e' veloce
        float s = regista.sfida * rabbia * std::max(0.2f, NumDi(BossJ(b.indice), "aggressivita", 1));
        b.tAtt -= dt * s;
        Vector2 bocca = { b.p.x, b.p.y + 120 };
        switch (b.tipo) {
            case 0: {   // Regina Vespa: ventagli di cristalli, vespe evocate, carica col pungiglione
                if (b.carica == 0) { b.p.x = 960 + std::sin(b.t * 0.5f) * 520; b.p.y = 300 + std::sin(b.t * 0.9f) * 40; }
                if (b.carica == 1) { b.tCarica -= dt; if (b.tCarica <= 0) { b.carica = 2; Suona("sc_tuffo", 0.7f, 0.7f); } }
                else if (b.carica == 2) { b.p.x += (b.caricaX - b.p.x) * std::min(1.0f, dt * 4); b.p.y += 900 * dt; if (b.p.y > 760) b.carica = 3; }
                else if (b.carica == 3) { b.p.y -= 500 * dt; if (b.p.y <= 300) { b.p.y = 300; b.carica = 0; b.t = std::asin(std::max(-1.0f, std::min(1.0f, (b.p.x - 960) / 520))) / 0.5f; } }
                if (b.carica >= 2 && tMorto <= 0 && tInv <= 0 && std::fabs(b.p.x - px) < 110 && b.p.y + 200 > Y_CACCIA - 40) Colpito();
                if (b.tAtt <= 0 && b.carica == 0) {
                    b.att = (b.att + 1) % 3;
                    if (b.att == 0) Ventaglio({ b.p.x, b.p.y + 150 }, rabbia > 1.4f ? 13 : 9, std::atan2(Y_CACCIA - b.p.y, px - b.p.x), 1.3f, VelProiettile(), 1, 11);
                    else if (b.att == 1) { EvocaInsetti(2, 3); Ventaglio({ b.p.x, b.p.y + 150 }, 5, TAU / 4, 0.8f, VelProiettile() * 0.8f, 1, 11); }
                    else { b.carica = 1; b.tCarica = 0.9f; b.caricaX = px; }
                    b.tAtt = 2.2f;
                }
                break;
            }
            case 1: {   // Idra Meccanica: tre teste, poi il nucleo
                b.p.x = 960 + std::sin(b.t * 0.35f) * 180; b.p.y = 200;
                int teste = 0;
                for (int i = 1; i <= 3; i++) {
                    Parte& q = b.parti[i];
                    q.off = { (i - 2) * 260 + std::sin(b.t * 0.9f + i * 2) * 70, 200 + std::sin(b.t * 1.3f + i) * 50 + (i == 2 ? 30 : 0) };
                    if (q.hp > 0) teste++;
                }
                b.parti[0].vulnerabile = teste == 0;
                if (b.tAtt <= 0) {
                    b.att++;
                    if (teste > 0) {
                        for (int i = 1; i <= 3; i++) {
                            Parte& q = b.parti[i];
                            if (q.hp <= 0 || (b.att + i) % (teste == 1 ? 1 : 2)) continue;
                            Vector2 t = Vector2Add(b.p, q.off);
                            for (int k = -1; k <= 1; k++) SparaNemico({ t.x, t.y + 50 }, VelProiettile() * 1.05f, 0, 9, k * 0.12f);
                        }
                        b.tAtt = 1.3f;
                    } else {
                        int n = 18;
                        for (int k = 0; k < n; k++) { float a = k * TAU / n + b.att * 0.17f; nemici.push_back({ bocca, { std::cos(a) * VelProiettile() * 0.75f, std::sin(a) * VelProiettile() * 0.75f }, 2, 14 }); }
                        SparaNemico(bocca, VelProiettile() * 1.2f, 0, 9);
                        b.tAtt = 1.6f;
                    }
                    Suona("sc_metallo", 0.4f, 0.6f);
                }
                break;
            }
            case 2: {   // Occhio della Nebulosa: si apre e si chiude, spirali e raggio
                b.p.x = 960 + std::sin(b.t * 0.4f) * 320; b.p.y = 320 + std::sin(b.t * 0.7f) * 30;
                b.tCiclo += dt;
                float aperto = std::fmod(b.tCiclo, 7.5f) < 5.0f ? 1.0f : 0.0f;
                b.aperto += (aperto - b.aperto) * std::min(1.0f, dt * 6);
                b.parti[0].vulnerabile = b.aperto > 0.6f;
                if (b.raggio == 1) { b.tRaggio -= dt; if (b.tRaggio <= 0) { b.raggio = 2; b.tRaggio = 0.9f; Suona("sc_laser", 1, 0.5f); tScosse = 0.4f; } }
                else if (b.raggio == 2) {
                    b.tRaggio -= dt;
                    if (tMorto <= 0 && std::fabs(px - b.raggioX) < 56) Colpito();
                    if (b.tRaggio <= 0) b.raggio = 0;
                }
                if (b.tSpirale > 0) {
                    b.tSpirale -= dt;
                    b.angSpirale += dt * 5.5f;
                    static float acc = 0; acc += dt;
                    if (acc > 0.06f / std::min(1.5f, s)) { acc = 0; for (int k = 0; k < 2; k++) { float a = b.angSpirale + k * TAU / 2; nemici.push_back({ b.p, { std::cos(a) * 330, std::sin(a) * 330 }, 3, 9 }); } }
                }
                if (b.tAtt <= 0 && b.aperto > 0.8f) {
                    b.att = (b.att + 1) % 3;
                    if (b.att == 0) b.tSpirale = 2.0f;
                    else if (b.att == 1) { b.raggio = 1; b.tRaggio = 1.1f; b.raggioX = px; }
                    else for (int k = 0; k < 22; k++) { float a = k * TAU / 22; nemici.push_back({ b.p, { std::cos(a) * VelProiettile() * 0.7f, std::sin(a) * VelProiettile() * 0.7f }, 2, 14 }); }
                    b.tAtt = 2.6f;
                }
                break;
            }
            case 3: {   // Corazzata Scarabeo: il guscio si apre e scopre il nucleo
                b.p.x = 960 + std::sin(b.t * 0.3f) * 380; b.p.y = 250;
                b.tCiclo += dt * (b.parti[1].hp <= 0 && b.parti[2].hp <= 0 ? 1.6f : 1.0f);
                float fase = std::fmod(b.tCiclo, 8.0f);
                float aperto = fase > 4.5f && fase < 7.6f ? 1.0f : 0.0f;
                b.aperto += (aperto - b.aperto) * std::min(1.0f, dt * 4);
                b.parti[0].vulnerabile = b.aperto > 0.7f;
                if (b.tAtt <= 0) {
                    b.att++;
                    for (int i = 1; i <= 2; i++) {
                        Parte& q = b.parti[i];
                        if (q.hp <= 0 || (b.att % 2) != i - 1) continue;
                        Vector2 t = Vector2Add(b.p, q.off);
                        SparaNemico({ t.x, t.y + 50 }, VelProiettile() * 0.85f, 2, 15);
                        SparaNemico({ t.x, t.y + 50 }, VelProiettile() * 0.85f, 2, 15, 0.25f);
                        SparaNemico({ t.x, t.y + 50 }, VelProiettile() * 0.85f, 2, 15, -0.25f);
                    }
                    if (b.aperto > 0.7f) Ventaglio(bocca, 11, TAU / 4, 1.6f, VelProiettile() * 0.9f, 0, 9);
                    b.tAtt = 0.9f;
                    Suona("sc_metallo", 0.5f, 0.5f);
                }
                break;
            }
            default: {   // Imperatrice: ali che spargono polvere, poi furia
                b.p.x = 960 + std::sin(b.t * 0.55f) * 420; b.p.y = 290 + std::sin(b.t * 1.1f) * 50;
                bool ali = b.parti[1].hp > 0 || b.parti[2].hp > 0;
                b.parti[0].vulnerabile = !ali;
                b.tEvoca -= dt;
                if (b.tEvoca <= 0) { EvocaInsetti(1, ali ? 3 : 4); b.tEvoca = ali ? 7 : 5; }
                if (!ali) {
                    b.angSpirale += dt * 3.2f;
                    static float acc = 0; acc += dt;
                    if (acc > 0.11f) { acc = 0; for (int k = 0; k < 3; k++) { float a = b.angSpirale + k * TAU / 3; nemici.push_back({ b.p, { std::cos(a) * 300, std::sin(a) * 300 }, 3, 9 }); } }
                }
                if (b.tAtt <= 0) {
                    b.att++;
                    if (ali) {
                        for (int i = 1; i <= 2; i++) {
                            if (b.parti[i].hp <= 0 || (b.att % 2) != i - 1) continue;
                            Vector2 t = Vector2Add(b.p, b.parti[i].off);
                            Ventaglio(t, 6, std::atan2(Y_CACCIA - t.y, px - t.x), 0.9f, VelProiettile() * 0.85f, 3, 10);
                        }
                        b.tAtt = 1.1f;
                    } else {
                        for (int k = -2; k <= 2; k++) SparaNemico(bocca, VelProiettile() * 1.15f, 1, 11, k * 0.1f);
                        b.tAtt = 1.4f;
                    }
                }
                break;
            }
        }
    }
    void EvocaInsetti(int specie, int quanti) {
        int vivi = Vivi();
        if (vivi > 14) return;
        for (int i = 0; i < quanti; i++) {
            Insetto a;
            a.specie = specie; a.var = (Settore() + i) % 2; a.vita = 1; a.riga = 4; a.col = caso.I(0, 9);
            a.aura = caso.P(0.2f);
            a.p = { boss.p.x + (i - 1) * 80.0f, boss.p.y + 60 };
            a.vivo = true;
            ins.push_back(a);
            LanciaPicchiata((int)ins.size() - 1);
            ins.back().s = -i * 60.0f;
        }
    }
    void ColpisciBoss(Parte& q, Vector2 dove, float danno) {
        if (!q.vulnerabile) { part.Esplosione(dove, Hex(0xa0c0ff), 5, 200, 3, 0.2f); Suona("sc_metallo", 0.3f, 1.4f); return; }
        q.hp -= danno; q.lampo = 0.08f;
        regista.Bersaglio();
        part.Esplosione(dove, Hex(0xffd23c), 6, 260, 4, 0.3f);
        Suona("sc_colpito", 0.4f, Vis(0.9f, 1.1f));
        if (q.hp <= 0) {
            Esplosione(Vector2Add(boss.p, q.off), Hex(0xff8a3a), 2.5f);
            Suona("sc_boom", 0.7f, 1.3f);
            Punti(1000, Vector2Add(boss.p, q.off), Hex(0xffd23c));
            tScosse = 0.4f;
        }
        if (boss.Hp() <= 0 && boss.tMorte <= 0) {
            boss.tMorte = 2.2f;
            Suona("sc_boom", 1);
            Punti(IntDi(BossJ(boss.indice), "punti", 10000 + boss.tipo * 5000), { boss.p.x, boss.p.y - 80 }, Hex(0xffe14a));
            nemici.clear();
            for (auto& a : ins) if (a.vivo) { a.vivo = false; Esplosione(a.p, ColSp(a.specie, a.var), 1); Punti(PuntiSp(a.specie, false), a.p, WHITE); }
        }
    }
    // Il punto della parte del boss colpito da p (o null).
    Parte* PartePresa(Vector2 p, float raggio) {
        if (!boss.vivo || boss.tMorte > 0 || boss.ingresso > 0) return nullptr;
        for (size_t i = boss.parti.size(); i-- > 0;) {
            Parte& q = boss.parti[i];
            if (q.hp <= 0) continue;
            if (Vector2Distance(p, Vector2Add(boss.p, q.off)) < q.r + raggio) return &q;
        }
        return nullptr;
    }

    // ----------------------- ciclo -----------------------
    void Aggiorna(float dt, const Pad& pad) override {
        tempo += dt; tLivello += dt;
        part.Aggiorna(dt);
        for (auto& t : testi) { t.vita -= dt; t.p.y -= 50 * dt; }
        testi.erase(std::remove_if(testi.begin(), testi.end(), [](const Testo& t) { return t.vita <= 0; }), testi.end());
        for (auto& a : anelli) a.t += dt;
        anelli.erase(std::remove_if(anelli.begin(), anelli.end(), [](const Anello& a) { return a.t >= a.durata; }), anelli.end());
        tScritta -= dt; tScosse -= dt; tMissile -= dt;
        if (finito) return;

        // --- caccia ---
        if (tMorto > 0) {
            tMorto -= dt;
            if (tMorto <= 0) {
                if (vite <= 0) { finito = true; ScriviRecord(); scritta = "FINE PARTITA"; sotto = "Punteggio " + std::to_string(punteggio); colScritta = Hex(0xff6a8a); tScritta = 99; return; }
                tInv = 2.2f; px = 960;
            }
        } else {
            float vmax = NumDi(Gen(), "velocitaCaccia", 820);
            float vx = pad.x * vmax;
            if (pad.clic && pad.mouse.y > 0) vx = std::max(-vmax, std::min(vmax, (pad.mouse.x - px) * 8));
            px = std::max(80.0f, std::min(1840.0f - (doppio ? 34 : 0), px + vx * dt));
            tRicarica -= dt; tInv -= dt;
            if (pad.fuoco) Spara();
            regista.Osserva(dt, px / 1920, 0.5f, pad.fuoco);
            if (tArma > 0) { tArma -= dt; if (tArma <= 0) arma = 0; }
        }

        AggiornaInsetti(dt);
        AggiornaBoss(dt);

        // --- colpi del giocatore ---
        for (auto& c : colpi) {
            c.t += dt;
            if (c.missile) {
                // il missile cerca il bersaglio piu' vicino
                Vector2 bers = { c.p.x, -100 };
                float best = 1e9f;
                for (auto& a : ins) if (a.vivo && a.p.y > 0 && a.p.y < c.p.y) { float d = Vector2Distance(a.p, c.p); if (d < best) { best = d; bers = a.p; } }
                if (boss.vivo && boss.ingresso <= 0) for (auto& q : boss.parti) if (q.hp > 0 && q.vulnerabile) { Vector2 bp = Vector2Add(boss.p, q.off); float d = Vector2Distance(bp, c.p); if (d < best) { best = d; bers = bp; } }
                float vy = -900;
                c.vx += (bers.x - c.p.x) * dt * 9; c.vx *= std::pow(0.2f, dt);
                c.p.x += c.vx * dt; c.p.y += vy * dt;
                if (Vis() < 0.6f) { Particella q; q.p = { c.p.x, c.p.y + 14 }; q.v = { Vis(-30, 30), Vis(60, 140) }; q.vita = q.durata = 0.35f; q.r = 4; q.c = Hex(0xffb04a); part.Aggiungi(q); }
            } else c.p.y -= (c.laser ? 2100 : 1700) * dt;
        }
        for (auto& c : colpi) {
            bool via = c.p.y < 60;
            for (auto& a : ins) {
                if (via || !a.vivo || a.p.y < 0) continue;
                if (std::fabs(a.p.x - c.p.x) < 34 && std::fabs(a.p.y - c.p.y) < 34) {
                    ColpisciInsetto(a, c.missile ? 2 : 1);
                    if (!c.laser) via = true;
                }
            }
            if (!via) if (Parte* q = PartePresa(c.p, 6)) { ColpisciBoss(*q, c.p, c.missile ? 2.0f : c.laser ? 1.5f : 1.0f); via = true; }
            if (!via && boss.vivo && boss.tipo == 2 && boss.ingresso <= 0) {   // le sfere che girano intorno all'Occhio fermano i colpi
                for (int k = 0; k < 6 && !via; k++) {
                    float a = boss.t * 0.9f + k * TAU / 6;
                    Vector2 o = { boss.p.x + std::cos(a) * 240, boss.p.y + std::sin(a) * 170 };
                    if (Vector2Distance(o, c.p) < 30) { via = true; part.Esplosione(c.p, Hex(0xff9ae0), 5, 200, 3, 0.2f); }
                }
            }
            if (via) c.p.y = -9999;
        }
        colpi.erase(std::remove_if(colpi.begin(), colpi.end(), [](const Colpo& c) { return c.p.y < 0 || c.t > 3; }), colpi.end());

        // --- proiettili nemici ---
        for (auto& n : nemici) {
            n.p.x += n.v.x * dt; n.p.y += n.v.y * dt;
            if (tMorto <= 0 && tInv <= 0) {
                bool tocca = (std::fabs(n.p.x - px) < 22 + n.r && std::fabs(n.p.y - Y_CACCIA) < 30 + n.r) ||
                             (doppio && std::fabs(n.p.x - px - 34) < 22 + n.r && std::fabs(n.p.y - Y_CACCIA) < 30 + n.r);
                if (tocca) { n.p.y = 9999; Colpito(); }
            }
        }
        nemici.erase(std::remove_if(nemici.begin(), nemici.end(), [](const Proiettile& n) { return n.p.y > 1120 || n.p.y < -60 || n.p.x < -60 || n.p.x > 1980; }), nemici.end());

        // --- capsule ---
        for (auto& k : capsule) {
            k.p.y += 200 * dt;
            if (tMorto <= 0 && std::fabs(k.p.x - px) < 80 && std::fabs(k.p.y - Y_CACCIA) < 60) {
                k.p.y = 9999;
                Suona("sc_capsula");
                const char* nomi[4] = { "LASER", "MISSILI", "SCUDO", "DOPPIO CACCIA" };
                testi.push_back({ { px, Y_CACCIA - 120 }, nomi[k.tipo], 1.4f, WHITE });
                if (k.tipo == 0) { arma = 1; tArma = NumDi(cfg["armi"], "laser", 15); }
                else if (k.tipo == 1) { arma = 2; tArma = NumDi(cfg["armi"], "missili", 15); }
                else if (k.tipo == 2) scudo = true;
                else { doppio = true; px = std::min(px, 1806.0f); }
            }
        }
        capsule.erase(std::remove_if(capsule.begin(), capsule.end(), [](const Capsula& k) { return k.p.y > 1100; }), capsule.end());

        // --- fine del livello ---
        bool pulito = Vivi() == 0 && gruppiEntrati >= (int)gruppi.size() && (!LivelloBoss() || !boss.vivo);
        if (pulito && tFine < 0) {
            tFine = 3.0f;
            regista.TappaFinita(tLivello, LivelloBoss() ? 70.0f : 50.0f);
            float sparati = regista.colpiSparati - colpiLivello, presi = regista.colpiAndatiASegno - aSegnoLivello;
            int prec = sparati > 0 ? (int)std::round(100 * std::min(1.0f, presi / sparati)) : 100;
            int bonus = 2000 + livello * 200 + prec * 30;
            Punti(bonus, { 960, 560 }, Hex(0xffe14a));
            scritta = LivelloBoss() ? "BOSS SCONFITTO!" : "LIVELLO SUPERATO!"; colScritta = Hex(0xffe14a);
            sotto = "Precisione " + std::to_string(prec) + "%   -   Bonus " + std::to_string(bonus) + "   -   Stemma conquistato";
            tScritta = 3.0f;
            Suona("sc_stemma");
        }
        if (tFine > 0) {
            tFine -= dt;
            if (tFine <= 0) {
                if (livello >= NLiv()) { finito = vinto = true; ScriviRecord(); scritta = TestoDi(Gen(), "fine", "LO SCIAME E' SCONFITTO!"); sotto = "Hai superato tutti i " + std::to_string(NLiv()) + " livelli"; colScritta = Hex(0x8affd0); tScritta = 99; Suona("sc_vita"); return; }
                livello++;
                IniziaLivello();
            }
        }
        if (punteggio > record) record = punteggio;
    }

    Pad Automatico() override {
        Pad p;
        float bers = px, best = 1e9f;
        for (auto& a : ins) {
            if (!a.vivo || a.p.y < 0) continue;
            float tempoVolo = (Y_CACCIA - a.p.y) / 1700;
            float x = a.p.x + (a.stato == 3 ? std::cos(a.ang) * a.vel * tempoVolo : 0);
            float d = std::fabs(x - px) * (a.stato == 3 ? 0.6f : 1.0f) - a.p.y * 0.2f;
            if (d < best) { best = d; bers = x; }
        }
        if (boss.vivo && boss.ingresso <= 0 && Vivi() < 4)
            for (auto& q : boss.parti) if (q.hp > 0 && q.vulnerabile) { bers = boss.p.x + q.off.x; break; }
        for (auto& k : capsule) if (k.p.y > 650) bers = k.p.x;
        // schiva proiettili e insetti in picchiata vicini
        float pericolo = 0;
        for (auto& n : nemici) if (n.p.y > 700 && n.p.y < Y_CACCIA + 20 && std::fabs(n.p.x + n.v.x * 0.25f - px) < 70) pericolo += n.p.x < px ? 1 : -1;
        for (auto& a : ins) if (a.vivo && a.stato == 3 && a.p.y > 720 && std::fabs(a.p.x - px) < 110) pericolo += a.p.x < px ? 1 : -1;
        if (boss.raggio >= 1 && std::fabs(boss.raggioX - px) < 140) pericolo += boss.raggioX < px ? 1 : -1;
        if (pericolo != 0) bers = px + (pericolo > 0 ? 260 : -260);
        float d = bers - px;
        p.x = std::fabs(d) < 10 ? 0 : std::max(-1.0f, std::min(1.0f, d / 60));
        p.fuoco = std::fabs(d) < 50 || boss.vivo;
        return p;
    }

    // ======================= DISEGNO =======================
    void DisegnaCaccia(float x, float y, float alfa) {
        // razzi dai colori cangianti
        for (int s = -1; s <= 1; s += 2) {
            float fx = x + s * 16 * 0.88f;
            float h = 58 + 14 * std::sin(tempo * 37 + s);
            for (int k = 0; k < 5; k++) {
                float t = k / 5.0f;
                Color c = ColorFromHSV(std::fmod(tempo * 240 + k * 50 + (s > 0 ? 90 : 0), 360.0f), 0.75f, 1);
                DrawTriangle({ fx - 9 * (1 - t), y + 26 + h * t * 0.4f }, { fx, y + 26 + h * (1 - t * 0.15f) }, { fx + 9 * (1 - t), y + 26 + h * t * 0.4f }, ConAlfa(c, alfa * (0.55f - t * 0.08f)));
            }
            DrawTriangle({ fx - 4, y + 26 }, { fx, y + 26 + h * 0.5f }, { fx + 4, y + 26 }, ConAlfa(WHITE, alfa * 0.9f));
            Bagliore(fx, y + 40, 46, ConAlfa(ColorFromHSV(std::fmod(tempo * 240, 360.0f), 0.6f, 1), 0.45f * alfa));
        }
        Bagliore(x, y, 110, ConAlfa(Hex(0x78b4ff), 0.22f * alfa));
        if (!DisegnaGrafica(cfg["generale"], { x, y + 4 }, 120, "fermo", tempo, false, ConAlfa(WHITE, alfa))) Sprite(cacciaTex, x, y + 4, 0.55f, 0, ConAlfa(WHITE, alfa));
    }

    void DisegnaBoss(float ox, float oy) {
        Boss& b = boss;
        if (!b.vivo) return;
        Vector2 p = { b.p.x + ox, b.p.y + oy };
        bool lampo = false;
        for (auto& q : b.parti) if (q.lampo > 0) lampo = true;
        Color tinta = lampo ? Color{ 255, 220, 220, 255 } : WHITE;
        if (b.tMorte > 0) tinta = ConAlfa(Color{ 255, (unsigned char)(160 + 90 * std::sin(tempo * 40)), 160, 255 }, 1);
        // grafica propria scelta nello Studio
        if (DisegnaGrafica(BossJ(b.indice), p, 360, lampo ? "colpito|fermo" : "attacco|fermo", tempo, false, tinta)) return;
        switch (b.tipo) {
            case 0: {
                Bagliore(p.x, p.y, 380, ConAlfa(Hex(0xff3c78), 0.25f));
                float batti = 0.75f + 0.25f * std::sin(tempo * 24);
                Rectangle src = { 0, 0, (float)alaRegina.width, (float)alaRegina.height };
                DrawTexturePro(alaRegina, src, { p.x + 30, p.y - 40, alaRegina.width * 0.9f, alaRegina.height * 0.9f * batti }, { 18, 153 * batti }, 0, ConAlfa(tinta, 0.95f));
                src.width = -src.width;
                DrawTexturePro(alaRegina, src, { p.x - 30, p.y - 40, alaRegina.width * 0.9f, alaRegina.height * 0.9f * batti }, { alaRegina.width * 0.9f - 18, 153 * batti }, 0, ConAlfa(tinta, 0.95f));
                if (b.carica == 1) Bagliore(p.x, p.y + 180, 120, ConAlfa(Hex(0xffffff), 0.5f + 0.5f * std::sin(tempo * 40)));
                Sprite(bossTex[0], p.x, p.y + 10, 0.9f, 0, tinta);
                break;
            }
            case 1: {
                Bagliore(p.x, p.y, 380, ConAlfa(Hex(0x50c8ff), 0.2f));
                for (int i = 1; i <= 3; i++) {
                    Parte& q = b.parti[i];
                    Vector2 t = { p.x + q.off.x, p.y + q.off.y };
                    Vector2 base = { p.x + (i - 2) * 140.0f, p.y + 60 };
                    for (int k = 0; k <= 8; k++) {   // collo a segmenti
                        float f = k / 8.0f;
                        Vector2 m = { base.x + (t.x - base.x) * f + std::sin(f * 3.14f) * 30 * (i - 2), base.y + (t.y - base.y) * f - std::sin(f * 3.14f) * 40 };
                        DrawCircleV(m, 22 - f * 6, q.hp > 0 ? Hex(0x5a6480) : Hex(0x2a2a3a));
                        DrawCircleV({ m.x - 4, m.y - 5 }, 9 - f * 3, q.hp > 0 ? Hex(0xa8b4d0) : Hex(0x3a3a4a));
                    }
                    if (q.hp > 0) { Bagliore(t.x, t.y, 90, ConAlfa(Hex(0xff6a3a), 0.3f)); Sprite(testaIdra, t.x, t.y, 0.75f, std::sin(tempo * 1.5f + i) * 8, q.lampo > 0 ? Color{ 255, 200, 200, 255 } : WHITE); }
                    else Sprite(testaIdra, t.x, t.y + 30, 0.6f, 40, Color{ 60, 60, 70, 200 });
                }
                Sprite(bossTex[1], p.x, p.y, 0.85f, 0, tinta);
                if (b.parti[0].vulnerabile) Bagliore(p.x, p.y + 10, 140 + 20 * std::sin(tempo * 8), ConAlfa(Hex(0x6af0ff), 0.5f));
                break;
            }
            case 2: {
                for (int k = 0; k < 6; k++) {
                    float a = b.t * 0.9f + k * TAU / 6;
                    Vector2 o = { p.x + std::cos(a) * 240, p.y + std::sin(a) * 170 };
                    Bagliore(o.x, o.y, 60, ConAlfa(Hex(0xff6ad8), 0.5f));
                    Sprite(sferaTex, o.x, o.y, 0.8f);
                }
                Sprite(bossTex[2], p.x, p.y, 0.9f, 0, tinta);
                Vector2 dir = Vector2Normalize({ px - p.x, Y_CACCIA - p.y });
                Sprite(irideTex, p.x + dir.x * 40, p.y + dir.y * 30, 0.95f, 0, tinta);
                // palpebra di cristallo
                float chiuso = 1 - b.aperto;
                if (chiuso > 0.02f) {
                    DrawEllipse((int)p.x, (int)p.y, 140, 104 * chiuso, ConAlfa(Hex(0x3a1a6a), chiuso));
                    DrawEllipseLines((int)p.x, (int)p.y, 140, 104 * chiuso, ConAlfa(Hex(0xc0a0ff), chiuso));
                    Bagliore(p.x, p.y, 200, ConAlfa(Hex(0x8a6aff), 0.3f * chiuso));
                }
                if (b.raggio == 1) {
                    float a = 0.3f + 0.3f * std::sin(tempo * 30);
                    DrawRectangle((int)b.raggioX - 3, (int)p.y, 6, 1080, ConAlfa(Hex(0xff5ad8), a));
                } else if (b.raggio == 2) {
                    DrawRectangleGradientH((int)b.raggioX - 60, (int)p.y, 60, 1080, ConAlfa(Hex(0xff3ab0), 0), ConAlfa(Hex(0xff6ad8), 0.8f));
                    DrawRectangleGradientH((int)b.raggioX, (int)p.y, 60, 1080, ConAlfa(Hex(0xff6ad8), 0.8f), ConAlfa(Hex(0xff3ab0), 0));
                    DrawRectangle((int)b.raggioX - 12, (int)p.y, 24, 1080, ConAlfa(WHITE, 0.9f));
                }
                break;
            }
            case 3: {
                Bagliore(p.x, p.y, 380, ConAlfa(Hex(0x8aff6a), 0.18f));
                Sprite(bossTex[3], p.x, p.y + 10, 0.9f, 0, tinta);
                if (b.parti[0].vulnerabile) Bagliore(p.x, p.y + 20, 150, ConAlfa(Hex(0xc0ff6a), 0.4f + 0.2f * std::sin(tempo * 10)));
                float sp = b.aperto * 150;
                Rectangle src = { 0, 0, (float)gusciTex.width, (float)gusciTex.height };
                DrawTexturePro(gusciTex, src, { p.x - sp, p.y + 20, gusciTex.width * 0.9f, gusciTex.height * 0.9f }, { 290 * 0.9f, 190 * 0.9f }, 0, tinta);
                src.width = -src.width;
                DrawTexturePro(gusciTex, src, { p.x + sp, p.y + 20, gusciTex.width * 0.9f, gusciTex.height * 0.9f }, { 10 * 0.9f, 190 * 0.9f }, 0, tinta);
                for (int i = 1; i <= 2; i++) {
                    Parte& q = b.parti[i];
                    Vector2 t = { p.x + q.off.x, p.y + q.off.y };
                    float rot = std::atan2(Y_CACCIA - t.y, px - t.x) * RAD2DEG - 90;
                    if (q.hp > 0) Sprite(cannoneTex, t.x, t.y, 0.9f, rot, q.lampo > 0 ? Color{ 255, 200, 200, 255 } : WHITE);
                    else { Sprite(cannoneTex, t.x, t.y, 0.8f, 160, Color{ 50, 50, 50, 220 }); if (Vis() < 0.3f) { Particella f; f.p = t; f.v = { Vis(-20, 20), Vis(-120, -60) }; f.vita = f.durata = 0.8f; f.r = 10; f.c = Color{ 80, 80, 80, 160 }; f.additiva = false; part.Aggiungi(f); } }
                }
                break;
            }
            default: {
                Bagliore(p.x, p.y, 420, ConAlfa(Hex(0xffb050), 0.22f));
                float batti = std::sin(tempo * 5) * 10;
                for (int i = 1; i <= 2; i++) {
                    Parte& q = b.parti[i];
                    bool sx = i == 1;
                    Rectangle src = { 0, 0, (float)alaImp.width * (sx ? 1 : -1), (float)alaImp.height };
                    Color c = q.hp > 0 ? (q.lampo > 0 ? Color{ 255, 210, 210, 255 } : WHITE) : Color{ 90, 60, 80, 120 };
                    float sc = q.hp > 0 ? 0.95f : 0.7f;
                    DrawTexturePro(alaImp, src, { p.x + (sx ? -20.0f : 20.0f), p.y - 10, alaImp.width * sc, alaImp.height * sc },
                                   { sx ? 420 * sc : 20 * sc, 200 * sc }, sx ? -batti : batti, c);
                }
                Sprite(bossTex[4], p.x, p.y + 20, 0.85f, 0, tinta);
                if (b.parti[0].vulnerabile) Bagliore(p.x, p.y, 160, ConAlfa(Hex(0xff6ad8), 0.35f + 0.15f * std::sin(tempo * 9)));
                break;
            }
        }
    }

    void Disegna() override {
        float ox = tScosse > 0 ? Vis(-9, 9) * tScosse : 0, oy = tScosse > 0 ? Vis(-9, 9) * tScosse : 0;
        int sett = Settore();
        if (!DisegnaSfondo(Elem(cfg["settori"], sett), tempo * 30))
            DrawTexturePro(sfondi[sett], { 0, 0, 1280, 720 }, { ox - 20, oy - 20, 1960, 1120 }, { 0, 0 }, 0, WHITE);
        // scie di stelle in velocita' (tre piani)
        for (int i = 0; i < 160; i++) {
            unsigned h = (unsigned)(i + 1) * 2654435761u;
            int piano = i % 3;
            float v = 60.0f + piano * 160;
            float x = (float)(h % 1920), y = std::fmod((float)((h >> 9) % 1080) + tempo * v, 1100.0f) - 10;
            float l = 4.0f + piano * 12;
            DrawLineEx({ x, y }, { x, y + l }, 1.0f + piano * 0.6f, ConAlfa(Hex(0xc8e6ff), 0.25f + piano * 0.2f));
        }

        DisegnaBoss(ox, oy);

        // insetti
        for (auto& a : ins) {
            if (!a.vivo || a.p.x < -200) continue;
            int f = (int)(tempo * (a.stato == 2 ? 3 : 8) + a.col) % 2;
            float rot = a.stato == 2 ? 0 : (a.ang + TAU / 4) * RAD2DEG;
            float x = a.p.x + ox, y = a.p.y + oy;
            if (a.aura) {
                float pul = 0.5f + 0.5f * std::sin(tempo * 6 + a.col);
                Bagliore(x, y, 70, ConAlfa(Hex(0xffdc50), 0.25f + 0.2f * pul));
                DrawRing({ x, y }, 38, 41, tempo * 120, tempo * 120 + 300, 24, ConAlfa(Hex(0xffe678), 0.7f));
            } else Bagliore(x, y, 46, ConAlfa(ColSp(a.specie, a.var), 0.18f));
            if (!DisegnaGrafica(SpecieJ(a.specie), { x, y }, 66, a.stato == 2 ? "fermo" : "corsa|fermo", tempo + a.col * 0.13f, false, WHITE, rot))
                Sprite(insTex[a.specie][a.var][f], x, y, 0.5f, rot, WHITE);
            if (a.lampo > 0) Bagliore(x, y, 40, ConAlfa(WHITE, 0.7f));
        }

        // capsule
        for (auto& k : capsule) {
            const char* let[4] = { "L", "M", "S", "D" };
            const unsigned col[4] = { 0xff5a5a, 0x5ac8ff, 0x7aff7a, 0xffd23c };
            Bagliore(k.p.x, k.p.y, 70, ConAlfa(Hex(col[k.tipo]), 0.4f));
            Sprite(capsTex[k.tipo], k.p.x, k.p.y, 0.75f + 0.05f * std::sin(tempo * 10));
            Scritta(let[k.tipo], k.p.x, k.p.y + 1, 26, Hex(0x101830), 0, false);
        }

        // colpi del giocatore
        for (auto& c : colpi) {
            if (c.missile) {
                float a = std::atan2(-900.0f, c.vx) * RAD2DEG + 90;
                DrawRectanglePro({ c.p.x, c.p.y, 8, 24 }, { 4, 12 }, a, Hex(0xe8eef8));
                DrawCircleV({ c.p.x, c.p.y - 10 }, 4, Hex(0xff5a3a));
                Bagliore(c.p.x, c.p.y + 14, 22, ConAlfa(Hex(0xffb04a), 0.8f));
            } else if (c.laser) {
                DrawRectangleGradientV((int)c.p.x - 5, (int)c.p.y - 30, 10, 70, ConAlfa(Hex(0xffffff), 1), ConAlfa(Hex(0xff5a5a), 0));
                Bagliore(c.p.x, c.p.y, 40, ConAlfa(Hex(0xff5a5a), 0.8f));
            } else {
                DrawRectangleGradientV((int)c.p.x - 4, (int)c.p.y - 26, 8, 34, Hex(0xfff6c0), ConAlfa(Hex(0xa8e4ff), 0.2f));
                Bagliore(c.p.x, c.p.y - 10, 26, ConAlfa(Hex(0xffdc64), 0.7f));
            }
        }

        // caccia
        if (tMorto <= 0) {
            bool vis = tInv <= 0 || std::fmod(tempo, 0.2f) < 0.13f;
            if (vis) {
                DisegnaCaccia(px + ox, Y_CACCIA + oy, 1);
                if (doppio) DisegnaCaccia(px + 34 * 2 + ox - 34, Y_CACCIA + oy, 1);
                if (scudo) {
                    float cx = px + (doppio ? 34 : 0);
                    DrawRing({ cx, Y_CACCIA }, 78, 82 + 3 * std::sin(tempo * 9), 0, 360, 48, ConAlfa(Hex(0x7aff7a), 0.7f));
                    Bagliore(cx, Y_CACCIA, 110, ConAlfa(Hex(0x7aff7a), 0.15f));
                }
            }
        }

        // proiettili nemici
        for (auto& n : nemici) {
            Color c = n.tipo == 0 ? Hex(0xff4a8a) : n.tipo == 1 ? Hex(0x9af6ff) : n.tipo == 2 ? Hex(0xff9a3a) : Hex(0xffd0ff);
            Bagliore(n.p.x, n.p.y, n.r * 3, ConAlfa(c, 0.7f));
            if (n.tipo == 1) {
                float a = std::atan2(n.v.y, n.v.x) * RAD2DEG;
                DrawPoly(n.p, 4, n.r, a, c);
                DrawPoly(n.p, 4, n.r * 0.5f, a, WHITE);
            } else {
                DrawCircleV(n.p, n.r, c);
                DrawCircleV({ n.p.x - n.r * 0.25f, n.p.y - n.r * 0.25f }, n.r * 0.45f, ConAlfa(WHITE, 0.9f));
            }
        }

        part.Disegna();
        for (auto& a : anelli) {
            float k = a.t / a.durata;
            DrawRing(a.p, a.r * k * 0.8f, a.r * k, 0, 360, 36, ConAlfa(a.c, 0.6f * (1 - k)));
        }
        for (auto& t : testi) Scritta(t.t, t.p.x, t.p.y, 30, ConAlfa(t.c, std::min(1.0f, t.vita * 2)));

        // --- HUD ---
        DrawRectangleGradientV(0, 0, 1920, 72, Hex(0x000814, 210), Hex(0x000814, 40));
        Scritta((giocatore == 0 ? "1UP  " : "2UP  ") + std::to_string(punteggio), 40, 36, 34, WHITE, -1);
        Scritta("RECORD  " + std::to_string(std::max(record, punteggio)), 960, 36, 32, Hex(0xffd27a), 0);
        Scritta("LIVELLO " + std::to_string(livello) + " / " + std::to_string(NLiv()), 1880, 36, 34, Hex(0x88ffff), 1);
        if (altroPunteggio >= 0) Scritta((giocatore == 0 ? "2UP " : "1UP ") + std::to_string(altroPunteggio), 1880, 76, 24, Hex(0xb0b8e0), 1);
        for (int i = 0; i < std::min(vite - (tMorto > 0 ? 0 : 1), 8); i++) Sprite(cacciaTex, 60 + i * 64.0f, 1040, 0.22f);
        if (arma) {
            const char* nomi[3] = { "", "ARMA: LASER", "ARMA: MISSILI" };
            Scritta(nomi[arma], 960, 1036, 26, arma == 1 ? Hex(0xff8a8a) : Hex(0x8ad8ff), 0);
            DrawRectangle(860, 1058, (int)(200 * tArma / 15), 6, arma == 1 ? Hex(0xff8a8a) : Hex(0x8ad8ff));
        } else if (doppio) Scritta("DOPPIO CACCIA", 960, 1036, 26, Hex(0xffd23c), 0);
        // stemmi dei livelli superati
        for (int i = 1; i < livello || (tFine > 0 && i == livello); i++) {
            if (i > NLiv()) break;
            float x = 1880 - (std::min(NLiv(), 30) - i) * 30.0f, y = 1036;
            if (x < 1200) continue;
            bool boss = IntDi(Elem(cfg["livelli"], i - 1), "boss", -1) >= 0;
            Color c = boss ? Hex(0xffc83a) : (i % 2 ? Hex(0xe03050) : Hex(0x3060e0));
            Vector2 q[5] = { { x - 11, y - 14 }, { x + 11, y - 14 }, { x + 11, y + 3 }, { x, y + 15 }, { x - 11, y + 3 } };
            DrawTriangle(q[0], q[4], q[1], c); DrawTriangle(q[1], q[4], q[2], c); DrawTriangle(q[4], q[3], q[2], c);
            for (int k = 0; k < 5; k++) DrawLineEx(q[k], q[(k + 1) % 5], 2, WHITE);
            if (boss) DrawPoly({ x, y - 3 }, 5, 6, -90, WHITE);
        }
        // barra del boss
        if (boss.vivo && boss.ingresso < 0.9f && boss.tMorte <= 0) {
            float k = boss.Hp() / boss.HpMax();
            DrawRectangleRounded({ 560, 96, 800, 26 }, 1, 12, ConAlfa(BLACK, 0.6f));
            DrawRectangleRounded({ 564, 100, 792 * k, 18 }, 1, 12, Hex(0xff3a6a));
            DrawRectangleRounded({ 564, 100, 792 * k, 7 }, 1, 12, ConAlfa(Hex(0xffb02a), 0.8f));
            Scritta("BOSS  -  " + NomeBoss(), 960, 142, 24, Hex(0xffd0e8), 0);
        }
        if (tScritta > 0) {
            float a = std::min(1.0f, tScritta * 2);
            Scritta(scritta, 960 + ox, 470, 92, ConAlfa(colScritta, a), 0);
            Scritta(sotto, 960, 560, 38, ConAlfa(Hex(0x9ff6ff), a), 0);
        }
    }

    // ======================= SALVATAGGI =======================
    std::string Stato() const override {
        json j;
        j["seme"] = std::to_string(seme);
        j["casoInizio"] = std::to_string(casoInizio);
        j["livello"] = livello; j["vite"] = vite; j["prossimaVita"] = prossimaVita; j["punteggio"] = punteggio; j["tempo"] = tempo;
        j["arma"] = arma; j["tArma"] = tArma; j["scudo"] = scudo; j["doppio"] = doppio; j["giocatore"] = giocatore;
        j["regista"] = json::parse(regista.Json());
        return j.dump();
    }
    bool Ripristina(const std::string& t, std::string& errore) override {
        json j = json::parse(t, nullptr, false);
        if (!j.is_object() || !j.contains("livello")) { errore = "dati di Sciame di Orione non validi"; return false; }
        uint64_t s, c;
        try { s = std::stoull(j.value("seme", std::string("1"))); c = std::stoull(j.value("casoInizio", std::string("1"))); }
        catch (...) { errore = "stato del caso non valido"; return false; }
        bool vecchioSilenzio = gSilenzio;
        gSilenzio = true;
        Nuova(s, j.value("giocatore", 0));
        gSilenzio = vecchioSilenzio;
        livello = std::max(1, std::min(NLiv(), j.value("livello", 1)));
        vite = std::max(1, j.value("vite", 3));
        prossimaVita = j.value("prossimaVita", 30000);
        punteggio = j.value("punteggio", 0);
        tempo = j.value("tempo", 0.0f);
        arma = j.value("arma", 0); tArma = j.value("tArma", 0.0f); scudo = j.value("scudo", false); doppio = j.value("doppio", false);
        if (j.contains("regista")) regista.DaJson(j["regista"].dump());
        caso.s = c;
        int p = puntiDiControllo;
        IniziaLivello();
        puntiDiControllo = p;
        return true;
    }
};

}  // namespace

std::unique_ptr<GiocoArcade> CreaSciame() { return std::unique_ptr<GiocoArcade>(new Sciame()); }

}  // namespace arcade
