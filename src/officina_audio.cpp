// Sezione Audio dell'officina: musica a note (rullo di pianoforte a 4 tracce), effetti sonori
// (sintetizzatore con anteprima dell'onda), suoni degli eventi del gioco e musica di ogni mappa.
#include "officina.h"
#include "audio.h"
#include "raccolte.h"
#include "sistema.h"
#include "ui.h"
#include <algorithm>

namespace {

const char* NOMI_STRUMENTI[] = { "Onda quadra", "Triangolo", "Dente di sega", "Seno", "Batteria" };
const char* NOMI_NOTE[] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
const Color COLORI_TRACCE[] = { Hex(0xffd23f), Hex(0x5ec8ff), Hex(0x7ee07e), Hex(0xff7ab8), Hex(0xff9a4a), Hex(0xb88aff) };

bool Nera(int n) { int k = n % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }

// Una nota suonata subito (cliccando nel rullo).
void SuonaNota(int altezza, int strumento) {
    EffettoSuono e;
    e.nome = "nota";
    if (strumento == 4) {
        e.onda = 4;
        e.f0 = altezza <= 36 ? 120 : altezza <= 40 ? 400 : 3000;
        e.f1 = altezza <= 36 ? 50 : e.f0;
        e.durata = altezza == 42 ? 0.06f : 0.15f;
        e.rumore = altezza <= 36 ? 0.3f : 1;
    } else {
        e.onda = strumento;
        e.f0 = e.f1 = 440.0f * std::pow(2.0f, (altezza - 69) / 12.0f);
        e.durata = 0.25f;
    }
    e.volume = 0.4f;
    gAudio.SuonaEffetto(e);
}

bool SalvaWav(const std::vector<float>& c, const std::string& file) {
    if (c.empty()) return false;
    std::vector<short> d(c.size());
    for (size_t i = 0; i < c.size(); i++) d[i] = (short)(Limita(c[i], -1, 1) * 32000);
    Wave w{};
    w.frameCount = (unsigned int)d.size();
    w.sampleRate = 44100;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = d.data();
    return ExportWave(w, file.c_str());
}

} // namespace

