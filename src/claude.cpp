#include "claude.h"
#include "sistema.h"
#include "json.hpp"
#include <fstream>
#include <iterator>

using nlohmann::json;

static std::string LeggiRiga(const std::string& percorso) {
    std::ifstream f(percorso);
    std::string s;
    if (f) std::getline(f, s);
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

ConfigClaude ConfigClaude::Carica() {
    ConfigClaude c;
    c.chiave = LeggiVariabileAmbiente("ANTHROPIC_API_KEY");
    std::string dir = CartellaEseguibile();
    if (c.chiave.empty()) c.chiave = LeggiRiga(dir + "/claude_chiave.txt");
    std::string m = LeggiRiga(dir + "/claude_modello.txt");
    if (!m.empty()) c.modello = m;
    return c;
}

std::string Claude::PromptSistema(TipoRichiesta tipo) {
    std::string base =
        "Sei l'assistente creativo di uno studio per creare videogiochi 2D in pixel art. "
        "Rispondi SOLO con un oggetto JSON valido: niente testo prima o dopo, niente blocchi di codice. ";
    if (tipo == TipoRichiesta::Sprite) {
        return base +
            "Crea uno sprite animato in pixel art con questo formato: "
            "{\"nome\": string, \"fps\": numero tra 2 e 10, "
            "\"palette\": {un carattere: \"#rrggbb\"}, \"fotogrammi\": [[righe di testo]]}. "
            "Regole: 16x16 pixel, da 2 a 4 fotogrammi tutti della stessa misura, al massimo 10 colori, "
            "il carattere '.' e' trasparente e non va nella palette, contorno scuro di 1 pixel, "
            "luci e ombre per dare volume, animazione fluida e piena di personalita'.";
    }
    return base +
        "Crea un livello per un gioco a piattaforme con questo formato: {\"nome\": string, \"righe\": [stringhe]}. "
        "Legenda: '#' terra, '=' pietra, '-' piattaforma attraversabile dal basso, '^' spine, ' ' vuoto, "
        "'P' partenza (una sola), 'F' bandiera d'arrivo (una sola), 'o' moneta, 'g' gemma, 'm' molla, "
        "'c' forziere, 'k' lanterna di controllo, 'e' slime, 'b' pipistrello. "
        "Regole: esattamente 15 righe tutte lunghe uguali, tra 60 e 160 colonne; le ultime 3 righe sono il terreno; "
        "gli oggetti stanno su una cella vuota appena sopra un appoggio; il personaggio salta al massimo 4 tile "
        "in alto e 5 in lungo (con la molla 8 in alto): ogni tratto deve essere raggiungibile; "
        "buche larghe al massimo 4; difficolta' crescente e qualche segreto.";
}

void Claude::Invia(const ConfigClaude& cfg, TipoRichiesta tipo, const std::string& descrizione) {
    json corpo = {
        { "model", cfg.modello },
        { "max_tokens", 16000 },
        { "output_config", { { "effort", "low" } } },
        { "system", PromptSistema(tipo) },
        { "messages", json::array({ { { "role", "user" }, { "content", descrizione } } }) },
    };
    std::string intestazioni =
        "content-type: application/json\r\n"
        "anthropic-version: 2023-06-01\r\n"
        "x-api-key: " + cfg.chiave + "\r\n";
    HttpAvviaPost("api.anthropic.com", "/v1/messages", intestazioni, corpo.dump());
}

int Claude::Stato() const { return HttpStato(); }

std::string Claude::Risultato() const {
    std::string r = HttpRisposta();
    if (HttpStato() != 2) return r;
    std::string j, errore;
    if (!EstraiRisposta(r, j, errore)) { const_cast<Claude*>(this)->ultimoErrore = errore; return errore; }
    const_cast<Claude*>(this)->ultimoErrore.clear();
    return j;
}

std::string Claude::EstraiJson(const std::string& testo) {
    // Primo oggetto JSON completo: si contano le graffe, ignorando quelle dentro le stringhe.
    size_t a = testo.find('{');
    while (a != std::string::npos) {
        int livello = 0;
        bool stringa = false, escape = false;
        for (size_t i = a; i < testo.size(); i++) {
            char ch = testo[i];
            if (stringa) {
                if (escape) escape = false;
                else if (ch == '\\') escape = true;
                else if (ch == '"') stringa = false;
                continue;
            }
            if (ch == '"') stringa = true;
            else if (ch == '{') livello++;
            else if (ch == '}' && --livello == 0) {
                std::string candidato = testo.substr(a, i - a + 1);
                if (!json::parse(candidato, nullptr, false).is_discarded()) return candidato;
                break;
            }
        }
        a = testo.find('{', a + 1);
    }
    return testo;
}

bool Claude::EstraiRisposta(const std::string& corpoHttp, std::string& risultato, std::string& errore) {
    {
        std::ofstream log(CartellaEseguibile() + "/claude_ultima_risposta.txt", std::ios::binary);
        if (log) log << corpoHttp;
    }
    json j = json::parse(corpoHttp, nullptr, false);
    if (j.is_discarded() || !j.is_object()) { errore = "risposta del server non leggibile (vedi claude_ultima_risposta.txt)"; return false; }
    if (j.value("type", "") == "error" && j.contains("error")) {
        errore = "errore del server: " + j["error"].value("message", std::string("sconosciuto"));
        return false;
    }
    std::string motivo = j.value("stop_reason", "");
    std::string testo;
    if (j.contains("content") && j["content"].is_array())
        for (auto& blocco : j["content"])
            if (blocco.value("type", "") == "text") testo += blocco.value("text", "");
    if (motivo == "refusal") { errore = "Claude non puo' rispondere a questa richiesta: prova a riformularla"; return false; }
    if (motivo == "max_tokens") { errore = "la risposta e' troppo lunga ed e' stata interrotta: chiedi qualcosa di piu' semplice"; return false; }
    if (testo.empty()) { errore = "Claude non ha scritto nessun testo (vedi claude_ultima_risposta.txt)"; return false; }
    risultato = EstraiJson(testo);
    if (json::parse(risultato, nullptr, false).is_discarded()) {
        errore = "Claude non ha risposto in JSON: \"" + testo.substr(0, 80) + (testo.size() > 80 ? "...\"" : "\"");
        return false;
    }
    return true;
}

bool Claude::LivelloDaJson(const std::string& testoJson, std::string& testo, std::string& nome, std::string& errore) {
    json j = json::parse(testoJson, nullptr, false);
    if (j.is_discarded() || !j.contains("righe") || !j["righe"].is_array()) { errore = "manca l'elenco delle righe"; return false; }
    nome = j.value("nome", std::string("Livello di Claude"));
    testo.clear();
    for (auto& r : j["righe"]) {
        if (!r.is_string()) { errore = "riga non valida"; return false; }
        testo += r.get<std::string>() + "\n";
    }
    return true;
}

// Risposte d'esempio, per provare lo studio anche senza internet o chiave API.
const char* Claude::EsempioSprite() {
    return R"({
  "nome": "Fantasmino",
  "fps": 4,
  "palette": { "k": "#1a1426", "w": "#f0f4ff", "b": "#a9b8ff", "p": "#ff7aa8", "s": "#ffffff" },
  "fotogrammi": [
    ["................",
     ".....kkkkkk.....",
     "....kwwwwwwk....",
     "...kwswwwwwwk...",
     "..kwswwwwwwwwk..",
     "..kwwkwwwwkwwk..",
     "..kwwkwwwwkwwk..",
     "..kwpwwwwwwpwk..",
     "..kwwwwkkwwwwk..",
     "..kwwwwwwwwwwk..",
     "..kbwwwwwwwwbk..",
     "..kbbwwwwwwbbk..",
     "..kbbbbbbbbbbk..",
     "..kbkbbkkbbkbk..",
     "..kk.kk..kk.kk..",
     "................"],
    ["................",
     "................",
     ".....kkkkkk.....",
     "....kwwwwwwk....",
     "...kwswwwwwwk...",
     "..kwswwwwwwwwk..",
     "..kwwkwwwwkwwk..",
     "..kwwkwwwwkwwk..",
     "..kwpwwwwwwpwk..",
     "..kwwwwkkwwwwk..",
     "..kbwwwwwwwwbk..",
     "..kbbwwwwwwbbk..",
     "..kbbbbbbbbbbk..",
     "..kkbbkbbkbbkk..",
     "...kk.kkkk.kk...",
     "................"]
  ]
})";
}

