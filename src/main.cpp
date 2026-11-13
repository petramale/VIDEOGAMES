// Studio Videogiochi: vetrina dei 7 strumenti dello studio, editor dei livelli con la
// chat di Claude, comandi delle periferiche e costruttore di eseguibili.
// Sviluppatore: Mario Petramale - Elvantis.
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
#include "editor.h"
#include "gioco.h"
#include "interfaccia.h"
#include "immagini.h"
#include "mondo.h"
#include <cctype>
#include "librerie.h"
#include "pacchetto.h"
#include "periferiche.h"
#include "progetto.h"
#include "risorse_dati.h"
#include "pixelart.h"
#include "sistema.h"
#include <cstring>
#include <fstream>
#include <iterator>

namespace {

enum class Schermata { Menu, Scenari, Personaggi, Oggetti, Audio, Logica, Librerie, Claude, Editor, Comandi, Mondi };

const char* SVILUPPATORE = "Mario Petramale";
Texture2D gLogo{};   // logo Elvantis, disegnato a risoluzione piena

struct VoceMenu { const char* titolo; const char* descrizione; Schermata schermata; };
const VoceMenu VOCI[] = {
    { "Sfondi e scenari",       "Paesaggi a parallasse, ciclo giorno e notte, 4 temi",       Schermata::Scenari },
    { "Personaggi e animazioni","Eroe animato a stati, colori personalizzabili, pixel art",  Schermata::Personaggi },
    { "Oggetti interattivi",    "Monete, gemme, molle, forzieri, lanterne, nemici",          Schermata::Oggetti },
    { "Effetti e musica",       "Sintetizzatore chiptune e colonna sonora originale",        Schermata::Audio },
    { "Logica di gioco",        "Livello completo: collisioni, scroll, nemici. Crea il .exe",Schermata::Logica },
    { "Librerie",               "Le librerie open-source incluse e le loro licenze",         Schermata::Librerie },
    { "Crea con Claude",        "Descrivi uno sprite o un livello: Claude lo crea",          Schermata::Claude },
    { "Editor + chat Claude",   "Disegna il tuo livello, provalo, crea il .exe. Claude lo modifica parlando con te nella chat", Schermata::Editor },
    { "Comandi e periferiche",  "Tastiera, mouse e joypad: cambia i tasti e prova i comandi dal vivo", Schermata::Comandi },
    { "Nuovo motore: mondi",    "Anteprima del nuovo motore: mappe grandi, vista laterale e dall'alto, scroll in 4 direzioni, telecamera a scelta, porte fra mappe. Formato JSON", Schermata::Mondi },
};
constexpr int NUM_VOCI = 10;
constexpr float MENU_Y = 82, MENU_PASSO = 17, MENU_H = 15;

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
    void DisegnaSopra(Rectangle area);   // a risoluzione piena, sopra l'immagine del gioco
    bool ChatVisibile() const { return attuale == Schermata::Editor && editor.ChatAperta(); }
    Editor editor;
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
    void AggiornaComandi(float dt);   void DisegnaComandi();
    void AggiornaMondi(float dt);     void DisegnaMondi();
    void CaricaMondi();

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
    bool inserisciChiave = false;
    std::string bozzaChiave;
    SpritePixel spriteClaude;
    std::string livelloClaude, nomeLivelloClaude;
    Livello anteprimaLivello;
    Gioco giocoClaude;
    // Mondi del nuovo motore
    std::vector<Mondo> mondi;
    std::vector<std::string> fileMondi;    // "" = mondo d'esempio, altrimenti il file .json
    int sceltaMondo = 0;
    bool giocaMondo = false;
    float tAiutoMondo = 0;
    Gioco giocoMondo;
    // Comandi
    int rigaComando = 0, colonnaComando = 0;
    bool attesaComando = false;
    float tAttesa = 0;
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
    editor.Inizializza();
}

