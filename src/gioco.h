// Categorie 3 e 5: oggetti interattivi e logica di gioco.
// Fisica a piattaforme (gravita', salto variabile, "coyote time"), collisioni con
// tile e oggetti, nemici con intelligenza semplice, telecamera che scorre con
// anticipo, punti di controllo, vittoria e sconfitta.
#pragma once
#include "comune.h"
#include "sfondo.h"
#include "personaggio.h"
#include "livello.h"
#include "particelle.h"
#include "progetto.h"
#include "mondo.h"
#include "progetto_gioco.h"
#include <map>
#include <set>

struct Comandi {
    float asse = 0;                 // -1 sinistra .. 1 destra (anche analogico)
    float asseY = 0;                // -1 su .. 1 giu' (labirinto, puzzle, sparatutto)
    bool salto = false, saltoPremuto = false;   // nello sparatutto e' il fuoco
    bool mouseGuida = false;        // mouse: cammina verso il puntatore
    float mouseX = 0, mouseY = 0;   // posizione del puntatore sullo schermo virtuale
    bool annulla = false;           // puzzle: annulla l'ultima mossa
    static Comandi DaPeriferiche(); // tastiera + mouse + joypad, secondo la mappatura
};

struct Proiettile {
    Vector2 pos, vel;
    bool amico;                     // sparato dall'eroe
    float vita;
    Color colore;
    int disegno = -1;               // oggetto della libreria che fa da proiettile (-1 = palla luminosa)
    int danno = 1;
};

// Disegna un oggetto del livello (usato anche dalla tavolozza dell'editor).
void DisegnaOggetto(const Oggetto& o, float camX, float camY, float tempo, bool bandieraVinta);
Rectangle BoxOggetto(const Oggetto& o);

class Gioco {
public:
    // Accetta il JSON del nuovo formato e il vecchio testo a caratteri.
    bool Carica(const std::string& testoLivello, std::string& errore);
    bool CaricaMondo(const Mondo& m, std::string& errore);
    void Aggiorna(float dt, const Comandi& c);
    void Disegna();
    // Il gioco completo: mondo + libreria (personaggi, oggetti, caselle, scenari, brani, effetti, regole).
    bool CaricaProgetto(const ProgettoGioco& p, std::string& errore);
    const ProgettoGioco& Libreria() const { return lib; }
    // Disegna in una texture ad alta risoluzione (VW*scala x VH*scala), creandola se serve.
    void DisegnaHD(RenderTexture2D& rt, int scala);
    void Ricomincia() { std::string e; if (conLibreria) CaricaProgetto(ProgettoGioco(lib), e); else CaricaMondo(Mondo(mondo), e); }
    bool MusicaDellaLibreria() const { return musica < 0; }
    std::map<std::string, float> variabili;
    std::map<std::string, int> inventario;
    std::string messaggio;          // testo mostrato da una regola
    float tMessaggio = 0;

    Sfondo sfondo;
    Eroe eroe;
    ParametriFisica fisica;
    bool mostraEtichette = false;   // nomi degli oggetti (vetrina della categoria 3)
    bool mostraCollisioni = false;  // riquadri di collisione (categoria 5)
    bool cicloGiorno = true;        // l'ora del giorno avanza lentamente
    bool mostraHud = true;
    std::string titolo = "La valle delle gemme";
    std::string crediti;            // riga mostrata alla vittoria (giochi creati)
    bool mostraEsito = true;        // scritte "livello completato" e "game over" (la Partita usa le sue)
    bool mostraFps = false;         // debug: FPS e contatori
    int lingua = 0;                 // 0 italiano, 1 inglese (testi dei cartelli)

    Modello modello = Modello::Platform;
    float zoomBase = 1;
    int musica = 0;
    std::vector<TestoCartello> testi;

    int monete = 0, gemme = 0, vite = 3, punteggio = 0;
    int energia = 3, energiaMax = 3;
    int mosse = 0;                  // puzzle
    float tempo = 0;
    bool vinto = false, finito = false;

    float CameraX() const { return camX; }
    float CameraY() const { return camY; }
    bool VistaAlto() const { return vistaAlto; }
    const Mondo& MondoAttuale() const { return mondo; }
    int MappaAttuale() const { return mappaAttuale; }
    Telecamera telecamera;          // della mappa attuale (si puo' cambiare al volo)
    int GemmeTotali() const { return gemmeTotali; }

