#include "editor_disegno.h"
#include "audio.h"
#include "ui.h"
#include "sistema.h"
#include <algorithm>
#include <cstring>

namespace {

const char* NOMI_ATTREZZI[] = { "Matita", "Gomma", "Secchio", "Linea", "Rettang.", "Ellisse", "Contagocce", "Sposta" };
const char* TASTI_ATTREZZI[] = { "P", "E", "F", "L", "R", "O", "I", "M" };
const int KEY_ATTREZZI[] = { KEY_P, KEY_E, KEY_F, KEY_L, KEY_R, KEY_O, KEY_I, KEY_M };

bool Uguali(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

void Scacchiera(Rectangle r, float lato) {
    DrawRectangleRec(r, Hex(0x3a3d58));
    Ui::Ritaglia((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    int nx = (int)(r.width / lato) + 1, ny = (int)(r.height / lato) + 1;
    for (int y = 0; y < ny; y++)
        for (int x = (y & 1); x < nx; x += 2) DrawRectangleRec({ r.x + x * lato, r.y + y * lato, lato, lato }, Hex(0x4a4e6c));
    Ui::FineRitaglio();
}

// Colori di partenza della tavolozza: una scala pensata per la pixel art.
const unsigned int BASE[] = { 0x000000, 0x1a1c2c, 0x5d275d, 0xb13e53, 0xef7d57, 0xffcd75, 0xa7f070, 0x38b764, 0x257179, 0x29366f,
                              0x3b5dc9, 0x41a6f6, 0x73eff7, 0xf4f4f4, 0x94b0c2, 0x566c86, 0x333c57, 0x8a4836, 0xd8a070, 0xffe8c8,
                              0x6a3a2a, 0x2a7a3a, 0xc83a3a, 0xffffff };

} // namespace

void EditorDisegno::Apri(Disegno* nd, const std::string& nid) {
    if (nid == id && !id.empty()) { d = nd; return; }   // stesso disegno (magari spostato in memoria): la storia resta
    d = nd;
    id = nid;
    anim = fot = 0;
    annulla.clear(); ripeti.clear();
    tratto = false;
    adattaVista = true;
    if (d) {
        if (d->animazioni.empty()) { d->animazioni.push_back({}); }
        for (auto& a : d->animazioni) if (a.fotogrammi.empty()) { Fotogramma f; f.Nuovo(d->w, d->h); a.fotogrammi.push_back(f); }
        nuovaW = d->w; nuovaH = d->h;
        pennello = d->w > 64 ? 3 : 1;
        RaccogliTavolozza();
    }
}

Fotogramma& EditorDisegno::F() {
    anim = (int)Limita((float)anim, 0, (float)d->animazioni.size() - 1);
    auto& a = d->animazioni[anim];
    if (a.fotogrammi.empty()) { Fotogramma f; f.Nuovo(d->w, d->h); a.fotogrammi.push_back(f); }
    fot = (int)Limita((float)fot, 0, (float)a.fotogrammi.size() - 1);
    return a.fotogrammi[fot];
}

void EditorDisegno::Ricorda() {
    annulla.push_back({ d->w, d->h, d->animazioni });
    if (annulla.size() > 50) annulla.erase(annulla.begin());
    ripeti.clear();
}

void EditorDisegno::Annulla() {
    if (annulla.empty() || !d) return;
    ripeti.push_back({ d->w, d->h, d->animazioni });
    d->w = annulla.back().w; d->h = annulla.back().h; d->animazioni = annulla.back().animazioni;
    annulla.pop_back();
    d->Invalida();
    cambiato = true;
}

void EditorDisegno::Ripeti() {
    if (ripeti.empty() || !d) return;
    annulla.push_back({ d->w, d->h, d->animazioni });
    d->w = ripeti.back().w; d->h = ripeti.back().h; d->animazioni = ripeti.back().animazioni;
    ripeti.pop_back();
    d->Invalida();
    cambiato = true;
}

void EditorDisegno::AggiornaColoreDaHsv() {
    unsigned char a = colore.a ? colore.a : 255;
    colore = ColorFromHSV(hue, sat, val);
    colore.a = a;
}

void EditorDisegno::ColoreScelto(Color c) {
    colore = c;
    if (c.a > 0) { Vector3 v = ColorToHSV(c); hue = v.x; sat = v.y; val = v.z; }
}

void EditorDisegno::RaccogliTavolozza() {
    tavolozza.clear();
    for (unsigned int b : BASE) tavolozza.push_back(Hex(b));
    if (!d) return;
    // I colori usati nel disegno (i piu' frequenti), cosi' si riprendono subito.
    std::vector<std::pair<int, Color>> usati;
    for (auto& a : d->animazioni)
        for (auto& f : a.fotogrammi)
            for (size_t i = 0; i < f.px.size(); i += (f.px.size() > 8000 ? 3 : 1)) {
                Color c = f.px[i];
                if (c.a < 128) continue;
                c.a = 255;
                bool trovato = false;
                for (auto& u : usati) if (std::abs(u.second.r - c.r) + std::abs(u.second.g - c.g) + std::abs(u.second.b - c.b) < 18) { u.first++; trovato = true; break; }
                if (!trovato && usati.size() < 200) usati.push_back({ 1, c });
            }
    std::sort(usati.begin(), usati.end(), [](const std::pair<int, Color>& x, const std::pair<int, Color>& y) { return x.first > y.first; });
    for (size_t i = 0; i < usati.size() && tavolozza.size() < 48; i++) tavolozza.push_back(usati[i].second);
}

void EditorDisegno::Punto(Fotogramma& f, int x, int y, Color c) {
    int r = pennello - 1;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (pennello > 2 && dx * dx + dy * dy > r * r + r) continue;   // pennello tondo
            f.Metti(x + dx, y + dy, c);
            if (specchio) f.Metti(f.w - 1 - (x + dx), y + dy, c);
        }
}

void EditorDisegno::LineaPixel(Fotogramma& f, int ax, int ay, int bx, int by, Color c) {
    int dx = std::abs(bx - ax), dy = -std::abs(by - ay), sx = ax < bx ? 1 : -1, sy = ay < by ? 1 : -1, e = dx + dy;
    for (int i = 0; i < 4096; i++) {
        Punto(f, ax, ay, c);
        if (ax == bx && ay == by) break;
        int e2 = 2 * e;
        if (e2 >= dy) { e += dy; ax += sx; }
        if (e2 <= dx) { e += dx; ay += sy; }
    }
}

void EditorDisegno::Forma(Fotogramma& f, int ax, int ay, int bx, int by, Color c) {
    int x0 = std::min(ax, bx), x1 = std::max(ax, bx), y0 = std::min(ay, by), y1 = std::max(ay, by);
    if (attrezzo == Rettangolo) {
        if (pieno) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { f.Metti(x, y, c); if (specchio) f.Metti(f.w - 1 - x, y, c); } }
        else { LineaPixel(f, x0, y0, x1, y0, c); LineaPixel(f, x0, y1, x1, y1, c); LineaPixel(f, x0, y0, x0, y1, c); LineaPixel(f, x1, y0, x1, y1, c); }
        return;
    }
    // Ellisse dentro il rettangolo.
    float cx = (x0 + x1) / 2.0f, cy = (y0 + y1) / 2.0f, rx = (x1 - x0) / 2.0f + 0.5f, ry = (y1 - y0) / 2.0f + 0.5f;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float nx = (x - cx) / rx, ny = (y - cy) / ry, q = nx * nx + ny * ny;
            if (q > 1) continue;
            if (!pieno) {
                // Bordo: un vicino e' fuori.
                auto Fuori = [&](int xx, int yy) { float a = (xx - cx) / rx, b = (yy - cy) / ry; return a * a + b * b > 1; };
                if (!Fuori(x + 1, y) && !Fuori(x - 1, y) && !Fuori(x, y + 1) && !Fuori(x, y - 1)) continue;
            }
            Punto(f, x, y, c);
        }
}

