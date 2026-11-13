#include "ui.h"
#include "audio.h"
#include "progetto.h"
#include <cstring>

namespace Ui {
namespace {
float scala = 1;
Rectangle modale{ 0, 0, 0, 0 };
bool dentroModale = false;
std::string fuoco, bozza;     // campo con la tastiera e testo in modifica
bool fuocoUsato = false;      // il campo col fuoco e' stato disegnato in questo fotogramma
float lampeggio = 0;
}

void Inizia(float s) {
    scala = Limita(s, 0.7f, 2.5f);
    modale = { 0, 0, 0, 0 };
    dentroModale = false;
    if (!fuocoUsato) fuoco.clear();   // il campo non c'e' piu' (pannello chiuso)
    fuocoUsato = false;
    lampeggio += GetFrameTime();
}

float S() { return scala; }
int T() { return (int)(17 * scala); }
int TP() { return (int)(14 * scala); }

Color Fondo() { return Hex(0x0d0f24); }
Color Pannello() { return Hex(0x161a3a); }
Color Bordo() { return Hex(0x343c78); }
Color Accento() { return Hex(0xffd23f); }
Color TestoChiaro() { return Hex(0xe8ecff); }
Color TestoSpento() { return Hex(0x8a90c0); }
Color Selezione() { return Hex(0x2e3680); }

void ImpostaModale(Rectangle r) { modale = r; }
void ApriModale() { dentroModale = true; }
void ChiudiModale() { dentroModale = false; }

bool MouseSu(Rectangle r) {
    Vector2 m = GetMousePosition();
    if (!CheckCollisionPointRec(m, r)) return false;
    if (!dentroModale && modale.width > 0 && CheckCollisionPointRec(m, modale)) return false;
    return true;
}

bool Cliccato(Rectangle r, int tasto) { return MouseSu(r) && IsMouseButtonPressed(tasto); }

static Font Carattere() { return GetFontDefault(); }

void Testo(const std::string& t, float x, float y, Color c, int dim) {
    if (dim <= 0) dim = T();
    DrawTextEx(Carattere(), t.c_str(), { std::round(x), std::round(y) }, (float)dim, dim / 10.0f, c);
}

int Larghezza(const std::string& t, int dim) {
    if (dim <= 0) dim = T();
    return (int)MeasureTextEx(Carattere(), t.c_str(), (float)dim, dim / 10.0f).x;
}

void TestoCentrato(const std::string& t, Rectangle r, Color c, int dim) {
    if (dim <= 0) dim = T();
    Testo(t, r.x + (r.width - Larghezza(t, dim)) / 2, r.y + (r.height - dim) / 2, c, dim);
}

float TestoACapo(const std::string& t, float x, float y, float larghezza, Color c, int dim, bool disegna) {
    if (dim <= 0) dim = T();
    float riga = dim * 1.3f, yy = y;
    std::string paragrafo;
    size_t inizio = 0;
    while (inizio <= t.size()) {
        size_t a = t.find('\n', inizio);
        paragrafo = t.substr(inizio, a == std::string::npos ? std::string::npos : a - inizio);
        std::string linea, parola;
        for (size_t i = 0; i <= paragrafo.size(); i++) {
            char ch = i < paragrafo.size() ? paragrafo[i] : ' ';
            if (ch != ' ') { parola += ch; continue; }
            std::string prova = linea.empty() ? parola : linea + " " + parola;
            if (!linea.empty() && Larghezza(prova, dim) > larghezza) {
                if (disegna) Testo(linea, x, yy, c, dim);
                yy += riga;
                linea = parola;
            } else linea = prova;
            parola.clear();
        }
        if (disegna) Testo(linea, x, yy, c, dim);
        yy += riga;
        if (a == std::string::npos) break;
        inizio = a + 1;
    }
    return yy - y;
}

bool Pulsante(Rectangle r, const std::string& testo, bool attivo, bool abilitato) {
    bool sopra = abilitato && MouseSu(r);
    Color fondo = attivo ? Selezione() : sopra ? Hex(0x252b5c) : Pannello();
    DrawRectangleRounded(r, 0.25f, 6, fondo);
    DrawRectangleRoundedLinesEx(r, 0.25f, 6, std::max(1.0f, scala), attivo || sopra ? Accento() : Bordo());
    // Testo troppo lungo: si accorcia con "..." (niente forbici: il pulsante puo' stare in un pannello che ne usa gia').
    std::string t = testo;
    float max = r.width - 10 * scala;
    if (Larghezza(t, TP()) > max) {
        while (!t.empty() && Larghezza(t + "...", TP()) > max) {
            t.pop_back();
            while (!t.empty() && ((unsigned char)t.back() & 0xC0) == 0x80) t.pop_back();
        }
        t += "...";
    }
    TestoCentrato(t, r, abilitato ? (attivo ? WHITE : TestoChiaro()) : TestoSpento(), TP());
    bool premuto = sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (premuto) { gAudio.Suona(Effetto::Menu, 0.35f); TogliFuoco(); }
    return premuto;
}

bool StaScrivendo() { return !fuoco.empty(); }
void TogliFuoco() { fuoco.clear(); }
void DaiFuoco(const std::string& id) { fuoco = id; bozza.clear(); fuocoUsato = true; lampeggio = 0; }
std::string Fuoco() { return fuoco; }

bool Campo(const std::string& id, Rectangle r, std::string& testo, size_t massimo) {
    bool mio = fuoco == id;
    bool confermato = false;
    if (mio) fuocoUsato = true;
    if (!mio && Cliccato(r)) { fuoco = id; bozza = testo; mio = true; fuocoUsato = true; lampeggio = 0; }
    else if (mio && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), r)) {
        // Clic altrove: conferma.
        if (bozza != testo) { testo = bozza; confermato = true; }
        fuoco.clear(); mio = false;
    }
    if (mio) {
        int c;
        while ((c = GetCharPressed()) > 0)
            if (c >= 32 && bozza.size() < massimo) {
                // UTF-8 per le lettere accentate.
                int n = 0;
                const char* u = CodepointToUTF8(c, &n);
                bozza.append(u, n);
            }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !bozza.empty()) {
            size_t i = bozza.size() - 1;
            while (i > 0 && ((unsigned char)bozza[i] & 0xC0) == 0x80) i--;   // toglie un carattere intero
            bozza.erase(i);
        }
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrl && IsKeyPressed(KEY_V)) {
            const char* incolla = GetClipboardText();
            if (incolla) for (const char* p = incolla; *p && bozza.size() < massimo; p++) if (*p != '\n' && *p != '\r') bozza += *p;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_TAB)) {
            if (bozza != testo) { testo = bozza; confermato = true; }
            fuoco.clear(); mio = false;
        } else if (IsKeyPressed(KEY_ESCAPE)) { fuoco.clear(); mio = false; }
    }
    bool sopra = MouseSu(r);
    DrawRectangleRec(r, mio ? Hex(0x0a0c1e) : Hex(0x10132c));
    DrawRectangleLinesEx(r, std::max(1.0f, scala), mio ? Accento() : sopra ? Hex(0x5a64b0) : Bordo());
    const std::string& mostra = mio ? bozza : testo;
    BeginScissorMode((int)r.x + 2, (int)r.y, (int)r.width - 4, (int)r.height);
    int dim = TP();
    float w = (float)Larghezza(mostra, dim);
    float x = r.x + 6 * scala;
    if (w > r.width - 14 * scala) x -= w - (r.width - 14 * scala);   // si vede la fine mentre si scrive
    Testo(mostra, x, r.y + (r.height - dim) / 2, TestoChiaro(), dim);
    if (mio && std::fmod(lampeggio, 1.0f) < 0.6f) DrawRectangle((int)(x + w + 2), (int)(r.y + 4 * scala), (int)std::max(1.0f, 2 * scala), (int)(r.height - 8 * scala), Accento());
    EndScissorMode();
    return confermato;
}

