#include "editor_mondo.h"
#include "audio.h"
#include "claude_mondo.h"
#include "immagini.h"
#include "pacchetto.h"
#include "periferiche.h"
#include "sistema.h"
#include "ui.h"
#include <algorithm>
#include <fstream>

namespace {

const char* NOMI_STRUMENTI[] = { "Pennello", "Gomma", "Riempi", "Rettangolo", "Seleziona", "Contagocce" };
const char* TASTI_STRUMENTI[] = { "B", "E", "F", "R", "V", "I" };
const char* CASELLE = " #=-^.";
const char* NOMI_CASELLE[] = { "Vuoto", "Terra", "Pietra", "Piattaforma", "Spine", "Bersaglio" };

struct VoceMenuEd { const char* testo; const char* tasto; };
const char* MENU[] = { "File", "Modifica", "Mondo", "Mappa", "Vista", "Prova", "Aiuto" };
constexpr int NUM_MENU = 7;
const std::vector<std::vector<VoceMenuEd>>& VociMenu() {
    static const std::vector<std::vector<VoceMenuEd>> v = {
        { { "Nuovo mondo...", "Ctrl+N" }, { "Apri...", "Ctrl+O" }, { "Salva", "Ctrl+S" }, { "Salva con nuovo nome", "" },
          { "Crea il gioco .exe", "Ctrl+E" }, { "Editor classico (musiche e suoni)", "" }, { "Torna al menu dello studio", "Esc" } },
        { { "Annulla", "Ctrl+Z" }, { "Ripeti", "Ctrl+Y" }, { "Copia oggetto", "Ctrl+C" }, { "Incolla oggetto", "Ctrl+V" },
          { "Elimina oggetto", "Canc" } },
        { { "Proprieta' del mondo", "" }, { "Strati dello sfondo", "" }, { "Chat con Claude", "F3" } },
        { { "Nuova mappa (stessa vista)", "" }, { "Nuova mappa vista dall'alto", "" }, { "Duplica mappa", "" },
          { "Rinomina mappa", "" }, { "Elimina mappa", "" }, { "Proprieta' della mappa", "" } },
        { { "Griglia", "G" }, { "Sfondo nell'anteprima", "" }, { "Inquadratura del gioco", "" }, { "Ingrandisci", "+" },
          { "Rimpicciolisci", "-" }, { "Inquadra la partenza", "Home" } },
        { { "Prova dall'inizio", "F5" }, { "Prova da qui (sotto il mouse)", "F6" } },
        { { "Comandi dell'editor", "F1" } },
    };
    return v;
}

const std::vector<std::string>& TipiOggetto() {
    static std::vector<std::string> t;
    if (t.empty()) for (int i = 0; i < NUM_TIPI_OGGETTO; i++) t.push_back(ChiaveOggetto((TipoOggetto)i));
    return t;
}

std::string Maiuscola(std::string s) { if (!s.empty()) s[0] = (char)std::toupper((unsigned char)s[0]); return s; }

bool Ctrl() { return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL); }
bool Maiusc() { return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT); }

Oggetto DaMappa(const OggettoMappa& om, float t) {
    Oggetto o{};
    TipoDaNome(om.tipo, o.tipo);
    o.pos = o.origine = { om.x, om.y };
    o.numero = om.numero;
    o.t = t;
    o.attivo = true;
    o.dir = -1;
    return o;
}

} // namespace

// ---------------------------------------------------------------------------

void EditorMondo::Inizializza() {
    cfg = ConfigClaude::Carica();
    std::vector<Mondo> esempi = MondiDiProva();
    Apri(esempi.size() > 1 ? esempi[1] : esempi[0], "", "Benvenuto nel nuovo editor! Premi F1 per i comandi.");
}

void EditorMondo::Apri(const Mondo& m, const std::string& p, const std::string& msg) {
    mondo = m;
    if (mondo.mappe.empty()) { Mappa vuota; vuota.Nuova(80, 20, Vista::Laterale); mondo.mappe.push_back(vuota); }
    percorso = p;
    mappa = mondo.mappaIniziale;
    annulla.clear(); ripeti.clear();
    modificato = false;
    selezionato = -1;
    CambiaMappa(mappa);
    if (!msg.empty()) Avvisa(msg);
}

void EditorMondo::Entra() {
    if (rtSfondo.id == 0) rtSfondo = LoadRenderTexture(VW, VH);
    if (rtGioco.id == 0) rtGioco = LoadRenderTexture(VW, VH);
    vuoleUscire = false;
    cfg = ConfigClaude::Carica();
    CaricaImmagini();
    gAudio.ScegliBrano(M().musica);
}

void EditorMondo::Esci() {
    provando = false;
    gAudio.ScegliBrano(0);
}

bool EditorMondo::Scrivendo() const { return Ui::StaScrivendo(); }

void EditorMondo::Ricorda() {
    annulla.push_back(mondo);
    if (annulla.size() > 60) annulla.erase(annulla.begin());
    ripeti.clear();
    modificato = true;
}

void EditorMondo::Annulla() {
    if (annulla.empty()) { Avvisa("Niente da annullare"); return; }
    ripeti.push_back(mondo);
    mondo = annulla.back();
    annulla.pop_back();
    mappa = std::min(mappa, (int)mondo.mappe.size() - 1);
    selezionato = -1;
    Ricostruisci();
    Avvisa("Annullato");
}

void EditorMondo::Ripeti() {
    if (ripeti.empty()) { Avvisa("Niente da ripetere"); return; }
    annulla.push_back(mondo);
    mondo = ripeti.back();
    ripeti.pop_back();
    mappa = std::min(mappa, (int)mondo.mappe.size() - 1);
    selezionato = -1;
    Ricostruisci();
    Avvisa("Ripetuto");
}

void EditorMondo::Ricostruisci() {
    std::string errore;
    if (!vista.DaMappa(M(), errore)) Avvisa("Mappa non valida: " + errore);
    sfondo.tema = M().tema; sfondo.ora = M().ora;
    sfondo.generato = M().sfondoGenerato;
    sfondo.cieloPersonale = M().cieloPersonale;
    sfondo.cieloAlto = M().cieloAlto; sfondo.cieloBasso = M().cieloBasso;
    sfondo.strati = M().strati;
    if (selezionato >= (int)M().oggetti.size()) selezionato = -1;
}

void EditorMondo::CambiaMappa(int i) {
    mappa = (int)Limita((float)i, 0, (float)mondo.mappe.size() - 1);
    selezionato = -1;
    Ricostruisci();
    Inquadra();
    if (IsAudioDeviceReady()) gAudio.ScegliBrano(M().musica);
}

void EditorMondo::Inquadra() {
    // La partenza al centro della vista (la vista si conosce solo nel disegno: stima con lo schermo).
    float vw = (rVista.width > 1 ? rVista.width : GetScreenWidth() * 0.6f) / zoom;
    float vh = (rVista.height > 1 ? rVista.height : GetScreenHeight() * 0.8f) / zoom;
    camX = M().partenza.x - vw / 2;
    camY = M().partenza.y - vh * 0.6f;
    // La mappa resta inquadrata: centrata se e' piu' piccola della vista, altrimenti senza vuoti ai bordi.
    float W = M().larghezza * (float)TILE, H = M().altezza * (float)TILE;
    camX = W < vw ? (W - vw) / 2 : Limita(camX, -TILE, W - vw + TILE);
    camY = H < vh ? (H - vh) / 2 : Limita(camY, -TILE, H - vh + TILE);
}

Vector2 EditorMondo::MondoDaSchermo(Vector2 p) const {
    return { (p.x - rVista.x) / zoom + camX, (p.y - rVista.y) / zoom + camY };
}

int EditorMondo::OggettoSotto(Vector2 w) const {
    const auto& ogg = mondo.mappe[mappa].oggetti;
    for (int i = (int)ogg.size() - 1; i >= 0; i--) {
        Rectangle b = BoxOggetto(DaMappa(ogg[i], 0));
        b.x -= 2; b.y -= 2; b.width += 4; b.height += 4;
        if (CheckCollisionPointRec(w, b)) return i;
    }
    return -1;
}

void EditorMondo::Dipingi(int cx, int cy) {
    Mappa& m = M();
    if (cx < 0 || cy < 0 || cx >= m.larghezza || cy >= m.altezza) return;
    if (pennello == "partenza") {
        m.partenza = m.vista == Vista::Alto ? Vector2{ cx * TILE + TILE / 2.0f, cy * TILE + TILE - 2.0f } : Vector2{ cx * TILE + TILE / 2.0f, (cy + 1) * (float)TILE };
        return;
    }
    if (pennello.size() == 1) {
        if (m.Casella(cx, cy) != pennello[0]) { m.ImpostaCasella(cx, cy, pennello[0]); vista.ImpostaCella(cx, cy, pennello[0] == ' ' ? ' ' : pennello[0]); }
        return;
    }
    // Oggetto: uno per casella e per tipo.
    for (auto& o : m.oggetti)
        if (o.tipo == pennello && (int)(o.x / TILE) == cx && (int)((o.y - 1) / TILE) == cy) return;
    OggettoMappa o;
    o.tipo = pennello;
    Vector2 p = PosizioneInCasella(pennello, cx, cy, m.vista);
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) p = MondoDaSchermo(GetMousePosition());   // posizione libera
    o.x = p.x; o.y = p.y;
    if (pennello == "cartello") {
        int n = 1;
        for (auto& a : m.oggetti) if (a.tipo == "cartello") n = std::max(n, a.numero + 1);
        o.numero = std::min(n, 9);
    }
    if (pennello == "boss") o.numero = 9;
    m.oggetti.push_back(o);
    selezionato = (int)m.oggetti.size() - 1;
    gAudio.Suona(Effetto::Moneta, 0.25f);
}

void EditorMondo::Cancella(int cx, int cy, Vector2 punto) {
    Mappa& m = M();
    int i = OggettoSotto(punto);
    if (i >= 0) { m.oggetti.erase(m.oggetti.begin() + i); selezionato = -1; return; }
    if (cx >= 0 && cy >= 0 && cx < m.larghezza && cy < m.altezza && m.Casella(cx, cy) != ' ') {
        m.ImpostaCasella(cx, cy, ' ');
        vista.ImpostaCella(cx, cy, ' ');
    }
}

void EditorMondo::Riempi(int cx, int cy, char c) {
    Mappa& m = M();
    if (cx < 0 || cy < 0 || cx >= m.larghezza || cy >= m.altezza) return;
    char da = m.Casella(cx, cy);
    if (da == c) return;
    std::vector<std::pair<int, int>> pila{ { cx, cy } };
    int n = 0;
    while (!pila.empty() && n < 400000) {
        auto [x, y] = pila.back();
        pila.pop_back();
        if (x < 0 || y < 0 || x >= m.larghezza || y >= m.altezza || m.Casella(x, y) != da) continue;
        m.ImpostaCasella(x, y, c);
        n++;
        pila.push_back({ x + 1, y }); pila.push_back({ x - 1, y }); pila.push_back({ x, y + 1 }); pila.push_back({ x, y - 1 });
    }
    Ricostruisci();
}

// --- File --------------------------------------------------------------------

void EditorMondo::Salva(bool conNome) {
    std::string cartella = CartellaEseguibile() + "/mondi";
    MakeDirectory(cartella.c_str());
    if (percorso.empty() || conNome) {
        std::string base = cartella + "/" + NomeFileDaTitolo(mondo.titolo);
        percorso = base + ".json";
        for (int i = 2; conNome && FileExists(percorso.c_str()); i++) percorso = base + std::to_string(i) + ".json";
    }
    std::string testo = mondo.Json();
    if (SaveFileText(percorso.c_str(), &testo[0])) { modificato = false; Avvisa("Salvato in mondi/" + std::string(GetFileName(percorso.c_str()))); }
    else Avvisa("Non riesco a salvare " + percorso);
}

void EditorMondo::CreaEseguibile() {
    std::string problema = mondo.Controlla();
    if (!problema.empty()) { Avvisa("Il mondo ha un problema: " + problema); return; }
#ifdef _WIN32
    std::string nome = NomeFileDaTitolo(mondo.titolo) + ".exe";
#else
    std::string nome = NomeFileDaTitolo(mondo.titolo);
#endif
    std::string errore;
    DatiGioco d{ mondo.titolo, mondo.Json(true), {} };
    // Le immagini usate dal mondo viaggiano dentro il gioco.
    std::vector<std::string> usate;
    for (auto& m : mondo.mappe) {
        for (auto& s : m.strati) if (!s.immagine.empty()) usate.push_back(s.immagine);
        for (auto& dd : m.decorazioni) if (!dd.immagine.empty()) usate.push_back(dd.immagine);
    }
    std::sort(usate.begin(), usate.end());
    usate.erase(std::unique(usate.begin(), usate.end()), usate.end());
    for (auto& u : usate) {
        int n = 0;
        unsigned char* b = LoadFileData((CartellaEseguibile() + "/immagini/" + u).c_str(), &n);
        if (b) { d.file.push_back({ u, std::string((const char*)b, (size_t)n) }); UnloadFileData(b); }
    }
    if (::CreaEseguibile(CartellaEseguibile() + "/" + nome, SerializzaGioco(d), errore)) {
        Avvisa("Creato " + nome + " accanto allo studio!");
        gAudio.Suona(Effetto::Vittoria);
    } else Avvisa("Errore: " + errore);
}

void EditorMondo::ApriElenco() {
    elencoMondi = MondiDiProva();
    elencoPercorsi.assign(elencoMondi.size(), "");
    std::string cartella = CartellaEseguibile() + "/mondi";
    if (DirectoryExists(cartella.c_str())) {
        FilePathList l = LoadDirectoryFilesEx(cartella.c_str(), ".json", false);
        for (unsigned int i = 0; i < l.count; i++) {
            char* t = LoadFileText(l.paths[i]);
            if (!t) continue;
            Mondo m;
            std::string e;
            if (m.DaJson(t, e)) { elencoMondi.push_back(m); elencoPercorsi.push_back(l.paths[i]); }
            UnloadFileText(t);
        }
        UnloadDirectoryFiles(l);
    }
    // Anche i livelli del vecchio editor (cartella "livelli"): si importano da soli.
    std::string vecchi = CartellaEseguibile() + "/livelli";
    if (DirectoryExists(vecchi.c_str())) {
        FilePathList l = LoadDirectoryFilesEx(vecchi.c_str(), ".txt", false);
        for (unsigned int i = 0; i < l.count; i++) {
            char* t = LoadFileText(l.paths[i]);
            if (!t) continue;
            Mondo m;
            std::string e;
            if (m.Carica(t, e)) {
                if (m.titolo.empty()) m.titolo = GetFileNameWithoutExt(l.paths[i]);
                m.titolo += " (vecchio editor)";
                elencoMondi.push_back(m);
                elencoPercorsi.push_back("");
            }
            UnloadFileText(t);
        }
        UnloadDirectoryFiles(l);
    }
    finestra = 2;
}