void EditorDisegno::Riempi(Fotogramma& f, int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= f.w || y >= f.h) return;
    Color da = f.Prendi(x, y);
    if (Uguali(da, c)) return;
    std::vector<std::pair<int, int>> pila{ { x, y } };
    while (!pila.empty()) {
        auto p = pila.back();
        pila.pop_back();
        if (p.first < 0 || p.second < 0 || p.first >= f.w || p.second >= f.h || !Uguali(f.Prendi(p.first, p.second), da)) continue;
        f.Metti(p.first, p.second, c);
        pila.push_back({ p.first + 1, p.second }); pila.push_back({ p.first - 1, p.second });
        pila.push_back({ p.first, p.second + 1 }); pila.push_back({ p.first, p.second - 1 });
    }
}

bool EditorDisegno::Importa(const std::string& file, std::string& messaggio) {
    if (!d) return false;
    Image img = LoadImage(file.c_str());
    if (!img.data) { messaggio = "Non riesco ad aprire " + file; return false; }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    // Foglio di sprite: piu' fotogrammi quadrati (o della misura del disegno) uno accanto all'altro.
    int fw = img.width, fh = img.height;
    if (img.width >= img.height * 2 && img.width % img.height == 0) fw = img.height;
    else if (img.width % d->w == 0 && img.height == d->h && img.width > d->w) fw = d->w;
    int n = std::max(1, img.width / fw);
    Ricorda();
    // Se l'immagine e' piu' grande del disegno, il disegno si allarga (fino a 256).
    int nw = std::min(256, std::max(d->w, fw)), nh = std::min(256, std::max(d->h, fh));
    if (nw != d->w || nh != d->h) d->Ridimensiona(nw, nh);
    auto& a = d->animazioni[anim];
    bool vuota = a.fotogrammi.size() == 1 && a.fotogrammi[0].Vuoto();
    if (vuota) a.fotogrammi.clear();
    for (int i = 0; i < n; i++) {
        Fotogramma f = FotogrammaDaImmagine(img, { (float)(i * fw), 0, (float)fw, (float)fh });
        Fotogramma g;
        g.Nuovo(d->w, d->h);
        int ox = (d->w - f.w) / 2, oy = d->h - f.h;
        for (int y = 0; y < f.h; y++) for (int x = 0; x < f.w; x++) g.Metti(x + ox, y + oy, f.Prendi(x, y));
        a.fotogrammi.push_back(g);
    }
    UnloadImage(img);
    fot = (int)a.fotogrammi.size() - n;
    d->Invalida();
    RaccogliTavolozza();
    cambiato = true;
    messaggio = n > 1 ? TextFormat("Importati %d fotogrammi da %s", n, GetFileName(file.c_str())) : std::string("Importato ") + GetFileName(file.c_str());
    return true;
}

bool EditorDisegno::EsportaPng(const std::string& file) const {
    if (!d || d->animazioni.empty()) return false;
    // Foglio di sprite: una riga per animazione.
    int righe = (int)d->animazioni.size(), colonne = 1;
    for (auto& a : d->animazioni) colonne = std::max(colonne, (int)a.fotogrammi.size());
    Image img = GenImageColor(d->w * colonne, d->h * righe, BLANK);
    for (int r = 0; r < righe; r++)
        for (int c = 0; c < (int)d->animazioni[r].fotogrammi.size(); c++) {
            const Fotogramma& f = d->animazioni[r].fotogrammi[c];
            for (int y = 0; y < f.h; y++) for (int x = 0; x < f.w; x++) ImageDrawPixel(&img, c * d->w + x, r * d->h + y, f.Prendi(x, y));
        }
    bool ok = ExportImage(img, file.c_str());
    UnloadImage(img);
    return ok;
}

// --- Fotogramma dell'editor -----------------------------------------------------------

bool EditorDisegno::Frame(Rectangle r, float dt) {
    cambiato = false;
    if (!d) return false;
    float s = Ui::S();
    F();
    tAnteprima += dt;
    // L'intestazione va su due righe quando lo spazio e' stretto (schermi piccoli, chat aperta).
    float hTesta = (r.width < 1030 * s ? 76 : 40) * s, hBasso = 118 * s, wSx = 86 * s, wDx = 262 * s;
    Rectangle testa = { r.x, r.y, r.width, hTesta };
    Rectangle sx = { r.x, r.y + hTesta, wSx, r.height - hTesta - hBasso };
    Rectangle dx = { r.x + r.width - wDx, r.y + hTesta, wDx, r.height - hTesta };
    Rectangle basso = { r.x, r.y + r.height - hBasso, r.width - wDx, hBasso };
    Rectangle centro = { r.x + wSx, r.y + hTesta, r.width - wSx - wDx, r.height - hTesta - hBasso };

    // Scorciatoie (solo con il mouse sopra l'editor e senza un campo di testo attivo).
    if (!Ui::StaScrivendo() && CheckCollisionPointRec(GetMousePosition(), r)) {
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrl && IsKeyPressed(KEY_Z)) { if (IsKeyDown(KEY_LEFT_SHIFT)) Ripeti(); else Annulla(); }
        else if (ctrl && IsKeyPressed(KEY_Y)) Ripeti();
        else if (!ctrl) {
            for (int i = 0; i < NumAttrezzi; i++) if (IsKeyPressed(KEY_ATTREZZI[i])) attrezzo = i;
            if (IsKeyPressed(KEY_LEFT_BRACKET)) pennello = std::max(1, pennello - 1);
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) pennello = std::min(12, pennello + 1);
            if (IsKeyPressed(KEY_COMMA)) fot = (fot + (int)d->animazioni[anim].fotogrammi.size() - 1) % (int)d->animazioni[anim].fotogrammi.size();
            if (IsKeyPressed(KEY_PERIOD)) fot = (fot + 1) % (int)d->animazioni[anim].fotogrammi.size();
            if (IsKeyPressed(KEY_X)) std::swap(colore, secondo);
        }
    }

    DrawRectangleRec(r, Hex(0x11142e));
    Tela(centro, dt);
    Intestazione(testa);
    Attrezzi(sx);
    Fotogrammi(basso);
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    float hColori = std::min(dx.height * 0.52f, 400 * s);
    Colori({ dx.x + 10 * s, dx.y + 8 * s, dx.width - 20 * s, hColori });
    Animazioni({ dx.x + 10 * s, dx.y + hColori + 16 * s, dx.width - 20 * s, dx.height - hColori - 24 * s });
    if (cambiato) d->Invalida();
    return cambiato;
}

