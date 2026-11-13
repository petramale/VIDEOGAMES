#include "gioco.h"
#include "audio.h"
#include "periferiche.h"
#include "progetto.h"

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
    }
    return { x, y, 0, 0 };
}


Comandi Comandi::DaPeriferiche() {
    Comandi c;
    c.asse = gInput.Asse();
    c.salto = gInput.Tenuto(Azione::Salto);
    c.saltoPremuto = gInput.Premuto(Azione::Salto);
    if (gInput.m.mouseNelGioco) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { c.mouseGuida = true; c.mouseX = gInput.Mouse().x; }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) c.salto = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) c.saltoPremuto = true;
    }
    return c;
}

bool Gioco::Carica(const std::string& testoLivello, std::string& errore) {
    if (!livello.Carica(testoLivello, errore)) return false;
    sorgente = testoLivello;
    Progetto p;
    if (p.LeggiImpostazioni(testoLivello)) { // il livello porta con se' scenario, fisica ed eroe
        sfondo.tema = p.tema; sfondo.ora = p.ora; fisica = p.fisica; eroe.aspetto = p.eroe;
    }
    oggetti = livello.Oggetti();
    particelle.Svuota();
    AspettoEroe aspetto = eroe.aspetto;
    eroe = Eroe{};
    eroe.aspetto = aspetto;
    eroe.pos = livello.Partenza();
    checkpoint = eroe.pos;
    monete = gemme = 0; vite = 3; tempo = 0; vinto = finito = false; tFine = 0;
    camX = eroe.pos.x - VW / 2.0f;
    return true;
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
    if (eroe.lampeggio > 0 || vinto) return;
    vite--;
    gAudio.Suona(Effetto::Colpo);
    scossa = 0.35f;
    eroe.lampeggio = 1.6f;
    eroe.vel = { spintaX, -200 };
    eroe.CambiaStato(StatoEroe::Colpito);
    particelle.Emetti({ eroe.pos.x, eroe.pos.y - 10 }, 14, Hex(0xff4060), 90, 0.5f, 200, 2, true);
    if (vite <= 0) { finito = true; tFine = 0; gAudio.Suona(Effetto::Esplosione); }
}

void Gioco::Rinasci() {
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
        Ferisci(0);
        if (!finito) Rinasci();
    }
    eroe.AggiornaAnimazione(dt);
}

