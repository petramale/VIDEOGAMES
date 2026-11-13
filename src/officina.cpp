// L'officina: struttura, menu di sistema, home, file, prova, eseguibile e chat con Claude.
// Le sezioni sono in officina_sezioni.cpp, officina_audio.cpp e officina_logica.cpp.
#include "officina.h"
#include "audio.h"
#include "claude_progetto.h"
#include "immagini.h"
#include "pacchetto.h"
#include "periferiche.h"
#include "progetto_json.h"
#include "raccolte.h"
#include "sistema.h"
#include "ui.h"
#include <algorithm>
#include <fstream>

using nlohmann::json;

namespace {

const char* NOMI_SEZIONI[] = { "Home", "Mondi", "Personaggi", "Oggetti", "Scenari", "Audio", "Logica", "Librerie", "Comandi" };
const char* TASTI_SEZIONI[] = { "", "Alt+1", "Alt+2", "Alt+3", "Alt+4", "Alt+5", "Alt+6", "Alt+7", "Alt+8" };

bool Ctrl() { return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL); }
bool Alt() { return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT); }

std::string CartellaGiochi() { return CartellaEseguibile() + "/giochi"; }

std::string LeggiTesto(const std::string& f) {
    std::ifstream in(f, std::ios::binary);
    if (!in) return "";
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string Estensione(const std::string& f) {
    const char* e = GetFileExtension(f.c_str());
    std::string s = e ? e : "";
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Piccola icona disegnata per ogni sezione (niente file esterni).
void IconaSezione(int i, Rectangle r, Color c) {
    float cx = r.x + r.width / 2, cy = r.y + r.height / 2, u = r.height / 16;
    switch (i) {
        case 0: DrawTriangle({ cx, cy - 6 * u }, { cx - 7 * u, cy }, { cx + 7 * u, cy }, c); DrawRectangleRec({ cx - 5 * u, cy, 10 * u, 6 * u }, c); break;
        case 1: DrawRectangleRec({ cx - 7 * u, cy + 2 * u, 14 * u, 4 * u }, c); DrawRectangleRec({ cx - 3 * u, cy - 3 * u, 5 * u, 3 * u }, c); DrawCircleV({ cx + 4 * u, cy - 5 * u }, 2 * u, c); break;
        case 2: DrawCircleV({ cx, cy - 4 * u }, 3 * u, c); DrawRectangleRec({ cx - 3 * u, cy - u, 6 * u, 6 * u }, c); DrawRectangleRec({ cx - 3 * u, cy + 5 * u, 2 * u, 2 * u }, c); DrawRectangleRec({ cx + u, cy + 5 * u, 2 * u, 2 * u }, c); break;
        case 3: DrawPoly({ cx, cy }, 4, 6 * u, 45, c); DrawPoly({ cx, cy }, 4, 3 * u, 45, Hex(0x0d0f24)); break;
        case 4: DrawTriangle({ cx - 7 * u, cy + 5 * u }, { cx + u, cy + 5 * u }, { cx - 3 * u, cy - 3 * u }, c); DrawTriangle({ cx - u, cy + 5 * u }, { cx + 7 * u, cy + 5 * u }, { cx + 3 * u, cy - 5 * u }, c); break;
        case 5: DrawRectangleRec({ cx - 4 * u, cy - 6 * u, 2 * u, 10 * u }, c); DrawRectangleRec({ cx + 3 * u, cy - 6 * u, 2 * u, 8 * u }, c); DrawRectangleRec({ cx - 4 * u, cy - 6 * u, 9 * u, 2 * u }, c);
                DrawCircleV({ cx - 5 * u, cy + 4 * u }, 2.5f * u, c); DrawCircleV({ cx + 2 * u, cy + 2 * u }, 2.5f * u, c); break;
        case 6: DrawRectangleLinesEx({ cx - 7 * u, cy - 5 * u, 6 * u, 4 * u }, u, c); DrawRectangleLinesEx({ cx + u, cy + u, 6 * u, 4 * u }, u, c);
                DrawLineEx({ cx - 4 * u, cy - u }, { cx - 4 * u, cy + 3 * u }, u, c); DrawLineEx({ cx - 4 * u, cy + 3 * u }, { cx + u, cy + 3 * u }, u, c); break;
        case 7: for (int k = 0; k < 3; k++) DrawRectangleRec({ cx - 6 * u + k * 4.5f * u, cy - 6 * u + (k == 1 ? 2 * u : 0), 3 * u, 12 * u - (k == 1 ? 2 * u : 0) }, c); break;
        case 8: DrawRectangleRounded({ cx - 7 * u, cy - 3 * u, 14 * u, 7 * u }, 0.6f, 6, c); DrawCircleV({ cx + 3 * u, cy }, 1.2f * u, Hex(0x0d0f24)); DrawRectangleRec({ cx - 5 * u, cy - 0.5f * u, 4 * u, u }, Hex(0x0d0f24)); break;
    }
}

} // namespace

// --- Avvio e file -------------------------------------------------------------------------

void Officina::Avvia() {
    cfg = ConfigClaude::Carica();
    editorMondo.Inizializza();
    editorMondo.incorporato = true;
    editorMondo.progetto = &progetto;
    sfondoHome.tema = 0;
    sfondoHome.ora = 0.72f;
    CaricaRecenti();
    if (!recenti.empty() && FileExists(recenti[0].c_str()) && ApriFile(recenti[0])) Avvisa("Riaperto " + std::string(GetFileName(recenti[0].c_str())));
    else NuovoGioco(Genere::Platform, StileGrafico::PixelFine, "Il mio gioco");
    sezione = Sezione::Home;
    modificato = false;
}

void Officina::CaricaRecenti() {
    recenti.clear();
    std::string t = LeggiTesto(CartellaEseguibile() + "/giochi_recenti.txt");
    size_t i = 0;
    while (i < t.size()) {
        size_t a = t.find('\n', i);
        std::string r = t.substr(i, a == std::string::npos ? std::string::npos : a - i);
        while (!r.empty() && (r.back() == '\r' || r.back() == ' ')) r.pop_back();
        if (!r.empty() && recenti.size() < 8) recenti.push_back(r);
        if (a == std::string::npos) break;
        i = a + 1;
    }
}

void Officina::AggiungiRecente(const std::string& p) {
    recenti.erase(std::remove(recenti.begin(), recenti.end(), p), recenti.end());
    recenti.insert(recenti.begin(), p);
    if (recenti.size() > 8) recenti.resize(8);
    std::string t;
    for (auto& r : recenti) t += r + "\n";
    SaveFileText((CartellaEseguibile() + "/giochi_recenti.txt").c_str(), &t[0]);
}

void Officina::NuovoGioco(Genere g, StileGrafico stile, const std::string& titolo) {
    progetto = NuovoProgetto(g, stile, titolo);
    percorso.clear();
    annulla.clear(); ripeti.clear();
    modificato = false;
    selPersonaggio = selOggetto = selScenario = selCasella = selBrano = selEffetto = selRegola = 0;
    editorDisegno.Chiudi();
    editorMondo.Apri(progetto.mondo, "", "");
    LibreriaCambiata();
    Avvisa(std::string("Nuovo gioco: ") + NomeGenere(g) + ", " + NomeStile(stile) + ". Salvalo con Ctrl+S.");
}

bool Officina::ApriFile(const std::string& f) {
    std::string testo = LeggiTesto(f), errore;
    if (testo.empty()) { Avvisa("Non riesco a leggere " + f); return false; }
    ProgettoGioco p;
    if (!p.Carica(testo, errore)) { Avvisa("File non valido: " + errore); return false; }
    progetto = p;
    percorso = Estensione(f) == ".gioco" ? f : "";
    if (progetto.mondo.titolo.empty()) progetto.mondo.titolo = GetFileNameWithoutExt(f.c_str());
    annulla.clear(); ripeti.clear();
    modificato = false;
    selPersonaggio = selOggetto = selScenario = selCasella = selBrano = selEffetto = selRegola = 0;
    editorDisegno.Chiudi();
    for (auto& im : progetto.immagini) AggiungiFileImmagine(im.first, im.second);
    editorMondo.Apri(progetto.mondo, "", "");
    LibreriaCambiata();
    if (!percorso.empty()) AggiungiRecente(percorso);
    return true;
}

void Officina::SincronizzaMondo() {
    if (editorMondo.Modificato() || sezione == Sezione::Mondi) {
        if (progetto.mondo.Json(true) != editorMondo.MondoAperto().Json(true)) { progetto.mondo = editorMondo.MondoAperto(); modificato = true; }
        editorMondo.SegnaSalvato();
    }
    // Le immagini usate dal mondo (strati, decorazioni) viaggiano dentro il gioco.
    for (auto& m : progetto.mondo.mappe) {
        std::vector<std::string> usate;
        for (auto& s : m.strati) if (!s.immagine.empty()) usate.push_back(s.immagine);
        for (auto& d : m.decorazioni) if (!d.immagine.empty()) usate.push_back(d.immagine);
        for (auto& u : usate) {
            bool c = false;
            for (auto& im : progetto.immagini) if (im.first == u) c = true;
            if (c) continue;
            std::string dati = LeggiTesto(CartellaEseguibile() + "/immagini/" + u);
            if (!dati.empty()) progetto.immagini.push_back({ u, dati });
        }
    }
}

void Officina::MondoCambiato() {
    editorMondo.Sostituisci(progetto.mondo);
    editorMondo.SegnaSalvato();
}

void Officina::LibreriaCambiata() {
    std::vector<std::string> nomi;
    for (auto& x : progetto.personaggi) nomi.push_back(x.nome);
    for (auto& x : progetto.oggetti) nomi.push_back(x.nome);
    ImpostaNomiLibreria(nomi);
    editorMondo.AggiornaVista();
}

void Officina::Ricorda() {
    SincronizzaMondo();
    annulla.push_back(progetto);
    if (annulla.size() > 30) annulla.erase(annulla.begin());
    ripeti.clear();
    modificato = true;
}

void Officina::Annulla() {
    if (sezione == Sezione::Mondi) { editorMondo.Esegui(1, 0); return; }
    if (annulla.empty()) { Avvisa("Niente da annullare"); return; }
    SincronizzaMondo();
    ripeti.push_back(progetto);
    progetto = annulla.back();
    annulla.pop_back();
    MondoCambiato();
    LibreriaCambiata();
    modificato = true;
    Avvisa("Annullato");
}

void Officina::Ripeti() {
    if (sezione == Sezione::Mondi) { editorMondo.Esegui(1, 1); return; }
    if (ripeti.empty()) { Avvisa("Niente da ripetere"); return; }
    SincronizzaMondo();
    annulla.push_back(progetto);
    progetto = ripeti.back();
    ripeti.pop_back();
    MondoCambiato();
    LibreriaCambiata();
    modificato = true;
    Avvisa("Ripetuto");
}

void Officina::Salva(bool conNome) {
    SincronizzaMondo();
    MakeDirectory(CartellaGiochi().c_str());
    if (percorso.empty() || conNome) {
        std::string base = CartellaGiochi() + "/" + NomeFileDaTitolo(progetto.mondo.titolo);
        percorso = base + ".gioco";
        for (int i = 2; conNome && FileExists(percorso.c_str()); i++) percorso = base + " " + std::to_string(i) + ".gioco";
    }
    std::string testo = progetto.Json();
    std::ofstream out(percorso, std::ios::binary);
    if (out && out.write(testo.data(), (std::streamsize)testo.size())) {
        modificato = false;
        AggiungiRecente(percorso);
        Avvisa("Gioco salvato in giochi/" + std::string(GetFileName(percorso.c_str())));
        gAudio.Suona(Effetto::Conferma, 0.6f);
    } else Avvisa("Non riesco a salvare " + percorso);
}

void Officina::CreaEseguibile() {
    SincronizzaMondo();
    std::string problema = progetto.Controlla();
    if (!problema.empty()) { Avvisa("Il gioco ha un problema: " + problema); return; }
#ifdef _WIN32
    std::string nome = NomeFileDaTitolo(progetto.mondo.titolo) + ".exe";
#else
    std::string nome = NomeFileDaTitolo(progetto.mondo.titolo);
#endif
    std::string errore;
    DatiGioco d{ progetto.mondo.titolo, progetto.Json(true), {} };
    if (::CreaEseguibile(CartellaEseguibile() + "/" + nome, SerializzaGioco(d), errore)) {
        Avvisa("Creato " + nome + " accanto allo studio! Parte con il logo Elvantis e il menu.");
        gAudio.Suona(Effetto::Vittoria);
    } else Avvisa("Errore: " + errore);
}

void Officina::ApriElenco() {
    elencoGiochi.clear();
    if (DirectoryExists(CartellaGiochi().c_str())) {
        FilePathList l = LoadDirectoryFilesEx(CartellaGiochi().c_str(), ".gioco", false);
        for (unsigned int i = 0; i < l.count; i++) elencoGiochi.push_back(l.paths[i]);
        UnloadDirectoryFiles(l);
    }
    std::string mondi = CartellaEseguibile() + "/mondi";
    if (DirectoryExists(mondi.c_str())) {
        FilePathList l = LoadDirectoryFilesEx(mondi.c_str(), ".json", false);
        for (unsigned int i = 0; i < l.count; i++) elencoGiochi.push_back(l.paths[i]);
        UnloadDirectoryFiles(l);
    }
    scorriElenco = 0;
    finestra = 2;
}

void Officina::Vai(Sezione s) {
    if (s == sezione) return;
    if (sezione == Sezione::Mondi) SincronizzaMondo();
    if (s == Sezione::Mondi) { editorMondo.Sostituisci(progetto.mondo); editorMondo.SegnaSalvato(); LibreriaCambiata(); editorMondo.Entra(); }
    if (sezione == Sezione::Audio) { gAudio.FermaBranoPersonale(); suonaBrano = false; }
    sezione = s;
    scorriLista = scorriProprieta = 0;
    menuAperto = -1;
    Ui::TogliFuoco();
}

// --- Prova del gioco (F5) ------------------------------------------------------------------------

void Officina::AvviaProva() {
    if (sezione == Sezione::Mondi) { editorMondo.Esegui(5, 0); return; }
    SincronizzaMondo();
    std::string errore;
    prova = Gioco{};
    if (!prova.CaricaProgetto(progetto, errore)) { Avvisa("Non si puo' provare: " + errore); return; }
    provando = true;
    tProva = 0;
    Ui::TogliFuoco();
    if (!prova.MusicaDellaLibreria()) gAudio.ScegliBrano(prova.musica);
    gAudio.Suona(Effetto::Conferma);
}

void Officina::FrameProva(float dt) {
    tProva += dt;
    if (tProva > 0.2f && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F5))) {
        provando = false;
        gAudio.TogliSostituzioni();
        gAudio.FermaBranoPersonale();
        gAudio.ScegliBrano(0);
        return;
    }
    if (IsKeyPressed(KEY_R) || (prova.finito && gInput.Premuto(Azione::Conferma))) prova.Ricomincia();
    if (IsKeyPressed(KEY_F1)) prova.mostraCollisioni = !prova.mostraCollisioni;
    prova.Aggiorna(dt, Comandi::DaPeriferiche());
    if (!prova.MusicaDellaLibreria() && (prova.musica != gAudio.BranoAttuale() || !gAudio.BranoPersonale().empty())) gAudio.ScegliBrano(prova.musica);
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float s = std::min(sw / VW, sh / VH);
    prova.DisegnaHD(rtProva, std::max(1, std::min(4, (int)std::ceil(s))));
    Rectangle area = { (sw - VW * s) / 2, (sh - VH * s) / 2, VW * s, VH * s };
    ClearBackground(BLACK);
    DrawTexturePro(rtProva.texture, { 0, 0, (float)rtProva.texture.width, -(float)rtProva.texture.height }, area, { 0, 0 }, 0, WHITE);
    int dim = std::max(12, (int)(sh / 60));
    DrawRectangle(0, (int)(sh - dim * 1.7f), (int)sw, (int)(dim * 1.7f), Hex(0x0b0e22, 200));
    DrawText("PROVA DEL GIOCO   Esc o F5: torna all'officina   R: ricomincia   F1: collisioni", 12, (int)(sh - dim * 1.35f), dim, Hex(0xc8d0f0));
    postoAudio = { area.x + area.width - 44, area.y + 6, 36, 36 };
}

