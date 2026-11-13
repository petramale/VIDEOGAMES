#include "mondo.h"
#include "livello.h"
#include "sfondo.h"
#include "json.hpp"
#include <sstream>

using nlohmann::json;

const char* NomeGenere(Genere g) {
    switch (g) {
        case Genere::Platform: return "Piattaforme";
        case Genere::OpenWorld: return "Open world";
        case Genere::Sparatutto: return "Sparatutto";
        case Genere::Avventura: return "Avventura";
        case Genere::Labirinto: return "Labirinto";
        case Genere::Puzzle: return "Puzzle";
        default: return "";
    }
}

const char* ChiaveGenere(Genere g) {
    switch (g) {
        case Genere::Platform: return "platform";
        case Genere::OpenWorld: return "openworld";
        case Genere::Sparatutto: return "sparatutto";
        case Genere::Avventura: return "avventura";
        case Genere::Labirinto: return "labirinto";
        case Genere::Puzzle: return "puzzle";
        default: return "";
    }
}

const char* NomeModoTelecamera(ModoTelecamera m) {
    switch (m) {
        case ModoTelecamera::Segui: return "Segue l'eroe";
        case ModoTelecamera::Stanze: return "A stanze";
        case ModoTelecamera::Automatica: return "Scorrimento automatico";
        case ModoTelecamera::Fissa: return "Fissa";
        default: return "";
    }
}

const char* ChiaveModoTelecamera(ModoTelecamera m) {
    switch (m) {
        case ModoTelecamera::Segui: return "segui";
        case ModoTelecamera::Stanze: return "stanze";
        case ModoTelecamera::Automatica: return "automatica";
        case ModoTelecamera::Fissa: return "fissa";
        default: return "";
    }
}

bool SembraJson(const std::string& testo) {
    size_t i = testo.find_first_not_of(" \t\r\n");
    if (i != std::string::npos && testo.compare(i, 3, "\xEF\xBB\xBF") == 0) i += 3; // BOM UTF-8
    return i != std::string::npos && testo[i] == '{';
}

// --- Mappa ------------------------------------------------------------------

void Mappa::Nuova(int w, int h, Vista v) {
    larghezza = (int)Limita((float)w, 8, 1000);
    altezza = (int)Limita((float)h, 6, 1000);
    vista = v;
    caselle.assign(altezza, std::string(larghezza, ' '));
    oggetti.clear();
    decorazioni.clear();
    if (v == Vista::Laterale) {
        for (int y = altezza - 3; y < altezza; y++) caselle[y] = std::string(larghezza, '#');
        partenza = { 2 * TILE + TILE / 2.0f, (altezza - 3) * (float)TILE };
    } else {
        for (int x = 0; x < larghezza; x++) { caselle[0][x] = '#'; caselle[altezza - 1][x] = '#'; }
        for (int y = 0; y < altezza; y++) { caselle[y][0] = '#'; caselle[y][larghezza - 1] = '#'; }
        partenza = { 3 * TILE + TILE / 2.0f, 3 * TILE + TILE - 2.0f };
        sfondoGenerato = false;
    }
}

char Mappa::Casella(int x, int y) const {
    if (y < 0 || y >= (int)caselle.size() || x < 0 || x >= (int)caselle[y].size()) return ' ';
    return caselle[y][x];
}

void Mappa::ImpostaCasella(int x, int y, char c) {
    if (y < 0 || y >= altezza || x < 0 || x >= larghezza) return;
    if ((int)caselle.size() < altezza) caselle.resize(altezza);
    if ((int)caselle[y].size() < larghezza) caselle[y].resize(larghezza, ' ');
    caselle[y][x] = c;
}

void Mappa::Ridimensiona(int w, int h) {
    larghezza = (int)Limita((float)w, 8, 1000);
    altezza = (int)Limita((float)h, 6, 1000);
    caselle.resize(altezza, std::string(larghezza, ' '));
    for (auto& r : caselle) r.resize(larghezza, ' ');
}

// --- JSON --------------------------------------------------------------------

