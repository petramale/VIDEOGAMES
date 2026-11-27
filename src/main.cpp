// Studio Videogiochi - Elvantis. Sviluppatore: Mario Petramale.
//
//   StudioVideogiochi.exe                                       apre l'officina (lo studio)
//   StudioVideogiochi.exe --crea Gioco.exe liv.txt "Titolo"     crea un gioco da riga di comando
//   StudioVideogiochi.exe --esporta-audio cartella              salva effetti e musica in .wav
//   StudioVideogiochi.exe --verifica-mondi                      prove automatiche (senza finestra)
//
// Un eseguibile creato dallo studio contiene il gioco in coda (vedi pacchetto.h) e parte
// con il suo flusso: intro con il logo Elvantis e "Mario Petramale", musica, "premi un tasto",
// menu (1 giocatore, 2 giocatori a turni, opzioni, crediti, esci) e poi il gioco.
#include "comune.h"
#include "audio.h"
#include "claude_mondo.h"
#include "claude_progetto.h"
#include "gioco.h"
#include "interfaccia.h"
#include "immagini.h"
#include "mondo.h"
#include "officina.h"
#include "pacchetto.h"
#include "periferiche.h"
#include "progetto.h"
#include "progetto_gioco.h"
#include "raccolte.h"
#include "risorse_dati.h"
#include "salvataggio.h"
#include "licenze_dati.h"
#include "sistema.h"
#include "ui.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>

#include "flusso.h"

const char* SVILUPPATORE = "Mario Petramale";
Texture2D gLogo{};   // logo Elvantis, disegnato a risoluzione piena

// Disegna il logo dentro un rettangolo della finestra, mantenendo le proporzioni.
void DisegnaLogo(Rectangle r, float alfa) {
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

// File accanto all'eseguibile con lo stesso nome e un'altra estensione (es. Gioco.opzioni).
std::string FileAccanto(const std::string& estensione) {
    std::string p = PercorsoEseguibile();
    size_t barra = p.find_last_of("/\\"), punto = p.find_last_of('.');
    if (punto != std::string::npos && (barra == std::string::npos || punto > barra)) p = p.substr(0, punto);
    return p + estensione;
}

// Scritta con ombra, centrata in x se 'centro' (carattere Outfit; grassetto per i titoli).
void Scritta2(const std::string& t, float x, float y, int dim, Color c, bool centro) {
    bool forte = dim >= 24;
    int w = forte ? Ui::LarghezzaForte(t, dim) : Ui::Larghezza(t, dim);
    float px = centro ? x - w / 2.0f : x;
    float o = std::max(1.0f, dim / 16.0f);
    Color ombra = ConAlfa(BLACK, c.a / 255.0f * 0.6f);
    if (forte) { Ui::TestoForte(t, px + o, y + o, ombra, dim); Ui::TestoForte(t, px, y, c, dim); }
    else { Ui::Testo(t, px + o, y + o, ombra, dim); Ui::Testo(t, px, y, c, dim); }
}

// Cielo stellato animato dietro all'intro e ai menu.
void DisegnaStelle(float t, float alfa) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    DrawRectangleGradientV(0, 0, (int)sw, (int)sh, Hex(0x05081c), Hex(0x1e1046));
    for (int i = 0; i < 160; i++) {
        unsigned h = (unsigned)i * 2654435761u;
        float x = (float)(h % 10007) / 10007 * sw;
        float vel = 4 + (h >> 7) % 30;
        float y = std::fmod((float)((h >> 3) % 9973) / 9973 * sh + t * vel, sh);
        float lum = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(t * (1 + (h >> 11) % 4) + i));
        float r = 1 + (h >> 13) % 3 * sh / 900;
        DrawCircleV({ x, y }, r, ConAlfa(i % 7 == 0 ? Hex(0xffd2b8) : Hex(0xbcd0ff), lum * alfa));
    }
}

// Intro con il logo Elvantis: il logo arriva con un rimbalzo, un lampo di luce lo attraversa,
// poi "Mario Petramale presenta" e il titolo. Con 'aspettaTasto' resta su "Premi un tasto".
// Ritorna false se la finestra e' stata chiusa.
bool Intro(const std::string& titolo, const std::string& sotto, bool aspettaTasto, float durata) {
    float t = 0;
    bool suonato = false;
    gAudio.ScegliBrano(BRANO_INTRO);
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        t += dt;
        gInput.Aggiorna(1, { 0, 0 });
        gAudio.Aggiorna(dt);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        bool tasto = GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || gInput.Premuto(Azione::Conferma) || gInput.Premuto(Azione::Salto);
        if (tasto && t > 0.3f) {
            if (!aspettaTasto || t >= durata) { gAudio.Suona(Effetto::Conferma); return true; }
            t = durata;   // salta l'animazione
        }
        if (!aspettaTasto && t >= durata + 0.6f) return true;
        if (!suonato && t > 0.9f) { gAudio.Suona(Effetto::Gemma, 0.8f); suonato = true; }
        float uscita = aspettaTasto ? 1 : Limita((durata + 0.6f - t) / 0.6f, 0, 1);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        BeginDrawing();
        DisegnaStelle(t, uscita);
        // Il logo: entra con un rimbalzo elastico.
        float k = Limita(t / 1.0f, 0, 1);
        float rimbalzo = 1 - std::pow(2.0f, -8 * k) * std::cos(k * 9.5f);
        float lw = std::min(sw * 0.56f, 980.0f) * (0.55f + 0.45f * rimbalzo);
        float alfa = Limita(t / 0.4f, 0, 1) * uscita;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawEllipse((int)(sw / 2), (int)(sh * 0.36f), sw * 0.36f, sh * 0.2f, ConAlfa(Hex(0x5e8cff), 0.12f * alfa));
        EndBlendMode();
        Rectangle rl = { sw / 2 - lw / 2, sh * 0.36f - lw * 0.17f, lw, lw * 0.34f };
        DisegnaLogo(rl, alfa);
        // Lampo di luce che attraversa il logo.
        if (t > 0.9f && t < 1.7f) {
            float p = (t - 0.9f) / 0.8f;
            float x = rl.x - rl.width * 0.2f + p * rl.width * 1.4f;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRectangleGradientH((int)(x - rl.width * 0.08f), (int)rl.y, (int)(rl.width * 0.08f), (int)rl.height, ConAlfa(WHITE, 0), ConAlfa(WHITE, 0.35f * uscita));
            DrawRectangleGradientH((int)x, (int)rl.y, (int)(rl.width * 0.08f), (int)rl.height, ConAlfa(WHITE, 0.35f * uscita), ConAlfa(WHITE, 0));
            EndBlendMode();
        }
        int d1 = std::max(22, (int)(sh / 22)), d2 = std::max(14, (int)(sh / 42));
        float a2 = Limita((t - 1.3f) / 0.6f, 0, 1) * uscita;
        Scritta2(std::string(SVILUPPATORE) + " presenta", sw / 2, sh * 0.58f, d2, ConAlfa(Hex(0xffd2b8), a2));
        float a3 = Limita((t - 1.9f) / 0.6f, 0, 1) * uscita;
        float onda = 1 + 0.03f * std::sin(t * 3);
        Scritta2(titolo, sw / 2, sh * 0.64f, (int)(d1 * onda), ConAlfa(WHITE, a3));
        if (!sotto.empty()) Scritta2(sotto, sw / 2, sh * 0.64f + d1 * 1.5f, d2, ConAlfa(Hex(0xa8b4e8), a3));
        if (aspettaTasto && t >= durata) {
            float lamp = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(t * 5));
            Scritta2("PREMI UN TASTO", sw / 2, sh * 0.86f, d2 + 4, ConAlfa(Hex(0xffe066), lamp));
        }
        EndDrawing();
    }
    return false;
}