void EditorDisegno::Intestazione(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x0f1230));
    bool dueRighe = r.height > 60 * s;
    float h = (dueRighe ? 28 : r.height / s - 12) * s;
    float x = r.x + 10 * s, y = r.y + 6 * s;
    // Ogni elemento va avanti nella riga; se non ci sta, passa alla riga sotto.
    auto Posto = [&](float w) {
        if (x + w > r.x + r.width - 6 * s && x > r.x + 10 * s && dueRighe && y < r.y + 10 * s) { x = r.x + 10 * s; y += h + 8 * s; }
        Rectangle q = { x, y, w, h };
        x += w + 6 * s;
        return q;
    };
    std::string info = TextFormat("%d x %d pixel  -  %s", d->w, d->h, d->liscio ? "HD" : "pixel art");
    Rectangle qi = Posto((float)Ui::Larghezza(info, Ui::TP()) + 8 * s);
    Ui::Testo(info, qi.x, qi.y + (h - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    Ui::NumeroIntero("ed_w", Posto(150 * s), "Larg.", nuovaW, 4, 256);
    Ui::NumeroIntero("ed_h", Posto(150 * s), "Alt.", nuovaH, 4, 256);
    if (Ui::Pulsante(Posto(104 * s), "Ridimensiona", false, nuovaW != d->w || nuovaH != d->h)) {
        Ricorda();
        d->Ridimensiona(nuovaW, nuovaH);
        adattaVista = true;
        cambiato = true;
    }
    if (Ui::Pulsante(Posto(96 * s), "Specchia")) {
        Ricorda();
        Fotogramma& f = F();
        for (int yy = 0; yy < f.h; yy++) for (int xx = 0; xx < f.w / 2; xx++) { Color a = f.Prendi(xx, yy); f.Metti(xx, yy, f.Prendi(f.w - 1 - xx, yy)); f.Metti(f.w - 1 - xx, yy, a); }
        cambiato = true;
    }
    Ui::Aiuto("Cancella tutto il fotogramma (si annulla con Ctrl+Z).");
    if (Ui::Pulsante(Posto(80 * s), "Pulisci")) { Ricorda(); F().Nuovo(d->w, d->h); cambiato = true; }
    if (Ui::Pulsante(Posto(80 * s), "Annulla", false, !annulla.empty())) Annulla();
    if (Ui::Pulsante(Posto(80 * s), "Ripeti", false, !ripeti.empty())) Ripeti();
}

void EditorDisegno::Attrezzi(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Ui::Pannello());
    float pad = 6 * s, h = 34 * s, y = r.y + pad;
    for (int i = 0; i < NumAttrezzi; i++) {
        Rectangle b = { r.x + pad, y, r.width - 2 * pad, h };
        if (Ui::Pulsante(b, NOMI_ATTREZZI[i], attrezzo == i)) attrezzo = i;
        if (Ui::MouseSu(b)) Ui::Testo(TASTI_ATTREZZI[i], b.x + b.width - 10 * s, b.y + 2 * s, Ui::TestoSpento(), (int)(10 * s));
        y += h + 4 * s;
    }
    y += 6 * s;
    Ui::Testo(TextFormat("Punta %d", pennello), r.x + pad, y, Ui::TestoSpento(), Ui::TP());
    y += Ui::TP() + 4 * s;
    float bw = (r.width - 3 * pad) / 2;
    if (Ui::Pulsante({ r.x + pad, y, bw, h * 0.8f }, "-")) pennello = std::max(1, pennello - 1);
    if (Ui::Pulsante({ r.x + 2 * pad + bw, y, bw, h * 0.8f }, "+")) pennello = std::min(12, pennello + 1);
    y += h * 0.8f + 8 * s;
    auto Leva = [&](const char* t, bool& v) {
        if (Ui::Pulsante({ r.x + pad, y, r.width - 2 * pad, h * 0.85f }, t, v)) v = !v;
        y += h * 0.85f + 4 * s;
    };
    Leva("Pieno", pieno);
    Leva("Simmetria", specchio);
    Leva("Cipolla", cipolla);
    Leva("Griglia", griglia);
}

void EditorDisegno::Tela(Rectangle r, float dt) {
    (void)dt;
    float s = Ui::S();
    Fotogramma& f = F();
    if (adattaVista) {
        zoom = std::max(1.0f, std::floor(std::min((r.width - 40 * s) / f.w, (r.height - 40 * s) / f.h)));
        panX = panY = 0;
        adattaVista = false;
    }
    // Texture della tela (filtro a pixel netti per lavorare).
    if (tela.id == 0 || telaW != f.w || telaH != f.h) {
        if (tela.id) UnloadTexture(tela);
        Image img = GenImageColor(f.w, f.h, BLANK);
        tela = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tela, TEXTURE_FILTER_POINT);
        telaW = f.w; telaH = f.h;
    }
    float w = f.w * zoom, h = f.h * zoom;
    rTela = { std::round(r.x + (r.width - w) / 2 + panX), std::round(r.y + (r.height - h) / 2 + panY), w, h };
    Ui::Ritaglia((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    Scacchiera(rTela, std::max(4.0f, zoom * (f.w > 64 ? 4 : 1)));
    // Buccia di cipolla: il fotogramma precedente, trasparente.
    auto& a = d->animazioni[anim];
    if (cipolla && a.fotogrammi.size() > 1) {
        const Fotogramma& p = a.fotogrammi[(fot + a.fotogrammi.size() - 1) % a.fotogrammi.size()];
        if (p.w == f.w && p.h == f.h) {
            UpdateTexture(tela, p.px.data());
            DrawTexturePro(tela, { 0, 0, (float)f.w, (float)f.h }, rTela, { 0, 0 }, 0, Hex(0xffffff, 70));
        }
    }
    UpdateTexture(tela, f.px.data());
    DrawTexturePro(tela, { 0, 0, (float)f.w, (float)f.h }, rTela, { 0, 0 }, 0, WHITE);
    if (griglia && zoom >= 6) {
        Color cg = Hex(0xffffff, 26);
        for (int x = 0; x <= f.w; x++) DrawLineV({ rTela.x + x * zoom, rTela.y }, { rTela.x + x * zoom, rTela.y + h }, (x % 8 == 0) ? Hex(0xffffff, 50) : cg);
        for (int y = 0; y <= f.h; y++) DrawLineV({ rTela.x, rTela.y + y * zoom }, { rTela.x + w, rTela.y + y * zoom }, (y % 8 == 0) ? Hex(0xffffff, 50) : cg);
    }
    if (specchio) DrawLineEx({ rTela.x + w / 2, rTela.y }, { rTela.x + w / 2, rTela.y + h }, 2, Hex(0x7ee0ff, 140));
    DrawRectangleLinesEx({ rTela.x - 1, rTela.y - 1, w + 2, h + 2 }, 1, Ui::Bordo());

    // Mouse sulla tela.
    Vector2 m = GetMousePosition();
    bool sopra = Ui::MouseSu(r);
    int px = (int)std::floor((m.x - rTela.x) / zoom), py = (int)std::floor((m.y - rTela.y) / zoom);
    if (sopra) {
        float rot = GetMouseWheelMove();
        if (rot != 0) {
            float vecchio = zoom;
            zoom = Limita(rot > 0 ? zoom * 1.25f : zoom / 1.25f, 1, 64);
            zoom = std::max(1.0f, std::round(zoom));
            // Lo zoom resta centrato sul mouse.
            panX += (m.x - (rTela.x + w / 2)) * (1 - zoom / vecchio);
            panY += (m.y - (rTela.y + h / 2)) * (1 - zoom / vecchio);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) { trascina = true; trascinaDa = m; panDa = { panX, panY }; }
        // Contorno della punta sotto il mouse.
        if (px >= -pennello && py >= -pennello && px < f.w + pennello && py < f.h + pennello && !trascina) {
            float rr = (float)(pennello - 1);
            DrawRectangleLinesEx({ rTela.x + (px - rr) * zoom, rTela.y + (py - rr) * zoom, (2 * rr + 1) * zoom, (2 * rr + 1) * zoom }, 1, Hex(0xffffff, 160));
        }
    }
    if (trascina) {
        panX = panDa.x + m.x - trascinaDa.x; panY = panDa.y + m.y - trascinaDa.y;
        if (!IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) trascina = false;
    } else {
        bool sx = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), dx = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if (sopra && (sx || dx) && !tratto) {
            tratto = true;
            tastoDestro = dx;
            x0 = ux = px; y0 = uy = py;
            Color c = tastoDestro ? secondo : colore;
            if (attrezzo == Contagocce) { Color p = f.Prendi(px, py); if (tastoDestro) secondo = p; else ColoreScelto(p); tratto = false; }
            else {
                Ricorda();
                primaDelTratto = f;
                if (attrezzo == Matita) Punto(f, px, py, c);
                else if (attrezzo == Gomma) Punto(f, px, py, BLANK);
                else if (attrezzo == Secchio) { Riempi(f, px, py, c); if (specchio) Riempi(f, f.w - 1 - px, py, c); tratto = false; }
                cambiato = true;
            }
        } else if (tratto) {
            bool giu = tastoDestro ? IsMouseButtonDown(MOUSE_BUTTON_RIGHT) : IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            Color c = tastoDestro ? secondo : colore;
            if (attrezzo == Matita || attrezzo == Gomma) {
                if (px != ux || py != uy) { LineaPixel(f, ux, uy, px, py, attrezzo == Gomma ? BLANK : c); ux = px; uy = py; cambiato = true; }
            } else if (attrezzo == Linea || attrezzo == Rettangolo || attrezzo == Ellisse) {
                if (px != ux || py != uy) {
                    f = primaDelTratto;
                    if (attrezzo == Linea) LineaPixel(f, x0, y0, px, py, c); else Forma(f, x0, y0, px, py, c);
                    ux = px; uy = py;
                    cambiato = true;
                }
            } else if (attrezzo == Sposta) {
                if (px != ux || py != uy) {
                    Fotogramma g;
                    g.Nuovo(f.w, f.h);
                    int ox = px - x0, oy = py - y0;
                    for (int y = 0; y < f.h; y++) for (int x = 0; x < f.w; x++) g.Metti(x + ox, y + oy, primaDelTratto.Prendi(x, y));
                    f = g;
                    ux = px; uy = py;
                    cambiato = true;
                }
            }
            if (!giu) {
                tratto = false;
                if (attrezzo == Linea || attrezzo == Rettangolo || attrezzo == Ellisse) {
                    f = primaDelTratto;
                    if (attrezzo == Linea) LineaPixel(f, x0, y0, px, py, c); else Forma(f, x0, y0, px, py, c);
                    cambiato = true;
                }
            }
        }
    }
    Ui::FineRitaglio();
    if (sopra && px >= 0 && py >= 0 && px < f.w && py < f.h)
        Ui::Testo(TextFormat("%d, %d   zoom %.0fx", px, py, zoom), r.x + 8 * s, r.y + r.height - Ui::TP() - 6 * s, Ui::TestoSpento(), Ui::TP());
    else
        Ui::Testo("Sinistro: colore 1  Destro: colore 2  Rotella: zoom  Centrale/Spazio: sposta la vista  X: scambia i colori",
                  r.x + 8 * s, r.y + r.height - Ui::TP() - 6 * s, Ui::TestoSpento(), (int)(12 * s));
}

