// Salvataggio e caricamento: busta con CRC-32, scrittura atomica, migrazioni, e lo stato della partita
// (Gioco::StatoJson / Gioco::Ripristina). Vedi salvataggio.h.
//
// Il ripristino segue la regola "ricostruisci, poi sovrapponi":
//   1. si ricarica il gioco con il seme salvato (il Destino rifa' le stesse variazioni);
//   2. si rientra nella mappa salvata (layout, creature e oggetti rinascono identici);
//   3. si sovrappone lo stato salvato, entita' per entita', cercandole per id stabile;
//   4. si controlla tutto (numeri validi, eroe fuori dai muri) e si azzera cio' che e' passeggero.
#include "salvataggio.h"
#include "gioco.h"
#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

using nlohmann::json;

// --- Busta -----------------------------------------------------------------------------------------------

uint32_t Crc32(const std::string& dati) {
    static uint32_t tabella[256];
    static bool pronta = false;
    if (!pronta) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            tabella[i] = c;
        }
        pronta = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char b : dati) c = tabella[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static const char* MAGIA = "ELVANTIS-SALVATAGGIO 1";

std::string ImbustaSalvataggio(const std::string& j) {
    char riga[64];
    snprintf(riga, sizeof riga, "CRC32 %08x BYTE %lu", (unsigned)Crc32(j), (unsigned long)j.size());
    return std::string(MAGIA) + "\n" + riga + "\n" + j;
}

bool SbustaSalvataggio(const std::string& t, std::string& j, std::string& errore) {
    if (t.empty()) { errore = "file vuoto"; return false; }
    size_t a = t.find('\n');
    std::string prima = t.substr(0, a == std::string::npos ? t.size() : a);
    if (!prima.empty() && prima.back() == '\r') prima.pop_back();
    if (prima.rfind("ELVANTIS-SALVATAGGIO", 0) != 0) { errore = "non e' un salvataggio"; return false; }
    if (prima != MAGIA) { errore = "busta di un formato sconosciuto (" + prima + ")"; return false; }
    if (a == std::string::npos) { errore = "file troncato"; return false; }
    size_t b = t.find('\n', a + 1);
    if (b == std::string::npos) { errore = "file troncato"; return false; }
    std::string seconda = t.substr(a + 1, b - a - 1);
    unsigned crc = 0;
    unsigned long n = 0;
    if (sscanf(seconda.c_str(), "CRC32 %8x BYTE %lu", &crc, &n) != 2) { errore = "intestazione rovinata"; return false; }
    j = t.substr(b + 1);
    if (j.size() < n) { errore = "file troncato (mancano " + std::to_string(n - j.size()) + " byte)"; return false; }
    if (j.size() > n) j.resize(n);   // spazi o righe aggiunte in fondo da un editor: si ignorano
    if (Crc32(j) != crc) { errore = "file danneggiato (il codice di controllo non torna)"; return false; }
    return true;
}

// --- Migrazioni -------------------------------------------------------------------------------------------
// Ogni funzione porta il salvataggio dalla versione N alla N+1. Per cambiare il formato:
// aumentare VERSIONE_SALVATAGGIO e aggiungere qui MigraNaN+1, senza toccare le precedenti.

namespace {

// Versione 0: il salvataggio "minimo" (posizione, inventario, punti), come lo scriverebbe un gioco semplice.
// Diventa una versione 1 "parziale": le creature e gli oggetti della mappa ripartono come nuovi.
void Migra0a1(json& j) {
    json n;
    n["formato"] = "elvantis-salvataggio";
    n["versione"] = 1;
    n["parziale"] = true;
    n["gioco"] = j.value("gioco", json::object());
    n["destino"] = { { "seme", "0" }, { "evento", "" } };
    json p;
    p["mappa"] = j.value("mappa", 0);
    p["eroe"] = { { "x", j.value("x", -1.0) }, { "y", j.value("y", -1.0) } };
    for (const char* k : { "punteggio", "vite", "energia", "monete", "gemme" }) if (j.contains(k)) p[k] = j[k];
    if (j.contains("inventario")) p["inventario"] = j["inventario"];
    if (j.contains("variabili")) p["variabili"] = j["variabili"];
    n["partita"] = p;
    j = n;
}

} // namespace

bool MigraSalvataggio(std::string& testo, std::string& errore, int* versioneLetta) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { errore = "il contenuto non e' un JSON valido"; return false; }
    int v = 0;
    if (j.contains("versione") && j["versione"].is_number_integer()) v = j["versione"].get<int>();
    if (versioneLetta) *versioneLetta = v;
    if (v > VERSIONE_SALVATAGGIO) {
        errore = "salvataggio di una versione piu' nuova del gioco (" + std::to_string(v) + ", questo gioco legge fino alla " +
                 std::to_string(VERSIONE_SALVATAGGIO) + ")";
        return false;
    }
    if (v < 0) { errore = "versione non valida"; return false; }
    if (v == 0) {
        if (!j.contains("mappa") && !j.contains("x")) { errore = "non e' un salvataggio"; return false; }
        Migra0a1(j); v = 1;
    }
    // if (v == 1) { Migra1a2(j); v = 2; }   <- le prossime versioni si aggiungono qui
    if (!j.contains("partita") || !j["partita"].is_object()) { errore = "manca lo stato della partita"; return false; }
    testo = j.dump();
    return true;
}

