#include "gioco.h"
#include "audio.h"
#include "periferiche.h"
#include "progetto.h"
#include "immagini.h"

namespace {
constexpr float CADUTA_MAX = 420, ACCEL = 1000, ATTRITO = 1400, RIMBALZO = 230;

bool Sovrappone(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }
}

// Riquadro di collisione di ogni oggetto: lo stesso per la logica e per il disegno (F1).
Rectangle BoxOggetto(const Oggetto& o) {
    float x = o.pos.x, y = o.pos.y;
    switch (o.tipo) {
        case TipoOggetto::Moneta: case TipoOggetto::Gemma:
            return o.fisica ? Rectangle{ x - 4, y - 8, 8, 8 } : Rectangle{ x - 5, y - 6, 10, 12 };
        case TipoOggetto::Molla: return { x - 7, y - 8, 14, 8 };
        case TipoOggetto::Forziere: return { x - 8, y - 12, 16, 12 };
        case TipoOggetto::Lanterna: return { x - 6, y - 24, 12, 24 };
        case TipoOggetto::Slime: return { x - 6, y - 10, 12, 10 };
        case TipoOggetto::Pipistrello: return { x - 6, y - 4, 12, 8 };
        case TipoOggetto::Bandiera: return { x - 4, y - 48, 16, 48 };
        case TipoOggetto::Cuore: return { x - 6, y - 6, 12, 12 };
        case TipoOggetto::Cartello: return { x - 10, y - 18, 20, 18 };
        case TipoOggetto::Porta: return { x - 8, y - 28, 16, 28 };
        case TipoOggetto::Fantasma: return { x - 6, y - 7, 12, 14 };
        case TipoOggetto::Torretta: return { x - 7, y - 12, 14, 12 };
        case TipoOggetto::Boss: return { x - 14, y - 28, 28, 28 };
        case TipoOggetto::Cassa: return { x - 7, y - 7, 14, 14 };
    }
    return { x, y, 0, 0 };
}


Comandi Comandi::DaPeriferiche() {
    Comandi c;
    c.asse = gInput.Asse();
    c.asseY = (gInput.Tenuto(Azione::Giu) ? 1.0f : 0.0f) - (gInput.Tenuto(Azione::Su) ? 1.0f : 0.0f);
    if (std::fabs(gInput.LevettaY()) > gInput.m.zonaMorta) c.asseY = Limita(gInput.LevettaY(), -1, 1);
    c.salto = gInput.Tenuto(Azione::Salto);
    c.saltoPremuto = gInput.Premuto(Azione::Salto);
    c.annulla = IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_BACKSPACE);
    if (gInput.m.mouseNelGioco) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { c.mouseGuida = true; c.mouseX = gInput.Mouse().x; c.mouseY = gInput.Mouse().y; }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) c.salto = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) c.saltoPremuto = true;
    }
    return c;
}

bool Gioco::Carica(const std::string& testoLivello, std::string& errore) {
    Mondo m;
    if (!m.Carica(testoLivello, errore)) return false;
    return CaricaMondo(m, errore);
}

bool Gioco::CaricaMondo(const Mondo& m, std::string& errore) {
    std::string problema = m.Controlla();
    if (!problema.empty()) { errore = problema; return false; }
    Mondo copia = m;   // m potrebbe essere proprio 'mondo'
    mondo = copia;
    fisica = mondo.fisica;
    eroe = Eroe{};
    eroe.aspetto = mondo.eroe;
    if (!mondo.titolo.empty()) titolo = mondo.titolo;
    switch (mondo.genere) {
        case Genere::Sparatutto: modello = Modello::Sparatutto; break;
        case Genere::Labirinto: modello = Modello::Labirinto; break;
        case Genere::Puzzle: modello = Modello::Puzzle; break;
        case Genere::OpenWorld: modello = Modello::OpenWorld; break;
        case Genere::Avventura: modello = Modello::Avventura; break;
        default: modello = Modello::Platform; break;
    }
    monete = gemme = 0; vite = 3; punteggio = 0; tempo = 0; mosse = 0;
    energia = energiaMax;
    return EntraMappa(mondo.mappaIniziale, { -1, -1 }, errore);
}

// Prepara la mappa 'indice'. Punteggio, vite e monete restano (si cambia mappa, non partita).
bool Gioco::EntraMappa(int indice, Vector2 arrivo, std::string& errore) {
    if (indice < 0 || indice >= (int)mondo.mappe.size()) { errore = "mappa inesistente"; return false; }
    const Mappa& m = mondo.mappe[indice];
    if (!livello.DaMappa(m, errore)) return false;
    mappaAttuale = indice;
    mappaVerso = -1; mappaDopoVittoria = -1; tTransizione = 0;
    vistaAlto = m.vista == Vista::Alto || modello == Modello::Labirinto || modello == Modello::Puzzle;
    sfondo.tema = m.tema; sfondo.ora = m.ora;
    sfondo.generato = m.sfondoGenerato;
    sfondo.cieloPersonale = m.cieloPersonale;
    sfondo.cieloAlto = m.cieloAlto; sfondo.cieloBasso = m.cieloBasso;
    sfondo.strati = m.strati;
    if (!m.cicloGiorno) cicloGiorno = false;   // altrimenti resta la scelta di chi usa il gioco
    telecamera = m.telecamera;
    zoomBase = telecamera.zoom;
    musica = m.musica;
    testi = m.testi;
    oggetti = livello.Oggetti();
    particelle.Svuota();
    proiettili.clear();
    storiaPuzzle.clear(); storiaEroe.clear();
    gemmeTotali = 0; bossIndice = -1; cartello = -1;
    tSparo = 0; tFineScorrimento = 0; fineScorrimento = false;
    for (auto& o : oggetti) {
        if (o.tipo == TipoOggetto::Gemma) gemmeTotali++;
        if (o.tipo == TipoOggetto::Boss && modello == Modello::Sparatutto && o.numero == 9) o.numero = 30;
    }
    AspettoEroe aspetto = eroe.aspetto;
    eroe = Eroe{};
    eroe.aspetto = aspetto;
    eroe.pos = (arrivo.x >= 0 && arrivo.y >= 0) ? arrivo : livello.Partenza();
    vinto = finito = false; tFine = 0;
    if (modello == Modello::Puzzle || modello == Modello::Labirinto) {
        // Visti dall'alto: l'eroe sta al centro della sua casella.
        eroe.pos = { std::floor(eroe.pos.x / TILE) * TILE + TILE / 2.0f, std::floor((eroe.pos.y - 1) / TILE) * TILE + TILE - 2.0f };
    }
    if (modello == Modello::Sparatutto) eroe.pos.y -= 8;
    checkpoint = eroe.pos;
    portaBloccata = true;
    zoom = zoomBase;
    // Telecamera di partenza: lo scorrimento automatico parte dal lato opposto alla direzione.
    float larg = livello.Larghezza() * (float)TILE, alt = livello.Altezza() * (float)TILE;
    camX = eroe.pos.x - VW / 2.0f;
    camY = eroe.pos.y - VH / 2.0f;
    if (telecamera.modo == ModoTelecamera::Automatica) {
        if (telecamera.dirX > 0) camX = 0;
        if (telecamera.dirX < 0) camX = larg - VW;
        if (telecamera.dirY < 0) camY = alt - VH;
        if (telecamera.dirY > 0) camY = 0;
    }
    AggiornaTelecamera(1, true);
    return true;
}