    // --- Pilastri 1-3: mente, destino della partita, salvataggio (mente.h, destino in gioco_mente.cpp, salvataggio.cpp) ---
    uint64_t semePartita = 0;       // 0 = niente variazioni (vetrina, prove); va scelto prima di CaricaProgetto
    std::string eventoPartita;      // chiave dell'evento speciale ("" = nessuno)
    bool mostraPensieri = false;    // F9: i pensieri delle creature sopra la testa
    int autosalvataggi = 0;         // cresce quando conviene salvare da soli (cambio mappa, punto di controllo)
    std::map<std::string, Sapere> sapere;   // quello che ogni specie ha imparato (chiave: nome del personaggio)
    float pieta = 0;                // l'eroe e' in difficolta': le creature allentano (0..0.5)
    // Lo stato completo della partita in JSON (senza busta: vedi salvataggio.h).
    std::string StatoJson() const;
    // Ricarica il progetto con il seme salvato, rientra nella mappa e sovrappone lo stato. 'avviso' spiega le correzioni.
    bool Ripristina(const ProgettoGioco& p, const std::string& statoJson, std::string& errore, std::string& avviso);
    std::string SapereJson() const;                 // memoria fra le partite
    void SapereDaJson(const std::string& testo);
    std::string NomeEventoPartita() const;
    int NumeroEntita() const { return (int)entita.size(); }
    // Per le prove: riassunto (azione e umore) delle creature della mappa.
    std::vector<std::string> PensieriCreature() const;
    // Per le prove: firma dello stato (posizioni, energie, menti) per confrontare due partite.
    std::string Firma() const;
    // Dopo aver copiato un Gioco in un altro: le caselle puntano ai disegni della libreria della copia.
    void RicollegaRisorse();
    bool UscitaAperta() const { return modello != Modello::Labirinto || gemme >= gemmeTotali; }

private:
    void Fisica(float dt, const Comandi& c);
    void FisicaLabirinto(float dt, const Comandi& c);
    void FisicaPuzzle(float dt, const Comandi& c);
    void FisicaSparatutto(float dt, const Comandi& c);
    void AggiornaProiettili(float dt);
    void AggiornaBoss(Oggetto& o, float dt, Rectangle box);
    void AggiornaTelecamera(float dt, bool subito = false);
    bool EntraMappa(int indice, Vector2 arrivo, std::string& errore);
    void VaiAMappa(int indice, Vector2 arrivo);
    bool Laterale() const { return !vistaAlto && modello != Modello::Sparatutto; }
    void DisegnaDecorazioni(float cx, float cy, bool primoPiano) const;
    void Spara(Vector2 da, Vector2 verso, float velocita, bool amico, Color c);
    void Colpito(Oggetto& o, int danno);       // un nemico preso da un colpo o schiacciato
    bool CassaIn(int tx, int ty) const;
    void DisegnaNave(Vector2 p) const;
    void DisegnaCartello() const;
    void MuoviConCollisioni(Vector2& pos, Vector2& vel, float w, float hgt, float dt, bool& aTerra, bool attraversa);
    void AggiornaOggetti(float dt);
    void Ferisci(float spintaX);
    void Rinasci();
    void DisegnaOggettoConInfo(const Oggetto& o) const;
    void DisegnaHud() const;

    // --- Libreria del gioco (gioco_libreria.cpp) ---
    struct Entita {
        bool personaggio = true;
        int def = 0;                // indice in lib.personaggi o lib.oggetti
        std::string nome;
        Vector2 pos{ 0, 0 }, vel{ 0, 0 }, origine{ 0, 0 };
        int dir = -1, energia = 1, mappaPorta = -1;
        Vector2 destPorta{ -1, -1 };
        bool attivo = true, aTerra = false, aperta = false, acceso = false, toccando = false, vicino = false;
        float t = 0, tAnim = 0, timer = 0, lampeggio = 0, morte = 0, velMolt = 1, spinta = 0;
        std::string anim;           // animazione chiesta (vuoto = automatica)
        Color tinta = WHITE;
        // Identita' stabile per il salvataggio: indice nell'elenco della mappa, o 1000000+n se creata durante il gioco.
        int uid = -1;
        Mente mente;
        int energiaMax = 1, veterano = 0;   // veterano: 1 piu' forte, -1 timido
        float puntiMolt = 1, valoreMolt = 1;
        bool bloccato = false, amicoAnnunciato = false;
        int bersaglioCura = -1, bersaglioRivale = -1;   // si ricalcolano a ogni pensiero (non si salvano)
        float tGrido = 0, tContatto = 0;
        // Indole degli oggetti
        float carica = 1, forza = 1, tRicarica = 0, tSveglio = 0;
        int usi = 0;
        bool rotto = false;
    };
    ProgettoGioco lib;
    bool conLibreria = false;
    int eroeDef = -1;
    std::vector<Entita> entita;
    std::vector<Rectangle> boxPersonaggi, boxOggetti;   // riquadro del disegno rispetto ai piedi
    std::vector<float> timerRegole;
    std::vector<bool> condizioniRegole;
    std::set<std::pair<int, int>> energiaNotificata;
    std::vector<std::string> nuoveRegoleInizio;
    int pioggia = 0, appoggio = -1, dialogo = -1;
    float buio = 0;                 // scenari bui: si vede solo vicino alle luci
    bool acqua = false;             // scenari sott'acqua: si nuota
    void DisegnaBuio(float cx, float cy) const;
    float asseInput = 0;
    void PreparaLibreria();
    void EntitaDellaMappa(const Mappa& m);
    int CreaEntita(const std::string& nome, Vector2 pos);
    Rectangle BoxEntita(const Entita& e) const;
    const Disegno* DisegnoEntita(const Entita& e) const;
    void AggiornaEntita(float dt, const Comandi& c);
    void AggiornaPersonaggio(Entita& e, int i, float dt, Rectangle eroeBox);
    void AggiornaOggettoLib(Entita& e, int i, float dt, Rectangle eroeBox);
    void ColpisciEntita(int i, int danno);
    bool ProiettileSuEntita(Proiettile& p);
    void SparaDa(int i, const std::string& proiettile, int quanti);
    void DisegnaEntita(float cx, float cy, bool decorazioni) const;
    void DisegnaEroeLibreria(Vector2 p) const;
    void DisegnaProiettileLib(const Proiettile& p, float cx, float cy) const;
    void DisegnaExtraHud() const;
    void DisegnaPioggia() const;
    void Evento(const std::string& tipo, const std::string& chi, const std::string& conChi, int ent);
    void EseguiRegola(const Regola& r, int ent);
    bool Condizione(const Regola& r) const;
    float Valore(const std::string& v) const;
    void ImpostaValore(const std::string& v, float x);
    void AggiornaRegole(float dt);
    void SolidiEntita(Vector2& pos, Vector2& vel, float w, float hgt, float piediPrima, bool& aTerra, int ignora);
    void SuonaLibreria(const std::string& effetto, float volume = 1);

