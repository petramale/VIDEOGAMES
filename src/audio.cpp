#include "audio.h"
#include "progetto_gioco.h"
#include <cstring>

Audio gAudio;

namespace {

constexpr int SR = 44100;

enum class Onda { Quadra, Triangolo, Dente, Seno, Rumore };

struct Rng { unsigned int s = 12345; float Prossimo() { s = s * 1664525u + 1013904223u; return (s >> 8) / 8388607.5f - 1.0f; } };

// Oscillatore con stato: la fase e il rumore "campiona e tieni" tipico delle console a 8 bit.
struct Oscillatore {
    float fase = 0, rumore = 0;
    Rng rng;
    float Passo(Onda o, float freq, float duty) {
        fase += freq / SR;
        if (fase >= 1) { fase -= std::floor(fase); rumore = rng.Prossimo(); }
        switch (o) {
            case Onda::Quadra: return fase < duty ? 1.0f : -1.0f;
            case Onda::Triangolo: { float t = fase < 0.5f ? fase * 4 - 1 : 3 - fase * 4; return std::round(t * 7) / 7; }
            case Onda::Dente: return fase * 2 - 1;
            case Onda::Seno: return std::sin(fase * 2 * PI);
            case Onda::Rumore: return rumore;
        }
        return 0;
    }
};

float Midi(int n) { return 440.0f * std::pow(2.0f, (n - 69) / 12.0f); }

// "A4", "C#5", "R" (pausa) -> numero MIDI (-1 per la pausa)
int NotaDaNome(const std::string& s) {
    if (s.empty() || s[0] == 'R') return -1;
    static const int base[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    int n = base[s[0] - 'A'];
    size_t i = 1;
    if (i < s.size() && s[i] == '#') { n++; i++; }
    if (i < s.size() && s[i] == 'b') { n--; i++; }
    int ottava = std::stoi(s.substr(i));
    return 12 * (ottava + 1) + n;
}

// --- Effetti sonori -------------------------------------------------------

struct Segmento {
    Onda onda; float f0, f1, durata, v0, v1, duty = 0.5f, vibratoHz = 0, vibrato = 0;
};
using Traccia = std::vector<Segmento>;

std::vector<float> RenderEffetto(const std::vector<Traccia>& tracce) {
    float totale = 0;
    for (auto& t : tracce) { float d = 0; for (auto& s : t) d += s.durata; totale = std::max(totale, d); }
    std::vector<float> out((size_t)(totale * SR) + 1, 0.0f);
    for (auto& t : tracce) {
        Oscillatore osc;
        size_t i = 0;
        for (auto& s : t) {
            int n = (int)(s.durata * SR);
            for (int k = 0; k < n && i < out.size(); k++, i++) {
                float p = (float)k / n;
                float f = s.f0 * std::pow(s.f1 / s.f0, p); // glissando esponenziale, suona piu' naturale
                f *= 1 + s.vibrato * std::sin(2 * PI * s.vibratoHz * k / SR);
                float v = Lerp1(s.v0, s.v1, p);
                float bordo = Limita(k / 40.0f, 0, 1) * Limita((n - k) / 40.0f, 0, 1); // niente "click"
                out[i] += osc.Passo(s.onda, f, s.duty) * v * bordo;
            }
        }
    }
    return out;
}

std::vector<Traccia> DefinisciEffetto(Effetto e) {
    using O = Onda;
    switch (e) {
        case Effetto::Salto:
            return { { { O::Quadra, 260, 680, 0.17f, 0.45f, 0.0f, 0.25f } } };
        case Effetto::Moneta:
            return { { { O::Quadra, 988, 988, 0.06f, 0.4f, 0.4f }, { O::Quadra, 1319, 1319, 0.28f, 0.4f, 0.0f } } };
        case Effetto::Gemma:
            return { { { O::Triangolo, 1047, 1047, 0.05f, 0.5f, 0.5f }, { O::Triangolo, 1319, 1319, 0.05f, 0.5f, 0.5f },
                       { O::Triangolo, 1568, 1568, 0.05f, 0.5f, 0.5f }, { O::Triangolo, 2093, 2093, 0.45f, 0.5f, 0.0f, 0.5f, 9, 0.01f } },
                     { { O::Quadra, 2093, 2093, 0.15f, 0.0f, 0.0f }, { O::Quadra, 4186, 4186, 0.45f, 0.12f, 0.0f, 0.125f, 12, 0.01f } } };
        case Effetto::Colpo:
            return { { { O::Rumore, 900, 120, 0.28f, 0.5f, 0.0f } }, { { O::Quadra, 220, 55, 0.25f, 0.35f, 0.0f } } };
        case Effetto::Molla:
            return { { { O::Triangolo, 140, 900, 0.38f, 0.6f, 0.0f, 0.5f, 28, 0.12f } } };
        case Effetto::Forziere:
            return { { { O::Quadra, 784, 784, 0.07f, 0.35f, 0.35f }, { O::Quadra, 988, 988, 0.07f, 0.35f, 0.35f },
                       { O::Quadra, 1175, 1175, 0.07f, 0.35f, 0.35f }, { O::Quadra, 1568, 1568, 0.4f, 0.35f, 0.0f, 0.5f, 7, 0.008f } },
                     { { O::Triangolo, 196, 196, 0.21f, 0.5f, 0.5f }, { O::Triangolo, 392, 392, 0.4f, 0.5f, 0.0f } } };
        case Effetto::Esplosione:
            return { { { O::Rumore, 1500, 40, 0.75f, 0.7f, 0.0f } }, { { O::Seno, 120, 30, 0.5f, 0.6f, 0.0f } } };
        case Effetto::Vittoria: {
            Traccia t, b;
            int melodia[] = { 72, 76, 79, 84, 79, 84 };
            float dur[] = { 0.11f, 0.11f, 0.11f, 0.22f, 0.11f, 0.7f };
            for (int i = 0; i < 6; i++) t.push_back({ O::Quadra, Midi(melodia[i]), Midi(melodia[i]), dur[i], 0.35f, i == 5 ? 0.0f : 0.3f, 0.25f, 6, i == 5 ? 0.01f : 0 });
            int basso[] = { 48, 52, 55, 60 };
            for (int i = 0; i < 4; i++) b.push_back({ O::Triangolo, Midi(basso[i]), Midi(basso[i]), i == 3 ? 1.03f : 0.11f, 0.6f, i == 3 ? 0.0f : 0.5f });
            return { t, b };
        }
        case Effetto::Passo:
            return { { { O::Rumore, 300, 200, 0.035f, 0.18f, 0.0f } } };
        case Effetto::Menu:
            return { { { O::Quadra, 660, 660, 0.045f, 0.25f, 0.0f, 0.5f } } };
        case Effetto::Conferma:
            return { { { O::Quadra, 880, 880, 0.05f, 0.3f, 0.3f, 0.25f }, { O::Quadra, 1320, 1320, 0.12f, 0.3f, 0.0f, 0.25f } } };
        default: return {};
    }
}

// Converte in PCM a 16 bit; la saturazione morbida (tanh) evita distorsioni dure.
std::vector<short> InPcm(const std::vector<float>& campioni, float guadagno) {
    std::vector<short> pcm(campioni.size());
    for (size_t i = 0; i < campioni.size(); i++) pcm[i] = (short)(std::tanh(campioni[i] * guadagno) * 30000);
    return pcm;
}

Wave ComeWave(std::vector<short>& pcm) {
    Wave w{};
    w.frameCount = (unsigned int)pcm.size();
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm.data();
    return w;
}

} // namespace

const char* Audio::NomeEffetto(Effetto e) {
    static const char* nomi[] = { "Salto", "Moneta", "Gemma", "Colpo", "Molla", "Forziere", "Esplosione", "Vittoria", "Passo", "Menu", "Conferma" };
    return nomi[(int)e];
}

// --- Colonna sonora -------------------------------------------------------

void Audio::ComponiBrano() {
    const float battito = 60.0f / bpm, ottavo = battito / 2, sedicesimo = battito / 4, battuta = battito * 4;
    const int battute = 16;
    durata = battute * battuta;
    size_t n = (size_t)(durata * SR);
    brano.assign(n, 0.0f);
    std::vector<float> melodia(n, 0.0f);
    note.clear();

    // Progressione epica in La minore: Am F C G | Am F E E (due volte, la seconda piu' ricca).
    struct Accordo { int radice; int terza; int quinta; };
    const Accordo accordi[8] = { { 57, 60, 64 }, { 53, 57, 60 }, { 48, 52, 55 }, { 55, 59, 62 },
                                 { 57, 60, 64 }, { 53, 57, 60 }, { 52, 56, 59 }, { 52, 56, 59 } };
    // Melodia: nota:durata in ottavi, 8 ottavi per battuta.
    const char* tema[8] = {
        "E5:2 A5:2 G5:1 E5:1 C5:2", "D5:1 C5:1 A4:2 C5:2 F5:2", "E5:3 G5:1 E5:2 D5:2", "D5:1 E5:1 D5:2 B4:2 G4:2",
        "A5:2 G5:1 A5:1 C6:2 B5:1 A5:1", "A5:2 F5:2 C5:2 F5:1 G5:1", "G#5:2 E5:2 B4:2 G#4:2", "E5:6 R:2" };

    auto Inviluppo = [](float t, float d, float a, float dec, float sus, float rel) {
        if (t < a) return t / a;
        if (t < a + dec) return 1 - (1 - sus) * (t - a) / dec;
        if (t < d) return sus;
        return std::max(0.0f, sus * (1 - (t - d) / rel));
    };

    auto Nota = [&](std::vector<float>& dest, float inizio, float dur, float freq, Onda onda, float duty, float vol,
                    float a, float dec, float sus, float rel, float vibrato) {
        Oscillatore osc;
        size_t s0 = (size_t)(inizio * SR);
        size_t len = (size_t)((dur + rel) * SR);
        for (size_t k = 0; k < len; k++) {
            float t = (float)k / SR;
            float vib = t > 0.12f ? vibrato * std::sin(2 * PI * 5.5f * t) : 0;
            float v = Inviluppo(t, dur, a, dec, sus, rel) * vol;
            dest[(s0 + k) % dest.size()] += osc.Passo(onda, freq * (1 + vib), duty) * v;
        }
    };

    for (int giro = 0; giro < 2; giro++) {
        for (int b = 0; b < 8; b++) {
            float t0 = (giro * 8 + b) * battuta;
            const Accordo& acc = accordi[b];

            // Melodia (con raddoppio all'ottava nel secondo giro).
            float t = t0;
            std::string riga = tema[b];
            size_t p = 0;
            while (p < riga.size()) {
                size_t sp = riga.find(' ', p);
                std::string tok = riga.substr(p, sp == std::string::npos ? std::string::npos : sp - p);
                p = sp == std::string::npos ? riga.size() : sp + 1;
                size_t due = tok.find(':');
                int m = NotaDaNome(tok.substr(0, due));
                float d = std::stoi(tok.substr(due + 1)) * ottavo;
                if (m >= 0) {
                    Nota(melodia, t, d * 0.92f, Midi(m), Onda::Quadra, 0.25f, 0.20f, 0.01f, 0.08f, 0.7f, 0.08f, 0.004f);
                    if (giro == 1) Nota(melodia, t, d * 0.92f, Midi(m + 12), Onda::Quadra, 0.125f, 0.07f, 0.01f, 0.08f, 0.6f, 0.08f, 0.004f);
                    note.push_back({ 0, t, d, m });
                }
                t += d;
            }

            // Basso: radice e ottava in ottavi, onda triangolare a 8 bit.
            for (int i = 0; i < 8; i++) {
                int m = acc.radice - 12 + ((i % 4 == 2) ? 12 : 0);
                Nota(brano, t0 + i * ottavo, ottavo * 0.8f, Midi(m), Onda::Triangolo, 0.5f, 0.42f, 0.005f, 0.05f, 0.8f, 0.03f, 0);
                note.push_back({ 1, t0 + i * ottavo, ottavo, m });
            }

            // Arpeggio veloce in sedicesimi.
            int arp[4] = { acc.radice + 12, acc.terza + 12, acc.quinta + 12, acc.terza + 12 };
            for (int i = 0; i < 16; i++) {
                int m = arp[i % 4] + (giro == 1 && i >= 8 ? 12 : 0);
                Nota(brano, t0 + i * sedicesimo, sedicesimo * 0.7f, Midi(m), Onda::Quadra, 0.125f, 0.05f, 0.002f, 0.03f, 0.5f, 0.02f, 0);
                if (i % 2 == 0) note.push_back({ 2, t0 + i * sedicesimo, sedicesimo, m });
            }

            // Pad morbido che riempie l'armonia.
            for (int m : { acc.radice, acc.terza, acc.quinta })
                Nota(brano, t0, battuta * 0.95f, Midi(m), Onda::Triangolo, 0.5f, 0.035f, 0.3f, 0.2f, 0.8f, 0.3f, 0.003f);

            // Batteria: cassa, rullante, charleston.
            for (int i = 0; i < 8; i++) {
                float tt = t0 + i * ottavo;
                size_t s0 = (size_t)(tt * SR);
                bool cassa = (i == 0 || i == 4) || (giro == 1 && (i == 5 || i == 7) && b % 2 == 1);
                bool rullante = (i == 2 || i == 6);
                if (cassa) {
                    float fase = 0;
                    for (int k = 0; k < SR / 6; k++) {
                        float tk = (float)k / SR;
                        fase += (45 + 120 * std::exp(-tk * 30)) / SR;
                        brano[(s0 + k) % n] += std::sin(fase * 2 * PI) * std::exp(-tk * 14) * 0.75f;
                    }
                    note.push_back({ 3, tt, ottavo, 36 });
                }
                if (rullante) {
                    Rng r;
                    for (int k = 0; k < SR / 5; k++) {
                        float tk = (float)k / SR;
                        float rumore = r.Prossimo() * std::exp(-tk * 18) * 0.32f;
                        float corpo = std::sin(2 * PI * 185 * tk) * std::exp(-tk * 30) * 0.3f;
                        brano[(s0 + k) % n] += rumore + corpo;
                    }
                    note.push_back({ 3, tt, ottavo, 38 });
                }
                // Charleston: chiuso, aperto sul controtempo.
                Rng r; r.s = 99 + i;
                float lungo = (i % 2 == 1) ? 0.09f : 0.03f;
                float prec = 0;
                for (int k = 0; k < (int)(lungo * SR); k++) {
                    float tk = (float)k / SR;
                    float x = r.Prossimo();
                    float alto = x - prec; prec = x; // filtro passa-alto: suono metallico
                    brano[(s0 + k) % n] += alto * std::exp(-tk * (i % 2 ? 30.0f : 90.0f)) * 0.06f;
                }
            }
            if (giro == 1 && b == 0) { // piatto all'inizio del secondo giro
                Rng r; float prec = 0;
                size_t s0 = (size_t)(t0 * SR);
                for (int k = 0; k < SR * 2; k++) {
                    float x = r.Prossimo(), alto = x - prec; prec = x;
                    brano[(s0 + k) % n] += alto * std::exp(-(float)k / SR * 2.2f) * 0.12f;
                }
            }
        }
    }

    // Eco sulla melodia (3/8 di battito), che si richiude in loop senza stacchi.
    size_t ritardo = (size_t)(battito * 0.75f * SR);
    for (size_t i = 0; i < n; i++) {
        brano[i] += melodia[i];
        brano[(i + ritardo) % n] += melodia[i] * 0.32f;
        brano[(i + 2 * ritardo) % n] += melodia[i] * 0.12f;
    }
    for (auto& c : brano) c = std::tanh(c * 1.1f) * 0.9f; // saturazione morbida, niente distorsione dura
}

const std::vector<const char*>& Audio::Brani() {
    static const std::vector<const char*> nomi = {
        "La valle delle gemme", "Caverna misteriosa", "Corsa nel deserto", "Ghiaccio di cristallo", "Battaglia col boss",
        "Viaggio spaziale", "Notte dei fantasmi", "Allegra avventura", "Enigma", "Vittoria finale" };
    return nomi;
}

// Compositore automatico: dato uno stile (tempo, scala, accordi, timbri, ritmo) scrive
// melodia, basso, arpeggi, pad e batteria. Lo stesso stile da' sempre lo stesso brano.
void Audio::ComponiStile(int n) {
    struct Stile { float bpm; int tonica; int scala[7]; int giro[8]; Onda guida; float duty; int batteria; bool arpeggio; float eco; unsigned seme; };
    static const Stile stili[] = {
        { 132, 57, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 5, 2, 6, 0, 5, 4, 4 }, Onda::Quadra, 0.25f, 1, true, 0.32f, 1 },
        { 96, 50, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 0, 5, 5, 3, 3, 4, 4 }, Onda::Triangolo, 0.5f, 0, true, 0.45f, 7 },        // caverna
        { 150, 52, { 0, 1, 4, 5, 7, 8, 10 }, { 0, 1, 0, 1, 5, 4, 1, 0 }, Onda::Quadra, 0.125f, 2, true, 0.2f, 11 },      // deserto
        { 110, 60, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 5, 3, 4, 0, 5, 1, 4 }, Onda::Seno, 0.5f, 1, true, 0.4f, 23 },           // ghiaccio
        { 160, 48, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 0, 5, 4, 0, 0, 5, 4 }, Onda::Dente, 0.5f, 2, true, 0.15f, 31 },         // boss
        { 124, 53, { 0, 2, 4, 6, 7, 9, 11 }, { 0, 1, 0, 1, 5, 4, 3, 4 }, Onda::Dente, 0.5f, 1, true, 0.4f, 41 },          // spazio
        { 90, 57, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 3, 0, 4, 5, 3, 4, 4 }, Onda::Triangolo, 0.5f, 0, false, 0.5f, 53 },      // fantasmi
        { 140, 55, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 3, 4, 0, 5, 3, 4, 4 }, Onda::Quadra, 0.25f, 1, true, 0.25f, 61 },       // allegra
        { 100, 52, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 5, 3, 6, 0, 5, 3, 4 }, Onda::Seno, 0.5f, 0, true, 0.35f, 71 },          // enigma
        { 120, 50, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 4, 5, 3, 0, 4, 3, 4 }, Onda::Quadra, 0.5f, 1, true, 0.3f, 83 },         // vittoria
    };
    const Stile& st = stili[n];
    bpm = st.bpm;
    const float battito = 60.0f / bpm, ottavo = battito / 2, battuta = battito * 4;
    const int battute = 16;
    durata = battute * battuta;
    size_t len = (size_t)(durata * SR);
    brano.assign(len, 0.0f);
    std::vector<float> melodia(len, 0.0f);
    note.clear();
    Rng rng; rng.s = st.seme * 2654435761u + 17;
    auto Dado = [&](int k) { return (int)((rng.Prossimo() * 0.5f + 0.5f) * k * 0.9999f); };
    // Grado della scala -> nota MIDI (anche oltre l'ottava).
    auto Grado = [&](int g, int base) { int o = (int)std::floor(g / 7.0f); int r = g - o * 7; return base + st.scala[r] + 12 * o; };

    auto Inviluppo = [](float t, float d, float a, float dec, float sus, float rel) {
        if (t < a) return t / a;
        if (t < a + dec) return 1 - (1 - sus) * (t - a) / dec;
        if (t < d) return sus;
        return std::max(0.0f, sus * (1 - (t - d) / rel));
    };
    auto Nota = [&](std::vector<float>& dest, float inizio, float dur, float freq, Onda onda, float duty, float vol,
                    float a, float dec, float sus, float rel, float vibrato) {
        Oscillatore osc;
        size_t s0 = (size_t)(inizio * SR);
        size_t l = (size_t)((dur + rel) * SR);
        for (size_t k = 0; k < l; k++) {
            float t = (float)k / SR;
            float vib = t > 0.12f ? vibrato * std::sin(2 * PI * 5.5f * t) : 0;
            dest[(s0 + k) % dest.size()] += osc.Passo(onda, freq * (1 + vib), duty) * Inviluppo(t, dur, a, dec, sus, rel) * vol;
        }
    };
    // Ritmi della melodia in ottavi (somma 8): il compositore ne sceglie uno per battuta.
    static const std::vector<std::vector<int>> ritmi = { { 2, 2, 2, 2 }, { 3, 1, 2, 2 }, { 1, 1, 2, 1, 1, 2 }, { 2, 1, 1, 4 },
                                                         { 4, 2, 2 }, { 1, 1, 1, 1, 2, 2 }, { 3, 3, 2 }, { 2, 2, 4 } };
    int frase[4][6];      // gradi della frase (si ripete variata nella seconda meta')
    int ritmoFrase[4];
    int grado = 4;        // parte dalla quinta
    for (int b = 0; b < 4; b++) {
        ritmoFrase[b] = Dado((int)ritmi.size());
        for (int k = 0; k < 6; k++) {
            int passo = Dado(5) - 2;            // movimento per gradi congiunti, ogni tanto un salto
            if (Dado(6) == 0) passo *= 2;
            grado = (int)Limita((float)(grado + passo), 0, 9);
            frase[b][k] = grado;
        }
    }

    for (int giro = 0; giro < 2; giro++) {
        for (int b = 0; b < 8; b++) {
            float t0 = (giro * 8 + b) * battuta;
            int radiceGrado = st.giro[b];
            int radice = Grado(radiceGrado, st.tonica), terza = Grado(radiceGrado + 2, st.tonica), quinta = Grado(radiceGrado + 4, st.tonica);
            // Melodia: la frase di 4 battute, ripetuta e trasposta sull'accordo; l'ultima battuta chiude sulla tonica.
            const std::vector<int>& ritmo = ritmi[ritmoFrase[b % 4]];
            float t = t0;
            bool ultima = b == 7;
            for (size_t k = 0; k < (ultima ? 1 : ritmo.size()); k++) {
                int g = frase[b % 4][k % 6] + (b >= 4 ? (k % 2 ? 1 : 0) : 0);
                // sui tempi forti si appoggia su una nota dell'accordo
                if (k == 0) { int vicino = radiceGrado + ((g - radiceGrado) / 2) * 2; g = vicino; }
                int m = ultima ? Grado(7, st.tonica) : Grado(g, st.tonica + 12);
                float d = ultima ? battuta * 0.75f : ritmo[k] * ottavo;
                if (!(Dado(9) == 0 && k > 0)) { // qualche pausa per respirare
                    Nota(melodia, t, d * 0.9f, Midi(m), st.guida, st.duty, st.guida == Onda::Dente ? 0.12f : 0.19f, 0.01f, 0.08f, 0.7f, 0.08f, 0.004f);
                    if (giro == 1) Nota(melodia, t, d * 0.9f, Midi(m + 12), Onda::Quadra, 0.125f, 0.05f, 0.01f, 0.08f, 0.6f, 0.08f, 0.004f);
                    note.push_back({ 0, t, d, m });
                }
                t += d;
            }
            // Basso.
            int passi = st.batteria == 2 ? 8 : 4;
            for (int i = 0; i < passi; i++) {
                float dur = battuta / passi;
                int m = radice - 12 + ((st.batteria == 2 && i % 2) ? 12 : 0);
                Nota(brano, t0 + i * dur, dur * 0.8f, Midi(m), Onda::Triangolo, 0.5f, 0.42f, 0.005f, 0.05f, 0.8f, 0.03f, 0);
                note.push_back({ 1, t0 + i * dur, dur, m });
            }
            // Arpeggio.
            if (st.arpeggio) {
                int arp[4] = { radice + 12, terza + 12, quinta + 12, terza + 12 };
                int quanti = st.bpm > 120 ? 16 : 8;
                float passo = battuta / quanti;
                for (int i = 0; i < quanti; i++) {
                    int m = arp[i % 4] + (giro == 1 && i >= quanti / 2 ? 12 : 0);
                    Nota(brano, t0 + i * passo, passo * 0.7f, Midi(m), n == 3 ? Onda::Seno : Onda::Quadra, 0.125f, n == 3 ? 0.07f : 0.045f, 0.002f, 0.03f, 0.5f, 0.02f, 0);
                    if (i % 2 == 0) note.push_back({ 2, t0 + i * passo, passo, m });
                }
            }
            // Pad.
            for (int m : { radice, terza, quinta })
                Nota(brano, t0, battuta * 0.95f, Midi(m), Onda::Triangolo, 0.5f, 0.04f, 0.3f, 0.2f, 0.8f, 0.3f, 0.003f);
            // Batteria: 0 solo charleston leggero, 1 base, 2 incalzante.
            for (int i = 0; i < 8; i++) {
                float tt = t0 + i * ottavo;
                size_t s0 = (size_t)(tt * SR);
                bool cassa = st.batteria > 0 && ((i == 0 || i == 4) || (st.batteria == 2 && (i == 3 || i == 6)));
                bool rullante = st.batteria > 0 && (i == 2 || i == 6);
                if (cassa) {
                    float fase = 0;
                    for (int k = 0; k < SR / 6; k++) {
                        float tk = (float)k / SR;
                        fase += (45 + 120 * std::exp(-tk * 30)) / SR;
                        brano[(s0 + k) % len] += std::sin(fase * 2 * PI) * std::exp(-tk * 14) * 0.7f;
                    }
                    note.push_back({ 3, tt, ottavo, 36 });
                }
                if (rullante) {
                    Rng r;
                    for (int k = 0; k < SR / 5; k++) {
                        float tk = (float)k / SR;
                        brano[(s0 + k) % len] += r.Prossimo() * std::exp(-tk * 18) * 0.3f + std::sin(2 * PI * 185 * tk) * std::exp(-tk * 30) * 0.28f;
                    }
                    note.push_back({ 3, tt, ottavo, 38 });
                }
                Rng r; r.s = 99 + i;
                float prec = 0, lungo = (i % 2 == 1) ? 0.08f : 0.03f, vol = st.batteria == 0 ? 0.035f : 0.06f;
                for (int k = 0; k < (int)(lungo * SR); k++) {
                    float x = r.Prossimo(), alto = x - prec; prec = x;
                    brano[(s0 + k) % len] += alto * std::exp(-(float)k / SR * (i % 2 ? 30.0f : 90.0f)) * vol;
                }
            }
        }
    }
    size_t ritardo = (size_t)(battito * 0.75f * SR);
    for (size_t i = 0; i < len; i++) {
        brano[i] += melodia[i];
        brano[(i + ritardo) % len] += melodia[i] * st.eco;
        brano[(i + 2 * ritardo) % len] += melodia[i] * st.eco * 0.4f;
    }
    for (auto& c : brano) c = std::tanh(c * 1.1f) * 0.9f;
}

