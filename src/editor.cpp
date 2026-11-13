#include "editor.h"
#include "audio.h"
#include "periferiche.h"
#include "sistema.h"
#include "json.hpp"
#include <cctype>

using nlohmann::json;

namespace {

constexpr int MAPPA_Y = 16;          // riga 0 della mappa sullo schermo virtuale
constexpr int BARRA_ALTA = 18;
constexpr int BARRA_BASSA = VH - 14;

struct Strumento { char c; const char* nome; };
const Strumento STRUMENTI[] = {
    { '#', "Terra" }, { '=', "Pietra" }, { '-', "Piattaforma" }, { '^', "Spine" },
    { 'o', "Moneta" }, { 'g', "Gemma" }, { 'm', "Molla" }, { 'c', "Forziere" }, { 'k', "Lanterna" },
    { 'e', "Slime" }, { 'b', "Pipistrello" }, { 'P', "Partenza" }, { 'F', "Bandiera" }, { ' ', "Gomma" } };
constexpr int NUM_STRUMENTI = 14;

const float ORE[] = { 0.35f, 0.72f, 0.0f, 0.25f };
const char* NOMI_ORE[] = { "giorno", "tramonto", "notte", "alba" };

bool EOggetto(char c) { return std::string("ogmckebPF").find(c) != std::string::npos; }

std::string Slug(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (std::isalnum((unsigned char)c)) r += c;
        else if (!r.empty() && r.back() != '_') r += '_';
    }
    while (!r.empty() && r.back() == '_') r.pop_back();
    return r.empty() ? std::string("livello") : r;
}

std::string Minuscolo(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }

bool EstensioneAudio(const std::string& est) {
    return est == ".wav" || est == ".ogg" || est == ".mp3" || est == ".qoa";
}

// Righe di testo spezzate per stare in una larghezza (per la chat a risoluzione piena).
std::vector<std::string> ACapo(const std::string& testo, int larghezza, int dim) {
    std::vector<std::string> righe;
    std::string riga, parola;
    for (size_t i = 0; i <= testo.size(); i++) {
        char ch = i < testo.size() ? testo[i] : ' ';
        if (ch == '\n') {
            std::string prova = riga.empty() ? parola : riga + " " + parola;
            righe.push_back(prova); riga.clear(); parola.clear();
            continue;
        }
        if (ch != ' ') { parola += ch; continue; }
        std::string prova = riga.empty() ? parola : riga + " " + parola;
        if (!riga.empty() && MeasureText(prova.c_str(), dim) > larghezza) { righe.push_back(riga); riga = parola; }
        else riga = prova;
        parola.clear();
    }
    if (!riga.empty()) righe.push_back(riga);
    if (righe.empty()) righe.push_back("");
    return righe;
}

// Le lettere accentate non sono nel carattere di sistema: si scrivono con l'apostrofo.
void AggiungiCarattere(std::string& s, int c, size_t massimo) {
    if (s.size() >= massimo) return;
    if (c >= 32 && c < 127) { s.push_back((char)c); return; }
    const char* base = nullptr;
    switch (c) {
        case 0xE0: case 0xC0: base = "a"; break;
        case 0xE8: case 0xE9: case 0xC8: case 0xC9: base = "e"; break;
        case 0xEC: case 0xCC: base = "i"; break;
        case 0xF2: case 0xD2: base = "o"; break;
        case 0xF9: case 0xD9: base = "u"; break;
    }
    if (base) { s += base; s += '\''; }
}

} // namespace

// ---------------------------------------------------------------------------

void Editor::Inizializza() {
    cfg = ConfigClaude::Carica();
    prog.Nuovo(80);
    prog.titolo = "Il mio livello";
    Ricostruisci();
}

void Editor::Entra() { vuoleUscire = false; ApplicaAudio(); }

void Editor::Esci() { gAudio.FermaTutto(); }

void Editor::Apri(const Progetto& p, const std::string& m) {
    Ricorda();
    prog = p;
    camX = 0;
    modificato = false;
    percorsoFile.clear();
    Ricostruisci();
    ApplicaAudio();
    Avvisa(m);
}

std::string Editor::Cartella() const { return CartellaEseguibile() + "/livelli"; }
std::string Editor::NomeFile() const { return Cartella() + "/" + Slug(prog.titolo) + ".txt"; }

void Editor::Ricostruisci() {
    std::string err;
    vistaValida = vista.Carica(prog.Testo(), err);
    sfondo.tema = prog.tema;
    sfondo.ora = prog.ora;
}

void Editor::Ricorda() {
    annulla.push_back(prog);
    if (annulla.size() > 60) annulla.erase(annulla.begin());
}

void Editor::Metti(int x, int y, char c) {
    if (y < 0 || y >= Progetto::ALTEZZA || x < 0 || x >= prog.Larghezza()) return;
    char& cella = prog.righe[y][x];
    if (cella == c) return;
    if (cella == 'P' && c != 'P') { Avvisa("La partenza si sposta: scegli lo strumento Partenza e clicca altrove"); return; }
    if (c == 'P' || c == 'F')
        for (auto& r : prog.righe) for (auto& k : r) if (k == c) k = ' ';
    cella = c;
    modificato = true;
    Ricostruisci();
}

