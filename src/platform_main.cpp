// Riga di comando dei giochi di piattaforme d'esempio:
//   StudioVideogiochi --crea-platform A|B|C file.gioco [Gioco.exe [base.exe]]
//   StudioVideogiochi --foto-gioco file.gioco cartella   (una foto di ogni mappa, per controllo)
#include "platform_tre.h"
#include "gioco.h"
#include "pacchetto.h"
#include "audio.h"
#include <cstdio>
#include <fstream>
#include <sstream>

namespace piatt {
}

static bool LeggiTutto(const std::string& f, std::string& s) {
    std::ifstream in(f, std::ios::binary); if (!in) return false;
    std::stringstream ss; ss << in.rdbuf(); s = ss.str(); return true;
}

int CreaPlatformDaRigaDiComando(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "Uso: --crea-platform A|B|C file.gioco [Gioco.exe [base.exe]]\n"); return 1; }
    char quale = argv[2][0];
    SetTraceLogLevel(LOG_WARNING);
    ProgettoGioco p = quale == 'A' ? piatt::CreaLumina() : quale == 'C' ? piatt::CreaNilo() : piatt::CreaNoraEBullone();
    std::string problema = p.Controlla();
    if (!problema.empty()) { std::fprintf(stderr, "Progetto non valido: %s\n", problema.c_str()); return 1; }
    std::string json = p.Json(true);
    { std::ofstream f(argv[3], std::ios::binary); f << json; }
    std::printf("%s: %d mappe, %d personaggi, %d oggetti, %d scenari, %d caselle, %d brani, %d immagini, %.1f MB\n", argv[3], (int)p.mondo.mappe.size(),
                (int)p.personaggi.size(), (int)p.oggetti.size(), (int)p.scenari.size(), (int)p.caselle.size(), (int)p.brani.size(), (int)p.immagini.size(), json.size() / 1048576.0);
    if (argc > 4) {
        DatiGioco d{ p.mondo.titolo, json, {} };
        std::string dati = SerializzaGioco(d), errore;
        bool ok;
        if (argc > 5) {   // eseguibile di un'altra piattaforma (es. lo studio per Windows)
            std::string base;
            if (!LeggiTutto(argv[5], base)) { std::fprintf(stderr, "Non leggo %s\n", argv[5]); return 1; }
            ok = CreaEseguibileDa(base, argv[4], dati, errore);
        } else ok = CreaEseguibile(argv[4], dati, errore);
        if (!ok) { std::fprintf(stderr, "%s\n", errore.c_str()); return 1; }
        std::printf("creato %s\n", argv[4]);
    }
    return 0;
}

// Foto di controllo: carica il gioco, porta l'eroe un po' avanti in ogni mappa e salva un'immagine.
int FotoGiocoDaRigaDiComando(int argc, char** argv) {
    if (argc < 4) return 1;
    std::string testo;
    if (!LeggiTutto(argv[2], testo)) return 1;
    ProgettoGioco p; std::string errore;
    if (!p.DaJson(testo, errore)) { std::fprintf(stderr, "%s\n", errore.c_str()); return 1; }
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "foto");
    MakeDirectory(argv[3]);
    int solo = argc > 4 ? std::atoi(argv[4]) : -1;
    for (size_t m = 0; m < p.mondo.mappe.size(); m++) {
        if (solo >= 0 && (int)m != solo) continue;
        ProgettoGioco q = p; q.mondo.mappaIniziale = (int)m;
        Gioco g;
        if (!g.CaricaProgetto(q, errore)) { std::fprintf(stderr, "mappa %d: %s\n", (int)m, errore.c_str()); continue; }
        Comandi c;
        for (int i = 0; i < 150; i++) { c.asse = i < 100 ? 1.0f : 0.0f; c.salto = c.saltoPremuto = (i == 40); g.Aggiorna(1 / 60.0f, c); }
        float dove = argc > 5 ? (float)std::atof(argv[5]) : 0;
        if (dove > 0) {   // l'eroe salta a quel punto della mappa (0..1) e ci atterra
            g.eroe.pos = { q.mondo.mappe[m].larghezza * TILE * dove, 8 };
            g.eroe.vel = { 0, 0 };
            Comandi fermo;
            for (int i = 0; i < 90; i++) g.Aggiorna(1 / 60.0f, fermo);
        }
        RenderTexture2D rt{};
        g.DisegnaHD(rt, 3);
        Image img = LoadImageFromTexture(rt.texture);
        ImageFlipVertical(&img);
        ImageResize(&img, 1280, 720);
        char nome[512]; std::snprintf(nome, sizeof nome, "%s/mappa_%02d.png", argv[3], (int)m);
        ExportImage(img, nome);
        UnloadImage(img); UnloadRenderTexture(rt);
        std::printf("%s: %s energia %d\n", nome, q.mondo.mappe[m].nome.c_str(), g.energia);
    }
    CloseWindow();
    return 0;
}