void Audio::ScegliBrano(int n) {
    if (!branoPersonale.empty()) { branoPersonale.clear(); RipristinaMusica(); branoAttuale = -2; }
    n = (int)Limita((float)n, 0, (float)Brani().size() - 1);
    if (n == branoAttuale && !brano.empty()) return;
    auto it = archivio.find(n);
    Composizione c;
    if (it != archivio.end()) c = it->second;
    else {
        // Si compone in variabili a parte: il brano che sta suonando non viene toccato.
        std::vector<float> vecchio; std::vector<EventoNota> vecchieNote; float vd = durata, vb = bpm;
        {
            std::lock_guard<std::mutex> g(blocco);
            vecchio.swap(brano); vecchieNote.swap(note);
            if (n == 0) ComponiBrano(); else ComponiStile(n);
            c = { brano, note, durata, bpm };
            brano.swap(vecchio); note.swap(vecchieNote); durata = vd; bpm = vb;
        }
        archivio[n] = c;
    }
    std::lock_guard<std::mutex> g(blocco);
    brano = c.brano; note = c.note; durata = c.durata; bpm = c.bpm;
    posizione.store(0);
    branoAttuale = n;
}

void Audio::Callback(void* buffer, unsigned int frame) {
    short* out = (short*)buffer;
    Audio& a = gAudio;
    std::unique_lock<std::mutex> g(a.blocco, std::try_to_lock);
    if (!g.owns_lock()) { std::memset(out, 0, frame * sizeof(short)); return; }
    unsigned long pos = a.posizione.load();
    size_t n = a.brano.size();
    float vol = (a.musicaAttiva && !a.musicaFile && !a.muto) ? a.volumeMusica : 0.0f;
    for (unsigned int i = 0; i < frame; i++) {
        out[i] = n ? (short)(a.brano[(pos + i) % n] * vol * 32000) : 0;
    }
    a.posizione.store(n ? (unsigned long)((pos + frame) % n) : 0);
}

