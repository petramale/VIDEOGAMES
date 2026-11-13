// Studio Videogiochi: vetrina dei 7 strumenti dello studio e costruttore di eseguibili.
//
//   StudioVideogiochi.exe                           apre lo studio
//   StudioVideogiochi.exe --crea Gioco.exe liv.txt "Titolo"   crea un gioco da riga di comando
//   StudioVideogiochi.exe --esporta-audio cartella             salva effetti e musica in .wav
//
// Un eseguibile creato dallo studio contiene i dati del gioco in coda e parte
// direttamente con il gioco (vedi pacchetto.h).
#include "comune.h"
#include "audio.h"
#include "claude.h"
#include "gioco.h"
#include "librerie.h"
#include "pacchetto.h"
#include "pixelart.h"
#include "sistema.h"
#include <cstring>
#include <fstream>
#include <iterator>

namespace {

enum class Schermata { Menu, Scenari, Personaggi, Oggetti, Audio, Logica, Librerie, Claude };

struct VoceMenu { const char* titolo; const char* descrizione; Schermata schermata; };
const VoceMenu VOCI[] = {
    { "Sfondi e scenari",       "Paesaggi a parallasse, ciclo giorno e notte, 4 temi",       Schermata::Scenari },
    { "Personaggi e animazioni","Eroe animato a stati, colori personalizzabili, pixel art",  Schermata::Personaggi },
    { "Oggetti interattivi",    "Monete, gemme, molle, forzieri, lanterne, nemici",          Schermata::Oggetti },
    { "Effetti e musica",       "Sintetizzatore chiptune e colonna sonora originale",        Schermata::Audio },
    { "Logica di gioco",        "Livello completo: collisioni, scroll, nemici. Crea il .exe",Schermata::Logica },
    { "Librerie",               "Le librerie open-source incluse e le loro licenze",         Schermata::Librerie },
    { "Crea con Claude",        "Descrivi uno sprite o un livello: Claude lo crea",          Schermata::Claude },
};
constexpr int NUM_VOCI = 7;

struct PaletteEroe { const char* nome; AspettoEroe a; };
std::vector<PaletteEroe> PaletteEroi() {
    std::vector<PaletteEroe> p(4);
    p[0].nome = "Avventuriero";
    p[1].nome = "Cavaliere";  p[1].a.maglia = Hex(0xb8c0d8); p[1].a.mantello = Hex(0x2a5bd8); p[1].a.capelli = Hex(0xe8c050); p[1].a.pantaloni = Hex(0x5a6078);
    p[2].nome = "Ninja";      p[2].a.maglia = Hex(0x2a2a3a); p[2].a.mantello = Hex(0x8a1a2a); p[2].a.capelli = Hex(0x1a1a24); p[2].a.pantaloni = Hex(0x1a1a24); p[2].a.scarpe = Hex(0x101018);
    p[3].nome = "Maga";       p[3].a.maglia = Hex(0x7a3ad8); p[3].a.mantello = Hex(0x3ad8b0); p[3].a.capelli = Hex(0xf08ac8); p[3].a.pelle = Hex(0xe8b48a);
    return p;
}

class Studio {
public:
    void Avvia();
    void Aggiorna(float dt);
    void Disegna();
    bool esci = false;

private:
    void Apri(Schermata s);
    void Intestazione(const char* titolo, const char* aiuto) const;
    void Messaggio(const std::string& m) { messaggio = m; tMessaggio = 4; }

    void AggiornaMenu(float dt);      void DisegnaMenu();
    void AggiornaScenari(float dt);   void DisegnaScenari();
    void AggiornaPersonaggi(float dt);void DisegnaPersonaggi();
    void AggiornaAudio(float dt);     void DisegnaAudio();
    void AggiornaLogica(float dt);
    void DisegnaLibrerie();
    void AggiornaClaude(float dt);    void DisegnaClaude();

    Schermata attuale = Schermata::Menu;
    int scelta = 0;
    float tempo = 0;
    std::string messaggio;
    float tMessaggio = 0;

    Sfondo sfondoMenu;
    // 1
    Sfondo scenario; Livello terreno; float camScenario = 0, velScenario = 40; bool scorrimentoAuto = true;
    // 2
    Eroe modello; int statoScelto = 0; bool cicloStati = true; float tCiclo = 0; int palette = 0; SpritePixel fantasma;
    std::vector<PaletteEroe> palettes = PaletteEroi();
    // 3 e 5
    Gioco giocoOggetti, giocoLogica;
    // 4
    int effettoScelto = 0; float livelliCanale[4] = { 0, 0, 0, 0 };
    // 7
    ConfigClaude cfg;
    Claude claude;
    TipoRichiesta tipo = TipoRichiesta::Sprite;
    std::string descrizione[2] = { "un piccolo drago verde che sputa scintille",
                                   "un livello notturno tra le rovine, con gemme nascoste e tanti salti" };
    std::string esito = "Scrivi una descrizione e premi Invio, oppure F2 per un esempio.";
    bool inAttesa = false, provaLivello = false, esempio = false;
    SpritePixel spriteClaude;
    std::string livelloClaude, nomeLivelloClaude;
    Livello anteprimaLivello;
    Gioco giocoClaude;
};

void Studio::Avvia() {
    std::string err;
    terreno.Carica(LivelloAvventura(), err);
    fantasma.CaricaDaJson(Claude::EsempioSprite(), err);
    giocoOggetti.mostraEtichette = true;
    giocoOggetti.cicloGiorno = false;
    giocoOggetti.titolo = "Vetrina degli oggetti";
    giocoOggetti.Carica(LivelloVetrinaOggetti(), err);
    giocoLogica.Carica(LivelloAvventura(), err);
    sfondoMenu.ora = 0.72f;
    cfg = ConfigClaude::Carica();
}

void Studio::Apri(Schermata s) {
    attuale = s;
    tMessaggio = 0;
    gAudio.Suona(Effetto::Conferma, 0.6f);
    std::string err;
    if (s == Schermata::Oggetti) giocoOggetti.Carica(LivelloVetrinaOggetti(), err);
}

void Studio::Intestazione(const char* titolo, const char* aiuto) const {
    DrawRectangle(0, 0, VW, 16, Hex(0x0b0e22, 200));
    Scritta(titolo, 6, 3, 10, Hex(0xffd23f));
    DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
    Scritta(aiuto, 6, VH - 12, 10, Hex(0xc8d0f0));
}

// --- Menu -----------------------------------------------------------------

void Studio::AggiornaMenu(float dt) {
    sfondoMenu.Aggiorna(dt);
    sfondoMenu.ora = std::fmod(sfondoMenu.ora + dt / 90.0f, 1.0f);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { scelta = (scelta + 1) % NUM_VOCI; gAudio.Suona(Effetto::Menu); }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { scelta = (scelta + NUM_VOCI - 1) % NUM_VOCI; gAudio.Suona(Effetto::Menu); }
    for (int i = 0; i < NUM_VOCI; i++) if (IsKeyPressed(KEY_ONE + i)) { scelta = i; Apri(VOCI[i].schermata); return; }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) Apri(VOCI[scelta].schermata);
    if (IsKeyPressed(KEY_ESCAPE)) esci = true;

