// La Mente di personaggi e oggetti (pilastro 1 dell'IA adattiva).
//
// Ogni creatura della libreria ha uno stato interno (minaccia, paura, fiducia, stanchezza, allerta),
// una piccola memoria episodica e un carattere. Dieci volte al secondo valuta il contesto e sceglie
// un'azione con una "utility AI": ogni azione riceve un punteggio dalle considerazioni del momento,
// pesate dal carattere e dal profilo scritto nella sezione Comportamenti; vince il punteggio piu' alto.
// L'isteresi (bonus all'azione in corso, durata minima) impedisce di cambiare idea a scatti.
//
// Le creature della stessa specie condividono un "Sapere": imparano come le sconfigge il giocatore
// e si adattano (schivano, tengono le distanze), sempre entro limiti fissi.
//
// Qui ci sono solo dati e calcoli puri: il movimento vero lo fa il motore (gioco_libreria.cpp).
#pragma once
#include "raylib.h"
#include <cstdint>
#include <string>
#include <vector>

// --- Numeri casuali deterministici -----------------------------------------------------------------
// Stesso seme = stessa sequenza, su ogni PC: e' cio' che rende ripetibile una partita e affidabile il caricamento.
struct Generatore {
    uint64_t s = 0x9E3779B97F4A7C15ull;
    void Semina(uint64_t seme);
    uint64_t Prossimo();                      // xorshift64*
    float Float();                            // 0 .. 1
    float Float(float lo, float hi);
    int Intero(int lo, int hi);               // lo .. hi compresi
    bool Probabile(float p) { return Float() < p; }
};
uint64_t Mescola64(uint64_t x);               // "splitmix64": sparpaglia i bit di un numero
uint64_t ImprontaTesto(const std::string& s); // FNV-1a a 64 bit (impronta di un gioco)
std::string Esadecimale64(uint64_t v);
bool DaEsadecimale64(const std::string& t, uint64_t& v);

// --- Profili di comportamento (sezione Comportamenti) ---------------------------------------------

enum class AzioneMente { Pattuglia, Insegui, Attacca, Aggira, Cerca, Fuggi, Curati, ChiamaAiuto, Schiva, Guardia, Riposa, Amico, Rivale, Totale };
constexpr int NUM_AZIONI_MENTE = (int)AzioneMente::Totale;
const char* NomeAzioneMente(AzioneMente a);     // "Insegui"
const char* ChiaveAzioneMente(AzioneMente a);   // "insegui" (JSON)
const char* SpiegaAzioneMente(AzioneMente a);   // una riga per l'editor

struct Carattere {
    float coraggio = 0.5f;        // poco coraggio = la paura sale in fretta
    float aggressivita = 0.5f;    // voglia di inseguire e attaccare
    float prudenza = 0.4f;        // preferisce distanza, cure, imboscate
    float socievolezza = 0.3f;    // chiama aiuto, risponde ai richiami, puo' diventare amico
    float rancore = 0.5f;         // quanto a lungo ricorda i torti (perdona piano)
    float curiosita = 0.4f;       // esplora, va a cercare l'eroe sparito
    float imprevedibilita = 0.3f; // un pizzico di sorpresa nelle scelte
};

struct ProfiloComportamento {
    std::string nome = "Nuovo profilo";
    std::string autore;
    std::string descrizione;
    Carattere car;
    float apprendimento = 0.35f;  // 0 = non impara mai (comportamento fisso), 1 = impara in fretta
    float variabilita = 0.25f;    // quanto cambia il carattere da una partita all'altra
    float fuga = 0.3f;            // sotto questa energia (frazione) pensa seriamente a fuggire
    float durataMinima = 0.6f;    // secondi prima di poter cambiare azione
    float peso[NUM_AZIONI_MENTE];       // 0 = azione vietata, 1 normale, fino a 3 = preferita
    bool chiamaAiuto = true;      // grida ai compagni quando vede l'eroe
    bool imparaSpecie = true;     // impara insieme agli altri della stessa specie
    bool ricordaPartite = false;  // quello che impara resta per la partita dopo
    // Abitanti: cosa dicono secondo l'umore (vuoto = la frase del personaggio).
    std::string fraseAmico, fraseNeutra, fraseOstile, frasePaura;
    ProfiloComportamento();
};

// Modelli pronti ("Istintivo", "Lupo di branco"...).
const std::vector<ProfiloComportamento>& ModelliProfilo();

// --- Indole degli oggetti -------------------------------------------------------------------------------
struct IndoleOggetto {
    bool attiva = false;
    float usura = 0;          // quanto si consuma a ogni uso (0 = mai, 0.25 = si rompe al quarto uso)
    float ricarica = 0;       // secondi per tornare come nuovo dopo rotto o raccolto (0 = mai)
    float adattamento = 0;    // -1..1: con l'uso si indebolisce (-) o si rinforza (+)
    bool notturno = false;    // pericoli: attivo solo al buio o di notte
    float risveglio = 0;      // pericoli: secondi con l'eroe vicino prima di attivarsi (0 = sempre attivo)
};

// --- Varieta' della partita (pilastro 2) -------------------------------------------------------------
struct VariabileCasuale { std::string nome; float minimo = 0, massimo = 10; bool intero = true; };

