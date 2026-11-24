// I tre grandi giochi di piattaforme d'esempio (Lumina, Nora e Bullone, Nilo), creati
// interamente in C++ con il Pennello: personaggi animati, paesaggi a strati, caselle,
// oggetti, livelli, musiche ed effetti finiscono in un normale progetto .gioco,
// che si apre e si modifica nello Studio come qualsiasi altro gioco.
#pragma once
#include "arte.h"
#include "progetto_gioco.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace piatt {

using arte::K;
using arte::Tela;

// Come si disegna un gioco: HD liscio (r = 1) o pixel art fine (r = 0.5, pixel netti).
struct Resa {
    bool hd = false;
    float r = 0.5f;        // risoluzione dei paesaggi rispetto a 1920x720 logici
    float dens = 2;        // pixel del disegno per pixel del mondo (personaggi, oggetti, caselle)
};

// --- immagini e disegni ---------------------------------------------------------------
std::string Png(const Image& img);
void AggiungiImmagine(ProgettoGioco& p, const std::string& nome, const Tela& t);
// Disegno di un oggetto: f(c, animazione, i, n) disegna in unita' del mondo, in un riquadro w x h
// con la base in basso al centro (x da -w/2 a w/2, y da -h a 0).
using DisegnaOggetto = std::function<void(K& c, const std::string& anim, int i, int n)>;
struct AnimOggetto { std::string nome; int fotogrammi = 1; float fps = 8; bool ripeti = true; };
Disegno CreaOggettoDisegno(float w, float h, const Resa& resa, const std::vector<AnimOggetto>& anims, const DisegnaOggetto& f);
// Personaggio: altezza nel mondo e ingombro della funzione di disegno.
Disegno CreaPersonaggio(const arte::DisegnaPersonaggio& f, float altezzaMondo, const Resa& resa, float alto = 175, float largo = 130, float sopra = 0);

// --- paesaggi ----------------------------------------------------------------------------
// Uno scenario fatto di immagini: cielo fisso, strati in parallasse, primo piano sfocato.
struct StratoArte { float parallasse = 0.2f; std::function<void(K&)> disegna; };
struct ScenarioArte {
    std::string nome;
    std::function<void(K&)> cielo;              // 1280x720 logici
    std::vector<StratoArte> strati;              // 1920x720 logici, ripetuti
    std::string primoPiano; std::string tipoPrimoPiano = "leaves";
    int atmosfera = 0;                           // 0 niente, 1 pioggia, 2 neve, 3.. lucciole, braci... (vedi Gioco::DisegnaPioggia)
};
Scenario CreaScenarioArte(ProgettoGioco& p, const ScenarioArte& s, const Resa& resa);

// Caselle del terreno disegnate con un materiale (superficie e interno, con varianti).
CasellaDef CreaCasella(char carattere, const std::string& nome, const arte::Materiale& m, const Resa& resa, unsigned seme);
CasellaDef CreaCasellaDisegnata(char carattere, const std::string& nome, const Resa& resa, const DisegnaOggetto& f, int varianti);

// --- musica ed effetti ------------------------------------------------------------------
// Melodia come testo: "C5:2 E5:1 R:1 G#4:4" (durate in ottavi; R = pausa).
// Batteria: 16 caratteri per battuta, K cassa, S rullante, h charleston, o piatto, . niente.
struct TracciaTesto { std::string nome; int strumento = 0; float volume = 0.5f; std::string note; bool batteria = false; int ottava = 0; };
Brano CreaBrano(const std::string& nome, float bpm, int battute, const std::vector<TracciaTesto>& tracce);
EffettoSuono Effetto(const std::string& nome, int onda, float f0, float f1, float durata, float volume, float rumore = 0, float vibrato = 0, float vibratoHz = 0);

// --- livelli a pezzi --------------------------------------------------------------------
// Un pezzo di livello: righe di caratteri tutte larghe uguali, alte RIGHE_PEZZO.
//   # terreno  = blocco  - piattaforma  ^ spine
//   o ingranaggio/moneta  h cuore  e nemico a terra  j nemico che salta/insegue  f nemico in volo
//   s nemico che spara  m molla  p piattaforma mobile  c punto di controllo  k chiave  d porta chiusa
//   B boss  g cancello del boss  G arrivo  S partenza
constexpr int RIGHE_PEZZO = 12;
constexpr int RIGHE_MAPPA = 15;
struct Pezzo { std::vector<std::string> righe; int difficolta = 0; };
// Nomi della libreria per ogni lettera ('e' -> "Raptor"...).
using Legenda = std::map<char, std::string>;
// Costruisce una mappa unendo i pezzi; mappaDopo: indice della mappa a cui porta l'arrivo (-1 = fine del gioco).
Mappa CostruisciMappa(const std::string& nome, const std::vector<const Pezzo*>& pezzi, const Legenda& leg, int mappaDopo, int righeSopra = RIGHE_MAPPA - RIGHE_PEZZO);
// Sceglie i pezzi di un livello: inizio, n pezzi del mondo con difficolta' crescente, controllo a meta', arrivo.
std::vector<const Pezzo*> ScegliPezzi(const std::vector<Pezzo>& tutti, const Pezzo& inizio, const Pezzo& controllo, const Pezzo& fine,
                                      int quanti, int livello, unsigned seme);
const std::vector<Pezzo>& PezziComuni();
const Pezzo& PezzoInizio();
const Pezzo& PezzoControllo();
const Pezzo& PezzoFine();
const Pezzo& PezzoBoss();

// --- un gioco a mondi ----------------------------------------------------------------------
// Ogni mondo: scenario, terreno, nemici (e a terra, j salta/insegue, f vola, s spara, boss) e musica.
struct MondoGioco {
    std::string nome;
    ScenarioArte scen;
    arte::Materiale terra, blocco;
    std::string legnoPiatt = "#8a6a3a", luceMobile = "#78f0ff";
    std::string e, j, f, s, boss, brano, frase;
    float buio = 0;          // 0 di giorno .. 1 buio pesto (si vede solo intorno alle luci)
    bool acqua = false;      // sott'acqua: si nuota
};
// Nomi degli oggetti comuni a tutti i mondi del gioco.
struct NomiGioco {
    std::string moneta, cuore, molla, controllo, chiave, porta, cancello, arrivo;
    std::string branoBoss, branoVittoria, fraseBoss, fraseFinale;
    int energiaBoss = 10;
    unsigned seme = 1;
};
// Scenari, caselle, piattaforme mobili, 4 mappe per mondo (3 livelli + guardiano) e regole.
void CostruisciMondi(ProgettoGioco& p, const Resa& resa, std::vector<MondoGioco>& mondi, const NomiGioco& n);

// --- i tre giochi -------------------------------------------------------------------------
ProgettoGioco CreaNoraEBullone();
ProgettoGioco CreaLumina();
ProgettoGioco CreaNilo();

}  // namespace piatt

// Riga di comando: StudioVideogiochi --crea-platform A|B|C file.gioco [Gioco.exe]
int CreaPlatformDaRigaDiComando(int argc, char** argv);
