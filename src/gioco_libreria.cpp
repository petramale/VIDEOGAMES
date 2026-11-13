// Il motore con la libreria del gioco: personaggi e oggetti disegnati nello studio, con i loro
// comportamenti e le loro funzioni, le caselle e gli scenari personalizzati, i brani e gli
// effetti sonori, e le regole "quando ... allora ..." scritte nella sezione Logica.
#include "gioco.h"
#include "audio.h"
#include "immagini.h"
#include "periferiche.h"
#include <algorithm>
#include <cctype>

namespace {

bool Sovrappone(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }

std::string Minuscolo(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

bool Uguale(const std::string& a, const std::string& b) { return Minuscolo(a) == Minuscolo(b); }

// Il riquadro "pieno" del primo fotogramma, rispetto ai piedi (centro in basso), in pixel del mondo.
Rectangle BoxDisegno(const Disegno& d) {
    if (d.animazioni.empty() || d.animazioni[0].fotogrammi.empty()) return { -6, -12, 12, 12 };
    const Fotogramma& f = d.animazioni[0].fotogrammi[0];
    int x0 = f.w, y0 = f.h, x1 = -1, y1 = -1;
    for (int y = 0; y < f.h; y++)
        for (int x = 0; x < f.w; x++)
            if (f.Prendi(x, y).a > 60) { x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y); }
    if (x1 < 0) return { -6, -12, 12, 12 };
    float k = 1.0f / d.densita;
    float larg = (x1 - x0 + 1) * k, alt = (y1 - y0 + 1) * k;
    // un po' piu' stretto del disegno: le collisioni "giuste" sembrano piu' generose
    float cx = ((x0 + x1 + 1) / 2.0f - f.w / 2.0f) * k;
    float basso = (f.h - 1 - y1) * k;
    return { cx - larg * 0.4f, -basso - alt * 0.9f, larg * 0.8f, alt * 0.9f };
}

int TastoDaNome(const std::string& n) {
    std::string s = Minuscolo(n);
    if (s.size() == 1) {
        char c = s[0];
        if (c >= 'a' && c <= 'z') return KEY_A + (c - 'a');
        if (c >= '0' && c <= '9') return KEY_ZERO + (c - '0');
    }
    if (s == "spazio") return KEY_SPACE;
    if (s == "invio") return KEY_ENTER;
    if (s == "su") return KEY_UP;
    if (s == "giu" || s == "giu'") return KEY_DOWN;
    if (s == "sinistra") return KEY_LEFT;
    if (s == "destra") return KEY_RIGHT;
    if (s == "ctrl") return KEY_LEFT_CONTROL;
    if (s == "shift" || s == "maiuscolo") return KEY_LEFT_SHIFT;
    if (s == "tab") return KEY_TAB;
    return 0;
}

} // namespace

bool Gioco::CaricaProgetto(const ProgettoGioco& p, std::string& errore) {
    std::string problema = p.Controlla();
    if (!problema.empty()) { errore = problema; return false; }
    lib = p;
    conLibreria = true;
    // immagini importate: strati, decorazioni
    std::vector<FileIncluso> file;
    for (auto& im : lib.immagini) file.push_back(im);
    ImpostaFileImmagini(file);
    gAudio.TogliSostituzioni();
    static const std::pair<const char*, Effetto> eventi[] = { { "salto", Effetto::Salto }, { "moneta", Effetto::Moneta }, { "gemma", Effetto::Gemma },
        { "colpo", Effetto::Colpo }, { "molla", Effetto::Molla }, { "forziere", Effetto::Forziere }, { "esplosione", Effetto::Esplosione },
        { "vittoria", Effetto::Vittoria }, { "passo", Effetto::Passo } };
    for (auto& e : eventi) {
        auto it = lib.suoniEventi.find(e.first);
        if (it != lib.suoniEventi.end() && !it->second.empty()) gAudio.SostituisciEffetto(e.second, lib.TrovaEffetto(it->second));
    }
    return CaricaMondo(lib.mondo, errore);
}

// Dopo CaricaMondo: eroe della libreria, misure dei disegni, caselle.
void Gioco::PreparaLibreria() {
    boxPersonaggi.clear(); boxOggetti.clear();
    for (auto& p : lib.personaggi) boxPersonaggi.push_back(BoxDisegno(p.disegno));
    for (auto& o : lib.oggetti) boxOggetti.push_back(BoxDisegno(o.disegno));
    eroeDef = -1;
    if (!lib.eroe.empty())
        for (size_t i = 0; i < lib.personaggi.size(); i++) if (Uguale(lib.personaggi[i].nome, lib.eroe)) eroeDef = (int)i;
    if (eroeDef >= 0) {
        const Comportamento& c = lib.personaggi[eroeDef].comportamento;
        if (c.velocita > 0) fisica.velocita = c.velocita;
        if (c.salto > 0) fisica.salto = c.salto;
        energiaMax = std::max(1, std::min(10, c.energia));
        energia = energiaMax;
    }
    livello.caselleLibreria.clear();
    for (auto& c : lib.caselle) livello.caselleLibreria.push_back({ c.carattere, &c.disegno });
    timerRegole.assign(lib.regole.size(), 0.0f);
    condizioniRegole.assign(lib.regole.size(), false);
}

void Gioco::EntitaDellaMappa(const Mappa& m) {
    for (auto& om : m.oggetti) {
        TipoOggetto t;
        if (TipoDaNome(om.tipo, t)) continue;
        int i = CreaEntita(om.tipo, { om.x, om.y });
        if (i >= 0) { entita[i].mappaPorta = om.mappa; entita[i].destPorta = { om.destX, om.destY }; if (om.numero > 0) entita[i].energia = om.numero; }
    }
    // Scenario della libreria
    if (const Scenario* s = m.scenarioLibreria.empty() ? nullptr : lib.TrovaScenario(m.scenarioLibreria)) {
        sfondo.tema = s->tema; sfondo.ora = s->ora; sfondo.generato = s->sfondoGenerato;
        sfondo.cieloPersonale = s->cieloPersonale; sfondo.cieloAlto = s->cieloAlto; sfondo.cieloBasso = s->cieloBasso;
        sfondo.strati = s->strati;
        if (!s->cicloGiorno) cicloGiorno = false;
        pioggia = s->pioggia;
    }
    // Brano della libreria
    if (const Brano* b = m.brano.empty() ? nullptr : lib.TrovaBrano(m.brano)) {
        gAudio.SuonaBrano(*b);
        musica = -1;
    }
}