// Modelli di partenza per ogni genere.
void EditorMondo::NuovoMondo(int genere) {
    Mondo w;
    Mappa m;
    switch (genere) {
        case 1:   // open world
            w.titolo = "Il mio open world"; w.genere = Genere::OpenWorld;
            m.Nuova(80, 60, Vista::Alto);
            m.nome = "Mondo"; m.telecamera.anticipo = 0; m.telecamera.zonaMortaX = 30; m.telecamera.zonaMortaY = 20; m.musica = 7;
            break;
        case 2:   // sparatutto
            w.titolo = "Il mio sparatutto"; w.genere = Genere::Sparatutto;
            m.Nuova(200, 17, Vista::Laterale);
            for (int y = 14; y < 17; y++) for (int x = 0; x < 200; x++) m.ImpostaCasella(x, y, ' ');
            m.nome = "Cielo"; m.telecamera.modo = ModoTelecamera::Automatica; m.telecamera.velocita = 40; m.musica = 5; m.tema = 2;
            m.partenza = { 60, 120 };
            break;
        case 3:   // avventura a stanze
            w.titolo = "La mia avventura"; w.genere = Genere::Avventura;
            m.Nuova(90, 34, Vista::Laterale);
            m.nome = "Castello"; m.telecamera.modo = ModoTelecamera::Stanze; m.tema = 3; m.ora = 0.9f; m.cicloGiorno = false; m.musica = 6;
            for (int x = 0; x < 90; x++) m.ImpostaCasella(x, 16, '=');
            break;
        default:  // platform
            w.titolo = "Il mio platform"; w.genere = Genere::Platform;
            m.Nuova(120, 20, Vista::Laterale);
            m.nome = "Livello 1"; m.telecamera.zonaMortaX = 16; m.telecamera.zonaMortaY = 24;
            {
                OggettoMappa f; f.tipo = "bandiera"; Vector2 p = PosizioneInCasella("bandiera", 115, 16, Vista::Laterale);
                f.x = p.x; f.y = p.y; m.oggetti.push_back(f);
            }
            break;
    }
    w.mappe.push_back(m);
    Apri(w, "", std::string("Nuovo mondo: ") + NomeGenere(w.genere) + ". Disegna con il mouse o chiedi a Claude!");
}

// --- Chat con Claude -----------------------------------------------------------

void EditorMondo::InviaChat(const std::string& testo) {
    if (testo.empty() || chatInAttesa) return;
    if (cfg.chiave.empty()) { chat.storia.push_back({ false, "Prima serve la chiave API: incollala qui sotto e premi Salva.", "" }); return; }
    chat.Invia(cfg, testo, StatoMondoPerClaude(mondo, mappa), PromptMondo());
    chatInAttesa = true;
    tAttesa = 0;
    scorriChat = 0;
    gAudio.Suona(Effetto::Conferma, 0.5f);
}

void EditorMondo::ControllaChat() {
    if (!chatInAttesa) return;
    int s = chat.Stato();
    if (s != 2 && s != 3) return;
    chatInAttesa = false;
    if (s == 3) {
        chat.storia.push_back({ false, "Non riesco a parlare con Claude: " + chat.RispostaGrezza(), "{\"risposta\": \"errore di rete\", \"azioni\": []}" });
        return;
    }
    std::string grezza = chat.RispostaGrezza();
    Mondo prima = mondo;
    EsitoClaude e = ApplicaRispostaClaude(mondo, mappa, grezza);
    std::string mostra = e.risposta.empty() ? "(nessun messaggio)" : e.risposta;
    if (e.modificato) {
        annulla.push_back(prima);
        ripeti.clear();
        modificato = true;
        mostra += "\n[Fatto: ";
        for (size_t i = 0; i < e.fatte.size(); i++) mostra += (i ? "; " : "") + e.fatte[i];
        mostra += ". Ctrl+Z per annullare]";
        gAudio.Suona(Effetto::Gemma);
    }
    if (!e.scartate.empty()) {
        mostra += "\n[Non applicato: ";
        for (size_t i = 0; i < e.scartate.size(); i++) mostra += (i ? "; " : "") + e.scartate[i];
        mostra += "]";
    }
    chat.storia.push_back({ false, mostra, grezza });
    if (e.apriMappa >= 0) CambiaMappa(e.apriMappa);
    else if (e.modificato) { mappa = std::min(mappa, (int)mondo.mappe.size() - 1); Ricostruisci(); }
}

// --- Prova -----------------------------------------------------------------------

void EditorMondo::AvviaProva(bool daQui) {
    Mondo copia = mondo;
    copia.mappaIniziale = mappa;
    if (daQui) {
        Vector2 punto = GetMousePosition();
        if (!CheckCollisionPointRec(punto, rVista)) punto = { rVista.x + rVista.width / 2, rVista.y + rVista.height / 2 };
        Vector2 w = MondoDaSchermo(punto);
        int cx = (int)std::floor(w.x / TILE), cy = (int)std::floor(w.y / TILE);
        if (cx >= 0 && cy >= 0 && cx < M().larghezza && cy < M().altezza)
            copia.mappe[mappa].partenza = M().vista == Vista::Alto ? Vector2{ cx * TILE + TILE / 2.0f, cy * TILE + TILE - 2.0f }
                                                                  : Vector2{ cx * TILE + TILE / 2.0f, (cy + 1) * (float)TILE };
    }
    std::string errore;
    prova = Gioco{};
    if (!prova.CaricaMondo(copia, errore)) { Avvisa("Non si puo' provare: " + errore); return; }
    provando = true;
    tProva = 0;
    Ui::TogliFuoco();
    gAudio.ScegliBrano(prova.musica);
    gAudio.Suona(Effetto::Conferma);
}

void EditorMondo::FrameProva(float dt) {
    tProva += dt;
    if (tProva > 0.2f && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F5))) { provando = false; gAudio.ScegliBrano(M().musica); return; }
    if (IsKeyPressed(KEY_R) || (prova.finito && gInput.Premuto(Azione::Conferma))) prova.Ricomincia();
    if (IsKeyPressed(KEY_F1)) prova.mostraCollisioni = !prova.mostraCollisioni;
    if (IsKeyPressed(KEY_F9)) prova.mostraFps = !prova.mostraFps;
    prova.Aggiorna(dt, Comandi::DaPeriferiche());
    if (prova.musica != gAudio.BranoAttuale()) gAudio.ScegliBrano(prova.musica);
    BeginTextureMode(rtGioco);
    ClearBackground(BLACK);
    prova.Disegna();
    DrawRectangle(0, VH - 13, VW, 13, Hex(0x0b0e22, 190));
    Scritta("PROVA   Esc o F5: torna all'editor   R: ricomincia   F1: collisioni", 6, VH - 11, 10, Hex(0xc8d0f0));
    EndTextureMode();
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float s = std::min(sw / VW, sh / VH);
    Rectangle area = { (sw - VW * s) / 2, (sh - VH * s) / 2, VW * s, VH * s };
    ClearBackground(BLACK);
    DrawTexturePro(rtGioco.texture, { 0, 0, (float)VW, -(float)VH }, area, { 0, 0 }, 0, WHITE);
    postoAudio = { area.x + area.width - 44, area.y + 6, 36, 36 };
}

// --- Immagini importate ------------------------------------------------------------

namespace {
bool EImmagine(const std::string& f) {
    std::string e = GetFileExtension(f.c_str()) ? GetFileExtension(f.c_str()) : "";
    for (auto& c : e) c = (char)std::tolower((unsigned char)c);
    return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".tga" || e == ".gif" || e == ".qoi";
}
std::string LeggiBinario(const std::string& f) {
    int n = 0;
    unsigned char* b = LoadFileData(f.c_str(), &n);
    if (!b) return "";
    std::string s((const char*)b, (size_t)n);
    UnloadFileData(b);
    return s;
}
}

void EditorMondo::CaricaImmagini() {
    std::string cartella = CartellaEseguibile() + "/immagini";
    if (!DirectoryExists(cartella.c_str())) return;
    FilePathList l = LoadDirectoryFiles(cartella.c_str());
    for (unsigned int i = 0; i < l.count; i++)
        if (EImmagine(l.paths[i])) AggiungiFileImmagine(GetFileName(l.paths[i]), LeggiBinario(l.paths[i]));
    UnloadDirectoryFiles(l);
}

// Un file trascinato sulla finestra: diventa uno strato (se si stanno guardando gli strati)
// oppure una decorazione al centro della vista. Si copia nella cartella "immagini".
void EditorMondo::ImportaFile(const std::string& file) {
    if (!EImmagine(file)) { Avvisa("Si possono importare solo immagini (PNG, JPG, BMP, GIF...)"); return; }
    std::string dati = LeggiBinario(file);
    if (dati.empty()) { Avvisa("Non riesco a leggere " + file); return; }
    std::string cartella = CartellaEseguibile() + "/immagini";
    MakeDirectory(cartella.c_str());
    std::string nome = GetFileName(file.c_str());
    SaveFileData((cartella + "/" + nome).c_str(), (void*)dati.data(), (int)dati.size());
    AggiungiFileImmagine(nome, dati);
    Texture2D t = Immagine(nome);
    if (t.id == 0) { Avvisa("L'immagine " + nome + " non si apre"); return; }
    Ricorda();
    if (pannello == SchedaDestra::Strati) {
        StratoSfondo s;
        s.nome = GetFileNameWithoutExt(nome.c_str());
        s.tipo = "immagine";
        s.immagine = nome;
        s.altezza = 1;
        s.parallasseX = 0.3f; s.parallasseY = 0.1f;
        s.scala = std::max(0.25f, (float)VH / t.height);
        M().strati.push_back(s);
        stratoScelto = (int)M().strati.size() - 1;
        Avvisa("Nuovo strato dallo sfondo " + nome);
    } else {
        Decorazione d;
        d.immagine = nome;
        Vector2 c = MondoDaSchermo({ rVista.x + rVista.width / 2, rVista.y + rVista.height / 2 });
        d.x = std::round(c.x - t.width / 2.0f); d.y = std::round(c.y - t.height / 2.0f);
        M().decorazioni.push_back(d);
        decorazioneScelta = (int)M().decorazioni.size() - 1;
        selezionato = -1;
        strumento = Strumento::Seleziona;
        pannello = SchedaDestra::Proprieta;
        Avvisa("Decorazione " + nome + " aggiunta: trascinala con Seleziona");
    }
    Ricostruisci();
}

// --- Fotogramma -----------------------------------------------------------------------

void EditorMondo::Layout(Rectangle& barra, Rectangle& schede, Rectangle& sinistra, Rectangle& centro, Rectangle& destra, Rectangle& basso) const {
    float s = Ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    float hb = std::round(28 * s), hs = std::round(30 * s), hbasso = std::round(58 * s);
    float ws = std::round(196 * s), wd = std::round(std::min(410 * s, W * 0.34f));
    barra = { 0, 0, W, hb };
    schede = { ws, hb, W - ws - wd, hs };
    sinistra = { 0, hb, ws, H - hb - hbasso };
    destra = { W - wd, hb, wd, H - hb - hbasso };
    centro = { ws, hb + hs, W - ws - wd, H - hb - hs - hbasso };
    basso = { 0, H - hbasso, W, hbasso };
}

