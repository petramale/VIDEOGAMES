// Mondi d'esempio della fase 1: ognuno mostra una parte del nuovo motore
// (import del vecchio formato, scorrimento verticale, mappe grandi viste dall'alto
// collegate da porte, telecamera a stanze, cielo e strati personalizzati).
#include "mondo.h"
#include "livello.h"

namespace {

void Oggetto(Mappa& m, const char* tipo, int tx, int ty, bool appoggiato, int numero = 0) {
    OggettoMappa o;
    o.tipo = tipo;
    o.x = tx * TILE + TILE / 2.0f;
    o.y = appoggiato ? (ty + 1) * (float)TILE : ty * TILE + TILE / 2.0f;
    o.numero = numero;
    m.oggetti.push_back(o);
}

void Rettangolo(Mappa& m, int x0, int y0, int x1, int y1, char c) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) m.ImpostaCasella(x, y, c);
}

void Testo(Mappa& m, int n, const char* it, const char* en) {
    TestoCartello t; t.numero = n; t.it = it; t.en = en;
    m.testi.push_back(t);
}

// --- Torre del cielo: platform verticale, cielo e strati personalizzati --------
Mondo TorreDelCielo() {
    Mondo w;
    w.titolo = "La torre del cielo";
    w.genere = Genere::Platform;
    w.fisica.salto = 330;
    w.eroe.maglia = Hex(0xe0a030); w.eroe.mantello = Hex(0x5a3ad8);
    Mappa m;
    m.nome = "Torre";
    m.Nuova(36, 70, Vista::Laterale);
    m.musica = 5;
    m.sfondoGenerato = false;
    m.cicloGiorno = false;
    m.ora = 0.5f;
    m.cieloPersonale = true;
    m.cieloAlto = Hex(0x1a2a6c); m.cieloBasso = Hex(0xf7b267);
    StratoSfondo s;
    s.tipo = "stelle"; s.colore = Hex(0xfff4d8); s.altezza = 0.45f; s.parallasseX = 0.02f; s.parallasseY = 0.05f; s.nome = "Stelle";
    m.strati.push_back(s);
    s = StratoSfondo{}; s.nome = "Montagne lontane"; s.tipo = "montagne"; s.colore = Hex(0x7a5a9a); s.altezza = 0.78f;
    s.ampiezza = 70; s.parallasseX = 0.06f; s.parallasseY = 0.12f; s.seme = 2;
    m.strati.push_back(s);
    s = StratoSfondo{}; s.nome = "Nuvole"; s.tipo = "nuvole"; s.colore = Hex(0xffd6c8); s.altezza = 0.32f;
    s.ampiezza = 60; s.parallasseX = 0.15f; s.parallasseY = 0.35f; s.vento = 6; s.seme = 4;
    m.strati.push_back(s);
    s = StratoSfondo{}; s.nome = "Colline"; s.tipo = "colline"; s.colore = Hex(0x3d2c5e); s.altezza = 0.92f;
    s.ampiezza = 40; s.parallasseX = 0.25f; s.parallasseY = 0.45f; s.seme = 7;
    m.strati.push_back(s);
    m.telecamera.modo = ModoTelecamera::Segui;
    m.telecamera.zonaMortaX = 40; m.telecamera.zonaMortaY = 30; m.telecamera.anticipo = 20;
    // Piattaforme a zig-zag fino in cima: ogni piattaforma e' 3 caselle sopra la precedente
    // e si sovrappone di una casella in orizzontale, quindi si raggiunge sempre con un salto.
    const int posizioni[8] = { 3, 9, 15, 21, 26, 20, 14, 8 };
    int riga = m.altezza - 3, k = 0;
    while (riga - 3 >= 6) {
        riga -= 3;
        int x = posizioni[k % 8];
        Rettangolo(m, x, riga, x + 6, riga, '-');
        Oggetto(m, "moneta", x + 2, riga - 2, false);
        Oggetto(m, "moneta", x + 4, riga - 2, false);
        if (k % 4 == 3) Oggetto(m, "gemma", x + 3, riga - 3, false);
        if (k % 6 == 5) Oggetto(m, "lanterna", x + 5, riga - 1, true);
        if (k % 5 == 2) Oggetto(m, "pipistrello", x + 3, riga - 4, false);
        k++;
    }
    // In cima: una terrazza di pietra con la bandiera.
    int xTop = posizioni[(k - 1) % 8];
    Rettangolo(m, xTop, riga - 3, xTop + 6, riga - 3, '-');
    Oggetto(m, "bandiera", xTop + 4, riga - 4, true);
    Oggetto(m, "cuore", xTop + 1, riga - 5, false);
    Oggetto(m, "molla", 30, m.altezza - 4, true);
    Oggetto(m, "cartello", 6, m.altezza - 4, true, 1);
    Testo(m, 1, "Sali fino in cima alla torre! La telecamera ti segue in alto e in basso.",
          "Climb to the top of the tower! The camera follows you up and down.");
    m.partenza = { 3 * TILE + TILE / 2.0f, (m.altezza - 3) * (float)TILE };
    w.mappe.push_back(m);
    return w;
}

