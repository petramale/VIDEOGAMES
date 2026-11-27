// Elenco dei giochi arcade e riga di comando per crearli e provarli:
//   StudioVideogiochi --crea-arcade nome Gioco.exe [base.exe]    crea l'eseguibile del gioco
//   StudioVideogiochi --foto-arcade nome cartella [secondi]        foto di controllo (pilota automatico)
//   StudioVideogiochi --prova-arcade nome [secondi]                prova veloce: partita automatica + salvataggio
#include "arcade.h"
#include "pacchetto.h"
#include "audio.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "json.hpp"

namespace arcade {

std::unique_ptr<GiocoArcade> CreaStellaria();
std::unique_ptr<GiocoArcade> CreaSciame();
std::unique_ptr<GiocoArcade> CreaPolvere();
std::unique_ptr<GiocoArcade> CreaQuerciaverde();

std::unique_ptr<GiocoArcade> Crea(const std::string& nome) {
    if (nome == "stellaria") return CreaStellaria();
    if (nome == "sciame") return CreaSciame();
    if (nome == "polvere") return CreaPolvere();
    if (nome == "querciaverde") return CreaQuerciaverde();
    return nullptr;
}
const std::vector<std::string>& Nomi() {
    static const std::vector<std::string> n = { "stellaria", "sciame", "polvere", "querciaverde" };
    return n;
}
std::string TitoloDi(const std::string& nome) {
    if (nome == "stellaria") return "Stellaria: l'Invasione dei Prismi";
    if (nome == "sciame") return "Sciame di Orione";
    if (nome == "polvere") return "Polvere da Sparo: Commando del West";
    if (nome == "querciaverde") return "Il Regno di Querciaverde";
    return nome;
}

ProgettoGioco ProgettoArcade(const std::string& nome) {
    ProgettoGioco p = NuovoProgetto(Genere::Sparatutto, nome == "polvere" ? StileGrafico::PixelFine : StileGrafico::Disegnato, TitoloDi(nome));
    p.personaggi.clear(); p.oggetti.clear(); p.scenari.clear(); p.caselle.clear(); p.regole.clear(); p.variabili.clear(); p.effetti.clear();
    p.eroe.clear(); p.eroe2.clear();
    for (auto& m : p.mondo.mappe) { m.brano.clear(); m.scenarioLibreria.clear(); }
    p.arcade = nome;
    auto g = Crea(nome);
    if (g) { p.arcadeDati = g->Predefiniti().dump(); p.brani = g->BraniPredefiniti(); }
    return p;
}

}  // namespace arcade

using namespace arcade;

static bool LeggiFile(const std::string& f, std::string& s) {
    std::ifstream in(f, std::ios::binary); if (!in) return false;
    std::stringstream ss; ss << in.rdbuf(); s = ss.str(); return true;
}

int CreaArcadeDaRigaDiComando(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "Uso: --crea-arcade nome Gioco.exe [base.exe [progetto.gioco]]\n"); return 1; }
    std::string nome = argv[2];
    bool noto = false;
    for (auto& n : Nomi()) noto |= n == nome;
    if (!noto) { std::fprintf(stderr, "Gioco sconosciuto: %s\n", nome.c_str()); return 1; }
    // Il gioco e' un progetto .gioco come quelli dello Studio (accanto all'exe si salva anche il .gioco).
    ProgettoGioco p = ProgettoArcade(nome);
    std::string testo = p.Json();
    if (argc > 5) { std::ofstream f(argv[5], std::ios::binary); f << testo; }
    DatiGioco d{ TitoloDi(nome), p.Json(true), {} };
    std::string dati = SerializzaGioco(d), errore;
    bool ok;
    if (argc > 4 && std::string(argv[4]) != "-") {
        std::string base;
        if (!LeggiFile(argv[4], base)) { std::fprintf(stderr, "Non leggo %s\n", argv[4]); return 1; }
        ok = CreaEseguibileDa(base, argv[3], dati, errore);
    } else ok = CreaEseguibile(argv[3], dati, errore);
    if (!ok) { std::fprintf(stderr, "%s\n", errore.c_str()); return 1; }
    std::printf("creato %s (%s)\n", argv[3], d.titolo.c_str());
    return 0;
}

