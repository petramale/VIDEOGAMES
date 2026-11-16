#include "progetto_gioco.h"
#include "raccolte.h"
#include "sfondo.h"
#include "json.hpp"
#include <algorithm>
#include <cctype>

using nlohmann::json;

const char* NomeStile(StileGrafico s) { return s == StileGrafico::Disegnato ? "Disegnato in alta definizione" : "Pixel art fine"; }

const char* NomeComportamento(TipoComportamento t) {
    switch (t) {
        case TipoComportamento::Giocatore: return "Eroe (giocatore)";
        case TipoComportamento::Fermo: return "Fermo";
        case TipoComportamento::Pattuglia: return "Pattuglia avanti e indietro";
        case TipoComportamento::Insegue: return "Insegue l'eroe";
        case TipoComportamento::Vola: return "Vola (ondeggia)";
        case TipoComportamento::Spara: return "Torretta: spara";
        case TipoComportamento::Boss: return "Boss";
        case TipoComportamento::Parla: return "Parla (personaggio amico)";
        default: return "";
    }
}

const char* ChiaveComportamento(TipoComportamento t) {
    switch (t) {
        case TipoComportamento::Giocatore: return "giocatore";
        case TipoComportamento::Fermo: return "fermo";
        case TipoComportamento::Pattuglia: return "pattuglia";
        case TipoComportamento::Insegue: return "insegue";
        case TipoComportamento::Vola: return "vola";
        case TipoComportamento::Spara: return "spara";
        case TipoComportamento::Boss: return "boss";
        case TipoComportamento::Parla: return "parla";
        default: return "";
    }
}

const char* NomeFunzione(FunzioneOggetto f) {
    switch (f) {
        case FunzioneOggetto::Decorazione: return "Decorazione";
        case FunzioneOggetto::Raccogli: return "Da raccogliere (punti)";
        case FunzioneOggetto::Cura: return "Cura (cuori)";
        case FunzioneOggetto::Chiave: return "Chiave";
        case FunzioneOggetto::Porta: return "Porta (si apre con...)";
        case FunzioneOggetto::Molla: return "Molla";
        case FunzioneOggetto::Cassa: return "Cassa da spingere";
        case FunzioneOggetto::Interruttore: return "Interruttore / leva";
        case FunzioneOggetto::PiattaformaMobile: return "Piattaforma mobile";
        case FunzioneOggetto::Pericolo: return "Pericolo (fa male)";
        case FunzioneOggetto::Arrivo: return "Arrivo (vittoria)";
        case FunzioneOggetto::Proiettile: return "Proiettile";
        default: return "";
    }
}

const char* ChiaveFunzione(FunzioneOggetto f) {
    switch (f) {
        case FunzioneOggetto::Decorazione: return "decorazione";
        case FunzioneOggetto::Raccogli: return "raccogli";
        case FunzioneOggetto::Cura: return "cura";
        case FunzioneOggetto::Chiave: return "chiave";
        case FunzioneOggetto::Porta: return "porta";
        case FunzioneOggetto::Molla: return "molla";
        case FunzioneOggetto::Cassa: return "cassa";
        case FunzioneOggetto::Interruttore: return "interruttore";
        case FunzioneOggetto::PiattaformaMobile: return "piattaforma";
        case FunzioneOggetto::Pericolo: return "pericolo";
        case FunzioneOggetto::Arrivo: return "arrivo";
        case FunzioneOggetto::Proiettile: return "proiettile";
        default: return "";
    }
}

const std::vector<std::string>& TipiEvento() {
    static const std::vector<std::string> v = { "inizio", "tocca", "raccoglie", "colpisce", "sconfitto", "energia", "vicino",
                                                "ferito", "salta", "tasto", "clic", "joypad", "ogni", "variabile" };
    return v;
}
const std::vector<std::string>& NomiEvento() {
    static const std::vector<std::string> v = { "inizia la mappa", "tocca", "raccoglie", "colpisce", "viene sconfitto",
                                                "ha energia sotto", "si avvicina a", "viene ferito", "salta", "tasto premuto",
                                                "clic del mouse", "pulsante del joypad", "ogni tot secondi", "variabile cambia" };
    return v;
}
const std::vector<std::string>& TipiAzione() {
    static const std::vector<std::string> v = { "suona", "musica", "ferma_musica", "messaggio", "aumenta", "imposta", "punti", "cuori",
                                                "energia", "vai_mappa", "crea", "togli", "anima", "velocita", "salta", "spara",
                                                "colore", "apri", "scuoti", "spingi", "vittoria", "game_over" };
    return v;
}
const std::vector<std::string>& NomiAzione() {
    static const std::vector<std::string> v = { "suona l'effetto", "metti la musica", "ferma la musica", "mostra il messaggio",
                                                "aumenta la variabile", "imposta la variabile", "dai punti", "cuori all'eroe (+/-)",
                                                "energia (+/-)", "vai alla mappa", "crea", "togli di mezzo", "cambia animazione",
                                                "velocita' x", "fai saltare", "spara", "colora", "apri le porte", "scuoti lo schermo",
                                                "spingi indietro l'eroe", "vittoria", "game over" };
    return v;
}

// --- Fotogrammi e disegni ------------------------------------------------------------

bool Fotogramma::Vuoto() const {
    for (auto& c : px) if (c.a) return false;
    return true;
}

void Disegno::Nuovo(int larghezza, int altezza, float dens) {
    Invalida();
    w = larghezza; h = altezza; densita = dens;
    animazioni.clear();
    Animazione a;
    a.fotogrammi.resize(1);
    a.fotogrammi[0].Nuovo(w, h);
    animazioni.push_back(a);
}