// --- Il gioco creato ---------------------------------------------------------------------------

int MenuGioco::Frame(const std::vector<std::string>& voci, float cx, float y, float passo, int dim) {
    if (gInput.Premuto(Azione::Su)) { scelta = (scelta + (int)voci.size() - 1) % (int)voci.size(); gAudio.Suona(Effetto::Menu, 0.6f); }
    if (gInput.Premuto(Azione::Giu)) { scelta = (scelta + 1) % (int)voci.size(); gAudio.Suona(Effetto::Menu, 0.6f); }
    int premuta = -1;
    for (int i = 0; i < (int)voci.size(); i++) {
        int w = Ui::LarghezzaForte(voci[i], dim);
        Rectangle r = { cx - w / 2.0f - dim, y + i * passo - dim * 0.3f, w + dim * 2.0f, dim * 1.6f };
        bool sopra = CheckCollisionPointRec(GetMousePosition(), r);
        if (sopra && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0) && scelta != i) scelta = i;
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) premuta = i;
        bool sel = scelta == i;
        if (sel) {
            DrawRectangleRounded(r, 0.5f, 8, Hex(0x2a3a8a, 200));
            DrawRectangleRoundedLinesEx(r, 0.5f, 8, 3, Hex(0xffe066));
        }
        Scritta2(voci[i], cx, y + i * passo, dim, sel ? Hex(0xffe066) : Hex(0xe8ecff));
    }
    if (gInput.Premuto(Azione::Conferma) || gInput.Premuto(Azione::Salto)) premuta = scelta;
    if (premuta >= 0) { scelta = premuta; gAudio.Suona(Effetto::Conferma); }
    return premuta;
}

namespace {

bool EProgetto(const std::string& testo) { return testo.find("studio-videogiochi/gioco-1") != std::string::npos; }

int GiocaPacchetto(const DatiGioco& dati) {
    if (EArcade(dati.livello)) return GiocaArcade(dati);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VW * 2, VH * 2, dati.titolo.c_str());
    FinestraGrande();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    CaricaRisorse();
    gInput.Carica();
    gAudio.Inizializza();
    gAudio.ApplicaFile(dati.file);   // musiche ed effetti caricati da file (vecchio formato)
    const std::string opzioni = FileAccanto(".opzioni");
    CaricaOpzioniAudio(opzioni);
    gAudio.MusicaAttiva(gMusicaGioco);

    // Il gioco: un progetto completo (.gioco) o un vecchio livello / mondo.
    ProgettoGioco progetto;
    bool conProgetto = false;
    std::string errore;
    if (EProgetto(dati.livello)) {
        conProgetto = progetto.DaJson(dati.livello, errore);
        if (conProgetto) for (auto& im : progetto.immagini) AggiungiFileImmagine(im.first, im.second);
    } else ImpostaFileImmagini(dati.file);
    int turno = 0;
    int giocatori = 1;
    int punti[2] = { 0, 0 };
    uint64_t semeCorrente = 0;   // il seme della partita in corso (0 = la vetrina dei menu)
    const uint64_t impronta = ImprontaTesto(dati.livello);
    auto Carica = [&](Gioco& g, uint64_t seme) {
        g = Gioco{};
        g.semePartita = seme;
        g.titolo = dati.titolo;
        g.crediti = std::string("Creato con Studio Videogiochi di ") + SVILUPPATORE;
        std::string e;
        bool ok;
        if (conProgetto && turno == 1 && !progetto.eroe2.empty()) {
            ProgettoGioco p2 = progetto; p2.eroe = progetto.eroe2;   // il giocatore 2 ha il suo eroe
            ok = g.CaricaProgetto(p2, e);
        } else ok = conProgetto ? g.CaricaProgetto(progetto, e) : g.Carica(dati.livello, e);
        if (!ok) errore = e;
        return ok;
    };
    Gioco gioco, vetrina;
    bool ok = (EProgetto(dati.livello) ? conProgetto : true) && Carica(gioco, 0);
    if (ok) { Carica(vetrina, 0); vetrina.mostraHud = false; vetrina.mostraEsito = false; }