void Audio::Prepara() {
    if (preparato) return;
    for (int i = 0; i < (int)Effetto::Totale; i++) campioniEffetti[i] = RenderEffetto(DefinisciEffetto((Effetto)i));
    ComponiBrano();
    archivio[0] = { brano, note, durata, bpm };
    preparato = true;
}

bool Audio::EsportaWav(const std::string& cartella) {
    Prepara();
    bool ok = true;
    for (int i = 0; i < (int)Effetto::Totale; i++) {
        std::vector<short> pcm = InPcm(campioniEffetti[i], 1.0f);
        ok &= ExportWave(ComeWave(pcm), TextFormat("%s/effetto_%s.wav", cartella.c_str(), NomeEffetto((Effetto)i)));
    }
    std::vector<short> pcm = InPcm(brano, 1.0f);
    ok &= ExportWave(ComeWave(pcm), TextFormat("%s/musica_la_valle_delle_gemme.wav", cartella.c_str()));
    return ok;
}

void Audio::Inizializza() {
    Prepara();
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    for (int i = 0; i < (int)Effetto::Totale; i++) {
        std::vector<short> pcm = InPcm(campioniEffetti[i], 1.0f);
        suoni[i] = LoadSoundFromWave(ComeWave(pcm)); // copia i dati: pcm puo' essere liberato
    }
    SetAudioStreamBufferSizeDefault(2048);
    flusso = LoadAudioStream(SR, 16, 1);
    SetAudioStreamCallback(flusso, Callback);
    PlayAudioStream(flusso);
    pronto = true;
}

