// Strumenti comuni ai tre giochi di piattaforme: immagini, disegni, scenari, caselle,
// musica scritta come testo e livelli costruiti unendo pezzi disegnati a caratteri.
#include "platform_tre.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace piatt {

using namespace arte;

std::string Png(const Image& img) {
    int n = 0;
    unsigned char* d = ExportImageToMemory(img, ".png", &n);
    std::string s(d ? (const char*)d : "", d ? (size_t)n : 0);
    if (d) MemFree(d);
    return s;
}

void AggiungiImmagine(ProgettoGioco& p, const std::string& nome, const Tela& t) {
    Image img = t.Immagine();
    std::string dati = Png(img);
    UnloadImage(img);
    for (auto& im : p.immagini) if (im.first == nome) { im.second = dati; return; }
    p.immagini.push_back({ nome, dati });
}

Disegno CreaOggettoDisegno(float w, float h, const Resa& resa, const std::vector<AnimOggetto>& anims, const DisegnaOggetto& f) {
    int W = (int)std::ceil(w * resa.dens), H = (int)std::ceil(h * resa.dens);
    Disegno d; d.Nuovo(W, H, resa.dens); d.liscio = resa.hd; d.animazioni.clear();
    for (auto& a : anims) {
        Animazione an; an.nome = a.nome; an.fps = a.fps; an.ripeti = a.ripeti;
        for (int i = 0; i < a.fotogrammi; i++) {
            Tela tela(W, H); K c(tela);
            c.scale(resa.dens, resa.dens);
            c.translate(w / 2, h);
            f(c, a.nome, i, a.fotogrammi);
            Image img = tela.Immagine();
            an.fotogrammi.push_back(FotogrammaDaImmagine(img, { 0, 0, (float)W, (float)H }));
            UnloadImage(img);
        }
        d.animazioni.push_back(an);
    }
    return d;
}

Disegno CreaPersonaggio(const DisegnaPersonaggio& f, float altezzaMondo, const Resa& resa, float alto, float largo, float sopra) {
    OpzioniDisegno o;
    o.altezzaMondo = altezzaMondo; o.dens = resa.dens; o.liscio = resa.hd;
    o.alto = alto; o.largo = largo; o.sopra = sopra;
    return CreaDisegno(f, o);
}

// --- scenari -----------------------------------------------------------------------------------

Scenario CreaScenarioArte(ProgettoGioco& p, const ScenarioArte& s, const Resa& resa) {
    Scenario sc;
    sc.nome = s.nome; sc.autore = "Elvantis";
    sc.ora = 0.4f; sc.cicloGiorno = false; sc.sfondoGenerato = false;
    sc.cieloPersonale = true; sc.pioggia = s.atmosfera;
    std::string base = s.nome;
    for (auto& ch : base) if (ch == ' ' || ch == '\'') ch = '_';
    const float r = resa.r;
    // cielo: fermo, riempie lo schermo
    if (s.cielo) {
        Tela t((int)(1280 * r), (int)(720 * r)); K c(t); c.scale(r, r); s.cielo(c);
        std::string nome = base + "_cielo.png";
        AggiungiImmagine(p, nome, t);
        StratoSfondo st; st.nome = "Cielo"; st.tipo = "immagine"; st.immagine = nome;
        st.parallasseX = 0; st.parallasseY = 0; st.altezza = 1.0f; st.scala = VH / (720 * r); st.ripeti = false;
        sc.strati.push_back(st);
        Image img = t.Immagine();   // colori del cielo per la mappa (bordo alto e basso)
        Color* px = (Color*)img.data;
        sc.cieloAlto = px[img.width / 2]; sc.cieloBasso = px[(size_t)(img.height - 1) * img.width + img.width / 2];
        UnloadImage(img);
    }
    for (size_t i = 0; i < s.strati.size(); i++) {
        Tela t((int)(LS * r), (int)(HS * r)); K c(t); c.scale(r, r); s.strati[i].disegna(c);
        std::string nome = base + "_strato" + std::to_string(i + 1) + ".png";
        AggiungiImmagine(p, nome, t);
        StratoSfondo st; st.nome = "Strato " + std::to_string(i + 1); st.tipo = "immagine"; st.immagine = nome;
        st.parallasseX = s.strati[i].parallasse; st.parallasseY = 0.04f + 0.1f * s.strati[i].parallasse;
        st.altezza = 1.04f; st.scala = VH / (720 * r); st.ripeti = true;
        sc.strati.push_back(st);
    }
    if (!s.primoPiano.empty()) {
        Tela t((int)(LS * r), (int)(HS * r)); K c(t); c.scale(r, r);
        PrimoPiano(c, s.primoPiano, s.tipoPrimoPiano);
        t.Sfoca(3 * r);
        std::string nome = base + "_primopiano.png";
        AggiungiImmagine(p, nome, t);
        StratoSfondo st; st.nome = "Primo piano"; st.tipo = "immagine"; st.immagine = nome;
        st.parallasseX = 1.35f; st.parallasseY = 0.3f; st.altezza = 1.16f; st.scala = VH / (720 * r); st.ripeti = true; st.primoPiano = true;
        sc.strati.push_back(st);
    }
    return sc;
}