    // --- Salvataggi: tre slot e uno automatico accanto all'exe (o nella cartella dell'utente) ---
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
    const std::string fileMemoria = baseSalvataggi + ".memoria";
    bool ricordaPartite = false;
    if (conProgetto) for (auto& pr : progetto.profili) if (pr.ricordaPartite) ricordaPartite = true;
    std::vector<InfoSalvataggio> infoSlot(4);
    auto AggiornaSlot = [&]() { for (int n = 0; n < 4; n++) infoSlot[n] = InfoDiSalvataggio(PercorsoSlot(n)); };
    AggiornaSlot();
    std::string avvisoMenu;
    float tAvvisoMenu = 0;
    auto Avvisa = [&](const std::string& t) { avvisoMenu = t; tAvvisoMenu = 4.5f; };
    int ultimoAuto = 0;
    auto SalvaMemoria = [&]() {
        if (!ricordaPartite || !conProgetto) return;
        std::string e;
        ScriviAtomico(fileMemoria, gioco.SapereJson(), e);
    };
    auto CaricaMemoria = [&](Gioco& g) {
        if (!ricordaPartite) return;
        std::ifstream in(fileMemoria, std::ios::binary);
        if (in) g.SapereDaJson(std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
    };
    auto Salva = [&](int slot) -> bool {
        MetaSalvataggio m;
        m.titolo = dati.titolo; m.impronta = impronta; m.punteggio = gioco.punteggio; m.tempo = gioco.tempo;
        int ma = gioco.MappaAttuale();
        if (ma >= 0 && ma < (int)gioco.MondoAttuale().mappe.size()) m.mappa = gioco.MondoAttuale().mappe[ma].nome;
        m.giocatori = giocatori; m.turno = turno; m.punti[0] = punti[0]; m.punti[1] = punti[1];
        std::string e;
        bool fatto = ScriviAtomico(PercorsoSlot(slot), ImbustaSalvataggio(ComponiSalvataggio(gioco.StatoJson(), m)), e);
        if (!fatto) Avvisa("Salvataggio non riuscito: " + e);
        SalvaMemoria();
        AggiornaSlot();
        return fatto;
    };
    RenderTexture2D rt{}, rtVetrina{};

    enum class Fase { Menu, Opzioni, Crediti, Licenze, Gioca, Pausa, Cambio, Risultati, Slot };
    Fase fase = Fase::Menu, faseDopoSlot = Fase::Menu;
    bool slotPerSalvare = false;
    MenuGioco menu, menuOpzioni, menuPausa, menuSlot;
    float tFase = 0, tFine = 0, tempo = 0;
    float scorriLicenze = 0;

    auto MusicaGioco = [&]() {
        if (!ok || gAudio.MusicaDaFile()) return;
        if (!gioco.MusicaDellaLibreria() && (gioco.musica != gAudio.BranoAttuale() || !gAudio.BranoPersonale().empty())) gAudio.ScegliBrano(gioco.musica);
    };
    auto MusicaMenu = [&]() {
        gAudio.TogliSostituzioni();
        gAudio.FermaBranoPersonale();
        if (!gAudio.MusicaDaFile()) gAudio.ScegliBrano(BRANO_INTRO);
    };
    auto NuovoSeme = [&]() -> uint64_t {
        // Ogni partita ha il suo seme (salvo un seme fisso scelto nello studio per le prove).
        if (conProgetto && progetto.varieta.semeFisso) return progetto.varieta.semeFisso;
        uint64_t x = Mescola64((uint64_t)std::time(nullptr) ^ ((uint64_t)(GetTime() * 1e6) << 20) ^ (uint64_t)GetRandomValue(0, 1 << 30));
        return x ? x : 1;
    };
    auto Inizia = [&](int n) {
        giocatori = n; turno = 0; punti[0] = punti[1] = 0;
        semeCorrente = NuovoSeme();
        Carica(gioco, semeCorrente);
        CaricaMemoria(gioco);
        ultimoAuto = gioco.autosalvataggi;
        fase = n == 2 ? Fase::Cambio : Fase::Gioca;
        tFase = tFine = 0;
        if (n == 1) MusicaGioco();
    };
    auto CaricaSlot = [&](int n) -> bool {
        std::string j;
        InfoSalvataggio info;
        if (!LeggiSalvataggio(PercorsoSlot(n), j, info)) { Avvisa(info.esiste ? "Salvataggio non valido: " + info.errore : "Lo spazio e' vuoto"); return false; }
        if (!conProgetto) { Avvisa("Questo gioco non supporta i salvataggi"); return false; }
        MetaSalvataggio m = MetaDi(j);
        Gioco g;
        std::string e, avviso;
        CaricaMemoria(g);
        if (!g.Ripristina(m.giocatori == 2 && m.turno == 1 && !progetto.eroe2.empty() ? [&]() { ProgettoGioco p2 = progetto; p2.eroe = progetto.eroe2; return p2; }() : progetto, j, e, avviso)) {
            Avvisa("Non riesco a caricare: " + e);
            return false;
        }
        g.titolo = dati.titolo;
        g.crediti = std::string("Creato con Studio Videogiochi di ") + SVILUPPATORE;
        gioco = g;
        gioco.RicollegaRisorse();   // la copia non deve usare i disegni di 'g', che sta per sparire
        giocatori = m.giocatori; turno = m.turno; punti[0] = m.punti[0]; punti[1] = m.punti[1];
        semeCorrente = gioco.semePartita;
        ultimoAuto = gioco.autosalvataggi;
        std::string nota;
        if (info.daRiserva) nota = "Il file era rovinato: caricata la copia di riserva. ";
        if (info.impronta && info.impronta != impronta) nota += "Il gioco e' cambiato dopo questo salvataggio. ";
        if (!avviso.empty()) nota += avviso;
        if (!nota.empty()) Avvisa(nota);
        fase = Fase::Gioca; tFase = tFine = 0;
        MusicaGioco();
        return true;
    };
    auto ApriSlot = [&](bool salvare, Fase ritorno) {
        AggiornaSlot();
        slotPerSalvare = salvare; faseDopoSlot = ritorno; fase = Fase::Slot; tFase = 0; menuSlot.scelta = 0;
    };
    // "Continua": il salvataggio valido piu' recente (anche quello automatico).
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

    bool aperta = Intro(dati.titolo, "", true, 3.2f);
    while (aperta && !WindowShouldClose()) {
        float dt = GetFrameTime();
        tempo += dt; tFase += dt;
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float s = std::min(sw / VW, sh / VH);
        Rectangle area = { (sw - VW * s) / 2, (sh - VH * s) / 2, VW * s, VH * s };
        gInput.Aggiorna(s, { area.x, area.y });
        gAudio.Aggiorna(dt);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        int scalaHD = std::max(1, std::min(4, (int)std::ceil(s)));
        int dim = std::max(20, (int)(sh / 18)), dimP = std::max(14, (int)(sh / 40));
        bool indietro = IsKeyPressed(KEY_ESCAPE) || gInput.Premuto(Azione::Indietro);

        BeginDrawing();
        ClearBackground(BLACK);
        if (!ok) {
            DisegnaStelle(tempo, 1);
            Scritta2("Dati del gioco non validi", sw / 2, sh * 0.4f, dim, WHITE);
            Scritta2(errore, sw / 2, sh * 0.4f + dim * 1.6f, dimP, Hex(0xffb0b0));
            Scritta2("Esc: esci", sw / 2, sh * 0.8f, dimP, Hex(0xa8b4e8));
            EndDrawing();
            if (indietro) break;
            continue;
        }

        if (fase == Fase::Gioca) {
            if (indietro || gInput.Premuto(Azione::Pausa)) { fase = Fase::Pausa; menuPausa.scelta = 0; tFase = 0; gAudio.Suona(Effetto::Menu); }
            else {
                gioco.Aggiorna(dt, Comandi::DaPeriferiche());
                MusicaGioco();
                // Salvataggio automatico a ogni cambio di mappa e a ogni punto di controllo.
                if (conProgetto && gioco.autosalvataggi != ultimoAuto && !gioco.finito && !gioco.vinto) {
                    ultimoAuto = gioco.autosalvataggi;
                    Salva(0);
                }
                if (gioco.finito) tFine += dt;
                if (gioco.finito && tFine > 2.2f && (GetKeyPressed() != 0 || gInput.Premuto(Azione::Conferma) || tFine > 8)) {
                    punti[turno] = gioco.punteggio;
                    if (giocatori == 2 && turno == 0) { turno = 1; Carica(gioco, semeCorrente); CaricaMemoria(gioco); ultimoAuto = gioco.autosalvataggi; fase = Fase::Cambio; MusicaMenu(); }
                    else { fase = Fase::Risultati; MusicaMenu(); SalvaMemoria(); }
                    tFase = tFine = 0;
                }
            }
        }

        // Lo sfondo: il gioco vero (in partita) o la "vetrina" che si muove da sola (nei menu).
        bool inPartita = fase == Fase::Gioca || fase == Fase::Pausa;
        if (inPartita) gioco.DisegnaHD(rt, scalaHD);
        else { vetrina.Aggiorna(dt * 0.5f, Comandi{}); vetrina.messaggio.clear();   // i messaggi delle regole coprirebbero il menu
               vetrina.DisegnaHD(rtVetrina, scalaHD); }
        RenderTexture2D& vista = inPartita ? rt : rtVetrina;
        DrawTexturePro(vista.texture, { 0, 0, (float)vista.texture.width, -(float)vista.texture.height }, area, { 0, 0 }, 0, WHITE);
        if (!inPartita || fase == Fase::Pausa) DrawRectangle(0, 0, (int)sw, (int)sh, Hex(0x05081c, 175));

        if (fase == Fase::Menu) {
            float lw = std::min(sw * 0.3f, 520.0f);
            DisegnaLogo({ sw / 2 - lw / 2, sh * 0.06f, lw, lw * 0.34f });
            Scritta2(dati.titolo, sw / 2, sh * 0.06f + lw * 0.34f + dim * 0.6f, (int)(dim * 1.25f), WHITE);
            std::vector<std::string> voci, codici;
            int ultimo = conProgetto ? UltimoSlot() : -1;
            if (ultimo >= 0) { voci.push_back("Continua"); codici.push_back("continua"); }
            voci.push_back("1 giocatore"); codici.push_back("1");
            voci.push_back("2 giocatori (a turni)"); codici.push_back("2");
            if (conProgetto) { voci.push_back("Carica partita"); codici.push_back("carica"); }
            for (const char* x : { "Opzioni", "Crediti", "Esci" }) { voci.push_back(x); codici.push_back(x); }
            float passo = dim * (voci.size() > 5 ? 1.45f : 1.7f);
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
            Scritta2("OPZIONI", sw / 2, sh * 0.14f, (int)(dim * 1.3f), Hex(0xffe066));
            std::vector<std::string> voci = {
                std::string("Musica: ") + (gMusicaGioco ? "accesa (F7)" : "spenta (F7)"),
                std::string("Effetti sonori: ") + (gAudio.volumeEffetti > 0 ? "accesi (F6)" : "spenti (F6)"),
                std::string("Audio: ") + (gAudio.muto ? "muto" : "attivo"),
                std::string("Schermo intero: ") + (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? "si'" : "no"),
                "Indietro" };
            int v = menuOpzioni.Frame(voci, sw / 2, sh * 0.32f, dim * 1.7f, dim);
            if (v == 0) CambiaMusicaGioco(opzioni);
            else if (v == 1) CambiaEffetti(opzioni);
            else if (v == 2) gAudio.muto = !gAudio.muto;
            else if (v == 3) ToggleBorderlessWindowed();
            if (v == 2) SalvaOpzioniAudio(opzioni);
            if (v == 4 || indietro) fase = Fase::Menu;
            Scritta2("Comandi: frecce o WASD per muoversi, Spazio per saltare/sparare, Esc pausa", sw / 2, sh * 0.82f, dimP, Hex(0xa8b4e8));
            Scritta2("F7 musica, F6 effetti sonori, F8 tutto l'audio. Funzionano anche mouse e joypad", sw / 2, sh * 0.82f + dimP * 1.6f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Crediti) {
            float lw = std::min(sw * 0.34f, 600.0f);
            DisegnaLogo({ sw / 2 - lw / 2, sh * 0.1f, lw, lw * 0.34f });
            std::vector<std::pair<std::string, std::string>> righe = {
                { "Gioco", dati.titolo }, { "Ideato e sviluppato da", SVILUPPATORE }, { "Studio", "Elvantis" },
                { "Creato con", "Studio Videogiochi" }, { "Componenti open source", "raylib, GLFW, miniaudio, stb, nlohmann/json, Outfit (premi L)" } };
            float y = sh * 0.1f + lw * 0.34f + dim;
            for (auto& r : righe) {
                Scritta2(r.first, sw / 2, y, dimP, Hex(0xa8b4e8));
                Scritta2(r.second, sw / 2, y + dimP * 1.3f, dim, WHITE);
                y += dimP * 1.3f + dim * 1.5f;
            }
            Scritta2("L: licenze dei componenti      un altro tasto: torna al menu", sw / 2, sh - dimP * 2.4f, dimP, Hex(0xffe066));
            if (tFase > 0.3f && IsKeyPressed(KEY_L)) { fase = Fase::Licenze; tFase = 0; scorriLicenze = 0; }
            else if (tFase > 0.3f && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || gInput.Premuto(Azione::Conferma) || gInput.Premuto(Azione::Indietro))) fase = Fase::Menu;
        } else if (fase == Fase::Licenze) {
            // La licenza di Studio Videogiochi e gli avvisi delle librerie (zlib, MIT, OFL...),
            // inclusi in ogni gioco creato. Si scorrono con frecce, rotella o Pagina su/giu'.
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
            float passo = visibili * 0.9f;
            scorriLicenze -= GetMouseWheelMove() * 3;
            if (IsKeyDown(KEY_DOWN) || gInput.Tenuto(Azione::Giu)) scorriLicenze += 40 * GetFrameTime();
            if (IsKeyDown(KEY_UP) || gInput.Tenuto(Azione::Su)) scorriLicenze -= 40 * GetFrameTime();
            if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressed(KEY_SPACE)) scorriLicenze += passo;
            if (IsKeyPressed(KEY_PAGE_UP)) scorriLicenze -= passo;
            if (IsKeyPressed(KEY_HOME)) scorriLicenze = 0;
            if (IsKeyPressed(KEY_END)) scorriLicenze = massimo;
            scorriLicenze = std::max(0.0f, std::min(massimo, scorriLicenze));
            float x = std::max(20.0f, sw / 2 - 40 * dimL * 0.55f);
            DrawRectangleRounded({ x - 24, top - 12, sw - 2 * x + 48, basso - top + 24 }, 0.03f, 8, ConAlfa(Hex(0x0d0f24), 0.86f));
            BeginScissorMode(0, (int)top, (int)sw, (int)(basso - top));
            int primo = (int)scorriLicenze;
            for (int i = primo; i < (int)righe.size() && i < primo + (int)visibili + 2; i++)
                Ui::Testo(righe[i], x, top + (i - scorriLicenze) * riga, Hex(0xd8def8), dimL);
            EndScissorMode();
            Scritta2("Frecce, rotella o Pagina giu': scorri      Esc o Invio: torna ai crediti", sw / 2, sh - dimP * 2.2f, dimP, Hex(0xa8b4e8));
            if (tFase > 0.3f && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_BACKSPACE) || gInput.Premuto(Azione::Indietro) || gInput.Premuto(Azione::Conferma)))
                { fase = Fase::Crediti; tFase = 0; }
        } else if (fase == Fase::Pausa) {
            Scritta2("PAUSA", sw / 2, sh * 0.22f, (int)(dim * 1.4f), Hex(0xffe066));
            std::vector<std::string> voci = { "Riprendi" };
            if (conProgetto) { voci.push_back("Salva partita"); voci.push_back("Carica partita"); }
            voci.push_back("Ricomincia");
            voci.push_back(std::string("Musica: ") + (gMusicaGioco ? "accesa (F7)" : "spenta (F7)"));
            voci.push_back(std::string("Effetti sonori: ") + (gAudio.volumeEffetti > 0 ? "accesi (F6)" : "spenti (F6)"));
            voci.push_back("Menu principale");
            int v = menuPausa.Frame(voci, sw / 2, sh * 0.30f, dim * 1.45f, dim);
            std::string cod = v >= 0 ? voci[v] : "";
            if (cod == "Riprendi" || (v < 0 && indietro && tFase > 0.1f)) fase = Fase::Gioca;
            else if (cod == "Salva partita") ApriSlot(true, Fase::Pausa);
            else if (cod == "Carica partita") ApriSlot(false, Fase::Pausa);
            else if (cod == "Ricomincia") { Carica(gioco, semeCorrente); CaricaMemoria(gioco); ultimoAuto = gioco.autosalvataggi; fase = Fase::Gioca; tFine = 0; }
            else if (cod.rfind("Musica", 0) == 0) CambiaMusicaGioco(opzioni);
            else if (cod.rfind("Effetti", 0) == 0) CambiaEffetti(opzioni);
            else if (cod == "Menu principale") { fase = Fase::Menu; MusicaMenu(); SalvaMemoria(); }
        } else if (fase == Fase::Slot) {
            Scritta2(slotPerSalvare ? "SALVA LA PARTITA" : "CARICA UNA PARTITA", sw / 2, sh * 0.12f, (int)(dim * 1.3f), Hex(0xffe066));
            std::vector<std::string> voci;
            std::vector<int> numeri;
            for (int n = slotPerSalvare ? 1 : 0; n < 4; n++) {
                const InfoSalvataggio& in = infoSlot[n];
                std::string nome = n == 0 ? "Automatico" : "Spazio " + std::to_string(n);
                std::string riga;
                if (in.valido) {
                    int min = (int)(in.tempo / 60), sec = (int)in.tempo % 60;
                    riga = nome + ":  " + (in.mappa.empty() ? "" : in.mappa + ",  ") + std::to_string(in.punteggio) + " punti,  " +
                           std::to_string(min) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec) + "   " + in.data;
                    if (in.daRiserva) riga += "  (riserva)";
                } else if (in.esiste) riga = nome + ":  danneggiato (" + in.errore + ")";
                else riga = nome + ":  vuoto";
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
            Scritta2(slotPerSalvare ? "Si salva tutto: mappa, eroe, creature con i loro ricordi, oggetti, seme della partita"
                                    : "Il salvataggio automatico si aggiorna a ogni cambio di mappa e a ogni punto di controllo",
                     sw / 2, sh - dimP * 2.4f, dimP, Hex(0xa8b4e8));
        } else if (fase == Fase::Cambio) {
            Scritta2(turno == 0 ? "GIOCATORE 1" : "GIOCATORE 2", sw / 2, sh * 0.36f, (int)(dim * 1.6f), turno == 0 ? Hex(0x7ad0ff) : Hex(0xff8a7a));
            if (turno == 1) Scritta2("Il giocatore 1 ha fatto " + std::to_string(punti[0]) + " punti", sw / 2, sh * 0.5f, dim, WHITE);
            Scritta2("Tocca a te! Premi un tasto per iniziare", sw / 2, sh * 0.64f, dimP + 2, Hex(0xffe066));
            if (tFase > 0.5f && (GetKeyPressed() != 0 || gInput.Premuto(Azione::Conferma) || gInput.Premuto(Azione::Salto) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                fase = Fase::Gioca; tFine = 0; MusicaGioco();
            }
        } else if (fase == Fase::Risultati) {
            Scritta2(gioco.vinto ? "COMPLIMENTI!" : "PARTITA FINITA", sw / 2, sh * 0.22f, (int)(dim * 1.5f), Hex(0xffe066));
            if (giocatori == 1) Scritta2("Punteggio: " + std::to_string(punti[0]), sw / 2, sh * 0.42f, dim, WHITE);
            else {
                Scritta2("Giocatore 1: " + std::to_string(punti[0]) + " punti", sw / 2, sh * 0.38f, dim, Hex(0x7ad0ff));
                Scritta2("Giocatore 2: " + std::to_string(punti[1]) + " punti", sw / 2, sh * 0.38f + dim * 1.6f, dim, Hex(0xff8a7a));
                std::string vince = punti[0] == punti[1] ? "Pareggio!" : punti[0] > punti[1] ? "Vince il giocatore 1!" : "Vince il giocatore 2!";
                Scritta2(vince, sw / 2, sh * 0.38f + dim * 3.6f, (int)(dim * 1.2f), WHITE);
            }
            Scritta2("Premi un tasto per tornare al menu", sw / 2, sh * 0.8f, dimP + 2, Hex(0xa8b4e8));
            if (tFase > 0.8f && (GetKeyPressed() != 0 || gInput.Premuto(Azione::Conferma) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) fase = Fase::Menu;
        }
        if (tAvvisoMenu > 0) {
            tAvvisoMenu -= dt;
            float a = std::min(1.0f, tAvvisoMenu);
            int w = Ui::LarghezzaForte(avvisoMenu, dimP) + dimP * 2;
            Rectangle r = { sw / 2 - w / 2.0f, sh * 0.9f - dimP, (float)w, dimP * 2.2f };
            DrawRectangleRounded(r, 0.4f, 8, ConAlfa(Hex(0x101a40), 0.9f * a));
            Scritta2(avvisoMenu, sw / 2, r.y + dimP * 0.55f, dimP, ConAlfa(Hex(0xffe066), a));
        }
        if (fase == Fase::Gioca && giocatori == 2) Scritta2(turno == 0 ? "G1" : "G2", area.x + dimP * 1.5f, area.y + area.height - dimP * 1.8f, dimP, turno == 0 ? Hex(0x7ad0ff) : Hex(0xff8a7a));
        Rectangle pa = AngoloAudio(area, true);   // in basso a destra: non copre il punteggio
        PulsanteAudio(pa, opzioni);
        PulsantiAudioGioco(pa, opzioni);
        gAudio.MusicaAttiva(gMusicaGioco);
        EndDrawing();
    }
    // Chiudendo la finestra a meta' partita si salva da soli (spazio automatico).
    if (ok && conProgetto && (fase == Fase::Gioca || fase == Fase::Pausa) && !gioco.finito && !gioco.vinto) Salva(0);
    SalvaMemoria();
    if (rt.id) UnloadRenderTexture(rt);
    if (rtVetrina.id) UnloadRenderTexture(rtVetrina);
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
    // Risposte di Claude d'esempio: le azioni si applicano come nell'editor.
    {
        int prima = errori;
        auto Prova = [&](const char* nome, bool ok) {
            if (!ok) { TraceLog(LOG_ERROR, "Claude: %s FALLITO", nome); errori++; }
            else TraceLog(LOG_INFO, "Claude: %s ok", nome);
        };
        Mondo base;
        { Mappa m; m.Nuova(40, 15, Vista::Laterale); m.nome = "Prato"; base.mappe.push_back(m); }
        // 1. Impostazioni del mondo e fisica (con limiti).
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Fatto!","azioni":[{"tipo":"mondo","modifiche":{"titolo":"Corsa nel cielo","genere":"sparatutto","fisica":{"salto":5000}}}]})");
            Prova("mondo", e.modificato && w.titolo == "Corsa nel cielo" && w.genere == Genere::Sparatutto && w.fisica.salto <= 600 && w.mappe.size() == 1 && e.risposta == "Fatto!");
        }
        // 2. Proprieta' della mappa e strati dello sfondo.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Tramonto","azioni":[{"tipo":"mappa","modifiche":{"ora":0.74,"cicloGiorno":false,"telecamera":{"modo":"automatica","velocita":50},
                "strati":[{"nome":"Monti","tipo":"montagne","colore":"#6a3a8a","parallasseX":0.2},{"tipo":"nuvole","colore":"#ffffff"}]}}]})");
            Prova("mappa e strati", e.modificato && w.mappe[0].strati.size() == 2 && w.mappe[0].strati[0].tipo == "montagne" && !w.mappe[0].cicloGiorno &&
                  w.mappe[0].telecamera.modo == ModoTelecamera::Automatica && w.mappe[0].caselle.size() == 15);
        }
        // 3. Terreno a rettangoli e oggetti per casella.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Piattaforme","azioni":[{"tipo":"rettangolo","x0":5,"y0":8,"x1":9,"y1":8,"casella":"-"},
                {"tipo":"aggiungi","oggetti":[{"tipo":"moneta","cx":6,"cy":7},{"tipo":"Moneta","cx":7,"cy":7},{"tipo":"slime","cx":20,"cy":11}]}]})");
            int monete = 0;
            for (auto& o : w.mappe[0].oggetti) monete += o.tipo == "moneta";
            Prova("rettangolo e oggetti", e.modificato && w.mappe[0].Casella(7, 8) == '-' && monete == 2 && w.mappe[0].oggetti.size() == 3);
        }
        // 4. Un'azione sbagliata viene scartata, le altre restano.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Draghi","azioni":[{"tipo":"aggiungi","oggetti":[{"tipo":"drago","cx":3,"cy":3}]},{"tipo":"rettangolo","x0":0,"y0":0,"x1":2,"y1":0,"casella":"="},{"tipo":"vola"}]})");
            Prova("azioni sbagliate scartate", e.modificato && e.scartate.size() == 2 && w.mappe[0].oggetti.empty() && w.mappe[0].Casella(1, 0) == '=');
        }
        // 5. Nuova mappa collegata con una porta; l'editor la apre.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Grotta","azioni":[{"tipo":"nuovaMappa","mappa":{"nome":"Grotta","larghezza":30,"altezza":20,"vista":"alto","caselle":[]}},
                {"tipo":"aggiungi","mappa":0,"oggetti":[{"tipo":"porta","cx":30,"cy":11,"mappa":1}]}]})");
            Prova("nuova mappa con porta", e.modificato && w.mappe.size() == 2 && w.mappe[1].vista == Vista::Alto && w.mappe[1].larghezza == 30 && e.apriMappa == 1 &&
                  w.mappe[0].oggetti.size() == 1 && w.mappe[0].oggetti[0].mappa == 1);
        }
        // 6. Risultato non valido (porta verso una mappa che non c'e'): nessuna modifica.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Porta","azioni":[{"tipo":"aggiungi","oggetti":[{"tipo":"porta","cx":4,"cy":11,"mappa":7}]}]})");
            Prova("risultato non valido rifiutato", !e.modificato && w.mappe[0].oggetti.empty() && !e.scartate.empty());
        }
        // 7. JSON dentro un blocco di testo, e solo testo.
        {
            Mondo w = base;
            EsitoClaude e = ApplicaRispostaClaude(w, 0, "Ecco qui:\n```json\n{\"risposta\":\"Ok\",\"azioni\":[{\"tipo\":\"rimuovi\",\"oggettiTipo\":\"tutti\"},{\"tipo\":\"mappa\",\"modifiche\":{\"nome\":\"Valle\"}}]}\n```");
            Prova("JSON fra le righe", e.modificato && w.mappe[0].nome == "Valle");
            EsitoClaude t = ApplicaRispostaClaude(w, 0, "Certo, ti spiego come funzionano le porte.");
            Prova("risposta solo a parole", !t.modificato && t.risposta.find("porte") != std::string::npos);
        }
        // 8. Togliere oggetti per tipo e per casella; caselle nuove con la loro misura.
        {
            Mondo w = base;
            ApplicaRispostaClaude(w, 0, R"({"risposta":"","azioni":[{"tipo":"aggiungi","oggetti":[{"tipo":"moneta","cx":2,"cy":5},{"tipo":"gemma","cx":3,"cy":5},{"tipo":"slime","cx":10,"cy":11}]}]})");
            EsitoClaude e = ApplicaRispostaClaude(w, 0, R"({"risposta":"Via","azioni":[{"tipo":"rimuovi","oggettiTipo":"slime"},{"tipo":"rimuovi","caselle":[[2,5]]}]})");
            bool soloGemma = w.mappe[0].oggetti.size() == 1 && w.mappe[0].oggetti[0].tipo == "gemma";
            EsitoClaude c = ApplicaRispostaClaude(w, 0, R"({"risposta":"Nuova","azioni":[{"tipo":"mappa","modifiche":{"caselle":["          ","          ","          ","          ","##########","##########"],"partenza":{"x":24,"y":64}}}]})");
            Prova("rimuovi e ridisegna", e.modificato && soloGemma && c.modificato && w.mappe[0].larghezza == 10 && w.mappe[0].altezza == 6);
        }
        // 9. Lo stato mandato a Claude si rilegge.
        {
            std::string stato = StatoMondoPerClaude(MondiDiProva()[2], 1);
            Prova("stato per Claude", stato.find("\"mappe\"") != std::string::npos && stato.size() < 200000);
            Prova("istruzioni per Claude", PromptMondo().size() > 2000);
        }
        TraceLog(LOG_INFO, "Prove delle risposte di Claude: %d problemi", errori - prima);
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


// Prove dei giochi completi (.gioco): ogni genere e stile, con tutte le raccolte nella
// libreria, va in JSON e torna uguale, passa il controllo e gira nel motore. Poi un drago
// creato da Claude diventa l'eroe e il gioco si salva e si riapre.
int VerificaProgetti() {
    int errori = 0;
    auto Prova = [&](const std::string& nome, bool ok, const std::string& extra = "") {
        if (!ok) { TraceLog(LOG_ERROR, "Progetti: %s FALLITO %s", nome.c_str(), extra.c_str()); errori++; }
        else TraceLog(LOG_INFO, "Progetti: %s ok", nome.c_str());
    };
    for (int st = 0; st < 2; st++) {
        StileGrafico stile = (StileGrafico)st;
        for (int g = 0; g < (int)Genere::Totale; g++) {
            ProgettoGioco p = NuovoProgetto((Genere)g, stile, std::string("Prova ") + NomeGenere((Genere)g));
            if (g == 0) for (int i = 0; i < NumeroRaccolte(); i++) {
                const Raccolta& r = CreaRaccolta(i, stile);
                for (auto x : r.personaggi) { x.nome = p.NomeLibero(x.nome); p.personaggi.push_back(x); }
                for (auto x : r.oggetti) { x.nome = p.NomeLibero(x.nome); p.oggetti.push_back(x); }
                for (auto& x : r.scenari) p.scenari.push_back(x);
                for (auto& x : r.brani) p.brani.push_back(x);
                for (auto& x : r.effetti) p.effetti.push_back(x);
            }
            std::string nome = std::string(NomeStile(stile)) + " / " + NomeGenere((Genere)g);
            std::string json = p.Json(), errore;
            ProgettoGioco q;
            bool riletto = q.DaJson(json, errore);
            Prova(nome + ": JSON riletto", riletto, errore);
            if (!riletto) continue;
            Prova(nome + ": JSON uguale", q.Json() == json);
            std::string problema = q.Controlla();
            Prova(nome + ": controllo", problema.empty(), problema);
            Gioco gioco;
            bool caricato = gioco.CaricaProgetto(q, errore);
            Prova(nome + ": nel motore", caricato, errore);
            Comandi c; c.asse = 1;
            for (int i = 0; caricato && i < 240; i++) { c.salto = c.saltoPremuto = i % 40 == 0; gioco.Aggiorna(1 / 60.0f, c); }
            if (g == 0) TraceLog(LOG_INFO, "Progetti: %s con le raccolte: %d personaggi, %d oggetti, %d scenari, %d brani, %d effetti, JSON %d KB",
                                 NomeStile(stile), (int)q.personaggi.size(), (int)q.oggetti.size(), (int)q.scenari.size(), (int)q.brani.size(), (int)q.effetti.size(), (int)(json.size() / 1024));
        }
    }
    // Il drago di Claude: si crea, diventa l'eroe, si salva e si riapre.
    {
        ProgettoGioco p = NuovoProgetto(Genere::Platform, StileGrafico::PixelFine, "Il drago");
        EsitoProgetto e = ApplicaRispostaProgetto(p, 0, R"({"risposta":"Ecco il drago!","azioni":[
            {"tipo":"personaggio","personaggio":{"nome":"Drago","disegno":{"palette":{"g":"#3aa04a","y":"#ffd040","r":"#d8344a"},
              "animazioni":[{"nome":"fermo","fps":4,"fotogrammi":[["..gg....",".gggg...","gygggg.r","gggggggr","..gggg..","..g..g.."],
                                                                 ["..gg....",".gggg...","gygggg..","gggggggr","..gggg..",".g....g."]]},
                            {"nome":"corsa","fps":8,"fotogrammi":[["..gg....",".gggg...","gygggg.r","gggggggr","..gggg..","..g..g.."]]}]}}},
            {"tipo":"eroe","nome":"Drago"}]})");
        const PersonaggioDef* d = p.Personaggio("Drago");
        Prova("drago di Claude creato", e.modificato && d && p.eroe == "Drago" && !d->disegno.animazioni.empty());
        ProgettoGioco q;
        std::string errore;
        bool ok = q.DaJson(p.Json(), errore);
        Prova("drago salvato e riaperto", ok && q.Personaggio("Drago") && q.eroe == "Drago", errore);
        Gioco gioco;
        Prova("drago come eroe nel gioco", ok && gioco.CaricaProgetto(q, errore), errore);
        // Il gioco creato: il pacchetto porta il progetto e si riconosce.
        DatiGioco dg{ "Il drago", q.Json(true), {} };
        DatiGioco letto;
        Prova("pacchetto del gioco", DeserializzaGioco(SerializzaGioco(dg), letto) && EProgetto(letto.livello) && letto.livello == dg.livello);
    }
    // Un gioco con tutto: un personaggio per ogni comportamento, un oggetto per ogni funzione,
    // una regola per ogni evento con tutte le azioni, caselle, brano ed effetto disegnati.
    // Si salva, si riapre uguale e gira nel motore con comandi di ogni tipo.
    for (int st = 0; st < 2; st++) {
        StileGrafico stile = (StileGrafico)st;
        ProgettoGioco p = NuovoProgetto(Genere::Platform, stile, "Tutto");
        const Raccolta& r = CreaRaccolta(0, stile);
        Mappa& m = p.mondo.mappe[0];
        { Mappa b = m; b.nome = "Seconda"; p.mondo.mappe.push_back(b); }
        Mappa& m0 = p.mondo.mappe[0];
        for (int t = 0; t < (int)TipoComportamento::Totale; t++) {
            PersonaggioDef d = r.personaggi.empty() ? PersonaggioDef() : r.personaggi[0];
            d.nome = std::string("P") + ChiaveComportamento((TipoComportamento)t);
            d.comportamento.tipo = (TipoComportamento)t;
            d.comportamento.salto = t % 2 ? 200 : 0;
            d.comportamento.proiettile = t == (int)TipoComportamento::Spara ? "Oproiettile" : "";
            d.comportamento.dialogo = "Ciao!";
            p.personaggi.push_back(d);
            OggettoMappa o; o.tipo = d.nome; o.x = 120.0f + t * 40; o.y = 200; m0.oggetti.push_back(o);
        }
        for (int f = 0; f < (int)FunzioneOggetto::Totale; f++) {
            OggettoDef d = r.oggetti.empty() ? OggettoDef() : r.oggetti[0];
            d.nome = std::string("O") + ChiaveFunzione((FunzioneOggetto)f);
            d.funzione = (FunzioneOggetto)f;
            d.richiede = f == (int)FunzioneOggetto::Porta ? "Ochiave" : "";
            d.variabile = "gemme";
            p.oggetti.push_back(d);
            OggettoMappa o; o.tipo = d.nome; o.x = 60.0f + f * 24; o.y = 230; o.mappa = f == (int)FunzioneOggetto::Porta ? 1 : -1;
            m0.oggetti.push_back(o);
        }
        p.eroe = "Pgiocatore";
        p.variabili["gemme"] = 0;
        const auto& ev = TipiEvento();
        const auto& az = TipiAzione();
        for (size_t i = 0; i < ev.size(); i++) {
            Regola g;
            g.evento = ev[i];
            g.chi = i % 3 == 0 ? "eroe" : i % 3 == 1 ? "gioco" : "Ppattuglia";
            g.conChi = ev[i] == "tasto" ? "T" : ev[i] == "ogni" ? "" : "Omoneta";
            g.variabile = ev[i] == "variabile" ? "gemme" : "";
            g.valore = 2;
            for (size_t k = 0; k < az.size(); k++) {
                if ((k + i) % 4) continue;
                AzioneRegola a; a.tipo = az[k]; a.numero = 1;
                a.testo = az[k] == "vai_mappa" ? "Seconda" : az[k] == "crea" ? "Oraccogli" : az[k] == "aumenta" || az[k] == "imposta" ? "gemme" : "Ciao";
                if (az[k] == "vittoria" || az[k] == "game_over" || az[k] == "vai_mappa") continue;
                g.azioni.push_back(a);
            }
            p.regole.push_back(g);
        }
        p.mondo.mappe[0].ImpostaCasella(5, 5, '#');
        std::string nome = std::string("tutto il gioco (") + NomeStile(stile) + ")";
        std::string json = p.Json(), errore;
        ProgettoGioco q;
        bool riletto = q.DaJson(json, errore);
        Prova(nome + ": JSON riletto", riletto, errore);
        if (!riletto) continue;
        Prova(nome + ": JSON uguale", q.Json() == json);
        Prova(nome + ": tutte le regole", q.regole.size() == ev.size() && q.personaggi.size() == p.personaggi.size() && q.oggetti.size() == p.oggetti.size());
        Gioco gioco;
        bool caricato = gioco.CaricaProgetto(q, errore);
        Prova(nome + ": nel motore", caricato, errore);
        for (int i = 0; caricato && i < 900; i++) {
            Comandi c;
            c.asse = (i / 90) % 2 ? 1.0f : -0.6f;
            c.asseY = (i / 50) % 3 - 1.0f;
            c.salto = c.saltoPremuto = i % 30 == 0;
            gioco.Aggiorna(1 / 60.0f, c);
        }
    }
    TraceLog(errori ? LOG_ERROR : LOG_INFO, "Prove dei progetti: %d problemi", errori);
    return errori;
}


} // namespace

