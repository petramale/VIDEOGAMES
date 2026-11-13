#include "immagini.h"
#include <map>

namespace {
std::map<std::string, std::string> dati;       // nome -> contenuto del file
std::map<std::string, Texture2D> texture;      // gia' caricate (id 0 = non leggibile)

std::string Estensione(const std::string& nome) {
    size_t p = nome.find_last_of('.');
    std::string e = p == std::string::npos ? std::string(".png") : nome.substr(p);
    for (auto& c : e) c = (char)std::tolower((unsigned char)c);
    return e;
}

void Scarica(const std::string& nome) {
    auto t = texture.find(nome);
    if (t == texture.end()) return;
    if (t->second.id && IsWindowReady()) UnloadTexture(t->second);
    texture.erase(t);
}
}

void LiberaImmagini() {
    if (IsWindowReady()) for (auto& t : texture) if (t.second.id) UnloadTexture(t.second);
    texture.clear();
}

void ImpostaFileImmagini(const std::vector<FileIncluso>& file) {
    LiberaImmagini();
    dati.clear();
    for (auto& f : file) {
        std::string e = Estensione(f.first);
        if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".tga" || e == ".gif" || e == ".qoi") dati[f.first] = f.second;
    }
}

void AggiungiFileImmagine(const std::string& nome, const std::string& contenuto) {
    Scarica(nome);
    dati[nome] = contenuto;
}

Texture2D Immagine(const std::string& nome) {
    auto t = texture.find(nome);
    if (t != texture.end()) return t->second;
    Texture2D tex{};
    auto d = dati.find(nome);
    if (d != dati.end() && IsWindowReady()) {
        Image img = LoadImageFromMemory(Estensione(nome).c_str(), (const unsigned char*)d->second.data(), (int)d->second.size());
        if (img.data) {
            tex = LoadTextureFromImage(img);
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);   // pixel art nitida
            UnloadImage(img);
        }
    }
    texture[nome] = tex;
    return tex;
}