namespace {

std::string Colore(Color c) { return ColoreInTesto(c); }
// Numeri puliti nel JSON (0.35 invece di 0.3499999940395355).
double R(float v) { return std::round((double)v * 1000.0) / 1000.0; }

void LeggiColore(const json& j, const char* k, Color& c) {
    if (j.contains(k) && j[k].is_string()) TestoInColore(j[k].get<std::string>(), c);
}

template <typename T>
void Leggi(const json& j, const char* k, T& v) {
    if (!j.contains(k)) return;
    const json& x = j[k];
    if constexpr (std::is_same<T, std::string>::value) { if (x.is_string()) v = x.get<std::string>(); }
    else if constexpr (std::is_same<T, bool>::value) { if (x.is_boolean()) v = x.get<bool>(); else if (x.is_number()) v = x.get<double>() != 0; }
    else { if (x.is_number()) v = (T)x.get<double>(); }
}

json EroeJson(const AspettoEroe& e) {
    return { { "pelle", Colore(e.pelle) }, { "capelli", Colore(e.capelli) }, { "maglia", Colore(e.maglia) },
             { "mantello", Colore(e.mantello) }, { "pantaloni", Colore(e.pantaloni) }, { "scarpe", Colore(e.scarpe) } };
}

void EroeDaJson(const json& j, AspettoEroe& e) {
    LeggiColore(j, "pelle", e.pelle); LeggiColore(j, "capelli", e.capelli); LeggiColore(j, "maglia", e.maglia);
    LeggiColore(j, "mantello", e.mantello); LeggiColore(j, "pantaloni", e.pantaloni); LeggiColore(j, "scarpe", e.scarpe);
}

json MappaJson(const Mappa& m) {
    json j;
    j["nome"] = m.nome;
    j["larghezza"] = m.larghezza;
    j["altezza"] = m.altezza;
    j["vista"] = m.vista == Vista::Alto ? "alto" : "laterale";
    j["musica"] = m.musica;
    j["scenario"] = { { "tema", m.tema }, { "ora", R(m.ora) }, { "cicloGiorno", m.cicloGiorno }, { "sfondoGenerato", m.sfondoGenerato } };
    if (m.cieloPersonale) j["cielo"] = { { "alto", Colore(m.cieloAlto) }, { "basso", Colore(m.cieloBasso) } };
    json strati = json::array();
    for (auto& s : m.strati) {
        json js = { { "tipo", s.tipo }, { "colore", Colore(s.colore) }, { "parallasseX", R(s.parallasseX) },
                    { "parallasseY", R(s.parallasseY) }, { "altezza", R(s.altezza) }, { "ampiezza", R(s.ampiezza) }, { "seme", R(s.seme) } };
        if (!s.nome.empty()) js["nome"] = s.nome;
        if (!s.immagine.empty()) { js["immagine"] = s.immagine; js["scala"] = R(s.scala); js["ripeti"] = s.ripeti; }
        if (s.vento != 0) js["vento"] = R(s.vento);
        if (s.primoPiano) js["primoPiano"] = true;
        strati.push_back(js);
    }
    j["strati"] = strati;
    const Telecamera& t = m.telecamera;
    j["telecamera"] = { { "modo", ChiaveModoTelecamera(t.modo) }, { "zonaMortaX", R(t.zonaMortaX) }, { "zonaMortaY", R(t.zonaMortaY) },
                        { "anticipo", R(t.anticipo) }, { "velocita", R(t.velocita) }, { "dirX", t.dirX }, { "dirY", t.dirY }, { "zoom", R(t.zoom) } };
    j["partenza"] = { { "x", m.partenza.x }, { "y", m.partenza.y } };
    j["caselle"] = m.caselle;
    json dec = json::array();
    for (auto& d : m.decorazioni) {
        json jd = { { "immagine", d.immagine }, { "x", d.x }, { "y", d.y } };
        if (d.scala != 1) jd["scala"] = d.scala;
        if (d.primoPiano) jd["primoPiano"] = true;
        dec.push_back(jd);
    }
    j["decorazioni"] = dec;
    json ogg = json::array();
    for (auto& o : m.oggetti) {
        json jo = { { "tipo", o.tipo }, { "x", o.x }, { "y", o.y } };
        if (o.numero) jo["numero"] = o.numero;
        if (o.mappa >= 0) { jo["mappa"] = o.mappa; jo["destX"] = o.destX; jo["destY"] = o.destY; }
        ogg.push_back(jo);
    }
    j["oggetti"] = ogg;
    json testi = json::array();
    for (auto& tc : m.testi) testi.push_back({ { "numero", tc.numero }, { "it", tc.it }, { "en", tc.en } });
    j["testi"] = testi;
    return j;
}

bool MappaDaJson(const json& j, Mappa& m, std::string& errore) {
    if (!j.is_object()) { errore = "una mappa non e' un oggetto JSON"; return false; }
    Leggi(j, "nome", m.nome);
    int w = 0, h = 0;
    Leggi(j, "larghezza", w);
    Leggi(j, "altezza", h);
    std::string vista;
    Leggi(j, "vista", vista);
    m.vista = vista == "alto" ? Vista::Alto : Vista::Laterale;
    Leggi(j, "musica", m.musica);
    if (j.contains("scenario") && j["scenario"].is_object()) {
        const json& s = j["scenario"];
        Leggi(s, "tema", m.tema);
        Leggi(s, "ora", m.ora);
        Leggi(s, "cicloGiorno", m.cicloGiorno);
        Leggi(s, "sfondoGenerato", m.sfondoGenerato);
        m.tema = (int)Limita((float)m.tema, 0, (float)Sfondo::Temi().size() - 1);
    }
    if (j.contains("cielo") && j["cielo"].is_object()) {
        m.cieloPersonale = true;
        LeggiColore(j["cielo"], "alto", m.cieloAlto);
        LeggiColore(j["cielo"], "basso", m.cieloBasso);
    }
    m.strati.clear();
    if (j.contains("strati") && j["strati"].is_array())
        for (auto& js : j["strati"]) {
            if (!js.is_object()) continue;
            StratoSfondo s;
            Leggi(js, "nome", s.nome); Leggi(js, "tipo", s.tipo); Leggi(js, "immagine", s.immagine);
            LeggiColore(js, "colore", s.colore);
            Leggi(js, "parallasseX", s.parallasseX); Leggi(js, "parallasseY", s.parallasseY);
            Leggi(js, "altezza", s.altezza); Leggi(js, "ampiezza", s.ampiezza); Leggi(js, "seme", s.seme);
            Leggi(js, "scala", s.scala); Leggi(js, "vento", s.vento); Leggi(js, "ripeti", s.ripeti); Leggi(js, "primoPiano", s.primoPiano);
            m.strati.push_back(s);
        }
    if (j.contains("telecamera") && j["telecamera"].is_object()) {
        const json& t = j["telecamera"];
        std::string modo;
        Leggi(t, "modo", modo);
        for (int i = 0; i < (int)ModoTelecamera::Totale; i++) if (modo == ChiaveModoTelecamera((ModoTelecamera)i)) m.telecamera.modo = (ModoTelecamera)i;
        Leggi(t, "zonaMortaX", m.telecamera.zonaMortaX); Leggi(t, "zonaMortaY", m.telecamera.zonaMortaY);
        Leggi(t, "anticipo", m.telecamera.anticipo); Leggi(t, "velocita", m.telecamera.velocita);
        Leggi(t, "dirX", m.telecamera.dirX); Leggi(t, "dirY", m.telecamera.dirY); Leggi(t, "zoom", m.telecamera.zoom);
        m.telecamera.zoom = Limita(m.telecamera.zoom, 0.5f, 3);
    }
    m.caselle.clear();
    if (j.contains("caselle") && j["caselle"].is_array())
        for (auto& r : j["caselle"]) if (r.is_string()) m.caselle.push_back(r.get<std::string>());
    if (h <= 0) h = (int)m.caselle.size();
    if (w <= 0) for (auto& r : m.caselle) w = std::max(w, (int)r.size());
    if (w < 8 || h < 6) { errore = "la mappa \"" + m.nome + "\" e' troppo piccola (minimo 8x6 caselle)"; return false; }
    if (w > 1000 || h > 1000) { errore = "la mappa \"" + m.nome + "\" e' troppo grande (massimo 1000x1000)"; return false; }
    m.larghezza = w; m.altezza = h;
    m.caselle.resize(h);
    for (auto& r : m.caselle) r.resize(w, ' ');
    if (j.contains("partenza") && j["partenza"].is_object()) { Leggi(j["partenza"], "x", m.partenza.x); Leggi(j["partenza"], "y", m.partenza.y); }
    m.decorazioni.clear();
    if (j.contains("decorazioni") && j["decorazioni"].is_array())
        for (auto& jd : j["decorazioni"]) {
            if (!jd.is_object()) continue;
            Decorazione d;
            Leggi(jd, "immagine", d.immagine); Leggi(jd, "x", d.x); Leggi(jd, "y", d.y);
            Leggi(jd, "scala", d.scala); Leggi(jd, "primoPiano", d.primoPiano);
            m.decorazioni.push_back(d);
        }
    m.oggetti.clear();
    if (j.contains("oggetti") && j["oggetti"].is_array())
        for (auto& jo : j["oggetti"]) {
            if (!jo.is_object()) continue;
            OggettoMappa o;
            Leggi(jo, "tipo", o.tipo); Leggi(jo, "x", o.x); Leggi(jo, "y", o.y); Leggi(jo, "numero", o.numero);
            Leggi(jo, "mappa", o.mappa); Leggi(jo, "destX", o.destX); Leggi(jo, "destY", o.destY);
            if (!o.tipo.empty()) m.oggetti.push_back(o);
        }
    m.testi.clear();
    if (j.contains("testi") && j["testi"].is_array())
        for (auto& jt : j["testi"]) {
            if (!jt.is_object()) continue;
            TestoCartello t;
            Leggi(jt, "numero", t.numero); Leggi(jt, "it", t.it); Leggi(jt, "en", t.en);
            if (t.en.empty()) t.en = t.it;
            m.testi.push_back(t);
        }
    return true;
}

} // namespace

