#include "livello.h"
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
    }
    return "";
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
    bool haPartenza = false;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < (int)righe[y].size(); x++) {
            char c = righe[y][x];
            Vector2 centro = { x * TILE + TILE / 2.0f, y * TILE + TILE / 2.0f };
            Vector2 base = { x * TILE + TILE / 2.0f, (y + 1) * TILE * 1.0f };
            Oggetto o{};
            o.attivo = true;
            switch (c) {
                case '#': case '=': case '-': case '^': celle[(size_t)y * w + x] = c; break;
                case 'P': partenza = base; haPartenza = true; break;
                case 'o': o.tipo = TipoOggetto::Moneta; o.pos = centro; break;
                case 'g': o.tipo = TipoOggetto::Gemma; o.pos = centro; break;
                case 'm': o.tipo = TipoOggetto::Molla; o.pos = base; break;
                case 'c': o.tipo = TipoOggetto::Forziere; o.pos = base; break;
                case 'k': o.tipo = TipoOggetto::Lanterna; o.pos = base; break;
                case 'e': o.tipo = TipoOggetto::Slime; o.pos = base; break;
                case 'b': o.tipo = TipoOggetto::Pipistrello; o.pos = centro; break;
                case 'F': o.tipo = TipoOggetto::Bandiera; o.pos = base; break;
                default: continue;
            }
            if (std::string("ogmckebF").find(c) != std::string::npos) {
                o.origine = o.pos;
                o.t = Hash1(x * 31 + y) * 10;
                oggetti.push_back(o);
            }
        }
    }
    if (!haPartenza) { errore = "manca la partenza (P)"; return false; }
    return true;
}

char Livello::Cella(int x, int y) const {
    if (x < 0 || x >= w) return '=';          // bordi laterali solidi
    if (y < 0 || y >= h) return ' ';
    return celle[(size_t)y * w + x];
}

void Livello::DisegnaTile(int tx, int ty, float sx, float sy, float tempo, float notte) const {
    char c = Cella(tx, ty);
    int x = (int)std::round(sx), y = (int)std::round(sy);
    float buio = notte * 0.45f;
    auto Ombra = [&](unsigned int col) { return Mescola(Hex(col), Hex(0x0a0d24), buio); };

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

void Livello::Disegna(float camX, float camY, float tempo, float notte) const {
    int x0 = (int)std::floor(camX / TILE) - 1, x1 = x0 + VW / TILE + 3;
    int y0 = (int)std::floor(camY / TILE) - 1, y1 = y0 + VH / TILE + 3;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (Cella(tx, ty) != ' ' && tx >= 0 && tx < w)
                DisegnaTile(tx, ty, tx * TILE - camX, ty * TILE - camY, tempo, notte);
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