void EditorDisegno::Colori(Rectangle r) {
    float s = Ui::S();
    float y = r.y;
    // Quadrato saturazione / luminosita' e barra della tinta.
    float lato = std::min(r.width - 34 * s, r.height * 0.42f);
    Rectangle q = { r.x, y, lato, lato };
    Rectangle barra = { r.x + lato + 8 * s, y, 22 * s, lato };
    int passi = 24;
    for (int i = 0; i < passi; i++) {
        float t0 = i / (float)passi;
        Color sopra = ColorFromHSV(hue, t0, 1), sotto = ColorFromHSV(hue, t0, 0);
        DrawRectangleGradientV((int)(q.x + t0 * lato), (int)q.y, (int)std::ceil(lato / passi) + 1, (int)lato, sopra, sotto);
    }
    for (int i = 0; i < 36; i++) DrawRectangle((int)barra.x, (int)(barra.y + i * lato / 36), (int)barra.width, (int)std::ceil(lato / 36) + 1, ColorFromHSV(i * 10.0f, 1, 1));
    DrawRectangleLinesEx(q, 1, Ui::Bordo());
    DrawRectangleLinesEx(barra, 1, Ui::Bordo());
    DrawCircleLinesV({ q.x + sat * lato, q.y + (1 - val) * lato }, 5 * s, WHITE);
    DrawRectangleLinesEx({ barra.x - 2, barra.y + hue / 360 * lato - 2, barra.width + 4, 4 }, 2, WHITE);
    Vector2 m = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (Ui::MouseSu(q)) { sat = Limita((m.x - q.x) / lato, 0, 1); val = Limita(1 - (m.y - q.y) / lato, 0, 1); colore.a = 255; AggiornaColoreDaHsv(); }
        if (Ui::MouseSu(barra)) { hue = Limita((m.y - barra.y) / lato, 0, 0.999f) * 360; colore.a = 255; AggiornaColoreDaHsv(); }
    }
    y += lato + 8 * s;
    // Colore 1 e 2, codice esadecimale e trasparenza.
    float c = 34 * s;
    Scacchiera({ r.x, y, c, c }, 6 * s);
    DrawRectangleRec({ r.x, y, c, c }, colore);
    DrawRectangleLinesEx({ r.x, y, c, c }, 2, Ui::Accento());
    Scacchiera({ r.x + c * 0.6f + c, y + c * 0.25f, c * 0.75f, c * 0.75f }, 6 * s);
    DrawRectangleRec({ r.x + c * 0.6f + c, y + c * 0.25f, c * 0.75f, c * 0.75f }, secondo);
    DrawRectangleLinesEx({ r.x + c * 0.6f + c, y + c * 0.25f, c * 0.75f, c * 0.75f }, 1, Ui::Bordo());
    if (Ui::Cliccato({ r.x + c * 0.6f + c, y + c * 0.25f, c * 0.75f, c * 0.75f })) std::swap(colore, secondo);
    std::string hex = ColoreInTesto(colore);
    Rectangle campo = { r.x + c * 2.6f, y + 4 * s, r.width - c * 2.6f - 64 * s, c - 8 * s };
    if (Ui::Campo("ed_hex", campo, hex, 9)) { Color nc; if (TestoInColore(hex, nc)) ColoreScelto(nc); }
    if (Ui::Pulsante({ r.x + r.width - 58 * s, y + 4 * s, 58 * s, c - 8 * s }, "Vuoto", colore.a == 0)) colore = BLANK;
    y += c + 6 * s;
    float alfa = colore.a / 255.0f * 100;
    if (Ui::Numero("ed_alfa", { r.x, y, r.width, 26 * s }, "Opacita' %", alfa, 0, 100, 10)) colore.a = (unsigned char)(alfa * 2.55f);
    y += 32 * s;
    // Tavolozza.
    int col = 8;
    float l = (r.width - (col - 1) * 3 * s) / col;
    for (size_t i = 0; i < tavolozza.size(); i++) {
        Rectangle b = { r.x + (i % col) * (l + 3 * s), y + (i / col) * (l + 3 * s), l, l };
        if (b.y + b.height > r.y + r.height) break;
        DrawRectangleRec(b, tavolozza[i]);
        bool sel = Uguali(tavolozza[i], colore);
        DrawRectangleLinesEx(b, sel ? 2.0f : 1.0f, sel ? Ui::Accento() : Hex(0x000000, 120));
        if (Ui::Cliccato(b)) ColoreScelto(tavolozza[i]);
        if (Ui::Cliccato(b, MOUSE_BUTTON_RIGHT)) secondo = tavolozza[i];
    }
}