struct VarietaPartita {
    bool mente = true;            // IA adattiva accesa
    int livello = 2;              // 0 spenta, 1 leggera, 2 media, 3 forte
    bool carattere = true;        // il carattere delle creature cambia un po'
    bool veterani = true;         // qualche creatura piu' forte o piu' timida
    bool ricolloca = true;        // monete, gemme e cure si scambiano di posto
    bool meteo = true;            // ora del giorno e pioggia
    bool eventi = true;           // un evento speciale per partita
    bool relazioni = true;        // specie alleate e rivali, fiducia iniziale degli abitanti
    uint32_t semeFisso = 0;       // 0 = un seme nuovo a ogni partita
    std::vector<std::string> eventiVietati;
    std::vector<VariabileCasuale> variabiliCasuali;
};

struct EventoPartita { const char* chiave; const char* nome; const char* descrizione; };
const std::vector<EventoPartita>& EventiPartita();

// --- La mente di una creatura ----------------------------------------------------------------------------
enum class TipoRicordo : int { VistoEroe, ColpitoDaEroe, Risparmiato, CompagnoCaduto, Aiutato, HaColpitoEroe, Richiamo, Totale };

struct Ricordo { TipoRicordo tipo = TipoRicordo::VistoEroe; float quando = 0; Vector2 dove{ 0, 0 }; float forza = 0; };

// Quello che il motore vede intorno alla creatura (calcolato a ogni "pensiero").
struct Contesto {
    float tempo = 0;
    float distanza = 10;      // distanza dall'eroe in "viste" (1 = al limite della vista)
    float vede = 0;           // 1 se vede l'eroe
    float energia = 1;        // energia propria (0..1)
    float energiaEroe = 1;    // energia dell'eroe (0..1)
    float alleati = 0;        // compagni vicini
    float cura = 0;           // 1 se c'e' una cura vicina
    float rivale = 0;         // 1 se c'e' un rivale (o un nemico, per un amico) vicino
    float eroeSopra = 0;      // l'eroe gli sta cadendo addosso
    float proiettile = 0;     // un proiettile dell'eroe sta arrivando
    float lontanoCasa = 0;    // distanza dall'origine (0..1)
    float memoria = 0;        // ricorda dove ha visto l'eroe l'ultima volta
    float puoSparare = 0;
    float chiuso = 0;         // spalle al muro o al burrone
    float pieta = 0;          // l'eroe e' in difficolta' (difficolta' dinamica)
    bool abitante = false;    // personaggio che parla
    bool boss = false;
};

// Quello che una specie ha imparato sul giocatore (medie mobili, tutte fra 0 e 1).
struct Sapere {
    float schiacciato = 0;    // muoiono saltandoci sopra
    float colpito = 0;        // muoiono per i proiettili
    float colpiDati = 0;      // riescono a ferire l'eroe
    float saltiEroe = 0;      // il giocatore salta molto vicino a loro
    float spariEroe = 0;      // il giocatore spara molto
    int esperienze = 0;
    // Tendenze apprese (al massimo 0.6): schivare i salti, saltare i proiettili, tenere le distanze.
    float Schiva() const;
    float EvitaColpi() const;
    float Distanza() const;
    void Impara(float& valore, float bersaglio, float apprendimento);
};

struct Mente {
    bool attiva = false;
    Carattere car;
    float minaccia = 0, paura = 0, fiducia = 0, fiduciaBase = 0, stanchezza = 0, allerta = 0;
    AzioneMente azione = AzioneMente::Pattuglia;
    float tAzione = 0, tPensa = 0, tRichiamo = 0, tSchiva = 0, tScatto = 0, tColpoEroe = 99, tRivale = 0;
    Vector2 ultimoVisto{ -1, -1 };
    float tVisto = 999;
    int bersaglio = -1;               // entita' bersaglio (cura, rivale) - non si salva, si ricalcola
    Ricordo ricordi[8];
    int numRicordi = 0, prossimo = 0;
    float punteggi[NUM_AZIONI_MENTE] = {};  // ultimo calcolo (pensieri con F9, simulatore)

    void Ricorda(TipoRicordo t, float quando, Vector2 dove, float forza);
    int Conta(TipoRicordo t, float dopo = -1) const;
    void Sbiadisci(float ora);        // i ricordi vecchi si perdono
};

float PunteggioAzione(const Mente& m, const ProfiloComportamento& p, const Sapere& s, const Contesto& c, AzioneMente a);
// Ricalcola i punteggi e sceglie (con isteresi e un pizzico di sorpresa dal generatore della partita).
AzioneMente Decidi(Mente& m, const ProfiloComportamento& p, const Sapere& s, const Contesto& c, Generatore& rng);
// Emozioni che salgono e scendono col tempo (a ogni fotogramma).
void AggiornaEmozioni(Mente& m, const ProfiloComportamento& p, const Contesto& c, float dt);
// Carattere di una creatura in questa partita: quello del profilo con una variazione dal seme.
Carattere CarattereDellaPartita(const Carattere& base, float variabilita, Generatore& rng);
// Frase di un abitante secondo l'umore.
std::string FraseUmore(const Mente& m, const ProfiloComportamento& p, const std::string& frase);
// Riga breve dei pensieri (F9): "Insegui  minaccia 0.4  paura 0.1".
std::string Pensiero(const Mente& m);