void Gioco::AggiornaOggetti(float dt) {
    Rectangle box = eroe.Box();
    for (auto& o : oggetti) {
        if (!o.attivo) continue;
        o.t += dt;
        switch (o.tipo) {
            case TipoOggetto::Moneta: case TipoOggetto::Gemma: {
                if (o.fisica) {
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
                if (o.tipo == TipoOggetto::Slime) {
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
                    bool dallAlto = eroe.vel.y > 40 && eroe.pos.y < r.y + r.height * 0.6f;
                    if (dallAlto) {
                        o.stato = 0.01f;
                        eroe.vel.y = -RIMBALZO;
                        saltoTagliato = false;
                        gAudio.Suona(Effetto::Esplosione, 0.5f);
                        particelle.Esplosione({ o.pos.x, r.y + 4 }, o.tipo == TipoOggetto::Slime ? Hex(0x7cff6a) : Hex(0xb07cff));
                        scossa = 0.15f;
                    } else {
                        Ferisci(eroe.pos.x < o.pos.x ? -150.0f : 150.0f);
                    }
                }
                break;
            }
            case TipoOggetto::Bandiera: {
                Rectangle r = BoxOggetto(o);
                if (!vinto && Sovrappone(box, r)) {
                    vinto = true; tFine = 0;
                    gAudio.Suona(Effetto::Vittoria);
                }
                break;
            }
        }
    }
}

void Gioco::Aggiorna(float dt, const Comandi& c) {
    dt = std::min(dt, 1.0f / 30); // niente salti di fisica se il gioco rallenta
    tempoVisivo += dt;
    if (!vinto && !finito) tempo += dt;
    if (cicloGiorno) sfondo.ora = std::fmod(sfondo.ora + dt / 240.0f, 1.0f);
    sfondo.Aggiorna(dt);
    Fisica(dt, c);
    AggiornaOggetti(dt);
    particelle.Aggiorna(dt);
    if (vinto || finito) tFine += dt;

    // Fuochi d'artificio per la vittoria.
    if (vinto && GetRandomValue(0, 100) < 8) {
        Color colori[] = { Hex(0xff5c8a), Hex(0x5ef0ff), Hex(0xffd23f), Hex(0x8cff6a), Hex(0xb07cff) };
        particelle.Esplosione({ camX + Casuale(60, VW - 60), camY + Casuale(30, 120) }, colori[GetRandomValue(0, 4)]);
    }

    // Telecamera morbida che guarda un po' avanti nella direzione di marcia.
    float obiettivoX = eroe.pos.x - VW / 2.0f + eroe.dir * 40 + eroe.vel.x * 0.2f;
    camX += (obiettivoX - camX) * Limita(dt * 4, 0, 1);
    camX = Limita(camX, 0, std::max(0.0f, livello.Larghezza() * (float)TILE - VW));
    camY = livello.Altezza() * (float)TILE - VH;
    if (scossa > 0) scossa -= dt;
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
    // Cuori.
    for (int i = 0; i < 3; i++) {
        Color c = i < vite ? Hex(0xff3f6a) : Hex(0x3a3050);
        int hx = 6 + i * 13;
        DrawCircle(hx + 3, 7, 3, c); DrawCircle(hx + 7, 7, 3, c);
        DrawTriangle({ (float)hx, 8 }, { (float)hx + 5, 14 }, { (float)hx + 10, 8 }, c);
    }
    DrawEllipse(56, 9, 4, 5, Hex(0xffd23f));
    Scritta(TextFormat("%02d", monete), 64, 4, 10, WHITE);
    DrawTriangle({ 92, 4 }, { 88, 9 }, { 96, 9 }, Hex(0x9ff6ff));
    DrawTriangle({ 88, 9 }, { 92, 15 }, { 96, 9 }, Hex(0x2aa8e0));
    Scritta(TextFormat("%d", gemme), 100, 4, 10, WHITE);
    ScrittaCentrata(titolo.c_str(), VW / 2, 4, 10, Hex(0xfff3c0));
    Scritta(TextFormat("%02d:%02d", (int)tempo / 60, (int)tempo % 60), VW - 36, 4, 10, WHITE);

    if (vinto) {
        float k = Limita(tFine * 2, 0, 1);
        DrawRectangle(0, VH / 2 - 30, VW, 60, Hex(0x0b0e22, (unsigned char)(170 * k)));
        ScrittaCentrata("LIVELLO COMPLETATO!", VW / 2, VH / 2 - 20, 20, Mescola(BLANK, Hex(0xffd23f), k));
        ScrittaCentrata(TextFormat("Monete %d   Gemme %d   Tempo %02d:%02d", monete, gemme, (int)tempo / 60, (int)tempo % 60),
                        VW / 2, VH / 2 + 6, 10, Mescola(BLANK, WHITE, k));
        if (!crediti.empty()) ScrittaCentrata(crediti.c_str(), VW / 2, VH / 2 + 40, 10, Mescola(BLANK, Hex(0xc8d0f0), k));
    } else if (finito) {
        float k = Limita(tFine * 2, 0, 1);
        DrawRectangle(0, 0, VW, VH, Hex(0x200010, (unsigned char)(150 * k)));
        ScrittaCentrata("GAME OVER", VW / 2, VH / 2 - 20, 20, Mescola(BLANK, Hex(0xff3f6a), k));
        ScrittaCentrata("Premi R per riprovare", VW / 2, VH / 2 + 6, 10, Mescola(BLANK, WHITE, k));
    }
}

void Gioco::Disegna() {
    float sx = 0, sy = 0;
    if (scossa > 0) { sx = Casuale(-2, 2) * scossa * 8; sy = Casuale(-2, 2) * scossa * 8; }
    float cx = camX + sx, cy = camY + sy;
    float notte = sfondo.Notte();

    sfondo.Disegna(cx);
    livello.Disegna(cx, cy, tempoVisivo, notte);

    float salvaX = camX, salvaY = camY;
    camX = cx; camY = cy;
    for (const auto& o : oggetti) if (o.attivo) DisegnaOggettoConInfo(o);

    Eroe e = eroe;
    e.pos = { std::round(eroe.pos.x - cx), std::round(eroe.pos.y - cy) };
    DrawEllipse((int)e.pos.x, (int)e.pos.y, 5, 1, Hex(0x000000, 70));
    e.Disegna(tempoVisivo);
    if (mostraCollisioni) {
        Rectangle b = e.Box();
        DrawRectangleLinesEx(b, 1, Hex(0x00ffcc));
        DrawText(NomeStato(eroe.stato), (int)b.x - 10, (int)b.y - 12, 10, Hex(0x00ffcc));
    }

    // Particelle nel sistema di riferimento dello schermo.
    Camera2D cam{};
    cam.offset = { 0, 0 }; cam.target = { cx, cy }; cam.zoom = 1;
    BeginMode2D(cam);
    particelle.Disegna();
    EndMode2D();
    camX = salvaX; camY = salvaY;

    // Atmosfera notturna: vignettatura scura ai bordi.
    if (notte > 0.05f) {
        DrawRectangleGradientV(0, 0, VW, 60, Hex(0x05081c, (unsigned char)(120 * notte)), BLANK);
        DrawRectangleGradientH(0, 0, 50, VH, Hex(0x05081c, (unsigned char)(100 * notte)), BLANK);
        DrawRectangleGradientH(VW - 50, 0, 50, VH, BLANK, Hex(0x05081c, (unsigned char)(100 * notte)));
    }
    if (mostraHud) DisegnaHud();
}