// --- Il fotogramma -----------------------------------------------------------------------------

void Officina::Layout() {
    float s = Ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    float hb = std::round(34 * s), hs = std::round(48 * s);
    float wc = chatAperta ? std::round(std::min(400 * s, W * 0.27f)) : std::round(34 * s);
    rBarra = { 0, 0, W, hb };
    rSchede = { 0, hb, W, hs };
    rChat = { W - wc, hb + hs, wc, H - hb - hs };
    rCorpo = { 0, hb + hs, W - wc, H - hb - hs };
}

void Officina::Frame(float dt) {
    Ui::Inizia(GetScreenHeight() / 900.0f);
    tempo += dt;
    if (tAvviso > 0) tAvviso -= dt;
    ControllaChat();
    if (chatInAttesa) tChat += dt;
    if (provando) { FrameProva(dt); return; }
    if (editorMondo.Provando()) { editorMondo.Frame(dt); postoAudio = editorMondo.PostoAudio(); return; }

    Layout();
    if (menuAperto >= 0 || finestra != 0) Ui::ImpostaModale({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() });
    if (finestra == 0 && menuAperto < 0 && !Ui::StaScrivendo()) Scorciatoie();
    if (provando) { FrameProva(0); return; }
    FileTrascinati();

    ClearBackground(Ui::Fondo());
    switch (sezione) {
        case Sezione::Home: Home(rCorpo); break;
        case Sezione::Mondi: Mondi(rCorpo, dt); break;
        case Sezione::Personaggi: Personaggi(rCorpo, dt); break;
        case Sezione::Oggetti: Oggetti(rCorpo, dt); break;
        case Sezione::Scenari: Scenari(rCorpo, dt); break;
        case Sezione::Audio: Audio(rCorpo, dt); break;
        case Sezione::Logica: Logica(rCorpo, dt); break;
        case Sezione::Librerie: Librerie(rCorpo, dt); break;
        case Sezione::Comandi: Comandi(rCorpo, dt); break;
        default: break;
    }
    if (editorMondo.Provando()) return;   // la prova e' partita da un menu dell'editor
    PannelloChat(rChat);
    Schede();
    Ui::ApriModale();
    BarraMenu();
    MenuTendina();
    Ui::ChiudiModale();
    Finestre();
    // Avviso in basso al centro.
    if (tAvviso > 0 && !avviso.empty()) {
        float s = Ui::S(), k = Limita(tAvviso * 2, 0, 1);
        float w = Ui::Larghezza(avviso, Ui::T()) + 40 * s;
        Rectangle r = { rCorpo.x + (rCorpo.width - w) / 2, rCorpo.y + rCorpo.height - 70 * s, w, 40 * s };
        DrawRectangleRounded(r, 0.4f, 8, Hex(0x101430, (unsigned char)(235 * k)));
        DrawRectangleRoundedLinesEx(r, 0.4f, 8, 2, ConAlfa(Ui::Accento(), k));
        Ui::TestoCentrato(avviso, r, ConAlfa(WHITE, k), Ui::T());
    }
}