void EditorMondo::Frame(float dt) {
    Ui::Inizia(GetScreenHeight() / 900.0f);
    tempo += dt;
    if (tMessaggio > 0) tMessaggio -= dt;
    sfondo.Aggiorna(dt);
    ControllaChat();
    if (chatInAttesa) tAttesa += dt;
    if (provando) { FrameProva(dt); return; }

    if (IsFileDropped()) {
        FilePathList l = LoadDroppedFiles();
        for (unsigned int i = 0; i < l.count; i++) ImportaFile(l.paths[i]);
        UnloadDroppedFiles(l);
    }

    Rectangle barra, schede, sinistra, centro, destra, basso;
    Layout(barra, schede, sinistra, centro, destra, basso);
    rVista = centro;
    // Un menu aperto o una finestra coprono tutto: sotto non arrivano i clic.
    if (menuAperto >= 0 || finestra != 0) Ui::ImpostaModale({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
    if (finestra == 0 && menuAperto < 0 && !Ui::StaScrivendo()) Scorciatoie();
    if (provando) { FrameProva(0); return; }   // una scorciatoia ha avviato la prova

    ClearBackground(Ui::Fondo());
    VistaMappa(centro, dt);
    Schede(schede);
    Tavolozza(sinistra);
    PannelloDestro(destra);
    Strumenti(basso);
    Ui::ApriModale();
    BarraMenu(barra);
    MenuTendina(barra);
    Ui::ChiudiModale();
    Finestre();
}

// --- Barra dei menu -----------------------------------------------------------------

void EditorMondo::BarraMenu(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x0a0c1e));
    DrawLineEx({ r.x, r.y + r.height }, { r.x + r.width, r.y + r.height }, 1, Ui::Bordo());
    float x = r.x + 8 * s;
    for (int i = 0; i < NUM_MENU; i++) {
        float w = Ui::Larghezza(MENU[i], Ui::TP()) + 22 * s;
        Rectangle v = { x, r.y + 2 * s, w, r.height - 4 * s };
        bool sopra = Ui::MouseSu(v);
        if (menuAperto == i || sopra) DrawRectangleRounded(v, 0.3f, 6, menuAperto == i ? Ui::Selezione() : Hex(0x1c2150));
        Ui::TestoCentrato(MENU[i], v, menuAperto == i ? Ui::Accento() : Ui::TestoChiaro(), Ui::TP());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { menuAperto = menuAperto == i ? -1 : i; gAudio.Suona(Effetto::Menu, 0.3f); }
        else if (sopra && menuAperto >= 0) menuAperto = i;   // scorrendo sulla barra si passa da un menu all'altro
        x += w + 2 * s;
    }
    // Titolo del mondo e stato.
    std::string t = mondo.titolo + (modificato ? "  *" : "") + "   -   " + NomeGenere(mondo.genere);
    Ui::Testo(t, x + 24 * s, r.y + (r.height - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    std::string chiave = cfg.chiave.empty() ? "Claude: manca la chiave" : chatInAttesa ? "Claude sta pensando..." : "Claude pronto";
    Color cc = cfg.chiave.empty() ? Hex(0xff8a6a) : chatInAttesa ? Ui::Accento() : Hex(0x7ee0a0);
    float wc = (float)Ui::Larghezza(chiave, Ui::TP());
    DrawCircle((int)(r.x + r.width - wc - 70 * s), (int)(r.y + r.height / 2), 4 * s, cc);
    Ui::Testo(chiave, r.x + r.width - wc - 60 * s, r.y + (r.height - Ui::TP()) / 2, cc, Ui::TP());
    postoAudio = { r.x + r.width - 30 * s, r.y + 2 * s, r.height - 4 * s, r.height - 4 * s };
}

void EditorMondo::MenuTendina(Rectangle barra) {
    if (menuAperto < 0) return;
    float s = Ui::S();
    float x = barra.x + 8 * s;
    for (int i = 0; i < menuAperto; i++) x += Ui::Larghezza(MENU[i], Ui::TP()) + 24 * s;
    const auto& voci = VociMenu()[menuAperto];
    float h = 30 * s, w = 300 * s;
    Rectangle r = { x, barra.y + barra.height, w, h * voci.size() + 8 * s };
    DrawRectangleRec({ r.x + 4 * s, r.y + 4 * s, r.width, r.height }, Hex(0x000000, 120));
    DrawRectangleRec(r, Ui::Pannello());
    DrawRectangleLinesEx(r, 1, Ui::Bordo());
    for (size_t k = 0; k < voci.size(); k++) {
        Rectangle v = { r.x + 4 * s, r.y + 4 * s + k * h, r.width - 8 * s, h };
        bool sopra = Ui::MouseSu(v);
        if (sopra) DrawRectangleRec(v, Ui::Selezione());
        // Le voci "interruttore" mostrano lo stato.
        std::string testo = voci[k].testo;
        if (menuAperto == 4 && k < 3) testo = std::string((k == 0 ? griglia : k == 1 ? anteprimaSfondo : inquadratura) ? "[x] " : "[  ] ") + testo;
        Ui::Testo(testo, v.x + 10 * s, v.y + (h - Ui::TP()) / 2, sopra ? WHITE : Ui::TestoChiaro(), Ui::TP());
        Ui::Testo(voci[k].tasto, v.x + v.width - Ui::Larghezza(voci[k].tasto, Ui::TP()) - 10 * s, v.y + (h - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
        if (sopra && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            int m = menuAperto;
            menuAperto = -1;
            gAudio.Suona(Effetto::Menu, 0.35f);
            Esegui(m, (int)k);
            return;
        }
    }
    bool suBarra = CheckCollisionPointRec(GetMousePosition(), barra);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !suBarra && !CheckCollisionPointRec(GetMousePosition(), r)) menuAperto = -1;
    if (IsKeyPressed(KEY_ESCAPE)) menuAperto = -1;
}

void EditorMondo::Esegui(int menu, int voce) {
    switch (menu * 10 + voce) {
        case 0: finestra = 1; break;
        case 1: ApriElenco(); scorriElenco = 0; break;
        case 2: Salva(false); break;
        case 3: Salva(true); break;
        case 4: CreaEseguibile(); break;
        case 5: vuoleClassico = true; vuoleUscire = true; break;
        case 6: vuoleUscire = true; break;
        case 10: Annulla(); break;
        case 11: Ripeti(); break;
        case 12:
            if (selezionato >= 0) { appunti = M().oggetti[selezionato]; appuntiPieni = true; Avvisa("Copiato: " + appunti.tipo); }
            else Avvisa("Seleziona prima un oggetto");
            break;
        case 13:
            if (appuntiPieni) {
                Ricorda();
                OggettoMappa o = appunti;
                Vector2 w = MondoDaSchermo(GetMousePosition());
                if (CheckCollisionPointRec(GetMousePosition(), rVista)) { o.x = std::round(w.x); o.y = std::round(w.y); }
                else { o.x += TILE; }
                M().oggetti.push_back(o);
                selezionato = (int)M().oggetti.size() - 1;
            }
            break;
        case 14:
            if (selezionato >= 0) { Ricorda(); M().oggetti.erase(M().oggetti.begin() + selezionato); selezionato = -1; }
            else if (decorazioneScelta >= 0 && decorazioneScelta < (int)M().decorazioni.size()) {
                Ricorda(); M().decorazioni.erase(M().decorazioni.begin() + decorazioneScelta); decorazioneScelta = -1;
            }
            break;
        case 20: pannello = SchedaDestra::MondoTab; break;
        case 21: pannello = SchedaDestra::Strati; break;
        case 22: pannello = SchedaDestra::Claude; break;
        case 30: case 31: {
            Ricorda();
            Mappa n;
            Vista v = voce == 1 ? Vista::Alto : M().vista;
            if (v == Vista::Alto) n.Nuova(60, 40, v); else n.Nuova(80, 17, v);
            n.nome = "Mappa " + std::to_string(mondo.mappe.size() + 1);
            n.tema = M().tema; n.musica = M().musica;
            mondo.mappe.push_back(n);
            CambiaMappa((int)mondo.mappe.size() - 1);
            Avvisa("Nuova mappa: collegala con una porta (oggetto Porta, poi Proprieta')");
            break;
        }
        case 32: {
            Ricorda();
            Mappa n = M();
            n.nome += " (copia)";
            mondo.mappe.push_back(n);
            CambiaMappa((int)mondo.mappe.size() - 1);
            break;
        }
        case 33: nomeMappaBozza = M().nome; finestra = 4; Ui::TogliFuoco(); break;
        case 34: {
            if (mondo.mappe.size() < 2) { Avvisa("Un mondo ha almeno una mappa"); break; }
            Ricorda();
            int via = mappa;
            mondo.mappe.erase(mondo.mappe.begin() + via);
            // Le porte verso la mappa tolta diventano uscite; gli indici dopo scalano.
            for (auto& m : mondo.mappe)
                for (auto& o : m.oggetti) {
                    if (o.mappa == via) o.mappa = -1;
                    else if (o.mappa > via) o.mappa--;
                }
            if (mondo.mappaIniziale == via) mondo.mappaIniziale = 0;
            else if (mondo.mappaIniziale > via) mondo.mappaIniziale--;
            CambiaMappa(std::min(via, (int)mondo.mappe.size() - 1));
            Avvisa("Mappa eliminata (Ctrl+Z per riaverla)");
            break;
        }
        case 35: selezionato = -1; decorazioneScelta = -1; pannello = SchedaDestra::Proprieta; break;
        case 40: griglia = !griglia; break;
        case 41: anteprimaSfondo = !anteprimaSfondo; break;
        case 42: inquadratura = !inquadratura; break;
        case 43: zoom = std::min(8.0f, zoom * 1.25f); break;
        case 44: zoom = std::max(0.25f, zoom / 1.25f); break;
        case 45: Inquadra(); break;
        case 50: AvviaProva(false); break;
        case 51: AvviaProva(true); break;
        case 60: finestra = 3; break;
    }
}

// --- Schede delle mappe ------------------------------------------------------------------

void EditorMondo::Schede(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x0f1230));
    float x = r.x + 6 * s;
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    for (int i = 0; i < (int)mondo.mappe.size(); i++) {
        const Mappa& m = mondo.mappe[i];
        std::string t = m.nome + (m.vista == Vista::Alto ? "  (alto)" : "");
        if (i == mondo.mappaIniziale) t = "> " + t;
        float w = Ui::Larghezza(t, Ui::TP()) + 24 * s;
        Rectangle v = { x, r.y + 4 * s, w, r.height - 4 * s };
        bool attiva = i == mappa;
        bool sopra = Ui::MouseSu(v);
        DrawRectangleRounded({ v.x, v.y, v.width, v.height + 8 * s }, 0.25f, 6, attiva ? Ui::Pannello() : sopra ? Hex(0x1a1f48) : Hex(0x12153a));
        if (attiva) DrawRectangle((int)v.x + 4, (int)v.y, (int)v.width - 8, (int)(2 * s), Ui::Accento());
        Ui::TestoCentrato(t, v, attiva ? WHITE : Ui::TestoSpento(), Ui::TP());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !attiva) { CambiaMappa(i); gAudio.Suona(Effetto::Menu, 0.3f); }
        else if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { CambiaMappa(i); Esegui(3, 3); }
        x += w + 4 * s;
    }
    Rectangle piu = { x, r.y + 5 * s, r.height - 8 * s, r.height - 8 * s };
    if (Ui::Pulsante(piu, "+")) Esegui(3, 0);
    EndScissorMode();
}

// --- Tavolozza --------------------------------------------------------------------------

void EditorMondo::IconaCasella(char c, Rectangle r) const {
    float u = r.width / 16;
    switch (c) {
        case '#':
            DrawRectangleRec(r, Hex(0x7a4a2a));
            DrawRectangleRec({ r.x, r.y, r.width, 4 * u }, Hex(0x5ac85a));
            DrawRectangleRec({ r.x, r.y + 4 * u, r.width, u }, Hex(0x3a8a3a));
            DrawRectangleRec({ r.x + 4 * u, r.y + 9 * u, 2 * u, 2 * u }, Hex(0x5a3420));
            DrawRectangleRec({ r.x + 10 * u, r.y + 12 * u, 2 * u, 2 * u }, Hex(0x5a3420));
            break;
        case '=':
            DrawRectangleRec(r, Hex(0x8a90ac));
            DrawRectangleLinesEx(r, u, Hex(0x5a607c));
            DrawLineEx({ r.x, r.y + 8 * u }, { r.x + r.width, r.y + 8 * u }, u, Hex(0x5a607c));
            DrawLineEx({ r.x + 8 * u, r.y }, { r.x + 8 * u, r.y + 8 * u }, u, Hex(0x5a607c));
            break;
        case '-':
            DrawRectangleRec({ r.x, r.y, r.width, 5 * u }, Hex(0xc8904a));
            DrawRectangleRec({ r.x, r.y + 4 * u, r.width, u }, Hex(0x8a5a2a));
            DrawRectangleRec({ r.x + 2 * u, r.y + 5 * u, 2 * u, 5 * u }, Hex(0x8a5a2a));
            DrawRectangleRec({ r.x + 12 * u, r.y + 5 * u, 2 * u, 5 * u }, Hex(0x8a5a2a));
            break;
        case '^':
            for (int i = 0; i < 4; i++)
                DrawTriangle({ r.x + i * 4 * u, r.y + r.height }, { r.x + i * 4 * u + 4 * u, r.y + r.height }, { r.x + i * 4 * u + 2 * u, r.y + 6 * u }, Hex(0xd8dde8));
            break;
        case '.':
            DrawCircleLinesV({ r.x + r.width / 2, r.y + r.height / 2 }, 6 * u, Hex(0xffd23f));
            DrawCircleV({ r.x + r.width / 2, r.y + r.height / 2 }, 2 * u, Hex(0xffd23f));
            break;
        default:
            DrawRectangleLinesEx(r, u, Hex(0x5a64b0));
            DrawLineEx({ r.x + 3 * u, r.y + 3 * u }, { r.x + 13 * u, r.y + 13 * u }, u * 1.5f, Hex(0xff6a6a));
            break;
    }
}

void EditorMondo::IconaOggetto(const std::string& tipo, Rectangle r) const {
    Camera2D cam{};
    cam.offset = { r.x + r.width / 2, r.y + r.height / 2 };
    if (tipo == "partenza") {
        Eroe e;
        e.aspetto = mondo.eroe;
        e.pos = { 0, 0 };
        cam.target = { 0, -Eroe::ALTEZZA / 2.0f };
        cam.zoom = r.height / 30;
        BeginMode2D(cam); e.Disegna(tempo); EndMode2D();
        return;
    }
    Oggetto o{};
    if (!TipoDaNome(tipo, o.tipo)) return;
    o.pos = PosizioneInCasella(tipo, 0, 0, Vista::Laterale);
    o.origine = o.pos;
    o.t = tempo;
    o.dir = -1;
    o.numero = tipo == "boss" ? 9 : 1;
    Rectangle b = BoxOggetto(o);
    cam.target = { b.x + b.width / 2, b.y + b.height / 2 };
    cam.zoom = std::min(r.width * 0.7f / std::max(8.0f, b.width), r.height * 0.7f / std::max(8.0f, b.height));
    cam.zoom = std::min(cam.zoom, r.height / 18);
    BeginMode2D(cam);
    DisegnaOggetto(o, 0, 0, tempo, false);
    EndMode2D();
}

void EditorMondo::Tavolozza(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Ui::Pannello());
    DrawLineEx({ r.x + r.width, r.y }, { r.x + r.width, r.y + r.height }, 1, Ui::Bordo());
    float pad = 8 * s, y = r.y + pad;
    int col = 3;
    float lato = (r.width - pad * (col + 1)) / col;
    std::string sopraNome;
    auto Cella = [&](int i, float y0, const std::string& valore, bool casella, const std::string& nome) {
        Rectangle c = { r.x + pad + (i % col) * (lato + pad), y0 + (i / col) * (lato + pad), lato, lato };
        bool scelto = pennello == valore;
        bool sopra = Ui::MouseSu(c);
        DrawRectangleRounded(c, 0.15f, 6, scelto ? Ui::Selezione() : sopra ? Hex(0x252b5c) : Hex(0x10132c));
        DrawRectangleRoundedLinesEx(c, 0.15f, 6, std::max(1.0f, s * (scelto ? 2 : 1)), scelto ? Ui::Accento() : sopra ? Hex(0x5a64b0) : Ui::Bordo());
        Rectangle dentro = { c.x + lato * 0.18f, c.y + lato * 0.14f, lato * 0.64f, lato * 0.64f };
        BeginScissorMode((int)c.x + 1, (int)c.y + 1, (int)c.width - 2, (int)c.height - 2);
        if (casella) IconaCasella(valore[0], dentro); else IconaOggetto(valore, dentro);
        EndScissorMode();
        if (sopra) sopraNome = nome;
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            pennello = valore;
            if (strumento == Strumento::Gomma || strumento == Strumento::Seleziona || strumento == Strumento::Contagocce) strumento = Strumento::Pennello;
            if (!casella && strumento == Strumento::Riempi) strumento = Strumento::Pennello;
            gAudio.Suona(Effetto::Menu, 0.3f);
        }
        return c.y + lato;
    };
    Ui::Testo("CASELLE", r.x + pad, y, Ui::Accento(), Ui::TP());
    y += Ui::TP() + 6 * s;
    float fine = y;
    for (int i = 0; i < 6; i++) fine = Cella(i, y, std::string(1, CASELLE[i]), true, std::string(NOMI_CASELLE[i]) + "  (tasto " + std::to_string(i + 1) + ")");
    y = fine + pad * 1.5f;
    Ui::Testo("OGGETTI", r.x + pad, y, Ui::Accento(), Ui::TP());
    y += Ui::TP() + 6 * s;
    const auto& tipi = TipiOggetto();
    for (int i = 0; i < (int)tipi.size(); i++) fine = Cella(i, y, tipi[i], false, Maiuscola(tipi[i]));
    fine = Cella((int)tipi.size(), y, "partenza", false, "Partenza dell'eroe");
    y = fine + pad;
    // Nome di cio' che c'e' sotto il mouse, oppure del pennello attuale.
    std::string info = sopraNome.empty() ? "Pennello: " + (pennello.size() == 1 ? std::string(NOMI_CASELLE[std::string(CASELLE).find(pennello[0])])
                                                                                    : Maiuscola(pennello)) : sopraNome;
    Ui::TestoACapo(info, r.x + pad, y, r.width - 2 * pad, sopraNome.empty() ? Ui::TestoSpento() : WHITE, Ui::TP());
    y += Ui::TP() * 2.8f;
    if (y < r.y + r.height - 70 * s)
        Ui::TestoACapo("Trascina qui un'immagine PNG o JPG per usarla come decorazione o strato dello sfondo.",
                       r.x + pad, r.y + r.height - 66 * s, r.width - 2 * pad, Ui::TestoSpento(), (int)(12 * s));
}