void Editor::AssegnaFileAudio(const std::string& percorso) {
    std::string est = Minuscolo(GetFileExtension(percorso.c_str()));
    if (!EstensioneAudio(est)) { Avvisa("Formato non supportato: usa WAV, OGG, MP3 o QOA"); return; }
    int dim = 0;
    unsigned char* dati = LoadFileData(percorso.c_str(), &dim);
    if (!dati) { Avvisa("Non riesco a leggere il file"); return; }
    std::string contenuto((const char*)dati, (size_t)dim);
    UnloadFileData(dati);
    std::string base = sceltaAudio == 0 ? std::string("musica") : std::string("effetto_") + Audio::NomeEffetto((Effetto)(sceltaAudio - 1));
    for (size_t i = 0; i < prog.file.size();)
        if (prog.file[i].first.rfind(base + ".", 0) == 0) prog.file.erase(prog.file.begin() + i); else i++;
    prog.file.push_back({ base + est, contenuto });
    modificato = true;
    ApplicaAudio();
    bool ok = sceltaAudio == 0 ? gAudio.MusicaDaFile() : gAudio.EffettoDaFile((Effetto)(sceltaAudio - 1));
    Avvisa(ok ? std::string("Caricato ") + GetFileName(percorso.c_str()) : std::string("File audio non valido"));
    if (ok && sceltaAudio > 0) gAudio.Suona((Effetto)(sceltaAudio - 1));
}

void Editor::ApplicaAudio() { gAudio.ApplicaFile(prog.file); }

void Editor::Salva() {
    MakeDirectory(Cartella().c_str());
    percorsoFile = NomeFile();
    bool ok = SaveFileText(percorsoFile.c_str(), (char*)prog.Testo().c_str());
    if (!prog.file.empty()) {
        std::string dir = Cartella() + "/" + Slug(prog.titolo) + "_audio";
        MakeDirectory(dir.c_str());
        for (const auto& f : prog.file)
            ok &= SaveFileData((dir + "/" + f.first).c_str(), (void*)f.second.data(), (int)f.second.size());
    }
    if (ok) { modificato = false; Avvisa("Salvato in livelli/" + Slug(prog.titolo) + ".txt"); gAudio.Suona(Effetto::Conferma); }
    else Avvisa("Salvataggio non riuscito");
}

void Editor::ApriElenco() {
    fileElenco = { "[Nuovo livello vuoto]", "[La valle delle gemme]", "[Vetrina degli oggetti]" };
    if (DirectoryExists(Cartella().c_str())) {
        FilePathList l = LoadDirectoryFilesEx(Cartella().c_str(), ".txt", false);
        for (unsigned int i = 0; i < l.count; i++) fileElenco.push_back(l.paths[i]);
        UnloadDirectoryFiles(l);
    }
    sceltaElenco = 0;
    elenco = true;
}

void Editor::CreaEseguibile() {
#ifdef _WIN32
    std::string nome = Slug(prog.titolo) + ".exe";
#else
    std::string nome = Slug(prog.titolo);
#endif
    if (!vistaValida) { Avvisa("Il livello non e' valido: serve la partenza"); return; }
    std::string errore;
    DatiGioco d{ prog.titolo, prog.Testo(), prog.file };
    if (::CreaEseguibile(CartellaEseguibile() + "/" + nome, SerializzaGioco(d), errore)) {
        Avvisa("Creato " + nome + " accanto allo studio!");
        gAudio.Suona(Effetto::Vittoria);
    } else Avvisa("Errore: " + errore);
}

void Editor::AvviaProva() {
    std::string err;
    prova.titolo = prog.titolo;
    prova.crediti = "Studio Videogiochi - sviluppatore Mario Petramale";
    if (prova.Carica(prog.Testo(), err)) { provando = true; gAudio.Suona(Effetto::Conferma); }
    else Avvisa("Non posso provarlo: " + err);
}

void Editor::ApplicaRisposta(const std::string& testo) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        chat.storia.push_back({ false, "Non ho capito la risposta, puoi riprovare?", testo });
        return;
    }
    std::string risposta = j.value("risposta", std::string(""));
    std::vector<std::string> fatte;
    if (j.contains("azioni") && j["azioni"].is_array() && !j["azioni"].empty()) {
        Ricorda();
        for (auto& a : j["azioni"]) {
            std::string tipo = a.value("tipo", std::string(""));
            if (tipo == "livello" && a.contains("righe") && a["righe"].is_array()) {
                std::string mappa;
                for (auto& r : a["righe"]) if (r.is_string()) mappa += r.get<std::string>() + "\n";
                Progetto prova2 = prog;
                std::string err;
                Livello verifica;
                if (prova2.DaTesto(mappa, err) && verifica.Carica(prova2.Testo(), err)) {
                    prog.righe = prova2.righe;
                    fatte.push_back("livello");
                } else fatte.push_back("livello non valido (" + err + ")");
            } else if (tipo == "titolo" && a.contains("valore")) {
                prog.titolo = a.value("valore", prog.titolo); fatte.push_back("titolo");
            } else if (tipo == "scenario") {
                if (a.contains("tema")) prog.tema = (int)Limita(a.value("tema", 0.0f), 0, (float)Sfondo::Temi().size() - 1);
                if (a.contains("ora")) prog.ora = std::fmod(Limita(a.value("ora", 0.35f), 0, 1), 1.0f);
                fatte.push_back("scenario");
            } else if (tipo == "eroe") {
                Color* campi[] = { &prog.eroe.pelle, &prog.eroe.capelli, &prog.eroe.maglia, &prog.eroe.mantello, &prog.eroe.pantaloni, &prog.eroe.scarpe };
                const char* nomi[] = { "pelle", "capelli", "maglia", "mantello", "pantaloni", "scarpe" };
                for (int i = 0; i < 6; i++) if (a.contains(nomi[i])) TestoInColore(a.value(nomi[i], std::string()), *campi[i]);
                fatte.push_back("eroe");
            } else if (tipo == "fisica") {
                if (a.contains("gravita")) prog.fisica.gravita = Limita(a.value("gravita", 900.0f), 300, 2000);
                if (a.contains("salto")) prog.fisica.salto = Limita(a.value("salto", 305.0f), 150, 600);
                if (a.contains("velocita")) prog.fisica.velocita = Limita(a.value("velocita", 115.0f), 40, 300);
                if (a.contains("molla")) prog.fisica.molla = Limita(a.value("molla", 470.0f), 250, 900);
                fatte.push_back("fisica");
            }
        }
        modificato = true;
        Ricostruisci();
    }
    std::string mostra = risposta;
    if (!fatte.empty()) {
        mostra += "\n[Modificato: ";
        for (size_t i = 0; i < fatte.size(); i++) mostra += (i ? ", " : "") + fatte[i];
        mostra += ". Ctrl+Z per annullare]";
        gAudio.Suona(Effetto::Gemma);
    }
    chat.storia.push_back({ false, mostra, testo });
}