// --- File ---------------------------------------------------------------------------------------------------

namespace {
bool LeggiFile(const std::string& p, std::string& out) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}
bool Esiste(const std::string& p) { std::ifstream in(p, std::ios::binary); return (bool)in; }
}

bool ScriviAtomico(const std::string& percorso, const std::string& contenuto, std::string& errore) {
    std::string tmp = percorso + ".tmp", bak = percorso + ".bak";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) { errore = "non posso scrivere " + tmp; return false; }
        out.write(contenuto.data(), (std::streamsize)contenuto.size());
        out.flush();
        if (!out) { errore = "scrittura interrotta (disco pieno?)"; out.close(); std::remove(tmp.c_str()); return false; }
    }
    // Controllo: si rilegge quello che si e' scritto prima di toccare il vecchio salvataggio.
    std::string verifica;
    if (!LeggiFile(tmp, verifica) || verifica != contenuto) { errore = "il file scritto non si rilegge uguale"; std::remove(tmp.c_str()); return false; }
    if (Esiste(percorso)) {
        std::remove(bak.c_str());
        if (std::rename(percorso.c_str(), bak.c_str()) != 0) { std::remove(percorso.c_str()); }
    }
    if (std::rename(tmp.c_str(), percorso.c_str()) != 0) {
        std::rename(bak.c_str(), percorso.c_str());   // si rimette a posto il vecchio
        errore = "non posso rinominare il file temporaneo";
        return false;
    }
    return true;
}

namespace {
void RiempiInfo(const std::string& j, InfoSalvataggio& info) {
    json x = json::parse(j, nullptr, false);
    if (x.is_discarded()) return;
    const json g = x.value("gioco", json::object());
    info.titolo = g.value("titolo", "");
    info.mappa = g.value("mappa", "");
    info.data = g.value("data", "");
    info.punteggio = g.value("punteggio", 0);
    info.tempo = g.value("tempo", 0.0f);
    info.giocatori = x.value("multi", json::object()).value("giocatori", 1);
    uint64_t imp = 0;
    if (DaEsadecimale64(g.value("impronta", ""), imp)) info.impronta = imp;
}

bool LeggiUno(const std::string& percorso, std::string& j, InfoSalvataggio& info) {
    std::string t;
    if (!LeggiFile(percorso, t)) { info.errore = "non c'e'"; return false; }
    info.esiste = true;
    if (!SbustaSalvataggio(t, j, info.errore)) return false;
    if (!MigraSalvataggio(j, info.errore, &info.versione)) return false;
    RiempiInfo(j, info);
    return true;
}
}

bool LeggiSalvataggio(const std::string& percorso, std::string& j, InfoSalvataggio& info) {
    info = InfoSalvataggio{};
    if (LeggiUno(percorso, j, info)) { info.valido = true; return true; }
    // Il file principale non va: si prova la copia di riserva.
    InfoSalvataggio riserva;
    std::string errorePrincipale = info.errore;
    bool esisteva = info.esiste;
    if (LeggiUno(percorso + ".bak", j, riserva)) {
        info = riserva;
        info.esiste = true; info.valido = true; info.daRiserva = true;
        info.errore = esisteva ? errorePrincipale : "";
        return true;
    }
    info.errore = errorePrincipale;
    info.esiste = esisteva || riserva.esiste;
    return false;
}

InfoSalvataggio InfoDiSalvataggio(const std::string& percorso) {
    InfoSalvataggio info;
    std::string j;
    LeggiSalvataggio(percorso, j, info);
    return info;
}

std::string DataOra() {
    std::time_t t = std::time(nullptr);
    std::tm* l = std::localtime(&t);
    char b[32] = "";
    if (l) std::strftime(b, sizeof b, "%d/%m/%Y %H:%M", l);
    return b;
}

std::string ComponiSalvataggio(const std::string& statoGioco, const MetaSalvataggio& m) {
    json j = json::parse(statoGioco, nullptr, false);
    if (j.is_discarded() || !j.is_object()) j = json::object();
    j["formato"] = "elvantis-salvataggio";
    j["versione"] = VERSIONE_SALVATAGGIO;
    j["gioco"] = { { "titolo", m.titolo }, { "impronta", Esadecimale64(m.impronta) }, { "data", DataOra() }, { "mappa", m.mappa },
                   { "punteggio", m.punteggio }, { "tempo", std::round(m.tempo * 10) / 10 } };
    j["multi"] = { { "giocatori", m.giocatori }, { "turno", m.turno }, { "punti", { m.punti[0], m.punti[1] } } };
    return j.dump(1);
}