int Gioco::CreaEntita(const std::string& nome, Vector2 pos) {
    Entita e;
    e.pos = e.origine = pos;
    e.nome = nome;
    bool trovato = false;
    for (size_t i = 0; i < lib.personaggi.size() && !trovato; i++)
        if (Uguale(lib.personaggi[i].nome, nome)) { e.personaggio = true; e.def = (int)i; trovato = true; e.energia = lib.personaggi[i].comportamento.energia; e.nome = lib.personaggi[i].nome; }
    for (size_t i = 0; i < lib.oggetti.size() && !trovato; i++)
        if (Uguale(lib.oggetti[i].nome, nome)) { e.personaggio = false; e.def = (int)i; trovato = true; e.nome = lib.oggetti[i].nome; }
    if (!trovato) return -1;
    if (e.personaggio && (int)e.def == eroeDef) return -1;   // l'eroe non si mette due volte
    e.timer = Casuale(0.5f, 1.5f);
    entita.push_back(e);
    return (int)entita.size() - 1;
}

const Disegno* Gioco::DisegnoEntita(const Entita& e) const {
    if (e.personaggio) return e.def < (int)lib.personaggi.size() ? &lib.personaggi[e.def].disegno : nullptr;
    return e.def < (int)lib.oggetti.size() ? &lib.oggetti[e.def].disegno : nullptr;
}

Rectangle Gioco::BoxEntita(const Entita& e) const {
    const std::vector<Rectangle>& v = e.personaggio ? boxPersonaggi : boxOggetti;
    Rectangle b = e.def < (int)v.size() ? v[e.def] : Rectangle{ -6, -12, 12, 12 };
    return { e.pos.x + b.x, e.pos.y + b.y, b.width, b.height };
}

void Gioco::SuonaLibreria(const std::string& effetto, float volume) {
    if (effetto.empty()) return;
    if (const EffettoSuono* s = lib.TrovaEffetto(effetto)) gAudio.SuonaEffetto(*s, volume);
}

// Cose solide della libreria (porte chiuse, casse, piattaforme): l'eroe ci sbatte e ci sale sopra.
void Gioco::SolidiEntita(Vector2& pos, Vector2& vel, float w, float hgt, float piediPrima, bool& aTerra, int ignora) {
    appoggio = -1;
    for (size_t i = 0; i < entita.size(); i++) {
        Entita& e = entita[i];
        if ((int)i == ignora || !e.attivo || e.personaggio || e.morte > 0) continue;
        const OggettoDef& d = lib.oggetti[e.def];
        bool solido = (d.solido && !(d.funzione == FunzioneOggetto::Porta && e.aperta)) || d.funzione == FunzioneOggetto::PiattaformaMobile;
        if (!solido) continue;
        Rectangle r = BoxEntita(e);
        if (d.funzione == FunzioneOggetto::PiattaformaMobile || d.funzione == FunzioneOggetto::Cassa) { r.y = e.pos.y - BoxEntita(e).height; }
        Rectangle me = { pos.x - w / 2, pos.y - hgt, w, hgt };
        // si sale sopra
        if (vel.y >= 0 && piediPrima <= r.y + 1.5f && pos.y >= r.y && me.x + me.width > r.x + 1 && me.x < r.x + r.width - 1) {
            pos.y = r.y; vel.y = 0; aTerra = true; appoggio = (int)i;
            continue;
        }
        if (d.funzione == FunzioneOggetto::PiattaformaMobile) continue;   // attraversabile da sotto e di lato
        if (!Sovrappone(me, r)) continue;
        float sx = std::min(me.x + me.width - r.x, r.x + r.width - me.x);
        float sy = std::min(me.y + me.height - r.y, r.y + r.height - me.y);
        if (sx < sy) { pos.x += (me.x + me.width / 2 < r.x + r.width / 2) ? -sx : sx; vel.x = 0; }
        else if (me.y < r.y) { pos.y -= sy; vel.y = 0; aTerra = true; }
        else { pos.y += sy; if (vel.y < 0) vel.y = 0; }
    }
}

void Gioco::SparaDa(int i, const std::string& nome, int quanti) {
    Vector2 da, verso;
    bool amico = i < 0;
    std::string proiettile = nome;
    if (amico) {
        da = { eroe.pos.x + eroe.dir * 8, eroe.pos.y - Eroe::ALTEZZA * 0.55f };
        verso = { da.x + eroe.dir * 100, da.y };
        if (vistaAlto && modello != Modello::Sparatutto) {
            // dall'alto si spara nella direzione in cui si cammina
            Vector2 dd = { eroe.vel.x, eroe.vel.y };
            if (std::fabs(dd.x) + std::fabs(dd.y) < 1) dd = { (float)eroe.dir, 0 };
            verso = { da.x + dd.x, da.y + dd.y };
        }
        if (proiettile.empty() && eroeDef >= 0) proiettile = lib.personaggi[eroeDef].comportamento.proiettile;
    } else {
        Entita& e = entita[i];
        Rectangle b = BoxEntita(e);
        da = { e.pos.x + e.dir * b.width * 0.4f, b.y + b.height * 0.4f };
        verso = { eroe.pos.x, eroe.pos.y - 10 };
        if (proiettile.empty() && e.personaggio) proiettile = lib.personaggi[e.def].comportamento.proiettile;
        e.anim = ""; e.tAnim = 0; e.spinta = 0.5f;   // anima "attacco" per un attimo
        if (e.personaggio) {
            auto it = lib.personaggi[e.def].suoni.find("attacco");
            if (it != lib.personaggi[e.def].suoni.end()) SuonaLibreria(it->second, 0.7f);
        }
    }
    int def = -1;
    float vel = amico ? 260.0f : 110.0f;
    for (size_t k = 0; k < lib.oggetti.size(); k++)
        if (Uguale(lib.oggetti[k].nome, proiettile)) { def = (int)k; vel = std::max(60.0f, lib.oggetti[k].velocita * (amico ? 1.6f : 1.0f)); }
    if (def >= 0) SuonaLibreria(lib.oggetti[def].suono, 0.5f);
    else gAudio.Suona(Effetto::Menu, 0.35f);
    quanti = std::max(1, std::min(9, quanti));
    float ang = std::atan2(verso.y - da.y, verso.x - da.x);
    for (int q = 0; q < quanti; q++) {
        float a = ang + (q - (quanti - 1) / 2.0f) * 0.25f;
        Proiettile p{ da, { std::cos(a) * vel, std::sin(a) * vel }, amico, 3.0f, amico ? Hex(0x9ff6ff) : Hex(0xff7a3a) };
        p.disegno = def;
        p.danno = def >= 0 ? std::max(1, lib.oggetti[def].valore) : 1;
        proiettili.push_back(p);
    }
}

