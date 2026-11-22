// Modelli di gioco oltre al platform: labirinto (visto dall'alto), puzzle con le casse,
// sparatutto a scorrimento. Qui ci sono anche proiettili, boss e nemici colpiti.
#include "gioco.h"
#include "audio.h"
#include <algorithm>

namespace {
bool Sovrappone(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }
}

// --- Nemici colpiti e proiettili ---------------------------------------------

void Gioco::Colpito(Oggetto& o, int danno) {
    if (o.stato > 0 && o.tipo != TipoOggetto::Boss) return;
    if (o.tipo == TipoOggetto::Boss) {
        if (o.sconfitto || o.stato > 0) return;   // lampeggia: invulnerabile per un attimo
        o.numero -= danno;
        o.stato = modello == Modello::Sparatutto ? 0.08f : 0.9f;
        scossa = 0.2f;
        gAudio.Suona(Effetto::Colpo, 0.8f);
        particelle.Esplosione({ o.pos.x, o.pos.y - 16 }, Hex(0xff9a3a));
        if (o.numero <= 0) {
            o.sconfitto = true;
            o.timer = 0;
            punteggio += 1000;
            gAudio.Suona(Effetto::Esplosione);
            scossa = 0.8f;
        }
        return;
    }
    o.stato = 0.01f;
    punteggio += o.tipo == TipoOggetto::Torretta ? 150 : 100;
    gAudio.Suona(Effetto::Esplosione, 0.5f);
    Color c = o.tipo == TipoOggetto::Slime ? Hex(0x7cff6a) : o.tipo == TipoOggetto::Pipistrello ? Hex(0xb07cff)
            : o.tipo == TipoOggetto::Fantasma ? Hex(0xd0e8ff) : Hex(0xff9a3a);
    particelle.Esplosione(o.pos, c);
    scossa = 0.15f;
}

void Gioco::Spara(Vector2 da, Vector2 verso, float velocita, bool amico, Color c) {
    Vector2 d = { verso.x - da.x, verso.y - da.y };
    float l = std::sqrt(d.x * d.x + d.y * d.y);
    if (l < 0.01f) return;
    proiettili.push_back({ da, { d.x / l * velocita, d.y / l * velocita }, amico, 4.0f, c });
}

void Gioco::AggiornaProiettili(float dt) {
    Rectangle box = eroe.Box();
    for (auto& p : proiettili) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vita -= dt;
        if (livello.Solido((int)std::floor(p.pos.x / TILE), (int)std::floor(p.pos.y / TILE))) {
            p.vita = 0;
            particelle.Emetti(p.pos, 4, p.colore, 40, 0.2f, 0, 1.5f, true);
            continue;
        }
        if (p.amico) {
            if (conLibreria && ProiettileSuEntita(p)) continue;
            for (auto& o : oggetti) {
                if (!o.attivo || (o.stato > 0 && o.tipo != TipoOggetto::Boss)) continue;
                bool nemico = o.tipo == TipoOggetto::Slime || o.tipo == TipoOggetto::Pipistrello || o.tipo == TipoOggetto::Fantasma ||
                              o.tipo == TipoOggetto::Torretta || o.tipo == TipoOggetto::Boss;
                if (nemico && CheckCollisionPointRec(p.pos, BoxOggetto(o))) { Colpito(o, 1); p.vita = 0; break; }
            }
        } else if (CheckCollisionPointRec(p.pos, box)) {
            p.vita = 0;
            Ferisci(p.vel.x < 0 ? -120.0f : 120.0f);
        }
    }
    proiettili.erase(std::remove_if(proiettili.begin(), proiettili.end(), [](const Proiettile& p) { return p.vita <= 0; }), proiettili.end());
}

// --- Boss a tre fasi -----------------------------------------------------------
// Fase 1 (energia alta): cammina verso l'eroe.  Fase 2: salta e fa tremare il suolo.
// Fase 3: corre e spara ventagli di colpi. Si colpisce saltandogli in testa
// (nello sparatutto con i colpi).