// --- La mappa -----------------------------------------------------------------------------

void EditorMondo::VistaMappa(Rectangle r, float dt) {
    Mappa& m = M();
    bool alto = m.vista == Vista::Alto;
    float s = Ui::S();
    Vector2 mouse = GetMousePosition();
    bool sopra = Ui::MouseSu(r);
    float W = m.larghezza * (float)TILE, H = m.altezza * (float)TILE;

    // --- Movimento della vista: rotella, Ctrl+rotella (zoom), tasto centrale o Spazio+trascina, frecce.
    if (sopra) {
        float ruota = GetMouseWheelMove();
        if (ruota != 0) {
            if (Ctrl()) {
                Vector2 prima = MondoDaSchermo(mouse);
                zoom = Limita(zoom * (ruota > 0 ? 1.2f : 1 / 1.2f), 0.25f, 8);
                camX = prima.x - (mouse.x - r.x) / zoom;
                camY = prima.y - (mouse.y - r.y) / zoom;
            } else if (Maiusc() || (!alto && H <= r.height / zoom)) camX -= ruota * 64 / zoom;
            else camY -= ruota * 64 / zoom;
        }
    }
    bool pan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && !Ui::StaScrivendo() && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    if (pan && !trascinando && sopra) { trascinando = true; trascinaDa = mouse; camDa = { camX, camY }; }
    if (trascinando) {
        if (!pan) trascinando = false;
        else { camX = camDa.x - (mouse.x - trascinaDa.x) / zoom; camY = camDa.y - (mouse.y - trascinaDa.y) / zoom; }
    }
    if (!Ui::StaScrivendo() && finestra == 0 && menuAperto < 0 && !Ctrl()) {
        float v = 420 * dt / std::sqrt(zoom) * (Maiusc() ? 2.5f : 1);
        if (IsKeyDown(KEY_LEFT)) camX -= v;
        if (IsKeyDown(KEY_RIGHT)) camX += v;
        if (IsKeyDown(KEY_UP)) camY -= v;
        if (IsKeyDown(KEY_DOWN)) camY += v;
    }
    float vw = r.width / zoom, vh = r.height / zoom;
    camX = Limita(camX, -vw * 0.75f, std::max(-vw * 0.75f, W - vw * 0.25f));
    camY = Limita(camY, -vh * 0.75f, std::max(-vh * 0.75f, H - vh * 0.25f));

    Vector2 w = MondoDaSchermo(mouse);
    int cx = (int)std::floor(w.x / TILE), cy = (int)std::floor(w.y / TILE);
    bool dentro = cx >= 0 && cy >= 0 && cx < m.larghezza && cy < m.altezza;

    // --- Strumenti.
    if (!trascinando && !IsKeyDown(KEY_SPACE)) {
        bool premuto = sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool giu = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        if (sceltaArrivo) {
            if (premuto && portaMappa >= 0 && portaMappa < (int)mondo.mappe.size() && portaIndice < (int)mondo.mappe[portaMappa].oggetti.size()) {
                Ricorda();
                OggettoMappa& porta = mondo.mappe[portaMappa].oggetti[portaIndice];
                Vector2 p = PosizioneInCasella("porta", std::max(0, cx), std::max(0, cy), m.vista);
                porta.mappa = mappa; porta.destX = p.x; porta.destY = p.y;
                sceltaArrivo = false;
                Avvisa("Arrivo della porta impostato in " + m.nome + " (" + std::to_string(cx) + ", " + std::to_string(cy) + ")");
                int torna = portaMappa, quale = portaIndice;
                CambiaMappa(torna);
                selezionato = quale;
                pannello = SchedaDestra::Proprieta;
            }
        } else if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Ricorda(); tratto = true;   // il tasto destro cancella sempre
        } else if (tratto && IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !giu) {
            Cancella(cx, cy, w);
        } else {
            switch (strumento) {
                case Strumento::Pennello:
                    if (premuto) {
                        Ricorda(); tratto = true;
                        Dipingi(cx, cy);
                    } else if (tratto && giu && pennello.size() == 1) Dipingi(cx, cy);
                    break;
                case Strumento::Gomma:
                    if (premuto) { Ricorda(); tratto = true; }
                    if (tratto && giu) Cancella(cx, cy, w);
                    break;
                case Strumento::Riempi:
                    if (premuto && dentro) { Ricorda(); Riempi(cx, cy, pennello.size() == 1 ? pennello[0] : '#'); }
                    break;
                case Strumento::Rettangolo:
                    if (premuto) { rettangolo = true; rettX = cx; rettY = cy; }
                    if (rettangolo && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                        rettangolo = false;
                        Ricorda();
                        int x0 = std::min(rettX, cx), x1 = std::max(rettX, cx), y0 = std::min(rettY, cy), y1 = std::max(rettY, cy);
                        int n = 0;
                        for (int y = y0; y <= y1; y++)
                            for (int x = x0; x <= x1; x++) {
                                bool bordo = x == x0 || x == x1 || y == y0 || y == y1;
                                if (Maiusc() && !bordo) continue;   // Maiusc: solo il contorno
                                if (pennello.size() == 1) { if (x >= 0 && y >= 0 && x < m.larghezza && y < m.altezza) m.ImpostaCasella(x, y, pennello[0]); }
                                else if (pennello != "partenza" && n++ < 300) Dipingi(x, y);
                            }
                        Ricostruisci();
                    }
                    break;
                case Strumento::Seleziona:
                    if (premuto) {
                        int i = OggettoSotto(w);
                        selezionato = i;
                        decorazioneScelta = -1;
                        if (i >= 0) {
                            primaDiSpostare = mondo; spostato = false; spostando = true;
                            presa = { w.x - m.oggetti[i].x, w.y - m.oggetti[i].y };
                            pannello = SchedaDestra::Proprieta;
                        } else {
                            for (int k = (int)m.decorazioni.size() - 1; k >= 0; k--) {
                                Texture2D t = Immagine(m.decorazioni[k].immagine);
                                const Decorazione& d = m.decorazioni[k];
                                float dw = (t.id ? t.width : 16) * d.scala, dh = (t.id ? t.height : 16) * d.scala;
                                if (CheckCollisionPointRec(w, { d.x, d.y, dw, dh })) {
                                    decorazioneScelta = k;
                                    primaDiSpostare = mondo; spostato = false; spostando = true;
                                    presa = { w.x - d.x, w.y - d.y };
                                    pannello = SchedaDestra::Proprieta;
                                    break;
                                }
                            }
                        }
                    }
                    if (spostando && giu) {
                        float nx = w.x - presa.x, ny = w.y - presa.y;
                        if (!(IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) { nx = std::round(nx / 4) * 4; ny = std::round(ny / 4) * 4; }
                        if (selezionato >= 0) {
                            auto& o = m.oggetti[selezionato];
                            if (o.x != nx || o.y != ny) { o.x = nx; o.y = ny; spostato = true; }
                        } else if (decorazioneScelta >= 0) {
                            auto& d = m.decorazioni[decorazioneScelta];
                            if (d.x != nx || d.y != ny) { d.x = nx; d.y = ny; spostato = true; }
                        }
                    }
                    if (spostando && !giu) {
                        spostando = false;
                        if (spostato) { annulla.push_back(primaDiSpostare); ripeti.clear(); modificato = true; }
                    }
                    break;
                case Strumento::Contagocce:
                    if (premuto) {
                        int i = OggettoSotto(w);
                        if (i >= 0) pennello = m.oggetti[i].tipo;
                        else if (dentro) pennello = std::string(1, m.Casella(cx, cy));
                        strumento = Strumento::Pennello;
                        Avvisa("Pennello: " + (pennello.size() == 1 ? std::string(NOMI_CASELLE[std::string(CASELLE).find(pennello[0])]) : pennello));
                    }
                    break;
                default: break;
            }
        }
        if (!giu && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            if (tratto && (strumento == Strumento::Pennello || strumento == Strumento::Gomma || true)) Ricostruisci();
            tratto = false;
        }
    }

    // --- Disegno.
    DrawRectangleRec(r, alto ? Hex(0x101418) : Hex(0x1a2040));
    float notte = sfondo.Notte();
    if (anteprimaSfondo && !alto) {
        float dy = std::max(0.0f, H - VH - camY);
        BeginTextureMode(rtSfondo);
        ClearBackground(BLACK);
        sfondo.Disegna(camX, dy);
        EndTextureMode();
        // Lo sfondo copre tutta la vista mantenendo le proporzioni.
        float k = std::max(r.width / VW, r.height / VH);
        float sw = r.width / k, sh = r.height / k;
        DrawTexturePro(rtSfondo.texture, { (VW - sw) / 2, -(VH - (VH - sh) / 2), sw, -sh }, r, { 0, 0 }, 0, WHITE);
    } else if (!alto) {
        // Scacchiera: si vede bene dove non c'e' niente.
        BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
        float q = 32 * s;
        for (float y = r.y; y < r.y + r.height; y += q)
            for (float x = r.x; x < r.x + r.width; x += q)
                if (((int)((x - r.x) / q) + (int)((y - r.y) / q)) % 2) DrawRectangleRec({ x, y, q, q }, Hex(0x20264c));
        EndScissorMode();
    }

    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    Camera2D cam{};
    cam.offset = { r.x, r.y };
    cam.target = { 0, 0 };
    cam.zoom = zoom;
    BeginMode2D(cam);
    if (alto) vista.DisegnaPavimento(camX, camY, m.tema, vw, vh, false);
    // Decorazioni di sfondo, mappa, oggetti, decorazioni in primo piano.
    auto Decorazioni = [&](bool primo) {
        for (int k = 0; k < (int)m.decorazioni.size(); k++) {
            const Decorazione& d = m.decorazioni[k];
            if (d.primoPiano != primo) continue;
            Texture2D t = Immagine(d.immagine);
            if (t.id == 0) { DrawRectangleLinesEx({ d.x - camX, d.y - camY, 16, 16 }, 1, RED); continue; }
            DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, { d.x - camX, d.y - camY, t.width * d.scala, t.height * d.scala }, { 0, 0 }, 0, WHITE);
        }
    };
    Decorazioni(false);
    vista.Disegna(camX, camY, tempo, alto ? 0.0f : notte * 0.6f, alto, vw, vh);
    for (const auto& o : m.oggetti) {
        Oggetto og = DaMappa(o, tempo);
        if (og.pos.x < camX - 64 || og.pos.x > camX + vw + 64 || og.pos.y < camY - 64 || og.pos.y > camY + vh + 64) continue;
        DisegnaOggetto(og, camX, camY, tempo, false);
    }
    Decorazioni(true);
    EndMode2D();

    // Sopra, in coordinate della mappa: griglia, bordi, partenza, selezione.
    Camera2D camM{};
    camM.offset = { r.x, r.y };
    camM.target = { camX, camY };
    camM.zoom = zoom;
    BeginMode2D(camM);
    float linea = 1 / zoom;
    if (griglia && zoom >= 0.75f) {
        int x0 = std::max(0, (int)(camX / TILE)), x1 = std::min(m.larghezza, (int)((camX + vw) / TILE) + 1);
        int y0 = std::max(0, (int)(camY / TILE)), y1 = std::min(m.altezza, (int)((camY + vh) / TILE) + 1);
        Color cg = Hex(0xffffff, 28);
        for (int x = x0; x <= x1; x++) DrawLineEx({ x * (float)TILE, y0 * (float)TILE }, { x * (float)TILE, y1 * (float)TILE }, linea, x % 10 == 0 ? Hex(0xffffff, 60) : cg);
        for (int y = y0; y <= y1; y++) DrawLineEx({ x0 * (float)TILE, y * (float)TILE }, { x1 * (float)TILE, y * (float)TILE }, linea, y % 10 == 0 ? Hex(0xffffff, 60) : cg);
    }
    DrawRectangleLinesEx({ -linea * 2, -linea * 2, W + linea * 4, H + linea * 4 }, linea * 2, Hex(0xffd23f, 160));
    // Partenza dell'eroe e inquadratura del gioco.
    Eroe e;
    e.aspetto = mondo.eroe;
    e.pos = m.partenza;
    e.Disegna(tempo);
    DrawCircleLinesV({ m.partenza.x, m.partenza.y - 10 }, 14, ConAlfa(WHITE, 0.5f + 0.3f * std::sin(tempo * 4)));
    if (inquadratura) {
        float zc = std::max(0.25f, m.telecamera.zoom);
        float iw = VW / zc, ih = VH / zc;
        float ix = Limita(m.partenza.x - iw / 2, 0, std::max(0.0f, W - iw)), iy = Limita(m.partenza.y - ih * 0.6f, 0, std::max(0.0f, H - ih));
        if (m.telecamera.modo == ModoTelecamera::Stanze) { ix = std::floor(m.partenza.x / iw) * iw; iy = std::floor((m.partenza.y - 1) / ih) * ih; }
        Rectangle q = { ix, iy, iw, ih };
        for (float t = 0; t < 1; t += 0.02f) {   // tratteggio
            float a = t, b = t + 0.01f;
            DrawLineEx({ q.x + q.width * a, q.y }, { q.x + q.width * b, q.y }, linea * 1.5f, Hex(0x7ee0ff, 170));
            DrawLineEx({ q.x + q.width * a, q.y + q.height }, { q.x + q.width * b, q.y + q.height }, linea * 1.5f, Hex(0x7ee0ff, 170));
            DrawLineEx({ q.x, q.y + q.height * a }, { q.x, q.y + q.height * b }, linea * 1.5f, Hex(0x7ee0ff, 170));
            DrawLineEx({ q.x + q.width, q.y + q.height * a }, { q.x + q.width, q.y + q.height * b }, linea * 1.5f, Hex(0x7ee0ff, 170));
        }
        if (m.telecamera.modo == ModoTelecamera::Stanze) {
            // Le stanze: righe sottili su tutta la mappa.
            for (float x = iw; x < W; x += iw) DrawLineEx({ x, 0 }, { x, H }, linea, Hex(0x7ee0ff, 70));
            for (float y = ih; y < H; y += ih) DrawLineEx({ 0, y }, { W, y }, linea, Hex(0x7ee0ff, 70));
        }
    }
    // Porte: freccia verso l'arrivo se e' nella stessa mappa, etichetta altrimenti.
    for (int i = 0; i < (int)m.oggetti.size(); i++) {
        const auto& o = m.oggetti[i];
        if (o.tipo != "porta" && !(o.tipo == "bandiera" && o.mappa >= 0)) continue;
        std::string dove = o.mappa < 0 ? "uscita" : o.mappa < (int)mondo.mappe.size() ? "-> " + mondo.mappe[o.mappa].nome : "-> ?";
        if (o.mappa == mappa && o.destX >= 0) {
            DrawLineEx({ o.x, o.y - 12 }, { o.destX, o.destY - 12 }, linea * 2, Hex(0x5ef0ff, 150));
            DrawCircleV({ o.destX, o.destY - 12 }, 3, Hex(0x5ef0ff));
        }
        float ds = 10 / zoom * s;
        DrawTextEx(GetFontDefault(), dove.c_str(), { o.x - 12, o.y - 40 - ds }, ds, ds / 10, Hex(0x5ef0ff));
    }
    if (selezionato >= 0 && selezionato < (int)m.oggetti.size()) {
        Rectangle b = BoxOggetto(DaMappa(m.oggetti[selezionato], tempo));
        DrawRectangleLinesEx({ b.x - 3, b.y - 3, b.width + 6, b.height + 6 }, linea * 2, ConAlfa(Hex(0xffd23f), 0.6f + 0.4f * std::sin(tempo * 8)));
    }
    if (decorazioneScelta >= 0 && decorazioneScelta < (int)m.decorazioni.size()) {
        const Decorazione& d = m.decorazioni[decorazioneScelta];
        Texture2D t = Immagine(d.immagine);
        DrawRectangleLinesEx({ d.x - 2, d.y - 2, (t.id ? t.width : 16) * d.scala + 4, (t.id ? t.height : 16) * d.scala + 4 }, linea * 2, ConAlfa(Hex(0xff8aff), 0.6f + 0.4f * std::sin(tempo * 8)));
    }
    // Anteprima del pennello o del rettangolo.
    if (sopra && !trascinando) {
        if (strumento == Strumento::Rettangolo && rettangolo) {
            int x0 = std::min(rettX, cx), x1 = std::max(rettX, cx), y0 = std::min(rettY, cy), y1 = std::max(rettY, cy);
            DrawRectangleRec({ x0 * (float)TILE, y0 * (float)TILE, (x1 - x0 + 1) * (float)TILE, (y1 - y0 + 1) * (float)TILE }, Hex(0xffd23f, 50));
            DrawRectangleLinesEx({ x0 * (float)TILE, y0 * (float)TILE, (x1 - x0 + 1) * (float)TILE, (y1 - y0 + 1) * (float)TILE }, linea * 2, Hex(0xffd23f));
        } else if (dentro && strumento != Strumento::Seleziona) {
            Rectangle c = { cx * (float)TILE, cy * (float)TILE, (float)TILE, (float)TILE };
            if (strumento == Strumento::Pennello && pennello.size() == 1) {
                BeginBlendMode(BLEND_ALPHA);
                IconaCasella(pennello[0], c);
                DrawRectangleRec(c, Hex(0xffffff, 60));
            }
            DrawRectangleLinesEx(c, linea * 2, strumento == Strumento::Gomma ? Hex(0xff6a6a) : sceltaArrivo ? Hex(0x5ef0ff) : Hex(0xffd23f));
        }
    }
    EndMode2D();
    EndScissorMode();

    // Riquadri informativi sopra la mappa.
    std::string info = dentro ? TextFormat("casella %d, %d   -   zoom %.0f%%", cx, cy, zoom * 100) : TextFormat("zoom %.0f%%", zoom * 100);
    info = m.nome + "  " + std::to_string(m.larghezza) + "x" + std::to_string(m.altezza) + (alto ? "  vista dall'alto" : "  vista laterale") + "   " + info;
    float iw = Ui::Larghezza(info, Ui::TP()) + 16 * s;
    DrawRectangleRounded({ r.x + 8 * s, r.y + 8 * s, iw, Ui::TP() + 10 * s }, 0.4f, 6, Hex(0x0a0c1e, 200));
    Ui::Testo(info, r.x + 16 * s, r.y + 13 * s, Ui::TestoChiaro(), Ui::TP());
    if (sceltaArrivo) {
        std::string t = "Clicca dove arriva la porta (anche in un'altra mappa: cambia scheda). Esc annulla.";
        float tw = Ui::Larghezza(t, Ui::T()) + 30 * s;
        Rectangle q = { r.x + (r.width - tw) / 2, r.y + 44 * s, tw, Ui::T() + 16 * s };
        DrawRectangleRounded(q, 0.4f, 6, Hex(0x0a3a4a, 230));
        Ui::TestoCentrato(t, q, Hex(0x5ef0ff), Ui::T());
    }
    Minimappa({ r.x + r.width - 190 * s, r.y + r.height - 130 * s, 180 * s, 120 * s });
    if (tMessaggio > 0) {
        float k = Limita(tMessaggio * 2, 0, 1);
        float tw = std::min(r.width - 40 * s, (float)Ui::Larghezza(messaggio, Ui::T()) + 30 * s);
        Rectangle q = { r.x + (r.width - tw) / 2, r.y + r.height - 60 * s, tw, Ui::T() + 18 * s };
        if (q.x + q.width > r.x + r.width - 200 * s) q.x = std::max(r.x + 8 * s, r.x + r.width - 200 * s - q.width);
        DrawRectangleRounded(q, 0.4f, 6, Hex(0x101430, (unsigned char)(235 * k)));
        DrawRectangleRoundedLinesEx(q, 0.4f, 6, std::max(1.0f, s), ConAlfa(Ui::Accento(), k));
        BeginScissorMode((int)q.x, (int)q.y, (int)q.width, (int)q.height);
        Ui::TestoCentrato(messaggio, q, ConAlfa(WHITE, k), Ui::T());
        EndScissorMode();
    }
}