void Gioco::ColpisciEntita(int i, int danno) {
    Entita& e = entita[i];
    if (!e.attivo || e.morte > 0 || e.lampeggio > 0) return;
    e.energia -= danno;
    e.lampeggio = 0.5f;
    Evento("colpisce", "eroe", e.nome, i);
    const PersonaggioDef& p = lib.personaggi[e.def];
    auto it = p.suoni.find("colpito");
    if (it != p.suoni.end()) SuonaLibreria(it->second); else gAudio.Suona(Effetto::Colpo, 0.7f);
    scossa = std::max(scossa, 0.15f);
    if (e.energia <= 0) {
        e.morte = 0.01f;
        punteggio += p.comportamento.punti;
        auto s = p.suoni.find("sconfitto");
        if (s != p.suoni.end()) SuonaLibreria(s->second); else gAudio.Suona(Effetto::Esplosione, 0.6f);
        Rectangle b = BoxEntita(e);
        particelle.Esplosione({ b.x + b.width / 2, b.y + b.height / 2 }, Hex(0xffd23f));
        Evento("sconfitto", e.nome, "", i);
    }
}

bool Gioco::ProiettileSuEntita(Proiettile& p) {
    for (size_t i = 0; i < entita.size(); i++) {
        Entita& e = entita[i];
        if (!e.attivo || !e.personaggio || e.morte > 0) continue;
        if (lib.personaggi[e.def].comportamento.tipo == TipoComportamento::Parla) continue;
        if (CheckCollisionPointRec(p.pos, BoxEntita(e))) {
            ColpisciEntita((int)i, p.danno);
            p.vita = 0;
            return true;
        }
    }
    return false;
}

void Gioco::AggiornaEntita(float dt, const Comandi& c) {
    asseInput = c.asse;
    Rectangle box = eroe.Box();
    // L'eroe della libreria spara il suo proiettile (X o Ctrl; dall'alto anche il tasto di salto).
    if (eroeDef >= 0 && !lib.personaggi[eroeDef].comportamento.proiettile.empty() && !vinto && !finito) {
        tSparo -= dt;
        bool fuoco = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_LEFT_CONTROL) || (vistaAlto && c.saltoPremuto);
        if (gInput.Joypad() >= 0 && IsGamepadButtonPressed(gInput.Joypad(), GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) fuoco = true;
        if (fuoco && tSparo <= 0) { tSparo = 0.25f; SparaDa(-1, "", 1); }
    }
    dialogo = -1;
    size_t n = entita.size();   // le regole possono crearne altre: si aggiornano dal prossimo fotogramma
    for (size_t i = 0; i < n && i < entita.size(); i++) {
        Entita& e = entita[i];
        if (!e.attivo) continue;
        e.t += dt; e.tAnim += dt;
        if (e.lampeggio > 0) e.lampeggio -= dt;
        if (e.spinta > 0) e.spinta -= dt;
        if (e.morte > 0) { e.morte += dt; if (e.morte > 0.5f) e.attivo = false; continue; }
        if (e.personaggio) AggiornaPersonaggio(e, (int)i, dt, box);
        else AggiornaOggettoLib(entita[i], (int)i, dt, box);
        if (i >= entita.size()) break;
        Entita& f = entita[i];
        // eventi "tocca" e "vicino" (solo quando cominciano)
        Rectangle r = BoxEntita(f);
        bool tocca = f.attivo && Sovrappone(box, r);
        if (tocca && !f.toccando) { Evento("tocca", "eroe", f.nome, (int)i); Evento("tocca", f.nome, "eroe", (int)i); }
        if (i < entita.size()) entita[i].toccando = tocca;
        if (i >= entita.size()) break;
        float dx = entita[i].pos.x - eroe.pos.x, dy = entita[i].pos.y - eroe.pos.y;
        bool vicino = dx * dx + dy * dy < 48 * 48;
        if (vicino && !entita[i].vicino) Evento("vicino", "eroe", entita[i].nome, (int)i);
        if (i < entita.size()) entita[i].vicino = vicino;
    }
    // energia sotto una soglia (regole "ha energia sotto")
    for (size_t r = 0; r < lib.regole.size(); r++) {
        const Regola& rg = lib.regole[r];
        if (!rg.attiva || rg.evento != "energia") continue;
        float soglia = (float)std::atof(rg.conChi.c_str());
        for (size_t i = 0; i < entita.size(); i++) {
            Entita& e = entita[i];
            if (!e.attivo || !e.personaggio || !Uguale(e.nome, rg.chi) || e.energia > soglia) continue;
            if (energiaNotificata.count({ (int)r, (int)i })) continue;
            if (!Condizione(rg)) continue;
            energiaNotificata.insert({ (int)r, (int)i });
            EseguiRegola(rg, (int)i);
        }
    }
    // la piattaforma porta con se' l'eroe
    if (appoggio >= 0 && appoggio < (int)entita.size()) {
        Entita& e = entita[appoggio];
        if (!e.personaggio && lib.oggetti[e.def].funzione == FunzioneOggetto::PiattaformaMobile) eroe.pos.x += e.vel.x * dt;
    }
}