    // Mouse: la posizione viene riportata alla risoluzione virtuale.
    float scala = std::min((float)GetScreenWidth() / VW, (float)GetScreenHeight() / VH);
    Vector2 m = GetMousePosition();
    m.x = (m.x - (GetScreenWidth() - VW * scala) / 2) / scala;
    m.y = (m.y - (GetScreenHeight() - VH * scala) / 2) / scala;
    Vector2 delta = GetMouseDelta();
    for (int i = 0; i < NUM_VOCI; i++) {
        Rectangle r = { 24, 92.0f + i * 22, 200, 20 };
        if (CheckCollisionPointRec(m, r)) {
            if ((delta.x != 0 || delta.y != 0) && scelta != i) { scelta = i; gAudio.Suona(Effetto::Menu, 0.5f); }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Apri(VOCI[i].schermata);
        }
    }
}

void Studio::DisegnaMenu() {
    sfondoMenu.Disegna(tempo * 18);
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x05081c, 60), Hex(0x05081c, 170));

    // Titolo con lettere che ondeggiano e un bagliore pulsante.
    const char* titolo = "STUDIO VIDEOGIOCHI";
    int dim = 30, larghezza = MeasureText(titolo, dim) + (int)std::strlen(titolo) * 2;
    float x = VW / 2.0f - larghezza / 2.0f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawEllipse(VW / 2, 44, 170 + std::sin(tempo * 2) * 8, 26, ConAlfa(Hex(0xff6a9a), 0.16f));
    EndBlendMode();
    for (int i = 0; titolo[i]; i++) {
        char c[2] = { titolo[i], 0 };
        float y = 30 + std::sin(tempo * 3 + i * 0.45f) * 3;
        Color col = Mescola(Hex(0xffd23f), Hex(0xff5c8a), 0.5f + 0.5f * std::sin(tempo * 2 + i * 0.3f));
        DrawText(c, (int)x + 2, (int)y + 2, dim, Hex(0x1a0a2a));
        DrawText(c, (int)x, (int)y, dim, col);
        x += MeasureText(c, dim) + 2;
    }
    ScrittaCentrata("crea i tuoi videogiochi, con Claude al tuo fianco", VW / 2, 66, 10, Hex(0xe0e6ff));

    for (int i = 0; i < NUM_VOCI; i++) {
        Rectangle r = { 24, 92.0f + i * 22, 200, 20 };
        bool sel = i == scelta;
        float sposta = sel ? 6 + std::sin(tempo * 6) * 1.5f : 0;
        r.x += sposta;
        Pannello(r, sel ? Hex(0x2a2f6a, 230) : Hex(0x101430, 190), sel ? Hex(0xffd23f) : Hex(0x3a4080));
        Scritta(TextFormat("%d", i + 1), (int)r.x + 6, (int)r.y + 5, 10, sel ? Hex(0xffd23f) : Hex(0x8a90c0));
        Scritta(VOCI[i].titolo, (int)r.x + 20, (int)r.y + 5, 10, sel ? WHITE : Hex(0xc8d0f0));
    }
    // Scheda descrittiva della voce scelta.
    Rectangle d = { 246, 92, 214, 152 };
    Pannello(d, Hex(0x101430, 200), Hex(0x3a4080));
    Scritta(VOCI[scelta].titolo, (int)d.x + 8, (int)d.y + 8, 10, Hex(0xffd23f));
    ScrittaACapo(VOCI[scelta].descrizione, (int)d.x + 8, (int)d.y + 24, (int)d.width - 16, 10, Hex(0xe0e6ff));
    // Piccola anteprima animata per ogni voce.
    Vector2 c = { d.x + d.width / 2, d.y + 110 };
    switch (VOCI[scelta].schermata) {
        case Schermata::Personaggi: case Schermata::Logica: {
            Eroe e; e.pos = { c.x, c.y + 20 }; e.stato = StatoEroe::Corsa; e.fase = tempo * 10; e.vel.x = 100;
            Camera2D cam{}; cam.target = e.pos; cam.offset = e.pos; cam.zoom = 2;
            BeginMode2D(cam); e.Disegna(tempo); EndMode2D();
            break;
        }
        case Schermata::Claude: fantasma.Disegna(c, 3, tempo); break;
        case Schermata::Audio:
            for (int i = 0; i < 24; i++) {
                float h = 6 + std::fabs(std::sin(tempo * 4 + i * 0.7f) * std::sin(tempo * 1.3f + i)) * 34;
                DrawRectangle((int)c.x - 72 + i * 6, (int)(c.y + 20 - h), 4, (int)h, Mescola(Hex(0x5ef0ff), Hex(0xff5c8a), i / 24.0f));
            }
            break;
        default: {
            for (int i = 0; i < 5; i++) {
                float a = tempo * 2 + i * 1.256f;
                float px = c.x + std::cos(a) * 40, py = c.y + std::sin(a) * 16;
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV({ px, py }, 8, ConAlfa(Hex(0xffc000), 0.2f));
                EndBlendMode();
                DrawEllipse((int)px, (int)py, std::fabs(std::cos(tempo * 4 + i)) * 4 + 1, 5, Hex(0xffd23f));
            }
        }
    }
    DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
    Scritta("Frecce o mouse: scegli   Invio: apri   M: musica   F11: schermo intero   Esc: esci", 6, VH - 12, 10, Hex(0xc8d0f0));
}