MetaSalvataggio MetaDi(const std::string& testo) {
    MetaSalvataggio m;
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded()) return m;
    const json g = j.value("gioco", json::object());
    m.titolo = g.value("titolo", ""); m.mappa = g.value("mappa", "");
    m.punteggio = g.value("punteggio", 0); m.tempo = g.value("tempo", 0.0f);
    uint64_t imp = 0;
    if (DaEsadecimale64(g.value("impronta", ""), imp)) m.impronta = imp;
    const json x = j.value("multi", json::object());
    m.giocatori = std::max(1, std::min(2, x.value("giocatori", 1)));
    m.turno = std::max(0, std::min(1, x.value("turno", 0)));
    if (x.contains("punti") && x["punti"].is_array() && x["punti"].size() == 2) {
        m.punti[0] = x["punti"][0].is_number() ? x["punti"][0].get<int>() : 0;
        m.punti[1] = x["punti"][1].is_number() ? x["punti"][1].get<int>() : 0;
    }
    return m;
}

// --- Stato della partita ---------------------------------------------------------------------------------

namespace {
// Valori esatti (un float in double si rilegge identico): la partita ripresa resta uguale bit per bit.
double R3(float v) { return std::isfinite(v) ? (double)v : 0.0; }

// Lettura sicura: un numero assente o non valido lascia il valore com'e'.
template <typename T> bool Num(const json& j, const char* k, T& v) {
    if (!j.is_object() || !j.contains(k) || !j[k].is_number()) return false;
    double d = j[k].get<double>();
    if (!std::isfinite(d)) return false;
    v = (T)d;
    return true;
}
void Bool(const json& j, const char* k, bool& v) { if (j.is_object() && j.contains(k) && j[k].is_boolean()) v = j[k].get<bool>(); }
void Testo(const json& j, const char* k, std::string& v) { if (j.is_object() && j.contains(k) && j[k].is_string()) v = j[k].get<std::string>(); }
float L01(float v) { return std::isfinite(v) ? (v < 0 ? 0 : v > 1 ? 1 : v) : 0; }

json CarattereJ(const Carattere& c) {
    return { R3(c.coraggio), R3(c.aggressivita), R3(c.prudenza), R3(c.socievolezza), R3(c.rancore), R3(c.curiosita), R3(c.imprevedibilita) };
}
void CarattereDaJ(const json& j, Carattere& c) {
    if (!j.is_array() || j.size() != 7) return;
    float* v[] = { &c.coraggio, &c.aggressivita, &c.prudenza, &c.socievolezza, &c.rancore, &c.curiosita, &c.imprevedibilita };
    for (int i = 0; i < 7; i++) if (j[i].is_number()) *v[i] = L01(j[i].get<float>());
}

json MenteJ(const Mente& m) {
    json r = json::array();
    for (int i = 0; i < m.numRicordi; i++) {
        const Ricordo& x = m.ricordi[i];
        r.push_back({ (int)x.tipo, R3(x.quando), R3(x.dove.x), R3(x.dove.y), R3(x.forza) });
    }
    return { { "az", ChiaveAzioneMente(m.azione) }, { "tAz", R3(m.tAzione) }, { "min", R3(m.minaccia) }, { "pau", R3(m.paura) },
             { "fid", R3(m.fiducia) }, { "fidB", R3(m.fiduciaBase) }, { "sta", R3(m.stanchezza) }, { "all", R3(m.allerta) },
             { "car", CarattereJ(m.car) }, { "uv", { R3(m.ultimoVisto.x), R3(m.ultimoVisto.y) } }, { "tv", R3(std::min(m.tVisto, 9999.0f)) },
             { "tRich", R3(m.tRichiamo) }, { "tSch", R3(m.tSchiva) }, { "tScat", R3(m.tScatto) }, { "tCol", R3(std::min(m.tColpoEroe, 9999.0f)) },
             { "ric", r }, { "prossimo", m.prossimo }, { "tPensa", R3(m.tPensa) }, { "tRiv", R3(m.tRivale) } };
}

void MenteDaJ(const json& j, Mente& m) {
    if (!j.is_object()) return;
    std::string az;
    Testo(j, "az", az);
    for (int i = 0; i < NUM_AZIONI_MENTE; i++) if (az == ChiaveAzioneMente((AzioneMente)i)) m.azione = (AzioneMente)i;
    Num(j, "tAz", m.tAzione);
    Num(j, "min", m.minaccia); Num(j, "pau", m.paura); Num(j, "fid", m.fiducia); Num(j, "fidB", m.fiduciaBase);
    Num(j, "sta", m.stanchezza); Num(j, "all", m.allerta);
    m.minaccia = L01(m.minaccia); m.paura = L01(m.paura); m.stanchezza = L01(m.stanchezza); m.allerta = L01(m.allerta);
    m.fiducia = std::max(-1.0f, std::min(1.0f, m.fiducia)); m.fiduciaBase = std::max(-1.0f, std::min(1.0f, m.fiduciaBase));
    if (j.contains("car")) CarattereDaJ(j["car"], m.car);
    if (j.contains("uv") && j["uv"].is_array() && j["uv"].size() == 2 && j["uv"][0].is_number() && j["uv"][1].is_number())
        m.ultimoVisto = { j["uv"][0].get<float>(), j["uv"][1].get<float>() };
    Num(j, "tv", m.tVisto); Num(j, "tRich", m.tRichiamo); Num(j, "tSch", m.tSchiva); Num(j, "tScat", m.tScatto); Num(j, "tCol", m.tColpoEroe);
    m.numRicordi = 0; m.prossimo = 0;
    if (j.contains("ric") && j["ric"].is_array())
        for (auto& x : j["ric"]) {
            if (m.numRicordi >= 8 || !x.is_array() || x.size() != 5) continue;
            bool ok = true;
            for (auto& v : x) if (!v.is_number()) ok = false;
            if (!ok) continue;
            int t = x[0].get<int>();
            if (t < 0 || t >= (int)TipoRicordo::Totale) continue;
            m.ricordi[m.numRicordi++] = { (TipoRicordo)t, x[1].get<float>(), { x[2].get<float>(), x[3].get<float>() }, L01(x[4].get<float>()) };
        }
    int p = 0;
    if (Num(j, "prossimo", p)) m.prossimo = (p >= 0 && p < 8) ? p : 0;
    if (m.numRicordi < 8) m.prossimo = m.numRicordi;
    m.tPensa = 0;
    Num(j, "tPensa", m.tPensa); Num(j, "tRiv", m.tRivale);
    m.tPensa = std::max(0.0f, std::min(0.2f, m.tPensa));
}

json SapereJ(const std::map<std::string, Sapere>& s) {
    json j = json::object();
    for (auto& x : s) {
        const Sapere& v = x.second;
        j[x.first] = { { "schiacciato", R3(v.schiacciato) }, { "colpito", R3(v.colpito) }, { "colpiDati", R3(v.colpiDati) },
                       { "saltiEroe", R3(v.saltiEroe) }, { "spariEroe", R3(v.spariEroe) }, { "esperienze", v.esperienze } };
    }
    return j;
}

void SapereDaJ(const json& j, std::map<std::string, Sapere>& s) {
    if (!j.is_object()) return;
    for (auto it = j.begin(); it != j.end(); ++it) {
        Sapere v;
        const json& x = it.value();
        Num(x, "schiacciato", v.schiacciato); Num(x, "colpito", v.colpito); Num(x, "colpiDati", v.colpiDati);
        Num(x, "saltiEroe", v.saltiEroe); Num(x, "spariEroe", v.spariEroe); Num(x, "esperienze", v.esperienze);
        v.schiacciato = L01(v.schiacciato); v.colpito = L01(v.colpito); v.colpiDati = L01(v.colpiDati);
        v.saltiEroe = L01(v.saltiEroe); v.spariEroe = L01(v.spariEroe);
        s[it.key()] = v;
    }
}

json Insieme(const std::set<int>& s) { json a = json::array(); for (int x : s) a.push_back(x); return a; }
void DaInsieme(const json& j, const char* k, std::set<int>& s) {
    if (!j.is_object() || !j.contains(k) || !j[k].is_array()) return;
    for (auto& x : j[k]) if (x.is_number_integer()) s.insert(x.get<int>());
}

std::string ColoreHex(Color c) { char b[12]; snprintf(b, sizeof b, "%02x%02x%02x%02x", c.r, c.g, c.b, c.a); return b; }
bool HexColore(const std::string& t, Color& c) {
    unsigned r, g, b, a;
    if (t.size() != 8 || sscanf(t.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a) != 4) return false;
    c = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    return true;
}

bool UgualeNome(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

} // namespace

std::string Gioco::StatoJson() const {
    json j;
    j["destino"] = { { "seme", Esadecimale64(semePartita) }, { "evento", eventoPartita }, { "rng", Esadecimale64(rng.s) } };
    json p;
    p["mappa"] = mappaAttuale;
    if (mappaAttuale >= 0 && mappaAttuale < (int)mondo.mappe.size()) p["mappaNome"] = mondo.mappe[mappaAttuale].nome;
    p["eroe"] = { { "x", R3(eroe.pos.x) }, { "y", R3(eroe.pos.y) }, { "vx", R3(eroe.vel.x) }, { "vy", R3(eroe.vel.y) }, { "dir", eroe.dir } };
    p["energia"] = energia; p["energiaMax"] = energiaMax; p["vite"] = vite; p["punteggio"] = punteggio;
    p["monete"] = monete; p["gemme"] = gemme; p["mosse"] = mosse; p["tempo"] = R3(tempo);
    p["checkpoint"] = { R3(checkpoint.x), R3(checkpoint.y) };
    p["pieta"] = R3(pieta); p["creati"] = contatoreCreati;
    p["ora"] = R3(sfondo.ora); p["pioggia"] = pioggia;
    p["vinto"] = vinto; p["finito"] = finito;
    json v = json::object(); for (auto& x : variabili) v[x.first] = R3(x.second);
    p["variabili"] = v;
    json inv = json::object(); for (auto& x : inventario) inv[x.first] = x.second;
    p["inventario"] = inv;
    json tr = json::array(); for (float t : timerRegole) tr.push_back(R3(t));
    json cr = json::array(); for (bool c : condizioniRegole) cr.push_back(c);
    p["regole"] = { { "timer", tr }, { "condizioni", cr } };
    // Fisica fine dell'eroe, telecamera e tempi: servono perche' la partita riprenda identica.
    p["moto"] = { { "aTerra", eroe.aTerra }, { "stato", (int)eroe.stato }, { "tStato", R3(eroe.tStato) }, { "lampeggio", R3(eroe.lampeggio) },
                  { "camX", R3(camX) }, { "camY", R3(camY) }, { "zoom", R3(zoom) }, { "tSparo", R3(tSparo) }, { "tPassoPuzzle", R3(tPassoPuzzle) },
                  { "coyote", R3(coyote) }, { "bufferSalto", R3(bufferSalto) }, { "saltiFatti", saltiFatti }, { "saltoTagliato", saltoTagliato },
                  { "tFineScorrimento", R3(tFineScorrimento) }, { "fineScorrimento", fineScorrimento }, { "portaBloccata", portaBloccata },
                  { "aTerraPrima", eroeATerraPrima } };
    json pr = json::array();
    for (auto& q : proiettili)
        pr.push_back({ R3(q.pos.x), R3(q.pos.y), R3(q.vel.x), R3(q.vel.y), q.amico, R3(q.vita), ColoreHex(q.colore), q.disegno, q.danno });
    p["proiettili"] = pr;
    json no = json::array();
    for (auto& q : energiaNotificata) no.push_back({ q.first, q.second });
    p["notificate"] = no;
    p["appoggio"] = appoggio; p["dialogo"] = dialogo;
    j["partita"] = p;
    json en = json::array();
    for (const Entita& e : entita) {
        json x = { { "uid", e.uid }, { "nome", e.nome }, { "x", R3(e.pos.x) }, { "y", R3(e.pos.y) }, { "vx", R3(e.vel.x) }, { "vy", R3(e.vel.y) },
                   { "ox", R3(e.origine.x) }, { "oy", R3(e.origine.y) }, { "dir", e.dir }, { "en", e.energia }, { "enMax", e.energiaMax },
                   { "att", e.attivo && e.morte <= 0 }, { "ap", e.aperta }, { "acc", e.acceso }, { "t", R3(e.t) }, { "timer", R3(e.timer) },
                   { "velMolt", R3(e.velMolt) }, { "anim", e.anim }, { "tinta", ColoreHex(e.tinta) }, { "vet", e.veterano },
                   { "pm", R3(e.puntiMolt) }, { "vm", R3(e.valoreMolt) }, { "amico", e.amicoAnnunciato },
                   { "morte", R3(e.morte) }, { "aT", e.aTerra }, { "tAn", R3(e.tAnim) }, { "lamp", R3(e.lampeggio) }, { "sp", R3(e.spinta) },
                   { "bl", e.bloccato }, { "tGr", R3(e.tGrido) }, { "tCo", R3(e.tContatto) }, { "toc", e.toccando }, { "vic", e.vicino } };
        x["att"] = e.attivo;
        if (!e.personaggio) {
            x["carica"] = R3(e.carica); x["forza"] = R3(e.forza); x["usi"] = e.usi; x["rotto"] = e.rotto;
            x["tRic"] = R3(e.tRicarica); x["tSv"] = R3(e.tSveglio);
        }
        if (e.uid >= 1000000) { x["mappaPorta"] = e.mappaPorta; x["dest"] = { R3(e.destPorta.x), R3(e.destPorta.y) }; }
        if (e.mente.attiva) x["mente"] = MenteJ(e.mente);
        en.push_back(x);
    }
    j["entita"] = en;
    json ob = json::array();
    for (const Oggetto& o : oggetti)
        ob.push_back({ { "a", o.attivo }, { "x", R3(o.pos.x) }, { "y", R3(o.pos.y) }, { "vx", R3(o.vel.x) }, { "vy", R3(o.vel.y) },
                       { "s", R3(o.stato) }, { "n", o.numero }, { "sc", o.sconfitto }, { "dir", o.dir }, { "f", o.fisica } });
    j["oggettiBase"] = ob;
    json mp = json::object();
    for (auto& m : mappeStato)
        mp[std::to_string(m.first)] = { { "tolti", Insieme(m.second.tolti) }, { "aperti", Insieme(m.second.aperti) },
                                        { "accesi", Insieme(m.second.accesi) }, { "base", Insieme(m.second.base) } };
    j["mappe"] = mp;
    j["sapere"] = SapereJ(sapere);
    return j.dump();
}

std::string Gioco::SapereJson() const { return SapereJ(sapere).dump(1); }

void Gioco::SapereDaJson(const std::string& testo) {
    json j = json::parse(testo, nullptr, false);
    if (!j.is_discarded()) SapereDaJ(j, sapere);
}

bool Gioco::Ripristina(const ProgettoGioco& progetto, const std::string& testo, std::string& errore, std::string& avviso) {
    avviso.clear();
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object() || !j.contains("partita")) { errore = "stato della partita non valido"; return false; }
    const json& d = j.value("destino", json::object());
    uint64_t seme = 0;
    std::string s;
    Testo(d, "seme", s);
    if (!s.empty() && !DaEsadecimale64(s, seme)) { errore = "seme non valido"; return false; }
    bool parziale = j.value("parziale", false);
    std::vector<std::string> correzioni;