void Gioco::AggiornaPersonaggio(Entita& e, int i, float dt, Rectangle eroeBox) {
    const PersonaggioDef& p = lib.personaggi[e.def];
    const Comportamento& c = p.comportamento;
    Rectangle b0 = BoxEntita(e);
    float w = b0.width, h = b0.height;
    float v = c.velocita * e.velMolt;
    float dx = eroe.pos.x - e.pos.x, dy = (eroe.pos.y - 10) - (e.pos.y - h / 2);
    float dist = std::sqrt(dx * dx + dy * dy);
    bool gravita = c.gravita && Laterale();
    bool vede = dist < c.vista;
    switch (c.tipo) {
        case TipoComportamento::Pattuglia: case TipoComportamento::Giocatore: {
            if (gravita) {
                int davanti = (int)std::floor((e.pos.x + e.dir * (w / 2 + 2)) / TILE);
                int sotto = (int)std::floor((e.pos.y + 2) / TILE);
                bool vuoto = e.aTerra && !livello.Solido(davanti, sotto) && !livello.Piattaforma(davanti, sotto);
                bool muro = livello.Solido(davanti, (int)std::floor((e.pos.y - 4) / TILE));
                if (vuoto || muro) e.dir = -e.dir;
                e.vel.x = e.dir * v;
                if (c.salto > 0 && e.aTerra) { e.timer -= dt; if (e.timer <= 0) { e.vel.y = -c.salto; e.timer = Casuale(1.5f, 3); } }
            } else {
                int davanti = (int)std::floor((e.pos.x + e.dir * (w / 2 + 2)) / TILE);
                if (livello.Solido(davanti, (int)std::floor((e.pos.y - h / 2) / TILE)) || std::fabs(e.pos.x - e.origine.x) > 96) e.dir = e.pos.x < e.origine.x ? 1 : -1;
                e.vel = { e.dir * v, 0 };
            }
            break;
        }
        case TipoComportamento::Insegue: {
            if (vede) e.dir = dx > 0 ? 1 : -1;
            if (gravita) {
                e.vel.x = vede ? e.dir * v : e.dir * v * 0.3f;
                int davanti = (int)std::floor((e.pos.x + e.dir * (w / 2 + 2)) / TILE);
                bool muro = livello.Solido(davanti, (int)std::floor((e.pos.y - 4) / TILE));
                if (muro && e.aTerra) { if (c.salto > 0) e.vel.y = -c.salto; else if (!vede) e.dir = -e.dir; }
                if (vede && c.salto > 0 && e.aTerra && dy < -24) e.vel.y = -c.salto;
            } else if (vede && dist > 1) e.vel = { dx / dist * v, dy / dist * v };
            else e.vel = { 0, 0 };
            break;
        }
        case TipoComportamento::Vola: {
            e.pos.x = e.origine.x + std::sin(e.t * 0.9f * v / 35) * 40;
            e.pos.y = e.origine.y + std::sin(e.t * 2.3f) * 14;
            e.dir = std::cos(e.t * 0.9f * v / 35) > 0 ? 1 : -1;
            e.vel = { 0, 0 };
            break;
        }
        case TipoComportamento::Spara: {
            if (vede) e.dir = dx > 0 ? 1 : -1;
            e.vel.x = 0;
            if (v > 0 && !vede) { e.pos.x = e.origine.x + std::sin(e.t * 0.6f) * 24; }
            e.timer -= dt;
            if (vede && e.timer <= 0) { e.timer = c.attaccoOgni; SparaDa(i, "", 1); }
            break;
        }
        case TipoComportamento::Boss: {
            e.dir = dx > 0 ? 1 : -1;
            if (gravita) {
                e.vel.x = vede ? e.dir * v : 0;
                if (c.salto > 0 && e.aTerra && vede) { e.timer -= 0; }
            } else if (vede) {
                // vola sopra l'eroe, un po' di lato, ondeggiando
                float tx = eroe.pos.x - e.dir * 70, ty = eroe.pos.y - 70 + std::sin(e.t * 1.7f) * 20;
                float ddx = tx - e.pos.x, ddy = ty - e.pos.y;
                float l = std::sqrt(ddx * ddx + ddy * ddy);
                e.vel = l > 2 ? Vector2{ ddx / l * v, ddy / l * v } : Vector2{ 0, 0 };
            } else e.vel = { 0, 0 };
            e.timer -= dt;
            if (vede && e.timer <= 0) { e.timer = c.attaccoOgni; SparaDa(i, "", e.energia * 2 < c.energia ? 3 : 1); }
            break;
        }
        case TipoComportamento::Parla: {
            e.vel.x = 0;
            e.dir = dx > 0 ? 1 : -1;
            if (dist < 36 && !c.dialogo.empty()) dialogo = i;
            break;
        }
        default: e.vel.x = 0; break;
    }
    if (gravita) {
        e.vel.y = std::min(e.vel.y + fisica.gravita * dt, 420.0f);
        MuoviConCollisioni(e.pos, e.vel, std::max(4.0f, w), std::max(4.0f, h), dt, e.aTerra, true);
        if (e.pos.y > livello.Altezza() * TILE + 60) e.attivo = false;
    } else if (c.tipo != TipoComportamento::Vola) {
        e.pos.x += e.vel.x * dt; e.pos.y += e.vel.y * dt;
    }
    if (std::fabs(e.vel.x) > 1 && c.tipo != TipoComportamento::Insegue && c.tipo != TipoComportamento::Boss) e.dir = e.vel.x > 0 ? 1 : -1;
    if (c.tipo == TipoComportamento::Parla) return;
    // contatto con l'eroe
    Rectangle r = BoxEntita(e);
    if (Sovrappone(eroeBox, r) && !vinto && !finito) {
        bool dallAlto = Laterale() && c.calpestabile && eroe.vel.y > 40 && eroe.pos.y < r.y + r.height * 0.55f;
        if (dallAlto) {
            ColpisciEntita(i, 1);
            eroe.vel.y = -230;
        } else if (c.danno > 0) {
            Ferisci(eroe.pos.x < e.pos.x ? -150.0f : 150.0f);
        }
    }
}