// ---------------------------------------------------------------------------

void Editor::Aggiorna(float dt) {
    tempo += dt;
    if (tMessaggio > 0) tMessaggio -= dt;
    sfondo.Aggiorna(dt);
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    int joy = gInput.Joypad();

    if (chatInAttesa && chat.Stato() >= 2) {
        chatInAttesa = false;
        if (chat.Stato() == 3) chat.storia.push_back({ false, "Errore: " + chat.RispostaGrezza(), "(errore)" });
        else ApplicaRisposta(chat.RispostaGrezza());
        scorriChat = 0;
    }

    // File trascinati sulla finestra: audio al canale scelto, .txt come livello.
    if (IsFileDropped()) {
        FilePathList l = LoadDroppedFiles();
        for (unsigned int i = 0; i < l.count; i++) {
            std::string p = l.paths[i], est = Minuscolo(GetFileExtension(l.paths[i]));
            if (EstensioneAudio(est)) { if (!pannelloAudio) { pannelloAudio = true; } AssegnaFileAudio(p); }
            else if (est == ".txt") {
                char* t = LoadFileText(p.c_str());
                Progetto np = prog; np.file.clear();
                std::string err;
                if (t && np.DaTesto(t, err)) Apri(np, std::string("Aperto ") + GetFileName(p.c_str()));
                else Avvisa("Livello non valido: " + err);
                if (t) UnloadFileText(t);
            } else Avvisa("Trascina file .wav .ogg .mp3 (audio) o .txt (livelli)");
        }
        UnloadDroppedFiles(l);
    }

    if (provando) {
        prova.Aggiorna(dt, Comandi::DaPeriferiche());
        if (IsKeyPressed(KEY_R)) { std::string e; prova.Carica(prog.Testo(), e); }
        if (gInput.Premuto(Azione::Indietro) || IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_F5) ||
            (joy >= 0 && IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_MIDDLE_LEFT))) provando = false;
        return;
    }

    if (rinomina) {
        int c = GetCharPressed();
        while (c > 0) { AggiungiCarattere(prog.titolo, c, 40); c = GetCharPressed(); }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !prog.titolo.empty()) prog.titolo.pop_back();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F2)) {
            if (prog.titolo.empty()) prog.titolo = "Il mio livello";
            rinomina = false; modificato = true;
        }
        return;
    }
    if (aiuto) { if (gInput.Premuto(Azione::Indietro) || IsKeyPressed(KEY_F1) || gInput.Premuto(Azione::Conferma)) aiuto = false; return; }
    if (elenco) {
        int n = (int)fileElenco.size();
        if (gInput.Premuto(Azione::Giu)) sceltaElenco = (sceltaElenco + 1) % n;
        if (gInput.Premuto(Azione::Su)) sceltaElenco = (sceltaElenco + n - 1) % n;
        Vector2 m = gInput.Mouse();
        int sopra = -1;
        for (int i = 0; i < n && i < 12; i++) if (CheckCollisionPointRec(m, { 100, 52.0f + i * 14, 280, 13 })) sopra = i;
        if (sopra >= 0 && gInput.MouseMosso()) sceltaElenco = sopra;
        bool apri = gInput.Premuto(Azione::Conferma) || (sopra >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        if (apri) {
            Progetto np;
            std::string err, scelto = fileElenco[sceltaElenco];
            bool ok = true;
            if (sceltaElenco == 0) { np.Nuovo(80); np.titolo = "Nuovo livello"; }
            else if (sceltaElenco == 1) { ok = np.DaTesto(LivelloAvventura(), err); np.titolo = "La valle delle gemme"; }
            else if (sceltaElenco == 2) { ok = np.DaTesto(LivelloVetrinaOggetti(), err); np.titolo = "Vetrina degli oggetti"; }
            else {
                char* t = LoadFileText(scelto.c_str());
                ok = t && np.DaTesto(t, err);
                if (t) UnloadFileText(t);
                std::string dir = scelto.substr(0, scelto.size() - 4) + "_audio";
                if (ok && DirectoryExists(dir.c_str())) {
                    FilePathList l = LoadDirectoryFiles(dir.c_str());
                    for (unsigned int i = 0; i < l.count; i++) {
                        int dim = 0;
                        unsigned char* d = LoadFileData(l.paths[i], &dim);
                        if (d) { np.file.push_back({ GetFileName(l.paths[i]), std::string((const char*)d, (size_t)dim) }); UnloadFileData(d); }
                    }
                    UnloadDirectoryFiles(l);
                }
            }
            if (ok) { Apri(np, "Aperto: " + np.titolo); if (sceltaElenco >= 3) percorsoFile = scelto; }
            else Avvisa("Non riesco ad aprirlo: " + err);
            elenco = false;
        }
        if (gInput.Premuto(Azione::Indietro)) elenco = false;
        return;
    }
    if (pannelloAudio) {
        int n = 1 + (int)Effetto::Totale;
        if (gInput.Premuto(Azione::Giu)) sceltaAudio = (sceltaAudio + 1) % n;
        if (gInput.Premuto(Azione::Su)) sceltaAudio = (sceltaAudio + n - 1) % n;
        if (gInput.Premuto(Azione::Conferma) && sceltaAudio > 0) gAudio.Suona((Effetto)(sceltaAudio - 1));
        if (IsKeyPressed(KEY_DELETE)) {
            std::string base = sceltaAudio == 0 ? std::string("musica") : std::string("effetto_") + Audio::NomeEffetto((Effetto)(sceltaAudio - 1));
            for (size_t i = 0; i < prog.file.size();)
                if (prog.file[i].first.rfind(base + ".", 0) == 0) prog.file.erase(prog.file.begin() + i); else i++;
            ApplicaAudio();
            Avvisa("Ripristinato il suono generato");
        }
        if (gInput.Premuto(Azione::Indietro) || IsKeyPressed(KEY_F4)) pannelloAudio = false;
        return;
    }

    // --- Comandi generali ---
    if (!fuocoChat) {
        if (gInput.Premuto(Azione::Indietro)) { vuoleUscire = true; return; }
        if (IsKeyPressed(KEY_Q)) strumento = (strumento + NUM_STRUMENTI - 1) % NUM_STRUMENTI;
        if (IsKeyPressed(KEY_E) && !ctrl) strumento = (strumento + 1) % NUM_STRUMENTI;
        for (int i = 0; i < 10; i++) if (IsKeyPressed(KEY_ONE + i) && !ctrl) strumento = i;
        if (IsKeyPressed(KEY_ZERO)) strumento = 9;
        if (IsKeyPressed(KEY_T)) { Ricorda(); prog.tema = (prog.tema + 1) % (int)Sfondo::Temi().size(); Ricostruisci(); modificato = true; Avvisa(std::string("Tema: ") + Sfondo::Temi()[prog.tema].nome); }
        if (IsKeyPressed(KEY_Y)) {
            Ricorda();
            int k = 0;
            for (int i = 0; i < 4; i++) if (std::fabs(prog.ora - ORE[i]) < 0.01f) k = (i + 1) % 4;
            prog.ora = ORE[k]; Ricostruisci(); modificato = true; Avvisa(std::string("Ora: ") + NOMI_ORE[k]);
        }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) { Ricorda(); prog.Ridimensiona(prog.Larghezza() + 10); Ricostruisci(); Avvisa(TextFormat("Larghezza: %d tile", prog.Larghezza())); }
        if (IsKeyPressed(KEY_LEFT_BRACKET)) { Ricorda(); prog.Ridimensiona(prog.Larghezza() - 10); Ricostruisci(); Avvisa(TextFormat("Larghezza: %d tile", prog.Larghezza())); }
        if (IsKeyPressed(KEY_TAB)) AvviaProva();
    }
    if (IsKeyPressed(KEY_F5) || (joy >= 0 && IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_MIDDLE_LEFT))) AvviaProva();
    if (IsKeyPressed(KEY_F1)) aiuto = true;
    if (IsKeyPressed(KEY_F2)) rinomina = true;
    if (IsKeyPressed(KEY_F3) || (joy >= 0 && IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_RIGHT_FACE_UP))) { chatAperta = !chatAperta; fuocoChat = chatAperta; }
    if (IsKeyPressed(KEY_F4)) pannelloAudio = true;
    if (ctrl && IsKeyPressed(KEY_S)) Salva();
    if (ctrl && IsKeyPressed(KEY_O)) ApriElenco();
    if (ctrl && IsKeyPressed(KEY_N)) { Progetto np; np.Nuovo(80); np.titolo = "Nuovo livello"; Apri(np, "Nuovo livello"); }
    if (ctrl && IsKeyPressed(KEY_E)) CreaEseguibile();
    if (ctrl && IsKeyPressed(KEY_Z) && !annulla.empty()) { prog = annulla.back(); annulla.pop_back(); Ricostruisci(); ApplicaAudio(); Avvisa("Annullato"); }

    // --- Scorrimento ---
    float maxCam = std::max(0.0f, prog.Larghezza() * (float)TILE - VW + 32);
    if (!fuocoChat) camX += gInput.Asse() * 260 * dt;
    Vector2 m = gInput.Mouse();
    bool sullaMappa = m.y >= BARRA_ALTA && m.y < BARRA_BASSA && m.x >= 0 && m.x < VW;
    if (sullaMappa) camX -= GetMouseWheelMove() * 48;
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) camX -= GetMouseDelta().x / std::max(1.0f, (float)GetScreenHeight() / VH);

    // --- Joypad: cursore a celle ---
    if (joy >= 0 && gInput.Ultima() == Periferica::Joypad) {
        ripetizione -= dt;
        int dx = (gInput.Tenuto(Azione::Destra) ? 1 : 0) - (gInput.Tenuto(Azione::Sinistra) ? 1 : 0);
        int dy = (gInput.Tenuto(Azione::Giu) ? 1 : 0) - (gInput.Tenuto(Azione::Su) ? 1 : 0);
        bool appena = gInput.Premuto(Azione::Destra) || gInput.Premuto(Azione::Sinistra) || gInput.Premuto(Azione::Su) || gInput.Premuto(Azione::Giu);
        if ((dx || dy) && (appena || ripetizione <= 0)) {
            cursoreX = (int)Limita((float)(cursoreX + dx), 0, (float)prog.Larghezza() - 1);
            cursoreY = (int)Limita((float)(cursoreY + dy), 0, Progetto::ALTEZZA - 1);
            ripetizione = appena ? 0.3f : 0.07f;
            if (IsGamepadButtonDown(joy, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) Metti(cursoreX, cursoreY, STRUMENTI[strumento].c);
            if (IsGamepadButtonDown(joy, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) Metti(cursoreX, cursoreY, ' ');
        }
        float sx = cursoreX * (float)TILE - camX;
        if (sx < 48) camX -= 48 - sx;
        if (sx > VW - 64) camX += sx - (VW - 64);
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) { Ricorda(); Metti(cursoreX, cursoreY, STRUMENTI[strumento].c); }
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) { Ricorda(); Metti(cursoreX, cursoreY, ' '); }
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) strumento = (strumento + NUM_STRUMENTI - 1) % NUM_STRUMENTI;
        if (IsGamepadButtonPressed(joy, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) strumento = (strumento + 1) % NUM_STRUMENTI;
    }
    camX = Limita(camX, 0, maxCam);

    // --- Mouse: tavolozza e disegno ---
    if (m.y >= 0 && m.y < BARRA_ALTA && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int i = (int)((m.x - 4) / 18);
        if (i >= 0 && i < NUM_STRUMENTI) { strumento = i; gAudio.Suona(Effetto::Menu, 0.5f); }
        fuocoChat = false;
    }
    if (sullaMappa) {
        int cx = (int)std::floor((m.x + camX) / TILE), cy = (int)std::floor((m.y - MAPPA_Y) / TILE);
        bool sinistro = IsMouseButtonDown(MOUSE_BUTTON_LEFT), destro = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { Ricorda(); tratto = true; fuocoChat = false; }
        char c = STRUMENTI[strumento].c;
        if (sinistro && tratto && (!EOggetto(c) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) Metti(cx, cy, c);
        if (destro && tratto) Metti(cx, cy, ' ');
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) tratto = false;
}

