// Gioco C: "Nilo e il Regno Capovolto".
// Il Mago Rovescio ha capovolto il regno: le isole volano, i giocattoli si ribellano, il castello
// sta a testa in giu'. Nilo, con la sua fionda di stelle, e il pinguino Fiocco rimettono le cose
// al loro posto in sei mondi. Pixel art fine, colori allegri.
#include "platform_tre.h"
#include <cmath>

namespace piatt {

using namespace arte;

namespace {

const float TAU_ = 6.28318530718f;
void ell(K& c, float x, float y, float rx, float ry, float rot = 0) { c.beginPath(); c.ellipse(x, y, std::max(0.05f, rx), std::max(0.05f, ry), rot, 0, TAU_); }

Umanoide Nilo() {
    Umanoide u; u.skin = "#f6cfa8"; u.hair = "#3a2a1e"; u.hairStyle = "cap"; u.capc = "#2f5fd8"; u.eye = "#d87a1a"; u.top = "#f2c23a";
    u.bottom = "#2f4fa8"; u.shoes = "#c83a3a"; u.scarf = "#e84a4a"; u.bag = true; u.grin = true; return u;
}
Umanoide Soldatino() {
    Umanoide u; u.skin = "#f6cfa8"; u.hair = "#3a2a1e"; u.hairStyle = "spiky"; u.eye = "#2a2a3a"; u.top = "#d83a3a"; u.bottom = "#2a3a8a";
    u.shoes = "#1a1a1a"; u.hat = "shako"; u.hatc = "#2a3a8a"; u.belt = "#ffffff"; u.emblem = "#f2c94a"; return u;
}
Umanoide Cavaliere() {
    Umanoide u; u.skin = "#cccccc"; u.helmet = "#6a5a8a"; u.plume = "#ffd04a"; u.top = "#5a4a7a"; u.bottom = "#3a2e5a"; u.shoes = "#2a2040";
    u.belt = "#ffd04a"; u.item = "sword"; u.glow = "#ffd04a"; return u;
}
Umanoide Mago() {
    Umanoide u; u.skin = "#f2c8a8"; u.hair = "#e8e8f0"; u.eye = "#8a3ad8"; u.brows = true; u.top = "#4a2a8a"; u.robe = true; u.trim = "#f2c94a";
    u.hat = "wizard"; u.hatc = "#4a2a8a"; u.beard = true; u.item = "staff"; u.orb = "#ff5ad8"; return u;
}
DisegnaPersonaggio Uman(Umanoide u, float s = 1) { return [u, s](K& c, float x, float y, float k, const Posa& p) { DisegnaUmanoide(c, x, y, k * s, u, p); }; }

void Cuore(K& c, float x, float y, float s, const std::string& col) {
    c.save(); c.translate(x, y); c.scale(s, s);
    c.beginPath(); c.moveTo(0, 3); c.bezierCurveTo(-1, 2, -6, -1, -6, -4); c.bezierCurveTo(-6, -7, -2, -8, 0, -5);
    c.bezierCurveTo(2, -8, 6, -7, 6, -4); c.bezierCurveTo(6, -1, 1, 2, 0, 3); c.closePath();
    c.fillStyle(rg(-2, -5, 0, 8, { sh(col, 0.5f), col, sh(col, -0.4f) })); c.fill();
    c.strokeStyle(V(sh(col, -0.6f))); c.lineWidth(0.6f); c.stroke();
    ell(c, -3, -5, 1.4f, 0.9f, -0.5f); c.fillStyle(V("rgba(255,255,255,0.85)")); c.fill();
    c.restore();
}

void Stella(K& c, float r, float r2, const std::string& a, const std::string& b, const std::string& bordo) {
    c.beginPath();
    for (int k = 0; k < 10; k++) { float rr = k % 2 ? r2 : r, an = -TAU_ / 4 + k * TAU_ / 10; if (k == 0) c.moveTo(std::cos(an) * rr, std::sin(an) * rr); else c.lineTo(std::cos(an) * rr, std::sin(an) * rr); }
    c.closePath(); c.fillStyle(rg(-r * 0.3f, -r * 0.3f, 0, r, { "#ffffff", a, b })); c.fill();
    c.strokeStyle(V(bordo)); c.lineWidth(0.6f); c.stroke();
}

}  // namespace

ProgettoGioco CreaNilo() {
    Resa resa; resa.hd = false; resa.r = 0.5f; resa.dens = 2;
    ProgettoGioco p = NuovoProgetto(Genere::Platform, StileGrafico::PixelFine, "Nilo e il Regno Capovolto");
    p.personaggi.clear(); p.oggetti.clear(); p.scenari.clear(); p.caselle.clear(); p.brani.clear(); p.effetti.clear(); p.regole.clear();
    p.variabili.clear(); p.immagini.clear(); p.mondo.mappe.clear();
    p.mondo.titolo = "Nilo e il Regno Capovolto";
    p.mondo.autori = "Mario Petramale - Elvantis";
    p.mondo.giocatoriMax = 2;
    p.mondo.fisica.gravita = 900; p.mondo.fisica.salto = 320; p.mondo.fisica.velocita = 128; p.mondo.fisica.molla = 540;
    p.mondo.fisica.saltiInAria = 1;
    p.variabili["monete"] = 0;

    auto Personaggio = [&](const std::string& nome, Disegno d, TipoComportamento t, float vel, int energia, int danno, const std::string& proiettile = "",
                           float salto = 0, float vista = 140, float ogni = 2, bool gravita = true, int punti = 100) {
        PersonaggioDef pd; pd.nome = nome; pd.autore = "Elvantis"; pd.disegno = std::move(d);
        pd.comportamento.tipo = t; pd.comportamento.velocita = vel; pd.comportamento.energia = energia; pd.comportamento.danno = danno;
        pd.comportamento.proiettile = proiettile; pd.comportamento.salto = salto; pd.comportamento.vista = vista; pd.comportamento.attaccoOgni = ogni;
        pd.comportamento.gravita = gravita; pd.comportamento.punti = punti;
        pd.suoni["colpito"] = "Colpo"; pd.suoni["sconfitto"] = "Sconfitto";
        if (!proiettile.empty()) pd.suoni["attacco"] = "Sparo nemico";
        p.personaggi.push_back(pd);
    };
    auto Nemico = [&](const DisegnaPersonaggio& f, float alt, float largo = 150, float sopra = 0) {
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = false; o.largo = largo; o.sopra = sopra;
        o.animazioni = { Anim::Fermo, Anim::Corsa, Anim::Attacco, Anim::Colpito }; o.fotogrammiCorsa = 6; o.fotogrammiFermo = 3;
        return CreaDisegno(f, o);
    };
    auto Volante = [&](const DisegnaPersonaggio& f, float alt, float largo = 190) {
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = false; o.largo = largo; o.alto = 175;
        o.animazioni = { Anim::Fermo, Anim::Colpito }; o.fotogrammiFermo = 6;
        return CreaDisegno(f, o);
    };
    auto DinoF = [](Dino d, float s = 1) { return [d, s](K& c, float x, float y, float k, const Posa& q) { DisegnaDino(c, x, y, k * s, d, q); }; };
    auto GolemF = [](const std::string& a, const std::string& b, const std::string& cc, const std::string& l, float s = 1) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaGolem(c, x, y, k * s, a, b, cc, l, q); };
    };
    auto OmbraF = [](const std::string& col, const std::string& aura, const std::string& occhi, float s = 1) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaOmbra(c, x, y - 40 * k * s, k * s, col, aura, occhi, q); };
    };
    auto RobotF = [](Robot r) { return [r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }; };
    auto ApeF = [](float s = 1) { return [s](K& c, float x, float y, float k, const Posa& q) { DisegnaApe(c, x, y - 60 * k * s, k * s, q); }; };
    auto MedusaF = [](const std::string& col, float s = 1) { return [col, s](K& c, float x, float y, float k, const Posa& q) { DisegnaMedusa(c, x, y - 50 * k * s, k * s, col, q); }; };
    auto PipF = [](const std::string& ali, const std::string& corpo, const std::string& occhi) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * k, k, ali, corpo, occhi, q); };
    };
    auto DroneF = [](const std::string& col, const std::string& occhio) {
        return [=](K& c, float x, float y, float s, const Posa& q) { DisegnaDrone(c, x, y - 60 * s, s, col, occhio, q); };
    };

    // --- eroi ------------------------------------------------------------------------------------------------
    Personaggio("Nilo", CreaPersonaggio(Uman(Nilo()), 30, resa), TipoComportamento::Giocatore, 128, 4, 0, "Stella lanciata", 320);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    Personaggio("Fiocco", CreaPersonaggio([](K& c, float x, float y, float s, const Posa& q) { DisegnaPinguino(c, x, y, s * 1.2f, "#3ad8a8", q); }, 26, resa),
                TipoComportamento::Giocatore, 122, 5, 0, "Palla di neve", 310);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    p.eroe = "Nilo"; p.eroe2 = "Fiocco";

    // --- nemici dei sei mondi -------------------------------------------------------------------------------
    // 1 Isole del Vento
    { Dino d; d.c = "#5ac0e8"; d.belly = "#f2fbff"; d.stripe = "#2a7ab0";
      Personaggio("Dino delle nuvole", Nemico(DinoF(d, 0.7f), 20, 190), TipoComportamento::Pattuglia, 42, 1, 1); }
    { Dino d; d.c = "#8ad84a"; d.belly = "#fff6c0"; d.stripe = "#4a8a2a";
      Personaggio("Dino saltamontagne", Nemico(DinoF(d, 0.6f), 18, 190), TipoComportamento::Insegue, 62, 1, 1, "", 250, 120); }
    Personaggio("Ape trombettiera", Volante(ApeF(), 20), TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    Personaggio("Golem di nuvola", Nemico(GolemF("#ffffff", "#dfe8f2", "#9ab4d0", "#5ab0ff", 0.75f), 26, 170), TipoComportamento::Spara, 0, 2, 1, "Ghianda", 0, 170, 2.2f);
    Personaggio("Re delle Nuvole", Nemico(GolemF("#ffffff", "#dfe8f2", "#8aa4c8", "#ffd04a"), 58, 170), TipoComportamento::Boss, 50, 10, 1, "Ghianda", 0, 260, 1.7f, true, 2000);
    // 2 Giardino dei Giganti
    { Dino d; d.c = "#c8a43a"; d.belly = "#fff0b0"; d.stripe = "#8a6a1a";
      Personaggio("Lucertola dorata", Nemico(DinoF(d, 0.75f), 20, 190), TipoComportamento::Pattuglia, 48, 1, 1); }
    Personaggio("Spiritello del prato", Nemico(OmbraF("#4ab83a", "#c8ff6a", "#ffffff", 0.8f), 18), TipoComportamento::Insegue, 60, 1, 1, "", 240, 120);
    Personaggio("Ape operaia", Volante(ApeF(0.9f), 18), TipoComportamento::Vola, 60, 1, 1, "", 0, 140, 2, false);
    Personaggio("Fungo sparaspore", Nemico(GolemF("#ffe0d8", "#e8503a", "#8a2a1a", "#ffffff", 0.75f), 26, 170), TipoComportamento::Spara, 0, 2, 1, "Spora", 0, 170, 2);
    Personaggio("Regina delle Api", Nemico(ApeF(2.4f), 52, 200), TipoComportamento::Boss, 60, 12, 1, "Spora", 0, 260, 1.5f, false, 2500);
    // 3 Abissi di Corallo (sott'acqua: si nuota)
    Personaggio("Golem di corallo", Nemico(GolemF("#ffb0a0", "#ff6a8a", "#a82a4a", "#ffe14a", 0.7f), 24, 170), TipoComportamento::Pattuglia, 30, 2, 1);
    Personaggio("Pesce palla", Nemico(OmbraF("#f2c23a", "#ffe8a0", "#2a2a3a", 0.85f), 20), TipoComportamento::Insegue, 55, 1, 1, "", 200, 130);
    Personaggio("Medusa elettrica", Volante(MedusaF("#e88aff"), 22), TipoComportamento::Vola, 40, 1, 1, "", 0, 140, 2, false);
    Personaggio("Medusa sparabolle", Nemico(MedusaF("#5af0ff"), 24, 170), TipoComportamento::Spara, 0, 2, 1, "Bolla elettrica", 0, 170, 2, false);
    Personaggio("Medusa Regina", Nemico(MedusaF("#ff7ad8", 2.2f), 56, 200), TipoComportamento::Boss, 45, 12, 1, "Bolla elettrica", 0, 260, 1.4f, false, 2500);
    // 4 Fabbrica dei Giocattoli
    { Umanoide u = Soldatino(); u.flip = true;
      Personaggio("Soldatino a carica", Nemico(Uman(u), 26), TipoComportamento::Pattuglia, 45, 1, 1); }
    { Robot r; r.metal = "#4ac86a"; r.core = "#ffffff"; r.eye = "#ffffff"; r.ant = "#f2c23a"; r.fist = "#e84a5a";
      Personaggio("Robottino a molla", Nemico(RobotF(r), 22), TipoComportamento::Insegue, 62, 1, 1, "", 260, 130); }
    Personaggio("Elicottero giocattolo", Volante(DroneF("#ff7ab0", "#4a8ae8"), 20), TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    { Umanoide u = Soldatino(); u.top = "#2a8a4a"; u.hatc = "#1a5a2a"; u.item = "pistol";
      Personaggio("Cannoniere di latta", Nemico(Uman(u), 28), TipoComportamento::Spara, 0, 2, 1, "Tappo", 0, 180, 1.9f); }
    Personaggio("Robot Gigante", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaMech(c, x, y, s, "#e84a5a", q); }, 62, 200),
                TipoComportamento::Boss, 55, 14, 1, "Tappo", 0, 280, 1.3f, true, 3000);
    // 5 Cuore del Vulcano
    { Dino d; d.c = "#ff6a2a"; d.belly = "#ffe14a"; d.stripe = "#8a1a0a";
      Personaggio("Salamandra di lava", Nemico(DinoF(d, 0.75f), 20, 190), TipoComportamento::Pattuglia, 50, 2, 1); }
    Personaggio("Lapillo", Nemico(OmbraF("#c83a0a", "#ffb03a", "#fff6a0", 0.75f), 18), TipoComportamento::Insegue, 66, 1, 1, "", 260, 130);
    Personaggio("Pipistrello di fuoco", Volante(PipF("#c8300a", "#5a0e04", "#ffe14a"), 22), TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    Personaggio("Golem di magma", Nemico(GolemF("#ff8a3a", "#8a2a14", "#3a0e08", "#ffe14a", 0.8f), 28, 170), TipoComportamento::Spara, 0, 2, 1, "Palla di fuoco", 0, 190, 1.8f);
    { Dino d; d.c = "#c8200a"; d.belly = "#ffb03a"; d.stripe = "#4a0a04";
      Personaggio("Drago del Vulcano", Nemico(DinoF(d, 1.0f), 60, 200), TipoComportamento::Boss, 60, 15, 1, "Palla di fuoco", 0, 280, 1.3f, true, 3000); }
    // 6 Castello Sottosopra
    { Umanoide u = Cavaliere(); u.flip = true;
      Personaggio("Cavaliere rovescio", Nemico(Uman(u), 30), TipoComportamento::Pattuglia, 42, 2, 1); }
    Personaggio("Folletto rovescio", Nemico(OmbraF("#7a4ad8", "#ffd04a", "#ffffff", 0.8f), 18), TipoComportamento::Insegue, 68, 1, 1, "", 260, 130);
    Personaggio("Pipistrello rovescio", Volante(PipF("#8a5ad8", "#3a2a6a", "#ffd04a"), 22), TipoComportamento::Vola, 58, 1, 1, "", 0, 140, 2, false);
    { Umanoide u = Mago(); u.top = "#2a6a8a"; u.hatc = "#1a4a6a"; u.orb = "#5af0ff"; u.beard = false; u.hair = "#3a2a1e";
      Personaggio("Apprendista mago", Nemico(Uman(u), 28), TipoComportamento::Spara, 0, 2, 1, "Stella rovescia", 0, 200, 1.8f); }
    Personaggio("Mago Rovescio", Nemico(Uman(Mago()), 64), TipoComportamento::Boss, 60, 18, 2, "Stella rovescia", 0, 300, 1.1f, true, 5000);

    // --- oggetti ---------------------------------------------------------------------------------------------
    auto Oggetto = [&](const std::string& nome, FunzioneOggetto f, Disegno d, int valore = 10, const std::string& suono = "") {
        OggettoDef o; o.nome = nome; o.autore = "Elvantis"; o.funzione = f; o.disegno = std::move(d); o.valore = valore; o.suono = suono;
        p.oggetti.push_back(o);
        return &p.oggetti.back();
    };
    Oggetto("Stellina", FunzioneOggetto::Raccogli, CreaOggettoDisegno(14, 16, resa, { { "fermo", 8, 10, true } }, [](K& c, const std::string&, int i, int n) {
        float a = i * TAU_ / n, w = std::cos(a);   // gira su se' stessa
        c.save(); c.translate(0, -8 + std::sin(a) * 1); c.scale(std::max(0.18f, std::fabs(w)), 1);
        Stella(c, 6, 2.8f, w > 0 ? "#ffe14a" : "#ffc83a", "#e89a1a", "#a8601a");
        if (w > 0) { ell(c, -1.6f, -2, 0.8f, 1.2f); c.fillStyle(V("#5a3a1a")); c.fill(); ell(c, 1.6f, -2, 0.8f, 1.2f); c.fill(); }
        c.restore();
    }), 50, "Stellina")->variabile = "monete";
    Oggetto("Cuore", FunzioneOggetto::Cura, CreaOggettoDisegno(14, 14, resa, { { "fermo", 4, 6, true } }, [](K& c, const std::string&, int i, int) {
        Cuore(c, 0, -7, 0.92f + 0.08f * std::sin(i * TAU_ / 4), "#ff4a6a");
    }), 1, "Cuore");
    Oggetto("Chiave a carica", FunzioneOggetto::Chiave, CreaOggettoDisegno(16, 16, resa, { { "fermo", 6, 8, true } }, [](K& c, const std::string&, int i, int n) {
        c.save(); c.translate(0, -8 + std::sin(i * TAU_ / n) * 0.8f);
        c.fillStyle(lg(-6, -6, 6, 6, { "#fff6c0", "#f2c23a", "#a8701a" }));
        c.save(); c.scale(std::max(0.3f, std::fabs(std::cos(i * TAU_ / n))), 1);
        ell(c, -3.5f, -2.5f, 3.2f, 2.4f); c.fill(); ell(c, 3.5f, -2.5f, 3.2f, 2.4f); c.fill();
        ell(c, -3.5f, -2.5f, 1.2f, 0.9f); c.fillStyle(V("#a8701a")); c.fill(); ell(c, 3.5f, -2.5f, 1.2f, 0.9f); c.fill();
        c.restore();
        c.fillStyle(V("#c8902a")); c.fillRect(-1, -1, 2, 7); c.fillRect(-2, 5, 4, 1.4f);
        c.restore();
    }), 100, "Chiave");
    { OggettoDef* o = Oggetto("Portone rovescio", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 1, 1, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int, int) {
        c.fillStyle(V("#5a3a6a")); c.fillRect(-8, -32, 16, 32);
        if (a == "chiusa") {
            c.fillStyle(lg(-7, 0, 7, 0, { "#c8843a", "#8a5226" })); c.fillRect(-7, -31, 14, 31);
            c.fillStyle(V("#6a3a1a")); for (int k = 0; k < 3; k++) c.fillRect(-7 + k * 4.7f, -31, 0.7f, 31);
            c.fillStyle(V("#f2c23a")); c.fillRect(-7, -27, 14, 1.4f); c.fillRect(-7, -5, 14, 1.4f);
            // serratura capovolta
            c.fillStyle(V("#2a1a10")); ell(c, 0, -13, 1.6f, 1.6f); c.fill(); c.beginPath(); c.moveTo(-1.2f, -13); c.lineTo(0, -17.5f); c.lineTo(1.2f, -13); c.fill();
        } else { c.fillStyle(V("#1a0e24")); c.fillRect(-7, -31, 14, 31); }
    }), 0, "Porta"); o->richiede = "Chiave a carica"; o->solido = true; }
    { OggettoDef* o = Oggetto("Sbarre del Mago", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 4, 6, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int i, int) {
        if (a != "chiusa") return;
        c.fillStyle(V("#4a3a72")); c.fillRect(-8, -32, 16, 2.4f); c.fillRect(-8, -2.4f, 16, 2.4f);
        for (int k = 0; k < 4; k++) { c.fillStyle(lg(-6 + k * 4, 0, -4.6f + k * 4, 0, { "#ffe9a0", "#c8902a" })); c.fillRect(-6.4f + k * 4, -30, 1.6f, 28); }
        c.save(); c.lighter(); for (int k = 0; k < 3; k++) { ell(c, -4 + k * 4, -28 + ((i + k * 3) % 8) * 3.4f, 1.1f, 1.1f); c.fillStyle(V("rgba(255,120,240,0.9)")); c.fill(); } c.restore();
    }), 0, "Porta"); o->richiede = "sconfiggere il guardiano"; o->solido = true; }
    Oggetto("Fungo rimbalzino", FunzioneOggetto::Molla, CreaOggettoDisegno(16, 16, resa, { { "fermo", 1, 1, true }, { "scatta", 3, 12, false } }, [](K& c, const std::string& a, int i, int) {
        float sq = a == "scatta" ? (i == 0 ? 0.6f : i == 1 ? 1.25f : 1.05f) : 1;
        c.fillStyle(V("#f2e6c8")); c.fillRect(-2.2f, -6 * sq, 4.4f, 6 * sq);
        c.save(); c.translate(0, -6 * sq); c.scale(1 / std::sqrt(sq), 1);
        c.beginPath(); c.moveTo(-8, 0); c.quadraticCurveTo(-8, -8, 0, -8); c.quadraticCurveTo(8, -8, 8, 0); c.closePath();
        c.fillStyle(lg(0, -8, 0, 0, { "#ff8a7a", "#e8303a", "#a81a2a" })); c.fill();
        c.fillStyle(V("#ffffff")); ell(c, -4, -3.5f, 1.4f, 1.1f); c.fill(); ell(c, 2, -5.5f, 1.6f, 1.2f); c.fill(); ell(c, 5, -2, 1, 0.8f); c.fill();
        c.restore();
    }), 540, "Molla");
    Oggetto("Bandierina", FunzioneOggetto::Controllo, CreaOggettoDisegno(18, 32, resa, { { "fermo", 1, 1, true }, { "accesa", 6, 8, true } }, [](K& c, const std::string& a, int i, int n) {
        c.fillStyle(V("#6a5a4a")); c.fillRect(-4, -1.6f, 8, 1.6f);
        c.fillStyle(V("#e8e8f0")); c.fillRect(-0.7f, -30, 1.4f, 29); ell(c, 0, -30.5f, 1.4f, 1.4f); c.fillStyle(V("#f2c23a")); c.fill();
        bool su = a == "accesa"; float y0 = su ? -29 : -10, t = i * TAU_ / std::max(1, n);
        c.beginPath(); c.moveTo(0.7f, y0);
        for (int k = 0; k <= 6; k++) { float x = 0.7f + k * 1.6f; c.lineTo(x, y0 + (su ? std::sin(t + k * 0.8f) * 0.9f : k * 0.9f)); }
        for (int k = 6; k >= 0; k--) { float x = 0.7f + k * 1.6f; c.lineTo(x, y0 + 7 + (su ? std::sin(t + k * 0.8f) * 0.9f : k * 0.9f)); }
        c.closePath(); c.fillStyle(V(su ? "#e84a4a" : "#8a8a9a")); c.fill();
        if (su) { c.save(); c.translate(5.5f, y0 + 3.5f + std::sin(t + 3) * 0.9f); Stella(c, 2.4f, 1, "#ffe14a", "#e89a1a", "#a8601a"); c.restore(); }
    }), 0, "Bandierina");
    Oggetto("Mongolfiera", FunzioneOggetto::Arrivo, CreaOggettoDisegno(40, 60, resa, { { "fermo", 8, 6, true } }, [](K& c, const std::string&, int i, int n) {
        float t = i * TAU_ / n, dy = std::sin(t) * 1.5f;
        c.save(); c.translate(0, dy);
        // cesta e corde
        c.fillStyle(lg(0, -10, 0, 0, { "#c8843a", "#7a4a1e" })); c.fillRect(-6, -10, 12, 9);
        c.fillStyle(V("#5a3214")); for (int k = 0; k < 3; k++) c.fillRect(-6, -8 + k * 3, 12, 0.6f);
        c.strokeStyle(V("#5a3214")); c.lineWidth(0.6f); c.beginPath(); c.moveTo(-6, -10); c.lineTo(-9, -22); c.moveTo(6, -10); c.lineTo(9, -22); c.stroke();
        // pallone a spicchi
        const char* col[] = { "#e84a5a", "#f2c23a", "#4a8ae8", "#4ac86a", "#b05ae8", "#f2c23a" };
        for (int k = 0; k < 6; k++) {
            float a0 = -1 + k / 3.f, a1 = -1 + (k + 1) / 3.f;
            c.beginPath(); c.moveTo(-8 + (k / 6.f) * 16 * 0.0f + a0 * 8, -22);
            c.quadraticCurveTo(a0 * 22, -40, a0 * 4, -58); c.lineTo(a1 * 4, -58); c.quadraticCurveTo(a1 * 22, -40, a1 * 8, -22); c.closePath();
            c.fillStyle(V(col[k])); c.fill();
        }
        c.fillStyle(rg(-6, -46, 0, 18, { "rgba(255,255,255,0.45)", "rgba(255,255,255,0)" })); ell(c, 0, -40, 17, 18); c.fill();
        c.fillStyle(V("#ffffff")); c.fillRect(-4, -36, 8, 4); c.fillStyle(V("#e84a5a")); c.fillRect(-3, -35, 6, 2);
        c.restore();
    }), 1000, "Portale");
    auto Sfera = [&](const std::string& nome, const std::string& col, const std::string& luce, float r, int danno, float vel) {
        Oggetto(nome, FunzioneOggetto::Proiettile, CreaOggettoDisegno(r * 3, r * 3, resa, { { "fermo", 2, 12, true } }, [col, luce, r](K& c, const std::string&, int i, int) {
            c.save(); c.translate(0, -r * 1.5f);
            ell(c, 0, 0, r * (i ? 0.85f : 0.75f), r * (i ? 0.75f : 0.85f)); c.fillStyle(rg(-r * 0.3f, -r * 0.3f, 0, r, { "#ffffff", luce, col })); c.fill();
            c.restore();
        }), danno)->velocita = vel;
    };
    Oggetto("Stella lanciata", FunzioneOggetto::Proiettile, CreaOggettoDisegno(9, 9, resa, { { "fermo", 4, 16, true } }, [](K& c, const std::string&, int i, int) {
        c.save(); c.translate(0, -4.5f); c.rotate(i * TAU_ / 20); Stella(c, 4.2f, 1.9f, "#ffe14a", "#e89a1a", "#a8601a"); c.restore();
    }), 1)->velocita = 185;
    Sfera("Palla di neve", "#a8c8e8", "#ffffff", 3, 1, 175);
    Sfera("Ghianda", "#7a4a1e", "#c8904a", 3, 1, 110);
    Sfera("Spora", "#a83a8a", "#ffb0e8", 3, 1, 100);
    Sfera("Bolla elettrica", "#3a8ad8", "#c8f8ff", 3.4f, 1, 95);
    Sfera("Tappo", "#3a3a4a", "#e84a5a", 2.8f, 1, 125);
    Sfera("Palla di fuoco", "#a8301a", "#ffd04a", 3.2f, 1, 120);
    Oggetto("Stella rovescia", FunzioneOggetto::Proiettile, CreaOggettoDisegno(10, 10, resa, { { "fermo", 4, 16, true } }, [](K& c, const std::string&, int i, int) {
        c.save(); c.translate(0, -5); c.rotate(-i * TAU_ / 20); c.scale(1, -1); Stella(c, 4.6f, 2, "#ff8ae8", "#a03ad8", "#5a1a8a"); c.restore();
    }), 1)->velocita = 115;

    // --- i sei mondi ------------------------------------------------------------------------------------------
    std::vector<MondoGioco> mondi(6);
    auto Cr = [](unsigned seme, float base, float amp, const std::string& top, const std::string& bot, const std::string& light, const std::string& shade,
                 const std::string& haze = "", float hazeA = 0.5f, bool punte = false) {
        Cresta r; r.seme = seme; r.base = base; r.amp = amp; r.top = top; r.bot = bot; r.light = light; r.shade = shade; r.haze = haze; r.hazeA = hazeA; r.punte = punte; return r;
    };
    auto Righe = [](unsigned seme, float base, float hmin, float hmax, float passo, const std::function<void(float, float, float, unsigned)>& f) {
        Fila(seme, base, hmin, hmax, passo, [&](float x, float b, float h, unsigned sd) { Ripeti(x, h * 0.6f, [&](float xx) { f(xx, b, h, sd); }); });
    };
    auto Nuvole = [](K& c, unsigned seme, int quante, float y0, float y1, float w0, float w1, const ColoriNuvola& n) {
        for (int i = 0; i < quante; i++) { Caso r(seme + i); float x = r() * LS, y = y0 + r() * (y1 - y0), w = w0 + r() * (w1 - w0);
            Ripeti(x, w, [&](float xx) { Nuvola(c, xx, y, w, n, seme + i); }); }
    };
    // 1. Isole del Vento
    {
        MondoGioco& m = mondi[0]; m.nome = "Isole del Vento";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#3aa8f2", "#7ad0ff", "#e2f6ff" }; ci.sole[0] = 1080; ci.sole[1] = 100; ci.sole[2] = 38; ci.coloreSole = "#fffbe0"; ci.raggi = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.03f, [Nuvole](K& c) { Nuvole(c, 21, 10, 120, 480, 200, 420, { "#f6fbff", "#ffffff", "#bcd8f2" }); } },
            { 0.12f, [](K& c) { for (int i = 0; i < 6; i++) { Caso r(i + 31); float y = 200 + r() * 220, w = 120 + r() * 120;
                    ColoriAlbero a; a.trunk = "#7a5a32"; a.dark = "#3a9a3a"; a.mid = "#5ac04a"; a.light = "#9ae06a"; a.fruit = "#ff6a8a";
                    Ripeti(160 + i * 320.f, 200, [&](float xx) { Isola(c, xx, y, w, "#b07a4a", "#6ad04a", a, i + 31); }); } } },
            { 0.3f, [Nuvole](K& c) { Nuvole(c, 51, 8, 520, 580, 240, 440, { "#ffffff", "#ffffff", "#cfe4f6" }); } } };
        m.scen.primoPiano = "#2a5a1a"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 7;
        m.terra.kind = "grass"; m.terra.body = "#9a6a3a"; m.terra.top = "#6ad04a"; m.terra.blades = { "#5ac03a", "#7ade5a", "#a6f07a", "#4aa82a" }; m.terra.flowers = { "#ff6ab0", "#ffffff", "#ffd04a" };
        m.blocco.kind = "stone"; m.blocco.body = "#a8a49a"; m.blocco.top = "#d8d4c8"; m.blocco.moss = "#6ad04a";
        m.legnoPiatt = "#b07a4a"; m.luceMobile = "#ffffff";
        m.e = "Dino delle nuvole"; m.j = "Dino saltamontagne"; m.f = "Ape trombettiera"; m.s = "Golem di nuvola"; m.boss = "Re delle Nuvole";
        m.frase = "Il Mago Rovescio ha capovolto il regno e le isole volano via! Spazio salta (anche in aria), X lancia le stelle.";
    }
    // 2. Giardino dei Giganti
    {
        MondoGioco& m = mondi[1]; m.nome = "Giardino dei Giganti";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#7ad0ff", "#bfeaff", "#f2ffe0" }; ci.sole[0] = 200; ci.sole[1] = 90; ci.sole[2] = 34; ci.coloreSole = "#fffbe0"; ci.raggi = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.05f, [](K& c) { RiempiSotto(c, 440, "#8ad06a", "#6ab04a");
                for (float x = 0; x < LS; x += 14) { float h = 80 + std::sin(x * 0.02f) * 40 + (float)(((int)x * 13) % 40);
                    c.fillStyle(V(((int)x / 14) % 2 ? "#7ac85a" : "#9ae07a")); c.beginPath(); c.moveTo(x, 450); c.quadraticCurveTo(x + 10, 450 - h * 0.6f, x + 18 + std::sin(x) * 8, 450 - h); c.lineTo(x + 8, 450); c.fill(); } } },
            { 0.18f, [Righe](K& c) { RiempiSotto(c, 520, "#4a9a3a");
                Righe(81, 525, 220, 360, 300, [&](float x, float b, float h, unsigned sd) { ColoriFungo f{ "#f2e6c8", "#ffffff", sd % 2 ? "#e8503a" : "#a05ad8", "#ffffff" }; Fungo(c, x, b, h, f, sd); }); } },
            { 0.4f, [Righe](K& c) { Righe(83, 580, 180, 300, 180, [&](float x, float b, float h, unsigned sd) { ColoriFiore f{ "#3a8a3a", "#4aa83a", "#ff7ac0", "#ffd04a" }; Fiore(c, x, b, h, f, sd); }); } } };
        m.scen.primoPiano = "#1a4a14"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 8;
        m.terra.kind = "grass"; m.terra.body = "#7a5232"; m.terra.top = "#5ac03a"; m.terra.blades = { "#4aa82a", "#6ad04a", "#8ae06a" }; m.terra.flowers = { "#ff6ab0", "#ffd04a" };
        m.blocco.kind = "wood"; m.blocco.body = "#a8743a"; m.blocco.top = "#d8a45a";
        m.legnoPiatt = "#8a5a2a"; m.luceMobile = "#ffd04a";
        m.e = "Lucertola dorata"; m.j = "Spiritello del prato"; m.f = "Ape operaia"; m.s = "Fungo sparaspore"; m.boss = "Regina delle Api";
        m.frase = "Il Giardino dei Giganti: qui i fili d'erba sono alberi e i funghi sono case. Attento alle api!";
    }
    // 3. Abissi di Corallo
    {
        MondoGioco& m = mondi[2]; m.nome = "Abissi di Corallo";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#0a5a8a", "#0a3a6a", "#061a3a" }; ci.raggi = true; ci.sole[0] = 640; ci.sole[1] = -60; ci.sole[2] = 10; ci.coloreSole = "#bff4ff"; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.05f, [Cr](K& c) { DisegnaCresta(c, Cr(91, 500, 200, "#0e4a7a", "#08305a", "rgba(120,220,255,0.15)", "rgba(0,0,30,0.2)", "#0a3a6a", 0.6f)); } },
            { 0.18f, [Cr, Righe](K& c) { DisegnaCresta(c, Cr(93, 560, 80, "#1a6a8a", "#0a4a6a", "rgba(120,220,255,0.2)", "rgba(0,0,30,0.2)"));
                Righe(94, 560, 140, 260, 90, [&](float x, float b, float h, unsigned sd) { Alga(c, x, b, h, "#2a8a5a", sd % 100); }); } },
            { 0.4f, [Righe](K& c) { Righe(96, 592, 120, 200, 200, [&](float x, float b, float h, unsigned sd) { Corallo(c, x, b, h, sd % 2 ? "#ff6a8a" : "#ffa84a", sd); }); } } };
        m.scen.primoPiano = "#041628"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 6;
        m.acqua = true; m.buio = 0.15f;
        m.terra.kind = "sand"; m.terra.body = "#8a7a5a"; m.terra.top = "#e2cc94";
        m.blocco.kind = "stone"; m.blocco.body = "#3a6a7a"; m.blocco.top = "#6aa8b0"; m.blocco.moss = "#ff6a8a";
        m.legnoPiatt = "#6a5a3a"; m.luceMobile = "#5af0ff";
        m.e = "Golem di corallo"; m.j = "Pesce palla"; m.f = "Medusa elettrica"; m.s = "Medusa sparabolle"; m.boss = "Medusa Regina";
        m.frase = "Gli Abissi di Corallo: qui si nuota! Premi Spazio piu' volte per risalire.";
    }
    // 4. Fabbrica dei Giocattoli
    {
        MondoGioco& m = mondi[3]; m.nome = "Fabbrica dei Giocattoli";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#ffc8de", "#ffe0ec", "#fff6d8" }; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.05f, [](K& c) { const char* col[] = { "#f2a8c8", "#a8c8f2", "#f2d88a" };
                for (int i = 0; i < 12; i++) { Caso r(i + 70); float x = r() * LS, rr = 40 + r() * 80, y = 120 + r() * 300, rot = r() * 3;
                    Ripeti(x, rr * 1.2f, [&](float xx) { Ingranaggio(c, xx, y, rr, col[i % 3], "#ffe6f0", rot); }); } } },
            { 0.2f, [](K& c) { RiempiSotto(c, 540, "#8ab8f2");
                const char* col[] = { "#e84a5a", "#f2c23a", "#4ac86a", "#4a8ae8", "#b05ae8" };
                float x = 40; int i = 0;
                while (x < LS - 120) { Caso r(i + 3); float s = 50 + r() * 40; int n = 1 + (int)(r() * 3);
                    for (int k = 0; k < n; k++) Cubo(c, x, 540 - k * s, s, col[(i + k) % 5]);
                    x += s * 1.6f + r() * 90; i++; } } },
            { 0.4f, [](K& c) { c.fillStyle(V("#4a4a6a")); c.fillRect(0, 470, LS, 14); c.fillStyle(V("#2a2a3a"));
                for (float x = 0; x < LS; x += 30) { ell(c, x, 477, 7, 7); c.fill(); }
                c.strokeStyle(V("#6a6a8a")); c.lineWidth(6); for (float x = 100; x < LS; x += 400) { c.beginPath(); c.moveTo(x, 484); c.lineTo(x, 600); c.stroke(); } } } };
        m.scen.primoPiano = "#3a2a5a"; m.scen.tipoPrimoPiano = "springs"; m.scen.atmosfera = 9;
        m.terra.kind = "toy"; m.terra.body = "#5a6ad8"; m.terra.cols = { "#e84a5a", "#f2c23a", "#4ac86a", "#4a8ae8", "#b05ae8" };
        m.blocco.kind = "toy"; m.blocco.body = "#e84a5a"; m.blocco.cols = { "#f2c23a", "#4a8ae8" };
        m.legnoPiatt = "#4a8ae8"; m.luceMobile = "#ff7ab0";
        m.e = "Soldatino a carica"; m.j = "Robottino a molla"; m.f = "Elicottero giocattolo"; m.s = "Cannoniere di latta"; m.boss = "Robot Gigante";
        m.frase = "La Fabbrica dei Giocattoli: i giocattoli si sono ribellati! Trova le chiavi a carica per aprire i portoni.";
    }
    // 5. Cuore del Vulcano
    {
        MondoGioco& m = mondi[4]; m.nome = "Cuore del Vulcano";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#1a0404", "#4a0e06", "#a8300a" }; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.05f, [Cr](K& c) { Cresta s = Cr(101, 640, 240, "#3a1410", "#1a0604", "", ""); s.soffitto = true; DisegnaCresta(c, s);
                Cresta v = Cr(103, 480, 220, "#3a1410", "#2a0a06", "rgba(255,120,40,0.35)", "rgba(0,0,0,0.3)", "#a8300a", 0.45f, true); v.lava = true; DisegnaCresta(c, v); } },
            { 0.2f, [Righe](K& c) { for (float x = 120; x < LS; x += 520) { c.fillStyle(lg(x, 0, x + 40, 0, { "#ffb03a", "#ffe27a", "#ff6a1a" })); c.fillRect(x, 0, 40, 560); }
                Righe(105, 560, 100, 180, 240, [&](float x, float b, float h, unsigned sd) { ColoriCristallo k{ "#5a1a0a", "#c84a1a", "#ffb03a", "#ff6a1a" }; Cristalli(c, x, b, h, k, sd); }); } } };
        m.scen.primoPiano = "#0a0202"; m.scen.tipoPrimoPiano = "rocks"; m.scen.atmosfera = 4;
        m.buio = 0.25f;
        m.terra.kind = "volcanic"; m.terra.body = "#2a1410"; m.terra.top = "#4a2a22"; m.terra.glow = "#ff6a1a";
        m.blocco.kind = "stone"; m.blocco.body = "#3a1a14"; m.blocco.top = "#6a3a2a";
        m.legnoPiatt = "#4a2a22"; m.luceMobile = "#ffb03a";
        m.e = "Salamandra di lava"; m.j = "Lapillo"; m.f = "Pipistrello di fuoco"; m.s = "Golem di magma"; m.boss = "Drago del Vulcano";
        m.frase = "Il Cuore del Vulcano: fa un caldo tremendo! Salta sulle rocce e non toccare le punte.";
    }
    // 6. Castello Sottosopra
    {
        MondoGioco& m = mondi[5]; m.nome = "Castello Sottosopra";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#1a0e36", "#3a2a6a", "#6a4a9a" }; ci.stelle = true; ci.luna[0] = 640; ci.luna[1] = 360; ci.luna[2] = 46; ci.coloreLuna = "#e8d8ff"; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.05f, [](K& c) { ColoriCastello a{ "#4a3a72", "#8a3ab8", "#ffd04a", "#ffd04a", "#8a3ab8" };
                c.save(); c.translate(0, HS); c.scale(1, -1); Castello(c, 300, 640, 230, a); Castello(c, 1450, 620, 200, a); c.restore();
                ColoriCastello b{ "#3a2a5a", "#6a2a9a", "#ffd04a", "#ffd04a", "#6a2a9a" }; Castello(c, 1000, 500, 240, b); } },
            { 0.25f, [](K& c) { RiempiSotto(c, 600, "#2a1e4a");
                for (float x = 40; x < LS; x += 300) { Colonna(c, x, 600, 240, 30, "#4a3a72", "#ffd04a");
                    c.save(); c.translate(0, HS); c.scale(1, -1); Colonna(c, x + 150, 600, 200, 30, "#4a3a72", "#ffd04a"); c.restore(); } } } };
        m.scen.primoPiano = "#0e061e"; m.scen.tipoPrimoPiano = "thorns"; m.scen.atmosfera = 10;
        m.buio = 0.35f;
        m.terra.kind = "purple"; m.terra.body = "#3a2a5a"; m.terra.top = "#9a7ad8";
        m.blocco.kind = "stone"; m.blocco.body = "#3a2a5a"; m.blocco.top = "#6a5a9a";
        m.legnoPiatt = "#5a3a8a"; m.luceMobile = "#ffd04a";
        m.e = "Cavaliere rovescio"; m.j = "Folletto rovescio"; m.f = "Pipistrello rovescio"; m.s = "Apprendista mago"; m.boss = "Mago Rovescio";
        m.frase = "Il Castello Sottosopra: le torri pendono dal cielo. Il Mago Rovescio ti aspetta in fondo!";
    }

    // --- musica ----------------------------------------------------------------------------------------------
    p.brani.push_back(CreaBrano("Tema di Nilo", 120, 8, {
        { "Melodia", 0, 0.42f, "C5:1 E5:1 G5:2 E5:1 G5:1 A5:2 G5:1 E5:1 C5:2 D5:4 E5:1 F5:1 G5:2 A5:1 G5:1 F5:2 E5:1 D5:1 C5:2 G4:4 C5:1 E5:1 G5:2 C6:2 B5:1 A5:1 G5:2 F5:1 E5:1 D5:2 C5:4" },
        { "Basso", 1, 0.5f, "C3:2 G2:2 C3:2 G2:2 F2:2 C3:2 G2:2 D3:2 C3:2 G2:2 A2:2 E2:2 F2:2 G2:2 C3:4" },
        { "Batteria", 4, 0.45f, "K.h.S.h.K.hKS.h.", true } }));
    p.brani.push_back(CreaBrano("Isole del Vento", 126, 8, {
        { "Flauto", 3, 0.42f, "G5:2 E5:1 C5:1 D5:2 G4:2 A4:1 B4:1 C5:1 D5:1 E5:4 F5:2 E5:1 D5:1 C5:2 A4:2 B4:1 C5:1 D5:2 G4:4 R:2" },
        { "Ukulele", 1, 0.25f, "C4:1 E4:1 G4:1 E4:1 A3:1 C4:1 E4:1 C4:1 F3:1 A3:1 C4:1 A3:1 G3:1 B3:1 D4:1 B3:1" },
        { "Basso", 1, 0.45f, "C2:4 A1:4 F1:4 G1:4" },
        { "Batteria", 4, 0.4f, "K.h.S.hKK.h.S.h.", true } }));
    p.brani.push_back(CreaBrano("Giardino dei Giganti", 108, 8, {
        { "Clarinetto", 1, 0.42f, "F5:1 A5:1 C6:2 A5:2 F5:2 G5:1 A5:1 Bb5:2 A5:2 G5:2 E5:1 F5:1 G5:2 C5:2 E5:2 F5:6 R:2" },
        { "Pizzicato", 0, 0.25f, "F4:1 R:1 C4:1 R:1 A3:1 R:1 C4:1 R:1 Bb3:1 R:1 D4:1 R:1 C4:1 R:1 E4:1 R:1" },
        { "Basso", 1, 0.45f, "F2:2 C2:2 Bb1:2 C2:2" },
        { "Batteria", 4, 0.35f, "K...S..hK.h.S...", true } }));
    p.brani.push_back(CreaBrano("Abissi di Corallo", 88, 8, {
        { "Vibrafono", 3, 0.42f, "E5:2 G5:2 B5:3 A5:1 G5:2 E5:2 D5:4 C5:2 E5:2 G5:3 F#5:1 E5:2 B4:2 E5:8 R:4" },
        { "Bolle", 3, 0.18f, "B5:1 R:3 E6:1 R:3 G5:1 R:3 D6:1 R:3" },
        { "Basso", 1, 0.45f, "E2:4 C2:4 A1:4 B1:4" } }));
    p.brani.push_back(CreaBrano("Fabbrica dei Giocattoli", 132, 8, {
        { "Carillon", 3, 0.42f, "C6:1 G5:1 E5:1 G5:1 C6:1 D6:1 E6:2 D6:1 B5:1 G5:1 B5:1 D6:2 C6:2 A5:1 F5:1 A5:1 C6:1 B5:1 G5:1 E5:1 G5:1 C6:4 R:4" },
        { "Basso", 1, 0.5f, "C3:1 R:1 G2:1 R:1 C3:1 R:1 G2:1 R:1 G2:1 R:1 D3:1 R:1 G2:1 R:1 D3:1 R:1" },
        { "Batteria", 4, 0.45f, "K.hhS.hhK.hhS.ho", true } }));
    p.brani.push_back(CreaBrano("Cuore del Vulcano", 112, 8, {
        { "Ottoni", 2, 0.3f, "D4:2 F4:1 G4:1 Ab4:2 G4:2 F4:1 D4:1 F4:2 D4:4 D4:2 F4:1 G4:1 C5:2 Bb4:2 Ab4:1 G4:1 F4:2 G4:4" },
        { "Basso", 2, 0.45f, "D2:1 D2:1 R:1 D2:1 F2:1 R:1 Ab2:1 G2:1" },
        { "Tamburi", 4, 0.55f, "K..KS..KK.K.S.KS", true } }));
    p.brani.push_back(CreaBrano("Castello Sottosopra", 100, 8, {
        { "Clavicembalo", 1, 0.38f, "A4:1 C5:1 E5:1 A5:1 G#5:2 E5:2 F5:1 D5:1 B4:1 G#4:1 A4:4 C5:1 E5:1 A5:1 C6:1 B5:2 G#5:2 A5:1 E5:1 C5:1 E5:1 A4:4" },
        { "Basso", 1, 0.45f, "A2:4 D2:4 E2:4 A2:4" },
        { "Tamburello", 4, 0.3f, "K...h.h.S...h.hh", true } }));
    p.brani.push_back(CreaBrano("Duello capovolto", 148, 8, {
        { "Melodia", 2, 0.3f, "E5:1 E5:1 G5:1 E5:1 A5:2 G5:2 E5:1 D5:1 C5:1 D5:1 E5:4 E5:1 E5:1 G5:1 B5:1 C6:2 B5:2 A5:1 G5:1 A5:1 B5:1 E5:4" },
        { "Basso", 2, 0.45f, "E2:1 E3:1 E2:1 E3:1 C2:1 C3:1 C2:1 C3:1 D2:1 D3:1 D2:1 D3:1 B1:1 B2:1 B1:1 B2:1" },
        { "Batteria", 4, 0.55f, "KhShKhShKKShKhSo", true } }));
    for (int i = 0; i < 6; i++) mondi[i].brano = p.brani[i + 1].nome;

    // --- effetti sonori --------------------------------------------------------------------------------------
    p.effetti = {
        Effetto("Salto", 0, 300, 820, 0.16f, 0.35f),
        Effetto("Stellina", 0, 1046, 1568, 0.12f, 0.32f),
        Effetto("Cuore", 3, 520, 1040, 0.3f, 0.45f, 0, 0.05f, 9),
        Effetto("Chiave", 1, 900, 1800, 0.3f, 0.4f, 0, 0.04f, 12),
        Effetto("Porta", 1, 220, 70, 0.45f, 0.4f, 0.2f),
        Effetto("Molla", 1, 220, 1200, 0.32f, 0.45f, 0, 0.12f, 18),
        Effetto("Bandierina", 0, 660, 1320, 0.45f, 0.4f, 0, 0.04f, 8),
        Effetto("Portale", 3, 262, 1568, 1.0f, 0.5f, 0, 0.08f, 5),
        Effetto("Colpo", 4, 320, 90, 0.12f, 0.5f),
        Effetto("Sconfitto", 0, 700, 90, 0.35f, 0.42f, 0.25f),
        Effetto("Ahi", 2, 420, 130, 0.25f, 0.5f, 0.2f),
        Effetto("Sparo nemico", 0, 520, 200, 0.14f, 0.3f, 0.15f),
        Effetto("Boss sconfitto", 3, 880, 40, 1.5f, 0.6f, 0.4f, 0.1f, 6),
    };
    p.suoniEventi = { { "salto", "Salto" }, { "moneta", "Stellina" }, { "colpo", "Colpo" }, { "molla", "Molla" } };

    NomiGioco n;
    n.moneta = "Stellina"; n.cuore = "Cuore"; n.molla = "Fungo rimbalzino"; n.controllo = "Bandierina"; n.chiave = "Chiave a carica";
    n.porta = "Portone rovescio"; n.cancello = "Sbarre del Mago"; n.arrivo = "Mongolfiera";
    n.branoBoss = "Duello capovolto"; n.branoVittoria = "Tema di Nilo";
    n.fraseBoss = "Il guardiano e' sconfitto! Le sbarre del Mago spariscono: sali sulla mongolfiera.";
    n.fraseFinale = "Il Mago Rovescio e' sconfitto e il regno torna dritto! Grazie, Nilo e Fiocco!";
    n.energiaBoss = 10; n.seme = 3;
    CostruisciMondi(p, resa, mondi, n);
    return p;
}

}  // namespace piatt