void EditorMondo::Minimappa(Rectangle r) {
    const Mappa& m = M();
    float s = Ui::S();
    float k = std::min((r.width - 8 * s) / m.larghezza, (r.height - 8 * s) / m.altezza);
    float w = m.larghezza * k, h = m.altezza * k;
    Rectangle q = { r.x + r.width - w - 4 * s, r.y + r.height - h - 4 * s, w, h };
    DrawRectangleRec({ q.x - 4 * s, q.y - 4 * s, w + 8 * s, h + 8 * s }, Hex(0x0a0c1e, 210));
    // Disegno per righe: tratti consecutivi dello stesso colore insieme (veloce anche per mappe grandi).
    int passo = std::max(1, (int)(1 / k));
    for (int y = 0; y < m.altezza; y += passo) {
        int x = 0;
        while (x < m.larghezza) {
            char c = m.Casella(x, y);
            int x2 = x;
            while (x2 < m.larghezza && m.Casella(x2, y) == c) x2 += passo;
            if (c != ' ') {
                Color col = c == '#' ? Hex(0x5ac85a) : c == '=' ? Hex(0x8a90ac) : c == '-' ? Hex(0xc8904a) : c == '^' ? Hex(0xff6a6a) : Hex(0xffd23f);
                DrawRectangleRec({ q.x + x * k, q.y + y * k, (x2 - x) * k, std::max(1.0f, passo * k) }, col);
            }
            x = x2;
        }
    }
    for (auto& o : m.oggetti) {
        Color col = (o.tipo == "moneta" || o.tipo == "gemma") ? Hex(0xffe066) : (o.tipo == "porta" || o.tipo == "bandiera") ? Hex(0x5ef0ff) : Hex(0xff5c8a);
        DrawRectangleRec({ q.x + o.x / TILE * k - 1, q.y + o.y / TILE * k - 1, 2, 2 }, col);
    }
    DrawRectangleRec({ q.x + m.partenza.x / TILE * k - 2, q.y + m.partenza.y / TILE * k - 3, 4, 4 }, WHITE);
    // Riquadro della vista; clic o trascinamento spostano la vista.
    float vw = rVista.width / zoom, vh = rVista.height / zoom;
    Rectangle v = { q.x + camX / TILE * k, q.y + camY / TILE * k, vw / TILE * k, vh / TILE * k };
    BeginScissorMode((int)q.x - 2, (int)q.y - 2, (int)w + 4, (int)h + 4);
    DrawRectangleLinesEx(v, std::max(1.0f, s), Hex(0xffd23f));
    EndScissorMode();
    if (Ui::MouseSu({ q.x - 4 * s, q.y - 4 * s, w + 8 * s, h + 8 * s }) && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !tratto && !spostando && !rettangolo) {
        Vector2 p = GetMousePosition();
        camX = (p.x - q.x) / k * TILE - vw / 2;
        camY = (p.y - q.y) / k * TILE - vh / 2;
    }
}

// --- Barra degli strumenti -------------------------------------------------------------------

void EditorMondo::Strumenti(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x0f1230));
    DrawLineEx({ r.x, r.y }, { r.x + r.width, r.y }, 1, Ui::Bordo());
    float pad = 8 * s, x = r.x + pad, h = r.height - 2 * pad;
    for (int i = 0; i < (int)Strumento::Totale; i++) {
        std::string t = std::string(NOMI_STRUMENTI[i]) + " (" + TASTI_STRUMENTI[i] + ")";
        float w = Ui::Larghezza(t, Ui::TP()) + 22 * s;
        if (Ui::Pulsante({ x, r.y + pad, w, h }, t, (int)strumento == i)) { strumento = (Strumento)i; sceltaArrivo = false; }
        x += w + 4 * s;
    }
    x += 14 * s;
    auto Interr = [&](const std::string& t, bool& v) {
        float w = Ui::Larghezza(t, Ui::TP()) + 22 * s;
        if (Ui::Pulsante({ x, r.y + pad, w, h }, t, v)) v = !v;
        x += w + 4 * s;
    };
    Interr("Griglia", griglia);
    Interr("Sfondo", anteprimaSfondo);
    Interr("Inquadratura", inquadratura);
    x += 14 * s;
    if (Ui::Pulsante({ x, r.y + pad, h, h }, "-")) zoom = std::max(0.25f, zoom / 1.25f);
    x += h + 2 * s;
    Ui::TestoCentrato(TextFormat("%.0f%%", zoom * 100), { x, r.y + pad, 56 * s, h }, Ui::TestoChiaro(), Ui::TP());
    x += 56 * s + 2 * s;
    if (Ui::Pulsante({ x, r.y + pad, h, h }, "+")) zoom = std::min(8.0f, zoom * 1.25f);
    // A destra: prova e crea il gioco.
    float wp = 150 * s, we = 130 * s;
    Rectangle prova = { r.x + r.width - pad - wp - we - 6 * s, r.y + pad, wp, h };
    Rectangle exe = { r.x + r.width - pad - we, r.y + pad, we, h };
    if (prova.x > x + h + 10 * s) {
        if (Ui::Pulsante(prova, "PROVA  (F5)", true)) AvviaProva(false);
        if (Ui::Pulsante(exe, "Crea .exe")) CreaEseguibile();
    }
}

// --- Pannelli a destra ---------------------------------------------------------------------

namespace {
// Impaginazione verticale di un pannello che scorre: i controlli fuori dalla vista non si disegnano
// (e quindi non ricevono clic).
struct Colonna {
    Rectangle area;
    float y, s;
    float Fine() const { return y; }
    bool Prendi(float h, Rectangle& r) {
        r = { area.x, y, area.width, h };
        y += h + 6 * s;
        return r.y >= area.y - 1 && r.y + r.height <= area.y + area.height + 1;
    }
    void Titolo(const std::string& t) {
        y += 6 * s;
        Rectangle r;
        if (Prendi(Ui::TP() + 4 * s, r)) {
            Ui::Testo(t, r.x, r.y + 2 * s, Ui::Accento(), Ui::TP());
            DrawLineEx({ r.x + Ui::Larghezza(t, Ui::TP()) + 8 * s, r.y + r.height / 2 }, { r.x + r.width, r.y + r.height / 2 }, 1, Ui::Bordo());
        }
    }
    void Nota(const std::string& t, Color c) {
        float h = Ui::TestoACapo(t, 0, 0, area.width, c, Ui::TP(), false);
        Rectangle r;
        if (Prendi(h, r)) Ui::TestoACapo(t, r.x, r.y, r.width, c, Ui::TP());
    }
};

std::vector<std::string> NomiTemi() { std::vector<std::string> v; for (auto& t : Sfondo::Temi()) v.push_back(t.nome); return v; }
std::vector<std::string> NomiBrani() { std::vector<std::string> v; for (auto b : Audio::Brani()) v.push_back(b); return v; }
std::vector<std::string> NomiModi() { std::vector<std::string> v; for (int i = 0; i < (int)ModoTelecamera::Totale; i++) v.push_back(NomeModoTelecamera((ModoTelecamera)i)); return v; }
std::vector<std::string> NomiGeneri() { std::vector<std::string> v; for (int i = 0; i < (int)Genere::Totale; i++) v.push_back(NomeGenere((Genere)i)); return v; }
const std::vector<std::string>& TipiStrato() {
    static const std::vector<std::string> v = { "montagne", "colline", "foresta", "nuvole", "stelle", "fascia", "immagine" };
    return v;
}
}

void EditorMondo::PannelloDestro(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Ui::Pannello());
    DrawLineEx({ r.x, r.y }, { r.x, r.y + r.height }, 1, Ui::Bordo());
    const char* nomi[] = { "Proprieta'", "Mondo", "Strati", "Claude" };
    float pad = 6 * s, h = 30 * s;
    float w = (r.width - pad * 5) / 4;
    for (int i = 0; i < 4; i++) {
        if (Ui::Pulsante({ r.x + pad + i * (w + pad), r.y + pad, w, h }, i == 3 ? (chatInAttesa ? "Claude..." : "Claude") : nomi[i], (int)pannello == i)) {
            pannello = (SchedaDestra)i;
            scorriPannello = 0;
        }
    }
    Rectangle corpo = { r.x + 12 * s, r.y + h + 2 * pad + 4 * s, r.width - 24 * s, r.height - h - 3 * pad - 4 * s };
    switch (pannello) {
        case SchedaDestra::Proprieta: PannelloProprieta(corpo); break;
        case SchedaDestra::MondoTab: PannelloMondo(corpo); break;
        case SchedaDestra::Strati: PannelloStrati(corpo); break;
        default: PannelloClaude(corpo); break;
    }
}