bool Numero(const std::string& id, Rectangle r, const std::string& etichetta, float& v, float minimo, float massimo,
            float passo, const char* formato) {
    int dim = TP();
    float le = etichetta.empty() ? 0 : r.width * 0.45f;
    if (le > 0) Testo(etichetta, r.x, r.y + (r.height - dim) / 2, TestoSpento(), dim);
    float b = r.height;
    Rectangle meno = { r.x + le, r.y, b, r.height }, piu = { r.x + r.width - b, r.y, b, r.height };
    Rectangle valore = { meno.x + b + 2, r.y, r.width - le - 2 * b - 4, r.height };
    bool cambiato = false;
    float vecchio = v;
    if (Pulsante(meno, "-")) v -= passo;
    if (Pulsante(piu, "+")) v += passo;
    std::string testo = TextFormat(formato, v);
    if (Campo(id, valore, testo, 16)) v = (float)std::atof(testo.c_str());
    v = Limita(v, minimo, massimo);
    if (v != vecchio) cambiato = true;
    return cambiato;
}

bool NumeroIntero(const std::string& id, Rectangle r, const std::string& etichetta, int& v, int minimo, int massimo) {
    float f = (float)v;
    bool c = Numero(id, r, etichetta, f, (float)minimo, (float)massimo, 1, "%.0f");
    v = (int)std::round(f);
    return c;
}