void Officina::Scorciatoie() {
    if (Ctrl()) {
        if (IsKeyPressed(KEY_S)) Salva(IsKeyDown(KEY_LEFT_SHIFT));
        if (IsKeyPressed(KEY_N)) finestra = 1;
        if (IsKeyPressed(KEY_O)) ApriElenco();
        if (IsKeyPressed(KEY_E)) CreaEseguibile();
        if (sezione != Sezione::Mondi && sezione != Sezione::Personaggi && sezione != Sezione::Oggetti && !(sezione == Sezione::Scenari && schedaScenari == 1)) {
            if (IsKeyPressed(KEY_Z)) Annulla();
            if (IsKeyPressed(KEY_Y)) Ripeti();
        }
    }
    if (Alt()) for (int i = 1; i < (int)Sezione::Totale; i++) if (IsKeyPressed(KEY_ZERO + i)) Vai((Sezione)i);
    if (IsKeyPressed(KEY_F5)) AvviaProva();
    if (IsKeyPressed(KEY_F1)) finestra = 3;
    if (IsKeyPressed(KEY_F3)) { chatAperta = true; Ui::DaiFuoco("chat"); }
    if (IsKeyPressed(KEY_F2)) chatAperta = !chatAperta;
}

void Officina::FileTrascinati() {
    if (!IsFileDropped()) return;
    if (sezione == Sezione::Mondi) return;   // le immagini nel mondo le prende l'editor dei mondi
    FilePathList l = LoadDroppedFiles();
    for (unsigned int i = 0; i < l.count; i++) ImportaFile(l.paths[i]);
    UnloadDroppedFiles(l);
}

// --- Barra dei menu ---------------------------------------------------------------------------------

std::vector<Officina::MenuO> Officina::Menu() const {
    std::vector<MenuO> m;
    MenuO g{ "Gioco", {} };
    g.voci = { { "Nuovo gioco...", "Ctrl+N", "nuovo" }, { "Apri gioco...", "Ctrl+O", "apri" }, { "Salva gioco", "Ctrl+S", "salva" },
               { "Salva con nome", "Ctrl+Maiusc+S", "salvaNome" }, { "Prova il gioco", "F5", "prova" }, { "Crea il gioco .exe", "Ctrl+E", "exe" },
               { std::string("Stile grafico: ") + NomeStile(progetto.stile) + "...", "", "stile" }, { "Annulla", "Ctrl+Z", "annulla" },
               { "Ripeti", "Ctrl+Y", "ripeti" }, { "Chiave di Claude...", "", "chiave" }, { "Torna alla home", "", "home" }, { "Esci dallo studio", "", "esci" } };
    m.push_back(g);
    switch (sezione) {
        case Sezione::Mondi:
            for (int i = 1; i < EditorMondo::NumeroMenu() - 1; i++) {
                MenuO x{ EditorMondo::NomeMenu(i), {} };
                auto v = EditorMondo::VociDelMenu(i);
                for (int k = 0; k < (int)v.size(); k++) {
                    if (i == 2 && v[k].first.find("Claude") != std::string::npos) continue;
                    VoceO voce{ v[k].first, v[k].second, "ed:" + std::to_string(i) + ":" + std::to_string(k) };
                    voce.spunta = editorMondo.VoceAttiva(i, k);
                    x.voci.push_back(voce);
                }
                m.push_back(x);
            }
            break;
        case Sezione::Personaggi:
            m.push_back({ "Personaggio", { { "Nuovo personaggio", "", "p:nuovo" }, { "Duplica", "", "p:duplica" }, { "Elimina", "", "p:elimina" },
                                           { "Usa come eroe del gioco", "", "p:eroe" }, { "Metti nella mappa aperta", "", "p:mappa" },
                                           { "Esporta il foglio PNG", "", "png" }, { "Esporta il personaggio (.json)", "", "esporta" } } });
            break;
        case Sezione::Oggetti:
            m.push_back({ "Oggetto", { { "Nuovo oggetto", "", "o:nuovo" }, { "Duplica", "", "o:duplica" }, { "Elimina", "", "o:elimina" },
                                       { "Metti nella mappa aperta", "", "o:mappa" }, { "Esporta il foglio PNG", "", "png" }, { "Esporta l'oggetto (.json)", "", "esporta" } } });
            break;
        case Sezione::Scenari:
            m.push_back({ "Scenario", { { "Nuovo scenario", "", "s:nuovo" }, { "Duplica", "", "s:duplica" }, { "Elimina", "", "s:elimina" },
                                        { "Usa nella mappa aperta", "", "s:usa" }, { "Aggiungi uno strato", "", "s:strato" }, { "Esporta lo scenario (.json)", "", "esporta" } } });
            m.push_back({ "Caselle", { { "Disegna le caselle", "", "c:apri" }, { "Nuova casella", "", "c:nuova" }, { "Elimina la casella", "", "c:elimina" } } });
            break;
        case Sezione::Audio:
            m.push_back({ "Musica", { { "Nuovo brano vuoto", "", "b:nuovo" }, { "Componi un brano", "", "b:componi" }, { "Duplica", "", "b:duplica" },
                                      { "Elimina", "", "b:elimina" }, { "Ascolta / ferma", "Spazio", "b:suona" }, { "Usa nella mappa aperta", "", "b:usa" },
                                      { "Esporta in WAV", "", "b:wav" } } });
            m.push_back({ "Effetti", { { "Nuovo effetto", "", "e:nuovo" }, { "Effetto a caso", "", "e:caso" }, { "Duplica", "", "e:duplica" },
                                       { "Elimina", "", "e:elimina" }, { "Ascolta", "", "e:suona" }, { "Esporta in WAV", "", "e:wav" } } });
            break;
        case Sezione::Logica:
            m.push_back({ "Regole", { { "Nuova regola", "", "r:nuova" }, { "Duplica", "", "r:duplica" }, { "Elimina", "", "r:elimina" },
                                      { "Sposta su", "", "r:su" }, { "Sposta giu'", "", "r:giu" }, { "Aggiungi le regole d'esempio", "", "r:esempi" } } });
            break;
        case Sezione::Librerie:
            m.push_back({ "Libreria", { { "Esporta l'elemento scelto (.json)", "", "esporta" }, { "Aggiungi la raccolta scelta", "", "l:raccolta" } } });
            break;
        case Sezione::Comandi:
            m.push_back({ "Comandi", { { "Ripristina i comandi predefiniti", "", "k:predefiniti" }, { "Salva i comandi", "", "k:salva" } } });
            break;
        default: break;
    }
    m.push_back({ "Aiuto", { { "Guida rapida", "F1", "aiuto" }, { "Chat con Claude", "F3", "claude" }, { "Mostra / nascondi la chat", "F2", "chat" } } });
    return m;
}