// Ostacoli a blocchi su una griglia: i corridoi fra i blocchi restano sempre larghi
// almeno 4 caselle, quindi tutta la mappa e' raggiungibile.
void Boschetti(Mappa& m, int seme, char a, char b) {
    for (int gy = 6; gy < m.altezza - 6; gy += 8)
        for (int gx = 6; gx < m.larghezza - 6; gx += 8) {
            float h = Hash1(gx * 31 + gy * 17 + seme);
            if (h < 0.3f) continue;
            int w = 2 + (int)(Hash1(gx + gy * 3 + seme) * 3), hh = 2 + (int)(Hash1(gx * 5 + gy + seme) * 3);
            Rettangolo(m, gx, gy, gx + w - 1, gy + hh - 1, h > 0.65f ? b : a);
        }
}

// Cerca una casella libera vicino a (tx, ty).
void Libera(const Mappa& m, int& tx, int& ty) {
    for (int r = 0; r < 6; r++)
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++)
                if (m.Casella(tx + dx, ty + dy) == ' ') { tx += dx; ty += dy; return; }
}

void OggettoAlto(Mappa& m, const char* tipo, int tx, int ty, int numero = 0) {
    Libera(m, tx, ty);
    OggettoMappa o;
    o.tipo = tipo;
    o.x = tx * TILE + TILE / 2.0f;
    // Vista dall'alto: "appoggiato" vuol dire alla base della casella.
    std::string t = tipo;
    bool base = t == "porta" || t == "forziere" || t == "lanterna" || t == "cartello" || t == "bandiera" || t == "slime" || t == "torretta" || t == "molla";
    o.y = base ? (ty + 1) * (float)TILE : ty * TILE + TILE / 2.0f;
    o.numero = numero;
    m.oggetti.push_back(o);
}

