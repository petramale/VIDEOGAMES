#include "audio.h"
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

void Audio::Callback(void* buffer, unsigned int frame) {
    short* out = (short*)buffer;
    Audio& a = gAudio;
    unsigned long pos = a.posizione.load();
    size_t n = a.brano.size();
    float vol = a.musicaAttiva ? a.volumeMusica : 0.0f;
    for (unsigned int i = 0; i < frame; i++) {
        out[i] = n ? (short)(a.brano[(pos + i) % n] * vol * 32000) : 0;
    }
    a.posizione.store(n ? (unsigned long)((pos + frame) % n) : 0);
}

void Audio::Prepara() {
    if (preparato) return;
    for (int i = 0; i < (int)Effetto::Totale; i++) campioniEffetti[i] = RenderEffetto(DefinisciEffetto((Effetto)i));
    ComponiBrano();
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
    StopAudioStream(flusso);
    UnloadAudioStream(flusso);
    for (auto& s : suoni) UnloadSound(s);
    CloseAudioDevice();
    pronto = false;
}

void Audio::Suona(Effetto e, float volume) {
    if (!pronto) return;
    Sound& s = suoni[(int)e];
    SetSoundVolume(s, volume);
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
    if (pronto || !musicaAttiva || brano.empty()) return;
    posizione.store((unsigned long)((posizione.load() + (unsigned long)(dt * SR)) % brano.size()));
}
