// Pezzi comuni dei giochi arcade: disegno, scritte, particelle, suoni sintetizzati, regista adattivo.
#include "arcade.h"
#include "audio.h"
#include "ui.h"
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