// Proprieta' dell'oggetto selezionato, della decorazione, oppure della mappa.
void EditorMondo::PannelloProprieta(Rectangle r) {
    float s = Ui::S();
    float hr = 28 * s;
    if (Ui::MouseSu(r)) scorriPannello -= GetMouseWheelMove() * 50 * s;
    scorriPannello = std::max(0.0f, scorriPannello);
    BeginScissorMode((int)r.x - 4, (int)r.y, (int)r.width + 8, (int)r.height);
    Colonna L{ r, r.y - scorriPannello, s };
    Rectangle q;
    Mappa& m = M();

    if (selezionato >= 0 && selezionato < (int)m.oggetti.size()) {
        OggettoMappa o = m.oggetti[selezionato];
        bool c = false, elimina = false, duplica = false;
        L.Titolo("OGGETTO SELEZIONATO");
        if (L.Prendi(70 * s, q)) {
            DrawRectangleRounded({ q.x, q.y, 70 * s, 70 * s }, 0.15f, 6, Hex(0x10132c));
            IconaOggetto(o.tipo, { q.x + 10 * s, q.y + 10 * s, 50 * s, 50 * s });
            Ui::Testo(Maiuscola(o.tipo), q.x + 84 * s, q.y + 12 * s, WHITE, Ui::T());
            Ui::Testo(TextFormat("casella %d, %d", (int)(o.x / TILE), (int)((o.y - 1) / TILE)), q.x + 84 * s, q.y + 40 * s, Ui::TestoSpento(), Ui::TP());
        }
        int tipo = (int)(std::find(TipiOggetto().begin(), TipiOggetto().end(), o.tipo) - TipiOggetto().begin());
        if (tipo < (int)TipiOggetto().size() && L.Prendi(hr, q)) {
            std::vector<std::string> nomi;
            for (auto& t : TipiOggetto()) nomi.push_back(Maiuscola(t));
            if (Ui::Scelta(q, "Tipo", tipo, nomi)) { o.tipo = TipiOggetto()[tipo]; c = true; }
        }
        if (L.Prendi(hr, q)) c |= Ui::Numero("ox", q, "X (pixel)", o.x, -10000, 20000, 4);
        if (L.Prendi(hr, q)) c |= Ui::Numero("oy", q, "Y (pixel)", o.y, -10000, 20000, 4);
        if (o.tipo == "cartello") {
            if (L.Prendi(hr, q)) c |= Ui::NumeroIntero("onum", q, "Testo numero", o.numero, 1, 9);
            // Il testo del cartello sta nella mappa.
            TestoCartello* t = nullptr;
            for (auto& tc : m.testi) if (tc.numero == o.numero) t = &tc;
            std::string it = t ? t->it : "", en = t ? t->en : "";
            L.Nota("Testo in italiano:", Ui::TestoSpento());
            if (L.Prendi(hr, q) && Ui::Campo("cit" + std::to_string(o.numero), q, it, 200)) {
                Ricorda();
                if (!t) { m.testi.push_back({ o.numero, it, en }); } else t->it = it;
            }
            L.Nota("Testo in inglese:", Ui::TestoSpento());
            if (L.Prendi(hr, q) && Ui::Campo("cen" + std::to_string(o.numero), q, en, 200)) {
                Ricorda();
                bool trovato = false;
                for (auto& tc : m.testi) if (tc.numero == o.numero) { tc.en = en; trovato = true; }
                if (!trovato) m.testi.push_back({ o.numero, it, en });
            }
        }
        if (o.tipo == "boss" && L.Prendi(hr, q)) c |= Ui::NumeroIntero("onum", q, "Energia", o.numero, 1, 30);
        if (o.tipo == "porta" || o.tipo == "bandiera") {
            L.Titolo(o.tipo == "porta" ? "DOVE PORTA" : "DOPO LA VITTORIA");
            std::vector<std::string> mete = { o.tipo == "porta" ? "Fine del livello" : "Fine del gioco" };
            for (auto& mm : mondo.mappe) mete.push_back(mm.nome);
            int meta = o.mappa + 1;
            if (meta >= (int)mete.size()) meta = 0;
            if (L.Prendi(hr, q) && Ui::Scelta(q, "Mappa", meta, mete)) { o.mappa = meta - 1; o.destX = o.destY = -1; c = true; }
            if (o.mappa >= 0) {
                L.Nota(o.destX < 0 ? "Arrivo: la partenza di quella mappa." : TextFormat("Arrivo: casella %d, %d", (int)(o.destX / TILE), (int)((o.destY - 1) / TILE)), Ui::TestoChiaro());
                if (L.Prendi(hr, q) && Ui::Pulsante(q, "Scegli il punto d'arrivo sulla mappa")) {
                    sceltaArrivo = true; portaMappa = mappa; portaIndice = selezionato;
                    CambiaMappa(o.mappa);
                    Avvisa("Clicca il punto d'arrivo");
                    EndScissorMode();
                    return;
                }
                if (o.destX >= 0 && L.Prendi(hr, q) && Ui::Pulsante(q, "Arrivo alla partenza della mappa")) { o.destX = o.destY = -1; c = true; }
            }
        }
        L.y += 8 * s;
        if (L.Prendi(hr, q)) {
            duplica = Ui::Pulsante({ q.x, q.y, q.width / 2 - 3 * s, q.height }, "Duplica (Ctrl+C, V)");
            elimina = Ui::Pulsante({ q.x + q.width / 2 + 3 * s, q.y, q.width / 2 - 3 * s, q.height }, "Elimina (Canc)");
        }
        if (L.Prendi(hr, q) && Ui::Pulsante(q, "Torna alle proprieta' della mappa")) selezionato = -1;
        if (c && selezionato >= 0) { Ricorda(); m.oggetti[selezionato] = o; }
        if (duplica && selezionato >= 0) { Ricorda(); OggettoMappa d = o; d.x += TILE; m.oggetti.push_back(d); selezionato = (int)m.oggetti.size() - 1; }
        if (elimina && selezionato >= 0) Esegui(1, 4);
        EndScissorMode();
        return;
    }

    if (decorazioneScelta >= 0 && decorazioneScelta < (int)m.decorazioni.size()) {
        Decorazione d = m.decorazioni[decorazioneScelta];
        bool c = false;
        L.Titolo("DECORAZIONE");
        L.Nota("Immagine: " + d.immagine, WHITE);
        if (L.Prendi(hr, q)) c |= Ui::Numero("dx", q, "X (pixel)", d.x, -10000, 20000, 4);
        if (L.Prendi(hr, q)) c |= Ui::Numero("dy", q, "Y (pixel)", d.y, -10000, 20000, 4);
        if (L.Prendi(hr, q)) c |= Ui::Numero("dsc", q, "Scala", d.scala, 0.05f, 10, 0.05f, "%.2f");
        if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Davanti all'eroe", d.primoPiano);
        bool elimina = false;
        if (L.Prendi(hr, q)) elimina = Ui::Pulsante(q, "Elimina la decorazione");
        if (L.Prendi(hr, q) && Ui::Pulsante(q, "Torna alle proprieta' della mappa")) decorazioneScelta = -1;
        if (c) { Ricorda(); m.decorazioni[decorazioneScelta] = d; }
        if (elimina) { Ricorda(); m.decorazioni.erase(m.decorazioni.begin() + decorazioneScelta); decorazioneScelta = -1; }
        EndScissorMode();
        return;
    }

    // Proprieta' della mappa: si lavora su una copia dei valori e si salva solo se qualcosa cambia.
    L.Titolo("MAPPA");
    std::string nome = m.nome;
    if (L.Prendi(hr, q) && Ui::Campo("mnome", q, nome, 40) && !nome.empty()) { Ricorda(); m.nome = nome; }
    int w = m.larghezza, h = m.altezza;
    bool dim = false;
    if (L.Prendi(hr, q)) dim |= Ui::NumeroIntero("mw", q, "Larghezza", w, 8, 1000);
    if (L.Prendi(hr, q)) dim |= Ui::NumeroIntero("mh", q, "Altezza", h, 6, 1000);
    if (dim) { Ricorda(); m.Ridimensiona(w, h); Ricostruisci(); }
    int v = (int)m.vista;
    if (L.Prendi(hr, q) && Ui::Scelta(q, "Vista", v, { "Laterale (gravita')", "Dall'alto" })) { Ricorda(); m.vista = (Vista)v; Ricostruisci(); }

    L.Titolo("ASPETTO");
    int tema = m.tema, musica = m.musica;
    float ora = m.ora;
    bool ciclo = m.cicloGiorno, gen = m.sfondoGenerato, cp = m.cieloPersonale;
    Color ca = m.cieloAlto, cb = m.cieloBasso;
    bool c = false;
    if (L.Prendi(hr, q)) c |= Ui::Scelta(q, "Tema", tema, NomiTemi());
    if (L.Prendi(hr, q)) c |= Ui::Numero("mora", q, "Ora del giorno", ora, 0, 1, 0.05f, "%.2f");
    if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Il giorno passa", ciclo);
    if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Paesaggio generato", gen);
    if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Cielo personalizzato", cp);
    if (cp) {
        if (L.Prendi(hr, q)) c |= Ui::Colore("mca", q, "Cielo in alto", ca);
        if (L.Prendi(hr, q)) c |= Ui::Colore("mcb", q, "Cielo in basso", cb);
    }
    if (L.Prendi(hr, q)) c |= Ui::Scelta(q, "Musica", musica, NomiBrani());
    if (c) {
        Ricorda();
        m.tema = tema; m.ora = ora; m.cicloGiorno = ciclo; m.sfondoGenerato = gen; m.cieloPersonale = cp;
        m.cieloAlto = ca; m.cieloBasso = cb;
        if (m.musica != musica) { m.musica = musica; gAudio.ScegliBrano(musica); }
        Ricostruisci();
    }

    L.Titolo("TELECAMERA DEL GIOCO");
    Telecamera t = m.telecamera;
    int modo = (int)t.modo;
    c = false;
    if (L.Prendi(hr, q)) c |= Ui::Scelta(q, "Modo", modo, NomiModi());
    t.modo = (ModoTelecamera)modo;
    const char* spiega[] = { "Segue l'eroe; dentro la zona morta l'eroe si muove senza far scorrere.",
                             "A stanze: lo schermo scatta quando l'eroe esce dalla stanza.",
                             "Scorre da sola (sparatutto): l'eroe deve restare nello schermo.",
                             "Ferma: tutta la scena in un solo schermo." };
    L.Nota(spiega[modo % 4], Ui::TestoSpento());
    if (t.modo == ModoTelecamera::Segui) {
        if (L.Prendi(hr, q)) c |= Ui::Numero("tzx", q, "Zona morta X", t.zonaMortaX, 0, 200, 4);
        if (L.Prendi(hr, q)) c |= Ui::Numero("tzy", q, "Zona morta Y", t.zonaMortaY, 0, 120, 4);
        if (L.Prendi(hr, q)) c |= Ui::Numero("tan", q, "Anticipo", t.anticipo, 0, 160, 4);
    }
    if (t.modo == ModoTelecamera::Automatica) {
        if (L.Prendi(hr, q)) c |= Ui::Numero("tv", q, "Velocita'", t.velocita, 5, 300, 5);
        if (L.Prendi(hr, q)) c |= Ui::NumeroIntero("tdx", q, "Direzione X", t.dirX, -1, 1);
        if (L.Prendi(hr, q)) c |= Ui::NumeroIntero("tdy", q, "Direzione Y", t.dirY, -1, 1);
    }
    if (L.Prendi(hr, q)) c |= Ui::Numero("tz", q, "Zoom", t.zoom, 0.5f, 3, 0.1f, "%.1f");
    if (c) { Ricorda(); m.telecamera = t; }

    L.Titolo("PARTENZA DELL'EROE");
    Vector2 p = m.partenza;
    c = false;
    if (L.Prendi(hr, q)) c |= Ui::Numero("px", q, "X (pixel)", p.x, 0, 20000, 8);
    if (L.Prendi(hr, q)) c |= Ui::Numero("py", q, "Y (pixel)", p.y, 0, 20000, 8);
    if (c) { Ricorda(); m.partenza = p; }
    L.Nota("Oppure scegli \"Partenza\" nella tavolozza e clicca sulla mappa.", Ui::TestoSpento());
    L.Nota(TextFormat("Oggetti: %d   Decorazioni: %d   Strati: %d", (int)m.oggetti.size(), (int)m.decorazioni.size(), (int)m.strati.size()), Ui::TestoSpento());
    EndScissorMode();
    float tot = L.Fine() + scorriPannello - r.y;
    scorriPannello = std::min(scorriPannello, std::max(0.0f, tot - r.height + 20 * s));
}