void Audio::Chiudi() {
    if (!pronto) return;
    FermaTutto();
    StopAudioStream(flusso);
    UnloadAudioStream(flusso);
    for (auto& s : suoni) UnloadSound(s);
    for (auto& s : effettiPersonali) if (IsSoundValid(s.second)) UnloadSound(s.second);
    effettiPersonali.clear();
    CloseAudioDevice();
    pronto = false;
}

void Audio::Suona(Effetto e, float volume) {
    if (!pronto || muto) return;
    auto so = sostituzioni.find((int)e);
    if (so != sostituzioni.end()) {
        auto it = effettiPersonali.find(so->second);
        if (it != effettiPersonali.end()) { SetSoundVolume(it->second, volume * volumeEffetti); PlaySound(it->second); return; }
    }
    Sound& s = suoni[(int)e];
    SetSoundVolume(s, volume * volumeEffetti);
    SetSoundPitch(s, 1.0f + Casuale(-0.03f, 0.03f)); // piccola variazione: meno ripetitivo
    PlaySound(s);
}

void Audio::MusicaAttiva(bool attiva) { musicaAttiva = attiva; }

float Audio::TempoBrano() const { return (float)posizione.load() / SR; }

float Audio::Campione(long indice) const {
    if (brano.empty()) return 0;
    long n = (long)brano.size();
    return brano[((indice % n) + n) % n];
}