bool Interruttore(Rectangle r, const std::string& etichetta, bool& v) {
    int dim = TP();
    float h = r.height * 0.7f, w = h * 1.8f;
    Rectangle tasto = { r.x + r.width - w, r.y + (r.height - h) / 2, w, h };
    Testo(etichetta, r.x, r.y + (r.height - dim) / 2, TestoSpento(), dim);
    bool sopra = MouseSu(r);
    DrawRectangleRounded(tasto, 1, 8, v ? Hex(0x3aa860) : Hex(0x2a2f50));
    DrawRectangleRoundedLinesEx(tasto, 1, 8, std::max(1.0f, scala), sopra ? Accento() : Bordo());
    DrawCircleV({ v ? tasto.x + w - h / 2 : tasto.x + h / 2, tasto.y + h / 2 }, h * 0.38f, WHITE);
    if (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { v = !v; gAudio.Suona(Effetto::Menu, 0.35f); return true; }
    return false;
}

bool Scelta(Rectangle r, const std::string& etichetta, int& v, const std::vector<std::string>& voci) {
    if (voci.empty()) return false;
    int dim = TP();
    float le = etichetta.empty() ? 0 : r.width * 0.45f;
    if (le > 0) Testo(etichetta, r.x, r.y + (r.height - dim) / 2, TestoSpento(), dim);
    float b = r.height;
    Rectangle sx = { r.x + le, r.y, b, r.height }, dx = { r.x + r.width - b, r.y, b, r.height };
    Rectangle centro = { sx.x + b + 2, r.y, r.width - le - 2 * b - 4, r.height };
    int n = (int)voci.size(), vecchio = v;
    if (Pulsante(sx, "<")) v = (v + n - 1) % n;
    if (Pulsante(dx, ">")) v = (v + 1) % n;
    if (MouseSu(centro)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) v = (v + 1) % n;
    }
    v = (v % n + n) % n;
    DrawRectangleRec(centro, Hex(0x10132c));
    DrawRectangleLinesEx(centro, std::max(1.0f, scala), MouseSu(centro) ? Hex(0x5a64b0) : Bordo());
    BeginScissorMode((int)centro.x, (int)centro.y, (int)centro.width, (int)centro.height);
    TestoCentrato(voci[v], centro, TestoChiaro(), dim);
    EndScissorMode();
    return v != vecchio;
}

bool Colore(const std::string& id, Rectangle r, const std::string& etichetta, Color& c) {
    int dim = TP();
    float le = etichetta.empty() ? 0 : r.width * 0.45f;
    if (le > 0) Testo(etichetta, r.x, r.y + (r.height - dim) / 2, TestoSpento(), dim);
    Rectangle campione = { r.x + le, r.y, r.height * 1.4f, r.height };
    DrawRectangleRec(campione, c);
    DrawRectangleLinesEx(campione, std::max(1.0f, scala), Bordo());
    std::string testo = ColoreInTesto(c);
    Rectangle campo = { campione.x + campione.width + 4, r.y, r.width - le - campione.width - 4, r.height };
    if (Campo(id, campo, testo, 7)) return TestoInColore(testo, c);
    return false;
}

}