int Disegno::TrovaAnimazione(const std::string& nome) const {
    for (size_t i = 0; i < animazioni.size(); i++) if (animazioni[i].nome == nome) return (int)i;
    // senza badare a maiuscole
    for (size_t i = 0; i < animazioni.size(); i++) {
        const std::string& a = animazioni[i].nome;
        if (a.size() == nome.size() && std::equal(a.begin(), a.end(), nome.begin(), [](char x, char y) { return std::tolower((unsigned char)x) == std::tolower((unsigned char)y); }))
            return (int)i;
    }
    return -1;
}

int Disegno::FotogrammaAl(int anim, float t) const {
    if (anim < 0 || anim >= (int)animazioni.size()) return 0;
    const Animazione& a = animazioni[anim];
    int n = (int)a.fotogrammi.size();
    if (n <= 1) return 0;
    int f = (int)(t * std::max(0.1f, a.fps));
    return a.ripeti ? f % n : std::min(f, n - 1);
}

Texture2D Disegno::Texture(int anim, int fot) const {
    if (anim < 0 || anim >= (int)animazioni.size()) return Texture2D{};
    const Animazione& a = animazioni[anim];
    if (fot < 0 || fot >= (int)a.fotogrammi.size()) return Texture2D{};
    if (!IsWindowReady()) return Texture2D{};
    if (cache.size() != animazioni.size()) { Invalida(); cache.resize(animazioni.size()); }
    auto& c = cache[anim];
    if (c.size() != a.fotogrammi.size()) {
        for (auto& t : c) if (t.id) UnloadTexture(t);
        c.assign(a.fotogrammi.size(), Texture2D{});
    }
    if (c[fot].id == 0) {
        const Fotogramma& f = a.fotogrammi[fot];
        if (f.px.size() != (size_t)f.w * f.h || f.w <= 0) return Texture2D{};
        Image img{ (void*)f.px.data(), f.w, f.h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
        c[fot] = LoadTextureFromImage(img);
        SetTextureFilter(c[fot], liscio ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    }
    return c[fot];
}

void Disegno::Invalida() const {
    if (IsWindowReady())
        for (auto& a : cache) for (auto& t : a) if (t.id) UnloadTexture(t);
    cache.clear();
    versione++;
}

void Disegno::DisegnaBase(int anim, float t, Vector2 base, bool specchia, Color tinta, float scala) const {
    if (animazioni.empty()) return;
    if (anim < 0 || anim >= (int)animazioni.size()) anim = 0;
    const Animazione& a = animazioni[anim];
    if (a.fotogrammi.empty()) return;
    int f = FotogrammaAl(anim, t);
    Texture2D tex = Texture(anim, f);
    if (tex.id == 0) return;
    const Fotogramma& fo = a.fotogrammi[f];
    float ww = fo.w / densita * scala, hh = fo.h / densita * scala;
    Rectangle src = { 0, 0, (float)fo.w * (specchia ? -1 : 1), (float)fo.h };
    DrawTexturePro(tex, src, { base.x - ww / 2, base.y - hh, ww, hh }, { 0, 0 }, 0, tinta);
}

void Disegno::DisegnaIn(Rectangle r, int anim, float t, Color tinta) const {
    if (animazioni.empty()) return;
    if (anim < 0 || anim >= (int)animazioni.size()) anim = 0;
    const Animazione& a = animazioni[anim];
    if (a.fotogrammi.empty()) return;
    int f = FotogrammaAl(anim, t);
    Texture2D tex = Texture(anim, f);
    if (tex.id == 0) return;
    const Fotogramma& fo = a.fotogrammi[f];
    float s = std::min(r.width / fo.w, r.height / fo.h);
    if (!liscio && s >= 1) s = std::floor(s);
    float ww = fo.w * s, hh = fo.h * s;
    DrawTexturePro(tex, { 0, 0, (float)fo.w, (float)fo.h }, { r.x + (r.width - ww) / 2, r.y + (r.height - hh) / 2, ww, hh }, { 0, 0 }, 0, tinta);
}

void Disegno::Ridimensiona(int nw, int nh) {
    Invalida();
    for (auto& a : animazioni)
        for (auto& f : a.fotogrammi) {
            Fotogramma n;
            n.Nuovo(nw, nh);
            // il disegno resta centrato in orizzontale e appoggiato in basso
            int ox = (nw - f.w) / 2, oy = nh - f.h;
            for (int y = 0; y < f.h; y++)
                for (int x = 0; x < f.w; x++) n.Metti(x + ox, y + oy, f.Prendi(x, y));
            f = n;
        }
    w = nw; h = nh;
}

Disegno::~Disegno() { Invalida(); }
Disegno::Disegno(const Disegno& d) : w(d.w), h(d.h), densita(d.densita), liscio(d.liscio), animazioni(d.animazioni) {}
Disegno& Disegno::operator=(const Disegno& d) {
    if (this == &d) return *this;
    Invalida();
    w = d.w; h = d.h; densita = d.densita; liscio = d.liscio; animazioni = d.animazioni;
    return *this;
}

// --- Base64 -------------------------------------------------------------------------

namespace {

std::string Base64(const std::string& dati) {
    if (dati.empty()) return "";
    int n = 0;
    char* t = EncodeDataBase64((const unsigned char*)dati.data(), (int)dati.size(), &n);
    if (!t) return "";
    std::string s(t, t + std::max(0, n));
    while (!s.empty() && s.back() == '\0') s.pop_back();
    MemFree(t);
    return s;
}

std::string DaBase64(const std::string& testo) {
    if (testo.empty()) return "";
    int n = 0;
    unsigned char* d = DecodeDataBase64((const unsigned char*)testo.c_str(), &n);
    if (!d) return "";
    std::string s((const char*)d, (size_t)std::max(0, n));
    MemFree(d);
    return s;
}

} // namespace

std::string FotogrammaInBase64(const Fotogramma& f) {
    if (f.w <= 0 || f.h <= 0 || f.px.size() != (size_t)f.w * f.h) return "";
    Image img{ (void*)f.px.data(), f.w, f.h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    int n = 0;
    unsigned char* png = ExportImageToMemory(img, ".png", &n);
    if (!png) return "";
    std::string s((const char*)png, (size_t)n);
    MemFree(png);
    return Base64(s);
}

bool FotogrammaDaBase64(const std::string& testo, Fotogramma& f) {
    std::string png = DaBase64(testo);
    if (png.empty()) return false;
    Image img = LoadImageFromMemory(".png", (const unsigned char*)png.data(), (int)png.size());
    if (!img.data) return false;
    f = FotogrammaDaImmagine(img, { 0, 0, (float)img.width, (float)img.height });
    UnloadImage(img);
    return true;
}

Fotogramma FotogrammaDaImmagine(const Image& img, Rectangle parte) {
    Fotogramma f;
    f.Nuovo((int)parte.width, (int)parte.height);
    Color* c = LoadImageColors(img);
    if (!c) return f;
    for (int y = 0; y < f.h; y++)
        for (int x = 0; x < f.w; x++) {
            int sx = (int)parte.x + x, sy = (int)parte.y + y;
            if (sx >= 0 && sy >= 0 && sx < img.width && sy < img.height) f.Metti(x, y, c[sy * img.width + sx]);
        }
    UnloadImageColors(c);
    return f;
}

bool FotogrammaDaRighe(const std::vector<std::string>& righe, const std::map<char, Color>& palette, Fotogramma& f, std::string& errore) {
    if (righe.empty()) { errore = "fotogramma senza righe"; return false; }
    size_t w = 0;
    for (auto& r : righe) w = std::max(w, r.size());
    if (w == 0 || w > 256 || righe.size() > 256) { errore = "fotogramma troppo grande (massimo 256x256)"; return false; }
    f.Nuovo((int)w, (int)righe.size());
    for (size_t y = 0; y < righe.size(); y++)
        for (size_t x = 0; x < righe[y].size(); x++) {
            char c = righe[y][x];
            if (c == '.' || c == ' ') continue;
            auto p = palette.find(c);
            if (p != palette.end()) f.Metti((int)x, (int)y, p->second);
        }
    return true;
}

// --- JSON -----------------------------------------------------------------------------

namespace {

double R(float v) { return std::round((double)v * 1000.0) / 1000.0; }

template <typename T>
void Leggi(const json& j, const char* k, T& v) {
    if (!j.is_object() || !j.contains(k)) return;
    const json& x = j[k];
    if constexpr (std::is_same<T, std::string>::value) { if (x.is_string()) v = x.get<std::string>(); }
    else if constexpr (std::is_same<T, bool>::value) { if (x.is_boolean()) v = x.get<bool>(); else if (x.is_number()) v = x.get<double>() != 0; }
    else { if (x.is_number()) v = (T)x.get<double>(); }
}

void LeggiColore(const json& j, const char* k, Color& c) {
    if (j.is_object() && j.contains(k) && j[k].is_string()) TestoInColore(j[k].get<std::string>(), c);
}

std::map<char, Color> LeggiPalette(const json& j) {
    std::map<char, Color> p;
    if (!j.is_object()) return p;
    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.key().empty() || !it.value().is_string()) continue;
        Color c = BLANK;
        std::string s = it.value().get<std::string>();
        if (s == "trasparente" || s == "none") continue;
        if (TestoInColore(s, c)) {
            if (s.size() == 9) {   // #rrggbbaa
                int a = 255;
                sscanf(s.c_str() + 7, "%2x", &a);
                c.a = (unsigned char)a;
            }
            p[it.key()[0]] = c;
        }
    }
    return p;
}

} // namespace

json DisegnoJson(const Disegno& d) {
    json j = { { "w", d.w }, { "h", d.h }, { "densita", R(d.densita) } };
    if (d.liscio) j["liscio"] = true;
    json an = json::array();
    for (auto& a : d.animazioni) {
        json ja = { { "nome", a.nome }, { "fps", R(a.fps) }, { "ripeti", a.ripeti } };
        json fs = json::array();
        for (auto& f : a.fotogrammi) fs.push_back(FotogrammaInBase64(f));
        ja["fotogrammi"] = fs;
        an.push_back(ja);
    }
    j["animazioni"] = an;
    return j;
}

// Accetta i fotogrammi in PNG base64 e anche nel formato a righe di Claude (con "palette").
bool DisegnoDaJson(const json& j, Disegno& d, std::string& errore) {
    if (!j.is_object()) { errore = "disegno non valido"; return false; }
    Disegno n;
    Leggi(j, "w", n.w); Leggi(j, "h", n.h); Leggi(j, "densita", n.densita); Leggi(j, "liscio", n.liscio);
    n.densita = Limita(n.densita, 0.25f, 16);
    std::map<char, Color> palette;
    if (j.contains("palette")) palette = LeggiPalette(j["palette"]);
    auto LeggiAnimazione = [&](const json& ja, Animazione& a) -> bool {
        Leggi(ja, "nome", a.nome); Leggi(ja, "fps", a.fps); Leggi(ja, "ripeti", a.ripeti);
        a.fps = Limita(a.fps, 0.5f, 60);
        std::map<char, Color> pal = palette;
        if (ja.contains("palette")) for (auto& p : LeggiPalette(ja["palette"])) pal[p.first] = p.second;
        if (!ja.contains("fotogrammi") || !ja["fotogrammi"].is_array()) return true;
        for (auto& jf : ja["fotogrammi"]) {
            Fotogramma f;
            if (jf.is_string()) { if (!FotogrammaDaBase64(jf.get<std::string>(), f)) continue; }
            else if (jf.is_array()) {
                std::vector<std::string> righe;
                for (auto& r : jf) if (r.is_string()) righe.push_back(r.get<std::string>());
                if (!FotogrammaDaRighe(righe, pal, f, errore)) return false;
            } else continue;
            a.fotogrammi.push_back(f);
        }
        return true;
    };
    if (j.contains("animazioni") && j["animazioni"].is_array()) {
        for (auto& ja : j["animazioni"]) {
            if (!ja.is_object()) continue;
            Animazione a;
            if (!LeggiAnimazione(ja, a)) return false;
            if (!a.fotogrammi.empty()) n.animazioni.push_back(a);
        }
    } else if (j.contains("fotogrammi")) {   // vecchio sprite di Claude: una sola animazione
        Animazione a;
        a.nome = "fermo";
        if (!LeggiAnimazione(j, a)) return false;
        if (!a.fotogrammi.empty()) n.animazioni.push_back(a);
    }
    if (n.animazioni.empty()) { errore = "il disegno non ha fotogrammi"; return false; }
    // Tutti i fotogrammi della stessa misura: quella del primo (o w/h se indicati).
    int fw = n.animazioni[0].fotogrammi[0].w, fh = n.animazioni[0].fotogrammi[0].h;
    if (!j.contains("w")) n.w = fw;
    if (!j.contains("h")) n.h = fh;
    for (auto& a : n.animazioni)
        for (auto& f : a.fotogrammi)
            if (f.w != n.w || f.h != n.h) {
                Fotogramma g;
                g.Nuovo(n.w, n.h);
                int ox = (n.w - f.w) / 2, oy = n.h - f.h;
                for (int y = 0; y < f.h; y++) for (int x = 0; x < f.w; x++) g.Metti(x + ox, y + oy, f.Prendi(x, y));
                f = g;
            }
    d = n;
    return true;
}

json PersonaggioJson(const PersonaggioDef& p) {
    const Comportamento& c = p.comportamento;
    json jc = { { "tipo", ChiaveComportamento(c.tipo) }, { "velocita", R(c.velocita) }, { "salto", R(c.salto) }, { "energia", c.energia },
                { "danno", c.danno }, { "vista", R(c.vista) }, { "attaccoOgni", R(c.attaccoOgni) }, { "gravita", c.gravita },
                { "calpestabile", c.calpestabile }, { "punti", c.punti } };
    if (!c.proiettile.empty()) jc["proiettile"] = c.proiettile;
    if (!c.dialogo.empty()) jc["dialogo"] = c.dialogo;
    json j = { { "nome", p.nome }, { "comportamento", jc }, { "disegno", DisegnoJson(p.disegno) } };
    if (!p.autore.empty()) j["autore"] = p.autore;
    if (!p.suoni.empty()) j["suoni"] = p.suoni;
    return j;
}

bool PersonaggioDaJson(const json& j, PersonaggioDef& p, std::string& errore) {
    if (!j.is_object()) { errore = "personaggio non valido"; return false; }
    Leggi(j, "nome", p.nome); Leggi(j, "autore", p.autore);
    if (p.nome.empty()) { errore = "il personaggio non ha un nome"; return false; }
    if (j.contains("comportamento") && j["comportamento"].is_object()) {
        const json& jc = j["comportamento"];
        Comportamento& c = p.comportamento;
        std::string t;
        Leggi(jc, "tipo", t);
        for (int i = 0; i < (int)TipoComportamento::Totale; i++) if (t == ChiaveComportamento((TipoComportamento)i)) c.tipo = (TipoComportamento)i;
        Leggi(jc, "velocita", c.velocita); Leggi(jc, "salto", c.salto); Leggi(jc, "energia", c.energia); Leggi(jc, "danno", c.danno);
        Leggi(jc, "vista", c.vista); Leggi(jc, "attaccoOgni", c.attaccoOgni); Leggi(jc, "proiettile", c.proiettile);
        Leggi(jc, "gravita", c.gravita); Leggi(jc, "calpestabile", c.calpestabile); Leggi(jc, "dialogo", c.dialogo); Leggi(jc, "punti", c.punti);
        c.velocita = Limita(c.velocita, 0, 600); c.energia = (int)Limita((float)c.energia, 1, 999); c.attaccoOgni = Limita(c.attaccoOgni, 0.1f, 60);
    }
    if (j.contains("suoni") && j["suoni"].is_object())
        for (auto it = j["suoni"].begin(); it != j["suoni"].end(); ++it) if (it.value().is_string()) p.suoni[it.key()] = it.value().get<std::string>();
    if (j.contains("disegno")) return DisegnoDaJson(j["disegno"], p.disegno, errore);
    errore = "il personaggio \"" + p.nome + "\" non ha il disegno";
    return false;
}

json OggettoJson(const OggettoDef& o) {
    json j = { { "nome", o.nome }, { "funzione", ChiaveFunzione(o.funzione) }, { "valore", o.valore }, { "solido", o.solido },
               { "gravita", o.gravita }, { "velocita", R(o.velocita) }, { "distanza", R(o.distanza) }, { "disegno", DisegnoJson(o.disegno) } };
    if (!o.autore.empty()) j["autore"] = o.autore;
    if (!o.richiede.empty()) j["richiede"] = o.richiede;
    if (!o.suono.empty()) j["suono"] = o.suono;
    if (!o.variabile.empty()) j["variabile"] = o.variabile;
    return j;
}

bool OggettoDaJson(const json& j, OggettoDef& o, std::string& errore) {
    if (!j.is_object()) { errore = "oggetto non valido"; return false; }
    Leggi(j, "nome", o.nome); Leggi(j, "autore", o.autore);
    if (o.nome.empty()) { errore = "l'oggetto non ha un nome"; return false; }
    std::string f;
    Leggi(j, "funzione", f);
    for (int i = 0; i < (int)FunzioneOggetto::Totale; i++) if (f == ChiaveFunzione((FunzioneOggetto)i)) o.funzione = (FunzioneOggetto)i;
    Leggi(j, "valore", o.valore); Leggi(j, "richiede", o.richiede); Leggi(j, "solido", o.solido); Leggi(j, "gravita", o.gravita);
    Leggi(j, "velocita", o.velocita); Leggi(j, "distanza", o.distanza); Leggi(j, "suono", o.suono); Leggi(j, "variabile", o.variabile);
    if (j.contains("disegno")) return DisegnoDaJson(j["disegno"], o.disegno, errore);
    errore = "l'oggetto \"" + o.nome + "\" non ha il disegno";
    return false;
}

json StratoJson(const StratoSfondo& s) {
    json js = { { "tipo", s.tipo }, { "colore", ColoreInTesto(s.colore) }, { "parallasseX", R(s.parallasseX) }, { "parallasseY", R(s.parallasseY) },
                { "altezza", R(s.altezza) }, { "ampiezza", R(s.ampiezza) }, { "seme", R(s.seme) } };
    if (!s.nome.empty()) js["nome"] = s.nome;
    if (!s.immagine.empty()) { js["immagine"] = s.immagine; js["scala"] = R(s.scala); js["ripeti"] = s.ripeti; }
    if (s.vento != 0) js["vento"] = R(s.vento);
    if (s.primoPiano) js["primoPiano"] = true;
    return js;
}

StratoSfondo StratoDaJson(const json& js) {
    StratoSfondo s;
    Leggi(js, "nome", s.nome); Leggi(js, "tipo", s.tipo); Leggi(js, "immagine", s.immagine);
    LeggiColore(js, "colore", s.colore);
    Leggi(js, "parallasseX", s.parallasseX); Leggi(js, "parallasseY", s.parallasseY);
    Leggi(js, "altezza", s.altezza); Leggi(js, "ampiezza", s.ampiezza); Leggi(js, "seme", s.seme);
    Leggi(js, "scala", s.scala); Leggi(js, "vento", s.vento); Leggi(js, "ripeti", s.ripeti); Leggi(js, "primoPiano", s.primoPiano);
    return s;
}

json ScenarioJson(const Scenario& s) {
    json j = { { "nome", s.nome }, { "tema", s.tema }, { "ora", R(s.ora) }, { "cicloGiorno", s.cicloGiorno }, { "sfondoGenerato", s.sfondoGenerato },
               { "pioggia", s.pioggia } };
    if (!s.autore.empty()) j["autore"] = s.autore;
    if (s.cieloPersonale) j["cielo"] = { { "alto", ColoreInTesto(s.cieloAlto) }, { "basso", ColoreInTesto(s.cieloBasso) } };
    json st = json::array();
    for (auto& x : s.strati) st.push_back(StratoJson(x));
    j["strati"] = st;
    return j;
}

bool ScenarioDaJson(const json& j, Scenario& s, std::string& errore) {
    if (!j.is_object()) { errore = "scenario non valido"; return false; }
    Leggi(j, "nome", s.nome); Leggi(j, "autore", s.autore);
    if (s.nome.empty()) { errore = "lo scenario non ha un nome"; return false; }
    Leggi(j, "tema", s.tema); Leggi(j, "ora", s.ora); Leggi(j, "cicloGiorno", s.cicloGiorno); Leggi(j, "sfondoGenerato", s.sfondoGenerato);
    Leggi(j, "pioggia", s.pioggia);
    s.tema = (int)Limita((float)s.tema, 0, (float)Sfondo::Temi().size() - 1);
    s.ora = Limita(s.ora, 0, 1);
    if (j.contains("cielo") && j["cielo"].is_object()) {
        s.cieloPersonale = true;
        LeggiColore(j["cielo"], "alto", s.cieloAlto); LeggiColore(j["cielo"], "basso", s.cieloBasso);
    }
    s.strati.clear();
    if (j.contains("strati") && j["strati"].is_array()) for (auto& js : j["strati"]) if (js.is_object()) s.strati.push_back(StratoDaJson(js));
    return true;
}

json BranoJson(const Brano& b) {
    json j = { { "nome", b.nome }, { "bpm", R(b.bpm) }, { "battute", b.battute } };
    if (!b.autore.empty()) j["autore"] = b.autore;
    if (!b.file.empty()) { j["file"] = b.file; j["dati"] = Base64(b.dati); }
    json tr = json::array();
    for (auto& t : b.tracce) {
        json jt = { { "nome", t.nome }, { "strumento", t.strumento }, { "volume", R(t.volume) }, { "batteria", t.batteria } };
        json note = json::array();
        for (auto& n : t.note) note.push_back({ R(n.inizio), R(n.durata), n.altezza });
        jt["note"] = note;
        tr.push_back(jt);
    }
    j["tracce"] = tr;
    return j;
}

bool BranoDaJson(const json& j, Brano& b, std::string& errore) {
    if (!j.is_object()) { errore = "brano non valido"; return false; }
    Leggi(j, "nome", b.nome); Leggi(j, "autore", b.autore); Leggi(j, "bpm", b.bpm); Leggi(j, "battute", b.battute);
    if (b.nome.empty()) { errore = "il brano non ha un nome"; return false; }
    b.bpm = Limita(b.bpm, 30, 300);
    b.battute = (int)Limita((float)b.battute, 1, 128);
    Leggi(j, "file", b.file);
    std::string d;
    Leggi(j, "dati", d);
    b.dati = DaBase64(d);
    b.tracce.clear();
    if (j.contains("tracce") && j["tracce"].is_array())
        for (auto& jt : j["tracce"]) {
            if (!jt.is_object()) continue;
            TracciaBrano t;
            Leggi(jt, "nome", t.nome); Leggi(jt, "strumento", t.strumento); Leggi(jt, "volume", t.volume); Leggi(jt, "batteria", t.batteria);
            t.strumento = (int)Limita((float)t.strumento, 0, 4);
            t.volume = Limita(t.volume, 0, 1);
            if (jt.contains("note") && jt["note"].is_array())
                for (auto& jn : jt["note"]) {
                    NotaBrano n;
                    if (jn.is_array() && jn.size() >= 3 && jn[0].is_number() && jn[1].is_number() && jn[2].is_number()) {
                        n.inizio = jn[0].get<float>(); n.durata = jn[1].get<float>(); n.altezza = jn[2].get<int>();
                    } else if (jn.is_object()) {
                        Leggi(jn, "inizio", n.inizio); Leggi(jn, "durata", n.durata); Leggi(jn, "altezza", n.altezza);
                    } else continue;
                    if (n.durata <= 0 || n.inizio < 0) continue;
                    n.altezza = (int)Limita((float)n.altezza, 12, 108);
                    t.note.push_back(n);
                }
            b.tracce.push_back(t);
        }
    return true;
}

json EffettoJson(const EffettoSuono& e) {
    json j = { { "nome", e.nome }, { "onda", e.onda }, { "f0", R(e.f0) }, { "f1", R(e.f1) }, { "durata", R(e.durata) }, { "volume", R(e.volume) },
               { "duty", R(e.duty) }, { "vibrato", R(e.vibrato) }, { "vibratoHz", R(e.vibratoHz) }, { "rumore", R(e.rumore) } };
    if (!e.autore.empty()) j["autore"] = e.autore;
    if (!e.file.empty()) { j["file"] = e.file; j["dati"] = Base64(e.dati); }
    return j;
}

bool EffettoDaJson(const json& j, EffettoSuono& e, std::string& errore) {
    if (!j.is_object()) { errore = "effetto non valido"; return false; }
    Leggi(j, "nome", e.nome); Leggi(j, "autore", e.autore);
    if (e.nome.empty()) { errore = "l'effetto non ha un nome"; return false; }
    Leggi(j, "onda", e.onda); Leggi(j, "f0", e.f0); Leggi(j, "f1", e.f1); Leggi(j, "durata", e.durata); Leggi(j, "volume", e.volume);
    Leggi(j, "duty", e.duty); Leggi(j, "vibrato", e.vibrato); Leggi(j, "vibratoHz", e.vibratoHz); Leggi(j, "rumore", e.rumore);
    e.onda = (int)Limita((float)e.onda, 0, 4);
    e.f0 = Limita(e.f0, 20, 8000); e.f1 = Limita(e.f1, 20, 8000);
    e.durata = Limita(e.durata, 0.01f, 5); e.volume = Limita(e.volume, 0, 1); e.duty = Limita(e.duty, 0.05f, 0.95f);
    e.rumore = Limita(e.rumore, 0, 1);
    Leggi(j, "file", e.file);
    std::string d;
    Leggi(j, "dati", d);
    e.dati = DaBase64(d);
    return true;
}

json RegolaJson(const Regola& r) {
    json j = { { "chi", r.chi }, { "evento", r.evento } };
    if (!r.attiva) j["attiva"] = false;
    if (!r.conChi.empty()) j["conChi"] = r.conChi;
    if (!r.variabile.empty()) { j["variabile"] = r.variabile; j["confronto"] = r.confronto; j["valore"] = R(r.valore); }
    else if (r.valore != 0) j["valore"] = R(r.valore);
    json az = json::array();
    for (auto& a : r.azioni) {
        json ja = { { "tipo", a.tipo } };
        if (!a.testo.empty()) ja["testo"] = a.testo;
        if (a.numero != 0) ja["numero"] = R(a.numero);
        az.push_back(ja);
    }
    j["azioni"] = az;
    return j;
}

bool RegolaDaJson(const json& j, Regola& r, std::string& errore) {
    if (!j.is_object()) { errore = "regola non valida"; return false; }
    Leggi(j, "attiva", r.attiva); Leggi(j, "chi", r.chi); Leggi(j, "evento", r.evento); Leggi(j, "conChi", r.conChi);
    Leggi(j, "variabile", r.variabile); Leggi(j, "confronto", r.confronto); Leggi(j, "valore", r.valore);
    const auto& ev = TipiEvento();
    if (std::find(ev.begin(), ev.end(), r.evento) == ev.end()) { errore = "evento sconosciuto \"" + r.evento + "\""; return false; }
    static const std::vector<std::string> confronti = { ">=", "<=", "==", "!=", ">", "<" };
    if (std::find(confronti.begin(), confronti.end(), r.confronto) == confronti.end()) r.confronto = ">=";
    r.azioni.clear();
    if (j.contains("azioni") && j["azioni"].is_array())
        for (auto& ja : j["azioni"]) {
            AzioneRegola a;
            Leggi(ja, "tipo", a.tipo); Leggi(ja, "testo", a.testo); Leggi(ja, "numero", a.numero);
            const auto& ta = TipiAzione();
            if (std::find(ta.begin(), ta.end(), a.tipo) == ta.end()) { errore = "azione sconosciuta \"" + a.tipo + "\""; return false; }
            r.azioni.push_back(a);
        }
    return true;
}

std::string ProgettoGioco::Json(bool compatto) const {
    json j;
    j["formato"] = "studio-videogiochi/gioco-1";
    j["stile"] = stile == StileGrafico::Disegnato ? "disegnato" : "pixel";
    j["eroe"] = eroe;
    j["mondo"] = json::parse(mondo.Json(true));
    json a = json::array();
    for (auto& p : personaggi) a.push_back(PersonaggioJson(p));
    j["personaggi"] = a;
    a = json::array(); for (auto& o : oggetti) a.push_back(OggettoJson(o)); j["oggetti"] = a;
    a = json::array(); for (auto& s : scenari) a.push_back(ScenarioJson(s)); j["scenari"] = a;
    a = json::array();
    for (auto& c : caselle) a.push_back({ { "carattere", std::string(1, c.carattere) }, { "nome", c.nome }, { "disegno", DisegnoJson(c.disegno) } });
    j["caselle"] = a;
    a = json::array(); for (auto& b : brani) a.push_back(BranoJson(b)); j["brani"] = a;
    a = json::array(); for (auto& e : effetti) a.push_back(EffettoJson(e)); j["effetti"] = a;
    a = json::array(); for (auto& r : regole) a.push_back(RegolaJson(r)); j["regole"] = a;
    json v = json::object();
    for (auto& x : variabili) v[x.first] = R(x.second);
    j["variabili"] = v;
    j["suoniEventi"] = suoniEventi;
    a = json::array();
    for (auto& im : immagini) a.push_back({ { "nome", im.first }, { "dati", Base64(im.second) } });
    j["immagini"] = a;
    return compatto ? j.dump() : j.dump(1);
}

bool ProgettoGioco::DaJson(const std::string& testo, std::string& errore) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { errore = "il file non e' un JSON valido"; return false; }
    if (!j.contains("mondo")) { errore = "manca il mondo del gioco"; return false; }
    ProgettoGioco p;
    std::string st;
    Leggi(j, "stile", st);
    p.stile = st == "disegnato" ? StileGrafico::Disegnato : StileGrafico::PixelFine;
    Leggi(j, "eroe", p.eroe);
    auto Elenco = [&](const char* k) -> const json* { return j.contains(k) && j[k].is_array() ? &j[k] : nullptr; };
    if (auto a = Elenco("personaggi")) for (auto& x : *a) { PersonaggioDef d; if (!PersonaggioDaJson(x, d, errore)) return false; p.personaggi.push_back(d); }
    if (auto a = Elenco("oggetti")) for (auto& x : *a) { OggettoDef d; if (!OggettoDaJson(x, d, errore)) return false; p.oggetti.push_back(d); }
    if (auto a = Elenco("scenari")) for (auto& x : *a) { Scenario d; if (!ScenarioDaJson(x, d, errore)) return false; p.scenari.push_back(d); }
    if (auto a = Elenco("caselle"))
        for (auto& x : *a) {
            CasellaDef c;
            std::string car;
            Leggi(x, "carattere", car); Leggi(x, "nome", c.nome);
            if (car.size() != 1) continue;
            c.carattere = car[0];
            if (!x.contains("disegno") || !DisegnoDaJson(x["disegno"], c.disegno, errore)) return false;
            p.caselle.push_back(c);
        }
    if (auto a = Elenco("brani")) for (auto& x : *a) { Brano d; if (!BranoDaJson(x, d, errore)) return false; p.brani.push_back(d); }
    if (auto a = Elenco("effetti")) for (auto& x : *a) { EffettoSuono d; if (!EffettoDaJson(x, d, errore)) return false; p.effetti.push_back(d); }
    if (auto a = Elenco("regole")) for (auto& x : *a) { Regola d; if (!RegolaDaJson(x, d, errore)) return false; p.regole.push_back(d); }
    if (j.contains("variabili") && j["variabili"].is_object())
        for (auto it = j["variabili"].begin(); it != j["variabili"].end(); ++it) if (it.value().is_number()) p.variabili[it.key()] = it.value().get<float>();
    if (j.contains("suoniEventi") && j["suoniEventi"].is_object())
        for (auto it = j["suoniEventi"].begin(); it != j["suoniEventi"].end(); ++it) if (it.value().is_string()) p.suoniEventi[it.key()] = it.value().get<std::string>();
    if (auto a = Elenco("immagini"))
        for (auto& x : *a) {
            std::string n, d;
            Leggi(x, "nome", n); Leggi(x, "dati", d);
            if (!n.empty()) p.immagini.push_back({ n, DaBase64(d) });
        }
    // Il mondo si controlla conoscendo i nomi della libreria.
    std::vector<std::string> nomi;
    for (auto& x : p.personaggi) nomi.push_back(x.nome);
    for (auto& x : p.oggetti) nomi.push_back(x.nome);
    ImpostaNomiLibreria(nomi);
    if (!p.mondo.DaJson(j["mondo"].dump(), errore)) return false;
    *this = p;
    return true;
}