// Una casella del terreno: 16x16 del mondo = 64x64 logici (lo schermo 1280x720 mostra 20 caselle).
CasellaDef CreaCasella(char carattere, const std::string& nome, const Materiale& m, const Resa& resa, unsigned seme) {
    const int VAR = 4;
    const float T = 64;                           // casella in unita' logiche
    int px = (int)(TILE * resa.dens);             // pixel della casella nel disegno
    float k = px / T;
    CasellaDef cd; cd.carattere = carattere; cd.nome = nome;
    cd.disegno.Nuovo(px, px, resa.dens); cd.disegno.liscio = resa.hd; cd.disegno.animazioni.clear();
    for (int quale = 0; quale < 2; quale++) {
        Animazione an; an.nome = quale == 0 ? "cima" : "dentro"; an.fps = 0; an.ripeti = true;
        Tela t(px * VAR, px); K c(t); c.scale(k, k);
        if (quale == 0) DisegnaMateriale(c, 0, T * VAR, 10, T * 3, m, seme);
        else { c.translate(0, -T * 1.5f); DisegnaMateriale(c, 0, T * VAR, -40, T * 6, m, seme + 17); }
        Image img = t.Immagine();
        for (int v = 0; v < VAR; v++) an.fotogrammi.push_back(FotogrammaDaImmagine(img, { (float)(v * px), 0, (float)px, (float)px }));
        UnloadImage(img);
        cd.disegno.animazioni.push_back(an);
    }
    return cd;
}

CasellaDef CreaCasellaDisegnata(char carattere, const std::string& nome, const Resa& resa, const DisegnaOggetto& f, int varianti) {
    CasellaDef cd; cd.carattere = carattere; cd.nome = nome;
    cd.disegno = CreaOggettoDisegno(TILE, TILE, resa, { { "cima", varianti, 0, true } }, f);
    return cd;
}

// --- musica ----------------------------------------------------------------------------------

static int Midi(const std::string& n) {
    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
    if (n.empty() || n[0] < 'A' || n[0] > 'G') return -1;
    int s = base[n[0] - 'A'];
    size_t i = 1;
    if (i < n.size() && n[i] == '#') { s++; i++; } else if (i < n.size() && n[i] == 'b') { s--; i++; }
    int ott = i < n.size() ? std::atoi(n.c_str() + i) : 4;
    return 12 * (ott + 1) + s;
}