void Officina::BarraMenu() {
    float s = Ui::S();
    Rectangle r = rBarra;
    DrawRectangleRec(r, Hex(0x080a1c));
    DrawLineEx({ r.x, r.y + r.height }, { r.x + r.width, r.y + r.height }, 1, Ui::Bordo());
    // Marchio.
    DrawRectangleRounded({ r.x + 8 * s, r.y + 6 * s, 22 * s, r.height - 12 * s }, 0.3f, 6, Ui::Accento());
    Ui::TestoCentrato("SV", { r.x + 8 * s, r.y + 6 * s, 22 * s, r.height - 12 * s }, Hex(0x1a1030), (int)(11 * s));
    auto menu = Menu();
    float x = r.x + 38 * s;
    for (int i = 0; i < (int)menu.size(); i++) {
        float w = Ui::Larghezza(menu[i].titolo, Ui::TP()) + 24 * s;
        Rectangle v = { x, r.y + 3 * s, w, r.height - 6 * s };
        xMenu[std::min(i, 15)] = x;
        bool sopra = Ui::MouseSu(v);
        if (menuAperto == i || sopra) DrawRectangleRounded(v, 0.3f, 6, menuAperto == i ? Ui::Selezione() : Hex(0x1c2150));
        Ui::TestoCentrato(menu[i].titolo, v, i == 0 ? Ui::Accento() : menuAperto == i ? Ui::Accento() : Ui::TestoChiaro(), Ui::TP());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { menuAperto = menuAperto == i ? -1 : i; gAudio.Suona(Effetto::Menu, 0.3f); }
        else if (sopra && menuAperto >= 0) menuAperto = i;
        x += w + 2 * s;
    }
    std::string t = progetto.mondo.titolo + (modificato || editorMondo.Modificato() ? "  *" : "") + "   -   " + NomeGenere(progetto.mondo.genere) +
                    "   -   " + NomeStile(progetto.stile);
    Ui::Testo(t, x + 20 * s, r.y + (r.height - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    std::string stato = cfg.chiave.empty() ? "Claude: manca la chiave" : chatInAttesa ? "Claude sta pensando..." : "Claude pronto";
    Color cc = cfg.chiave.empty() ? Hex(0xff8a6a) : chatInAttesa ? Ui::Accento() : Hex(0x7ee0a0);
    float wc = (float)Ui::Larghezza(stato, Ui::TP());
    DrawCircle((int)(r.x + r.width - wc - 64 * s), (int)(r.y + r.height / 2), 4 * s, cc);
    Ui::Testo(stato, r.x + r.width - wc - 54 * s, r.y + (r.height - Ui::TP()) / 2, cc, Ui::TP());
    postoAudio = { r.x + r.width - 34 * s, r.y + 3 * s, r.height - 6 * s, r.height - 6 * s };
}

void Officina::MenuTendina() {
    if (menuAperto < 0) return;
    auto menu = Menu();
    if (menuAperto >= (int)menu.size()) { menuAperto = -1; return; }
    float s = Ui::S();
    const auto& voci = menu[menuAperto].voci;
    float h = 32 * s, w = 340 * s;
    Rectangle r = { xMenu[std::min(menuAperto, 15)], rBarra.y + rBarra.height, w, h * voci.size() + 8 * s };
    DrawRectangleRec({ r.x + 5 * s, r.y + 5 * s, r.width, r.height }, Hex(0x000000, 120));
    DrawRectangleRec(r, Ui::Pannello());
    DrawRectangleLinesEx(r, 1, Ui::Bordo());
    for (size_t k = 0; k < voci.size(); k++) {
        Rectangle v = { r.x + 4 * s, r.y + 4 * s + k * h, r.width - 8 * s, h };
        bool sopra = Ui::MouseSu(v);
        if (sopra) DrawRectangleRec(v, Ui::Selezione());
        std::string testo = (voci[k].spunta ? "[x] " : "") + voci[k].testo;
        Ui::Testo(testo, v.x + 10 * s, v.y + (h - Ui::TP()) / 2, sopra ? WHITE : Ui::TestoChiaro(), Ui::TP());
        Ui::Testo(voci[k].tasto, v.x + v.width - Ui::Larghezza(voci[k].tasto, Ui::TP()) - 10 * s, v.y + (h - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
        if (sopra && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            menuAperto = -1;
            gAudio.Suona(Effetto::Menu, 0.35f);
            Azione(voci[k].codice);
            return;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), rBarra) && !CheckCollisionPointRec(GetMousePosition(), r)) menuAperto = -1;
    if (IsKeyPressed(KEY_ESCAPE)) menuAperto = -1;
}

void Officina::Schede() {
    float s = Ui::S();
    Rectangle r = rSchede;
    DrawRectangleRec(r, Hex(0x0b0d22));
    DrawLineEx({ r.x, r.y + r.height }, { r.x + r.width, r.y + r.height }, 1, Ui::Bordo());
    float x = r.x + 10 * s;
    for (int i = 0; i < (int)Sezione::Totale; i++) {
        float w = Ui::Larghezza(NOMI_SEZIONI[i], Ui::T()) + 54 * s;
        Rectangle v = { x, r.y + 6 * s, w, r.height - 6 * s };
        bool attiva = (int)sezione == i, sopra = Ui::MouseSu(v);
        DrawRectangleRounded({ v.x, v.y, v.width, v.height + 10 * s }, 0.25f, 6, attiva ? Ui::Fondo() : sopra ? Hex(0x1a1f48) : Hex(0x12153a));
        if (attiva) DrawRectangle((int)v.x + 6, (int)v.y, (int)v.width - 12, (int)(3 * s), Ui::Accento());
        Color c = attiva ? Ui::Accento() : sopra ? WHITE : Ui::TestoSpento();
        IconaSezione(i, { v.x + 10 * s, v.y + (v.height - 22 * s) / 2, 22 * s, 22 * s }, c);
        Ui::Testo(NOMI_SEZIONI[i], v.x + 40 * s, v.y + (v.height - Ui::T()) / 2, attiva ? WHITE : c, Ui::T());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !attiva) { Vai((Sezione)i); gAudio.Suona(Effetto::Menu, 0.35f); }
        if (sopra && TASTI_SEZIONI[i][0]) Ui::Testo(TASTI_SEZIONI[i], v.x + v.width - Ui::Larghezza(TASTI_SEZIONI[i], (int)(10 * s)) - 6 * s, v.y + 2 * s, Ui::TestoSpento(), (int)(10 * s));
        x += w + 4 * s;
    }
    // Pulsanti rapidi a destra: prova e .exe.
    float bw = 120 * s, bh = r.height - 14 * s;
    if (x < r.x + r.width - 2 * bw - 30 * s) {
        if (Ui::Pulsante({ r.x + r.width - 2 * bw - 20 * s, r.y + 7 * s, bw, bh }, "Prova  F5", true)) AvviaProva();
        if (Ui::Pulsante({ r.x + r.width - bw - 10 * s, r.y + 7 * s, bw, bh }, "Crea .exe")) CreaEseguibile();
    }
}

void Officina::Azione(const std::string& c) {
    if (c.rfind("ed:", 0) == 0) {
        int m = std::atoi(c.c_str() + 3);
        int v = std::atoi(c.c_str() + c.find(':', 3) + 1);
        editorMondo.Esegui(m, v);
        return;
    }
    if (c == "nuovo") finestra = 1;
    else if (c == "apri") ApriElenco();
    else if (c == "salva") Salva(false);
    else if (c == "salvaNome") Salva(true);
    else if (c == "prova") AvviaProva();
    else if (c == "exe") CreaEseguibile();
    else if (c == "stile") finestra = 5;
    else if (c == "annulla") Annulla();
    else if (c == "ripeti") Ripeti();
    else if (c == "chiave") { finestra = 6; bozzaChiave.clear(); }
    else if (c == "home") Vai(Sezione::Home);
    else if (c == "esci") esci = true;
    else if (c == "aiuto") finestra = 3;
    else if (c == "claude") { chatAperta = true; Ui::DaiFuoco("chat"); }
    else if (c == "chat") chatAperta = !chatAperta;
    else if (c == "esporta") {
        std::string j = ElementoJson();
        if (j.empty()) { Avvisa("Scegli prima un elemento"); return; }
        std::string cartella = CartellaEseguibile() + "/esportati";
        MakeDirectory(cartella.c_str());
        std::string nome = NomeFileDaTitolo(ElementoAperto().empty() ? "elemento" : ElementoAperto());
        std::string f = cartella + "/" + nome + ".json";
        if (SaveFileText(f.c_str(), &j[0])) Avvisa("Esportato in esportati/" + nome + ".json (trascinalo in un altro gioco per importarlo)");
    } else if (c == "png") {
        const Disegno* d = nullptr;
        if (sezione == Sezione::Personaggi && selPersonaggio < (int)progetto.personaggi.size()) d = &progetto.personaggi[selPersonaggio].disegno;
        if (sezione == Sezione::Oggetti && selOggetto < (int)progetto.oggetti.size()) d = &progetto.oggetti[selOggetto].disegno;
        if (!d) return;
        std::string cartella = CartellaEseguibile() + "/esportati";
        MakeDirectory(cartella.c_str());
        std::string nome = NomeFileDaTitolo(ElementoAperto()) + ".png";
        EditorDisegno tmp;
        tmp.Apri(const_cast<Disegno*>(d), "esporta");
        if (tmp.EsportaPng(cartella + "/" + nome)) Avvisa("Foglio di sprite salvato in esportati/" + nome);
    } else {
        // Azioni delle sezioni (officina_sezioni.cpp e simili): le gestiscono i pulsanti delle sezioni,
        // qui si ripetono le piu' comuni.
        AzioneSezione(c);
    }
}

// --- Finestre ------------------------------------------------------------------------------------

void Officina::Finestre() {
    if (finestra == 0) return;
    float s = Ui::S();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Hex(0x000000, 150));
    Ui::ApriModale();
    float fw = std::min(W - 40 * s, 860 * s), fh = std::min(H - 40 * s, 620 * s);
    if (finestra == 6 || finestra == 5) fh = 360 * s;
    Rectangle f = { (W - fw) / 2, (H - fh) / 2, fw, fh };
    DrawRectangleRounded(f, 0.03f, 8, Ui::Pannello());
    DrawRectangleRoundedLinesEx(f, 0.03f, 8, 2, Ui::Accento());
    float pad = 22 * s, x = f.x + pad, y = f.y + pad, w = f.width - 2 * pad, h = 34 * s;
    bool chiudi = IsKeyPressed(KEY_ESCAPE) && !Ui::StaScrivendo();
    if (Ui::Pulsante({ f.x + f.width - 44 * s, f.y + 10 * s, 34 * s, 30 * s }, "X")) chiudi = true;
    switch (finestra) {
        case 1: {   // nuovo gioco
            Ui::Testo("Nuovo gioco", x, y, Ui::Accento(), (int)(26 * s));
            y += 46 * s;
            Ui::Testo("Titolo", x, y + 8 * s, Ui::TestoSpento(), Ui::TP());
            Ui::Campo("nuovo_titolo", { x + 160 * s, y, w - 160 * s, h }, titoloNuovo, 60);
            y += h + 18 * s;
            std::vector<std::string> generi;
            for (int i = 0; i < (int)Genere::Totale; i++) generi.push_back(NomeGenere((Genere)i));
            Ui::Testo("Genere", x, y, Ui::TestoSpento(), Ui::TP());
            y += 24 * s;
            float bw = (w - 5 * 8 * s) / 6;
            for (int i = 0; i < (int)generi.size(); i++)
                if (Ui::Pulsante({ x + i * (bw + 8 * s), y, bw, 46 * s }, generi[i], genereNuovo == i)) genereNuovo = i;
            y += 64 * s;
            Ui::Testo("Stile grafico", x, y, Ui::TestoSpento(), Ui::TP());
            y += 24 * s;
            const char* stili[] = { "Pixel art fine", "Disegnato in alta definizione" };
            const char* desc[] = { "Pixel netti, il doppio dei dettagli di prima: personaggi 32x32 in una casella.",
                                   "Disegni morbidi con sfumature e contorni lisci, a quattro volte la risoluzione." };
            for (int i = 0; i < 2; i++) {
                Rectangle b = { x + i * (w / 2 + 4 * s), y, w / 2 - 4 * s, 90 * s };
                if (Ui::Pulsante(b, "", stileNuovo == i)) stileNuovo = i;
                Ui::Testo(stili[i], b.x + 14 * s, b.y + 12 * s, stileNuovo == i ? Ui::Accento() : WHITE, Ui::T());
                Ui::TestoACapo(desc[i], b.x + 14 * s, b.y + 40 * s, b.width - 28 * s, Ui::TestoSpento(), Ui::TP());
            }
            y += 110 * s;
            Ui::TestoACapo("Il gioco nuovo parte con la raccolta Base (eroe, nemici, oggetti, scenario, musica e suoni). "
                           "Dopo puoi aggiungere altre raccolte dalla sezione Librerie o chiedere a Claude di creare tutto quello che vuoi.",
                           x, y, w, Ui::TestoSpento(), Ui::TP());
            if (Ui::Pulsante({ f.x + f.width - pad - 200 * s, f.y + f.height - pad - 44 * s, 200 * s, 44 * s }, "Crea il gioco", true)) {
                NuovoGioco((Genere)genereNuovo, stileNuovo ? StileGrafico::Disegnato : StileGrafico::PixelFine, titoloNuovo);
                Vai(Sezione::Mondi);
                finestra = 0;
            }
            break;
        }
        case 2: {   // apri
            Ui::Testo("Apri un gioco", x, y, Ui::Accento(), (int)(26 * s));
            y += 44 * s;
            std::vector<Mondo> esempi = MondiDiProva();
            Rectangle lista = { x, y, w, f.y + f.height - pad - y };
            if (Ui::MouseSu(lista)) scorriElenco = std::max(0.0f, scorriElenco - GetMouseWheelMove() * 60 * s);
            BeginScissorMode((int)lista.x, (int)lista.y, (int)lista.width, (int)lista.height);
            float yy = y - scorriElenco;
            Ui::Testo("I TUOI GIOCHI (cartella giochi)", x, yy, Ui::TestoSpento(), Ui::TP());
            yy += 26 * s;
            if (elencoGiochi.empty()) { Ui::Testo("Nessun gioco salvato: crea un gioco e premi Ctrl+S.", x, yy, Ui::TestoSpento(), Ui::TP()); yy += 30 * s; }
            for (auto& g : elencoGiochi) {
                Rectangle b = { x, yy, w, h };
                if (Ui::Pulsante(b, std::string(GetFileName(g.c_str()))) && CheckCollisionPointRec(GetMousePosition(), lista)) {
                    if (ApriFile(g)) { finestra = 0; Vai(Sezione::Mondi); Avvisa("Aperto " + std::string(GetFileName(g.c_str()))); }
                }
                yy += h + 6 * s;
            }
            yy += 14 * s;
            Ui::Testo("GIOCHI D'ESEMPIO", x, yy, Ui::TestoSpento(), Ui::TP());
            yy += 26 * s;
            float bw = (w - 8 * s) / 2;
            for (int i = 0; i < (int)esempi.size(); i++) {
                Rectangle b = { x + (i % 2) * (bw + 8 * s), yy + (i / 2) * (h + 6 * s), bw, h };
                std::string t = esempi[i].titolo + "  (" + NomeGenere(esempi[i].genere) + ")";
                if (Ui::Pulsante(b, t) && CheckCollisionPointRec(GetMousePosition(), lista)) {
                    ProgettoGioco p;
                    std::string e;
                    if (p.Carica(esempi[i].Json(), e)) {
                        progetto = p;
                        percorso.clear();
                        annulla.clear(); ripeti.clear();
                        editorDisegno.Chiudi();
                        editorMondo.Apri(progetto.mondo, "", "");
                        LibreriaCambiata();
                        modificato = true;
                        finestra = 0;
                        Vai(Sezione::Mondi);
                        Avvisa("Esempio aperto: salvalo con Ctrl+S per farne il tuo gioco");
                    }
                }
            }
            EndScissorMode();
            break;
        }
        case 3: {   // aiuto
            Ui::Testo("Guida rapida", x, y, Ui::Accento(), (int)(26 * s));
            y += 46 * s;
            std::string t =
                "MENU GIOCO (in alto a sinistra): nuovo, apri, salva gioco (Ctrl+S), prova (F5), crea il .exe (Ctrl+E), stile grafico, chiave di Claude.\n"
                "SEZIONI (Alt+1...8): Mondi (le mappe a blocchi), Personaggi (disegno, animazioni e comportamento), Oggetti (disegno e funzione), "
                "Scenari (sfondi a strati e caselle), Audio (musica a note, effetti, suoni degli eventi), Logica (regole quando... allora...), "
                "Librerie (tutto il gioco e le 22 raccolte pronte), Comandi (tastiera, mouse e joypad).\n"
                "CLAUDE e' nella colonna a destra in ogni sezione (F3 per scrivere, F2 per nasconderla): sa in che sezione sei e cosa hai aperto. "
                "Chiedigli di disegnare un personaggio, comporre una musica, scrivere regole o costruire una mappa.\n"
                "DISEGNO: P matita, E gomma, F secchiello, L linea, R rettangolo, O ellisse, I contagocce, M sposta; [ e ] punta; X scambia i colori; "
                ", e . sfogliano i fotogrammi; rotella zoom; Ctrl+Z annulla.\n"
                "IMPORTA: trascina sulla finestra un PNG (diventa fotogrammi; un foglio di sprite si divide da solo), un WAV/OGG/MP3 (effetto o brano), "
                "un .json esportato da un altro gioco o un .gioco.\n"
                "Il gioco creato parte con il logo Elvantis, il nome dello sviluppatore e la musica, poi il menu: 1 o 2 giocatori, opzioni, crediti.";
            Ui::TestoACapo(t, x, y, w, Ui::TestoChiaro(), Ui::TP());
            break;
        }
        case 5: {   // stile grafico
            Ui::Testo("Stile grafico del gioco", x, y, Ui::Accento(), (int)(26 * s));
            y += 46 * s;
            Ui::TestoACapo(std::string("Ora: ") + NomeStile(progetto.stile) + ". Cambiando stile i disegni della libreria vengono convertiti "
                           "(pixel art -> disegno HD: ingranditi e ammorbiditi; HD -> pixel art: rimpiccioliti). Si puo' annullare con Ctrl+Z.",
                           x, y, w, Ui::TestoChiaro(), Ui::TP());
            y += 80 * s;
            for (int i = 0; i < 2; i++) {
                StileGrafico st = i ? StileGrafico::Disegnato : StileGrafico::PixelFine;
                if (Ui::Pulsante({ x + i * (w / 2 + 4 * s), y, w / 2 - 4 * s, 50 * s }, NomeStile(st), progetto.stile == st) && progetto.stile != st) {
                    Ricorda();
                    progetto.stile = st;
                    auto Converti = [&](Disegno& d) {
                        if (st == StileGrafico::Disegnato && !d.liscio) DisegnoInHD(d);
                        else if (st == StileGrafico::PixelFine && d.liscio) {
                            for (auto& a : d.animazioni)
                                for (auto& fo : a.fotogrammi) {
                                    Fotogramma g;
                                    g.Nuovo(std::max(1, fo.w / 2), std::max(1, fo.h / 2));
                                    for (int yy = 0; yy < g.h; yy++) for (int xx = 0; xx < g.w; xx++) g.Metti(xx, yy, fo.Prendi(xx * 2, yy * 2 + 1));
                                    fo = g;
                                }
                            d.w = std::max(1, d.w / 2); d.h = std::max(1, d.h / 2);
                            d.densita = std::max(1.0f, d.densita / 2);
                            d.liscio = false;
                            d.Invalida();
                        }
                    };
                    for (auto& p : progetto.personaggi) Converti(p.disegno);
                    for (auto& o : progetto.oggetti) Converti(o.disegno);
                    for (auto& c : progetto.caselle) Converti(c.disegno);
                    editorDisegno.Chiudi();
                    LibreriaCambiata();
                    Avvisa(std::string("Stile: ") + NomeStile(st));
                    finestra = 0;
                }
            }
            break;
        }
        case 6: {   // chiave
            Ui::Testo("Chiave di Claude", x, y, Ui::Accento(), (int)(26 * s));
            y += 46 * s;
            Ui::TestoACapo("Copia la chiave API dalla console di Anthropic (inizia con sk-ant-), clicca nel campo, Ctrl+V e premi Salva. "
                           "Resta nel file claude_chiave.txt accanto allo studio (non viene mai messa nei giochi creati).",
                           x, y, w, Ui::TestoChiaro(), Ui::TP());
            y += 70 * s;
            Ui::Campo("chiave_api", { x, y, w - 130 * s, h }, bozzaChiave, 300);
            if (Ui::Pulsante({ x + w - 120 * s, y, 120 * s, h }, "Salva", true)) {
                std::string k = bozzaChiave;
                while (!k.empty() && (k.back() == ' ' || k.back() == '\n' || k.back() == '\r')) k.pop_back();
                while (!k.empty() && k[0] == ' ') k.erase(0, 1);
                if (k.rfind("sk-ant-", 0) != 0) Avvisa("La chiave deve iniziare con sk-ant-");
                else if (SaveFileText((CartellaEseguibile() + "/claude_chiave.txt").c_str(), &k[0])) {
                    cfg.chiave = k; bozzaChiave.clear(); finestra = 0;
                    Avvisa("Chiave salvata: ora Claude ti aiuta in ogni sezione!");
                    gAudio.Suona(Effetto::Gemma);
                } else Avvisa("Non riesco a salvare claude_chiave.txt");
            }
            break;
        }
        default: break;
    }
    Ui::ChiudiModale();
    if (chiudi) { finestra = 0; Ui::TogliFuoco(); }
}

// --- Home ---------------------------------------------------------------------------------------

void Officina::Home(Rectangle r) {
    float s = Ui::S();
    sfondoHome.Aggiorna(GetFrameTime());
    // Paesaggio in alto, disegnato a risoluzione piena.
    Rectangle alto = { r.x, r.y, r.width, std::min(r.height * 0.36f, 300 * s) };
    BeginScissorMode((int)alto.x, (int)alto.y, (int)alto.width, (int)alto.height);
    float scala = alto.width / VW;
    float hv = alto.height / scala;
    float salva = gScalaHD;
    gScalaHD = scala;
    BeginMode2D(CameraHD({ alto.x / scala, alto.y / scala + (hv - VH) / 2 }, { 0, 0 }, 1));
    sfondoHome.Disegna(tempo * 18, 0);
    EndMode2D();
    gScalaHD = salva;
    EndScissorMode();
    DrawRectangleGradientV((int)alto.x, (int)(alto.y + alto.height * 0.5f), (int)alto.width, (int)(alto.height * 0.5f) + 1, Hex(0x0d0f24, 0), Ui::Fondo());
    Ui::TestoForte("Studio Videogiochi", r.x + 40 * s, r.y + 34 * s, WHITE, (int)(48 * s));
    Ui::Testo("Elvantis  -  sviluppatore Mario Petramale", r.x + 44 * s, r.y + 92 * s, Hex(0xffd2b8), Ui::T());
    Ui::TestoForte(progetto.mondo.titolo, r.x + 44 * s, r.y + 124 * s, Ui::Accento(), (int)(26 * s));
    Ui::Testo(TextFormat("%d mappe, %d personaggi, %d oggetti, %d scenari, %d brani, %d effetti, %d regole", (int)progetto.mondo.mappe.size(),
                         (int)progetto.personaggi.size(), (int)progetto.oggetti.size(), (int)progetto.scenari.size(), (int)progetto.brani.size(),
                         (int)progetto.effetti.size(), (int)progetto.regole.size()),
              r.x + 44 * s, r.y + 158 * s, Ui::TestoChiaro(), Ui::TP());
    float y = alto.y + alto.height + 10 * s;
    // Azioni principali.
    float bw = 220 * s, bh = 52 * s, x = r.x + 40 * s;
    if (Ui::Pulsante({ x, y, bw, bh }, "Nuovo gioco", true)) finestra = 1;
    if (Ui::Pulsante({ x + bw + 12 * s, y, bw, bh }, "Apri un gioco")) ApriElenco();
    if (Ui::Pulsante({ x + 2 * (bw + 12 * s), y, bw, bh }, "Salva gioco")) Salva(false);
    if (Ui::Pulsante({ x + 3 * (bw + 12 * s), y, bw, bh }, "Prova il gioco  F5")) AvviaProva();
    y += bh + 24 * s;
    // Le sezioni come carte.
    const char* desc[] = { "", "Le mappe a blocchi: caselle, oggetti, porte, scroll, telecamera",
                           "Disegna, anima e dai un comportamento ai personaggi", "Oggetti disegnati con la loro funzione nel gioco",
                           "Sfondi a strati, cielo, pioggia e neve, caselle disegnate", "Musica a note, effetti sonori, suoni degli eventi",
                           "Regole: quando succede qualcosa, allora...", "La libreria del gioco e le 22 raccolte pronte",
                           "Tastiera, mouse e joypad" };
    int col = r.width > 1300 * s ? 4 : 3;
    float gap = 14 * s, cw = (r.width - 80 * s - (col - 1) * gap) / col, ch = 92 * s;
    for (int i = 1; i < (int)Sezione::Totale; i++) {
        Rectangle c = { x + ((i - 1) % col) * (cw + gap), y + ((i - 1) / col) * (ch + gap), cw, ch };
        if (c.y + c.height > r.y + r.height) break;
        bool sopra = Ui::MouseSu(c);
        DrawRectangleRounded(c, 0.12f, 8, sopra ? Hex(0x1f2556) : Ui::Pannello());
        DrawRectangleRoundedLinesEx(c, 0.12f, 8, sopra ? 2.0f : 1.0f, sopra ? Ui::Accento() : Ui::Bordo());
        IconaSezione(i, { c.x + 14 * s, c.y + 16 * s, 34 * s, 34 * s }, sopra ? Ui::Accento() : Hex(0x9aa4ff));
        Ui::Testo(NOMI_SEZIONI[i], c.x + 60 * s, c.y + 14 * s, WHITE, Ui::T());
        Ui::TestoACapo(desc[i], c.x + 60 * s, c.y + 40 * s, c.width - 72 * s, Ui::TestoSpento(), Ui::TP());
        if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) Vai((Sezione)i);
    }
    // Giochi recenti.
    int righe = ((int)Sezione::Totale - 2) / col + 1;
    y += righe * (ch + gap) + 6 * s;
    if (!recenti.empty() && y < r.y + r.height - 60 * s) {
        Ui::Testo("Giochi recenti", x, y, Ui::TestoSpento(), Ui::TP());
        y += 24 * s;
        float rx = x;
        for (auto& g : recenti) {
            std::string n = GetFileNameWithoutExt(g.c_str());
            float w = Ui::Larghezza(n, Ui::TP()) + 30 * s;
            if (rx + w > r.x + r.width - 20 * s) break;
            if (Ui::Pulsante({ rx, y, w, 34 * s }, n, g == percorso) && FileExists(g.c_str()) && ApriFile(g)) Avvisa("Aperto " + n);
            rx += w + 8 * s;
        }
    }
}