    // 1. Ricostruire: stesso gioco, stesso seme.
    semePartita = seme;
    ripristinando = true;
    bool ok = CaricaProgetto(progetto, errore);
    if (!ok) { ripristinando = false; return false; }
    std::string ev;
    Testo(d, "evento", ev);
    if (!parziale && ev != eventoPartita) correzioni.push_back("l'evento della partita e' cambiato (gioco modificato?)");

    // 2. I progressi delle altre mappe, poi si rientra nella mappa salvata.
    mappeStato.clear();
    if (j.contains("mappe") && j["mappe"].is_object())
        for (auto it = j["mappe"].begin(); it != j["mappe"].end(); ++it) {
            int m = std::atoi(it.key().c_str());
            if (m < 0 || m >= (int)mondo.mappe.size()) continue;
            StatoMappa sm;
            DaInsieme(it.value(), "tolti", sm.tolti); DaInsieme(it.value(), "aperti", sm.aperti);
            DaInsieme(it.value(), "accesi", sm.accesi); DaInsieme(it.value(), "base", sm.base);
            mappeStato[m] = sm;
        }
    const json& p = j["partita"];
    int mappa = mappaAttuale;
    Num(p, "mappa", mappa);
    std::string nomeMappa;
    Testo(p, "mappaNome", nomeMappa);
    if (!nomeMappa.empty() && (mappa < 0 || mappa >= (int)mondo.mappe.size() || mondo.mappe[mappa].nome != nomeMappa))
        for (size_t m = 0; m < mondo.mappe.size(); m++) if (mondo.mappe[m].nome == nomeMappa) mappa = (int)m;   // mappe riordinate
    if (mappa < 0 || mappa >= (int)mondo.mappe.size()) { correzioni.push_back("la mappa salvata non c'e' piu'"); mappa = mondo.mappaIniziale; }
    Vector2 pos = { -1, -1 };
    const json pe = p.value("eroe", json::object());
    Num(pe, "x", pos.x); Num(pe, "y", pos.y);
    std::string e2;
    if (!EntraMappa(mappa, pos, e2)) { ripristinando = false; errore = "non riesco a rientrare nella mappa: " + e2; return false; }
    ripristinando = false;