    // --- Mente e destino (gioco_mente.cpp) ---
    struct StatoMappa { std::set<int> tolti, aperti, accesi, base; };
    std::map<int, StatoMappa> mappeStato;          // progressi di scena delle mappe lasciate
    std::map<std::pair<int, int>, int> relazioni;  // fra specie (indici dei personaggi): 1 alleate, -1 rivali
    std::map<int, float> fiduciaIniziale;           // abitanti: fiducia iniziale in questa partita
    std::vector<ProfiloComportamento> profiliDef;   // il profilo di ogni personaggio della libreria
    Generatore rng;                                        // il generatore della partita (decisioni)
    int contatoreCreati = 0, profonditaEventi = 0, causaColpo = 0;
    bool ripristinando = false, eroeATerraPrima = true;
    float vistaMolt = 1, velocitaMolt = 1, puntiPartitaMolt = 1;
    void PreparaDestino();
    void DestinoMappa(int indice);
    void RicordaMappa();
    void ApplicaStatoMappa(int indice);
    void AggiornaConMente(Entita& e, int i, float dt, Rectangle eroeBox);
    void MovimentoClassico(Entita& e, int i, float dt, TipoComportamento tipo);
    void FinePersonaggio(Entita& e, int i, float dt, Rectangle eroeBox);
    Contesto ContestoDi(const Entita& e, int i, float dist, float vista, bool vede);
    void ChiamaAiuto(int i);
    void DannoFraCreature(int i, int danno);
    void ReazioneColpo(int i);
    void ImparaMorte(int i);
    void AggiornaIndole(Entita& e, int i, float dt);
    int Relazione(int defA, int defB) const;
    bool Ostile(const Entita& e) const;
    void DisegnaPensieri(float cx, float cy, bool barra = false) const;

    Livello livello;
    Mondo mondo;
    int mappaAttuale = 0, mappaVerso = -1, mappaDopoVittoria = -1;
    Vector2 arrivoVerso{ -1, -1 };
    float tTransizione = 0;         // dissolvenza tra una mappa e l'altra
    bool portaBloccata = false;     // appena arrivati su una porta: non si riparte subito
    bool vistaAlto = false;
    std::vector<Oggetto> oggetti;
    std::vector<Proiettile> proiettili;
    std::vector<std::vector<Oggetto>> storiaPuzzle; // per annullare le mosse
    std::vector<Vector2> storiaEroe;
    Particelle particelle;
    int gemmeTotali = 0, bossIndice = -1, cartello = -1;
    float zoom = 1, tSparo = 0, tPassoPuzzle = 0, tFineScorrimento = 0;
    bool fineScorrimento = false;
    Vector2 checkpoint{ 0, 0 };
    float camX = 0, camY = 0, scossa = 0;
    float coyote = 0, bufferSalto = 0, tFine = 0, tPasso = 0;
    bool saltoTagliato = false;
    int saltiFatti = 0;
    float tempoVisivo = 0;
};