void Audio::Aggiorna(float dt) {
    if (musicaFile) {
        SetMusicVolume(musica, (musicaAttiva && !muto) ? volumeMusica : 0.0f);
        UpdateMusicStream(musica);
        return;
    }
    if (pronto || !musicaAttiva || brano.empty()) return;
    posizione.store((unsigned long)((posizione.load() + (unsigned long)(dt * SR)) % brano.size()));
}

bool Audio::CaricaMusica(const std::string& estensione, const std::string& dati) {
    if (!pronto) return false;
    RipristinaMusica();
    datiMusica = dati; // raylib legge il file un pezzo alla volta: la copia deve restare viva
    musica = LoadMusicStreamFromMemory(estensione.c_str(), (const unsigned char*)datiMusica.data(), (int)datiMusica.size());
    if (!IsMusicValid(musica)) { datiMusica.clear(); return false; }
    musica.looping = true;
    PlayMusicStream(musica);
    musicaFile = true;
    return true;
}

void Audio::RipristinaMusica() {
    if (!musicaFile) return;
    StopMusicStream(musica);
    UnloadMusicStream(musica);
    datiMusica.clear();
    musicaFile = false;
}

bool Audio::CaricaEffetto(Effetto e, const std::string& estensione, const std::string& dati) {
    if (!pronto) return false;
    Wave w = LoadWaveFromMemory(estensione.c_str(), (const unsigned char*)dati.data(), (int)dati.size());
    if (!IsWaveValid(w)) return false;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    if (!IsSoundValid(s)) return false;
    UnloadSound(suoni[(int)e]);
    suoni[(int)e] = s;
    effettoFile[(int)e] = true;
    return true;
}