void Gioco::AggiornaBoss(Oggetto& o, float dt, Rectangle box) {
    if (o.sconfitto) { // esplosioni a catena, poi sparisce
        o.timer += dt;
        if (GetRandomValue(0, 100) < 30) particelle.Esplosione({ o.pos.x + Casuale(-14, 14), o.pos.y - Casuale(0, 28) }, Hex(0xffb347));
        if (o.timer > 1.6f) {
            o.attivo = false;
            bossIndice = -1;
            bool uscita = false;
            for (auto& u : oggetti) if (u.attivo && (u.tipo == TipoOggetto::Bandiera || u.tipo == TipoOggetto::Porta)) uscita = true;
            if (!uscita && !vinto && modello != Modello::Sparatutto) { vinto = true; tFine = 0; gAudio.Suona(Effetto::Vittoria); }
        }
        return;
    }
    float dx = eroe.pos.x - o.pos.x;
    int indice = (int)(&o - &oggetti[0]);
    if (std::fabs(dx) < (modello == Modello::Sparatutto ? VW : 300)) bossIndice = indice;
    if (bossIndice != indice) return; // dorme finche' l'eroe e' lontano
    if (o.stato > 0) o.stato = std::max(0.0f, o.stato - dt);
    o.dir = dx > 0 ? 1 : -1;
    int massimo = modello == Modello::Sparatutto ? 30 : 9;
    float frazione = (float)o.numero / massimo;
    int fase = frazione > 0.67f ? 1 : frazione > 0.34f ? 2 : 3;
    o.timer -= dt;
    o.t += dt;

    if (modello == Modello::Sparatutto) {
        // In volo sul lato destro dello schermo.
        float xVoluta = camX + VW - 60;
        o.pos.x += (xVoluta - o.pos.x) * Limita(dt * 2, 0, 1);
        o.pos.y = camY + VH * 0.55f + std::sin(o.t * (0.8f + fase * 0.4f)) * 70 + 14;
        if (o.timer <= 0) {
            o.timer = fase == 3 ? 0.9f : 1.6f;
            int colpi = fase == 1 ? 1 : fase == 2 ? 3 : 5;
            for (int i = 0; i < colpi; i++) {
                float a = PI + (i - (colpi - 1) / 2.0f) * 0.25f;
                Vector2 da = { o.pos.x - 14, o.pos.y - 14 };
                Spara(da, { da.x + std::cos(a) * 10, da.y + std::sin(a) * 10 }, 120, false, Hex(0xff5c8a));
            }
        }
    } else {
        // A terra, con la gravita'.
        float velocita = fase == 3 ? 70.0f : 35.0f;
        bool aTerra = false;
        Vector2 v = { std::fabs(dx) > 8 ? o.dir * velocita : 0.0f, std::min(o.vy + fisica.gravita * dt, 420.0f) };
        MuoviConCollisioni(o.pos, v, 26, 26, dt, aTerra, true);
        o.vy = v.y;
        if (aTerra && o.inAria) {
            // Atterraggio: onda d'urto lungo il terreno.
            scossa = 0.25f;
            Spara({ o.pos.x - 14, o.pos.y - 4 }, { o.pos.x - 30, o.pos.y - 4 }, 90, false, Hex(0xffd23f));
            Spara({ o.pos.x + 14, o.pos.y - 4 }, { o.pos.x + 30, o.pos.y - 4 }, 90, false, Hex(0xffd23f));
            particelle.Polvere(o.pos, 12);
            gAudio.Suona(Effetto::Esplosione, 0.4f);
        }
        o.inAria = !aTerra;
        if (aTerra && o.timer <= 0 && fase >= 2) {
            o.vy = -fisica.salto * 1.1f;
            o.timer = fase == 3 ? 1.4f : 2.2f;
            if (fase == 3)
                for (int i = 0; i < 5; i++) {
                    float a = -PI / 2 + (i - 2) * 0.45f;
                    Vector2 da = { o.pos.x, o.pos.y - 20 };
                    Spara(da, { da.x + std::cos(a) * 10, da.y + std::sin(a) * 10 }, 110, false, Hex(0xff5c8a));
                }
        }
    }

    Rectangle r = BoxOggetto(o);
    if (Sovrappone(box, r)) {
        if (Laterale() && eroe.vel.y > 40 && eroe.pos.y < r.y + 10) {
            Colpito(o, 1);
            eroe.vel.y = -280;
        } else Ferisci(eroe.pos.x < o.pos.x ? -180.0f : 180.0f);
    }
}

