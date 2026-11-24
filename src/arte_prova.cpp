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