// --- 1. Scenari ----------------------------------------------------------

void Studio::AggiornaScenari(float dt) {
    scenario.Aggiorna(dt);
    if (IsKeyDown(KEY_RIGHT)) velScenario = std::min(velScenario + 200 * dt, 300.0f);
    if (IsKeyDown(KEY_LEFT)) velScenario = std::max(velScenario - 200 * dt, -300.0f);
    if (IsKeyDown(KEY_UP)) scenario.ora = std::fmod(scenario.ora + dt * 0.15f, 1.0f);
    if (IsKeyDown(KEY_DOWN)) scenario.ora = std::fmod(scenario.ora - dt * 0.15f + 1.0f, 1.0f);
    if (IsKeyPressed(KEY_T)) { scenario.tema = (scenario.tema + 1) % (int)Sfondo::Temi().size(); gAudio.Suona(Effetto::Gemma, 0.5f); }
    if (IsKeyPressed(KEY_G)) scorrimentoAuto = !scorrimentoAuto;
    if (scorrimentoAuto) scenario.ora = std::fmod(scenario.ora + dt / 40.0f, 1.0f);
    camScenario += velScenario * dt;
}

void Studio::DisegnaScenari() {
    float larg = terreno.Larghezza() * (float)TILE - VW;
    float cx = std::fmod(camScenario, larg);
    if (cx < 0) cx += larg;
    scenario.Disegna(camScenario);
    terreno.Disegna(cx, terreno.Altezza() * (float)TILE - VH, tempo, scenario.Notte());
    int ore = (int)(scenario.ora * 24), minuti = (int)(scenario.ora * 24 * 60) % 60;
    Rectangle p = { VW - 150, 22, 144, 44 };
    Pannello(p, Hex(0x101430, 200), Hex(0x3a4080));
    Scritta(TextFormat("Tema: %s", Sfondo::Temi()[scenario.tema].nome), (int)p.x + 6, (int)p.y + 6, 10, WHITE);
    Scritta(TextFormat("Ora: %02d:%02d %s", ore, minuti, scorrimentoAuto ? "(scorre)" : ""), (int)p.x + 6, (int)p.y + 18, 10, Hex(0xffd23f));
    Scritta(TextFormat("Velocita': %d px/s", (int)velScenario), (int)p.x + 6, (int)p.y + 30, 10, Hex(0xc8d0f0));
    Intestazione("1. SFONDI E SCENARI", "Destra/Sinistra: velocita'  Su/Giu': ora  T: tema  G: tempo auto  Esc: menu");
}

// --- 2. Personaggi -------------------------------------------------------

void Studio::AggiornaPersonaggi(float dt) {
    const StatoEroe stati[] = { StatoEroe::Fermo, StatoEroe::Corsa, StatoEroe::Salto, StatoEroe::Caduta,
                                StatoEroe::Atterraggio, StatoEroe::Vittoria, StatoEroe::Colpito };
    if (IsKeyPressed(KEY_RIGHT)) { statoScelto = (statoScelto + 1) % 7; cicloStati = false; gAudio.Suona(Effetto::Menu); }
    if (IsKeyPressed(KEY_LEFT)) { statoScelto = (statoScelto + 6) % 7; cicloStati = false; gAudio.Suona(Effetto::Menu); }
    if (IsKeyPressed(KEY_A)) cicloStati = !cicloStati;
    if (IsKeyPressed(KEY_C)) { palette = (palette + 1) % (int)palettes.size(); gAudio.Suona(Effetto::Gemma, 0.5f); }
    if (cicloStati) { tCiclo += dt; if (tCiclo > 2.2f) { tCiclo = 0; statoScelto = (statoScelto + 1) % 7; } }
    StatoEroe s = stati[statoScelto];
    modello.aspetto = palettes[palette].a;
    modello.vel = { s == StatoEroe::Corsa ? 110.0f : 0.0f, s == StatoEroe::Caduta ? 200.0f : 0.0f };
    modello.dir = 1;
    if (s == StatoEroe::Atterraggio && modello.tStato > 0.6f) modello.stato = StatoEroe::Fermo;
    modello.CambiaStato(s);
    if (s == StatoEroe::Colpito) modello.lampeggio = 0;
    modello.AggiornaAnimazione(dt);
}