void Gioco::VaiAMappa(int indice, Vector2 arrivo) {
    if (mappaVerso >= 0 || indice < 0 || indice >= (int)mondo.mappe.size()) return;
    mappaVerso = indice;
    arrivoVerso = arrivo;
    tTransizione = 0;
    gAudio.Suona(Effetto::Conferma, 0.6f);
}

void Gioco::MuoviConCollisioni(Vector2& pos, Vector2& vel, float w, float hgt, float dt, bool& aTerra, bool attraversa) {
    // Asse X.
    pos.x += vel.x * dt;
    Rectangle r = { pos.x - w / 2, pos.y - hgt, w, hgt };
    int ty0 = (int)std::floor(r.y / TILE), ty1 = (int)std::floor((r.y + r.height - 0.01f) / TILE);
    if (vel.x > 0) {
        int tx = (int)std::floor((r.x + r.width) / TILE);
        for (int ty = ty0; ty <= ty1; ty++) if (livello.Solido(tx, ty)) { pos.x = tx * TILE - w / 2 - 0.01f; vel.x = 0; break; }
    } else if (vel.x < 0) {
        int tx = (int)std::floor(r.x / TILE);
        for (int ty = ty0; ty <= ty1; ty++) if (livello.Solido(tx, ty)) { pos.x = (tx + 1) * TILE + w / 2 + 0.01f; vel.x = 0; break; }
    }
    // Asse Y.
    float piediPrima = pos.y;
    pos.y += vel.y * dt;
    aTerra = false;
    r = { pos.x - w / 2, pos.y - hgt, w, hgt };
    int tx0 = (int)std::floor(r.x / TILE), tx1 = (int)std::floor((r.x + r.width - 0.01f) / TILE);
    if (vel.y >= 0) {
        int ty = (int)std::floor(pos.y / TILE);
        for (int tx = tx0; tx <= tx1; tx++) {
            bool piattaforma = attraversa && livello.Piattaforma(tx, ty) && piediPrima <= ty * TILE + 0.5f;
            if (livello.Solido(tx, ty) || piattaforma) { pos.y = (float)ty * TILE; vel.y = 0; aTerra = true; break; }
        }
    } else {
        int ty = (int)std::floor(r.y / TILE);
        for (int tx = tx0; tx <= tx1; tx++)
            if (livello.Solido(tx, ty)) { pos.y = (ty + 1) * TILE + hgt + 0.01f; vel.y = 0; break; }
    }
}

void Gioco::Ferisci(float spintaX) {
    if (eroe.lampeggio > 0 || vinto || finito) return;
    energia--;
    gAudio.Suona(Effetto::Colpo);
    scossa = 0.35f;
    eroe.lampeggio = 1.6f;
    if (Laterale()) { eroe.vel = { spintaX, -200 }; eroe.CambiaStato(StatoEroe::Colpito); }
    particelle.Emetti({ eroe.pos.x, eroe.pos.y - 10 }, 14, Hex(0xff4060), 90, 0.5f, 200, 2, true);
    if (energia <= 0) {
        // Finita l'energia si perde una vita e si riparte dall'ultimo punto di controllo.
        vite--;
        if (vite <= 0) { finito = true; tFine = 0; gAudio.Suona(Effetto::Esplosione); return; }
        energia = energiaMax;
        particelle.Esplosione({ eroe.pos.x, eroe.pos.y - 10 }, Hex(0xff4060));
        Rinasci();
    }
}

void Gioco::Rinasci() {
    if (modello == Modello::Sparatutto) { eroe.pos = { camX + 60, VH * 0.4f + camY }; eroe.vel = { 0, 0 }; eroe.lampeggio = 2; return; }
    eroe.pos = checkpoint;
    eroe.vel = { 0, 0 };
    eroe.lampeggio = 1.2f;
    particelle.Scintille({ eroe.pos.x, eroe.pos.y - 10 }, Hex(0x9fdcff));
}

void Gioco::Fisica(float dt, const Comandi& c) {
    float dirInput = Limita(c.asse, -1, 1);
    if (c.mouseGuida) { // il mouse guida l'eroe verso il puntatore
        float d = c.mouseX + camX - eroe.pos.x;
        dirInput = std::fabs(d) < 6 ? 0.0f : (d > 0 ? 1.0f : -1.0f);
    }
    bool controllo = !(eroe.stato == StatoEroe::Colpito && eroe.tStato < 0.3f) && !vinto && !finito;
    if (!controllo) dirInput = 0;

    if (dirInput != 0) {
        eroe.vel.x += dirInput * ACCEL * dt * (eroe.aTerra ? 1.0f : 0.75f);
        eroe.dir = dirInput > 0 ? 1 : -1;
        // Con la levetta analogica la velocita' massima segue l'inclinazione.
        float massima = fisica.velocita * std::max(0.35f, std::fabs(dirInput));
        eroe.vel.x = Limita(eroe.vel.x, -massima, massima);
    } else if (eroe.aTerra) {
        float a = ATTRITO * dt;
        eroe.vel.x = std::fabs(eroe.vel.x) <= a ? 0 : eroe.vel.x - a * (eroe.vel.x > 0 ? 1 : -1);
    }
    eroe.vel.x = Limita(eroe.vel.x, -fisica.velocita, fisica.velocita);

    // Salto con "coyote time" (si puo' saltare un attimo dopo il bordo) e memoria del tasto.
    coyote = eroe.aTerra ? 0.1f : coyote - dt;
    bufferSalto = (c.saltoPremuto && controllo) ? 0.12f : bufferSalto - dt;
    if (bufferSalto > 0 && coyote > 0) {
        eroe.vel.y = -fisica.salto; bufferSalto = 0; coyote = 0; saltoTagliato = false;
        eroe.CambiaStato(StatoEroe::Salto);
        gAudio.Suona(Effetto::Salto, 0.7f);
        particelle.Polvere(eroe.pos, 6);
    }
    // Salto variabile: lasciando il tasto si salta piu' basso.
    if (!c.salto && eroe.vel.y < -90 && !saltoTagliato) { eroe.vel.y *= 0.5f; saltoTagliato = true; }

    eroe.vel.y = std::min(eroe.vel.y + fisica.gravita * dt, CADUTA_MAX);
    bool eraATerra = eroe.aTerra;
    MuoviConCollisioni(eroe.pos, eroe.vel, Eroe::LARGHEZZA, Eroe::ALTEZZA - 2, dt, eroe.aTerra, true);

    if (eroe.aTerra && !eraATerra) {
        eroe.CambiaStato(StatoEroe::Atterraggio);
        particelle.Polvere(eroe.pos, 8);
        gAudio.Suona(Effetto::Passo, 0.6f);
    }
    if (vinto) eroe.CambiaStato(StatoEroe::Vittoria);
    else if (!eroe.aTerra && eroe.stato != StatoEroe::Colpito) eroe.CambiaStato(eroe.vel.y < 0 ? StatoEroe::Salto : StatoEroe::Caduta);
    else if (eroe.aTerra && eroe.stato != StatoEroe::Atterraggio && !(eroe.stato == StatoEroe::Colpito && eroe.tStato < 0.3f))
        eroe.CambiaStato(std::fabs(eroe.vel.x) > 5 ? StatoEroe::Corsa : StatoEroe::Fermo);

    // Passi e polvere durante la corsa.
    if (eroe.stato == StatoEroe::Corsa) {
        tPasso -= dt;
        if (tPasso <= 0) { tPasso = 0.28f; gAudio.Suona(Effetto::Passo, 0.25f); particelle.Polvere(eroe.pos, 1); }
    }

    // Spine e caduta nel vuoto.
    Rectangle box = eroe.Box();
    for (int ty = (int)(box.y / TILE); ty <= (int)((box.y + box.height) / TILE); ty++)
        for (int tx = (int)(box.x / TILE); tx <= (int)((box.x + box.width) / TILE); tx++)
            if (livello.Spina(tx, ty) && eroe.pos.y > ty * TILE + 6) Ferisci(-eroe.dir * 120.0f);
    if (eroe.pos.y > livello.Altezza() * TILE + 40 && !finito) {
        eroe.lampeggio = 0;
        energia = 1;            // cadere nel vuoto costa una vita intera
        Ferisci(0);
        if (!finito && eroe.pos.y > livello.Altezza() * TILE) Rinasci();
    }
    eroe.AggiornaAnimazione(dt);
}

