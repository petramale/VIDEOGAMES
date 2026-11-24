// Prova del Pennello e dell'arte: crea immagini PNG di controllo (StudioVideogiochi --prova-arte cartella).
#include "arte.h"
#include <cstdio>

using namespace arte;

int ProvaArte(const char* cartella) {
    MakeDirectory(cartella);
    Umanoide lia; lia.skin = "#f6cfa8"; lia.hair = "#e0542e"; lia.hairStyle = "bob"; lia.eye = "#2a9a8a"; lia.lash = true; lia.top = "#2f8f86";
    lia.bottom = "#3a2e52"; lia.shoes = "#5a3a24"; lia.cloak = "#8a2a3a"; lia.belt = "#5a3a24"; lia.collar = "#f2e6c8"; lia.item = "lantern";
    Umanoide nora; nora.skin = "#f2c39a"; nora.hair = "#6a3a1e"; nora.hairStyle = "pony"; nora.accent = "#f2c23a"; nora.eye = "#3a8a3a"; nora.lash = true;
    nora.top = "#f2e6c8"; nora.overall = "#c8742a"; nora.bottom = "#8a4a1e"; nora.shoes = "#4a2a1a"; nora.goggles = true; nora.item = "wrench"; nora.brows = true; nora.grin = true;
    Robot bull;
    // foglio: 7 animazioni x 6 personaggi
    const int CW = 220, CH = 240;
    Tela tela(CW * 7, CH * 6); K c(tela);
    c.fillStyle(lg(0, 0, 0, CH * 6, { "#2a2440", "#141226" })); c.fillRect(0, 0, CW * 7, CH * 6);
    Anim an[7] = { Anim::Fermo, Anim::Corsa, Anim::Salto, Anim::Caduta, Anim::Attacco, Anim::Colpito, Anim::Vittoria };
    for (int i = 0; i < 7; i++) {
        Posa p; p.a = an[i]; p.t = 0.5f; p.ph = 1.2f;
        DisegnaUmanoide(c, CW * i + CW / 2.f, CH * 1 - 20, 1.1f, lia, p);
        DisegnaUmanoide(c, CW * i + CW / 2.f, CH * 2 - 20, 1.1f, nora, p);
        DisegnaRobot(c, CW * i + CW / 2.f, CH * 3 - 20, 1.0f, bull, p);
        Dino d; DisegnaDino(c, CW * i + CW / 2.f, CH * 4 - 20, 1.0f, d, p);
        DisegnaPinguino(c, CW * i + CW / 2.f - 50, CH * 5 - 20, 1.0f, "#3ad8a8", p);
        DisegnaApe(c, CW * i + CW / 2.f + 50, CH * 5 - 80, 0.9f, p);
        DisegnaMech(c, CW * i + CW / 2.f, CH * 6 - 20, 0.9f, "#8a6a4a", p);
    }
    Image img = tela.Immagine();
    ExportImage(img, (std::string(cartella) + "/foglio_personaggi.png").c_str());
    UnloadImage(img);
    // un Disegno vero, come lo usera' il gioco
    OpzioniDisegno o; o.altezzaMondo = 40; o.dens = 4;
    Disegno dl = CreaDisegno([&](K& k, float x, float y, float s, const Posa& p) { DisegnaUmanoide(k, x, y, s, lia, p); }, o);
    for (size_t a = 0; a < dl.animazioni.size(); a++)
        for (size_t f = 0; f < dl.animazioni[a].fotogrammi.size(); f++) {
            const Fotogramma& fo = dl.animazioni[a].fotogrammi[f];
            Image im = GenImageColor(fo.w, fo.h, BLANK);
            for (int y = 0; y < fo.h; y++) for (int x = 0; x < fo.w; x++) ImageDrawPixel(&im, x, y, fo.Prendi(x, y));
            char nome[256]; std::snprintf(nome, sizeof nome, "%s/lia_%s_%zu.png", cartella, dl.animazioni[a].nome.c_str(), f);
            ExportImage(im, nome); UnloadImage(im);
        }
    std::printf("ok: disegno %dx%d densita %.1f\n", dl.w, dl.h, dl.densita);
    return 0;
}