std::string Mondo::Json(bool compatto) const {
    json j;
    j["formato"] = "studio-videogiochi/2";
    j["titolo"] = titolo;
    j["autori"] = autori;
    j["genere"] = ChiaveGenere(genere);
    j["giocatori"] = { { "massimo", giocatoriMax }, { "schermoDiviso", schermoDiviso } };
    j["fisica"] = { { "gravita", fisica.gravita }, { "salto", fisica.salto }, { "velocita", fisica.velocita }, { "molla", fisica.molla } };
    j["eroe"] = EroeJson(eroe);
    j["mappaIniziale"] = mappaIniziale;
    json mj = json::array();
    for (auto& m : mappe) mj.push_back(MappaJson(m));
    j["mappe"] = mj;
    return compatto ? j.dump() : j.dump(1);
}

bool Mondo::DaJson(const std::string& testo, std::string& errore) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { errore = "il file non e' un JSON valido"; return false; }
    Mondo m;
    Leggi(j, "titolo", m.titolo);
    Leggi(j, "autori", m.autori);
    std::string g;
    Leggi(j, "genere", g);
    for (int i = 0; i < (int)Genere::Totale; i++) if (g == ChiaveGenere((Genere)i)) m.genere = (Genere)i;
    if (j.contains("giocatori") && j["giocatori"].is_object()) {
        Leggi(j["giocatori"], "massimo", m.giocatoriMax);
        Leggi(j["giocatori"], "schermoDiviso", m.schermoDiviso);
        m.giocatoriMax = (int)Limita((float)m.giocatoriMax, 1, 4);
    }
    if (j.contains("fisica") && j["fisica"].is_object()) {
        const json& f = j["fisica"];
        Leggi(f, "gravita", m.fisica.gravita); Leggi(f, "salto", m.fisica.salto);
        Leggi(f, "velocita", m.fisica.velocita); Leggi(f, "molla", m.fisica.molla);
    }
    if (j.contains("eroe") && j["eroe"].is_object()) EroeDaJson(j["eroe"], m.eroe);
    Leggi(j, "mappaIniziale", m.mappaIniziale);
    if (!j.contains("mappe") || !j["mappe"].is_array() || j["mappe"].empty()) { errore = "il mondo non ha mappe"; return false; }
    for (auto& jm : j["mappe"]) {
        Mappa mappa;
        if (!MappaDaJson(jm, mappa, errore)) return false;
        m.mappe.push_back(mappa);
    }
    m.mappaIniziale = (int)Limita((float)m.mappaIniziale, 0, (float)m.mappe.size() - 1);
    *this = m;
    return true;
}