// --- Labirinto: visto dall'alto, si raccolgono tutte le gemme per aprire l'uscita ---

void Gioco::FisicaLabirinto(float dt, const Comandi& c) {
    float ax = Limita(c.asse, -1, 1), ay = Limita(c.asseY, -1, 1);
    if (c.mouseGuida) {
        float dx = c.mouseX + camX - eroe.pos.x, dy = c.mouseY + camY - (eroe.pos.y - 6);
        ax = std::fabs(dx) < 4 ? 0.0f : Limita(dx / 20, -1, 1);
        ay = std::fabs(dy) < 4 ? 0.0f : Limita(dy / 20, -1, 1);
    }
    if (vinto || finito) ax = ay = 0;
    float v = fisica.velocita * 0.85f;
    eroe.vel = { ax * v, ay * v };
    if (ax != 0) eroe.dir = ax > 0 ? 1 : -1;
    bool terra;
    // Collisione solo con la parte bassa del corpo: sembra di camminare "dentro" il corridoio.
    MuoviConCollisioni(eroe.pos, eroe.vel, 10, 8, dt, terra, false);
    eroe.aTerra = true;
    if (vinto) eroe.CambiaStato(StatoEroe::Vittoria);
    else if (eroe.stato != StatoEroe::Colpito || eroe.tStato > 0.3f)
        eroe.CambiaStato(std::fabs(eroe.vel.x) + std::fabs(eroe.vel.y) > 5 ? StatoEroe::Corsa : StatoEroe::Fermo);
    if (eroe.stato == StatoEroe::Corsa) {
        tPasso -= dt;
        if (tPasso <= 0) { tPasso = 0.3f; gAudio.Suona(Effetto::Passo, 0.2f); }
    }
    int tx = (int)std::floor(eroe.pos.x / TILE), ty = (int)std::floor((eroe.pos.y - 4) / TILE);
    if (livello.Spina(tx, ty)) Ferisci(0);
    eroe.AggiornaAnimazione(dt);
}

// --- Puzzle: si spingono le casse sui bersagli, una casella alla volta ---------

bool Gioco::CassaIn(int tx, int ty) const {
    for (auto& o : oggetti)
        if (o.attivo && o.tipo == TipoOggetto::Cassa && (int)(o.origine.x / TILE) == tx && (int)(o.origine.y / TILE) == ty) return true;
    return false;
}