// --- Claude ----------------------------------------------------------------------------------------

std::string Officina::NomeSezione() const {
    switch (sezione) {
        case Sezione::Mondi: return "mondi";
        case Sezione::Personaggi: return "personaggi";
        case Sezione::Oggetti: return "oggetti";
        case Sezione::Scenari: return schedaScenari == 1 ? "scenari (caselle)" : "scenari";
        case Sezione::Audio: return schedaAudio == 0 ? "audio (musica)" : schedaAudio == 1 ? "audio (effetti)" : "audio (suoni degli eventi)";
        case Sezione::Logica: return "logica";
        case Sezione::Librerie: return "librerie";
        case Sezione::Comandi: return "comandi";
        default: return "home";
    }
}

std::string Officina::ElementoAperto() const {
    switch (sezione) {
        case Sezione::Personaggi: return selPersonaggio < (int)progetto.personaggi.size() ? progetto.personaggi[selPersonaggio].nome : "";
        case Sezione::Oggetti: return selOggetto < (int)progetto.oggetti.size() ? progetto.oggetti[selOggetto].nome : "";
        case Sezione::Scenari: return selScenario < (int)progetto.scenari.size() ? progetto.scenari[selScenario].nome : "";
        case Sezione::Audio:
            if (schedaAudio == 0) return selBrano < (int)progetto.brani.size() ? progetto.brani[selBrano].nome : "";
            return selEffetto < (int)progetto.effetti.size() ? progetto.effetti[selEffetto].nome : "";
        case Sezione::Logica: return selRegola < (int)progetto.regole.size() ? "regola " + std::to_string(selRegola) : "";
        case Sezione::Mondi: { int m = editorMondo.MappaAperta(); return m < (int)progetto.mondo.mappe.size() ? "mappa " + progetto.mondo.mappe[m].nome : ""; }
        default: return "";
    }
}

