// Pezzi comuni dei giochi arcade: disegno, scritte, particelle, suoni sintetizzati, regista adattivo.
#include "arcade.h"
#include "audio.h"
#include "ui.h"
#include "sfondo.h"
#include "rlgl.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

namespace arcade {

// --- caso visivo ------------------------------------------------------------------------------
static uint64_t gVis = 0x2545F4914F6CDD1Dull;
float Vis() { gVis ^= gVis >> 12; gVis ^= gVis << 25; gVis ^= gVis >> 27; return ((gVis * 2685821657736338717ull) >> 40) / 16777216.0f; }
float Vis(float a, float b) { return a + (b - a) * Vis(); }

std::string Num(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.17g", v);
    return b;
}

Color ColoreDi(const nlohmann::json& j, const char* k, Color d) {
    if (!j.is_object() || !j.contains(k) || !j[k].is_string()) return d;
    std::string t = j[k].get<std::string>();
    if (t.size() != 7 || t[0] != '#') return d;
    unsigned v = (unsigned)std::strtoul(t.c_str() + 1, nullptr, 16);
    return Color{ (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v, 255 };
}
std::string Esa(Color c) { char b[8]; std::snprintf(b, sizeof b, "#%02x%02x%02x", c.r, c.g, c.b); return b; }
float NumDi(const nlohmann::json& j, const char* k, float d) { return j.is_object() && j.contains(k) && j[k].is_number() ? j[k].get<float>() : d; }
int IntDi(const nlohmann::json& j, const char* k, int d) { return j.is_object() && j.contains(k) && j[k].is_number() ? (int)std::lround(j[k].get<double>()) : d; }
std::string TestoDi(const nlohmann::json& j, const char* k, const std::string& d) { return j.is_object() && j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : d; }

// --- regista ----------------------------------------------------------------------------------
void Regista::Osserva(float dt, float xn, float yn, bool spara) {
    float k = std::min(1.0f, dt * 0.15f);   // memoria di qualche decina di secondi
    abitX += (xn - abitX) * k;
    abitY += (yn - abitY) * k;
    fuocoMedio += ((spara ? 1.0f : 0.0f) - fuocoMedio) * k;
}
void Regista::VitaPersa() {
    vitePerse++;
    sfida = std::max(0.6f, sfida - 0.12f);   // chi perde una vita trova nemici un po' piu' clementi
}
void Regista::TappaFinita(float secondi, float atteso) {
    tappeFinite++;
    tempoTappe += secondi;
    // veloce e preciso: la sfida sale; lento o impreciso: scende piano
    float bravura = 0.5f * std::min(1.5f, atteso / std::max(1.0f, secondi)) + 0.5f * (Precisione() / 0.5f);
    sfida += (bravura - 1.0f) * 0.12f + 0.03f;
    sfida = std::max(0.6f, std::min(1.6f, sfida));
}
std::string Regista::Json() const {
    std::string s = "{";
    auto c = [&](const char* n, float v, bool ultimo = false) { s += std::string("\"") + n + "\":" + Num(v) + (ultimo ? "" : ","); };
    c("sfida", sfida); c("sparati", colpiSparati); c("aSegno", colpiAndatiASegno); c("vitePerse", vitePerse);
    c("tappe", tappeFinite); c("tempoTappe", tempoTappe); c("abitX", abitX); c("abitY", abitY);
    c("fuoco", fuocoMedio); c("rischio", rischio, true);
    return s + "}";
}
}  // namespace arcade

#include "json.hpp"
namespace arcade {
void Regista::DaJson(const std::string& t) {
    auto j = nlohmann::json::parse(t, nullptr, false);
    if (!j.is_object()) return;
    auto g = [&](const char* n, float& v) { if (j.contains(n) && j[n].is_number()) v = j[n].get<float>(); };
    g("sfida", sfida); g("sparati", colpiSparati); g("aSegno", colpiAndatiASegno); g("vitePerse", vitePerse);
    g("tappe", tappeFinite); g("tempoTappe", tempoTappe); g("abitX", abitX); g("abitY", abitY);
    g("fuoco", fuocoMedio); g("rischio", rischio);
}

// --- dati del progetto ------------------------------------------------------------------------
nlohmann::json Unisci(const nlohmann::json& a, const nlohmann::json& b) {
    if (!a.is_object() || !b.is_object()) return b.is_null() ? a : (a.is_null() || a.type() == b.type() || (a.is_number() && b.is_number()) ? b : a);
    nlohmann::json r = a;
    for (auto it = b.begin(); it != b.end(); ++it) r[it.key()] = a.contains(it.key()) ? Unisci(a[it.key()], it.value()) : it.value();
    return r;
}
void GiocoArcade::Configura(const nlohmann::json& dati, const ProgettoGioco* p) {
    progetto = p;
    cfg = Unisci(Predefiniti(), dati.is_object() ? dati : nlohmann::json::object());
    ImpostaSuoni(cfg.contains("suoni") ? cfg["suoni"] : nlohmann::json(), p);
}
Brano GiocoArcade::BranoDi(const std::string& nome) const {
    if (progetto) if (const Brano* b = progetto->TrovaBrano(nome)) return *b;
    for (auto& b : BraniPredefiniti()) if (b.nome == nome) return b;
    Brano vuoto; vuoto.nome = nome; return vuoto;
}

// --- grafica e sfondi propri ------------------------------------------------------------------
const Disegno* GiocoArcade::GraficaDi(const nlohmann::json& elem) const {
    if (!progetto || !elem.is_object()) return nullptr;
    std::string n = TestoDi(elem, "grafica", "");
    if (n.empty()) return nullptr;
    if (const PersonaggioDef* p = progetto->Personaggio(n)) return p->disegno.animazioni.empty() ? nullptr : &p->disegno;
    if (const OggettoDef* o = progetto->Oggetto(n)) return o->disegno.animazioni.empty() ? nullptr : &o->disegno;
    return nullptr;
}

bool GiocoArcade::DisegnaGrafica(const nlohmann::json& elem, Vector2 centro, float altezza, const std::string& anim, float t,
                                 bool specchia, Color tinta, float rot) const {
    const Disegno* d = GraficaDi(elem);
    if (!d) return false;
    int a = -1;
    size_t i = 0;
    while (a < 0 && i <= anim.size()) {
        size_t j = anim.find('|', i);
        if (j == std::string::npos) j = anim.size();
        a = d->TrovaAnimazione(anim.substr(i, j - i));
        i = j + 1;
    }
    if (a < 0) a = d->TrovaAnimazione("fermo");
    if (a < 0) a = 0;
    const Animazione& an = d->animazioni[a];
    if (an.fotogrammi.empty()) return false;
    int f = d->FotogrammaAl(a, t);
    Texture2D tex = d->Texture(a, f);
    if (tex.id == 0) return false;
    const Fotogramma& fo = an.fotogrammi[f];
    if (fo.h <= 0 || fo.w <= 0) return false;
    float k = altezza / fo.h, w = fo.w * k, h = altezza;
    float dx = an.ox * k * (specchia ? -1 : 1), dy = an.oy * k;
    Rectangle src = { 0, 0, (float)fo.w * (specchia ? -1 : 1), (float)fo.h };
    DrawTexturePro(tex, src, { centro.x + dx, centro.y + dy, w, h }, { w / 2, h / 2 }, rot, tinta);
    return true;
}

bool GiocoArcade::HaSfondo(const nlohmann::json& elem) const {
    if (!progetto || !elem.is_object()) return false;
    std::string n = TestoDi(elem, "sfondo", "");
    return !n.empty() && progetto->TrovaScenario(n) != nullptr;
}

bool GiocoArcade::DisegnaSfondo(const nlohmann::json& elem, float camX) {
    if (!HaSfondo(elem)) return false;
    std::string n = TestoDi(elem, "sfondo", "");
    const Scenario* s = progetto->TrovaScenario(n);
    if (!sfondoProprio) sfondoProprio = std::make_shared<Sfondo>();
    Sfondo& sf = *sfondoProprio;
    if (sfondoProprioNome != n) {
        sfondoProprioNome = n;
        sf.tema = s->tema; sf.ora = s->ora; sf.generato = s->sfondoGenerato;
        sf.cieloPersonale = s->cieloPersonale; sf.cieloAlto = s->cieloAlto; sf.cieloBasso = s->cieloBasso;
        sf.strati = s->strati;
    }
    sf.Aggiorna(GetFrameTime());
    // Lo scenario e' disegnato per uno schermo di VW x VH: qui si ingrandisce a tutto il gioco.
    rlDrawRenderBatchActive();
    rlPushMatrix();
    rlScalef((float)L() / VW, (float)A() / VH, 1);
    sf.Disegna(camX * VW / (float)L(), 0);
    sf.DisegnaPrimoPiano(camX * VW / (float)L(), 0);
    rlDrawRenderBatchActive();
    rlPopMatrix();
    return true;
}

// --- disegno ----------------------------------------------------------------------------------
Image DipingiImmagine(int l, int a, const std::function<void(K&)>& f) {
    Tela t(l, a);
    K c(t);
    f(c);
    return t.Immagine();
}
Texture2D Dipingi(int l, int a, const std::function<void(K&)>& f, bool mipmap) {
    Image img = DipingiImmagine(l, a, f);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (mipmap) { GenTextureMipmaps(&tex); SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR); }
    else SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}
void Sprite(const Texture2D& t, float x, float y, float s, float rot, Color c, bool flipX) {
    if (!t.id) return;
    Rectangle src = { 0, 0, (float)(flipX ? -t.width : t.width), (float)t.height };
    Rectangle dst = { x, y, t.width * s, t.height * s };
    DrawTexturePro(t, src, dst, { t.width * s / 2, t.height * s / 2 }, rot, c);
}
static Texture2D gAlone{};
void Bagliore(float x, float y, float r, Color c) {
    if (!gAlone.id) {
        Image img = GenImageColor(128, 128, BLANK);
        for (int j = 0; j < 128; j++)
            for (int i = 0; i < 128; i++) {
                float d = std::hypot(i - 63.5f, j - 63.5f) / 63.5f;
                float a = d >= 1 ? 0 : std::pow(1 - d, 2.2f);
                ImageDrawPixel(&img, i, j, Color{ 255, 255, 255, (unsigned char)(a * 255) });
            }
        gAlone = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(gAlone, TEXTURE_FILTER_BILINEAR);
    }
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(gAlone, { 0, 0, 128, 128 }, { x - r, y - r, r * 2, r * 2 }, { 0, 0 }, 0, c);
    EndBlendMode();
}
void Scritta(const std::string& t, float x, float y, int dim, Color c, int allinea, bool ombra) {
    bool forte = dim >= 22;
    int w = forte ? Ui::LarghezzaForte(t, dim) : Ui::Larghezza(t, dim);
    float px = allinea < 0 ? x : allinea == 0 ? x - w / 2.0f : x - w;
    float py = y - dim * 0.55f;
    if (ombra) {
        float o = std::max(2.0f, dim / 14.0f);
        Color k = { 0, 0, 0, (unsigned char)(c.a * 0.7f) };
        if (forte) Ui::TestoForte(t, px + o, py + o, k, dim); else Ui::Testo(t, px + o, py + o, k, dim);
    }
    if (forte) Ui::TestoForte(t, px, py, c, dim); else Ui::Testo(t, px, py, c, dim);
}
int LarghezzaPixel(const std::string& t, int scala) { return MeasureText(t.c_str(), 10 * scala); }
void ScrittaPixel(const std::string& t, float x, float y, int scala, Color c, int allinea) {
    int w = LarghezzaPixel(t, scala);
    float px = allinea < 0 ? x : allinea == 0 ? x - w / 2.0f : x - w;
    DrawText(t.c_str(), (int)px + scala, (int)y + scala, 10 * scala, Color{ 18, 10, 8, c.a });
    DrawText(t.c_str(), (int)px, (int)y, 10 * scala, c);
}

// --- particelle -------------------------------------------------------------------------------
void Particelle::Esplosione(Vector2 c, Color col, int n, float vel, float raggio, float durata) {
    for (int i = 0; i < n; i++) {
        float a = Vis(0, 6.2832f), s = vel * Vis(0.2f, 1);
        Particella p;
        p.p = c; p.v = { std::cos(a) * s, std::sin(a) * s };
        p.durata = p.vita = durata * Vis(0.6f, 1.2f);
        p.r = raggio * Vis(0.5f, 1.2f);
        p.c = col;
        p.attrito = 0.93f;
        Aggiungi(p);
    }
}
void Particelle::Aggiorna(float dt) {
    for (auto& p : v) {
        p.vita -= dt;
        float k = std::pow(p.attrito, dt * 60);
        p.v.x *= k; p.v.y = p.v.y * k + p.gravita * dt;
        p.p.x += p.v.x * dt; p.p.y += p.v.y * dt;
    }
    size_t j = 0;
    for (size_t i = 0; i < v.size(); i++) if (v[i].vita > 0) v[j++] = v[i];
    v.resize(j);
}
void Particelle::Disegna() const {
    for (int fase = 0; fase < 2; fase++) {
        if (fase == 1) BeginBlendMode(BLEND_ADDITIVE);
        for (auto& p : v) {
            if (p.additiva != (fase == 1)) continue;
            float k = std::max(0.0f, p.vita / p.durata);
            Color c = p.c; c.a = (unsigned char)(c.a * std::min(1.0f, k * 1.5f));
            float r = p.r * (0.4f + 0.6f * k);
            if (p.forma == 1) DrawRectangle((int)(p.p.x - r), (int)(p.p.y - r), (int)std::max(1.0f, r * 2), (int)std::max(1.0f, r * 2), c);
            else if (p.forma == 2) DrawLineEx(p.p, { p.p.x - p.v.x * 0.03f, p.p.y - p.v.y * 0.03f }, std::max(1.0f, r), c);
            else DrawCircleV(p.p, r, c);
        }
        if (fase == 1) EndBlendMode();
    }
}

// --- suoni ------------------------------------------------------------------------------------
float Onda(int tipo, float fase) {
    float f = fase - std::floor(fase);
    switch (tipo) {
        case 0: return f < 0.5f ? 1.0f : -1.0f;
        case 1: return 4 * std::fabs(f - 0.5f) - 1;
        case 2: return 2 * f - 1;
        default: return std::sin(f * 6.2831853f);
    }
}
static uint32_t gRum = 22222;
float Rumore() { gRum = gRum * 1664525u + 1013904223u; return (gRum >> 8) / 8388608.0f - 1; }
float Inviluppo(float t, float attacco, float durata, float curva) {
    if (t < 0 || t > durata) return 0;
    if (t < attacco) return t / attacco;
    return std::pow(1 - (t - attacco) / std::max(0.0001f, durata - attacco), curva);
}
struct VociSuono { Sound base{}; Sound alias[3]{}; int prossima = 0; };
static std::map<std::string, VociSuono> gSuoni;
void CreaSuono(const std::string& nome, float durata, const std::function<float(float)>& f) {
    if (gSuoni.count(nome) || !IsAudioDeviceReady()) return;
    const int sr = 44100;
    int n = (int)(durata * sr);
    std::vector<short> pcm(n);
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        float v = f(t);
        if (i > n - 200) v *= (n - i) / 200.0f;   // niente clic alla fine
        v = std::tanh(v * 1.2f) * 0.92f;          // morbida saturazione: niente distorsione dura
        pcm[i] = (short)(v * 32000);
    }
    Wave w{};
    w.frameCount = (unsigned)n; w.sampleRate = sr; w.sampleSize = 16; w.channels = 1; w.data = pcm.data();
    VociSuono vs;
    vs.base = LoadSoundFromWave(w);
    for (auto& a : vs.alias) a = LoadSoundAlias(vs.base);
    gSuoni[nome] = vs;
}
bool gSilenzio = false;
static std::map<std::string, EffettoSuono> gSostituiti;
void ImpostaSuoni(const nlohmann::json& suoni, const ProgettoGioco* p) {
    gSostituiti.clear();
    if (!p || !suoni.is_object()) return;
    for (auto it = suoni.begin(); it != suoni.end(); ++it)
        if (it.value().is_string() && !it.value().get<std::string>().empty())
            if (const EffettoSuono* e = p->TrovaEffetto(it.value().get<std::string>())) gSostituiti[it.key()] = *e;
}
void Suona(const std::string& nome, float volume, float tono) {
    if (gAudio.muto || gSilenzio) return;
    auto so = gSostituiti.find(nome);
    if (so != gSostituiti.end()) { gAudio.SuonaEffetto(so->second, volume); return; }
    auto it = gSuoni.find(nome);
    if (it == gSuoni.end()) return;
    VociSuono& v = it->second;
    Sound& s = v.prossima == 0 ? v.base : v.alias[v.prossima - 1];
    v.prossima = (v.prossima + 1) % 4;
    SetSoundVolume(s, volume * gAudio.volumeEffetti * 0.8f);
    SetSoundPitch(s, tono);
    PlaySound(s);
}
static std::string gBranoAttuale;
void SuonaMusica(const Brano& b) {
    if (gSilenzio) return;
    if (b.tracce.empty() && b.file.empty()) { FermaMusica(); return; }
    if (gBranoAttuale == b.nome) return;
    gBranoAttuale = b.nome;
    gAudio.SuonaBrano(b);
}
void FermaMusica() { gBranoAttuale.clear(); gAudio.FermaBranoPersonale(); }

}  // namespace arcade