Brano CreaBrano(const std::string& nome, float bpm, int battute, const std::vector<TracciaTesto>& tracce) {
    Brano b; b.nome = nome; b.autore = "Elvantis"; b.bpm = bpm; b.battute = battute; b.tracce.clear();
    const float totale = battute * 16.0f;
    for (auto& tt : tracce) {
        TracciaBrano t; t.nome = tt.nome; t.strumento = tt.strumento; t.volume = tt.volume; t.batteria = tt.batteria;
        if (tt.batteria) {
            std::string s; for (char ch : tt.note) if (ch != ' ' && ch != '|') s += ch;
            if (s.empty()) continue;
            for (int i = 0; i < (int)totale; i++) {
                char ch = s[i % s.size()];
                int a = ch == 'K' ? 36 : ch == 'S' ? 38 : ch == 'h' ? 42 : ch == 'o' ? 49 : 0;
                if (a) t.note.push_back({ (float)i, 1, a });
            }
        } else {
            std::istringstream in(tt.note);
            std::vector<std::pair<int, float>> seq;
            std::string w;
            float lun = 0;
            while (in >> w) {
                size_t c = w.find(':');
                std::string n = w.substr(0, c);
                float d = c == std::string::npos ? 1 : (float)std::atof(w.c_str() + c + 1);
                seq.push_back({ n == "R" ? -1 : Midi(n), d * 2 });
                lun += d * 2;
            }
            if (lun <= 0) continue;
            float x = 0;
            while (x < totale - 0.01f)
                for (auto& s : seq) {
                    if (x >= totale - 0.01f) break;
                    if (s.first >= 0) t.note.push_back({ x, std::min(s.second, totale - x), s.first + 12 * tt.ottava });
                    x += s.second;
                }
        }
        b.tracce.push_back(t);
    }
    return b;
}

EffettoSuono Effetto(const std::string& nome, int onda, float f0, float f1, float durata, float volume, float rumore, float vibrato, float vibratoHz) {
    EffettoSuono e; e.nome = nome; e.autore = "Elvantis"; e.onda = onda; e.f0 = f0; e.f1 = f1; e.durata = durata; e.volume = volume;
    e.rumore = rumore; e.vibrato = vibrato; e.vibratoHz = vibratoHz;
    return e;
}

// --- livelli ---------------------------------------------------------------------------------

Mappa CostruisciMappa(const std::string& nome, const std::vector<const Pezzo*>& pezzi, const Legenda& leg, int mappaDopo, int righeSopra) {
    int larg = 0;
    for (auto* p : pezzi) larg += p->righe.empty() ? 0 : (int)p->righe[0].size();
    Mappa m;
    m.Nuova(larg, RIGHE_PEZZO + righeSopra, Vista::Laterale);
    for (auto& r : m.caselle) r.assign(r.size(), ' ');   // Nuova mette il terreno in basso: qui lo disegnano i pezzi
    m.nome = nome;
    m.telecamera.modo = ModoTelecamera::Segui;
    m.telecamera.zoom = 1.5f;
    m.telecamera.zonaMortaX = 12; m.telecamera.zonaMortaY = 36; m.telecamera.anticipo = 36;
    if (righeSopra == 0) m.telecamera.zoom = 1.4f;   // arene: si vede tutta l'altezza
    m.cicloGiorno = false; m.sfondoGenerato = false;
    const int dy = righeSopra;
    int x0 = 0;
    auto Metti = [&](const std::string& tipo, int cx, int cy, int mappa = -1) {
        OggettoMappa o; o.tipo = tipo; o.x = cx * TILE + TILE / 2.0f; o.y = (cy + 1) * (float)TILE; o.mappa = mappa;
        m.oggetti.push_back(o);
    };
    for (auto* p : pezzi) {
        int w = p->righe.empty() ? 0 : (int)p->righe[0].size();
        for (int y = 0; y < RIGHE_PEZZO && y < (int)p->righe.size(); y++) {
            const std::string& r = p->righe[y];
            if ((int)r.size() != w) std::fprintf(stderr, "pezzo con righe di lunghezza diversa (%d invece di %d): \"%s\"\n", (int)r.size(), w, r.c_str());
            for (int x = 0; x < (int)r.size(); x++) {
                char ch = r[x];
                int cx = x0 + x, cy = y + dy;
                if (ch == '#' || ch == '=' || ch == '-' || ch == '^') { m.ImpostaCasella(cx, cy, ch); continue; }
                if (ch == ' ') continue;
                if (ch == 'S') { m.partenza = { cx * TILE + TILE / 2.0f, (cy + 1) * (float)TILE }; continue; }
                auto it = leg.find(ch);
                if (it == leg.end() || it->second.empty()) continue;
                Metti(it->second, cx, cy, ch == 'G' ? mappaDopo : -1);
            }
        }
        x0 += w;
    }
    return m;
}

