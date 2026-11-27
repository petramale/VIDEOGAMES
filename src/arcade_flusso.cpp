// Il flusso dei giochi arcade: lo stesso dei giochi creati dallo Studio.
//   intro (logo Elvantis, "Mario Petramale presenta", titolo, musica) -> "premi un tasto" ->
//   menu (Continua, 1 giocatore, 2 giocatori a turni, Carica partita, Opzioni, Crediti, Esci) -> gioco,
// con pausa (riprendi, salva, carica, ricomincia, musica, effetti, menu), salvataggio automatico
// a ogni tappa e tre spazi di salvataggio accanto all'eseguibile.
#include "arcade.h"
#include "immagini.h"
#include "audio.h"
#include "flusso.h"
#include "interfaccia.h"
#include "licenze_dati.h"
#include "mente.h"
#include "periferiche.h"
#include "salvataggio.h"
#include "sistema.h"
#include "ui.h"
#include "json.hpp"
#include <cmath>
#include <ctime>
#include <fstream>

using nlohmann::json;

bool EArcade(const std::string& testo) { return testo.find("studio-videogiochi/arcade-1") != std::string::npos; }

namespace arcade {

std::string Pacchetto(const std::string& nome) {
    json j = { { "formato", "studio-videogiochi/arcade-1" }, { "gioco", nome }, { "titolo", TitoloDi(nome) }, { "autore", "Mario Petramale - Elvantis" } };
    return j.dump(1);
}

// Comandi dalle periferiche (tastiera, mouse, joypad), con il mouse riportato nelle coordinate del gioco.
static Pad gPrecedente;
Pad LeggiPad(Rectangle area, int L, int A) {
    Pad p;
    int jp = gInput.Joypad();
    float lx = gInput.LevettaX(), ly = gInput.LevettaY();
    p.sx = gInput.Tenuto(Azione::Sinistra) || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    p.dx = gInput.Tenuto(Azione::Destra) || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    p.su = gInput.Tenuto(Azione::Su) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    p.giu = gInput.Tenuto(Azione::Giu) || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    p.x = (p.dx ? 1.0f : 0.0f) - (p.sx ? 1.0f : 0.0f);
    p.y = (p.giu ? 1.0f : 0.0f) - (p.su ? 1.0f : 0.0f);
    if (std::fabs(lx) > 0.3f) p.x = lx;
    if (std::fabs(ly) > 0.3f) p.y = ly;
    p.fuoco = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_J) || IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
              (jp >= 0 && (IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)));
    p.bomba = IsKeyDown(KEY_Z) || IsKeyDown(KEY_X) || IsKeyDown(KEY_K) || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
              IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
              (jp >= 0 && (IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)));
    // salto (Polvere da Sparo): le frecce servono a mirare, quindi si salta con C, L o Maiuscolo
    p.salto = IsKeyDown(KEY_C) || IsKeyDown(KEY_L) || IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
              (jp >= 0 && (IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) || IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_FACE_UP)));
    p.conferma = IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_KP_ENTER) || (jp >= 0 && IsGamepadButtonDown(jp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    float s = area.width / L;
    Vector2 m = GetMousePosition();
    p.mouse = { (m.x - area.x) / s, (m.y - area.y) / s };
    p.clic = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    p.clicP = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    p.clicDxP = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    Vector2 d = GetMouseDelta();
    p.mouseMosso = d.x != 0 || d.y != 0;
    p.rotella = GetMouseWheelMove();
    p.fuocoP = p.fuoco && !gPrecedente.fuoco;
    p.bombaP = p.bomba && !gPrecedente.bomba;
    p.saltoP = p.salto && !gPrecedente.salto;
    p.suP = p.su && !gPrecedente.su; p.giuP = p.giu && !gPrecedente.giu;
    p.sxP = p.sx && !gPrecedente.sx; p.dxP = p.dx && !gPrecedente.dx;
    gPrecedente = p;
    return p;
}

}  // namespace arcade

using namespace arcade;