void Studio::Apri(Schermata s) {
    attuale = s;
    tMessaggio = 0;
    gAudio.Suona(Effetto::Conferma, 0.6f);
    std::string err;
    if (s == Schermata::Oggetti) giocoOggetti.Carica(LivelloVetrinaOggetti(), err);
    if (s == Schermata::Editor) editor.Entra();
    if (s == Schermata::Mondi) { CaricaMondi(); giocaMondo = false; }
    if (s == Schermata::Comandi) attesaComando = false;
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
    if (gInput.Premuto(Azione::Giu)) { scelta = (scelta + 1) % NUM_VOCI; gAudio.Suona(Effetto::Menu); }
    if (gInput.Premuto(Azione::Su)) { scelta = (scelta + NUM_VOCI - 1) % NUM_VOCI; gAudio.Suona(Effetto::Menu); }
    for (int i = 0; i < NUM_VOCI; i++) if (IsKeyPressed(i < 9 ? KEY_ONE + i : KEY_ZERO)) { scelta = i; Apri(VOCI[i].schermata); return; }
    if (gInput.Premuto(Azione::Conferma)) { Apri(VOCI[scelta].schermata); return; }
    if (IsKeyPressed(KEY_ESCAPE)) esci = true;

    // Mouse: la posizione arriva gia' nella risoluzione virtuale.
    Vector2 m = gInput.Mouse();
    for (int i = 0; i < NUM_VOCI; i++) {
        Rectangle r = { 24, MENU_Y + i * MENU_PASSO, 200, MENU_H };
        if (CheckCollisionPointRec(m, r)) {
            if (gInput.MouseMosso() && scelta != i) { scelta = i; gAudio.Suona(Effetto::Menu, 0.5f); }
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
        Rectangle r = { 24, MENU_Y + i * MENU_PASSO, 200, MENU_H };
        bool sel = i == scelta;
        float sposta = sel ? 6 + std::sin(tempo * 6) * 1.5f : 0;
        r.x += sposta;
        Pannello(r, sel ? Hex(0x2a2f6a, 230) : Hex(0x101430, 190), sel ? Hex(0xffd23f) : Hex(0x3a4080));
        Scritta(TextFormat("%d", (i + 1) % 10), (int)r.x + 6, (int)r.y + 3, 10, sel ? Hex(0xffd23f) : Hex(0x8a90c0));
        Scritta(VOCI[i].titolo, (int)r.x + 20, (int)r.y + 3, 10, sel ? WHITE : Hex(0xc8d0f0));
    }
    // Scheda descrittiva della voce scelta.
    Rectangle d = { 246, 84, 214, 152 };
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
        case Schermata::Editor: {
            // Mini mappa che si costruisce da sola.
            for (int i = 0; i < 12; i++) {
                int h = 1 + (int)(Rumore1(i * 0.7f) * 3);
                for (int k = 0; k < h; k++)
                    if (std::fmod(tempo * 4, 16.0f) > i) DrawRectangle((int)c.x - 72 + i * 12, (int)c.y + 24 - k * 12, 11, 11, k == h - 1 ? Hex(0x5ac85a) : Hex(0x7a4a2a));
            }
            DrawRectangleRounded({ c.x + 30, c.y - 40, 64, 22 }, 0.4f, 4, Hex(0xd97757));
            Scritta("Claude", (int)c.x + 44, (int)c.y - 34, 10, WHITE);
            break;
        }
        case Schermata::Comandi: {
            const char* nomi[3] = { "Tastiera", "Mouse", "Joypad" };
            for (int i = 0; i < 3; i++) {
                bool attiva = (int)gInput.Ultima() == i;
                Rectangle r = { c.x - 90 + i * 62, c.y - 10, 56, 30 };
                Pannello(r, attiva ? Hex(0x2a5a3a) : Hex(0x1a2050), attiva ? Hex(0x8cff6a) : Hex(0x3a4080));
                ScrittaCentrata(nomi[i], (int)(r.x + r.width / 2), (int)r.y + 10, 10, WHITE);
            }
            break;
        }
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
    Scritta(TextFormat("Sviluppato da %s", SVILUPPATORE), (int)d.x, (int)(d.y + d.height + 4), 10, Hex(0xffd2b8));
    DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
    Scritta("Frecce, mouse o joypad: scegli   Invio/A: apri   M: musica   F11: schermo intero   Esc: esci", 6, VH - 12, 10, Hex(0xc8d0f0));
}

// --- 1. Scenari ----------------------------------------------------------

void Studio::AggiornaScenari(float dt) {
    scenario.Aggiorna(dt);
    if (gInput.Tenuto(Azione::Destra)) velScenario = std::min(velScenario + 200 * dt, 300.0f);
    if (gInput.Tenuto(Azione::Sinistra)) velScenario = std::max(velScenario - 200 * dt, -300.0f);
    if (gInput.Tenuto(Azione::Su)) scenario.ora = std::fmod(scenario.ora + dt * 0.15f, 1.0f);
    if (gInput.Tenuto(Azione::Giu)) scenario.ora = std::fmod(scenario.ora - dt * 0.15f + 1.0f, 1.0f);
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
    if (gInput.Premuto(Azione::Destra)) { statoScelto = (statoScelto + 1) % 7; cicloStati = false; gAudio.Suona(Effetto::Menu); }
    if (gInput.Premuto(Azione::Sinistra)) { statoScelto = (statoScelto + 6) % 7; cicloStati = false; gAudio.Suona(Effetto::Menu); }
    if (IsKeyPressed(KEY_A)) cicloStati = !cicloStati;
    if (IsKeyPressed(KEY_C) || gInput.Premuto(Azione::Conferma)) { palette = (palette + 1) % (int)palettes.size(); gAudio.Suona(Effetto::Gemma, 0.5f); }
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
    if (gInput.Premuto(Azione::Destra)) effettoScelto = (effettoScelto + 1) % n;
    if (gInput.Premuto(Azione::Sinistra)) effettoScelto = (effettoScelto + n - 1) % n;
    if (gInput.Premuto(Azione::Giu)) effettoScelto = (effettoScelto + 6) % n;
    if (gInput.Premuto(Azione::Su)) effettoScelto = (effettoScelto + n - 6) % n;
    if (gInput.Premuto(Azione::Conferma)) gAudio.Suona((Effetto)effettoScelto);
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
    giocoLogica.Aggiorna(dt, Comandi::DaPeriferiche());
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
    if (inserisciChiave) {
        // Finestra per incollare la chiave API: Ctrl+V incolla, Invio salva, Esc annulla.
        int c = GetCharPressed();
        while (c > 0) { if (c > 32 && c < 127 && bozzaChiave.size() < 200) bozzaChiave.push_back((char)c); c = GetCharPressed(); }
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrl && IsKeyPressed(KEY_V)) {
            const char* t = GetClipboardText();
            if (t) for (const char* p = t; *p; p++) if (*p > 32 && *p < 127) bozzaChiave.push_back(*p);
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !bozzaChiave.empty()) bozzaChiave.pop_back();
        if (IsKeyPressed(KEY_ENTER)) {
            if (bozzaChiave.rfind("sk-ant-", 0) != 0) esito = "La chiave deve iniziare con sk-ant-";
            else if (SaveFileText((CartellaEseguibile() + "/claude_chiave.txt").c_str(), (char*)bozzaChiave.c_str())) {
                cfg = ConfigClaude::Carica();
                if (cfg.chiave.empty()) cfg.chiave = bozzaChiave;
                editor.ImpostaChiave(cfg.chiave);
                esito = "Chiave salvata in claude_chiave.txt. Ora puoi chiedere a Claude!";
                gAudio.Suona(Effetto::Vittoria);
                inserisciChiave = false;
            } else esito = "Non riesco a salvare claude_chiave.txt accanto allo studio";
        }
        if (IsKeyPressed(KEY_ESCAPE)) inserisciChiave = false;
        return;
    }
    if (provaLivello) {
        giocoClaude.Aggiorna(dt, Comandi::DaPeriferiche());
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
        if (IsKeyPressed(KEY_F4)) { inserisciChiave = true; bozzaChiave.clear(); while (GetCharPressed() > 0) {} return; }
        if (IsKeyPressed(KEY_TAB)) { tipo = tipo == TipoRichiesta::Sprite ? TipoRichiesta::Livello : TipoRichiesta::Sprite; gAudio.Suona(Effetto::Menu); }
        if (IsKeyPressed(KEY_ENTER)) {
            if (cfg.chiave.empty()) esito = "Manca la chiave API: premi F4 per inserirla.";
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
        if (IsKeyPressed(KEY_F3) && !livelloClaude.empty()) {
            Progetto p;
            std::string e;
            if (p.DaTesto(livelloClaude, e)) {
                p.titolo = nomeLivelloClaude.empty() ? "Livello di Claude" : nomeLivelloClaude;
                p.ora = 0.85f;
                editor.Apri(p, "Livello di Claude aperto nell'editor");
                Apri(Schermata::Editor);
                return;
            }
            esito = "Livello non valido: " + e;
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
        if (!claude.ultimoErrore.empty()) { esito = "Errore: " + claude.ultimoErrore; gAudio.Suona(Effetto::Colpo); return; }
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
    Scritta(cfg.chiave.empty() ? "Chiave API: non trovata - premi F4 per inserirla" : TextFormat("Chiave API: ok  -  modello %s  (F4 per cambiarla)", cfg.modello.c_str()),
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
        ScrittaCentrata("F3: aprilo nell'editor", (int)(ant.x + ant.width / 2), 176, 10, Hex(0xffd2b8));
    } else {
        ScrittaCentrata("Qui appare cio' che crea Claude", (int)(ant.x + ant.width / 2), 120, 10, Hex(0x8a7aa0));
    }
    if (inserisciChiave) {
        DrawRectangle(0, 0, VW, VH, Hex(0x000000, 150));
        Rectangle r = { 30, 50, VW - 60, 150 };
        Pannello(r, Hex(0x1a1030, 245), Hex(0xd97757));
        Scritta("Chiave API di Claude", (int)r.x + 10, (int)r.y + 8, 10, Hex(0xffd23f));
        ScrittaACapo("1. Vai su console.anthropic.com, sezione API Keys, e crea una chiave (inizia con sk-ant-). "
                     "Serve un po' di credito sull'account.  2. Copiala e premi qui Ctrl+V.  3. Premi Invio: viene salvata "
                     "in claude_chiave.txt accanto allo studio.", (int)r.x + 10, (int)r.y + 24, (int)r.width - 20, 10, Hex(0xe0d8f0));
        Rectangle in = { r.x + 10, r.y + 92, r.width - 20, 18 };
        Pannello(in, Hex(0x0e0818), Hex(0xd97757));
        std::string mostrata = bozzaChiave.size() > 14 ? bozzaChiave.substr(0, 10) + std::string(std::min<size_t>(30, bozzaChiave.size() - 14), '*') + bozzaChiave.substr(bozzaChiave.size() - 4) : bozzaChiave;
        if (std::fmod(tempo, 1.0f) < 0.5f) mostrata += "_";
        Scritta(mostrata.c_str(), (int)in.x + 4, (int)in.y + 4, 10, WHITE);
        Scritta("Invio: salva   Esc: annulla   Backspace: cancella", (int)r.x + 10, (int)r.y + 124, 10, Hex(0x8a7aa0));
    }
    Intestazione("7. CREA CON CLAUDE", "Tab: sprite/livello  Invio: chiedi  F2: esempio  F5: gioca  F3: editor  F4: chiave  Esc: menu");
}

// --- Comandi e periferiche ----------------------------------------------

// Righe: le 8 azioni, poi "mouse nel gioco" e "zona morta". Colonne: tasto 1, tasto 2, joypad.
constexpr int RIGHE_COMANDI = NUM_AZIONI + 2;

void Studio::AggiornaComandi(float dt) {
    Mappatura& mp = gInput.m;
    int joy = gInput.Joypad();
    if (attesaComando) {
        // Il prossimo tasto o pulsante premuto diventa il nuovo comando. Esc annulla.
        tAttesa += dt;
        int tasto = GetKeyPressed();
        if (tasto == KEY_ESCAPE && colonnaComando < 2) { attesaComando = false; return; }
        if (colonnaComando < 2 && tasto > 0) {
            mp.tasti[rigaComando][colonnaComando] = tasto;
            attesaComando = false; gInput.Salva(); gAudio.Suona(Effetto::Gemma);
        } else if (colonnaComando == 2) {
            if (tasto == KEY_ESCAPE) { attesaComando = false; return; }
            if (joy >= 0)
                for (int b = 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++)
                    if (IsGamepadButtonPressed(joy, b)) { mp.pulsanti[rigaComando] = b; attesaComando = false; gInput.Salva(); gAudio.Suona(Effetto::Gemma); break; }
        }
        if (tAttesa > 8) attesaComando = false;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE) || (joy >= 0 && IsGamepadButtonPressed(joy, mp.pulsanti[(int)Azione::Indietro]))) {
        gInput.Salva(); attuale = Schermata::Menu; gAudio.Suona(Effetto::Menu); return;
    }
    if (IsKeyPressed(KEY_DOWN)) rigaComando = (rigaComando + 1) % RIGHE_COMANDI;
    if (IsKeyPressed(KEY_UP)) rigaComando = (rigaComando + RIGHE_COMANDI - 1) % RIGHE_COMANDI;
    if (IsKeyPressed(KEY_RIGHT)) colonnaComando = std::min(2, colonnaComando + 1);
    if (IsKeyPressed(KEY_LEFT)) colonnaComando = std::max(0, colonnaComando - 1);
    if (joy >= 0) {
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) rigaComando = (rigaComando + 1) % RIGHE_COMANDI;
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_LEFT_FACE_UP)) rigaComando = (rigaComando + RIGHE_COMANDI - 1) % RIGHE_COMANDI;
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) colonnaComando = std::min(2, colonnaComando + 1);
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) colonnaComando = std::max(0, colonnaComando - 1);
    }
    // Clic del mouse su una cella della tabella.
    Vector2 m = gInput.Mouse();
    for (int r = 0; r < RIGHE_COMANDI; r++)
        for (int c = 0; c < 3; c++) {
            Rectangle cel = { 96.0f + c * 66, 38.0f + r * 18, 62, 16 };
            if (r >= NUM_AZIONI && c > 0) continue;
            if (r >= NUM_AZIONI) cel.width = 194;
            if (CheckCollisionPointRec(m, cel) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (rigaComando == r && colonnaComando == c) attesaComando = true, tAttesa = 0;
                rigaComando = r; colonnaComando = c;
                if (r >= NUM_AZIONI) attesaComando = false;
            }
        }
    bool conferma = IsKeyPressed(KEY_ENTER) || (joy >= 0 && IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && rigaComando >= NUM_AZIONI);
    if (rigaComando == NUM_AZIONI && (conferma || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, { 96, 38.0f + NUM_AZIONI * 18, 194, 16 })))) {
        mp.mouseNelGioco = !mp.mouseNelGioco; gInput.Salva(); gAudio.Suona(Effetto::Menu);
    } else if (rigaComando == NUM_AZIONI + 1) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) { mp.zonaMorta = std::fmod(mp.zonaMorta + 0.05f, 0.80f); if (mp.zonaMorta < 0.1f) mp.zonaMorta = 0.1f; gInput.Salva(); }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) { mp.zonaMorta = std::max(0.1f, mp.zonaMorta - 0.05f); gInput.Salva(); }
    } else if (IsKeyPressed(KEY_ENTER)) { attesaComando = true; tAttesa = 0; gAudio.Suona(Effetto::Conferma, 0.5f); }
    if (IsKeyPressed(KEY_DELETE) && rigaComando < NUM_AZIONI) {
        if (colonnaComando < 2) mp.tasti[rigaComando][colonnaComando] = 0; else mp.pulsanti[rigaComando] = 0;
        gInput.Salva();
    }
    if (IsKeyPressed(KEY_R) && IsKeyDown(KEY_LEFT_CONTROL)) { mp = Mappatura::Predefinita(); gInput.Salva(); Messaggio("Comandi predefiniti ripristinati"); }
}