void Gioco::FisicaPuzzle(float dt, const Comandi& c) {
    // L'eroe scivola verso il centro della sua casella.
    // La casella "logica" dell'eroe e' in checkpoint; eroe.pos la raggiunge con calma.
    int hx = (int)std::floor(checkpoint.x / TILE), hy = (int)std::floor((checkpoint.y - 1) / TILE);
    Vector2 centro = { hx * TILE + TILE / 2.0f, hy * TILE + TILE - 2.0f };
    if (c.annulla && !storiaEroe.empty() && !vinto) {
        oggetti = storiaPuzzle.back(); storiaPuzzle.pop_back();
        eroe.pos = checkpoint = storiaEroe.back(); storiaEroe.pop_back();
        mosse = std::max(0, mosse - 1);
        gAudio.Suona(Effetto::Menu);
        return;
    }
    float ax = c.asse, ay = c.asseY;
    int dx = ax > 0.5f ? 1 : ax < -0.5f ? -1 : 0, dy = dx ? 0 : (ay > 0.5f ? 1 : ay < -0.5f ? -1 : 0);
    tPassoPuzzle -= dt;
    if ((dx || dy) && tPassoPuzzle <= 0 && !vinto && !finito) {
        tPassoPuzzle = 0.17f;
        if (dx) eroe.dir = dx;
        int nx = hx + dx, ny = hy + dy;
        bool libero = !livello.Solido(nx, ny);
        if (libero && CassaIn(nx, ny)) {
            int cx = nx + dx, cy = ny + dy;
            if (!livello.Solido(cx, cy) && !CassaIn(cx, cy)) {
                storiaPuzzle.push_back(oggetti); storiaEroe.push_back(centro);
                for (auto& o : oggetti)
                    if (o.attivo && o.tipo == TipoOggetto::Cassa && (int)(o.origine.x / TILE) == nx && (int)(o.origine.y / TILE) == ny) {
                        o.origine = { cx * TILE + TILE / 2.0f, cy * TILE + TILE / 2.0f };
                        gAudio.Suona(livello.Bersaglio(cx, cy) ? Effetto::Gemma : Effetto::Passo, livello.Bersaglio(cx, cy) ? 0.6f : 0.6f);
                        if (livello.Bersaglio(cx, cy)) { particelle.Scintille(o.origine, Hex(0xffd23f)); punteggio += 50; }
                    }
                centro = { nx * TILE + TILE / 2.0f, ny * TILE + TILE - 2.0f };
                mosse++;
            } else { libero = false; gAudio.Suona(Effetto::Passo, 0.3f); }
        } else if (libero) {
            storiaPuzzle.push_back(oggetti); storiaEroe.push_back(centro);
            centro = { nx * TILE + TILE / 2.0f, ny * TILE + TILE - 2.0f };
            mosse++;
            gAudio.Suona(Effetto::Passo, 0.3f);
        }
        if (storiaPuzzle.size() > 400) { storiaPuzzle.erase(storiaPuzzle.begin()); storiaEroe.erase(storiaEroe.begin()); }
        eroe.CambiaStato(libero ? StatoEroe::Corsa : StatoEroe::Fermo);
        checkpoint = centro;
    }
    if (!(dx || dy)) tPassoPuzzle = 0;
    // Movimento morbido verso la casella voluta.
    Vector2 voluta = checkpoint;
    eroe.pos.x += (voluta.x - eroe.pos.x) * Limita(dt * 16, 0, 1);
    eroe.pos.y += (voluta.y - eroe.pos.y) * Limita(dt * 16, 0, 1);
    bool fermo = std::fabs(voluta.x - eroe.pos.x) < 0.5f && std::fabs(voluta.y - eroe.pos.y) < 0.5f;
    eroe.aTerra = true;
    eroe.vel = { 0, 0 };
    if (fermo && eroe.stato == StatoEroe::Corsa) eroe.CambiaStato(StatoEroe::Fermo);

    // Vittoria: ogni cassa sul suo bersaglio.
    int casse = 0, aPosto = 0;
    for (auto& o : oggetti)
        if (o.attivo && o.tipo == TipoOggetto::Cassa) { casse++; if (livello.Bersaglio((int)(o.origine.x / TILE), (int)(o.origine.y / TILE))) aPosto++; }
    if (casse > 0 && aPosto == casse && !vinto && fermo) { vinto = true; tFine = 0; gAudio.Suona(Effetto::Vittoria); }
    if (vinto) eroe.CambiaStato(StatoEroe::Vittoria);
    eroe.AggiornaAnimazione(dt);
}

// --- Sparatutto a scorrimento orizzontale --------------------------------------