void Studio::DisegnaPersonaggi() {
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x1a1f4d), Hex(0x3a2a5a));
    for (int i = 0; i < 12; i++) DrawCircle(40 + i * 40, 60 + (i % 3) * 20, 1, Hex(0xffffff, 60));
    // Pedana luminosa.
    BeginBlendMode(BLEND_ADDITIVE);
    DrawEllipse(150, 214, 70, 12, ConAlfa(Hex(0x5e8cff), 0.25f));
    DrawEllipse(360, 214, 50, 9, ConAlfa(Hex(0xff7aa8), 0.2f));
    EndBlendMode();
    DrawEllipse(150, 214, 60, 8, Hex(0x2a3070));
    DrawEllipse(360, 214, 42, 6, Hex(0x4a2a60));

    Eroe e = modello;
    e.pos = { 150, 212 };
    Camera2D cam{}; cam.target = e.pos; cam.offset = e.pos; cam.zoom = 5;
    BeginMode2D(cam); e.Disegna(tempo); EndMode2D();

    float salto = std::fabs(std::sin(tempo * 2.5f)) * 6;
    fantasma.Disegna({ 360, 170 - salto }, 5, tempo);

    Rectangle p = { 12, 24, 170, 70 };
    Pannello(p, Hex(0x101430, 200), Hex(0x3a4080));
    Scritta("Eroe disegnato dal codice", (int)p.x + 6, (int)p.y + 6, 10, Hex(0xffd23f));
    Scritta(TextFormat("Animazione: %s", NomeStato(modello.stato)), (int)p.x + 6, (int)p.y + 22, 10, WHITE);
    Scritta(TextFormat("Aspetto: %s", palettes[palette].nome), (int)p.x + 6, (int)p.y + 36, 10, WHITE);
    Scritta(cicloStati ? "Ciclo automatico: si" : "Ciclo automatico: no", (int)p.x + 6, (int)p.y + 50, 10, Hex(0xc8d0f0));
    Rectangle q = { 286, 24, 182, 44 };
    Pannello(q, Hex(0x101430, 200), Hex(0x3a4080));
    Scritta("Sprite in pixel art", (int)q.x + 6, (int)q.y + 6, 10, Hex(0xffd23f));
    Scritta("formato JSON usato da Claude", (int)q.x + 6, (int)q.y + 22, 10, WHITE);
    Intestazione("2. PERSONAGGI E ANIMAZIONI", "Destra/Sinistra: animazione   A: ciclo automatico   C: cambia aspetto   Esc: menu");
}

// --- 4. Audio ------------------------------------------------------------

void Studio::AggiornaAudio(float dt) {
    const int n = (int)Effetto::Totale;
    if (IsKeyPressed(KEY_RIGHT)) effettoScelto = (effettoScelto + 1) % n;
    if (IsKeyPressed(KEY_LEFT)) effettoScelto = (effettoScelto + n - 1) % n;
    if (IsKeyPressed(KEY_DOWN)) effettoScelto = (effettoScelto + 6) % n;
    if (IsKeyPressed(KEY_UP)) effettoScelto = (effettoScelto + n - 6) % n;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) gAudio.Suona((Effetto)effettoScelto);
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) gAudio.volumeMusica = std::min(1.0f, gAudio.volumeMusica + 0.1f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) gAudio.volumeMusica = std::max(0.0f, gAudio.volumeMusica - 0.1f);
    float t = gAudio.TempoBrano();
    for (int c = 0; c < 4; c++) livelliCanale[c] = std::max(0.0f, livelliCanale[c] - dt * 3);
    for (const auto& nota : gAudio.Note())
        if (t >= nota.inizio && t < nota.inizio + 0.06f && gAudio.MusicaAttiva()) livelliCanale[nota.canale] = 1;
}