std::vector<std::string> Officina::SuggerimentiClaude() const {
    switch (sezione) {
        case Sezione::Personaggi: return { "Disegna un drago verde che sputa fuoco, con animazioni di volo e attacco, come boss",
                                           "Rendi questo personaggio piu' veloce e fagli inseguire l'eroe",
                                           "Crea un mago amico che dice un indovinello" };
        case Sezione::Oggetti: return { "Crea una chiave d'oro e una porta che si apre con quella chiave", "Disegna una piattaforma di legno che si muove",
                                        "Fai una pozione rossa che ridà 2 cuori" };
        case Sezione::Scenari: return { "Crea uno scenario notturno con la neve e montagne blu", "Fai un tramonto viola con nuvole e colline",
                                        "Disegna le caselle di terra come pietre di un castello" };
        case Sezione::Audio: return { "Componi un tema epico per il boss, 16 battute", "Crea un effetto laser e uno per le esplosioni",
                                      "Fai una musica allegra con melodia, basso e batteria" };
        case Sezione::Logica: return { "Quando l'eroe raccoglie 10 monete, apri la porta e suona una fanfara",
                                       "Ogni 5 secondi crea un pipistrello vicino all'eroe", "Quando il boss e' sconfitto, vittoria" };
        case Sezione::Librerie: return { "Aggiungi la raccolta Fantasy e metti il drago nella mappa", "Che personaggi ci sono nel gioco?",
                                         "Crea tre nemici diversi per un livello di ghiaccio" };
        case Sezione::Mondi: return { "Aggiungi piattaforme e 10 monete fino alla bandiera", "Metti il drago della libreria alla fine del livello",
                                      "Crea una seconda mappa nel castello collegata con una porta" };
        default: return { "Fammi un platform completo con un drago come boss finale", "Come si crea un personaggio animato?",
                          "Cosa posso fare in questo studio?" };
    }
}

void Officina::InviaChat(const std::string& testo) {
    if (testo.empty() || chatInAttesa) return;
    if (cfg.chiave.empty()) { finestra = 6; Avvisa("Prima serve la chiave API di Claude"); return; }
    SincronizzaMondo();
    // Si tengono gli ultimi scambi (lo stato completo va comunque con l'ultima richiesta).
    while (chat.storia.size() > 12) chat.storia.erase(chat.storia.begin(), chat.storia.begin() + 2);
    chat.Invia(cfg, testo, StatoProgettoPerClaude(progetto, editorMondo.MappaAperta(), NomeSezione(), ElementoAperto()), PromptProgetto());
    chatInAttesa = true;
    tChat = 0;
    scorriChat = 0;
    gAudio.Suona(Effetto::Conferma, 0.5f);
}

