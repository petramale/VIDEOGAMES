// Gioco B: "Nora e Bullone: gli Ingranaggi del Tempo".
// Nora, giovane inventrice, e Bullone, il suo robot, inseguono il Dottor Clessidra attraverso
// sei epoche per recuperare gli ingranaggi della Grande Macchina del Tempo. Pixel art fine.
#include "platform_tre.h"
#include <cmath>

namespace piatt {

using namespace arte;

namespace {

const float TAU_ = 6.28318530718f;
void ell(K& c, float x, float y, float rx, float ry, float rot = 0) { c.beginPath(); c.ellipse(x, y, std::max(0.05f, rx), std::max(0.05f, ry), rot, 0, TAU_); }

// --- il cast ----------------------------------------------------------------------------------
Umanoide Nora() {
    Umanoide u; u.skin = "#f2c39a"; u.hair = "#6a3a1e"; u.hairStyle = "pony"; u.accent = "#f2c23a"; u.eye = "#3a8a3a"; u.lash = true;
    u.top = "#f2e6c8"; u.overall = "#c8742a"; u.bottom = "#8a4a1e"; u.shoes = "#4a2a1a"; u.goggles = true; u.item = "wrench"; u.brows = true; u.grin = true;
    return u;
}
Umanoide Mummia() { Umanoide u; u.skin = "#e8dcb8"; u.face = "mummy"; u.top = "#e2d6b0"; u.bottom = "#d4c8a0"; u.shoes = "#c8bc98"; u.bandage = true; return u; }
Umanoide Pirata() {
    Umanoide u; u.skin = "#efe8d4"; u.face = "skull"; u.top = "#2a2a46"; u.bottom = "#3a2a24"; u.shoes = "#2a1a14"; u.hat = "bandana"; u.hatc = "#c83a3a";
    u.belt = "#5a3a1a"; u.item = "sword"; u.collar = "#efe8d4"; return u;
}
Umanoide Bandito() {
    Umanoide u; u.skin = "#d8a478"; u.hair = "#3a2a1a"; u.eye = "#6a4a2a"; u.brows = true; u.top = "#7a4a8a"; u.bottom = "#3a3a5a"; u.shoes = "#3a1e10";
    u.hat = "cowboy"; u.hatc = "#6a4a2a"; u.scarf = "#c83a3a"; u.item = "pistol"; u.belt = "#4a2a14"; return u;
}
Umanoide Cavaliere() {
    Umanoide u; u.skin = "#cccccc"; u.helmet = "#9aa4bc"; u.plume = "#c83a3a"; u.top = "#8a94ac"; u.bottom = "#6a7490"; u.shoes = "#4a5068";
    u.belt = "#5a3a1a"; u.item = "sword"; u.glow = "#ffd04a"; return u;
}
Umanoide Mago() {
    Umanoide u; u.skin = "#f2c8a8"; u.hair = "#e8e8f0"; u.eye = "#8a3ad8"; u.brows = true; u.top = "#2a6a4a"; u.robe = true; u.trim = "#f2c94a";
    u.hat = "wizard"; u.hatc = "#2a6a4a"; u.beard = true; u.item = "staff"; u.orb = "#7aff9a"; return u;
}
DisegnaPersonaggio Uman(Umanoide u, float s = 1) { return [u, s](K& c, float x, float y, float k, const Posa& p) { DisegnaUmanoide(c, x, y, k * s, u, p); }; }

// --- oggetti --------------------------------------------------------------------------------
void Ingr(K& c, float x, float y, float r, float rot, const std::string& col, const std::string& scuro) {
    c.save(); c.translate(x, y); c.rotate(rot);
    c.beginPath();
    const int denti = 8;
    for (int i = 0; i < denti * 2; i++) {
        float a0 = i * TAU_ / (denti * 2), a1 = (i + 1) * TAU_ / (denti * 2);
        float rr = i % 2 ? r * 0.78f : r;
        if (i == 0) c.moveTo(std::cos(a0) * rr, std::sin(a0) * rr); else c.lineTo(std::cos(a0) * rr, std::sin(a0) * rr);
        c.lineTo(std::cos(a1) * rr, std::sin(a1) * rr);
    }
    c.closePath();
    c.fillStyle(lg(-r, -r, r, r, { sh(col, 0.45f), col, scuro })); c.fill();
    c.strokeStyle(V(sh(scuro, -0.3f))); c.lineWidth(r * 0.08f); c.stroke();
    ell(c, 0, 0, r * 0.52f, r * 0.52f); c.fillStyle(lg(-r, -r, r, r, { scuro, sh(col, 0.2f) })); c.fill();
    ell(c, 0, 0, r * 0.22f, r * 0.22f); c.fillStyle(V(sh(scuro, -0.45f))); c.fill();
    c.restore();
}

void Cuore(K& c, float x, float y, float s, const std::string& col) {
    c.save(); c.translate(x, y); c.scale(s, s);
    c.beginPath(); c.moveTo(0, 3); c.bezierCurveTo(-1, 2, -6, -1, -6, -4); c.bezierCurveTo(-6, -7, -2, -8, 0, -5);
    c.bezierCurveTo(2, -8, 6, -7, 6, -4); c.bezierCurveTo(6, -1, 1, 2, 0, 3); c.closePath();
    c.fillStyle(rg(-2, -5, 0, 8, { sh(col, 0.5f), col, sh(col, -0.4f) })); c.fill();
    c.strokeStyle(V(sh(col, -0.6f))); c.lineWidth(0.6f); c.stroke();
    ell(c, -3, -5, 1.4f, 0.9f, -0.5f); c.fillStyle(V("rgba(255,255,255,0.8)")); c.fill();
    c.restore();
}

struct ColoriMondo {
    std::string nome;
    Materiale terra, blocco, piatt;
};

}  // namespace

ProgettoGioco CreaNoraEBullone() {
    Resa resa; resa.hd = false; resa.r = 0.5f; resa.dens = 2;
    ProgettoGioco p = NuovoProgetto(Genere::Platform, StileGrafico::PixelFine, "Nora e Bullone: gli Ingranaggi del Tempo");
    p.personaggi.clear(); p.oggetti.clear(); p.scenari.clear(); p.caselle.clear(); p.brani.clear(); p.effetti.clear(); p.regole.clear();
    p.variabili.clear(); p.immagini.clear();
    p.mondo.mappe.clear();
    p.mondo.titolo = "Nora e Bullone: gli Ingranaggi del Tempo";
    p.mondo.autori = "Mario Petramale - Elvantis";
    p.mondo.giocatoriMax = 2;
    p.mondo.fisica.gravita = 900; p.mondo.fisica.salto = 315; p.mondo.fisica.velocita = 125; p.mondo.fisica.molla = 520;
    p.mondo.fisica.saltiInAria = 1;
    p.variabili["monete"] = 0;

    // --- personaggi ---------------------------------------------------------------------------
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
    std::vector<Anim> animNemico = { Anim::Fermo, Anim::Corsa, Anim::Attacco, Anim::Colpito };
    auto Nemico = [&](const DisegnaPersonaggio& f, float alt, float largo = 150, float sopra = 0) {
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = false; o.largo = largo; o.sopra = sopra;
        o.animazioni = animNemico; o.fotogrammiCorsa = 6; o.fotogrammiFermo = 3;
        return CreaDisegno(f, o);
    };
    auto Volante = [&](const DisegnaPersonaggio& f, float alt, float largo = 190) {
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = false; o.largo = largo; o.alto = 175;
        o.animazioni = { Anim::Fermo, Anim::Colpito }; o.fotogrammiFermo = 6;
        return CreaDisegno(f, o);
    };

    Personaggio("Nora", CreaPersonaggio(Uman(Nora()), 30, resa), TipoComportamento::Giocatore, 125, 4, 0, "Chiave inglese", 315);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    Robot bull;
    Personaggio("Bullone", CreaPersonaggio([bull](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, bull, q); }, 30, resa),
                TipoComportamento::Giocatore, 120, 5, 0, "Raggio", 300);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    p.eroe = "Nora"; p.eroe2 = "Bullone";