const char* Claude::EsempioLivello() {
    return R"({
  "nome": "Il ponte delle lanterne",
  "righe": [
    "                                                                      ",
    "                                                                      ",
    "                                                                      ",
    "                                     g                                ",
    "                                    ---                               ",
    "                   ooo                           b                    ",
    "                  -----                                               ",
    "                              b             ooooo                     ",
    "          o o                              -------                    ",
    "         -----          k                                 ===    F    ",
    "                       ---       ===               e    =====  ====   ",
    "  P          m   e           e   ===  ^^^  c          =======  ====   ",
    "##################   ##############################################   ",
    "##################   ##############################################   ",
    "##################   ##############################################   "
  ]
})";
}

// --- Chat --------------------------------------------------------------------

std::string ChatClaude::PromptSistema() {
    return
        "Sei Claude, l'assistente creativo dello Studio Videogiochi (sviluppato da Mario Petramale). "
        "L'utente sta creando un gioco a piattaforme 2D in pixel art e ti scrive in italiano. "
        "A ogni richiesta ricevi lo stato del progetto in JSON. Rispondi SOLO con un oggetto JSON valido, "
        "senza testo prima o dopo e senza blocchi di codice, con questo formato:\n"
        "{\"risposta\": \"messaggio breve e amichevole in italiano su cosa hai fatto o una domanda\", "
        "\"azioni\": [ ... ]}\n"
        "Azioni possibili (usa solo quelle che servono, anche nessuna):\n"
        "- {\"tipo\": \"livello\", \"righe\": [15 stringhe lunghe uguali]}: sostituisce la mappa. Legenda: '#' terra, "
        "'=' pietra, '-' piattaforma attraversabile dal basso, '^' spine, ' ' vuoto, 'P' partenza (una), 'F' bandiera (una), "
        "'o' moneta, 'g' gemma, 'm' molla, 'c' forziere, 'k' lanterna checkpoint, 'e' slime, 'b' pipistrello. "
        "Le ultime 3 righe sono il terreno. Il personaggio salta circa 3 tile in alto e 5 in lungo con la fisica standard: "
        "ogni tratto deve essere raggiungibile. Per piccole modifiche parti dalla mappa attuale e cambia solo il necessario.\n"
        "- {\"tipo\": \"titolo\", \"valore\": \"...\"}\n"
        "- {\"tipo\": \"scenario\", \"tema\": 0-3, \"ora\": 0.0-1.0}: temi 0 valle verde, 1 deserto rosso, 2 ghiacci eterni, "
        "3 terre vulcaniche; ora 0 mezzanotte, 0.25 alba, 0.5 mezzogiorno, 0.75 tramonto.\n"
        "- {\"tipo\": \"eroe\", \"pelle\": \"#rrggbb\", \"capelli\": ..., \"maglia\": ..., \"mantello\": ..., \"pantaloni\": ..., \"scarpe\": ...} "
        "(anche solo alcuni colori)\n"
        "- {\"tipo\": \"fisica\", \"gravita\": 300-2000, \"salto\": 150-600, \"velocita\": 40-300, \"molla\": 250-900} "
        "(standard: 900, 305, 115, 470; anche solo alcuni valori)\n"
        "Se la richiesta riguarda qualcosa che ancora non puoi cambiare (per esempio nuovi tipi di nemici, musica o sprite), "
        "dillo con gentilezza nella risposta e proponi l'alternativa piu' vicina.";
}