    // 3. Sovrapporre lo stato salvato (EntraMappa sistema la posizione d'arrivo: qui si rimette quella esatta).
    if (std::isfinite(pos.x) && std::isfinite(pos.y) && pos.x >= 0 && pos.y >= 0) eroe.pos = pos;
    Num(pe, "vx", eroe.vel.x); Num(pe, "vy", eroe.vel.y);
    int dir = eroe.dir; if (Num(pe, "dir", dir)) eroe.dir = dir >= 0 ? 1 : -1;
    Num(p, "energiaMax", energiaMax); energiaMax = std::max(1, std::min(99, energiaMax));
    Num(p, "energia", energia);
    Num(p, "vite", vite); Num(p, "punteggio", punteggio); Num(p, "monete", monete); Num(p, "gemme", gemme);
    Num(p, "mosse", mosse); Num(p, "tempo", tempo); Num(p, "pieta", pieta);
    pieta = std::max(0.0f, std::min(0.5f, pieta));
    if (p.contains("checkpoint") && p["checkpoint"].is_array() && p["checkpoint"].size() == 2 && p["checkpoint"][0].is_number() && p["checkpoint"][1].is_number())
        checkpoint = { p["checkpoint"][0].get<float>(), p["checkpoint"][1].get<float>() };
    float ora = sfondo.ora;
    if (Num(p, "ora", ora) && ora >= 0 && ora <= 1) sfondo.ora = ora;
    Num(p, "pioggia", pioggia);
    Bool(p, "vinto", vinto); Bool(p, "finito", finito);
    if (p.contains("variabili") && p["variabili"].is_object()) {
        for (auto it = p["variabili"].begin(); it != p["variabili"].end(); ++it)
            if (it.value().is_number() && std::isfinite(it.value().get<double>())) variabili[it.key()] = it.value().get<float>();
    }
    if (p.contains("inventario") && p["inventario"].is_object()) {
        inventario.clear();
        for (auto it = p["inventario"].begin(); it != p["inventario"].end(); ++it)
            if (it.value().is_number_integer() && it.value().get<int>() > 0) inventario[it.key()] = it.value().get<int>();
    }
    if (p.contains("regole") && p["regole"].is_object()) {
        const json& r = p["regole"];
        if (r.contains("timer") && r["timer"].is_array() && r["timer"].size() == timerRegole.size())
            for (size_t k = 0; k < timerRegole.size(); k++) if (r["timer"][k].is_number()) timerRegole[k] = r["timer"][k].get<float>();
        if (r.contains("condizioni") && r["condizioni"].is_array() && r["condizioni"].size() == condizioniRegole.size())
            for (size_t k = 0; k < condizioniRegole.size(); k++) if (r["condizioni"][k].is_boolean()) condizioniRegole[k] = r["condizioni"][k].get<bool>();
    }