// --- Vecchio formato -----------------------------------------------------------

bool Mondo::ImportaVecchio(const std::string& testo, std::string& errore) {
    Progetto p;
    bool impostazioni = p.LeggiImpostazioni(testo);
    std::vector<std::string> righe;
    std::istringstream ss(testo);
    std::string r;
    while (std::getline(ss, r)) {
        if (!r.empty() && r.back() == '\r') r.pop_back();
        if (r.empty() || r[0] == ';') continue;
        righe.push_back(r);
    }
    if (righe.size() < 4) { errore = "il livello deve avere almeno 4 righe"; return false; }
    Mappa m;
    m.nome = impostazioni && !p.titolo.empty() ? p.titolo : "Livello";
    m.altezza = (int)righe.size();
    m.larghezza = 0;
    for (auto& riga : righe) m.larghezza = std::max(m.larghezza, (int)riga.size());
    if (m.larghezza < 8) { errore = "il livello e' troppo stretto"; return false; }
    m.caselle.assign(m.altezza, std::string(m.larghezza, ' '));
    bool haPartenza = false;
    for (int y = 0; y < m.altezza; y++)
        for (int x = 0; x < (int)righe[y].size(); x++) {
            char c = righe[y][x];
            Vector2 centro = { x * TILE + TILE / 2.0f, y * TILE + TILE / 2.0f };
            Vector2 base = { x * TILE + TILE / 2.0f, (y + 1) * TILE * 1.0f };
            TipoOggetto t;
            bool appoggiato;
            if (c == '#' || c == '=' || c == '-' || c == '^' || c == '.') m.caselle[y][x] = c;
            else if (c == 'P') { m.partenza = base; haPartenza = true; }
            else if (OggettoDaCarattere(c, t, appoggiato)) {
                OggettoMappa o;
                o.tipo = ChiaveOggetto(t);
                o.x = appoggiato ? base.x : centro.x;
                o.y = appoggiato ? base.y : centro.y;
                if (c >= '1' && c <= '9') o.numero = c - '0';
                m.oggetti.push_back(o);
            }
        }
    if (!haPartenza) { errore = "manca la partenza (P)"; return false; }
    m.tema = p.tema; m.ora = p.ora; m.musica = p.musica; m.testi = p.testi;
    m.telecamera.zoom = p.zoom;
    switch (p.modello) {
        case Modello::Sparatutto:
            m.telecamera.modo = ModoTelecamera::Automatica;
            m.telecamera.velocita = p.fisica.velocita * 0.35f;
            break;
        case Modello::Labirinto: case Modello::Puzzle:
            m.vista = Vista::Alto;
            m.telecamera.anticipo = 0;
            m.sfondoGenerato = false;
            break;
        default:
            // Come il vecchio motore: segue da vicino in orizzontale, guarda avanti.
            m.telecamera.zonaMortaX = 0;
            m.telecamera.zonaMortaY = 0;
            break;
    }
    if (mappe.empty()) {
        titolo = impostazioni && !p.titolo.empty() ? p.titolo : titolo;
        fisica = p.fisica;
        eroe = p.eroe;
        switch (p.modello) {
            case Modello::Sparatutto: genere = Genere::Sparatutto; break;
            case Modello::Labirinto: genere = Genere::Labirinto; break;
            case Modello::Puzzle: genere = Genere::Puzzle; break;
            default: genere = Genere::Platform; break;
        }
    }
    mappe.push_back(m);
    return true;
}