void EditorDisegno::Animazioni(Rectangle r) {
    float s = Ui::S();
    float y = r.y;
    auto& anims = d->animazioni;
    // Anteprima animata.
    float lato = std::min(r.width * 0.5f, 120 * s);
    Rectangle prev = { r.x, y, lato, lato };
    Scacchiera(prev, 8 * s);
    float t = riproduci ? tAnteprima : 0;
    if (riproduci) d->DisegnaIn(prev, anim, t); else { d->Invalida(); d->DisegnaIn(prev, anim, 0); }
    DrawRectangleLinesEx(prev, 1, Ui::Bordo());
    float bx = r.x + lato + 8 * s, bw = r.width - lato - 8 * s;
    if (Ui::Pulsante({ bx, y, bw, 28 * s }, riproduci ? "Pausa" : "Riproduci", riproduci)) riproduci = !riproduci;
    float fps = anims[anim].fps;
    Ui::Aiuto("Velocita' dell'animazione: fotogrammi al secondo.");
    if (Ui::Numero("ed_fps", { bx, y + 34 * s, bw, 26 * s }, "", fps, 1, 30, 1, "%.0f fps")) { anims[anim].fps = fps; cambiato = true; }
    bool rip = anims[anim].ripeti;
    if (Ui::Interruttore({ bx, y + 66 * s, bw, 26 * s }, "Ciclo", rip)) { anims[anim].ripeti = rip; cambiato = true; }
    y += lato + 10 * s;
    Ui::Testo("ANIMAZIONI", r.x, y, Ui::Accento(), Ui::TP());
    y += Ui::TP() + 6 * s;
    // Nome dell'animazione scelta (si puo' cambiare).
    std::string nome = anims[anim].nome;
    if (Ui::Campo("ed_nomeanim", { r.x, y, r.width, 26 * s }, nome, 30) && !nome.empty()) { Ricorda(); anims[anim].nome = nome; cambiato = true; }
    y += 32 * s;
    float h = 26 * s;
    float altezzaElenco = r.y + r.height - y - 2 * h - 14 * s;
    Rectangle elenco = { r.x, y, r.width, std::max(h, altezzaElenco) };
    Ui::Ritaglia((int)elenco.x, (int)elenco.y, (int)elenco.width, (int)elenco.height);
    if (Ui::MouseSu(elenco)) scorriAnimazioni = std::max(0.0f, scorriAnimazioni - GetMouseWheelMove() * 40 * s);
    for (int i = 0; i < (int)anims.size(); i++) {
        Rectangle b = { r.x, y + i * (h + 3 * s) - scorriAnimazioni, r.width, h };
        if (b.y + b.height < elenco.y || b.y > elenco.y + elenco.height) continue;
        std::string t = anims[i].nome + TextFormat("  (%d)", (int)anims[i].fotogrammi.size());
        if (Ui::Pulsante(b, t, i == anim) && CheckCollisionPointRec(GetMousePosition(), elenco)) { anim = i; fot = 0; tAnteprima = 0; }
    }
    Ui::FineRitaglio();
    y = r.y + r.height - 2 * h - 4 * s;
    Ui::Aiuto("Apre il gestore delle immagini: PNG, BMP, JPG, GIF animate, TGA, anche piu' file insieme o fogli di sprite.");
    if (Ui::Pulsante({ r.x, y, r.width, h }, "Carica immagini...", true)) {
        std::vector<std::string> f = ScegliFile("Scegli le immagini per le animazioni", "Immagini", EstensioniImmagini(), true);
        if (!f.empty()) PreparaImport(f);
    }
    y = r.y + r.height - h;
    float w3 = (r.width - 8 * s) / 3;
    // Nomi utili per il motore: fermo, corsa, salto, caduta, colpito, vittoria, attacco, aperto, acceso, interno...
    if (Ui::Pulsante({ r.x, y, w3, h }, "+ Nuova")) {
        Ricorda();
        static const char* nomi[] = { "fermo", "corsa", "salto", "caduta", "colpito", "attacco", "vittoria", "aperta", "chiusa", "accesa", "spenta" };
        std::string n = "animazione";
        for (auto nn : nomi) if (d->TrovaAnimazione(nn) < 0) { n = nn; break; }
        ::Animazione a;
        a.nome = n;
        a.fotogrammi.push_back(F());   // parte dal fotogramma attuale
        anims.push_back(a);
        anim = (int)anims.size() - 1; fot = 0;
        cambiato = true;
    }
    if (Ui::Pulsante({ r.x + w3 + 4 * s, y, w3, h }, "Duplica")) { Ricorda(); ::Animazione a = anims[anim]; a.nome += " 2"; anims.push_back(a); anim = (int)anims.size() - 1; cambiato = true; }
    if (Ui::Pulsante({ r.x + 2 * (w3 + 4 * s), y, w3, h }, "Elimina", false, anims.size() > 1)) { Ricorda(); anims.erase(anims.begin() + anim); anim = std::max(0, anim - 1); fot = 0; cambiato = true; }
}

