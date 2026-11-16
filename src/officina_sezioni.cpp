// Le sezioni dell'officina: Mondi, Personaggi, Oggetti, Scenari (con le caselle).
#include "officina.h"
#include "audio.h"
#include "claude_progetto.h"
#include "raccolte.h"
#include "ui.h"
#include <algorithm>

namespace {

// Colonna di proprieta' che scorre: dà i rettangoli delle righe e dice se sono visibili.
struct Colonna {
    Rectangle area;
    float y, s, scorri;
    Colonna(Rectangle a, float sc, float scorrimento) : area(a), y(a.y - scorrimento), s(sc), scorri(scorrimento) {}
    Rectangle Riga(float h) { Rectangle r = { area.x, y, area.width, h * s }; y += h * s + 6 * s; return r; }
    bool Vede(Rectangle r) const { return r.y >= area.y - 1 && r.y + r.height <= area.y + area.height + 1; }
    void Spazio(float h) { y += h * s; }
    void Sezione(const std::string& t) {
        y += 6 * s;
        Rectangle r = { area.x, y, area.width, 20 * s };
        if (Vede(r)) { Ui::Testo(t, area.x, y, Ui::Accento(), Ui::TP()); DrawLineEx({ area.x, y + 19 * s }, { area.x + area.width, y + 19 * s }, 1, Ui::Bordo()); }
        y += 26 * s;
    }
    float Altezza() const { return y + scorri - area.y; }
};

std::vector<std::string> Con(const std::vector<std::string>& v, const std::string& primo) {
    std::vector<std::string> r{ primo };
    r.insert(r.end(), v.begin(), v.end());
    return r;
}

int Indice(const std::vector<std::string>& v, const std::string& x) {
    for (int i = 0; i < (int)v.size(); i++) if (v[i] == x) return i;
    return 0;
}

const char* CARATTERI_CASELLE = "#=-^.";
const char* NOMI_CASELLE[] = { "Terra con erba", "Pietra / muro", "Piattaforma", "Spine", "Bersaglio" };

} // namespace

// --- Elementi comuni --------------------------------------------------------------------------------

void Officina::Titolo(Rectangle r, const std::string& t, const std::string& sotto) {
    float s = Ui::S();
    Ui::TestoForte(t, r.x, r.y, WHITE, (int)(22 * s));
    if (!sotto.empty()) Ui::Testo(sotto, r.x + Ui::LarghezzaForte(t, (int)(22 * s)) + 14 * s, r.y + 6 * s, Ui::TestoSpento(), Ui::TP());
}

float Officina::Riga(Rectangle area, float& y, float h) const {
    y += h > 0 ? h : 34 * Ui::S();
    return area.x;
}

int Officina::Elenco(Rectangle r, const std::string& titolo, std::vector<std::string> nomi, int& scelto,
                     const std::function<void(int, Rectangle)>& icona, bool puoNuovo) {
    float s = Ui::S();
    DrawRectangleRec(r, Ui::Pannello());
    DrawLineEx({ r.x + r.width, r.y }, { r.x + r.width, r.y + r.height }, 1, Ui::Bordo());
    float pad = 8 * s;
    Ui::Testo(titolo, r.x + pad, r.y + pad, Ui::Accento(), Ui::TP());
    Ui::Testo(std::to_string(nomi.size()), r.x + r.width - pad - Ui::Larghezza(std::to_string(nomi.size()), Ui::TP()), r.y + pad, Ui::TestoSpento(), Ui::TP());
    float hb = 30 * s;
    Rectangle lista = { r.x, r.y + pad + 24 * s, r.width, r.height - (pad + 24 * s) - (puoNuovo ? hb + 2 * pad : pad) };
    float h = 56 * s;
    if (Ui::MouseSu(lista)) scorriLista -= GetMouseWheelMove() * 60 * s;
    scorriLista = Limita(scorriLista, 0, std::max(0.0f, nomi.size() * (h + 4 * s) - lista.height));
    scelto = (int)Limita((float)scelto, 0, std::max(0.0f, (float)nomi.size() - 1));
    BeginScissorMode((int)lista.x, (int)lista.y, (int)lista.width, (int)lista.height);
    for (int i = 0; i < (int)nomi.size(); i++) {
        Rectangle b = { r.x + pad, lista.y + i * (h + 4 * s) - scorriLista, r.width - 2 * pad, h };
        if (b.y + b.height < lista.y || b.y > lista.y + lista.height) continue;
        bool sel = i == scelto, sopra = Ui::MouseSu(b) && CheckCollisionPointRec(GetMousePosition(), lista);
        DrawRectangleRounded(b, 0.15f, 6, sel ? Ui::Selezione() : sopra ? Hex(0x252b5c) : Hex(0x10132c));
        if (sel) DrawRectangleRoundedLinesEx(b, 0.15f, 6, 2, Ui::Accento());
        Rectangle ic = { b.x + 4 * s, b.y + 4 * s, h - 8 * s, h - 8 * s };
        DrawRectangleRec(ic, Hex(0x2a2e50));
        if (icona) icona(i, ic);
        Ui::TestoACapo(nomi[i], ic.x + ic.width + 8 * s, b.y + 8 * s, b.width - ic.width - 20 * s, sel ? WHITE : Ui::TestoChiaro(), Ui::TP());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) { scelto = i; scorriProprieta = 0; gAudio.Suona(Effetto::Menu, 0.3f); }
    }
    EndScissorMode();
    if (!puoNuovo) return 0;
    float bw = (r.width - 4 * pad) / 3, y = r.y + r.height - hb - pad;
    if (Ui::Pulsante({ r.x + pad, y, bw, hb }, "+ Nuovo", true)) return 1;
    if (Ui::Pulsante({ r.x + 2 * pad + bw, y, bw, hb }, "Duplica", false, !nomi.empty())) return 2;
    if (Ui::Pulsante({ r.x + 3 * pad + 2 * bw, y, bw, hb }, "Elimina", false, !nomi.empty())) return 3;
    return 0;
}