// ---------------------------------------------------------------------------

static void IconaStrumento(char c, float x, float y, float tempo) {
    float cx = x + 8, cy = y + 8;
    switch (c) {
        case '#': DrawRectangle((int)x + 1, (int)y + 2, 14, 12, Hex(0x7a4a2a)); DrawRectangle((int)x + 1, (int)y + 2, 14, 4, Hex(0x4caf3a)); return;
        case '=': DrawRectangle((int)x + 1, (int)y + 2, 14, 12, Hex(0x6f7590)); DrawRectangle((int)x + 1, (int)y + 7, 14, 1, Hex(0x4a4f66)); DrawRectangle((int)x + 7, (int)y + 2, 1, 5, Hex(0x4a4f66)); return;
        case '-': DrawRectangle((int)x + 1, (int)y + 6, 14, 4, Hex(0xb87a40)); DrawRectangle((int)x + 1, (int)y + 6, 14, 1, Hex(0xe0a868)); return;
        case '^': DrawTriangle({ cx, y + 3 }, { x + 2, y + 14 }, { x + 14, y + 14 }, Hex(0xc9cfdf)); return;
        case ' ': DrawLineEx({ x + 3, y + 3 }, { x + 13, y + 13 }, 2, Hex(0xff5c5c)); DrawLineEx({ x + 13, y + 3 }, { x + 3, y + 13 }, 2, Hex(0xff5c5c)); return;
        case 'P': {
            Eroe e; e.pos = { cx, y + 15 };
            Camera2D cam{}; cam.target = e.pos; cam.offset = e.pos; cam.zoom = 0.7f;
            BeginMode2D(cam); e.Disegna(tempo); EndMode2D();
            return;
        }
        default: break;
    }
    Oggetto o{};
    float zoom = 0.9f, alto = 0;
    switch (c) {
        case 'o': o.tipo = TipoOggetto::Moneta; break;
        case 'g': o.tipo = TipoOggetto::Gemma; break;
        case 'm': o.tipo = TipoOggetto::Molla; alto = 6; break;
        case 'c': o.tipo = TipoOggetto::Forziere; alto = 7; break;
        case 'k': o.tipo = TipoOggetto::Lanterna; alto = 14; zoom = 0.55f; break;
        case 'e': o.tipo = TipoOggetto::Slime; alto = 6; break;
        case 'b': o.tipo = TipoOggetto::Pipistrello; zoom = 0.75f; break;
        case 'F': o.tipo = TipoOggetto::Bandiera; alto = 25; zoom = 0.3f; break;
    }
    o.t = tempo;
    o.pos = { 1000, 1000 };
    Camera2D cam{};
    cam.target = { 0, -alto };
    cam.offset = { cx, cy };
    cam.zoom = zoom;
    BeginMode2D(cam);
    DisegnaOggetto(o, 1000, 1000, tempo, false);
    EndMode2D();
}