void EditorDisegno::Fotogrammi(Rectangle r) {
    float s = Ui::S();
    DrawRectangleRec(r, Hex(0x0f1230));
    DrawLineEx({ r.x, r.y }, { r.x + r.width, r.y }, 1, Ui::Bordo());
    auto& a = d->animazioni[anim];
    float pad = 6 * s;
    // Riga in alto: titolo a sinistra, pulsanti a destra (con nomi corti se lo spazio e' poco).
    float hRiga = 24 * s;
    bool stretto = r.width < 520 * s;
    const char* nomi[5] = { stretto ? "+" : "+ Vuoto", stretto ? "Copia" : "+ Copia", "<", ">", stretto ? "X" : "Elimina" };
    float larg[5] = { stretto ? 26 * s : 72 * s, stretto ? 52 * s : 72 * s, 26 * s, 26 * s, stretto ? 26 * s : 64 * s };
    float wTot = 4 * 4 * s;
    for (float l : larg) wTot += l;
    float bx = r.x + r.width - pad - wTot, by = r.y + pad * 0.6f;
    std::string titolo = TextFormat("Fotogrammi di \"%s\": %d", a.nome.c_str(), (int)a.fotogrammi.size());
    Ui::Ritaglia((int)(r.x + pad), (int)r.y, (int)std::max(0.0f, bx - r.x - 2 * pad), (int)(hRiga + pad));
    Ui::Testo(titolo, r.x + pad, by + (hRiga - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    Ui::FineRitaglio();
    int premuto = -1;
    for (int i = 0; i < 5; i++) {
        bool abil = i == 2 ? fot > 0 : i == 3 ? fot + 1 < (int)a.fotogrammi.size() : i == 4 ? a.fotogrammi.size() > 1 : true;
        if (i == 2) Ui::Aiuto("Sposta il fotogramma scelto a sinistra (prima nell'animazione).");
        if (i == 3) Ui::Aiuto("Sposta il fotogramma scelto a destra (dopo nell'animazione).");
        if (Ui::Pulsante({ bx, by, larg[i], hRiga }, nomi[i], false, abil)) premuto = i;
        bx += larg[i] + 4 * s;
    }
    if (premuto == 0) { Ricorda(); Fotogramma f; f.Nuovo(d->w, d->h); a.fotogrammi.insert(a.fotogrammi.begin() + fot + 1, f); fot++; cambiato = true; }
    if (premuto == 1) { Ricorda(); Fotogramma f = F(); a.fotogrammi.insert(a.fotogrammi.begin() + fot + 1, f); fot++; cambiato = true; }
    if (premuto == 2) { Ricorda(); std::swap(a.fotogrammi[fot], a.fotogrammi[fot - 1]); fot--; cambiato = true; }
    if (premuto == 3) { Ricorda(); std::swap(a.fotogrammi[fot], a.fotogrammi[fot + 1]); fot++; cambiato = true; }
    if (premuto == 4) { Ricorda(); a.fotogrammi.erase(a.fotogrammi.begin() + fot); fot = std::max(0, fot - 1); cambiato = true; }

    // La sequenza: tutta la larghezza, scorre con la rotella (anche senza Maiusc) o con la barra.
    float hBarra = 6 * s;
    Rectangle striscia = { r.x + pad, r.y + hRiga + pad * 1.6f, r.width - 2 * pad, r.height - hRiga - pad * 2.6f - hBarra - 2 * s };
    float lato = striscia.height;
    float totale = a.fotogrammi.size() * (lato + pad) - pad;
    float massimo = std::max(0.0f, totale - striscia.width);
    if (Ui::MouseSu(striscia)) {
        float rot = GetMouseWheelMove();
        scorriFotogrammi -= rot * (lato + pad);
    }
    // Il fotogramma scelto resta sempre in vista (anche quando si sfoglia con , e .).
    if (premuto >= 0 || IsKeyPressed(KEY_COMMA) || IsKeyPressed(KEY_PERIOD)) {
        float x0 = fot * (lato + pad);
        if (x0 < scorriFotogrammi) scorriFotogrammi = x0;
        if (x0 + lato > scorriFotogrammi + striscia.width) scorriFotogrammi = x0 + lato - striscia.width;
    }
    scorriFotogrammi = Limita(scorriFotogrammi, 0, massimo);
    Ui::Ritaglia((int)striscia.x, (int)striscia.y, (int)striscia.width, (int)striscia.height);
    for (int i = 0; i < (int)a.fotogrammi.size(); i++) {
        Rectangle b = { striscia.x + i * (lato + pad) - scorriFotogrammi, striscia.y, lato, lato };
        if (b.x > striscia.x + striscia.width || b.x + b.width < striscia.x) continue;
        Scacchiera(b, 6 * s);
        d->DisegnaIn({ b.x + 2, b.y + 2, b.width - 4, b.height - 4 }, anim, i / std::max(1.0f, a.fps));
        bool sel = i == fot;
        DrawRectangleLinesEx(b, sel ? 3.0f : 1.0f, sel ? Ui::Accento() : Ui::MouseSu(b) ? Hex(0x5a64b0) : Ui::Bordo());
        Ui::Testo(std::to_string(i + 1), b.x + 4 * s, b.y + 2 * s, Ui::TestoSpento(), (int)(11 * s));
        if (Ui::Cliccato(b) && CheckCollisionPointRec(GetMousePosition(), striscia)) fot = i;
    }
    Ui::FineRitaglio();
    // Barra di scorrimento (solo se la sequenza e' piu' lunga dello spazio): si trascina.
    if (massimo > 0) {
        Rectangle barra = { striscia.x, striscia.y + striscia.height + 3 * s, striscia.width, hBarra };
        DrawRectangleRounded(barra, 1, 6, Hex(0x1c2150));
        float wm = std::max(30 * s, barra.width * striscia.width / totale);
        float xm = barra.x + (barra.width - wm) * scorriFotogrammi / massimo;
        Rectangle maniglia = { xm, barra.y, wm, hBarra };
        static bool trascina = false;
        if (Ui::Cliccato({ barra.x, barra.y - 4 * s, barra.width, barra.height + 8 * s })) trascina = true;
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) trascina = false;
        if (trascina) scorriFotogrammi = Limita((GetMousePosition().x - barra.x - wm / 2) / std::max(1.0f, barra.width - wm) * massimo, 0, massimo);
        DrawRectangleRounded(maniglia, 1, 6, trascina || Ui::MouseSu(barra) ? Ui::Accento() : Hex(0x5a64b0));
    }
}

// --- Gestore delle immagini ------------------------------------------------------------------

namespace {
const char* AZIONI_MOTORE[] = { "fermo", "corsa", "salto", "caduta", "colpito", "attacco", "sparo", "vittoria", "aperta", "chiusa", "accesa", "spenta" };
}

const std::vector<std::string>& EditorDisegno::EstensioniImmagini() {
    static const std::vector<std::string> e = { ".png", ".bmp", ".jpg", ".jpeg", ".gif", ".tga", ".qoi" };
    return e;
}

std::vector<std::string> EditorDisegno::DestinazioniImport() const {
    std::vector<std::string> v;
    v.push_back("Quella aperta: " + (d && anim < (int)d->animazioni.size() ? d->animazioni[anim].nome : std::string("?")));
    for (auto n : AZIONI_MOTORE) v.push_back(n);
    v.push_back("Nuova animazione (nome sotto)");
    return v;
}

void EditorDisegno::ChiudiImport() {
    for (auto& f : impFile) { if (f.tex.id) UnloadTexture(f.tex); if (f.img.data) UnloadImage(f.img); }
    impFile.clear();
}

void EditorDisegno::PreparaImport(const std::vector<std::string>& file) {
    if (!d) return;
    ChiudiImport();
    std::vector<std::string> ordinati = file;
    std::sort(ordinati.begin(), ordinati.end());   // corsa_01.png, corsa_02.png... nell'ordine giusto
    for (auto& p : ordinati) {
        if (impFile.size() >= 64) break;
        FileImport f;
        f.percorso = p;
        std::string e = p.size() > 4 ? p.substr(p.size() - 4) : p;
        for (auto& c : e) c = (char)std::tolower((unsigned char)c);
        if (e == ".gif") {
            int n = 0;
            f.img = LoadImageAnim(p.c_str(), &n);
            // I fotogrammi della GIF stanno uno dopo l'altro: e' come un foglio alto n volte.
            if (f.img.data && n > 1) { f.img.height *= n; f.gif = n; }
        } else f.img = LoadImage(p.c_str());
        if (!f.img.data) continue;
        ImageFormat(&f.img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        f.tex = LoadTextureFromImage(f.img);
        impFile.push_back(f);
    }
    if (impFile.empty()) return;
    // Divisione automatica del primo file.
    const Image& im = impFile[0].img;
    impRighe = impColonne = 1;
    // Spazi vuoti fra i disegni (trasparenti o del colore dello sfondo): contano colonne e righe del foglio.
    auto Gruppi = [&](bool perColonne) {
        Color c0 = GetImageColor(im, 0, 0);
        auto Vuoto = [&](Color c) { return c.a < 10 || (std::abs(c.r - c0.r) + std::abs(c.g - c0.g) + std::abs(c.b - c0.b) <= 24 && c.a == c0.a); };
        int n = perColonne ? im.width : im.height, m = perColonne ? im.height : im.width, gruppi = 0;
        bool dentro = false;
        const Color* px = (const Color*)im.data;
        for (int i = 0; i < n; i++) {
            bool pieno = false;
            for (int j = 0; j < m && !pieno; j++) pieno = !Vuoto(perColonne ? px[(size_t)j * im.width + i] : px[(size_t)i * im.width + j]);
            if (pieno && !dentro) gruppi++;
            dentro = pieno;
        }
        return gruppi;
    };
    int gc = Gruppi(true), gr = Gruppi(false);
    if (impFile[0].gif > 1) impRighe = impFile[0].gif;
    else if ((gc > 1 || gr > 1) && im.width % std::max(1, gc) == 0 && im.height % std::max(1, gr) == 0) { impColonne = std::max(1, gc); impRighe = std::max(1, gr); }
    else if (im.width >= im.height * 2 && im.width % im.height == 0) impColonne = im.width / im.height;
    else if (im.height >= im.width * 2 && im.height % im.width == 0) impRighe = im.height / im.width;
    else if (im.width % d->w == 0 && im.height % d->h == 0 && (im.width > d->w || im.height > d->h)) { impColonne = im.width / d->w; impRighe = im.height / d->h; }
    impColonne = std::min(impColonne, 64); impRighe = std::min(impRighe, 64);
    // BMP e JPG non hanno trasparenza: si propone di togliere il colore dello sfondo.
    Color c0 = GetImageColor(im, 0, 0);
    impTrasparente = c0.a == 255;
    impDest = 0;
    impFps = d->animazioni.empty() ? 8 : d->animazioni[std::min(anim, (int)d->animazioni.size() - 1)].fps;
    tImport = 0;
}

void EditorDisegno::CelleImport(const FileImport& f, std::vector<Rectangle>& celle) const {
    int r = f.gif > 1 ? f.gif : impRighe, c = f.gif > 1 ? 1 : impColonne;
    float cw = (float)f.img.width / c, ch = (float)f.img.height / r;
    for (int y = 0; y < r; y++)
        for (int x = 0; x < c; x++) celle.push_back({ (float)(int)(x * cw), (float)(int)(y * ch), (float)(int)cw, (float)(int)ch });
}

bool EditorDisegno::FinestraImport() {
    if (impFile.empty() || !d) return false;
    float s = Ui::S();
    tImport += GetFrameTime();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)W, (int)H, Hex(0x000000, 150));
    Ui::ApriModale();
    float fw = std::min(W - 40 * s, 1100 * s), fh = std::min(H - 40 * s, 700 * s);
    Rectangle f = { (W - fw) / 2, (H - fh) / 2, fw, fh };
    DrawRectangleRounded(f, 0.02f, 8, Ui::Pannello());
    DrawRectangleRoundedLinesEx(f, 0.02f, 8, 2, Ui::Accento());
    float pad = 20 * s, x = f.x + pad, y = f.y + pad, h = 30 * s;
    Ui::Testo("Gestore delle immagini", x, y, Ui::Accento(), (int)(24 * s));
    bool chiudi = (IsKeyPressed(KEY_ESCAPE) && !Ui::StaScrivendo()) || Ui::Pulsante({ f.x + f.width - 44 * s, f.y + 10 * s, 34 * s, 30 * s }, "X");
    y += 40 * s;
    // Colonna sinistra: il primo file con la griglia, e l'anteprima animata del risultato.
    float wSx = fw * 0.55f;
    Rectangle vista = { x, y, wSx - pad, fh - (y - f.y) - 150 * s };
    Scacchiera(vista, 10 * s);
    const FileImport& p0 = impFile[0];
    float k = std::min(vista.width / p0.img.width, vista.height / p0.img.height);
    Rectangle dst = { vista.x + (vista.width - p0.img.width * k) / 2, vista.y + (vista.height - p0.img.height * k) / 2, p0.img.width * k, p0.img.height * k };
    // una GIF lunga si mostra solo nel primo fotogramma, con la griglia sopra
    if (p0.gif > 1) {
        float hh = (float)p0.img.height / p0.gif;
        float k2 = std::min(vista.width / p0.img.width, vista.height / hh);
        dst = { vista.x + (vista.width - p0.img.width * k2) / 2, vista.y + (vista.height - hh * k2) / 2, p0.img.width * k2, hh * k2 };
        DrawTexturePro(p0.tex, { 0, 0, (float)p0.img.width, hh }, dst, { 0, 0 }, 0, WHITE);
    } else {
        DrawTexturePro(p0.tex, { 0, 0, (float)p0.img.width, (float)p0.img.height }, dst, { 0, 0 }, 0, WHITE);
        for (int i = 1; i < impColonne; i++) { float xx = dst.x + dst.width * i / impColonne; DrawLineEx({ xx, dst.y }, { xx, dst.y + dst.height }, 1, Hex(0xffd23f, 200)); }
        for (int i = 1; i < impRighe; i++) { float yy = dst.y + dst.height * i / impRighe; DrawLineEx({ dst.x, yy }, { dst.x + dst.width, yy }, 1, Hex(0xffd23f, 200)); }
        DrawRectangleLinesEx(dst, 1, Hex(0xffd23f, 200));
    }
    DrawRectangleLinesEx(vista, 1, Ui::Bordo());
    // tutte le celle di tutti i file, in ordine
    struct Cella { int file; Rectangle r; };
    std::vector<Cella> celle;
    for (int i = 0; i < (int)impFile.size(); i++) { std::vector<Rectangle> c; CelleImport(impFile[i], c); for (auto& r : c) celle.push_back({ i, r }); }
    float ya = vista.y + vista.height + 10 * s;
    Rectangle anteprima = { x, ya, 120 * s, 120 * s };
    Scacchiera(anteprima, 8 * s);
    if (!celle.empty()) {
        int n = (int)(tImport * std::max(1.0f, impFps)) % (int)celle.size();
        const Cella& c = celle[n];
        float kk = std::min(anteprima.width / c.r.width, anteprima.height / c.r.height);
        Rectangle q = { anteprima.x + (anteprima.width - c.r.width * kk) / 2, anteprima.y + (anteprima.height - c.r.height * kk) / 2, c.r.width * kk, c.r.height * kk };
        DrawTexturePro(impFile[c.file].tex, c.r, q, { 0, 0 }, 0, WHITE);
    }
    DrawRectangleLinesEx(anteprima, 1, Ui::Bordo());
    std::string info = TextFormat("%d file, %d fotogrammi da %d x %d pixel.", (int)impFile.size(), (int)celle.size(),
                                  celle.empty() ? 0 : (int)celle[0].r.width, celle.empty() ? 0 : (int)celle[0].r.height);
    Ui::TestoACapo(info + "\nIl disegno e' di " + std::to_string(d->w) + " x " + std::to_string(d->h) + " pixel.\n" +
                   GetFileName(p0.percorso.c_str()) + (impFile.size() > 1 ? " e altri" : ""),
                   anteprima.x + anteprima.width + 12 * s, ya, wSx - pad - anteprima.width - 12 * s, Ui::TestoSpento(), Ui::TP());
    // Colonna destra: le scelte.
    float cx = f.x + wSx + 10 * s, cw = fw - wSx - 10 * s - pad;
    float cy = f.y + pad + 40 * s;
    if (p0.gif > 1) { Ui::TestoACapo(TextFormat("GIF animata: %d fotogrammi.", p0.gif), cx, cy + 6 * s, cw, Ui::TestoChiaro(), Ui::TP()); cy += h + 6 * s; }
    else {
        Ui::NumeroIntero("imp_col", { cx, cy, cw, h }, "Colonne (foglio di sprite)", impColonne, 1, 64); cy += h + 6 * s;
        Ui::NumeroIntero("imp_rig", { cx, cy, cw, h }, "Righe", impRighe, 1, 64); cy += h + 6 * s;
    }
    std::vector<std::string> dest = DestinazioniImport();
    Ui::Scelta({ cx, cy, cw, h }, "Animazione", impDest, dest); cy += h + 6 * s;
    if (impDest == (int)dest.size() - 1) { Ui::Campo("imp_nome", { cx, cy, cw, h }, impNome, 30); cy += h + 6 * s; }
    Ui::Scelta({ cx, cy, cw, h }, "Fotogrammi", impModo, { "Aggiungi in coda", "Sostituisci quelli che ci sono" }); cy += h + 6 * s;
    Ui::Scelta({ cx, cy, cw, h }, "Misura", impAdatta, { "Allarga il disegno (fino a 256)", "Rimpicciolisci nel disegno", "Disegno della misura delle immagini" }); cy += h + 6 * s;
    Ui::Scelta({ cx, cy, cw, h }, "Allinea", impAllinea, { "Piedi in basso, al centro", "Al centro", "In alto a sinistra" }); cy += h + 6 * s;
    Ui::Numero("imp_fps", { cx, cy, cw, h }, "Velocita' (fps)", impFps, 1, 30, 1); cy += h + 6 * s;
    Ui::Interruttore({ cx, cy, cw, h }, "Sfondo trasparente (colore dell'angolo)", impTrasparente); cy += h + 6 * s;
    Ui::Interruttore({ cx, cy, cw, h }, "Salta le celle vuote", impSaltaVuoti); cy += h + 10 * s;
    Ui::TestoACapo("Nomi che il motore usa da solo: fermo, corsa, salto, caduta, colpito, attacco, sparo, vittoria "
                   "(porte: aperta/chiusa; interruttori: accesa/spenta). Le altre si avviano dalla Logica con \"anima\".",
                   cx, cy, cw, Ui::TestoSpento(), (int)(13 * s));
    float by = f.y + fh - pad - 36 * s;
    bool importa = false;
    if (Ui::Pulsante({ cx, by, cw * 0.48f, 36 * s }, "Aggiungi altri file...")) {
        std::vector<std::string> altri = ScegliFile("Scegli le immagini", "Immagini", EstensioniImmagini(), true);
        if (!altri.empty()) {
            std::vector<std::string> tutti;
            for (auto& fi : impFile) tutti.push_back(fi.percorso);
            tutti.insert(tutti.end(), altri.begin(), altri.end());
            int r = impRighe, c = impColonne, dd = impDest; float fp = impFps;
            PreparaImport(tutti);
            impRighe = r; impColonne = c; impDest = dd; impFps = fp;
            Ui::ChiudiModale();
            return false;
        }
    }
    if (Ui::Pulsante({ cx + cw * 0.52f, by, cw * 0.48f, 36 * s }, "Importa", true)) importa = true;
    Ui::ChiudiModale();
    if (chiudi) { ChiudiImport(); Ui::TogliFuoco(); }
    return importa;
}