void Studio::DisegnaAudio() {
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x0a0820), Hex(0x24104a));
    // Rullo di note: il brano scorre da destra a sinistra, la linea bianca e' "adesso".
    Rectangle rullo = { 10, 22, VW - 20, 120 };
    Pannello(rullo, Hex(0x07061a, 220), Hex(0x3a2a80));
    float adesso = gAudio.TempoBrano(), durata = gAudio.DurataBrano();
    float pxSec = 70, xAdesso = rullo.x + 120;
    const Color colori[4] = { Hex(0xff5c8a), Hex(0x5ef0ff), Hex(0xffd23f), Hex(0x8cff6a) };
    BeginScissorMode((int)rullo.x + 1, (int)rullo.y + 1, (int)rullo.width - 2, (int)rullo.height - 2);
    for (int b = 0; b < 64; b++) { // linee delle battute
        float tb = b * gAudio.Battito() * 4;
        for (float giro : { -durata, 0.0f, durata }) {
            float x = xAdesso + (tb + giro - adesso) * pxSec;
            if (x > rullo.x && x < rullo.x + rullo.width) DrawLine((int)x, (int)rullo.y, (int)x, (int)(rullo.y + rullo.height), Hex(0x2a2060));
        }
    }
    for (const auto& n : gAudio.Note()) {
        for (float giro : { -durata, 0.0f, durata }) {
            float x = xAdesso + (n.inizio + giro - adesso) * pxSec;
            float w = std::max(2.0f, n.durata * pxSec - 1);
            if (x + w < rullo.x || x > rullo.x + rullo.width) continue;
            float y = n.canale == 3 ? rullo.y + rullo.height - 10 - (n.midi - 36) * 3
                                    : rullo.y + rullo.height - 16 - (n.midi - 33) * 1.9f;
            bool suona = adesso >= n.inizio + giro && adesso < n.inizio + giro + n.durata;
            Color c = colori[n.canale];
            if (suona) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawRectangle((int)x - 2, (int)y - 3, (int)w + 4, 8, ConAlfa(c, 0.35f));
                EndBlendMode();
            }
            DrawRectangle((int)x, (int)y - 1, (int)w, 3, suona ? WHITE : ConAlfa(c, 0.8f));
        }
    }
    EndScissorMode();
    DrawLine((int)xAdesso, (int)rullo.y, (int)xAdesso, (int)(rullo.y + rullo.height), WHITE);

    // Oscilloscopio: la forma d'onda reale che esce dalle casse.
    Rectangle osc = { 10, 148, 220, 60 };
    Pannello(osc, Hex(0x07061a, 220), Hex(0x3a2a80));
    long pos = gAudio.PosizioneFrame();
    Vector2 prec = { osc.x, osc.y + osc.height / 2 };
    for (int i = 0; i < (int)osc.width; i++) {
        float v = gAudio.MusicaAttiva() ? gAudio.Campione(pos - 2200 + i * 4) * gAudio.volumeMusica * 1.6f : 0;
        Vector2 p = { osc.x + i, osc.y + osc.height / 2 - Limita(v, -1, 1) * (osc.height / 2 - 3) };
        if (i > 0) DrawLineV(prec, p, Hex(0x5ef0ff));
        prec = p;
    }
    // Indicatori di attivita' dei canali.
    const char* canali[4] = { "Melodia", "Basso", "Arpeggio", "Batteria" };
    for (int c = 0; c < 4; c++) {
        float x = 242 + c * 58;
        DrawRectangle((int)x, 150, 50, 50, Hex(0x07061a, 220));
        float h = livelliCanale[c] * 44;
        DrawRectangle((int)x + 4, (int)(196 - h), 42, (int)h, colori[c]);
        Scritta(canali[c], (int)x + 2, 202, 10, Hex(0xc8d0f0));
    }
    Scritta(TextFormat("Brano originale: La valle delle gemme  -  132 BPM, La minore  -  volume %d%%  %s",
                       (int)(gAudio.volumeMusica * 100 + 0.5f), gAudio.MusicaAttiva() ? "" : "(musica spenta: M)"),
            10, 214, 10, Hex(0xffd23f));

    // Griglia degli effetti sonori.
    for (int i = 0; i < (int)Effetto::Totale; i++) {
        bool sel = i == effettoScelto;
        int x = 10 + (i % 6) * 77, y = 228 + (i / 6) * 13;
        DrawRectangle(x, y, 74, 12, sel ? Hex(0xff5c8a) : Hex(0x1a1440));
        Scritta(Audio::NomeEffetto((Effetto)i), x + 4, y + 1, 10, sel ? WHITE : Hex(0xc8d0f0));
    }
    Intestazione("4. EFFETTI E MUSICA", "Frecce: scegli effetto   Invio: suona   M: musica   +/-: volume   Esc: menu");
}

// --- 5. Logica -----------------------------------------------------------

void Studio::AggiornaLogica(float dt) {
    if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_TAB)) giocoLogica.mostraCollisioni = !giocoLogica.mostraCollisioni;
    if (IsKeyPressed(KEY_R)) { std::string e; giocoLogica.Carica(LivelloAvventura(), e); }
    if (IsKeyPressed(KEY_C)) {
#ifdef _WIN32
        std::string nome = "LaValleDelleGemme.exe";
#else
        std::string nome = "LaValleDelleGemme";
#endif
        std::string percorso = CartellaEseguibile() + "/" + nome, errore;
        DatiGioco dati{ "La valle delle gemme", LivelloAvventura() };
        if (CreaEseguibile(percorso, SerializzaGioco(dati), errore)) { Messaggio("Creato " + nome + " accanto allo studio!"); gAudio.Suona(Effetto::Vittoria); }
        else Messaggio("Errore: " + errore);
    }
    giocoLogica.Aggiorna(dt, Comandi::DaTastiera());
}

// --- 6. Librerie ---------------------------------------------------------

void Studio::DisegnaLibrerie() {
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x0e1230), Hex(0x1a2a4a));
    Scritta("Tutto cio' che serve e' dentro il progetto: nessun download, nessun programma esterno.", 10, 24, 10, WHITE);
    int x[5] = { 10, 132, 182, 290, 0 };
    const char* titoli[4] = { "Libreria", "Versione", "Licenza", "A cosa serve" };
    DrawRectangle(8, 40, VW - 16, 14, Hex(0x2a3070));
    for (int c = 0; c < 4; c++) Scritta(titoli[c], x[c], 42, 10, Hex(0xffd23f));
    int riga = 0;
    for (const auto& l : Librerie()) {
        int y = 58 + riga * 26;
        DrawRectangle(8, y - 2, VW - 16, 24, riga % 2 ? Hex(0x141a40, 200) : Hex(0x1a2050, 200));
        Scritta(l.nome, x[0], y, 10, WHITE);
        Scritta(l.versione, x[1], y, 10, Hex(0x8cff6a));
        Scritta(l.licenza, x[2], y, 10, Hex(0xc8d0f0));
        Scritta(l.uso, x[3], y, 10, Hex(0xc8d0f0));
        DrawText(l.cartella, x[0], y + 11, 10, Hex(0x7a84b8));
        riga++;
    }
    Scritta("Licenze permissive: puoi vendere e distribuire i tuoi giochi liberamente.", 10, 222, 10, Hex(0x8cff6a));
    Scritta("Testi completi delle licenze nella cartella librerie/.", 10, 236, 10, Hex(0xc8d0f0));
    Intestazione("6. LIBRERIE", "Esc: menu");
}

// --- 7. Claude -----------------------------------------------------------