void Gioco::AggiornaOggetti(float dt) {
    Rectangle box = eroe.Box();
    bool toccaPorta = false;
    for (auto& o : oggetti) {
        if (!o.attivo) continue;
        o.t += dt;
        switch (o.tipo) {
            case TipoOggetto::Moneta: case TipoOggetto::Gemma: {
                if (o.fisica && (vistaAlto || modello == Modello::Sparatutto)) {
                    o.pos.x += o.vel.x * dt * 0.3f; o.pos.y += o.vel.y * dt * 0.1f; o.vel.x *= 1 - dt * 3; o.vel.y *= 1 - dt * 3;
                } else if (o.fisica) {
                    bool terra;
                    o.vel.y = std::min(o.vel.y + fisica.gravita * dt, CADUTA_MAX);
                    o.vel.x *= 1 - dt * 1.5f;
                    MuoviConCollisioni(o.pos, o.vel, 6, 6, dt, terra, true);
                    if (terra) o.vel.y = -o.vel.y * 0.0f;
                    if (o.t < 0.4f) break; // appena uscita: non si raccoglie subito
                }
                Rectangle r = BoxOggetto(o);
                if (Sovrappone(box, r)) {
                    o.attivo = false;
                    punteggio += o.tipo == TipoOggetto::Moneta ? 10 : 50;
                    if (o.tipo == TipoOggetto::Moneta) { monete++; gAudio.Suona(Effetto::Moneta, 0.8f); particelle.Scintille(o.pos, Hex(0xffd23f)); }
                    else { gemme++; gAudio.Suona(Effetto::Gemma); particelle.Esplosione(o.pos, Hex(0x5ef0ff)); scossa = 0.12f; }
                }
                break;
            }
            case TipoOggetto::Molla: {
                o.stato = std::max(0.0f, o.stato - dt * 4);
                Rectangle r = BoxOggetto(o);
                if (eroe.vel.y > 0 && Sovrappone(box, r)) {
                    eroe.vel.y = -fisica.molla; saltoTagliato = true; o.stato = 1;
                    eroe.CambiaStato(StatoEroe::Salto);
                    gAudio.Suona(Effetto::Molla);
                    particelle.Emetti({ o.pos.x, o.pos.y - 8 }, 10, Hex(0xfff070), 80, 0.4f, 100, 1.5f, true);
                }
                break;
            }
            case TipoOggetto::Forziere: {
                Rectangle r = BoxOggetto(o);
                if (o.stato == 0 && Sovrappone(box, r)) {
                    o.stato = 0.01f;
                    gAudio.Suona(Effetto::Forziere);
                    particelle.Esplosione({ o.pos.x, o.pos.y - 10 }, Hex(0xffd23f));
                    Vector2 origine = o.pos; // 'o' non e' piu' valido dopo push_back
                    for (int i = 0; i < 7; i++) {
                        Oggetto m{};
                        m.tipo = i == 3 ? TipoOggetto::Gemma : TipoOggetto::Moneta;
                        m.pos = { origine.x, origine.y - 14 };
                        m.vel = { (i - 3) * 28.0f, -230.0f - (i % 2) * 40 };
                        m.fisica = true; m.attivo = true;
                        oggetti.push_back(m);
                    }
                    return; // il vettore e' cambiato: si riprende al prossimo fotogramma
                }
                if (o.stato > 0) o.stato = std::min(1.0f, o.stato + dt * 3);
                break;
            }
            case TipoOggetto::Lanterna: {
                Rectangle r = BoxOggetto(o);
                if (o.stato == 0 && Sovrappone(box, r)) {
                    o.stato = 1;
                    checkpoint = o.pos;
                    gAudio.Suona(Effetto::Conferma);
                    particelle.Scintille({ o.pos.x, o.pos.y - 20 }, Hex(0xffb347));
                }
                break;
            }
            case TipoOggetto::Slime: case TipoOggetto::Pipistrello: {
                if (o.stato > 0) { o.stato += dt; if (o.stato > 0.4f) o.attivo = false; break; } // schiacciato
                Rectangle r;
                if (o.tipo == TipoOggetto::Slime && !Laterale()) {
                    // Labirinto e sparatutto: va avanti e indietro finche' trova un muro.
                    int davanti = (int)std::floor((o.pos.x + o.dir * 8) / TILE);
                    if (livello.Solido(davanti, (int)std::floor((o.pos.y - 4) / TILE))) o.dir = -o.dir;
                    o.pos.x += o.dir * 30 * dt;
                    if (modello == Modello::Sparatutto) o.pos.y = o.origine.y + std::sin(o.t * 2) * 10;
                    r = BoxOggetto(o);
                } else if (o.tipo == TipoOggetto::Slime) {
                    // Pattuglia: cambia direzione contro i muri o sull'orlo del vuoto.
                    int davanti = (int)std::floor((o.pos.x + o.dir * 7) / TILE);
                    int sotto = (int)std::floor((o.pos.y + 2) / TILE);
                    bool vuoto = !livello.Solido(davanti, sotto) && !livello.Piattaforma(davanti, sotto);
                    bool muro = livello.Solido(davanti, (int)std::floor((o.pos.y - 4) / TILE));
                    if (vuoto || muro) o.dir = -o.dir;
                    float saltello = std::fabs(std::sin(o.t * 5));
                    o.pos.x += o.dir * 22 * dt * (0.3f + saltello);
                    r = BoxOggetto(o);
                } else {
                    o.pos.x = o.origine.x + std::sin(o.t * 0.9f) * 40;
                    o.pos.y = o.origine.y + std::sin(o.t * 2.3f) * 14;
                    o.dir = std::cos(o.t * 0.9f) > 0 ? 1 : -1;
                    r = BoxOggetto(o);
                }
                if (Sovrappone(box, r)) {
                    bool dallAlto = Laterale() && eroe.vel.y > 40 && eroe.pos.y < r.y + r.height * 0.6f;
                    if (dallAlto) {
                        Colpito(o, 1);
                        eroe.vel.y = -RIMBALZO;
                        saltoTagliato = false;
                    } else {
                        Ferisci(eroe.pos.x < o.pos.x ? -150.0f : 150.0f);
                    }
                }
                break;
            }
            case TipoOggetto::Fantasma: {
                if (o.stato > 0) { o.stato += dt; if (o.stato > 0.4f) o.attivo = false; break; }
                // Insegue l'eroe quando e' vicino, attraversando i muri. Rallenta se lo guardi (platform).
                Vector2 d = { eroe.pos.x - o.pos.x, eroe.pos.y - 10 - o.pos.y };
                float dist = std::sqrt(d.x * d.x + d.y * d.y);
                if (dist < 210 && dist > 1) {
                    bool guardato = Laterale() && (eroe.dir > 0) == (d.x < 0);
                    float v = guardato ? 12.0f : 42.0f;
                    o.pos.x += d.x / dist * v * dt;
                    o.pos.y += d.y / dist * v * dt + std::sin(o.t * 3) * 6 * dt;
                    o.dir = d.x > 0 ? 1 : -1;
                    o.vel.x = guardato ? 1.0f : 0.0f; // per il disegno: si copre gli occhi
                }
                if (Sovrappone(box, BoxOggetto(o))) Ferisci(eroe.pos.x < o.pos.x ? -150.0f : 150.0f);
                break;
            }
            case TipoOggetto::Torretta: {
                if (o.stato > 0) { o.stato += dt; if (o.stato > 0.4f) o.attivo = false; break; }
                Vector2 bocca = { o.pos.x, o.pos.y - 9 };
                float dx = eroe.pos.x - bocca.x, dy = eroe.pos.y - 10 - bocca.y;
                o.dir = dx > 0 ? 1 : -1;
                o.vel.x = std::atan2(dy, dx); // angolo della canna, per il disegno
                o.vel.y -= dt;
                if (std::fabs(dx) < 230 && std::fabs(dy) < 160 && o.vel.y <= 0) {
                    o.vel.y = 2.2f;
                    Spara(bocca, { eroe.pos.x, eroe.pos.y - 10 }, 105, false, Hex(0xff7a3a));
                }
                Rectangle r = BoxOggetto(o);
                if (Sovrappone(box, r)) {
                    if (Laterale() && eroe.vel.y > 40 && eroe.pos.y < r.y + 6) { Colpito(o, 1); eroe.vel.y = -RIMBALZO; }
                    else Ferisci(eroe.pos.x < o.pos.x ? -150.0f : 150.0f);
                }
                break;
            }
            case TipoOggetto::Boss: AggiornaBoss(o, dt, box); break;
            case TipoOggetto::Cuore: {
                if (Sovrappone(box, BoxOggetto(o))) {
                    o.attivo = false;
                    if (energia < energiaMax) energia++; else if (vite < 9) vite++;
                    punteggio += 25;
                    gAudio.Suona(Effetto::Gemma, 0.7f);
                    particelle.Scintille(o.pos, Hex(0xff5c8a));
                }
                break;
            }
            case TipoOggetto::Cartello:
                if (Sovrappone(box, BoxOggetto(o))) cartello = o.numero;
                break;
            case TipoOggetto::Porta: {
                o.stato = UscitaAperta() ? std::min(1.0f, o.stato + dt * 2) : 0.0f;
                bool sopra = Sovrappone(box, BoxOggetto(o));
                if (sopra) toccaPorta = true;
                if (o.destMappa >= 0) {
                    // Porta verso un'altra mappa: si entra toccandola (appena arrivati bisogna prima allontanarsi).
                    o.stato = std::min(1.0f, o.stato + dt * 2);
                    if (sopra && !portaBloccata && !vinto && !finito) VaiAMappa(o.destMappa, o.dest);
                } else if (!vinto && UscitaAperta() && sopra) {
                    vinto = true; tFine = 0;
                    gAudio.Suona(Effetto::Vittoria);
                }
                break;
            }
            case TipoOggetto::Cassa: {
                // Nel puzzle la cassa scivola verso la sua casella; altrove e' solo decorazione.
                o.pos.x += (o.origine.x - o.pos.x) * Limita(dt * 18, 0, 1);
                o.pos.y += (o.origine.y - o.pos.y) * Limita(dt * 18, 0, 1);
                o.stato = livello.Bersaglio((int)(o.origine.x / TILE), (int)(o.origine.y / TILE)) ? 1.0f : 0.0f;
                break;
            }
            case TipoOggetto::Bandiera: {
                Rectangle r = BoxOggetto(o);
                if (!vinto && UscitaAperta() && Sovrappone(box, r)) {
                    vinto = true; tFine = 0;
                    mappaDopoVittoria = o.destMappa;
                    gAudio.Suona(Effetto::Vittoria);
                }
                break;
            }
        }
    }
    if (!toccaPorta) portaBloccata = false;
}