// Scena di prova: mondo "Preistoria" di Nora e Bullone (cielo, 3 strati, terreno, piattaforma, eroi, primo piano).
int ProvaPaesaggio(const char* cartella) {
    MakeDirectory(cartella);
    const float GY = 596;
    Tela scena(1280, 720); K s(scena);
    { Cielo ci; ci.fermate = { "#f6a65a", "#f8c87a", "#fbe6b0" }; ci.sole[0] = 1020; ci.sole[1] = 170; ci.sole[2] = 46; ci.coloreSole = "#fff0b0"; ci.raggi = true;
      DisegnaCielo(s, ci); }
    auto strato = [&](const std::function<void(K&)>& f) { Tela t(LS, HS); K k(t); f(k); scena.Disegna(t, 0, 0); };
    strato([](K& c) { Cresta r; r.seme = 41; r.base = 430; r.amp = 260; r.punte = true; r.top = "#b86a52"; r.bot = "#8a4a3a"; r.light = "rgba(255,210,150,0.4)";
        r.shade = "rgba(60,20,20,0.3)"; r.lava = true; r.haze = "#f8c87a"; r.hazeA = 0.55f; DisegnaCresta(c, r); });
    strato([](K& c) { Cresta r; r.seme = 43; r.base = 510; r.amp = 110; r.top = "#7a8a4a"; r.bot = "#4a6a3a"; r.light = "rgba(220,255,160,0.3)";
        r.shade = "rgba(20,40,10,0.25)"; r.haze = "#fbe6b0"; r.hazeA = 0.4f; DisegnaCresta(c, r);
        ColoriAlbero a; a.trunk = "#8a6a3a"; a.dark = "#3a7a2a"; a.mid = "#5a9a3a"; a.light = "#8ac05a";
        Fila(44, 512, 120, 200, 110, [&](float x, float b, float h, unsigned sd) { Ripeti(x, h * 0.6f, [&](float xx) { Palma(c, xx, b, h, a, sd); }); }); });
    strato([](K& c) { RiempiSotto(c, 560, "#3a5a22");
        ColoriAlbero a; a.dark = "#2a5a1a"; a.mid = "#3a7a2a"; a.light = "#6aa83a";
        Fila(46, 566, 110, 180, 150, [&](float x, float b, float h, unsigned sd) { Ripeti(x, h * 0.6f, [&](float xx) { Felce(c, xx, b, h, a, sd); }); }); });
    Materiale m; m.kind = "grass"; m.body = "#7a4a2a"; m.top = "#6aa83a"; m.blades = { "#5a9a2a", "#7ac04a", "#9ad85a", "#4a8a2a" }; m.flowers = { "#ff6a8a", "#ffd04a" };
    DisegnaMateriale(s, 0, 1280, GY, 720 - GY, m, 7);
    { s.save(); s.translate(560, 410 - 22); DisegnaMateriale(s, 0, 240, 22, 58, m, 9); s.restore(); }
    Umanoide nora; nora.skin = "#f2c39a"; nora.hair = "#6a3a1e"; nora.hairStyle = "pony"; nora.accent = "#f2c23a"; nora.eye = "#3a8a3a"; nora.lash = true;
    nora.top = "#f2e6c8"; nora.overall = "#c8742a"; nora.bottom = "#8a4a1e"; nora.shoes = "#4a2a1a"; nora.goggles = true; nora.item = "wrench"; nora.brows = true; nora.grin = true;
    Posa p; p.a = Anim::Corsa; p.t = 0.4f; p.ph = 1.0f;
    DisegnaUmanoide(s, 300, GY + 2, 1.0f, nora, p);
    Robot bull; DisegnaRobot(s, 200, GY + 2, 1.0f, bull, p);
    Dino d; d.c = "#4aa85a"; d.belly = "#e8e0a0"; d.stripe = "#2a6a3a"; d.flip = true; DisegnaDino(s, 880, GY + 2, 0.9f, d, p);
    Dino d2; d2.c = "#c85a3a"; d2.belly = "#f2d8a0"; d2.stripe = "#7a2a1a"; d2.flip = true; Posa pf; DisegnaDino(s, 1120, GY + 2, 1.7f, d2, pf);
    { Tela f(LS, HS); K k(f); PrimoPiano(k, "#1a2a0e", "leaves"); f.Sfoca(3); scena.Disegna(f, 0, 0); }
    s.fillStyle(rg(640, 360, 360, 960, { "rgba(0,0,0,0)", "rgba(0,0,0,0.35)" })); s.fillRect(0, 0, 1280, 720);
    Image img = scena.Immagine();
    ExportImage(img, (std::string(cartella) + "/scena_preistoria.png").c_str());
    UnloadImage(img);
    std::printf("ok: scena\n");
    return 0;
}
