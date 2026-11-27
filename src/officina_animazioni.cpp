// Sezione Animazioni dell'officina: lo studio delle animazioni di personaggi e oggetti.
// A sinistra si sceglie chi animare, al centro l'anteprima che si muove (con il terreno e il punto
// d'appoggio da trascinare) e la pellicola dei fotogrammi (si trascinano per cambiare l'ordine),
// a destra le animazioni con il loro nome/azione, la velocita', il ciclo e il punto d'appoggio.
// Le immagini (PNG, JPG, BMP, GIF, TGA) si caricano con il gestore delle immagini dell'editor di disegno.
#include "officina.h"
#include "sistema.h"
#include "ui.h"
#include <algorithm>

namespace {

void Scacchi(Rectangle r, float lato) {
    DrawRectangleRec(r, Hex(0x2c2f48));
    Ui::Ritaglia((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    int nx = (int)(r.width / lato) + 1, ny = (int)(r.height / lato) + 1;
    for (int y = 0; y < ny; y++)
        for (int x = (y & 1); x < nx; x += 2) DrawRectangleRec({ r.x + x * lato, r.y + y * lato, lato, lato }, Hex(0x363a58));
    Ui::FineRitaglio();
}

const std::vector<std::string>& AzioniPersonaggio() {
    static const std::vector<std::string> v = { "fermo", "corsa", "salto", "caduta", "colpito", "attacco", "sparo", "vittoria" };
    return v;
}
const std::vector<std::string>& AzioniOggetto() {
    static const std::vector<std::string> v = { "fermo", "aperta", "chiusa", "accesa", "spenta", "scatta", "aperto", "chiuso" };
    return v;
}
const char* SpiegaAzione(const std::string& n) {
    if (n == "fermo") return "quando sta fermo (e la prima immagine se manca un'animazione)";
    if (n == "corsa") return "quando cammina o corre";
    if (n == "salto") return "quando sale nel salto";
    if (n == "caduta") return "quando scende (se manca usa salto)";
    if (n == "colpito") return "quando viene colpito";
    if (n == "attacco") return "quando attacca";
    if (n == "sparo") return "subito dopo uno sparo dell'eroe";
    if (n == "vittoria") return "quando vince il livello";
    if (n == "aperta" || n == "chiusa") return "porte: aperta / chiusa";
    if (n == "accesa" || n == "spenta") return "interruttori e punti di controllo";
    if (n == "scatta") return "molle: quando rimbalzano";
    if (n == "aperto" || n == "chiuso") return "forzieri";
    return "si avvia dalla Logica con \"cambia animazione\"";
}

} // namespace

void Officina::Animazioni(Rectangle r, float dt) {
    float s = Ui::S();
    int np = (int)progetto.personaggi.size(), no = (int)progetto.oggetti.size();
    Rectangle sx = { r.x, r.y, 250 * s, r.height };
    Rectangle dx = { r.x + r.width - 330 * s, r.y, 330 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width - dx.width, r.height };

    // --- chi animare ---
    std::vector<std::string> nomi;
    for (auto& p : progetto.personaggi) nomi.push_back(p.nome + "\npersonaggio, " + std::to_string(p.disegno.animazioni.size()) + " animazioni");
    for (auto& o : progetto.oggetti) nomi.push_back(o.nome + "\noggetto, " + std::to_string(o.disegno.animazioni.size()) + " animazioni");
    int prima = animElem;
    animElem = std::max(0, std::min(animElem, np + no - 1));
    Elenco(sx, "PERSONAGGI E OGGETTI", nomi, animElem, [&](int i, Rectangle q) {
        const Disegno& d = i < np ? progetto.personaggi[i].disegno : progetto.oggetti[i - np].disegno;
        d.DisegnaIn(q, std::max(0, d.TrovaAnimazione("fermo")), tempo);
    }, false);
    if (np + no == 0) {
        Ui::TestoACapo(progetto.EArcade() ? "Non ci sono ancora disegni tuoi. Nella scheda Gioco arcade scegli un nemico, un boss o l'eroe e premi "
                                            "\"Carica immagini...\" nel campo Grafica: il disegno compare qui, pronto da animare."
                                          : "Non ci sono ancora personaggi o oggetti: creane uno nella sezione Personaggi o Oggetti (o trascina qui un'immagine).",
                       centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    if (animElem != prima) { animAnim = 0; animFot = 0; tAnimStudio = 0; }
    bool personaggio = animElem < np;
    Disegno& d = personaggio ? progetto.personaggi[animElem].disegno : progetto.oggetti[animElem - np].disegno;
    std::string nomeElem = personaggio ? progetto.personaggi[animElem].nome : progetto.oggetti[animElem - np].nome;
    std::string id = (personaggio ? "p:" : "o:") + nomeElem;
    editorDisegno.Apri(&d, id);   // il gestore delle immagini lavora su questo disegno
    if (d.animazioni.empty()) { ::Animazione a; a.nome = "fermo"; Fotogramma f; f.Nuovo(d.w, d.h); a.fotogrammi.push_back(f); d.animazioni.push_back(a); }
    animAnim = std::max(0, std::min(animAnim, (int)d.animazioni.size() - 1));
    if (!editorDisegno.ImportAperto()) editorDisegno.ScegliAnimazione(animAnim);
    ::Animazione& A = d.animazioni[animAnim];
    if (A.fotogrammi.empty()) { Fotogramma f; f.Nuovo(d.w, d.h); A.fotogrammi.push_back(f); }
    animFot = std::max(0, std::min(animFot, (int)A.fotogrammi.size() - 1));
    if (!animPausa) tAnimStudio += dt;
    bool cambiato = false;
    auto Prima = [&]() { if (!cambiato) { Ricorda(); cambiato = true; } };
    auto CaricaImmagini = [&]() {
        std::vector<std::string> f = ScegliFile("Scegli le immagini per l'animazione", "Immagini", EditorDisegno::EstensioniImmagini(), true);
        if (!f.empty()) { editorDisegno.ScegliAnimazione(animAnim); editorDisegno.PreparaImport(f); }
    };

    // --- centro: barra, anteprima, pellicola ---
    DrawRectangleRec(centro, Hex(0x11142e));
    float pad = 14 * s, x = centro.x + pad, y = centro.y + 10 * s, w = centro.width - 2 * pad;
    Ui::TestoForte(nomeElem + "  -  " + A.nome, x, y, WHITE, (int)(22 * s));
    y += 36 * s;
    float bh = 40 * s;
    Ui::Aiuto("Carica una o piu' immagini PNG, JPG, BMP, GIF animate o TGA, anche un foglio di sprite: diventano i fotogrammi di questa animazione.");
    float wCarica = std::min(w * 0.5f, 380 * s);
    if (Ui::Pulsante({ x, y, wCarica, bh }, "Carica immagini (PNG, JPG, BMP, GIF)...", true)) CaricaImmagini();
    float bx = x + wCarica + 8 * s, bw = (w - wCarica - 16 * s) / 2;
    if (Ui::Pulsante({ bx, y, bw, bh }, animPausa ? "Riproduci" : "Pausa", !animPausa)) animPausa = !animPausa;
    Ui::Aiuto("Apre l'editor di disegno su questa animazione per ritoccare i pixel.");
    if (Ui::Pulsante({ bx + bw + 8 * s, y, bw, bh }, "Ritocca i pixel")) {
        editorDisegno.ScegliAnimazione(animAnim);
        if (personaggio) { selPersonaggio = animElem; Vai(Sezione::Personaggi); } else { selOggetto = animElem - np; Vai(Sezione::Oggetti); }
        return;
    }
    y += bh + 10 * s;
    float hPell = 150 * s;
    Rectangle vista = { x, y, w, centro.y + centro.height - y - hPell - 16 * s };
    Scacchi(vista, 12 * s);
    // il disegno grande, con i piedi sulla linea del terreno
    float k = std::max(1.0f, std::floor(std::min((vista.width - 40 * s) / d.w, (vista.height - 60 * s) / d.h)));
    if (d.liscio) k = std::min((vista.width - 40 * s) / d.w, (vista.height - 60 * s) / d.h);
    Vector2 piedi = { vista.x + vista.width / 2, vista.y + vista.height - 34 * s };
    DrawLineEx({ vista.x, piedi.y }, { vista.x + vista.width, piedi.y }, 2, Hex(0x5ac85a));
    DrawRectangle((int)vista.x, (int)piedi.y + 1, (int)vista.width, (int)(vista.y + vista.height - piedi.y - 1), Hex(0x3a6a3a, 120));
    int f = animPausa ? animFot : d.FotogrammaAl(animAnim, tAnimStudio);
    Texture2D tex = d.Texture(animAnim, f);
    Rectangle dove = { piedi.x - d.w * k / 2 + A.ox * k, piedi.y - d.h * k + A.oy * k, d.w * k, d.h * k };
    if (tex.id) DrawTexturePro(tex, { 0, 0, (float)tex.width, (float)tex.height }, dove, { 0, 0 }, 0, WHITE);
    DrawRectangleLinesEx(dove, 1, Hex(0xffffff, 40));
    // il punto d'appoggio (i piedi): una croce gialla; trascinando il disegno si sposta rispetto ai piedi
    DrawLineEx({ piedi.x - 10 * s, piedi.y }, { piedi.x + 10 * s, piedi.y }, 2, Ui::Accento());
    DrawLineEx({ piedi.x, piedi.y - 10 * s }, { piedi.x, piedi.y + 10 * s }, 2, Ui::Accento());
    Ui::Testo("Trascina il disegno per metterlo con i piedi sulla croce gialla (punto d'appoggio)", vista.x + 8 * s, vista.y + 6 * s, Hex(0xc8d0ff), Ui::TP());
    Ui::Testo(TextFormat("fotogramma %d di %d   -   %.0f fps%s", f + 1, (int)A.fotogrammi.size(), A.fps, A.ripeti ? "   -   in ciclo" : ""),
              vista.x + 8 * s, vista.y + vista.height - 24 * s, Hex(0xe8ecff), Ui::TP());
    if (Ui::Cliccato(dove)) { animTrascinaAppoggio = true; animAppoggioDa = { (float)A.ox, (float)A.oy }; animTrascinaDa = GetMousePosition(); }
    if (animTrascinaAppoggio) {
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) animTrascinaAppoggio = false;
        else {
            Vector2 m = GetMousePosition();
            int nx = (int)std::round(animAppoggioDa.x + (m.x - animTrascinaDa.x) / k), ny = (int)std::round(animAppoggioDa.y + (m.y - animTrascinaDa.y) / k);
            nx = std::max(-d.w, std::min(d.w, nx)); ny = std::max(-d.h, std::min(d.h, ny));
            if (nx != A.ox || ny != A.oy) { Prima(); A.ox = nx; A.oy = ny; }
        }
    }

    // pellicola dei fotogrammi
    Rectangle pell = { x, centro.y + centro.height - hPell - 8 * s, w, hPell };
    DrawRectangleRec(pell, Hex(0x0f1230));
    DrawRectangleLinesEx(pell, 1, Ui::Bordo());
    float hr = 30 * s, by = pell.y + 6 * s;
    Ui::Testo("FOTOGRAMMI  (trascinali per cambiare l'ordine)", pell.x + 8 * s, by + 7 * s, Ui::Accento(), Ui::TP());
    struct B { const char* t; int az; };
    B pul[] = { { "Duplica", 1 }, { "Elimina", 2 }, { "Inverti ordine", 3 }, { "Specchia", 4 } };
    float px = pell.x + pell.width - 8 * s;
    for (int i = 3; i >= 0; i--) {
        float wb = Ui::Larghezza(pul[i].t, Ui::TP()) + 24 * s;
        px -= wb;
        bool abil = pul[i].az != 2 || A.fotogrammi.size() > 1;
        if (Ui::Pulsante({ px, by, wb, hr }, pul[i].t, false, abil)) {
            Prima();
            if (pul[i].az == 1) { A.fotogrammi.insert(A.fotogrammi.begin() + animFot + 1, A.fotogrammi[animFot]); animFot++; }
            else if (pul[i].az == 2) { A.fotogrammi.erase(A.fotogrammi.begin() + animFot); animFot = std::max(0, animFot - 1); }
            else if (pul[i].az == 3) std::reverse(A.fotogrammi.begin(), A.fotogrammi.end());
            else {
                Fotogramma& fo = A.fotogrammi[animFot];
                for (int yy = 0; yy < fo.h; yy++) for (int xx = 0; xx < fo.w / 2; xx++) { Color c = fo.Prendi(xx, yy); fo.Metti(xx, yy, fo.Prendi(fo.w - 1 - xx, yy)); fo.Metti(fo.w - 1 - xx, yy, c); }
            }
        }
        px -= 6 * s;
    }
    Rectangle striscia = { pell.x + 8 * s, by + hr + 8 * s, pell.width - 16 * s, pell.y + pell.height - by - hr - 16 * s };
    float lato = striscia.height, gap = 8 * s;
    int n = (int)A.fotogrammi.size();
    float totale = n * (lato + gap) + lato;
    if (Ui::MouseSu(striscia)) scorriFotStudio -= GetMouseWheelMove() * (lato + gap);
    scorriFotStudio = Limita(scorriFotStudio, 0, std::max(0.0f, totale - striscia.width));
    Ui::Ritaglia((int)striscia.x, (int)striscia.y, (int)striscia.width, (int)striscia.height);
    int sopra = -1;
    for (int i = 0; i < n; i++) {
        Rectangle q = { striscia.x + i * (lato + gap) - scorriFotStudio, striscia.y, lato, lato };
        if (CheckCollisionPointRec(GetMousePosition(), { q.x - gap / 2, q.y, q.width + gap, q.height })) sopra = i;
        if (q.x > striscia.x + striscia.width || q.x + q.width < striscia.x) continue;
        Scacchi(q, 6 * s);
        Texture2D t = d.Texture(animAnim, i);
        if (t.id) {
            float kk = std::min((q.width - 6) / t.width, (q.height - 6) / t.height);
            DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height }, { q.x + (q.width - t.width * kk) / 2, q.y + (q.height - t.height * kk) / 2, t.width * kk, t.height * kk }, { 0, 0 }, 0,
                           animTrascina == i ? Hex(0xffffff, 90) : WHITE);
        }
        bool sel = i == animFot;
        DrawRectangleLinesEx(q, sel ? 3.0f : 1.0f, sel ? Ui::Accento() : Ui::Bordo());
        Ui::Testo(std::to_string(i + 1), q.x + 4 * s, q.y + 2 * s, Hex(0xe8ecff), (int)(12 * s));
        if (Ui::Cliccato(q) && CheckCollisionPointRec(GetMousePosition(), striscia)) { animFot = i; animTrascina = i; animPausa = true; }
    }
    // "+" alla fine: carica altre immagini
    Rectangle piu = { striscia.x + n * (lato + gap) - scorriFotStudio, striscia.y, lato, lato };
    if (Ui::Pulsante(piu, "+ immagini")) CaricaImmagini();
    // trascinamento: una linea gialla mostra dove andra' il fotogramma
    if (animTrascina >= 0) {
        if (sopra >= 0 && sopra != animTrascina) {
            float lx = striscia.x + sopra * (lato + gap) - scorriFotStudio + (sopra > animTrascina ? lato + gap / 2 : -gap / 2);
            DrawLineEx({ lx, striscia.y }, { lx, striscia.y + lato }, 4, Ui::Accento());
        }
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (sopra >= 0 && sopra != animTrascina && animTrascina < n) {
                Prima();
                Fotogramma fo = A.fotogrammi[animTrascina];
                A.fotogrammi.erase(A.fotogrammi.begin() + animTrascina);
                A.fotogrammi.insert(A.fotogrammi.begin() + sopra, fo);
                animFot = sopra;
            }
            animTrascina = -1;
        }
    }
    Ui::FineRitaglio();