void Gioco::FisicaSparatutto(float dt, const Comandi& c) {
    float ax = Limita(c.asse, -1, 1), ay = Limita(c.asseY, -1, 1);
    if (c.mouseGuida) {
        float dx = c.mouseX + camX - eroe.pos.x, dy = c.mouseY + camY - eroe.pos.y;
        ax = std::fabs(dx) < 3 ? 0.0f : Limita(dx / 16, -1, 1);
        ay = std::fabs(dy) < 3 ? 0.0f : Limita(dy / 16, -1, 1);
    }
    if (finito) ax = ay = 0;
    float v = fisica.velocita * 1.3f;
    eroe.vel.x += (ax * v - eroe.vel.x) * Limita(dt * 10, 0, 1);
    eroe.vel.y += (ay * v - eroe.vel.y) * Limita(dt * 10, 0, 1);
    if (vinto) eroe.vel = { 200, -20 };
    eroe.pos.x += eroe.vel.x * dt;
    eroe.pos.y += eroe.vel.y * dt;
    // Resta nello schermo; il bordo sinistro spinge in avanti.
    if (!vinto) eroe.pos.x = Limita(eroe.pos.x, camX + 12, camX + VW - 16);
    eroe.pos.y = Limita(eroe.pos.y, camY + 26, camY + VH - 4);
    eroe.dir = 1;
    // Urto contro le rocce: danno e spinta.
    Rectangle b = { eroe.pos.x - 8, eroe.pos.y - 9, 16, 8 };
    for (int ty = (int)std::floor(b.y / TILE); ty <= (int)std::floor((b.y + b.height) / TILE); ty++)
        for (int tx = (int)std::floor(b.x / TILE); tx <= (int)std::floor((b.x + b.width) / TILE); tx++)
            if (livello.Solido(tx, ty) || livello.Spina(tx, ty)) {
                if (!vinto) Ferisci(0);
                float cy = ty * TILE + TILE / 2.0f;
                eroe.pos.y += (eroe.pos.y - 5 > cy ? 1 : -1) * 60 * dt;
                eroe.pos.x -= 40 * dt;
            }
    // Fuoco automatico tenendo premuto.
    tSparo -= dt;
    if (c.salto && tSparo <= 0 && !vinto && !finito) {
        tSparo = 0.16f;
        Vector2 da = { eroe.pos.x + 10, eroe.pos.y - 5 };
        proiettili.push_back({ da, { 340, 0 }, true, 1.6f, Hex(0x9ff6ff) });
        gAudio.Suona(Effetto::Menu, 0.35f);
    }
    // Fine del percorso: vittoria se il boss non c'e' piu'.
    bool bossVivo = false;
    for (auto& o : oggetti) if (o.attivo && o.tipo == TipoOggetto::Boss) bossVivo = true;
    if (fineScorrimento && !bossVivo && !vinto) {
        tFineScorrimento += dt;
        if (tFineScorrimento > 1.5f) { vinto = true; tFine = 0; gAudio.Suona(Effetto::Vittoria); }
    }
    eroe.stato = vinto ? StatoEroe::Vittoria : StatoEroe::Fermo;
    if (eroe.lampeggio > 0) eroe.lampeggio -= dt;
}

void Gioco::DisegnaNave(Vector2 p) const {
    if (eroe.lampeggio > 0 && std::fmod(eroe.lampeggio, 0.16f) < 0.08f) return;
    const AspettoEroe& a = eroe.aspetto;
    float fiamma = 4 + std::sin(tempoVisivo * 40) * 2 + std::max(0.0f, eroe.vel.x) / 40;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTriangle({ p.x - 9, p.y - 7 }, { p.x - 9 - fiamma * 2, p.y - 5 }, { p.x - 9, p.y - 3 }, ConAlfa(Hex(0xffb347), 0.9f));
    DrawCircleV({ p.x - 9, p.y - 5 }, 4, ConAlfa(Hex(0xff7a3a), 0.25f));
    EndBlendMode();
    float incl = Limita(eroe.vel.y / 200, -1, 1) * 2;
    DrawTriangle({ p.x - 9, p.y - 10 - incl }, { p.x - 9, p.y }, { p.x + 11, p.y - 5 }, a.maglia);
    DrawTriangle({ p.x - 9, p.y - 10 - incl }, { p.x - 3, p.y - 5 }, { p.x + 11, p.y - 5 }, Mescola(a.maglia, WHITE, 0.35f));
    DrawTriangle({ p.x - 7, p.y - 1 }, { p.x - 1, p.y + 3 + incl }, { p.x + 2, p.y - 2 }, a.mantello);
    DrawTriangle({ p.x - 7, p.y - 9 }, { p.x + 2, p.y - 8 }, { p.x - 1, p.y - 13 - incl }, a.mantello);
    DrawCircleV({ p.x + 2, p.y - 6 }, 2, Hex(0x9ff6ff));
    DrawPixel((int)p.x + 1, (int)p.y - 7, WHITE);
}

void Gioco::DisegnaCartello() const {
    if (cartello < 1) return;
    std::string testo;
    for (auto& t : testi) if (t.numero == cartello) testo = lingua == 1 ? t.en : t.it;
    if (testo.empty()) testo = lingua == 1 ? "..." : "...";
    Rectangle r = { 40, VH - 66, VW - 80, 50 };
    Pannello(r, Hex(0x1a1428, 235), Hex(0xffd2b8));
    DrawRectangle((int)r.x + 6, (int)r.y - 6, 14, 10, Hex(0x8a5a2c));
    ScrittaACapo(testo, (int)r.x + 10, (int)r.y + 8, (int)r.width - 20, 10, WHITE, 12);
}