void EditorMondo::PannelloMondo(Rectangle r) {
    float s = Ui::S();
    float hr = 28 * s;
    if (Ui::MouseSu(r)) scorriPannello -= GetMouseWheelMove() * 50 * s;
    scorriPannello = std::max(0.0f, scorriPannello);
    BeginScissorMode((int)r.x - 4, (int)r.y, (int)r.width + 8, (int)r.height);
    Colonna L{ r, r.y - scorriPannello, s };
    Rectangle q;
    L.Titolo("IL GIOCO");
    std::string titolo = mondo.titolo, autori = mondo.autori;
    L.Nota("Titolo", Ui::TestoSpento());
    if (L.Prendi(hr, q) && Ui::Campo("wtit", q, titolo, 60) && !titolo.empty()) { Ricorda(); mondo.titolo = titolo; }
    L.Nota("Autori (nei crediti)", Ui::TestoSpento());
    if (L.Prendi(hr, q) && Ui::Campo("waut", q, autori, 80)) { Ricorda(); mondo.autori = autori; }
    int genere = (int)mondo.genere, gioc = mondo.giocatoriMax, iniz = mondo.mappaIniziale;
    bool diviso = mondo.schermoDiviso;
    bool c = false;
    if (L.Prendi(hr, q)) c |= Ui::Scelta(q, "Genere", genere, NomiGeneri());
    if (L.Prendi(hr, q)) c |= Ui::NumeroIntero("wgio", q, "Giocatori", gioc, 1, 4);
    if (gioc > 1 && L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Schermo diviso", diviso);
    std::vector<std::string> nomi;
    for (auto& m : mondo.mappe) nomi.push_back(m.nome);
    if (L.Prendi(hr, q)) c |= Ui::Scelta(q, "Mappa iniziale", iniz, nomi);
    if (c) { Ricorda(); mondo.genere = (Genere)genere; mondo.giocatoriMax = gioc; mondo.schermoDiviso = diviso; mondo.mappaIniziale = iniz; }

    L.Titolo("FISICA");
    ParametriFisica f = mondo.fisica;
    c = false;
    if (L.Prendi(hr, q)) c |= Ui::Numero("fg", q, "Gravita'", f.gravita, 100, 3000, 25);
    if (L.Prendi(hr, q)) c |= Ui::Numero("fs", q, "Salto", f.salto, 100, 800, 5);
    if (L.Prendi(hr, q)) c |= Ui::Numero("fv", q, "Velocita'", f.velocita, 30, 400, 5);
    if (L.Prendi(hr, q)) c |= Ui::Numero("fm", q, "Molla", f.molla, 100, 1200, 10);
    if (c) { Ricorda(); mondo.fisica = f; }
    if (L.Prendi(hr, q)) {
        float hs = f.salto * f.salto / (2 * f.gravita) / TILE;
        Ui::Testo(TextFormat("Il salto arriva a circa %.1f caselle", hs), q.x, q.y + 6 * s, Ui::TestoSpento(), Ui::TP());
    }

    L.Titolo("L'EROE");
    AspettoEroe e = mondo.eroe;
    c = false;
    if (L.Prendi(70 * s, q)) {
        DrawRectangleRounded({ q.x, q.y, 70 * s, 70 * s }, 0.15f, 6, Hex(0x10132c));
        IconaOggetto("partenza", { q.x + 5 * s, q.y + 5 * s, 60 * s, 60 * s });
    }
    if (L.Prendi(hr, q)) c |= Ui::Colore("ep", q, "Pelle", e.pelle);
    if (L.Prendi(hr, q)) c |= Ui::Colore("ec", q, "Capelli", e.capelli);
    if (L.Prendi(hr, q)) c |= Ui::Colore("em", q, "Maglia", e.maglia);
    if (L.Prendi(hr, q)) c |= Ui::Colore("emn", q, "Mantello", e.mantello);
    if (L.Prendi(hr, q)) c |= Ui::Colore("epa", q, "Pantaloni", e.pantaloni);
    if (L.Prendi(hr, q)) c |= Ui::Colore("esc", q, "Scarpe", e.scarpe);
    if (c) { Ricorda(); mondo.eroe = e; }

    L.Titolo("CONTROLLO");
    std::string problema = mondo.Controlla();
    L.Nota(problema.empty() ? "Tutto in ordine: il gioco si puo' creare." : "Problema: " + problema, problema.empty() ? Hex(0x7ee0a0) : Hex(0xff8a6a));
    L.Nota(TextFormat("%d mappe nel mondo.", (int)mondo.mappe.size()), Ui::TestoSpento());
    EndScissorMode();
    float tot = L.Fine() + scorriPannello - r.y;
    scorriPannello = std::min(scorriPannello, std::max(0.0f, tot - r.height + 20 * s));
}

void EditorMondo::PannelloStrati(Rectangle r) {
    float s = Ui::S();
    float hr = 28 * s;
    Mappa& m = M();
    if (Ui::MouseSu(r)) scorriPannello -= GetMouseWheelMove() * 50 * s;
    scorriPannello = std::max(0.0f, scorriPannello);
    BeginScissorMode((int)r.x - 4, (int)r.y, (int)r.width + 8, (int)r.height);
    Colonna L{ r, r.y - scorriPannello, s };
    Rectangle q;
    L.Titolo("STRATI DELLO SFONDO");
    if (m.vista == Vista::Alto) L.Nota("Nelle mappe viste dall'alto lo sfondo non si vede: conta il pavimento del tema.", Hex(0xff8a6a));
    L.Nota("Dal piu' lontano (in alto) al piu' vicino. Trascina un'immagine sulla finestra per aggiungerla come strato.", Ui::TestoSpento());
    if (stratoScelto >= (int)m.strati.size()) stratoScelto = (int)m.strati.size() - 1;
    int sposta = 0, togli = -1;
    for (int i = 0; i < (int)m.strati.size(); i++) {
        const StratoSfondo& st = m.strati[i];
        if (!L.Prendi(hr, q)) continue;
        Rectangle nome = { q.x, q.y, q.width - 3 * (hr + 4 * s), q.height };
        std::string t = (st.nome.empty() ? Maiuscola(st.tipo) : st.nome) + (st.primoPiano ? "  (davanti)" : "");
        if (Ui::Pulsante(nome, t, i == stratoScelto)) stratoScelto = i;
        DrawRectangleRec({ nome.x + 6 * s, nome.y + 7 * s, 10 * s, nome.height - 14 * s }, st.colore);
        float x = nome.x + nome.width + 4 * s;
        if (Ui::Pulsante({ x, q.y, hr, hr }, "^", false, i > 0)) { stratoScelto = i; sposta = -1; }
        if (Ui::Pulsante({ x + hr + 4 * s, q.y, hr, hr }, "v", false, i + 1 < (int)m.strati.size())) { stratoScelto = i; sposta = 1; }
        if (Ui::Pulsante({ x + 2 * (hr + 4 * s), q.y, hr, hr }, "x")) togli = i;
    }
    if (sposta != 0 && stratoScelto >= 0) {
        Ricorda();
        std::swap(m.strati[stratoScelto], m.strati[stratoScelto + sposta]);
        stratoScelto += sposta;
        Ricostruisci();
    }
    if (togli >= 0) { Ricorda(); m.strati.erase(m.strati.begin() + togli); stratoScelto = -1; Ricostruisci(); }
    static int nuovoTipo = 0;
    if (L.Prendi(hr, q)) {
        Rectangle a = { q.x, q.y, q.width * 0.62f, q.height };
        Ui::Scelta(a, "", nuovoTipo, TipiStrato());
        if (Ui::Pulsante({ q.x + q.width * 0.62f + 6 * s, q.y, q.width * 0.38f - 6 * s, q.height }, "+ Aggiungi strato")) {
            Ricorda();
            StratoSfondo st;
            st.tipo = TipiStrato()[nuovoTipo];
            st.nome = Maiuscola(st.tipo);
            if (st.tipo == "nuvole") { st.colore = Hex(0xffffff, 200); st.altezza = 0.25f; st.vento = 6; }
            if (st.tipo == "stelle") { st.colore = Hex(0xfff6d0); st.parallasseX = 0.02f; }
            if (st.tipo == "foresta") { st.colore = Hex(0x1e3a2b); st.altezza = 0.8f; st.parallasseX = 0.5f; }
            if (st.tipo == "colline") { st.colore = Hex(0x3f7a4a); st.altezza = 0.75f; st.parallasseX = 0.35f; }
            if (st.tipo == "fascia") { st.colore = Hex(0x2a4a8a, 120); st.altezza = 0.85f; }
            st.seme = (float)GetRandomValue(1, 99);
            m.strati.push_back(st);
            stratoScelto = (int)m.strati.size() - 1;
            Ricostruisci();
        }
    }
    if (stratoScelto >= 0 && stratoScelto < (int)m.strati.size()) {
        StratoSfondo st = m.strati[stratoScelto];
        bool c = false;
        L.Titolo("STRATO SCELTO");
        if (L.Prendi(hr, q)) c |= Ui::Campo("snome", q, st.nome, 30);
        int tipo = (int)(std::find(TipiStrato().begin(), TipiStrato().end(), st.tipo) - TipiStrato().begin());
        if (tipo >= (int)TipiStrato().size()) tipo = 0;
        if (L.Prendi(hr, q) && Ui::Scelta(q, "Tipo", tipo, TipiStrato())) { st.tipo = TipiStrato()[tipo]; c = true; }
        if (st.tipo == "immagine") {
            L.Nota("File (nella cartella immagini):", Ui::TestoSpento());
            if (L.Prendi(hr, q)) c |= Ui::Campo("simm", q, st.immagine, 80);
            if (L.Prendi(hr, q)) c |= Ui::Numero("ssc", q, "Scala", st.scala, 0.05f, 10, 0.05f, "%.2f");
            if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Ripetuta in orizzontale", st.ripeti);
        } else if (L.Prendi(hr, q)) c |= Ui::Colore("scol", q, "Colore", st.colore);
        if (L.Prendi(hr, q)) c |= Ui::Numero("spx", q, "Parallasse X", st.parallasseX, 0, 1.5f, 0.05f, "%.2f");
        if (L.Prendi(hr, q)) c |= Ui::Numero("spy", q, "Parallasse Y", st.parallasseY, 0, 1.5f, 0.05f, "%.2f");
        if (L.Prendi(hr, q)) c |= Ui::Numero("sal", q, "Altezza (0-1)", st.altezza, -0.5f, 1.5f, 0.05f, "%.2f");
        if (st.tipo == "montagne" || st.tipo == "colline" || st.tipo == "foresta")
            if (L.Prendi(hr, q)) c |= Ui::Numero("sam", q, "Ampiezza", st.ampiezza, 0, 200, 5);
        if (st.tipo != "fascia" && st.tipo != "immagine")
            if (L.Prendi(hr, q)) c |= Ui::Numero("sse", q, "Forma (seme)", st.seme, 0, 999, 1);
        if (L.Prendi(hr, q)) c |= Ui::Numero("sve", q, "Vento", st.vento, -100, 100, 2);
        if (L.Prendi(hr, q)) c |= Ui::Interruttore(q, "Davanti a tutto", st.primoPiano);
        if (c) { Ricorda(); m.strati[stratoScelto] = st; Ricostruisci(); }
    }

    L.Titolo("DECORAZIONI");
    if (m.decorazioni.empty()) L.Nota("Nessuna. Trascina un'immagine sulla mappa per aggiungerne una.", Ui::TestoSpento());
    for (int i = 0; i < (int)m.decorazioni.size(); i++)
        if (L.Prendi(hr, q) && Ui::Pulsante(q, m.decorazioni[i].immagine + TextFormat("  (%.0f, %.0f)", m.decorazioni[i].x, m.decorazioni[i].y), i == decorazioneScelta)) {
            decorazioneScelta = i; selezionato = -1; pannello = SchedaDestra::Proprieta; strumento = Strumento::Seleziona;
            camX = m.decorazioni[i].x - rVista.width / zoom / 2; camY = m.decorazioni[i].y - rVista.height / zoom / 2;
        }
    EndScissorMode();
    float tot = L.Fine() + scorriPannello - r.y;
    scorriPannello = std::min(scorriPannello, std::max(0.0f, tot - r.height + 20 * s));
}

// --- Chat con Claude --------------------------------------------------------------------------

void EditorMondo::PannelloClaude(Rectangle r) {
    float s = Ui::S();
    float hr = 30 * s, pad = 8 * s;
    float y = r.y;

    // Chiave mancante: si incolla qui, senza uscire dall'editor.
    if (cfg.chiave.empty()) {
        float h = Ui::TestoACapo("Per parlare con Claude serve una chiave API (inizia con sk-ant-). Copiala dalla console di Anthropic, "
                                 "clicca nel campo, Ctrl+V e premi Salva: resta nel file claude_chiave.txt accanto allo studio.",
                                 0, 0, r.width - 2 * pad, WHITE, Ui::TP(), false);
        Rectangle box = { r.x, y, r.width, h + hr + 3 * pad };
        DrawRectangleRounded(box, 0.08f, 6, Hex(0x3a2030));
        Ui::TestoACapo("Per parlare con Claude serve una chiave API (inizia con sk-ant-). Copiala dalla console di Anthropic, "
                       "clicca nel campo, Ctrl+V e premi Salva: resta nel file claude_chiave.txt accanto allo studio.",
                       r.x + pad, y + pad, r.width - 2 * pad, WHITE, Ui::TP());
        Rectangle campo = { r.x + pad, y + h + 2 * pad, r.width - 2 * pad - 90 * s, hr };
        std::string mostra = bozzaChiave;
        Ui::Campo("chiave", campo, bozzaChiave, 300);
        if (Ui::Pulsante({ campo.x + campo.width + 6 * s, campo.y, 84 * s, hr }, "Salva") ||
            (mostra != bozzaChiave && IsKeyPressed(KEY_ENTER))) {
            std::string k = bozzaChiave;
            while (!k.empty() && (k.back() == ' ' || k.back() == '\n' || k.back() == '\r')) k.pop_back();
            while (!k.empty() && k[0] == ' ') k.erase(0, 1);
            if (k.rfind("sk-ant-", 0) != 0) Avvisa("La chiave deve iniziare con sk-ant-");
            else if (SaveFileText((CartellaEseguibile() + "/claude_chiave.txt").c_str(), &k[0])) {
                cfg.chiave = k; bozzaChiave.clear();
                Avvisa("Chiave salvata: ora puoi chiedere a Claude!");
                gAudio.Suona(Effetto::Gemma);
            } else Avvisa("Non riesco a salvare claude_chiave.txt");
        }
        y += box.height + pad;
    }

    // Intestazione.
    Ui::Testo("Chiedi a Claude di cambiare il mondo", r.x, y + 4 * s, Ui::Accento(), Ui::TP());
    if (Ui::Pulsante({ r.x + r.width - 90 * s, y, 90 * s, 24 * s }, "Pulisci", false, !chat.storia.empty() && !chatInAttesa)) { chat.storia.clear(); scorriChat = 0; }
    y += 30 * s;

    // Suggerimenti e campo in basso.
    std::vector<std::string> idee;
    if (mondo.genere == Genere::OpenWorld || M().vista == Vista::Alto)
        idee = { "Aggiungi un villaggio con case di pietra e un sentiero", "Metti 15 gemme sparse e qualche slime a guardia",
                 "Crea una grotta collegata con una porta e un tesoro dentro" };
    else if (mondo.genere == Genere::Sparatutto)
        idee = { "Riempi il cielo di nemici a ondate", "Metti nuvole e montagne al tramonto", "Allunga il livello e aggiungi un boss alla fine" };
    else
        idee = { "Aggiungi piattaforme e 10 monete fino alla bandiera", "Fai un tramonto con montagne viola e nuvole",
                 "Crea una seconda mappa nel castello collegata con una porta" };
    float hIdee = 0;
    if (bozza.empty() && !chatInAttesa) hIdee = idee.size() * (26 * s + 4 * s) + 18 * s;
    Rectangle input = { r.x, r.y + r.height - hr - 2 * s, r.width - 84 * s, hr };
    Rectangle storia = { r.x, y, r.width, input.y - hIdee - 8 * s - y };

    // Messaggi: dal basso verso l'alto, con la rotella si risale.
    DrawRectangleRounded(storia, 0.03f, 6, Hex(0x0d1028));
    float larg = storia.width * 0.86f - 2 * pad;
    std::vector<float> alt;
    float tot = pad;
    for (auto& msg : chat.storia) {
        float h = Ui::TestoACapo(msg.testo, 0, 0, larg, WHITE, Ui::TP(), false) + 2 * pad + (msg.utente ? 0 : Ui::TP() + 4 * s);
        alt.push_back(h);
        tot += h + pad;
    }
    if (chatInAttesa) tot += 40 * s;
    if (Ui::MouseSu(storia)) scorriChat += GetMouseWheelMove() * 50 * s;
    scorriChat = Limita(scorriChat, 0, std::max(0.0f, tot - storia.height));
    BeginScissorMode((int)storia.x, (int)storia.y, (int)storia.width, (int)storia.height);
    if (chat.storia.empty() && !chatInAttesa) {
        Ui::TestoACapo("Claude vede tutto il mondo: mappe, oggetti, strati, telecamere. Scrivi cosa vuoi in italiano: "
                       "lui risponde e modifica il mondo. Ogni modifica si annulla con Ctrl+Z.\n\n"
                       "Esempi: \"metti dei pipistrelli sopra il burrone\", \"fai la mappa piu' lunga con una caverna\", "
                       "\"la telecamera scorra da sola\", \"cambia i colori dell'eroe in verde\".",
                       storia.x + 2 * pad, storia.y + 2 * pad, storia.width - 4 * pad, Ui::TestoSpento(), Ui::TP());
    }
    float yy = storia.y + storia.height - tot + scorriChat;
    if (tot < storia.height) yy = storia.y;
    yy += pad;
    for (size_t i = 0; i < chat.storia.size(); i++) {
        const auto& msg = chat.storia[i];
        float h = alt[i];
        if (yy + h > storia.y && yy < storia.y + storia.height) {
            float bw = msg.utente ? std::min(larg, (float)Ui::Larghezza(msg.testo, Ui::TP())) + 2 * pad : larg + 2 * pad;
            Rectangle b = { msg.utente ? storia.x + storia.width - bw - pad : storia.x + pad, yy, bw, h };
            DrawRectangleRounded(b, std::min(0.3f, 12 * s / h), 6, msg.utente ? Hex(0x2e3680) : Hex(0x262038));
            float ty = b.y + pad;
            if (!msg.utente) {
                DrawCircle((int)(b.x + pad + 5 * s), (int)(ty + Ui::TP() / 2), 5 * s, Hex(0xd97757));
                Ui::Testo("Claude", b.x + pad + 14 * s, ty, Hex(0xd97757), Ui::TP());
                ty += Ui::TP() + 4 * s;
            }
            // Le righe "[Fatto ...]" in verde, "[Non applicato ...]" in arancio.
            size_t a = msg.testo.find("\n[");
            std::string corpo = a == std::string::npos ? msg.testo : msg.testo.substr(0, a);
            float hc = Ui::TestoACapo(corpo, b.x + pad, ty, larg, WHITE, Ui::TP());
            if (a != std::string::npos) {
                std::string resto = msg.testo.substr(a + 1);
                size_t k = resto.find("\n[");
                std::string uno = k == std::string::npos ? resto : resto.substr(0, k);
                float h1 = Ui::TestoACapo(uno, b.x + pad, ty + hc, larg, uno.rfind("[Fatto", 0) == 0 ? Hex(0x7ee0a0) : Hex(0xffaa6a), Ui::TP());
                if (k != std::string::npos) Ui::TestoACapo(resto.substr(k + 1), b.x + pad, ty + hc + h1, larg, Hex(0xffaa6a), Ui::TP());
            }
        }
        yy += h + pad;
    }
    if (chatInAttesa) {
        Rectangle b = { storia.x + pad, yy, 150 * s, 32 * s };
        DrawRectangleRounded(b, 0.4f, 6, Hex(0x262038));
        for (int k = 0; k < 3; k++)
            DrawCircle((int)(b.x + 30 * s + k * 18 * s), (int)(b.y + b.height / 2 - 4 * s * std::max(0.0f, std::sin(tAttesa * 6 - k))), 4 * s, Hex(0xd97757));
        Ui::Testo(TextFormat("%.0fs", tAttesa), b.x + 100 * s, b.y + 9 * s, Ui::TestoSpento(), Ui::TP());
    }
    EndScissorMode();

    if (hIdee > 0) {
        float iy = storia.y + storia.height + 8 * s;
        Ui::Testo("Prova a chiedere:", r.x, iy, Ui::TestoSpento(), Ui::TP());
        iy += 18 * s;
        for (auto& idea : idee) {
            if (Ui::Pulsante({ r.x, iy, r.width, 26 * s }, idea)) { bozza = idea; Ui::DaiFuoco("chat"); }
            iy += 30 * s;
        }
    }
    std::string prima = bozza;
    bool invio = Ui::Campo("chat", input, bozza, 600) && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER));
    if (Ui::Fuoco() != "chat" && bozza.empty() && !Ui::StaScrivendo()) Ui::Testo("Scrivi qui e premi Invio...", input.x + 8 * s, input.y + (hr - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    bool premuto = Ui::Pulsante({ input.x + input.width + 6 * s, input.y, 78 * s, hr }, chatInAttesa ? "..." : "Invia", true, !chatInAttesa);
    (void)prima;
    if ((invio || premuto) && !bozza.empty() && !chatInAttesa) {
        std::string t = bozza;
        bozza.clear();
        InviaChat(t);
        if (cfg.chiave.empty()) bozza = t;   // resta da mandare dopo aver messo la chiave
        else Ui::DaiFuoco("chat");
    }
}

// --- Finestre --------------------------------------------------------------------------------

void EditorMondo::Finestre() {
    if (finestra == 0) return;
    float s = Ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Hex(0x000000, 150));
    Ui::ApriModale();
    float fw = std::min(W - 40 * s, 900 * s), fh = std::min(H - 40 * s, finestra == 4 ? 170 * s : 640 * s);
    Rectangle f = { (W - fw) / 2, (H - fh) / 2, fw, fh };
    DrawRectangleRounded(f, 0.03f, 8, Ui::Pannello());
    DrawRectangleRoundedLinesEx(f, 0.03f, 8, std::max(1.0f, 2 * s), Ui::Accento());
    const char* titoli[] = { "", "Nuovo mondo: scegli il genere", "Apri un mondo", "Comandi dell'editor", "Rinomina la mappa" };
    Ui::Testo(titoli[finestra], f.x + 20 * s, f.y + 16 * s, Ui::Accento(), Ui::T());
    bool chiudi = Ui::Pulsante({ f.x + f.width - 44 * s, f.y + 12 * s, 32 * s, 28 * s }, "X") || IsKeyPressed(KEY_ESCAPE);
    Rectangle c = { f.x + 20 * s, f.y + 52 * s, f.width - 40 * s, f.height - 72 * s };
    if (modificato && (finestra == 1 || finestra == 2)) {
        Ui::Testo("Attenzione: le modifiche non salvate andranno perse (Ctrl+S per salvare prima).", c.x, c.y, Hex(0xff8a6a), Ui::TP());
        c.y += 26 * s; c.height -= 26 * s;
    }

    if (finestra == 1) {
        const char* nomi[] = { "Platform", "Open world 2D", "Sparatutto", "Avventura" };
        const char* desc[] = { "Corri e salta fra piattaforme, monete, nemici e bandiera d'arrivo. Vista laterale con gravita'.",
                               "Mondo grande visto dall'alto, da esplorare in tutte le direzioni, con grotte collegate da porte.",
                               "Aereo o carro armato: lo schermo scorre da solo, si spara con mitra e bombe.",
                               "Castello a stanze: lo schermo cambia quando esci da una stanza; chiavi, porte, segreti." };
        float cw = (c.width - 3 * 16 * s) / 4;
        for (int i = 0; i < 4; i++) {
            Rectangle k = { c.x + i * (cw + 16 * s), c.y + 10 * s, cw, c.height - 20 * s };
            bool sopra = Ui::MouseSu(k);
            DrawRectangleRounded(k, 0.05f, 8, sopra ? Hex(0x252b5c) : Hex(0x10132c));
            DrawRectangleRoundedLinesEx(k, 0.05f, 8, std::max(1.0f, 2 * s), sopra ? Ui::Accento() : Ui::Bordo());
            // Piccola illustrazione del genere.
            Rectangle ill = { k.x + 12 * s, k.y + 12 * s, k.width - 24 * s, k.width * 0.6f };
            DrawRectangleRec(ill, i == 1 ? Hex(0x3a6a3a) : i == 3 ? Hex(0x1a1430) : Hex(0x4a8ad0));
            float u = ill.width / 12;
            BeginScissorMode((int)ill.x, (int)ill.y, (int)ill.width, (int)ill.height);
            if (i == 0) for (int t = 0; t < 12; t++) DrawRectangleRec({ ill.x + t * u, ill.y + ill.height - u * (1 + (t % 4 == 2)), u, u * (1 + (t % 4 == 2)) }, Hex(0x5ac85a));
            if (i == 1) for (int t = 0; t < 14; t++) DrawCircleV({ ill.x + Hash1(t) * ill.width, ill.y + Hash1(t + 40) * ill.height }, u * 0.6f, Hex(0x1e4a2b));
            if (i == 2) { DrawTriangle({ ill.x + 2 * u, ill.y + ill.height / 2 - u }, { ill.x + 2 * u, ill.y + ill.height / 2 + u }, { ill.x + 4 * u, ill.y + ill.height / 2 }, WHITE);
                          for (int t = 0; t < 4; t++) DrawRectangleRec({ ill.x + (5 + t * 1.5f) * u, ill.y + ill.height / 2 - 1, u * 0.6f, 2 }, Hex(0xffd23f)); }
            if (i == 3) { DrawRectangleLinesEx({ ill.x + u, ill.y + u, ill.width / 2 - u, ill.height - 2 * u }, 2, Hex(0x8a90ac));
                          DrawRectangleLinesEx({ ill.x + ill.width / 2, ill.y + u, ill.width / 2 - u, ill.height - 2 * u }, 2, Hex(0x8a90ac)); }
            EndScissorMode();
            Ui::Testo(nomi[i], k.x + 12 * s, ill.y + ill.height + 14 * s, WHITE, Ui::T());
            Ui::TestoACapo(desc[i], k.x + 12 * s, ill.y + ill.height + 44 * s, k.width - 24 * s, Ui::TestoSpento(), Ui::TP());
            if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { NuovoMondo(i); finestra = 0; gAudio.Suona(Effetto::Conferma); }
        }
    } else if (finestra == 2) {
        if (Ui::MouseSu(c)) scorriElenco -= GetMouseWheelMove() * 60 * s;
        float hv = 52 * s;
        scorriElenco = Limita(scorriElenco, 0, std::max(0.0f, elencoMondi.size() * (hv + 6 * s) - c.height));
        BeginScissorMode((int)c.x, (int)c.y, (int)c.width, (int)c.height);
        for (size_t i = 0; i < elencoMondi.size(); i++) {
            Rectangle v = { c.x, c.y + i * (hv + 6 * s) - scorriElenco, c.width, hv };
            if (v.y + v.height < c.y || v.y > c.y + c.height) continue;
            bool sopra = Ui::MouseSu(v) && CheckCollisionPointRec(GetMousePosition(), c);
            DrawRectangleRounded(v, 0.15f, 6, sopra ? Ui::Selezione() : Hex(0x10132c));
            const Mondo& w = elencoMondi[i];
            Ui::Testo(w.titolo, v.x + 14 * s, v.y + 8 * s, WHITE, Ui::T());
            std::string sotto = std::string(NomeGenere(w.genere)) + "  -  " + std::to_string(w.mappe.size()) + (w.mappe.size() == 1 ? " mappa" : " mappe") + "  -  " +
                                (elencoPercorsi[i].empty() ? "esempio incluso" : std::string(GetFileName(elencoPercorsi[i].c_str())));
            Ui::Testo(sotto, v.x + 14 * s, v.y + 30 * s, Ui::TestoSpento(), Ui::TP());
            if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Apri(w, elencoPercorsi[i], "Aperto: " + w.titolo);
                finestra = 0;
                gAudio.Suona(Effetto::Conferma);
            }
        }
        EndScissorMode();
    } else if (finestra == 3) {
        const char* righe[] = {
            "MOUSE",
            "Clic sinistro: usa lo strumento        Clic destro: cancella",
            "Rotella: scorri su e giu'     Maiusc+rotella: a destra e sinistra     Ctrl+rotella: zoom",
            "Tasto centrale o Spazio+trascina: sposta la vista     Clic sulla minimappa: salta li'",
            "Alt mentre metti o sposti un oggetto: posizione libera (non sulla griglia)",
            "Trascina un'immagine PNG/JPG sulla finestra: decorazione (o strato, se sei nella scheda Strati)",
            "",
            "TASTIERA",
            "Frecce: scorri la mappa nelle 4 direzioni (Maiusc: piu' veloce)",
            "B pennello   E gomma   F riempi   R rettangolo (Maiusc: solo bordo)   V seleziona   I contagocce",
            "1-6: caselle (vuoto, terra, pietra, piattaforma, spine, bersaglio)    G: griglia    +/-: zoom    Home: partenza",
            "Ctrl+Z annulla   Ctrl+Y ripeti   Ctrl+C / Ctrl+V copia e incolla l'oggetto   Canc elimina",
            "Ctrl+S salva   Ctrl+N nuovo   Ctrl+O apri   Ctrl+E crea il .exe   F3 chat con Claude",
            "F5 prova dall'inizio   F6 prova dal punto sotto il mouse   Esc torna allo studio",
            "",
            "PORTE E MAPPE",
            "Metti una Porta, poi in Proprieta' scegli la mappa d'arrivo e \"Scegli il punto d'arrivo\".",
            "La bandiera con una mappa di destinazione porta alla mappa successiva dopo la vittoria.",
        };
        float yy = c.y;
        for (auto r : righe) {
            bool tit = r[0] && std::isupper((unsigned char)r[0]) && std::isupper((unsigned char)r[1]);
            Ui::Testo(r, c.x, yy, tit ? Ui::Accento() : Ui::TestoChiaro(), Ui::TP());
            yy += Ui::TP() * 1.55f;
        }
    } else if (finestra == 4) {
        Rectangle campo = { c.x, c.y + 10 * s, c.width - 130 * s, 32 * s };
        bool ok = Ui::Campo("rinomina", campo, nomeMappaBozza, 40) && IsKeyPressed(KEY_ENTER);
        if (Ui::Fuoco() != "rinomina" && !ok && Ui::Fuoco().empty()) Ui::DaiFuoco("rinomina");
        if (Ui::Pulsante({ campo.x + campo.width + 10 * s, campo.y, 120 * s, 32 * s }, "OK", true)) ok = true;
        if (ok && !nomeMappaBozza.empty()) { Ricorda(); M().nome = nomeMappaBozza; finestra = 0; chiudi = false; }
    }
    Ui::ChiudiModale();
    if (chiudi) { finestra = 0; Ui::TogliFuoco(); }
}