    // mondo 1: Preistoria
    { Dino d; d.c = "#4aa85a"; d.belly = "#e8e0a0"; d.stripe = "#2a6a3a";
      Personaggio("Raptor", Nemico([d](K& c, float x, float y, float s, const Posa& q) { DisegnaDino(c, x, y, s, d, q); }, 24, 190), TipoComportamento::Pattuglia, 45, 1, 1); }
    { Dino d; d.c = "#e8a43a"; d.belly = "#fff0b0"; d.stripe = "#a8641a";
      Personaggio("Raptor saltatore", Nemico([d](K& c, float x, float y, float s, const Posa& q) { DisegnaDino(c, x, y, s, d, q); }, 20, 190), TipoComportamento::Insegue, 60, 1, 1, "", 250, 120); }
    Personaggio("Pipistrello delle caverne", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * s, s, "#7a5a3a", "#4a3222", "#ffd04a", q); }, 22),
                TipoComportamento::Vola, 40, 1, 1, "", 0, 140, 2, false);
    Personaggio("Lanciasassi", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaGolem(c, x, y, s * 0.8f, "#c8b49a", "#8a7a64", "#4a4034", "#ff8a3a", q); }, 26, 170),
                TipoComportamento::Spara, 0, 2, 1, "Sasso", 0, 170, 2.2f);
    { Dino d; d.c = "#c85a3a"; d.belly = "#f2d8a0"; d.stripe = "#7a2a1a";
      Personaggio("Re Rex", Nemico([d](K& c, float x, float y, float s, const Posa& q) { DisegnaDino(c, x, y, s, d, q); }, 58, 200), TipoComportamento::Boss, 55, 10, 1, "Palla di fuoco", 0, 260, 1.8f, true, 2000); }
    // mondo 2: Antico Egitto
    Personaggio("Mummia", Nemico(Uman(Mummia()), 28), TipoComportamento::Pattuglia, 28, 2, 1);
    { Umanoide u = Mummia(); u.top = "#c8b890"; u.bottom = "#b8a880"; u.eye = "#5af0ff";
      Personaggio("Mummia lesta", Nemico(Uman(u), 26), TipoComportamento::Insegue, 55, 1, 1, "", 220, 120); }
    Personaggio("Pipistrello del deserto", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * s, s, "#c89a5a", "#8a6232", "#5af0ff", q); }, 22),
                TipoComportamento::Vola, 45, 1, 1, "", 0, 140, 2, false);
    Personaggio("Guardiano d'oro", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaGolem(c, x, y, s * 0.8f, "#ffe27a", "#e8b23a", "#a8701a", "#5af0ff", q); }, 28, 170),
                TipoComportamento::Spara, 0, 2, 1, "Raggio d'oro", 0, 170, 2);
    Personaggio("Sfinge d'oro", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaGolem(c, x, y, s, "#ffe27a", "#e8b23a", "#a8701a", "#5af0ff", q); }, 60, 170),
                TipoComportamento::Boss, 50, 12, 1, "Raggio d'oro", 0, 260, 1.6f, true, 2500);
    // mondo 3: Medioevo
    Personaggio("Cavaliere", Nemico(Uman(Cavaliere()), 30), TipoComportamento::Pattuglia, 35, 2, 1);
    { Umanoide u = Cavaliere(); u.helmet = "#6a9a5a"; u.plume = "#f2c23a"; u.top = "#5a8a4a"; u.bottom = "#4a6a3a"; u.item = "";
      Personaggio("Scudiero", Nemico(Uman(u), 26), TipoComportamento::Insegue, 60, 1, 1, "", 230, 120); }
    Personaggio("Pipistrello del castello", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * s, s, "#6a4a3a", "#3a2a22", "#ffd04a", q); }, 22),
                TipoComportamento::Vola, 45, 1, 1, "", 0, 140, 2, false);
    Personaggio("Mago della torre", Nemico(Uman(Mago()), 30), TipoComportamento::Spara, 0, 2, 1, "Sfera magica", 0, 180, 2.2f);
    { Umanoide u = Cavaliere(); u.helmet = "#3a2a52"; u.plume = "#a04aff"; u.top = "#2a1e3e"; u.bottom = "#1e1630"; u.shoes = "#140e20"; u.belt = "#5a3a7a"; u.glow = "#ff3a8a";
      Personaggio("Cavaliere Nero", Nemico(Uman(u), 60), TipoComportamento::Boss, 60, 12, 1, "Sfera magica", 0, 260, 1.6f, true, 2500); }
    // mondo 4: Caraibi
    Personaggio("Scheletro pirata", Nemico(Uman(Pirata()), 28), TipoComportamento::Pattuglia, 40, 1, 1);
    { Umanoide u = Pirata(); u.hatc = "#2a8ad8"; u.top = "#6a2a2a"; u.item = "";
      Personaggio("Mozzo scheletro", Nemico(Uman(u), 24), TipoComportamento::Insegue, 65, 1, 1, "", 240, 120); }
    Personaggio("Gabbiano", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * s, s, "#f2f2f8", "#c8c8d8", "#2a2a3a", q); }, 22),
                TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    { Umanoide u = Pirata(); u.item = "pistol"; u.hatc = "#2a2a2a";
      Personaggio("Cannoniere", Nemico(Uman(u), 28), TipoComportamento::Spara, 0, 2, 1, "Palla di cannone", 0, 180, 2.4f); }
    { Umanoide u = Pirata(); u.hat = "cowboy"; u.hatc = "#1a1a24"; u.top = "#6a1a2a"; u.collar = "#f2c23a"; u.belt = "#c8a03a";
      Personaggio("Capitan Teschio", Nemico(Uman(u), 60), TipoComportamento::Boss, 65, 12, 1, "Palla di cannone", 0, 260, 1.5f, true, 2500); }
    // mondo 5: Far West
    Personaggio("Bandito", Nemico(Uman(Bandito()), 30), TipoComportamento::Pattuglia, 45, 2, 1);
    { Umanoide u = Bandito(); u.top = "#3a6a8a"; u.hatc = "#8a6a3a"; u.item = "";
      Personaggio("Bandito lesto", Nemico(Uman(u), 26), TipoComportamento::Insegue, 70, 1, 1, "", 240, 130); }
    Personaggio("Avvoltoio", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * s, s * 1.1f, "#5a3a2a", "#3a241a", "#f2e0a0", q); }, 24),
                TipoComportamento::Vola, 50, 1, 1, "", 0, 140, 2, false);
    { Umanoide u = Bandito(); u.top = "#4a4a4a"; u.scarf = "#2a2a2a";
      Personaggio("Pistolero", Nemico(Uman(u), 30), TipoComportamento::Spara, 0, 2, 1, "Pallottola", 0, 200, 1.8f); }
    { Umanoide u = Bandito(); u.hatc = "#1a1a1a"; u.top = "#1a1a24"; u.scarf = "#f2c23a"; u.bottom = "#2a2a2a";
      Personaggio("Jack il Lesto", Nemico(Uman(u), 60), TipoComportamento::Boss, 75, 14, 1, "Pallottola", 0, 280, 1.2f, true, 3000); }
    // mondo 6: Anno 3000
    { Robot r; r.metal = "#c84a4a"; r.core = "#ffd04a"; r.eye = "#ff3a3a"; r.ant = "#ffd04a"; r.fist = "#6a6a8a";
      Personaggio("Robot sentinella", Nemico([r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }, 28), TipoComportamento::Pattuglia, 45, 2, 1); }
    { Robot r; r.metal = "#4ac86a"; r.core = "#ffffff"; r.eye = "#ffffff"; r.ant = "#f2c23a"; r.fist = "#e84a5a";
      Personaggio("Robot saltellante", Nemico([r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }, 24), TipoComportamento::Insegue, 60, 1, 1, "", 260, 130); }
    Personaggio("Drone", Volante([](K& c, float x, float y, float s, const Posa& q) { DisegnaDrone(c, x, y - 60 * s, s, "#8a8ab8", "#ff3a6a", q); }, 20),
                TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    Personaggio("Drone laser", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaDrone(c, x, y - 60 * s, s, "#b8a84a", "#ff3a6a", q); }, 22, 190),
                TipoComportamento::Spara, 30, 2, 1, "Laser", 0, 200, 1.8f, false);
    Personaggio("Dottor Clessidra", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaMech(c, x, y, s, "#8a6a4a", q); }, 66, 200),
                TipoComportamento::Boss, 60, 18, 2, "Laser", 0, 300, 1.2f, true, 5000);

    // --- oggetti ---------------------------------------------------------------------------------
    auto Oggetto = [&](const std::string& nome, FunzioneOggetto f, Disegno d, int valore = 10, const std::string& suono = "") {
        OggettoDef o; o.nome = nome; o.autore = "Elvantis"; o.funzione = f; o.disegno = std::move(d); o.valore = valore; o.suono = suono;
        p.oggetti.push_back(o);
        return &p.oggetti.back();
    };
    Oggetto("Ingranaggio", FunzioneOggetto::Raccogli, CreaOggettoDisegno(14, 15, resa, { { "fermo", 8, 12, true } }, [](K& c, const std::string&, int i, int n) {
        float sq = std::fabs(std::cos(i * 3.14159f / n));   // gira su se stesso
        c.save(); c.translate(0, -8); c.scale(std::max(0.18f, sq), 1);
        c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 7, { "rgba(255,220,90,0.45)", "rgba(255,220,90,0)" })); c.fillRect(-7, -7, 14, 14); c.restore();
        Ingr(c, 0, 0, 5.6f, i * 0.2f, "#ffd04a", "#a8701a");
        c.restore();
        if (i == 0) { c.fillStyle(V("rgba(255,255,255,0.9)")); c.fillRect(-3.4f, -12, 1, 1); }
    }), 50, "Ingranaggio")->variabile = "monete";
    Oggetto("Cuore", FunzioneOggetto::Cura, CreaOggettoDisegno(14, 14, resa, { { "fermo", 4, 6, true } }, [](K& c, const std::string&, int i, int) {
        float s = 0.92f + 0.08f * std::sin(i * TAU_ / 4);
        Cuore(c, 0, -7, s, "#ff4a6a");
    }), 1, "Cuore");
    Oggetto("Chiave d'ottone", FunzioneOggetto::Chiave, CreaOggettoDisegno(14, 14, resa, { { "fermo", 4, 6, true } }, [](K& c, const std::string&, int i, int) {
        c.save(); c.translate(0, -7 + std::sin(i * TAU_ / 4) * 0.8f); c.rotate(-0.6f);
        c.fillStyle(lg(-5, -5, 5, 5, { "#fff0a0", "#e8b23a", "#8a5a1a" }));
        ell(c, -3, 0, 2.6f, 2.6f); c.fill(); c.fillRect(-1, -0.8f, 7, 1.6f); c.fillRect(3.4f, 0.8f, 1.2f, 1.8f); c.fillRect(5, 0.8f, 1.2f, 1.4f);
        ell(c, -3, 0, 1.1f, 1.1f); c.fillStyle(V("#3a2a14")); c.fill();
        c.restore();
    }), 100, "Chiave");
    { OggettoDef* o = Oggetto("Porta di bronzo", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 1, 1, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int, int) {
        c.fillStyle(lg(-8, -32, 8, 0, { "#8a5a2a", "#5a3a1a" })); c.fillRect(-8, -32, 16, 32);
        c.fillStyle(V("#3a2410")); c.fillRect(-6, -30, 12, 30);
        if (a == "chiusa") {
            c.fillStyle(lg(-6, -30, 6, 0, { "#e8b25a", "#a8703a", "#6a4220" })); c.fillRect(-6, -30, 12, 30);
            c.strokeStyle(V("#5a3a1a")); c.lineWidth(0.6f); for (int k = 0; k < 4; k++) { c.beginPath(); c.moveTo(-6, -24 + k * 7); c.lineTo(6, -24 + k * 7); c.stroke(); }
            Ingr(c, 0, -16, 3.4f, 0.3f, "#ffd04a", "#8a5a1a");
            c.fillStyle(V("#2a1a0a")); ell(c, 0, -16, 0.8f, 0.8f); c.fill(); c.fillRect(-0.4f, -16, 0.8f, 1.8f);
        }
    }), 0, "Porta"); o->richiede = "Chiave d'ottone"; o->solido = true; }
    { OggettoDef* o = Oggetto("Cancello", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 1, 1, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int, int) {
        c.fillStyle(V("#2a2a3a")); c.fillRect(-8, -32, 16, 3);
        if (a == "chiusa") {
            for (int k = 0; k < 4; k++) { c.fillStyle(lg(-8 + k * 4, 0, -6 + k * 4, 0, { "#c8ccd8", "#6a6e80" })); c.fillRect(-7 + k * 4, -30, 2, 30); c.fillStyle(V("#f2c23a")); c.fillRect(-7.2f + k * 4, -1.5f, 2.4f, 1.5f); }
            c.fillStyle(V("#8a8ea0")); c.fillRect(-8, -20, 16, 1.6f); c.fillRect(-8, -9, 16, 1.6f);
            c.save(); c.glow(4, "#ff4a6a"); ell(c, 0, -15, 2.2f, 2.2f); c.fillStyle(V("#ff4a6a")); c.fill(); c.restore();
        } else for (int k = 0; k < 4; k++) { c.fillStyle(V("#8a8ea0")); c.fillRect(-7 + k * 4, -32, 2, 4); }
    }), 0, "Porta"); o->richiede = "sconfiggere il guardiano"; o->solido = true; }
    Oggetto("Molla", FunzioneOggetto::Molla, CreaOggettoDisegno(16, 14, resa, { { "fermo", 1, 1, true }, { "scatta", 3, 12, false } }, [](K& c, const std::string& a, int i, int) {
        float h = a == "scatta" ? (i == 0 ? 4 : i == 1 ? 12 : 9) : 7;
        c.fillStyle(lg(0, -3, 0, 0, { "#6a6e80", "#3a3e4a" })); c.fillRect(-7, -3, 14, 3);
        c.strokeStyle(V("#c8ccd8")); c.lineWidth(1.1f); c.beginPath();
        for (int k = 0; k <= 6; k++) { float y = -3 - h * k / 6; float x = (k % 2 ? 4.5f : -4.5f); if (k == 0) c.moveTo(0, -3); c.lineTo(x, y); }
        c.stroke();
        c.fillStyle(lg(0, -h - 6, 0, -h - 3, { "#ff8a5a", "#c83a2a" })); c.fillRect(-7, -h - 6, 14, 3);
        c.fillStyle(V("rgba(255,255,255,0.6)")); c.fillRect(-6, -h - 6, 12, 0.8f);
    }), 520, "Molla");
    Oggetto("Lampione del tempo", FunzioneOggetto::Controllo, CreaOggettoDisegno(14, 34, resa, { { "fermo", 1, 1, true }, { "accesa", 4, 6, true } }, [](K& c, const std::string& a, int i, int) {
        bool on = a == "accesa";
        c.fillStyle(lg(-1.5f, 0, 1.5f, 0, { "#6a5a4a", "#3a2a1a" })); c.fillRect(-1.2f, -22, 2.4f, 22);
        c.fillStyle(V("#4a3a2a")); c.fillRect(-4, -2, 8, 2);
        // clessidra luminosa
        c.save(); c.translate(0, -27);
        if (on) { c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 11 + std::sin(i * 1.6f), { "rgba(120,240,255,0.55)", "rgba(120,240,255,0)" })); c.fillRect(-12, -12, 24, 24); c.restore(); }
        c.fillStyle(V("#c8a03a")); c.fillRect(-4.5f, -6.5f, 9, 1.4f); c.fillRect(-4.5f, 5.1f, 9, 1.4f);
        c.beginPath(); c.moveTo(-3.5f, -5); c.lineTo(3.5f, -5); c.lineTo(0.6f, 0); c.lineTo(3.5f, 5); c.lineTo(-3.5f, 5); c.lineTo(-0.6f, 0); c.closePath();
        c.fillStyle(V(on ? "rgba(200,250,255,0.9)" : "rgba(180,200,220,0.5)")); c.fill();
        c.fillStyle(V(on ? "#5af0ff" : "#8a7a5a"));
        float sab = on ? (i % 4) / 4.0f : 0.8f;
        c.beginPath(); c.moveTo(-2.6f * (1 - sab), -4 + sab * 3.5f); c.lineTo(2.6f * (1 - sab), -4 + sab * 3.5f); c.lineTo(0, 0); c.closePath(); c.fill();
        c.beginPath(); c.moveTo(0, 4.8f - sab * 3.5f); c.lineTo(3.2f, 4.8f); c.lineTo(-3.2f, 4.8f); c.closePath(); c.fill();
        c.restore();
    }), 0, "Controllo");
    Oggetto("Portale del tempo", FunzioneOggetto::Arrivo, CreaOggettoDisegno(34, 40, resa, { { "fermo", 8, 10, true } }, [](K& c, const std::string&, int i, int n) {
        float a = i * TAU_ / n;
        c.save(); c.translate(0, -19);
        c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 19, { "rgba(120,230,255,0.6)", "rgba(160,90,255,0.25)", "rgba(160,90,255,0)" })); c.fillRect(-19, -19, 38, 38); c.restore();
        for (int k = 0; k < 3; k++) {
            c.save(); c.rotate(a + k * 2.1f); c.lighter();
            c.strokeStyle(V(k == 1 ? "rgba(255,120,240,0.7)" : "rgba(120,240,255,0.75)")); c.lineWidth(1.2f);
            c.beginPath(); for (float t = 0; t < 9; t += 0.3f) { float rr = 1 + t * 1.4f; float x = std::cos(t) * rr, y = std::sin(t) * rr; if (t == 0) c.moveTo(x, y); else c.lineTo(x, y); } c.stroke();
            c.restore();
        }
        for (int k = 0; k < 8; k++) Ingr(c, std::cos(k * TAU_ / 8) * 14, std::sin(k * TAU_ / 8) * 14, 3.2f, a * (k % 2 ? 1 : -1) * 2, "#e8b23a", "#7a4a1a");
        c.restore();
        c.fillStyle(lg(0, -4, 0, 0, { "#8a8ea0", "#4a4e60" })); c.fillRect(-12, -3, 24, 3);
    }), 1000, "Portale");
    Oggetto("Chiave inglese", FunzioneOggetto::Proiettile, CreaOggettoDisegno(10, 6, resa, { { "fermo", 1, 1, true } }, [](K& c, const std::string&, int, int) {
        c.save(); c.translate(0, -3);
        c.fillStyle(lg(0, -2, 0, 2, { "#e8ecf8", "#8a8ea0" })); c.fillRect(-3.5f, -0.8f, 7, 1.6f);
        ell(c, 4, 0, 1.8f, 1.8f); c.fill(); ell(c, -4, 0, 1.8f, 1.8f); c.fill();
        c.fillStyle(V("#3a3e4a")); c.fillRect(4, -0.5f, 2, 1); c.fillRect(-6, -0.5f, 2, 1);
        c.restore();
    }), 1, "Lancio")->velocita = 170;
    auto Sfera = [&](const std::string& nome, const std::string& col, const std::string& luce, float r, int danno, float vel) {
        Oggetto(nome, FunzioneOggetto::Proiettile, CreaOggettoDisegno(r * 3, r * 3, resa, { { "fermo", 2, 12, true } }, [col, luce, r](K& c, const std::string&, int i, int) {
            c.save(); c.translate(0, -r * 1.5f);
            c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, r * 1.5f, { rgba(luce, 0.7f), rgba(luce, 0) })); c.fillRect(-r * 1.5f, -r * 1.5f, r * 3, r * 3); c.restore();
            ell(c, 0, 0, r * (i ? 0.85f : 0.75f), r * (i ? 0.75f : 0.85f)); c.fillStyle(rg(-r * 0.3f, -r * 0.3f, 0, r, { "#ffffff", luce, col })); c.fill();
            c.restore();
        }), danno)->velocita = vel;
    };
    Sfera("Raggio", "#2a8ad8", "#9ff8ff", 3, 1, 180);
    Sfera("Sasso", "#4a4034", "#c8b49a", 2.6f, 1, 100);
    Sfera("Palla di fuoco", "#c83a1a", "#ffb03a", 3.4f, 1, 110);
    Sfera("Raggio d'oro", "#a8701a", "#fff0a0", 3, 1, 115);
    Sfera("Sfera magica", "#6a2aa8", "#e8aaff", 3.2f, 1, 105);
    Sfera("Palla di cannone", "#1a1a24", "#8a8aa0", 3.2f, 1, 125);
    Sfera("Pallottola", "#6a4a1a", "#ffe08a", 1.8f, 1, 170);
    Sfera("Laser", "#c81a4a", "#ff8ab0", 2.4f, 1, 160);

    // --- mondi -------------------------------------------------------------------------------------
    std::vector<MondoGioco> mondi(6);
    auto Cr = [](unsigned seme, float base, float amp, const std::string& top, const std::string& bot, const std::string& light, const std::string& shade,
                 const std::string& haze = "", float hazeA = 0.5f, bool punte = false) {
        Cresta r; r.seme = seme; r.base = base; r.amp = amp; r.top = top; r.bot = bot; r.light = light; r.shade = shade; r.haze = haze; r.hazeA = hazeA; r.punte = punte; return r;
    };
    auto Righe = [](unsigned seme, float base, float hmin, float hmax, float passo, const std::function<void(float, float, float, unsigned)>& f) {
        Fila(seme, base, hmin, hmax, passo, [&](float x, float b, float h, unsigned sd) { Ripeti(x, h * 0.6f, [&](float xx) { f(xx, b, h, sd); }); });
    };
    auto CieloSemplice = [](std::vector<std::string> fermate, float sx, float sy, float sr, const std::string& sc, bool raggi, bool stelle = false) {
        return [=](K& c) { Cielo ci; ci.fermate = fermate; ci.sole[0] = sx; ci.sole[1] = sy; ci.sole[2] = sr; ci.coloreSole = sc; ci.raggi = raggi; ci.stelle = stelle; DisegnaCielo(c, ci); };
    };
    // 1. Preistoria
    {
        MondoGioco& m = mondi[0]; m.nome = "Preistoria";
        m.scen.nome = "Preistoria"; m.scen.cielo = CieloSemplice({ "#f6a65a", "#f8c87a", "#fbe6b0" }, 1020, 170, 46, "#fff0b0", true);
        m.scen.strati = {
            { 0.06f, [Cr](K& c) { Cresta r = Cr(41, 430, 260, "#b86a52", "#8a4a3a", "rgba(255,210,150,0.4)", "rgba(60,20,20,0.3)", "#f8c87a", 0.55f, true); r.lava = true; DisegnaCresta(c, r); } },
            { 0.18f, [Cr, Righe](K& c) { DisegnaCresta(c, Cr(43, 510, 110, "#7a8a4a", "#4a6a3a", "rgba(220,255,160,0.3)", "rgba(20,40,10,0.25)", "#fbe6b0", 0.4f));
                ColoriAlbero a; a.trunk = "#8a6a3a"; a.dark = "#3a7a2a"; a.mid = "#5a9a3a"; a.light = "#8ac05a";
                Righe(44, 512, 120, 200, 110, [&](float x, float b, float h, unsigned sd) { Palma(c, x, b, h, a, sd); }); } },
            { 0.42f, [Righe](K& c) { RiempiSotto(c, 560, "#3a5a22");
                ColoriAlbero a; a.dark = "#2a5a1a"; a.mid = "#3a7a2a"; a.light = "#6aa83a";
                Righe(46, 566, 110, 180, 150, [&](float x, float b, float h, unsigned sd) { Felce(c, x, b, h, a, sd); }); } } };
        m.scen.primoPiano = "#1a2a0e"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 8;
        m.terra.kind = "grass"; m.terra.body = "#7a4a2a"; m.terra.top = "#6aa83a"; m.terra.blades = { "#5a9a2a", "#7ac04a", "#9ad85a", "#4a8a2a" }; m.terra.flowers = { "#ff6a8a", "#ffd04a" };
        m.blocco.kind = "stone"; m.blocco.body = "#6a5a4a"; m.blocco.top = "#9a8a72"; m.blocco.moss = "#5a8a3a";
        m.legnoPiatt = "#8a6a3a";
        m.e = "Raptor"; m.j = "Raptor saltatore"; m.f = "Pipistrello delle caverne"; m.s = "Lanciasassi"; m.boss = "Re Rex";
    }
    // 2. Antico Egitto
    {
        MondoGioco& m = mondi[1]; m.nome = "Antico Egitto";
        m.scen.nome = "Antico Egitto"; m.scen.cielo = CieloSemplice({ "#4aa8e8", "#9ad4f2", "#f6e6b0" }, 260, 110, 40, "#fffbe0", true);
        m.scen.strati = {
            { 0.05f, [Cr](K& c) { DisegnaCresta(c, Cr(51, 500, 90, "#e8c07a", "#d8a05a", "rgba(255,250,220,0.35)", "rgba(150,90,30,0.25)", "#f6e6b0", 0.5f));
                Piramide(c, 200, 470, 420, "#f2cf8a", "#b8843a"); Piramide(c, 600, 470, 300, "#f2cf8a", "#b8843a"); Piramide(c, 1300, 470, 380, "#f2cf8a", "#b8843a"); } },
            { 0.2f, [Cr, Righe](K& c) { DisegnaCresta(c, Cr(53, 540, 60, "#f0c47a", "#d89a52", "rgba(255,250,220,0.4)", "rgba(150,90,30,0.25)"));
                ColoriAlbero a; a.trunk = "#9a7a4a"; a.dark = "#2a7a3a"; a.mid = "#3a9a4a"; a.light = "#7ac85a";
                Righe(54, 540, 150, 210, 260, [&](float x, float b, float h, unsigned sd) { Palma(c, x, b, h, a, sd); }); } },
            { 0.4f, [](K& c) { for (float x = 100; x < LS; x += 420) { Colonna(c, x, 580, 220, 40, "#e0b878", "#2a7ad8"); Colonna(c, x + 110, 580, 200, 40, "#e0b878", "#c83a3a");
                c.fillStyle(lg(0, 340, 0, 370, { "#f2d498", "#c8964a" })); c.fillRect(x - 14, 350, 178, 22); } } } };
        m.scen.primoPiano = "#5a3a14"; m.scen.tipoPrimoPiano = "rocks"; m.scen.atmosfera = 5;
        m.terra.kind = "sand"; m.terra.body = "#c8924a"; m.terra.top = "#f2d48a";
        m.blocco.kind = "stone"; m.blocco.body = "#b8945a"; m.blocco.top = "#e8c88a";
        m.legnoPiatt = "#c8a46a";
        m.e = "Mummia"; m.j = "Mummia lesta"; m.f = "Pipistrello del deserto"; m.s = "Guardiano d'oro"; m.boss = "Sfinge d'oro";
    }
    // 3. Medioevo
    {
        MondoGioco& m = mondi[2]; m.nome = "Medioevo";
        m.scen.nome = "Medioevo"; m.scen.cielo = CieloSemplice({ "#3a6ab8", "#7aa4d8", "#d8e6f2" }, 1080, 130, 34, "#fffbe8", false);
        m.scen.strati = {
            { 0.04f, [Cr](K& c) { for (int i = 0; i < 7; i++) { Caso r(i + 3); float x = r() * LS, y = 90 + r() * 140, w = 180 + r() * 160; ColoriNuvola n{ "#f4f8ff", "#ffffff", "#b8c8e0" }; Ripeti(x, w, [&](float xx) { Nuvola(c, xx, y, w, n, i); }); }
                DisegnaCresta(c, Cr(61, 460, 140, "#6a8ab0", "#5a7aa0", "rgba(255,255,255,0.2)", "rgba(20,30,60,0.2)", "#d8e6f2", 0.5f)); } },
            { 0.15f, [Cr](K& c) { DisegnaCresta(c, Cr(63, 520, 120, "#6aa04a", "#4a7a3a", "rgba(220,255,180,0.3)", "rgba(20,50,20,0.2)"));
                ColoriCastello k1{ "#8a8aa0", "#3a5ab8", "#f2c23a", "#2a2030", "" }; Castello(c, 560, 470, 260, k1);
                ColoriCastello k2{ "#8a8aa0", "#c83a4a", "#ffffff", "#2a2030", "" }; Castello(c, 1500, 490, 180, k2); } },
            { 0.4f, [Righe](K& c) { RiempiSotto(c, 560, "#3a5a2a");
                ColoriAlbero a; a.trunk = "#5a3a22"; a.dark = "#2a5a2a"; a.mid = "#3a7a32"; a.light = "#6aa84a"; a.fruit = "#e84a4a";
                Righe(66, 566, 200, 300, 200, [&](float x, float b, float h, unsigned sd) { AlberoTondo(c, x, b, h, a, sd); }); } } };
        m.scen.primoPiano = "#1a2a14"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 7;
        m.terra.kind = "grass"; m.terra.body = "#5a4a3a"; m.terra.top = "#5a9a3a"; m.terra.blades = { "#4a8a2a", "#6ab03a", "#8ac84a" }; m.terra.flowers = { "#ffffff", "#ffd04a", "#c87aff" };
        m.blocco.kind = "stone"; m.blocco.body = "#4a4a5a"; m.blocco.top = "#8a8a9c"; m.blocco.moss = "#5a8a3a";
        m.legnoPiatt = "#7a5a3a";
        m.e = "Cavaliere"; m.j = "Scudiero"; m.f = "Pipistrello del castello"; m.s = "Mago della torre"; m.boss = "Cavaliere Nero";
    }
    // 4. Caraibi
    {
        MondoGioco& m = mondi[3]; m.nome = "Caraibi";
        m.scen.nome = "Caraibi"; m.scen.cielo = CieloSemplice({ "#1a8ad8", "#5ab8ea", "#c8ecfa" }, 1040, 120, 36, "#fffbe8", true);
        m.scen.strati = {
            { 0.04f, [](K& c) { for (int i = 0; i < 8; i++) { Caso r(i + 9); float x = r() * LS, y = 80 + r() * 150, w = 160 + r() * 180; ColoriNuvola n{ "#ffffff", "#ffffff", "#c8dcf0" }; Ripeti(x, w, [&](float xx) { Nuvola(c, xx, y, w, n, i + 9); }); }
                c.fillStyle(lg(0, 420, 0, HS, { "#2a8ac8", "#1a5a98" })); c.fillRect(0, 420, LS, HS - 420);
                c.fillStyle(V("rgba(255,255,255,0.35)")); Caso r(5); for (int i = 0; i < 120; i++) { float x = r() * LS, y = 425 + r() * 280; c.fillRect(x, y, 10 + r() * 30, 1.5f); }
                ColoriAlbero t; t.trunk = "#7a5a32"; t.dark = "#2a7a2a"; t.mid = "#4a9a3a"; t.light = "#8ac85a";
                Isola(c, 400, 420, 260, "#c8a46a", "#5ab84a", t, 3); Isola(c, 1400, 425, 180, "#c8a46a", "#5ab84a", t, 4); } },
            { 0.18f, [](K& c) { Nave(c, 700, 470, 1.3f); Nave(c, 1650, 455, 0.7f); } } };
        m.scen.primoPiano = "#2a1a0e"; m.scen.tipoPrimoPiano = "ropes"; m.scen.atmosfera = 0;
        m.terra.kind = "wood"; m.terra.body = "#5a3a1e"; m.terra.top = "#b07a42";
        m.blocco.kind = "wood"; m.blocco.body = "#4a2e18"; m.blocco.top = "#8a5a2e";
        m.legnoPiatt = "#b07a42";
        m.e = "Scheletro pirata"; m.j = "Mozzo scheletro"; m.f = "Gabbiano"; m.s = "Cannoniere"; m.boss = "Capitan Teschio";
    }
    // 5. Far West
    {
        MondoGioco& m = mondi[4]; m.nome = "Far West";
        m.scen.nome = "Far West"; m.scen.cielo = CieloSemplice({ "#e8763a", "#f6a85a", "#fbe0a0" }, 860, 250, 60, "#fff2c0", true);
        m.scen.strati = {
            { 0.05f, [](K& c) { float x = 0; int i = 0; while (x < LS) { Caso r(i++); float w = 220 + r() * 260, h = 120 + r() * 130; Ripeti(x, w, [&](float xx) { Mesa(c, xx, 500, w, h, "#d8784a", "#c0603a", "#8a3e24", "#8a3e24", "#e89a62"); }); x += w * 1.3f; }
                c.fillStyle(lg(0, 300, 0, 520, { "rgba(251,224,160,0)", "rgba(251,224,160,0.5)" })); c.fillRect(0, 300, LS, 220); } },
            { 0.2f, [Cr](K& c) { DisegnaCresta(c, Cr(71, 540, 60, "#d88a4a", "#b8683a", "rgba(255,220,160,0.3)", "rgba(90,30,10,0.2)"));
                c.strokeStyle(V("#5a3a24")); c.lineWidth(4); c.beginPath(); c.moveTo(0, 530); c.lineTo(LS, 530); c.stroke();
                for (float x = 0; x < LS; x += 26) { c.fillStyle(V("#6a4a2a")); c.fillRect(x, 526, 14, 10); } } },
            { 0.4f, [Righe](K& c) { ColoriAlbero a; a.light = "#8ac05a"; a.mid = "#5a9a3a"; a.dark = "#2a5a22";
                Righe(73, 575, 120, 190, 300, [&](float x, float b, float h, unsigned) { Cactus(c, x, b, h, a); }); } } };
        m.scen.primoPiano = "#3a1a0a"; m.scen.tipoPrimoPiano = "rocks"; m.scen.atmosfera = 5;
        m.terra.kind = "sand"; m.terra.body = "#a8582a"; m.terra.top = "#e8a462";
        m.blocco.kind = "wood"; m.blocco.body = "#6a4a2a"; m.blocco.top = "#a87a4a";
        m.legnoPiatt = "#8a5a2e";
        m.e = "Bandito"; m.j = "Bandito lesto"; m.f = "Avvoltoio"; m.s = "Pistolero"; m.boss = "Jack il Lesto";
    }
    // 6. Anno 3000
    {
        MondoGioco& m = mondi[5]; m.nome = "Anno 3000";
        m.scen.nome = "Anno 3000"; m.scen.cielo = CieloSemplice({ "#05051e", "#1a0a40", "#5a1a6a" }, 0, 0, 0, "", false, true);
        m.scen.strati = {
            { 0.05f, [](K& c) { float x = 0; int i = 0; while (x < LS) { Caso r(i++); float w = 40 + r() * 60, h = 200 + r() * 260;
                    ColoriEdificio o; o.wall = "#1a1a46"; o.dark = "#10102e"; o.win = r() > 0.5f ? "#4af0ff" : "#ff4ad8"; o.lit = 0.35f; o.roof = r() > 0.5f ? "spire" : "flat"; o.neon = r() > 0.5f ? "#ff4ad8" : "#4af0ff"; o.ww = 4; o.wh = 6; o.gx = 9; o.gy = 11;
                    Ripeti(x, w, [&](float xx) { Edificio(c, xx, 560, w, h, o, i); }); x += w + 10; }
                c.fillStyle(lg(0, 300, 0, 560, { "rgba(90,26,106,0)", "rgba(160,40,160,0.45)" })); c.fillRect(0, 300, LS, 260); } },
            { 0.22f, [](K& c) { float x = 0; int i = 40; while (x < LS) { Caso r(i++); float w = 90 + r() * 80, h = 150 + r() * 150;
                    ColoriEdificio o; o.wall = "#22225a"; o.dark = "#14143a"; o.win = "#4af0ff"; o.lit = 0.4f; o.roof = "flat"; o.neon = "#ff4ad8"; o.ww = 10; o.wh = 6; o.gx = 18; o.gy = 16; o.frame = "#1a1a40";
                    Ripeti(x, w, [&](float xx) { Edificio(c, xx, 580, w, h, o, i); }); x += w + 70; }
                c.save(); c.glow(20, "#ff4ad8"); c.strokeStyle(V("#ff4ad8")); c.lineWidth(4); c.beginPath(); c.moveTo(0, 300); c.lineTo(LS, 300); c.stroke(); c.restore(); } } };
        m.scen.primoPiano = "#05051a"; m.scen.tipoPrimoPiano = "antennas"; m.scen.atmosfera = 10;
        m.terra.kind = "metal"; m.terra.body = "#14143a"; m.terra.top = "#2e2e5e"; m.terra.stripe1 = "#4af0ff"; m.terra.stripe2 = "#14143a"; m.terra.neon = "#4af0ff";
        m.blocco.kind = "metal"; m.blocco.body = "#1a1a46"; m.blocco.top = "#3a3a6e"; m.blocco.stripe1 = "#ff4ad8"; m.blocco.stripe2 = "#1a1a46";
        m.legnoPiatt = "#3a3a6e";
        m.e = "Robot sentinella"; m.j = "Robot saltellante"; m.f = "Drone"; m.s = "Drone laser"; m.boss = "Dottor Clessidra";
    }

    // --- musica ------------------------------------------------------------------------------------
    const char* DR_ROCK = "K.h.S.h.K.KhS.hh";
    const char* DR_TRIB = "K..hS.K.K.h.S.hK";
    const char* DR_MARC = "K.h.K.h.S.h.K.hh";
    const char* DR_SALT = "K.hK.hS.hK.hS.h.";
    const char* DR_GALL = "K.hhS.hhK.hhS.Sh";
    const char* DR_ELET = "KhhhShhhKhKhShhh";
    p.brani.push_back(CreaBrano("Tema di Nora", 132, 8, {
        { "Melodia", 0, 0.45f, "C5:1 E5:1 G5:2 E5:1 G5:1 C6:2 B5:1 A5:1 G5:2 E5:2 F5:1 A5:1 G5:1 F5:1 E5:2 D5:2 C5:1 E5:1 G5:2 A5:1 G5:1 E5:2 F5:1 E5:1 D5:1 B4:1 C5:4 R:4 C5:1 D5:1 E5:1 G5:1 A5:2 G5:2 F5:1 E5:1 D5:2 C5:4" },
        { "Basso", 1, 0.6f, "C3:2 G2:2 C3:2 G2:2 F2:2 C3:2 G2:2 G2:2 C3:2 G2:2 A2:2 E2:2 F2:2 G2:2 C3:4" },
        { "Batteria", 4, 0.5f, DR_ROCK, true } }));
    p.brani.push_back(CreaBrano("Preistoria", 118, 8, {
        { "Flauto", 1, 0.5f, "A4:2 C5:1 D5:1 E5:2 G5:2 E5:1 D5:1 C5:2 A4:4 G4:2 A4:2 C5:2 D5:1 E5:1 D5:2 C5:2 A4:4 E5:2 G5:2 A5:2 G5:1 E5:1 D5:2 E5:1 D5:1 C5:2 A4:2 G4:2 A4:1 C5:1 A4:6" },
        { "Basso", 1, 0.6f, "A2:3 A2:1 E3:2 A2:2 G2:3 G2:1 D3:2 G2:2 F2:3 F2:1 C3:2 F2:2 E2:3 E2:1 B2:2 E2:2" },
        { "Tamburi", 4, 0.55f, DR_TRIB, true } }));
    p.brani.push_back(CreaBrano("Antico Egitto", 112, 8, {
        { "Oboe", 2, 0.35f, "E5:2 F5:1 G#5:1 A5:2 G#5:1 F5:1 E5:2 F5:1 E5:1 D5:2 E5:2 C5:1 D5:1 E5:2 F5:1 E5:1 D5:1 C5:1 B4:2 C5:1 B4:1 A4:4 E5:1 F5:1 G#5:1 B5:1 A5:2 G#5:1 F5:1 E5:4 F5:1 E5:1 D5:1 C5:1 B4:1 C5:1 B4:1 G#4:1 A4:4" },
        { "Basso", 1, 0.6f, "A2:2 E3:2 A2:2 E3:2 D3:2 A2:2 E2:2 E2:2" },
        { "Darbuka", 4, 0.5f, "K..S..K.S.K.S..h", true } }));
    p.brani.push_back(CreaBrano("Medioevo", 104, 8, {
        { "Liuto", 1, 0.5f, "D5:1 E5:1 F5:2 E5:1 D5:1 C5:2 D5:2 A4:2 R:2 A4:1 C5:1 D5:2 E5:1 F5:1 G5:2 F5:1 E5:1 D5:4 F5:1 G5:1 A5:2 G5:1 F5:1 E5:2 C5:2 D5:1 E5:1 F5:2 E5:1 C5:1 D5:4 R:4" },
        { "Arpeggio", 3, 0.25f, "D4:1 A4:1 D5:1 A4:1 C4:1 G4:1 C5:1 G4:1 Bb3:1 F4:1 Bb4:1 F4:1 A3:1 E4:1 A4:1 E4:1" },
        { "Basso", 1, 0.55f, "D3:4 C3:4 Bb2:4 A2:4" },
        { "Tamburo", 4, 0.45f, DR_MARC, true } }));
    p.brani.push_back(CreaBrano("Caraibi", 140, 8, {
        { "Fisarmonica", 0, 0.4f, "G4:1 C5:1 E5:1 G5:2 E5:1 F5:1 G5:1 A5:2 G5:1 E5:1 C5:2 D5:1 E5:1 F5:2 D5:1 B4:1 G4:2 R:2 G4:1 B4:1 D5:1 F5:2 E5:1 D5:1 C5:1 E5:2 G5:1 E5:1 C5:4 R:2" },
        { "Basso", 1, 0.6f, "C3:1 R:1 G2:1 R:1 C3:1 R:1 G2:1 R:1 G2:1 R:1 D3:1 R:1 G2:1 R:1 D3:1 R:1" },
        { "Batteria", 4, 0.45f, DR_SALT, true } }));
    p.brani.push_back(CreaBrano("Far West", 126, 8, {
        { "Chitarra", 2, 0.32f, "E4:1 G4:1 A4:2 B4:2 D5:1 B4:1 A4:2 G4:1 E4:1 D4:2 E4:4 E4:1 G4:1 A4:2 B4:1 D5:1 E5:2 D5:1 B4:1 A4:1 G4:1 A4:4 R:2 B4:1 A4:1 G4:2 E4:2 D4:1 E4:1 G4:2 A4:2 E4:6" },
        { "Basso", 1, 0.6f, "E2:2 B2:2 E2:2 B2:2 A2:2 E3:2 B2:2 B2:2" },
        { "Galoppo", 4, 0.45f, DR_GALL, true } }));
    p.brani.push_back(CreaBrano("Anno 3000", 138, 8, {
        { "Sintetizzatore", 2, 0.3f, "A4:1 C5:1 E5:1 A5:1 G5:2 E5:2 F5:1 E5:1 D5:1 C5:1 D5:2 E5:2 A4:1 C5:1 E5:1 A5:1 B5:2 C6:2 B5:1 A5:1 G5:1 E5:1 A5:4" },
        { "Arpeggio", 0, 0.2f, "A3:1 C4:1 E4:1 A4:1 F3:1 A3:1 C4:1 F4:1 G3:1 B3:1 D4:1 G4:1 E3:1 G#3:1 B3:1 E4:1", false, 1 },
        { "Basso", 2, 0.45f, "A2:1 A2:1 A3:1 A2:1 F2:1 F2:1 F3:1 F2:1 G2:1 G2:1 G3:1 G2:1 E2:1 E2:1 E3:1 E2:1" },
        { "Batteria", 4, 0.5f, DR_ELET, true } }));
    p.brani.push_back(CreaBrano("Battaglia", 150, 8, {
        { "Allarme", 0, 0.4f, "E5:1 E5:1 G5:1 E5:1 A5:2 G5:1 E5:1 D5:1 E5:1 D5:1 B4:1 C5:2 B4:2 E5:1 E5:1 G5:1 E5:1 B5:2 A5:1 G5:1 F#5:1 G5:1 F#5:1 D5:1 E5:4" },
        { "Basso", 2, 0.45f, "E2:1 E3:1 E2:1 E3:1 C2:1 C3:1 C2:1 C3:1 D2:1 D3:1 D2:1 D3:1 B1:1 B2:1 B1:1 B2:1" },
        { "Batteria", 4, 0.55f, "KhShKhShKKShKhSS", true } }));
    for (int i = 0; i < 6; i++) mondi[i].brano = p.brani[i + 1].nome;

    // --- effetti sonori ----------------------------------------------------------------------------
    p.effetti = {
        Effetto("Salto", 0, 260, 720, 0.16f, 0.4f),
        Effetto("Ingranaggio", 0, 990, 1480, 0.12f, 0.35f),
        Effetto("Cuore", 3, 520, 1040, 0.3f, 0.45f, 0, 0.05f, 9),
        Effetto("Chiave", 1, 700, 1400, 0.25f, 0.4f, 0, 0.04f, 12),
        Effetto("Porta", 2, 160, 90, 0.4f, 0.4f, 0.3f),
        Effetto("Molla", 1, 200, 900, 0.3f, 0.5f, 0, 0.1f, 18),
        Effetto("Controllo", 3, 660, 1320, 0.45f, 0.45f, 0, 0.03f, 6),
        Effetto("Portale", 3, 220, 1760, 0.9f, 0.5f, 0.1f, 0.08f, 7),
        Effetto("Lancio", 4, 900, 400, 0.12f, 0.3f),
        Effetto("Colpo", 4, 300, 80, 0.14f, 0.5f),
        Effetto("Sconfitto", 0, 520, 60, 0.35f, 0.45f, 0.4f),
        Effetto("Ahi", 2, 400, 120, 0.25f, 0.5f, 0.2f),
        Effetto("Sparo nemico", 4, 600, 200, 0.12f, 0.3f),
        Effetto("Boss sconfitto", 2, 880, 40, 1.4f, 0.6f, 0.5f, 0.1f, 6),
    };
    p.suoniEventi = { { "salto", "Salto" }, { "moneta", "Ingranaggio" }, { "colpo", "Colpo" }, { "molla", "Molla" } };

    {
        const char* frasi[6] = {
            "Preistoria! Frecce: corri. Spazio: salta (di nuovo in aria: doppio salto). X: lancia la chiave inglese.",
            "Epoca 2: l'Antico Egitto. Attenti alle mummie e ai guardiani d'oro!",
            "Epoca 3: il Medioevo. Il castello del Cavaliere Nero vi aspetta.",
            "Epoca 4: i Caraibi. Ciurma di scheletri all'orizzonte!",
            "Epoca 5: il Far West. Jack il Lesto e' il pistolero piu' veloce del tempo.",
            "Epoca 6: l'Anno 3000. Il laboratorio del Dottor Clessidra!" };
        for (int w = 0; w < 6; w++) mondi[w].frase = frasi[w];
    }
    NomiGioco n;
    n.moneta = "Ingranaggio"; n.cuore = "Cuore"; n.molla = "Molla"; n.controllo = "Lampione del tempo"; n.chiave = "Chiave d'ottone";
    n.porta = "Porta di bronzo"; n.cancello = "Cancello"; n.arrivo = "Portale del tempo";
    n.branoBoss = "Battaglia"; n.branoVittoria = "Tema di Nora";
    n.fraseBoss = "Il guardiano e' sconfitto! Il portale del tempo e' aperto.";
    n.fraseFinale = "Il Dottor Clessidra e' sconfitto! La Macchina del Tempo e' salva!";
    n.energiaBoss = 10; n.seme = 2;
    CostruisciMondi(p, resa, mondi, n);
    return p;
}

}  // namespace piatt