std::vector<const Pezzo*> ScegliPezzi(const std::vector<Pezzo>& tutti, const Pezzo& inizio, const Pezzo& controllo, const Pezzo& fine,
                                      int quanti, int livello, unsigned seme) {
    std::vector<const Pezzo*> v{ &inizio };
    Caso r(seme * 7919 + 13);
    // difficolta' massima secondo il livello: 0 facile .. 3 difficile
    int massima = std::min(3, 1 + livello / 3), minima = livello >= 9 ? 1 : 0;
    std::vector<const Pezzo*> scelta;
    for (auto& p : tutti) if (p.difficolta <= massima && p.difficolta >= minima) scelta.push_back(&p);
    if (scelta.empty()) for (auto& p : tutti) scelta.push_back(&p);
    const Pezzo* ultimo = nullptr;
    for (int i = 0; i < quanti; i++) {
        if (i == quanti / 2) v.push_back(&controllo);
        const Pezzo* p = nullptr;
        for (int tent = 0; tent < 8; tent++) { p = scelta[(size_t)(r() * scelta.size()) % scelta.size()]; if (p != ultimo) break; }
        // verso la fine del livello preferisce i pezzi piu' difficili
        if (i > quanti * 2 / 3) {
            const Pezzo* q = scelta[(size_t)(r() * scelta.size()) % scelta.size()];
            if (q->difficolta > p->difficolta && q != ultimo) p = q;
        }
        v.push_back(p);
        ultimo = p;
    }
    v.push_back(&fine);
    return v;
}

// Pezzi di livello. Le righe 10 e 11 sono il terreno; l'eroe salta circa 3 caselle in alto
// e 4-5 in lungo (con il doppio salto molto di piu').
const Pezzo& PezzoInizio() {
    static Pezzo p{ {
        "            ",
        "            ",
        "            ",
        "            ",
        "            ",
        "            ",
        "            ",
        "        o   ",
        "       o o  ",
        "  S         ",
        "############",
        "############" }, 0 };
    return p;
}
const Pezzo& PezzoControllo() {
    static Pezzo p{ {
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "   o o    ",
        "          ",
        "          ",
        "    c   h ",
        "##########",
        "##########" }, 0 };
    return p;
}
const Pezzo& PezzoFine() {
    static Pezzo p{ {
        "                ",
        "                ",
        "                ",
        "                ",
        "                ",
        "            ### ",
        "  o o o     ### ",
        "                ",
        "                ",
        "          G    #",
        "################",
        "################" }, 0 };
    return p;
}
const Pezzo& PezzoBoss() {
    static Pezzo p{ {
        "=                              =       ",
        "=                              =       ",
        "=                              =       ",
        "=                              =       ",
        "=      ---          ---        =       ",
        "=                              =       ",
        "=  h                       h   =       ",
        "=----        ----        ----  =       ",
        "=                              =       ",
        "=  S                    B      g  G   =",
        "=======================================",
        "=======================================" }, 0 };
    return p;
}