void Officina::AzioneAudio(const std::string& c) {
    auto Esporta = [&](const std::vector<float>& campioni, const std::string& nome) {
        std::string cartella = CartellaEseguibile() + "/esportati";
        MakeDirectory(cartella.c_str());
        std::string f = cartella + "/" + NomeFileDaTitolo(nome) + ".wav";
        if (SalvaWav(campioni, f)) Avvisa("Salvato esportati/" + std::string(GetFileName(f.c_str())));
        else Avvisa("Non riesco a salvare il WAV");
    };
    if (c == "b:nuovo" || c == "b:componi" || c == "b:duplica" || c == "b:elimina") {
        Ricorda();
        schedaAudio = 0;
        if (c == "b:nuovo") {
            Brano b;
            b.Nuovo();
            b.nome = progetto.NomeLibero("Brano");
            for (auto& t : b.tracce) t.note.clear();
            progetto.brani.push_back(b);
            selBrano = (int)progetto.brani.size() - 1;
            Avvisa("Brano vuoto: clicca nel rullo per mettere le note");
        } else if (c == "b:componi") {
            int stile = GetRandomValue(0, (int)StiliBrano().size() - 1);
            Brano b = ComponiBrano(progetto.NomeLibero(StiliBrano()[stile]), GetRandomValue(1, 99999), stile, 16);
            progetto.brani.push_back(b);
            selBrano = (int)progetto.brani.size() - 1;
            Avvisa("Composto un brano " + StiliBrano()[stile] + ": modificalo come vuoi");
        } else if (selBrano < (int)progetto.brani.size()) {
            if (c == "b:duplica") {
                Brano b = progetto.brani[selBrano];
                b.nome = progetto.NomeLibero(b.nome);
                progetto.brani.push_back(b);
                selBrano = (int)progetto.brani.size() - 1;
            } else {
                std::string n = progetto.brani[selBrano].nome;
                progetto.brani.erase(progetto.brani.begin() + selBrano);
                for (auto& m : progetto.mondo.mappe) if (m.brano == n) m.brano.clear();
                selBrano = std::max(0, selBrano - 1);
                gAudio.FermaBranoPersonale();
                suonaBrano = false;
                MondoCambiato();
            }
        }
    } else if (c == "b:suona" && selBrano < (int)progetto.brani.size()) {
        suonaBrano = !suonaBrano;
        if (suonaBrano) gAudio.SuonaBrano(progetto.brani[selBrano]); else gAudio.FermaBranoPersonale();
    } else if (c == "b:usa" && selBrano < (int)progetto.brani.size()) {
        SincronizzaMondo();
        Ricorda();
        int mi = std::min(editorMondo.MappaAperta(), (int)progetto.mondo.mappe.size() - 1);
        progetto.mondo.mappe[mi].brano = progetto.brani[selBrano].nome;
        MondoCambiato();
        Avvisa("La mappa \"" + progetto.mondo.mappe[mi].nome + "\" ora suona " + progetto.brani[selBrano].nome);
    } else if (c == "b:wav" && selBrano < (int)progetto.brani.size()) {
        Esporta(Audio::RenderBrano(progetto.brani[selBrano]), progetto.brani[selBrano].nome);
    } else if (c == "e:nuovo" || c == "e:caso" || c == "e:duplica" || c == "e:elimina") {
        Ricorda();
        schedaAudio = 1;
        if (c == "e:nuovo" || c == "e:caso") {
            EffettoSuono e = c == "e:caso" ? EffettoCasuale(GetRandomValue(1, 999999)) : EffettoSuono{};
            e.nome = progetto.NomeLibero(c == "e:caso" ? "Effetto" : "Suono");
            progetto.effetti.push_back(e);
            selEffetto = (int)progetto.effetti.size() - 1;
            gAudio.SuonaEffetto(e);
        } else if (selEffetto < (int)progetto.effetti.size()) {
            if (c == "e:duplica") {
                EffettoSuono e = progetto.effetti[selEffetto];
                e.nome = progetto.NomeLibero(e.nome);
                progetto.effetti.push_back(e);
                selEffetto = (int)progetto.effetti.size() - 1;
            } else {
                progetto.effetti.erase(progetto.effetti.begin() + selEffetto);
                selEffetto = std::max(0, selEffetto - 1);
            }
        }
    } else if (c == "e:suona" && selEffetto < (int)progetto.effetti.size()) {
        gAudio.SuonaEffetto(progetto.effetti[selEffetto]);
    } else if (c == "e:wav" && selEffetto < (int)progetto.effetti.size()) {
        Esporta(Audio::RenderEffettoLibreria(progetto.effetti[selEffetto]), progetto.effetti[selEffetto].nome);
    }
}