void Studio::DisegnaComandi() {
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x0e1230), Hex(0x24184a));
    const Mappatura& mp = gInput.m;
    const char* colonne[3] = { "Tasto 1", "Tasto 2", "Joypad" };
    for (int c = 0; c < 3; c++) Scritta(colonne[c], 100 + c * 66, 24, 10, Hex(0xffd23f));
    for (int r = 0; r < RIGHE_COMANDI; r++) {
        float y = 38.0f + r * 18;
        if (r < NUM_AZIONI) {
            bool attiva = gInput.Tenuto((Azione)r);
            DrawRectangle(8, (int)y, 84, 16, attiva ? Hex(0x2a6a3a) : Hex(0x141a40, 220));
            Scritta(NomeAzione((Azione)r), 12, (int)y + 3, 10, attiva ? Hex(0x8cff6a) : WHITE);
            for (int c = 0; c < 3; c++) {
                Rectangle cel = { 96.0f + c * 66, y, 62, 16 };
                bool sel = r == rigaComando && c == colonnaComando;
                Pannello(cel, sel ? Hex(0x2a2f6a) : Hex(0x101430, 220), sel ? Hex(0xffd23f) : Hex(0x2a3060));
                const char* t = sel && attesaComando ? (std::fmod(tempo, 0.6f) < 0.3f ? "premi..." : "")
                              : c < 2 ? NomeTasto(mp.tasti[r][c]) : NomePulsante(mp.pulsanti[r]);
                Scritta(t, (int)cel.x + 4, (int)cel.y + 3, 10, sel ? WHITE : Hex(0xc8d0f0));
            }
        } else {
            bool sel = r == rigaComando;
            Rectangle cel = { 96, y, 194, 16 };
            DrawRectangle(8, (int)y, 84, 16, Hex(0x141a40, 220));
            Scritta(r == NUM_AZIONI ? "Mouse" : "Zona morta", 12, (int)y + 3, 10, WHITE);
            Pannello(cel, sel ? Hex(0x2a2f6a) : Hex(0x101430, 220), sel ? Hex(0xffd23f) : Hex(0x2a3060));
            Scritta(r == NUM_AZIONI ? (mp.mouseNelGioco ? "nel gioco: si (sinistro cammina, destro salta)" : "nel gioco: no")
                                    : TextFormat("levette: %d%%  (+/- per cambiare)", (int)(mp.zonaMorta * 100 + 0.5f)),
                    (int)cel.x + 4, (int)cel.y + 3, 10, sel ? WHITE : Hex(0xc8d0f0));
        }
    }
    // Stato delle periferiche.
    float y = 38.0f + RIGHE_COMANDI * 18 + 6;
    int joy = gInput.Joypad();
    const char* ultime[3] = { "tastiera", "mouse", "joypad" };
    Scritta(TextFormat("Joypad: %s", joy >= 0 ? gInput.NomeJoypad() : "nessuno collegato"), 8, (int)y, 10, joy >= 0 ? Hex(0x8cff6a) : Hex(0xff8a8a));
    Scritta(TextFormat("Ultima periferica usata: %s", ultime[(int)gInput.Ultima()]), 8, (int)y + 13, 10, Hex(0xc8d0f0));
    if (joy >= 0) {
        // Levetta sinistra: cerchio con la zona morta.
        Vector2 c = { 270, y + 8 };
        DrawCircleLinesV(c, 10, Hex(0x5a6aa0));
        DrawCircleLinesV(c, 10 * mp.zonaMorta, Hex(0x3a4070));
        DrawCircleV({ c.x + gInput.LevettaX() * 10, c.y + gInput.LevettaY() * 10 }, 3, Hex(0xffd23f));
    }
    // Riquadro di prova dal vivo.
    Rectangle prova = { 298, 22, 176, 160 };
    Pannello(prova, Hex(0x07061a, 230), Hex(0x3a4080));
    Scritta("Prova dal vivo", (int)prova.x + 6, (int)prova.y + 4, 10, Hex(0xffd23f));
    Vector2 m = gInput.Mouse();
    Scritta(TextFormat("Mouse: %d, %d", (int)m.x, (int)m.y), (int)prova.x + 6, (int)prova.y + 18, 10, Hex(0xc8d0f0));
    const char* frecce[4] = { "<", ">", "^", "v" };
    for (int k = 0; k < 4; k++) {
        bool on = gInput.Tenuto((Azione)k);
        Rectangle r = { prova.x + 40 + (k == 0 ? 0 : k == 1 ? 40 : 20), prova.y + (k == 2 ? 40 : k == 3 ? 80 : 60), 18, 18 };
        Pannello(r, on ? Hex(0xffd23f) : Hex(0x1a2050), Hex(0x3a4080));
        ScrittaCentrata(frecce[k], (int)(r.x + 9), (int)r.y + 5, 10, on ? BLACK : WHITE);
    }
    const char* tasti[4] = { "Salto", "OK", "Indietro", "Pausa" };
    for (int k = 0; k < 4; k++) {
        bool on = gInput.Tenuto((Azione)(4 + k));
        Rectangle r = { prova.x + 110, prova.y + 40 + k * 20, 58, 16 };
        Pannello(r, on ? Hex(0xff5c8a) : Hex(0x1a2050), Hex(0x3a4080));
        ScrittaCentrata(tasti[k], (int)(r.x + 29), (int)r.y + 4, 10, WHITE);
    }
    // Asse analogico.
    float a = gInput.Asse();
    DrawRectangle((int)prova.x + 10, (int)prova.y + 128, 156, 6, Hex(0x1a2050));
    DrawRectangle((int)(prova.x + 88 + std::min(0.0f, a) * 78), (int)prova.y + 128, (int)(std::fabs(a) * 78), 6, Hex(0x8cff6a));
    Scritta("asse orizzontale", (int)prova.x + 10, (int)prova.y + 140, 10, Hex(0x8a90c0));
    Scritta("Le impostazioni valgono anche per i giochi creati (comandi.cfg).", 8, (int)y + 28, 10, Hex(0x8a90c0));
    Scritta(TextFormat("Ctrl+R: ripristina   Canc: cancella"), 8, (int)y + 41, 10, Hex(0x8a90c0));
    Intestazione("COMANDI E PERIFERICHE", attesaComando ? "Premi il nuovo tasto o pulsante (Esc annulla)"
                                                       : "Frecce: scegli  Invio o clic: cambia  Canc: cancella  Ctrl+R: predefiniti  Esc: menu");
}