// --- Isola aperta: open world dall'alto con due mappe collegate -------------------
Mondo IsolaAperta() {
    Mondo w;
    w.titolo = "L'isola aperta";
    w.genere = Genere::OpenWorld;
    w.eroe.maglia = Hex(0x3ac8a0); w.eroe.mantello = Hex(0xe8a030);
    Mappa isola;
    isola.nome = "Isola";
    isola.Nuova(80, 56, Vista::Alto);
    isola.tema = 0;
    isola.musica = 7;
    isola.telecamera.zonaMortaX = 30; isola.telecamera.zonaMortaY = 20; isola.telecamera.anticipo = 0;
    Boschetti(isola, 3, '#', '=');
    // Un piccolo campo di spine al centro, da aggirare.
    Rettangolo(isola, 38, 27, 41, 28, '^');
    for (int i = 0; i < 40; i++) {
        int tx = 3 + (int)(Hash1(i * 13 + 1) * 74), ty = 3 + (int)(Hash1(i * 7 + 2) * 50);
        OggettoAlto(isola, "moneta", tx, ty);
    }
    const int gemme[6][2] = { { 74, 50 }, { 10, 48 }, { 40, 6 }, { 60, 30 }, { 22, 22 }, { 52, 46 } };
    for (auto& g : gemme) OggettoAlto(isola, "gemma", g[0], g[1]);
    OggettoAlto(isola, "fantasma", 46, 20);
    OggettoAlto(isola, "fantasma", 20, 40);
    OggettoAlto(isola, "fantasma", 66, 44);
    OggettoAlto(isola, "slime", 30, 14);
    OggettoAlto(isola, "slime", 58, 12);
    OggettoAlto(isola, "cuore", 36, 36);
    OggettoAlto(isola, "cuore", 70, 20);
    OggettoAlto(isola, "forziere", 12, 30);
    OggettoAlto(isola, "lanterna", 44, 34);
    OggettoAlto(isola, "cartello", 5, 3, 1);
    Testo(isola, 1, "Benvenuto sull'isola! Esplora in tutte le direzioni e trova la grotta a nord-est.",
          "Welcome to the island! Explore in every direction and find the cave to the north-east.");
    // Porta verso la grotta.
    Rettangolo(isola, 72, 2, 77, 4, ' ');
    OggettoMappa porta;
    porta.tipo = "porta"; porta.x = 75 * TILE + TILE / 2.0f; porta.y = 3 * (float)TILE;
    porta.mappa = 1; porta.destX = 4 * TILE + TILE / 2.0f; porta.destY = 5 * TILE + TILE - 2.0f;
    isola.oggetti.push_back(porta);
    isola.partenza = { 3 * TILE + TILE / 2.0f, 3 * TILE + TILE - 2.0f };

    Mappa grotta;
    grotta.nome = "Grotta";
    grotta.Nuova(44, 30, Vista::Alto);
    for (int y = 0; y < grotta.altezza; y++) for (int x = 0; x < grotta.larghezza; x++) if (grotta.Casella(x, y) == '#') grotta.ImpostaCasella(x, y, '=');
    grotta.tema = 3;
    grotta.musica = 1;
    grotta.telecamera.modo = ModoTelecamera::Segui;
    grotta.telecamera.anticipo = 0; grotta.telecamera.zoom = 1.25f;
    Boschetti(grotta, 9, '=', '=');
    Rettangolo(grotta, 2, 2, 7, 7, ' ');
    OggettoMappa ritorno;
    ritorno.tipo = "porta"; ritorno.x = 4 * TILE + TILE / 2.0f; ritorno.y = 3 * (float)TILE;
    ritorno.mappa = 0; ritorno.destX = 75 * TILE + TILE / 2.0f; ritorno.destY = 5 * TILE + TILE - 2.0f;
    grotta.oggetti.push_back(ritorno);
    for (int i = 0; i < 14; i++) OggettoAlto(grotta, "moneta", 3 + (int)(Hash1(i * 5 + 9) * 38), 3 + (int)(Hash1(i * 11 + 4) * 24));
    OggettoAlto(grotta, "gemma", 38, 26);
    OggettoAlto(grotta, "gemma", 20, 16);
    OggettoAlto(grotta, "pipistrello", 24, 8);
    OggettoAlto(grotta, "pipistrello", 30, 22);
    OggettoAlto(grotta, "fantasma", 34, 12);
    OggettoAlto(grotta, "forziere", 12, 24);
    Rettangolo(grotta, 38, 2, 41, 6, ' ');
    OggettoAlto(grotta, "bandiera", 40, 4);
    grotta.partenza = { 4 * TILE + TILE / 2.0f, 5 * TILE + TILE - 2.0f };
    w.mappe.push_back(isola);
    w.mappe.push_back(grotta);
    return w;
}

