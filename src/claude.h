// Categoria 7: creare i contenuti con Claude.
// Lo studio manda a Claude la tua descrizione e chiede in risposta un JSON preciso
// (uno sprite in pixel art o un livello giocabile), che poi controlla e importa.
#pragma once
#include <string>
#include <vector>

enum class TipoRichiesta { Sprite, Livello };

struct ConfigClaude {
    std::string chiave;                       // da ANTHROPIC_API_KEY o dal file claude_chiave.txt
    std::string modello = "claude-opus-5-5";   // modificabile con il file claude_modello.txt
    static ConfigClaude Carica();
};

class Claude {
public:
    void Invia(const ConfigClaude& cfg, TipoRichiesta tipo, const std::string& descrizione);
    // 0 nessuna richiesta, 1 in attesa, 2 risposta pronta, 3 errore
    int Stato() const;
    // JSON del contenuto (sprite o livello) estratto dalla risposta, oppure messaggio di errore.
    std::string Risultato() const;
    std::string ultimoErrore;              // vuoto se la risposta e' un JSON valido

    static std::string PromptSistema(TipoRichiesta tipo);
    static std::string EstraiJson(const std::string& testo);
    // Dalla risposta HTTP completa al JSON prodotto da Claude. Controlla il motivo di fine
    // (troppo lunga, rifiutata...), toglie eventuali ``` e salva tutto in claude_ultima_risposta.txt.
    static bool EstraiRisposta(const std::string& corpoHttp, std::string& json, std::string& errore);
    // Converte il JSON di un livello ({"righe": [...]}) nel testo letto dal motore.
    static bool LivelloDaJson(const std::string& json, std::string& testo, std::string& nome, std::string& errore);

    static const char* EsempioSprite();
    static const char* EsempioLivello();
};

// Chat con Claude dentro l'editor: Claude legge lo stato del progetto aperto e
// risponde con un messaggio e un elenco di modifiche da applicare.
struct MessaggioChat {
    bool utente;
    std::string testo;       // cio' che si vede nella chat
    std::string grezzo;      // per Claude: richiesta con lo stato, oppure il JSON della risposta
};

class ChatClaude {
public:
    std::vector<MessaggioChat> storia;
    // prompt vuoto = quello del vecchio editor a caratteri.
    void Invia(const ConfigClaude& cfg, const std::string& richiesta, const std::string& statoProgettoJson, const std::string& prompt = "");
    int Stato() const;                    // come Claude::Stato
    std::string RispostaGrezza() const;   // testo della risposta (JSON) o errore
    static std::string PromptSistema();
};