void Gioco::AggiornaOggettoLib(Entita& e, int i, float dt, Rectangle box) {
    const OggettoDef& d = lib.oggetti[e.def];
    if (d.gravita && Laterale() && d.funzione != FunzioneOggetto::PiattaformaMobile) {
        Rectangle b = BoxEntita(e);
        e.vel.y = std::min(e.vel.y + fisica.gravita * dt, 420.0f);
        e.vel.x *= 1 - std::min(1.0f, dt * 8);
        MuoviConCollisioni(e.pos, e.vel, b.width, b.height, dt, e.aTerra, true);
    }
    Rectangle r = BoxEntita(e);
    bool tocca = Sovrappone(box, r);
    if (vinto || finito) return;
    switch (d.funzione) {
        case FunzioneOggetto::Raccogli: case FunzioneOggetto::Cura: case FunzioneOggetto::Chiave: {
            if (!tocca) break;
            e.attivo = false;
            if (d.funzione == FunzioneOggetto::Raccogli) {
                punteggio += d.valore;
                std::string v = Minuscolo(d.variabile);
                if (v == "monete" || (v.empty() && Minuscolo(d.nome).find("monet") != std::string::npos)) monete++;
                else if (v == "gemme") gemme++;
                else if (!v.empty()) variabili[d.variabile] += 1;
                if (!v.empty() && v == "monete") variabili["monete"] = (float)monete;
                particelle.Scintille({ r.x + r.width / 2, r.y + r.height / 2 }, Hex(0xffd23f));
            } else if (d.funzione == FunzioneOggetto::Cura) {
                energia = std::min(energiaMax, energia + std::max(1, d.valore));
                particelle.Scintille({ r.x + r.width / 2, r.y + r.height / 2 }, Hex(0xff5c8a));
            } else {
                inventario[d.nome]++;
                particelle.Scintille({ r.x + r.width / 2, r.y + r.height / 2 }, Hex(0xffe060));
            }
            if (!d.suono.empty() && lib.TrovaEffetto(d.suono)) SuonaLibreria(d.suono, 0.8f);
            else gAudio.Suona(d.funzione == FunzioneOggetto::Raccogli ? Effetto::Moneta : Effetto::Gemma, 0.8f);
            Evento("raccoglie", "eroe", d.nome, i);
            break;
        }
        case FunzioneOggetto::Porta: {
            if (!e.aperta) {
                Rectangle vicino = { r.x - 3, r.y, r.width + 6, r.height };
                bool chiave = d.richiede.empty() || inventario[d.richiede] > 0;
                if (Sovrappone(box, vicino) && chiave) {
                    e.aperta = true; e.anim = "aperta"; e.tAnim = 0;
                    if (!d.richiede.empty() && inventario[d.richiede] > 0) inventario[d.richiede]--;
                    SuonaLibreria(d.suono);
                    if (d.suono.empty()) gAudio.Suona(Effetto::Forziere, 0.6f);
                } else if (Sovrappone(box, vicino) && !chiave && dialogo < 0 && tMessaggio <= 0) {
                    messaggio = "Serve: " + d.richiede; tMessaggio = 1.5f;
                }
            } else if (tocca && e.mappaPorta >= 0 && !portaBloccata) VaiAMappa(e.mappaPorta, e.destPorta);
            break;
        }
        case FunzioneOggetto::Molla: {
            if (tocca && eroe.vel.y > 0 && eroe.pos.y < r.y + r.height * 0.7f) {
                eroe.vel.y = -(d.valore > 100 ? (float)d.valore : fisica.molla);
                saltoTagliato = true;
                eroe.CambiaStato(StatoEroe::Salto);
                e.anim = "scatta"; e.tAnim = 0;
                if (!d.suono.empty() && lib.TrovaEffetto(d.suono)) SuonaLibreria(d.suono); else gAudio.Suona(Effetto::Molla);
            }
            if (e.anim == "scatta" && e.tAnim > 0.4f) e.anim.clear();
            break;
        }
        case FunzioneOggetto::Cassa: {
            // si spinge camminandoci contro
            Rectangle lato = { r.x - 2, r.y + 2, r.width + 4, r.height - 4 };
            if (Sovrappone(box, lato) && std::fabs(asseInput) > 0.2f) {
                int dir = asseInput > 0 ? 1 : -1;
                if ((dir > 0 && eroe.pos.x < e.pos.x) || (dir < 0 && eroe.pos.x > e.pos.x)) {
                    float nx = e.pos.x + dir * 40 * dt;
                    int tx = (int)std::floor((nx + dir * r.width / 2) / TILE);
                    if (!livello.Solido(tx, (int)std::floor((e.pos.y - 2) / TILE))) e.pos.x = nx;
                }
            }
            break;
        }
        case FunzioneOggetto::Interruttore: {
            if (tocca && !e.toccando) {
                e.acceso = !e.acceso;
                e.anim = e.acceso ? "accesa" : "spenta";
                SuonaLibreria(d.suono);
                if (d.suono.empty()) gAudio.Suona(Effetto::Menu);
                for (auto& p : entita)
                    if (!p.personaggio && lib.oggetti[p.def].funzione == FunzioneOggetto::Porta && Uguale(lib.oggetti[p.def].richiede, d.nome)) {
                        p.aperta = e.acceso; p.anim = e.acceso ? "aperta" : "chiusa"; p.tAnim = 0;
                    }
            }
            break;
        }
        case FunzioneOggetto::PiattaformaMobile: {
            float x0 = e.pos.x;
            float dist = std::max(8.0f, d.distanza);
            e.pos.x = e.origine.x + std::sin(e.t * std::max(0.1f, d.velocita) / dist) * dist;
            e.vel.x = dt > 0 ? (e.pos.x - x0) / dt : 0;
            break;
        }
        case FunzioneOggetto::Pericolo:
            if (tocca) Ferisci(eroe.pos.x < e.pos.x ? -150.0f : 150.0f);
            break;
        case FunzioneOggetto::Arrivo:
            if (tocca && UscitaAperta()) {
                vinto = true; tFine = 0;
                mappaDopoVittoria = e.mappaPorta;
                if (!d.suono.empty() && lib.TrovaEffetto(d.suono)) SuonaLibreria(d.suono); else gAudio.Suona(Effetto::Vittoria);
            }
            break;
        default: break;
    }
}

// --- Regole --------------------------------------------------------------------------------

float Gioco::Valore(const std::string& v) const {
    std::string s = Minuscolo(v);
    if (s == "punti" || s == "punteggio") return (float)punteggio;
    if (s == "monete") return (float)monete;
    if (s == "gemme") return (float)gemme;
    if (s == "cuori" || s == "energia") return (float)energia;
    if (s == "vite") return (float)vite;
    if (s == "tempo") return tempo;
    auto it = variabili.find(v);
    if (it != variabili.end()) return it->second;
    for (auto& x : inventario) if (Uguale(x.first, v)) return (float)x.second;
    return 0;
}

