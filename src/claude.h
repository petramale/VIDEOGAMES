// Categoria 7: creare i contenuti con Claude.
// Lo studio manda a Claude la tua descrizione e chiede in risposta un JSON preciso
// (uno sprite in pixel art o un livello giocabile), che poi controlla e importa.
#pragma once
#include <string>

enum class TipoRichiesta { Sprite, Livello };

struct ConfigClaude {
    std::string chiave;                       // da ANTHROPIC_API_KEY o dal file claude_chiave.txt
    std::string modello = "claude-sonnet-5-5"; // modificabile con il file claude_modello.txt
    static ConfigClaude Carica();
};

class Claude {
public:
    void Invia(const ConfigClaude& cfg, TipoRichiesta tipo, const std::string& descrizione);
    // 0 nessuna richiesta, 1 in attesa, 2 risposta pronta, 3 errore
    int Stato() const;
    // JSON del contenuto (sprite o livello) estratto dalla risposta, oppure messaggio di errore.
    std::string Risultato() const;

    static std::string PromptSistema(TipoRichiesta tipo);
    static std::string EstraiJson(const std::string& testo);
    // Converte il JSON di un livello ({"righe": [...]}) nel testo letto dal motore.
    static bool LivelloDaJson(const std::string& json, std::string& testo, std::string& nome, std::string& errore);

    static const char* EsempioSprite();
    static const char* EsempioLivello();
};