bool ProgettoGioco::Carica(const std::string& testo, std::string& errore) {
    if (SembraJson(testo)) {
        json j = json::parse(testo, nullptr, false);
        if (!j.is_discarded() && j.is_object() && j.contains("mondo")) return DaJson(testo, errore);
    }
    // Un mondo (JSON) o un vecchio livello: diventa un gioco con la libreria di base.
    Mondo m;
    if (!m.Carica(testo, errore)) return false;
    ProgettoGioco p = NuovoProgetto(m.genere, StileGrafico::PixelFine, m.titolo);
    p.mondo = m;
    p.eroe.clear();   // l'eroe del vecchio mondo e' quello disegnato dal motore
    *this = p;
    return true;
}

std::string ProgettoGioco::Controlla() const {
    std::vector<std::string> nomi;
    for (auto& x : personaggi) nomi.push_back(x.nome);
    for (auto& x : oggetti) nomi.push_back(x.nome);
    ImpostaNomiLibreria(nomi);
    std::string e = mondo.Controlla();
    if (!e.empty()) return e;
    if (!eroe.empty() && !Personaggio(eroe)) return "l'eroe \"" + eroe + "\" non e' nella libreria dei personaggi";
    for (auto& m : mondo.mappe) {
        if (!m.scenarioLibreria.empty() && !TrovaScenario(m.scenarioLibreria)) return "la mappa \"" + m.nome + "\" usa lo scenario \"" + m.scenarioLibreria + "\" che non c'e'";
        if (!m.brano.empty() && !TrovaBrano(m.brano)) return "la mappa \"" + m.nome + "\" usa il brano \"" + m.brano + "\" che non c'e'";
    }
    return "";
}