// --- 10. Mondi del nuovo motore ----------------------------------------------

// Nome di file senza spazi ne' simboli: "L'isola aperta" -> "LIsolaAperta".
std::string NomeFile(const std::string& titolo) {
    std::string n;
    bool maiuscola = true;
    for (unsigned char c : titolo) {
        if (std::isalnum(c)) { n += maiuscola ? (char)std::toupper(c) : (char)c; maiuscola = false; }
        else maiuscola = true;
    }
    return n.empty() ? std::string("Gioco") : n;
}

void Studio::CaricaMondi() {
    mondi = MondiDiProva();
    fileMondi.assign(mondi.size(), "");
    // Anche i mondi salvati nella cartella "mondi" accanto allo studio (anche scritti a mano o da Claude).
    std::string cartella = CartellaEseguibile() + "/mondi";
    if (DirectoryExists(cartella.c_str())) {
        FilePathList elenco = LoadDirectoryFilesEx(cartella.c_str(), ".json", false);
        for (unsigned int i = 0; i < elenco.count; i++) {
            char* testo = LoadFileText(elenco.paths[i]);
            if (!testo) continue;
            Mondo m;
            std::string errore;
            if (m.DaJson(testo, errore)) { mondi.push_back(m); fileMondi.push_back(elenco.paths[i]); }
            else Messaggio(std::string(GetFileName(elenco.paths[i])) + ": " + errore);
            UnloadFileText(testo);
        }
        UnloadDirectoryFiles(elenco);
    }
    sceltaMondo = (int)Limita((float)sceltaMondo, 0, (float)mondi.size() - 1);
}