void Editor::Disegna() {
    if (provando) {
        prova.Disegna();
        DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 200));
        Scritta("PROVA DEL LIVELLO   R: ricomincia   Esc/Tab/F5: torna all'editor", 6, VH - 12, 10, Hex(0xffd23f));
        return;
    }
    sfondo.Disegna(camX);
    float notte = sfondo.Notte();
    if (vistaValida) {
        vista.Disegna(camX, -MAPPA_Y, tempo, notte);
        for (const auto& o : vista.Oggetti()) {
            Oggetto copia = o; copia.t = tempo + o.t;
            DisegnaOggetto(copia, camX, -MAPPA_Y, tempo, false);
        }
        Eroe e; e.aspetto = prog.eroe;
        e.pos = { std::round(vista.Partenza().x - camX), std::round(vista.Partenza().y + MAPPA_Y) };
        e.Disegna(tempo);
    }
    // Griglia leggera e fine del livello.
    float fine = prog.Larghezza() * (float)TILE - camX;
    for (int x = 0; x <= prog.Larghezza(); x++) {
        float sx = x * (float)TILE - camX;
        if (sx >= 0 && sx <= VW) DrawLine((int)sx, MAPPA_Y, (int)sx, MAPPA_Y + Progetto::ALTEZZA * TILE, Hex(0xffffff, x % 10 == 0 ? 40 : 14));
    }
    for (int y = 0; y <= Progetto::ALTEZZA; y++) DrawLine(0, MAPPA_Y + y * TILE, (int)std::min((float)VW, fine), MAPPA_Y + y * TILE, Hex(0xffffff, 14));
    if (fine < VW) { DrawRectangle((int)fine, 0, VW - (int)fine, VH, Hex(0x05081c, 200)); DrawLine((int)fine, 0, (int)fine, VH, Hex(0xff5c8a)); }

    // Cella sotto il mouse o il cursore del joypad, con l'anteprima dello strumento.
    Vector2 m = gInput.Mouse();
    int cx = -1, cy = -1;
    if (gInput.Ultima() == Periferica::Joypad) { cx = cursoreX; cy = cursoreY; }
    else if (m.y >= BARRA_ALTA && m.y < BARRA_BASSA && m.x >= 0 && m.x < VW) { cx = (int)std::floor((m.x + camX) / TILE); cy = (int)std::floor((m.y - MAPPA_Y) / TILE); }
    if (cx >= 0 && cx < prog.Larghezza() && cy >= 0 && cy < Progetto::ALTEZZA) {
        float sx = cx * (float)TILE - camX, sy = (float)MAPPA_Y + cy * TILE;
        BeginBlendMode(BLEND_ALPHA);
        DrawRectangle((int)sx, (int)sy, TILE, TILE, Hex(0xffffff, 40));
        EndBlendMode();
        IconaStrumento(STRUMENTI[strumento].c, sx, sy, tempo);
        float pulsa = 0.6f + 0.4f * std::sin(tempo * 8);
        DrawRectangleLinesEx({ sx, sy, (float)TILE, (float)TILE }, 1, ConAlfa(Hex(0xffd23f), pulsa));
        Scritta(TextFormat("%d,%d", cx, cy), (int)sx + 18, (int)sy, 10, Hex(0xffffff, 200));
    }

    // Tavolozza in alto.
    DrawRectangle(0, 0, VW, BARRA_ALTA, Hex(0x0b0e22, 230));
    for (int i = 0; i < NUM_STRUMENTI; i++) {
        float x = 4.0f + i * 18;
        if (i == strumento) { DrawRectangle((int)x - 1, 0, 18, BARRA_ALTA, Hex(0x3a4080)); DrawRectangleLines((int)x - 1, 0, 18, BARRA_ALTA, Hex(0xffd23f)); }
        IconaStrumento(STRUMENTI[i].c, x, 1, tempo);
    }
    std::string info = prog.titolo + (modificato ? " *" : "");
    Scritta(STRUMENTI[strumento].nome, 262, 4, 10, Hex(0xffd23f));
    Scritta(info.c_str(), VW - 6 - MeasureText(info.c_str(), 10), 4, 10, WHITE);

    DrawRectangle(0, VH - 14, VW, 14, Hex(0x0b0e22, 220));
    Scritta("Sx: disegna  Dx: cancella  Tab: prova  Ctrl+S: salva  Ctrl+E: crea .exe  F1: aiuto", 6, VH - 12, 10, Hex(0xc8d0f0));

    // Finestre interne.
    if (aiuto) {
        Rectangle r = { 40, 24, VW - 80, 220 };
        Pannello(r, Hex(0x0b0e22, 240), Hex(0xffd23f));
        const char* righe[] = {
            "MOUSE: sinistro disegna, destro cancella, rotella scorre, centrale trascina",
            "TASTIERA: frecce scorrono, Q/E o 1-0 scelgono lo strumento",
            "T: tema dello scenario   Y: ora del giorno   [ ]: larghezza del livello",
            "Tab o F5: prova il livello   F2: titolo   F3: chat con Claude",
            "F4: musiche ed effetti (trascina qui file WAV, OGG, MP3)",
            "Ctrl+S salva   Ctrl+O apri   Ctrl+N nuovo   Ctrl+Z annulla",
            "Ctrl+E: crea l'eseguibile del gioco accanto allo studio",
            "JOYPAD: croce sposta il cursore, A disegna, X cancella,",
            "LB/RB strumento, Select prova, Y chat, B torna al menu",
            "Esc: torna al menu" };
        Scritta("AIUTO DELL'EDITOR", (int)r.x + 10, (int)r.y + 8, 10, Hex(0xffd23f));
        for (int i = 0; i < 10; i++) DrawText(righe[i], (int)r.x + 10, (int)r.y + 28 + i * 18, 10, WHITE);
    }
    if (elenco) {
        Rectangle r = { 90, 30, 300, 210 };
        Pannello(r, Hex(0x0b0e22, 240), Hex(0xffd23f));
        Scritta("APRI UN LIVELLO", (int)r.x + 10, (int)r.y + 6, 10, Hex(0xffd23f));
        for (int i = 0; i < (int)fileElenco.size() && i < 12; i++) {
            bool sel = i == sceltaElenco;
            if (sel) DrawRectangle(100, 52 + i * 14, 280, 13, Hex(0x3a4080));
            std::string n = i < 3 ? fileElenco[i] : std::string(GetFileNameWithoutExt(fileElenco[i].c_str()));
            DrawText(n.c_str(), 106, 54 + i * 14, 10, sel ? WHITE : Hex(0xc8d0f0));
        }
        DrawText("Invio o clic: apri   Esc: chiudi", (int)r.x + 10, (int)(r.y + r.height - 16), 10, Hex(0x8a90c0));
    }
    if (rinomina) {
        Rectangle r = { 100, 100, 280, 50 };
        Pannello(r, Hex(0x0b0e22, 240), Hex(0xffd23f));
        Scritta("Titolo del gioco:", (int)r.x + 10, (int)r.y + 8, 10, Hex(0xffd23f));
        std::string t = prog.titolo + (std::fmod(tempo, 1.0f) < 0.5f ? "_" : "");
        DrawText(t.c_str(), (int)r.x + 10, (int)r.y + 26, 10, WHITE);
    }
    if (pannelloAudio) {
        Rectangle r = { 60, 24, VW - 120, 226 };
        Pannello(r, Hex(0x0b0e22, 240), Hex(0xffd23f));
        Scritta("MUSICHE ED EFFETTI DEL GIOCO", (int)r.x + 10, (int)r.y + 6, 10, Hex(0xffd23f));
        DrawText("Scegli una voce e trascina sulla finestra un file WAV, OGG o MP3", (int)r.x + 10, (int)r.y + 20, 10, Hex(0xc8d0f0));
        for (int i = 0; i <= (int)Effetto::Totale; i++) {
            bool sel = i == sceltaAudio;
            float y = r.y + 36 + i * 14;
            if (sel) DrawRectangle((int)r.x + 6, (int)y - 1, (int)r.width - 12, 13, Hex(0x3a4080));
            std::string nome = i == 0 ? "Musica" : Audio::NomeEffetto((Effetto)(i - 1));
            std::string base = i == 0 ? std::string("musica") : "effetto_" + nome, sorgente = "generato dallo studio";
            for (const auto& f : prog.file) if (f.first.rfind(base + ".", 0) == 0) sorgente = "file " + f.first;
            DrawText(nome.c_str(), (int)r.x + 12, (int)y, 10, sel ? WHITE : Hex(0xc8d0f0));
            DrawText(sorgente.c_str(), (int)r.x + 110, (int)y, 10, sorgente[0] == 'f' ? Hex(0x8cff6a) : Hex(0x8a90c0));
        }
        DrawText("Invio: ascolta   Canc: torna al suono generato   Esc: chiudi", (int)r.x + 10, (int)(r.y + r.height - 14), 10, Hex(0x8a90c0));
    }
    if (tMessaggio > 0) {
        int w = MeasureText(messaggio.c_str(), 10) + 16;
        float k = Limita(tMessaggio * 2, 0, 1);
        Rectangle r = { VW / 2.0f - w / 2.0f, 24, (float)w, 18 };
        Pannello(r, Hex(0x101430, (unsigned char)(230 * k)), ConAlfa(Hex(0xffd23f), k));
        DrawText(messaggio.c_str(), (int)r.x + 8, (int)r.y + 4, 10, ConAlfa(WHITE, k));
    }
}