// Prova automatica dei livelli: un "giocatore robot" corre a destra e salta davanti ai muri e ai fossi
// (con il doppio salto). Non muore mai: serve a trovare i punti impossibili da superare.
int ProvaLivelliDaRigaDiComando(int argc, char** argv) {
    if (argc < 3) return 1;
    std::string testo;
    if (!LeggiTutto(argv[2], testo)) return 1;
    ProgettoGioco p; std::string errore;
    if (!p.DaJson(testo, errore)) { std::fprintf(stderr, "%s\n", errore.c_str()); return 1; }
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(320, 200, "prova");
    int problemi = 0;
    for (size_t m = 0; m < p.mondo.mappe.size(); m++) {
        ProgettoGioco q = p; q.mondo.mappaIniziale = (int)m;
        Gioco g;
        if (!g.CaricaProgetto(q, errore)) { std::fprintf(stderr, "mappa %d: %s\n", (int)m, errore.c_str()); problemi++; continue; }
        const Mappa& mp = q.mondo.mappe[m];
        float massimo = 0, fermoDa = 0, ultimoX = g.eroe.pos.x;
        int salti = 0, tSalto = 0;
        bool arrivato = false;
        for (int f = 0; f < 60 * 150 && !arrivato; f++) {
            g.energia = g.energiaMax; g.vite = 3; g.eroe.lampeggio = 1;
            for (auto& o : q.oggetti) if (o.funzione == FunzioneOggetto::Chiave) g.inventario[o.nome] = 1;
            Comandi c; c.asse = 1;
            int tx = (int)((g.eroe.pos.x + 10) / TILE), ty = (int)((g.eroe.pos.y - 4) / TILE);
            bool muro = mp.Casella(tx, ty) == '#' || mp.Casella(tx, ty) == '=' || mp.Casella(tx, ty - 1) == '#' || mp.Casella(tx, ty - 1) == '=';
            int tf = (int)((g.eroe.pos.x + 14) / TILE), tg = (int)((g.eroe.pos.y + 4) / TILE);
            bool buco = true;
            for (int k = 0; k < 4; k++) { char ch = mp.Casella(tf, tg + k); if (ch == '#' || ch == '=' || ch == '-') { buco = false; break; } }
            char sotto = mp.Casella(tf, tg);
            bool spine = sotto == '^';
            // vicino all'arrivo smette di saltare (sott'acqua altrimenti ci passerebbe sopra a nuoto)
            for (auto& om : mp.oggetti) for (auto& o : q.oggetti)
                if (o.funzione == FunzioneOggetto::Arrivo && o.nome == om.tipo && om.x > g.eroe.pos.x - 40 && om.x - g.eroe.pos.x < 40) { muro = buco = spine = false; fermoDa = 0; if (!g.eroe.aTerra && std::fabs(om.x - g.eroe.pos.x) < 24) c.asse = om.x > g.eroe.pos.x ? 0.3f : -0.3f; }
            tSalto--;
            if (g.eroe.aTerra && (muro || buco || spine || fermoDa > 0.3f) && tSalto <= 0) { c.salto = c.saltoPremuto = true; tSalto = 26; salti++; }
            else if (!g.eroe.aTerra && g.eroe.vel.y > 20 && (buco || muro || spine) && tSalto <= 0) { c.salto = c.saltoPremuto = true; tSalto = 30; }
            else c.salto = tSalto > 0;
            g.Aggiorna(1 / 60.0f, c);
            if (g.MappaAttuale() != (int)m || g.vinto) arrivato = true;
            massimo = std::max(massimo, g.eroe.pos.x);
            if (std::fabs(g.eroe.pos.x - ultimoX) < 0.2f) fermoDa += 1 / 60.0f; else fermoDa = 0;
            ultimoX = g.eroe.pos.x;
        }
        float perc = massimo / (mp.larghezza * TILE) * 100;
        std::printf("%-30s %s  %5.1f%%  salti %d\n", mp.nome.c_str(), arrivato ? "ARRIVATO" : "fermo   ", perc, salti);
        if (!arrivato && mp.nome.find("guardiano") == std::string::npos) {
            problemi++;
            int cx = (int)(massimo / TILE);
            for (int y = 0; y < mp.altezza; y++) {
                std::string r;
                for (int x = std::max(0, cx - 12); x < std::min(mp.larghezza, cx + 14); x++) r += x == cx ? (mp.Casella(x, y) == ' ' ? '|' : '!') : (mp.Casella(x, y) == ' ' ? '.' : mp.Casella(x, y));
                std::printf("   %s\n", r.c_str());
            }
            for (auto& o : mp.oggetti) if (std::fabs(o.x / TILE - cx) < 13) std::printf("   %s a %d,%d\n", o.tipo.c_str(), (int)(o.x / TILE), (int)(o.y / TILE));
        }
    }
    CloseWindow();
    std::printf("livelli non finiti dal robot: %d\n", problemi);
    return 0;
}