void Gioco::Aggiorna(float dt, const Comandi& c) {
    dt = std::min(dt, 1.0f / 30); // niente salti di fisica se il gioco rallenta
    tempoVisivo += dt;
    // Cambio di mappa: dissolvenza al nero, poi la nuova mappa si schiarisce.
    if (mappaVerso >= 0) {
        tTransizione += dt;
        if (tTransizione >= 0.35f) {
            std::string errore;
            int verso = mappaVerso;
            Vector2 arrivo = arrivoVerso;
            mappaVerso = -1;
            if (EntraMappa(verso, arrivo, errore)) tTransizione = -0.35f;   // ora schiarisce
        }
        return;
    }
    if (tTransizione < 0) tTransizione = std::min(0.0f, tTransizione + dt);
    if (!vinto && !finito) tempo += dt;
    if (cicloGiorno) sfondo.ora = std::fmod(sfondo.ora + dt / 240.0f, 1.0f);
    sfondo.Aggiorna(dt);
    cartello = -1;
    switch (modello) {
        case Modello::Labirinto: FisicaLabirinto(dt, c); break;
        case Modello::Puzzle: FisicaPuzzle(dt, c); break;
        case Modello::Sparatutto: FisicaSparatutto(dt, c); break;
        default: if (vistaAlto) FisicaLabirinto(dt, c); else Fisica(dt, c); break;
    }
    AggiornaOggetti(dt);
    AggiornaProiettili(dt);
    particelle.Aggiorna(dt);
    if (vinto || finito) tFine += dt;
    // Bandiera collegata a un'altra mappa: dopo la festa si passa al livello successivo.
    if (vinto && mappaDopoVittoria >= 0 && tFine > 2.6f) VaiAMappa(mappaDopoVittoria, { -1, -1 });

    // Fuochi d'artificio per la vittoria.
    if (vinto && GetRandomValue(0, 100) < 8) {
        Color colori[] = { Hex(0xff5c8a), Hex(0x5ef0ff), Hex(0xffd23f), Hex(0x8cff6a), Hex(0xb07cff) };
        particelle.Esplosione({ camX + Casuale(60, VW - 60), camY + Casuale(30, 120) }, colori[GetRandomValue(0, 4)]);
    }

    AggiornaTelecamera(dt);
    if (scossa > 0) scossa -= dt;
}