int GiocaArcade(const DatiGioco& dati) {
    // Il pacchetto e' un progetto .gioco con i dati del gioco arcade (o il vecchio pacchetto con il solo nome).
    json pj = json::parse(dati.livello, nullptr, false);
    static ProgettoGioco progetto;
    json datiArcade = json::object();
    std::string nome;
    std::string errProgetto;
    if (pj.is_object() && pj.contains("arcade") && progetto.DaJson(dati.livello, errProgetto)) {
        nome = progetto.arcade;
        datiArcade = json::parse(progetto.arcadeDati.empty() ? std::string("{}") : progetto.arcadeDati, nullptr, false);
        for (auto& im : progetto.immagini) AggiungiFileImmagine(im.first, im.second);   // immagini degli sfondi propri
    } else nome = pj.is_object() ? pj.value("gioco", "") : "";
    std::unique_ptr<GiocoArcade> gioco = Crea(nome), vetrina = Crea(nome);
    if (gioco) { gioco->Configura(datiArcade, &progetto); vetrina->Configura(datiArcade, &progetto); }
    std::string titolo = !dati.titolo.empty() ? dati.titolo : gioco ? gioco->Titolo() : nome;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, titolo.c_str());
    FinestraGrande();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    CaricaRisorse();
    gInput.Carica();
    gAudio.Inizializza();
    const std::string opzioni = FileAccanto(".opzioni");
    CaricaOpzioniAudio(opzioni);
    gAudio.MusicaAttiva(gMusicaGioco);

    if (!gioco) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            DisegnaStelle((float)GetTime(), 1);
            Scritta2("Gioco arcade sconosciuto: " + nome, GetScreenWidth() / 2.0f, GetScreenHeight() * 0.45f, 30, WHITE);
            EndDrawing();
            if (IsKeyPressed(KEY_ESCAPE)) break;
        }
        CloseWindow();
        return 1;
    }

    // Schermata di caricamento mentre si dipinge la grafica (qualche secondo la prima volta).
    for (int i = 0; i < 2; i++) {
        BeginDrawing();
        DisegnaStelle(0, 1);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float lw = std::min(sw * 0.3f, 520.0f);
        DisegnaLogo({ sw / 2 - lw / 2, sh * 0.3f, lw, lw * 0.34f });
        Scritta2("Preparo " + titolo + "...", sw / 2, sh * 0.62f, std::max(18, (int)(sh / 34)), Hex(0xa8b4e8));
        EndDrawing();
    }
    gioco->Prepara();
    vetrina->Prepara();
    const int L = gioco->L(), A = gioco->A();
    RenderTexture2D rt = LoadRenderTexture(L, A);
    SetTextureFilter(rt.texture, gioco->PixelArt() ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);

    int turno = 0, giocatori = 1, punti[2] = { 0, 0 };
    uint64_t semeCorrente = 1;
    const uint64_t impronta = ImprontaTesto(nome);
    auto NuovoSeme = [&]() -> uint64_t {
        uint64_t x = Mescola64((uint64_t)std::time(nullptr) ^ ((uint64_t)(GetTime() * 1e6) << 20) ^ (uint64_t)GetRandomValue(0, 1 << 30));
        return x ? x : 1;
    };
    gSilenzio = true;
    vetrina->Nuova(12345, 0);
    gSilenzio = false;

    // --- salvataggi -----------------------------------------------------------------------------
    std::string baseSalvataggi = FileAccanto("");
    {
        std::string prova = baseSalvataggi + ".prova", e;
        if (!ScriviAtomico(prova, "ok", e)) {
            const char* ad = getenv("APPDATA");
            if (!ad) ad = getenv("HOME");
            if (ad) {
                std::string dir = std::string(ad) + "/Elvantis";
                MakeDirectory(dir.c_str());
                baseSalvataggi = dir + "/" + GetFileNameWithoutExt(PercorsoEseguibile().c_str());
            }
        } else { std::remove(prova.c_str()); std::remove((prova + ".bak").c_str()); }
    }
    auto PercorsoSlot = [&](int n) { return baseSalvataggi + (n == 0 ? std::string(".auto.sav") : ".slot" + std::to_string(n) + ".sav"); };
    std::vector<InfoSalvataggio> infoSlot(4);
    auto AggiornaSlot = [&]() { for (int n = 0; n < 4; n++) infoSlot[n] = InfoDiSalvataggio(PercorsoSlot(n)); };
    AggiornaSlot();
    std::string avvisoMenu;
    float tAvvisoMenu = 0;
    auto Avvisa = [&](const std::string& t) { avvisoMenu = t; tAvvisoMenu = 4.5f; };
    int ultimoAuto = 0;
    auto Salva = [&](int slot) -> bool {
        MetaSalvataggio m;
        m.titolo = titolo; m.impronta = impronta; m.punteggio = gioco->punteggio; m.tempo = gioco->tempo;
        m.mappa = gioco->Tappa();
        m.giocatori = giocatori; m.turno = turno; m.punti[0] = punti[0]; m.punti[1] = punti[1];
        json st = { { "arcade", json::parse(gioco->Stato(), nullptr, false) }, { "nome", nome } };
        std::string e;
        bool fatto = ScriviAtomico(PercorsoSlot(slot), ImbustaSalvataggio(ComponiSalvataggio(st.dump(), m)), e);
        if (!fatto) Avvisa("Salvataggio non riuscito: " + e);
        AggiornaSlot();
        return fatto;
    };

    enum class Fase { Menu, Opzioni, Crediti, Licenze, Gioca, Pausa, Cambio, Risultati, Slot };
    Fase fase = Fase::Menu, faseDopoSlot = Fase::Menu;
    bool slotPerSalvare = false;
    MenuGioco menu, menuOpzioni, menuPausa, menuSlot;
    float tFase = 0, tFine = 0, tempo = 0, scorriLicenze = 0;

    auto MusicaMenu = [&]() { SuonaMusica(gioco->MusicaMenu()); };
    auto Inizia = [&](int n) {
        giocatori = n; turno = 0; punti[0] = punti[1] = 0;
        semeCorrente = NuovoSeme();
        gioco->Nuova(semeCorrente, 0);
        gioco->altroPunteggio = n == 2 ? 0 : -1;
        ultimoAuto = gioco->puntiDiControllo;
        fase = n == 2 ? Fase::Cambio : Fase::Gioca;
        tFase = tFine = 0;
    };
    auto CaricaSlot = [&](int n) -> bool {
        std::string j;
        InfoSalvataggio info;
        if (!LeggiSalvataggio(PercorsoSlot(n), j, info)) { Avvisa(info.esiste ? "Salvataggio non valido: " + info.errore : "Lo spazio e' vuoto"); return false; }
        json tutto = json::parse(j, nullptr, false);
        if (!tutto.is_object() || !tutto.contains("arcade") || tutto.value("nome", "") != nome) { Avvisa("Questo salvataggio e' di un altro gioco"); return false; }
        MetaSalvataggio m = MetaDi(j);
        std::string e;
        std::unique_ptr<GiocoArcade> g = Crea(nome);
        g->Configura(datiArcade, &progetto);
        g->Prepara();
        if (!g->Ripristina(tutto["arcade"].dump(), e)) { Avvisa("Non riesco a caricare: " + e); return false; }
        gioco = std::move(g);
        giocatori = m.giocatori; turno = m.turno; punti[0] = m.punti[0]; punti[1] = m.punti[1];
        gioco->giocatore = turno;
        gioco->altroPunteggio = giocatori == 2 ? punti[1 - turno] : -1;
        ultimoAuto = gioco->puntiDiControllo;
        if (info.daRiserva) Avvisa("Il file era rovinato: caricata la copia di riserva.");
        fase = Fase::Gioca; tFase = tFine = 0;
        return true;
    };
    auto ApriSlot = [&](bool salvare, Fase ritorno) {
        AggiornaSlot();
        slotPerSalvare = salvare; faseDopoSlot = ritorno; fase = Fase::Slot; tFase = 0; menuSlot.scelta = 0;
    };
    auto UltimoSlot = [&]() -> int {
        int migliore = -1;
        long t = -1;
        for (int n = 0; n < 4; n++) {
            if (!infoSlot[n].valido) continue;
            std::string p = PercorsoSlot(n);
            long m = FileExists(p.c_str()) ? GetFileModTime(p.c_str()) : 0;
            if (m > t) { t = m; migliore = n; }
        }
        return migliore;
    };

    bool aperta = Intro(titolo, "", true, 3.2f);
    MusicaMenu();
    while (aperta && !WindowShouldClose()) {
        float dt = std::min(GetFrameTime(), 1 / 20.0f);
        tempo += dt; tFase += dt;
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float s = std::min(sw / L, sh / A);
        Rectangle area = { std::floor((sw - L * s) / 2), std::floor((sh - A * s) / 2), L * s, A * s };
        gInput.Aggiorna(1, { 0, 0 });
        gAudio.Aggiorna(dt);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        int dim = std::max(20, (int)(sh / 18)), dimP = std::max(14, (int)(sh / 40));
        bool indietro = IsKeyPressed(KEY_ESCAPE) || gInput.Premuto(Azione::Indietro);
        Pad pad = LeggiPad(area, L, A);

        if (fase == Fase::Gioca) {
            // in partita il tasto B del joypad serve al gioco (bomba, dinamite, chiudere le finestre): la pausa e' Esc, P o Start
            if (IsKeyPressed(KEY_ESCAPE) || gInput.Premuto(Azione::Pausa) || IsKeyPressed(KEY_P)) { fase = Fase::Pausa; menuPausa.scelta = 0; tFase = 0; gAudio.Suona(Effetto::Menu); }
            else {
                gioco->Aggiorna(dt, pad);
                if (gioco->puntiDiControllo != ultimoAuto && !gioco->finito) { ultimoAuto = gioco->puntiDiControllo; Salva(0); }
                if (gioco->finito) tFine += dt;
                if (gioco->finito && tFine > 3.0f && (GetKeyPressed() != 0 || pad.fuocoP || pad.clicP || tFine > 10)) {
                    punti[turno] = gioco->punteggio;
                    if (giocatori == 2 && turno == 0) {
                        turno = 1;
                        gioco->Nuova(semeCorrente, 1);
                        gioco->altroPunteggio = punti[0];
                        ultimoAuto = gioco->puntiDiControllo;
                        fase = Fase::Cambio; MusicaMenu();
                    } else { fase = Fase::Risultati; MusicaMenu(); std::remove(PercorsoSlot(0).c_str()); AggiornaSlot(); }
                    tFase = tFine = 0;
                }
            }
        }

        bool inPartita = fase == Fase::Gioca || fase == Fase::Pausa || (fase == Fase::Slot && faseDopoSlot == Fase::Pausa);
        gSilenzio = true;
        if (!inPartita) vetrina->Aggiorna(dt, vetrina->Automatico());
        if (!inPartita && vetrina->finito) vetrina->Nuova((uint64_t)(tempo * 1000) + 7, 0);
        gSilenzio = false;
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        (inPartita ? gioco : vetrina)->Disegna();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(rt.texture, { 0, 0, (float)L, -(float)A }, area, { 0, 0 }, 0, WHITE);
        if (fase != Fase::Gioca) DrawRectangle(0, 0, (int)sw, (int)sh, Hex(0x05081c, 170));

        if (fase == Fase::Menu) {
            float lw = std::min(sw * 0.3f, 520.0f);
            DisegnaLogo({ sw / 2 - lw / 2, sh * 0.05f, lw, lw * 0.34f });
            Scritta2(titolo, sw / 2, sh * 0.05f + lw * 0.34f + dim * 0.5f, (int)(dim * 1.25f), WHITE);
            std::vector<std::string> voci, codici;
            int ultimo = UltimoSlot();
            if (ultimo >= 0) { voci.push_back("Continua"); codici.push_back("continua"); }
            voci.push_back("1 giocatore"); codici.push_back("1");
            voci.push_back("2 giocatori (a turni)"); codici.push_back("2");
            voci.push_back("Carica partita"); codici.push_back("carica");
            for (const char* x : { "Opzioni", "Crediti", "Esci" }) { voci.push_back(x); codici.push_back(x); }
            float passo = dim * (voci.size() > 6 ? 1.35f : 1.5f);
            int v = menu.Frame(voci, sw / 2, sh * 0.40f, passo, dim);
            std::string cod = v >= 0 ? codici[v] : "";
            if (cod == "continua") CaricaSlot(ultimo);
            else if (cod == "1") Inizia(1);
            else if (cod == "2") Inizia(2);
            else if (cod == "carica") ApriSlot(false, Fase::Menu);
            else if (cod == "Opzioni") { fase = Fase::Opzioni; menuOpzioni.scelta = 0; }
            else if (cod == "Crediti") { fase = Fase::Crediti; tFase = 0; }
            else if (cod == "Esci") { EndDrawing(); break; }
            Scritta2("Frecce o joypad per scegliere, Invio o A per confermare   F11 schermo intero", sw / 2, sh - dimP * 2.2f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Opzioni) {
            Scritta2("OPZIONI", sw / 2, sh * 0.10f, (int)(dim * 1.3f), Hex(0xffe066));
            std::vector<std::string> voci = {
                std::string("Musica: ") + (gMusicaGioco ? "accesa (F7)" : "spenta (F7)"),
                std::string("Effetti sonori: ") + (gAudio.volumeEffetti > 0 ? "accesi (F6)" : "spenti (F6)"),
                std::string("Audio: ") + (gAudio.muto ? "muto" : "attivo"),
                std::string("Schermo intero: ") + (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? "si'" : "no"),
                "Indietro" };
            int v = menuOpzioni.Frame(voci, sw / 2, sh * 0.24f, dim * 1.5f, dim);
            if (v == 0) CambiaMusicaGioco(opzioni);
            else if (v == 1) CambiaEffetti(opzioni);
            else if (v == 2) gAudio.muto = !gAudio.muto;
            else if (v == 3) ToggleBorderlessWindowed();
            if (v == 2) SalvaOpzioniAudio(opzioni);
            if (v == 4 || indietro) fase = Fase::Menu;
            float y = sh * 0.24f + dim * 1.5f * 5 + dim * 0.4f;
            Scritta2("COMANDI", sw / 2, y, dimP + 2, Hex(0xffe066));
            for (auto& r : gioco->Aiuto()) { y += dimP * 1.45f; Scritta2(r, sw / 2, y, dimP, Hex(0xd8def8)); }
            Scritta2("F7 musica, F6 effetti sonori, F8 tutto l'audio, F11 schermo intero, Esc pausa", sw / 2, y + dimP * 1.8f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Crediti) {
            float lw = std::min(sw * 0.34f, 600.0f);
            DisegnaLogo({ sw / 2 - lw / 2, sh * 0.08f, lw, lw * 0.34f });
            std::vector<std::pair<std::string, std::string>> righe = {
                { "Gioco", titolo }, { "Ideato e sviluppato da", SVILUPPATORE }, { "Studio", "Elvantis" },
                { "Grafica, musica ed effetti", "disegnati e composti nel codice dallo Studio Videogiochi" },
                { "Componenti open source", "raylib, GLFW, miniaudio, stb, nlohmann/json, Outfit (premi L)" } };
            float y = sh * 0.08f + lw * 0.34f + dim;
            for (auto& r : righe) {
                Scritta2(r.first, sw / 2, y, dimP, Hex(0xa8b4e8));
                Scritta2(r.second, sw / 2, y + dimP * 1.3f, dim, WHITE);
                y += dimP * 1.3f + dim * 1.5f;
            }
            Scritta2("L: licenze dei componenti      un altro tasto: torna al menu", sw / 2, sh - dimP * 2.4f, dimP, Hex(0xffe066));
            if (tFase > 0.3f && IsKeyPressed(KEY_L)) { fase = Fase::Licenze; tFase = 0; scorriLicenze = 0; }
            else if (tFase > 0.3f && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || gInput.Premuto(Azione::Conferma) || gInput.Premuto(Azione::Indietro))) fase = Fase::Menu;
        } else if (fase == Fase::Licenze) {
            static std::vector<std::string> righe;
            if (righe.empty()) {
                std::string tutto((const char*)LICENZE_TXT, sizeof(LICENZE_TXT));
                size_t a = 0;
                while (a <= tutto.size()) {
                    size_t b = tutto.find('\n', a);
                    std::string r = tutto.substr(a, b == std::string::npos ? std::string::npos : b - a);
                    if (!r.empty() && r.back() == '\r') r.pop_back();
                    righe.push_back(r);
                    if (b == std::string::npos) break;
                    a = b + 1;
                }
            }
            int dimL = std::max(12, (int)(dimP * 0.9f));
            float riga = dimL * 1.35f;
            Scritta2("LICENZE / LICENSES", sw / 2, sh * 0.05f, dim, Hex(0xffe066));
            float top = sh * 0.05f + dim * 1.8f, basso = sh - dimP * 3.2f;
            float visibili = (basso - top) / riga;
            float massimo = std::max(0.0f, righe.size() - visibili);
            scorriLicenze -= GetMouseWheelMove() * 3;
            if (IsKeyDown(KEY_DOWN) || gInput.Tenuto(Azione::Giu)) scorriLicenze += 40 * dt;
            if (IsKeyDown(KEY_UP) || gInput.Tenuto(Azione::Su)) scorriLicenze -= 40 * dt;
            if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressed(KEY_SPACE)) scorriLicenze += visibili * 0.9f;
            if (IsKeyPressed(KEY_PAGE_UP)) scorriLicenze -= visibili * 0.9f;
            scorriLicenze = std::max(0.0f, std::min(massimo, scorriLicenze));
            float x = std::max(20.0f, sw / 2 - 40 * dimL * 0.55f);
            DrawRectangleRounded({ x - 24, top - 12, sw - 2 * x + 48, basso - top + 24 }, 0.03f, 8, ConAlfa(Hex(0x0d0f24), 0.86f));
            BeginScissorMode(0, (int)top, (int)sw, (int)(basso - top));
            int primo = (int)scorriLicenze;
            for (int i = primo; i < (int)righe.size() && i < primo + (int)visibili + 2; i++)
                Ui::Testo(righe[i], x, top + (i - scorriLicenze) * riga, Hex(0xd8def8), dimL);
            EndScissorMode();
            Scritta2("Frecce, rotella o Pagina giu': scorri      Esc o Invio: torna ai crediti", sw / 2, sh - dimP * 2.2f, dimP, Hex(0xa8b4e8));
            if (tFase > 0.3f && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || gInput.Premuto(Azione::Indietro))) { fase = Fase::Crediti; tFase = 0; }
        } else if (fase == Fase::Pausa) {
            Scritta2("PAUSA", sw / 2, sh * 0.18f, (int)(dim * 1.4f), Hex(0xffe066));
            std::vector<std::string> voci = { "Riprendi", "Salva partita", "Carica partita", "Ricomincia",
                std::string("Musica: ") + (gMusicaGioco ? "accesa (F7)" : "spenta (F7)"),
                std::string("Effetti sonori: ") + (gAudio.volumeEffetti > 0 ? "accesi (F6)" : "spenti (F6)"), "Menu principale" };
            int v = menuPausa.Frame(voci, sw / 2, sh * 0.28f, dim * 1.45f, dim);
            std::string cod = v >= 0 ? voci[v] : "";
            if (cod == "Riprendi" || (v < 0 && indietro && tFase > 0.1f)) fase = Fase::Gioca;
            else if (cod == "Salva partita") ApriSlot(true, Fase::Pausa);
            else if (cod == "Carica partita") ApriSlot(false, Fase::Pausa);
            else if (cod == "Ricomincia") { gioco->Nuova(semeCorrente, turno); gioco->altroPunteggio = giocatori == 2 ? punti[1 - turno] : -1; ultimoAuto = gioco->puntiDiControllo; fase = Fase::Gioca; tFine = 0; }
            else if (cod.rfind("Musica", 0) == 0) CambiaMusicaGioco(opzioni);
            else if (cod.rfind("Effetti", 0) == 0) CambiaEffetti(opzioni);
            else if (cod == "Menu principale") { fase = Fase::Menu; MusicaMenu(); }
            if (!gioco->SalvaQuandoVuoi())
                Scritta2("Il salvataggio riparte dall'inizio della tappa in corso (" + gioco->Tappa() + ")", sw / 2, sh - dimP * 2.4f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Slot) {
            Scritta2(slotPerSalvare ? "SALVA LA PARTITA" : "CARICA UNA PARTITA", sw / 2, sh * 0.12f, (int)(dim * 1.3f), Hex(0xffe066));
            std::vector<std::string> voci;
            std::vector<int> numeri;
            for (int n = slotPerSalvare ? 1 : 0; n < 4; n++) {
                const InfoSalvataggio& in = infoSlot[n];
                std::string nomeS = n == 0 ? "Automatico" : "Spazio " + std::to_string(n);
                std::string riga;
                if (in.valido) {
                    int min = (int)(in.tempo / 60), sec = (int)in.tempo % 60;
                    riga = nomeS + ":  " + (in.mappa.empty() ? "" : in.mappa + ",  ") + std::to_string(in.punteggio) + " punti,  " +
                           std::to_string(min) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec) + "   " + in.data;
                } else if (in.esiste) riga = nomeS + ":  danneggiato (" + in.errore + ")";
                else riga = nomeS + ":  vuoto";
                voci.push_back(riga);
                numeri.push_back(n);
            }
            voci.push_back("Indietro");
            numeri.push_back(-1);
            int dimS = std::max(16, (int)(dim * 0.72f));
            int v = menuSlot.Frame(voci, sw / 2, sh * 0.3f, dimS * 2.0f, dimS);
            if (v >= 0) {
                int n = numeri[v];
                if (n < 0) fase = faseDopoSlot;
                else if (slotPerSalvare) { if (Salva(n)) { Avvisa("Partita salvata nello spazio " + std::to_string(n)); fase = Fase::Gioca; } }
                else if (infoSlot[n].valido || infoSlot[n].esiste) CaricaSlot(n);
                else Avvisa("Lo spazio e' vuoto");
            } else if (indietro && tFase > 0.1f) fase = faseDopoSlot;
            Scritta2("Il salvataggio automatico si aggiorna all'inizio di ogni tappa", sw / 2, sh - dimP * 2.4f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Cambio) {
            Scritta2(turno == 0 ? "GIOCATORE 1" : "GIOCATORE 2", sw / 2, sh * 0.36f, (int)(dim * 1.6f), turno == 0 ? Hex(0x7ad0ff) : Hex(0xff8a7a));
            if (turno == 1) Scritta2("Il giocatore 1 ha fatto " + std::to_string(punti[0]) + " punti", sw / 2, sh * 0.5f, dim, WHITE);
            Scritta2("Tocca a te! Premi un tasto per iniziare", sw / 2, sh * 0.64f, dimP + 2, Hex(0xffe066));
            if (tFase > 0.5f && (GetKeyPressed() != 0 || pad.fuocoP || pad.clicP)) { fase = Fase::Gioca; tFine = 0; gioco->giocatore = turno; }
        } else if (fase == Fase::Risultati) {
            Scritta2(gioco->vinto ? "COMPLIMENTI!" : "PARTITA FINITA", sw / 2, sh * 0.22f, (int)(dim * 1.5f), Hex(0xffe066));
            if (giocatori == 1) Scritta2("Punteggio: " + std::to_string(punti[0]), sw / 2, sh * 0.42f, dim, WHITE);
            else {
                Scritta2("Giocatore 1: " + std::to_string(punti[0]) + " punti", sw / 2, sh * 0.38f, dim, Hex(0x7ad0ff));
                Scritta2("Giocatore 2: " + std::to_string(punti[1]) + " punti", sw / 2, sh * 0.38f + dim * 1.6f, dim, Hex(0xff8a7a));
                std::string vince = punti[0] == punti[1] ? "Pareggio!" : punti[0] > punti[1] ? "Vince il giocatore 1!" : "Vince il giocatore 2!";
                Scritta2(vince, sw / 2, sh * 0.38f + dim * 3.6f, (int)(dim * 1.2f), WHITE);
            }
            Scritta2("Premi un tasto per tornare al menu", sw / 2, sh * 0.8f, dimP + 2, Hex(0xa8b4e8));
            if (tFase > 0.8f && (GetKeyPressed() != 0 || pad.fuocoP || pad.clicP)) fase = Fase::Menu;
        }
        if (tAvvisoMenu > 0) {
            tAvvisoMenu -= dt;
            float a = std::min(1.0f, tAvvisoMenu);
            int w = Ui::LarghezzaForte(avvisoMenu, dimP) + dimP * 2;
            Rectangle r = { sw / 2 - w / 2.0f, sh * 0.9f - dimP, (float)w, dimP * 2.2f };
            DrawRectangleRounded(r, 0.4f, 8, ConAlfa(Hex(0x101a40), 0.9f * a));
            Scritta2(avvisoMenu, sw / 2, r.y + dimP * 0.55f, dimP, ConAlfa(Hex(0xffe066), a));
        }
        Rectangle pa = AngoloAudio(area, true);
        PulsanteAudio(pa, opzioni);
        PulsantiAudioGioco(pa, opzioni);
        gAudio.MusicaAttiva(gMusicaGioco);
        EndDrawing();
    }
    UnloadRenderTexture(rt);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}
