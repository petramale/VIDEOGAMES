// L'officina: il nuovo studio a risoluzione piena.
//   in alto    il menu di sistema "Gioco" (nuovo, apri, salva gioco, prova, crea .exe, stile...)
//              e i menu della sezione aperta; a destra lo stato di Claude e l'audio
//   sotto      le schede delle sezioni: Mondi, Personaggi, Oggetti, Scenari, Audio, Logica, Librerie, Comandi
//   al centro  la sezione (ognuna e' un vero editor: crea, modifica, salva, usa)
//   a destra   la chat con Claude, presente in ogni sezione: sa dove sei e cosa hai aperto
// Il gioco intero (mondo + libreria + regole) e' un solo file .gioco nella cartella "giochi".
#pragma once
#include "comune.h"
#include "claude.h"
#include "editor_disegno.h"
#include "editor_mondo.h"
#include "gioco.h"
#include "progetto_gioco.h"
#include "sfondo.h"
#include <functional>
#include <string>
#include <vector>

enum class Sezione { Home, Mondi, Personaggi, Oggetti, Scenari, Audio, Logica, Librerie, Comandi, Totale };

class Officina {
public:
    void Avvia();
    void Frame(float dt);                 // fra BeginDrawing ed EndDrawing
    bool esci = false;
    Rectangle PostoAudio() const { return postoAudio; }
    bool Provando() const { return provando || editorMondo.Provando(); }
    // Per le prove automatiche: apre un gioco, sceglie la sezione.
    bool ApriFile(const std::string& percorso);
    void Vai(Sezione s);
    void NuovoGioco(Genere g, StileGrafico stile, const std::string& titolo);
    ProgettoGioco& Progetto() { return progetto; }
    // Alla chiusura della finestra: salva il lavoro non salvato (un file nuovo non sovrascrive altri giochi).
    void SalvaPrimaDiUscire() { if (modificato || editorMondo.Modificato()) Salva(percorso.empty()); }

private:
    // --- Il gioco aperto ---
    ProgettoGioco progetto;
    std::string percorso;                 // file .gioco ("" = mai salvato)
    bool modificato = false;
    std::vector<ProgettoGioco> annulla, ripeti;
    void Ricorda();                       // prima di una modifica alla libreria
    void Annulla();
    void Ripeti();
    void Modificato() { modificato = true; }
    void Salva(bool conNome);
    void CreaEseguibile();
    void SincronizzaMondo();              // dall'editor dei mondi al progetto
    void MondoCambiato();                 // dal progetto all'editor dei mondi
    void LibreriaCambiata();
    std::vector<std::string> recenti;
    void CaricaRecenti();
    void AggiungiRecente(const std::string& p);

    // --- Struttura ---
    Sezione sezione = Sezione::Home;
    float tempo = 0;
    std::string avviso;
    float tAvviso = 0;
    void Avvisa(const std::string& m) { avviso = m; tAvviso = 4.5f; }
    Rectangle rBarra{}, rSchede{}, rCorpo{}, rChat{};
    Rectangle postoAudio{ 0, 0, 0, 0 };
    bool chatAperta = true;
    int menuAperto = -1;
    float xMenu[16] = {};
    int finestra = 0;                     // 0 nessuna, 1 nuovo gioco, 2 apri, 3 aiuto, 4 salva con nome, 5 stile, 6 chiave API
    void Layout();
    void BarraMenu();
    void MenuTendina();
    void Schede();
    void Finestre();
    void Scorciatoie();
    struct VoceO { std::string testo, tasto, codice; bool spunta = false, attiva = true; };
    struct MenuO { std::string titolo; std::vector<VoceO> voci; };
    std::vector<MenuO> Menu() const;      // "Gioco" (menu di sistema), i menu della sezione, "Aiuto"
    void Azione(const std::string& codice);
    void AzioneSezione(const std::string& codice);   // officina_sezioni.cpp
    void AzioneAudio(const std::string& codice);     // officina_audio.cpp
    void AzioneLogica(const std::string& codice);    // officina_logica.cpp
    void FileTrascinati();

    // --- Home ---
    Sfondo sfondoHome;
    int genereNuovo = 0, stileNuovo = 0;
    std::string titoloNuovo = "Il mio gioco";
    std::vector<std::string> elencoGiochi;
    float scorriElenco = 0;
    void Home(Rectangle r);
    void ApriElenco();

    // --- Prova (F5) ---
    bool provando = false;
    float tProva = 0;
    Gioco prova;
    RenderTexture2D rtProva{};
    void AvviaProva();
    void FrameProva(float dt);

    // --- Sezioni ---
    EditorMondo editorMondo;
    EditorDisegno editorDisegno;
    int selPersonaggio = 0, selOggetto = 0, selScenario = 0, selCasella = 0, selBrano = 0, selEffetto = 0, selRegola = 0;
    int schedaScenari = 0, schedaAudio = 0;
    float scorriLista = 0, scorriProprieta = 0, scorriRegole = 0;
    std::string bozzaNome;
    void Mondi(Rectangle r, float dt);
    void Personaggi(Rectangle r, float dt);
    void Oggetti(Rectangle r, float dt);
    void Scenari(Rectangle r, float dt);
    void Audio(Rectangle r, float dt);
    void Logica(Rectangle r, float dt);
    void Librerie(Rectangle r, float dt);
    void Comandi(Rectangle r, float dt);
    // Elenco di elementi con icona, a sinistra delle sezioni. Restituisce l'azione: 0 niente, 1 nuovo, 2 duplica, 3 elimina.
    int Elenco(Rectangle r, const std::string& titolo, std::vector<std::string> nomi, int& scelto,
               const std::function<void(int, Rectangle)>& icona, bool puoNuovo = true);
    void Titolo(Rectangle r, const std::string& t, const std::string& sotto = "");
    float Riga(Rectangle area, float& y, float h = 0) const;   // restituisce la x e avanza y

    // Scenari
    Sfondo anteprimaScenario;
    Livello terrenoScenario;
    float camScenario = 0, oraAnteprima = 0;
    int stratoScelto = 0;
    // Audio
    int tracciaScelta = 0, strumentoNota = 0;
    float scorriNoteX = 0, scorriNoteY = 0, zoomNote = 1;
    bool suonaBrano = false;
    float tBrano = 0;
    bool trascinaNota = false;
    int notaPresa = -1;
    float lunghezzaNota = 2;
    void PianoRoll(Rectangle r, Brano& b);
    void EditorEffetto(Rectangle r, EffettoSuono& e);
    void SuoniEventi(Rectangle r);
    // Logica
    std::string bozzaVariabile;
    // Librerie
    int categoriaLib = 0, raccoltaScelta = 0;
    float scorriLib = 0, scorriRaccolte = 0;
    // Comandi
    int rigaComando = 0, colonnaComando = 0;
    bool attesaComando = false;
    float tAttesa = 0;

    // --- Claude ---
    ChatClaude chat;
    ConfigClaude cfg;
    bool chatInAttesa = false;
    float tChat = 0, scorriChat = 0;
    std::string bozzaChat, bozzaChiave;
    void PannelloChat(Rectangle r);
    void InviaChat(const std::string& testo);
    void ControllaChat();
    std::string NomeSezione() const;
    std::string ElementoAperto() const;
    std::vector<std::string> SuggerimentiClaude() const;

    // Importazioni
    void ImportaFile(const std::string& file);
    bool ImportaElemento(const std::string& testo, std::string& messaggio);
    std::string ElementoJson() const;   // l'elemento aperto, da esportare
};
