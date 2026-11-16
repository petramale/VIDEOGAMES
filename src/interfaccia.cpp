#include "interfaccia.h"
#include "audio.h"
#include <fstream>
#include <sstream>

bool gMusicaStudio = false;

void CaricaOpzioniAudio(const std::string& file) {
    std::ifstream f(file);
    std::string riga;
    while (std::getline(f, riga)) {
        std::istringstream r(riga);
        std::string chiave;
        float v = 0;
        if (!(r >> chiave >> v)) continue;
        if (chiave == "muto") gAudio.muto = v != 0;
        else if (chiave == "musica") gAudio.volumeMusica = Limita(v, 0, 1);
        else if (chiave == "effetti") gAudio.volumeEffetti = Limita(v, 0, 1);
        else if (chiave == "musica_studio") gMusicaStudio = v != 0;
        else if (chiave == "musica_gioco") gMusicaGioco = v != 0;
    }
}

void SalvaOpzioniAudio(const std::string& file) {
    if (file.empty()) return;
    std::ofstream f(file);
    f << "muto " << (gAudio.muto ? 1 : 0) << "\n"
      << "musica " << gAudio.volumeMusica << "\n"
      << "effetti " << gAudio.volumeEffetti << "\n"
      << "musica_studio " << (gMusicaStudio ? 1 : 0) << "\n"
      << "musica_gioco " << (gMusicaGioco ? 1 : 0) << "\n";
}

static void DisegnaAltoparlante(Rectangle r, bool spento, bool sopra) {
    float s = r.width;
    DrawRectangleRounded(r, 0.3f, 6, sopra ? Hex(0x2a2f66, 230) : Hex(0x101430, 190));
    DrawRectangleRoundedLinesEx(r, 0.3f, 6, std::max(1.0f, s * 0.04f), sopra ? Hex(0xffd23f) : Hex(0x8890c8));
    Color c = spento ? Hex(0xff6b6b) : WHITE;
    float cx = r.x + s * 0.36f, cy = r.y + s * 0.5f;
    // Corpo e cono dell'altoparlante.
    DrawRectangleRec({ r.x + s * 0.18f, cy - s * 0.1f, s * 0.14f, s * 0.2f }, c);
    DrawTriangle({ cx - s * 0.06f, cy - s * 0.1f }, { cx - s * 0.06f, cy + s * 0.1f }, { cx + s * 0.14f, cy + s * 0.26f }, c);
    DrawTriangle({ cx - s * 0.06f, cy - s * 0.1f }, { cx + s * 0.14f, cy + s * 0.26f }, { cx + s * 0.14f, cy - s * 0.26f }, c);
    float spessore = std::max(1.5f, s * 0.06f);
    if (spento) {
        float x0 = r.x + s * 0.6f, x1 = r.x + s * 0.82f;
        DrawLineEx({ x0, cy - s * 0.11f }, { x1, cy + s * 0.11f }, spessore, c);
        DrawLineEx({ x0, cy + s * 0.11f }, { x1, cy - s * 0.11f }, spessore, c);
    } else {
        Vector2 o = { cx + s * 0.12f, cy };
        DrawRing(o, s * 0.16f, s * 0.16f + spessore, -45, 45, 12, c);
        DrawRing(o, s * 0.29f, s * 0.29f + spessore, -45, 45, 16, c);
    }
}

Rectangle AngoloAudio(Rectangle area, bool inBasso) {
    float s = Limita(area.height * 0.06f, 24, 48);
    float m = s * 0.2f;
    return { area.x + area.width - s - m, inBasso ? area.y + area.height - s - m : area.y + m, s, s };
}