// Quando un elemento cambia nome, si aggiornano tutti i riferimenti nel gioco.
static void RinominaOvunque(ProgettoGioco& p, const std::string& vecchio, const std::string& nuovo) {
    if (vecchio == nuovo) return;
    for (auto& m : p.mondo.mappe) {
        for (auto& o : m.oggetti) if (o.tipo == vecchio) o.tipo = nuovo;
        if (m.scenarioLibreria == vecchio) m.scenarioLibreria = nuovo;
        if (m.brano == vecchio) m.brano = nuovo;
    }
    if (p.eroe == vecchio) p.eroe = nuovo;
    for (auto& x : p.personaggi) {
        if (x.comportamento.proiettile == vecchio) x.comportamento.proiettile = nuovo;
        for (auto& sn : x.suoni) if (sn.second == vecchio) sn.second = nuovo;
    }
    for (auto& o : p.oggetti) {
        if (o.richiede == vecchio) o.richiede = nuovo;
        if (o.suono == vecchio) o.suono = nuovo;
    }
    for (auto& r : p.regole) {
        if (r.chi == vecchio) r.chi = nuovo;
        if (r.conChi == vecchio) r.conChi = nuovo;
        for (auto& a : r.azioni) if (a.testo == vecchio) a.testo = nuovo;
    }
    for (auto& e : p.suoniEventi) if (e.second == vecchio) e.second = nuovo;
}

// --- Mondi ------------------------------------------------------------------------------------------

void Officina::Mondi(Rectangle r, float dt) {
    editorMondo.area = r;
    editorMondo.Frame(dt);
    if (editorMondo.Modificato()) modificato = true;
}

// --- Azioni dai menu delle sezioni ---------------------------------------------------------------------

void Officina::AzioneSezione(const std::string& c) {
    auto MettiInMappa = [&](const std::string& nome) {
        SincronizzaMondo();
        int mi = std::min(editorMondo.MappaAperta(), (int)progetto.mondo.mappe.size() - 1);
        if (mi < 0) return;
        Ricorda();
        Mappa& m = progetto.mondo.mappe[mi];
        OggettoMappa o;
        o.tipo = nome;
        o.x = m.partenza.x + 3 * TILE;
        o.y = m.partenza.y;
        m.oggetti.push_back(o);
        MondoCambiato();
        Avvisa(nome + " messo nella mappa \"" + m.nome + "\" vicino alla partenza: spostalo nella sezione Mondi");
    };
    if (c == "p:nuovo" || c == "p:duplica" || c == "p:elimina") {
        Ricorda();
        if (c == "p:nuovo") {
            PersonaggioDef p;
            p.nome = progetto.NomeLibero("Personaggio");
            bool hd = progetto.stile == StileGrafico::Disegnato;
            p.disegno.Nuovo(hd ? 64 : 32, hd ? 64 : 32, hd ? 4.0f : 2.0f);
            p.disegno.liscio = hd;
            p.disegno.animazioni[0].nome = "fermo";
            progetto.personaggi.push_back(p);
            selPersonaggio = (int)progetto.personaggi.size() - 1;
            Avvisa("Nuovo personaggio: disegnalo o chiedi a Claude di farlo!");
        } else if (selPersonaggio < (int)progetto.personaggi.size()) {
            if (c == "p:duplica") {
                PersonaggioDef p = progetto.personaggi[selPersonaggio];
                p.nome = progetto.NomeLibero(p.nome);
                progetto.personaggi.push_back(p);
                selPersonaggio = (int)progetto.personaggi.size() - 1;
            } else {
                std::string n = progetto.personaggi[selPersonaggio].nome;
                progetto.personaggi.erase(progetto.personaggi.begin() + selPersonaggio);
                for (auto& m : progetto.mondo.mappe) m.oggetti.erase(std::remove_if(m.oggetti.begin(), m.oggetti.end(), [&](const OggettoMappa& o) { return o.tipo == n; }), m.oggetti.end());
                if (progetto.eroe == n) progetto.eroe.clear();
                selPersonaggio = std::max(0, selPersonaggio - 1);
                MondoCambiato();
                Avvisa(n + " eliminato (anche dalle mappe)");
            }
        }
        editorDisegno.Chiudi();
        LibreriaCambiata();
    } else if (c == "p:eroe" && selPersonaggio < (int)progetto.personaggi.size()) {
        Ricorda();
        progetto.eroe = progetto.personaggi[selPersonaggio].nome;
        progetto.personaggi[selPersonaggio].comportamento.tipo = TipoComportamento::Giocatore;
        LibreriaCambiata();
        Avvisa(progetto.eroe + " ora e' l'eroe del gioco");
    } else if (c == "p:mappa" && selPersonaggio < (int)progetto.personaggi.size()) {
        MettiInMappa(progetto.personaggi[selPersonaggio].nome);
    } else if (c == "o:nuovo" || c == "o:duplica" || c == "o:elimina") {
        Ricorda();
        if (c == "o:nuovo") {
            OggettoDef o;
            o.nome = progetto.NomeLibero("Oggetto");
            bool hd = progetto.stile == StileGrafico::Disegnato;
            o.disegno.Nuovo(hd ? 64 : 32, hd ? 64 : 32, hd ? 4.0f : 2.0f);
            o.disegno.liscio = hd;
            o.disegno.animazioni[0].nome = "fermo";
            progetto.oggetti.push_back(o);
            selOggetto = (int)progetto.oggetti.size() - 1;
        } else if (selOggetto < (int)progetto.oggetti.size()) {
            if (c == "o:duplica") {
                OggettoDef o = progetto.oggetti[selOggetto];
                o.nome = progetto.NomeLibero(o.nome);
                progetto.oggetti.push_back(o);
                selOggetto = (int)progetto.oggetti.size() - 1;
            } else {
                std::string n = progetto.oggetti[selOggetto].nome;
                progetto.oggetti.erase(progetto.oggetti.begin() + selOggetto);
                for (auto& m : progetto.mondo.mappe) m.oggetti.erase(std::remove_if(m.oggetti.begin(), m.oggetti.end(), [&](const OggettoMappa& o) { return o.tipo == n; }), m.oggetti.end());
                selOggetto = std::max(0, selOggetto - 1);
                MondoCambiato();
            }
        }
        editorDisegno.Chiudi();
        LibreriaCambiata();
    } else if (c == "o:mappa" && selOggetto < (int)progetto.oggetti.size()) {
        MettiInMappa(progetto.oggetti[selOggetto].nome);
    } else if (c == "s:nuovo" || c == "s:duplica" || c == "s:elimina") {
        Ricorda();
        schedaScenari = 0;
        if (c == "s:nuovo") {
            Scenario sc;
            sc.nome = progetto.NomeLibero("Scenario");
            sc.autore = "";
            progetto.scenari.push_back(sc);
            selScenario = (int)progetto.scenari.size() - 1;
        } else if (selScenario < (int)progetto.scenari.size()) {
            if (c == "s:duplica") {
                Scenario sc = progetto.scenari[selScenario];
                sc.nome = progetto.NomeLibero(sc.nome);
                progetto.scenari.push_back(sc);
                selScenario = (int)progetto.scenari.size() - 1;
            } else {
                std::string n = progetto.scenari[selScenario].nome;
                progetto.scenari.erase(progetto.scenari.begin() + selScenario);
                for (auto& m : progetto.mondo.mappe) if (m.scenarioLibreria == n) m.scenarioLibreria.clear();
                selScenario = std::max(0, selScenario - 1);
                MondoCambiato();
            }
        }
    } else if (c == "s:usa" && selScenario < (int)progetto.scenari.size()) {
        SincronizzaMondo();
        Ricorda();
        int mi = std::min(editorMondo.MappaAperta(), (int)progetto.mondo.mappe.size() - 1);
        progetto.mondo.mappe[mi].scenarioLibreria = progetto.scenari[selScenario].nome;
        MondoCambiato();
        Avvisa("La mappa \"" + progetto.mondo.mappe[mi].nome + "\" ora usa lo scenario " + progetto.scenari[selScenario].nome);
    } else if (c == "s:strato" && selScenario < (int)progetto.scenari.size()) {
        Ricorda();
        StratoSfondo st;
        st.nome = "Strato";
        progetto.scenari[selScenario].strati.push_back(st);
        stratoScelto = (int)progetto.scenari[selScenario].strati.size() - 1;
    } else if (c == "c:apri") {
        schedaScenari = 1;
    } else if (c == "c:nuova" || c == "c:elimina") {
        schedaScenari = 1;
        char car = CARATTERI_CASELLE[std::min(selCasella, 4)];
        Ricorda();
        if (c == "c:elimina") {
            progetto.caselle.erase(std::remove_if(progetto.caselle.begin(), progetto.caselle.end(), [&](const CasellaDef& d) { return d.carattere == car; }), progetto.caselle.end());
            Avvisa("La casella torna al disegno del motore");
        } else if (!progetto.Casella(car)) {
            CasellaDef d;
            d.carattere = car;
            d.nome = NOMI_CASELLE[std::min(selCasella, 4)];
            bool hd = progetto.stile == StileGrafico::Disegnato;
            d.disegno.Nuovo(hd ? 64 : 32, hd ? 64 : 32, hd ? 4.0f : 2.0f);
            d.disegno.liscio = hd;
            d.disegno.animazioni[0].nome = "superficie";
            Animazione interno = d.disegno.animazioni[0];
            interno.nome = "interno";
            d.disegno.animazioni.push_back(interno);
            progetto.caselle.push_back(d);
        }
        editorDisegno.Chiudi();
        LibreriaCambiata();
    } else {
        // Musica ed effetti (officina_audio.cpp); regole, librerie e comandi (officina_logica.cpp).
        if (!c.empty() && (c[0] == 'b' || c[0] == 'e')) AzioneAudio(c);
        else AzioneLogica(c);
    }
}