void Studio::AggiornaMondi(float dt) {
    if (mondi.empty()) { if (gInput.Premuto(Azione::Indietro)) attuale = Schermata::Menu; return; }
    Mondo& m = mondi[sceltaMondo];
    if (giocaMondo) {
        tAiutoMondo -= dt;
        if (IsKeyPressed(KEY_ESCAPE)) { giocaMondo = false; gAudio.ScegliBrano(0); gAudio.Suona(Effetto::Menu); return; }
        if (IsKeyPressed(KEY_F1)) giocoMondo.mostraCollisioni = !giocoMondo.mostraCollisioni;
        if (IsKeyPressed(KEY_F9)) giocoMondo.mostraFps = !giocoMondo.mostraFps;
        if (IsKeyPressed(KEY_R) || (giocoMondo.finito && gInput.Premuto(Azione::Conferma))) giocoMondo.Ricomincia();
        if (IsKeyPressed(KEY_T)) {
            // Prova al volo le diverse telecamere sulla mappa attuale.
            int modo = ((int)giocoMondo.telecamera.modo + 1) % (int)ModoTelecamera::Totale;
            giocoMondo.telecamera.modo = (ModoTelecamera)modo;
            if (giocoMondo.telecamera.modo == ModoTelecamera::Automatica && giocoMondo.telecamera.dirX == 0 && giocoMondo.telecamera.dirY == 0)
                giocoMondo.telecamera.dirX = 1;
            Messaggio(std::string("Telecamera: ") + NomeModoTelecamera(giocoMondo.telecamera.modo));
        }
        giocoMondo.Aggiorna(dt, Comandi::DaPeriferiche());
        if (giocoMondo.musica != gAudio.BranoAttuale()) gAudio.ScegliBrano(giocoMondo.musica);
        return;
    }
    if (gInput.Premuto(Azione::Indietro)) { attuale = Schermata::Menu; gAudio.Suona(Effetto::Menu); return; }
    int n = (int)mondi.size();
    if (gInput.Premuto(Azione::Giu)) { sceltaMondo = (sceltaMondo + 1) % n; gAudio.Suona(Effetto::Menu); }
    if (gInput.Premuto(Azione::Su)) { sceltaMondo = (sceltaMondo + n - 1) % n; gAudio.Suona(Effetto::Menu); }
    Vector2 mouse = gInput.Mouse();
    bool clic = false;
    for (int i = 0; i < n; i++) {
        Rectangle r = { 12, 40.0f + i * 22, 196, 20 };
        if (CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (sceltaMondo == i) clic = true;
            sceltaMondo = i;
        }
    }
    if (gInput.Premuto(Azione::Conferma) || clic) {
        std::string errore;
        giocoMondo = Gioco{};
        if (giocoMondo.CaricaMondo(mondi[sceltaMondo], errore)) {
            giocaMondo = true;
            tAiutoMondo = 5;
            gAudio.ScegliBrano(giocoMondo.musica);
            gAudio.Suona(Effetto::Conferma);
        } else Messaggio("Errore: " + errore);
        return;
    }
    if (IsKeyPressed(KEY_F2)) {
        std::string cartella = CartellaEseguibile() + "/mondi";
        MakeDirectory(cartella.c_str());
        std::string percorso = fileMondi[sceltaMondo].empty() ? cartella + "/" + NomeFile(m.titolo) + ".json" : fileMondi[sceltaMondo];
        std::string testo = m.Json();
        if (SaveFileText(percorso.c_str(), &testo[0])) Messaggio("Salvato " + std::string(GetFileName(percorso.c_str())) + " nella cartella mondi");
        else Messaggio("Non riesco a salvare " + percorso);
    }
    if (IsKeyPressed(KEY_F5)) { CaricaMondi(); Messaggio(TextFormat("%d mondi caricati", (int)mondi.size())); }
    if (IsKeyPressed(KEY_C)) {
#ifdef _WIN32
        std::string nome = NomeFile(m.titolo) + ".exe";
#else
        std::string nome = NomeFile(m.titolo);
#endif
        std::string errore;
        DatiGioco dati{ m.titolo, m.Json(true) };
        if (CreaEseguibile(CartellaEseguibile() + "/" + nome, SerializzaGioco(dati), errore)) { Messaggio("Creato " + nome + " accanto allo studio!"); gAudio.Suona(Effetto::Vittoria); }
        else Messaggio("Errore: " + errore);
    }
}

void Studio::DisegnaMondi() {
    if (giocaMondo) {
        giocoMondo.Disegna();
        if (tAiutoMondo > 0) {
            float k = Limita(tAiutoMondo, 0, 1);
            DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, (unsigned char)(200 * k)));
            Scritta("T: cambia telecamera   F1: collisioni   F9: FPS   R: ricomincia   Esc: torna all'elenco", 6, VH - 12, 10, ConAlfa(Hex(0xc8d0f0), k));
        }
        return;
    }
    DrawRectangleGradientV(0, 0, VW, VH, Hex(0x0e1230), Hex(0x24124a));
    Intestazione("NUOVO MOTORE: MONDI", "Invio: gioca   F2: salva JSON   C: crea il .exe   F5: ricarica   Esc: menu");
    for (int i = 0; i < (int)mondi.size(); i++) {
        Rectangle r = { 12, 40.0f + i * 22, 196, 20 };
        bool sel = i == sceltaMondo;
        Pannello(r, sel ? Hex(0x2a2f6a, 230) : Hex(0x101430, 190), sel ? Hex(0xffd23f) : Hex(0x3a4080));
        Scritta(mondi[i].titolo.c_str(), (int)r.x + 6, (int)r.y + 5, 10, sel ? WHITE : Hex(0xc8d0f0));
        if (!fileMondi[i].empty()) Scritta("json", (int)(r.x + r.width - 26), (int)r.y + 5, 10, Hex(0x8cff6a));
    }
    Scritta("Metti altri mondi .json nella cartella", 12, VH - 40, 10, Hex(0x8a90c0));
    Scritta("\"mondi\" accanto allo studio (F5).", 12, VH - 28, 10, Hex(0x8a90c0));
    if (mondi.empty()) return;
    const Mondo& m = mondi[sceltaMondo];
    Rectangle d = { 220, 26, 250, 228 };
    Pannello(d, Hex(0x101430, 210), Hex(0x3a4080));
    int y = (int)d.y + 8, x = (int)d.x + 8;
    Scritta(m.titolo.c_str(), x, y, 10, Hex(0xffd23f)); y += 16;
    Scritta(TextFormat("Genere: %s    Mappe: %d", NomeGenere(m.genere), (int)m.mappe.size()), x, y, 10, WHITE); y += 16;
    for (size_t i = 0; i < m.mappe.size() && i < 4; i++) {
        const Mappa& mp = m.mappe[i];
        Scritta(TextFormat("%s: %dx%d caselle, vista %s", mp.nome.c_str(), mp.larghezza, mp.altezza, mp.vista == Vista::Alto ? "dall'alto" : "laterale"),
                x, y, 10, Hex(0xc8d0f0)); y += 12;
        Scritta(TextFormat("  %s, strati %d, oggetti %d", NomeModoTelecamera(mp.telecamera.modo), (int)mp.strati.size(), (int)mp.oggetti.size()),
                x, y, 10, Hex(0x8a90c0)); y += 14;
    }
    // Minimappa della prima mappa.
    const Mappa& mp = m.mappe[m.mappaIniziale];
    float spazio = d.y + d.height - 8 - y;
    float scala = std::min((d.width - 16) / mp.larghezza, spazio / mp.altezza);
    scala = std::min(scala, 3.0f);
    float ox = d.x + (d.width - mp.larghezza * scala) / 2, oy = (float)y + 2;
    DrawRectangle((int)ox, (int)oy, (int)(mp.larghezza * scala), (int)(mp.altezza * scala), Hex(0x05081c));
    for (int ty = 0; ty < mp.altezza; ty++)
        for (int tx = 0; tx < mp.larghezza; tx++) {
            char c = mp.Casella(tx, ty);
            if (c == ' ') continue;
            Color col = c == '#' ? Hex(0x5ac85a) : c == '=' ? Hex(0x8a90ac) : c == '-' ? Hex(0xc8904a) : c == '^' ? Hex(0xff6a6a) : Hex(0xffd23f);
            DrawRectangleRec({ ox + tx * scala, oy + ty * scala, std::max(1.0f, scala), std::max(1.0f, scala) }, col);
        }
    for (auto& o : mp.oggetti) {
        Color col = (o.tipo == "moneta" || o.tipo == "gemma") ? Hex(0xffe066) : (o.tipo == "porta" || o.tipo == "bandiera") ? Hex(0x5ef0ff) : Hex(0xff5c8a);
        DrawRectangleRec({ ox + o.x / TILE * scala - 1, oy + o.y / TILE * scala - 1, 2, 2 }, col);
    }
    DrawRectangleRec({ ox + mp.partenza.x / TILE * scala - 1.5f, oy + mp.partenza.y / TILE * scala - 3, 3, 3 }, WHITE);
}

