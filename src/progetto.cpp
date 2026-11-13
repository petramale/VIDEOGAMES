#include "progetto.h"
#include "sfondo.h"
#include "json.hpp"
#include <cstdlib>
#include <sstream>

const char* NomeModello(Modello m) {
    switch (m) {
        case Modello::Platform: return "Platform";
        case Modello::Sparatutto: return "Sparatutto";
        case Modello::Labirinto: return "Labirinto";
        case Modello::Puzzle: return "Puzzle";
        default: return "";
    }
}

const char* ChiaveModello(Modello m) {
    switch (m) {
        case Modello::Sparatutto: return "sparatutto";
        case Modello::Labirinto: return "labirinto";
        case Modello::Puzzle: return "puzzle";
        default: return "platform";
    }
}

std::vector<std::string> DividiLivelli(const std::string& testoGioco) {
    std::vector<std::string> out(1);
    std::istringstream ss(testoGioco);
    std::string r;
    while (std::getline(ss, r)) {
        std::string pulita = r;
        if (!pulita.empty() && pulita.back() == '\r') pulita.pop_back();
        if (pulita == ";;") { out.emplace_back(); continue; }
        out.back() += r + "\n";
    }
    // Toglie le sezioni vuote (per esempio un ";;" finale).
    std::vector<std::string> pieni;
    for (auto& l : out) if (l.find_first_not_of(" \r\n") != std::string::npos) pieni.push_back(l);
    return pieni;
}

std::string UnisciLivelli(const std::vector<std::string>& livelli) {
    std::string t;
    for (size_t i = 0; i < livelli.size(); i++) { if (i) t += ";;\n"; t += livelli[i]; }
    return t;
}

const TestoCartello* Progetto::Testo(int numero) const {
    for (auto& t : testi) if (t.numero == numero) return &t;
    return nullptr;
}

std::string ColoreInTesto(Color c) { return TextFormat("#%02x%02x%02x", c.r, c.g, c.b); }

bool TestoInColore(const std::string& s, Color& c) {
    if (s.size() != 7 || s[0] != '#') return false;
    for (size_t i = 1; i < 7; i++) if (!std::isxdigit((unsigned char)s[i])) return false;
    c = Hex((unsigned int)std::strtoul(s.c_str() + 1, nullptr, 16));
    return true;
}

void Progetto::Nuovo(int larghezza) {
    righe.assign(ALTEZZA, std::string(larghezza, ' '));
    for (int y = 12; y < ALTEZZA; y++) righe[y] = std::string(larghezza, '#');
    righe[11][2] = 'P';
    righe[11][larghezza - 4] = 'F';
}

void Progetto::Ridimensiona(int larghezza) {
    larghezza = (int)Limita((float)larghezza, 30, 400);
    for (auto& r : righe) {
        char fondo = r.empty() ? ' ' : r.back();
        if (fondo != '#' && fondo != '=') fondo = ' ';
        r.resize(larghezza, fondo);
    }
}

bool Progetto::LeggiImpostazioni(const std::string& testo) {
    std::istringstream ss(testo);
    std::string riga;
    bool trovata = false;
    while (std::getline(ss, riga)) {
        if (riga.empty() || riga[0] != ';') continue;
        if (!riga.empty() && riga.back() == '\r') riga.pop_back();
        if (riga.rfind(";testo", 0) == 0) {
            // ;testo 3 Testo in italiano | English text
            TestoCartello t;
            std::string resto = riga.substr(6);
            size_t i = resto.find_first_not_of(' ');
            if (i == std::string::npos) continue;
            t.numero = std::atoi(resto.c_str() + i);
            if (t.numero < 1 || t.numero > 9) continue;
            size_t inizio = resto.find(' ', i);
            std::string corpo = inizio == std::string::npos ? "" : resto.substr(inizio + 1);
            size_t barra = corpo.find(" | ");
            t.it = corpo.substr(0, barra);
            t.en = barra == std::string::npos ? t.it : corpo.substr(barra + 3);
            for (auto& v : testi) if (v.numero == t.numero) { v = t; t.numero = 0; }
            if (t.numero) testi.push_back(t);
            continue;
        }
        if (trovata || riga.rfind(";;", 0) == 0) continue;
        trovata = true;
        // titolo= va per ultimo: prende tutto il resto della riga, spazi compresi.
        size_t t = riga.find("titolo=");
        if (t != std::string::npos) { titolo = riga.substr(t + 7); riga = riga.substr(0, t); }
        std::istringstream parti(riga.substr(1));
        std::string p;
        while (parti >> p) {
            size_t u = p.find('=');
            if (u == std::string::npos) continue;
            std::string k = p.substr(0, u), v = p.substr(u + 1);
            float f = (float)std::atof(v.c_str());
            if (k == "tema") tema = (int)Limita(f, 0, (float)Sfondo::Temi().size() - 1);
            else if (k == "ora") ora = std::fmod(Limita(f, 0, 1), 1.0f);
            else if (k == "gravita") fisica.gravita = Limita(f, 300, 2000);
            else if (k == "salto") fisica.salto = Limita(f, 150, 600);
            else if (k == "velocita") fisica.velocita = Limita(f, 40, 300);
            else if (k == "molla") fisica.molla = Limita(f, 250, 900);
            else if (k == "pelle") TestoInColore(v, eroe.pelle);
            else if (k == "capelli") TestoInColore(v, eroe.capelli);
            else if (k == "maglia") TestoInColore(v, eroe.maglia);
            else if (k == "mantello") TestoInColore(v, eroe.mantello);
            else if (k == "pantaloni") TestoInColore(v, eroe.pantaloni);
            else if (k == "scarpe") TestoInColore(v, eroe.scarpe);
            else if (k == "zoom") zoom = Limita(f, 0.75f, 2.5f);
            else if (k == "musica") musica = (int)Limita(f, 0, 99);
            else if (k == "modello")
                for (int m = 0; m < (int)Modello::Totale; m++) if (v == ChiaveModello((Modello)m)) modello = (Modello)m;
        }
    }
    return trovata;
}