// Gioca da solo con il pilota automatico e fotografa lo schermo a intervalli regolari.
int FotoArcadeDaRigaDiComando(int argc, char** argv) {
    if (argc < 4) return 1;
    auto g = Crea(argv[2]);
    if (!g) return 1;
    g->Configura(nlohmann::json::object(), nullptr);
    float secondi = argc > 4 ? (float)std::atof(argv[4]) : 60;
    int foto = argc > 5 ? std::atoi(argv[5]) : 6;
    uint64_t seme = argc > 6 ? std::strtoull(argv[6], nullptr, 10) : 777;
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "foto");
    g->Prepara();
    g->Nuova(seme, 0);
    MakeDirectory(argv[3]);
    RenderTexture2D rt = LoadRenderTexture(g->L(), g->A());
    int passi = (int)(secondi * 60), ogni = std::max(1, passi / foto), n = 0;
    for (int i = 1; i <= passi && !g->finito; i++) {
        g->Aggiorna(1 / 60.0f, g->Automatico());
        if (i % ogni == 0 || i == passi) {
            BeginTextureMode(rt);
            ClearBackground(BLACK);
            g->Disegna();
            EndTextureMode();
            Image img = LoadImageFromTexture(rt.texture);
            ImageFlipVertical(&img);
            char nome[512]; std::snprintf(nome, sizeof nome, "%s/%s_%02d.png", argv[3], argv[2], n++);
            ExportImage(img, nome);
            UnloadImage(img);
            std::printf("%s  t=%.0fs  %s  punti %d\n", nome, i / 60.0f, g->Tappa().c_str(), g->punteggio);
        }
    }
    UnloadRenderTexture(rt);
    CloseWindow();
    return 0;
}

// Partita automatica veloce: controlla che non si blocchi e che il salvataggio si ricarichi uguale.
int ProvaArcadeDaRigaDiComando(int argc, char** argv) {
    if (argc < 3) return 1;
    auto g = Crea(argv[2]);
    if (!g) return 1;
    g->Configura(nlohmann::json::object(), nullptr);
    float secondi = argc > 3 ? (float)std::atof(argv[3]) : 300;
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(320, 200, "prova");
    g->Prepara();
    int problemi = 0;
    for (uint64_t seme = 1; seme <= 3; seme++) {
        g->Nuova(seme * 1000003, 0);
        int pc = g->puntiDiControllo, tappe = 0;
        int passi = (int)(secondi * 60);
        int i = 0;
        for (; i < passi && !g->finito; i++) {
            g->Aggiorna(1 / 60.0f, g->Automatico());
            if (g->puntiDiControllo != pc) {
                pc = g->puntiDiControllo; tappe++;
                // salva e ricarica in un'altra istanza: lo stato deve tornare identico
                std::string s = g->Stato(), errore;
                auto h = Crea(argv[2]);
                h->Configura(nlohmann::json::object(), nullptr);
                if (!h->Ripristina(s, errore)) { std::printf("  ripristino fallito: %s\n", errore.c_str()); problemi++; }
                else if (h->Stato() != s) {
                    auto a = nlohmann::json::parse(s), b = nlohmann::json::parse(h->Stato());
                    std::string diff;
                    for (auto it = a.begin(); it != a.end(); ++it) if (!b.contains(it.key()) || b[it.key()] != it.value()) diff += " " + it.key() + "=" + it.value().dump().substr(0, 80) + "/" + (b.contains(it.key()) ? b[it.key()].dump().substr(0, 80) : "?");
                    std::printf("  salvataggio diverso dopo il ripristino (%s):%s\n", g->Tappa().c_str(), diff.c_str());
                    problemi++;
                }
            }
        }
        std::printf("seme %llu: %.0f s, %s, punti %d, tappe %d%s\n", (unsigned long long)seme, i / 60.0f, g->Tappa().c_str(), g->punteggio, tappe,
                    g->finito ? (g->vinto ? " VINTO" : " finito") : "");
    }
    CloseWindow();
    std::printf("problemi: %d\n", problemi);
    return problemi ? 1 : 0;
}