const std::vector<Pezzo>& PezziComuni() {
    static std::vector<Pezzo> v = {
        // 0: collinetta con ingranaggi
        { { "              ",
            "              ",
            "              ",
            "              ",
            "              ",
            "      o o     ",
            "              ",
            "     #####    ",
            "    o#####o   ",
            "   ########   ",
            "##############",
            "##############" }, 0 },
        // 1: primo fosso
        { { "            ",
            "            ",
            "            ",
            "            ",
            "            ",
            "            ",
            "     o o    ",
            "    o   o   ",
            "            ",
            "            ",
            "####    ####",
            "####    ####" }, 0 },
        // 2: nemico a terra fra due blocchi
        { { "                ",
            "                ",
            "                ",
            "                ",
            "                ",
            "                ",
            "       ooo      ",
            "                ",
            "                ",
            " ==     e    == ",
            "################",
            "################" }, 0 },
        // 3: scalinata
        { { "                ",
            "                ",
            "                ",
            "            o   ",
            "           o    ",
            "          ####  ",
            "      o   ####  ",
            "     o  ######  ",
            "       #######  ",
            "    ##########  ",
            "################",
            "################" }, 0 },
        // 4: piattaforme sospese sopra il fosso
        { { "                  ",
            "                  ",
            "                  ",
            "                  ",
            "        o o       ",
            "                  ",
            "       ----       ",
            "   o          o   ",
            "  ---        ---  ",
            "                  ",
            "##              ##",
            "##              ##" }, 1 },
        // 5: molla verso l'alto
        { { "                ",
            "                ",
            "      o o o     ",
            "                ",
            "     -------    ",
            "                ",
            "                ",
            "                ",
            "                ",
            "   m        e   ",
            "################",
            "################" }, 1 },
        // 6: spine e piattaforme
        { { "                  ",
            "                  ",
            "                  ",
            "                  ",
            "                  ",
            "       o  o       ",
            "                  ",
            "      --  --      ",
            "                  ",
            "                  ",
            "####^^^^^^^^^^####",
            "##################" }, 1 },
        // 7: nemici in volo
        { { "                  ",
            "                  ",
            "                  ",
            "      f      f    ",
            "                  ",
            "                  ",
            "   o  o  o  o  o  ",
            "                  ",
            "                  ",
            "                  ",
            "##################",
            "##################" }, 1 },
        // 8: piattaforma mobile su un fosso largo
        { { "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "        o o o       ",
            "                    ",
            "                    ",
            "         p          ",
            "                    ",
            "###              ###",
            "###              ###" }, 1 },
        // 9: torre con nemico che spara
        { { "                ",
            "                ",
            "                ",
            "                ",
            "         s      ",
            "        ====    ",
            "        ====    ",
            "   o    ====    ",
            "  ---   ====  o ",
            "        ====    ",
            "################",
            "################" }, 2 },
        // 10: chiave e porta
        { { "                #    ",
            "                #    ",
            "                #    ",
            "       k        #    ",
            "                #    ",
            "     -----      #    ",
            "                #    ",
            "  ---           #    ",
            "                     ",
            "           j    d    ",
            "#####################",
            "#####################" }, 2 },
        // 11: fossi in serie con nemici in volo
        { { "                        ",
            "                        ",
            "                        ",
            "        f               ",
            "                 f      ",
            "                        ",
            "     o      o      o    ",
            "                        ",
            "                        ",
            "                        ",
            "###   ###   ####   #####",
            "###   ###   ####   #####" }, 2 },
        // 12: corridoio di nemici
        { { "                      ",
            "                      ",
            "                      ",
            "                      ",
            "                      ",
            "                      ",
            "     o   h   o        ",
            "    ===     ===       ",
            "                      ",
            "   e     j      e     ",
            "######################",
            "######################" }, 2 },
        // 13: salti di precisione sulle spine (si puo' usare il doppio salto)
        { { "                        ",
            "                        ",
            "                        ",
            "                        ",
            "           o            ",
            "                        ",
            "                        ",
            "      ==   ==   ==   o  ",
            "      ==   ==   ==      ",
            "      ==   ==   ==      ",
            "####^^^^^^^^^^^^^^^^####",
            "########################" }, 3 },
        // 14: due piattaforme mobili
        { { "                          ",
            "                          ",
            "                          ",
            "                          ",
            "      o o       o o       ",
            "                          ",
            "                          ",
            "                          ",
            "       p         p        ",
            "                          ",
            "###                    ###",
            "###                    ###" }, 3 },
        // 15: fortino con tiratore e molla
        { { "                      ",
            "                      ",
            "             o o o    ",
            "                      ",
            "            s         ",
            "          ======      ",
            "          =    =      ",
            "          =    =      ",
            "   o         h =      ",
            "     m         =   e  ",
            "######################",
            "######################" }, 3 },
        // 16: discesa nella valle con monete
        { { "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "                    ",
            "      o o o         ",
            "     o     o        ",
            "    o       o   ##  ",
            "            j   ##  ",
            "####################",
            "####################" }, 1 },
        // 17: ponte di piattaforme sopra le spine con nemico in volo
        { { "                        ",
            "                        ",
            "                        ",
            "             f          ",
            "                        ",
            "                        ",
            "   o   o   o   o   o    ",
            "                        ",
            "  ---  ---  ---  ---    ",
            "                        ",
            "##^^^^^^^^^^^^^^^^^^^^##",
            "########################" }, 2 },
    };
    return v;
}

}  // namespace piatt