// --- Chat a risoluzione piena -------------------------------------------------

void Editor::AggiornaChat(Rectangle p) {
    Vector2 m = GetMousePosition();
    bool dentro = CheckCollisionPointRec(m, p);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) fuocoChat = dentro;
    if (dentro) scorriChat = std::max(0.0f, scorriChat + GetMouseWheelMove() * 40);

    // Suggerimenti cliccabili quando la chat e' vuota.
    static const char* idee[] = { "Rendi il livello piu' difficile, con piu' nemici", "Un tramonto nel deserto",
                                  "Vesti l'eroe da cavaliere", "Gravita' lunare e salti altissimi" };
    if (chat.storia.empty() && !chatInAttesa && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        float y = p.y + 120;
        for (int i = 0; i < 4; i++) {
            Rectangle r = { p.x + 16, y + i * 44, p.width - 32, 36 };
            if (CheckCollisionPointRec(m, r)) { bozza = idee[i]; fuocoChat = true; }
        }
    }
    if (!fuocoChat || chatInAttesa) return;
    int c = GetCharPressed();
    while (c > 0) { AggiungiCarattere(bozza, c, 600); c = GetCharPressed(); }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !bozza.empty()) bozza.pop_back();
    if (IsKeyPressed(KEY_ESCAPE)) fuocoChat = false;
    if (IsKeyPressed(KEY_ENTER) && !bozza.empty()) {
        if (cfg.chiave.empty()) {
            chat.storia.push_back({ true, bozza, "" });
            chat.storia.push_back({ false, "Per parlare con me serve una chiave API di Anthropic: mettila nella variabile "
                                           "ANTHROPIC_API_KEY oppure nel file claude_chiave.txt accanto allo studio, poi riavvia.", "" });
            bozza.clear();
            return;
        }
        chat.Invia(cfg, bozza, prog.Json());
        bozza.clear();
        chatInAttesa = true;
        scorriChat = 0;
        gAudio.Suona(Effetto::Conferma, 0.6f);
    }
}