void Officina::ControllaChat() {
    if (!chatInAttesa) return;
    int st = chat.Stato();
    if (st != 2 && st != 3) return;
    chatInAttesa = false;
    if (st == 3) {
        chat.storia.push_back({ false, "Non riesco a parlare con Claude: " + chat.RispostaGrezza(), "{\"risposta\": \"errore di rete\", \"azioni\": []}" });
        return;
    }
    std::string grezza = chat.RispostaGrezza();
    SincronizzaMondo();
    ProgettoGioco prima = progetto;
    EsitoProgetto e = ApplicaRispostaProgetto(progetto, editorMondo.MappaAperta(), grezza);
    std::string mostra = e.risposta.empty() ? "(nessun messaggio)" : e.risposta;
    if (e.modificato) {
        annulla.push_back(prima);
        ripeti.clear();
        modificato = true;
        mostra += "\n[Fatto: ";
        for (size_t i = 0; i < e.fatte.size(); i++) mostra += (i ? "; " : "") + e.fatte[i];
        mostra += ". Gioco > Annulla per tornare indietro]";
        gAudio.Suona(Effetto::Gemma);
        MondoCambiato();
        LibreriaCambiata();
        // Si mostra cio' che Claude ha appena fatto.
        auto Indice = [](const auto& v, const std::string& n) { for (int i = 0; i < (int)v.size(); i++) if (v[i].nome == n) return i; return -1; };
        if (e.sezione == "personaggi") { int i = Indice(progetto.personaggi, e.nome); if (i >= 0) selPersonaggio = i; if (sezione != Sezione::Mondi) Vai(Sezione::Personaggi); }
        else if (e.sezione == "oggetti") { int i = Indice(progetto.oggetti, e.nome); if (i >= 0) selOggetto = i; if (sezione != Sezione::Mondi) Vai(Sezione::Oggetti); }
        else if (e.sezione == "scenari") { int i = Indice(progetto.scenari, e.nome); if (i >= 0) selScenario = i; }
        else if (e.sezione == "audio") {
            int i = Indice(progetto.brani, e.nome); if (i >= 0) { selBrano = i; schedaAudio = 0; }
            int k = Indice(progetto.effetti, e.nome); if (k >= 0) { selEffetto = k; schedaAudio = 1; }
        }
        if (e.apriMappa >= 0) { Vai(Sezione::Mondi); editorMondo.MostraMappa(e.apriMappa); }
    }
    if (!e.scartate.empty()) {
        mostra += "\n[Non applicato: ";
        for (size_t i = 0; i < e.scartate.size(); i++) mostra += (i ? "; " : "") + e.scartate[i];
        mostra += "]";
    }
    chat.storia.push_back({ false, mostra, grezza });
}

void Officina::PannelloChat(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x10132e));
    DrawLineEx({ r.x, r.y }, { r.x, r.y + r.height }, 1, Ui::Bordo());
    if (!chatAperta) {
        Rectangle b = { r.x + 3 * s, r.y + 8 * s, r.width - 6 * s, 120 * s };
        if (Ui::Pulsante(b, "")) chatAperta = true;
        DrawCircle((int)(b.x + b.width / 2), (int)(b.y + 18 * s), 6 * s, Hex(0xd97757));
        const char* lettere = "CLAUDE";
        for (int i = 0; i < 6; i++) Ui::TestoCentrato(std::string(1, lettere[i]), { b.x, b.y + 30 * s + i * 14 * s, b.width, 14 * s }, Ui::TestoChiaro(), (int)(12 * s));
        return;
    }
    float pad = 10 * s, hr = 32 * s;
    float x = r.x + pad, w = r.width - 2 * pad, y = r.y + pad;
    DrawCircle((int)(x + 7 * s), (int)(y + 10 * s), 7 * s, Hex(0xd97757));
    Ui::Testo("Claude", x + 20 * s, y + 2 * s, Hex(0xd97757), Ui::T());
    Ui::Testo("in " + NomeSezione(), x + 20 * s + Ui::Larghezza("Claude ", Ui::T()), y + 4 * s, Ui::TestoSpento(), Ui::TP());
    if (Ui::Pulsante({ r.x + r.width - pad - 26 * s, y, 26 * s, 24 * s }, ">")) chatAperta = false;
    if (Ui::Pulsante({ r.x + r.width - pad - 100 * s, y, 68 * s, 24 * s }, "Pulisci", false, !chat.storia.empty() && !chatInAttesa)) { chat.storia.clear(); scorriChat = 0; }
    y += 34 * s;
    if (cfg.chiave.empty()) {
        Rectangle b = { x, y, w, 64 * s };
        DrawRectangleRounded(b, 0.1f, 6, Hex(0x3a2030));
        Ui::TestoACapo("Per parlare con Claude serve la chiave API.", b.x + 8 * s, b.y + 6 * s, b.width - 16 * s, WHITE, Ui::TP());
        if (Ui::Pulsante({ b.x + 8 * s, b.y + 32 * s, 160 * s, 26 * s }, "Inserisci la chiave", true)) { finestra = 6; bozzaChiave.clear(); }
        y += b.height + 8 * s;
    }
    std::vector<std::string> idee = SuggerimentiClaude();
    float hIdee = 0;
    if (bozzaChat.empty() && !chatInAttesa) {
        for (auto& i : idee) hIdee += Ui::TestoACapo(i, 0, 0, w - 16 * s, WHITE, Ui::TP(), false) + 14 * s;
        hIdee += 22 * s;
    }
    Rectangle input = { x, r.y + r.height - hr - pad, w - 70 * s, hr };
    Rectangle storia = { x, y, w, input.y - hIdee - 8 * s - y };
    DrawRectangleRounded(storia, 0.02f, 6, Hex(0x0b0d22));
    float larg = storia.width - 4 * pad;
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
    BeginScissorMode((int)storia.x, (int)storia.y, (int)storia.width, (int)std::max(1.0f, storia.height));
    if (chat.storia.empty() && !chatInAttesa)
        Ui::TestoACapo("Claude vede tutto il gioco: mappe, personaggi, oggetti, scenari, musica, regole. Scrivi in italiano cosa vuoi: "
                       "crea, modifica, spiega. Ogni modifica si annulla dal menu Gioco.",
                       storia.x + pad, storia.y + pad, storia.width - 2 * pad, Ui::TestoSpento(), Ui::TP());
    float yy = tot < storia.height ? storia.y : storia.y + storia.height - tot + scorriChat;
    yy += pad;
    for (size_t i = 0; i < chat.storia.size(); i++) {
        const auto& msg = chat.storia[i];
        float h = alt[i];
        if (yy + h > storia.y && yy < storia.y + storia.height) {
            float bw = msg.utente ? std::min(larg, (float)Ui::Larghezza(msg.testo, Ui::TP())) + 2 * pad : larg + 2 * pad;
            Rectangle b = { msg.utente ? storia.x + storia.width - bw - pad : storia.x + pad, yy, bw, h };
            DrawRectangleRounded(b, std::min(0.3f, 12 * s / h), 6, msg.utente ? Hex(0x2e3680) : Hex(0x262038));
            float ty = b.y + pad;
            if (!msg.utente) { Ui::Testo("Claude", b.x + pad, ty, Hex(0xd97757), Ui::TP()); ty += Ui::TP() + 4 * s; }
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
            DrawCircle((int)(b.x + 30 * s + k * 18 * s), (int)(b.y + b.height / 2 - 4 * s * std::max(0.0f, std::sin(tChat * 6 - k))), 4 * s, Hex(0xd97757));
        Ui::Testo(TextFormat("%.0fs", tChat), b.x + 100 * s, b.y + 9 * s, Ui::TestoSpento(), Ui::TP());
    }
    EndScissorMode();
    if (hIdee > 0) {
        float iy = storia.y + storia.height + 8 * s;
        Ui::Testo("Prova a chiedere:", x, iy, Ui::TestoSpento(), Ui::TP());
        iy += 20 * s;
        for (auto& idea : idee) {
            float h = Ui::TestoACapo(idea, 0, 0, w - 16 * s, WHITE, Ui::TP(), false) + 8 * s;
            Rectangle b = { x, iy, w, h };
            bool sopra = Ui::MouseSu(b);
            DrawRectangleRounded(b, 0.2f, 6, sopra ? Hex(0x252b5c) : Hex(0x161a3a));
            Ui::TestoACapo(idea, b.x + 8 * s, b.y + 4 * s, w - 16 * s, sopra ? WHITE : Ui::TestoChiaro(), Ui::TP());
            if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { bozzaChat = idea; Ui::DaiFuoco("chat"); }
            iy += h + 6 * s;
        }
    }
    bool invio = Ui::Campo("chat", input, bozzaChat, 800) && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER));
    if (Ui::Fuoco() != "chat" && bozzaChat.empty()) Ui::Testo("Scrivi a Claude...  (F3)", input.x + 8 * s, input.y + (hr - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    bool premuto = Ui::Pulsante({ input.x + input.width + 6 * s, input.y, 64 * s, hr }, chatInAttesa ? "..." : "Invia", true, !chatInAttesa);
    if ((invio || premuto) && !bozzaChat.empty() && !chatInAttesa) {
        std::string t = bozzaChat;
        bozzaChat.clear();
        InviaChat(t);
        if (cfg.chiave.empty()) bozzaChat = t;
    }
}

// --- Importazioni ---------------------------------------------------------------------------------

std::string Officina::ElementoJson() const {
    json j;
    switch (sezione) {
        case Sezione::Personaggi: if (selPersonaggio < (int)progetto.personaggi.size()) j = { { "personaggio", PersonaggioJson(progetto.personaggi[selPersonaggio]) } }; break;
        case Sezione::Oggetti: if (selOggetto < (int)progetto.oggetti.size()) j = { { "oggetto", OggettoJson(progetto.oggetti[selOggetto]) } }; break;
        case Sezione::Scenari: if (selScenario < (int)progetto.scenari.size()) j = { { "scenario", ScenarioJson(progetto.scenari[selScenario]) } }; break;
        case Sezione::Audio:
            if (schedaAudio == 0 && selBrano < (int)progetto.brani.size()) j = { { "brano", BranoJson(progetto.brani[selBrano]) } };
            else if (selEffetto < (int)progetto.effetti.size()) j = { { "effetto", EffettoJson(progetto.effetti[selEffetto]) } };
            break;
        case Sezione::Logica: if (selRegola < (int)progetto.regole.size()) j = { { "regola", RegolaJson(progetto.regole[selRegola]) } }; break;
        default: break;
    }
    if (j.is_null()) return "";
    j["formato"] = "studio-videogiochi/elemento-1";
    return j.dump(1);
}

bool Officina::ImportaElemento(const std::string& testo, std::string& msg) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { msg = "il file non e' un JSON valido"; return false; }
    std::string errore;
    Ricorda();
    if (j.contains("personaggio")) {
        PersonaggioDef p;
        if (!PersonaggioDaJson(j["personaggio"], p, errore)) { msg = errore; annulla.pop_back(); return false; }
        p.nome = progetto.Personaggio(p.nome) ? progetto.NomeLibero(p.nome) : p.nome;
        progetto.personaggi.push_back(p);
        selPersonaggio = (int)progetto.personaggi.size() - 1;
        Vai(Sezione::Personaggi);
        msg = "Importato il personaggio " + p.nome;
    } else if (j.contains("oggetto")) {
        OggettoDef o;
        if (!OggettoDaJson(j["oggetto"], o, errore)) { msg = errore; annulla.pop_back(); return false; }
        o.nome = progetto.Oggetto(o.nome) ? progetto.NomeLibero(o.nome) : o.nome;
        progetto.oggetti.push_back(o);
        selOggetto = (int)progetto.oggetti.size() - 1;
        Vai(Sezione::Oggetti);
        msg = "Importato l'oggetto " + o.nome;
    } else if (j.contains("scenario")) {
        Scenario sc;
        if (!ScenarioDaJson(j["scenario"], sc, errore)) { msg = errore; annulla.pop_back(); return false; }
        sc.nome = progetto.TrovaScenario(sc.nome) ? progetto.NomeLibero(sc.nome) : sc.nome;
        progetto.scenari.push_back(sc);
        selScenario = (int)progetto.scenari.size() - 1;
        Vai(Sezione::Scenari);
        msg = "Importato lo scenario " + sc.nome;
    } else if (j.contains("brano")) {
        Brano b;
        if (!BranoDaJson(j["brano"], b, errore)) { msg = errore; annulla.pop_back(); return false; }
        b.nome = progetto.TrovaBrano(b.nome) ? progetto.NomeLibero(b.nome) : b.nome;
        progetto.brani.push_back(b);
        selBrano = (int)progetto.brani.size() - 1; schedaAudio = 0;
        Vai(Sezione::Audio);
        msg = "Importato il brano " + b.nome;
    } else if (j.contains("effetto")) {
        EffettoSuono e;
        if (!EffettoDaJson(j["effetto"], e, errore)) { msg = errore; annulla.pop_back(); return false; }
        e.nome = progetto.TrovaEffetto(e.nome) ? progetto.NomeLibero(e.nome) : e.nome;
        progetto.effetti.push_back(e);
        selEffetto = (int)progetto.effetti.size() - 1; schedaAudio = 1;
        Vai(Sezione::Audio);
        msg = "Importato l'effetto " + e.nome;
    } else if (j.contains("regola")) {
        Regola r;
        if (!RegolaDaJson(j["regola"], r, errore)) { msg = errore; annulla.pop_back(); return false; }
        progetto.regole.push_back(r);
        selRegola = (int)progetto.regole.size() - 1;
        Vai(Sezione::Logica);
        msg = "Importata una regola";
    } else if (j.contains("fotogrammi") || j.contains("animazioni")) {
        // Uno sprite fatto con Claude (vecchio formato: palette e righe): diventa un personaggio.
        PersonaggioDef p;
        if (!DisegnoDaJson(j, p.disegno, errore)) { msg = errore; annulla.pop_back(); return false; }
        if (progetto.stile == StileGrafico::Disegnato && !p.disegno.liscio) DisegnoInHD(p.disegno);
        std::string nome = j.contains("nome") && j["nome"].is_string() ? j["nome"].get<std::string>() : "Sprite di Claude";
        p.nome = progetto.NomeLibero(nome);
        p.autore = "Claude";
        progetto.personaggi.push_back(p);
        selPersonaggio = (int)progetto.personaggi.size() - 1;
        Vai(Sezione::Personaggi);
        msg = "Lo sprite e' diventato il personaggio " + p.nome + ": dagli un comportamento e mettilo nelle mappe!";
    } else { annulla.pop_back(); msg = "non riconosco il contenuto del file"; return false; }
    LibreriaCambiata();
    return true;
}