namespace {
bool Uguale(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}
}

PersonaggioDef* ProgettoGioco::Personaggio(const std::string& nome) {
    for (auto& p : personaggi) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
const PersonaggioDef* ProgettoGioco::Personaggio(const std::string& nome) const {
    for (auto& p : personaggi) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
OggettoDef* ProgettoGioco::Oggetto(const std::string& nome) {
    for (auto& p : oggetti) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
const OggettoDef* ProgettoGioco::Oggetto(const std::string& nome) const {
    for (auto& p : oggetti) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
const Scenario* ProgettoGioco::TrovaScenario(const std::string& nome) const {
    for (auto& p : scenari) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
const CasellaDef* ProgettoGioco::Casella(char c) const {
    for (auto& p : caselle) if (p.carattere == c) return &p;
    return nullptr;
}
const Brano* ProgettoGioco::TrovaBrano(const std::string& nome) const {
    for (auto& p : brani) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}
const EffettoSuono* ProgettoGioco::TrovaEffetto(const std::string& nome) const {
    for (auto& p : effetti) if (Uguale(p.nome, nome)) return &p;
    return nullptr;
}

std::string ProgettoGioco::NomeLibero(const std::string& base) const {
    auto Usato = [&](const std::string& n) {
        return Personaggio(n) || Oggetto(n) || TrovaScenario(n) || TrovaBrano(n) || TrovaEffetto(n);
    };
    if (!Usato(base)) return base;
    for (int i = 2; i < 1000; i++) {
        std::string n = base + " " + std::to_string(i);
        if (!Usato(n)) return n;
    }
    return base + " nuovo";
}

void Brano::Nuovo() {
    bpm = 120; battute = 8;
    tracce.clear();
    TracciaBrano m; m.nome = "Melodia"; m.strumento = 0; m.volume = 0.45f;
    TracciaBrano b; b.nome = "Basso"; b.strumento = 1; b.volume = 0.6f;
    TracciaBrano a; a.nome = "Accordi"; a.strumento = 3; a.volume = 0.3f;
    TracciaBrano d; d.nome = "Batteria"; d.strumento = 4; d.volume = 0.5f; d.batteria = true;
    tracce = { m, b, a, d };
}

// --- Nuovo progetto -------------------------------------------------------------------

namespace {

Mondo MondoIniziale(Genere g, const std::string& titolo) {
    Mondo w;
    w.titolo = titolo.empty() ? "Il mio gioco" : titolo;
    w.genere = g;
    Mappa m;
    m.nome = "Inizio";
    switch (g) {
        case Genere::OpenWorld: case Genere::Avventura: case Genere::Labirinto: case Genere::Puzzle:
            m.Nuova(g == Genere::OpenWorld ? 60 : 40, g == Genere::OpenWorld ? 40 : 24, Vista::Alto);
            m.telecamera.modo = g == Genere::Avventura ? ModoTelecamera::Stanze : ModoTelecamera::Segui;
            break;
        case Genere::Sparatutto:
            m.Nuova(160, 17, Vista::Laterale);
            for (int x = 0; x < m.larghezza; x++) m.caselle[m.altezza - 3][x] = m.caselle[m.altezza - 2][x] = m.caselle[m.altezza - 1][x] = ' ';
            m.telecamera.modo = ModoTelecamera::Automatica;
            m.telecamera.velocita = 40;
            m.partenza = { 48, 120 };
            break;
        default:
            m.Nuova(100, 20, Vista::Laterale);
            break;
    }
    w.mappe.push_back(m);
    return w;
}

} // namespace

ProgettoGioco NuovoProgetto(Genere g, StileGrafico stile, const std::string& titolo) {
    ProgettoGioco p;
    p.stile = stile;
    p.mondo = MondoIniziale(g, titolo);
    Raccolta base = CreaRaccolta(0, stile);   // la raccolta "Base": eroe, nemici, oggetti, scenario, musica, suoni
    p.personaggi = base.personaggi;
    p.oggetti = base.oggetti;
    p.scenari = base.scenari;
    p.caselle = base.caselle;
    p.brani = base.brani;
    p.effetti = base.effetti;
    if (g == Genere::Sparatutto) {
        Raccolta spazio = CreaRaccolta(IndiceRaccolta("Spazio"), stile);
        for (auto& x : spazio.personaggi) if (!p.Personaggio(x.nome)) p.personaggi.push_back(x);
        for (auto& x : spazio.oggetti) if (!p.Oggetto(x.nome)) p.oggetti.push_back(x);
    }
    p.eroe = p.personaggi.empty() ? "" : p.personaggi[0].nome;
    if (g == Genere::Sparatutto) p.eroe.clear();   // nello sparatutto l'eroe e' l'astronave del motore
    if (!p.scenari.empty() && p.mondo.mappe[0].vista == Vista::Laterale) p.mondo.mappe[0].scenarioLibreria = p.scenari[0].nome;
    if (!p.brani.empty()) p.mondo.mappe[0].brano = p.brani[0].nome;
    p.variabili["monete"] = 0;
    return p;
}
