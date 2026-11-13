#include "editor_disegno.h"
#include "audio.h"
#include "ui.h"
#include <algorithm>
#include <cstring>

namespace {

const char* NOMI_ATTREZZI[] = { "Matita", "Gomma", "Secchio", "Linea", "Rettang.", "Ellisse", "Contagocce", "Sposta" };
const char* TASTI_ATTREZZI[] = { "P", "E", "F", "L", "R", "O", "I", "M" };
const int KEY_ATTREZZI[] = { KEY_P, KEY_E, KEY_F, KEY_L, KEY_R, KEY_O, KEY_I, KEY_M };

bool Uguali(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

void Scacchiera(Rectangle r, float lato) {
    DrawRectangleRec(r, Hex(0x3a3d58));
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    int nx = (int)(r.width / lato) + 1, ny = (int)(r.height / lato) + 1;
    for (int y = 0; y < ny; y++)
        for (int x = (y & 1); x < nx; x += 2) DrawRectangleRec({ r.x + x * lato, r.y + y * lato, lato, lato }, Hex(0x4a4e6c));
    EndScissorMode();
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
    float hTesta = 40 * s, hBasso = 104 * s, wSx = 86 * s, wDx = 262 * s;
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
    float x = r.x + 10 * s, y = r.y + 6 * s, h = r.height - 12 * s;
    Ui::Testo(TextFormat("%d x %d pixel  -  %s", d->w, d->h, d->liscio ? "HD" : "pixel art"), x, r.y + (r.height - Ui::TP()) / 2, Ui::TestoSpento(), Ui::TP());
    x += 230 * s;
    Ui::NumeroIntero("ed_w", { x, y, 150 * s, h }, "Larg.", nuovaW, 4, 256);
    x += 156 * s;
    Ui::NumeroIntero("ed_h", { x, y, 150 * s, h }, "Alt.", nuovaH, 4, 256);
    x += 156 * s;
    if (Ui::Pulsante({ x, y, 104 * s, h }, "Ridimensiona", false, nuovaW != d->w || nuovaH != d->h)) {
        Ricorda();
        d->Ridimensiona(nuovaW, nuovaH);
        adattaVista = true;
        cambiato = true;
    }
    x += 112 * s;
    if (Ui::Pulsante({ x, y, 96 * s, h }, "Specchia")) {
        Ricorda();
        Fotogramma& f = F();
        for (int yy = 0; yy < f.h; yy++) for (int xx = 0; xx < f.w / 2; xx++) { Color a = f.Prendi(xx, yy); f.Metti(xx, yy, f.Prendi(f.w - 1 - xx, yy)); f.Metti(f.w - 1 - xx, yy, a); }
        cambiato = true;
    }
    x += 102 * s;
    if (Ui::Pulsante({ x, y, 80 * s, h }, "Pulisci")) { Ricorda(); F().Nuovo(d->w, d->h); cambiato = true; }
    x += 86 * s;
    if (Ui::Pulsante({ x, y, 80 * s, h }, "Annulla", false, !annulla.empty())) Annulla();
    x += 86 * s;
    if (Ui::Pulsante({ x, y, 80 * s, h }, "Ripeti", false, !ripeti.empty())) Ripeti();
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
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
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
    EndScissorMode();
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
    if (Ui::Numero("ed_fps", { bx, y + 34 * s, bw, 26 * s }, "fps", fps, 1, 30, 1)) { anims[anim].fps = fps; cambiato = true; }
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
    float altezzaElenco = r.y + r.height - y - h - 10 * s;
    Rectangle elenco = { r.x, y, r.width, std::max(h, altezzaElenco) };
    BeginScissorMode((int)elenco.x, (int)elenco.y, (int)elenco.width, (int)elenco.height);
    if (Ui::MouseSu(elenco)) scorriAnimazioni = std::max(0.0f, scorriAnimazioni - GetMouseWheelMove() * 40 * s);
    for (int i = 0; i < (int)anims.size(); i++) {
        Rectangle b = { r.x, y + i * (h + 3 * s) - scorriAnimazioni, r.width, h };
        if (b.y + b.height < elenco.y || b.y > elenco.y + elenco.height) continue;
        std::string t = anims[i].nome + TextFormat("  (%d)", (int)anims[i].fotogrammi.size());
        if (Ui::Pulsante(b, t, i == anim) && CheckCollisionPointRec(GetMousePosition(), elenco)) { anim = i; fot = 0; tAnteprima = 0; }
    }
    EndScissorMode();
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
    float pad = 8 * s, lato = r.height - 2 * pad - 18 * s;
    float wPulsanti = 120 * s;
    Rectangle striscia = { r.x + pad, r.y + pad, r.width - wPulsanti - 3 * pad, r.height - 2 * pad };
    Ui::Testo(TextFormat("Fotogrammi di \"%s\"   (, e . per sfogliarli)", a.nome.c_str()), striscia.x, r.y + pad * 0.5f, Ui::TestoSpento(), (int)(12 * s));
    float y = r.y + pad + 16 * s;
    if (Ui::MouseSu(striscia)) scorriFotogrammi = std::max(0.0f, scorriFotogrammi - GetMouseWheelMove() * 60 * s);
    BeginScissorMode((int)striscia.x, (int)striscia.y, (int)striscia.width, (int)striscia.height);
    for (int i = 0; i < (int)a.fotogrammi.size(); i++) {
        Rectangle b = { striscia.x + i * (lato + pad) - scorriFotogrammi, y, lato, lato };
        if (b.x > striscia.x + striscia.width || b.x + b.width < striscia.x) continue;
        Scacchiera(b, 6 * s);
        d->DisegnaIn({ b.x + 2, b.y + 2, b.width - 4, b.height - 4 }, anim, 0 + i / std::max(1.0f, a.fotogrammi.size() > 0 ? a.fps : 1));
        bool sel = i == fot;
        DrawRectangleLinesEx(b, sel ? 3.0f : 1.0f, sel ? Ui::Accento() : Ui::MouseSu(b) ? Hex(0x5a64b0) : Ui::Bordo());
        Ui::Testo(std::to_string(i + 1), b.x + 4 * s, b.y + 2 * s, Ui::TestoSpento(), (int)(11 * s));
        if (Ui::Cliccato(b) && CheckCollisionPointRec(GetMousePosition(), striscia)) fot = i;
    }
    EndScissorMode();
    float bx = r.x + r.width - wPulsanti - pad, bh = (r.height - 2 * pad - 9 * s) / 4;
    float bw = (wPulsanti - 4 * s) / 2;
    if (Ui::Pulsante({ bx, r.y + pad, wPulsanti, bh }, "+ Vuoto")) { Ricorda(); Fotogramma f; f.Nuovo(d->w, d->h); a.fotogrammi.insert(a.fotogrammi.begin() + fot + 1, f); fot++; cambiato = true; }
    if (Ui::Pulsante({ bx, r.y + pad + (bh + 3 * s), wPulsanti, bh }, "+ Copia")) { Ricorda(); Fotogramma f = F(); a.fotogrammi.insert(a.fotogrammi.begin() + fot + 1, f); fot++; cambiato = true; }
    if (Ui::Pulsante({ bx, r.y + pad + 2 * (bh + 3 * s), bw, bh }, "<", false, fot > 0)) { Ricorda(); std::swap(a.fotogrammi[fot], a.fotogrammi[fot - 1]); fot--; cambiato = true; }
    if (Ui::Pulsante({ bx + bw + 4 * s, r.y + pad + 2 * (bh + 3 * s), bw, bh }, ">", false, fot + 1 < (int)a.fotogrammi.size())) { Ricorda(); std::swap(a.fotogrammi[fot], a.fotogrammi[fot + 1]); fot++; cambiato = true; }
    if (Ui::Pulsante({ bx, r.y + pad + 3 * (bh + 3 * s), wPulsanti, bh }, "Elimina", false, a.fotogrammi.size() > 1)) { Ricorda(); a.fotogrammi.erase(a.fotogrammi.begin() + fot); fot = std::max(0, fot - 1); cambiato = true; }
}