// --- Scorciatoie -----------------------------------------------------------------------------

void EditorMondo::Scorciatoie() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (sceltaArrivo) { sceltaArrivo = false; CambiaMappa(portaMappa); selezionato = portaIndice; Avvisa("Scelta dell'arrivo annullata"); }
        else if (selezionato >= 0 || decorazioneScelta >= 0) { selezionato = -1; decorazioneScelta = -1; }
        else vuoleUscire = true;
        return;
    }
    if (Ctrl()) {
        if (IsKeyPressed(KEY_Z)) { if (Maiusc()) Ripeti(); else Annulla(); }
        if (IsKeyPressed(KEY_Y)) Ripeti();
        if (IsKeyPressed(KEY_S)) Salva(false);
        if (IsKeyPressed(KEY_N)) finestra = 1;
        if (IsKeyPressed(KEY_O)) { ApriElenco(); scorriElenco = 0; }
        if (IsKeyPressed(KEY_E)) CreaEseguibile();
        if (IsKeyPressed(KEY_C)) Esegui(1, 2);
        if (IsKeyPressed(KEY_V)) Esegui(1, 3);
        return;
    }
    if (IsKeyPressed(KEY_DELETE)) Esegui(1, 4);
    if (IsKeyPressed(KEY_F1)) finestra = 3;
    if (IsKeyPressed(KEY_F3)) { pannello = SchedaDestra::Claude; Ui::DaiFuoco("chat"); }
    if (IsKeyPressed(KEY_F5)) AvviaProva(false);
    if (IsKeyPressed(KEY_F6)) AvviaProva(true);
    if (IsKeyPressed(KEY_G)) griglia = !griglia;
    if (IsKeyPressed(KEY_HOME)) Inquadra();
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) zoom = std::min(8.0f, zoom * 1.25f);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) zoom = std::max(0.25f, zoom / 1.25f);
    const int tasti[] = { KEY_B, KEY_E, KEY_F, KEY_R, KEY_V, KEY_I };
    for (int i = 0; i < 6; i++) if (IsKeyPressed(tasti[i])) { strumento = (Strumento)i; sceltaArrivo = false; }
    for (int i = 0; i < 6; i++)
        if (IsKeyPressed(KEY_ONE + i)) { pennello = std::string(1, CASELLE[i]); if (strumento != Strumento::Riempi && strumento != Strumento::Rettangolo) strumento = Strumento::Pennello; }
}