int main(int argc, char** argv) {
    if (argc > 2 && std::string(argv[1]) == "--prova-arte") { extern int ProvaArte(const char*); extern int ProvaPaesaggio(const char*); ProvaPaesaggio(argv[2]); return ProvaArte(argv[2]); }
    if (argc > 3 && std::string(argv[1]) == "--crea-platform") { extern int CreaPlatformDaRigaDiComando(int, char**); return CreaPlatformDaRigaDiComando(argc, argv); }
    if (argc > 2 && std::string(argv[1]) == "--prova-livelli") { extern int ProvaLivelliDaRigaDiComando(int, char**); return ProvaLivelliDaRigaDiComando(argc, argv); }
    if (argc > 3 && std::string(argv[1]) == "--crea-arcade") { extern int CreaArcadeDaRigaDiComando(int, char**); return CreaArcadeDaRigaDiComando(argc, argv); }
    if (argc > 3 && std::string(argv[1]) == "--foto-arcade") { extern int FotoArcadeDaRigaDiComando(int, char**); return FotoArcadeDaRigaDiComando(argc, argv); }
    if (argc > 2 && std::string(argv[1]) == "--prova-arcade") { extern int ProvaArcadeDaRigaDiComando(int, char**); return ProvaArcadeDaRigaDiComando(argc, argv); }
    if (argc > 3 && std::string(argv[1]) == "--foto-gioco") { extern int FotoGiocoDaRigaDiComando(int, char**); return FotoGiocoDaRigaDiComando(argc, argv); }
    if (argc > 1 && std::string(argv[1]) == "--crea") return CreaDaRigaDiComando(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "--verifica-salvataggi") { extern int VerificaSalvataggi(); return VerificaSalvataggi() ? 1 : 0; }
    if (argc > 1 && std::string(argv[1]) == "--verifica-mondi") { int e = VerificaMondi(); e += VerificaProgetti(); return e ? 1 : 0; }
    if (argc > 2 && std::string(argv[1]) == "--esporta-mondi") {
        MakeDirectory(argv[2]);
        for (auto& m : MondiDiProva()) { std::string t = m.Json(); SaveFileText((std::string(argv[2]) + "/" + NomeFileDaTitolo(m.titolo) + ".json").c_str(), &t[0]); }
        return 0;
    }
    if (argc > 2 && std::string(argv[1]) == "--esporta-audio") return gAudio.EsportaWav(argv[2]) ? 0 : 1;

    std::string dati;
    DatiGioco gioco;
    if (LeggiPacchettoIncluso(dati) && DeserializzaGioco(dati, gioco)) return GiocaPacchetto(gioco);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Studio Videogiochi - Elvantis");
    FinestraGrande();
    SetWindowMinSize(1024, 600);
    SetExitKey(KEY_NULL); // Esc non chiude lo studio
    SetTargetFPS(60);
    CaricaRisorse();
    gInput.Carica();
    gAudio.Inizializza();
    const std::string opzioni = CartellaEseguibile() + "/studio.opzioni";
    CaricaOpzioniAudio(opzioni);
    // La musica dello studio e' spenta di serie (F7 o il pulsante a nota la accendono);
    // gli effetti sonori restano sempre, e la musica suona comunque nella prova del gioco.
    gAudio.MusicaAttiva(gMusicaStudio);

    Officina officina;
    officina.fileOpzioni = opzioni;
    officina.Avvia();
    // Un file .gioco passato all'avvio (es. trascinato sull'icona) si apre subito.
    if (argc > 1 && argv[1][0] != '-') officina.ApriFile(argv[1]);
    bool aperta = Intro("Studio Videogiochi", std::string("Sviluppato da ") + SVILUPPATORE + " - Elvantis", false, 3.0f);
    gAudio.ScegliBrano(0);
    while (aperta && !WindowShouldClose() && !officina.esci) {
        float dt = GetFrameTime();
        gInput.Aggiorna(1, { 0, 0 });
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        gAudio.Aggiorna(dt);
        BeginDrawing();
        officina.Frame(dt);
        Ui::DisegnaAiuto();   // help contestuale sopra a tutto
        Rectangle pa = officina.PostoAudio();
        PulsanteAudio(pa, opzioni);
        if (!officina.Provando()) PulsanteMusica({ pa.x - pa.width - 6, pa.y, pa.width, pa.height }, opzioni);
        else PulsantiAudioGioco(pa, opzioni);   // nella prova: musica (F7) ed effetti (F6) del gioco, separati
        gAudio.MusicaAttiva(officina.Provando() ? gMusicaGioco : (gMusicaStudio || officina.AscoltaMusica()));
        EndDrawing();
    }
    officina.SalvaPrimaDiUscire();
    if (gLogo.id) UnloadTexture(gLogo);
    gAudio.Chiudi();
    CloseWindow();
    return 0;
}