void Officina::Audio(Rectangle r, float dt) {
    (void)dt;
    float s = Ui::S();
    Rectangle schede = { r.x, r.y, r.width, 40 * s };
    DrawRectangleRec(schede, Hex(0x0f1230));
    const char* nomiSchede[] = { "Musica", "Effetti sonori", "Suoni del gioco e musica delle mappe" };
    float x = r.x + 10 * s;
    for (int i = 0; i < 3; i++) {
        float w = Ui::Larghezza(nomiSchede[i], Ui::TP()) + 50 * s;
        if (Ui::Pulsante({ x, r.y + 5 * s, w, 30 * s }, nomiSchede[i], schedaAudio == i)) { schedaAudio = i; scorriLista = 0; }
        x += w + 8 * s;
    }
    r.y += schede.height; r.height -= schede.height;
    if (schedaAudio == 2) { SuoniEventi(r); return; }
    Rectangle sx = { r.x, r.y, 250 * s, r.height };
    Rectangle centro = { sx.x + sx.width, r.y, r.width - sx.width, r.height };
    if (schedaAudio == 0) {
        std::vector<std::string> nomi;
        for (auto& b : progetto.brani) {
            int usato = 0;
            for (auto& m : progetto.mondo.mappe) usato += m.brano == b.nome;
            nomi.push_back(b.nome + "\n" + (b.file.empty() ? TextFormat("%.0f bpm, %d battute", b.bpm, b.battute) : b.file.c_str()) + (usato ? "  - in uso" : ""));
        }
        int prima = selBrano;
        int az = Elenco(sx, "BRANI DEL GIOCO", nomi, selBrano, [&](int i, Rectangle q) {
            Color c = COLORI_TRACCE[i % 6];
            DrawCircleV({ q.x + q.width * 0.38f, q.y + q.height * 0.68f }, q.width * 0.16f, c);
            DrawRectangleRec({ q.x + q.width * 0.5f, q.y + q.height * 0.2f, q.width * 0.07f, q.height * 0.5f }, c);
            DrawRectangleRec({ q.x + q.width * 0.5f, q.y + q.height * 0.2f, q.width * 0.25f, q.height * 0.08f }, c);
        });
        if (prima != selBrano && suonaBrano && selBrano < (int)progetto.brani.size()) gAudio.SuonaBrano(progetto.brani[selBrano]);
        if (az == 1) AzioneAudio("b:nuovo");
        if (az == 2) AzioneAudio("b:duplica");
        if (az == 3) AzioneAudio("b:elimina");
        if (progetto.brani.empty()) {
            Ui::TestoACapo("Nessun brano. \"+ Nuovo\" per scriverlo nota per nota, il menu Musica > Componi per uno automatico, trascina un OGG/MP3/WAV, "
                           "oppure chiedi a Claude: \"componi un tema epico per il boss\".", centro.x + 40 * s, centro.y + 60 * s, centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
            return;
        }
        selBrano = std::min(selBrano, (int)progetto.brani.size() - 1);
        Brano& b = progetto.brani[selBrano];
        Brano vecchio = b;
        bool cambiato = false;
        // Barra in alto: nome, tempo, battute, ascolta, componi.
        float y = centro.y + 10 * s, bx = centro.x + 14 * s, h = 30 * s;
        std::string nome = b.nome;
        if (Ui::Campo("b_nome", { bx, y, 220 * s, h }, nome, 40) && !nome.empty() && nome != b.nome && !progetto.TrovaBrano(nome)) {
            Ricorda();
            for (auto& m : progetto.mondo.mappe) if (m.brano == b.nome) m.brano = nome;
            for (auto& rg : progetto.regole) for (auto& a : rg.azioni) if (a.tipo == "musica" && a.testo == b.nome) a.testo = nome;
            b.nome = nome;
            MondoCambiato();
            vecchio = b;
        }
        bx += 228 * s;
        if (Ui::Pulsante({ bx, y, 110 * s, h }, suonaBrano ? "Ferma" : "Ascolta", suonaBrano)) AzioneAudio("b:suona");
        bx += 118 * s;
        if (b.file.empty()) {
            if (Ui::Numero("b_bpm", { bx, y, 190 * s, h }, "Tempo", b.bpm, 40, 260, 5)) cambiato = true;
            bx += 198 * s;
            if (Ui::NumeroIntero("b_bat", { bx, y, 190 * s, h }, "Battute", b.battute, 1, 64)) cambiato = true;
            bx += 198 * s;
        }
        if (Ui::Pulsante({ bx, y, 200 * s, h }, "Usa nella mappa aperta")) AzioneAudio("b:usa");
        bx += 208 * s;
        if (Ui::Pulsante({ bx, y, 110 * s, h }, "Esporta WAV")) AzioneAudio("b:wav");
        y += h + 10 * s;
        if (!b.file.empty()) {
            Ui::TestoACapo("Brano importato dal file " + b.file + TextFormat(" (%.0f KB). Viene incluso nel gioco creato. ", b.dati.size() / 1024.0f) +
                           "Per comporre a note crea un brano nuovo.", centro.x + 14 * s, y + 20 * s, centro.width - 28 * s, Ui::TestoChiaro(), Ui::T());
            return;
        }
        // Tracce.
        float tw = std::min(240 * s, (centro.width - 28 * s) / std::max(1, (int)b.tracce.size()) - 8 * s);
        for (int i = 0; i < (int)b.tracce.size(); i++) {
            TracciaBrano& t = b.tracce[i];
            Rectangle tr = { centro.x + 14 * s + i * (tw + 8 * s), y, tw, 92 * s };
            bool sel = i == tracciaScelta;
            DrawRectangleRounded(tr, 0.1f, 6, sel ? Ui::Selezione() : Ui::Pannello());
            DrawRectangleRoundedLinesEx(tr, 0.1f, 6, sel ? 2.0f : 1.0f, sel ? COLORI_TRACCE[i % 6] : Ui::Bordo());
            if (Ui::MouseSu({ tr.x, tr.y, tr.width, 26 * s }) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) tracciaScelta = i;
            Ui::Testo(t.nome + TextFormat("  (%d note)", (int)t.note.size()), tr.x + 8 * s, tr.y + 6 * s, sel ? WHITE : COLORI_TRACCE[i % 6], Ui::TP());
            std::vector<std::string> strum(NOMI_STRUMENTI, NOMI_STRUMENTI + 5);
            if (Ui::Scelta({ tr.x + 6 * s, tr.y + 30 * s, tr.width - 12 * s, 26 * s }, "", t.strumento, strum)) { t.batteria = t.strumento == 4; cambiato = true; tracciaScelta = i; }
            if (Ui::Numero("b_vol" + std::to_string(i), { tr.x + 6 * s, tr.y + 60 * s, tr.width - 12 * s, 26 * s }, "Volume", t.volume, 0, 1, 0.05f, "%.2f")) cambiato = true;
        }
        if (b.tracce.size() < 6 && Ui::Pulsante({ centro.x + 14 * s + b.tracce.size() * (tw + 8 * s), y, 44 * s, 92 * s }, "+")) {
            TracciaBrano t;
            t.nome = "Traccia " + std::to_string(b.tracce.size() + 1);
            b.tracce.push_back(t);
            tracciaScelta = (int)b.tracce.size() - 1;
            cambiato = true;
        }
        y += 100 * s;
        tracciaScelta = (int)Limita((float)tracciaScelta, 0, (float)b.tracce.size() - 1);
        Rectangle rullo = { centro.x + 14 * s, y, centro.width - 28 * s, centro.y + centro.height - y - 40 * s };
        Brano primaNote = b;
        PianoRoll(rullo, b);
        bool noteCambiate = primaNote.tracce.size() == b.tracce.size();
        if (noteCambiate) {
            noteCambiate = false;
            for (size_t i = 0; i < b.tracce.size(); i++) if (b.tracce[i].note.size() != primaNote.tracce[i].note.size()) noteCambiate = true;
            if (!noteCambiate)
                for (size_t i = 0; i < b.tracce.size() && !noteCambiate; i++)
                    for (size_t k = 0; k < b.tracce[i].note.size(); k++) {
                        const NotaBrano& a = b.tracce[i].note[k], &c = primaNote.tracce[i].note[k];
                        if (a.inizio != c.inizio || a.durata != c.durata || a.altezza != c.altezza) { noteCambiate = true; break; }
                    }
        }
        if (noteCambiate && !trascinaNota) cambiato = true;
        // Composizione automatica in fondo.
        float yb = centro.y + centro.height - 34 * s;
        Ui::Testo("Lunghezza nota:", centro.x + 14 * s, yb + 7 * s, Ui::TestoSpento(), Ui::TP());
        const float lung[] = { 1, 2, 4, 8, 16 };
        const char* nl[] = { "1/16", "1/8", "1/4", "1/2", "1" };
        for (int i = 0; i < 5; i++)
            if (Ui::Pulsante({ centro.x + 140 * s + i * 58 * s, yb, 54 * s, 28 * s }, nl[i], lunghezzaNota == lung[i])) lunghezzaNota = lung[i];
        Ui::Testo("Clic: nota   Trascina: allunga   Destro: cancella   Rotella: su/giu'   Maiusc+rotella: avanti", centro.x + 450 * s, yb + 7 * s, Ui::TestoSpento(), (int)(13 * s));
        if (cambiato) {
            ProgettoGioco v = progetto;
            v.brani[selBrano] = vecchio;
            annulla.push_back(v);
            if (annulla.size() > 30) annulla.erase(annulla.begin());
            ripeti.clear();
            modificato = true;
            if (suonaBrano) gAudio.SuonaBrano(b);
        }
        return;
    }
    // Effetti.
    std::vector<std::string> nomi;
    for (auto& e : progetto.effetti) nomi.push_back(e.nome + "\n" + (e.file.empty() ? NOMI_STRUMENTI[std::min(4, std::max(0, e.onda))] : e.file.c_str()));
    int prima = selEffetto;
    int az = Elenco(sx, "EFFETTI SONORI", nomi, selEffetto, [&](int i, Rectangle q) {
        const EffettoSuono& e = progetto.effetti[i];
        for (int k = 0; k < 8; k++) {
            float hh = (0.2f + 0.8f * std::fabs(std::sin(k * 1.3f + e.f0 * 0.01f))) * q.height * 0.6f;
            DrawRectangleRec({ q.x + 6 + k * (q.width - 12) / 8, q.y + q.height / 2 - hh / 2, (q.width - 12) / 8 - 2, hh }, COLORI_TRACCE[i % 6]);
        }
    });
    if (prima != selEffetto && selEffetto < (int)progetto.effetti.size()) gAudio.SuonaEffetto(progetto.effetti[selEffetto]);
    if (az == 1) AzioneAudio("e:nuovo");
    if (az == 2) AzioneAudio("e:duplica");
    if (az == 3) AzioneAudio("e:elimina");
    if (progetto.effetti.empty()) {
        Ui::TestoACapo("Nessun effetto: \"+ Nuovo\", trascina un WAV o chiedi a Claude \"crea un effetto laser\".", centro.x + 40 * s, centro.y + 60 * s,
                       centro.width - 80 * s, Ui::TestoSpento(), Ui::T());
        return;
    }
    selEffetto = std::min(selEffetto, (int)progetto.effetti.size() - 1);
    EditorEffetto(centro, progetto.effetti[selEffetto]);
}

void Officina::PianoRoll(Rectangle r, Brano& b) {
    float s = Ui::S();
    TracciaBrano& t = b.tracce[tracciaScelta];
    bool batteria = t.strumento == 4;
    int basso = batteria ? 34 : 28, alto = batteria ? 52 : 96;
    float wk = 56 * s, ch = batteria ? 22 * s : 14 * s, cw = 20 * s * zoomNote;
    int sedicesimi = b.battute * 16;
    Rectangle griglia = { r.x + wk, r.y, r.width - wk, r.height };
    DrawRectangleRec(r, Hex(0x0b0d22));
    // Scorrimento.
    if (Ui::MouseSu(r)) {
        float rot = GetMouseWheelMove();
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) scorriNoteX -= rot * cw * 4;
        else if (IsKeyDown(KEY_LEFT_CONTROL)) zoomNote = Limita(zoomNote * (rot > 0 ? 1.2f : rot < 0 ? 1 / 1.2f : 1), 0.4f, 3);
        else scorriNoteY -= rot * ch * 3;
    }
    float totH = (alto - basso + 1) * ch, totW = sedicesimi * cw;
    if (!batteria && scorriNoteY == 0 && totH > r.height) scorriNoteY = (alto - 72) * ch - r.height / 2;   // si parte intorno al do centrale
    scorriNoteY = Limita(scorriNoteY, 0, std::max(0.0f, totH - r.height));
    scorriNoteX = Limita(scorriNoteX, 0, std::max(0.0f, totW - griglia.width));
    auto YNota = [&](int n) { return r.y + (alto - n) * ch - scorriNoteY; };
    auto XSed = [&](float sd) { return griglia.x + sd * cw - scorriNoteX; };
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    // Righe delle note e tastiera.
    for (int n = basso; n <= alto; n++) {
        float y = YNota(n);
        if (y + ch < r.y || y > r.y + r.height) continue;
        DrawRectangleRec({ griglia.x, y, griglia.width, ch }, Nera(n) ? Hex(0x10132c) : Hex(0x161a3a));
        DrawLineEx({ griglia.x, y + ch }, { griglia.x + griglia.width, y + ch }, 1, Hex(0x0b0d22));
        std::string nome;
        if (batteria) nome = n == 36 ? "Cassa" : n == 38 ? "Rullante" : n == 42 ? "Charlest." : n == 46 ? "Piatto" : std::to_string(n);
        else nome = std::string(NOMI_NOTE[n % 12]) + std::to_string(n / 12 - 1);
        DrawRectangleRec({ r.x, y, wk - 2, ch }, Nera(n) && !batteria ? Hex(0x22223a) : Hex(0xd8dcf0));
        if (batteria || n % 12 == 0) Ui::Testo(nome, r.x + 4 * s, y + (ch - 11 * s) / 2, Nera(n) && !batteria ? WHITE : Hex(0x1a1c30), (int)(11 * s));
    }
    // Battute.
    for (int k = 0; k <= sedicesimi; k++) {
        float x = XSed((float)k);
        if (x < griglia.x || x > griglia.x + griglia.width) continue;
        DrawLineEx({ x, r.y }, { x, r.y + r.height }, k % 16 == 0 ? 2.0f : 1.0f, k % 16 == 0 ? Hex(0x5a64b0) : k % 4 == 0 ? Hex(0x2a3060) : Hex(0x1a1e40));
        if (k % 16 == 0 && k < sedicesimi) Ui::Testo(std::to_string(k / 16 + 1), x + 3 * s, r.y + 2 * s, Ui::TestoSpento(), (int)(11 * s));
    }
    // Note delle altre tracce (sbiadite) e della traccia scelta.
    for (int i = 0; i < (int)b.tracce.size(); i++) {
        if (i == tracciaScelta || (b.tracce[i].strumento == 4) != batteria) continue;
        for (auto& n : b.tracce[i].note) {
            if (n.altezza < basso || n.altezza > alto) continue;
            DrawRectangleRec({ XSed(n.inizio), YNota(n.altezza) + 2, n.durata * cw - 1, ch - 4 }, ConAlfa(COLORI_TRACCE[i % 6], 0.25f));
        }
    }
    Color ct = COLORI_TRACCE[tracciaScelta % 6];
    for (int k = 0; k < (int)t.note.size(); k++) {
        auto& n = t.note[k];
        Rectangle q = { XSed(n.inizio) + 1, YNota(n.altezza) + 1, std::max(3.0f, n.durata * cw - 2), ch - 2 };
        DrawRectangleRounded(q, 0.3f, 4, k == notaPresa ? WHITE : ct);
        DrawRectangleRoundedLinesEx(q, 0.3f, 4, 1, Hex(0x000000, 120));
    }
    // Cursore di riproduzione.
    if (suonaBrano) {
        float sd = std::fmod(gAudio.SedicesimoAttuale(), (float)sedicesimi);
        float x = XSed(sd);
        DrawLineEx({ x, r.y }, { x, r.y + r.height }, 2, Ui::Accento());
    }
    EndScissorMode();
    DrawRectangleLinesEx(r, 1, Ui::Bordo());
    // Mouse.
    Vector2 m = GetMousePosition();
    bool sopra = Ui::MouseSu(griglia);
    float sd = (m.x - griglia.x + scorriNoteX) / cw;
    int nota = alto - (int)std::floor((m.y - r.y + scorriNoteY) / ch);
    auto NotaSotto = [&]() {
        for (int k = (int)t.note.size() - 1; k >= 0; k--) {
            auto& n = t.note[k];
            if (n.altezza == nota && sd >= n.inizio && sd < n.inizio + n.durata) return k;
        }
        return -1;
    };
    if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sd >= 0 && sd < sedicesimi) {
        int k = NotaSotto();
        if (k < 0) {
            NotaBrano n;
            n.inizio = std::floor(sd / std::min(lunghezzaNota, 4.0f)) * std::min(lunghezzaNota, 4.0f);
            n.durata = lunghezzaNota;
            n.altezza = nota;
            t.note.push_back(n);
            k = (int)t.note.size() - 1;
        }
        notaPresa = k;
        trascinaNota = true;
        SuonaNota(t.note[k].altezza, t.strumento);
    }
    if (trascinaNota && notaPresa >= 0 && notaPresa < (int)t.note.size()) {
        auto& n = t.note[notaPresa];
        n.durata = std::max(1.0f, std::ceil(sd - n.inizio));
        n.durata = std::min(n.durata, (float)sedicesimi - n.inizio);
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { trascinaNota = false; notaPresa = -1; }
    } else trascinaNota = false;
    if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        int k = NotaSotto();
        if (k >= 0) t.note.erase(t.note.begin() + k);
    }
    if (sopra && !trascinaNota) {
        std::string nome = batteria ? std::to_string(nota) : std::string(NOMI_NOTE[((nota % 12) + 12) % 12]) + std::to_string(nota / 12 - 1);
        Ui::Testo(nome + TextFormat("  battuta %d", (int)(sd / 16) + 1), r.x + r.width - 180 * s, r.y + r.height + 4 * s, Ui::TestoSpento(), (int)(12 * s));
    }
    if (!Ui::StaScrivendo() && IsKeyPressed(KEY_SPACE)) AzioneAudio("b:suona");
}