// --- Ciclo principale ----------------------------------------------------

void Studio::Aggiorna(float dt) {
    tempo += dt;
    if (tMessaggio > 0) tMessaggio -= dt;
    bool scrivendo = (attuale == Schermata::Claude && (!provaLivello || inserisciChiave)) || (attuale == Schermata::Editor && editor.Scrivendo())
                  || (attuale == Schermata::Comandi && attesaComando);
    if (IsKeyPressed(KEY_M) && !scrivendo) gAudio.MusicaAttiva(!gAudio.MusicaAttiva());
    if (attuale == Schermata::Editor) {
        editor.Aggiorna(dt);
        if (editor.vuoleUscire) { editor.Esci(); attuale = Schermata::Menu; gAudio.Suona(Effetto::Menu); }
        return;
    }
    if (attuale == Schermata::Comandi) { AggiornaComandi(dt); return; }
    if (attuale == Schermata::Mondi) { AggiornaMondi(dt); return; }
    bool indietro = attuale == Schermata::Claude ? (IsKeyPressed(KEY_ESCAPE) && !inserisciChiave) : gInput.Premuto(Azione::Indietro);
    if (attuale != Schermata::Menu && indietro && !(attuale == Schermata::Claude && provaLivello)) {
        attuale = Schermata::Menu; gAudio.Suona(Effetto::Menu); return;
    }
    switch (attuale) {
        case Schermata::Menu: AggiornaMenu(dt); break;
        case Schermata::Scenari: AggiornaScenari(dt); break;
        case Schermata::Personaggi: AggiornaPersonaggi(dt); break;
        case Schermata::Oggetti:
            if (IsKeyPressed(KEY_R)) { std::string e; giocoOggetti.Carica(LivelloVetrinaOggetti(), e); }
            giocoOggetti.Aggiorna(dt, Comandi::DaPeriferiche());
            break;
        case Schermata::Audio: AggiornaAudio(dt); break;
        case Schermata::Logica: AggiornaLogica(dt); break;
        case Schermata::Librerie: break;
        case Schermata::Claude: AggiornaClaude(dt); break;
        default: break;
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
            Scritta("3. OGGETTI   Frecce/WASD/joypad: muovi   Spazio/A: salta   R: ricomincia   Esc: menu", 6, VH - 12, 10, Hex(0xc8d0f0));
            break;
        case Schermata::Audio: DisegnaAudio(); break;
        case Schermata::Logica:
            giocoLogica.Disegna();
            DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
            Scritta("5. LOGICA   Spazio: salta   F1: collisioni   R: ricomincia   C: crea il .exe   Esc: menu", 6, VH - 12, 10, Hex(0xc8d0f0));
            break;
        case Schermata::Librerie: DisegnaLibrerie(); break;
        case Schermata::Claude: DisegnaClaude(); break;
        case Schermata::Editor: editor.Disegna(); break;
        case Schermata::Comandi: DisegnaComandi(); break;
        case Schermata::Mondi: DisegnaMondi(); break;
    }
    if (tMessaggio > 0) {
        int w = MeasureText(messaggio.c_str(), 10) + 16;
        float k = Limita(tMessaggio * 2, 0, 1);
        Rectangle r = { VW / 2.0f - w / 2.0f, 40, (float)w, 18 };
        Pannello(r, Hex(0x101430, (unsigned char)(230 * k)), ConAlfa(Hex(0xffd23f), k));
        DrawText(messaggio.c_str(), (int)r.x + 8, (int)r.y + 4, 10, ConAlfa(WHITE, k));
    }
}

// Disegna il logo dentro un rettangolo della finestra, mantenendo le proporzioni.
void DisegnaLogo(Rectangle r, float alfa = 1) {
    if (gLogo.id == 0) return;
    float s = std::min(r.width / gLogo.width, r.height / gLogo.height);
    float w = gLogo.width * s, h = gLogo.height * s;
    // Il logo ha scritte scure: lo appoggio su una targa chiara per farlo leggere su ogni sfondo.
    float bordo = h * 0.12f;
    DrawRectangleRounded({ r.x + (r.width - w) / 2 - bordo, r.y + (r.height - h) / 2 - bordo * 0.6f, w + bordo * 2, h + bordo * 1.2f },
                         0.35f, 8, ConAlfa(Hex(0xf4f1ea), 0.92f * alfa));
    DrawTexturePro(gLogo, { 0, 0, (float)gLogo.width, (float)gLogo.height },
                   { r.x + (r.width - w) / 2, r.y + (r.height - h) / 2, w, h }, { 0, 0 }, 0, ConAlfa(WHITE, alfa));
}

void Studio::DisegnaSopra(Rectangle area) {
    if (attuale != Schermata::Menu) return;
    float s = area.width / VW;
    DisegnaLogo({ area.x + 6 * s, area.y + 4 * s, 58 * s, 19 * s });
}

