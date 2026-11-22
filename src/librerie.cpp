#include "librerie.h"
#include "raylib.h"
#include "json.hpp"

#define STRINGA2(x) #x
#define STRINGA(x) STRINGA2(x)

const std::vector<InfoLibreria>& Librerie() {
    static const std::vector<InfoLibreria> elenco = {
        { "raylib", RAYLIB_VERSION, "zlib", "Grafica, audio, input, finestra", "librerie/raylib" },
        { "GLFW (in raylib)", "3.4", "zlib", "Finestra e tastiera su Windows", "librerie/raylib/src/external/glfw" },
        { "miniaudio (in raylib)", "0.11", "MIT-0", "Uscita audio", "librerie/raylib/src/external" },
        { "stb_image (in raylib)", "2.30", "MIT", "Lettura immagini PNG", "librerie/raylib/src/external" },
        { "nlohmann/json",
          STRINGA(NLOHMANN_JSON_VERSION_MAJOR) "." STRINGA(NLOHMANN_JSON_VERSION_MINOR) "." STRINGA(NLOHMANN_JSON_VERSION_PATCH),
          "MIT", "File JSON, dialogo con Claude", "librerie/nlohmann" },
        { "WinHTTP", "Windows", "parte di Windows", "Collegamento HTTPS a Claude", "(sistema operativo)" },
    };
    return elenco;
}