void Officina::EditorEffetto(Rectangle r, EffettoSuono& e) {
    float s = Ui::S();
    EffettoSuono prima = e;
    bool cambiato = false;
    float x = r.x + 20 * s, y = r.y + 14 * s, w = r.width - 40 * s;
    std::string nome = e.nome;
    if (Ui::Campo("e_nome", { x, y, 260 * s, 32 * s }, nome, 40) && !nome.empty() && nome != e.nome && !progetto.TrovaEffetto(nome)) {
        Ricorda();
        for (auto& p : progetto.personaggi) for (auto& sn : p.suoni) if (sn.second == e.nome) sn.second = nome;
        for (auto& o : progetto.oggetti) if (o.suono == e.nome) o.suono = nome;
        for (auto& ev : progetto.suoniEventi) if (ev.second == e.nome) ev.second = nome;
        for (auto& rg : progetto.regole) for (auto& a : rg.azioni) if (a.tipo == "suona" && a.testo == e.nome) a.testo = nome;
        e.nome = nome;
        prima = e;
    }
    if (Ui::Pulsante({ x + 270 * s, y, 120 * s, 32 * s }, "Ascolta", true)) gAudio.SuonaEffetto(e);
    if (Ui::Pulsante({ x + 398 * s, y, 120 * s, 32 * s }, "A caso")) {
        std::string n = e.nome;
        e = EffettoCasuale(GetRandomValue(1, 999999));
        e.nome = n;
        cambiato = true;
        gAudio.SuonaEffetto(e);
    }
    if (Ui::Pulsante({ x + 526 * s, y, 130 * s, 32 * s }, "Esporta WAV")) AzioneAudio("e:wav");
    y += 46 * s;
    // Forma dell'onda.
    std::vector<float> c = Audio::RenderEffettoLibreria(e);
    Rectangle onda = { x, y, w, 150 * s };
    DrawRectangleRounded(onda, 0.05f, 6, Hex(0x0b0d22));
    if (!c.empty()) {
        int n = (int)onda.width;
        for (int i = 0; i < n; i++) {
            size_t a = (size_t)i * c.size() / n, b2 = (size_t)(i + 1) * c.size() / n;
            float mn = 0, mx = 0;
            for (size_t k = a; k < b2 && k < c.size(); k++) { mn = std::min(mn, c[k]); mx = std::max(mx, c[k]); }
            DrawLineEx({ onda.x + i, onda.y + onda.height / 2 - mx * onda.height * 0.45f }, { onda.x + i, onda.y + onda.height / 2 - mn * onda.height * 0.45f + 1 }, 1, Hex(0x7ee0ff));
        }
    }
    Ui::Testo(TextFormat("%.2f secondi", c.size() / 44100.0f), onda.x + 8 * s, onda.y + 6 * s, Ui::TestoSpento(), Ui::TP());
    y += onda.height + 14 * s;
    if (!e.file.empty()) {
        Ui::TestoACapo("Effetto importato dal file " + e.file + TextFormat(" (%.0f KB). ", e.dati.size() / 1024.0f) + "Viene incluso nel gioco creato.",
                       x, y, w, Ui::TestoChiaro(), Ui::T());
        return;
    }
    float cw = (w - 20 * s) / 2, h = 30 * s;
    float yl = y, yr = y;
    std::vector<std::string> onde(NOMI_STRUMENTI, NOMI_STRUMENTI + 5);
    onde[4] = "Rumore";
    if (Ui::Scelta({ x, yl, cw, h }, "Onda", e.onda, onde)) cambiato = true;
    yl += h + 8 * s;
    auto Num = [&](float& yy, float xx, const char* id, const char* et, float& v, float lo, float hi, float passo, const char* f) {
        if (Ui::Numero(id, { xx, yy, cw, h }, et, v, lo, hi, passo, f)) cambiato = true;
        yy += h + 8 * s;
    };
    Num(yl, x, "e_f0", "Frequenza iniziale (Hz)", e.f0, 20, 8000, 20, "%.0f");
    Num(yl, x, "e_f1", "Frequenza finale (Hz)", e.f1, 20, 8000, 20, "%.0f");
    Num(yl, x, "e_dur", "Durata (s)", e.durata, 0.02f, 3, 0.02f, "%.2f");
    Num(yr, x + cw + 20 * s, "e_vol", "Volume", e.volume, 0, 1, 0.05f, "%.2f");
    Num(yr, x + cw + 20 * s, "e_duty", "Onda quadra: duty", e.duty, 0.05f, 0.95f, 0.05f, "%.2f");
    Num(yr, x + cw + 20 * s, "e_vib", "Vibrato (0-1)", e.vibrato, 0, 1, 0.01f, "%.2f");
    Num(yr, x + cw + 20 * s, "e_vibhz", "Vibrato (Hz)", e.vibratoHz, 0, 40, 1, "%.0f");
    Num(yr, x + cw + 20 * s, "e_rum", "Rumore (0-1)", e.rumore, 0, 1, 0.05f, "%.2f");
    y = std::max(yl, yr) + 10 * s;
    Ui::Testo("Parti da un effetto pronto:", x, y, Ui::TestoSpento(), Ui::TP());
    y += 24 * s;
    static const std::vector<EffettoSuono> pronti = EffettiArcade();
    float bw = (w - 9 * 6 * s) / 10;
    for (int i = 0; i < (int)pronti.size(); i++) {
        Rectangle b = { x + (i % 10) * (bw + 6 * s), y + (i / 10) * (h + 6 * s), bw, h };
        if (b.y + b.height > r.y + r.height) break;
        if (Ui::Pulsante(b, pronti[i].nome)) {
            std::string n = e.nome;
            e = pronti[i];
            e.nome = n;
            e.autore.clear();
            cambiato = true;
            gAudio.SuonaEffetto(e);
        }
    }
    if (cambiato) {
        ProgettoGioco v = progetto;
        v.effetti[selEffetto] = prima;
        annulla.push_back(v);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) gAudio.SuonaEffetto(e);
    }
}