    if (!parziale && j.contains("entita") && j["entita"].is_array()) {
        // Le entita' create durante il gioco si tolgono (anche quelle appena create dalle regole "inizio") e si rifanno dal salvataggio.
        entita.erase(std::remove_if(entita.begin(), entita.end(), [](const Entita& e) { return e.uid >= 1000000; }), entita.end());
        std::map<int, int> perUid;
        for (size_t i = 0; i < entita.size(); i++) perUid[entita[i].uid] = (int)i;
        std::set<int> viste;
        int scartate = 0;
        for (const json& x : j["entita"]) {
            if (!x.is_object()) { scartate++; continue; }
            int uid = -1;
            std::string nome;
            Num(x, "uid", uid); Testo(x, "nome", nome);
            Vector2 q = { 0, 0 };
            Num(x, "x", q.x); Num(x, "y", q.y);
            int k = -1;
            if (uid >= 0 && uid < 1000000) {
                auto it = perUid.find(uid);
                if (it == perUid.end() || !UgualeNome(entita[it->second].nome, nome)) { scartate++; continue; }
                k = it->second;
            } else if (uid >= 1000000) {
                k = CreaEntita(nome, q);
                if (k < 0) { scartate++; continue; }
                contatoreCreati--;
                entita[k].uid = uid;
                Num(x, "mappaPorta", entita[k].mappaPorta);
                if (x.contains("dest") && x["dest"].is_array() && x["dest"].size() == 2 && x["dest"][0].is_number() && x["dest"][1].is_number())
                    entita[k].destPorta = { x["dest"][0].get<float>(), x["dest"][1].get<float>() };
            } else { scartate++; continue; }
            viste.insert(entita[k].uid);
            Entita& e = entita[k];
            e.pos = q;
            Num(x, "vx", e.vel.x); Num(x, "vy", e.vel.y); Num(x, "ox", e.origine.x); Num(x, "oy", e.origine.y);
            int dd = e.dir; if (Num(x, "dir", dd)) e.dir = dd >= 0 ? 1 : -1;
            Num(x, "enMax", e.energiaMax); e.energiaMax = std::max(1, std::min(999, e.energiaMax));
            Num(x, "en", e.energia); e.energia = std::min(e.energia, e.energiaMax);
            Bool(x, "att", e.attivo);
            e.morte = 0;
            Num(x, "morte", e.morte); if (!(e.morte >= 0 && e.morte < 10)) e.morte = 0;
            Bool(x, "aT", e.aTerra); Num(x, "tAn", e.tAnim); Num(x, "lamp", e.lampeggio); Num(x, "sp", e.spinta);
            Bool(x, "bl", e.bloccato); Num(x, "tGr", e.tGrido); Num(x, "tCo", e.tContatto);
            Bool(x, "ap", e.aperta); Bool(x, "acc", e.acceso);
            Num(x, "t", e.t); Num(x, "timer", e.timer); Num(x, "velMolt", e.velMolt);
            e.velMolt = std::max(0.0f, std::min(10.0f, e.velMolt));
            Testo(x, "anim", e.anim);
            std::string tinta; Testo(x, "tinta", tinta); HexColore(tinta, e.tinta);
            Num(x, "vet", e.veterano); Num(x, "pm", e.puntiMolt); Num(x, "vm", e.valoreMolt);
            Bool(x, "amico", e.amicoAnnunciato);
            Num(x, "carica", e.carica); Num(x, "forza", e.forza); Num(x, "usi", e.usi); Bool(x, "rotto", e.rotto);
            Num(x, "tRic", e.tRicarica); Num(x, "tSv", e.tSveglio);
            e.carica = L01(e.carica); e.forza = std::max(0.2f, std::min(3.0f, e.forza));
            if (e.mente.attiva && x.contains("mente")) MenteDaJ(x["mente"], e.mente);
            // Senza questi dati (salvataggio vecchio): niente eventi "tocca" finti al primo fotogramma.
            e.toccando = true; e.vicino = true;
            Bool(x, "toc", e.toccando); Bool(x, "vic", e.vicino);
        }
        // Entita' della mappa che il salvataggio non conosce: il gioco e' cambiato, restano come nuove.
        int nuove = 0;
        for (auto& e : entita) if (!viste.count(e.uid)) nuove++;
        if (scartate) correzioni.push_back(std::to_string(scartate) + " elementi salvati non esistono piu' nel gioco");
        if (nuove) correzioni.push_back(std::to_string(nuove) + " elementi nuovi del gioco ripartono da capo");
        Num(p, "creati", contatoreCreati);
        int ap = -1, di = -1;
        Num(p, "appoggio", ap); Num(p, "dialogo", di);
        appoggio = (ap >= 0 && ap < (int)entita.size()) ? ap : -1;
        dialogo = (di >= 0 && di < (int)entita.size()) ? di : -1;
        if (p.contains("notificate") && p["notificate"].is_array()) {
            energiaNotificata.clear();
            for (auto& q : p["notificate"])
                if (q.is_array() && q.size() == 2 && q[0].is_number_integer() && q[1].is_number_integer()) energiaNotificata.insert({ q[0].get<int>(), q[1].get<int>() });
        }
    }
    if (!parziale && j.contains("oggettiBase") && j["oggettiBase"].is_array()) {
        const json& ob = j["oggettiBase"];
        size_t n = std::min(ob.size(), oggetti.size());
        if (ob.size() < oggetti.size() && ob.size() != livello.Oggetti().size()) correzioni.push_back("gli oggetti della mappa sono cambiati");
        for (size_t i = 0; i < n; i++) {
            const json& x = ob[i];
            Oggetto& o = oggetti[i];
            Bool(x, "a", o.attivo); Num(x, "x", o.pos.x); Num(x, "y", o.pos.y); Num(x, "vx", o.vel.x); Num(x, "vy", o.vel.y);
            Num(x, "s", o.stato); Num(x, "n", o.numero); Bool(x, "sc", o.sconfitto); Num(x, "dir", o.dir); Bool(x, "f", o.fisica);
        }
    }
    if (j.contains("sapere")) SapereDaJ(j["sapere"], sapere);
    std::string r;
    Testo(d, "rng", r);
    uint64_t st = 0;
    if (DaEsadecimale64(r, st) && st != 0) rng.s = st;