void Officina::ImportaFile(const std::string& file) {
    std::string e = Estensione(file), msg;
    if (e == ".gioco") { if (ApriFile(file)) { Vai(Sezione::Mondi); Avvisa("Aperto " + std::string(GetFileName(file.c_str()))); } return; }
    if (e == ".json") {
        std::string t = LeggiTesto(file);
        json j = json::parse(t, nullptr, false);
        if (!j.is_discarded() && j.is_object() && (j.contains("mappe") || j.contains("mondo"))) {
            if (ApriFile(file)) { Vai(Sezione::Mondi); Avvisa("Aperto " + std::string(GetFileName(file.c_str()))); }
            return;
        }
        if (ImportaElemento(t, msg)) Avvisa(msg); else Avvisa("Non importato: " + msg);
        return;
    }
    if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".gif" || e == ".tga" || e == ".qoi") {
        // Nelle sezioni di disegno diventa fotogrammi; altrove uno strato dello scenario o un nuovo personaggio.
        if ((sezione == Sezione::Personaggi || sezione == Sezione::Oggetti || (sezione == Sezione::Scenari && schedaScenari == 1)) && !editorDisegno.Id().empty()) {
            Ricorda();
            if (editorDisegno.Importa(file, msg)) { Avvisa(msg); LibreriaCambiata(); }
            else { annulla.pop_back(); Avvisa(msg); }
            return;
        }
        std::string dati = LeggiTesto(file);
        std::string nome = GetFileName(file.c_str());
        if (sezione == Sezione::Scenari && selScenario < (int)progetto.scenari.size()) {
            Ricorda();
            AggiungiFileImmagine(nome, dati);
            bool c = false;
            for (auto& im : progetto.immagini) if (im.first == nome) { im.second = dati; c = true; }
            if (!c) progetto.immagini.push_back({ nome, dati });
            Texture2D t = Immagine(nome);
            StratoSfondo st;
            st.nome = GetFileNameWithoutExt(nome.c_str());
            st.tipo = "immagine";
            st.immagine = nome;
            st.altezza = 1;
            st.parallasseX = 0.3f; st.parallasseY = 0.1f;
            st.scala = t.id ? std::max(0.25f, (float)VH / t.height) : 1;
            progetto.scenari[selScenario].strati.push_back(st);
            stratoScelto = (int)progetto.scenari[selScenario].strati.size() - 1;
            Avvisa("Nuovo strato dall'immagine " + nome);
            return;
        }
        // Altrove: un nuovo personaggio con quell'immagine.
        Ricorda();
        PersonaggioDef p;
        p.nome = progetto.NomeLibero(GetFileNameWithoutExt(file.c_str()));
        p.disegno.Nuovo(32, 32, 2);
        if (progetto.stile == StileGrafico::Disegnato) { p.disegno.Nuovo(64, 64, 4); p.disegno.liscio = true; }
        progetto.personaggi.push_back(p);
        selPersonaggio = (int)progetto.personaggi.size() - 1;
        Vai(Sezione::Personaggi);
        editorDisegno.Apri(&progetto.personaggi[selPersonaggio].disegno, "p:" + p.nome);
        editorDisegno.Importa(file, msg);
        LibreriaCambiata();
        Avvisa("Nuovo personaggio " + p.nome + ": " + msg);
        return;
    }
    if (e == ".wav" || e == ".ogg" || e == ".mp3" || e == ".qoa" || e == ".flac") {
        std::string dati = LeggiTesto(file);
        Ricorda();
        // Brevi = effetti, lunghi (oltre 1 MB o OGG/MP3 lunghi) = musica. Nella scheda effetti diventa sempre un effetto.
        bool musica = (schedaAudio == 0 && sezione == Sezione::Audio) || dati.size() > 600000;
        if (musica) {
            Brano b;
            b.nome = progetto.NomeLibero(GetFileNameWithoutExt(file.c_str()));
            b.file = GetFileName(file.c_str());
            b.dati = dati;
            b.autore = "importato";
            progetto.brani.push_back(b);
            selBrano = (int)progetto.brani.size() - 1; schedaAudio = 0;
            Avvisa("Musica importata: " + b.nome);
        } else {
            EffettoSuono ef;
            ef.nome = progetto.NomeLibero(GetFileNameWithoutExt(file.c_str()));
            ef.file = GetFileName(file.c_str());
            ef.dati = dati;
            ef.autore = "importato";
            progetto.effetti.push_back(ef);
            selEffetto = (int)progetto.effetti.size() - 1; schedaAudio = 1;
            Avvisa("Effetto importato: " + ef.nome);
        }
        Vai(Sezione::Audio);
        return;
    }
    Avvisa("Non so importare " + std::string(GetFileName(file.c_str())));
}
