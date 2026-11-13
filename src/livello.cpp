#include "livello.h"
#include "mondo.h"
#include <cctype>
#include <cstdlib>
#include <sstream>

const char* NomeOggetto(TipoOggetto t) {
    switch (t) {
        case TipoOggetto::Moneta: return "Moneta";
        case TipoOggetto::Gemma: return "Gemma";
        case TipoOggetto::Molla: return "Molla";
        case TipoOggetto::Forziere: return "Forziere";
        case TipoOggetto::Lanterna: return "Lanterna";
        case TipoOggetto::Slime: return "Slime";
        case TipoOggetto::Pipistrello: return "Pipistrello";
        case TipoOggetto::Bandiera: return "Bandiera";
        case TipoOggetto::Cuore: return "Cuore";
        case TipoOggetto::Cartello: return "Cartello";
        case TipoOggetto::Porta: return "Porta";
        case TipoOggetto::Fantasma: return "Fantasma";
        case TipoOggetto::Torretta: return "Torretta";
        case TipoOggetto::Boss: return "Boss";
        case TipoOggetto::Cassa: return "Cassa";
    }
    return "";
}

const char* CaratteriOggetti() { return "ogmckebFhDrsBx123456789"; }

bool OggettoDaCarattere(char c, TipoOggetto& t, bool& appoggiato) {
    appoggiato = true;
    switch (c) {
        case 'o': t = TipoOggetto::Moneta; appoggiato = false; return true;
        case 'g': t = TipoOggetto::Gemma; appoggiato = false; return true;
        case 'm': t = TipoOggetto::Molla; return true;
        case 'c': t = TipoOggetto::Forziere; return true;
        case 'k': t = TipoOggetto::Lanterna; return true;
        case 'e': t = TipoOggetto::Slime; return true;
        case 'b': t = TipoOggetto::Pipistrello; appoggiato = false; return true;
        case 'F': t = TipoOggetto::Bandiera; return true;
        case 'h': t = TipoOggetto::Cuore; appoggiato = false; return true;
        case 'D': t = TipoOggetto::Porta; return true;
        case 'r': t = TipoOggetto::Fantasma; appoggiato = false; return true;
        case 's': t = TipoOggetto::Torretta; return true;
        case 'B': t = TipoOggetto::Boss; return true;
        case 'x': t = TipoOggetto::Cassa; appoggiato = false; return true;
        default:
            if (c >= '1' && c <= '9') { t = TipoOggetto::Cartello; return true; }
            return false;
    }
}

std::string ChiaveOggetto(TipoOggetto t) {
    std::string s = NomeOggetto(t);
    for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch);
    return s;
}

bool TipoDaNome(const std::string& nome, TipoOggetto& t) {
    std::string n = nome;
    for (auto& ch : n) ch = (char)std::tolower((unsigned char)ch);
    for (int i = 0; i < NUM_TIPI_OGGETTO; i++)
        if (ChiaveOggetto((TipoOggetto)i) == n) { t = (TipoOggetto)i; return true; }
    return false;
}

static Oggetto NuovoOggetto(TipoOggetto tipo, Vector2 pos, int numero, int seme) {
    Oggetto o{};
    o.tipo = tipo;
    o.pos = o.origine = pos;
    o.attivo = true;
    o.numero = numero;
    if (tipo == TipoOggetto::Boss && o.numero <= 0) o.numero = 9;
    o.t = Hash1(seme) * 10;
    return o;
}

bool Livello::Carica(const std::string& testo, std::string& errore) {
    std::vector<std::string> righe;
    std::stringstream ss(testo);
    std::string r;
    while (std::getline(ss, r)) {
        if (!r.empty() && r.back() == '\r') r.pop_back();
        if (!r.empty() && r[0] == ';') continue; // impostazioni: le legge progetto.cpp
        if (!r.empty()) righe.push_back(r);
    }
    if (righe.size() < 4) { errore = "il livello deve avere almeno 4 righe"; return false; }
    h = (int)righe.size();
    w = 0;
    for (auto& riga : righe) w = std::max(w, (int)riga.size());
    if (w < 10) { errore = "il livello e' troppo stretto"; return false; }
    celle.assign((size_t)w * h, ' ');
    oggetti.clear();
    bordiChiusi = false;
    bool haPartenza = false;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < (int)righe[y].size(); x++) {
            char c = righe[y][x];
            Vector2 centro = { x * TILE + TILE / 2.0f, y * TILE + TILE / 2.0f };
            Vector2 base = { x * TILE + TILE / 2.0f, (y + 1) * TILE * 1.0f };
            TipoOggetto t;
            bool appoggiato;
            if (c == '#' || c == '=' || c == '-' || c == '^' || c == '.') celle[(size_t)y * w + x] = c;
            else if (c == 'P') { partenza = base; haPartenza = true; }
            else if (OggettoDaCarattere(c, t, appoggiato))
                oggetti.push_back(NuovoOggetto(t, appoggiato ? base : centro, (c >= '1' && c <= '9') ? c - '0' : 0, x * 31 + y));
        }
    }
    if (!haPartenza) { errore = "manca la partenza (P)"; return false; }
    return true;
}