    // --- destra: le animazioni ---
    DrawRectangleRec(dx, Ui::Pannello());
    DrawLineEx({ dx.x, dx.y }, { dx.x, dx.y + dx.height }, 1, Ui::Bordo());
    float qx = dx.x + 12 * s, qw = dx.width - 24 * s, qy = dx.y + 10 * s;
    Ui::Testo("ANIMAZIONI DI " + nomeElem, qx, qy, Ui::Accento(), Ui::TP());
    qy += 26 * s;
    float hl = 46 * s;
    float hElenco = std::min((float)d.animazioni.size() * (hl + 4 * s), dx.height * 0.38f);
    Rectangle elenco = { qx, qy, qw, hElenco };
    if (Ui::MouseSu(elenco)) scorriAnimStudio -= GetMouseWheelMove() * hl;
    scorriAnimStudio = Limita(scorriAnimStudio, 0, std::max(0.0f, d.animazioni.size() * (hl + 4 * s) - hElenco));
    Ui::Ritaglia((int)elenco.x, (int)elenco.y, (int)elenco.width, (int)elenco.height);
    for (int i = 0; i < (int)d.animazioni.size(); i++) {
        Rectangle q = { qx, qy + i * (hl + 4 * s) - scorriAnimStudio, qw, hl };
        if (q.y + q.height < elenco.y || q.y > elenco.y + elenco.height) continue;
        if (Ui::Pulsante(q, "", i == animAnim) && CheckCollisionPointRec(GetMousePosition(), elenco)) { animAnim = i; animFot = 0; tAnimStudio = 0; scorriFotStudio = 0; }
        Rectangle ic = { q.x + 4 * s, q.y + 3 * s, hl - 6 * s, hl - 6 * s };
        d.DisegnaIn(ic, i, tempo);
        Ui::Testo(d.animazioni[i].nome, q.x + hl + 6 * s, q.y + 5 * s, i == animAnim ? Ui::Accento() : WHITE, Ui::TP());
        Ui::Testo(TextFormat("%d fotogrammi, %.0f fps", (int)d.animazioni[i].fotogrammi.size(), d.animazioni[i].fps), q.x + hl + 6 * s, q.y + 25 * s, Ui::TestoSpento(), (int)(12 * s));
    }
    Ui::FineRitaglio();
    qy += hElenco + 8 * s;
    float w3 = (qw - 8 * s) / 3, hq = 30 * s;
    if (Ui::Pulsante({ qx, qy, w3, hq }, "+ Nuova")) {
        Prima();
        const auto& az = personaggio ? AzioniPersonaggio() : AzioniOggetto();
        std::string nn = "animazione";
        for (auto& a : az) if (d.TrovaAnimazione(a) < 0) { nn = a; break; }
        ::Animazione na; na.nome = nn; na.fps = A.fps;
        Fotogramma fo; fo.Nuovo(d.w, d.h); na.fotogrammi.push_back(fo);
        d.animazioni.push_back(na);
        animAnim = (int)d.animazioni.size() - 1; animFot = 0;
    }
    if (Ui::Pulsante({ qx + w3 + 4 * s, qy, w3, hq }, "Duplica")) {
        Prima();
        ::Animazione na = A; na.nome += " 2";
        d.animazioni.push_back(na);
        animAnim = (int)d.animazioni.size() - 1; animFot = 0;
    }
    if (Ui::Pulsante({ qx + 2 * (w3 + 4 * s), qy, w3, hq }, "Elimina", false, d.animazioni.size() > 1)) {
        Prima();
        d.animazioni.erase(d.animazioni.begin() + animAnim);
        animAnim = std::max(0, animAnim - 1); animFot = 0;
    }
    if (animAnim >= (int)d.animazioni.size()) animAnim = (int)d.animazioni.size() - 1;
    ::Animazione& B2 = d.animazioni[animAnim];
    qy += hq + 16 * s;
    Ui::Testo("ANIMAZIONE SCELTA", qx, qy, Ui::Accento(), Ui::TP());
    DrawLineEx({ qx, qy + 19 * s }, { qx + qw, qy + 19 * s }, 1, Ui::Bordo());
    qy += 28 * s;
    // nome = azione del gioco
    const auto& az = personaggio ? AzioniPersonaggio() : AzioniOggetto();
    std::vector<std::string> voci(az.begin(), az.end());
    voci.push_back("(nome mio)");
    int k2 = (int)voci.size() - 1;
    for (int i = 0; i < (int)az.size(); i++) if (az[i] == B2.nome) k2 = i;
    Ui::Aiuto("Per quale azione del gioco si usa questa animazione: il motore la sceglie da solo con questo nome.");
    if (Ui::Scelta({ qx, qy, qw, hq }, "Si usa per", k2, voci)) {
        if (k2 < (int)az.size() && d.TrovaAnimazione(az[k2]) >= 0 && d.TrovaAnimazione(az[k2]) != animAnim) Avvisa("C'e' gia' un'animazione \"" + az[k2] + "\"");
        else if (k2 < (int)az.size()) { Prima(); B2.nome = az[k2]; }
    }
    qy += hq + 4 * s;
    qy += Ui::TestoACapo(SpiegaAzione(B2.nome), qx, qy, qw, Ui::TestoSpento(), (int)(12 * s)) + 6 * s;
    std::string nome = B2.nome;
    Ui::Testo("Nome", qx, qy + 7 * s, Ui::TestoSpento(), Ui::TP());
    if (Ui::Campo("as_nome", { qx + qw * 0.45f, qy, qw * 0.55f, hq }, nome, 30) && !nome.empty() && nome != B2.nome) { Prima(); B2.nome = nome; }
    qy += hq + 8 * s;
    float fps = B2.fps;
    if (Ui::Numero("as_fps", { qx, qy, qw, hq }, "Velocita' (fps)", fps, 1, 30, 1)) { Prima(); B2.fps = fps; }
    qy += hq + 8 * s;
    bool rip = B2.ripeti;
    if (Ui::Interruttore({ qx, qy, qw, hq }, "Si ripete (ciclo)", rip)) { Prima(); B2.ripeti = rip; }
    qy += hq + 14 * s;
    Ui::Testo("PUNTO D'APPOGGIO (piedi)", qx, qy, Ui::Accento(), Ui::TP());
    qy += 24 * s;
    int ox = B2.ox, oy = B2.oy;
    if (Ui::NumeroIntero("as_ox", { qx, qy, qw, hq }, "Sposta a destra", ox, -d.w, d.w)) { Prima(); B2.ox = ox; }
    qy += hq + 6 * s;
    if (Ui::NumeroIntero("as_oy", { qx, qy, qw, hq }, "Sposta in basso", oy, -d.h, d.h)) { Prima(); B2.oy = oy; }
    qy += hq + 6 * s;
    if (Ui::Pulsante({ qx, qy, qw, hq }, "Rimetti al centro", false, B2.ox != 0 || B2.oy != 0)) { Prima(); B2.ox = B2.oy = 0; }
    qy += hq + 10 * s;
    Ui::TestoACapo(TextFormat("Disegno di %d x %d pixel. Trascina qui le immagini o usa \"Carica immagini\". Claude e' nella chat a destra: "
                              "\"fai l'animazione di corsa con 4 fotogrammi\".", d.w, d.h),
                   qx, qy, qw, Ui::TestoSpento(), (int)(12 * s));

    if (cambiato) {
        d.Invalida();
        modificato = true;
        LibreriaCambiata();
    }
}