void Editor::DisegnaChat(Rectangle p) {
    const int dim = 20, riga = 24;
    DrawRectangleRec(p, Hex(0x14101f));
    DrawRectangle((int)p.x, (int)p.y, 3, (int)p.height, fuocoChat ? Hex(0xd97757) : Hex(0x3a2a4a));
    // Intestazione.
    DrawRectangle((int)p.x, (int)p.y, (int)p.width, 56, Hex(0x221a30));
    DrawCircle((int)p.x + 30, (int)p.y + 28, 14, Hex(0xd97757));
    DrawText("C", (int)p.x + 24, (int)p.y + 18, 20, WHITE);
    DrawText("Chat con Claude", (int)p.x + 54, (int)p.y + 10, dim, WHITE);
    DrawText(cfg.chiave.empty() ? "chiave API non trovata" : ("modello " + cfg.modello).c_str(), (int)p.x + 54, (int)p.y + 34, 16, Hex(0xa898c0));
    DrawText("F3 chiude", (int)(p.x + p.width - 90), (int)p.y + 34, 16, Hex(0x6a5a80));

    float inputH = 110;
    Rectangle area = { p.x + 12, p.y + 64, p.width - 24, p.height - 64 - inputH - 12 };
    int testoW = (int)area.width - 36;

    if (chat.storia.empty()) {
        DrawText("Chiedimi di modificare il gioco aperto:", (int)area.x, (int)p.y + 80, dim, Hex(0xe0d8f0));
        static const char* idee[] = { "Rendi il livello piu' difficile, con piu' nemici", "Un tramonto nel deserto",
                                      "Vesti l'eroe da cavaliere", "Gravita' lunare e salti altissimi" };
        Vector2 m = GetMousePosition();
        for (int i = 0; i < 4; i++) {
            Rectangle r = { p.x + 16, p.y + 120 + i * 44, p.width - 32, 36 };
            bool sopra = CheckCollisionPointRec(m, r);
            DrawRectangleRounded(r, 0.4f, 6, sopra ? Hex(0x3a2a50) : Hex(0x2a2040));
            DrawText(idee[i], (int)r.x + 12, (int)r.y + 9, dim, Hex(0xffd2b8));
        }
    } else {
        // Messaggi dal basso verso l'alto, con lo scorrimento della rotella.
        BeginScissorMode((int)area.x, (int)area.y, (int)area.width, (int)area.height);
        float y = area.y + area.height + scorriChat;
        if (chatInAttesa) {
            y -= 36;
            for (int i = 0; i < 3; i++) {
                float s = std::sin((float)GetTime() * 6 - i);
                DrawCircle((int)area.x + 20 + i * 18, (int)(y + 14 - s * 4), 5, Hex(0xd97757));
            }
        }
        for (int i = (int)chat.storia.size() - 1; i >= 0; i--) {
            const MessaggioChat& msg = chat.storia[i];
            std::vector<std::string> righe = ACapo(msg.testo, testoW, dim);
            float h = righe.size() * riga + 16;
            y -= h + 10;
            if (y + h < area.y) break;
            float w = 0;
            for (auto& r : righe) w = std::max(w, (float)MeasureText(r.c_str(), dim));
            w += 24;
            float x = msg.utente ? area.x + area.width - w : area.x;
            DrawRectangleRounded({ x, y, w, h }, 0.15f, 6, msg.utente ? Hex(0x3a4080) : Hex(0x2a2040));
            for (size_t k = 0; k < righe.size(); k++) {
                Color c = msg.utente ? WHITE : (righe[k].rfind("[Modificato", 0) == 0 ? Hex(0x8cff6a) : Hex(0xf0e8ff));
                DrawText(righe[k].c_str(), (int)x + 12, (int)(y + 8 + k * riga), dim, c);
            }
        }
        EndScissorMode();
    }

    // Casella di scrittura.
    Rectangle in = { p.x + 12, p.y + p.height - inputH - 8, p.width - 24, inputH };
    DrawRectangleRounded(in, 0.12f, 6, Hex(0x0e0a18));
    DrawRectangleRoundedLinesEx(in, 0.12f, 6, 2, fuocoChat ? Hex(0xd97757) : Hex(0x3a2a4a));
    if (bozza.empty() && !fuocoChat) DrawText("Clicca qui e scrivi la tua richiesta...", (int)in.x + 12, (int)in.y + 12, dim, Hex(0x6a5a80));
    else {
        std::string t = bozza + (fuocoChat && std::fmod((float)GetTime(), 1.0f) < 0.5f ? "_" : "");
        std::vector<std::string> righe = ACapo(t, (int)in.width - 24, dim);
        size_t inizio = righe.size() > 4 ? righe.size() - 4 : 0;
        for (size_t k = inizio; k < righe.size(); k++) DrawText(righe[k].c_str(), (int)in.x + 12, (int)(in.y + 10 + (k - inizio) * riga), dim, WHITE);
    }
    DrawText(chatInAttesa ? "Claude sta lavorando sul tuo gioco..." : "Invio: invia", (int)in.x + 12, (int)(in.y + in.height - 22), 16, Hex(0x8a7aa0));
}
