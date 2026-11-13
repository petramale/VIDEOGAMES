// Editor dei livelli: unisce le categorie 1, 3 e 5 in uno strumento di creazione.
// Si disegnano terreno e oggetti con il mouse (o con il joypad), si cambia scenario,
// si prova subito il gioco nell'anteprima integrata, si salva e si crea il .exe.
// A destra c'e' la chat con Claude, che modifica direttamente il progetto aperto.
#pragma once
#include "comune.h"
#include "claude.h"
#include "gioco.h"
#include "progetto.h"

class Editor {
public:
    void Inizializza();
    void Entra();                           // all'ingresso: applica le musiche del progetto
    void Esci();                            // all'uscita: torna ai suoni dello studio
    void Apri(const Progetto& p, const std::string& messaggio);
    void Aggiorna(float dt);
    void Disegna();                         // nella risoluzione virtuale
    // La chat si disegna a risoluzione piena, nel pannello a destra della finestra.
    bool ChatAperta() const { return chatAperta && !provando; }
    void AggiornaChat(Rectangle pannello);
    void DisegnaChat(Rectangle pannello);
    bool vuoleUscire = false;
    bool Provando() const { return provando; }
    bool Scrivendo() const { return fuocoChat || rinomina; }   // la tastiera serve per scrivere

private:
    Progetto prog;
    std::string percorsoFile;               // dove e' stato salvato l'ultima volta
    Livello vista;
    bool vistaValida = false;
    Sfondo sfondo;
    float camX = 0, tempo = 0;
    int strumento = 0;
    std::vector<Progetto> annulla;
    bool modificato = false, tratto = false;
    std::string messaggio;
    float tMessaggio = 0;
    // Joypad: cursore a celle
    int cursoreX = 3, cursoreY = 11;
    float ripetizione = 0;
    // Finestre interne
    bool provando = false, aiuto = false, elenco = false, rinomina = false, pannelloAudio = false;
    Gioco prova;
    std::vector<std::string> fileElenco;
    int sceltaElenco = 0, sceltaAudio = 0;
    // Chat
    bool chatAperta = true, chatInAttesa = false, fuocoChat = false;
    ChatClaude chat;
    ConfigClaude cfg;
    std::string bozza;
    float scorriChat = 0;

    void Ricostruisci();
    void Ricorda();
    void Metti(int x, int y, char c);
    void Avvisa(const std::string& m) { messaggio = m; tMessaggio = 4; }
    void Salva();
    void ApriElenco();
    void CreaEseguibile();
    void AvviaProva();
    void ApplicaRisposta(const std::string& json);
    void AssegnaFileAudio(const std::string& percorso);
    void ApplicaAudio();
    std::string Cartella() const;
    std::string NomeFile() const;
};