bool Livello::DaMappa(const Mappa& m, std::string& errore) {
    w = m.larghezza; h = m.altezza;
    if (w < 4 || h < 4) { errore = "la mappa e' troppo piccola"; return false; }
    celle.assign((size_t)w * h, ' ');
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char c = m.Casella(x, y);
            if (c == '#' || c == '=' || c == '-' || c == '^' || c == '.') celle[(size_t)y * w + x] = c;
        }
    oggetti.clear();
    int i = 0;
    for (auto& om : m.oggetti) {
        TipoOggetto t;
        if (!TipoDaNome(om.tipo, t)) continue;   // tipi sconosciuti: ignorati (versioni future)
        Oggetto o = NuovoOggetto(t, { om.x, om.y }, om.numero, i++ * 7 + 3);
        o.destMappa = om.mappa;
        o.dest = { om.destX, om.destY };
        oggetti.push_back(o);
    }
    partenza = m.partenza;
    bordiChiusi = m.vista == Vista::Alto;
    return true;
}
char Livello::Cella(int x, int y) const {
    if (x < 0 || x >= w) return '=';          // bordi laterali solidi
    if (y < 0 || y >= h) return bordiChiusi ? '=' : ' ';
    return celle[(size_t)y * w + x];
}

void Livello::DisegnaTile(int tx, int ty, float sx, float sy, float tempo, float notte, bool dallAlto) const {
    char c = Cella(tx, ty);
    int x = (int)std::round(sx), y = (int)std::round(sy);
    float buio = notte * 0.45f;
    auto Ombra = [&](unsigned int col) { return Mescola(Hex(col), Hex(0x0a0d24), buio); };

    if (dallAlto && (c == '#' || c == '=')) {
        // Muro visto dall'alto: blocco in rilievo con la faccia superiore chiara.
        bool siepe = c == '#';
        Color fianco = siepe ? Hex(0x2f6a2a) : Hex(0x4a4f66), cima = siepe ? Hex(0x4caf3a) : Hex(0x8a90ac);
        DrawRectangle(x, y, TILE, TILE, fianco);
        DrawRectangle(x, y, TILE, TILE - 4, cima);
        DrawRectangle(x, y, TILE, 1, siepe ? Hex(0x8ee36a) : Hex(0xb8bed6));
        for (int i = 0; i < 3; i++) {
            float h = Hash1(tx * 37 + ty * 91 + i);
            DrawRectangle(x + 2 + (int)(h * 11), y + 2 + i * 4, 2, 1, siepe ? Hex(0x6fd34e) : Hex(0x6f7590));
        }
        if (Cella(tx, ty + 1) != c) DrawRectangle(x, y + TILE, TILE, 3, Hex(0x000000, 70)); // ombra sul pavimento
        return;
    }
    if (c == '.') {
        float p = 0.6f + 0.4f * std::sin(tempo * 4 + tx + ty);
        DrawRectangleLines(x + 3, y + 3, TILE - 6, TILE - 6, ConAlfa(Hex(0xffd23f), p));
        DrawRectangle(x + 6, y + 6, 4, 4, ConAlfa(Hex(0xffd23f), p * 0.7f));
        return;
    }

    if (c == '#') {
        bool cima = Cella(tx, ty - 1) != '#';
        DrawRectangle(x, y, TILE, TILE, Ombra(0x7a4a2a));
        // Sassolini e venature, sempre uguali per ogni posizione.
        for (int i = 0; i < 4; i++) {
            float hx = Hash1(tx * 92821 + ty * 31 + i * 7), hy = Hash1(tx * 13 + ty * 68917 + i * 3);
            DrawRectangle(x + (int)(hx * 14), y + 4 + (int)(hy * 10), 2, 1, Ombra(i % 2 ? 0x5e361e : 0x96603a));
        }
        if (Cella(tx - 1, ty) != '#' && Cella(tx - 1, ty) != '=') DrawRectangle(x, y, 1, TILE, Ombra(0x4a2a16));
        if (Cella(tx + 1, ty) != '#' && Cella(tx + 1, ty) != '=') DrawRectangle(x + TILE - 1, y, 1, TILE, Ombra(0x4a2a16));
        if (cima) {
            DrawRectangle(x, y, TILE, 5, Ombra(0x4caf3a));
            DrawRectangle(x, y + 5, TILE, 1, Ombra(0x2f7a28));
            DrawRectangle(x, y, TILE, 1, Ombra(0x8ee36a));
            // Fili d'erba che ondeggiano al vento.
            for (int i = 0; i < 4; i++) {
                float ox = std::sin(tempo * 2.2f + tx * 0.7f + i) * 1.0f;
                int bx = x + 2 + i * 4;
                DrawLine(bx, y, (int)(bx + ox), y - 2 - (int)(Hash1(tx * 4 + i) * 2), Ombra(0x6fd34e));
            }
            if (Hash1(tx * 7 + 3) > 0.8f) { // fiorellini
                Color fiore = Hash1(tx) > 0.5f ? Hex(0xffe066) : Hex(0xff7aa8);
                DrawRectangle(x + 6, y - 3, 2, 2, Mescola(fiore, Hex(0x0a0d24), buio));
            }
        }
    } else if (c == '=') {
        DrawRectangle(x, y, TILE, TILE, Ombra(0x6f7590));
        DrawRectangle(x, y, TILE, 1, Ombra(0xa3a9c4));
        DrawRectangle(x, y + 7, TILE, 1, Ombra(0x4a4f66));
        DrawRectangle(x + ((ty % 2) ? 4 : 11), y, 1, 7, Ombra(0x4a4f66));
        DrawRectangle(x + ((ty % 2) ? 11 : 4), y + 8, 1, 8, Ombra(0x4a4f66));
        DrawRectangle(x, y + TILE - 1, TILE, 1, Ombra(0x3a3e52));
    } else if (c == '-') {
        DrawRectangle(x, y, TILE, 5, Ombra(0xb87a40));
        DrawRectangle(x, y, TILE, 1, Ombra(0xe0a868));
        DrawRectangle(x, y + 4, TILE, 1, Ombra(0x7a4a22));
        DrawRectangle(x + 3, y + 2, 1, 1, Ombra(0x5a3418));
        DrawRectangle(x + 12, y + 2, 1, 1, Ombra(0x5a3418));
    } else if (c == '^') {
        for (int i = 0; i < 3; i++) {
            float bx = x + i * 5.33f;
            DrawTriangle({ bx + 2.6f, (float)y + 6 }, { bx, (float)y + TILE }, { bx + 5.3f, (float)y + TILE }, Ombra(0xc9cfdf));
            DrawLine((int)bx + 2, y + 8, (int)bx + 2, y + 14, Ombra(0xffffff));
        }
    }
}