void Audio::RipristinaEffetto(Effetto e) {
    if (!pronto || !effettoFile[(int)e]) return;
    UnloadSound(suoni[(int)e]);
    std::vector<short> pcm = InPcm(campioniEffetti[(int)e], 1.0f);
    suoni[(int)e] = LoadSoundFromWave(ComeWave(pcm));
    effettoFile[(int)e] = false;
}

void Audio::FermaTutto() {
    RipristinaMusica();
    for (int i = 0; i < (int)Effetto::Totale; i++) RipristinaEffetto((Effetto)i);
}

void Audio::ApplicaFile(const std::vector<std::pair<std::string, std::string>>& file) {
    FermaTutto();
    for (const auto& f : file) {
        size_t punto = f.first.rfind('.');
        if (punto == std::string::npos) continue;
        std::string nome = f.first.substr(0, punto), est = f.first.substr(punto);
        if (nome == "musica") CaricaMusica(est, f.second);
        else
            for (int i = 0; i < (int)Effetto::Totale; i++)
                if (nome == std::string("effetto_") + NomeEffetto((Effetto)i)) CaricaEffetto((Effetto)i, est, f.second);
    }
}

// --- Brani ed effetti della libreria del gioco ---------------------------------------------

std::vector<float> Audio::RenderBrano(const Brano& b) {
    float sedicesimo = 60.0f / std::max(30.0f, b.bpm) / 4;
    float durata = b.battute * 16 * sedicesimo;
    size_t len = (size_t)(durata * SR);
    std::vector<float> out(std::max<size_t>(len, 1), 0.0f);
    for (size_t ti = 0; ti < b.tracce.size(); ti++) {
        const TracciaBrano& t = b.tracce[ti];
        std::vector<float> traccia(out.size(), 0.0f);
        for (const NotaBrano& n : t.note) {
            size_t s0 = (size_t)(n.inizio * sedicesimo * SR);
            if (s0 >= out.size()) continue;
            float dur = n.durata * sedicesimo;
            Oscillatore osc;
            Rng rng; rng.s = (unsigned)(s0 * 31 + n.altezza);
            if (t.batteria || t.strumento == 4) {
                // batteria: 35-36 cassa, 37-40 rullante, 42-46 charleston, altro piatto
                int a = n.altezza;
                float lung = a <= 36 ? 0.28f : (a <= 40 ? 0.18f : (a <= 44 ? 0.05f : 0.35f));
                size_t l = (size_t)(lung * SR);
                float fase = 0;
                for (size_t k = 0; k < l; k++) {
                    float tk = (float)k / SR, v = 0;
                    if (a <= 36) { float f = 45 + 110 * std::exp(-tk * 28); fase += f / SR; v = std::sin(fase * 2 * PI) * std::exp(-tk * 12) * 1.1f; }
                    else if (a <= 40) { fase += 185.0f / SR; v = rng.Prossimo() * std::exp(-tk * 20) * 0.55f + std::sin(fase * 2 * PI) * std::exp(-tk * 30) * 0.4f; }
                    else if (a <= 44) v = rng.Prossimo() * std::exp(-tk * 70) * 0.35f;
                    else v = rng.Prossimo() * std::exp(-tk * 7) * 0.25f;
                    traccia[(s0 + k) % traccia.size()] += v;
                }
                continue;
            }
            static const Onda onde[] = { Onda::Quadra, Onda::Triangolo, Onda::Dente, Onda::Seno, Onda::Rumore };
            Onda o = onde[std::min(4, std::max(0, t.strumento))];
            float duty = t.strumento == 0 ? (ti == 0 ? 0.5f : 0.25f) : 0.5f;
            float f = Midi(n.altezza);
            float rel = 0.04f;
            size_t l = (size_t)((dur + rel) * SR);
            for (size_t k = 0; k < l; k++) {
                float tk = (float)k / SR;
                float env = tk < 0.006f ? tk / 0.006f : (tk < dur ? 1 - 0.3f * std::min(1.0f, (tk - 0.006f) / 0.15f) : 0.7f * std::max(0.0f, 1 - (tk - dur) / rel));
                float vib = tk > 0.15f ? 0.004f * std::sin(2 * PI * 5.5f * tk) : 0;
                traccia[(s0 + k) % traccia.size()] += osc.Passo(o, f * (1 + vib), duty) * env * (o == Onda::Dente ? 0.6f : 1.0f);
            }
        }
        float vol = t.volume * 0.5f;
        size_t eco = (size_t)(sedicesimo * 3 * SR);
        for (size_t i = 0; i < out.size(); i++) {
            out[i] += traccia[i] * vol;
            if (ti == 0 && !t.batteria) out[(i + eco) % out.size()] += traccia[i] * vol * 0.25f;
        }
    }
    for (auto& c : out) c = std::tanh(c * 1.2f) * 0.9f;
    return out;
}