// Dove va l'immagine del gioco nella finestra: tutta la finestra, oppure la parte
// sinistra quando a destra c'e' la chat di Claude.
Rectangle PannelloChat(bool visibile) {
    if (!visibile) return { 0, 0, 0, 0 };
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float w = Limita(sw * 0.30f, 340, 620);
    return { sw - w, 0, w, sh };
}

Rectangle AreaGioco(const Rectangle& chat) {
    float sw = (float)GetScreenWidth() - chat.width, sh = (float)GetScreenHeight();
    float scala = std::min(sw / VW, sh / VH);
    float w = VW * scala, h = VH * scala;
    return { (sw - w) / 2, (sh - h) / 2, w, h };
}

void DisegnaSchermo(RenderTexture2D& schermo, Rectangle area) {
    DrawTexturePro(schermo.texture, { 0, 0, (float)VW, -(float)VH }, area, { 0, 0 }, 0, WHITE);
}

void CaricaRisorse() {
    Image icona = LoadImageFromMemory(".png", ICONA_PNG, (int)sizeof(ICONA_PNG));
    if (icona.data) { SetWindowIcon(icona); UnloadImage(icona); }
    Image logo = LoadImageFromMemory(".png", LOGO_PNG, (int)sizeof(LOGO_PNG));
    if (logo.data) {
        gLogo = LoadTextureFromImage(logo);
        GenTextureMipmaps(&gLogo);
        SetTextureFilter(gLogo, TEXTURE_FILTER_TRILINEAR);
        UnloadImage(logo);
    }
}

// Finestra grande: il 90% del monitor, centrata.
void FinestraGrande() {
    int mon = GetCurrentMonitor();
    int mw = GetMonitorWidth(mon), mh = GetMonitorHeight(mon);
    if (mw <= 0 || mh <= 0) return;
    int w = (int)(mw * 0.9f), h = (int)(mh * 0.86f);
    SetWindowSize(w, h);
    SetWindowPosition((mw - w) / 2, std::max(30, (mh - h) / 2));
}

// Schermata iniziale con il logo Elvantis. Ritorna false se la finestra e' stata chiusa.
bool Presentazione(const std::string& riga1, const std::string& riga2, float durata) {
    float t = 0;
    gAudio.Suona(Effetto::Gemma, 0.7f);
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        t += dt;
        gInput.Aggiorna(1, { 0, 0 });
        bool salta = t > 0.4f && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || gInput.Premuto(Azione::Conferma));
        if (t >= durata || salta) return true;
        float alfa = std::min(Limita(t / 0.6f, 0, 1), Limita((durata - t) / 0.5f, 0, 1));
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        BeginDrawing();
        DrawRectangleGradientV(0, 0, (int)sw, (int)sh, Hex(0x05081c), Hex(0x1a1040));
        // Bagliore dietro al logo.
        BeginBlendMode(BLEND_ADDITIVE);
        DrawEllipse((int)(sw / 2), (int)(sh * 0.42f), sw * 0.32f, sh * 0.16f, ConAlfa(Hex(0x5e8cff), 0.10f * alfa));
        EndBlendMode();
        float lw = std::min(sw * 0.6f, 1000.0f);
        float zoom = 0.96f + 0.04f * Limita(t / durata, 0, 1);
        DisegnaLogo({ sw / 2 - lw * zoom / 2, sh * 0.42f - lw * 0.33f * zoom / 2, lw * zoom, lw * 0.33f * zoom }, alfa);
        int d1 = std::max(20, (int)(sh / 26)), d2 = std::max(14, (int)(sh / 40));
        int w1 = MeasureText(riga1.c_str(), d1), w2 = MeasureText(riga2.c_str(), d2);
        DrawText(riga1.c_str(), (int)(sw / 2 - w1 / 2), (int)(sh * 0.66f), d1, ConAlfa(WHITE, alfa));
        DrawText(riga2.c_str(), (int)(sw / 2 - w2 / 2), (int)(sh * 0.66f) + d1 + 14, d2, ConAlfa(Hex(0xffd2b8), alfa));
        EndDrawing();
    }
    return false;
}

// File accanto all'eseguibile con lo stesso nome e un'altra estensione (es. Gioco.opzioni).
std::string FileAccanto(const std::string& estensione) {
    std::string p = PercorsoEseguibile();
    size_t barra = p.find_last_of("/\\"), punto = p.find_last_of('.');
    if (punto != std::string::npos && (barra == std::string::npos || punto > barra)) p = p.substr(0, punto);
    return p + estensione;
}

int GiocaPacchetto(const DatiGioco& dati) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VW * 2, VH * 2, dati.titolo.c_str());
    FinestraGrande();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    CaricaRisorse();
    gInput.Carica();
    gAudio.Inizializza();
    gAudio.ApplicaFile(dati.file);   // musiche ed effetti caricati da file nello studio
    const std::string opzioni = FileAccanto(".opzioni");
    CaricaOpzioniAudio(opzioni);
    gAudio.MusicaAttiva(true);
    RenderTexture2D schermo = LoadRenderTexture(VW, VH);
    Gioco gioco;
    gioco.titolo = dati.titolo;
    gioco.crediti = std::string("Creato con Studio Videogiochi di ") + SVILUPPATORE;
    std::string errore;
    ImpostaFileImmagini(dati.file);
    bool ok = gioco.Carica(dati.livello, errore);
    if (ok && !gAudio.MusicaDaFile()) gAudio.ScegliBrano(gioco.musica);
    bool aperta = Presentazione(dati.titolo, std::string("Studio Videogiochi - Elvantis - sviluppatore ") + SVILUPPATORE, 2.6f);
    while (aperta && !WindowShouldClose()) {
        Rectangle area = AreaGioco({ 0, 0, 0, 0 });
        gInput.Aggiorna(area.width / VW, { area.x, area.y });
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_ESCAPE)) break;
        if (IsKeyPressed(KEY_M)) gAudio.MusicaAttiva(!gAudio.MusicaAttiva());
        if (ok && IsKeyPressed(KEY_R)) gioco.Carica(dati.livello, errore);
        gAudio.Aggiorna(GetFrameTime());
        if (ok) gioco.Aggiorna(GetFrameTime(), Comandi::DaPeriferiche());
        if (ok && !gAudio.MusicaDaFile() && gioco.musica != gAudio.BranoAttuale()) gAudio.ScegliBrano(gioco.musica);   // cambio di mappa
        BeginTextureMode(schermo);
        if (ok) gioco.Disegna();
        else { ClearBackground(BLACK); Scritta(("Dati del gioco non validi: " + errore).c_str(), 10, 10, 10, WHITE); }
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DisegnaSchermo(schermo, area);
        PulsanteAudio(PostoAudioLibero(area), opzioni);
        EndDrawing();
    }
    UnloadRenderTexture(schermo);
    if (gLogo.id) UnloadTexture(gLogo);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}