void ChatClaude::Invia(const ConfigClaude& cfg, const std::string& richiesta, const std::string& statoProgettoJson, const std::string& prompt) {
    MessaggioChat m{ true, richiesta, "Stato attuale del progetto:\n" + statoProgettoJson + "\n\nRichiesta: " + richiesta };
    storia.push_back(m);
    json messaggi = json::array();
    // Le richieste precedenti senza lo stato (pesante): basta l'ultimo stato completo.
    for (size_t i = 0; i < storia.size(); i++) {
        const auto& s = storia[i];
        bool ultimo = i + 1 == storia.size();
        std::string contenuto = s.utente ? (ultimo ? s.grezzo : s.testo) : s.grezzo;
        if (contenuto.empty()) contenuto = "(vuoto)";
        messaggi.push_back({ { "role", s.utente ? "user" : "assistant" }, { "content", contenuto } });
    }
    json corpo = {
        { "model", cfg.modello },
        { "max_tokens", 16000 },
        { "output_config", { { "effort", "medium" } } },
        { "system", prompt.empty() ? PromptSistema() : prompt },
        { "messages", messaggi },
    };
    std::string intestazioni =
        "content-type: application/json\r\n"
        "anthropic-version: 2023-06-01\r\n"
        "x-api-key: " + cfg.chiave + "\r\n";
    HttpAvviaPost("api.anthropic.com", "/v1/messages", intestazioni, corpo.dump());
}

int ChatClaude::Stato() const { return HttpStato(); }

std::string ChatClaude::RispostaGrezza() const {
    std::string r = HttpRisposta();
    if (HttpStato() != 2) return r;
    std::string j, errore;
    if (!Claude::EstraiRisposta(r, j, errore)) return "{\"risposta\": " + json(errore).dump() + ", \"azioni\": []}";
    return j;
}