bool EditorDisegno::EseguiImport(std::string& messaggio) {
    if (impFile.empty() || !d) return false;
    Ricorda();
    // Le celle diventano fotogrammi (prima di toccare il disegno).
    std::vector<Fotogramma> nuovi;
    int maxW = 0, maxH = 0;
    for (auto& fi : impFile) {
        std::vector<Rectangle> celle;
        CelleImport(fi, celle);
        Color sfondo = GetImageColor(fi.img, 0, 0);
        for (auto& r : celle) {
            Fotogramma fo = FotogrammaDaImmagine(fi.img, r);
            if (impTrasparente)
                for (auto& c : fo.px)
                    if (std::abs(c.r - sfondo.r) + std::abs(c.g - sfondo.g) + std::abs(c.b - sfondo.b) <= 24) c = BLANK;
            if (impSaltaVuoti && fo.Vuoto()) continue;
            maxW = std::max(maxW, fo.w); maxH = std::max(maxH, fo.h);
            nuovi.push_back(fo);
        }
    }
    if (nuovi.empty()) { messaggio = "Nessun fotogramma da importare (celle vuote?)"; annulla.pop_back(); return false; }
    // Misura del disegno
    if (impAdatta == 0) {
        int nw = std::min(256, std::max(d->w, maxW)), nh = std::min(256, std::max(d->h, maxH));
        if (nw != d->w || nh != d->h) d->Ridimensiona(nw, nh);
    } else if (impAdatta == 2) {
        int nw = std::min(256, maxW), nh = std::min(256, maxH);
        if (nw != d->w || nh != d->h) d->Ridimensiona(nw, nh);
    }
    // Ogni fotogramma entra nel disegno (se e' troppo grande si rimpicciolisce mantenendo le proporzioni).
    for (auto& fo : nuovi) {
        if (fo.w > d->w || fo.h > d->h) {
            float k = std::min((float)d->w / fo.w, (float)d->h / fo.h);
            int w2 = std::max(1, (int)(fo.w * k)), h2 = std::max(1, (int)(fo.h * k));
            Image im = GenImageColor(fo.w, fo.h, BLANK);
            std::memcpy(im.data, fo.px.data(), fo.px.size() * sizeof(Color));
            if (d->liscio) ImageResize(&im, w2, h2); else ImageResizeNN(&im, w2, h2);
            fo = FotogrammaDaImmagine(im, { 0, 0, (float)w2, (float)h2 });
            UnloadImage(im);
        }
        Fotogramma g;
        g.Nuovo(d->w, d->h);
        int ox = impAllinea == 2 ? 0 : (d->w - fo.w) / 2;
        int oy = impAllinea == 0 ? d->h - fo.h : impAllinea == 1 ? (d->h - fo.h) / 2 : 0;
        for (int yy = 0; yy < fo.h; yy++) for (int xx = 0; xx < fo.w; xx++) g.Metti(xx + ox, yy + oy, fo.Prendi(xx, yy));
        fo = g;
    }
    // L'animazione di destinazione
    std::vector<std::string> dest = DestinazioniImport();
    int a = anim;
    if (impDest > 0) {
        std::string nome = impDest == (int)dest.size() - 1 ? (impNome.empty() ? std::string("animazione") : impNome) : dest[impDest];
        a = d->TrovaAnimazione(nome);
        if (a < 0) {
            ::Animazione na; na.nome = nome;
            d->animazioni.push_back(na);
            a = (int)d->animazioni.size() - 1;
        }
    }
    auto& an = d->animazioni[a];
    bool vuota = an.fotogrammi.empty() || (an.fotogrammi.size() == 1 && an.fotogrammi[0].Vuoto());
    if (impModo == 1 || vuota) an.fotogrammi.clear();
    an.fotogrammi.insert(an.fotogrammi.end(), nuovi.begin(), nuovi.end());
    an.fps = impFps;
    anim = a;
    fot = (int)an.fotogrammi.size() - (int)nuovi.size();
    d->Invalida();
    RaccogliTavolozza();
    adattaVista = true;
    cambiato = true;
    messaggio = TextFormat("%d fotogrammi nell'animazione \"%s\"", (int)nuovi.size(), an.nome.c_str());
    ChiudiImport();
    return true;
}
