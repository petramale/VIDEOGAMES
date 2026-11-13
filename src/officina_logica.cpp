// Sezioni Logica (regole "quando ... allora ..." e variabili), Librerie (la libreria del gioco
// e le raccolte pronte) e Comandi (tastiera, mouse e joypad) dell'officina.
#include "officina.h"
#include "audio.h"
#include "periferiche.h"
#include "raccolte.h"
#include "ui.h"
#include <algorithm>

namespace {

std::vector<std::string> Con(const std::vector<std::string>& v, const std::string& primo) {
    std::vector<std::string> r{ primo };
    r.insert(r.end(), v.begin(), v.end());
    return r;
}

int Indice(const std::vector<std::string>& v, const std::string& x, int altrimenti = 0) {
    for (int i = 0; i < (int)v.size(); i++) if (v[i] == x) return i;
    return altrimenti;
}

int IndiceEvento(const std::string& e) { return Indice(TipiEvento(), e); }
int IndiceAzione(const std::string& a) { return Indice(TipiAzione(), a); }

const std::vector<std::string>& Tasti() {
    static std::vector<std::string> v;
    if (v.empty()) {
        for (char c = 'A'; c <= 'Z'; c++) v.push_back(std::string(1, c));
        for (char c = '0'; c <= '9'; c++) v.push_back(std::string(1, c));
        for (auto t : { "SPAZIO", "INVIO", "SU", "GIU", "SINISTRA", "DESTRA", "CTRL", "SHIFT", "TAB" }) v.push_back(t);
    }
    return v;
}

std::string Frase(const Regola& r) {
    int e = IndiceEvento(r.evento);
    std::string f = "Quando " + r.chi + " " + NomiEvento()[e];
    if (!r.conChi.empty()) f += " " + r.conChi + (r.evento == "ogni" ? " s" : "");
    if (!r.variabile.empty()) f += TextFormat(", se %s %s %g", r.variabile.c_str(), r.confronto.c_str(), r.valore);
    f += ":";
    for (size_t i = 0; i < r.azioni.size() && i < 3; i++) {
        const AzioneRegola& a = r.azioni[i];
        f += (i ? ", " : " ") + NomiAzione()[IndiceAzione(a.tipo)];
        if (!a.testo.empty()) f += " " + a.testo;
        if (a.numero != 0) f += TextFormat(" %g", a.numero);
    }
    if (r.azioni.size() > 3) f += "...";
    if (r.azioni.empty()) f += " (nessuna azione)";
    return f;
}

constexpr int RIGHE_COMANDI = NUM_AZIONI + 2;

} // namespace

// --- Azioni dai menu ------------------------------------------------------------------------------

void Officina::AzioneLogica(const std::string& c) {
    if (c == "r:nuova") {
        Ricorda();
        Regola r;
        r.chi = "eroe";
        r.evento = "tocca";
        AzioneRegola a; a.tipo = "suona";
        if (!progetto.effetti.empty()) a.testo = progetto.effetti[0].nome;
        r.azioni.push_back(a);
        progetto.regole.push_back(r);
        selRegola = (int)progetto.regole.size() - 1;
    } else if (c == "r:duplica" && selRegola < (int)progetto.regole.size()) {
        Ricorda();
        progetto.regole.insert(progetto.regole.begin() + selRegola + 1, progetto.regole[selRegola]);
        selRegola++;
    } else if (c == "r:elimina" && selRegola < (int)progetto.regole.size()) {
        Ricorda();
        progetto.regole.erase(progetto.regole.begin() + selRegola);
        selRegola = std::max(0, selRegola - 1);
    } else if (c == "r:su" && selRegola > 0 && selRegola < (int)progetto.regole.size()) {
        Ricorda();
        std::swap(progetto.regole[selRegola], progetto.regole[selRegola - 1]);
        selRegola--;
    } else if (c == "r:giu" && selRegola + 1 < (int)progetto.regole.size()) {
        Ricorda();
        std::swap(progetto.regole[selRegola], progetto.regole[selRegola + 1]);
        selRegola++;
    } else if (c == "r:esempi") {
        Ricorda();
        auto Nuova = [&](const std::string& chi, const std::string& ev, const std::string& con, std::vector<AzioneRegola> az) {
            Regola r; r.chi = chi; r.evento = ev; r.conChi = con; r.azioni = az; progetto.regole.push_back(r);
        };
        std::string nemico, moneta, porta;
        for (auto& p : progetto.personaggi) if (p.comportamento.tipo != TipoComportamento::Giocatore && nemico.empty()) nemico = p.nome;
        for (auto& o : progetto.oggetti) if (o.funzione == FunzioneOggetto::Raccogli && moneta.empty()) moneta = o.nome;
        for (auto& o : progetto.oggetti) if (o.funzione == FunzioneOggetto::Porta && porta.empty()) porta = o.nome;
        Nuova("gioco", "inizio", "", { { "messaggio", "Benvenuto! Raccogli tutto e arriva alla fine", 0 } });
        if (!moneta.empty()) Nuova("eroe", "raccoglie", moneta, { { "aumenta", "monete", 1 } });
        Regola r; r.chi = "gioco"; r.evento = "variabile"; r.variabile = "monete"; r.confronto = ">="; r.valore = 10;
        r.azioni = { { "messaggio", "10 monete: la porta si apre!", 0 }, { "apri", porta, 0 }, { "suona", progetto.effetti.empty() ? "" : progetto.effetti[0].nome, 0 } };
        progetto.regole.push_back(r);
        if (!nemico.empty()) Nuova(nemico, "sconfitto", "", { { "punti", "", 200 }, { "scuoti", "", 0.3f } });
        Nuova("eroe", "tasto", "H", { { "messaggio", "Frecce per muoverti, spazio per saltare, X per sparare", 0 } });
        progetto.variabili["monete"] = 0;
        selRegola = (int)progetto.regole.size() - 1;
        Avvisa("Aggiunte 5 regole d'esempio: guardale e cambiale come vuoi");
    } else if (c == "l:raccolta") {
        Ricorda();
        const Raccolta& r = CreaRaccolta(raccoltaScelta, progetto.stile);
        int n = 0;
        for (auto& x : r.personaggi) if (!progetto.Personaggio(x.nome)) { progetto.personaggi.push_back(x); n++; }
        for (auto& x : r.oggetti) if (!progetto.Oggetto(x.nome)) { progetto.oggetti.push_back(x); n++; }
        for (auto& x : r.scenari) if (!progetto.TrovaScenario(x.nome)) { progetto.scenari.push_back(x); n++; }
        for (auto& x : r.caselle) if (!progetto.Casella(x.carattere)) { progetto.caselle.push_back(x); n++; }
        for (auto& x : r.brani) if (!progetto.TrovaBrano(x.nome)) { progetto.brani.push_back(x); n++; }
        for (auto& x : r.effetti) if (!progetto.TrovaEffetto(x.nome)) { progetto.effetti.push_back(x); n++; }
        LibreriaCambiata();
        Avvisa(TextFormat("Raccolta %s: %d elementi aggiunti al gioco", r.nome.c_str(), n));
        gAudio.Suona(Effetto::Gemma);
    } else if (c == "k:predefiniti") {
        gInput.m = Mappatura::Predefinita();
        gInput.Salva();
        Avvisa("Comandi predefiniti ripristinati");
    } else if (c == "k:salva") {
        gInput.Salva();
        Avvisa("Comandi salvati in comandi.cfg (valgono anche per i giochi creati)");
    }
}

