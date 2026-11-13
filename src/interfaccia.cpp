#include "interfaccia.h"
#include "audio.h"
#include <fstream>
#include <sstream>

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
    }
}

void SalvaOpzioniAudio(const std::string& file) {
    if (file.empty()) return;
    std::ofstream f(file);
    f << "muto " << (gAudio.muto ? 1 : 0) << "\n"
      << "musica " << gAudio.volumeMusica << "\n"
      << "effetti " << gAudio.volumeEffetti << "\n";
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