void Gioco::ImpostaValore(const std::string& v, float x) {
    std::string s = Minuscolo(v);
    if (s == "punti" || s == "punteggio") punteggio = (int)x;
    else if (s == "monete") monete = (int)x;
    else if (s == "gemme") gemme = (int)x;
    else if (s == "cuori" || s == "energia") energia = (int)Limita(x, 0, (float)energiaMax);
    else if (s == "vite") vite = (int)x;
    else variabili[v] = x;
}

bool Gioco::Condizione(const Regola& r) const {
    if (r.variabile.empty()) return true;
    float a = Valore(r.variabile), b = r.valore;
    if (r.confronto == ">") return a > b;
    if (r.confronto == "<") return a < b;
    if (r.confronto == "<=") return a <= b;
    if (r.confronto == "==") return std::fabs(a - b) < 0.001f;
    if (r.confronto == "!=") return std::fabs(a - b) >= 0.001f;
    return a >= b;
}

void Gioco::Evento(const std::string& tipo, const std::string& chi, const std::string& conChi, int ent) {
    if (!conLibreria) return;
    for (size_t k = 0; k < lib.regole.size(); k++) {
        const Regola& r = lib.regole[k];
        if (!r.attiva || r.evento != tipo) continue;
        bool chiOk = Uguale(r.chi, chi) || r.chi == "*" || Uguale(r.chi, "qualsiasi");
        bool conOk = r.conChi.empty() || Uguale(r.conChi, conChi) || r.conChi == "*";
        if (tipo == "inizio" && !r.conChi.empty() && !Uguale(r.conChi, conChi)) conOk = false;
        if (!chiOk || !conOk || !Condizione(r)) continue;
        Regola copia = r;   // l'azione puo' cambiare la mappa o le entita'
        EseguiRegola(copia, ent);
        if (mappaVerso >= 0) return;
    }
}

void Gioco::AggiornaRegole(float dt) {
    if (timerRegole.size() != lib.regole.size()) { timerRegole.assign(lib.regole.size(), 0.0f); condizioniRegole.assign(lib.regole.size(), false); }
    if (tMessaggio > 0) tMessaggio -= dt;
    for (size_t k = 0; k < lib.regole.size() && mappaVerso < 0; k++) {
        Regola r = lib.regole[k];
        if (!r.attiva) continue;
        if (r.evento == "ogni") {
            float periodo = std::max(0.1f, (float)std::atof(r.conChi.c_str()));
            if (r.conChi.empty()) periodo = 1;
            timerRegole[k] += dt;
            if (timerRegole[k] >= periodo) { timerRegole[k] = 0; if (Condizione(r)) EseguiRegola(r, -1); }
        } else if (r.evento == "tasto") {
            int t = TastoDaNome(r.conChi);
            if (t && IsKeyPressed(t) && Condizione(r)) EseguiRegola(r, -1);
        } else if (r.evento == "clic") {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Condizione(r)) EseguiRegola(r, -1);
        } else if (r.evento == "joypad") {
            int j = gInput.Joypad();
            if (j >= 0 && IsGamepadButtonPressed(j, std::atoi(r.conChi.c_str())) && Condizione(r)) EseguiRegola(r, -1);
        } else if (r.evento == "variabile") {
            bool ora = !r.variabile.empty() && Condizione(r);
            if (ora && !condizioniRegole[k]) EseguiRegola(r, -1);
            condizioniRegole[k] = ora;
        }
    }
}