void Gioco::AggiornaTelecamera(float dt, bool subito) {
    // Zoom: si avvicina un po' per festeggiare la vittoria.
    float zoomVoluto = zoomBase * (vinto ? 1.25f : 1.0f);
    zoom = subito ? zoomVoluto : zoom + (zoomVoluto - zoom) * Limita(dt * 2, 0, 1);
    float mezzaW = VW / (2 * zoom), mezzaH = VH / (2 * zoom);
    float larg = livello.Larghezza() * (float)TILE, alt = livello.Altezza() * (float)TILE;
    float cx = camX + VW / 2.0f, cy = camY + VH / 2.0f;   // centro attuale
    float segui = subito ? 1.0f : Limita(dt * 4, 0, 1);
    const Telecamera& t = telecamera;
    switch (t.modo) {
        case ModoTelecamera::Automatica:
            if (!vinto && !finito && !subito) { cx += t.dirX * t.velocita * dt; cy += t.dirY * t.velocita * dt; }
            // Sull'asse che non scorre, segue l'eroe.
            if (t.dirX == 0) cx += (eroe.pos.x - cx) * segui;
            if (t.dirY == 0 && t.dirX != 0 && Laterale()) cy += (eroe.pos.y - 30 - cy) * segui;
            break;
        case ModoTelecamera::Stanze: {
            // Lo schermo e' diviso in stanze grandi quanto la visuale: si passa da una all'altra.
            float rw = 2 * mezzaW, rh = 2 * mezzaH;
            float px = (std::floor(eroe.pos.x / rw) + 0.5f) * rw;
            float py = (std::floor((eroe.pos.y - 4) / rh) + 0.5f) * rh;
            float k = subito ? 1.0f : Limita(dt * 9, 0, 1);
            cx += (px - cx) * k;
            cy += (py - cy) * k;
            break;
        }
        case ModoTelecamera::Fissa:
            if (subito) { cx = eroe.pos.x; cy = eroe.pos.y - 30; }
            break;
        default: {
            // Segue l'eroe: dentro la zona morta la telecamera sta ferma, fuori lo insegue.
            float tx = eroe.pos.x + (Laterale() && t.anticipo > 0 ? eroe.dir * t.anticipo + eroe.vel.x * 0.2f : 0);
            float ty = Laterale() ? eroe.pos.y - 30 : eroe.pos.y - 8;
            float dx = tx - cx, dy = ty - cy;
            if (!subito) {
                dx = std::fabs(dx) <= t.zonaMortaX ? 0 : dx - (dx > 0 ? t.zonaMortaX : -t.zonaMortaX);
                dy = std::fabs(dy) <= t.zonaMortaY ? 0 : dy - (dy > 0 ? t.zonaMortaY : -t.zonaMortaY);
            }
            cx += dx * segui;
            cy += dy * segui;
            break;
        }
    }
    float loX = mezzaW, hiX = larg - mezzaW;
    cx = loX > hiX ? larg / 2 : Limita(cx, loX, hiX);
    float loY = mezzaH, hiY = alt - mezzaH;
    // Mappa piu' bassa dello schermo: si appoggia al fondo (il cielo sta sopra).
    cy = loY > hiY ? alt - mezzaH : Limita(cy, loY, hiY);
    if (t.modo == ModoTelecamera::Automatica)
        fineScorrimento = (t.dirX > 0 && cx >= hiX - 0.5f) || (t.dirX < 0 && cx <= loX + 0.5f) ||
                          (t.dirY > 0 && cy >= hiY - 0.5f) || (t.dirY < 0 && cy <= loY + 0.5f) || (t.dirX == 0 && t.dirY == 0);
    else fineScorrimento = true;
    camX = cx - VW / 2.0f;
    camY = cy - VH / 2.0f;
}

