#include "pixelart.h"
#include "json.hpp"
#include <map>

using nlohmann::json;

static bool ColoreDaTesto(const std::string& s, Color& c) {
    if (s.size() != 7 || s[0] != '#') return false;
    unsigned int v = (unsigned int)std::stoul(s.substr(1), nullptr, 16);
    c = Hex(v);
    return true;
}

bool SpritePixel::CaricaDaJson(const std::string& testo, std::string& errore) {
    json j = json::parse(testo, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { errore = "JSON non valido"; return false; }
    if (!j.contains("palette") || !j["palette"].is_object()) { errore = "manca la palette"; return false; }
    if (!j.contains("fotogrammi") || !j["fotogrammi"].is_array() || j["fotogrammi"].empty()) {
        errore = "mancano i fotogrammi"; return false;
    }
    std::map<char, Color> palette;
    for (auto& [chiave, valore] : j["palette"].items()) {
        Color c;
        if (chiave.size() != 1 || !valore.is_string() || !ColoreDaTesto(valore.get<std::string>(), c)) {
            errore = "colore non valido nella palette: " + chiave; return false;
        }
        palette[chiave[0]] = c;
    }
    Scarica();
    nome = j.value("nome", std::string("sprite"));
    fps = j.value("fps", 4.0f);
    for (auto& f : j["fotogrammi"]) {
        if (!f.is_array() || f.empty()) { errore = "fotogramma vuoto"; Scarica(); return false; }
        int h = (int)f.size(), w = 0;
        for (auto& riga : f) w = std::max(w, (int)riga.get<std::string>().size());
        if (w > 64 || h > 64) { errore = "sprite troppo grande (massimo 64x64)"; Scarica(); return false; }
        Image img = GenImageColor(w, h, BLANK);
        for (int y = 0; y < h; y++) {
            std::string riga = f[y].get<std::string>();
            for (int x = 0; x < (int)riga.size(); x++) {
                auto it = palette.find(riga[x]);
                if (it != palette.end()) ImageDrawPixel(&img, x, y, it->second);
            }
        }
        fotogrammi.push_back(LoadTextureFromImage(img));
        UnloadImage(img);
        larghezza = w; altezza = h;
    }
    return true;
}

void SpritePixel::Scarica() {
    for (auto& t : fotogrammi) UnloadTexture(t);
    fotogrammi.clear();
}

void SpritePixel::Disegna(Vector2 centro, float scala, float tempo, bool specchia) const {
    if (fotogrammi.empty()) return;
    const Texture2D& t = fotogrammi[(size_t)(tempo * fps) % fotogrammi.size()];
    Rectangle src = { 0, 0, (float)(specchia ? -t.width : t.width), (float)t.height };
    Rectangle dst = { std::round(centro.x - t.width * scala / 2), std::round(centro.y - t.height * scala / 2),
                      t.width * scala, t.height * scala };
    DrawTexturePro(t, src, dst, { 0, 0 }, 0, WHITE);
}