void Studio::AggiornaClaude(float dt) {
    if (provaLivello) {
        giocoClaude.Aggiorna(dt, Comandi::DaTastiera());
        if (IsKeyPressed(KEY_R)) { std::string e; giocoClaude.Carica(livelloClaude, e); }
        if (IsKeyPressed(KEY_ESCAPE)) provaLivello = false;
        return;
    }
    int idx = tipo == TipoRichiesta::Sprite ? 0 : 1;
    std::string& testo = descrizione[idx];
    if (!inAttesa) {
        int c = GetCharPressed();
        while (c > 0) {
            if (c >= 32 && c < 127 && testo.size() < 140) testo.push_back((char)c);
            else if (c == 0xE0 || c == 0xE8 || c == 0xE9 || c == 0xEC || c == 0xF2 || c == 0xF9) {
                // Lettere accentate: il carattere di sistema non le ha, si scrivono con l'apostrofo.
                const char* base = c == 0xE0 ? "a" : (c == 0xE8 || c == 0xE9) ? "e" : c == 0xEC ? "i" : c == 0xF2 ? "o" : "u";
                if (testo.size() < 139) { testo += base; testo += '\''; }
            }
            c = GetCharPressed();
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !testo.empty()) testo.pop_back();
        if (IsKeyPressed(KEY_TAB)) { tipo = tipo == TipoRichiesta::Sprite ? TipoRichiesta::Livello : TipoRichiesta::Sprite; gAudio.Suona(Effetto::Menu); }
        if (IsKeyPressed(KEY_ENTER)) {
            if (cfg.chiave.empty()) esito = "Manca la chiave API: imposta ANTHROPIC_API_KEY o crea claude_chiave.txt accanto allo studio.";
            else { claude.Invia(cfg, tipo, testo); inAttesa = true; esempio = false; esito = "Claude sta creando..."; gAudio.Suona(Effetto::Conferma); }
        }
        if (IsKeyPressed(KEY_F2)) {
            // Carica una risposta d'esempio (identica nel formato a quella di Claude).
            std::string json = tipo == TipoRichiesta::Sprite ? Claude::EsempioSprite() : Claude::EsempioLivello();
            esempio = true;
            std::string err;
            if (tipo == TipoRichiesta::Sprite) { spriteClaude.CaricaDaJson(json, err); livelloClaude.clear(); esito = "Esempio caricato: sprite \"" + spriteClaude.nome + "\""; }
            else if (Claude::LivelloDaJson(json, livelloClaude, nomeLivelloClaude, err) && anteprimaLivello.Carica(livelloClaude, err)) {
                spriteClaude.Scarica(); esito = "Esempio caricato: \"" + nomeLivelloClaude + "\". F5 per giocarlo!";
            }
            gAudio.Suona(Effetto::Gemma);
        }
        if (IsKeyPressed(KEY_F5) && !livelloClaude.empty()) {
            std::string e;
            giocoClaude.titolo = nomeLivelloClaude;
            giocoClaude.sfondo.ora = 0.85f;
            if (giocoClaude.Carica(livelloClaude, e)) provaLivello = true;
            else esito = "Livello non valido: " + e;
        }
    } else if (claude.Stato() >= 2) {
        inAttesa = false;
        std::string r = claude.Risultato(), err;
        if (claude.Stato() == 3) { esito = "Errore: " + r; gAudio.Suona(Effetto::Colpo); return; }
        if (tipo == TipoRichiesta::Sprite) {
            if (spriteClaude.CaricaDaJson(r, err)) { livelloClaude.clear(); esito = "Fatto! Sprite \"" + spriteClaude.nome + "\""; gAudio.Suona(Effetto::Vittoria); }
            else { esito = "Risposta non valida (" + err + "): riprova"; gAudio.Suona(Effetto::Colpo); }
        } else {
            if (Claude::LivelloDaJson(r, livelloClaude, nomeLivelloClaude, err) && anteprimaLivello.Carica(livelloClaude, err)) {
                spriteClaude.Scarica(); esito = "Fatto! \"" + nomeLivelloClaude + "\". F5 per giocarlo!"; gAudio.Suona(Effetto::Vittoria);
            } else { livelloClaude.clear(); esito = "Livello non valido (" + err + "): riprova"; gAudio.Suona(Effetto::Colpo); }
        }
    }
}