bool Progetto::DaTesto(const std::string& testo, std::string& errore) {
    LeggiImpostazioni(testo);
    std::vector<std::string> nuove;
    std::istringstream ss(testo);
    std::string r;
    while (std::getline(ss, r)) {
        if (!r.empty() && r.back() == '\r') r.pop_back();
        if (r.empty() || r[0] == ';') continue;
        nuove.push_back(r);
    }
    if (nuove.size() < 4) { errore = "il livello deve avere almeno 4 righe"; return false; }
    // Sempre 15 righe: si aggiunge cielo sopra o si tolgono le righe in eccesso in alto.
    while ((int)nuove.size() < ALTEZZA) nuove.insert(nuove.begin(), std::string());
    while ((int)nuove.size() > ALTEZZA) nuove.erase(nuove.begin());
    size_t w = 30;
    for (auto& n : nuove) w = std::max(w, n.size());
    for (auto& n : nuove) n.resize(w, ' ');
    righe = nuove;
    return true;
}

std::string Progetto::Testo() const {
    std::string t = TextFormat(";modello=%s tema=%d ora=%.2f gravita=%.0f salto=%.0f velocita=%.0f molla=%.0f zoom=%.2f musica=%d",
                               ChiaveModello(modello), tema, ora, fisica.gravita, fisica.salto, fisica.velocita, fisica.molla, zoom, musica);
    t += " pelle=" + ColoreInTesto(eroe.pelle) + " capelli=" + ColoreInTesto(eroe.capelli) +
         " maglia=" + ColoreInTesto(eroe.maglia) + " mantello=" + ColoreInTesto(eroe.mantello) +
         " pantaloni=" + ColoreInTesto(eroe.pantaloni) + " scarpe=" + ColoreInTesto(eroe.scarpe) +
         " titolo=" + titolo + "\n";
    for (auto& tc : testi) t += ";testo " + std::to_string(tc.numero) + " " + tc.it + " | " + tc.en + "\n";
    for (auto& r : righe) t += r + "\n";
    return t;
}

std::string Progetto::Json() const {
    nlohmann::json j;
    j["titolo"] = titolo;
    j["modello"] = ChiaveModello(modello);
    j["zoom"] = zoom;
    j["musica"] = musica;
    nlohmann::json tj = nlohmann::json::array();
    for (auto& tc : testi) tj.push_back({ { "numero", tc.numero }, { "it", tc.it }, { "en", tc.en } });
    j["testi"] = tj;
    j["scenario"] = { { "tema", tema }, { "nome_tema", Sfondo::Temi()[tema].nome }, { "ora", ora } };
    j["fisica"] = { { "gravita", fisica.gravita }, { "salto", fisica.salto }, { "velocita", fisica.velocita }, { "molla", fisica.molla } };
    j["eroe"] = { { "pelle", ColoreInTesto(eroe.pelle) }, { "capelli", ColoreInTesto(eroe.capelli) },
                  { "maglia", ColoreInTesto(eroe.maglia) }, { "mantello", ColoreInTesto(eroe.mantello) },
                  { "pantaloni", ColoreInTesto(eroe.pantaloni) }, { "scarpe", ColoreInTesto(eroe.scarpe) } };
    j["righe"] = righe;
    return j.dump(1);
}