void Gioco::EseguiRegola(const Regola& r, int ent) {
    // A chi si applicano le azioni sui personaggi: quello dell'evento, oppure tutti quelli che si chiamano come 'chi'.
    std::vector<int> bersagli;
    if (ent >= 0 && ent < (int)entita.size()) bersagli.push_back(ent);
    else if (!Uguale(r.chi, "eroe") && !Uguale(r.chi, "gioco"))
        for (size_t i = 0; i < entita.size(); i++) if (entita[i].attivo && Uguale(entita[i].nome, r.chi)) bersagli.push_back((int)i);
    for (const AzioneRegola& a : r.azioni) {
        const std::string& t = a.tipo;
        if (t == "suona") SuonaLibreria(a.testo);
        else if (t == "musica") { if (const Brano* b = lib.TrovaBrano(a.testo)) { gAudio.SuonaBrano(*b); musica = -1; } }
        else if (t == "ferma_musica") { Brano silenzio; silenzio.nome = "(silenzio)"; silenzio.battute = 1; gAudio.SuonaBrano(silenzio); musica = -1; }
        else if (t == "messaggio") { messaggio = a.testo; tMessaggio = 3.5f; }
        else if (t == "aumenta") ImpostaValore(a.testo, Valore(a.testo) + (a.numero == 0 ? 1 : a.numero));
        else if (t == "imposta") ImpostaValore(a.testo, a.numero);
        else if (t == "punti") punteggio += (int)a.numero;
        else if (t == "cuori") {
            if (a.numero >= 0) energia = std::min(energiaMax, energia + std::max(1, (int)a.numero));
            else { int n = (int)-a.numero; energia -= n - 1; eroe.lampeggio = 0; Ferisci(0); }
        } else if (t == "energia") {
            for (int b : bersagli) {
                entita[b].energia += (int)a.numero;
                if (entita[b].energia <= 0 && entita[b].morte <= 0) { entita[b].energia = 1; entita[b].lampeggio = 0; ColpisciEntita(b, 1); }
            }
        } else if (t == "vai_mappa") {
            for (size_t m = 0; m < mondo.mappe.size(); m++) if (Uguale(mondo.mappe[m].nome, a.testo)) { VaiAMappa((int)m, { -1, -1 }); return; }
        } else if (t == "crea") {
            Vector2 dove = { eroe.pos.x + eroe.dir * 40, eroe.pos.y - 30 };
            if (!bersagli.empty()) { Entita& b = entita[bersagli[0]]; dove = { b.pos.x + Casuale(-20, 20), b.pos.y - 8 }; }
            int n = std::max(1, (int)a.numero);
            for (int k = 0; k < n && k < 20; k++) CreaEntita(a.testo, { dove.x + k * 14.0f, dove.y });
            bersagli.clear();
            if (ent >= 0 && ent < (int)entita.size()) bersagli.push_back(ent);
        } else if (t == "togli") {
            for (int b : bersagli) {
                Rectangle bx = BoxEntita(entita[b]);
                particelle.Esplosione({ bx.x + bx.width / 2, bx.y + bx.height / 2 }, Hex(0xd0e8ff));
                entita[b].attivo = false;
            }
        } else if (t == "anima") { for (int b : bersagli) { entita[b].anim = a.testo; entita[b].tAnim = 0; } }
        else if (t == "velocita") { for (int b : bersagli) entita[b].velMolt = a.numero > 0 ? a.numero : 1; }
        else if (t == "salta") {
            if (bersagli.empty() && Uguale(r.chi, "eroe")) eroe.vel.y = -(a.numero > 0 ? a.numero : fisica.salto);
            for (int b : bersagli) entita[b].vel.y = -(a.numero > 0 ? a.numero : 250.0f);
        } else if (t == "spara") {
            if (bersagli.empty()) SparaDa(-1, a.testo, std::max(1, (int)a.numero));
            for (int b : bersagli) SparaDa(b, a.testo, std::max(1, (int)a.numero));
        } else if (t == "colore") {
            Color c = WHITE;
            if (TestoInColore(a.testo, c)) for (int b : bersagli) entita[b].tinta = c;
        } else if (t == "apri") {
            for (auto& p : entita)
                if (!p.personaggio && lib.oggetti[p.def].funzione == FunzioneOggetto::Porta &&
                    (a.testo.empty() || Uguale(p.nome, a.testo) || Uguale(lib.oggetti[p.def].richiede, a.testo))) { p.aperta = true; p.anim = "aperta"; p.tAnim = 0; }
        } else if (t == "scuoti") scossa = a.numero > 0 ? a.numero : 0.4f;
        else if (t == "spingi") {
            float verso = (!bersagli.empty() && entita[bersagli[0]].pos.x > eroe.pos.x) ? -1.0f : 1.0f;
            eroe.vel.x = verso * (a.numero > 0 ? a.numero : 200);
            if (Laterale()) eroe.vel.y = -150;
        } else if (t == "vittoria") { if (!vinto) { vinto = true; tFine = 0; gAudio.Suona(Effetto::Vittoria); } }
        else if (t == "game_over") { if (!finito) { finito = true; tFine = 0; gAudio.Suona(Effetto::Esplosione); } }
    }
}

// --- Disegno -----------------------------------------------------------------------------

void Gioco::DisegnaEntita(float cx, float cy, bool decorazioni) const {
    for (const Entita& e : entita) {
        if (!e.attivo) continue;
        bool deco = !e.personaggio && lib.oggetti[e.def].funzione == FunzioneOggetto::Decorazione;
        if (deco != decorazioni) continue;
        const Disegno* d = DisegnoEntita(e);
        if (!d) continue;
        float x = e.pos.x - cx, y = e.pos.y - cy;
        if (x < -120 || x > VW + 120 || y < -120 || y > VH + 160) continue;
        // animazione: quella chiesta da una regola, altrimenti secondo lo stato
        int anim = e.anim.empty() ? -1 : d->TrovaAnimazione(e.anim);
        if (anim < 0) {
            if (e.personaggio) {
                if (e.lampeggio > 0.25f) anim = d->TrovaAnimazione("colpito");
                if (anim < 0 && e.spinta > 0) anim = d->TrovaAnimazione("attacco");
                if (anim < 0 && (std::fabs(e.vel.x) > 1 || std::fabs(e.vel.y) > 1)) anim = d->TrovaAnimazione("corsa");
                if (anim < 0) anim = d->TrovaAnimazione("fermo");
            } else {
                const OggettoDef& o = lib.oggetti[e.def];
                if (o.funzione == FunzioneOggetto::Porta) anim = d->TrovaAnimazione(e.aperta ? "aperta" : "chiusa");
            }
            if (anim < 0) anim = 0;
        }
        float tt = e.anim.empty() && anim >= 0 ? (e.lampeggio > 0.25f || e.spinta > 0 ? e.tAnim : e.t) : e.tAnim;
        Color tinta = e.tinta;
        if (e.lampeggio > 0 && std::fmod(e.lampeggio, 0.12f) < 0.06f) tinta = Mescola(tinta, Hex(0xff6060), 0.7f);
        float scala = 1;
        if (e.morte > 0) { tinta = ConAlfa(tinta, 1 - e.morte * 2); scala = 1 + e.morte; }
        if (e.personaggio && lib.personaggi[e.def].comportamento.gravita && Laterale())
            DrawEllipse((int)x, (int)y, BoxEntita(e).width * 0.5f, 1.5f, Hex(0x000000, 60));
        bool galleggia = !e.personaggio && (lib.oggetti[e.def].funzione == FunzioneOggetto::Raccogli || lib.oggetti[e.def].funzione == FunzioneOggetto::Cura ||
                                            lib.oggetti[e.def].funzione == FunzioneOggetto::Chiave) && !lib.oggetti[e.def].gravita;
        if (galleggia) y += std::sin(e.t * 3 + e.origine.x) * 1.5f;
        d->DisegnaBase(anim, tt, { x, y }, e.dir > 0 ? false : e.personaggio, tinta, scala);
        if (mostraCollisioni) {
            Rectangle r = BoxEntita(e);
            DrawRectangleLinesEx({ r.x - cx, r.y - cy, r.width, r.height }, 0.5f, Hex(0xff00ff));
        }
    }
}

