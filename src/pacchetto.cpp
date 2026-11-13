#include "pacchetto.h"
#include "sistema.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>

namespace {
const char FIRMA[9] = "VGPAK001";
const std::string INTESTAZIONE = "STUDIO-VIDEOGIOCHI 1\n";

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

std::string SerializzaGioco(const DatiGioco& g) {
    return INTESTAZIONE + g.titolo + "\n" + g.livello;
}

bool DeserializzaGioco(const std::string& dati, DatiGioco& g) {
    if (dati.compare(0, INTESTAZIONE.size(), INTESTAZIONE) != 0) return false;
    size_t a = INTESTAZIONE.size(), b = dati.find('\n', a);
    if (b == std::string::npos) return false;
    g.titolo = dati.substr(a, b - a);
    g.livello = dati.substr(b + 1);
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
