#include "midi.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace {

struct Lettore {
    const std::string& d;
    size_t p = 0, fine = 0;
    bool ok = true;
    explicit Lettore(const std::string& dati) : d(dati), fine(dati.size()) {}
    int B() { if (p >= fine) { ok = false; return 0; } return (unsigned char)d[p++]; }
    unsigned U32() { unsigned v = 0; for (int i = 0; i < 4; i++) v = (v << 8) | (unsigned)B(); return v; }
    unsigned U16() { unsigned v = (unsigned)B() << 8; return v | (unsigned)B(); }
    unsigned Var() { unsigned v = 0; for (int i = 0; i < 4; i++) { int c = B(); v = (v << 7) | (unsigned)(c & 0x7f); if (!(c & 0x80)) break; } return v; }
};

struct NotaMidi { long inizio, fine; int altezza, canale; };

// Onda dello studio per gli strumenti General MIDI (0 quadra, 1 triangolo, 2 dente di sega, 3 seno).
int Onda(int programma) {
    int g = programma / 8;
    switch (g) {
        case 0: return 1;    // pianoforti
        case 1: return 3;    // campane, xilofoni
        case 2: return 0;    // organi
        case 3: return 2;    // chitarre
        case 4: return 1;    // bassi
        case 5: case 6: return 2;   // archi, cori
        case 7: return 0;    // ottoni
        case 8: case 9: return 3;   // ance e flauti
        case 10: return 0;   // synth lead
        case 11: return 3;   // pad
        default: return 0;
    }
}

const char* NomeGruppo(int programma) {
    static const char* n[] = { "Pianoforte", "Campane", "Organo", "Chitarra", "Basso", "Archi", "Coro", "Ottoni",
                               "Sax", "Flauto", "Synth", "Pad", "Effetti", "Etnico", "Percussioni", "Suoni" };
    return n[(programma / 8) & 15];
}

} // namespace

bool BranoDaMidi(const std::string& dati, Brano& b, std::string& errore) {
    Lettore r(dati);
    if (dati.size() < 14 || dati.compare(0, 4, "MThd") != 0) { errore = "non e' un file MIDI"; return false; }
    r.p = 4;
    unsigned lh = r.U32();
    r.U16();                       // formato
    int nTracce = (int)r.U16();
    int divisione = (int)r.U16();
    if (divisione & 0x8000) divisione = 480;   // tempo SMPTE: raro, si usa un valore tipico
    if (divisione <= 0) divisione = 480;
    r.p = 8 + lh;
    std::vector<NotaMidi> note;
    int programma[16] = { 0 };
    bool programmaVisto[16] = { false };
    double usPerQuarto = 0;
    std::map<int, std::string> nomiCanale;
    for (int t = 0; t < nTracce && r.p + 8 <= dati.size(); t++) {
        if (dati.compare(r.p, 4, "MTrk") != 0) { errore = "traccia MIDI rovinata"; break; }
        r.p += 4;
        unsigned lung = r.U32();
        size_t fineTraccia = std::min(dati.size(), r.p + lung);
        r.fine = fineTraccia;
        long tick = 0;
        int stato = 0;
        std::string nomeTraccia;
        std::map<int, long> accese[16];   // altezza -> inizio
        while (r.p < fineTraccia && r.ok) {
            tick += (long)r.Var();
            int c = r.B();
            if (c < 0x80) { if (!stato) break; r.p--; c = stato; }   // running status
            if (c == 0xFF) {
                int tipo = r.B();
                unsigned l = r.Var();
                if (tipo == 0x51 && l == 3 && usPerQuarto == 0) { unsigned v = (unsigned)r.B() << 16; v |= (unsigned)r.B() << 8; v |= (unsigned)r.B(); usPerQuarto = v; }
                else if (tipo == 0x03) { if (r.p < dati.size()) nomeTraccia = dati.substr(r.p, std::min<size_t>(l, 40)); r.p += l; }
                else r.p += l;
                if (tipo == 0x2F) break;
                continue;
            }
            if (c == 0xF0 || c == 0xF7) { r.p += r.Var(); continue; }
            stato = c;
            int tipo = c & 0xF0, ch = c & 0x0F;
            if (tipo == 0xC0) { int pr = r.B(); if (!programmaVisto[ch]) { programma[ch] = pr; programmaVisto[ch] = true; } continue; }
            if (tipo == 0xD0) { r.B(); continue; }
            int a = r.B(), v = r.B();
            if (tipo == 0x90 && v > 0) {
                if (accese[ch].count(a)) note.push_back({ accese[ch][a], tick, a, ch });
                accese[ch][a] = tick;
                if (!nomeTraccia.empty() && !nomiCanale.count(ch)) nomiCanale[ch] = nomeTraccia;
            } else if (tipo == 0x80 || tipo == 0x90) {
                auto it = accese[ch].find(a);
                if (it != accese[ch].end()) { note.push_back({ it->second, tick, a, ch }); accese[ch].erase(it); }
            }
        }
        for (int ch = 0; ch < 16; ch++) for (auto& x : accese[ch]) note.push_back({ x.second, tick, x.first, ch });
        r.p = fineTraccia;
        r.fine = dati.size();
        r.ok = true;
    }
    if (note.empty()) { if (errore.empty()) errore = "il file MIDI non ha note"; return false; }
    errore.clear();
    // Le parti: la batteria e i 5 canali con piu' note.
    std::map<int, int> conta;
    for (auto& n : note) conta[n.canale]++;
    std::vector<std::pair<int, int>> canali;
    for (auto& c : conta) if (c.first != 9) canali.push_back({ c.second, c.first });
    std::sort(canali.rbegin(), canali.rend());
    if (canali.size() > 5) canali.resize(5);
    std::vector<int> scelti;
    for (auto& c : canali) scelti.push_back(c.second);
    std::sort(scelti.begin(), scelti.end());
    if (conta.count(9)) scelti.push_back(9);
    double sedicesimo = divisione / 4.0;
    b.bpm = usPerQuarto > 0 ? (float)std::round(60000000.0 / usPerQuarto) : 120.0f;
    b.bpm = std::max(30.0f, std::min(300.0f, b.bpm));
    b.tracce.clear();
    b.file.clear(); b.dati.clear();
    float fineBrano = 0;
    float volume = scelti.size() > 3 ? 0.4f : 0.5f;
    for (int ch : scelti) {
        TracciaBrano t;
        bool bat = ch == 9;
        t.batteria = bat;
        t.strumento = bat ? 4 : Onda(programma[ch]);
        t.volume = bat ? volume * 0.8f : volume;
        t.nome = bat ? std::string("Batteria") : nomiCanale.count(ch) ? nomiCanale[ch] : std::string(NomeGruppo(programma[ch]));
        for (auto& n : note) {
            if (n.canale != ch) continue;
            NotaBrano nb;
            nb.inizio = (float)(std::round(n.inizio / sedicesimo * 4) / 4);
            nb.durata = std::max(0.25f, (float)(std::round((n.fine - n.inizio) / sedicesimo * 4) / 4));
            if (bat) nb.durata = std::min(nb.durata, 1.0f);
            nb.altezza = n.altezza;
            fineBrano = std::max(fineBrano, nb.inizio + nb.durata);
            t.note.push_back(nb);
        }
        std::sort(t.note.begin(), t.note.end(), [](const NotaBrano& a, const NotaBrano& c) { return a.inizio < c.inizio; });
        b.tracce.push_back(t);
    }
    b.battute = std::max(1, std::min(256, (int)std::ceil(fineBrano / 16.0f)));
    return true;
}