// Controllo automatico dei mondi d'esempio (senza finestra): JSON avanti e indietro,
// caricamento nel motore e, per le mappe viste dall'alto, che ogni oggetto sia raggiungibile.
int VerificaMondi() {
    int errori = 0;
    for (auto& m : MondiDiProva()) {
        std::string json = m.Json(), errore;
        Mondo copia;
        if (!copia.DaJson(json, errore)) { TraceLog(LOG_ERROR, "%s: JSON non riletto: %s", m.titolo.c_str(), errore.c_str()); errori++; continue; }
        if (copia.Json() != json) { TraceLog(LOG_ERROR, "%s: il JSON cambia dopo la rilettura", m.titolo.c_str()); errori++; }
        Gioco g;
        if (!g.CaricaMondo(copia, errore)) { TraceLog(LOG_ERROR, "%s: %s", m.titolo.c_str(), errore.c_str()); errori++; continue; }
        for (auto& mp : copia.mappe) {
            if (mp.vista != Vista::Alto) continue;
            // Visita in ampiezza dalle caselle libere della partenza.
            std::vector<char> visto((size_t)mp.larghezza * mp.altezza, 0);
            std::vector<std::pair<int, int>> coda;
            auto Libera = [&](int x, int y) { char c = mp.Casella(x, y); return x >= 0 && y >= 0 && x < mp.larghezza && y < mp.altezza && c != '#' && c != '='; };
            int sx = (int)(mp.partenza.x / TILE), sy = (int)((mp.partenza.y - 1) / TILE);
            coda.push_back({ sx, sy }); visto[(size_t)sy * mp.larghezza + sx] = 1;
            for (size_t i = 0; i < coda.size(); i++) {
                int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
                for (int k = 0; k < 4; k++) {
                    int x = coda[i].first + dx[k], y = coda[i].second + dy[k];
                    if (Libera(x, y) && !visto[(size_t)y * mp.larghezza + x]) { visto[(size_t)y * mp.larghezza + x] = 1; coda.push_back({ x, y }); }
                }
            }
            int irraggiungibili = 0;
            for (auto& o : mp.oggetti) {
                int x = (int)(o.x / TILE), y = (int)((o.y - 1) / TILE);
                if (x < 0 || y < 0 || x >= mp.larghezza || y >= mp.altezza || !visto[(size_t)y * mp.larghezza + x]) {
                    TraceLog(LOG_WARNING, "%s / %s: %s in %d,%d non raggiungibile", m.titolo.c_str(), mp.nome.c_str(), o.tipo.c_str(), x, y);
                    irraggiungibili++;
                }
            }
            errori += irraggiungibili;
            TraceLog(LOG_INFO, "%s / %s: %d caselle raggiungibili, %d oggetti", m.titolo.c_str(), mp.nome.c_str(), (int)coda.size(), (int)mp.oggetti.size());
        }
        TraceLog(LOG_INFO, "%s: ok (%d mappe, JSON %d byte)", m.titolo.c_str(), (int)m.mappe.size(), (int)json.size());
    }
    // Porta fra due mappe: l'eroe cammina verso destra e deve arrivare nella seconda mappa.
    {
        Mondo w;
        w.genere = Genere::OpenWorld;
        for (int i = 0; i < 2; i++) { Mappa m; m.Nuova(16, 10, Vista::Alto); m.nome = i ? "B" : "A"; w.mappe.push_back(m); }
        OggettoMappa p; p.tipo = "porta"; p.x = 7 * TILE + 8; p.y = 4 * TILE; p.mappa = 1; p.destX = 3 * TILE + 8; p.destY = 6 * TILE + 14;
        w.mappe[0].oggetti.push_back(p);
        w.mappe[0].partenza = { 3 * TILE + 8, 3 * TILE + 14 };
        Gioco g;
        std::string errore;
        g.CaricaMondo(w, errore);
        Comandi c; c.asse = 1;
        for (int i = 0; i < 120 && g.MappaAttuale() == 0; i++) g.Aggiorna(1 / 60.0f, c);
        Comandi fermo;
        for (int i = 0; i < 60; i++) g.Aggiorna(1 / 60.0f, fermo);
        if (g.MappaAttuale() != 1) { TraceLog(LOG_ERROR, "La porta non porta alla seconda mappa"); errori++; }
        else TraceLog(LOG_INFO, "Porta fra mappe: ok");
    }
    TraceLog(errori ? LOG_ERROR : LOG_INFO, "Verifica finita: %d problemi", errori);
    return errori ? 1 : 0;
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
    if (argc > 1 && std::string(argv[1]) == "--verifica-mondi") return VerificaMondi();
    if (argc > 2 && std::string(argv[1]) == "--esporta-mondi") {
        MakeDirectory(argv[2]);
        for (auto& m : MondiDiProva()) { std::string t = m.Json(); SaveFileText((std::string(argv[2]) + "/" + NomeFile(m.titolo) + ".json").c_str(), &t[0]); }
        return 0;
    }
    if (argc > 2 && std::string(argv[1]) == "--esporta-audio") return gAudio.EsportaWav(argv[2]) ? 0 : 1;

    std::string dati;
    DatiGioco gioco;
    if (LeggiPacchettoIncluso(dati) && DeserializzaGioco(dati, gioco)) return GiocaPacchetto(gioco);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VW * 2, VH * 2, "Studio Videogiochi - Elvantis");
    FinestraGrande();
    SetWindowMinSize(VW * 2, VH * 2);
    SetExitKey(KEY_NULL); // Esc torna al menu invece di chiudere
    SetTargetFPS(60);
    CaricaRisorse();
    gInput.Carica();
    gAudio.Inizializza();
    const std::string opzioni = CartellaEseguibile() + "/studio.opzioni";
    CaricaOpzioniAudio(opzioni);
    gAudio.MusicaAttiva(true);
    RenderTexture2D schermo = LoadRenderTexture(VW, VH);

    Studio studio;
    studio.Avvia();
    bool aperta = Presentazione("Studio Videogiochi", std::string("Sviluppato da ") + SVILUPPATORE + " - Elvantis", 3.2f);
    while (aperta && !WindowShouldClose() && !studio.esci) {
        float dt = GetFrameTime();
        Rectangle chat = PannelloChat(studio.ChatVisibile());
        Rectangle area = AreaGioco(chat);
        gInput.Aggiorna(area.width / VW, { area.x, area.y });
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        gAudio.Aggiorna(dt);
        studio.Aggiorna(dt);
        if (studio.ChatVisibile()) studio.editor.AggiornaChat(PannelloChat(true));
        BeginTextureMode(schermo);
        ClearBackground(BLACK);
        studio.Disegna();
        EndTextureMode();
        BeginDrawing();
        ClearBackground(Hex(0x07061a));
        chat = PannelloChat(studio.ChatVisibile());
        area = AreaGioco(chat);
        DisegnaSchermo(schermo, area);
        studio.DisegnaSopra(area);
        if (studio.ChatVisibile()) studio.editor.DisegnaChat(chat);
        PulsanteAudio(studio.ChatVisibile() ? Rectangle{ chat.x + chat.width - 50, chat.y + 8, 40, 40 } : PostoAudioLibero(area), opzioni);
        EndDrawing();
    }
    UnloadRenderTexture(schermo);
    if (gLogo.id) UnloadTexture(gLogo);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}