std::vector<float> Audio::RenderEffettoLibreria(const EffettoSuono& e) {
    static const Onda onde[] = { Onda::Quadra, Onda::Triangolo, Onda::Dente, Onda::Seno, Onda::Rumore };
    Segmento s{ onde[std::min(4, std::max(0, e.onda))], std::max(20.0f, e.f0), std::max(20.0f, e.f1), std::max(0.01f, e.durata), e.volume, 0.0f, e.duty, e.vibratoHz, e.vibrato };
    std::vector<Traccia> tr = { { s } };
    if (e.rumore > 0.01f && s.onda != Onda::Rumore) {
        Segmento r = s; r.onda = Onda::Rumore; r.v0 = e.volume * e.rumore; r.f0 *= 4; r.f1 *= 4;
        tr.push_back({ r });
    }
    return ::RenderEffetto(tr);
}

Sound Audio::CaricaEffettoPersonale(const EffettoSuono& e) {
    Sound snd{};
    if (!e.file.empty() && !e.dati.empty()) {
        size_t p = e.file.find_last_of('.');
        std::string est = p == std::string::npos ? ".wav" : e.file.substr(p);
        for (auto& c : est) c = (char)std::tolower((unsigned char)c);
        Wave w = LoadWaveFromMemory(est.c_str(), (const unsigned char*)e.dati.data(), (int)e.dati.size());
        if (IsWaveValid(w)) { snd = LoadSoundFromWave(w); UnloadWave(w); }
        return snd;
    }
    std::vector<short> pcm = InPcm(RenderEffettoLibreria(e), 1.0f);
    return LoadSoundFromWave(ComeWave(pcm));
}