void Officina::SuoniEventi(Rectangle r) {
    float s = Ui::S();
    float x = r.x + 30 * s, y = r.y + 20 * s, w = std::min(r.width - 60 * s, 900 * s), h = 32 * s;
    Ui::Testo("Suoni degli eventi del gioco", x, y, WHITE, (int)(22 * s));
    y += 34 * s;
    Ui::TestoACapo("Ogni evento del motore puo' usare un effetto della libreria del gioco al posto di quello di serie.", x, y, w, Ui::TestoSpento(), Ui::TP());
    y += 30 * s;
    std::vector<std::string> effetti = { "(quello di serie)" };
    for (auto& e : progetto.effetti) effetti.push_back(e.nome);
    const char* eventi[] = { "salto", "moneta", "gemma", "colpo", "molla", "forziere", "esplosione", "vittoria", "passo" };
    const char* spiega[] = { "L'eroe salta", "Raccoglie una moneta", "Raccoglie una gemma o un oggetto", "Colpisce o viene colpito", "Rimbalza su una molla",
                             "Apre un forziere o una porta", "Un nemico esplode", "Livello completato", "Passi dell'eroe" };
    float cw = (w - 20 * s) / 2;
    for (int i = 0; i < 9; i++) {
        float xx = x + (i % 2) * (cw + 20 * s), yy = y + (i / 2) * (h + 10 * s);
        Ui::Testo(spiega[i], xx, yy + 8 * s, Ui::TestoChiaro(), Ui::TP());
        int k = 0;
        auto it = progetto.suoniEventi.find(eventi[i]);
        if (it != progetto.suoniEventi.end()) for (int j = 1; j < (int)effetti.size(); j++) if (effetti[j] == it->second) k = j;
        if (Ui::Scelta({ xx + cw * 0.42f, yy, cw * 0.58f - 44 * s, h }, "", k, effetti)) {
            Ricorda();
            if (k == 0) progetto.suoniEventi.erase(eventi[i]); else progetto.suoniEventi[eventi[i]] = effetti[k];
        }
        if (Ui::Pulsante({ xx + cw - 40 * s, yy, 40 * s, h }, ">")) {
            if (k > 0) { if (auto* e = progetto.TrovaEffetto(effetti[k])) gAudio.SuonaEffetto(*e); }
            else {
                const Effetto ev[] = { Effetto::Salto, Effetto::Moneta, Effetto::Gemma, Effetto::Colpo, Effetto::Molla, Effetto::Forziere, Effetto::Esplosione, Effetto::Vittoria, Effetto::Passo };
                gAudio.Suona(ev[i]);
            }
        }
    }
    y += 5 * (h + 10 * s) + 20 * s;
    Ui::Testo("Musica di ogni mappa", x, y, WHITE, (int)(22 * s));
    y += 38 * s;
    SincronizzaMondo();
    std::vector<std::string> brani;
    for (auto b : ::Audio::Brani()) brani.push_back(std::string("di serie: ") + b);
    int primiGenerati = (int)brani.size();
    for (auto& b : progetto.brani) brani.push_back(b.nome);
    for (int i = 0; i < (int)progetto.mondo.mappe.size(); i++) {
        Mappa& m = progetto.mondo.mappe[i];
        float yy = y + i * (h + 8 * s);
        if (yy + h > r.y + r.height) break;
        Ui::Testo(m.nome, x, yy + 8 * s, Ui::TestoChiaro(), Ui::TP());
        int k = m.musica;
        if (!m.brano.empty()) for (int j = 0; j < (int)progetto.brani.size(); j++) if (progetto.brani[j].nome == m.brano) k = primiGenerati + j;
        if (Ui::Scelta({ x + 260 * s, yy, w - 260 * s, h }, "", k, brani)) {
            Ricorda();
            if (k < primiGenerati) { m.musica = k; m.brano.clear(); }
            else m.brano = progetto.brani[k - primiGenerati].nome;
            MondoCambiato();
        }
    }
}