namespace piatt {

void CostruisciMondi(ProgettoGioco& p, const Resa& resa, std::vector<MondoGioco>& mondi, const NomiGioco& n) {
    const float TAU_ = 6.28318530718f;
    auto ell = [TAU_](K& c, float x, float y, float rx, float ry) { c.beginPath(); c.ellipse(x, y, rx, ry, 0, 0, TAU_); };
    const int N = (int)mondi.size();
    // --- scenari e caselle: ogni mondo ha il suo terreno (caselle legate allo scenario) ----------------
    for (int w = 0; w < N; w++) {
        MondoGioco& m = mondi[w];
        Scenario sc = CreaScenarioArte(p, m.scen, resa);
        sc.buio = m.buio; sc.acqua = m.acqua;
        p.scenari.push_back(sc);
        CasellaDef t = CreaCasella('#', m.nome + ": terreno", m.terra, resa, 100 + w); t.scenario = sc.nome; p.caselle.push_back(t);
        CasellaDef b = CreaCasella('=', m.nome + ": blocco", m.blocco, resa, 200 + w); b.scenario = sc.nome; p.caselle.push_back(b);
        std::string legno = m.legnoPiatt, luce = m.luceMobile;
        CasellaDef pi = CreaCasellaDisegnata('-', m.nome + ": piattaforma", resa, [legno, ell](K& c, const std::string&, int i, int) {
            c.fillStyle(lg(0, -16, 0, -10, { sh(legno, 0.25f), legno, sh(legno, -0.35f) })); c.fillRect(-8, -16, 16, 6);
            c.fillStyle(V(sh(legno, 0.5f))); c.fillRect(-8, -16, 16, 0.8f);
            c.fillStyle(V(sh(legno, -0.55f))); c.fillRect(-8, -10.6f, 16, 0.8f);
            c.fillStyle(V(sh(legno, -0.4f))); c.fillRect(i % 2 ? -1 : 3, -15, 0.6f, 4);
            c.fillStyle(V("#c8ccd8")); ell(c, -6, -13, 0.6f, 0.6f); c.fill(); ell(c, 6, -13, 0.6f, 0.6f); c.fill();
            c.fillStyle(V("rgba(0,0,0,0.25)")); c.fillRect(-7, -10, 14, 1.2f);
        }, 2);
        pi.scenario = sc.nome; p.caselle.push_back(pi);
        // piattaforma mobile del mondo
        OggettoDef mob; mob.nome = "Piattaforma " + m.nome; mob.autore = "Elvantis"; mob.funzione = FunzioneOggetto::PiattaformaMobile;
        mob.disegno = CreaOggettoDisegno(48, 10, resa, { { "fermo", 1, 1, true } }, [legno, luce](K& c, const std::string&, int, int) {
            c.fillStyle(lg(0, -10, 0, -3, { sh(legno, 0.3f), legno, sh(legno, -0.4f) })); c.fillRect(-24, -10, 48, 7);
            c.fillStyle(V(sh(legno, 0.55f))); c.fillRect(-24, -10, 48, 0.9f);
            for (int k = -2; k <= 2; k++) { c.fillStyle(V(sh(legno, -0.45f))); c.fillRect(k * 10 - 0.3f, -9, 0.6f, 6); }
            c.fillStyle(V("#4a4e60")); c.fillRect(-20, -3, 4, 3); c.fillRect(16, -3, 4, 3);
            c.save(); c.lighter(); c.fillStyle(rg(-18, 0, 0, 4, { rgba(luce, 0.7f), rgba(luce, 0) })); c.fillRect(-22, -2, 8, 4);
            c.fillStyle(rg(18, 0, 0, 4, { rgba(luce, 0.7f), rgba(luce, 0) })); c.fillRect(14, -2, 8, 4); c.restore();
        });
        mob.velocita = 60; mob.distanza = 80;
        p.oggetti.push_back(mob);
    }
    { // spine comuni a tutti i mondi
        CasellaDef sp = CreaCasellaDisegnata('^', "Spine", resa, [](K& c, const std::string&, int i, int) {
            for (int k = 0; k < 3; k++) {
                float x = -8 + k * 5.33f + 2.66f, h = 9 + ((k + i) % 2) * 2;
                c.beginPath(); c.moveTo(x - 2.6f, 0); c.lineTo(x, -h); c.lineTo(x + 2.6f, 0); c.closePath();
                c.fillStyle(lg(x - 2, 0, x + 2, 0, { "#e8ecf8", "#8a8ea0", "#4a4e60" })); c.fill();
                c.fillStyle(V("rgba(255,255,255,0.8)")); c.fillRect(x - 0.4f, -h + 1.5f, 0.6f, h * 0.5f);
            }
            c.fillStyle(V("#3a3e4a")); c.fillRect(-8, -1.5f, 16, 1.5f);
        }, 2);
        p.caselle.push_back(sp);
    }
    // --- mappe: N mondi x (3 livelli + la sfida del guardiano) ------------------------------------------
    const std::vector<Pezzo>& pezzi = PezziComuni();
    const int totale = N * 4;
    for (int w = 0; w < N; w++) {
        MondoGioco& m = mondi[w];
        Legenda leg = { { 'o', n.moneta }, { 'h', n.cuore }, { 'e', m.e }, { 'j', m.j }, { 'f', m.f }, { 's', m.s }, { 'm', n.molla },
                        { 'p', "Piattaforma " + m.nome }, { 'c', n.controllo }, { 'k', n.chiave }, { 'd', n.porta },
                        { 'B', m.boss }, { 'g', n.cancello }, { 'G', n.arrivo } };
        for (int l = 0; l < 4; l++) {
            int indice = w * 4 + l;
            int dopo = indice + 1 < totale ? indice + 1 : -1;
            std::string nome = m.nome + " " + (l < 3 ? std::to_string(l + 1) : std::string("- il guardiano"));
            Mappa mp;
            if (l < 3) {
                int livello = w * 3 + l;
                std::vector<const Pezzo*> scelti = ScegliPezzi(pezzi, PezzoInizio(), PezzoControllo(), PezzoFine(), 7 + livello / 3, livello / 2, n.seme * 131 + 31 * w + l + 1);
                mp = CostruisciMappa(nome, scelti, leg, dopo);
            } else {
                mp = CostruisciMappa(nome, { &PezzoBoss() }, leg, dopo, 0);
                for (auto& o : mp.oggetti) if (o.tipo == m.boss) o.numero = n.energiaBoss + w * 2;
            }
            mp.scenarioLibreria = m.scen.nome;
            mp.brano = l < 3 ? m.brano : n.branoBoss;
            mp.cieloPersonale = true;
            for (auto& sc : p.scenari) if (sc.nome == m.scen.nome) { mp.cieloAlto = sc.cieloAlto; mp.cieloBasso = sc.cieloBasso; }
            p.mondo.mappe.push_back(mp);
        }
        // il guardiano sconfitto apre il cancello verso l'arrivo
        Regola r; r.chi = m.boss; r.evento = "sconfitto";
        r.azioni = { { "apri", "sconfiggere il guardiano", 0 }, { "suona", "Boss sconfitto", 0 }, { "scuoti", "", 0.6f },
                     { "messaggio", w < N - 1 ? n.fraseBoss : n.fraseFinale, 0 }, { "musica", n.branoVittoria, 0 } };
        p.regole.push_back(r);
        if (!m.frase.empty()) {
            Regola ri; ri.chi = "gioco"; ri.evento = "inizio"; ri.conChi = m.nome + " 1";
            ri.azioni = { { "messaggio", m.frase, 0 } };
            p.regole.push_back(ri);
        }
    }
    p.mondo.mappaIniziale = 0;
}

}  // namespace piatt