void Gioco::DisegnaEroeLibreria(Vector2 p) const {
    const Disegno& d = lib.personaggi[eroeDef].disegno;
    if (eroe.lampeggio > 0 && std::fmod(eroe.lampeggio, 0.16f) < 0.08f) return;
    const char* nome = "fermo";
    switch (eroe.stato) {
        case StatoEroe::Corsa: nome = "corsa"; break;
        case StatoEroe::Salto: nome = "salto"; break;
        case StatoEroe::Caduta: nome = "caduta"; break;
        case StatoEroe::Colpito: nome = "colpito"; break;
        case StatoEroe::Vittoria: nome = "vittoria"; break;
        default: break;
    }
    if (vistaAlto && (std::fabs(eroe.vel.x) > 5 || std::fabs(eroe.vel.y) > 5)) nome = "corsa";
    int a = d.TrovaAnimazione(nome);
    if (a < 0 && eroe.stato == StatoEroe::Caduta) a = d.TrovaAnimazione("salto");
    if (a < 0) a = d.TrovaAnimazione("fermo");
    if (a < 0) a = 0;
    if (Laterale()) DrawEllipse((int)p.x, (int)p.y, 5, 1.2f, Hex(0x000000, 70));
    d.DisegnaBase(a, eroe.stato == StatoEroe::Corsa ? tempoVisivo : eroe.tStato, p, eroe.dir < 0);
}

void Gioco::DisegnaProiettileLib(const Proiettile& q, float cx, float cy) const {
    if (q.disegno < 0 || q.disegno >= (int)lib.oggetti.size()) return;
    const Disegno& d = lib.oggetti[q.disegno].disegno;
    Texture2D t = d.Texture(0, d.FotogrammaAl(0, tempoVisivo));
    if (!t.id) return;
    float w = t.width / d.densita, h = t.height / d.densita;
    float ang = std::atan2(q.vel.y, q.vel.x) * RAD2DEG;
    DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, { q.pos.x - cx, q.pos.y - cy, w, h }, { w / 2, h / 2 }, ang, WHITE);
}

void Gioco::DisegnaPioggia() const {
    float cx = camX;
    for (int i = 0; i < (pioggia == 2 ? 90 : 140); i++) {
        float vel = pioggia == 2 ? 30 + Hash1(i) * 25 : 260 + Hash1(i) * 120;
        float x = std::fmod(Hash1(i * 7) * 900 - cx * 0.9f + (pioggia == 2 ? std::sin(tempoVisivo + i) * 12 : -tempoVisivo * 60) + 9000, (float)VW + 40) - 20;
        float y = std::fmod(Hash1(i * 13) * VH + tempoVisivo * vel, (float)VH + 20) - 10;
        if (pioggia == 2) DrawCircleV({ x, y }, 0.8f + Hash1(i * 3) * 0.9f, ConAlfa(WHITE, 0.85f));
        else DrawLineEx({ x, y }, { x - 2, y + 7 }, 0.6f, ConAlfa(Hex(0xb8d0ff), 0.55f));
    }
}

void Gioco::DisegnaExtraHud() const {
    // variabili del gioco (oltre a monete, gemme...)
    float y = 22;
    for (auto& v : lib.variabili) {
        std::string k = Minuscolo(v.first);
        if (k == "monete" || k == "gemme" || k == "punti") continue;
        auto it = variabili.find(v.first);
        float val = it != variabili.end() ? it->second : v.second;
        Scritta(TextFormat("%s %g", v.first.c_str(), val), 6, (int)y, 10, Hex(0xc8f0ff));
        y += 11;
    }
    // oggetti in tasca (chiavi)
    float x = VW - 8;
    for (auto& o : inventario) {
        if (o.second <= 0) continue;
        const OggettoDef* d = lib.Oggetto(o.first);
        if (!d) continue;
        x -= 16;
        d->disegno.DisegnaIn({ x, 22, 14, 14 }, 0, 0);
        if (o.second > 1) Scritta(TextFormat("%d", o.second), (int)x + 9, 31, 10, WHITE);
    }
    // barra del boss della libreria
    for (const Entita& e : entita) {
        if (!e.attivo || !e.personaggio || e.morte > 0) continue;
        const Comportamento& c = lib.personaggi[e.def].comportamento;
        if (c.tipo != TipoComportamento::Boss) continue;
        float dx = e.pos.x - eroe.pos.x;
        if (std::fabs(dx) > c.vista + 80) continue;
        float k = Limita((float)e.energia / std::max(1, c.energia), 0, 1);
        Rectangle r = { VW / 2.0f - 90, 22, 180, 7 };
        DrawRectangleRec(r, Hex(0x1a0a20, 220));
        DrawRectangle((int)r.x + 1, (int)r.y + 1, (int)((r.width - 2) * k), 5, Mescola(Hex(0xff3f6a), Hex(0xffd23f), 1 - k));
        DrawRectangleLinesEx(r, 1, Hex(0xffd2b8));
        ScrittaCentrata(e.nome.c_str(), VW / 2, 31, 10, Hex(0xffd2b8));
        break;
    }
    // dialogo di un personaggio che parla, oppure messaggio di una regola
    std::string testo, chi;
    if (dialogo >= 0 && dialogo < (int)entita.size()) { testo = lib.personaggi[entita[dialogo].def].comportamento.dialogo; chi = entita[dialogo].nome; }
    else if (tMessaggio > 0) testo = messaggio;
    if (!testo.empty()) {
        Rectangle r = { 40, VH - 66, VW - 80, 50 };
        Pannello(r, Hex(0x1a1428, 235), Hex(0xffd2b8));
        float yy = r.y + 8;
        if (!chi.empty()) {
            if (const Disegno* d = DisegnoEntita(entita[dialogo])) d->DisegnaIn({ r.x + 6, r.y + 6, 36, 38 }, 0, tempoVisivo);
            Scritta(chi.c_str(), (int)r.x + 48, (int)yy, 10, Hex(0xffd23f));
            yy += 12;
        }
        ScrittaACapo(testo, (int)r.x + (chi.empty() ? 10 : 48), (int)yy, (int)r.width - (chi.empty() ? 20 : 58), 10, WHITE, 11);
    }
}

void Gioco::DisegnaHD(RenderTexture2D& rt, int scala) {
    scala = std::max(1, std::min(6, scala));
    if (rt.id == 0 || rt.texture.width != VW * scala) {
        if (rt.id) UnloadRenderTexture(rt);
        rt = LoadRenderTexture(VW * scala, VH * scala);
        SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);
    }
    float prima = gScalaHD;
    gScalaHD = (float)scala;
    BeginTextureMode(rt);
    ClearBackground(BLACK);
    BaseHD();
    Disegna();
    EndMode2D();
    EndTextureMode();
    gScalaHD = prima;
}