void DisegnaOggetto(const Oggetto& o, float camX, float camY, float tempoVisivo, bool vinto) {
    float x = o.pos.x - camX, y = o.pos.y - camY, t = o.t;
    switch (o.tipo) {
        case TipoOggetto::Moneta: {
            float galleggia = o.fisica ? 0 : std::sin(t * 3) * 1.5f;
            float larg = std::fabs(std::cos(t * 4)) * 7 + 1;   // rotazione: la moneta si "assottiglia"
            float cy = y + galleggia - (o.fisica ? 4 : 0);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV({ x, cy }, 7, ConAlfa(Hex(0xffc000), 0.12f));
            EndBlendMode();
            DrawEllipse((int)x, (int)cy, larg / 2 + 1, 5, Hex(0xa86a00));
            DrawEllipse((int)x, (int)cy, larg / 2, 4, Hex(0xffd23f));
            if (larg > 4) DrawRectangle((int)x - 1, (int)cy - 2, 1, 3, Hex(0xfff6b0));
            break;
        }
        case TipoOggetto::Gemma: {
            float cy = y + std::sin(t * 2.5f) * 2 - (o.fisica ? 5 : 0);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV({ x, cy }, 10 + std::sin(t * 5) * 2, ConAlfa(Hex(0x40d0ff), 0.18f));
            EndBlendMode();
            Vector2 a = { x, cy - 6 }, b = { x - 5, cy - 1 }, c = { x, cy + 6 }, d = { x + 5, cy - 1 };
            DrawTriangle(a, b, d, Hex(0x9ff6ff));
            DrawTriangle(b, c, d, Hex(0x2aa8e0));
            DrawTriangle(a, b, { x, cy - 1 }, Hex(0xe0ffff));
            if (std::fmod(t, 1.6f) < 0.15f) { DrawPixel((int)x + 2, (int)cy - 4, WHITE); DrawPixel((int)x + 3, (int)cy - 5, WHITE); }
            break;
        }
        case TipoOggetto::Molla: {
            float comp = o.stato > 0.5f ? (1 - o.stato) * 2 : o.stato * 2; // si comprime e si distende
            float h = 8 - comp * 4 + (o.stato > 0.5f ? 3 : 0) * (o.stato - 0.5f);
            DrawRectangle((int)x - 7, (int)y - 2, 14, 2, Hex(0x555a70));
            for (int i = 0; i < 3; i++) DrawRectangle((int)x - 5 + (i % 2), (int)(y - 2 - (i + 1) * h / 4), 10, 1, Hex(0xc0c6d8));
            DrawRectangle((int)x - 7, (int)(y - 3 - h), 14, 3, Hex(0xff5c5c));
            DrawRectangle((int)x - 7, (int)(y - 3 - h), 14, 1, Hex(0xffa0a0));
            break;
        }
        case TipoOggetto::Forziere: {
            float apertura = o.stato;
            DrawRectangle((int)x - 8, (int)y - 9, 16, 9, Hex(0x8a4a1c));
            DrawRectangle((int)x - 8, (int)y - 9, 16, 1, Hex(0xc07a3a));
            DrawRectangle((int)x - 8, (int)y - 5, 16, 1, Hex(0xd8a020));
            if (apertura > 0) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV({ x, y - 10 }, 12 * apertura, ConAlfa(Hex(0xffd060), 0.3f));
                EndBlendMode();
            }
            float coperchio = -apertura * 6;
            DrawRectangle((int)x - 8, (int)(y - 13 + coperchio), 16, 4, Hex(0xa65a24));
            DrawRectangle((int)x - 8, (int)(y - 13 + coperchio), 16, 1, Hex(0xd88a4a));
            DrawRectangle((int)x - 1, (int)(y - 10 + coperchio * 0.5f), 2, 3, Hex(0xffe060));
            break;
        }
        case TipoOggetto::Lanterna: {
            DrawRectangle((int)x - 1, (int)y - 22, 2, 22, Hex(0x3a3040));
            DrawRectangle((int)x - 4, (int)y - 26, 8, 6, Hex(0x3a3040));
            Color fiamma = o.stato > 0 ? Hex(0xffb347) : Hex(0x5a5068);
            DrawRectangle((int)x - 3, (int)y - 25, 6, 4, fiamma);
            if (o.stato > 0) {
                float tremola = 1 + std::sin(t * 13) * 0.08f + std::sin(t * 7) * 0.05f;
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV({ x, y - 23 }, 22 * tremola, ConAlfa(Hex(0xff9030), 0.12f));
                DrawCircleV({ x, y - 23 }, 9 * tremola, ConAlfa(Hex(0xffd080), 0.3f));
                EndBlendMode();
            }
            break;
        }
        case TipoOggetto::Slime: {
            float schiacciato = o.stato > 0 ? 1 - o.stato / 0.4f : 1;
            float salto = std::fabs(std::sin(t * 5));
            float w = (12 + (1 - salto) * 3) / (o.stato > 0 ? 0.6f : 1), h = (8 + salto * 3) * schiacciato;
            float cy = y - salto * 3;
            DrawEllipse((int)x, (int)y, 6, 1, Hex(0x000000, 60));
            DrawRectangleRounded({ x - w / 2, cy - h, w, h }, 0.6f, 4, Hex(0x3fbf4a));
            DrawRectangleRounded({ x - w / 2 + 1, cy - h + 1, w - 4, h / 2 }, 0.6f, 4, Hex(0x7cff6a));
            if (o.stato == 0) {
                float occhio = x + o.dir * 2;
                DrawRectangle((int)occhio - 3, (int)(cy - h + 3), 2, 3, WHITE);
                DrawRectangle((int)occhio + 2, (int)(cy - h + 3), 2, 3, WHITE);
                DrawRectangle((int)occhio - 2 + (o.dir > 0), (int)(cy - h + 4), 1, 2, Hex(0x102010));
                DrawRectangle((int)occhio + 2 + (o.dir > 0), (int)(cy - h + 4), 1, 2, Hex(0x102010));
            }
            break;
        }
        case TipoOggetto::Pipistrello: {
            float ala = std::sin(t * 18) * 5;
            float scala = o.stato > 0 ? 1 - o.stato / 0.4f : 1;
            Color corpo = Hex(0x5a3a8a), alaCol = Hex(0x7a4ab8);
            DrawTriangle({ x - 3, y }, { x - 11 * scala, y - 2 + ala }, { x - 3, y + 3 }, alaCol);
            DrawTriangle({ x + 3, y }, { x + 3, y + 3 }, { x + 11 * scala, y - 2 + ala }, alaCol);
            DrawCircleV({ x, y + 1 }, 4 * scala, corpo);
            DrawTriangle({ x - 3, y - 2 }, { x - 2, y - 6 }, { x - 1, y - 2 }, corpo);
            DrawTriangle({ x + 1, y - 2 }, { x + 2, y - 6 }, { x + 3, y - 2 }, corpo);
            DrawPixel((int)x - 2, (int)y, Hex(0xff4060));
            DrawPixel((int)x + 1, (int)y, Hex(0xff4060));
            break;
        }
        case TipoOggetto::Bandiera: {
            DrawRectangle((int)x - 1, (int)y - 48, 2, 48, Hex(0xd0d4e0));
            DrawCircleV({ x, y - 49 }, 2, Hex(0xffd23f));
            // Telo che sventola: colonne spostate da un'onda.
            for (int i = 0; i < 16; i++) {
                float onda = std::sin(tempoVisivo * 6 - i * 0.5f) * (i / 16.0f) * 3;
                Color c = vinto ? Hex(0x5ef06a) : Hex(0xff3f6a);
                DrawRectangle((int)x + 1 + i, (int)(y - 47 + onda), 1, 11, Mescola(c, BLACK, (i % 4 == 0) ? 0.15f : 0));
            }
            break;
        }
        case TipoOggetto::Cuore: {
            float cy = y + std::sin(t * 3) * 1.5f, p = 1 + std::sin(t * 6) * 0.08f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleV({ x, cy }, 9, ConAlfa(Hex(0xff3f6a), 0.15f));
            EndBlendMode();
            DrawCircleV({ x - 2.5f * p, cy - 1 }, 3 * p, Hex(0xff3f6a));
            DrawCircleV({ x + 2.5f * p, cy - 1 }, 3 * p, Hex(0xff3f6a));
            DrawTriangle({ x - 5.5f * p, cy }, { x, cy + 6 * p }, { x + 5.5f * p, cy }, Hex(0xff3f6a));
            DrawPixel((int)x - 3, (int)cy - 2, Hex(0xffc0d0));
            break;
        }
        case TipoOggetto::Cartello: {
            DrawRectangle((int)x - 1, (int)y - 10, 2, 10, Hex(0x5a3418));
            DrawRectangle((int)x - 9, (int)y - 18, 18, 10, Hex(0xb8844a));
            DrawRectangleLines((int)x - 9, (int)y - 18, 18, 10, Hex(0x6a4020));
            DrawRectangle((int)x - 6, (int)y - 15, 12, 1, Hex(0x6a4020));
            DrawRectangle((int)x - 6, (int)y - 12, 9, 1, Hex(0x6a4020));
            float b = 0.5f + 0.5f * std::sin(tempoVisivo * 4 + o.numero);
            DrawText("!", (int)x - 1, (int)(y - 30 - b * 2), 10, ConAlfa(Hex(0xffd23f), 0.5f + b * 0.5f));
            break;
        }
        case TipoOggetto::Porta: {
            float apertura = o.stato;
            DrawRectangle((int)x - 10, (int)y - 30, 20, 30, Hex(0x4a4f66));
            DrawRectangle((int)x - 8, (int)y - 28, 16, 28, Hex(0x1a1020));
            if (apertura > 0) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangle((int)x - 8, (int)y - 28, 16, 28, ConAlfa(Hex(0xffd080), 0.35f * apertura));
                DrawCircleV({ x, y - 14 }, 18, ConAlfa(Hex(0xffb347), 0.12f * apertura));
                EndBlendMode();
            }
            float anta = 16 * (1 - apertura * 0.8f);
            DrawRectangle((int)x - 8, (int)y - 28, (int)anta, 28, Hex(0x8a4a1c));
            DrawRectangle((int)x - 8, (int)y - 28, (int)anta, 1, Hex(0xc07a3a));
            DrawRectangle((int)(x - 8 + anta - 3), (int)y - 15, 2, 2, Hex(0xffd23f));
            if (apertura <= 0) { DrawRectangle((int)x - 3, (int)y - 18, 6, 6, Hex(0xffd23f)); DrawRectangle((int)x - 1, (int)y - 16, 2, 3, Hex(0x3a2a10)); }
            break;
        }
        case TipoOggetto::Fantasma: {
            float scala = o.stato > 0 ? 1 - o.stato / 0.4f : 1;
            float alfa = 0.75f + 0.2f * std::sin(t * 5);
            Color c = ConAlfa(Hex(0xe0ecff), alfa * scala);
            DrawCircleV({ x, y - 2 }, 6 * scala, c);
            DrawRectangle((int)(x - 6 * scala), (int)y - 2, (int)(12 * scala), (int)(7 * scala), c);
            for (int i = 0; i < 3; i++) DrawCircleV({ x - 4 * scala + i * 4 * scala, y + 5 * scala + std::sin(t * 8 + i) }, 2 * scala, c);
            if (o.vel.x > 0) DrawRectangle((int)x - 4, (int)y - 4, 8, 3, Hex(0xb8c8e8)); // si copre gli occhi
            else { DrawRectangle((int)x - 3 + o.dir, (int)y - 4, 2, 3, Hex(0x202040)); DrawRectangle((int)x + 1 + o.dir, (int)y - 4, 2, 3, Hex(0x202040)); }
            break;
        }
        case TipoOggetto::Torretta: {
            float scala = o.stato > 0 ? 1 - o.stato / 0.4f : 1;
            DrawRectangle((int)x - 7, (int)(y - 6 * scala), 14, (int)(6 * scala), Hex(0x4a4f66));
            DrawCircleV({ x, y - 7 * scala }, 6 * scala, Hex(0x6f7590));
            Vector2 punta = { x + std::cos(o.vel.x) * 10 * scala, y - 9 + std::sin(o.vel.x) * 10 * scala };
            DrawLineEx({ x, y - 9 }, punta, 3, Hex(0x3a3e52));
            float carica = Limita(1 - o.vel.y / 2.2f, 0, 1);
            DrawCircleV({ x, y - 8 }, 2, Mescola(Hex(0x602010), Hex(0xff7a3a), carica));
            break;
        }
        case TipoOggetto::Boss: {
            bool lampo = o.stato > 0 && std::fmod(o.stato, 0.2f) < 0.1f;
            float respiro = std::sin(t * 3) * 1.5f;
            Color corpo = lampo ? WHITE : Hex(0x7a2a8a), ombra = lampo ? WHITE : Hex(0x4a1a5a);
            DrawEllipse((int)x, (int)y, 14, 2, Hex(0x000000, 80));
            DrawRectangleRounded({ x - 14, y - 26 - respiro, 28, 26 + respiro }, 0.4f, 6, corpo);
            DrawRectangleRounded({ x - 14, y - 10, 28, 10 }, 0.4f, 6, ombra);
            // corna e occhi che guardano l'eroe
            DrawTriangle({ x - 12, y - 24 - respiro }, { x - 16, y - 34 - respiro }, { x - 6, y - 26 - respiro }, Hex(0xe8e0c8));
            DrawTriangle({ x + 6, y - 26 - respiro }, { x + 16, y - 34 - respiro }, { x + 12, y - 24 - respiro }, Hex(0xe8e0c8));
            float oc = o.dir * 3;
            DrawRectangle((int)(x - 8 + oc), (int)(y - 20 - respiro), 5, 5, Hex(0xffd23f));
            DrawRectangle((int)(x + 3 + oc), (int)(y - 20 - respiro), 5, 5, Hex(0xffd23f));
            DrawRectangle((int)(x - 6 + oc + o.dir), (int)(y - 18 - respiro), 2, 2, BLACK);
            DrawRectangle((int)(x + 5 + oc + o.dir), (int)(y - 18 - respiro), 2, 2, BLACK);
            for (int i = 0; i < 4; i++) DrawTriangle({ x - 8 + i * 4.0f, y - 10 }, { x - 6 + i * 4.0f, y - 7 }, { x - 4 + i * 4.0f, y - 10 }, WHITE);
            break;
        }
        case TipoOggetto::Cassa: {
            bool aPosto = o.stato > 0;
            Color legno = aPosto ? Hex(0xd8a020) : Hex(0xa86a30);
            DrawRectangle((int)x - 7, (int)y - 7, 14, 14, legno);
            DrawRectangleLines((int)x - 7, (int)y - 7, 14, 14, Hex(0x5a3418));
            DrawLine((int)x - 6, (int)y - 6, (int)x + 6, (int)y + 6, Hex(0x6a4020));
            DrawLine((int)x + 6, (int)y - 6, (int)x - 6, (int)y + 6, Hex(0x6a4020));
            DrawRectangle((int)x - 7, (int)y + 7, 14, 2, Hex(0x000000, 70));
            if (aPosto) { BeginBlendMode(BLEND_ADDITIVE); DrawRectangle((int)x - 8, (int)y - 8, 16, 16, ConAlfa(Hex(0xffd23f), 0.2f)); EndBlendMode(); }
            break;
        }
    }
}