// --- Personaggi ------------------------------------------------------------------------------------

void Officina::Personaggi(Rectangle r, float dt) {
    float s = Ui::S();
    Rectangle sx = { r.x, r.y, 250 * s, r.height };
    Rectangle dx = { r.x + r.width - 330 * s, r.y, 330 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width - dx.width, r.height };
    std::vector<std::string> nomi;
    for (auto& p : progetto.personaggi) nomi.push_back(p.nome + (p.nome == progetto.eroe ? "  (eroe)" : "") + "\n" + NomeComportamento(p.comportamento.tipo));
    int az = Elenco(sx, "PERSONAGGI DEL GIOCO", nomi, selPersonaggio, [&](int i, Rectangle q) {
        const Disegno& d = progetto.personaggi[i].disegno;
        d.DisegnaIn(q, std::max(0, d.TrovaAnimazione("fermo")), tempo);
    });
    if (az == 1) AzioneSezione("p:nuovo");
    if (az == 2) AzioneSezione("p:duplica");
    if (az == 3) AzioneSezione("p:elimina");
    if (progetto.personaggi.empty()) {
        Ui::TestoACapo("Nessun personaggio. Premi \"+ Nuovo\" per disegnarne uno, trascina qui un PNG, oppure chiedi a Claude: "
                       "\"disegna un drago verde che sputa fuoco\".", centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    selPersonaggio = std::min(selPersonaggio, (int)progetto.personaggi.size() - 1);
    PersonaggioDef& p = progetto.personaggi[selPersonaggio];
    editorDisegno.Apri(&p.disegno, "p:" + p.nome);
    if (editorDisegno.Frame(centro, dt)) modificato = true;

    // Proprieta' e comportamento.
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    Rectangle area = { dx.x + 12 * s, dx.y + 10 * s, dx.width - 24 * s, dx.height - 20 * s };
    if (Ui::MouseSu(area)) scorriProprieta = std::max(0.0f, scorriProprieta - GetMouseWheelMove() * 50 * s);
    PersonaggioDef prima = p;
    bool cambiato = false;
    Colonna col(area, s, scorriProprieta);
    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    col.Sezione("PERSONAGGIO");
    Rectangle q = col.Riga(30);
    std::string nome = p.nome;
    if (col.Vede(q) && Ui::Campo("p_nome", q, nome, 40) && !nome.empty() && nome != p.nome) {
        if (progetto.Personaggio(nome) || progetto.Oggetto(nome)) Avvisa("C'e' gia' un elemento chiamato " + nome);
        else { Ricorda(); RinominaOvunque(progetto, p.nome, nome); p.nome = nome; editorDisegno.Apri(&p.disegno, "p:" + p.nome); MondoCambiato(); LibreriaCambiata(); prima = p; }
    }
    q = col.Riga(30);
    if (col.Vede(q)) {
        float w2 = (q.width - 6 * s) / 2;
        if (Ui::Pulsante({ q.x, q.y, w2, q.height }, p.nome == progetto.eroe ? "E' l'eroe" : "Usa come eroe", p.nome == progetto.eroe)) AzioneSezione("p:eroe");
        if (Ui::Pulsante({ q.x + w2 + 6 * s, q.y, w2, q.height }, "Metti in mappa")) AzioneSezione("p:mappa");
    }
    if (!p.autore.empty()) { q = col.Riga(18); if (col.Vede(q)) Ui::Testo("Autore: " + p.autore, q.x, q.y, Ui::TestoSpento(), Ui::TP()); }
    col.Sezione("COMPORTAMENTO");
    Comportamento& c = p.comportamento;
    std::vector<std::string> tipi;
    for (int i = 0; i < (int)TipoComportamento::Totale; i++) tipi.push_back(NomeComportamento((TipoComportamento)i));
    int t = (int)c.tipo;
    q = col.Riga(30);
    if (col.Vede(q) && Ui::Scelta(q, "Tipo", t, tipi)) { c.tipo = (TipoComportamento)t; cambiato = true; }
    auto Num = [&](const char* id, const char* et, float& v, float lo, float hi, float passo, const char* f = "%.0f") {
        Rectangle rr = col.Riga(28);
        if (col.Vede(rr) && Ui::Numero(id, rr, et, v, lo, hi, passo, f)) cambiato = true;
    };
    auto NumI = [&](const char* id, const char* et, int& v, int lo, int hi) {
        Rectangle rr = col.Riga(28);
        if (col.Vede(rr) && Ui::NumeroIntero(id, rr, et, v, lo, hi)) cambiato = true;
    };
    auto Leva = [&](const char* et, bool& v) {
        Rectangle rr = col.Riga(28);
        if (col.Vede(rr) && Ui::Interruttore(rr, et, v)) cambiato = true;
    };
    Num("p_vel", "Velocita'", c.velocita, 0, 400, 10);
    Num("p_salto", "Salto", c.salto, 0, 600, 20);
    NumI("p_en", "Energia (colpi)", c.energia, 1, 99);
    NumI("p_danno", "Danno (cuori)", c.danno, 0, 9);
    Num("p_vista", "Vista (pixel)", c.vista, 0, 600, 20);
    Num("p_att", "Attacco ogni (s)", c.attaccoOgni, 0.2f, 20, 0.2f, "%.1f");
    NumI("p_punti", "Punti", c.punti, 0, 99999);
    Leva("Gravita'", c.gravita);
    Leva("Si sconfigge saltandoci sopra", c.calpestabile);
    std::vector<std::string> proiettili;
    for (auto& o : progetto.oggetti) if (o.funzione == FunzioneOggetto::Proiettile) proiettili.push_back(o.nome);
    proiettili = Con(proiettili, "Palla di fuoco (motore)");
    int pi = c.proiettile.empty() ? 0 : Indice(proiettili, c.proiettile);
    q = col.Riga(30);
    if (col.Vede(q) && Ui::Scelta(q, "Proiettile", pi, proiettili)) { c.proiettile = pi == 0 ? "" : proiettili[pi]; cambiato = true; }
    if (c.tipo == TipoComportamento::Parla) {
        q = col.Riga(18);
        if (col.Vede(q)) Ui::Testo("Cosa dice:", q.x, q.y, Ui::TestoSpento(), Ui::TP());
        q = col.Riga(30);
        if (col.Vede(q) && Ui::Campo("p_dialogo", q, c.dialogo, 300)) cambiato = true;
    }
    col.Sezione("SUONI");
    std::vector<std::string> effetti;
    for (auto& e : progetto.effetti) effetti.push_back(e.nome);
    effetti = Con(effetti, "(nessuno)");
    const char* eventi[] = { "salto", "colpito", "attacco", "sconfitto" };
    for (auto ev : eventi) {
        int k = p.suoni.count(ev) ? Indice(effetti, p.suoni[ev]) : 0;
        q = col.Riga(28);
        if (col.Vede(q) && Ui::Scelta(q, ev, k, effetti)) {
            if (k == 0) p.suoni.erase(ev); else p.suoni[ev] = effetti[k];
            cambiato = true;
            if (k > 0) if (auto* e = progetto.TrovaEffetto(effetti[k])) gAudio.SuonaEffetto(*e);
        }
    }
    col.Sezione("ANIMAZIONI USATE DAL MOTORE");
    q = col.Riga(64);
    if (col.Vede(q))
        Ui::TestoACapo("fermo, corsa, salto, caduta, colpito, attacco, vittoria. Se ne manca una si usa \"fermo\". "
                       "Il disegno guarda a destra: il motore lo specchia.", q.x, q.y, q.width, Ui::TestoSpento(), (int)(13 * s));
    EndScissorMode();
    scorriProprieta = std::min(scorriProprieta, std::max(0.0f, col.Altezza() - area.height));
    if (cambiato) {
        ProgettoGioco vecchio = progetto;
        for (auto& x : vecchio.personaggi) if (x.nome == p.nome) x = prima;
        annulla.push_back(vecchio);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
    }
}

// --- Oggetti --------------------------------------------------------------------------------------------

void Officina::Oggetti(Rectangle r, float dt) {
    float s = Ui::S();
    Rectangle sx = { r.x, r.y, 250 * s, r.height };
    Rectangle dx = { r.x + r.width - 330 * s, r.y, 330 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width - dx.width, r.height };
    std::vector<std::string> nomi;
    for (auto& o : progetto.oggetti) nomi.push_back(o.nome + "\n" + NomeFunzione(o.funzione));
    int az = Elenco(sx, "OGGETTI DEL GIOCO", nomi, selOggetto, [&](int i, Rectangle q) {
        const Disegno& d = progetto.oggetti[i].disegno;
        d.DisegnaIn(q, std::max(0, d.TrovaAnimazione("fermo")), tempo);
    });
    if (az == 1) AzioneSezione("o:nuovo");
    if (az == 2) AzioneSezione("o:duplica");
    if (az == 3) AzioneSezione("o:elimina");
    if (progetto.oggetti.empty()) {
        Ui::TestoACapo("Nessun oggetto. Premi \"+ Nuovo\" oppure chiedi a Claude: \"crea una chiave d'oro e una porta che si apre con quella chiave\".",
                       centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    selOggetto = std::min(selOggetto, (int)progetto.oggetti.size() - 1);
    OggettoDef& o = progetto.oggetti[selOggetto];
    editorDisegno.Apri(&o.disegno, "o:" + o.nome);
    if (editorDisegno.Frame(centro, dt)) modificato = true;

    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    Rectangle area = { dx.x + 12 * s, dx.y + 10 * s, dx.width - 24 * s, dx.height - 20 * s };
    if (Ui::MouseSu(area)) scorriProprieta = std::max(0.0f, scorriProprieta - GetMouseWheelMove() * 50 * s);
    OggettoDef prima = o;
    bool cambiato = false;
    Colonna col(area, s, scorriProprieta);
    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    col.Sezione("OGGETTO");
    Rectangle q = col.Riga(30);
    std::string nome = o.nome;
    if (col.Vede(q) && Ui::Campo("o_nome", q, nome, 40) && !nome.empty() && nome != o.nome) {
        if (progetto.Personaggio(nome) || progetto.Oggetto(nome)) Avvisa("C'e' gia' un elemento chiamato " + nome);
        else { Ricorda(); RinominaOvunque(progetto, o.nome, nome); o.nome = nome; editorDisegno.Apri(&o.disegno, "o:" + o.nome); MondoCambiato(); LibreriaCambiata(); prima = o; }
    }
    q = col.Riga(30);
    if (col.Vede(q) && Ui::Pulsante(q, "Metti nella mappa aperta")) AzioneSezione("o:mappa");
    col.Sezione("FUNZIONE NEL GIOCO");
    std::vector<std::string> funz;
    for (int i = 0; i < (int)FunzioneOggetto::Totale; i++) funz.push_back(NomeFunzione((FunzioneOggetto)i));
    int f = (int)o.funzione;
    q = col.Riga(30);
    if (col.Vede(q) && Ui::Scelta(q, "Funzione", f, funz)) { o.funzione = (FunzioneOggetto)f; cambiato = true; }
    const char* spiega = "";
    const char* etValore = "Valore";
    switch (o.funzione) {
        case FunzioneOggetto::Decorazione: spiega = "Solo bello da vedere (se e' solido ci si cammina sopra)."; break;
        case FunzioneOggetto::Raccogli: spiega = "Si raccoglie toccandolo: da' punti e aumenta la variabile."; etValore = "Punti"; break;
        case FunzioneOggetto::Cura: spiega = "Si raccoglie: ridà cuori all'eroe."; etValore = "Cuori"; break;
        case FunzioneOggetto::Chiave: spiega = "Va nell'inventario: apre le porte che la richiedono."; etValore = "Punti"; break;
        case FunzioneOggetto::Porta: spiega = "Blocca il passaggio finche' l'eroe non ha la chiave (o l'interruttore e' acceso). Animazioni \"chiusa\" e \"aperta\"."; break;
        case FunzioneOggetto::Molla: spiega = "Fa rimbalzare in alto l'eroe."; etValore = "Spinta"; break;
        case FunzioneOggetto::Cassa: spiega = "Si spinge camminandoci contro."; break;
        case FunzioneOggetto::Interruttore: spiega = "Toccandolo si accende e apre le porte che lo richiedono (per nome). Animazioni \"spenta\" e \"accesa\"."; break;
        case FunzioneOggetto::PiattaformaMobile: spiega = "Si muove avanti e indietro e porta l'eroe."; break;
        case FunzioneOggetto::Pericolo: spiega = "Fa male a chi lo tocca."; etValore = "Danno"; break;
        case FunzioneOggetto::Arrivo: spiega = "Toccandolo si vince il livello."; etValore = "Punti"; break;
        case FunzioneOggetto::Proiettile: spiega = "Lanciato dai personaggi (o dall'eroe) che lo scelgono come proiettile."; etValore = "Danno"; break;
        default: break;
    }
    q = col.Riga(46);
    if (col.Vede(q)) Ui::TestoACapo(spiega, q.x, q.y, q.width, Ui::TestoSpento(), (int)(13 * s));
    q = col.Riga(28);
    if (col.Vede(q) && Ui::NumeroIntero("o_val", q, etValore, o.valore, -999, 99999)) cambiato = true;
    if (o.funzione == FunzioneOggetto::Porta) {
        std::vector<std::string> apre;
        for (auto& x : progetto.oggetti) if (x.funzione == FunzioneOggetto::Chiave || x.funzione == FunzioneOggetto::Interruttore) apre.push_back(x.nome);
        apre = Con(apre, "(sempre aperta)");
        int k = o.richiede.empty() ? 0 : Indice(apre, o.richiede);
        q = col.Riga(30);
        if (col.Vede(q) && Ui::Scelta(q, "Si apre con", k, apre)) { o.richiede = k == 0 ? "" : apre[k]; cambiato = true; }
    }
    auto Leva = [&](const char* et, bool& v) { Rectangle rr = col.Riga(28); if (col.Vede(rr) && Ui::Interruttore(rr, et, v)) cambiato = true; };
    Leva("Solido (ci si sale sopra)", o.solido);
    Leva("Cade (gravita')", o.gravita);
    if (o.funzione == FunzioneOggetto::PiattaformaMobile || o.funzione == FunzioneOggetto::Proiettile) {
        q = col.Riga(28);
        if (col.Vede(q) && Ui::Numero("o_vel", q, "Velocita'", o.velocita, 0, 600, 10)) cambiato = true;
    }
    if (o.funzione == FunzioneOggetto::PiattaformaMobile) {
        q = col.Riga(28);
        if (col.Vede(q) && Ui::Numero("o_dist", q, "Distanza", o.distanza, 0, 800, 16)) cambiato = true;
    }
    std::vector<std::string> effetti;
    for (auto& e : progetto.effetti) effetti.push_back(e.nome);
    effetti = Con(effetti, "(nessuno)");
    int k = o.suono.empty() ? 0 : Indice(effetti, o.suono);
    q = col.Riga(28);
    if (col.Vede(q) && Ui::Scelta(q, "Suono", k, effetti)) {
        o.suono = k == 0 ? "" : effetti[k];
        cambiato = true;
        if (auto* e = progetto.TrovaEffetto(o.suono)) gAudio.SuonaEffetto(*e);
    }
    if (o.funzione == FunzioneOggetto::Raccogli) {
        q = col.Riga(18);
        if (col.Vede(q)) Ui::Testo("Variabile da aumentare (es. monete):", q.x, q.y, Ui::TestoSpento(), Ui::TP());
        q = col.Riga(28);
        if (col.Vede(q) && Ui::Campo("o_var", q, o.variabile, 30)) cambiato = true;
    }
    EndScissorMode();
    scorriProprieta = std::min(scorriProprieta, std::max(0.0f, col.Altezza() - area.height));
    if (cambiato) {
        ProgettoGioco vecchio = progetto;
        for (auto& x : vecchio.oggetti) if (x.nome == o.nome) x = prima;
        annulla.push_back(vecchio);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
    }
}

// --- Scenari e caselle --------------------------------------------------------------------------------

void Officina::Scenari(Rectangle r, float dt) {
    float s = Ui::S();
    // Schede: scenari / caselle.
    Rectangle schede = { r.x, r.y, r.width, 40 * s };
    DrawRectangleRec(schede, Hex(0x0f1230));
    const char* nomiSchede[] = { "Scenari (sfondi)", "Caselle (blocchi del terreno)" };
    for (int i = 0; i < 2; i++)
        if (Ui::Pulsante({ r.x + 10 * s + i * 250 * s, r.y + 5 * s, 240 * s, 30 * s }, nomiSchede[i], schedaScenari == i)) { schedaScenari = i; scorriLista = 0; }
    r.y += schede.height; r.height -= schede.height;
    Rectangle sx = { r.x, r.y, 250 * s, r.height };
    Rectangle dx = { r.x + r.width - 340 * s, r.y, 340 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width - dx.width, r.height };

    if (schedaScenari == 1) {
        // Caselle: i cinque tipi; ognuno puo' avere il suo disegno (animazioni "superficie" e "interno").
        std::vector<std::string> nomi;
        for (int i = 0; i < 5; i++) {
            const CasellaDef* c = progetto.Casella(CARATTERI_CASELLE[i]);
            nomi.push_back(std::string("'") + CARATTERI_CASELLE[i] + "'  " + NOMI_CASELLE[i] + "\n" + (c ? "disegnata" : "disegno del motore"));
        }
        Elenco(sx, "CASELLE", nomi, selCasella, [&](int i, Rectangle q) {
            const CasellaDef* c = progetto.Casella(CARATTERI_CASELLE[i]);
            if (c) c->disegno.DisegnaIn(q, 0, tempo);
            else { Ui::TestoCentrato(std::string(1, CARATTERI_CASELLE[i]), q, Ui::TestoSpento(), Ui::T()); }
        }, false);
        char car = CARATTERI_CASELLE[std::min(selCasella, 4)];
        CasellaDef* c = nullptr;
        for (auto& x : progetto.caselle) if (x.carattere == car) c = &x;
        DrawRectangleRec(dx, Ui::Pannello());
        Rectangle area = { dx.x + 12 * s, dx.y + 10 * s, dx.width - 24 * s, dx.height - 20 * s };
        Colonna col(area, s, 0);
        col.Sezione("CASELLA '" + std::string(1, car) + "'");
        Rectangle q = col.Riga(80);
        Ui::TestoACapo("Ogni casella e' un quadrato di 16 pixel del mondo. Due animazioni: \"superficie\" (con il vuoto sopra, es. l'erba) "
                       "e \"interno\" (sotto un'altra casella uguale). Con piu' fotogrammi la casella e' animata (acqua, lava...).",
                       q.x, q.y, q.width, Ui::TestoSpento(), (int)(13 * s));
        if (!c) {
            q = col.Riga(36);
            if (Ui::Pulsante(q, "Disegna questa casella", true)) AzioneSezione("c:nuova");
            Ui::TestoACapo("Ora si usa il disegno del motore. Puoi anche chiedere a Claude: \"disegna le caselle di terra come mattoni di un castello\".",
                           centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
            return;
        }
        q = col.Riga(30);
        std::string nome = c->nome;
        if (Ui::Campo("c_nome", q, nome, 40)) { Ricorda(); c->nome = nome; }
        q = col.Riga(36);
        if (Ui::Pulsante(q, "Torna al disegno del motore")) { AzioneSezione("c:elimina"); return; }
        // Anteprima: un pezzo di terreno fatto con questa casella.
        col.Sezione("ANTEPRIMA");
        q = col.Riga(150);
        DrawRectangleRec(q, Hex(0x6aa8e8));
        float lato = std::floor(q.width / 6);
        for (int y = 0; y < 3; y++)
            for (int x = 0; x < 6; x++) {
                if (y == 0 && (x == 0 || x == 5)) continue;
                int an = y == 0 || (y == 1 && (x == 0 || x == 5)) ? 0 : 1;
                an = std::min(an, (int)c->disegno.animazioni.size() - 1);
                c->disegno.DisegnaIn({ q.x + x * lato, q.y + q.height - (3 - y) * lato, lato, lato }, an, tempo);
            }
        editorDisegno.Apri(&c->disegno, std::string("c:") + car);
        if (editorDisegno.Frame(centro, dt)) { modificato = true; editorMondo.AggiornaVista(); }
        return;
    }

    // Scenari.
    std::vector<std::string> nomi;
    for (auto& sc : progetto.scenari) {
        int usato = 0;
        for (auto& m : progetto.mondo.mappe) usato += m.scenarioLibreria == sc.nome;
        nomi.push_back(sc.nome + "\n" + (usato ? TextFormat("usato in %d mappe", usato) : "non usato"));
    }
    int az = Elenco(sx, "SCENARI DEL GIOCO", nomi, selScenario, [&](int i, Rectangle q) {
        const Scenario& sc = progetto.scenari[i];
        Color a = sc.cieloPersonale ? sc.cieloAlto : Hex(0x3f8ff0), b = sc.cieloPersonale ? sc.cieloBasso : Hex(0xb9e3ff);
        DrawRectangleGradientV((int)q.x, (int)q.y, (int)q.width, (int)q.height, a, b);
        if (!sc.strati.empty()) DrawTriangle({ q.x, q.y + q.height }, { q.x + q.width, q.y + q.height }, { q.x + q.width * 0.5f, q.y + q.height * 0.35f }, sc.strati[0].colore);
    });
    if (az == 1) AzioneSezione("s:nuovo");
    if (az == 2) AzioneSezione("s:duplica");
    if (az == 3) AzioneSezione("s:elimina");
    if (progetto.scenari.empty()) {
        Ui::TestoACapo("Nessuno scenario: premi \"+ Nuovo\" o chiedi a Claude \"crea uno scenario di montagne innevate al tramonto\".",
                       centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    selScenario = std::min(selScenario, (int)progetto.scenari.size() - 1);
    Scenario& sc = progetto.scenari[selScenario];

    // Anteprima grande, con un terreno d'esempio e la pioggia/neve.
    anteprimaScenario.tema = sc.tema;
    anteprimaScenario.ora = sc.ora;
    anteprimaScenario.generato = sc.sfondoGenerato;
    anteprimaScenario.cieloPersonale = sc.cieloPersonale;
    anteprimaScenario.cieloAlto = sc.cieloAlto;
    anteprimaScenario.cieloBasso = sc.cieloBasso;
    anteprimaScenario.strati = sc.strati;
    anteprimaScenario.Aggiorna(dt);
    oraAnteprima = sc.cicloGiorno ? std::fmod(oraAnteprima + dt * 0.02f, 1.0f) : 0;
    anteprimaScenario.ora = std::fmod(sc.ora + oraAnteprima, 1.0f);
    camScenario += dt * 40;
    float pw = centro.width - 30 * s, ph = pw * VH / VW;
    if (ph > centro.height * 0.62f) { ph = centro.height * 0.62f; pw = ph * VW / VH; }
    Rectangle prev = { centro.x + (centro.width - pw) / 2, centro.y + 14 * s, pw, ph };
    if (terrenoScenario.Larghezza() == 0) {
        Mappa m;
        m.Nuova(120, 17, Vista::Laterale);
        for (int x = 0; x < 120; x++) if ((x / 9) % 4 == 2) for (int y = 9; y < 10; y++) m.ImpostaCasella(x, y, '-');
        std::string e;
        terrenoScenario.DaMappa(m, e);
    }
    terrenoScenario.caselleLibreria.clear();
    for (auto& c : progetto.caselle) terrenoScenario.caselleLibreria.push_back({ c.carattere, &c.disegno });
    float salva = gScalaHD;
    gScalaHD = pw / VW;
    BeginScissorMode((int)prev.x, (int)prev.y, (int)prev.width, (int)prev.height);
    BeginMode2D(CameraHD({ prev.x / gScalaHD, prev.y / gScalaHD }, { 0, 0 }, 1));
    anteprimaScenario.Disegna(camScenario, 0);
    float cx = std::fmod(camScenario, 120.0f * TILE - VW);
    terrenoScenario.Disegna(cx, 17 * TILE - VH, tempo, anteprimaScenario.Notte() * 0.6f);
    anteprimaScenario.DisegnaPrimoPiano(camScenario, 0);
    if (sc.pioggia > 0) {
        for (int i = 0; i < 160; i++) {
            float x = std::fmod(Hash1(i) * VW + tempo * (sc.pioggia == 1 ? 60 : 12) + 1000, (float)VW);
            float y = std::fmod(Hash1(i + 500) * VH + tempo * (sc.pioggia == 1 ? 300 : 30) * (0.7f + Hash1(i + 9) * 0.6f), (float)VH);
            if (sc.pioggia == 1) DrawLineEx({ x, y }, { x - 1.5f, y + 6 }, 0.6f, Hex(0xb8d0ff, 150));
            else DrawCircleV({ x, y }, 0.8f + Hash1(i + 3), Hex(0xffffff, 220));
        }
    }
    EndMode2D();
    EndScissorMode();
    gScalaHD = salva;
    DrawRectangleLinesEx(prev, 1, Ui::Bordo());
    float y = prev.y + prev.height + 12 * s;
    std::string usate;
    for (auto& m : progetto.mondo.mappe) if (m.scenarioLibreria == sc.nome) usate += (usate.empty() ? "" : ", ") + m.nome;
    Ui::Testo(usate.empty() ? "Non e' usato da nessuna mappa." : "Usato nelle mappe: " + usate, prev.x, y, Ui::TestoSpento(), Ui::TP());
    if (Ui::Pulsante({ prev.x + prev.width - 260 * s, y - 4 * s, 260 * s, 30 * s }, "Usa nella mappa aperta", true)) AzioneSezione("s:usa");
    y += 34 * s;
    Ui::TestoACapo("Trascina qui un'immagine PNG o JPG: diventa uno strato dello scenario. Gli strati si muovono a velocita' diverse (parallasse) "
                   "mentre la telecamera scorre.", prev.x, y, prev.width, Ui::TestoSpento(), (int)(13 * s));

    // Proprieta'.
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    Rectangle area = { dx.x + 12 * s, dx.y + 10 * s, dx.width - 24 * s, dx.height - 20 * s };
    if (Ui::MouseSu(area)) scorriProprieta = std::max(0.0f, scorriProprieta - GetMouseWheelMove() * 50 * s);
    Scenario prima = sc;
    bool cambiato = false;
    Colonna col(area, s, scorriProprieta);
    BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    col.Sezione("SCENARIO");
    Rectangle q = col.Riga(30);
    std::string nome = sc.nome;
    if (col.Vede(q) && Ui::Campo("s_nome", q, nome, 40) && !nome.empty() && nome != sc.nome && !progetto.TrovaScenario(nome)) {
        Ricorda(); RinominaOvunque(progetto, sc.nome, nome); sc.nome = nome; MondoCambiato(); prima = sc;
    }
    std::vector<std::string> temi;
    for (auto& t : Sfondo::Temi()) temi.push_back(t.nome);
    q = col.Riga(28);
    if (col.Vede(q) && Ui::Scelta(q, "Tema", sc.tema, temi)) cambiato = true;
    q = col.Riga(28);
    if (col.Vede(q) && Ui::Numero("s_ora", q, "Ora del giorno", sc.ora, 0, 1, 0.05f, "%.2f")) cambiato = true;
    auto Leva = [&](const char* et, bool& v) { Rectangle rr = col.Riga(28); if (col.Vede(rr) && Ui::Interruttore(rr, et, v)) cambiato = true; };
    Leva("Ciclo giorno e notte", sc.cicloGiorno);
    Leva("Paesaggio generato", sc.sfondoGenerato);
    Leva("Cielo personalizzato", sc.cieloPersonale);
    if (sc.cieloPersonale) {
        q = col.Riga(28); if (col.Vede(q) && Ui::Colore("s_ca", q, "Cielo in alto", sc.cieloAlto)) cambiato = true;
        q = col.Riga(28); if (col.Vede(q) && Ui::Colore("s_cb", q, "Cielo in basso", sc.cieloBasso)) cambiato = true;
    }
    q = col.Riga(28);
    if (col.Vede(q) && Ui::Scelta(q, "Meteo", sc.pioggia, { "Sereno", "Pioggia", "Neve" })) cambiato = true;
    col.Sezione("STRATI");
    for (int i = 0; i < (int)sc.strati.size(); i++) {
        q = col.Riga(28);
        if (!col.Vede(q)) continue;
        const StratoSfondo& st = sc.strati[i];
        DrawRectangleRec({ q.x, q.y + 4 * s, 20 * s, q.height - 8 * s }, st.colore);
        if (Ui::Pulsante({ q.x + 26 * s, q.y, q.width - 26 * s, q.height }, (st.nome.empty() ? st.tipo : st.nome) + "  (" + st.tipo + (st.primoPiano ? ", davanti" : "") + ")", i == stratoScelto))
            stratoScelto = i;
    }
    q = col.Riga(28);
    if (col.Vede(q)) {
        float w4 = (q.width - 18 * s) / 4;
        if (Ui::Pulsante({ q.x, q.y, w4, q.height }, "+ Strato")) AzioneSezione("s:strato");
        bool ok = stratoScelto >= 0 && stratoScelto < (int)sc.strati.size();
        if (Ui::Pulsante({ q.x + w4 + 6 * s, q.y, w4, q.height }, "Su", false, ok && stratoScelto > 0)) { Ricorda(); std::swap(sc.strati[stratoScelto], sc.strati[stratoScelto - 1]); stratoScelto--; }
        if (Ui::Pulsante({ q.x + 2 * (w4 + 6 * s), q.y, w4, q.height }, "Giu'", false, ok && stratoScelto + 1 < (int)sc.strati.size())) { Ricorda(); std::swap(sc.strati[stratoScelto], sc.strati[stratoScelto + 1]); stratoScelto++; }
        if (Ui::Pulsante({ q.x + 3 * (w4 + 6 * s), q.y, w4, q.height }, "Togli", false, ok)) { Ricorda(); sc.strati.erase(sc.strati.begin() + stratoScelto); stratoScelto = std::max(0, stratoScelto - 1); }
    }
    if (stratoScelto >= 0 && stratoScelto < (int)sc.strati.size()) {
        StratoSfondo& st = sc.strati[stratoScelto];
        col.Sezione("STRATO SCELTO");
        q = col.Riga(28); if (col.Vede(q) && Ui::Campo("st_nome", q, st.nome, 30)) cambiato = true;
        static const std::vector<std::string> tipi = { "montagne", "colline", "foresta", "nuvole", "stelle", "fascia", "immagine" };
        int t = Indice(tipi, st.tipo);
        q = col.Riga(28); if (col.Vede(q) && Ui::Scelta(q, "Tipo", t, tipi)) { st.tipo = tipi[t]; cambiato = true; }
        q = col.Riga(28); if (col.Vede(q) && Ui::Colore("st_col", q, "Colore", st.colore)) cambiato = true;
        auto Num = [&](const char* id, const char* et, float& v, float lo, float hi, float passo, const char* f) {
            Rectangle rr = col.Riga(28); if (col.Vede(rr) && Ui::Numero(id, rr, et, v, lo, hi, passo, f)) cambiato = true;
        };
        Num("st_px", "Parallasse X", st.parallasseX, 0, 1, 0.05f, "%.2f");
        Num("st_py", "Parallasse Y", st.parallasseY, 0, 1, 0.05f, "%.2f");
        Num("st_alt", "Altezza", st.altezza, 0, 1, 0.05f, "%.2f");
        Num("st_amp", "Ampiezza", st.ampiezza, 0, 200, 5, "%.0f");
        Num("st_seme", "Forma (seme)", st.seme, 0, 99, 1, "%.0f");
        Num("st_vento", "Vento", st.vento, -100, 100, 5, "%.0f");
        if (st.tipo == "immagine") {
            q = col.Riga(28); if (col.Vede(q) && Ui::Campo("st_img", q, st.immagine, 60)) cambiato = true;
            Num("st_scala", "Scala", st.scala, 0.1f, 8, 0.1f, "%.1f");
            Leva("Ripetuta", st.ripeti);
        }
        Leva("In primo piano", st.primoPiano);
    }
    EndScissorMode();
    scorriProprieta = std::min(scorriProprieta, std::max(0.0f, col.Altezza() - area.height));
    if (cambiato) {
        ProgettoGioco vecchio = progetto;
        vecchio.scenari[selScenario] = prima;
        annulla.push_back(vecchio);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
    }
}