// --- Controllo dei dati e modifiche di Claude ------------------------------------------------------

namespace arcade {

void Sistema(const nlohmann::json& schema, nlohmann::json& dati) {
    using nlohmann::json;
    if (!dati.is_object()) dati = json::object();
    auto SistemaElemento = [&](const json& cat, json& e) {
        if (!e.is_object()) e = json::object();
        for (auto& c : cat["campi"]) {
            std::string k = c.value("chiave", ""), tipo = c.value("tipo", "");
            if (!e.contains(k)) continue;
            json& v = e[k];
            if (tipo == "numero") {
                if (!v.is_number()) { e.erase(k); continue; }
                double x = v.get<double>(), mi = c.value("min", -1e9), ma = c.value("max", 1e9);
                v = std::max(mi, std::min(ma, x));
            } else if (tipo == "scelta" || tipo == "rif") {
                json voci = json::array();
                if (tipo == "scelta") voci = c["voci"];
                else if (dati.contains(c.value("da", "")) && dati[c.value("da", "")].is_array())
                    for (auto& t : dati[c.value("da", "")]) voci.push_back(t.is_object() && t.contains("nome") ? t["nome"] : json(""));
                bool vuoto = tipo == "rif" && c.contains("vuoto");
                if (v.is_string()) {
                    int trovato = vuoto ? -1 : 0;
                    for (int i = 0; i < (int)voci.size(); i++) if (voci[i].is_string() && voci[i].get<std::string>() == v.get<std::string>()) trovato = i;
                    v = trovato;
                } else if (!v.is_number()) v = vuoto ? -1 : 0;
                int i = v.get<int>(), n = (int)voci.size();
                if (i < 0) v = vuoto ? -1 : 0; else if (i >= n) v = std::max(0, n - 1);
            } else if (tipo == "vero") { if (!v.is_boolean()) v = v.is_number() ? v.get<double>() != 0 : false; }
            else if (!v.is_string()) e.erase(k);
        }
    };
    // prima gli elenchi (i rimandi dipendono dalla loro lunghezza), poi i campi
    for (auto& cat : schema) {
        std::string k = cat.value("chiave", "");
        if (!dati.contains(k)) continue;
        if (cat.value("lista", false)) {
            if (!dati[k].is_array()) { dati.erase(k); continue; }
            int mn = cat.value("min", 0), mx = cat.value("max", 999);
            json& a = dati[k];
            while ((int)a.size() > mx) a.erase(a.end() - 1);
            while ((int)a.size() < mn && !a.empty()) a.push_back(a.back());
        }
    }
    for (auto& cat : schema) {
        std::string k = cat.value("chiave", "");
        if (!dati.contains(k)) continue;
        if (cat.value("lista", false)) { for (auto& e : dati[k]) SistemaElemento(cat, e); }
        else SistemaElemento(cat, dati[k]);
    }
}

bool ModificaArcade(ProgettoGioco& p, const nlohmann::json& a, std::string& esito) {
    using nlohmann::json;
    if (!p.EArcade()) { esito = "questo non e' un gioco arcade"; return false; }
    auto g = Crea(p.arcade);
    if (!g) { esito = "tipo di gioco sconosciuto"; return false; }
    json schema = g->Schema(), predef = g->Predefiniti();
    json d = json::parse(p.arcadeDati.empty() ? std::string("{}") : p.arcadeDati, nullptr, false);
    if (!d.is_object()) d = json::object();
    d = Unisci(predef, d);
    std::string tipo = a.value("tipo", "");
    if (tipo == "arcade") {
        if (!a.contains("dati") || !a["dati"].is_object()) { esito = "mancano i dati"; return false; }
        d = Unisci(d, a["dati"]);
        esito = "gioco arcade modificato";
    } else {
        std::string k = a.value("categoria", "");
        const json* cat = nullptr;
        for (auto& c : schema) if (c.value("chiave", "") == k) cat = &c;
        if (!cat) { esito = "categoria sconosciuta: " + k; return false; }
        if (!cat->value("lista", false)) {
            if (!a.contains("elemento") || !a["elemento"].is_object()) { esito = "manca l'elemento"; return false; }
            d[k] = Unisci(d[k], a["elemento"]);
            esito = cat->value("nome", k) + " modificato";
        } else {
            json& arr = d[k];
            int n = (int)arr.size(), i = a.contains("indice") && a["indice"].is_number() ? a["indice"].get<int>() : -1;
            int mn = cat->value("min", 0), mx = cat->value("max", 999);
            std::string el = cat->value("elemento", std::string("elemento"));
            if (a.value("elimina", false)) {
                if (i < 0 || i >= n) { esito = el + " da eliminare inesistente"; return false; }
                if (n <= mn) { esito = "ne servono almeno " + std::to_string(mn); return false; }
                arr.erase(arr.begin() + i);
                esito = el + " " + std::to_string(i + 1) + " eliminato";
            } else if (i >= 0 && i < n) {
                arr[i] = Unisci(arr[i], a.value("elemento", json::object()));
                esito = el + " " + std::to_string(i + 1) + " modificato";
            } else {
                if (n >= mx) { esito = "al massimo " + std::to_string(mx); return false; }
                json base = n ? arr[n - 1] : json::object();
                json nuovo = Unisci(base, a.value("elemento", json::object()));
                int dove = a.contains("prima") && a["prima"].is_number() ? std::max(0, std::min(n, a["prima"].get<int>())) : n;
                arr.insert(arr.begin() + dove, nuovo);
                esito = "nuovo " + el + " " + std::to_string(dove + 1);
            }
        }
    }
    Sistema(schema, d);
    p.arcadeDati = d.dump();
    return true;
}

}  // namespace arcade