bool Mondo::Carica(const std::string& testo, std::string& errore) {
    if (SembraJson(testo)) return DaJson(testo, errore);
    Mondo m;
    m.titolo.clear();   // lo da' la riga delle impostazioni, se c'e'
    std::vector<std::string> livelli = DividiLivelli(testo);
    if (livelli.empty()) { errore = "il testo e' vuoto"; return false; }
    for (size_t i = 0; i < livelli.size(); i++) {
        if (!m.ImportaVecchio(livelli[i], errore)) { errore = "livello " + std::to_string(i + 1) + ": " + errore; return false; }
        // Livelli in sequenza: la bandiera o la porta d'uscita portano al livello successivo.
        if (i > 0)
            for (auto& o : m.mappe[i - 1].oggetti)
                if (o.tipo == "bandiera" || (o.tipo == "porta" && o.mappa < 0)) o.mappa = (int)i;
    }
    *this = m;
    return true;
}

std::string Mondo::Controlla() const {
    if (mappe.empty()) return "il mondo non ha mappe";
    for (size_t i = 0; i < mappe.size(); i++) {
        const Mappa& m = mappe[i];
        if (m.partenza.x < 0 || m.partenza.y < 0 || m.partenza.x > m.larghezza * TILE || m.partenza.y > m.altezza * TILE)
            return "mappa \"" + m.nome + "\": la partenza e' fuori dalla mappa";
        for (auto& o : m.oggetti) {
            TipoOggetto t;
            if (!TipoDaNome(o.tipo, t)) return "mappa \"" + m.nome + "\": oggetto sconosciuto \"" + o.tipo + "\"";
            if (o.mappa >= (int)mappe.size()) return "mappa \"" + m.nome + "\": una porta porta a una mappa che non esiste";
        }
    }
    return "";
}