// --- Castello a stanze: avventura laterale con la telecamera a stanze ----------------
Mondo CastelloAStanze() {
    Mondo w;
    w.titolo = "Il castello a stanze";
    w.genere = Genere::Avventura;
    w.fisica.salto = 330;
    w.eroe.maglia = Hex(0xb8c0d8); w.eroe.mantello = Hex(0x2a5bd8); w.eroe.capelli = Hex(0xe8c050);
    Mappa m;
    m.nome = "Castello";
    m.larghezza = 90; m.altezza = 34;
    m.caselle.assign(m.altezza, std::string(m.larghezza, ' '));
    m.vista = Vista::Laterale;
    m.tema = 3; m.ora = 0.92f; m.cicloGiorno = false; m.musica = 6;
    m.telecamera.modo = ModoTelecamera::Stanze;
    // Pavimento in basso, piano superiore con un'apertura nella stanza di destra.
    Rettangolo(m, 0, 32, 89, 33, '=');
    Rettangolo(m, 0, 16, 89, 16, '=');
    Rettangolo(m, 81, 16, 86, 16, '-');
    // Muri fra le stanze, con un passaggio in basso.
    for (int x : { 30, 60 }) { Rettangolo(m, x, 17, x, 28, '='); Rettangolo(m, x, 0, x, 12, '='); }
    // Stanza 2 in basso: spine con una passerella sopra.
    Rettangolo(m, 40, 31, 46, 31, '^');
    Rettangolo(m, 39, 29, 47, 29, '-');
    // Stanza 3 in basso: la scalata verso il piano di sopra.
    Rettangolo(m, 64, 29, 70, 29, '-');
    Rettangolo(m, 71, 26, 77, 26, '-');
    Rettangolo(m, 78, 23, 84, 23, '-');
    Rettangolo(m, 72, 20, 78, 20, '-');
    Rettangolo(m, 79, 18, 85, 18, '-');
    // Piano di sopra: qualche piattaforma e le spine.
    Rettangolo(m, 50, 15, 54, 15, '^');
    Rettangolo(m, 49, 12, 55, 12, '-');
    Rettangolo(m, 14, 13, 18, 13, '-');
    // Oggetti.
    Oggetto(m, "cartello", 4, 31, true, 1);
    Testo(m, 1, "Le stanze scorrono una alla volta. Vai a destra e sali dalla torre!",
          "Rooms scroll one at a time. Go right and climb the tower!");
    for (int x = 8; x < 28; x += 3) Oggetto(m, "moneta", x, 29, false);
    Oggetto(m, "slime", 20, 31, true);
    for (int x = 40; x <= 46; x += 2) Oggetto(m, "moneta", x, 27, false);
    Oggetto(m, "gemma", 52, 30, false);
    Oggetto(m, "lanterna", 62, 31, true);
    Oggetto(m, "forziere", 88, 31, true);
    Oggetto(m, "pipistrello", 75, 22, false);
    Oggetto(m, "gemma", 82, 14, false);
    Oggetto(m, "fantasma", 45, 8, false);
    for (int x = 50; x <= 54; x += 2) Oggetto(m, "moneta", x, 10, false);
    Oggetto(m, "cuore", 36, 14, false);
    Oggetto(m, "slime", 24, 15, true);
    Oggetto(m, "gemma", 16, 11, false);
    Oggetto(m, "bandiera", 4, 15, true);
    m.partenza = { 2 * TILE + TILE / 2.0f, 32 * (float)TILE };
    w.mappe.push_back(m);
    return w;
}

} // namespace

std::vector<Mondo> MondiDiProva() {
    std::vector<Mondo> mondi;
    Mondo valle;
    std::string errore;
    if (valle.Carica(LivelloAvventura(), errore)) {
        valle.titolo = "La valle delle gemme (importata)";
        mondi.push_back(valle);
    }
    mondi.push_back(TorreDelCielo());
    mondi.push_back(IsolaAperta());
    mondi.push_back(CastelloAStanze());
    return mondi;
}
