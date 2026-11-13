// Il nuovo editor dei mondi (fase 2): aspetto da videogioco, ma lavora come un programma.
//   in alto   la barra dei menu e le schede delle mappe
//   a sinistra la tavolozza: caselle, oggetti, partenza
//   al centro la mappa con scroll in 4 direzioni, zoom, griglia e minimappa
//   a destra  i pannelli: proprieta' (oggetto o mappa), mondo, strati dello sfondo, chat con Claude
//   in basso  gli strumenti: pennello, gomma, riempi, rettangolo, seleziona, contagocce
// Si disegna a risoluzione piena (non nella risoluzione virtuale del gioco).
#pragma once
#include "comune.h"
#include "claude.h"
#include "gioco.h"
#include "mondo.h"

enum class Strumento { Pennello, Gomma, Riempi, Rettangolo, Seleziona, Contagocce, Totale };
enum class SchedaDestra { Proprieta, MondoTab, Strati, Claude, Totale };

class EditorMondo {
public:
    void Inizializza();
    void Apri(const Mondo& m, const std::string& percorso, const std::string& messaggio);
    void Entra();
    void Esci();
    // Gestisce input e disegno di un fotogramma (da chiamare fra BeginDrawing ed EndDrawing).
    void Frame(float dt);
    bool vuoleUscire = false;
    bool vuoleClassico = false;    // chiede di aprire il vecchio editor (musiche e suoni importati)
    bool Scrivendo() const;
    bool Provando() const { return provando; }
    void ImpostaChiave(const std::string& k) { cfg.chiave = k; }
    Rectangle PostoAudio() const { return postoAudio; }
    const Mondo& MondoAperto() const { return mondo; }

private:
    // Dati
    Mondo mondo;
    int mappa = 0;
    std::string percorso;
    bool modificato = false;
    std::vector<Mondo> annulla, ripeti;
    Livello vista;                 // la mappa aperta, pronta da disegnare
    Sfondo sfondo;
    RenderTexture2D rtSfondo{}, rtGioco{};
    float tempo = 0;

    // Mappa sullo schermo
    float camX = 0, camY = 0, zoom = 3;
    bool griglia = true, anteprimaSfondo = true, inquadratura = true;
    Vector2 trascinaDa{ 0, 0 }, camDa{ 0, 0 };
    bool trascinando = false;

    // Strumenti
    Strumento strumento = Strumento::Pennello;
    std::string pennello = "#";    // carattere della casella, nome dell'oggetto, oppure "partenza"
    bool tratto = false;
    int rettX = 0, rettY = 0;      // inizio del rettangolo
    bool rettangolo = false;
    int selezionato = -1;          // indice dell'oggetto selezionato
    bool spostando = false;
    Vector2 presa{ 0, 0 };
    OggettoMappa appunti;
    bool appuntiPieni = false;
    bool sceltaArrivo = false;     // si sta scegliendo il punto d'arrivo di una porta
    int portaMappa = -1, portaIndice = -1;

    // Interfaccia
    SchedaDestra pannello = SchedaDestra::Claude;
    int menuAperto = -1;
    int finestra = 0;              // 0 nessuna, 1 nuovo, 2 apri, 3 aiuto, 4 rinomina mappa
    std::vector<Mondo> elencoMondi;
    std::vector<std::string> elencoPercorsi;
    float scorriPannello = 0, scorriStrati = 0;
    int stratoScelto = 0;
    std::string messaggio;
    float tMessaggio = 0;
    Rectangle postoAudio{ 0, 0, 0, 0 };
    Rectangle rVista{ 0, 0, 1, 1 };   // dove si vede la mappa sullo schermo
    float scorriElenco = 0;
    Mondo primaDiSpostare;         // per annullare lo spostamento di un oggetto
    bool spostato = false;
    int decorazioneScelta = -1;
    std::string nomeMappaBozza;

    // Prova del gioco
    bool provando = false;
    float tProva = 0;              // da quanto dura la prova (il tasto che la avvia non la chiude)
    Gioco prova;

    // Chat
    ChatClaude chat;
    ConfigClaude cfg;
    bool chatInAttesa = false;
    std::string bozza, bozzaChiave;
    float scorriChat = 0;
    float tAttesa = 0;

    // Funzioni
    Mappa& M() { return mondo.mappe[mappa]; }
    void Ricorda();
    void Annulla();
    void Ripeti();
    void Ricostruisci();
    void CambiaMappa(int i);
    void Inquadra();
    void Avvisa(const std::string& m) { messaggio = m; tMessaggio = 4; }
    void Salva(bool conNome);
    void CreaEseguibile();
    void AvviaProva(bool daQui);
    void ApriElenco();
    void NuovoMondo(int genere);
    Vector2 MondoDaSchermo(Vector2 p) const;
    int OggettoSotto(Vector2 mondoPos) const;
    void Dipingi(int cx, int cy);
    void Cancella(int cx, int cy, Vector2 punto);
    void Riempi(int cx, int cy, char c);
    void InviaChat(const std::string& testo);
    void ControllaChat();
    void CaricaImmagini();
    void ImportaFile(const std::string& file);
    void Esegui(int menu, int voce);

    // Disegno e input delle parti
    void Layout(Rectangle& barra, Rectangle& schede, Rectangle& sinistra, Rectangle& centro, Rectangle& destra, Rectangle& basso) const;
    void BarraMenu(Rectangle r);
    void MenuTendina(Rectangle barra);
    void Schede(Rectangle r);
    void Tavolozza(Rectangle r);
    void VistaMappa(Rectangle r, float dt);
    void Minimappa(Rectangle r);
    void Strumenti(Rectangle r);
    void PannelloDestro(Rectangle r);
    void PannelloProprieta(Rectangle r);
    void PannelloMondo(Rectangle r);
    void PannelloStrati(Rectangle r);
    void PannelloClaude(Rectangle r);
    void Finestre();
    void Scorciatoie();
    void FrameProva(float dt);
    void IconaOggetto(const std::string& tipo, Rectangle r) const;
    void IconaCasella(char c, Rectangle r) const;
};