void Gioco::DisegnaOggettoConInfo(const Oggetto& o) const {
    DisegnaOggetto(o, camX, camY, tempoVisivo, vinto);
    float x = o.pos.x - camX, y = o.pos.y - camY;
    if (mostraEtichette && !o.fisica) {
        const char* nome = NomeOggetto(o.tipo);
        int w = MeasureText(nome, 10);
        float ly = (o.tipo == TipoOggetto::Bandiera || o.tipo == TipoOggetto::Lanterna) ? y - 64 : y - 30;
        if (o.tipo == TipoOggetto::Pipistrello) ly = y - 20;
        DrawRectangle((int)x - w / 2 - 3, (int)ly - 2, w + 6, 13, Hex(0x101428, 200));
        DrawText(nome, (int)x - w / 2, (int)ly, 10, Hex(0xfff3c0));
    }
    if (mostraCollisioni) {
        Rectangle r = BoxOggetto(o);
        DrawRectangleLines((int)(r.x - camX), (int)(r.y - camY), (int)r.width, (int)r.height, Hex(0xff00ff));
    }
}

void Gioco::DisegnaHud() const {
    DrawRectangle(0, 0, VW, 18, Hex(0x0b0e22, 170));
    // Energia (cuori) e vite.
    for (int i = 0; i < energiaMax; i++) {
        Color c = i < energia ? Hex(0xff3f6a) : Hex(0x3a3050);
        int hx = 6 + i * 13;
        DrawCircle(hx + 3, 7, 3, c); DrawCircle(hx + 7, 7, 3, c);
        DrawTriangle({ (float)hx, 8 }, { (float)hx + 5, 14 }, { (float)hx + 10, 8 }, c);
    }
    int x = 8 + energiaMax * 13;
    Scritta(TextFormat("x%d", vite), x, 4, 10, Hex(0xffc0d0));
    x += 22;
    DrawEllipse(x + 4, 9, 4, 5, Hex(0xffd23f));
    Scritta(TextFormat("%02d", monete), x + 11, 4, 10, WHITE);
    x += 34;
    DrawTriangle({ (float)x + 4, 4 }, { (float)x, 9 }, { (float)x + 8, 9 }, Hex(0x9ff6ff));
    DrawTriangle({ (float)x, 9 }, { (float)x + 4, 15 }, { (float)x + 8, 9 }, Hex(0x2aa8e0));
    if (modello == Modello::Labirinto) Scritta(TextFormat("%d/%d", gemme, gemmeTotali), x + 12, 4, 10, gemme >= gemmeTotali ? Hex(0x8cff6a) : WHITE);
    else Scritta(TextFormat("%d", gemme), x + 12, 4, 10, WHITE);
    ScrittaCentrata(titolo.c_str(), VW / 2, 4, 10, Hex(0xfff3c0));
    if (modello == Modello::Puzzle) Scritta(TextFormat("%s %d", lingua == 1 ? "Moves" : "Mosse", mosse), VW - 150, 4, 10, Hex(0xc8d0f0));
    Scritta(TextFormat("%06d", punteggio), VW - 96, 4, 10, Hex(0xffd23f));
    Scritta(TextFormat("%02d:%02d", (int)tempo / 60, (int)tempo % 60), VW - 36, 4, 10, WHITE);

    // Barra del boss.
    if (bossIndice >= 0 && bossIndice < (int)oggetti.size() && oggetti[bossIndice].attivo && !oggetti[bossIndice].sconfitto) {
        const Oggetto& b = oggetti[bossIndice];
        float k = b.numero / (modello == Modello::Sparatutto ? 30.0f : 9.0f);
        Rectangle r = { VW / 2.0f - 90, 22, 180, 7 };
        DrawRectangleRec(r, Hex(0x1a0a20, 220));
        DrawRectangle((int)r.x + 1, (int)r.y + 1, (int)((r.width - 2) * k), 5, Mescola(Hex(0xff3f6a), Hex(0xffd23f), 1 - k));
        DrawRectangleLinesEx(r, 1, Hex(0xffd2b8));
        ScrittaCentrata("BOSS", VW / 2, 31, 10, Hex(0xffd2b8));
    }
    DisegnaCartello();
    if (mostraFps) {
        DrawRectangle(4, 22, 120, 40, Hex(0x000000, 160));
        DrawText(TextFormat("FPS %d", GetFPS()), 8, 25, 10, Hex(0x8cff6a));
        DrawText(TextFormat("oggetti %d  colpi %d", (int)oggetti.size(), (int)proiettili.size()), 8, 37, 10, WHITE);
        DrawText(TextFormat("eroe %d,%d  zoom %.2f", (int)eroe.pos.x, (int)eroe.pos.y, zoom), 8, 49, 10, WHITE);
    }

    if (!mostraEsito) return;
    if (vinto) {
        float k = Limita(tFine * 2, 0, 1);
        DrawRectangle(0, VH / 2 - 30, VW, 60, Hex(0x0b0e22, (unsigned char)(170 * k)));
        ScrittaCentrata(lingua == 1 ? "LEVEL COMPLETE!" : "LIVELLO COMPLETATO!", VW / 2, VH / 2 - 20, 20, Mescola(BLANK, Hex(0xffd23f), k));
        ScrittaCentrata(TextFormat("%s %d   %s %02d:%02d", lingua == 1 ? "Score" : "Punti", punteggio, lingua == 1 ? "Time" : "Tempo",
                                   (int)tempo / 60, (int)tempo % 60), VW / 2, VH / 2 + 6, 10, Mescola(BLANK, WHITE, k));
        if (!crediti.empty()) ScrittaCentrata(crediti.c_str(), VW / 2, VH / 2 + 40, 10, Mescola(BLANK, Hex(0xc8d0f0), k));
    } else if (finito) {
        float k = Limita(tFine * 2, 0, 1);
        DrawRectangle(0, 0, VW, VH, Hex(0x200010, (unsigned char)(150 * k)));
        ScrittaCentrata("GAME OVER", VW / 2, VH / 2 - 20, 20, Mescola(BLANK, Hex(0xff3f6a), k));
        ScrittaCentrata(lingua == 1 ? "Press R to retry" : "Premi R per riprovare", VW / 2, VH / 2 + 6, 10, Mescola(BLANK, WHITE, k));
    }
}