void Studio::DisegnaClaude() {
    if (provaLivello) {
        giocoClaude.Disegna();
        DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
        Scritta("Livello creato da Claude   R: ricomincia   Esc: torna allo studio", 6, VH - 12, 10, Hex(0xc8d0f0));
        return;
    }
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x1a1030), Hex(0x3a1a3a));
    // Schede Sprite / Livello.
    const char* schede[2] = { "Sprite animato", "Livello giocabile" };
    for (int i = 0; i < 2; i++) {
        bool sel = (int)tipo == i;
        Rectangle r = { 10.0f + i * 112, 22, 108, 16 };
        Pannello(r, sel ? Hex(0xd97757) : Hex(0x2a1a3a), sel ? Hex(0xffd2b8) : Hex(0x5a3a6a));
        ScrittaCentrata(schede[i], (int)(r.x + r.width / 2), (int)r.y + 4, 10, WHITE);
    }
    // Casella di testo.
    Rectangle box = { 10, 42, 220, 76 };
    Pannello(box, Hex(0x0e0818, 230), Hex(0xd97757));
    std::string testo = descrizione[(int)tipo];
    if (!inAttesa && std::fmod(tempo, 1.0f) < 0.5f) testo += "_";
    ScrittaACapo(testo, (int)box.x + 6, (int)box.y + 6, (int)box.width - 12, 10, WHITE);

    // Stato e chiave.
    Scritta(cfg.chiave.empty() ? "Chiave API: non trovata" : TextFormat("Chiave API: ok  -  modello %s", cfg.modello.c_str()),
            10, 124, 10, cfg.chiave.empty() ? Hex(0xff8a8a) : Hex(0x8cff6a));
    if (inAttesa) {
        for (int i = 0; i < 8; i++) {
            float a = tempo * 6 + i * PI / 4;
            DrawCircleV({ 20 + std::cos(a) * 6, 146 + std::sin(a) * 6 }, 1.5f, ConAlfa(Hex(0xd97757), (i + 1) / 8.0f));
        }
    }
    ScrittaACapo(esito, inAttesa ? 32 : 10, 140, inAttesa ? 196 : 218, 10, Hex(0xffd2b8));

    // Anteprima del risultato.
    Rectangle ant = { 240, 22, 230, 228 };
    Pannello(ant, Hex(0x0e0818, 200), Hex(0x5a3a6a));
    Scritta(esempio ? "Anteprima (risposta d'esempio)" : "Anteprima", (int)ant.x + 6, (int)ant.y + 5, 10, Hex(0xffd23f));
    if (!spriteClaude.fotogrammi.empty()) {
        float scala = std::floor(std::min(150.0f / spriteClaude.larghezza, 150.0f / spriteClaude.altezza));
        float salto = std::fabs(std::sin(tempo * 2.5f)) * 5;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawEllipse((int)(ant.x + ant.width / 2), 200, 60, 10, ConAlfa(Hex(0xd97757), 0.25f));
        EndBlendMode();
        spriteClaude.Disegna({ ant.x + ant.width / 2, 120 - salto }, scala, tempo);
        ScrittaCentrata(spriteClaude.nome.c_str(), (int)(ant.x + ant.width / 2), 220, 10, WHITE);
        ScrittaCentrata(TextFormat("%dx%d, %d fotogrammi", spriteClaude.larghezza, spriteClaude.altezza, (int)spriteClaude.fotogrammi.size()),
                        (int)(ant.x + ant.width / 2), 234, 10, Hex(0xc8d0f0));
    } else if (!livelloClaude.empty()) {
        // Mappa in miniatura: 2 pixel per tile.
        int lw = anteprimaLivello.Larghezza(), lh = anteprimaLivello.Altezza();
        float s = std::min(2.0f, (ant.width - 12) / lw);
        float ox = ant.x + 6, oy = ant.y + 30;
        DrawRectangle((int)ox, (int)oy, (int)(lw * s), (int)(lh * s), Hex(0x1a2a5a));
        for (int y = 0; y < lh; y++)
            for (int x = 0; x < lw; x++) {
                char c = anteprimaLivello.Cella(x, y);
                Color col = c == '#' ? Hex(0x7a4a2a) : c == '=' ? Hex(0x6f7590) : c == '-' ? Hex(0xb87a40) : c == '^' ? Hex(0xc9cfdf) : BLANK;
                if (col.a) DrawRectangle((int)(ox + x * s), (int)(oy + y * s), (int)std::ceil(s), (int)std::ceil(s), col);
            }
        for (const auto& o : anteprimaLivello.Oggetti()) {
            Color col = o.tipo == TipoOggetto::Moneta ? Hex(0xffd23f) : o.tipo == TipoOggetto::Gemma ? Hex(0x5ef0ff)
                      : (o.tipo == TipoOggetto::Slime || o.tipo == TipoOggetto::Pipistrello) ? Hex(0xff5c8a) : Hex(0x8cff6a);
            DrawRectangle((int)(ox + o.pos.x / TILE * s), (int)(oy + o.pos.y / TILE * s) - 1, 2, 2, col);
        }
        ScrittaCentrata(nomeLivelloClaude.c_str(), (int)(ant.x + ant.width / 2), 120, 10, WHITE);
        ScrittaCentrata(TextFormat("%d x %d tile, %d oggetti", lw, lh, (int)anteprimaLivello.Oggetti().size()), (int)(ant.x + ant.width / 2), 134, 10, Hex(0xc8d0f0));
        float b = 0.5f + 0.5f * std::sin(tempo * 4);
        ScrittaCentrata("F5: gioca questo livello", (int)(ant.x + ant.width / 2), 160, 10, Mescola(Hex(0xffd23f), WHITE, b));
    } else {
        ScrittaCentrata("Qui appare cio' che crea Claude", (int)(ant.x + ant.width / 2), 120, 10, Hex(0x8a7aa0));
    }
    Intestazione("7. CREA CON CLAUDE", "Scrivi   Tab: sprite o livello   Invio: chiedi a Claude   F2: esempio   F5: gioca   Esc: menu");
}

// --- Ciclo principale ----------------------------------------------------

void Studio::Aggiorna(float dt) {
    tempo += dt;
    if (tMessaggio > 0) tMessaggio -= dt;
    bool scrivendo = attuale == Schermata::Claude && !provaLivello;
    if (IsKeyPressed(KEY_M) && !scrivendo) gAudio.MusicaAttiva(!gAudio.MusicaAttiva());
    if (attuale != Schermata::Menu && IsKeyPressed(KEY_ESCAPE) && !(attuale == Schermata::Claude && provaLivello)) {
        attuale = Schermata::Menu; gAudio.Suona(Effetto::Menu); return;
    }
    switch (attuale) {
        case Schermata::Menu: AggiornaMenu(dt); break;
        case Schermata::Scenari: AggiornaScenari(dt); break;
        case Schermata::Personaggi: AggiornaPersonaggi(dt); break;
        case Schermata::Oggetti:
            if (IsKeyPressed(KEY_R)) { std::string e; giocoOggetti.Carica(LivelloVetrinaOggetti(), e); }
            giocoOggetti.Aggiorna(dt, Comandi::DaTastiera());
            break;
        case Schermata::Audio: AggiornaAudio(dt); break;
        case Schermata::Logica: AggiornaLogica(dt); break;
        case Schermata::Librerie: break;
        case Schermata::Claude: AggiornaClaude(dt); break;
    }
}

