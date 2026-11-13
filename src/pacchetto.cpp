#include "pacchetto.h"
#include "sistema.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

namespace {
const char FIRMA[9] = "VGPAK001";
const std::string INTESTAZIONE = "STUDIO-VIDEOGIOCHI 2\n";

bool LeggiFile(const std::string& percorso, std::string& contenuto) {
    std::ifstream f(percorso, std::ios::binary);
    if (!f) return false;
    contenuto.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

// Separa l'eseguibile "pulito" dagli eventuali dati gia' attaccati.
size_t DimensioneEseguibile(const std::string& exe, std::string* dati) {
    if (exe.size() < 16 || std::memcmp(exe.data() + exe.size() - 8, FIRMA, 8) != 0) return exe.size();
    uint64_t n = 0;
    for (int i = 0; i < 8; i++) n |= (uint64_t)(unsigned char)exe[exe.size() - 16 + i] << (8 * i);
    if (n > exe.size() - 16) return exe.size();
    size_t inizio = exe.size() - 16 - (size_t)n;
    if (dati) *dati = exe.substr(inizio, (size_t)n);
    return inizio;
}
} // namespace

// Formato: intestazione, titolo, lunghezza e testo del livello, numero di file,
// poi per ogni file: nome, lunghezza, contenuto.
std::string SerializzaGioco(const DatiGioco& g) {
    std::string s = INTESTAZIONE + g.titolo + "\n" + std::to_string(g.livello.size()) + "\n" + g.livello;
    s += std::to_string(g.file.size()) + "\n";
    for (const auto& f : g.file) s += f.first + "\n" + std::to_string(f.second.size()) + "\n" + f.second;
    return s;
}

bool DeserializzaGioco(const std::string& dati, DatiGioco& g) {
    if (dati.compare(0, INTESTAZIONE.size(), INTESTAZIONE) != 0) return false;
    size_t p = INTESTAZIONE.size();
    auto Riga = [&](std::string& out) {
        size_t e = dati.find('\n', p);
        if (e == std::string::npos) return false;
        out = dati.substr(p, e - p);
        p = e + 1;
        return true;
    };
    auto Blocco = [&](std::string& out) {
        std::string n;
        if (!Riga(n)) return false;
        size_t len = (size_t)std::strtoull(n.c_str(), nullptr, 10);
        if (p + len > dati.size()) return false;
        out = dati.substr(p, len);
        p += len;
        return true;
    };
    std::string conta;
    if (!Riga(g.titolo) || !Blocco(g.livello) || !Riga(conta)) return false;
    int n = std::atoi(conta.c_str());
    g.file.clear();
    for (int i = 0; i < n; i++) {
        FileIncluso f;
        if (!Riga(f.first) || !Blocco(f.second)) return false;
        g.file.push_back(f);
    }
    return true;
}

bool LeggiPacchettoIncluso(std::string& dati) {
    std::string exe;
    if (!LeggiFile(PercorsoEseguibile(), exe)) return false;
    return DimensioneEseguibile(exe, &dati) < exe.size();
}

bool CreaEseguibile(const std::string& destinazione, const std::string& dati, std::string& errore) {
    std::string exe;
    if (!LeggiFile(PercorsoEseguibile(), exe)) { errore = "non riesco a leggere lo studio"; return false; }
    exe.resize(DimensioneEseguibile(exe, nullptr));
    std::ofstream f(destinazione, std::ios::binary);
    if (!f) { errore = "non riesco a scrivere " + destinazione; return false; }
    uint64_t n = dati.size();
    char dim[8];
    for (int i = 0; i < 8; i++) dim[i] = (char)((n >> (8 * i)) & 0xFF);
    f.write(exe.data(), (std::streamsize)exe.size());
    f.write(dati.data(), (std::streamsize)dati.size());
    f.write(dim, 8);
    f.write(FIRMA, 8);
    if (!f) { errore = "scrittura non riuscita"; return false; }
    f.close();
    RendiEseguibile(destinazione);
    return true;
}
