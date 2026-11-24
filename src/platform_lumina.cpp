// Gioco A: "Lumina: le Lanterne del Mondo Spento".
// Il Re Eclisse ha spento tutte le luci del mondo. Lia, con la sua lanterna, e la lucciola Pip
// riaccendono le lanterne di sei regni immersi nel buio. Grafica disegnata HD, liscia.
#include "platform_tre.h"
#include <cmath>

namespace piatt {

using namespace arte;

namespace {

const float TAU_ = 6.28318530718f;
void ell(K& c, float x, float y, float rx, float ry, float rot = 0) { c.beginPath(); c.ellipse(x, y, std::max(0.05f, rx), std::max(0.05f, ry), rot, 0, TAU_); }

Umanoide Lia() {
    Umanoide u; u.skin = "#f6cfa8"; u.hair = "#e0542e"; u.hairStyle = "bob"; u.eye = "#2a9a8a"; u.lash = true; u.top = "#2f8f86";
    u.bottom = "#3a2e52"; u.shoes = "#5a3a24"; u.cloak = "#8a2a3a"; u.belt = "#5a3a24"; u.collar = "#f2e6c8"; u.item = "lantern";
    return u;
}
Umanoide CavaliereEclissi() {
    Umanoide u; u.skin = "#cccccc"; u.helmet = "#3a2a52"; u.plume = "#a04aff"; u.top = "#2a1e3e"; u.bottom = "#1e1630"; u.shoes = "#140e20";
    u.belt = "#5a3a7a"; u.item = "sword"; u.glow = "#ff3a8a"; return u;
}
Umanoide ReEclisse() {
    Umanoide u; u.skin = "#cccccc"; u.helmet = "#241838"; u.top = "#1e1430"; u.bottom = "#140e22"; u.shoes = "#0e0a16"; u.cloak = "#5a1a8a";
    u.crown = true; u.item = "sword"; u.glow = "#ff5ad8"; u.belt = "#c8a03a"; return u;
}
Umanoide MagoEclissi() {
    Umanoide u; u.skin = "#d8c8e8"; u.hair = "#c8b8f0"; u.eye = "#ff5ad8"; u.brows = true; u.top = "#3a1a5a"; u.robe = true; u.trim = "#a04aff";
    u.hat = "wizard"; u.hatc = "#2a1046"; u.beard = true; u.item = "staff"; u.orb = "#ff5ad8"; return u;
}
DisegnaPersonaggio Uman(Umanoide u, float s = 1) { return [u, s](K& c, float x, float y, float k, const Posa& p) { DisegnaUmanoide(c, x, y, k * s, u, p); }; }

void Cuore(K& c, float x, float y, float s, const std::string& col) {
    c.save(); c.translate(x, y); c.scale(s, s);
    c.beginPath(); c.moveTo(0, 3); c.bezierCurveTo(-1, 2, -6, -1, -6, -4); c.bezierCurveTo(-6, -7, -2, -8, 0, -5);
    c.bezierCurveTo(2, -8, 6, -7, 6, -4); c.bezierCurveTo(6, -1, 1, 2, 0, 3); c.closePath();
    c.fillStyle(rg(-2, -5, 0, 8, { sh(col, 0.5f), col, sh(col, -0.4f) })); c.fill();
    c.strokeStyle(V(sh(col, -0.6f))); c.lineWidth(0.5f); c.stroke();
    ell(c, -3, -5, 1.4f, 0.9f, -0.5f); c.fillStyle(V("rgba(255,255,255,0.8)")); c.fill();
    c.restore();
}

// Una lanterna su un palo: spenta (vetro scuro) o accesa (fiamma e alone).
void Lanterna(K& c, float h, bool accesa, float t, float scala = 1) {
    c.save(); c.scale(scala, scala);
    c.fillStyle(lg(-1.2f, 0, 1.2f, 0, { "#5a4a3a", "#2a1e14" })); c.fillRect(-1, -h + 8, 2, h - 8);
    c.fillStyle(V("#3a2a1e")); c.fillRect(-4, -1.6f, 8, 1.6f);
    c.save(); c.translate(0, -h);
    if (accesa) {
        c.save(); c.lighter();
        c.fillStyle(rg(0, 4, 0, 16 + std::sin(t) * 1.2f, { "rgba(255,210,110,0.55)", "rgba(255,170,60,0.18)", "rgba(255,170,60,0)" })); c.fillRect(-18, -14, 36, 36);
        c.restore();
    }
    c.fillStyle(V("#2a1e14")); c.beginPath(); c.moveTo(-5, 0); c.lineTo(0, -4); c.lineTo(5, 0); c.closePath(); c.fill();
    c.fillRect(-4.5f, 0, 9, 1.2f); c.fillRect(-4.5f, 9.4f, 9, 1.4f);
    c.fillStyle(V(accesa ? "rgba(255,236,170,0.95)" : "rgba(60,70,100,0.85)")); c.fillRect(-3.8f, 1.2f, 7.6f, 8.2f);
    if (accesa) {
        c.save(); c.glow(4, "#ffb040");
        c.beginPath(); c.moveTo(0, 2.5f + std::sin(t * 1.7f) * 0.4f); c.quadraticCurveTo(2.4f, 6, 0, 8.4f); c.quadraticCurveTo(-2.4f, 6, 0, 2.5f); c.closePath();
        c.fillStyle(lg(0, 2, 0, 9, { "#fffbe0", "#ffd04a", "#ff8a2a" })); c.fill(); c.restore();
    }
    c.strokeStyle(V("#2a1e14")); c.lineWidth(0.6f); c.beginPath(); c.moveTo(0, 1.2f); c.lineTo(0, 9.4f); c.stroke();
    c.restore();
    c.restore();
}

}  // namespace

ProgettoGioco CreaLumina() {
    Resa resa; resa.hd = true; resa.r = 0.75f; resa.dens = 3;
    ProgettoGioco p = NuovoProgetto(Genere::Platform, StileGrafico::Disegnato, "Lumina: le Lanterne del Mondo Spento");
    p.personaggi.clear(); p.oggetti.clear(); p.scenari.clear(); p.caselle.clear(); p.brani.clear(); p.effetti.clear(); p.regole.clear();
    p.variabili.clear(); p.immagini.clear(); p.mondo.mappe.clear();
    p.mondo.titolo = "Lumina: le Lanterne del Mondo Spento";
    p.mondo.autori = "Mario Petramale - Elvantis";
    p.mondo.giocatoriMax = 2;
    p.mondo.fisica.gravita = 880; p.mondo.fisica.salto = 310; p.mondo.fisica.velocita = 120; p.mondo.fisica.molla = 520;
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
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = true; o.largo = largo; o.sopra = sopra;
        o.animazioni = { Anim::Fermo, Anim::Corsa, Anim::Attacco, Anim::Colpito }; o.fotogrammiCorsa = 6; o.fotogrammiFermo = 4;
        return CreaDisegno(f, o);
    };
    auto Volante = [&](const DisegnaPersonaggio& f, float alt, float largo = 190) {
        OpzioniDisegno o; o.altezzaMondo = alt; o.dens = resa.dens; o.liscio = true; o.largo = largo;
        o.animazioni = { Anim::Fermo, Anim::Colpito }; o.fotogrammiFermo = 6;
        return CreaDisegno(f, o);
    };
    auto Ombra = [](const std::string& col, const std::string& aura, const std::string& occhi, float s = 1) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaOmbra(c, x, y - 40 * k * s, k * s, col, aura, occhi, q); };
    };
    auto Golem = [](const std::string& a, const std::string& b, const std::string& cc, const std::string& l, float s = 1) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaGolem(c, x, y, k * s, a, b, cc, l, q); };
    };
    auto Pipistrello = [](const std::string& ali, const std::string& corpo, const std::string& occhi) {
        return [=](K& c, float x, float y, float k, const Posa& q) { DisegnaPipistrello(c, x, y - 70 * k, k, ali, corpo, occhi, q); };
    };

    // --- eroi --------------------------------------------------------------------------------------------
    Personaggio("Lia", CreaPersonaggio(Uman(Lia()), 30, resa), TipoComportamento::Giocatore, 120, 4, 0, "Raggio di luce", 310);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    Personaggio("Pip", CreaPersonaggio([](K& c, float x, float y, float s, const Posa& q) { DisegnaLucciola(c, x, y - 60 * s, s * 1.3f, q); }, 26, resa, 175, 150),
                TipoComportamento::Giocatore, 125, 4, 0, "Raggio di luce", 320);
    p.personaggi.back().suoni = { { "salto", "Salto" }, { "colpito", "Ahi" } };
    p.eroe = "Lia"; p.eroe2 = "Pip";

    // --- nemici dei sei regni --------------------------------------------------------------------------
    // 1 Bosco delle Lucciole
    Personaggio("Ombrino", Nemico(Ombra("#3a2260", "#a04aff", "#ff5a9a"), 22), TipoComportamento::Pattuglia, 35, 1, 1);
    Personaggio("Ombrino saltellante", Nemico(Ombra("#4a2a70", "#ff5ad8", "#ffd04a", 0.85f), 20), TipoComportamento::Insegue, 55, 1, 1, "", 230, 120);
    Personaggio("Pipistrello d'inchiostro", Volante(Pipistrello("#4a2e78", "#2e1e4a", "#ff4a6a"), 22), TipoComportamento::Vola, 45, 1, 1, "", 0, 140, 2, false);
    Personaggio("Ombra lanciatrice", Nemico(Ombra("#2a1a4a", "#7a4aff", "#ff3a6a", 1.1f), 26), TipoComportamento::Spara, 0, 2, 1, "Goccia d'ombra", 0, 170, 2.2f);
    Personaggio("Grande Ombra", Nemico(Ombra("#2a1450", "#c04aff", "#ff3a8a", 1.0f), 56), TipoComportamento::Boss, 50, 10, 1, "Goccia d'ombra", 0, 260, 1.6f, false, 2000);
    // 2 Miniere di Cristallo
    Personaggio("Golem di quarzo", Nemico(Golem("#c8b8ff", "#8a74e8", "#4a3aa0", "#9ff8ff", 0.75f), 26, 170), TipoComportamento::Pattuglia, 28, 2, 1);
    Personaggio("Scheggia", Nemico(Golem("#e8d8ff", "#b8a4ff", "#6a5ac0", "#ff8ad8", 0.6f), 18, 170), TipoComportamento::Insegue, 60, 1, 1, "", 240, 120);
    Personaggio("Pipistrello di cristallo", Volante(Pipistrello("#5a4ab8", "#2e2a6a", "#9ff8ff"), 22), TipoComportamento::Vola, 50, 1, 1, "", 0, 140, 2, false);
    Personaggio("Golem cannone", Nemico(Golem("#a8c8ff", "#6a8ae8", "#3a4aa0", "#ffe14a", 0.8f), 28, 170), TipoComportamento::Spara, 0, 2, 1, "Scheggia di cristallo", 0, 180, 2);
    Personaggio("Re di Quarzo", Nemico(Golem("#d8c8ff", "#9a84f8", "#5a4ab0", "#9ff8ff"), 60, 170), TipoComportamento::Boss, 45, 12, 1, "Scheggia di cristallo", 0, 260, 1.5f, true, 2500);
    // 3 Citta' dei Tetti
    Personaggio("Ombra dei comignoli", Nemico(Ombra("#2a2a3a", "#ffd04a", "#ffe14a"), 22), TipoComportamento::Pattuglia, 40, 1, 1);
    { Umanoide u; u.skin = "#3a3a4a"; u.hair = "#1a1a24"; u.hairStyle = "spiky"; u.eye = "#ffe14a"; u.top = "#2a2a36"; u.bottom = "#1e1e28"; u.shoes = "#101018"; u.hat = "cowboy"; u.hatc = "#18181f"; u.scarf = "#5a5a6a";
      Personaggio("Spazzacamino ombra", Nemico(Uman(u), 26), TipoComportamento::Insegue, 62, 1, 1, "", 240, 130); }
    Personaggio("Pipistrello dei tetti", Volante(Pipistrello("#2a2a3a", "#18182a", "#ffcc33"), 22), TipoComportamento::Vola, 50, 1, 1, "", 0, 140, 2, false);
    { Umanoide u = MagoEclissi(); u.top = "#2a2a44"; u.hatc = "#1a1a2e"; u.orb = "#ffd04a"; u.trim = "#ffd04a";
      Personaggio("Lampionaio oscuro", Nemico(Uman(u), 28), TipoComportamento::Spara, 0, 2, 1, "Goccia d'ombra", 0, 180, 2); }
    Personaggio("Signore dei Lampioni", Nemico(Ombra("#1e1e2e", "#ffd04a", "#ffe14a"), 56), TipoComportamento::Boss, 55, 12, 1, "Goccia d'ombra", 0, 260, 1.4f, false, 2500);
    // 4 Ghiacciaio dell'Aurora
    Personaggio("Golem di ghiaccio", Nemico(Golem("#f2fbff", "#a8d4ea", "#5a8ab0", "#5af0ff", 0.75f), 26, 170), TipoComportamento::Pattuglia, 30, 2, 1);
    Personaggio("Spirito della neve", Nemico(Ombra("#6aa8d0", "#bff4ff", "#ffffff", 0.9f), 20), TipoComportamento::Insegue, 60, 1, 1, "", 220, 130);
    Personaggio("Pipistrello di brina", Volante(Pipistrello("#a8d4ea", "#6a9ac0", "#ffffff"), 22), TipoComportamento::Vola, 50, 1, 1, "", 0, 140, 2, false);
    Personaggio("Golem di brina", Nemico(Golem("#ffffff", "#c8e8f8", "#7aa8c8", "#5af0ff", 0.8f), 28, 170), TipoComportamento::Spara, 0, 2, 1, "Scheggia di ghiaccio", 0, 180, 1.9f);
    Personaggio("Colosso di ghiaccio", Nemico(Golem("#f2fbff", "#a8d4ea", "#5a8ab0", "#5af0ff"), 62, 170), TipoComportamento::Boss, 45, 13, 1, "Scheggia di ghiaccio", 0, 260, 1.4f, true, 2500);
    // 5 Fonderia dei Vapori
    { Robot r; r.metal = "#a07a4a"; r.core = "#ffb03a"; r.eye = "#ff4a2a"; r.ant = "#ffb03a"; r.fist = "#6a4a2a";
      Personaggio("Automa di rame", Nemico([r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }, 28), TipoComportamento::Pattuglia, 42, 2, 1); }
    { Robot r; r.metal = "#c89a5a"; r.core = "#ffe14a"; r.eye = "#ffe14a"; r.ant = "#ff6a3a"; r.fist = "#8a5a2a";
      Personaggio("Automa saltellante", Nemico([r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }, 22), TipoComportamento::Insegue, 60, 1, 1, "", 250, 130); }
    Personaggio("Ombra di fuliggine", Volante(Ombra("#c84a1a", "#ffb03a", "#fff6a0", 0.8f), 22), TipoComportamento::Vola, 50, 1, 1, "", 0, 140, 2, false);
    { Robot r; r.metal = "#6a5a4a"; r.core = "#ff6a2a"; r.eye = "#ff6a2a"; r.ant = "#ffb03a"; r.fist = "#4a3a2a";
      Personaggio("Cannone a vapore", Nemico([r](K& c, float x, float y, float s, const Posa& q) { DisegnaRobot(c, x, y, s, r, q); }, 28), TipoComportamento::Spara, 0, 2, 1, "Tizzone", 0, 190, 1.8f); }
    Personaggio("Caldaia Gigante", Nemico([](K& c, float x, float y, float s, const Posa& q) { DisegnaMech(c, x, y, s, "#a8542a", q); }, 64, 200),
                TipoComportamento::Boss, 55, 14, 1, "Tizzone", 0, 280, 1.3f, true, 3000);
    // 6 Torre dell'Eclissi
    Personaggio("Cavaliere dell'Eclissi", Nemico(Uman(CavaliereEclissi()), 30), TipoComportamento::Pattuglia, 40, 2, 1);
    Personaggio("Ombrino viola", Nemico(Ombra("#5a2a8a", "#ff5ad8", "#ffffff", 0.85f), 20), TipoComportamento::Insegue, 65, 1, 1, "", 250, 130);
    Personaggio("Pipistrello dell'Eclissi", Volante(Pipistrello("#5a1a8a", "#2a0e40", "#ff5ad8"), 22), TipoComportamento::Vola, 55, 1, 1, "", 0, 140, 2, false);
    Personaggio("Mago dell'Eclissi", Nemico(Uman(MagoEclissi()), 30), TipoComportamento::Spara, 0, 2, 1, "Goccia d'ombra", 0, 200, 1.8f);
    Personaggio("Re Eclisse", Nemico(Uman(ReEclisse()), 64), TipoComportamento::Boss, 60, 18, 2, "Goccia d'ombra", 0, 300, 1.1f, true, 5000);

    // --- oggetti -----------------------------------------------------------------------------------------
    auto Oggetto = [&](const std::string& nome, FunzioneOggetto f, Disegno d, int valore = 10, const std::string& suono = "") {
        OggettoDef o; o.nome = nome; o.autore = "Elvantis"; o.funzione = f; o.disegno = std::move(d); o.valore = valore; o.suono = suono;
        p.oggetti.push_back(o);
        return &p.oggetti.back();
    };
    Oggetto("Scintilla", FunzioneOggetto::Raccogli, CreaOggettoDisegno(14, 16, resa, { { "fermo", 8, 10, true } }, [](K& c, const std::string&, int i, int n) {
        float a = i * TAU_ / n, b = 0.8f + 0.2f * std::sin(a * 2);
        c.save(); c.translate(0, -8 + std::sin(a) * 1.2f);
        c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 7.5f, { "rgba(255,230,140,0.65)", "rgba(255,200,90,0.2)", "rgba(255,200,90,0)" })); c.fillRect(-8, -8, 16, 16); c.restore();
        c.rotate(a * 0.5f);
        c.beginPath();
        for (int k = 0; k < 8; k++) { float r = k % 2 ? 1.6f : 4.6f * b; float an = k * TAU_ / 8; if (k == 0) c.moveTo(std::cos(an) * r, std::sin(an) * r); else c.lineTo(std::cos(an) * r, std::sin(an) * r); }
        c.closePath(); c.fillStyle(rg(0, 0, 0, 4.6f, { "#ffffff", "#fff3a0", "#ffc83a" })); c.fill();
        c.restore();
    }), 50, "Scintilla")->variabile = "monete";
    Oggetto("Cuore", FunzioneOggetto::Cura, CreaOggettoDisegno(14, 14, resa, { { "fermo", 4, 6, true } }, [](K& c, const std::string&, int i, int) {
        float s = 0.92f + 0.08f * std::sin(i * TAU_ / 4);
        c.save(); c.lighter(); c.fillStyle(rg(0, -7, 0, 7, { "rgba(255,90,130,0.5)", "rgba(255,90,130,0)" })); c.fillRect(-7, -14, 14, 14); c.restore();
        Cuore(c, 0, -7, s, "#ff4a6a");
    }), 1, "Cuore");
    Oggetto("Chiave di luce", FunzioneOggetto::Chiave, CreaOggettoDisegno(14, 14, resa, { { "fermo", 6, 8, true } }, [](K& c, const std::string&, int i, int n) {
        c.save(); c.translate(0, -7 + std::sin(i * TAU_ / n) * 0.8f); c.rotate(-0.6f);
        c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 7, { "rgba(160,240,255,0.55)", "rgba(160,240,255,0)" })); c.fillRect(-7, -7, 14, 14); c.restore();
        c.fillStyle(lg(-5, -5, 5, 5, { "#ffffff", "#9ff8ff", "#3a8ad8" }));
        ell(c, -3, 0, 2.6f, 2.6f); c.fill(); c.fillRect(-1, -0.8f, 7, 1.6f); c.fillRect(3.4f, 0.8f, 1.2f, 1.8f); c.fillRect(5, 0.8f, 1.2f, 1.4f);
        ell(c, -3, 0, 1.1f, 1.1f); c.fillStyle(V("#1a2a4a")); c.fill();
        c.restore();
    }), 100, "Chiave");
    { OggettoDef* o = Oggetto("Porta d'ombra", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 4, 5, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int i, int) {
        c.fillStyle(lg(-8, -32, 8, 0, { "#3a2a52", "#1a1028" })); c.fillRect(-8, -32, 16, 32);
        if (a == "chiusa") {
            c.fillStyle(lg(-6, -30, 6, 0, { "#2a1a40", "#140a22" })); c.fillRect(-6, -30, 12, 30);
            c.save(); c.lighter(); c.strokeStyle(V("rgba(190,120,255,0.8)")); c.lineWidth(0.6f);
            for (int k = 0; k < 3; k++) { c.beginPath(); float y0 = -26 + k * 9 + i; c.moveTo(-5, y0); c.quadraticCurveTo(0, y0 - 3, 5, y0); c.stroke(); }
            c.restore();
            c.save(); c.glow(4, "#9ff8ff"); ell(c, 0, -15, 1.8f, 2.4f); c.fillStyle(V("#9ff8ff")); c.fill(); c.restore();
        } else { c.fillStyle(V("#0a0612")); c.fillRect(-6, -30, 12, 30); }
    }), 0, "Porta"); o->richiede = "Chiave di luce"; o->solido = true; }
    { OggettoDef* o = Oggetto("Sigillo d'ombra", FunzioneOggetto::Porta, CreaOggettoDisegno(16, 32, resa, { { "chiusa", 4, 6, true }, { "aperta", 1, 1, true } }, [](K& c, const std::string& a, int i, int) {
        if (a != "chiusa") return;
        c.save(); c.lighter();
        c.fillStyle(lg(0, -32, 0, 0, { "rgba(160,60,255,0)", "rgba(160,60,255,0.55)", "rgba(255,60,160,0.35)" })); c.fillRect(-7, -32, 14, 32);
        c.strokeStyle(V("rgba(255,150,240,0.9)")); c.lineWidth(0.7f);
        for (int k = 0; k < 4; k++) { c.beginPath(); c.moveTo(-6 + k * 4, 0); for (float y = 0; y > -32; y -= 4) c.lineTo(-6 + k * 4 + std::sin(y * 0.4f + i * 1.5f + k) * 1.5f, y); c.stroke(); }
        c.restore();
    }), 0, "Porta"); o->richiede = "sconfiggere il guardiano"; o->solido = true; }
    Oggetto("Fiore molla", FunzioneOggetto::Molla, CreaOggettoDisegno(16, 16, resa, { { "fermo", 1, 1, true }, { "scatta", 3, 12, false } }, [](K& c, const std::string& a, int i, int) {
        float h = a == "scatta" ? (i == 0 ? 3 : i == 1 ? 10 : 7) : 5;
        c.strokeStyle(V("#2e8a5a")); c.lineWidth(1.4f); c.beginPath(); c.moveTo(0, 0); c.quadraticCurveTo(2, -h * 0.5f, 0, -h); c.stroke();
        c.fillStyle(V("#3aa86a")); ell(c, -3, -1.5f, 3, 1.2f, 0.3f); c.fill(); ell(c, 3, -1.5f, 3, 1.2f, -0.3f); c.fill();
        c.save(); c.translate(0, -h - 2);
        c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, 8, { "rgba(255,140,220,0.45)", "rgba(255,140,220,0)" })); c.fillRect(-8, -8, 16, 16); c.restore();
        for (int k = 0; k < 6; k++) { c.save(); c.rotate(k * TAU_ / 6); ell(c, 0, -3, 2, 3.4f); c.fillStyle(lg(0, -6, 0, 0, { "#ffd8f8", "#e85ac8" })); c.fill(); c.restore(); }
        ell(c, 0, 0, 1.8f, 1.8f); c.fillStyle(V("#ffe14a")); c.fill();
        c.restore();
    }), 520, "Molla");
    Oggetto("Lanterna", FunzioneOggetto::Controllo, CreaOggettoDisegno(18, 34, resa, { { "fermo", 1, 1, true }, { "accesa", 6, 8, true } }, [](K& c, const std::string& a, int i, int n) {
        Lanterna(c, 26, a == "accesa", i * TAU_ / n);
    }), 0, "Lanterna");
    Oggetto("Grande Lanterna", FunzioneOggetto::Arrivo, CreaOggettoDisegno(40, 52, resa, { { "fermo", 8, 8, true } }, [](K& c, const std::string&, int i, int n) {
        float t = i * TAU_ / n;
        c.save(); c.lighter(); c.fillStyle(rg(0, -38, 0, 22 + std::sin(t) * 2, { "rgba(255,220,130,0.6)", "rgba(255,170,70,0.2)", "rgba(255,170,70,0)" })); c.fillRect(-26, -62, 52, 52); c.restore();
        c.fillStyle(lg(-10, 0, 10, 0, { "#6a5a4a", "#2a1e14" })); c.fillRect(-9, -4, 18, 4); c.fillRect(-2, -26, 4, 24);
        c.save(); c.translate(0, -48); c.scale(1.7f, 1.7f); c.translate(0, 0);
        c.fillStyle(V("#2a1e14")); c.beginPath(); c.moveTo(-5, 0); c.lineTo(0, -4); c.lineTo(5, 0); c.closePath(); c.fill();
        c.fillRect(-4.5f, 0, 9, 1.2f); c.fillRect(-4.5f, 9.4f, 9, 1.4f);
        c.fillStyle(V("rgba(255,240,190,0.95)")); c.fillRect(-3.8f, 1.2f, 7.6f, 8.2f);
        c.save(); c.glow(5, "#ffb040"); c.beginPath(); c.moveTo(0, 2 + std::sin(t * 2) * 0.5f); c.quadraticCurveTo(2.6f, 6, 0, 8.6f); c.quadraticCurveTo(-2.6f, 6, 0, 2); c.closePath();
        c.fillStyle(lg(0, 2, 0, 9, { "#ffffff", "#ffd04a", "#ff7a2a" })); c.fill(); c.restore();
        c.restore();
        for (int k = 0; k < 5; k++) { float an = t + k * TAU_ / 5; c.save(); c.lighter(); ell(c, std::cos(an) * 14, -40 + std::sin(an) * 6, 1.1f, 1.1f); c.fillStyle(V("rgba(255,240,160,0.9)")); c.fill(); c.restore(); }
    }), 1000, "Portale");
    auto Sfera = [&](const std::string& nome, const std::string& col, const std::string& luce, float r, int danno, float vel) {
        Oggetto(nome, FunzioneOggetto::Proiettile, CreaOggettoDisegno(r * 3, r * 3, resa, { { "fermo", 2, 12, true } }, [col, luce, r](K& c, const std::string&, int i, int) {
            c.save(); c.translate(0, -r * 1.5f);
            c.save(); c.lighter(); c.fillStyle(rg(0, 0, 0, r * 1.5f, { rgba(luce, 0.7f), rgba(luce, 0) })); c.fillRect(-r * 1.5f, -r * 1.5f, r * 3, r * 3); c.restore();
            ell(c, 0, 0, r * (i ? 0.85f : 0.75f), r * (i ? 0.75f : 0.85f)); c.fillStyle(rg(-r * 0.3f, -r * 0.3f, 0, r, { "#ffffff", luce, col })); c.fill();
            c.restore();
        }), danno)->velocita = vel;
    };
    Sfera("Raggio di luce", "#ffb03a", "#fff3a0", 3, 1, 180);
    Sfera("Goccia d'ombra", "#2a0e40", "#c04aff", 3.2f, 1, 105);
    Sfera("Scheggia di cristallo", "#4a3aa0", "#c8b8ff", 2.8f, 1, 120);
    Sfera("Scheggia di ghiaccio", "#3a7ab0", "#dff8ff", 2.8f, 1, 120);
    Sfera("Tizzone", "#a8301a", "#ffb03a", 3, 1, 115);

    // --- i sei regni ------------------------------------------------------------------------------------
    std::vector<MondoGioco> mondi(6);
    auto Cr = [](unsigned seme, float base, float amp, const std::string& top, const std::string& bot, const std::string& light, const std::string& shade,
                 const std::string& haze = "", float hazeA = 0.5f, bool punte = false) {
        Cresta r; r.seme = seme; r.base = base; r.amp = amp; r.top = top; r.bot = bot; r.light = light; r.shade = shade; r.haze = haze; r.hazeA = hazeA; r.punte = punte; return r;
    };
    auto Righe = [](unsigned seme, float base, float hmin, float hmax, float passo, const std::function<void(float, float, float, unsigned)>& f) {
        Fila(seme, base, hmin, hmax, passo, [&](float x, float b, float h, unsigned sd) { Ripeti(x, h * 0.6f, [&](float xx) { f(xx, b, h, sd); }); });
    };
    // 1. Bosco delle Lucciole
    {
        MondoGioco& m = mondi[0]; m.nome = "Bosco delle Lucciole";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#050b1e", "#0e2244", "#1d4a64" }; ci.luna[0] = 980; ci.luna[1] = 120; ci.luna[2] = 38; ci.coloreLuna = "#d8ecff"; ci.stelle = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.08f, [Cr](K& c) { DisegnaCresta(c, Cr(3, 430, 180, "#1e3e62", "#14283e", "rgba(120,170,220,0.25)", "rgba(0,0,20,0.25)", "#1d4a64", 0.6f)); } },
            { 0.2f, [Righe](K& c) { RiempiSotto(c, 470, "#10243a");
                ColoriAlbero a; a.trunk = "#0a1628"; a.dark = "#0f2238"; a.light = "#1a3a58"; a.rim = "rgba(150,200,255,0.35)";
                Righe(4, 478, 120, 220, 46, [&](float x, float b, float h, unsigned) { Pino(c, x, b, h, a); });
                c.fillStyle(lg(0, 380, 0, 520, { "rgba(40,90,120,0)", "rgba(40,90,120,0.55)" })); c.fillRect(0, 380, LS, 140); } },
            { 0.45f, [Righe](K& c) { RiempiSotto(c, 540, "#0a1a24");
                ColoriAlbero a; a.trunk = "#14202a"; a.dark = "#0c1e26"; a.mid = "#123040"; a.light = "#1e4a5a"; a.spec = "rgba(170,230,255,0.35)";
                Righe(9, 548, 260, 380, 170, [&](float x, float b, float h, unsigned sd) { AlberoTondo(c, x, b, h, a, sd); }); } } };
        m.scen.primoPiano = "#050a10"; m.scen.tipoPrimoPiano = "leaves"; m.scen.atmosfera = 3;
        m.buio = 0.72f;
        m.terra.kind = "nightgrass"; m.terra.body = "#2a2a3a"; m.terra.top = "#2e6a54"; m.terra.blades = { "#2a5a48", "#3a7a5e", "#4a9a74", "#5ab88a" }; m.terra.flowers = { "#9ad8ff", "#e8c0ff" };
        m.blocco.kind = "stone"; m.blocco.body = "#2a3040"; m.blocco.top = "#4a5468"; m.blocco.moss = "#2e6a54";
        m.legnoPiatt = "#4a3a2a"; m.luceMobile = "#c8ff6a";
        m.e = "Ombrino"; m.j = "Ombrino saltellante"; m.f = "Pipistrello d'inchiostro"; m.s = "Ombra lanciatrice"; m.boss = "Grande Ombra";
        m.frase = "Il Bosco delle Lucciole e' al buio. Accendi le lanterne toccandole! Spazio salta (due volte), X lancia la luce.";
    }
    // 2. Miniere di Cristallo
    {
        MondoGioco& m = mondi[1]; m.nome = "Miniere di Cristallo";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#08061a", "#1a1236", "#2a1d4a" }; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.06f, [Cr](K& c) { Cresta s = Cr(5, 640, 220, "#2a1e4a", "#120c24", "", ""); s.soffitto = true; DisegnaCresta(c, s);
                DisegnaCresta(c, Cr(7, 460, 120, "#2a2050", "#161030", "rgba(160,140,255,0.22)", "rgba(0,0,0,0.3)", "#2a1d4a")); } },
            { 0.22f, [Righe](K& c) { RiempiSotto(c, 500, "#1c1638");
                ColoriCristallo k{ "#3a2a8a", "#6a54d8", "#c8b8ff", "#8a6aff" };
                Righe(12, 505, 90, 200, 150, [&](float x, float b, float h, unsigned sd) { Cristalli(c, x, b, h, k, sd); }); } },
            { 0.4f, [](K& c) { for (int i = 0; i < 14; i++) { Caso r(i + 40); float x = r() * LS, h = 60 + r() * 140;
                    Ripeti(x, 40, [&](float xx) { c.fillStyle(lg(0, 0, 0, h, { "#120c24", "#2a2050" })); c.beginPath(); c.moveTo(xx - 26, 0); c.lineTo(xx, h); c.lineTo(xx + 26, 0); c.fill(); }); }
                c.strokeStyle(V("#5a3a2a")); c.lineWidth(10);
                for (float x = 80; x < LS; x += 420) { c.beginPath(); c.moveTo(x, 600); c.lineTo(x, 330); c.lineTo(x + 200, 330); c.lineTo(x + 200, 600); c.stroke(); } } } };
        m.scen.primoPiano = "#06040e"; m.scen.tipoPrimoPiano = "rocks"; m.scen.atmosfera = 10;
        m.buio = 0.62f;
        m.terra.kind = "crystalfloor"; m.terra.body = "#2a2248"; m.terra.top = "#5a4a90"; m.terra.glow = "#9ff8ff";
        m.blocco.kind = "stone"; m.blocco.body = "#2a2248"; m.blocco.top = "#5a4a90";
        m.legnoPiatt = "#6a4a2e"; m.luceMobile = "#9ff8ff";
        m.e = "Golem di quarzo"; m.j = "Scheggia"; m.f = "Pipistrello di cristallo"; m.s = "Golem cannone"; m.boss = "Re di Quarzo";
        m.frase = "Le Miniere di Cristallo: i cristalli brillano ancora un poco. Segui le scintille!";
    }
    // 3. Citta' dei Tetti
    {
        MondoGioco& m = mondi[2]; m.nome = "Citta' dei Tetti";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#0c0e22", "#23264a", "#4a3e62" }; ci.luna[0] = 300; ci.luna[1] = 110; ci.luna[2] = 30; ci.coloreLuna = "#fff0c8"; ci.stelle = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.08f, [](K& c) { RiempiSotto(c, 520, "#1a1a30"); float x = 0; int i = 0;
                while (x < LS) { Caso r(i++); float w = 50 + r() * 70, h = 120 + r() * 200;
                    ColoriEdificio o; o.wall = "#22243e"; o.dark = "#181a2e"; o.win = "#f2c86a"; o.lit = 0.2f; o.roof = r() > 0.6f ? "spire" : "flat"; o.neon = "#f2c86a";
                    Ripeti(x, w, [&](float xx) { Edificio(c, xx, 520, w, h, o, i); }); x += w + 4; }
                c.fillStyle(lg(0, 300, 0, 520, { "rgba(74,62,98,0)", "rgba(74,62,98,0.6)" })); c.fillRect(0, 300, LS, 220); } },
            { 0.3f, [](K& c) { RiempiSotto(c, 560, "#1a1424"); float x = 0; int i = 50;
                while (x < LS) { Caso r(i++); float w = 110 + r() * 90, h = 110 + r() * 130;
                    ColoriEdificio o; o.wall = "#3a2e44"; o.dark = "#22182a"; o.win = "#ffd98a"; o.lit = 0.3f; o.roof = "pitch"; o.roofc = "#5a2a30"; o.chim = true; o.frame = "#2a1a20"; o.ww = 14; o.wh = 18; o.gx = 30; o.gy = 36; o.brick = true;
                    Ripeti(x, w + 20, [&](float xx) { Edificio(c, xx, 560, w, h, o, i); }); x += w + 30; } } } };
        m.scen.primoPiano = "#0a0610"; m.scen.tipoPrimoPiano = "antennas"; m.scen.atmosfera = 1;
        m.buio = 0.45f;
        m.terra.kind = "roof"; m.terra.body = "#3a2630"; m.terra.top = "#8a3e3a";
        m.blocco.kind = "stone"; m.blocco.body = "#3a2e44"; m.blocco.top = "#6a5a74";
        m.legnoPiatt = "#5a3a2a"; m.luceMobile = "#ffd98a";
        m.e = "Ombra dei comignoli"; m.j = "Spazzacamino ombra"; m.f = "Pipistrello dei tetti"; m.s = "Lampionaio oscuro"; m.boss = "Signore dei Lampioni";
        m.frase = "La Citta' dei Tetti: piove e i lampioni sono spenti. Corri sui tetti!";
    }
    // 4. Ghiacciaio dell'Aurora
    {
        MondoGioco& m = mondi[3]; m.nome = "Ghiacciaio dell'Aurora";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#020a1e", "#0a2244", "#1a4e6e" }; ci.aurora = true; ci.stelle = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.06f, [Cr](K& c) { Cresta r = Cr(11, 470, 300, "#5a7ea0", "#2a4a6a", "rgba(220,240,255,0.5)", "rgba(10,30,60,0.35)", "#1a4e6e", 0.5f, true); r.neve = 330; DisegnaCresta(c, r); } },
            { 0.2f, [Cr, Righe](K& c) { DisegnaCresta(c, Cr(13, 520, 90, "#c8e0f0", "#7aa0c0", "rgba(255,255,255,0.5)", "rgba(40,80,120,0.25)"));
                ColoriAlbero a; a.trunk = "#2a3a4a"; a.dark = "#1e3a4a"; a.light = "#2e5a6a"; a.snow = true;
                Righe(5, 520, 110, 200, 90, [&](float x, float b, float h, unsigned) { Pino(c, x, b, h, a); }); } },
            { 0.42f, [Righe](K& c) { ColoriCristallo k{ "#6a9ac0", "#a8d4ea", "#f2fbff", "#9ff8ff" };
                Righe(21, 590, 80, 170, 260, [&](float x, float b, float h, unsigned sd) { Cristalli(c, x, b, h, k, sd); }); } } };
        m.scen.primoPiano = "#0a1a2a"; m.scen.tipoPrimoPiano = "rocks"; m.scen.atmosfera = 2;
        m.buio = 0.3f;
        m.terra.kind = "snow"; m.terra.body = "#6a8aa8"; m.terra.top = "#e8f4ff";
        m.blocco.kind = "stone"; m.blocco.body = "#5a7a98"; m.blocco.top = "#a8c8e0";
        m.legnoPiatt = "#6a7a8a"; m.luceMobile = "#9ff8ff";
        m.e = "Golem di ghiaccio"; m.j = "Spirito della neve"; m.f = "Pipistrello di brina"; m.s = "Golem di brina"; m.boss = "Colosso di ghiaccio";
        m.frase = "Il Ghiacciaio dell'Aurora: l'aurora da' un po' di luce, ma il ghiaccio scivola nei ricordi...";
    }
    // 5. Fonderia dei Vapori
    {
        MondoGioco& m = mondi[4]; m.nome = "Fonderia dei Vapori";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#1a0604", "#4a1a0a", "#a8461a" }; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.06f, [](K& c) { RiempiSotto(c, 520, "#2a0e08");
                for (int i = 0; i < 9; i++) { Caso r(i + 60); float x = r() * LS, h = 240 + r() * 120, w = 40 + r() * 20; Ripeti(x, 60, [&](float xx) { Ciminiera(c, xx, 520, h, w, "#3a1a10", "#5a2a18"); }); }
                c.fillStyle(lg(0, 200, 0, 520, { "rgba(168,70,26,0)", "rgba(168,70,26,0.5)" })); c.fillRect(0, 200, LS, 320); } },
            { 0.2f, [](K& c) { for (int i = 0; i < 10; i++) { Caso r(i + 80); float x = r() * LS, rr = 40 + r() * 70, y = 180 + r() * 260, rot = r() * 3;
                    Ripeti(x, rr * 1.2f, [&](float xx) { Ingranaggio(c, xx, y, rr, "#6a3a22", "#2a0e08", rot); }); }
                Tubi(c, 380, "#7a4a2a", 3); } },
            { 0.4f, [](K& c) { RiempiSotto(c, 560, "#1a0c08"); float x = 0; int i = 90;
                while (x < LS) { Caso r(i++); float w = 140 + r() * 80, h = 130 + r() * 90;
                    ColoriEdificio o; o.wall = "#3a2018"; o.dark = "#1e0e0a"; o.win = "#ff9a3a"; o.lit = 0.5f; o.roof = "flat"; o.ww = 18; o.wh = 12; o.gx = 34; o.gy = 30; o.brick = true;
                    Ripeti(x, w, [&](float xx) { Edificio(c, xx, 560, w, h, o, i); }); x += w + 60; } } } };
        m.scen.primoPiano = "#0e0604"; m.scen.tipoPrimoPiano = "pipesfg"; m.scen.atmosfera = 4;
        m.buio = 0.35f;
        m.terra.kind = "metal"; m.terra.body = "#3a2e2a"; m.terra.top = "#7a6a64"; m.terra.stripe1 = "#f2c23a"; m.terra.stripe2 = "#2a2020";
        m.blocco.kind = "stone"; m.blocco.body = "#4a2a1e"; m.blocco.top = "#8a5a3a";
        m.legnoPiatt = "#7a6a64"; m.luceMobile = "#ffb03a";
        m.e = "Automa di rame"; m.j = "Automa saltellante"; m.f = "Ombra di fuliggine"; m.s = "Cannone a vapore"; m.boss = "Caldaia Gigante";
        m.frase = "La Fonderia dei Vapori: le macchine del Re Eclisse lavorano giorno e notte. Attenta alle braci!";
    }
    // 6. Torre dell'Eclissi
    {
        MondoGioco& m = mondi[5]; m.nome = "Torre dell'Eclissi";
        m.scen.nome = m.nome;
        m.scen.cielo = [](K& c) { Cielo ci; ci.fermate = { "#020106", "#120420", "#2a0a3a" }; ci.eclissi[0] = 640; ci.eclissi[1] = 190; ci.eclissi[2] = 74; ci.stelle = true; DisegnaCielo(c, ci); };
        m.scen.strati = {
            { 0.06f, [Cr](K& c) { DisegnaCresta(c, Cr(31, 520, 200, "#1e0e30", "#0e0618", "rgba(255,150,120,0.18)", "rgba(0,0,0,0.3)", "#2a0a3a", 0.5f, true)); } },
            { 0.25f, [](K& c) { RiempiSotto(c, 560, "#120a1e");
                for (int x = 60; x < (int)LS; x += 240) Colonna(c, (float)x, 560, 200 + (float)((x * 7) % 140), 34, "#2a1a40", "#a04aff");
                ColoriCastello k{ "#24183a", "#3a1a5a", "#a04aff", "#ff5ad8", "#5a1a8a" }; Castello(c, 700, 560, 240, k); } } };
        m.scen.primoPiano = "#05020a"; m.scen.tipoPrimoPiano = "thorns"; m.scen.atmosfera = 10;
        m.buio = 0.78f;
        m.terra.kind = "purple"; m.terra.body = "#1e1430"; m.terra.top = "#5a3a7a";
        m.blocco.kind = "stone"; m.blocco.body = "#24183a"; m.blocco.top = "#4a3a6a";
        m.legnoPiatt = "#3a2a5a"; m.luceMobile = "#ff5ad8";
        m.e = "Cavaliere dell'Eclissi"; m.j = "Ombrino viola"; m.f = "Pipistrello dell'Eclissi"; m.s = "Mago dell'Eclissi"; m.boss = "Re Eclisse";
        m.frase = "La Torre dell'Eclissi. In cima ti aspetta il Re Eclisse: riporta la luce al mondo!";
    }

    // --- musica ----------------------------------------------------------------------------------------
    p.brani.push_back(CreaBrano("Tema di Lumina", 84, 8, {
        { "Melodia", 1, 0.5f, "A4:2 C5:2 E5:2 D5:1 C5:1 B4:4 G4:2 B4:2 D5:2 C5:1 B4:1 A4:4 E5:2 F5:2 E5:2 D5:2 C5:3 B4:1 A4:2 G#4:2 A4:8" },
        { "Basso", 3, 0.6f, "A2:8 G2:8 F2:8 E2:8" },
        { "Arpeggio", 3, 0.22f, "A4:1 E5:1 A5:1 E5:1 G4:1 D5:1 G5:1 D5:1 F4:1 C5:1 F5:1 C5:1 E4:1 B4:1 E5:1 G#4:1" } }));
    p.brani.push_back(CreaBrano("Bosco delle Lucciole", 80, 8, {
        { "Flauto", 3, 0.5f, "E5:3 D5:1 B4:2 A4:2 G4:4 E4:4 G4:2 A4:2 B4:2 D5:2 E5:6 R:2 G5:3 E5:1 D5:2 B4:2 A4:4 B4:2 G4:2 E4:8" },
        { "Arpa", 1, 0.25f, "E3:1 B3:1 E4:1 G4:1 B4:1 G4:1 E4:1 B3:1 C3:1 G3:1 C4:1 E4:1 G4:1 E4:1 C4:1 G3:1 D3:1 A3:1 D4:1 F#4:1 A4:1 F#4:1 D4:1 A3:1 B2:1 F#3:1 B3:1 D#4:1 F#4:1 D#4:1 B3:1 F#3:1" },
        { "Basso", 3, 0.5f, "E2:8 C2:8 D2:8 B1:8" } }));
    p.brani.push_back(CreaBrano("Miniere di Cristallo", 92, 8, {
        { "Campane", 3, 0.42f, "B5:1 R:1 F#5:1 R:1 D5:1 R:1 B4:2 C#5:1 R:1 E5:1 R:1 G5:2 F#5:2 B5:1 R:1 A5:1 R:1 F#5:1 R:1 D5:2 E5:2 C#5:2 B4:4" },
        { "Arpeggio", 1, 0.22f, "B3:1 D4:1 F#4:1 B4:1 G3:1 B3:1 D4:1 G4:1 A3:1 C#4:1 E4:1 A4:1 F#3:1 A3:1 C#4:1 F#4:1" },
        { "Basso", 1, 0.5f, "B1:4 G1:4 A1:4 F#1:4" },
        { "Gocce", 4, 0.3f, "....h.......h..h", true } }));
    p.brani.push_back(CreaBrano("Citta' dei Tetti", 100, 8, {
        { "Clarinetto", 1, 0.45f, "D5:2 F5:1 A5:1 G5:2 F5:1 E5:1 D5:2 A4:2 R:2 C5:1 D5:1 E5:2 G5:1 F5:1 E5:2 C5:2 D5:4 R:2 A4:1 C5:1 D5:2 F5:2 E5:1 D5:1 C5:1 A4:1 D5:6 R:2" },
        { "Basso", 1, 0.55f, "D2:2 A2:2 D2:2 A2:2 C2:2 G2:2 C2:2 G2:2 Bb1:2 F2:2 Bb1:2 F2:2 A1:2 E2:2 A1:2 E2:2" },
        { "Spazzole", 4, 0.35f, "K..hS.h.K.hhS.h.", true } }));
    p.brani.push_back(CreaBrano("Ghiacciaio dell'Aurora", 76, 8, {
        { "Voce", 3, 0.45f, "F#5:4 E5:2 C#5:2 B4:4 A4:4 B4:2 C#5:2 E5:2 F#5:2 G#5:8 F#5:4 E5:2 C#5:2 D5:4 C#5:2 B4:2 A4:8 R:8" },
        { "Pad", 1, 0.25f, "F#3:8 D3:8 A3:8 E3:8" },
        { "Brillii", 3, 0.15f, "C#6:1 R:3 E6:1 R:3 A5:1 R:3 F#6:1 R:3" } }));
    p.brani.push_back(CreaBrano("Fonderia dei Vapori", 116, 8, {
        { "Ottoni", 2, 0.3f, "E4:2 E4:1 G4:1 A4:2 E4:2 Bb4:2 A4:1 G4:1 E4:4 E4:2 E4:1 G4:1 A4:2 C5:2 B4:2 A4:1 G4:1 A4:4" },
        { "Basso", 2, 0.4f, "E2:1 E2:1 R:1 E2:1 G2:1 R:1 A2:1 R:1" },
        { "Martelli", 4, 0.55f, "K.S.KKS.K.S.KKSo", true } }));
    p.brani.push_back(CreaBrano("Torre dell'Eclissi", 96, 8, {
        { "Organo", 2, 0.3f, "D5:2 F5:2 A5:2 G#5:2 A5:4 F5:2 D5:2 E5:2 F5:2 E5:2 C#5:2 D5:8 Bb4:2 D5:2 F5:2 E5:2 F5:4 D5:2 Bb4:2 A4:2 C#5:2 E5:2 G5:2 F5:4 E5:4" },
        { "Basso", 1, 0.55f, "D2:4 D2:4 Bb1:4 A1:4 G1:4 Bb1:4 A1:8" },
        { "Tamburo", 4, 0.4f, "K.......S.......", true } }));
    p.brani.push_back(CreaBrano("Duello nel buio", 140, 8, {
        { "Archi", 2, 0.32f, "A4:1 C5:1 E5:1 A5:1 G#5:2 E5:2 F5:1 E5:1 D5:1 C5:1 B4:2 E5:2 A4:1 C5:1 E5:1 A5:1 B5:2 C6:2 B5:1 A5:1 G#5:1 B5:1 A5:4" },
        { "Basso", 2, 0.45f, "A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 E1:1 E2:1 E1:1 E2:1 E1:1 E2:1 G#1:1 G#2:1" },
        { "Batteria", 4, 0.55f, "KhShKhShKKShKhSS", true } }));
    for (int i = 0; i < 6; i++) mondi[i].brano = p.brani[i + 1].nome;

    // --- effetti sonori ---------------------------------------------------------------------------------
    p.effetti = {
        Effetto("Salto", 3, 330, 780, 0.18f, 0.4f),
        Effetto("Scintilla", 3, 1320, 1980, 0.16f, 0.35f, 0, 0.02f, 14),
        Effetto("Cuore", 3, 520, 1040, 0.3f, 0.45f, 0, 0.05f, 9),
        Effetto("Chiave", 1, 900, 1800, 0.3f, 0.4f, 0, 0.04f, 12),
        Effetto("Porta", 1, 200, 60, 0.5f, 0.4f, 0.2f),
        Effetto("Molla", 1, 260, 1100, 0.3f, 0.45f, 0, 0.1f, 16),
        Effetto("Lanterna", 3, 784, 1568, 0.6f, 0.45f, 0, 0.03f, 5),
        Effetto("Portale", 3, 262, 2093, 1.1f, 0.5f, 0, 0.06f, 6),
        Effetto("Colpo", 4, 300, 80, 0.14f, 0.5f),
        Effetto("Sconfitto", 3, 660, 80, 0.4f, 0.45f, 0.3f),
        Effetto("Ahi", 2, 400, 120, 0.25f, 0.5f, 0.2f),
        Effetto("Sparo nemico", 3, 500, 180, 0.16f, 0.3f, 0.2f),
        Effetto("Boss sconfitto", 3, 880, 40, 1.5f, 0.6f, 0.4f, 0.1f, 6),
    };
    p.suoniEventi = { { "salto", "Salto" }, { "moneta", "Scintilla" }, { "colpo", "Colpo" }, { "molla", "Molla" } };

    NomiGioco n;
    n.moneta = "Scintilla"; n.cuore = "Cuore"; n.molla = "Fiore molla"; n.controllo = "Lanterna"; n.chiave = "Chiave di luce";
    n.porta = "Porta d'ombra"; n.cancello = "Sigillo d'ombra"; n.arrivo = "Grande Lanterna";
    n.branoBoss = "Duello nel buio"; n.branoVittoria = "Tema di Lumina";
    n.fraseBoss = "Il guardiano e' sconfitto! Il sigillo d'ombra si dissolve: accendi la Grande Lanterna.";
    n.fraseFinale = "Il Re Eclisse e' sconfitto! Le lanterne del mondo tornano a brillare!";
    n.energiaBoss = 10; n.seme = 1;
    CostruisciMondi(p, resa, mondi, n);
    return p;
}

}  // namespace piatt