void Studio::Disegna() {
    switch (attuale) {
        case Schermata::Menu: DisegnaMenu(); break;
        case Schermata::Scenari: DisegnaScenari(); break;
        case Schermata::Personaggi: DisegnaPersonaggi(); break;
        case Schermata::Oggetti:
            giocoOggetti.Disegna();
            DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
            Scritta("3. OGGETTI   Frecce/WASD: muovi   Spazio: salta   R: ricomincia   Esc: menu", 6, VH - 12, 10, Hex(0xc8d0f0));
            break;
        case Schermata::Audio: DisegnaAudio(); break;
        case Schermata::Logica:
            giocoLogica.Disegna();
            DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
            Scritta("5. LOGICA   Spazio: salta   F1: collisioni   R: ricomincia   C: crea il .exe   Esc: menu", 6, VH - 12, 10, Hex(0xc8d0f0));
            break;
        case Schermata::Librerie: DisegnaLibrerie(); break;
        case Schermata::Claude: DisegnaClaude(); break;
    }
    if (tMessaggio > 0) {
        int w = MeasureText(messaggio.c_str(), 10) + 16;
        float k = Limita(tMessaggio * 2, 0, 1);
        Rectangle r = { VW / 2.0f - w / 2.0f, 40, (float)w, 18 };
        Pannello(r, Hex(0x101430, (unsigned char)(230 * k)), ConAlfa(Hex(0xffd23f), k));
        DrawText(messaggio.c_str(), (int)r.x + 8, (int)r.y + 4, 10, ConAlfa(WHITE, k));
    }
}

void Presenta(RenderTexture2D& schermo) {
    BeginDrawing();
    ClearBackground(BLACK);
    float scala = std::min((float)GetScreenWidth() / VW, (float)GetScreenHeight() / VH);
    float w = VW * scala, h = VH * scala;
    DrawTexturePro(schermo.texture, { 0, 0, (float)VW, -(float)VH },
                   { (GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h }, { 0, 0 }, 0, WHITE);
    EndDrawing();
}

int GiocaPacchetto(const DatiGioco& dati) {
    InitWindow(VW * 2, VH * 2, dati.titolo.c_str());
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    gAudio.Inizializza();
    gAudio.MusicaAttiva(true);
    RenderTexture2D schermo = LoadRenderTexture(VW, VH);
    Gioco gioco;
    gioco.titolo = dati.titolo;
    std::string errore;
    bool ok = gioco.Carica(dati.livello, errore);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_M)) gAudio.MusicaAttiva(!gAudio.MusicaAttiva());
        if (ok && IsKeyPressed(KEY_R)) gioco.Carica(dati.livello, errore);
        if (ok) gioco.Aggiorna(GetFrameTime(), Comandi::DaTastiera());
        BeginTextureMode(schermo);
        if (ok) gioco.Disegna();
        else { ClearBackground(BLACK); Scritta(("Dati del gioco non validi: " + errore).c_str(), 10, 10, 10, WHITE); }
        EndTextureMode();
        Presenta(schermo);
    }
    UnloadRenderTexture(schermo);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}

int CreaDaRigaDiComando(int argc, char** argv) {
    if (argc < 4) {
        TraceLog(LOG_ERROR, "Uso: StudioVideogiochi --crea Gioco.exe livello.txt [\"Titolo\"]");
        return 1;
    }
    std::ifstream f(argv[3], std::ios::binary);
    if (!f) { TraceLog(LOG_ERROR, "Non trovo il livello %s", argv[3]); return 1; }
    DatiGioco dati{ argc > 4 ? argv[4] : "Il mio gioco",
                    std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()) };
    Livello prova;
    std::string errore;
    if (!prova.Carica(dati.livello, errore)) { TraceLog(LOG_ERROR, "Livello non valido: %s", errore.c_str()); return 1; }
    if (!CreaEseguibile(argv[2], SerializzaGioco(dati), errore)) { TraceLog(LOG_ERROR, "%s", errore.c_str()); return 1; }
    TraceLog(LOG_INFO, "Creato %s", argv[2]);
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--crea") return CreaDaRigaDiComando(argc, argv);
    if (argc > 2 && std::string(argv[1]) == "--esporta-audio") return gAudio.EsportaWav(argv[2]) ? 0 : 1;

    std::string dati;
    DatiGioco gioco;
    if (LeggiPacchettoIncluso(dati) && DeserializzaGioco(dati, gioco)) return GiocaPacchetto(gioco);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VW * 2, VH * 2, "Studio Videogiochi");
    SetWindowMinSize(VW, VH);
    SetExitKey(KEY_NULL); // Esc torna al menu invece di chiudere
    SetTargetFPS(60);
    gAudio.Inizializza();
    gAudio.MusicaAttiva(true);
    RenderTexture2D schermo = LoadRenderTexture(VW, VH);

    Studio studio;
    studio.Avvia();
    while (!WindowShouldClose() && !studio.esci) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        gAudio.Aggiorna(GetFrameTime());
        studio.Aggiorna(GetFrameTime());
        BeginTextureMode(schermo);
        ClearBackground(BLACK);
        studio.Disegna();
        EndTextureMode();
        Presenta(schermo);
    }
    UnloadRenderTexture(schermo);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}