void Gioco::Disegna() {
    float sx = 0, sy = 0;
    if (scossa > 0) { sx = Casuale(-2, 2) * scossa * 8; sy = Casuale(-2, 2) * scossa * 8; }
    float cx = camX + sx, cy = camY + sy;
    float notte = sfondo.Notte();
    bool dallAlto = vistaAlto;

    // Quanto la telecamera e' salita rispetto al fondo: gli strati dello sfondo scendono.
    float dy = std::max(0.0f, livello.Altezza() * (float)TILE - VH - cy);
    if (!dallAlto) sfondo.Disegna(cx, dy);
    // Lo zoom ingrandisce attorno al centro dello schermo; lo sfondo resta fermo.
    Camera2D schermo{};
    schermo.offset = { VW / 2.0f, VH / 2.0f }; schermo.target = { VW / 2.0f, VH / 2.0f }; schermo.zoom = zoom;
    BeginMode2D(schermo);
    if (dallAlto) livello.DisegnaPavimento(cx, cy, sfondo.tema);
    livello.Disegna(cx, cy, tempoVisivo, dallAlto ? 0.0f : notte, dallAlto);
    DisegnaDecorazioni(cx, cy, false);

    float salvaX = camX, salvaY = camY;
    camX = cx; camY = cy;
    for (const auto& o : oggetti) if (o.attivo) DisegnaOggettoConInfo(o);

    Vector2 p = { std::round(eroe.pos.x - cx), std::round(eroe.pos.y - cy) };
    if (modello == Modello::Sparatutto) DisegnaNave(p);
    else {
        Eroe e = eroe;
        e.pos = p;
        DrawEllipse((int)e.pos.x, (int)e.pos.y, 5, 1, Hex(0x000000, 70));
        e.Disegna(tempoVisivo);
    }
    if (mostraCollisioni) {
        Rectangle b = eroe.Box();
        b.x -= cx; b.y -= cy;
        DrawRectangleLinesEx(b, 1, Hex(0x00ffcc));
        DrawText(NomeStato(eroe.stato), (int)b.x - 10, (int)b.y - 12, 10, Hex(0x00ffcc));
    }
    // Proiettili luminosi.
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& q : proiettili) DrawCircleV({ q.pos.x - cx, q.pos.y - cy }, q.amico ? 4.0f : 5.0f, ConAlfa(q.colore, 0.3f));
    EndBlendMode();
    for (const auto& q : proiettili) {
        if (q.amico) DrawRectangle((int)(q.pos.x - cx) - 4, (int)(q.pos.y - cy) - 1, 8, 2, WHITE);
        else DrawCircleV({ q.pos.x - cx, q.pos.y - cy }, 2, Mescola(q.colore, WHITE, 0.4f));
    }
    DisegnaDecorazioni(cx, cy, true);
    EndMode2D();

    // Particelle nel sistema di riferimento del mondo, con lo stesso zoom.
    Camera2D cam{};
    cam.offset = { VW / 2.0f, VH / 2.0f }; cam.target = { cx + VW / 2.0f, cy + VH / 2.0f }; cam.zoom = zoom;
    BeginMode2D(cam);
    particelle.Disegna();
    EndMode2D();
    camX = salvaX; camY = salvaY;

    if (!dallAlto) sfondo.DisegnaPrimoPiano(cx, dy);
    // Atmosfera notturna: vignettatura scura ai bordi.
    if (notte > 0.05f && !dallAlto) {
        DrawRectangleGradientV(0, 0, VW, 60, Hex(0x05081c, (unsigned char)(120 * notte)), BLANK);
        DrawRectangleGradientH(0, 0, 50, VH, Hex(0x05081c, (unsigned char)(100 * notte)), BLANK);
        DrawRectangleGradientH(VW - 50, 0, 50, VH, BLANK, Hex(0x05081c, (unsigned char)(100 * notte)));
    }
    if (mostraHud) DisegnaHud();
    // Dissolvenza nel passaggio da una mappa all'altra.
    float buio = mappaVerso >= 0 ? Limita(tTransizione / 0.35f, 0, 1) : Limita(-tTransizione / 0.35f, 0, 1);
    if (buio > 0) DrawRectangle(0, 0, VW, VH, ConAlfa(BLACK, buio));
}

void Gioco::DisegnaDecorazioni(float cx, float cy, bool primoPiano) const {
    if (mappaAttuale < 0 || mappaAttuale >= (int)mondo.mappe.size()) return;
    for (auto& d : mondo.mappe[mappaAttuale].decorazioni) {
        if (d.primoPiano != primoPiano) continue;
        Texture2D t = Immagine(d.immagine);
        if (t.id == 0) continue;
        float w = t.width * d.scala, h = t.height * d.scala;
        float x = d.x - cx, y = d.y - cy;
        if (x > VW + 64 || y > VH + 64 || x + w < -64 || y + h < -64) continue;
        DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, { std::round(x), std::round(y), w, h }, { 0, 0 }, 0, WHITE);
    }
}