void Livello::DisegnaPavimento(float camX, float camY, int tema, float larghezza, float altezza, bool pulisci) const {
    static const unsigned int colori[][2] = { { 0x3a5a3a, 0x345234 }, { 0x8a6a4a, 0x7e6044 }, { 0x6a7a9a, 0x62708e },
                                              { 0x3a2a30, 0x34262c }, { 0x2e2a40, 0x29263a }, { 0x101428, 0x0e1224 }, { 0x2a2620, 0x25221c } };
    int k = tema < 0 || tema > 6 ? 0 : tema;
    if (pulisci) ClearBackground(Hex(colori[k][1]));
    int x0 = (int)std::floor(camX / TILE) - 6, x1 = x0 + (int)(larghezza / TILE) + 14;
    int y0 = (int)std::floor(camY / TILE) - 6, y1 = y0 + (int)(altezza / TILE) + 14;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            int x = (int)std::round(tx * TILE - camX), y = (int)std::round(ty * TILE - camY);
            if (tx < 0 || tx >= w || ty < 0 || ty >= h) { DrawRectangle(x, y, TILE, TILE, Hex(0x05060c)); continue; }
            DrawRectangle(x, y, TILE, TILE, Hex(colori[k][(tx + ty) % 2]));
            if (Hash1(tx * 13 + ty * 7) > 0.85f) DrawRectangle(x + 5, y + 9, 2, 1, Hex(0x000000, 40));
        }
}

void Livello::Disegna(float camX, float camY, float tempo, float notte, bool dallAlto, float larghezza, float altezza) const {
    int x0 = (int)std::floor(camX / TILE) - 6, x1 = x0 + (int)(larghezza / TILE) + 14;
    int y0 = (int)std::floor(camY / TILE) - 6, y1 = y0 + (int)(altezza / TILE) + 14;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (Cella(tx, ty) != ' ' && tx >= 0 && tx < w && ty >= 0 && ty < h)
                DisegnaTile(tx, ty, tx * TILE - camX, ty * TILE - camY, tempo, notte, dallAlto);
}

// ---------------------------------------------------------------------------
// Livello principale: "La valle delle gemme".
const char* LivelloAvventura() {
    return R"(
                                                                                                                                                      
                                                                                                                                                      
                                                                                                                                                      
                                                                                                                                                      
                                   g                                                                                                                  
                                ------                                                                                                                
                                                          b                                                           g                               
                         o o o                                   g                     oooo                 b                          ooo            
                        -------                     ooo                               ------                        -----                 e      F    
     ooo                                       =                                                                                   ===================
                                              ==                --                             =                                  ====================
  P                 e            m           ===    ^^^              k      e     c            =  e             ^^               =====================
############  ########################   ######################    ##################################   ##############################################
############  ########################   ######################    ##################################   ##############################################
############  ########################   ######################    ##################################   ##############################################
)";
}

const char* LivelloVetrinaOggetti() {
    return R"(
                                                            
                                                            
                                                            
                                                            
                                                            
                                                            
                                                            
                      g                         b           
                                                            
                     ---                                    
        o  o  o                                             
 P              m           c         k      e        F     
############################################################
############################################################
)";
}