bool PulsanteAudio(Rectangle r, const std::string& fileOpzioni) {
    float s = r.width;
    Vector2 m = GetMousePosition();
    bool sopra = CheckCollisionPointRec(m, r);
    bool cambia = IsKeyPressed(KEY_F8) || (sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    if (cambia) {
        gAudio.muto = !gAudio.muto;
        if (!gAudio.muto) gAudio.Suona(Effetto::Conferma, 0.6f);
        SalvaOpzioniAudio(fileOpzioni);
    }
    DisegnaAltoparlante(r, gAudio.muto, sopra);
    if (sopra) {
        const char* t = gAudio.muto ? "Audio spento (F8)" : "Audio acceso (F8)";
        int d = (int)Limita(s * 0.4f, 12, 20);
        int w = MeasureText(t, d);
        float ty = r.y + s + 6 + d + 8 > GetScreenHeight() ? r.y - d - 14 : r.y + s + 6;
        DrawRectangle((int)(r.x + s - w - 10), (int)ty, w + 10, d + 8, Hex(0x101430, 220));
        DrawText(t, (int)(r.x + s - w - 5), (int)ty + 4, d, WHITE);
    }
    return cambia;
}

Rectangle PostoAudioLibero(Rectangle area) {
    float s = Limita(area.height * 0.06f, 24, 48);
    float destra = GetScreenWidth() - (area.x + area.width);
    if (destra >= 30) { float t = std::min(s, destra - 6); return { area.x + area.width + (destra - t) / 2, area.y + 6, t, t }; }
    if (area.y >= s + 8) return { area.x + area.width - s - 6, (area.y - s) / 2, s, s };
    return AngoloAudio(area, false);
}

// Pulsante quadrato con un'icona: 0 = nota (musica), 1 = onde (effetti sonori). Barrato se spento.
static bool PulsanteIcona(Rectangle r, int icona, bool acceso, const char* suggerimento) {
    float s = r.width;
    bool sopra = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, 0.3f, 6, sopra ? Hex(0x2a2f66, 230) : Hex(0x101430, 190));
    DrawRectangleRoundedLinesEx(r, 0.3f, 6, std::max(1.0f, s * 0.04f), sopra ? Hex(0xffd23f) : Hex(0x8890c8));
    Color c = acceso ? WHITE : Hex(0x8890c8);
    float sp = std::max(1.5f, s * 0.07f);
    if (icona == 0) {
        DrawEllipse((int)(r.x + s * 0.4f), (int)(r.y + s * 0.68f), s * 0.13f, s * 0.1f, c);
        DrawLineEx({ r.x + s * 0.52f, r.y + s * 0.68f }, { r.x + s * 0.52f, r.y + s * 0.22f }, sp, c);
        DrawLineEx({ r.x + s * 0.52f, r.y + s * 0.22f }, { r.x + s * 0.72f, r.y + s * 0.36f }, sp, c);
    } else {
        // Una piccola esplosione: il simbolo degli effetti sonori.
        Vector2 o = { r.x + s * 0.5f, r.y + s * 0.5f };
        for (int k = 0; k < 8; k++) {
            float ang = k * PI / 4, l0 = s * 0.1f, l1 = k % 2 ? s * 0.22f : s * 0.32f;
            DrawLineEx({ o.x + std::cos(ang) * l0, o.y + std::sin(ang) * l0 }, { o.x + std::cos(ang) * l1, o.y + std::sin(ang) * l1 }, sp, c);
        }
    }
    if (!acceso) DrawLineEx({ r.x + s * 0.2f, r.y + s * 0.8f }, { r.x + s * 0.8f, r.y + s * 0.2f }, sp, Hex(0xff6b6b));
    if (sopra) {
        int d = (int)Limita(s * 0.4f, 12, 20);
        int w = MeasureText(suggerimento, d);
        float ty = r.y + s + 6 + d + 8 > GetScreenHeight() ? r.y - d - 14 : r.y + s + 6;
        float tx = std::max(4.0f, r.x + s - w - 10);
        DrawRectangle((int)tx, (int)ty, w + 10, d + 8, Hex(0x101430, 220));
        DrawText(suggerimento, (int)tx + 5, (int)ty + 4, d, WHITE);
    }
    return sopra && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool PulsanteMusica(Rectangle r, const std::string& fileOpzioni) {
    bool cambia = PulsanteIcona(r, 0, gMusicaStudio, gMusicaStudio ? "Musica dello studio accesa (F7)" : "Musica dello studio spenta (F7)") || IsKeyPressed(KEY_F7);
    if (cambia) { gMusicaStudio = !gMusicaStudio; SalvaOpzioniAudio(fileOpzioni); }
    return cambia;
}

bool gMusicaGioco = true;

void CambiaMusicaGioco(const std::string& fileOpzioni) {
    gMusicaGioco = !gMusicaGioco;
    SalvaOpzioniAudio(fileOpzioni);
    AvvisoAudio(gMusicaGioco ? "Musica accesa (F7)" : "Musica spenta (F7)");
}

void CambiaEffetti(const std::string& fileOpzioni) {
    gAudio.volumeEffetti = gAudio.volumeEffetti > 0 ? 0.0f : 1.0f;
    SalvaOpzioniAudio(fileOpzioni);
    if (gAudio.volumeEffetti > 0) gAudio.Suona(Effetto::Conferma, 0.6f);
    AvvisoAudio(gAudio.volumeEffetti > 0 ? "Effetti sonori accesi (F6)" : "Effetti sonori spenti (F6)");
}

static std::string avvisoAudio;
static double tAvvisoAudio = -10;
void AvvisoAudio(const std::string& testo) { avvisoAudio = testo; tAvvisoAudio = GetTime(); }

void PulsantiAudioGioco(Rectangle altoparlante, const std::string& fileOpzioni) {
    Rectangle a = altoparlante;
    Rectangle rm = { a.x - a.width - 6, a.y, a.width, a.height }, re = { rm.x - a.width - 6, a.y, a.width, a.height };
    if (PulsanteIcona(rm, 0, gMusicaGioco, gMusicaGioco ? "Musica accesa (F7)" : "Musica spenta (F7)") || IsKeyPressed(KEY_F7)) CambiaMusicaGioco(fileOpzioni);
    bool eff = gAudio.volumeEffetti > 0;
    if (PulsanteIcona(re, 1, eff, eff ? "Effetti sonori accesi (F6)" : "Effetti sonori spenti (F6)") || IsKeyPressed(KEY_F6)) CambiaEffetti(fileOpzioni);
    // Avviso per due secondi sotto i pulsanti.
    double t = GetTime() - tAvvisoAudio;
    if (t < 2.0 && !avvisoAudio.empty()) {
        float k = (float)Limita((float)(2.0 - t) * 2, 0, 1);
        int d = (int)Limita(a.width * 0.45f, 13, 22);
        int w = MeasureText(avvisoAudio.c_str(), d);
        float x = a.x + a.width - w - 14, y = a.y + a.height + 8;
        if (y + d + 10 > GetScreenHeight()) y = a.y - d - 18;   // pulsanti in basso: avviso sopra
        DrawRectangleRounded({ x, y, (float)w + 14, (float)d + 10 }, 0.4f, 6, Hex(0x101430, (unsigned char)(230 * k)));
        DrawText(avvisoAudio.c_str(), (int)x + 7, (int)y + 5, d, ConAlfa(WHITE, k));
    }
}