    // 4. Controlli: niente numeri strani, l'eroe non resta dentro un muro.
    auto Valido = [&](Vector2 v) {
        if (!std::isfinite(v.x) || !std::isfinite(v.y)) return false;
        if (v.x < 0 || v.x > livello.Larghezza() * (float)TILE || v.y < -64 || v.y > livello.Altezza() * (float)TILE) return false;
        return !livello.Solido((int)std::floor(v.x / TILE), (int)std::floor((v.y - 8) / TILE));
    };
    if (!Valido(eroe.pos)) {
        correzioni.push_back("l'eroe era in un punto non valido: riparte dal punto di controllo");
        eroe.pos = Valido(checkpoint) ? checkpoint : livello.Partenza();
        eroe.vel = { 0, 0 };
    }
    if (!Valido(checkpoint)) checkpoint = eroe.pos;
    if (!std::isfinite(eroe.vel.x) || !std::isfinite(eroe.vel.y)) eroe.vel = { 0, 0 };
    for (auto& e : entita) {
        if (!std::isfinite(e.pos.x) || !std::isfinite(e.pos.y)) { e.pos = e.origine; e.vel = { 0, 0 }; }
        if (!std::isfinite(e.vel.x) || !std::isfinite(e.vel.y)) e.vel = { 0, 0 };
    }
    energia = std::max(1, std::min(energiaMax, energia));
    if (vite <= 0 && !finito) vite = 1;
    if (tempo < 0) tempo = 0;
    // Telecamera, fisica fine e proiettili in volo: la partita riprende esattamente da dove era.
    AggiornaTelecamera(1, true);
    proiettili.clear();
    particelle.Svuota();
    mappaVerso = -1; tTransizione = 0;
    if (p.contains("moto") && p["moto"].is_object()) {
        const json& m = p["moto"];
        Bool(m, "aTerra", eroe.aTerra);
        int st = (int)eroe.stato; if (Num(m, "stato", st) && st >= 0 && st < 16) eroe.stato = (StatoEroe)st;
        Num(m, "tStato", eroe.tStato); Num(m, "lampeggio", eroe.lampeggio);
        float cx = camX, cy = camY;
        if (Num(m, "camX", cx) && std::isfinite(cx)) camX = cx;
        if (Num(m, "camY", cy) && std::isfinite(cy)) camY = cy;
        float z = zoom; if (Num(m, "zoom", z) && z > 0.1f && z < 10) zoom = z;
        Num(m, "tSparo", tSparo); Num(m, "tPassoPuzzle", tPassoPuzzle); Num(m, "coyote", coyote); Num(m, "bufferSalto", bufferSalto);
        Num(m, "saltiFatti", saltiFatti); Bool(m, "saltoTagliato", saltoTagliato); Num(m, "tFineScorrimento", tFineScorrimento);
        Bool(m, "fineScorrimento", fineScorrimento); Bool(m, "portaBloccata", portaBloccata); Bool(m, "aTerraPrima", eroeATerraPrima);
    } else {
        // Salvataggio senza questi dati (vecchio o parziale): un attimo di protezione.
        portaBloccata = true;
        eroe.lampeggio = 1.5f;
        eroeATerraPrima = true;
    }
    if (p.contains("proiettili") && p["proiettili"].is_array())
        for (const json& q : p["proiettili"]) {
            if (!q.is_array() || q.size() != 9 || proiettili.size() >= 200) continue;
            bool ok = true;
            for (int k : { 0, 1, 2, 3, 5, 7, 8 }) if (!q[k].is_number()) ok = false;
            if (!ok || !q[4].is_boolean() || !q[6].is_string()) continue;
            Proiettile x;
            x.pos = { q[0].get<float>(), q[1].get<float>() }; x.vel = { q[2].get<float>(), q[3].get<float>() };
            x.amico = q[4].get<bool>(); x.vita = q[5].get<float>();
            x.colore = WHITE; HexColore(q[6].get<std::string>(), x.colore);
            x.disegno = q[7].get<int>(); x.danno = q[8].get<int>();
            if (!std::isfinite(x.pos.x) || !std::isfinite(x.pos.y) || !std::isfinite(x.vel.x) || !std::isfinite(x.vel.y)) continue;
            proiettili.push_back(x);
        }
    messaggio = "Partita caricata";
    tMessaggio = 2;
    for (size_t i = 0; i < correzioni.size(); i++) avviso += (i ? "; " : "") + correzioni[i];
    return true;
}