// --- Logica ------------------------------------------------------------------------------------------

void Officina::Logica(Rectangle r, float dt) {
    (void)dt;
    float s = Ui::S();
    Rectangle sx = { r.x, r.y, 360 * s, r.height };
    Rectangle dx = { r.x + r.width - 300 * s, r.y, 300 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width - dx.width, r.height };
    std::vector<std::string> frasi;
    for (auto& rg : progetto.regole) frasi.push_back((rg.attiva ? "" : "(spenta) ") + Frase(rg));
    int az = Elenco(sx, "REGOLE DEL GIOCO", frasi, selRegola, [&](int i, Rectangle q) {
        Ui::TestoCentrato(std::to_string(i + 1), q, progetto.regole[i].attiva ? Ui::Accento() : Ui::TestoSpento(), Ui::T());
    });
    if (az == 1) AzioneLogica("r:nuova");
    if (az == 2) AzioneLogica("r:duplica");
    if (az == 3) AzioneLogica("r:elimina");

    // Variabili (valori iniziali), a destra.
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    float x = dx.x + 12 * s, y = dx.y + 10 * s, w = dx.width - 24 * s, h = 28 * s;
    Ui::Testo("VARIABILI", x, y, Ui::Accento(), Ui::TP());
    y += 24 * s;
    Ui::TestoACapo("Numeri che le regole possono aumentare, controllare e mostrare. Si vedono in alto nel gioco.", x, y, w, Ui::TestoSpento(), (int)(13 * s));
    y += 44 * s;
    std::string togli;
    for (auto& v : progetto.variabili) {
        float val = v.second;
        if (Ui::Numero("var_" + v.first, { x, y, w - 36 * s, h }, v.first, val, -99999, 99999, 1, "%g")) { Ricorda(); progetto.variabili[v.first] = val; }
        if (Ui::Pulsante({ x + w - 30 * s, y, 30 * s, h }, "x")) togli = v.first;
        y += h + 6 * s;
        if (y > dx.y + dx.height - 80 * s) break;
    }
    if (!togli.empty()) { Ricorda(); progetto.variabili.erase(togli); }
    Ui::Campo("var_nuova", { x, y, w - 90 * s, h }, bozzaVariabile, 24);
    if (Ui::Pulsante({ x + w - 84 * s, y, 84 * s, h }, "+ Aggiungi") && !bozzaVariabile.empty()) {
        Ricorda();
        progetto.variabili[bozzaVariabile] = 0;
        bozzaVariabile.clear();
    }

    if (progetto.regole.empty()) {
        Ui::TestoACapo("Nessuna regola. Le regole dicono cosa succede nel gioco: \"quando l'eroe tocca il drago, perde un cuore e lo schermo trema\". "
                       "Premi \"+ Nuovo\", usa il menu Regole > Aggiungi le regole d'esempio, oppure chiedi a Claude.",
                       centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    selRegola = std::min(selRegola, (int)progetto.regole.size() - 1);
    Regola& rg = progetto.regole[selRegola];
    Regola prima = rg;
    bool cambiato = false;
    std::vector<std::string> nomi;
    for (auto& p : progetto.personaggi) nomi.push_back(p.nome);
    for (auto& o : progetto.oggetti) nomi.push_back(o.nome);
    std::vector<std::string> variabili;
    for (auto& v : progetto.variabili) variabili.push_back(v.first);
    for (auto v : { "punti", "cuori", "monete", "gemme", "vite" }) if (std::find(variabili.begin(), variabili.end(), v) == variabili.end()) variabili.push_back(v);
    std::vector<std::string> effetti, brani, mappe, proiettili, porte;
    for (auto& e : progetto.effetti) effetti.push_back(e.nome);
    for (auto& b : progetto.brani) brani.push_back(b.nome);
    for (auto& m : progetto.mondo.mappe) mappe.push_back(m.nome);
    for (auto& o : progetto.oggetti) if (o.funzione == FunzioneOggetto::Proiettile) proiettili.push_back(o.nome);
    for (auto& o : progetto.oggetti) if (o.funzione == FunzioneOggetto::Porta || o.funzione == FunzioneOggetto::Chiave || o.funzione == FunzioneOggetto::Interruttore) porte.push_back(o.nome);

    x = centro.x + 24 * s; y = centro.y + 16 * s; w = centro.width - 48 * s; h = 32 * s;
    Ui::Testo(TextFormat("Regola %d", selRegola + 1), x, y, WHITE, (int)(22 * s));
    if (Ui::Interruttore({ x + w - 170 * s, y, 170 * s, 28 * s }, "Attiva", rg.attiva)) cambiato = true;
    y += 40 * s;
    // Riquadro QUANDO.
    auto Riquadro = [&](const char* titolo, float altezza, Color c) {
        Rectangle b = { x, y, w, altezza };
        DrawRectangleRounded(b, 0.06f, 6, Hex(0x141838));
        DrawRectangleRec({ b.x, b.y, 6 * s, b.height }, c);
        Ui::Testo(titolo, b.x + 16 * s, b.y + 8 * s, c, Ui::T());
        return b;
    };
    float lw = 130 * s;
    Rectangle q = Riquadro("QUANDO", 130 * s, Hex(0x5ec8ff));
    float cy = q.y + 38 * s, cx = q.x + 16 * s, cw = q.width - 32 * s;
    std::vector<std::string> chi = Con(Con(nomi, "gioco"), "eroe");
    int ic = Indice(chi, rg.chi, -1);
    if (ic < 0) { chi.push_back(rg.chi); ic = (int)chi.size() - 1; }
    if (Ui::Scelta({ cx, cy, cw * 0.5f - 6 * s, h }, "Chi", ic, chi)) { rg.chi = chi[ic]; cambiato = true; }
    int ie = IndiceEvento(rg.evento);
    if (Ui::Scelta({ cx + cw * 0.5f + 6 * s, cy, cw * 0.5f - 6 * s, h }, "Evento", ie, NomiEvento())) { rg.evento = TipiEvento()[ie]; rg.conChi.clear(); cambiato = true; }
    cy += h + 10 * s;
    Rectangle rc = { cx, cy, cw, h };
    const std::string& ev = rg.evento;
    if (ev == "tocca" || ev == "raccoglie" || ev == "colpisce" || ev == "vicino") {
        std::vector<std::string> v = Con(Con(nomi, "eroe"), "(qualsiasi)");
        int k = rg.conChi.empty() ? 0 : Indice(v, rg.conChi, 0);
        if (Ui::Scelta(rc, "Con chi / che cosa", k, v)) { rg.conChi = k == 0 ? "" : v[k]; cambiato = true; }
    } else if (ev == "energia") {
        float soglia = (float)std::atof(rg.conChi.c_str());
        if (Ui::Numero("rg_soglia", rc, "Energia sotto", soglia, 0, 999, 1)) { rg.conChi = TextFormat("%g", soglia); cambiato = true; }
    } else if (ev == "tasto") {
        int k = Indice(Tasti(), rg.conChi, 0);
        if (rg.conChi.empty()) { rg.conChi = Tasti()[0]; cambiato = true; }
        if (Ui::Scelta(rc, "Tasto", k, Tasti())) { rg.conChi = Tasti()[k]; cambiato = true; }
    } else if (ev == "joypad") {
        int b = std::max(1, std::atoi(rg.conChi.c_str()));
        if (Ui::NumeroIntero("rg_joy", rc, "Pulsante", b, 1, 17)) { rg.conChi = std::to_string(b); cambiato = true; }
        if (rg.conChi.empty()) rg.conChi = "1";
    } else if (ev == "ogni") {
        float sec = rg.conChi.empty() ? 1 : (float)std::atof(rg.conChi.c_str());
        if (Ui::Numero("rg_ogni", rc, "Ogni (secondi)", sec, 0.2f, 600, 0.5f, "%.1f")) { rg.conChi = TextFormat("%g", sec); cambiato = true; }
    } else if (ev == "inizio") {
        std::vector<std::string> v = Con(mappe, "(ogni mappa)");
        int k = rg.conChi.empty() ? 0 : Indice(v, rg.conChi, 0);
        if (Ui::Scelta(rc, "Mappa", k, v)) { rg.conChi = k == 0 ? "" : v[k]; cambiato = true; }
    } else if (ev == "variabile") {
        Ui::Testo("Scatta quando la condizione qui sotto diventa vera.", rc.x, rc.y + 8 * s, Ui::TestoSpento(), Ui::TP());
    } else {
        Ui::Testo("Nessun dettaglio da scegliere per questo evento.", rc.x, rc.y + 8 * s, Ui::TestoSpento(), Ui::TP());
    }
    y += q.height + 12 * s;
    // Riquadro SE.
    q = Riquadro("SE (facoltativo)", 86 * s, Hex(0xffd23f));
    cy = q.y + 38 * s;
    std::vector<std::string> vv = Con(variabili, "(sempre)");
    int kv = rg.variabile.empty() ? 0 : Indice(vv, rg.variabile, 0);
    if (Ui::Scelta({ cx, cy, cw * 0.45f, h }, "", kv, vv)) { rg.variabile = kv == 0 ? "" : vv[kv]; cambiato = true; }
    if (!rg.variabile.empty()) {
        static const std::vector<std::string> confronti = { ">=", "<=", "==", "!=", ">", "<" };
        int kc = Indice(confronti, rg.confronto, 0);
        if (Ui::Scelta({ cx + cw * 0.47f, cy, cw * 0.2f, h }, "", kc, confronti)) { rg.confronto = confronti[kc]; cambiato = true; }
        if (Ui::Numero("rg_val", { cx + cw * 0.69f, cy, cw * 0.31f, h }, "", rg.valore, -99999, 99999, 1, "%g")) cambiato = true;
    }
    y += q.height + 12 * s;
    // Riquadro ALLORA.
    float altA = 46 * s + rg.azioni.size() * (h + 8 * s) + h + 10 * s;
    q = Riquadro("ALLORA", altA, Hex(0x7ee07e));
    cy = q.y + 38 * s;
    int togliAz = -1;
    for (int i = 0; i < (int)rg.azioni.size(); i++) {
        AzioneRegola& a = rg.azioni[i];
        int ka = IndiceAzione(a.tipo);
        float aw = cw * 0.36f;
        if (Ui::Scelta({ cx, cy, aw, h }, "", ka, NomiAzione())) { a.tipo = TipiAzione()[ka]; a.testo.clear(); a.numero = 0; cambiato = true; }
        Rectangle t1 = { cx + aw + 8 * s, cy, cw - aw - 8 * s - 40 * s, h };
        Rectangle tTesto = { t1.x, t1.y, t1.width * 0.62f, h }, tNum = { t1.x + t1.width * 0.64f, t1.y, t1.width * 0.36f, h };
        const std::string& t = a.tipo;
        auto SceltaTesto = [&](Rectangle rr, std::vector<std::string> v, const std::string& vuoto) {
            if (!vuoto.empty()) v = Con(v, vuoto);
            if (v.empty()) { Ui::Testo("(niente da scegliere)", rr.x, rr.y + 8 * s, Ui::TestoSpento(), Ui::TP()); return; }
            int k = Indice(v, a.testo, -1);
            if (k < 0) { if (!a.testo.empty()) { v.push_back(a.testo); k = (int)v.size() - 1; } else k = 0; }
            if (Ui::Scelta(rr, "", k, v)) { a.testo = (!vuoto.empty() && k == 0) ? "" : v[k]; cambiato = true; if (t == "suona") if (auto* e = progetto.TrovaEffetto(a.testo)) gAudio.SuonaEffetto(*e); }
            else if (a.testo.empty() && vuoto.empty()) { a.testo = v[0]; cambiato = true; }
        };
        auto Num = [&](Rectangle rr, const char* et, float lo, float hi, float passo) {
            if (Ui::Numero("rg_n" + std::to_string(i), rr, et, a.numero, lo, hi, passo, "%g")) cambiato = true;
        };
        if (t == "suona") SceltaTesto(t1, effetti, "");
        else if (t == "musica") SceltaTesto(t1, brani, "");
        else if (t == "messaggio" || t == "anima") { if (Ui::Campo("rg_t" + std::to_string(i), t1, a.testo, 200)) cambiato = true; }
        else if (t == "aumenta" || t == "imposta") { SceltaTesto(tTesto, variabili, ""); Num(tNum, "", -99999, 99999, 1); }
        else if (t == "punti" || t == "cuori" || t == "energia" || t == "velocita" || t == "salta" || t == "scuoti" || t == "spingi") Num(t1, "", -99999, 99999, t == "scuoti" ? 0.1f : 1);
        else if (t == "vai_mappa") SceltaTesto(t1, mappe, "");
        else if (t == "crea") { SceltaTesto(tTesto, nomi, ""); Num(tNum, "", 1, 20, 1); }
        else if (t == "spara") { SceltaTesto(tTesto, proiettili, "(palla di fuoco)"); Num(tNum, "", 1, 12, 1); }
        else if (t == "apri") SceltaTesto(t1, porte, "(tutte le porte)");
        else if (t == "colore") {
            Color c = WHITE;
            TestoInColore(a.testo, c);
            if (a.testo.empty()) a.testo = "#ff6060";
            if (Ui::Colore("rg_c" + std::to_string(i), t1, "", c)) { a.testo = ColoreInTesto(c); cambiato = true; }
        } else Ui::Testo("-", t1.x, t1.y + 8 * s, Ui::TestoSpento(), Ui::TP());
        if (Ui::Pulsante({ cx + cw - 34 * s, cy, 34 * s, h }, "x")) togliAz = i;
        cy += h + 8 * s;
    }
    if (togliAz >= 0) { rg.azioni.erase(rg.azioni.begin() + togliAz); cambiato = true; }
    if (Ui::Pulsante({ cx, cy, 200 * s, h }, "+ Aggiungi un'azione")) {
        AzioneRegola a; a.tipo = "messaggio"; a.testo = "Ciao!";
        rg.azioni.push_back(a);
        cambiato = true;
    }
    y += q.height + 14 * s;
    Ui::TestoACapo("In parole: " + Frase(rg), x, y, w, Ui::TestoChiaro(), Ui::TP());
    y += 50 * s;
    float bw = 140 * s;
    if (Ui::Pulsante({ x, y, bw, h }, "Sposta su", false, selRegola > 0)) AzioneLogica("r:su");
    if (Ui::Pulsante({ x + bw + 8 * s, y, bw, h }, "Sposta giu'", false, selRegola + 1 < (int)progetto.regole.size())) AzioneLogica("r:giu");
    if (Ui::Pulsante({ x + 2 * (bw + 8 * s), y, bw, h }, "Prova  F5", true)) AvviaProva();
    (void)lw;
    if (cambiato && selRegola < (int)progetto.regole.size()) {
        ProgettoGioco v = progetto;
        v.regole[selRegola] = prima;
        annulla.push_back(v);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
    }
}

// --- Librerie -------------------------------------------------------------------------------------------

void Officina::Librerie(Rectangle r, float dt) {
    (void)dt;
    float s = Ui::S();
    float meta = std::floor(r.width * 0.55f);
    Rectangle sx = { r.x, r.y, meta, r.height }, dx = { r.x + meta, r.y, r.width - meta, r.height };
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    // La libreria del gioco.
    float x = sx.x + 20 * s, y = sx.y + 14 * s, w = sx.width - 40 * s;
    Titolo({ x, y, w, 30 * s }, "La libreria del gioco", "tutto quello che il gioco usa");
    y += 40 * s;
    const char* cat[] = { "Personaggi", "Oggetti", "Scenari", "Caselle", "Brani", "Effetti", "Regole" };
    int conti[] = { (int)progetto.personaggi.size(), (int)progetto.oggetti.size(), (int)progetto.scenari.size(), (int)progetto.caselle.size(),
                    (int)progetto.brani.size(), (int)progetto.effetti.size(), (int)progetto.regole.size() };
    float bx = x;
    for (int i = 0; i < 7; i++) {
        std::string t = TextFormat("%s (%d)", cat[i], conti[i]);
        float bw = Ui::Larghezza(t, Ui::TP()) + 24 * s;
        if (Ui::Pulsante({ bx, y, bw, 30 * s }, t, categoriaLib == i)) { categoriaLib = i; scorriLib = 0; }
        bx += bw + 6 * s;
    }
    y += 42 * s;
    Rectangle griglia = { x, y, w, sx.y + sx.height - y - 10 * s };
    int col = std::max(2, (int)(w / (150 * s)));
    float gap = 10 * s, cw = (w - (col - 1) * gap) / col, ch = cw * 0.9f + 50 * s;
    int n = conti[categoriaLib];
    if (Ui::MouseSu(griglia)) scorriLib -= GetMouseWheelMove() * 80 * s;
    scorriLib = Limita(scorriLib, 0, std::max(0.0f, ((n + col - 1) / col) * (ch + gap) - griglia.height));
    BeginScissorMode((int)griglia.x, (int)griglia.y, (int)griglia.width, (int)griglia.height);
    int apri = -1, duplica = -1, elimina = -1;
    for (int i = 0; i < n; i++) {
        Rectangle c = { x + (i % col) * (cw + gap), griglia.y + (i / col) * (ch + gap) - scorriLib, cw, ch };
        if (c.y + c.height < griglia.y || c.y > griglia.y + griglia.height) continue;
        bool sopra = Ui::MouseSu(c) && CheckCollisionPointRec(GetMousePosition(), griglia);
        DrawRectangleRounded(c, 0.08f, 6, sopra ? Hex(0x1f2556) : Ui::Pannello());
        DrawRectangleRoundedLinesEx(c, 0.08f, 6, 1, sopra ? Ui::Accento() : Ui::Bordo());
        Rectangle ic = { c.x + 8 * s, c.y + 8 * s, c.width - 16 * s, c.width * 0.9f - 16 * s };
        std::string nome, sotto;
        switch (categoriaLib) {
            case 0: { auto& p = progetto.personaggi[i]; DrawRectangleRec(ic, Hex(0x2a2e50)); p.disegno.DisegnaIn(ic, std::max(0, p.disegno.TrovaAnimazione(sopra ? "corsa" : "fermo")), tempo);
                      nome = p.nome; sotto = p.nome == progetto.eroe ? "eroe" : ChiaveComportamento(p.comportamento.tipo); break; }
            case 1: { auto& o = progetto.oggetti[i]; DrawRectangleRec(ic, Hex(0x2a2e50)); o.disegno.DisegnaIn(ic, 0, tempo); nome = o.nome; sotto = ChiaveFunzione(o.funzione); break; }
            case 2: { auto& sc = progetto.scenari[i];
                      Color a = sc.cieloPersonale ? sc.cieloAlto : Hex(0x3f8ff0), b = sc.cieloPersonale ? sc.cieloBasso : Hex(0xb9e3ff);
                      DrawRectangleGradientV((int)ic.x, (int)ic.y, (int)ic.width, (int)ic.height, a, b);
                      for (size_t k = 0; k < sc.strati.size() && k < 4; k++)
                          DrawTriangle({ ic.x - ic.width * 0.2f + k * ic.width * 0.3f, ic.y + ic.height }, { ic.x + ic.width * 0.4f + k * ic.width * 0.3f, ic.y + ic.height },
                                       { ic.x + ic.width * 0.1f + k * ic.width * 0.3f, ic.y + ic.height * (0.3f + 0.1f * k) }, sc.strati[k].colore);
                      nome = sc.nome; sotto = TextFormat("%d strati", (int)sc.strati.size()); break; }
            case 3: { auto& cs = progetto.caselle[i]; DrawRectangleRec(ic, Hex(0x2a2e50)); cs.disegno.DisegnaIn(ic, 0, tempo); nome = cs.nome; sotto = std::string("casella '") + cs.carattere + "'"; break; }
            case 4: { auto& b = progetto.brani[i]; DrawRectangleRec(ic, Hex(0x1a1e40));
                      for (auto& t : b.tracce) for (auto& nt : t.note) {
                          float px = ic.x + nt.inizio / std::max(1, b.battute * 16) * ic.width, py = ic.y + ic.height - (nt.altezza - 30) / 70.0f * ic.height;
                          DrawRectangleRec({ px, Limita(py, ic.y, ic.y + ic.height - 2), std::max(1.0f, nt.durata / std::max(1, b.battute * 16) * ic.width), 2 }, Hex(0x7ee0ff)); }
                      nome = b.nome; sotto = b.file.empty() ? TextFormat("%.0f bpm", b.bpm) : "da file"; break; }
            case 5: { auto& e = progetto.effetti[i]; DrawRectangleRec(ic, Hex(0x1a1e40));
                      for (int k = 0; k < 16; k++) { float hh = (0.2f + 0.8f * std::fabs(std::sin(k * 0.9f + e.f0 * 0.003f))) * ic.height * 0.7f;
                          DrawRectangleRec({ ic.x + k * ic.width / 16, ic.y + (ic.height - hh) / 2, ic.width / 16 - 2, hh }, Hex(0xffd23f)); }
                      nome = e.nome; sotto = e.file.empty() ? "sintetizzato" : "da file"; break; }
            default: { auto& rg = progetto.regole[i]; DrawRectangleRec(ic, Hex(0x1a1e40));
                       Ui::TestoACapo(rg.chi + " " + rg.evento + " " + rg.conChi, ic.x + 6 * s, ic.y + 6 * s, ic.width - 12 * s, Ui::TestoChiaro(), (int)(12 * s));
                       nome = TextFormat("Regola %d", i + 1); sotto = TextFormat("%d azioni", (int)rg.azioni.size()); break; }
        }
        Ui::Testo(nome, c.x + 8 * s, ic.y + ic.height + 6 * s, WHITE, Ui::TP());
        Ui::Testo(sotto, c.x + 8 * s, ic.y + ic.height + 24 * s, Ui::TestoSpento(), (int)(12 * s));
        if (sopra) {
            float pw = (c.width - 24 * s) / 3, py = ic.y + ic.height - 28 * s;
            if (Ui::Pulsante({ c.x + 8 * s, py, pw, 24 * s }, "Apri", true)) apri = i;
            if (Ui::Pulsante({ c.x + 12 * s + pw, py, pw, 24 * s }, "Copia")) duplica = i;
            if (Ui::Pulsante({ c.x + 16 * s + 2 * pw, py, pw, 24 * s }, "Togli")) elimina = i;
        }
    }
    EndScissorMode();
    auto Vai2 = [&](int i) {
        switch (categoriaLib) {
            case 0: selPersonaggio = i; Vai(Sezione::Personaggi); break;
            case 1: selOggetto = i; Vai(Sezione::Oggetti); break;
            case 2: selScenario = i; schedaScenari = 0; Vai(Sezione::Scenari); break;
            case 3: for (int k = 0; k < 5; k++) if ("#=-^."[k] == progetto.caselle[i].carattere) selCasella = k; schedaScenari = 1; Vai(Sezione::Scenari); break;
            case 4: selBrano = i; schedaAudio = 0; Vai(Sezione::Audio); break;
            case 5: selEffetto = i; schedaAudio = 1; Vai(Sezione::Audio); break;
            default: selRegola = i; Vai(Sezione::Logica); break;
        }
    };
    if (apri >= 0) Vai2(apri);
    else if (duplica >= 0 || elimina >= 0) {
        int i = duplica >= 0 ? duplica : elimina;
        const char* codici[] = { "p", "o", "s", "c", "b", "e", "r" };
        switch (categoriaLib) {
            case 0: selPersonaggio = i; break; case 1: selOggetto = i; break; case 2: selScenario = i; break;
            case 3: for (int k = 0; k < 5; k++) if ("#=-^."[k] == progetto.caselle[i].carattere) selCasella = k; break;
            case 4: selBrano = i; break; case 5: selEffetto = i; break; default: selRegola = i; break;
        }
        std::string c = std::string(codici[categoriaLib]) + ":" + (duplica >= 0 ? (categoriaLib == 3 ? "nuova" : "duplica") : "elimina");
        if (categoriaLib == 3 && duplica >= 0) Avvisa("Le caselle sono una per tipo: si modificano in Scenari > Caselle");
        else AzioneSezione(c);
    }

    // Le raccolte pronte.
    x = dx.x + 20 * s; y = dx.y + 14 * s; w = dx.width - 40 * s;
    Titolo({ x, y, w, 30 * s }, "Raccolte pronte", TextFormat("%d raccolte, tutte in %s", NumeroRaccolte(), NomeStile(progetto.stile)));
    y += 40 * s;
    float hLista = std::min(dx.height * 0.42f, 330 * s);
    Rectangle lista = { x, y, w, hLista };
    float rh = 30 * s;
    int colR = 2;
    float rw = (w - 8 * s) / colR;
    if (Ui::MouseSu(lista)) scorriRaccolte -= GetMouseWheelMove() * 60 * s;
    scorriRaccolte = Limita(scorriRaccolte, 0, std::max(0.0f, ((NumeroRaccolte() + 1) / colR) * (rh + 4 * s) - lista.height));
    BeginScissorMode((int)lista.x, (int)lista.y, (int)lista.width, (int)lista.height);
    for (int i = 0; i < NumeroRaccolte(); i++) {
        Rectangle b = { x + (i % colR) * (rw + 8 * s), y + (i / colR) * (rh + 4 * s) - scorriRaccolte, rw, rh };
        if (b.y + b.height < lista.y || b.y > lista.y + lista.height) continue;
        if (Ui::Pulsante(b, NomeRaccolta(i), raccoltaScelta == i) && CheckCollisionPointRec(GetMousePosition(), lista)) raccoltaScelta = i;
    }
    EndScissorMode();
    y += hLista + 12 * s;
    const Raccolta& rac = CreaRaccolta(raccoltaScelta, progetto.stile);
    Ui::Testo(rac.nome, x, y, Ui::Accento(), Ui::T());
    if (Ui::Pulsante({ x + w - 220 * s, y - 4 * s, 220 * s, 32 * s }, TextFormat("Aggiungi tutto (%d)", rac.Totale()), true)) AzioneLogica("l:raccolta");
    y += 30 * s;
    y += Ui::TestoACapo(rac.descrizione, x, y, w, Ui::TestoSpento(), Ui::TP()) + 8 * s;
    // Anteprima degli elementi: clic per aggiungerne uno solo.
    float lato = 76 * s;
    int per = std::max(1, (int)((w + 6 * s) / (lato + 6 * s)));
    int k = 0;
    auto Casella = [&](const std::string& nome, const std::function<void(Rectangle)>& disegna, const std::function<void()>& aggiungi, bool presente) {
        Rectangle c = { x + (k % per) * (lato + 6 * s), y + (k / per) * (lato + 24 * s), lato, lato };
        k++;
        if (c.y + c.height + 20 * s > dx.y + dx.height) return;
        bool sopra = Ui::MouseSu(c);
        DrawRectangleRounded(c, 0.1f, 6, sopra ? Hex(0x252b5c) : Hex(0x10132c));
        disegna({ c.x + 4 * s, c.y + 4 * s, c.width - 8 * s, c.height - 8 * s });
        if (presente) DrawRectangleRec({ c.x + c.width - 18 * s, c.y + 2 * s, 16 * s, 16 * s }, Hex(0x3aa860));
        std::string corto = nome.size() > 11 ? nome.substr(0, 10) + "." : nome;
        Ui::Testo(corto, c.x, c.y + c.height + 3 * s, sopra ? WHITE : Ui::TestoSpento(), (int)(12 * s));
        if (sopra) Ui::Testo(presente ? "gia' nel gioco" : "clic: aggiungi", c.x, c.y - 14 * s, Ui::Accento(), (int)(11 * s));
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !presente) { Ricorda(); aggiungi(); LibreriaCambiata(); Avvisa(nome + " aggiunto alla libreria del gioco"); gAudio.Suona(Effetto::Moneta, 0.5f); }
    };
    for (auto& p : rac.personaggi) Casella(p.nome, [&](Rectangle q) { p.disegno.DisegnaIn(q, std::max(0, p.disegno.TrovaAnimazione("fermo")), tempo); },
                                           [&]() { progetto.personaggi.push_back(p); }, progetto.Personaggio(p.nome) != nullptr);
    for (auto& o : rac.oggetti) Casella(o.nome, [&](Rectangle q) { o.disegno.DisegnaIn(q, 0, tempo); }, [&]() { progetto.oggetti.push_back(o); }, progetto.Oggetto(o.nome) != nullptr);
    for (auto& c : rac.caselle) Casella(c.nome, [&](Rectangle q) { c.disegno.DisegnaIn(q, 0, tempo); },
                                        [&]() { progetto.caselle.erase(std::remove_if(progetto.caselle.begin(), progetto.caselle.end(), [&](const CasellaDef& d) { return d.carattere == c.carattere; }), progetto.caselle.end());
                                                progetto.caselle.push_back(c); }, false);
    for (auto& sc : rac.scenari) Casella(sc.nome, [&](Rectangle q) {
        Color a = sc.cieloPersonale ? sc.cieloAlto : Hex(0x3f8ff0), b = sc.cieloPersonale ? sc.cieloBasso : Hex(0xb9e3ff);
        DrawRectangleGradientV((int)q.x, (int)q.y, (int)q.width, (int)q.height, a, b);
        if (!sc.strati.empty()) DrawTriangle({ q.x, q.y + q.height }, { q.x + q.width, q.y + q.height }, { q.x + q.width / 2, q.y + q.height * 0.3f }, sc.strati[0].colore);
    }, [&]() { progetto.scenari.push_back(sc); }, progetto.TrovaScenario(sc.nome) != nullptr);
    for (auto& b : rac.brani) Casella(b.nome, [&](Rectangle q) { Ui::TestoCentrato("musica", q, Hex(0x7ee0ff), (int)(12 * s)); },
                                      [&]() { progetto.brani.push_back(b); }, progetto.TrovaBrano(b.nome) != nullptr);
    for (auto& e : rac.effetti) Casella(e.nome, [&](Rectangle q) { Ui::TestoCentrato("suono", q, Hex(0xffd23f), (int)(12 * s)); },
                                        [&]() { progetto.effetti.push_back(e); }, progetto.TrovaEffetto(e.nome) != nullptr);
}

// --- Comandi ---------------------------------------------------------------------------------------------

void Officina::Comandi(Rectangle r, float dt) {
    float s = Ui::S();
    Mappatura& mp = gInput.m;
    int joy = gInput.Joypad();
    gInput.Aggiorna(1, { 0, 0 });
    if (attesaComando) {
        tAttesa += dt;
        int tasto = GetKeyPressed();
        if (tasto == KEY_ESCAPE) attesaComando = false;
        else if (colonnaComando < 2 && tasto > 0) { mp.tasti[rigaComando][colonnaComando] = tasto; attesaComando = false; gInput.Salva(); gAudio.Suona(Effetto::Gemma); }
        else if (colonnaComando == 2 && joy >= 0)
            for (int b = 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++)
                if (IsGamepadButtonPressed(joy, b)) { mp.pulsanti[rigaComando] = b; attesaComando = false; gInput.Salva(); gAudio.Suona(Effetto::Gemma); break; }
        if (tAttesa > 8) attesaComando = false;
    }
    float x = r.x + 30 * s, y = r.y + 20 * s;
    Titolo({ x, y, r.width, 30 * s }, "Comandi e periferiche", "valgono per lo studio e per tutti i giochi creati (comandi.cfg)");
    y += 50 * s;
    float wa = 200 * s, wc = 190 * s, h = 36 * s;
    const char* colonne[3] = { "Tasto 1", "Tasto 2", "Joypad" };
    for (int c = 0; c < 3; c++) Ui::Testo(colonne[c], x + wa + c * (wc + 8 * s), y, Ui::Accento(), Ui::TP());
    y += 26 * s;
    for (int i = 0; i < NUM_AZIONI; i++) {
        bool attiva = gInput.Tenuto((::Azione)i);
        Rectangle et = { x, y, wa - 10 * s, h };
        DrawRectangleRounded(et, 0.2f, 6, attiva ? Hex(0x2a6a3a) : Hex(0x141a40));
        Ui::Testo(NomeAzione((::Azione)i), et.x + 12 * s, et.y + (h - Ui::T()) / 2, attiva ? Hex(0x8cff6a) : WHITE, Ui::T());
        for (int c = 0; c < 3; c++) {
            Rectangle cel = { x + wa + c * (wc + 8 * s), y, wc, h };
            bool sel = i == rigaComando && c == colonnaComando && attesaComando;
            std::string t = sel ? (std::fmod(tempo, 0.6f) < 0.3f ? "premi il nuovo..." : "") : c < 2 ? NomeTasto(mp.tasti[i][c]) : NomePulsante(mp.pulsanti[i]);
            if (Ui::Pulsante(cel, t, sel)) { rigaComando = i; colonnaComando = c; attesaComando = true; tAttesa = 0; }
        }
        if (Ui::Pulsante({ x + wa + 3 * (wc + 8 * s), y, 40 * s, h }, "x")) { mp.tasti[i][1] = 0; gInput.Salva(); }
        y += h + 6 * s;
    }
    y += 10 * s;
    bool mouse = mp.mouseNelGioco;
    if (Ui::Interruttore({ x, y, wa + 2 * wc, h }, "Mouse nel gioco (sinistro: cammina verso il puntatore, destro: salta)", mouse)) { mp.mouseNelGioco = mouse; gInput.Salva(); }
    y += h + 8 * s;
    float zm = mp.zonaMorta;
    if (Ui::Numero("k_zona", { x, y, wa + 2 * wc, h }, "Zona morta delle levette", zm, 0.05f, 0.8f, 0.05f, "%.2f")) { mp.zonaMorta = zm; gInput.Salva(); }
    y += h + 14 * s;
    if (Ui::Pulsante({ x, y, 260 * s, h }, "Ripristina i predefiniti")) AzioneLogica("k:predefiniti");
    y += h + 20 * s;
    // Comandi aggiunti dalle regole del gioco (tasti e pulsanti).
    std::string extra;
    for (auto& rg : progetto.regole)
        if (rg.attiva && (rg.evento == "tasto" || rg.evento == "joypad" || rg.evento == "clic"))
            extra += (rg.evento == "tasto" ? "Tasto " + rg.conChi : rg.evento == "joypad" ? "Pulsante " + rg.conChi : std::string("Clic")) + ": " +
                     (rg.azioni.empty() ? std::string("-") : NomiAzione()[Indice(TipiAzione(), rg.azioni[0].tipo)] + " " + rg.azioni[0].testo) + "\n";
    Ui::Testo("Comandi del tuo gioco (dalle regole della Logica)", x, y, Ui::Accento(), Ui::TP());
    y += 24 * s;
    Ui::TestoACapo(extra.empty() ? "Nessuno: aggiungi una regola con l'evento \"tasto premuto\" per creare un comando." : extra, x, y, wa + 3 * wc, Ui::TestoChiaro(), Ui::TP());

    // Prova dal vivo a destra.
    Rectangle prova = { x + wa + 3 * (wc + 8 * s) + 70 * s, r.y + 70 * s, 0, 420 * s };
    prova.width = r.x + r.width - prova.x - 30 * s;
    if (prova.width < 220 * s) return;
    DrawRectangleRounded(prova, 0.04f, 6, Hex(0x0b0d22));
    DrawRectangleRoundedLinesEx(prova, 0.04f, 6, 1, Ui::Bordo());
    float px = prova.x + 20 * s, py = prova.y + 16 * s;
    Ui::Testo("Prova dal vivo", px, py, Ui::Accento(), Ui::T());
    py += 32 * s;
    Ui::Testo(TextFormat("Joypad: %s", joy >= 0 ? gInput.NomeJoypad() : "nessuno collegato"), px, py, joy >= 0 ? Hex(0x8cff6a) : Hex(0xff8a8a), Ui::TP());
    py += 22 * s;
    const char* ultime[3] = { "tastiera", "mouse", "joypad" };
    Ui::Testo(TextFormat("Ultima periferica: %s", ultime[(int)gInput.Ultima()]), px, py, Ui::TestoChiaro(), Ui::TP());
    py += 40 * s;
    float l = 50 * s;
    Vector2 cr = { px + 1.5f * l, py + 1.5f * l };
    const char* frecce[4] = { "<", ">", "^", "v" };
    Vector2 pos[4] = { { cr.x - l, cr.y }, { cr.x + l, cr.y }, { cr.x, cr.y - l }, { cr.x, cr.y + l } };
    for (int k = 0; k < 4; k++) {
        bool on = gInput.Tenuto((::Azione)k);
        Rectangle b = { pos[k].x - l / 2 + 2, pos[k].y - l / 2 + 2, l - 4, l - 4 };
        DrawRectangleRounded(b, 0.2f, 6, on ? Ui::Accento() : Hex(0x1a2050));
        Ui::TestoCentrato(frecce[k], b, on ? BLACK : WHITE, Ui::T());
    }
    const char* tasti[4] = { "Salto", "OK", "Indietro", "Pausa" };
    for (int k = 0; k < 4; k++) {
        bool on = gInput.Tenuto((::Azione)(4 + k));
        Rectangle b = { px + 3.6f * l, py + k * (l * 0.75f), prova.x + prova.width - px - 3.6f * l - 20 * s, l * 0.65f };
        DrawRectangleRounded(b, 0.3f, 6, on ? Hex(0xff5c8a) : Hex(0x1a2050));
        Ui::TestoCentrato(tasti[k], b, WHITE, Ui::TP());
    }
    py += 3.4f * l;
    float a = gInput.Asse();
    Rectangle barra = { px, py, prova.width - 40 * s, 12 * s };
    DrawRectangleRec(barra, Hex(0x1a2050));
    DrawRectangleRec({ barra.x + barra.width / 2 + std::min(0.0f, a) * barra.width / 2, barra.y, std::fabs(a) * barra.width / 2, barra.height }, Hex(0x8cff6a));
    Ui::Testo("asse orizzontale (anche analogico)", px, py + 18 * s, Ui::TestoSpento(), (int)(12 * s));
    if (joy >= 0) {
        Vector2 c = { prova.x + prova.width - 60 * s, prova.y + 60 * s };
        DrawCircleLinesV(c, 30 * s, Hex(0x5a6aa0));
        DrawCircleLinesV(c, 30 * s * mp.zonaMorta, Hex(0x3a4070));
        DrawCircleV({ c.x + gInput.LevettaX() * 30 * s, c.y + gInput.LevettaY() * 30 * s }, 6 * s, Ui::Accento());
    }
}