namespace {
std::string FirmaEffetto(const EffettoSuono& e) {
    return e.nome + TextFormat("|%d|%.1f|%.1f|%.3f|%.2f|%.2f|%.3f|%.1f|%.2f|%s|%zu", e.onda, e.f0, e.f1, e.durata, e.volume, e.duty,
                               e.vibrato, e.vibratoHz, e.rumore, e.file.c_str(), e.dati.size());
}
}

void Audio::SuonaEffetto(const EffettoSuono& e, float volume) {
    if (!pronto || muto) return;
    std::string k = FirmaEffetto(e);
    auto it = effettiPersonali.find(k);
    if (it == effettiPersonali.end()) it = effettiPersonali.emplace(k, CaricaEffettoPersonale(e)).first;
    if (!IsSoundValid(it->second)) return;
    SetSoundVolume(it->second, volume * volumeEffetti);
    PlaySound(it->second);
}

void Audio::SostituisciEffetto(Effetto e, const EffettoSuono* def) {
    if (!def) { sostituzioni.erase((int)e); return; }
    if (!pronto) return;
    std::string k = FirmaEffetto(*def);
    if (!effettiPersonali.count(k)) effettiPersonali[k] = CaricaEffettoPersonale(*def);
    sostituzioni[(int)e] = k;
}

void Audio::TogliSostituzioni() { sostituzioni.clear(); }

void Audio::SuonaBrano(const Brano& b) {
    std::string firma = b.nome + TextFormat("|%.1f|%d|%zu", b.bpm, b.battute, b.dati.size());
    for (auto& t : b.tracce) { firma += TextFormat("|%d:%zu", t.strumento, t.note.size()); for (auto& n : t.note) firma += TextFormat(",%.1f%.1f%d", n.inizio, n.durata, n.altezza); }
    if (firma == branoPersonale && (musicaFile || !brano.empty())) return;
    if (!b.file.empty() && !b.dati.empty()) {
        size_t p = b.file.find_last_of('.');
        std::string est = p == std::string::npos ? ".ogg" : b.file.substr(p);
        for (auto& c : est) c = (char)std::tolower((unsigned char)c);
        if (CaricaMusica(est, b.dati)) { branoPersonale = firma; return; }
    }
    RipristinaMusica();
    std::vector<float> c = RenderBrano(b);
    {
        std::lock_guard<std::mutex> g(blocco);
        brano.swap(c);
        note.clear();
        durata = brano.size() / (float)SR;
        bpm = b.bpm;
        posizione.store(0);
    }
    bpmPersonale = b.bpm;
    branoPersonale = firma;
    branoAttuale = -1;
}

void Audio::FermaBranoPersonale() {
    if (branoPersonale.empty()) return;
    branoPersonale.clear();
    RipristinaMusica();
    int n = 0;
    branoAttuale = -2;
    ScegliBrano(n);
}

float Audio::SedicesimoAttuale() const {
    return TempoBrano() / (60.0f / std::max(30.0f, bpm) / 4);
}
