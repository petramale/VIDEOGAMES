// IL REGNO DI QUERCIAVERDE - avventura punta-e-clicca in scene isometriche dipinte.
// Gioco originale di Mario Petramale - Elvantis. Lo stregone Morgrano ha spezzato la Ghianda d'Oro,
// cuore del regno: l'eroe attraversa 12 capitoli (5 localita' ciascuno) raccogliendo risorse con un
// clic, aiutando gli abitanti (missioni "!"), commerciando al mercato, creando oggetti con le ricette e
// sconfiggendo i nemici con un clic in base alla forza dell'equipaggiamento. Si salva in ogni momento.
//
// Tutto e' nei dati del progetto (modificabili nello Studio): oggetti, ricette, nemici, capitoli,
// localita' con le loro uscite, gli elementi di ogni scena (persone, risorse, nemici, ostacoli,
// forzieri, case...) e le missioni.
#include "arcade.h"
#include "flusso.h"
#include "ui.h"
#include "json.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>

using nlohmann::json;

namespace arcade {
namespace {
using namespace arte;

constexpr float TAU = 6.2831853f;
constexpr float PI_ = 3.14159265f;

// --------------------------------------------------------------------------------------------
// Elenchi fissi (le voci delle scelte nello schema)
// --------------------------------------------------------------------------------------------
const char* SCENE[12] = { "Borgo", "Bosco", "Campi", "Fiume", "Miniera", "Palude", "Montagna", "Spiaggia", "Rovine", "Castello", "Deserto", "Grotta" };
const char* ORE[5] = { "Giorno", "Mattino", "Tramonto", "Notte", "Nebbia" };
const char* TIPI_OGGETTO[8] = { "Risorsa", "Cibo (energia)", "Cura (vita)", "Attrezzo", "Arma", "Armatura", "Zaino (posti in piu')", "Oggetto di missione" };
const char* TIPI_PUNTO[8] = { "Decorazione", "Risorsa", "Persona", "Nemico", "Mercante", "Ostacolo", "Forziere", "Riposo" };
const char* TIPI_MISSIONE[4] = { "Porta oggetti", "Sconfiggi", "Libera la strada", "Parla con" };
enum Forma {
    F_CONTADINO, F_ANZIANO, F_DAMA, F_GUARDIA, F_MONACO, F_MERCANTE, F_RE, F_BAMBINO,
    F_ALBERO, F_ABETE, F_MELO, F_CESPUGLIO, F_FUNGHI, F_ERBE, F_GRANO, F_CANNE, F_ROCCIA, F_FERRO, F_CARBONE, F_CRISTALLI,
    F_POZZO, F_PESCI, F_CONCHIGLIE, F_ALVEARE, F_PALMA, F_CACTUS, F_RAME, F_DUNA, F_SALINA,
    F_CASA_PAGLIA, F_CASA_TEGOLE, F_TORRE, F_MULINO, F_TENDA, F_CARRETTO, F_BARILI, F_CASSE, F_RECINTO, F_FALO, F_COLONNE,
    F_STATUA, F_BARCA, F_FIORI, F_PONTILE,
    F_TRONCO, F_PONTE, F_FRANA, F_ROVI, F_CANCELLO, F_GHIACCIO,
    F_FORZIERE, F_QUERCIA, F_FARO, N_FORME
};
const char* FORME[N_FORME] = { "Contadino", "Anziano", "Dama", "Guardia", "Monaco", "Mercante con bancarella", "Re", "Bambino",
    "Albero", "Abete", "Melo", "Cespuglio di bacche", "Funghi", "Erbe", "Grano", "Canne", "Roccia", "Filone di ferro", "Filone di carbone", "Cristalli",
    "Pozzo", "Pesci nell'acqua", "Conchiglie", "Alveare", "Palma", "Cactus", "Filone di rame", "Duna di sabbia", "Salina",
    "Casa di paglia", "Casa di tegole", "Torre di pietra", "Mulino", "Tenda", "Carretto", "Barili", "Casse", "Recinto", "Falo'", "Colonne in rovina",
    "Statua", "Barca", "Fiori", "Pontile",
    "Tronco caduto", "Ponte rotto", "Frana di massi", "Rovi", "Cancello chiuso", "Muro di ghiaccio",
    "Forziere", "Grande quercia", "Faro" };
enum FormaNemico { N_LUPO, N_BRIGANTE, N_CINGHIALE, N_RAGNO, N_SCHELETRO, N_SERPENTE, N_ORSO, N_TROLL, N_SPIRITO, N_GOLEM, N_SCORPIONE, N_DRAGO, N_STREGONE, N_CAVALIERE, N_FORME_NEMICO };
const char* FORME_NEMICO[N_FORME_NEMICO] = { "Lupo", "Brigante", "Cinghiale", "Ragno", "Scheletro", "Serpente", "Orso", "Troll", "Spirito", "Golem", "Scorpione", "Drago", "Stregone", "Cavaliere" };
enum Icona {
    I_LEGNA, I_PIETRA, I_MINERALE, I_CARBONE, I_BACCHE, I_FUNGHI, I_GRANO, I_MELA, I_PESCE, I_ACQUA, I_ERBE, I_CANNE, I_CONCHIGLIA, I_PEPITA, I_CRISTALLO, I_PELLE,
    I_MIELE, I_SALE, I_SABBIA, I_TAVOLA, I_CORDA, I_LINGOTTO, I_PANE, I_ZUPPA, I_POZIONE, I_TORCIA, I_MATTONI, I_VETRO, I_ASCIA, I_PICCONE, I_FALCE, I_CANNA,
    I_SECCHIO, I_LANTERNA, I_PALA, I_BASTONE, I_SPADA, I_LANCIA, I_ARCO, I_GIUBBA, I_SCUDO, I_ARMATURA, I_ZAINO, I_LETTERA, I_CHIAVE, I_GHIANDA, I_AMULETO, I_FORMAGGIO, N_ICONE
};
const char* ICONE[N_ICONE] = { "Legna", "Pietra", "Minerale", "Carbone", "Bacche", "Funghi", "Grano", "Mela", "Pesce", "Fiasca d'acqua", "Erbe", "Canne", "Conchiglia",
    "Pepita", "Cristallo", "Pelle", "Miele", "Sale", "Sabbia", "Tavola", "Corda", "Lingotto", "Pane", "Zuppa", "Pozione", "Torcia", "Mattoni", "Vetro", "Ascia", "Piccone",
    "Falce", "Canna da pesca", "Secchio", "Lanterna", "Pala", "Bastone", "Spada", "Lancia", "Arco", "Giubba", "Scudo", "Armatura", "Zaino", "Lettera", "Chiave",
    "Ghianda", "Amuleto", "Formaggio" };

// --------------------------------------------------------------------------------------------
// Il contenuto predefinito (12 capitoli): oggetti, ricette, nemici e la tabella dei capitoli
// --------------------------------------------------------------------------------------------
enum Ogg {
    O_LEGNA, O_PIETRA, O_FERRO, O_CARBONE, O_BACCHE, O_FUNGHI, O_GRANO, O_MELA, O_PESCE, O_ACQUA, O_ERBE, O_CANNE, O_CONCHIGLIE, O_RAME, O_CRISTALLO, O_PELLE,
    O_MIELE, O_SALE, O_SABBIA, O_TAVOLA, O_CORDA, O_LINGOTTO, O_PANE, O_ZUPPA, O_POZIONE, O_TORCIA, O_MATTONI, O_VETRO,
    O_ASCIA, O_PICCONE, O_FALCE, O_CANNA, O_SECCHIO, O_LANTERNA, O_PALA,
    O_BASTONE, O_SPADA_CORTA, O_SPADA_FERRO, O_ARCO, O_LANCIA, O_SPADA_CRISTALLO, O_LAMA,
    O_GIUBBA, O_SCUDO, O_COTTA, O_MANTELLO, O_ARMATURA_RAME, O_ARMATURA_CRISTALLO,
    O_ZAINO, O_LETTERA, O_CHIAVE, O_FRAMMENTO, O_AMULETO, O_FORMAGGIO, N_OGG
};
struct DefOggetto { const char* nome; int tipo, icona; const char* colore; int valore, prezzo, negozio; };
const DefOggetto DEF_OGGETTI[N_OGG] = {
    { "Legna", 0, I_LEGNA, "#8a5a30", 0, 2, 1 }, { "Pietra", 0, I_PIETRA, "#a09a8c", 0, 2, 1 }, { "Ferro grezzo", 0, I_MINERALE, "#9a6a5a", 0, 4, 3 },
    { "Carbone", 0, I_CARBONE, "#2a2a2e", 0, 3, 3 }, { "Bacche", 1, I_BACCHE, "#c03050", 2, 2, 1 }, { "Funghi", 1, I_FUNGHI, "#c8a070", 3, 3, 0 },
    { "Grano", 0, I_GRANO, "#e0c060", 0, 2, 1 }, { "Mela", 1, I_MELA, "#c83030", 2, 2, 1 }, { "Pesce", 1, I_PESCE, "#7a9ab0", 3, 4, 4 },
    { "Acqua", 0, I_ACQUA, "#4a8ad0", 0, 1, 1 }, { "Erbe", 0, I_ERBE, "#5a9a3a", 0, 3, 2 }, { "Canne", 0, I_CANNE, "#a0a050", 0, 1, 0 },
    { "Conchiglie", 0, I_CONCHIGLIA, "#f0d0c0", 0, 6, 0 }, { "Rame", 0, I_PEPITA, "#d07a3a", 0, 6, 8 }, { "Cristallo", 0, I_CRISTALLO, "#7adcff", 0, 10, 0 },
    { "Pelle", 0, I_PELLE, "#a07040", 0, 5, 2 }, { "Miele", 1, I_MIELE, "#f0a020", 4, 6, 0 }, { "Sale", 0, I_SALE, "#f0f0f0", 0, 4, 4 },
    { "Sabbia", 0, I_SABBIA, "#e0c890", 0, 1, 0 }, { "Tavola", 0, I_TAVOLA, "#b08050", 0, 6, 0 }, { "Corda", 0, I_CORDA, "#c0a060", 0, 6, 0 },
    { "Lingotto di ferro", 0, I_LINGOTTO, "#a0a8b0", 0, 14, 0 }, { "Pane", 1, I_PANE, "#d0a060", 5, 8, 1 }, { "Zuppa", 1, I_ZUPPA, "#c06030", 7, 10, 0 },
    { "Pozione", 2, I_POZIONE, "#e03060", 5, 15, 2 }, { "Torcia", 0, I_TORCIA, "#f0a030", 0, 5, 0 }, { "Mattoni", 0, I_MATTONI, "#b05a3a", 0, 6, 0 },
    { "Vetro", 0, I_VETRO, "#a0e0f0", 0, 8, 0 },
    { "Ascia", 3, I_ASCIA, "#9aa0a8", 0, 20, 1 }, { "Piccone", 3, I_PICCONE, "#9aa0a8", 0, 30, 2 }, { "Falce", 3, I_FALCE, "#9aa0a8", 0, 20, 1 },
    { "Canna da pesca", 3, I_CANNA, "#8a6a40", 0, 25, 3 }, { "Secchio", 3, I_SECCHIO, "#8a6a40", 0, 12, 1 }, { "Lanterna", 3, I_LANTERNA, "#f0c040", 0, 40, 0 },
    { "Pala", 3, I_PALA, "#9aa0a8", 0, 25, 5 },
    { "Bastone", 4, I_BASTONE, "#8a5a30", 1, 5, 0 }, { "Spada corta", 4, I_SPADA, "#c0c0c8", 3, 60, 1 }, { "Spada di ferro", 4, I_SPADA, "#d8e0e8", 6, 150, 0 },
    { "Arco del cacciatore", 4, I_ARCO, "#8a5a30", 8, 200, 0 }, { "Lancia di rame", 4, I_LANCIA, "#d07a3a", 9, 260, 0 },
    { "Spada di cristallo", 4, I_SPADA, "#7adcff", 12, 400, 0 }, { "Lama di Querciaverde", 4, I_SPADA, "#f0d060", 16, 600, 0 },
    { "Giubba di cuoio", 5, I_GIUBBA, "#8a5a30", 1, 20, 0 }, { "Scudo di legno", 5, I_SCUDO, "#a07040", 2, 40, 2 }, { "Cotta di ferro", 5, I_ARMATURA, "#a0a8b0", 4, 120, 0 },
    { "Mantello del monaco", 5, I_GIUBBA, "#6a5a8a", 5, 160, 0 }, { "Armatura di rame", 5, I_ARMATURA, "#d07a3a", 7, 260, 0 },
    { "Armatura di cristallo", 5, I_ARMATURA, "#7adcff", 9, 400, 0 },
    { "Zaino grande", 6, I_ZAINO, "#8a5a30", 8, 80, 2 }, { "Lettera", 7, I_LETTERA, "#f0e0c0", 0, 0, 0 }, { "Chiave antica", 7, I_CHIAVE, "#d0b050", 0, 0, 0 },
    { "Frammento di Ghianda", 7, I_GHIANDA, "#f0c030", 0, 0, 0 }, { "Amuleto", 7, I_AMULETO, "#60c0a0", 0, 0, 0 }, { "Formaggio", 1, I_FORMAGGIO, "#f0d060", 4, 6, 1 } };

struct DefRicetta { int ris, q, i1, n1, i2, n2, i3, n3, cap; };
const DefRicetta DEF_RICETTE[] = {
    { O_TAVOLA, 1, O_LEGNA, 2, -1, 0, -1, 0, 1 }, { O_CORDA, 1, O_CANNE, 3, -1, 0, -1, 0, 1 }, { O_PANE, 1, O_GRANO, 2, O_ACQUA, 1, -1, 0, 1 },
    { O_POZIONE, 1, O_ERBE, 2, O_ACQUA, 1, -1, 0, 1 }, { O_TORCIA, 2, O_LEGNA, 1, O_CANNE, 1, -1, 0, 2 }, { O_GIUBBA, 1, O_PELLE, 2, O_CORDA, 1, -1, 0, 2 },
    { O_SCUDO, 1, O_TAVOLA, 2, O_CORDA, 1, -1, 0, 2 }, { O_LINGOTTO, 1, O_FERRO, 2, O_CARBONE, 1, -1, 0, 3 }, { O_SPADA_FERRO, 1, O_LINGOTTO, 2, O_TAVOLA, 1, -1, 0, 3 },
    { O_COTTA, 1, O_LINGOTTO, 3, O_PELLE, 1, -1, 0, 3 }, { O_ZUPPA, 1, O_PESCE, 1, O_ERBE, 1, O_ACQUA, 1, 4 }, { O_MATTONI, 2, O_PIETRA, 2, O_ACQUA, 1, -1, 0, 4 },
    { O_VETRO, 1, O_SABBIA, 2, O_CARBONE, 1, -1, 0, 8 }, { O_LANCIA, 1, O_RAME, 3, O_TAVOLA, 1, -1, 0, 8 }, { O_ARMATURA_RAME, 1, O_RAME, 4, O_PELLE, 2, -1, 0, 8 },
    { O_SPADA_CRISTALLO, 1, O_CRISTALLO, 3, O_LINGOTTO, 1, -1, 0, 10 }, { O_ARMATURA_CRISTALLO, 1, O_CRISTALLO, 4, O_LINGOTTO, 2, -1, 0, 10 },
    { O_LANTERNA, 1, O_VETRO, 1, O_LINGOTTO, 1, -1, 0, 8 } };

struct DefNemico { const char* nome; int forma; const char* colore; int potenza, oro, lascia, quantita; bool grande; };
enum Nem { E_LUPO, E_CAPOBRANCO, E_BRIGANTE, E_CAPOBRIGANTE, E_CINGHIALE, E_RAGNO, E_REGINA, E_PIRATA, E_NOSTROMO, E_CAPITANO, E_SERPENTE, E_SPIRITO,
           E_ORSO, E_TROLLGHIACCI, E_SCHELETRO, E_GUARDIANO, E_SCORPIONE, E_GOLEMRAME, E_SERPMARINO, E_ANNEGATO, E_TROLL, E_GIGANTE, E_APPRENDISTA, E_DRAGO,
           E_CAVALIERE, E_MORGRANO, N_NEM };
const DefNemico DEF_NEMICI[N_NEM] = {
    { "Lupo grigio", N_LUPO, "#8a8a90", 3, 5, O_PELLE, 1, false }, { "Capobranco", N_LUPO, "#4a4650", 4, 20, O_FRAMMENTO, 1, true },
    { "Brigante", N_BRIGANTE, "#8a3a2a", 5, 12, -1, 0, false }, { "Capo dei briganti", N_BRIGANTE, "#3a2a5a", 6, 40, O_FRAMMENTO, 1, true },
    { "Cinghiale", N_CINGHIALE, "#6a4a32", 4, 4, O_PELLE, 1, false }, { "Ragno delle grotte", N_RAGNO, "#4a3a50", 8, 10, -1, 0, false },
    { "Regina dei ragni", N_RAGNO, "#7a2a6a", 10, 60, O_FRAMMENTO, 1, true }, { "Pirata", N_BRIGANTE, "#2a4a7a", 9, 15, -1, 0, false },
    { "Nostromo", N_BRIGANTE, "#4a5a2a", 9, 20, O_CHIAVE, 1, false }, { "Capitano Salmastro", N_BRIGANTE, "#7a1a1a", 11, 80, O_FRAMMENTO, 1, true },
    { "Serpente di palude", N_SERPENTE, "#4a7a2a", 11, 15, O_PELLE, 1, false }, { "Spirito della palude", N_SPIRITO, "#8affc0", 13, 90, O_FRAMMENTO, 1, true },
    { "Orso delle nevi", N_ORSO, "#e8e8f0", 12, 20, O_PELLE, 2, false }, { "Troll dei ghiacci", N_TROLL, "#8ab8e0", 13, 100, O_FRAMMENTO, 1, true },
    { "Scheletro", N_SCHELETRO, "#e8e0c8", 13, 20, -1, 0, false }, { "Guardiano di pietra", N_GOLEM, "#8a8a7a", 14, 110, O_FRAMMENTO, 1, true },
    { "Scorpione del deserto", N_SCORPIONE, "#c08040", 15, 25, -1, 0, false }, { "Golem di rame", N_GOLEM, "#d07a3a", 17, 130, O_FRAMMENTO, 1, true },
    { "Serpente marino", N_SERPENTE, "#2a7a9a", 16, 25, O_PESCE, 2, false }, { "Spirito annegato", N_SPIRITO, "#6ac0ff", 17, 140, O_FRAMMENTO, 1, true },
    { "Troll di montagna", N_TROLL, "#6a7a4a", 20, 30, -1, 0, false }, { "Gigante di pietra", N_GOLEM, "#6a6a70", 22, 160, O_FRAMMENTO, 1, true },
    { "Apprendista stregone", N_STREGONE, "#4a2a7a", 21, 40, -1, 0, false }, { "Drago del faro", N_DRAGO, "#b03020", 22, 200, O_FRAMMENTO, 1, true },
    { "Cavaliere oscuro", N_CAVALIERE, "#2a2a3a", 24, 50, O_CHIAVE, 1, false }, { "Morgrano lo stregone", N_STREGONE, "#2a0a3a", 26, 500, O_FRAMMENTO, 1, true } };

// Un capitolo predefinito: 5 localita' (0 villaggio, 1 e 2 risorse, 3 nemico e ostacolo, 4 boss),
// 4 personaggi e 6 missioni sempre con lo stesso schema (porta, crea, sconfiggi, libera, boss, frammento).
struct DefRisorsa { int forma, ogg, q, attrezzo, costo, usi; float ric; };
struct DefCapitolo {
    const char* nome; const char* intro;
    const char* luoghi[5]; int scene[5]; int ore[5];
    const char* npc[4]; int formeNpc[4]; const char* coloriNpc[4];
    const char* mercante; const char* riposo; int formaRiposo;
    std::vector<DefRisorsa> ris[5];
    int r1, n1, premio1, premioN1, oro1;               // missione 1: porta r1 x n1
    int r2, n2, premio2, premioN2, oro2;               // missione 2: porta r2 (da ricetta) x n2
    int nemico, premio3, premioN3, oro3;               // missione 3: sconfiggi
    int formaOstacolo, ostacolo, nOstacolo; const char* nomeOstacolo;   // missione 4: libera la strada (nord di 3)
    int boss, oro5;                                    // missione 5: il boss in 4
    int premio6, premioN6, oro6;                       // missione 6: il frammento all'anziano
    const char* testi[6];
    const char* fine6;
    const char* musica;
    int extraNemico; int forziere;                     // nemico che ricompare in 1/2; forziere: 0 oro, 1 chiave (in 2)
};

#define ALB { F_ALBERO, O_LEGNA, 2, O_ASCIA, 1, 3, 45 }
#define ABE { F_ABETE, O_LEGNA, 2, O_ASCIA, 1, 3, 45 }
#define CES { F_CESPUGLIO, O_BACCHE, 2, -1, 1, 2, 30 }
#define FUN { F_FUNGHI, O_FUNGHI, 1, -1, 1, 1, 50 }
#define GRA { F_GRANO, O_GRANO, 2, O_FALCE, 1, 3, 40 }
#define MEL { F_MELO, O_MELA, 2, -1, 1, 2, 40 }
#define POZ { F_POZZO, O_ACQUA, 2, O_SECCHIO, 0, 0, 0 }
#define CAN { F_CANNE, O_CANNE, 2, -1, 1, 3, 35 }
#define ERB { F_ERBE, O_ERBE, 1, -1, 1, 2, 35 }
#define ROC { F_ROCCIA, O_PIETRA, 2, O_PICCONE, 1, 3, 50 }
#define FER { F_FERRO, O_FERRO, 2, O_PICCONE, 2, 3, 60 }
#define CAR { F_CARBONE, O_CARBONE, 2, O_PICCONE, 1, 3, 60 }
#define CRI { F_CRISTALLI, O_CRISTALLO, 1, O_PICCONE, 2, 2, 80 }
#define PES { F_PESCI, O_PESCE, 1, O_CANNA, 1, 3, 40 }
#define CON { F_CONCHIGLIE, O_CONCHIGLIE, 1, -1, 1, 2, 40 }
#define ALV { F_ALVEARE, O_MIELE, 1, -1, 1, 2, 50 }
#define RAM { F_RAME, O_RAME, 2, O_PICCONE, 2, 3, 60 }
#define DUN { F_DUNA, O_SABBIA, 2, O_PALA, 1, 0, 0 }
#define SAL { F_SALINA, O_SALE, 1, -1, 1, 2, 50 }

const std::vector<DefCapitolo>& Capitoli() {
    static const std::vector<DefCapitolo> c = {
    { "Il Borgo di Querciaverde",
      "Lo stregone Morgrano ha spezzato la Ghianda d'Oro, il cuore del regno, e ne ha sparso i dodici frammenti. Senza la Ghianda i campi seccano e le bestie diventano feroci. Il vecchio Fulgenzio ti aspetta nel borgo.",
      { "Borgo di Querciaverde", "Bosco dei Faggi", "Campi del Mulino", "Ruscello Chiaro", "Radura dei Lupi" }, { 0, 1, 2, 3, 1 }, { 0, 0, 1, 0, 2 },
      { "Fulgenzio l'anziano", "Berta la fornaia", "Tobia il mugnaio", "Selva la cacciatrice" }, { F_ANZIANO, F_DAMA, F_CONTADINO, F_GUARDIA },
      { "#6a5a8a", "#a04a3a", "#7a8a3a", "#3a6a3a" }, "Ugo il mercante", "Locanda della Quercia", F_CASA_TEGOLE,
      { { POZ }, { ALB, ALB, ALB, CES, CES, FUN }, { GRA, GRA, MEL, CES }, { CAN, CAN, ERB, ERB, ALB }, { CES } },
      O_LEGNA, 6, O_FALCE, 1, 25, O_PANE, 2, O_POZIONE, 2, 30, E_LUPO, O_PELLE, 1, 30, F_PONTE, O_TAVOLA, 3, "Ponte sul ruscello", E_CAPOBRANCO, 40, O_ZAINO, 1, 50,
      { "Benvenuto! Le notti sono fredde e la legna e' finita. Prendi la tua ascia e portami 6 Legna dal Bosco dei Faggi, a est del borgo.",
        "Il forno e' acceso ma manca il pane! Falcia il grano nei Campi del Mulino, attingi acqua al pozzo e prepara 2 Pani con i Lavori.",
        "Un lupo grigio ronda vicino al Ruscello e spaventa le mie pecore. Con un bastone pero' non basta: compra una spada al mercato del borgo.",
        "Il ponte sul ruscello e' crollato. Con 3 Tavole puoi ripararlo e raggiungere la Radura dei Lupi.",
        "Il Capobranco guida i lupi contro il borgo. Al collo porta qualcosa che brilla... e' un frammento della Ghianda!",
        "Hai trovato un frammento della Ghianda d'Oro? Portamelo: e' il primo di dodici." },
      "Il primo frammento! La quercia del borgo ha gia' una foglia nuova. Prendi questo zaino grande e va' nella Foresta dei Sussurri.",
      "Querciaverde - Borgo", E_LUPO, 0 },
    { "La Foresta dei Sussurri",
      "Tra gli alberi della Foresta dei Sussurri si nascondono i briganti. Hanno rubato il secondo frammento e tengono in pugno i boscaioli.",
      { "Capanna del Boscaiolo", "Sentiero dei Muschi", "Stagno delle Rane", "Quercia Cava", "Accampamento dei Briganti" }, { 1, 1, 5, 1, 1 }, { 0, 1, 0, 4, 2 },
      { "Gualtiero il boscaiolo", "Erminia l'erborista", "Nando il pescatore", "Ilda la ladra pentita" }, { F_CONTADINO, F_DAMA, F_CONTADINO, F_DAMA },
      { "#5a3a2a", "#4a7a5a", "#3a5a7a", "#6a3a5a" }, "Rosina la merciaia", "Capanna dei boscaioli", F_TENDA,
      { { POZ, ALB }, { ABE, ABE, FUN, FUN, FUN, CES }, { CAN, CAN, CAN, ERB, ERB }, { ABE, FUN }, {} },
      O_FUNGHI, 4, O_PANE, 2, 30, O_CORDA, 3, O_SCUDO, 1, 30, E_BRIGANTE, O_POZIONE, 1, 40, F_ROVI, O_TORCIA, 2, "Rovi stregati", E_CAPOBRIGANTE, 50, O_PICCONE, 1, 60,
      { "I briganti ci hanno svuotato la dispensa. Raccogli 4 Funghi lungo il Sentiero dei Muschi, per favore.",
        "Per le mie trappole servono corde: intreccia 3 Corde con le canne dello Stagno delle Rane. Ti daro' uno scudo.",
        "Un brigante fa la guardia alla Quercia Cava e non lascia passare nessuno. Sconfiggilo!",
        "Dietro la Quercia Cava i rovi stregati chiudono il sentiero. Bruciali con 2 Torce.",
        "Ero una di loro, ma ora voglio aiutarti. Il Capo dei briganti custodisce il frammento: battilo!",
        "Il secondo frammento! Portamelo, ne avro' cura." },
      "Grazie, eroe! Prendi questo piccone: alle Miniere di Pietrarossa ti servira'.", "Querciaverde - Bosco", E_CINGHIALE, 0 },
    { "Le Miniere di Pietrarossa",
      "Nelle Miniere di Pietrarossa i minatori hanno smesso di scavare: nelle gallerie si e' annidata la Regina dei ragni, attirata dal terzo frammento.",
      { "Villaggio dei Minatori", "Cava di Pietrarossa", "Bosco dei Carbonai", "Galleria Profonda", "Antro della Regina" }, { 4, 4, 1, 11, 11 }, { 0, 0, 2, 0, 3 },
      { "Bruno il capominatore", "Greta la fabbra", "Carlo il carbonaio", "Pina la minatrice" }, { F_GUARDIA, F_DAMA, F_CONTADINO, F_DAMA },
      { "#7a5a3a", "#8a4a2a", "#3a3a3a", "#6a6a3a" }, "Saverio il ferraio", "Baracca dei minatori", F_CASA_PAGLIA,
      { { POZ, ROC }, { ROC, ROC, FER, FER, FER, CAR }, { CAR, CAR, ALB, ALB, CES }, { FER, ROC }, {} },
      O_PIETRA, 6, O_PANE, 2, 40, O_LINGOTTO, 2, O_POZIONE, 2, 50, E_RAGNO, O_PELLE, 2, 40, F_FRANA, O_PICCONE, 1, "Frana nella galleria", E_REGINA, 60, O_CANNA, 1, 60,
      { "Le gallerie crollano! Portami 6 Pietre dalla Cava di Pietrarossa per rinforzarle.",
        "La mia fucina e' pronta. Fondi 2 Lingotti di ferro con ferro e carbone, cosi' forgero' attrezzi per tutti.",
        "Un ragno enorme e' uscito dalla Galleria Profonda. Ti serve una spada di ferro per batterlo.",
        "Una frana chiude la via per l'antro. Spacca i massi con il piccone.",
        "La Regina dei ragni e' laggiu', sopra il frammento. Vestiti di ferro: morde forte!",
        "Il terzo frammento! Portamelo, la miniera torna a vivere." },
      "Ecco una canna da pesca: al Porto di Salmastra ti tornera' utile.", "Querciaverde - Miniera", E_RAGNO, 0 },
    { "Il Porto di Salmastra",
      "Il porto di Salmastra e' in mano ai pirati del Capitano Salmastro, che ha nascosto il quarto frammento nella sua baia. Le barche non escono piu' a pescare.",
      { "Porto di Salmastra", "Scogli dei Gabbiani", "Saline del Sole", "Faro Spento", "Baia dei Pirati" }, { 7, 7, 7, 7, 7 }, { 0, 0, 1, 2, 3 },
      { "Marina la capitana di porto", "Oste Baldo", "Salvo il salinaro", "Lucia la guardiana del faro" }, { F_DAMA, F_CONTADINO, F_CONTADINO, F_GUARDIA },
      { "#2a5a8a", "#8a6a3a", "#b0b0a0", "#5a3a6a" }, "Kemal il mercante d'oltremare", "Taverna dell'Ancora", F_CASA_TEGOLE,
      { { POZ, PES }, { PES, PES, CON, CON, ERB, ERB }, { SAL, SAL, PES }, { ROC, PES }, {} },
      O_PESCE, 5, O_PANE, 2, 40, O_ZUPPA, 2, O_POZIONE, 2, 50, E_NOSTROMO, -1, 0, 40, F_CANCELLO, O_CHIAVE, 1, "Cancello del faro", E_CAPITANO, 80, O_POZIONE, 2, 70,
      { "Il porto ha fame! Pesca 5 Pesci dagli Scogli dei Gabbiani.",
        "Prepara 2 Zuppe di pesce: pesce, erbe e acqua. I marinai ti adoreranno!",
        "Il nostromo dei pirati sorveglia il faro e ha la chiave del cancello. Prendigliela!",
        "Con la chiave del nostromo apri il cancello del faro.",
        "Il Capitano Salmastro dorme sul suo tesoro, nella baia. Sveglialo... a modo tuo.",
        "Il quarto frammento! Portamelo, il mare ringrazia." },
      "Il mare e' di nuovo libero! La Palude delle Lucciole e' oltre le colline: buona fortuna.", "Querciaverde - Mare", E_PIRATA, 0 },
    { "La Palude delle Lucciole",
      "La palude e' avvolta da una nebbia stregata. Lo Spirito della palude, risvegliato dal quinto frammento, trascina i viandanti nel fango.",
      { "Villaggio sulle Palafitte", "Canneto", "Isola delle Erbe", "Torre della Strega", "Cuore della Palude" }, { 5, 5, 5, 8, 5 }, { 1, 0, 1, 4, 3 },
      { "Nonna Ortica", "Tano il cacciatore", "Viola la guaritrice", "Ombra il barcaiolo" }, { F_ANZIANO, F_GUARDIA, F_DAMA, F_MONACO },
      { "#4a6a4a", "#5a4a2a", "#7a4a8a", "#2a3a4a" }, "Gigia delle erbe", "Palafitta degli ospiti", F_CASA_PAGLIA,
      { { POZ, CAN }, { CAN, CAN, ERB, ERB, FUN }, { ERB, ERB, ERB, CES }, { ALB, FUN }, {} },
      O_ERBE, 6, O_ARCO, 1, 30, O_POZIONE, 3, -1, 0, 60, E_SERPENTE, O_PELLE, 2, 40, F_ROVI, O_TORCIA, 3, "Rovi della strega", E_SPIRITO, 90, O_POZIONE, 3, 80,
      { "Le mie pozioni tengono lontana la nebbia, ma ho finito le erbe. Portamene 6 e ti daro' l'arco di mio figlio.",
        "Prepara 3 Pozioni per i cacciatori feriti dalla nebbia.",
        "Un serpente gigante si e' annidato alla Torre della Strega.",
        "I rovi della strega chiudono il passaggio: bruciali con 3 Torce.",
        "Lo Spirito della palude vive nel cuore della nebbia. Solo un arco forte puo' colpirlo.",
        "Il quinto frammento! Portamelo, la nebbia sta gia' svanendo." },
      "La nebbia si dirada! Ora sali al Passo dei Venti, tra le montagne.", "Querciaverde - Palude", E_SERPENTE, 0 },
    { "Il Passo dei Venti",
      "Sul Passo dei Venti la neve non si scioglie piu'. Il Troll dei ghiacci ha murato il valico e custodisce il sesto frammento.",
      { "Rifugio del Passo", "Pineta Innevata", "Laghetto Ghiacciato", "Grotta di Ghiaccio", "Vetta dei Venti" }, { 6, 6, 6, 11, 6 }, { 0, 1, 0, 0, 2 },
      { "Oddone il rifugista", "Adele la pastora", "Fosco il pescatore", "Ardo la guida" }, { F_CONTADINO, F_DAMA, F_CONTADINO, F_GUARDIA },
      { "#8a2a2a", "#5a6a8a", "#4a5a6a", "#6a4a2a" }, "Remo il vivandiere", "Rifugio del Passo", F_CASA_TEGOLE,
      { { POZ, ABE }, { ABE, ABE, ABE, CES, ROC }, { PES, PES, ROC }, { CRI, CRI }, {} },
      O_LEGNA, 8, O_PANE, 2, 50, O_PESCE, 4, O_FORMAGGIO, 3, 60, E_ORSO, O_PELLE, 1, 50, F_GHIACCIO, O_TORCIA, 4, "Muro di ghiaccio", E_TROLLGHIACCI, 100, O_CRISTALLO, 2, 90,
      { "Il rifugio gela! Taglia 8 Legna nella Pineta Innevata.",
        "Noi pastori abbiamo fame: pesca 4 Pesci nel Laghetto Ghiacciato.",
        "Un orso delle nevi si e' preso la Grotta di Ghiaccio.",
        "Il troll ha murato il valico con il ghiaccio: scioglilo con 4 Torce.",
        "Il Troll dei ghiacci e' sulla vetta. Ha il frammento incastonato nella clava!",
        "Il sesto frammento! Portamelo, il vento si e' calmato." },
      "Il passo e' libero! Oltre le nuvole c'e' il Monastero: i monaci ti aspettano.", "Querciaverde - Montagna", E_ORSO, 0 },
    { "Il Monastero tra le Nuvole",
      "I monaci del Monastero tra le Nuvole custodivano il settimo frammento, ma i morti della cripta si sono risvegliati e il Guardiano di pietra non riconosce piu' nessuno.",
      { "Monastero delle Nuvole", "Orti dei Monaci", "Scalinata del Vento", "Cripta Antica", "Sala del Guardiano" }, { 9, 2, 6, 8, 8 }, { 1, 0, 1, 3, 4 },
      { "Abate Clemente", "Fra Tommaso l'apicoltore", "Suor Agnese", "Fra Silvio il custode" }, { F_MONACO, F_MONACO, F_DAMA, F_MONACO },
      { "#6a4a2a", "#8a6a3a", "#4a4a6a", "#3a3a3a" }, "Fra Mercurio", "Celle dei pellegrini", F_CASA_TEGOLE,
      { { POZ, ROC }, { ALV, ALV, ALV, GRA, MEL, ERB }, { ROC, ROC, ROC, ABE }, { ROC }, {} },
      O_MIELE, 4, O_POZIONE, 1, 60, O_MATTONI, 4, O_MANTELLO, 1, 40, E_SCHELETRO, -1, 0, 60, F_FRANA, O_PICCONE, 1, "Crollo nella cripta", E_GUARDIANO, 110, O_PALA, 1, 100,
      { "Senza miele non possiamo fare le candele. Raccogli 4 Miele dagli alveari degli orti.",
        "Il muro della cripta e' crepato: servono 4 Mattoni, fatti con pietra e acqua.",
        "Uno scheletro e' uscito dalla Cripta Antica e vaga tra le tombe!",
        "Un crollo chiude la scala della cripta: libera i massi con il piccone.",
        "Il Guardiano di pietra protegge il frammento da secoli, ma ora attacca chiunque.",
        "Il settimo frammento! Portamelo, lo custodiremo nelle preghiere." },
      "Il Guardiano riposa in pace. Prendi questa pala: nel Deserto di Rame la sabbia e' ovunque.", "Querciaverde - Castello", E_SCHELETRO, 0 },
    { "Il Deserto di Rame",
      "Oltre le montagne il Deserto di Rame brucia sotto il sole. Nel Tempio Sepolto un golem di rame vivo protegge l'ottavo frammento.",
      { "Oasi di Palmarossa", "Dune Dorate", "Canyon di Rame", "Tempio Sepolto", "Cuore del Tempio" }, { 10, 10, 10, 8, 8 }, { 0, 0, 2, 0, 3 },
      { "Zahir il carovaniere", "Amira la vetraia", "Kalim il cercatore", "Nadia l'esploratrice" }, { F_CONTADINO, F_DAMA, F_CONTADINO, F_GUARDIA },
      { "#c08030", "#3a8a8a", "#8a5a2a", "#a06a2a" }, "Faruk il mercante di spezie", "Tenda della carovana", F_TENDA,
      { { POZ }, { DUN, DUN, DUN }, { RAM, RAM, RAM, ROC, CAR }, { RAM }, {} },
      O_SABBIA, 6, -1, 0, 60, O_VETRO, 2, -1, 0, 80, E_SCORPIONE, -1, 0, 60, F_CANCELLO, O_CHIAVE, 1, "Porta del tempio", E_GOLEMRAME, 130, O_LINGOTTO, 2, 120,
      { "La carovana deve riparare le cisterne: portami 6 Sabbia dalle Dune Dorate (serve una pala).",
        "Fondi 2 Vetri con sabbia e carbone: ne faro' lanterne per il tempio.",
        "Uno scorpione gigante blocca l'ingresso del tempio. Ti servira' una lancia di rame.",
        "La porta del tempio e' chiusa. Nel canyon c'e' un forziere antico: forse la chiave e' li'.",
        "Il Golem di rame e' fatto dello stesso metallo delle armature: vestiti di rame anche tu!",
        "L'ottavo frammento! Portamelo, il sole scotta meno." },
      "Il deserto respira! Le barche dei pescatori ti porteranno alla Laguna Sommersa.", "Querciaverde - Deserto", E_SCORPIONE, 1 },
    { "La Laguna Sommersa",
      "Dove un tempo sorgeva una citta', ora c'e' la Laguna Sommersa. Lo Spirito annegato si aggira tra le rovine con il nono frammento.",
      { "Villaggio dei Pescatori", "Laguna Turchese", "Isolotto delle Conchiglie", "Rovine Sommerse", "Tempio delle Maree" }, { 7, 7, 7, 8, 8 }, { 0, 1, 0, 4, 3 },
      { "Nonno Corallo", "Perla la retaia", "Gino il tuffatore", "Sibilla l'indovina" }, { F_ANZIANO, F_DAMA, F_CONTADINO, F_MONACO },
      { "#2a6a8a", "#6a8aa0", "#3a7a7a", "#5a2a6a" }, "Tullio il barattiere", "Capanna sul pontile", F_CASA_PAGLIA,
      { { POZ, PES }, { PES, PES, CAN, CAN, CAN }, { CON, CON, CON, PES }, { ALB, ERB }, {} },
      O_CONCHIGLIE, 6, -1, 0, 80, O_CORDA, 4, O_POZIONE, 3, 50, E_SERPMARINO, -1, 0, 60, F_PONTE, O_TAVOLA, 4, "Passerella crollata", E_ANNEGATO, 140, O_LINGOTTO, 2, 130,
      { "Le conchiglie dell'isolotto tengono lontani gli spiriti. Portamene 6.",
        "Le reti sono strappate: intreccia 4 Corde con le canne della laguna.",
        "Un serpente marino si nasconde tra le Rovine Sommerse.",
        "La passerella per il tempio e' crollata: riparala con 4 Tavole.",
        "Lo Spirito annegato piange nel tempio. Liberalo e il frammento sara' tuo.",
        "Il nono frammento! Portamelo, la laguna ti ringrazia." },
      "La laguna brilla di nuovo. Ora la Valle dei Giganti: attento ai troll!", "Querciaverde - Mare", E_SERPMARINO, 0 },
    { "La Valle dei Giganti",
      "Nella Valle dei Giganti i pastori vivono nel terrore: il Gigante di pietra si e' risvegliato con il decimo frammento nel petto.",
      { "Accampamento dei Pastori", "Prati Alti", "Miniera dei Giganti", "Gola dei Troll", "Trono dei Giganti" }, { 2, 2, 4, 6, 8 }, { 0, 1, 0, 2, 2 },
      { "Mastro Ilario", "Rina la casara", "Duccio il minatore", "Gaia la scalatrice" }, { F_ANZIANO, F_DAMA, F_CONTADINO, F_GUARDIA },
      { "#7a6a4a", "#a08a4a", "#5a5a5a", "#2a6a4a" }, "Lapo il mercante di valle", "Tenda dei pastori", F_TENDA,
      { { POZ, MEL }, { GRA, GRA, GRA, MEL, CES }, { FER, FER, CAR, CAR, CRI, CRI, CRI }, { ROC, ABE }, {} },
      O_GRANO, 6, -1, 0, 80, O_LINGOTTO, 3, -1, 0, 100, E_TROLL, O_CRISTALLO, 2, 80, F_FRANA, O_PICCONE, 1, "Frana nella gola", E_GIGANTE, 160, -1, 0, 150,
      { "Il grano dei Prati Alti e' maturo, ma nessuno osa falciarlo. Portamene 6.",
        "Ci servono 3 Lingotti di ferro per i recinti. La Miniera dei Giganti e' piena di ferro e carbone.",
        "Un troll di montagna blocca la gola. Una spada di cristallo lo fara' scappare!",
        "Una frana chiude la gola: spacca i massi con il piccone.",
        "Il Gigante di pietra siede sul suo trono. Solo un'armatura di cristallo resiste ai suoi pugni.",
        "Il decimo frammento! Portamelo, la valle e' salva." },
      "Grazie! Resta l'Isola del Faro: da li' si vede il castello.", "Querciaverde - Montagna", -1, 0 },
    { "L'Isola del Faro",
      "Sull'Isola del Faro un drago ha spento la grande luce, e gli apprendisti di Morgrano preparano l'ultimo assalto al castello.",
      { "Approdo dell'Isola", "Scogliera", "Pineta Marina", "Faro Antico", "Cima del Faro" }, { 7, 7, 1, 7, 8 }, { 0, 0, 1, 2, 3 },
      { "Capitano Ettore", "Mirta la vedetta", "Fra Lino il guardiano", "Ondina la marinaia" }, { F_GUARDIA, F_DAMA, F_MONACO, F_DAMA },
      { "#2a3a6a", "#6a2a2a", "#5a4a3a", "#3a9a9a" }, "Bianca la mercantessa", "Casa del guardiano", F_CASA_TEGOLE,
      { { POZ, PES }, { PES, CON, CON, ROC, ERB }, { ABE, ABE, ALB, ALB, CAN, CAN, FUN }, { ROC }, {} },
      O_LEGNA, 8, -1, 0, 100, O_TORCIA, 4, -1, 0, 100, E_APPRENDISTA, O_POZIONE, 2, 80, F_CANCELLO, O_CHIAVE, 1, "Porta del faro", E_DRAGO, 200, O_LAMA, 1, 150,
      { "Dobbiamo costruire un pontile per le navi del re: portami 8 Legna dalla pineta.",
        "Il faro e' spento. Porta 4 Torce: la sua luce guidera' le navi.",
        "Un apprendista di Morgrano si e' chiuso nel Faro Antico.",
        "La porta del faro e' sbarrata: cerca la chiave nel forziere della pineta.",
        "Il drago del faro dorme sulla cima. Ha il frammento tra gli artigli!",
        "L'undicesimo frammento! Portamelo, presto." },
      "Questa e' la Lama di Querciaverde, la spada dei re. Ora va' al castello: Morgrano ti aspetta.", "Querciaverde - Mare", -1, 1 },
    { "Il Castello di Querciaverde",
      "Il castello di Querciaverde e' avvolto dalle ombre. Morgrano lo stregone siede sul trono con l'ultimo frammento: riunisci la Ghianda d'Oro e salva il regno!",
      { "Accampamento del Re", "Mura del Castello", "Giardini Reali", "Cortile del Castello", "Sala del Trono" }, { 2, 9, 2, 9, 9 }, { 0, 2, 1, 2, 3 },
      { "Re Aldobrando", "Dama Ginevra", "Fiorindo il giardiniere", "Sir Guido" }, { F_RE, F_DAMA, F_CONTADINO, F_GUARDIA },
      { "#7a1a2a", "#8a2a6a", "#4a7a3a", "#4a4a6a" }, "L'armaiolo del re", "Tenda reale", F_TENDA,
      { { POZ, ROC }, { ROC, ROC, ROC, ALB }, { ERB, ERB, ERB, MEL, ALV, FUN }, {}, {} },
      O_MATTONI, 6, -1, 0, 120, O_POZIONE, 4, -1, 0, 100, E_CAVALIERE, -1, 0, 100, F_CANCELLO, O_CHIAVE, 1, "Porta della sala del trono", E_MORGRANO, 500, -1, 0, 300,
      { "Le mura sono a pezzi. Portami 6 Mattoni per ripararle prima dell'assalto.",
        "I soldati sono feriti: prepara 4 Pozioni con le erbe dei giardini.",
        "Il Cavaliere oscuro sorveglia il cortile e ha la chiave della sala del trono.",
        "La porta della sala del trono e' chiusa: aprila con la chiave del cavaliere.",
        "Morgrano e' sul trono. Questa e' la battaglia finale: che la Ghianda ti protegga!",
        "L'ultimo frammento! Portamelo, eroe: la Ghianda d'Oro sara' di nuovo intera." },
      "La Ghianda d'Oro e' di nuovo intera! Querciaverde rifiorisce e il regno e' salvo.", "Querciaverde - Castello", -1, 0 },
    };
    return c;
}
#undef ALB
#undef ABE
#undef CES
#undef FUN
#undef GRA
#undef MEL
#undef POZ
#undef CAN
#undef ERB
#undef ROC
#undef FER
#undef CAR
#undef CRI
#undef PES
#undef CON
#undef ALV
#undef RAM
#undef DUN
#undef SAL


std::string NomeRisorsa(int forma, const char* oggetto) {
    const char* nm[] = { "Faggio", "Abete", "Melo", "Cespuglio di bacche", "Funghi", "Erbe selvatiche", "Grano maturo", "Canneto", "Roccia", "Filone di ferro",
                         "Filone di carbone", "Cristalli", "Pozzo", "Banco di pesci", "Conchiglie", "Alveare", "", "", "Filone di rame", "Duna", "Salina" };
    int fi = forma - F_ALBERO;
    return fi >= 0 && fi < 21 && *nm[fi] ? nm[fi] : oggetto;
}

// I dati predefiniti del gioco (12 capitoli), costruiti dalle tabelle qui sopra.
json DatiPredefiniti() {
    json j;
    j["generale"] = { { "eroe", "Elio" }, { "vita", 10 }, { "energia", 10 }, { "oro", 30 }, { "zaino", 20 }, { "forza", 1 },
                      { "veste", "#3a6a3a" }, { "mantello", "#7a4a22" }, { "capelli", "#5a3a1a" },
                      { "inizio1", O_BASTONE }, { "n1", 1 }, { "inizio2", O_ASCIA }, { "n2", 1 }, { "inizio3", O_SECCHIO }, { "n3", 1 }, { "inizio4", O_PANE }, { "n4", 2 },
                      { "riposo", 3 }, { "musicaMenu", "Querciaverde - Tema" }, { "musicaBattaglia", "Querciaverde - Battaglia" },
                      { "fine", "LA GHIANDA D'ORO E' DI NUOVO INTERA!" } };
    for (auto& o : DEF_OGGETTI) {
        int neg = o.negozio;
        if (&o == &DEF_OGGETTI[O_CANNE]) neg = 2;
        j["oggetti"].push_back({ { "nome", o.nome }, { "tipo", o.tipo }, { "icona", o.icona }, { "colore", o.colore }, { "valore", o.valore }, { "prezzo", o.prezzo }, { "negozio", neg } });
    }
    for (auto& r : DEF_RICETTE)
        j["ricette"].push_back({ { "risultato", r.ris }, { "quantita", r.q }, { "ing1", r.i1 }, { "n1", r.n1 }, { "ing2", r.i2 }, { "n2", r.n2 }, { "ing3", r.i3 }, { "n3", r.n3 }, { "capitolo", r.cap } });
    for (auto& n : DEF_NEMICI)
        j["nemici"].push_back({ { "nome", n.nome }, { "forma", n.forma }, { "colore", n.colore }, { "potenza", n.potenza }, { "oro", n.oro }, { "lascia", n.lascia },
                                { "quantita", n.quantita }, { "grande", n.grande } });
    const auto& caps = Capitoli();
    int NC = (int)caps.size();
    json punti = json::array(), missioni = json::array();
    auto Punto = [&](int luogo, int tipo, int forma, const std::string& nome, float x, float y) -> json& {
        punti.push_back({ { "luogo", luogo }, { "tipo", tipo }, { "forma", forma }, { "nome", nome }, { "x", (int)x }, { "y", (int)y }, { "oggetto", -1 }, { "quantita", 0 },
                          { "attrezzo", -1 }, { "costo", 0 }, { "usi", 0 }, { "ricrescita", 0 }, { "nemico", -1 }, { "blocca", 0 }, { "colore", "#6a5a4a" }, { "frase", "" } });
        return punti.back();
    };
    const float SLOT[][2] = { { 380, 600 }, { 640, 500 }, { 1320, 470 }, { 1580, 640 }, { 520, 810 }, { 830, 730 }, { 1170, 770 }, { 1430, 840 }, { 1720, 800 },
                              { 270, 820 }, { 1060, 580 }, { 1660, 430 }, { 330, 430 }, { 900, 440 } };
    const int NSLOT = sizeof SLOT / sizeof SLOT[0];
    const char* grazie[5] = { "Grazie di cuore! Ecco la tua ricompensa.", "Sei stato bravissimo. Prendi, te lo sei meritato.", "Lo sapevo che potevo contare su di te!",
                              "Perfetto! Il regno ha bisogno di eroi come te.", "Grazie! Questo e' per te." };
    for (int k = 0; k < NC; k++) {
        const DefCapitolo& d = caps[k];
        j["capitoli"].push_back({ { "nome", d.nome }, { "intro", d.intro }, { "partenza", k * 5 }, { "musica", d.musica } });
        for (int i = 0; i < 5; i++) {
            int u[4] = { -1, -1, -1, -1 };
            if (i == 0) { u[0] = k * 5 + 2; u[1] = k * 5 + 1; u[2] = k > 0 ? (k - 1) * 5 : -1; u[3] = k < NC - 1 ? (k + 1) * 5 : -1; }
            if (i == 1) { u[3] = k * 5; u[0] = k * 5 + 3; }
            if (i == 2) { u[2] = k * 5; u[1] = k * 5 + 3; }
            if (i == 3) { u[3] = k * 5 + 2; u[2] = k * 5 + 1; u[0] = k * 5 + 4; }
            if (i == 4) { u[2] = k * 5 + 3; }
            int sc = d.scene[i];
            const char* erba = sc == 6 ? "#e8eef4" : sc == 10 ? "#d8b878" : sc == 7 ? "#d8c89a" : sc == 5 ? "#4a5a2a" : sc == 11 ? "#3a3438" : sc == 4 ? "#8a7a62" : "#5a6a2e";
            const char* terra = sc == 9 || sc == 8 ? "#8a8478" : sc == 10 ? "#c89858" : sc == 7 ? "#c8b080" : sc == 11 ? "#4a4248" : sc == 6 ? "#b8c0c8" : "#7a6a52";
            j["luoghi"].push_back({ { "nome", d.luoghi[i] }, { "capitolo", k + 1 }, { "scena", sc }, { "ora", d.ore[i] }, { "erba", erba }, { "terreno", terra },
                                    { "nord", u[0] }, { "est", u[1] }, { "sud", u[2] }, { "ovest", u[3] }, { "musica", d.musica }, { "seme", k * 17 + i * 5 + 3 } });
        }
        // --- elementi delle cinque scene ---
        int L0 = k * 5, puntoA = -1, puntoB = -1, puntoC = -1, puntoD = -1, nemicoQ3 = -1, ostacolo = -1, boss = -1;
        {   // il villaggio
            int sc = d.scene[0];
            int casa1 = sc == 10 || sc == 2 ? F_TENDA : sc == 9 ? F_TORRE : sc == 6 || sc == 4 ? F_CASA_TEGOLE : F_CASA_PAGLIA;
            int casa2 = sc == 10 ? F_TENDA : sc == 2 && k == 11 ? F_TENDA : F_CASA_TEGOLE;
            Punto(L0, 0, casa1, "Casa", 560, 470)["colore"] = "#c8b48c";
            Punto(L0, 0, casa2, "Casa", 1160, 400)["colore"] = "#c8b48c";
            json& inn = Punto(L0, 7, d.formaRiposo, d.riposo, 1580, 590);
            inn["costo"] = 3 + k; inn["frase"] = "Un letto caldo e un pasto: tornano vita ed energia."; inn["colore"] = "#c8b48c";
            json& m = Punto(L0, 4, F_MERCANTE, d.mercante, 640, 810);
            m["colore"] = k % 2 ? "#8a2a2a" : "#2a5a8a"; m["frase"] = "Compro e vendo di tutto, a prezzi onesti!";
            puntoA = (int)punti.size(); Punto(L0, 2, d.formeNpc[0], d.npc[0], 860, 620)["colore"] = d.coloriNpc[0];
            puntoB = (int)punti.size(); Punto(L0, 2, d.formeNpc[1], d.npc[1], 1250, 660)["colore"] = d.coloriNpc[1];
            punti[puntoA]["frase"] = "Che la Ghianda ti protegga, viandante."; punti[puntoB]["frase"] = "Buona giornata! Hai visto che bel tempo?";
            const float pos0[][2] = { { 1000, 790 }, { 1440, 840 }, { 300, 700 } };
            for (size_t r = 0; r < d.ris[0].size() && r < 3; r++) {
                const DefRisorsa& rs = d.ris[0][r];
                json& p = Punto(L0, 1, rs.forma, DEF_OGGETTI[rs.ogg].nome, pos0[r][0], pos0[r][1]);
                p["oggetto"] = rs.ogg; p["quantita"] = rs.q; p["attrezzo"] = rs.attrezzo; p["costo"] = rs.costo; p["usi"] = rs.usi; p["ricrescita"] = rs.ric;
                p["nome"] = NomeRisorsa(rs.forma, DEF_OGGETTI[rs.ogg].nome);
            }
            Punto(L0, 0, F_BARILI, "Barili", 410, 560); Punto(L0, 0, F_CASSE, "Casse", 900, 500);
            Punto(L0, 0, sc == 7 ? F_BARCA : F_CARRETTO, sc == 7 ? "Barca" : "Carretto", 1420, 730); Punto(L0, 0, F_FIORI, "Fiori", 1660, 880);
            if (k == 0) Punto(L0, 0, F_QUERCIA, "La grande quercia", 230, 330);
            if (k == 11) Punto(L0, 0, F_STATUA, "Statua del re", 260, 380);
        }
        for (int i = 1; i < 5; i++) {
            int L = L0 + i, sc = d.scene[i];
            std::vector<int> ord;
            for (int s = 0; s < NSLOT; s++) ord.push_back(s);
            unsigned h = (unsigned)(k * 131 + i * 17 + 7);
            for (int s = NSLOT - 1; s > 0; s--) { h = h * 1103515245u + 12345u; std::swap(ord[s], ord[(h >> 8) % (s + 1)]); }
            std::vector<Vector2> occ;
            auto Occupa = [&](float x, float y) { occ.push_back({ x, y }); };
            auto Libero = [&](float& x, float& y) {
                for (int s : ord) {
                    bool ok = true;
                    for (auto& o : occ) if (std::fabs(o.x - SLOT[s][0]) < 190 && std::fabs(o.y - SLOT[s][1]) < 130) ok = false;
                    if (ok) { x = SLOT[s][0]; y = SLOT[s][1]; Occupa(x, y); return true; }
                }
                return false;
            };
            if (i == 2) { puntoC = (int)punti.size(); Punto(L, 2, d.formeNpc[2], d.npc[2], 920, 640)["colore"] = d.coloriNpc[2]; punti[puntoC]["frase"] = "Che fatica, di questi tempi..."; Occupa(920, 640); }
            if (i == 3) {
                ostacolo = (int)punti.size();
                json& o = Punto(L, 5, d.formaOstacolo, d.nomeOstacolo, 960, 260);
                o["attrezzo"] = d.ostacolo; o["costo"] = d.nOstacolo; o["blocca"] = 1; o["frase"] = "Da qui non si passa.";
                Occupa(960, 260); Occupa(960, 420);
                nemicoQ3 = (int)punti.size();
                json& e = Punto(L, 3, 0, DEF_NEMICI[d.nemico].nome, 1250, 540); e["nemico"] = d.nemico; Occupa(1250, 540);
            }
            if (i == 4) {
                boss = (int)punti.size();
                json& e = Punto(L, 3, 0, DEF_NEMICI[d.boss].nome, 1080, 470); e["nemico"] = d.boss; Occupa(1080, 470); Occupa(1080, 330);
                puntoD = (int)punti.size(); Punto(L, 2, d.formeNpc[3], d.npc[3], 640, 740)["colore"] = d.coloriNpc[3]; Occupa(640, 740);
                punti[puntoD]["frase"] = "Sta' attento, qui e' pericoloso.";
            }
            if (i == 1 && d.extraNemico >= 0) {
                float x, y; if (Libero(x, y)) { json& e = Punto(L, 3, 0, DEF_NEMICI[d.extraNemico].nome, x, y); e["nemico"] = d.extraNemico; e["ricrescita"] = 70; }
            }
            if ((d.forziere == 0 && i == 1) || (d.forziere == 1 && i == 2)) {
                float x, y;
                if (Libero(x, y)) {
                    json& f = Punto(L, 6, F_FORZIERE, d.forziere ? "Forziere antico" : "Forziere dimenticato", x, y);
                    if (d.forziere) { f["oggetto"] = O_CHIAVE; f["quantita"] = 1; f["costo"] = 10 + 5 * k; }
                    else { f["oggetto"] = O_POZIONE; f["quantita"] = 1; f["costo"] = 15 + 10 * k; }
                }
            }
            for (auto& rs : d.ris[i]) {
                float x, y;
                if (!Libero(x, y)) break;
                json& p = Punto(L, 1, rs.forma, DEF_OGGETTI[rs.ogg].nome, x, y);
                p["oggetto"] = rs.ogg; p["quantita"] = rs.q; p["attrezzo"] = rs.attrezzo; p["costo"] = rs.costo; p["usi"] = rs.usi; p["ricrescita"] = rs.ric;
                p["nome"] = NomeRisorsa(rs.forma, DEF_OGGETTI[rs.ogg].nome);
            }
            // decorazioni tipiche della scena
            int dec[2] = { -1, -1 };
            if (sc == 2) { dec[0] = k == 0 && i == 2 ? F_MULINO : F_RECINTO; dec[1] = F_FIORI; }
            if (sc == 4) { dec[0] = F_CARRETTO; dec[1] = F_BARILI; }
            if (sc == 7) { dec[0] = F_BARCA; dec[1] = (k == 3 || k == 10) && i == 3 ? F_FARO : F_CASSE; }
            if (sc == 8) { dec[0] = F_COLONNE; dec[1] = F_STATUA; }
            if (sc == 9) { dec[0] = F_TORRE; dec[1] = F_STATUA; }
            if (sc == 10) { dec[0] = F_PALMA; dec[1] = F_CACTUS; }
            if (sc == 3) { dec[0] = F_PONTILE; }
            if (sc == 1 || sc == 5) { dec[0] = F_FIORI; }
            for (int q = 0; q < 2; q++)
                if (dec[q] >= 0) {
                    float x, y;
                    if (dec[q] == F_MULINO) { x = 1500; y = 430; Occupa(x, y); }
                    else if (dec[q] == F_FARO) { x = 1560; y = 420; Occupa(x, y); }
                    else if (!Libero(x, y)) continue;
                    Punto(L, 0, dec[q], FORME[dec[q]], x, y);
                }
        }
        // --- le sei missioni del capitolo ---
        int m0 = k * 6;
        auto Missione = [&](int chi, int tipo, int ogg, int q, int bers, int rich, int oro, int premio, int premioQ, const char* testo, const std::string& fine, const std::string& nome) {
            missioni.push_back({ { "capitolo", k + 1 }, { "nome", nome }, { "testo", testo }, { "fine", fine }, { "chi", chi }, { "tipo", tipo }, { "oggetto", ogg },
                                 { "quantita", q }, { "bersaglio", bers }, { "richiede", rich }, { "oro", oro }, { "premio", premio }, { "premioQuantita", premioQ } });
        };
        std::string n1 = std::string(DEF_OGGETTI[d.r1].nome), n2 = std::string(DEF_OGGETTI[d.r2].nome);
        Missione(puntoA, 0, d.r1, d.n1, -1, -1, d.oro1, d.premio1, d.premioN1, d.testi[0], grazie[k % 5], "Porta " + n1);
        Missione(puntoB, 0, d.r2, d.n2, -1, m0, d.oro2, d.premio2, d.premioN2, d.testi[1], grazie[(k + 1) % 5], "Prepara " + n2);
        Missione(puntoC, 1, -1, 0, nemicoQ3, m0, d.oro3, d.premio3, d.premioN3, d.testi[2], grazie[(k + 2) % 5], std::string("Sconfiggi: ") + DEF_NEMICI[d.nemico].nome);
        Missione(puntoC, 2, -1, 0, ostacolo, m0 + 2, 20 + 5 * k, -1, 0, d.testi[3], "La strada e' libera! Ora puoi proseguire verso nord.", std::string("Libera: ") + d.nomeOstacolo);
        Missione(puntoD, 1, -1, 0, boss, m0 + 3, d.oro5, -1, 0, d.testi[4], "Incredibile! Ora porta il frammento a " + std::string(d.npc[0]) + ".", std::string("Sconfiggi: ") + DEF_NEMICI[d.boss].nome);
        Missione(puntoA, 0, O_FRAMMENTO, 1, -1, m0 + 4, d.oro6, d.premio6, d.premioN6, d.testi[5], d.fine6, "Il frammento di " + std::string(d.luoghi[0]));
    }
    j["punti"] = punti;
    j["missioni"] = missioni;
    json su = json::object();
    const char* suoni[] = { "qv_legna", "qv_pietra", "qv_raccogli", "qv_acqua", "qv_moneta", "qv_zaino", "qv_missione", "qv_completa", "qv_spada", "qv_colpito",
                            "qv_vittoria", "qv_sconfitta", "qv_mangia", "qv_lavoro", "qv_forziere", "qv_clic", "qv_viaggio", "qv_capitolo", "qv_no" };
    for (auto s : suoni) su[s] = "";
    j["suoni"] = su;
    return j;
}

json SchemaQuerciaverde() {
    json voci = json::array();
    auto Cat = [&](const char* chiave, const char* nome, bool lista, const char* aiuto, json campi, int min = 0, int max = 0, const char* elemento = "") {
        json c = { { "chiave", chiave }, { "nome", nome }, { "lista", lista }, { "aiuto", aiuto }, { "campi", campi } };
        if (lista) { c["min"] = min; c["max"] = max; c["elemento"] = elemento; }
        voci.push_back(c);
    };
    auto N = [](const char* k, const char* n, double mi, double ma, double passo, const char* f = "%.0f") { return json{ { "chiave", k }, { "nome", n }, { "tipo", "numero" }, { "min", mi }, { "max", ma }, { "passo", passo }, { "formato", f } }; };
    auto C = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "colore" } }; };
    auto T = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "testo" } }; };
    auto S = [](const char* k, const char* n, const char* const* v, int nv) { json a = json::array(); for (int i = 0; i < nv; i++) a.push_back(v[i]); return json{ { "chiave", k }, { "nome", n }, { "tipo", "scelta" }, { "voci", a } }; };
    auto Rf = [](const char* k, const char* n, const char* da, const char* vuoto = "") { json f = { { "chiave", k }, { "nome", n }, { "tipo", "rif" }, { "da", da } }; if (*vuoto) f["vuoto"] = vuoto; return f; };
    auto B = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "brano" } }; };
    auto V = [](const char* k, const char* n) { return json{ { "chiave", k }, { "nome", n }, { "tipo", "vero" } }; };
    Cat("capitoli", "Capitoli", true, "I capitoli della storia: nome, testo d'inizio, la localita' di partenza e la musica. Un capitolo finisce quando tutte le sue missioni sono compiute; "
        "dopo l'ultimo il gioco e' vinto.",
        { T("nome", "Nome"), T("intro", "Testo d'inizio"), Rf("partenza", "Localita' di partenza", "luoghi"), B("musica", "Musica") }, 1, 40, "Capitolo");
    Cat("luoghi", "Localita'", true, "Le scene dipinte: capitolo, tipo di paesaggio, ora del giorno, colori, le quattro uscite (frecce dorate ai bordi) e la musica. "
        "Le uscite verso capitoli non ancora iniziati restano nascoste.",
        { T("nome", "Nome"), N("capitolo", "Capitolo", 1, 40, 1), S("scena", "Paesaggio", SCENE, 12), S("ora", "Ora del giorno", ORE, 5), C("erba", "Colore dell'erba"),
          C("terreno", "Colore del terreno"), Rf("nord", "Uscita a nord", "luoghi", "(nessuna)"), Rf("est", "Uscita a est", "luoghi", "(nessuna)"),
          Rf("sud", "Uscita a sud", "luoghi", "(nessuna)"), Rf("ovest", "Uscita a ovest", "luoghi", "(nessuna)"), B("musica", "Musica"), N("seme", "Variante del disegno", 0, 999, 1) },
        1, 400, "Localita'");
    Cat("punti", "Elementi delle scene", true, "Tutto quello che sta in una localita': persone, risorse da raccogliere con un clic, nemici, mercanti, ostacoli che bloccano un'uscita, "
        "forzieri, posti per riposare e decorazioni (case, alberi...). Per le risorse: oggetto e quantita' che danno, l'attrezzo che serve, l'energia (costo), quante volte "
        "si raccolgono prima di ricrescere (usi, 0 = sempre) e i secondi per ricrescere. Per gli ostacoli: l'oggetto che serve (attrezzo) e quanti (costo). Per i forzieri: "
        "oggetto e quantita' dentro, oro (costo) e la chiave (attrezzo). Per il riposo: il prezzo (costo).",
        { T("nome", "Nome"), Rf("luogo", "Localita'", "luoghi"), S("tipo", "Tipo", TIPI_PUNTO, 8), S("forma", "Aspetto", FORME, N_FORME), N("x", "Posizione X", 60, 1860, 5),
          N("y", "Posizione Y (i piedi)", 140, 960, 5), Rf("oggetto", "Oggetto", "oggetti", "(nessuno)"), N("quantita", "Quantita'", 0, 99, 1),
          Rf("attrezzo", "Attrezzo o oggetto richiesto", "oggetti", "(nessuno)"), N("costo", "Costo (energia, quantita', oro)", 0, 999, 1), N("usi", "Usi prima di ricrescere", 0, 99, 1),
          N("ricrescita", "Ricrescita (secondi, 0 = mai)", 0, 999, 5), Rf("nemico", "Nemico", "nemici", "(nessuno)"),
          json{ { "chiave", "blocca" }, { "nome", "Blocca l'uscita" }, { "tipo", "scelta" }, { "voci", { "Nessuna", "Nord", "Est", "Sud", "Ovest" } } },
          C("colore", "Colore (vestito, muri)"), T("frase", "Frase") }, 1, 3000, "Elemento");
    Cat("missioni", "Missioni", true, "Le missioni '!': chi la affida, che cosa chiede (portare oggetti, sconfiggere un nemico, liberare un ostacolo, parlare con qualcuno), "
        "la missione che deve essere gia' compiuta e il premio.",
        { T("nome", "Nome"), N("capitolo", "Capitolo", 1, 40, 1), T("testo", "Testo"), T("fine", "Frase alla consegna"), Rf("chi", "Chi la affida", "punti"),
          S("tipo", "Tipo", TIPI_MISSIONE, 4), Rf("oggetto", "Oggetto da portare", "oggetti", "(nessuno)"), N("quantita", "Quanti", 0, 99, 1),
          Rf("bersaglio", "Nemico, ostacolo o persona", "punti", "(nessuno)"), Rf("richiede", "Dopo la missione", "missioni", "(nessuna)"), N("oro", "Premio in oro", 0, 9999, 5),
          Rf("premio", "Premio (oggetto)", "oggetti", "(nessuno)"), N("premioQuantita", "Quanti", 0, 99, 1) }, 1, 400, "Missione");
    Cat("oggetti", "Oggetti", true, "Tutto quello che va nello zaino: tipo (risorsa, cibo che ridà energia, cura che ridà vita, attrezzo, arma, armatura, zaino...), icona, "
        "colore, valore (energia, vita, forza, difesa o posti in piu'), prezzo al mercato e il capitolo da cui i mercanti lo vendono (0 = mai).",
        { T("nome", "Nome"), S("tipo", "Tipo", TIPI_OGGETTO, 8), S("icona", "Icona", ICONE, N_ICONE), C("colore", "Colore"), N("valore", "Valore", 0, 99, 1),
          N("prezzo", "Prezzo", 0, 9999, 1), N("negozio", "In vendita dal capitolo", 0, 40, 1) }, 1, 200, "Oggetto");
    Cat("ricette", "Ricette", true, "I lavori artigianali (pulsante del martello): da uno a tre ingredienti si ottiene un oggetto, dal capitolo indicato in poi.",
        { Rf("risultato", "Risultato", "oggetti"), N("quantita", "Quantita'", 1, 20, 1), Rf("ing1", "Ingrediente 1", "oggetti"), N("n1", "Quanti", 1, 20, 1),
          Rf("ing2", "Ingrediente 2", "oggetti", "(nessuno)"), N("n2", "Quanti", 0, 20, 1), Rf("ing3", "Ingrediente 3", "oggetti", "(nessuno)"), N("n3", "Quanti", 0, 20, 1),
          N("capitolo", "Dal capitolo", 1, 40, 1) }, 0, 200, "Ricetta");
    Cat("nemici", "Nemici", true, "Le creature da sconfiggere con un clic: vinci se la tua forza (forza + arma + armatura) e' almeno la loro potenza; "
        "perdi vita in base alla differenza. Lasciano oro e a volte un oggetto.",
        { T("nome", "Nome"), S("forma", "Aspetto", FORME_NEMICO, N_FORME_NEMICO), C("colore", "Colore"), N("potenza", "Potenza", 1, 99, 1), N("oro", "Oro", 0, 9999, 5),
          Rf("lascia", "Lascia", "oggetti", "(niente)"), N("quantita", "Quanti", 0, 20, 1), V("grande", "Grande (boss)") }, 1, 200, "Nemico");
    Cat("generale", "Eroe e regole", false, "L'eroe all'inizio: nome, vita, energia, oro, posti nello zaino, forza di base, colori e gli oggetti di partenza.",
        { T("eroe", "Nome dell'eroe"), N("vita", "Vita", 3, 50, 1), N("energia", "Energia", 3, 50, 1), N("oro", "Oro all'inizio", 0, 9999, 5), N("zaino", "Posti nello zaino", 6, 60, 1),
          N("forza", "Forza di base", 0, 20, 1), C("veste", "Colore della veste"), C("mantello", "Colore del mantello"), C("capelli", "Colore dei capelli"),
          Rf("inizio1", "Oggetto iniziale 1", "oggetti", "(nessuno)"), N("n1", "Quanti", 0, 99, 1), Rf("inizio2", "Oggetto iniziale 2", "oggetti", "(nessuno)"), N("n2", "Quanti", 0, 99, 1),
          Rf("inizio3", "Oggetto iniziale 3", "oggetti", "(nessuno)"), N("n3", "Quanti", 0, 99, 1), Rf("inizio4", "Oggetto iniziale 4", "oggetti", "(nessuno)"), N("n4", "Quanti", 0, 99, 1),
          N("riposo", "Energia che torna da sola (punti al minuto)", 0, 20, 1), B("musicaMenu", "Musica del menu"), B("musicaBattaglia", "Musica della battaglia"), T("fine", "Scritta della vittoria") });
    const char* nomiSuoni[][2] = { { "qv_legna", "Taglio della legna" }, { "qv_pietra", "Piccone sulla roccia" }, { "qv_raccogli", "Raccolta" }, { "qv_acqua", "Acqua" },
                                   { "qv_moneta", "Monete" }, { "qv_zaino", "Oggetto nello zaino" }, { "qv_missione", "Nuova missione" }, { "qv_completa", "Missione compiuta" },
                                   { "qv_spada", "Colpo di spada" }, { "qv_colpito", "Eroe ferito" }, { "qv_vittoria", "Nemico sconfitto" }, { "qv_sconfitta", "Eroe sconfitto" },
                                   { "qv_mangia", "Mangiare" }, { "qv_lavoro", "Lavoro artigiano" }, { "qv_forziere", "Forziere aperto" }, { "qv_clic", "Clic" },
                                   { "qv_viaggio", "Viaggio" }, { "qv_capitolo", "Nuovo capitolo" }, { "qv_no", "Non si puo'" } };
    json su = json::array();
    for (auto& n : nomiSuoni) su.push_back({ { "chiave", n[0] }, { "nome", n[1] }, { "tipo", "effetto" } });
    Cat("suoni", "Effetti sonori", false, "Ogni suono del gioco: lascia 'predefinito' o scegli un effetto creato nella sezione Audio.", su);
    return voci;
}

// ============================================================================================
// PITTURA: aiuti per dipingere a pennellate con il Pennello (K)
// ============================================================================================
struct R2 {
    uint32_t s;
    explicit R2(uint32_t v) : s(v * 2654435761u + 0x9e37u) { if (!s) s = 1; }
    float F() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) / 16777216.f; }
    float F(float a, float b) { return a + (b - a) * F(); }
};
std::string Hsl(float h, float s, float l, float a = 1) {
    h = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f) / 360.f; s = Clamp(s, 0, 100) / 100.f; l = Clamp(l, 0, 100) / 100.f;
    auto f = [&](float n) { float k = std::fmod(n + h * 12, 12.f); float a2 = s * std::min(l, 1 - l); return l - a2 * std::max(-1.f, std::min({ k - 3, 9 - k, 1.f })); };
    char b[64];
    std::snprintf(b, sizeof b, "rgba(%d,%d,%d,%.3f)", (int)(f(0) * 255), (int)(f(8) * 255), (int)(f(4) * 255), Clamp(a, 0, 1));
    return b;
}
inline Vector2 Pv(Vector2 a, Vector2 b, float k = 1) { return { a.x + b.x * k, a.y + b.y * k }; }
void Via(K& c, const std::vector<Vector2>& p) {
    c.beginPath();
    for (size_t i = 0; i < p.size(); i++) { if (i) c.lineTo(p[i].x, p[i].y); else c.moveTo(p[i].x, p[i].y); }
    c.closePath();
}
void Pieno(K& c, const std::vector<Vector2>& p, const std::string& col) { Via(c, p); c.fillStyle(V(col)); c.fill(); }
void Dab(K& c, float x, float y, float r, const std::string& col, float rot, float sq = 0.62f) {
    c.fillStyle(V(col)); c.beginPath(); c.ellipse(x, y, r, r * sq, rot, 0, TAU); c.fill();
}
void Ombra(K& c, float x, float y, float rx, float ry, float a = 0.35f) {
    c.fillStyle(V(rgba("#141905", a))); c.beginPath(); c.ellipse(x, y, rx, ry, 0, 0, TAU); c.fill();
}
void Linea(K& c, float x0, float y0, float x1, float y1, const std::string& col, float w) {
    c.strokeStyle(V(col)); c.lineWidth(w); c.beginPath(); c.moveTo(x0, y0); c.lineTo(x1, y1); c.stroke();
}

// --- Figure umane in piccolo (eroe, abitanti, briganti...) -----------------------------------
struct Look {
    std::string veste = "#3a6a3a", calzoni = "#4a3a2a", pelle = "#e0b088", capelli = "#5a3a1a", cappello, mantello, stivali = "#3a2616";
    int copricapo = 0;      // 0 capelli, 1 cappuccio, 2 cappello a tesa, 3 elmo, 4 corona, 5 crocchia, 6 cappello a punta, 7 benda
    bool barba = false, gonna = false, tonaca = false, scheletro = false, armatura = false;
    int attrezzo = 0;       // 0 niente, 1 spada, 2 ascia, 3 piccone, 4 bastone, 5 lancia, 6 falce, 7 canna, 8 secchio, 9 arco, 10 pala, 11 bastone magico
    std::string colAttrezzo = "#c8ccd4";
};
// Disegna una persona con i piedi in (0,0), alta circa 100 unita'. passo 0..3 (camminata), braccio 0 fermo, 1 alzato, 2 colpo.
void Figura(K& c, const Look& o, int passo, int braccio) {
    float sw[4] = { 0, 6, 0, -6 };
    float s = passo >= 0 ? sw[passo % 4] : 0;
    float bob = (passo == 1 || passo == 3) ? -1.5f : 0;
    Ombra(c, 3, 1, 20, 6, 0.38f);
    c.save(); c.translate(0, bob);
    std::string stoffa = o.scheletro ? "#e8e0c8" : o.veste;
    // mantello dietro
    if (!o.mantello.empty()) Pieno(c, { { -13, -76 }, { 12, -76 }, { 17, -30 + s * 0.3f }, { -19, -28 } }, sh(o.mantello, -0.15f));
    // gambe
    auto Gamba = [&](float dx, float sx, const std::string& col) {
        if (o.scheletro) { Linea(c, dx, -42, dx + sx, -3, "#e8e0c8", 3); return; }
        Linea(c, dx, -42, dx + sx, -6, col, 7);
        Pieno(c, { { dx + sx - 4, -10 }, { dx + sx + 4, -10 }, { dx + sx + 7, 0 }, { dx + sx - 4, 0 } }, o.stivali);
    };
    if (!o.gonna && !o.tonaca) { Gamba(-5, -s, sh(o.calzoni, -0.2f)); Gamba(5, s, o.calzoni); }
    // corpo
    if (o.scheletro) {
        for (int k = 0; k < 4; k++) Linea(c, -10, -70 + k * 6, 10, -70 + k * 6, "#e8e0c8", 2.5f);
        Linea(c, 0, -76, 0, -42, "#d8d0b8", 3);
        Pieno(c, { { -9, -46 }, { 9, -46 }, { 6, -38 }, { -6, -38 } }, "#d8d0b8");
    } else {
        float fondo = o.gonna || o.tonaca ? -1 : -36, larg = o.gonna || o.tonaca ? 20 : 15;
        c.fillStyle(lg(-14, 0, 14, 0, { sh(stoffa, 0.18f), stoffa, sh(stoffa, -0.35f) }));
        Via(c, { { -12, -78 }, { 12, -78 }, { larg, fondo }, { -larg - 1, fondo } }); c.fill();
        if (o.armatura) {
            c.fillStyle(lg(-12, -78, 12, -50, { "#e8eef4", "#9aa4b0", "#5a6470" }));
            Via(c, { { -12, -78 }, { 12, -78 }, { 13, -52 }, { -13, -52 } }); c.fill();
            Linea(c, -12, -64, 12, -64, "#4a525a", 1.5f);
        }
        if (!o.tonaca) Linea(c, -13, -50, 13, -50, "#3a2614", 3);
        else Linea(c, -13, -54, 13, -54, sh(stoffa, -0.5f), 2);
        if (o.gonna) for (int k = -1; k <= 1; k++) Linea(c, k * 7.f, -46, k * 9.f, -3, rgba("#000000", 0.15f), 1.5f);
    }
    // braccia
    auto Braccio = [&](float spalla, float hx, float hy, bool davanti) {
        std::string col = o.scheletro ? "#e8e0c8" : davanti ? stoffa : sh(stoffa, -0.25f);
        Linea(c, spalla, -74, hx, hy, col, o.scheletro ? 3 : 6);
        c.fillStyle(V(o.scheletro ? "#e8e0c8" : o.pelle)); c.beginPath(); c.arc(hx, hy, 3.2f, 0, TAU); c.fill();
    };
    float hx = 15 + s * 0.4f, hy = -46;
    if (braccio == 1) { hx = 10; hy = -100; }
    if (braccio == 2) { hx = 26; hy = -60; }
    Braccio(-11, -15 - s * 0.4f, -46, false);
    // attrezzo nella mano
    if (o.attrezzo) {
        float ang = braccio == 1 ? -2.2f : braccio == 2 ? -0.3f : -1.2f;
        if (o.attrezzo == 4 || o.attrezzo == 5 || o.attrezzo == 11) ang = braccio == 2 ? -0.5f : -1.45f;
        float ca = std::cos(ang), sa = std::sin(ang);
        auto P = [&](float a, float b) { return Vector2{ hx + ca * a - sa * b, hy + sa * a + ca * b }; };
        std::string legno = "#6a4424", ferro = o.colAttrezzo;
        switch (o.attrezzo) {
        case 1: { Linea(c, P(-6, 0).x, P(-6, 0).y, P(36, 0).x, P(36, 0).y, ferro, 4); Linea(c, P(0, -6).x, P(0, -6).y, P(0, 6).x, P(0, 6).y, "#6a4a1a", 3);
                  Linea(c, P(4, -1).x, P(4, -1).y, P(34, -1).x, P(34, -1).y, rgba("#ffffff", 0.6f), 1); break; }
        case 2: { Linea(c, P(-8, 0).x, P(-8, 0).y, P(30, 0).x, P(30, 0).y, legno, 3.5f);
                  Pieno(c, { P(22, -2), P(32, -2), P(34, 10), P(24, 8) }, ferro); break; }
        case 3: { Linea(c, P(-8, 0).x, P(-8, 0).y, P(30, 0).x, P(30, 0).y, legno, 3.5f);
                  c.strokeStyle(V(ferro)); c.lineWidth(3.5f); c.beginPath(); c.moveTo(P(26, -14).x, P(26, -14).y); c.quadraticCurveTo(P(34, 0).x, P(34, 0).y, P(26, 14).x, P(26, 14).y); c.stroke(); break; }
        case 4: case 11: { Linea(c, P(-30, 0).x, P(-30, 0).y, P(40, 0).x, P(40, 0).y, legno, 3.5f);
                  if (o.attrezzo == 11) { c.glow(10, "#c070ff"); c.fillStyle(V("#e0a0ff")); c.beginPath(); c.arc(P(42, 0).x, P(42, 0).y, 5, 0, TAU); c.fill(); c.noGlow(); } break; }
        case 5: { Linea(c, P(-30, 0).x, P(-30, 0).y, P(44, 0).x, P(44, 0).y, legno, 3); Pieno(c, { P(42, -4), P(56, 0), P(42, 4) }, ferro); break; }
        case 6: { Linea(c, P(-6, 0).x, P(-6, 0).y, P(30, 0).x, P(30, 0).y, legno, 3);
                  c.strokeStyle(V(ferro)); c.lineWidth(3); c.beginPath(); c.moveTo(P(30, 0).x, P(30, 0).y); c.quadraticCurveTo(P(40, 16).x, P(40, 16).y, P(24, 24).x, P(24, 24).y); c.stroke(); break; }
        case 7: { Linea(c, P(-6, 0).x, P(-6, 0).y, P(50, 0).x, P(50, 0).y, legno, 2); Linea(c, P(50, 0).x, P(50, 0).y, P(54, 30).x, P(54, 30).y, rgba("#ffffff", 0.6f), 1); break; }
        case 8: { Pieno(c, { { hx - 7, hy + 2 }, { hx + 7, hy + 2 }, { hx + 6, hy + 14 }, { hx - 6, hy + 14 } }, "#7a5a32"); break; }
        case 9: { c.strokeStyle(V(legno)); c.lineWidth(3); c.beginPath(); c.arc(hx, hy, 16, -1.3f, 1.3f); c.stroke(); Linea(c, hx + 4, hy - 15, hx + 4, hy + 15, "#e0d8c0", 1); break; }
        case 10: { Linea(c, P(-6, 0).x, P(-6, 0).y, P(34, 0).x, P(34, 0).y, legno, 3); Pieno(c, { P(32, -6), P(44, -5), P(44, 5), P(32, 6) }, ferro); break; }
        }
    }
    // testa
    float tx = 1, ty = -88;
    if (!o.mantello.empty() && o.copricapo != 1) Pieno(c, { { -12, -80 }, { 12, -80 }, { 9, -74 }, { -9, -74 } }, o.mantello);
    c.fillStyle(V(o.scheletro ? "#ece4cc" : o.pelle)); c.beginPath(); c.arc(tx, ty, 9.5f, 0, TAU); c.fill();
    if (o.scheletro) { c.fillStyle(V("#2a2018")); c.beginPath(); c.arc(tx + 3, ty - 1, 2.6f, 0, TAU); c.fill(); c.beginPath(); c.arc(tx - 3, ty - 1, 2.4f, 0, TAU); c.fill(); }
    else {
        c.fillStyle(V(rgba("#000000", 0.15f))); c.beginPath(); c.arc(tx - 3, ty + 1, 8, 1.2f, 3.6f); c.fill();
        c.fillStyle(V("#2a1a10")); c.fillRect(tx + 3.5f, ty - 2, 2, 2.4f);
        if (o.barba) { c.fillStyle(V(o.capelli)); Via(c, { { tx - 6, ty + 2 }, { tx + 8, ty + 2 }, { tx + 4, ty + 14 }, { tx - 2, ty + 12 } }); c.fill(); }
    }
    std::string cap = o.cappello.empty() ? o.capelli : o.cappello;
    switch (o.copricapo) {
    case 0: c.fillStyle(V(o.capelli)); c.beginPath(); c.arc(tx - 1, ty - 2, 10, PI_ * 0.95f, TAU + 0.15f); c.fill(); break;
    case 1: c.fillStyle(V(o.mantello.empty() ? cap : o.mantello)); Via(c, { { tx - 12, ty + 6 }, { tx - 10, ty - 10 }, { tx + 2, ty - 14 }, { tx + 10, ty - 6 }, { tx + 6, ty - 4 }, { tx - 4, ty + 8 } }); c.fill(); break;
    case 2: c.fillStyle(V(cap)); c.beginPath(); c.ellipse(tx, ty - 6, 17, 4.5f, 0, 0, TAU); c.fill(); c.beginPath(); c.arc(tx, ty - 7, 8, PI_, TAU); c.fill(); break;
    case 3: c.fillStyle(lg(tx - 10, 0, tx + 10, 0, { "#e0e6ee", "#8a94a0" })); c.beginPath(); c.arc(tx, ty - 1, 11, PI_ * 1.0f, TAU); c.fill();
            c.fillRect(tx - 11, ty - 2, 22, 4); Linea(c, tx + 2, ty - 12, tx + 2, ty + 2, "#5a6470", 2); break;
    case 4: c.fillStyle(V(o.capelli)); c.beginPath(); c.arc(tx - 1, ty - 2, 10, PI_ * 0.95f, TAU + 0.15f); c.fill();
            c.glow(6, "#ffd040"); Pieno(c, { { tx - 9, ty - 8 }, { tx + 9, ty - 8 }, { tx + 10, ty - 19 }, { tx + 5, ty - 13 }, { tx, ty - 21 }, { tx - 5, ty - 13 }, { tx - 10, ty - 19 } }, "#f0c030"); c.noGlow(); break;
    case 5: c.fillStyle(V(o.capelli)); c.beginPath(); c.arc(tx - 1, ty - 2, 10, PI_ * 0.95f, TAU + 0.15f); c.fill(); c.beginPath(); c.arc(tx - 7, ty - 9, 5.5f, 0, TAU); c.fill(); break;
    case 6: c.fillStyle(V(cap)); c.beginPath(); c.ellipse(tx, ty - 6, 15, 4, 0, 0, TAU); c.fill(); Pieno(c, { { tx - 9, ty - 7 }, { tx + 9, ty - 7 }, { tx + 6, ty - 34 } }, cap); break;
    case 7: c.fillStyle(V(o.capelli)); c.beginPath(); c.arc(tx - 1, ty - 2, 10, PI_ * 0.95f, TAU + 0.15f); c.fill(); Linea(c, tx - 10, ty - 3, tx + 10, ty - 3, cap, 4); break;
    }
    // braccio davanti
    Braccio(10, hx, hy, true);
    c.restore();
}

// --- Alberi e piante (dal vero, a pennellate) ---------------------------------------------
void Latifoglia(K& c, R2& r, float s, float hue, bool mele, bool chiaro = false) {
    Ombra(c, 40 * s, 6 * s, 80 * s, 24 * s, 0.32f);
    c.fillStyle(lg(-9 * s, 0, 9 * s, 0, { "#6a4a2a", "#4a3420", "#2a1c10" }));
    Via(c, { { -10 * s, 0 }, { -5 * s, -75 * s }, { 5 * s, -75 * s }, { 10 * s, 0 } }); c.fill();
    Linea(c, 0, -70 * s, -26 * s, -105 * s, "#4a3420", 5 * s); Linea(c, 0, -70 * s, 24 * s, -110 * s, "#4a3420", 4 * s);
    float L0 = chiaro ? 24 : 18;
    for (int k = 0; k < 26; k++) {
        float a = r.F() * TAU, d = std::sqrt(r.F()) * 60 * s;
        float cx = std::cos(a) * d * 1.1f, cy = -125 * s + std::sin(a) * d * 0.9f;
        float lite = -(std::cos(a) + std::sin(a)) * 0.5f * d / (60 * s);
        for (int i = 0; i < 38; i++) {
            float ex = cx + r.F(-22, 22) * s, ey = cy + r.F(-18, 18) * s, top = (cy - ey) / (18 * s);
            Dab(c, ex, ey, r.F(4, 8) * s, Hsl(hue + r.F(-10, 10), r.F(30, 50), L0 + lite * 10 + top * 6 + r.F(0, 10), 0.92f), r.F(0, 3));
        }
    }
    for (int i = 0; i < 70; i++) Dab(c, -25 * s + r.F(-30, 30) * s, -165 * s + r.F(-20, 20) * s, r.F(3, 6) * s, Hsl(hue - 15, 50, r.F(45, 60), 0.7f), r.F(0, 3));
    if (mele) for (int i = 0; i < 22; i++) { c.fillStyle(V(i % 4 ? "#b8282a" : "#e04a3a")); c.beginPath(); c.arc(r.F(-55, 55) * s, -125 * s + r.F(-40, 40) * s, 3.8f * s, 0, TAU); c.fill(); }
}
void Abete(K& c, R2& r, float s, bool neve) {
    Ombra(c, 30 * s, 6 * s, 60 * s, 18 * s, 0.32f);
    c.fillStyle(V("#3a2616")); c.fillRect(-5 * s, -30 * s, 10 * s, 34 * s);
    for (int l = 0; l < 9; l++) {
        float yy = -25 * s - l * 22 * s, ww = (70 - l * 7) * s;
        for (int i = 0; i < 70; i++) {
            float t = r.F(-1, 1);
            std::string col = neve && r.F() < 0.3f && t < 0.4f ? Hsl(210, 20, r.F(85, 96), 0.95f) : Hsl(r.F(120, 150), r.F(25, 40), r.F(14, 30) + (t < 0 ? 8 : 0), 0.92f);
            float px = t * ww, py = yy + std::fabs(t) * 14 * s + r.F(-6, 6) * s;
            Linea(c, t * ww * 0.3f, py - 10 * s, px, py, col, 2.2f);
        }
    }
}
void Cespuglio(K& c, R2& r, float s, float hue, int bacche, const std::string& colBacche = "#c02040") {
    Ombra(c, 8 * s, 2 * s, 36 * s, 10 * s, 0.3f);
    for (int i = 0; i < 90; i++) { float ex = r.F(-30, 30) * s, ey = r.F(-38, -2) * s; Dab(c, ex, ey, r.F(3, 7) * s, Hsl(hue + r.F(-8, 8), r.F(30, 50), r.F(18, 40) + (-ey / s - 20) * 0.6f, 0.95f), r.F(0, 3)); }
    for (int i = 0; i < bacche; i++) {
        float x = r.F(-26, 26) * s, y = r.F(-34, -6) * s;
        c.fillStyle(V(colBacche)); c.beginPath(); c.arc(x, y, 2.8f * s, 0, TAU); c.fill();
        c.fillStyle(V("rgba(255,255,255,0.6)")); c.beginPath(); c.arc(x - 0.8f * s, y - 0.8f * s, 0.9f * s, 0, TAU); c.fill();
    }
}
void Sasso(K& c, R2& r, float x, float y, float rx, float ry, const std::string& col) {
    c.fillStyle(rgs(x - rx * 0.3f, y - ry * 0.5f, 1, rx * 1.3f, { { 0, sh(col, 0.35f) }, { 0.6f, col }, { 1, sh(col, -0.45f) } }));
    c.beginPath();
    for (int i = 0; i < 9; i++) { float a = PI_ + i * PI_ / 8, rr = 0.85f + r.F() * 0.25f; c.lineTo(x + std::cos(a) * rx * rr, y + std::sin(a) * ry * 1.6f * rr); }
    c.lineTo(x + rx, y + ry * 0.2f); c.lineTo(x - rx, y + ry * 0.2f); c.closePath(); c.fill();
    c.strokeStyle(V(rgba("#1a140c", 0.35f))); c.lineWidth(1.5f); c.stroke();
}
void Roccia(K& c, R2& r, float s, const std::string& col, const std::string& vena, int nVene) {
    Ombra(c, 12 * s, 4 * s, 62 * s, 14 * s, 0.35f);
    Sasso(c, r, 0, 0, 56 * s, 34 * s, col);
    Sasso(c, r, -30 * s, 4 * s, 26 * s, 16 * s, sh(col, -0.08f));
    Sasso(c, r, 34 * s, 4 * s, 22 * s, 13 * s, sh(col, 0.05f));
    for (int i = 0; i < nVene; i++) {
        float x = r.F(-36, 36) * s, y = r.F(-46, -8) * s;
        c.glow(vena == "#7adcff" ? 8.f : 0.f, vena);
        Dab(c, x, y, r.F(4, 7) * s, vena, r.F(0, 3), 0.7f);
        c.noGlow();
        Dab(c, x - 1.5f * s, y - 1.5f * s, 1.6f * s, rgba("#ffffff", 0.6f), 0);
    }
    for (int i = 0; i < 40; i++) Dab(c, r.F(-50, 50) * s, r.F(-50, -4) * s, r.F(1, 3) * s, rgba(r.F() < 0.5f ? "#ffffff" : "#000000", 0.08f), 0);
}
void Cristalli(K& c, R2& r, float s, const std::string& col) {
    Ombra(c, 6 * s, 2 * s, 46 * s, 12 * s, 0.35f);
    Sasso(c, r, 0, 2 * s, 40 * s, 14 * s, "#5a5660");
    for (int i = 0; i < 6; i++) {
        float x = r.F(-28, 28) * s, h = r.F(40, 100) * s, w = r.F(9, 16) * s, a = r.F(-0.35f, 0.35f);
        c.save(); c.translate(x, -6 * s); c.rotate(a);
        c.glow(14, rgba(col, 0.8f));
        c.fillStyle(lg(-w, 0, w, 0, { sh(col, 0.6f), col, sh(col, -0.45f) }));
        Via(c, { { -w, 0 }, { -w * 0.8f, -h * 0.8f }, { 0, -h }, { w * 0.8f, -h * 0.8f }, { w, 0 } }); c.fill(); c.noGlow();
        Linea(c, 0, -h, 0, 0, rgba("#ffffff", 0.5f), 1.2f);
        c.restore();
    }
}

// --- Casa isometrica a graticcio (G = spigolo davanti in basso) ------------------------------
void Casa(K& c, R2& r, Vector2 G, float Rl, float Ll, float Hh, float rh, bool paglia, bool porta, const std::string& muroCol) {
    Vector2 Rv{ Rl, -Rl / 2 }, Lv{ -Ll, -Ll / 2 }, up{ 0, -Hh };
    Vector2 Gt = Pv(G, up), Rt = Pv(Gt, Rv), Lt = Pv(Gt, Lv), Bt = Pv(Rt, Lv);
    Vector2 r0 = Pv(Pv(Gt, Lv, 0.5f), { 0, -rh }), r1 = Pv(r0, Rv);
    Pieno(c, { G, Pv(G, Rv), Pv(Pv(G, Rv), { Hh * 0.9f, Hh * 0.15f }), Pv(Pv(Pv(G, Rv), Lv), { Hh * 1.1f, Hh * 0.1f }), Pv(Pv(G, Rv), Lv) }, "rgba(20,25,5,0.36)");
    auto Muro = [&](Vector2 o, Vector2 e, bool scuro) {
        std::vector<Vector2> P = { o, Pv(o, e), Pv(Pv(o, e), up), Pv(o, up) };
        Pieno(c, P, scuro ? sh(muroCol, -0.25f) : muroCol);
        c.save(); Via(c, P); c.clip();
        for (int i = 0; i < 500; i++) { float u = r.F(), v = r.F(); Vector2 p = Pv(Pv(o, e, u), up, v); c.fillStyle(V(r.F() < 0.5f ? "rgba(255,250,230,0.07)" : "rgba(80,60,40,0.07)")); c.fillRect(p.x, p.y, r.F(3, 9), r.F(2, 4)); }
        for (int row = 0; row < 3; row++)
            for (int k = 0; k < 20; k++) {
                float u = (k + (row % 2) * 0.5f) / 20, v = row * 8 / Hh + 0.02f;
                Vector2 p = Pv(Pv(o, e, u), up, v);
                c.fillStyle(V(Hsl(35, r.F(8, 16), r.F(scuro ? 30 : 40, scuro ? 42 : 56)))); c.beginPath(); c.ellipse(p.x, p.y, std::fabs(e.x) / 22, 5, std::atan2(e.y, e.x), 0, TAU); c.fill();
            }
        std::string trave = scuro ? "#3a2616" : "#4e321c";
        auto L = [&](float u1, float v1, float u2, float v2) { Vector2 a = Pv(Pv(o, e, u1), up, v1), b = Pv(Pv(o, e, u2), up, v2); Linea(c, a.x, a.y, b.x, b.y, trave, 6); };
        L(0, 0.2f, 1, 0.2f); L(0, 0.6f, 1, 0.6f); L(0, 1, 1, 1); L(0, 0.2f, 0, 1); L(1, 0.2f, 1, 1);
        int n = std::max(2, (int)std::round(std::hypot(e.x, e.y) / 90));
        for (int k = 1; k < n; k++) { L(k / (float)n, 0.2f, k / (float)n, 1); if (k % 2) L((k - 1) / (float)n, 0.2f, k / (float)n, 0.6f); else L((k - 1) / (float)n, 0.6f, k / (float)n, 0.2f); }
        for (int k = 0; k < n; k++) {
            if (porta && k == 0 && !scuro) {
                Vector2 a = Pv(Pv(o, e, (k + 0.25f) / n), up, 0.04f), b = Pv(a, e, 0.5f / n), bt = Pv(b, up, 0.48f), at = Pv(a, up, 0.48f);
                Pieno(c, { a, b, bt, at }, "#3a2414");
                for (int q = 1; q < 4; q++) { Vector2 x = Pv(a, e, q * 0.125f / n), y = Pv(x, up, 0.48f); Linea(c, x.x, x.y, y.x, y.y, "#6a4a2a", 2); }
                continue;
            }
            Vector2 a = Pv(Pv(o, e, (k + 0.3f) / n), up, 0.72f), b = Pv(a, e, 0.4f / n), bt = Pv(b, up, 0.18f), at = Pv(a, up, 0.18f);
            bool luce = r.F() < 0.5f;
            Pieno(c, { a, b, bt, at }, luce ? "#e0b858" : "#2a2a22");
            c.strokeStyle(V("#4e321c")); c.lineWidth(3); Via(c, { a, b, bt, at }); c.stroke();
        }
        c.restore();
    };
    Muro(Pv(G, Lv), { Ll, Ll / 2 }, false);
    Muro(G, Rv, true);
    std::vector<Vector2> gab = { Gt, Lt, r0 };
    Pieno(c, gab, sh(muroCol, -0.08f));
    c.save(); Via(c, gab); c.clip();
    Linea(c, Pv(Gt, Lv, 0.5f).x, Pv(Gt, Lv, 0.5f).y, r0.x, r0.y, "#4e321c", 5);
    c.restore();
    auto Falda = [&](Vector2 A, Vector2 B, Vector2 C, Vector2 D, const std::string& base) {
        Pieno(c, { A, B, C, D }, base);
        c.save(); Via(c, { A, B, C, D }); c.clip();
        const int righe = 20;
        for (int rr = 0; rr <= righe; rr++) {
            float v = rr / (float)righe;
            Vector2 p = Pv(A, { D.x - A.x, D.y - A.y }, v), q = Pv(B, { C.x - B.x, C.y - B.y }, v);
            int n = (int)std::round(std::hypot(q.x - p.x, q.y - p.y) / 14);
            for (int k = 0; k < n; k++) {
                float u = (k + (rr % 2) * 0.5f + r.F(-0.15f, 0.15f)) / n;
                float x = p.x + (q.x - p.x) * u, y = p.y + (q.y - p.y) * u;
                if (paglia) Linea(c, x, y, x + r.F(-3, 3) + (A.x - D.x) * 0.05f, y + (A.y - D.y) * 0.05f + r.F(6, 12), Hsl(r.F(36, 46), r.F(35, 55), r.F(38, 62), 0.9f), 2);
                else {
                    c.fillStyle(V(Hsl(r.F(20, 35), r.F(5, 15), r.F(30, 50)))); c.beginPath(); c.ellipse(x, y, 7, 4.5f, std::atan2(q.y - p.y, q.x - p.x), 0, TAU); c.fill();
                    c.fillStyle(V("rgba(0,0,0,0.25)")); c.fillRect(x - 6, y + 3, 12, 1.5f);
                }
            }
        }
        c.restore();
        c.strokeStyle(V("rgba(30,20,10,0.6)")); c.lineWidth(3); Via(c, { A, B, C, D }); c.stroke();
    };
    Falda(Pv(Lt, { -10, 6 }), Pv(Bt, { 10, -4 }), r1, r0, paglia ? "#a08048" : "#6a6458");
    Falda(Pv(Gt, { -14, 10 }), Pv(Rt, { 16, 8 }), Pv(r1, { 6, 0 }), Pv(r0, { -4, 0 }), paglia ? "#8a6a38" : "#5a554a");
    Vector2 ch = Pv(Pv(r0, Rv, 0.7f), { 0, 10 });
    for (int k = 0; k < 9; k++) { c.fillStyle(V(Hsl(30, 10, r.F(38, 58)))); c.fillRect(ch.x - 12 + (k % 3) * 8, ch.y - 20 - (k / 3) * 10, 9, 9); }
}
// La posizione del comignolo (per il fumo dal vivo), rispetto al piede della casa
Vector2 Comignolo(float Rl, float Ll, float Hh, float rh) {
    Vector2 G{ 0, 0 }, Rv{ Rl, -Rl / 2 }, Lv{ -Ll, -Ll / 2 }, up{ 0, -Hh };
    Vector2 r0 = Pv(Pv(Pv(G, up), Lv, 0.5f), { 0, -rh });
    Vector2 ch = Pv(Pv(r0, Rv, 0.7f), { 0, 10 });
    return { ch.x, ch.y - 40 };
}

// --- Le forme degli elementi: misura della texture, piede, densita' e area cliccabile --------
struct Spec { int w, h; float ax, ay, dens; float hw, hh, hx; };
Spec SpecForma(int f) {
    if (f <= F_BAMBINO && f != F_MERCANTE) return { 72, 116, 36, 108, 2, 46, 92, 0 };
    switch (f) {
    case F_MERCANTE: return { 240, 200, 120, 182, 1.5f, 200, 170, 0 };
    case F_ALBERO: case F_MELO: return { 250, 300, 115, 284, 1, 130, 270, 0 };
    case F_ABETE: return { 170, 300, 80, 292, 1, 110, 270, 0 };
    case F_CESPUGLIO: return { 110, 70, 55, 62, 1.6f, 90, 64, 0 };
    case F_FUNGHI: return { 80, 50, 40, 42, 2, 76, 54, 0 };
    case F_ERBE: return { 90, 56, 45, 48, 2, 80, 58, 0 };
    case F_GRANO: return { 180, 120, 90, 100, 1, 160, 110, 0 };
    case F_CANNE: return { 130, 130, 65, 118, 1, 110, 120, 0 };
    case F_ROCCIA: case F_FERRO: case F_CARBONE: case F_RAME: return { 150, 100, 75, 86, 1.2f, 130, 90, 0 };
    case F_CRISTALLI: return { 120, 140, 60, 126, 1.2f, 100, 130, 0 };
    case F_POZZO: return { 150, 180, 75, 160, 1, 120, 170, 0 };
    case F_PESCI: return { 180, 90, 90, 56, 1, 170, 80, 0 };
    case F_CONCHIGLIE: return { 110, 50, 55, 38, 1.6f, 110, 56, 0 };
    case F_ALVEARE: return { 90, 130, 45, 122, 1.4f, 80, 124, 0 };
    case F_PALMA: return { 240, 320, 110, 308, 1, 110, 290, 0 };
    case F_CACTUS: return { 110, 160, 55, 150, 1, 80, 150, 0 };
    case F_DUNA: return { 190, 90, 95, 74, 1, 170, 80, 0 };
    case F_SALINA: return { 190, 100, 95, 72, 1, 170, 80, 0 };
    case F_CASA_PAGLIA: return { 660, 500, 200, 490, 1, 360, 360, 50 };
    case F_CASA_TEGOLE: return { 720, 545, 220, 535, 1, 380, 390, 50 };
    case F_TORRE: return { 260, 470, 130, 450, 1, 180, 420, 0 };
    case F_MULINO: return { 420, 560, 200, 530, 1, 220, 480, 0 };
    case F_TENDA: return { 250, 210, 125, 192, 1, 220, 180, 0 };
    case F_CARRETTO: return { 270, 170, 135, 150, 1, 220, 130, 0 };
    case F_BARILI: return { 120, 90, 60, 80, 1.5f, 100, 80, 0 };
    case F_CASSE: return { 130, 100, 65, 88, 1.5f, 110, 90, 0 };
    case F_RECINTO: return { 330, 160, 165, 136, 1, 300, 130, 0 };
    case F_FALO: return { 120, 90, 60, 78, 1.4f, 100, 80, 0 };
    case F_COLONNE: return { 270, 290, 135, 270, 1, 220, 270, 0 };
    case F_STATUA: return { 150, 290, 75, 276, 1, 110, 270, 0 };
    case F_BARCA: return { 300, 150, 150, 120, 1, 260, 110, 0 };
    case F_FIORI: return { 210, 70, 105, 54, 1, 190, 60, 0 };
    case F_PONTILE: return { 330, 170, 165, 120, 1, 300, 120, 0 };
    case F_TRONCO: return { 370, 160, 185, 128, 1, 330, 130, 0 };
    case F_PONTE: return { 400, 210, 200, 150, 1, 360, 170, 0 };
    case F_FRANA: return { 350, 240, 175, 210, 1, 320, 210, 0 };
    case F_ROVI: return { 320, 180, 160, 158, 1, 300, 160, 0 };
    case F_CANCELLO: return { 330, 290, 165, 270, 1, 300, 260, 0 };
    case F_GHIACCIO: return { 330, 270, 165, 248, 1, 300, 240, 0 };
    case F_FORZIERE: return { 100, 80, 50, 70, 1.6f, 90, 76, 0 };
    case F_QUERCIA: return { 480, 540, 230, 520, 1, 260, 460, 0 };
    case F_FARO: return { 240, 580, 120, 560, 1, 140, 540, 0 };
    }
    return { 100, 100, 50, 90, 1, 80, 80, 0 };
}

Look LookPersona(int f, const std::string& col, unsigned seme) {
    Look o;
    R2 r(seme);
    o.veste = col; o.calzoni = sh(col, -0.5f);
    const char* capelli[] = { "#3a2414", "#6a4422", "#2a1a12", "#9a6a3a", "#b89a6a", "#5a3a20" };
    const char* pelli[] = { "#e8b890", "#d8a078", "#c08060", "#f0c8a0", "#a87050" };
    o.capelli = capelli[(int)(r.F() * 6) % 6]; o.pelle = pelli[(int)(r.F() * 5) % 5];
    switch (f) {
    case F_CONTADINO: o.copricapo = r.F() < 0.5f ? 2 : 0; o.cappello = "#c8a860"; break;
    case F_ANZIANO: o.capelli = "#e8e8e0"; o.barba = true; o.tonaca = true; o.attrezzo = 4; o.copricapo = 0; break;
    case F_DAMA: o.gonna = true; o.copricapo = 5; break;
    case F_GUARDIA: o.copricapo = 3; o.attrezzo = 5; o.armatura = r.F() < 0.5f; o.mantello = sh(col, -0.3f); break;
    case F_MONACO: o.tonaca = true; o.copricapo = 1; o.mantello = sh(col, -0.15f); break;
    case F_MERCANTE: o.copricapo = 2; o.cappello = "#7a2a2a"; o.barba = true; break;
    case F_RE: o.copricapo = 4; o.barba = true; o.tonaca = true; o.mantello = "#a01a2a"; o.capelli = "#d8d8d0"; break;
    case F_BAMBINO: o.copricapo = 0; break;
    }
    return o;
}
Look LookEroe(const json& gen, int arma) {
    Look o;
    o.veste = Esa(ColoreDi(gen, "veste", Hex(0x3a6a3a))); o.mantello = Esa(ColoreDi(gen, "mantello", Hex(0x7a4a22)));
    o.capelli = Esa(ColoreDi(gen, "capelli", Hex(0x5a3a1a))); o.calzoni = "#4a3a2a"; o.copricapo = 1; o.pelle = "#e8b890";
    o.attrezzo = arma;
    return o;
}

// Dipinge una forma con il piede in (0,0). stato: 0 normale, 1 esaurita (ceppo, cespuglio spoglio), 2 ostacolo superato.
void DipingiForma(K& c, int f, const std::string& col, unsigned seme, int stato) {
    R2 r(seme * 7919u + (unsigned)f * 131u + 17u);
    if (f <= F_BAMBINO && f != F_MERCANTE) {
        c.save(); if (f == F_BAMBINO) c.scale(0.72f, 0.72f);
        Figura(c, LookPersona(f, col, seme), 0, 0);
        c.restore(); return;
    }
    switch (f) {
    case F_MERCANTE: {
        Ombra(c, 10, 4, 100, 18, 0.35f);
        c.save(); c.translate(0, -28); Figura(c, LookPersona(F_MERCANTE, col, seme), -1, 0); c.restore();
        // banco
        Pieno(c, { { -90, -10 }, { 90, -10 }, { 90, -60 }, { -90, -60 } }, "#7a5430");
        for (int i = 0; i < 6; i++) Linea(c, -90 + i * 36.f, -60, -90 + i * 36.f, -10, "#5a3a1e", 2);
        Pieno(c, { { -96, -60 }, { 96, -60 }, { 86, -72 }, { -86, -72 } }, "#9a6e40");
        const char* merci[] = { "#c83030", "#e0c060", "#7a9ab0", "#5a9a3a", "#d07a3a", "#c8a070" };
        for (int i = 0; i < 9; i++) { float x = -76 + i * 19.f; c.fillStyle(V(merci[i % 6])); c.beginPath(); c.arc(x, -72, 7, PI_, TAU); c.fill(); }
        // tendone a strisce
        Linea(c, -92, -60, -92, -150, "#4a3018", 5); Linea(c, 92, -60, 92, -150, "#4a3018", 5);
        for (int i = 0; i < 8; i++) Pieno(c, { { -100 + i * 25.f, -150 }, { -75 + i * 25.f, -150 }, { -73 + i * 25.f, -128 }, { -98 + i * 25.f, -128 } }, i % 2 ? "#f0e6c8" : col);
        for (int i = 0; i < 8; i++) { c.fillStyle(V(i % 2 ? "#f0e6c8" : col)); c.beginPath(); c.arc(-86 + i * 25.f, -128, 12.5f, 0, PI_); c.fill(); }
        break;
    }
    case F_ALBERO: if (stato == 1) goto ceppo; Latifoglia(c, r, 1.0f, 88 + r.F(-12, 10), false); break;
    case F_MELO: if (stato == 1) { Latifoglia(c, r, 0.95f, 95, false); break; } Latifoglia(c, r, 0.95f, 95, true); break;
    case F_ABETE: if (stato == 1) goto ceppo; Abete(c, r, 1.0f, col == "#ffffff"); break;
    ceppo: {
        Ombra(c, 10, 2, 30, 9, 0.3f);
        c.fillStyle(lg(-18, 0, 18, 0, { "#7a5a32", "#5a3e22", "#3a2614" })); Via(c, { { -18, 0 }, { -16, -24 }, { 16, -26 }, { 18, 0 } }); c.fill();
        c.fillStyle(V("#c8a070")); c.beginPath(); c.ellipse(0, -25, 16, 6, 0, 0, TAU); c.fill();
        c.strokeStyle(V("#8a6a40")); c.lineWidth(1.2f); c.beginPath(); c.ellipse(0, -25, 9, 3.4f, 0, 0, TAU); c.stroke();
        for (int i = 0; i < 8; i++) Dab(c, r.F(-40, 40), r.F(-4, 4), r.F(3, 6), Hsl(30, 40, r.F(30, 50), 0.8f), r.F(0, 3));
        break;
    }
    case F_CESPUGLIO: Cespuglio(c, r, 1.0f, 95, stato == 1 ? 0 : 16); break;
    case F_FUNGHI: {
        Ombra(c, 0, 0, 30, 6, 0.25f);
        for (int i = 0; i < 5; i++) Dab(c, r.F(-24, 24), r.F(-4, 2), r.F(4, 8), Hsl(90, 35, r.F(20, 35)), r.F(0, 3));
        if (stato == 1) break;
        for (int i = 0; i < 4; i++) {
            float x = -18 + i * 12 + r.F(-3, 3), h = r.F(10, 20), rr = r.F(7, 11);
            Linea(c, x, 0, x, -h, "#efe6d0", 4);
            c.fillStyle(rgs(x - 3, -h - 3, 1, rr, { { 0, "#e8a070" }, { 1, i % 2 ? "#a0502a" : "#c8402a" } })); c.beginPath(); c.ellipse(x, -h, rr, rr * 0.6f, 0, PI_, TAU); c.fill();
            for (int d = 0; d < 3; d++) { c.fillStyle(V("#fff4e0")); c.beginPath(); c.arc(x + r.F(-rr * 0.6f, rr * 0.6f), -h - r.F(1, rr * 0.4f), 1.3f, 0, TAU); c.fill(); }
        }
        break;
    }
    case F_ERBE: {
        Ombra(c, 0, 0, 32, 7, 0.25f);
        int n = stato == 1 ? 6 : 26;
        for (int i = 0; i < n; i++) { float x = r.F(-26, 26), h = r.F(14, 34) * (stato == 1 ? 0.4f : 1); c.strokeStyle(V(Hsl(r.F(95, 125), 45, r.F(28, 48)))); c.lineWidth(2.5f); c.beginPath(); c.moveTo(x, 0); c.quadraticCurveTo(x + r.F(-8, 8), -h * 0.6f, x + r.F(-10, 10), -h); c.stroke(); }
        if (stato != 1) for (int i = 0; i < 7; i++) { c.fillStyle(V(i % 2 ? "#e8e0f8" : "#b080e0")); c.beginPath(); c.arc(r.F(-22, 22), r.F(-30, -14), 2.6f, 0, TAU); c.fill(); }
        break;
    }
    case F_GRANO: {
        Pieno(c, { { -80, -20 }, { 0, -60 }, { 80, -20 }, { 0, 20 } }, "#6a5030");
        for (int i = 0; i < 70; i++) Dab(c, r.F(-60, 60), r.F(-40, 10), r.F(2, 4), Hsl(30, 30, r.F(20, 35)), 0);
        if (stato == 1) { for (int i = 0; i < 60; i++) { float u = r.F(-1, 1), v = r.F(-1, 1); float x = (u - v) * 70, y = (u + v) * 18 - 20; Linea(c, x, y, x, y - 5, "#b8a050", 1.5f); } break; }
        for (int i = 0; i < 260; i++) {
            float u = r.F(-1, 1), v = r.F(-1, 1), x = (u - v) * 72, y = (u + v) * 19 - 20, h = r.F(28, 46);
            Linea(c, x, y, x + r.F(-3, 3), y - h, Hsl(r.F(40, 50), r.F(50, 70), r.F(45, 62)), 1.6f);
            Dab(c, x + r.F(-3, 3), y - h, 2.6f, Hsl(42, 70, r.F(55, 70)), -1.2f, 0.45f);
        }
        break;
    }
    case F_CANNE: {
        Ombra(c, 0, 0, 50, 10, 0.25f);
        Pieno(c, { { -50, -4 }, { 0, -22 }, { 50, -4 }, { 0, 14 } }, rgba("#3a6a7a", 0.6f));
        int n = stato == 1 ? 10 : 40;
        for (int i = 0; i < n; i++) {
            float x = r.F(-44, 44), y = r.F(-12, 6), h = r.F(50, 110) * (stato == 1 ? 0.35f : 1);
            Linea(c, x, y, x + r.F(-6, 6), y - h, Hsl(r.F(60, 80), 40, r.F(35, 55)), 2);
            if (stato != 1 && r.F() < 0.5f) Dab(c, x + r.F(-6, 6), y - h, 3.5f, "#6a4a2a", 1.5f, 0.35f);
        }
        break;
    }
    case F_ROCCIA: if (stato == 1) { Roccia(c, r, 0.55f, "#8a8478", "", 0); break; } Roccia(c, r, 1, "#9a9488", "", 0); break;
    case F_FERRO: if (stato == 1) { Roccia(c, r, 0.55f, "#7a7068", "", 0); break; } Roccia(c, r, 1, "#7a7068", "#c07a5a", 8); break;
    case F_CARBONE: if (stato == 1) { Roccia(c, r, 0.55f, "#5a5654", "", 0); break; } Roccia(c, r, 1, "#5a5654", "#1a1a1e", 10); break;
    case F_RAME: if (stato == 1) { Roccia(c, r, 0.55f, "#8a6a52", "", 0); break; } Roccia(c, r, 1, "#8a6a52", "#3ac0a0", 4); Roccia(c, r, 0.01f, "#000000", "#e08a3a", 0);
        for (int i = 0; i < 6; i++) Dab(c, r.F(-30, 30), r.F(-40, -10), 5, "#e08a3a", r.F(0, 3), 0.7f);
        break;
    case F_CRISTALLI: if (stato == 1) { Sasso(c, r, 0, 2, 40, 14, "#5a5660"); break; } Cristalli(c, r, 1, col.size() > 1 && col != "#6a5a4a" ? col : "#7adcff"); break;
    case F_POZZO: {
        Ombra(c, 20, 6, 62, 18, 0.35f);
        for (int rr = 0; rr < 4; rr++)
            for (int k = 0; k < 14; k++) { float a = k / 14.f * PI_; c.fillStyle(V(Hsl(30, 8, r.F(36, 58)))); c.beginPath(); c.ellipse(-std::cos(a) * 44, -rr * 12 + std::sin(a) * 16 - 4, 9, 7, 0, 0, TAU); c.fill(); }
        c.fillStyle(V("#1e2a30")); c.beginPath(); c.ellipse(0, -50, 36, 13, 0, 0, TAU); c.fill();
        c.fillStyle(V(rgba("#4a8ab0", 0.5f))); c.beginPath(); c.ellipse(0, -48, 26, 8, 0, 0, TAU); c.fill();
        Linea(c, -40, -44, -40, -124, "#4a3020", 7); Linea(c, 40, -44, 40, -124, "#4a3020", 7);
        for (int k = 0; k < 60; k++) { float u = k / 60.f; c.fillStyle(V(Hsl(r.F(36, 44), 40, r.F(35, 55)))); c.fillRect(-56 + u * 112, -144 + std::fabs(u - 0.5f) * 40 - r.F(0, 10), 6, 14); }
        Linea(c, 0, -124, 0, -76, "#8a7050", 1.5f);
        Pieno(c, { { -8, -80 }, { 8, -80 }, { 7, -66 }, { -7, -66 } }, "#7a5a32");
        break;
    }
    case F_PESCI: {
        c.fillStyle(rgs(0, -20, 4, 88, { { 0, "#5ab0d0" }, { 0.7f, "#2a6a8a" }, { 1, "rgba(42,106,138,0)" } })); c.beginPath(); c.ellipse(0, -20, 88, 34, 0, 0, TAU); c.fill();
        for (int i = 0; i < 10; i++) Sasso(c, r, std::cos(i * 0.63f) * 82, -20 + std::sin(i * 0.63f) * 32, 12, 7, "#8a8478");
        for (int i = 0; i < 6; i++) { c.strokeStyle(V(rgba("#ffffff", 0.35f))); c.lineWidth(1.5f); c.beginPath(); c.ellipse(r.F(-50, 50), r.F(-34, -8), r.F(8, 16), 3, 0, 0, TAU); c.stroke(); }
        if (stato != 1) for (int i = 0; i < 4; i++) {
            float x = r.F(-45, 45), y = r.F(-30, -10), a = r.F(-0.5f, 0.5f);
            c.save(); c.translate(x, y); c.rotate(a);
            Dab(c, 0, 0, 10, rgba("#c8d8e0", 0.8f), 0, 0.4f); Pieno(c, { { -9, 0 }, { -16, -4 }, { -16, 4 } }, rgba("#c8d8e0", 0.8f));
            c.restore();
        }
        break;
    }
    case F_CONCHIGLIE: {
        for (int i = 0; i < 10; i++) Dab(c, r.F(-40, 40), r.F(-12, 2), r.F(6, 12), Hsl(40, 40, r.F(70, 82), 0.6f), r.F(0, 3));
        int n = stato == 1 ? 1 : 6;
        for (int i = 0; i < n; i++) {
            float x = r.F(-36, 36), y = r.F(-10, 0);
            c.fillStyle(lg(x, y - 12, x, y, { "#fff0e8", i % 2 ? "#f0a890" : "#e8c8a0" })); c.beginPath(); c.moveTo(x - 9, y); c.lineTo(x, y - 12); c.lineTo(x + 9, y); c.closePath(); c.fill();
            for (int k = -1; k <= 1; k++) Linea(c, x, y - 11, x + k * 6.f, y, rgba("#a06050", 0.5f), 1);
        }
        break;
    }
    case F_ALVEARE: {
        Ombra(c, 6, 2, 30, 8, 0.3f);
        c.fillStyle(V("#5a3e22")); c.fillRect(-16, -40, 32, 40);
        for (int k = 0; k < 5; k++) { c.fillStyle(V(stato == 1 ? "#8a7a60" : k % 2 ? "#e0a030" : "#c88a20")); c.beginPath(); c.ellipse(0, -48 - k * 12, 26 - k * 3.5f, 9, 0, 0, TAU); c.fill(); }
        c.fillStyle(V("#2a1a0a")); c.beginPath(); c.arc(0, -52, 4, 0, TAU); c.fill();
        break;
    }
    case F_PALMA: {
        Ombra(c, 50, 4, 70, 16, 0.3f);
        c.strokeStyle(V("#8a6a40")); c.lineWidth(12); c.beginPath(); c.moveTo(0, 0); c.quadraticCurveTo(30, -120, 10, -230); c.stroke();
        for (int i = 0; i < 12; i++) Linea(c, 5 + i * 1.5f, -i * 19.f, 20 - i * 0.6f, -i * 19.f - 4, "#5a4020", 2);
        for (int i = 0; i < 8; i++) {
            float a = i * TAU / 8 + 0.3f, L = r.F(80, 110);
            c.strokeStyle(V(Hsl(r.F(95, 120), 45, r.F(25, 38)))); c.lineWidth(10); c.beginPath(); c.moveTo(10, -230);
            c.quadraticCurveTo(10 + std::cos(a) * L * 0.6f, -250 + std::sin(a) * L * 0.3f - 20, 10 + std::cos(a) * L, -230 + std::sin(a) * L * 0.5f + 30); c.stroke();
        }
        break;
    }
    case F_CACTUS: {
        Ombra(c, 20, 2, 34, 8, 0.3f);
        auto Braccio = [&](float x, float y, float w, float h) { c.fillStyle(lg(x - w, 0, x + w, 0, { "#7ab060", "#3a7a3a", "#1a4a2a" })); c.beginPath(); c.ellipse(x, y - h / 2, w, h / 2, 0, 0, TAU); c.fill(); };
        Braccio(0, 0, 16, 140); Braccio(-26, -50, 9, 50); Braccio(26, -70, 9, 50);
        Linea(c, -16, -60, -26, -60, "#3a7a3a", 9); Linea(c, 16, -80, 26, -80, "#3a7a3a", 9);
        for (int i = 0; i < 30; i++) Linea(c, r.F(-14, 14), -r.F(5, 135), r.F(-16, 16), -r.F(5, 135), "rgba(255,255,220,0.25)", 1);
        break;
    }
    case F_DUNA: {
        c.fillStyle(rgs(-20, -40, 5, 90, { { 0, "#f0d8a0" }, { 1, "#c89858" } })); c.beginPath(); c.ellipse(0, -14, 86, stato == 1 ? 14 : 34, 0, PI_, TAU); c.fill();
        c.fillStyle(V("#c89858")); c.beginPath(); c.ellipse(0, -14, 86, 10, 0, 0, PI_); c.fill();
        for (int i = 0; i < 6; i++) { c.strokeStyle(V(rgba("#8a6030", 0.35f))); c.lineWidth(1.5f); c.beginPath(); c.ellipse(r.F(-30, 30), -16 - i * 4.f, 60 - i * 8.f, 6, 0, PI_ * 1.1f, PI_ * 1.9f); c.stroke(); }
        if (stato != 1) { Linea(c, 40, -30, 60, -80, "#6a4424", 4); Pieno(c, { { 34, -24 }, { 46, -36 }, { 52, -22 }, { 40, -14 } }, "#9aa0a8"); }
        break;
    }
    case F_SALINA: {
        Pieno(c, { { -86, -24 }, { 0, -64 }, { 86, -24 }, { 0, 16 } }, "#8a8478");
        Pieno(c, { { -72, -24 }, { 0, -56 }, { 72, -24 }, { 0, 8 } }, stato == 1 ? "#6a9ab0" : "#f4f4f0");
        if (stato != 1) for (int i = 0; i < 4; i++) { float x = r.F(-30, 30), y = r.F(-34, -14); c.fillStyle(lg(0, y - 16, 0, y, { "#ffffff", "#d8d8d8" })); Via(c, { { x - 12, y }, { x, y - 16 }, { x + 12, y } }); c.fill(); }
        break;
    }
    case F_CASA_PAGLIA: Casa(c, r, { 0, 0 }, 280, 180, 140, 120, true, true, col.size() > 1 && col != "#6a5a4a" ? col : "#c8b48c"); break;
    case F_CASA_TEGOLE: Casa(c, r, { 0, 0 }, 300, 200, 160, 130, false, true, col.size() > 1 && col != "#6a5a4a" ? col : "#d0bc94"); break;
    case F_TORRE: {
        Ombra(c, 60, 4, 110, 26, 0.35f);
        c.fillStyle(lg(-70, 0, 70, 0, { "#c8c0b0", "#9a9284", "#5a544a" })); Via(c, { { -70, -10 }, { -64, -340 }, { 64, -340 }, { 70, -10 } }); c.fill();
        c.beginPath(); c.ellipse(0, -10, 70, 22, 0, 0, PI_); c.fill();
        for (int row = 0; row < 26; row++) for (int k = 0; k < 7; k++) { float x = -60 + k * 20 + (row % 2) * 10.f, y = -20 - row * 12.f; c.strokeStyle(V(rgba("#3a342a", 0.35f))); c.lineWidth(1); c.strokeRect(x - 9, y - 5, 18, 10); }
        Pieno(c, { { -12, -150 }, { 12, -150 }, { 12, -110 }, { -12, -110 } }, "#2a2014"); Pieno(c, { { -10, -260 }, { 10, -260 }, { 10, -230 }, { -10, -230 } }, "#e0b858");
        for (int k = 0; k < 6; k++) Pieno(c, { { -72 + k * 26.f, -340 }, { -58 + k * 26.f, -340 }, { -58 + k * 26.f, -364 }, { -72 + k * 26.f, -364 } }, "#a8a090");
        Pieno(c, { { -80, -364 }, { 80, -364 }, { 0, -450 } }, "#6a3a3a");
        Linea(c, 0, -450, 0, -470, "#3a2a1a", 2); Pieno(c, { { 0, -470 }, { 22, -464 }, { 0, -458 } }, "#c02a2a");
        break;
    }
    case F_MULINO: {
        Ombra(c, 60, 4, 120, 28, 0.35f);
        c.fillStyle(lg(-80, 0, 80, 0, { "#e0d0b0", "#b8a888", "#7a6a50" })); Via(c, { { -80, -6 }, { -56, -330 }, { 56, -330 }, { 80, -6 } }); c.fill();
        Pieno(c, { { -16, -6 }, { 16, -6 }, { 16, -70 }, { -16, -70 } }, "#3a2414");
        Pieno(c, { { -10, -200 }, { 10, -200 }, { 10, -170 }, { -10, -170 } }, "#2a2a22");
        Pieno(c, { { -70, -330 }, { 70, -330 }, { 0, -420 } }, "#8a6a38");
        for (int i = 0; i < 60; i++) Linea(c, r.F(-60, 60), -330 - r.F(0, 70), r.F(-60, 60), -330 - r.F(0, 70), Hsl(40, 45, r.F(40, 60), 0.6f), 1.5f);
        c.fillStyle(V("#3a2a1a")); c.beginPath(); c.arc(0, -330, 10, 0, TAU); c.fill();
        break;
    }
    case F_TENDA: {
        Ombra(c, 30, 4, 110, 22, 0.35f);
        std::string tela = col.size() > 1 && col != "#6a5a4a" && col != "#c8b48c" ? col : "#d8c8a0";
        Pieno(c, { { -100, 0 }, { 0, -170 }, { 20, -10 } }, sh(tela, 0.1f));
        Pieno(c, { { 20, -10 }, { 0, -170 }, { 110, -20 } }, sh(tela, -0.25f));
        Pieno(c, { { -20, -8 }, { 0, -110 }, { 10, -8 } }, "#2a1a10");
        for (int i = 0; i < 5; i++) Linea(c, -100 + i * 24.f, -i * 1.f, 0, -170, rgba("#8a2a2a", 0.5f), 3);
        Linea(c, 0, -170, 0, -190, "#5a3a1a", 3); Pieno(c, { { 0, -190 }, { 18, -185 }, { 0, -180 } }, "#c02a2a");
        break;
    }
    case F_CARRETTO: {
        Ombra(c, 20, 4, 110, 20, 0.35f);
        Pieno(c, { { -70, -10 }, { 60, -60 }, { 60, -80 }, { -70, -30 } }, "#7a5430");
        Pieno(c, { { -70, -30 }, { 60, -80 }, { 30, -95 }, { -100, -45 } }, "#9a6e40");
        for (int k = 0; k < 5; k++) { float x = -80 + k * 26.f, y = -50 - k * 10.f; c.fillStyle(V("#c8b080")); c.beginPath(); c.ellipse(x, y, 13, 17, 0, 0, TAU); c.fill(); c.fillStyle(V("rgba(0,0,0,0.2)")); c.beginPath(); c.ellipse(x + 4, y + 2, 7, 13, 0, 0, TAU); c.fill(); }
        c.strokeStyle(V("#3a2616")); c.lineWidth(5); c.beginPath(); c.ellipse(-30, -14, 16, 20, 0, 0, TAU); c.stroke();
        Linea(c, 60, -70, 120, -76, "#3a2616", 5);
        break;
    }
    case F_BARILI: {
        for (int k = 0; k < 2; k++) {
            float x = -18 + k * 30.f, y = -k * 4.f;
            Ombra(c, x + 8, y + 3, 16, 6, 0.3f);
            c.fillStyle(lg(x - 13, 0, x + 13, 0, { "#9a6a3a", "#6a4626", "#3a2616" })); c.fillRect(x - 13, y - 34, 26, 34);
            c.fillStyle(V("#3a3a3a")); c.fillRect(x - 13, y - 28, 26, 3); c.fillRect(x - 13, y - 9, 26, 3);
            c.fillStyle(V("#5a3a1e")); c.beginPath(); c.ellipse(x, y - 34, 13, 5, 0, 0, TAU); c.fill();
        }
        break;
    }
    case F_CASSE: {
        auto Cassa = [&](float x, float y) {
            Pieno(c, { { x, y }, { x + 24, y - 12 }, { x + 24, y - 36 }, { x, y - 24 } }, "#7a5430");
            Pieno(c, { { x, y }, { x - 20, y - 10 }, { x - 20, y - 34 }, { x, y - 24 } }, "#9a6e40");
            Pieno(c, { { x, y - 24 }, { x + 24, y - 36 }, { x + 4, y - 46 }, { x - 20, y - 34 } }, "#b08050");
            Linea(c, x - 20, y - 22, x, y - 12, "#5a3a1e", 2); Linea(c, x, y - 12, x + 24, y - 24, "#5a3a1e", 2);
        };
        Ombra(c, 10, 2, 44, 10, 0.3f); Cassa(-14, 0); Cassa(20, 6); Cassa(2, -24);
        break;
    }
    case F_RECINTO: {
        for (int k = 0; k < 9; k++) {
            float x = -150 + k * 37.f, y = 30 - k * 18.f;
            Pieno(c, { { x - 6, y }, { x - 6, y - 60 }, { x, y - 70 }, { x + 6, y - 60 }, { x + 6, y } }, Hsl(30, r.F(25, 35), r.F(26, 36)));
        }
        Linea(c, -150, -10, 146, -154, "#5a3a1e", 5); Linea(c, -150, -35, 146, -179, "#5a3a1e", 5);
        break;
    }
    case F_FALO: {
        for (int i = 0; i < 9; i++) { float a = i * TAU / 9; Sasso(c, r, std::cos(a) * 34, std::sin(a) * 12, 9, 6, "#8a8478"); }
        Linea(c, -24, -4, 20, -14, "#4a3020", 6); Linea(c, -20, -14, 24, -4, "#5a3a20", 6);
        break;
    }
    case F_COLONNE: {
        Ombra(c, 30, 4, 120, 20, 0.3f);
        Pieno(c, { { -120, -10 }, { 0, -60 }, { 120, -10 }, { 0, 40 } }, "#a8a090");
        auto Col = [&](float x, float y, float h) {
            c.fillStyle(lg(x - 16, 0, x + 16, 0, { "#e8e0d0", "#b8b0a0", "#7a7468" })); c.fillRect(x - 16, y - h, 32, h);
            for (int k = -1; k <= 1; k++) Linea(c, x + k * 8.f, y - h, x + k * 8.f, y, rgba("#5a544a", 0.4f), 1.5f);
            c.fillStyle(V("#d8d0c0")); c.fillRect(x - 22, y - h - 10, 44, 10);
            for (int i = 0; i < 6; i++) Dab(c, x + r.F(-14, 14), y - r.F(0, h), r.F(4, 8), Hsl(100, 30, r.F(25, 40), 0.7f), r.F(0, 3));
        };
        Col(-70, -10, 190); Col(10, -40, 120); Col(80, 4, 70);
        for (int i = 0; i < 3; i++) Sasso(c, r, r.F(-100, 100), r.F(-10, 20), r.F(14, 24), r.F(8, 12), "#b8b0a0");
        break;
    }
    case F_STATUA: {
        Ombra(c, 20, 4, 60, 14, 0.3f);
        Pieno(c, { { -40, 0 }, { 40, 0 }, { 40, -50 }, { -40, -50 } }, "#a8a090"); Pieno(c, { { -46, -50 }, { 46, -50 }, { 40, -60 }, { -40, -60 } }, "#c8c0b0");
        c.save(); c.translate(0, -60); c.scale(1.9f, 1.9f);
        Look o; o.veste = "#b8b0a0"; o.calzoni = "#a8a090"; o.pelle = "#c0b8a8"; o.capelli = "#a8a090"; o.stivali = "#989080"; o.mantello = "#a09888"; o.copricapo = 4; o.attrezzo = 1; o.colAttrezzo = "#c0b8a8";
        Figura(c, o, -1, 0);
        c.restore();
        for (int i = 0; i < 14; i++) Dab(c, r.F(-40, 40), r.F(-60, -2), r.F(3, 7), Hsl(100, 30, r.F(25, 40), 0.6f), r.F(0, 3));
        break;
    }
    case F_BARCA: {
        Ombra(c, 0, -2, 140, 20, 0.25f);
        c.fillStyle(lg(0, -60, 0, 0, { "#9a6e40", "#6a4626", "#3a2616" })); c.beginPath(); c.moveTo(-130, -50); c.quadraticCurveTo(-90, 0, 0, 0); c.quadraticCurveTo(90, 0, 130, -60); c.lineTo(-130, -50); c.closePath(); c.fill();
        for (int k = 1; k < 4; k++) Linea(c, -120 + k * 4.f, -50 + k * 9.f, 120 - k * 6.f, -58 + k * 10.f, rgba("#2a1a0a", 0.4f), 1.5f);
        Linea(c, 0, -50, 0, -110, "#5a3a1e", 3);
        break;
    }
    case F_FIORI: {
        for (int i = 0; i < 30; i++) Dab(c, r.F(-90, 90), r.F(-30, 0), r.F(4, 8), Hsl(r.F(80, 110), 40, r.F(25, 40)), r.F(0, 3));
        const char* fc[] = { "#e8d040", "#e05a40", "#f0f0e0", "#c060d0", "#ff90b0" };
        for (int i = 0; i < 60; i++) { c.fillStyle(V(fc[i % 5])); c.beginPath(); c.arc(r.F(-90, 90), r.F(-30, -2), 2.8f, 0, TAU); c.fill(); }
        break;
    }
    case F_PONTILE: {
        Pieno(c, { { -160, 0 }, { -40, -60 }, { 160, -60 }, { 40, 0 } }, rgba("#2a6a8a", 0.55f));
        for (int k = 0; k < 10; k++) { float x = -130 + k * 26.f; Pieno(c, { { x, -2 }, { x + 70, -38 }, { x + 90, -38 }, { x + 20, -2 } }, k % 2 ? "#9a6e40" : "#8a6038"); }
        for (int k = 0; k < 4; k++) Linea(c, -110 + k * 70.f, 10, -110 + k * 70.f, -30, "#4a3020", 6);
        break;
    }
    case F_TRONCO: {
        if (stato == 2) { for (int i = 0; i < 12; i++) Dab(c, r.F(-150, 150), r.F(-20, 10), r.F(4, 9), Hsl(30, 40, r.F(30, 45), 0.8f), r.F(0, 3)); break; }
        Ombra(c, 10, 4, 170, 24, 0.35f);
        c.save(); c.rotate(-0.28f);
        c.fillStyle(lg(0, -40, 0, 0, { "#8a6a42", "#5a3e22", "#2a1c10" })); c.fillRect(-160, -40, 320, 40);
        for (int i = 0; i < 20; i++) Linea(c, -160 + i * 16.f, -38 + r.F(0, 10), -150 + i * 16.f, -2 - r.F(0, 10), rgba("#2a1a0a", 0.4f), 1.5f);
        c.fillStyle(V("#c8a070")); c.beginPath(); c.ellipse(160, -20, 10, 20, 0, 0, TAU); c.fill();
        for (int i = 0; i < 4; i++) { c.strokeStyle(V("#8a6a40")); c.lineWidth(1); c.beginPath(); c.ellipse(160, -20, 2 + i * 2.f, 5 + i * 4.f, 0, 0, TAU); c.stroke(); }
        c.restore();
        Cespuglio(c, r, 0.8f, 90, 0);
        break;
    }
    case F_PONTE: {
        // acqua sotto il ponte (fa parte del disegno: il canale attraversa la scena in orizzontale)
        if (stato == 2) {
            for (int k = 0; k < 11; k++) { float x = -150 + k * 30.f; Pieno(c, { { x, -10 }, { x + 26, -10 }, { x + 26, -60 }, { x, -60 } }, k % 2 ? "#a07848" : "#8a6038"); Linea(c, x, -10, x, -60, "#4a3020", 2); }
            Linea(c, -160, -64, 160, -64, "#5a3a1e", 6); Linea(c, -160, -6, 160, -6, "#5a3a1e", 6);
            for (int k = 0; k < 5; k++) { Linea(c, -150 + k * 75.f, -64, -150 + k * 75.f, -100, "#4a3020", 6); }
            Linea(c, -150, -96, 150, -96, "#5a3a1e", 4);
            break;
        }
        for (int k = 0; k < 4; k++) { float x = -150 + k * 30.f; Pieno(c, { { x, -10 }, { x + 26, -10 }, { x + 26, -60 }, { x, -60 } }, "#8a6038"); }
        for (int k = 0; k < 3; k++) { float x = 70 + k * 30.f; Pieno(c, { { x, -10 }, { x + 26, -10 }, { x + 26, -60 }, { x, -60 } }, "#7a5430"); }
        Linea(c, -160, -64, -30, -64, "#5a3a1e", 6); Linea(c, 70, -64, 160, -64, "#5a3a1e", 6);
        c.save(); c.translate(10, -20); c.rotate(0.6f); c.fillStyle(V("#6a4626")); c.fillRect(-30, -6, 60, 10); c.restore();
        Linea(c, -150, -64, -150, -100, "#4a3020", 6); Linea(c, 150, -64, 150, -100, "#4a3020", 6);
        break;
    }
    case F_FRANA: {
        if (stato == 2) { for (int i = 0; i < 10; i++) Sasso(c, r, r.F(-160, 160), r.F(-10, 10), r.F(8, 14), r.F(5, 8), "#8a8478"); break; }
        Ombra(c, 10, 4, 170, 30, 0.35f);
        for (int i = 0; i < 16; i++) { float x = r.F(-140, 140), y = r.F(-30, 0) - (140 - std::fabs(x)) * 0.6f * r.F(0.5f, 1); Sasso(c, r, x, y + 0.f, r.F(26, 50), r.F(18, 30), Hsl(30, r.F(5, 12), r.F(40, 58))); }
        break;
    }
    case F_ROVI: {
        if (stato == 2) { for (int i = 0; i < 20; i++) Dab(c, r.F(-140, 140), r.F(-14, 6), r.F(4, 9), Hsl(20, 30, r.F(10, 25), 0.85f), r.F(0, 3)); break; }
        Ombra(c, 10, 4, 150, 24, 0.3f);
        for (int i = 0; i < 70; i++) {
            float x0 = r.F(-140, 140), y0 = r.F(-10, 4);
            c.strokeStyle(V(Hsl(r.F(280, 320), 25, r.F(18, 30)))); c.lineWidth(r.F(2, 4)); c.beginPath(); c.moveTo(x0, y0);
            c.bezierCurveTo(x0 + r.F(-40, 40), y0 - r.F(30, 90), x0 + r.F(-60, 60), y0 - r.F(40, 140), x0 + r.F(-60, 60), y0 - r.F(20, 120)); c.stroke();
        }
        for (int i = 0; i < 40; i++) Dab(c, r.F(-130, 130), r.F(-130, -10), r.F(3, 6), Hsl(r.F(280, 330), 40, r.F(25, 40), 0.8f), r.F(0, 3));
        for (int i = 0; i < 14; i++) { c.glow(8, "#c060ff"); c.fillStyle(V("#e0a0ff")); c.beginPath(); c.arc(r.F(-120, 120), r.F(-120, -20), 2.2f, 0, TAU); c.fill(); c.noGlow(); }
        break;
    }
    case F_CANCELLO: {
        Ombra(c, 10, 4, 160, 22, 0.3f);
        auto Pilastro = [&](float x) { c.fillStyle(lg(x - 24, 0, x + 24, 0, { "#c8c0b0", "#8a8478", "#5a544a" })); c.fillRect(x - 24, -240, 48, 240); c.fillStyle(V("#a8a090")); c.fillRect(x - 30, -256, 60, 18); };
        Pilastro(-140); Pilastro(140);
        if (stato == 2) {
            for (int k = 0; k < 5; k++) Linea(c, -116 + k * 10.f, -4, -112 + k * 10.f, -200, "#2a2a30", 4);
            for (int k = 0; k < 5; k++) Linea(c, 76 + k * 10.f, -4, 72 + k * 10.f, -200, "#2a2a30", 4);
            break;
        }
        for (int k = 0; k < 23; k++) { float x = -114 + k * 10.4f; Linea(c, x, -2, x, -210 + std::fabs(k - 11.f) * 3, "#2a2a30", 4); Pieno(c, { { x - 3, -210 + std::fabs(k - 11.f) * 3 }, { x, -220 + std::fabs(k - 11.f) * 3 }, { x + 3, -210 + std::fabs(k - 11.f) * 3 } }, "#2a2a30"); }
        Linea(c, -116, -60, 116, -60, "#2a2a30", 5); Linea(c, -116, -150, 116, -150, "#2a2a30", 5);
        c.fillStyle(V("#d0b050")); c.fillRect(-10, -110, 20, 24); c.fillStyle(V("#2a2010")); c.beginPath(); c.arc(0, -102, 3, 0, TAU); c.fill();
        break;
    }
    case F_GHIACCIO: {
        if (stato == 2) { for (int i = 0; i < 16; i++) Dab(c, r.F(-150, 150), r.F(-10, 8), r.F(6, 14), rgba("#a8d8f0", 0.5f), r.F(0, 3)); break; }
        Ombra(c, 10, 4, 160, 24, 0.3f);
        for (int i = 0; i < 9; i++) {
            float x = -130 + i * 32.f + r.F(-8, 8), h = r.F(120, 220), w = r.F(26, 40);
            c.glow(10, "rgba(160,220,255,0.6)");
            c.fillStyle(lg(x - w, 0, x + w, 0, { "#f0faff", "#a8d8f0", "#5a9ac8" }));
            Via(c, { { x - w, 0 }, { x - w * 0.7f, -h * 0.8f }, { x, -h }, { x + w * 0.7f, -h * 0.85f }, { x + w, 0 } }); c.fill(); c.noGlow();
            Linea(c, x - w * 0.3f, -h * 0.9f, x - w * 0.5f, -10, rgba("#ffffff", 0.6f), 2);
        }
        break;
    }
    case F_FORZIERE: {
        Ombra(c, 6, 2, 40, 9, 0.35f);
        if (stato == 1) {
            Pieno(c, { { -34, 0 }, { 34, 0 }, { 34, -30 }, { -34, -30 } }, "#6a4626");
            Pieno(c, { { -34, -30 }, { 34, -30 }, { 30, -60 }, { -30, -60 } }, "#8a5e32");
            c.fillStyle(V("#1a1008")); c.fillRect(-30, -34, 60, 6);
            break;
        }
        c.fillStyle(lg(0, -30, 0, 0, { "#9a6a3a", "#6a4626" })); c.fillRect(-34, -30, 68, 30);
        c.fillStyle(lg(0, -50, 0, -30, { "#b07a42", "#8a5e32" })); c.beginPath(); c.moveTo(-34, -30); c.quadraticCurveTo(0, -60, 34, -30); c.closePath(); c.fill();
        for (int k = -1; k <= 1; k += 2) { c.fillStyle(V("#d0b050")); c.fillRect(k * 22 - 4.f, -44, 8, 44); }
        c.fillStyle(V("#f0d060")); c.fillRect(-6, -32, 12, 12);
        break;
    }
    case F_QUERCIA: {
        Ombra(c, 80, 10, 200, 50, 0.32f);
        c.fillStyle(lg(-30, 0, 30, 0, { "#7a5a32", "#4a3420", "#2a1c10" }));
        Via(c, { { -46, 0 }, { -24, -140 }, { -60, -220 }, { -20, -200 }, { 0, -260 }, { 20, -200 }, { 70, -230 }, { 26, -140 }, { 50, 0 } }); c.fill();
        for (int i = 0; i < 12; i++) Linea(c, r.F(-30, 30), -r.F(0, 140), r.F(-30, 30), -r.F(0, 140), rgba("#1a1008", 0.3f), 2);
        for (int k = 0; k < 60; k++) {
            float a = r.F() * TAU, d = std::sqrt(r.F()) * 170;
            float cx = std::cos(a) * d * 1.2f, cy = -300 + std::sin(a) * d * 0.75f, lite = -(std::cos(a) + std::sin(a)) * 0.5f * d / 170;
            for (int i = 0; i < 30; i++) { float ex = cx + r.F(-30, 30), ey = cy + r.F(-24, 24); Dab(c, ex, ey, r.F(5, 10), Hsl(85 + r.F(-10, 10), r.F(35, 50), 20 + lite * 12 + (cy - ey) / 24 * 6 + r.F(0, 10), 0.92f), r.F(0, 3)); }
        }
        c.glow(18, "#ffd040");
        for (int i = 0; i < 12; i++) { float x = r.F(-160, 160), y = -300 + r.F(-110, 110); c.fillStyle(V("#e0b030")); c.beginPath(); c.ellipse(x, y, 5, 7, 0, 0, TAU); c.fill(); }
        c.noGlow();
        break;
    }
    case F_FARO: {
        Ombra(c, 60, 4, 110, 24, 0.35f);
        c.fillStyle(lg(-60, 0, 60, 0, { "#f0ece4", "#c8c0b0", "#8a8478" }));
        Via(c, { { -56, 0 }, { -32, -430 }, { 32, -430 }, { 56, 0 } }); c.fill();
        for (int k = 0; k < 4; k++) { float y0 = -40 - k * 110.f; Pieno(c, { { -54 + k * 5.6f, y0 }, { 54 - k * 5.6f, y0 }, { 52 - k * 5.6f, y0 - 40 }, { -52 + k * 5.6f, y0 - 40 } }, "#b03030"); }
        Pieno(c, { { -40, -430 }, { 40, -430 }, { 40, -444 }, { -40, -444 } }, "#3a3a3a");
        c.fillStyle(V(rgba("#f0e8a0", 0.8f))); c.fillRect(-26, -494, 52, 50);
        for (int k = 0; k < 4; k++) Linea(c, -26 + k * 17.3f, -494, -26 + k * 17.3f, -444, "#3a3a3a", 2);
        Pieno(c, { { -36, -494 }, { 36, -494 }, { 0, -540 } }, "#3a3a3a");
        break;
    }
    default: break;
    }
}

// --- Nemici (guardano a destra; piedi in 0,0) --------------------------------------------------
Spec SpecNemico(int f) {
    switch (f) {
    case N_LUPO: case N_CINGHIALE: return { 150, 100, 75, 90, 2, 120, 80, 0 };
    case N_BRIGANTE: case N_SCHELETRO: case N_STREGONE: case N_CAVALIERE: return { 90, 130, 45, 118, 2, 50, 100, 0 };
    case N_RAGNO: case N_SCORPIONE: return { 180, 110, 90, 96, 2, 140, 80, 0 };
    case N_SERPENTE: return { 160, 120, 80, 104, 2, 120, 100, 0 };
    case N_ORSO: return { 190, 150, 95, 138, 2, 150, 120, 0 };
    case N_TROLL: case N_GOLEM: return { 190, 220, 95, 206, 1.6f, 130, 200, 0 };
    case N_SPIRITO: return { 150, 180, 75, 160, 2, 100, 160, 0 };
    case N_DRAGO: return { 360, 280, 180, 256, 1.4f, 260, 220, 0 };
    }
    return { 100, 100, 50, 90, 2, 80, 80, 0 };
}
void DipingiNemico(K& c, int f, const std::string& col, unsigned seme) {
    R2 r(seme * 31u + (unsigned)f * 977u);
    std::string chiaro = sh(col, 0.35f), scuro = sh(col, -0.45f);
    switch (f) {
    case N_LUPO: {
        Ombra(c, 0, 2, 52, 9, 0.38f);
        Linea(c, -40, -40, -62, -56, scuro, 8);   // coda
        for (int k = 0; k < 4; k++) { float x = -30 + k * 18.f + (k % 2) * 4; Linea(c, x, -30, x + (k % 2 ? 4 : -2), 0, k < 2 ? scuro : col, 7); }
        c.fillStyle(lg(0, -64, 0, -22, { chiaro, col, scuro })); c.beginPath(); c.ellipse(-4, -40, 44, 18, -0.05f, 0, TAU); c.fill();
        for (int i = 0; i < 40; i++) Linea(c, r.F(-44, 36), r.F(-56, -26), r.F(-44, 36) - 6, r.F(-56, -26) + 4, rgba(i % 2 ? "#ffffff" : "#000000", 0.18f), 1.5f);
        c.fillStyle(V(col)); Via(c, { { 28, -54 }, { 52, -60 }, { 70, -48 }, { 66, -40 }, { 44, -36 }, { 30, -34 } }); c.fill();
        Pieno(c, { { 36, -58 }, { 40, -76 }, { 48, -58 } }, scuro); Pieno(c, { { 46, -60 }, { 54, -76 }, { 56, -58 } }, col);
        c.fillStyle(V("#ffe040")); c.beginPath(); c.arc(52, -52, 2.6f, 0, TAU); c.fill();
        c.fillStyle(V("#1a1a1a")); c.beginPath(); c.arc(69, -46, 2.6f, 0, TAU); c.fill();
        Linea(c, 58, -40, 68, -42, "#f0f0f0", 1.5f);
        break;
    }
    case N_CINGHIALE: {
        Ombra(c, 0, 2, 50, 9, 0.38f);
        for (int k = 0; k < 4; k++) { float x = -26 + k * 16.f; Linea(c, x, -24, x, 0, scuro, 7); }
        c.fillStyle(lg(0, -62, 0, -16, { chiaro, col, scuro })); c.beginPath(); c.ellipse(-4, -36, 42, 24, 0, 0, TAU); c.fill();
        for (int i = 0; i < 16; i++) Linea(c, -30 + i * 3.f, -58 + std::fabs(i - 8.f), -32 + i * 3.f, -66 + std::fabs(i - 8.f), scuro, 2);
        c.fillStyle(V(col)); Via(c, { { 28, -52 }, { 56, -40 }, { 60, -26 }, { 30, -20 } }); c.fill();
        c.fillStyle(V(sh(col, 0.5f))); c.beginPath(); c.ellipse(60, -32, 5, 7, 0, 0, TAU); c.fill();
        Linea(c, 50, -28, 58, -42, "#f8f0e0", 3); Linea(c, 44, -26, 50, -40, "#f8f0e0", 2.5f);
        c.fillStyle(V("#ff4020")); c.beginPath(); c.arc(42, -44, 2.4f, 0, TAU); c.fill();
        Pieno(c, { { 30, -50 }, { 34, -62 }, { 40, -50 } }, scuro);
        break;
    }
    case N_RAGNO: {
        Ombra(c, 0, 0, 70, 12, 0.38f);
        for (int k = 0; k < 4; k++)
            for (int sd = -1; sd <= 1; sd += 2) {
                float bx = -6 + k * 10.f, by = -34, kx = bx + sd * 0 + (k - 1.5f) * 26, ky = -66 + k * 2.f, fx = bx + (k - 1.5f) * 46, fy = sd < 0 ? 0 : -4;
                c.strokeStyle(V(sd < 0 ? scuro : col)); c.lineWidth(5); c.beginPath(); c.moveTo(bx, by); c.lineTo(kx, ky); c.lineTo(fx, fy); c.stroke();
            }
        c.fillStyle(rgs(-30, -50, 2, 40, { { 0, chiaro }, { 0.6f, col }, { 1, scuro } })); c.beginPath(); c.ellipse(-26, -40, 34, 26, 0, 0, TAU); c.fill();
        for (int i = 0; i < 3; i++) Pieno(c, { { -40 + i * 10.f, -60 }, { -34 + i * 10.f, -50 }, { -28 + i * 10.f, -60 } }, "#c02020");
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(16, -34, 20, 16, 0, 0, TAU); c.fill();
        for (int i = 0; i < 4; i++) { c.glow(6, "#ff3030"); c.fillStyle(V("#ff3030")); c.beginPath(); c.arc(22 + (i % 2) * 7.f, -40 + (i / 2) * 6.f, 2.4f, 0, TAU); c.fill(); c.noGlow(); }
        Linea(c, 32, -28, 38, -18, "#e8e0d0", 2.5f); Linea(c, 28, -26, 32, -16, "#e8e0d0", 2.5f);
        break;
    }
    case N_SCORPIONE: {
        Ombra(c, 0, 0, 70, 12, 0.38f);
        for (int k = 0; k < 3; k++) for (int sd = -1; sd <= 1; sd += 2) { float bx = -20 + k * 14.f; Linea(c, bx, -20, bx - 10 + k * 4.f, sd < 0 ? 0 : -6, scuro, 4); }
        for (int k = 0; k < 4; k++) { c.fillStyle(V(k % 2 ? col : sh(col, -0.15f))); c.beginPath(); c.ellipse(-10 + k * 14.f, -24, 14, 11, 0, 0, TAU); c.fill(); }
        // coda arcuata
        float tx = -20, ty = -24;
        for (int k = 0; k < 6; k++) { float a = PI_ * 0.95f + k * 0.42f; float nx = -44 + std::cos(a) * 30 * (1 - k * 0.04f), ny = -54 + std::sin(a) * 32; c.fillStyle(V(k % 2 ? col : scuro)); c.beginPath(); c.arc(nx, ny, 9 - k * 0.6f, 0, TAU); c.fill(); tx = nx; ty = ny; }
        Pieno(c, { { tx - 4, ty }, { tx + 14, ty + 6 }, { tx + 2, ty + 8 } }, "#2a1a0a");
        // chele
        for (int sd = 0; sd < 2; sd++) { float y = -20 - sd * 8.f; Linea(c, 30, y, 50, y - 6, col, 6); c.fillStyle(V(chiaro)); c.beginPath(); c.ellipse(58, y - 8, 12, 7, -0.3f, 0, TAU); c.fill(); Linea(c, 60, y - 8, 72, y - 2, scuro, 3); }
        c.fillStyle(V("#1a0a0a")); c.beginPath(); c.arc(40, -30, 2.2f, 0, TAU); c.fill();
        break;
    }
    case N_SERPENTE: {
        Ombra(c, 0, 0, 56, 10, 0.38f);
        c.strokeStyle(lg(-50, 0, 50, 0, { scuro, col, chiaro })); c.lineWidth(18);
        c.beginPath(); c.moveTo(-56, -8); c.bezierCurveTo(-30, 10, -10, -30, 10, -10); c.bezierCurveTo(30, 10, 30, -40, 10, -64); c.stroke();
        for (int i = 0; i < 14; i++) { float t = i / 14.f; Dab(c, -50 + t * 70, -6 - std::sin(t * 6) * 10 - t * 20, 3, rgba(chiaro, 0.5f), 0); }
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(18, -70, 18, 11, 0.4f, 0, TAU); c.fill();
        c.fillStyle(V("#ffe040")); c.beginPath(); c.arc(24, -74, 3, 0, TAU); c.fill();
        c.fillStyle(V("#000000")); c.fillRect(23.5f, -77, 1.4f, 6);
        Linea(c, 34, -64, 46, -60, "#d02020", 1.5f); Linea(c, 46, -60, 50, -64, "#d02020", 1.5f); Linea(c, 46, -60, 50, -57, "#d02020", 1.5f);
        break;
    }
    case N_ORSO: {
        Ombra(c, 0, 2, 70, 12, 0.38f);
        for (int k = 0; k < 4; k++) { float x = -40 + k * 26.f; Linea(c, x, -40, x + 2, -2, k < 2 ? scuro : col, 16); }
        c.fillStyle(lg(0, -110, 0, -30, { chiaro, col, scuro })); c.beginPath(); c.ellipse(-6, -66, 62, 40, -0.06f, 0, TAU); c.fill();
        for (int i = 0; i < 70; i++) Linea(c, r.F(-60, 50), r.F(-100, -36), r.F(-60, 50) - 6, r.F(-100, -36) + 6, rgba(i % 2 ? "#ffffff" : "#000000", 0.14f), 2);
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(58, -82, 26, 22, 0, 0, TAU); c.fill();
        c.beginPath(); c.arc(44, -102, 8, 0, TAU); c.fill(); c.beginPath(); c.arc(64, -104, 7, 0, TAU); c.fill();
        c.fillStyle(V(sh(col, -0.2f))); c.beginPath(); c.ellipse(78, -76, 12, 9, 0, 0, TAU); c.fill();
        c.fillStyle(V("#1a1a1a")); c.beginPath(); c.arc(86, -78, 3.5f, 0, TAU); c.fill(); c.beginPath(); c.arc(64, -88, 2.6f, 0, TAU); c.fill();
        break;
    }
    case N_TROLL: case N_GOLEM: {
        bool golem = f == N_GOLEM;
        Ombra(c, 0, 2, 70, 14, 0.4f);
        if (golem) {
            auto Blocco = [&](float x, float y, float w, float h) { c.fillStyle(lg(x - w, y - h, x + w, y + h, { chiaro, col, scuro })); Via(c, { { x - w, y - h * 0.8f }, { x - w * 0.7f, y - h }, { x + w, y - h * 0.9f }, { x + w * 0.9f, y + h }, { x - w * 0.9f, y + h } }); c.fill(); };
            Blocco(-22, -30, 18, 30); Blocco(24, -30, 18, 30);
            Blocco(0, -100, 52, 44); Blocco(-66, -110, 18, 34); Blocco(66, -104, 18, 34); Blocco(-70, -60, 16, 18); Blocco(72, -56, 16, 18);
            Blocco(0, -160, 26, 20);
            c.glow(12, "#ffb040");
            c.fillStyle(V("#ffd070")); c.fillRect(-14, -166, 9, 6); c.fillRect(6, -166, 9, 6);
            for (int i = 0; i < 4; i++) Linea(c, -30 + i * 16.f, -110 + (i % 2) * 14.f, -22 + i * 16.f, -96 + (i % 2) * 14.f, "#ffb040", 2.5f);
            c.noGlow();
        } else {
            Linea(c, -20, -70, -26, -4, scuro, 22); Linea(c, 20, -70, 26, -4, col, 22);
            c.fillStyle(lg(0, -170, 0, -60, { chiaro, col, scuro })); c.beginPath(); c.ellipse(0, -110, 52, 58, 0, 0, TAU); c.fill();
            Pieno(c, { { -46, -76 }, { 46, -76 }, { 40, -54 }, { -40, -54 } }, "#6a4a2a");
            Linea(c, -46, -130, -70, -66, scuro, 18);
            c.fillStyle(V(col)); c.beginPath(); c.arc(0, -172, 26, 0, TAU); c.fill();
            Pieno(c, { { -8, -164 }, { 0, -150 }, { 8, -164 } }, sh(col, -0.2f));
            c.fillStyle(V("#ffe040")); c.beginPath(); c.arc(-9, -176, 3.6f, 0, TAU); c.fill(); c.beginPath(); c.arc(9, -176, 3.6f, 0, TAU); c.fill();
            Linea(c, -10, -158, -6, -152, "#f8f0e0", 3); Linea(c, 10, -158, 6, -152, "#f8f0e0", 3);
            // clava
            Linea(c, 46, -130, 70, -76, col, 18);
            c.save(); c.translate(76, -80); c.rotate(-0.4f);
            c.fillStyle(lg(-10, 0, 10, 0, { "#8a6a42", "#4a3420" })); Via(c, { { -6, 0 }, { -16, -100 }, { 16, -100 }, { 6, 0 } }); c.fill();
            for (int i = 0; i < 6; i++) Pieno(c, { { -16, -50 - i * 9.f }, { -24, -46 - i * 9.f }, { -16, -42 - i * 9.f } }, "#c8c0b0");
            c.restore();
        }
        break;
    }
    case N_SPIRITO: {
        c.glow(26, rgba(col, 0.9f));
        c.fillStyle(lgs(0, -150, 0, 0, { { 0, rgba(chiaro, 0.95f) }, { 0.6f, rgba(col, 0.7f) }, { 1, rgba(col, 0) } }));
        c.beginPath(); c.moveTo(-40, -90); c.bezierCurveTo(-46, -160, 46, -160, 40, -90);
        c.bezierCurveTo(36, -40, 20, -20, 10, 0); c.bezierCurveTo(0, -20, -10, -10, -20, -4); c.bezierCurveTo(-26, -30, -40, -40, -40, -90); c.closePath(); c.fill();
        c.noGlow();
        Linea(c, -36, -90, -60, -60, rgba(col, 0.7f), 8); Linea(c, 36, -90, 62, -66, rgba(col, 0.7f), 8);
        c.fillStyle(V("#0a1a20")); c.beginPath(); c.ellipse(-12, -118, 7, 10, 0, 0, TAU); c.fill(); c.beginPath(); c.ellipse(12, -118, 7, 10, 0, 0, TAU); c.fill();
        c.beginPath(); c.ellipse(0, -94, 8, 5, 0, 0, TAU); c.fill();
        break;
    }
    case N_DRAGO: {
        Ombra(c, 0, 2, 150, 24, 0.4f);
        // ali
        c.fillStyle(lg(0, -250, 0, -100, { chiaro, scuro }));
        Via(c, { { -20, -120 }, { -120, -240 }, { -150, -170 }, { -170, -200 }, { -160, -120 }, { -90, -110 } }); c.fill();
        Via(c, { { 20, -130 }, { 80, -250 }, { 140, -210 }, { 160, -230 }, { 150, -140 }, { 70, -110 } }); c.fill();
        for (int k = 0; k < 3; k++) { Linea(c, -20, -120, -120 - k * 20.f, -240 + k * 40.f, scuro, 3); Linea(c, 20, -130, 80 + k * 30.f, -250 + k * 20.f, scuro, 3); }
        // coda
        c.strokeStyle(V(col)); c.lineWidth(22); c.beginPath(); c.moveTo(-60, -60); c.quadraticCurveTo(-140, -20, -170, -60); c.stroke();
        Pieno(c, { { -170, -60 }, { -190, -80 }, { -186, -46 } }, scuro);
        for (int k = 0; k < 4; k++) { float x = -60 + k * 40.f; Linea(c, x, -60, x + 4, -2, k < 2 ? scuro : col, 20); }
        c.fillStyle(lg(0, -150, 0, -40, { chiaro, col, scuro })); c.beginPath(); c.ellipse(0, -90, 90, 50, -0.1f, 0, TAU); c.fill();
        c.fillStyle(V(sh(col, 0.5f))); c.beginPath(); c.ellipse(10, -66, 60, 20, -0.1f, 0, TAU); c.fill();
        for (int k = 0; k < 7; k++) Pieno(c, { { -70 + k * 22.f, -132 + k * 2.f }, { -60 + k * 22.f, -152 + k * 2.f }, { -50 + k * 22.f, -132 + k * 2.f } }, scuro);
        // collo e testa
        c.strokeStyle(V(col)); c.lineWidth(30); c.beginPath(); c.moveTo(60, -110); c.quadraticCurveTo(100, -150, 120, -180); c.stroke();
        c.fillStyle(V(col)); Via(c, { { 104, -200 }, { 160, -196 }, { 176, -176 }, { 150, -164 }, { 110, -166 } }); c.fill();
        Pieno(c, { { 108, -198 }, { 96, -226 }, { 120, -200 } }, scuro); Pieno(c, { { 122, -200 }, { 116, -230 }, { 134, -198 } }, scuro);
        c.glow(10, "#ffd040"); c.fillStyle(V("#ffd040")); c.beginPath(); c.ellipse(138, -188, 5, 3.5f, 0, 0, TAU); c.fill(); c.noGlow();
        for (int k = 0; k < 4; k++) Pieno(c, { { 148 + k * 6.f, -168 }, { 151 + k * 6.f, -160 }, { 154 + k * 6.f, -168 } }, "#f8f0e0");
        break;
    }
    default: {   // figure umane: brigante, scheletro, stregone, cavaliere
        Look o;
        o.veste = col; o.calzoni = sh(col, -0.5f);
        if (f == N_BRIGANTE) { o.copricapo = 7; o.cappello = "#1a1a1a"; o.mantello = sh(col, -0.35f); o.attrezzo = 1; o.barba = r.F() < 0.5f; }
        if (f == N_SCHELETRO) { o.scheletro = true; o.attrezzo = 1; o.colAttrezzo = "#8a8a7a"; o.copricapo = -1; }
        if (f == N_STREGONE) { o.tonaca = true; o.copricapo = 6; o.cappello = sh(col, -0.2f); o.attrezzo = 11; o.barba = true; o.capelli = "#c8c8d0"; o.mantello = sh(col, -0.4f); }
        if (f == N_CAVALIERE) { o.armatura = true; o.copricapo = 3; o.attrezzo = 1; o.mantello = "#5a0a1a"; o.colAttrezzo = "#9098a8"; o.veste = "#3a3a46"; }
        c.save(); c.scale(1.08f, 1.08f); Figura(c, o, -1, 2); c.restore();
        if (f == N_STREGONE) { c.glow(20, "#c070ff"); c.fillStyle(V(rgba("#c070ff", 0.25f))); c.beginPath(); c.ellipse(0, -50, 40, 60, 0, 0, TAU); c.fill(); c.noGlow(); }
        break;
    }
    }
}

// --- Icone degli oggetti (96x96, centro 48,48) ---------------------------------------------
void DipingiIcona(K& c, int ic, const std::string& col) {
    R2 r((unsigned)ic * 97u + 5u);
    std::string ch = sh(col, 0.4f), sc = sh(col, -0.45f);
    c.translate(48, 48);
    auto Contorno = [&]() { c.strokeStyle(V(rgba("#1a1008", 0.85f))); c.lineWidth(2.5f); c.stroke(); };
    auto Tondo = [&](float x, float y, float rr, const std::string& a, const std::string& b) { c.fillStyle(rgs(x - rr * 0.35f, y - rr * 0.35f, 1, rr * 1.1f, { { 0, a }, { 1, b } })); c.beginPath(); c.arc(x, y, rr, 0, TAU); c.fill(); Contorno(); };
    auto Manico = [&](float x0, float y0, float x1, float y1) { Linea(c, x0, y0, x1, y1, "#1a1008", 8); Linea(c, x0, y0, x1, y1, "#8a5a30", 5); };
    switch (ic) {
    case I_LEGNA:
        for (int k = 0; k < 3; k++) { float y = 14 - k * 13.f, x = (k == 2) ? 0 : (k ? 10.f : -10.f); c.fillStyle(lg(0, y - 9, 0, y + 9, { "#a07040", "#6a4626" })); c.beginPath(); c.rect(x - 28, y - 9, 50, 18); c.fill(); Contorno();
            c.fillStyle(V("#e0c090")); c.beginPath(); c.ellipse(x + 22, y, 6, 9, 0, 0, TAU); c.fill(); Contorno(); }
        break;
    case I_PIETRA: case I_MINERALE: case I_CARBONE: case I_PEPITA: {
        std::string base = ic == I_PIETRA ? "#a8a294" : ic == I_CARBONE ? "#3a3a40" : ic == I_PEPITA ? col : "#8a7a6e";
        c.fillStyle(rgs(-10, -12, 2, 40, { { 0, sh(base, 0.45f) }, { 1, sh(base, -0.35f) } }));
        Via(c, { { -28, 18 }, { -32, -4 }, { -14, -26 }, { 12, -28 }, { 30, -8 }, { 26, 20 } }); c.fill(); Contorno();
        if (ic == I_MINERALE) for (int i = 0; i < 6; i++) Dab(c, r.F(-18, 18), r.F(-16, 12), 5, "#d07a5a", r.F(0, 3), 0.7f);
        if (ic == I_CARBONE) for (int i = 0; i < 5; i++) Dab(c, r.F(-18, 18), r.F(-16, 12), 3, "rgba(255,255,255,0.35)", 0);
        if (ic == I_PEPITA) for (int i = 0; i < 4; i++) Dab(c, r.F(-16, 16), r.F(-16, 10), 3, "rgba(255,255,255,0.6)", 0);
        break;
    }
    case I_BACCHE: for (int i = 0; i < 7; i++) Tondo(std::cos(i * 0.9f) * 14 * (i ? 1 : 0), std::sin(i * 0.9f) * 12 * (i ? 1 : 0) + 4, 11, ch, col);
        Pieno(c, { { -4, -20 }, { 18, -30 }, { 8, -16 } }, "#4a8a3a"); break;
    case I_FUNGHI: Linea(c, 0, 30, 0, 0, "#1a1008", 16); Linea(c, 0, 30, 0, 0, "#f0e6d0", 12);
        c.fillStyle(rgs(-8, -14, 2, 34, { { 0, "#f08060" }, { 1, col == "#c8a070" ? "#b04020" : col } })); c.beginPath(); c.ellipse(0, -2, 34, 24, 0, PI_, TAU); c.closePath(); c.fill(); Contorno();
        for (int i = 0; i < 5; i++) { c.fillStyle(V("#fff4e0")); c.beginPath(); c.arc(r.F(-22, 22), r.F(-18, -6), 3.2f, 0, TAU); c.fill(); } break;
    case I_GRANO: for (int k = -1; k <= 1; k++) { Linea(c, k * 10.f, 34, k * 14.f, -10, "#8a7030", 3); for (int i = 0; i < 5; i++) { Dab(c, k * 14.f - 5, -12 + i * 7.f, 5, col, -0.6f, 0.5f); Dab(c, k * 14.f + 5, -12 + i * 7.f, 5, col, 0.6f, 0.5f); } } break;
    case I_MELA: Tondo(0, 4, 28, ch, col); Linea(c, 0, -22, 4, -34, "#5a3a1e", 4); Pieno(c, { { 4, -26 }, { 22, -34 }, { 12, -20 } }, "#4a9a3a"); Dab(c, -10, -6, 6, "rgba(255,255,255,0.5)", -0.6f); break;
    case I_PESCE: c.fillStyle(lg(0, -18, 0, 18, { "#d8e8f0", col, sc })); c.beginPath(); c.ellipse(-4, 0, 32, 16, 0, 0, TAU); c.fill(); Contorno();
        Pieno(c, { { 24, 0 }, { 40, -16 }, { 40, 16 } }, col); c.fillStyle(V("#1a1a1a")); c.beginPath(); c.arc(-22, -4, 3, 0, TAU); c.fill(); break;
    case I_ACQUA: case I_POZIONE: {
        c.fillStyle(lg(-20, 0, 20, 0, { ch, col, sc })); c.beginPath(); c.arc(0, 10, 24, 0, TAU); c.fill(); Contorno();
        c.fillStyle(V("#d8e0e0")); c.fillRect(-8, -30, 16, 20); c.strokeStyle(V("#1a1008")); c.lineWidth(2.5f); c.strokeRect(-8, -30, 16, 20);
        c.fillStyle(V("#8a5a30")); c.fillRect(-6, -38, 12, 9);
        Dab(c, -10, 2, 6, "rgba(255,255,255,0.55)", -0.7f); break;
    }
    case I_ERBE: for (int i = 0; i < 9; i++) { float a = -2.4f + i * 0.2f; Linea(c, 0, 30, std::cos(a) * 34, 30 + std::sin(a) * 50, sh(col, r.F(-0.2f, 0.3f)), 5); Dab(c, std::cos(a) * 34, 30 + std::sin(a) * 50, 7, col, a, 0.45f); } break;
    case I_CANNE: for (int i = 0; i < 5; i++) { float x = -16 + i * 8.f; Linea(c, x, 34, x + 4, -30, "#1a1008", 6); Linea(c, x, 34, x + 4, -30, col, 3.5f); } Dab(c, 2, -30, 6, "#6a4a2a", 1.5f, 0.4f); break;
    case I_CONCHIGLIA: c.fillStyle(lg(0, -30, 0, 26, { "#fff4ec", col, "#c08070" })); c.beginPath(); c.moveTo(-32, 22); c.lineTo(0, -30); c.lineTo(32, 22); c.quadraticCurveTo(0, 34, -32, 22); c.fill(); Contorno();
        for (int k = -2; k <= 2; k++) Linea(c, 0, -28, k * 13.f, 24, rgba("#8a5040", 0.5f), 2);
        break;
    case I_CRISTALLO: c.glow(10, col); c.fillStyle(lg(-20, 0, 20, 0, { "#ffffff", col, sc })); Via(c, { { 0, -38 }, { 18, -10 }, { 10, 34 }, { -10, 34 }, { -18, -10 } }); c.fill(); c.noGlow(); Contorno(); Linea(c, 0, -38, 0, 34, rgba("#ffffff", 0.6f), 2); break;
    case I_PELLE: c.fillStyle(lg(0, -30, 0, 30, { ch, col, sc })); Via(c, { { -30, -24 }, { -14, -30 }, { 0, -24 }, { 14, -30 }, { 30, -24 }, { 26, 0 }, { 32, 24 }, { 10, 30 }, { -10, 30 }, { -32, 24 }, { -26, 0 } }); c.fill(); Contorno(); break;
    case I_MIELE: c.fillStyle(lg(-24, 0, 24, 0, { "#c88a40", "#8a5a30" })); c.beginPath(); c.ellipse(0, 8, 26, 26, 0, 0, TAU); c.fill(); Contorno();
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(0, -14, 20, 7, 0, 0, TAU); c.fill(); Dab(c, 6, -6, 6, col, 1.4f, 0.5f); c.fillStyle(V("#f0e6c8")); c.fillRect(-14, 2, 28, 12); break;
    case I_SALE: case I_SABBIA: c.fillStyle(lg(0, -26, 0, 30, { "#d8c8a0", "#a08860" })); Via(c, { { -26, 30 }, { -30, -8 }, { -16, -26 }, { 16, -26 }, { 30, -8 }, { 26, 30 } }); c.fill(); Contorno();
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(0, -18, 16, 6, 0, 0, TAU); c.fill(); Linea(c, -16, -26, 16, -26, "#6a4a2a", 3); break;
    case I_TAVOLA: for (int k = 0; k < 3; k++) { c.save(); c.translate(0, 14 - k * 12.f); c.rotate(-0.2f); c.fillStyle(lg(0, -6, 0, 6, { ch, col })); c.beginPath(); c.rect(-34, -6, 68, 12); c.fill(); Contorno(); Linea(c, -28, 0, 28, 0, rgba("#6a4626", 0.5f), 1); c.restore(); } break;
    case I_CORDA: for (int k = 0; k < 4; k++) { c.strokeStyle(V("#1a1008")); c.lineWidth(9); c.beginPath(); c.ellipse(0, 0, 30 - k * 6.f, 22 - k * 4.f, 0, 0, TAU); c.stroke(); c.strokeStyle(V(col)); c.lineWidth(6); c.stroke(); } break;
    case I_LINGOTTO: c.fillStyle(lg(0, -14, 0, 20, { ch, col, sc })); Via(c, { { -34, 18 }, { -22, -12 }, { 22, -12 }, { 34, 18 } }); c.fill(); Contorno(); Pieno(c, { { -20, -10 }, { 20, -10 }, { 14, -4 }, { -14, -4 } }, "rgba(255,255,255,0.4)"); break;
    case I_PANE: c.fillStyle(lg(0, -20, 0, 24, { "#f0c070", col, "#8a5020" })); c.beginPath(); c.ellipse(0, 4, 36, 22, 0, 0, TAU); c.fill(); Contorno();
        for (int k = -1; k <= 1; k++) Linea(c, k * 14.f - 6, -6, k * 14.f + 6, 6, "#7a4a1a", 3);
        break;
    case I_ZUPPA: c.fillStyle(lg(0, 0, 0, 30, { "#a07a5a", "#5a3e2a" })); c.beginPath(); c.ellipse(0, 6, 36, 26, 0, 0, PI_); c.fill(); Contorno();
        c.fillStyle(V(col)); c.beginPath(); c.ellipse(0, 6, 34, 10, 0, 0, TAU); c.fill(); Contorno();
        for (int k = 0; k < 3; k++) { c.strokeStyle(V("rgba(255,255,255,0.5)")); c.lineWidth(2.5f); c.beginPath(); c.moveTo(-12 + k * 12.f, -4); c.quadraticCurveTo(-6 + k * 12.f, -18, -12 + k * 12.f, -30); c.stroke(); } break;
    case I_TORCIA: Manico(-6, 34, 6, -6); c.glow(16, "#ffa030"); Pieno(c, { { -8, -4 }, { 14, -8 }, { 10, -24 }, { 4, -40 }, { -2, -26 }, { -12, -18 } }, "#ffb040"); Pieno(c, { { -2, -6 }, { 8, -8 }, { 4, -24 } }, "#fff0a0"); c.noGlow(); break;
    case I_MATTONI: for (int k = 0; k < 3; k++) for (int q = 0; q < 2 - (k == 2); q++) { float x = -30 + q * 32.f + (k % 2) * 16, y = 14 - k * 16.f; c.fillStyle(lg(0, y, 0, y + 14, { ch, col })); c.beginPath(); c.rect(x, y, 30, 14); c.fill(); Contorno(); } break;
    case I_VETRO: c.glow(6, "#ffffff"); c.fillStyle(lg(-26, -26, 26, 26, { "rgba(255,255,255,0.9)", rgba(col, 0.7f), rgba(sc, 0.8f) })); Via(c, { { -28, -24 }, { 24, -30 }, { 30, 24 }, { -24, 30 } }); c.fill(); c.noGlow(); Contorno();
        Linea(c, -14, -14, 4, -18, "rgba(255,255,255,0.9)", 3); break;
    case I_ASCIA: Manico(-24, 34, 18, -30); c.fillStyle(lg(0, -30, 0, 0, { "#e8eef4", col })); Via(c, { { 8, -30 }, { 34, -38 }, { 38, -6 }, { 18, -12 } }); c.fill(); Contorno(); break;
    case I_PICCONE: Manico(-24, 34, 14, -20); c.strokeStyle(V("#1a1008")); c.lineWidth(10); c.beginPath(); c.moveTo(-14, -26); c.quadraticCurveTo(14, -36, 38, -8); c.stroke();
        c.strokeStyle(V(col)); c.lineWidth(6); c.stroke(); break;
    case I_FALCE: Manico(-20, 34, 4, -30); c.strokeStyle(V("#1a1008")); c.lineWidth(9); c.beginPath(); c.moveTo(4, -30); c.quadraticCurveTo(34, -30, 36, 4); c.stroke(); c.strokeStyle(V(col)); c.lineWidth(5); c.stroke(); break;
    case I_CANNA: Manico(-30, 34, 26, -34); Linea(c, 26, -34, 30, 20, "#e0e0e0", 1.5f); c.fillStyle(V("#d03030")); c.beginPath(); c.arc(30, 22, 4, 0, TAU); c.fill(); break;
    case I_SECCHIO: c.fillStyle(lg(-24, 0, 24, 0, { ch, col, sc })); Via(c, { { -28, -14 }, { 28, -14 }, { 20, 30 }, { -20, 30 } }); c.fill(); Contorno();
        Linea(c, -26, -2, 26, -2, "#3a3a3a", 3); Linea(c, -24, 18, 24, 18, "#3a3a3a", 3);
        c.strokeStyle(V("#3a3a3a")); c.lineWidth(3); c.beginPath(); c.arc(0, -14, 26, PI_, TAU); c.stroke(); break;
    case I_LANTERNA: c.fillStyle(V("#3a3a3a")); c.fillRect(-20, -26, 40, 6); c.fillRect(-20, 24, 40, 6); c.glow(18, "#ffc040"); c.fillStyle(V("#ffd870")); c.fillRect(-16, -20, 32, 44); c.noGlow();
        for (int k = -1; k <= 1; k++) Linea(c, k * 16.f, -20, k * 16.f, 24, "#3a3a3a", 3);
        c.strokeStyle(V("#3a3a3a")); c.lineWidth(3); c.beginPath(); c.arc(0, -26, 12, PI_, TAU); c.stroke(); break;
    case I_PALA: Manico(-26, 34, 12, -14); c.fillStyle(lg(0, -40, 0, -8, { "#e8eef4", col })); Via(c, { { 6, -14 }, { 18, -42 }, { 38, -34 }, { 30, -6 } }); c.fill(); Contorno(); break;
    case I_BASTONE: Manico(-26, 36, 26, -36); break;
    case I_SPADA: {
        c.rotate(-0.78f);
        c.glow(col == "#7adcff" || col == "#f0d060" ? 12.f : 0.f, col);
        c.fillStyle(lg(-6, 0, 6, 0, { "#ffffff", col, sc })); Via(c, { { -6, 14 }, { -5, -38 }, { 0, -46 }, { 5, -38 }, { 6, 14 } }); c.fill(); c.noGlow(); Contorno();
        c.fillStyle(V("#c8a040")); c.beginPath(); c.rect(-18, 14, 36, 7); c.fill(); Contorno();
        c.fillStyle(V("#5a3a1e")); c.beginPath(); c.rect(-4, 21, 8, 16); c.fill(); Contorno();
        c.fillStyle(V("#c8a040")); c.beginPath(); c.arc(0, 40, 5, 0, TAU); c.fill(); break;
    }
    case I_LANCIA: Manico(-30, 36, 18, -18); c.fillStyle(lg(0, -40, 0, -10, { "#ffffff", col })); Via(c, { { 14, -12 }, { 34, -40 }, { 24, -6 } }); c.fill(); Contorno(); break;
    case I_ARCO: c.strokeStyle(V("#1a1008")); c.lineWidth(9); c.beginPath(); c.arc(-14, 0, 38, -1.2f, 1.2f); c.stroke(); c.strokeStyle(V(col)); c.lineWidth(5); c.stroke();
        Linea(c, -14 + 38 * std::cos(-1.2f), 38 * std::sin(-1.2f), -14 + 38 * std::cos(1.2f), 38 * std::sin(1.2f), "#e8e0c8", 1.5f);
        Linea(c, -30, 0, 34, 0, "#8a5a30", 3); Pieno(c, { { 34, -5 }, { 44, 0 }, { 34, 5 } }, "#c8ccd4"); break;
    case I_GIUBBA: c.fillStyle(lg(-30, 0, 30, 0, { ch, col, sc })); Via(c, { { -16, -32 }, { 16, -32 }, { 36, -18 }, { 30, 0 }, { 22, -4 }, { 22, 32 }, { -22, 32 }, { -22, -4 }, { -30, 0 }, { -36, -18 } }); c.fill(); Contorno();
        Linea(c, 0, -30, 0, 32, rgba("#1a1008", 0.5f), 2); Linea(c, -22, 10, 22, 10, "#3a2616", 4); break;
    case I_SCUDO: c.fillStyle(lg(-30, 0, 30, 0, { ch, col, sc })); c.beginPath(); c.moveTo(-30, -30); c.lineTo(30, -30); c.lineTo(28, 6); c.quadraticCurveTo(20, 28, 0, 38); c.quadraticCurveTo(-20, 28, -28, 6); c.closePath(); c.fill(); Contorno();
        Linea(c, 0, -28, 0, 34, "#5a3a1e", 4); Linea(c, -28, -4, 28, -4, "#5a3a1e", 4); c.fillStyle(V("#c8a040")); c.beginPath(); c.arc(0, -4, 7, 0, TAU); c.fill(); break;
    case I_ARMATURA: c.fillStyle(lg(-30, 0, 30, 0, { "#ffffff", col, sc })); Via(c, { { -18, -32 }, { 18, -32 }, { 34, -22 }, { 28, 4 }, { 22, 2 }, { 20, 32 }, { -20, 32 }, { -22, 2 }, { -28, 4 }, { -34, -22 } }); c.fill(); Contorno();
        Linea(c, -20, -6, 20, -6, rgba("#1a1008", 0.6f), 2); Linea(c, -20, 12, 20, 12, rgba("#1a1008", 0.6f), 2); Linea(c, 0, -30, 0, -6, rgba("#1a1008", 0.6f), 2); break;
    case I_ZAINO: c.fillStyle(lg(-28, 0, 28, 0, { ch, col, sc })); c.beginPath(); c.moveTo(-26, 34); c.lineTo(-30, -14); c.quadraticCurveTo(0, -40, 30, -14); c.lineTo(26, 34); c.closePath(); c.fill(); Contorno();
        c.fillStyle(V(sh(col, -0.2f))); c.beginPath(); c.moveTo(-30, -14); c.quadraticCurveTo(0, -40, 30, -14); c.lineTo(24, 4); c.lineTo(-24, 4); c.closePath(); c.fill(); Contorno();
        c.fillStyle(V("#c8a040")); c.fillRect(-5, 0, 10, 10); break;
    case I_LETTERA: c.fillStyle(V(col)); c.beginPath(); c.rect(-34, -22, 68, 46); c.fill(); Contorno(); Linea(c, -34, -22, 0, 4, "#8a6a40", 2); Linea(c, 34, -22, 0, 4, "#8a6a40", 2);
        c.fillStyle(V("#c02a2a")); c.beginPath(); c.arc(0, 6, 7, 0, TAU); c.fill(); break;
    case I_CHIAVE: c.rotate(-0.6f); c.strokeStyle(V("#1a1008")); c.lineWidth(10); c.beginPath(); c.arc(0, -20, 14, 0, TAU); c.stroke(); c.strokeStyle(V(col)); c.lineWidth(6); c.stroke();
        Linea(c, 0, -6, 0, 36, "#1a1008", 10); Linea(c, 0, -6, 0, 36, col, 6); Linea(c, 0, 24, 12, 24, col, 6); Linea(c, 0, 34, 10, 34, col, 6); break;
    case I_GHIANDA: c.glow(16, "#ffd040"); c.fillStyle(rgs(-8, -4, 2, 34, { { 0, "#fff0a0" }, { 0.5f, col }, { 1, "#a06010" } })); c.beginPath(); c.ellipse(0, 10, 22, 26, 0, 0, TAU); c.fill(); c.noGlow(); Contorno();
        c.fillStyle(lg(0, -26, 0, -4, { "#a07040", "#6a4626" })); c.beginPath(); c.ellipse(0, -10, 26, 14, 0, PI_, TAU); c.closePath(); c.fill(); Contorno();
        for (int k = -2; k <= 2; k++) Linea(c, k * 9.f - 4, -18, k * 9.f + 4, -10, "#4a3020", 1.5f);
        Linea(c, 0, -22, 4, -34, "#5a3a1e", 4); break;
    case I_AMULETO: c.strokeStyle(V("#c8a040")); c.lineWidth(3); c.beginPath(); c.arc(0, -10, 26, PI_ * 1.1f, TAU - 0.3f); c.stroke(); c.glow(12, col); Tondo(0, 14, 18, ch, col); c.noGlow(); break;
    case I_FORMAGGIO: c.fillStyle(lg(0, -20, 0, 26, { "#fff0a0", col, "#c89830" })); Via(c, { { -34, 24 }, { -34, -4 }, { 30, -24 }, { 34, 6 }, { 34, 24 } }); c.fill(); Contorno();
        for (int i = 0; i < 4; i++) { c.fillStyle(V("#c89830")); c.beginPath(); c.arc(r.F(-24, 24), r.F(0, 18), r.F(3, 6), 0, TAU); c.fill(); } break;
    default: Tondo(0, 0, 26, ch, col); break;
    }
}

// --- Le scene (sfondo 1920x1080) ---------------------------------------------------------------
void HslDi(Color col, float& h, float& s, float& l) {
    float r = col.r / 255.f, g = col.g / 255.f, b = col.b / 255.f, mx = std::max({ r, g, b }), mn = std::min({ r, g, b });
    l = (mx + mn) / 2; h = s = 0;
    if (mx != mn) {
        float d = mx - mn; s = l > 0.5f ? d / (2 - mx - mn) : d / (mx + mn);
        if (mx == r) h = (g - b) / d + (g < b ? 6 : 0); else if (mx == g) h = (b - r) / d + 2; else h = (r - g) / d + 4;
        h *= 60;
    }
    s *= 100; l *= 100;
}
struct InfoScena {
    int scena = 0; Color erba{}, terreno{}; unsigned seme = 1;
    bool uscite[4] = { false, false, false, false };
    float canale = -1;                   // y del canale d'acqua (sotto un ponte), -1 = niente
    std::vector<Vector2> liberi;         // dove stanno gli elementi: niente decorazioni li'
};
const Vector2 FRECCIA[4] = { { 960, 64 }, { 1866, 560 }, { 1700, 1012 }, { 54, 560 } };
const Vector2 ARRIVO[4] = { { 960, 300 }, { 1730, 600 }, { 1640, 900 }, { 190, 620 } };

void DipingiScena(K& c, const InfoScena& in) {
    R2 r(in.seme * 7u + 3u);
    const int W = 1920, H = 1080;
    int sc = in.scena;
    float eh, es, el, th, ts, tl;
    HslDi(in.erba, eh, es, el); HslDi(in.terreno, th, ts, tl);
    std::string erba = Esa(in.erba), terra = Esa(in.terreno);
    bool verde = sc == 0 || sc == 1 || sc == 2 || sc == 3 || sc == 5 || sc == 8 || sc == 9;
    auto Lontano = [&](float x, float y, float d) { for (auto& p : in.liberi) if (std::fabs(p.x - x) < d && std::fabs(p.y - y) < d * 0.7f) return false; return true; };
    // 1. fondo
    c.fillStyle(V(erba)); c.fillRect(0, 0, W, H);
    if (verde) {
        for (int i = 0; i < 22000; i++) {
            float x = r.F() * W, y = r.F() * H, n = std::sin(x / 140) + std::cos(y / 110) + std::sin((x + y) / 70) * 0.5f;
            Linea(c, x, y, x + r.F(-3, 3), y - r.F(4, 10), Hsl(eh + n * 6 + r.F(-6, 6), es + r.F(-8, 8), el - 8 + n * 4 + r.F(0, 14), 0.75f), r.F(1, 2.2f));
        }
    } else {
        for (int i = 0; i < 9000; i++) {
            float x = r.F() * W, y = r.F() * H, n = std::sin(x / 160) + std::cos(y / 120);
            Dab(c, x, y, r.F(2, 7), Hsl(eh + r.F(-6, 6), es + r.F(-6, 6), el + n * 3 + r.F(-10, 8), 0.5f), r.F(-0.3f, 0.3f), 0.45f);
        }
        if (sc == 10 || sc == 7)   // increspature della sabbia
            for (int i = 0; i < 260; i++) { float x = r.F() * W, y = r.F() * H; c.strokeStyle(V(Hsl(eh, es, el - 14, 0.25f))); c.lineWidth(1.6f); c.beginPath(); c.ellipse(x, y, r.F(30, 90), r.F(4, 9), -0.15f, PI_ * 1.1f, PI_ * 1.9f); c.stroke(); }
    }
    // 2. sentieri verso le uscite e piazza centrale
    auto Sentiero = [&](Vector2 a, Vector2 b, float w) {
        Vector2 m{ (a.x + b.x) / 2 + r.F(-120, 120), (a.y + b.y) / 2 + r.F(-60, 60) };
        for (int i = 0; i <= 120; i++) {
            float t = i / 120.f, u = 1 - t;
            float x = u * u * a.x + 2 * u * t * m.x + t * t * b.x, y = u * u * a.y + 2 * u * t * m.y + t * t * b.y;
            for (int k = 0; k < 10; k++) Dab(c, x + r.F(-w, w), y + r.F(-w, w) * 0.5f, r.F(8, 18), Hsl(th + r.F(-4, 4), ts + r.F(-6, 6), tl + r.F(-8, 6), 0.55f), r.F(0, 3), 0.55f);
        }
    };
    Vector2 centro{ 960, 640 };
    if (sc == 0 || sc == 9) {
        std::vector<Vector2> piazza = { { 260, 540 }, { 760, 270 }, { 1240, 270 }, { 1640, 440 }, { 1560, 720 }, { 1000, 950 }, { 420, 900 }, { 180, 720 } };
        Pieno(c, piazza, terra);
        c.save(); Via(c, piazza); c.clip();
        if (sc == 0) for (int i = 0; i < 9000; i++) { float x = r.F(150, 1700), y = r.F(250, 980); Dab(c, x, y, r.F(3, 8), Hsl(th + r.F(-6, 6), ts + r.F(-6, 6), tl + r.F(-12, 10), 0.9f), 0, 0.55f); }
        else for (int row = 0; row < 40; row++) for (int k = 0; k < 40; k++) {
            float x = 150 + k * 48 + (row % 2) * 24, y = 250 + row * 20;
            c.fillStyle(V(Hsl(th + r.F(-4, 4), ts, tl + r.F(-10, 8)))); c.beginPath(); c.rect(x, y, 44, 17); c.fill();
        }
        for (int i = 0; i < 40; i++) Dab(c, r.F(200, 1600), r.F(260, 940), r.F(40, 140), "rgba(60,45,30,0.14)", 0, 0.3f);
        c.restore();
    }
    for (int d = 0; d < 4; d++) if (in.uscite[d]) Sentiero(centro, FRECCIA[d], d % 2 ? 50 : 60);
    // 3. elementi tipici del paesaggio
    if (sc == 2) {   // campi arati ai lati
        for (int f = 0; f < 3; f++) {
            float x0 = f == 0 ? 120 : f == 1 ? 1500 : 220, y0 = f == 0 ? 330 : f == 1 ? 760 : 860;
            if (!Lontano(x0 + 120, y0, 160)) continue;
            Pieno(c, { { x0, y0 }, { x0 + 160, y0 - 80 }, { x0 + 320, y0 }, { x0 + 160, y0 + 80 } }, "#6a5030");
            for (int k = 0; k < 9; k++) Linea(c, x0 + 18 * k, y0 - 9 * k + 0, x0 + 160 + 18 * k, y0 + 80 - 9 * k, "rgba(60,40,20,0.45)", 3);
            for (int i = 0; i < 300; i++) { float u = r.F(), v = r.F(); Dab(c, x0 + 160 * u + 160 * v, y0 - 80 * u + 80 * v, 3, Hsl(r.F(70, 100), 45, r.F(28, 42)), 0); }
        }
    }
    if (sc == 5) {   // pozze della palude
        for (int i = 0; i < 14; i++) {
            float x = r.F(100, 1820), y = r.F(160, 940);
            if (!Lontano(x, y, 170)) continue;
            float rx = r.F(70, 160);
            c.fillStyle(rgs(x, y, 4, rx, { { 0, "#2a4a4a" }, { 0.8f, "#1e3a36" }, { 1, "rgba(30,58,54,0)" } })); c.beginPath(); c.ellipse(x, y, rx, rx * 0.4f, 0, 0, TAU); c.fill();
            for (int k = 0; k < 4; k++) Dab(c, x + r.F(-rx * 0.6f, rx * 0.6f), y + r.F(-10, 10), r.F(6, 12), "#4a7a3a", 0, 0.5f);
            for (int k = 0; k < 18; k++) { float px = x + r.F(-rx, rx), py = y + r.F(-rx * 0.4f, rx * 0.4f); Linea(c, px, py, px + r.F(-4, 4), py - r.F(20, 50), Hsl(r.F(60, 80), 40, r.F(30, 45)), 2); }
        }
    }
    if (sc == 7) {   // il mare in alto
        c.fillStyle(lg(0, 0, 0, 260, { "#2a5a8a", "#3a8ab0", "#5ab8c8" }));
        c.beginPath(); c.moveTo(0, 0); c.lineTo(W, 0); c.lineTo(W, 190);
        for (int x = W; x >= 0; x -= 40) c.lineTo((float)x, 210 + std::sin(x * 0.01f) * 22 + std::sin(x * 0.037f) * 8);
        c.closePath(); c.fill();
        for (int i = 0; i < 400; i++) { float x = r.F() * W, y = r.F(10, 200); Linea(c, x, y, x + r.F(10, 30), y, rgba("#ffffff", r.F(0.1f, 0.35f)), 1.5f); }
        for (int x = 0; x < W; x += 12) Dab(c, (float)x, 212 + std::sin(x * 0.01f) * 22 + std::sin(x * 0.037f) * 8, r.F(6, 12), "rgba(255,255,255,0.6)", 0, 0.4f);
    }
    if (sc == 4 || sc == 6) {   // parete di roccia o montagne in alto
        if (sc == 6) {
            for (int m = 0; m < 6; m++) {
                float x = m * 380 - 60 + r.F(-40, 40), h = r.F(160, 230);
                Pieno(c, { { x - 260, 230 }, { x, 230 - h }, { x + 260, 230 } }, Hsl(215, 18, r.F(42, 55)));
                Pieno(c, { { x - 70, 230 - h * 0.72f }, { x, 230 - h }, { x + 80, 230 - h * 0.7f }, { x + 30, 230 - h * 0.6f }, { x - 20, 230 - h * 0.66f } }, "#f4f8ff");
            }
            c.fillStyle(lg(0, 160, 0, 260, { "rgba(232,238,244,0)", erba })); c.fillRect(0, 160, W, 100);
        } else {
            c.fillStyle(lg(0, 0, 0, 250, { "#4a3a2e", "#7a6450", "#8a7058" }));
            c.beginPath(); c.moveTo(0, 0); c.lineTo(W, 0); c.lineTo(W, 200);
            for (int x = W; x >= 0; x -= 60) c.lineTo((float)x, 210 + r.F(-30, 30));
            c.closePath(); c.fill();
            for (int i = 0; i < 300; i++) Dab(c, r.F(0, W), r.F(0, 210), r.F(6, 20), Hsl(25, 15, r.F(22, 44), 0.6f), r.F(0, 3), 0.5f);
            if (in.uscite[0]) {   // ingresso della miniera
                c.fillStyle(V("#140c08")); c.beginPath(); c.ellipse(960, 210, 90, 120, 0, PI_, TAU); c.fill(); c.fillRect(870, 150, 180, 60);
                Linea(c, 870, 210, 870, 100, "#6a4a2a", 14); Linea(c, 1050, 210, 1050, 100, "#6a4a2a", 14); Linea(c, 856, 100, 1064, 100, "#7a5430", 16);
            }
        }
    }
    if (sc == 11) {   // pareti della grotta tutt'intorno
        for (int i = 0; i < 900; i++) {
            float a = r.F() * TAU, rr = r.F(0.86f, 1.25f);
            float x = 960 + std::cos(a) * 1080 * rr, y = 560 + std::sin(a) * 620 * rr;
            Dab(c, x, y, r.F(30, 80), Hsl(280, 8, r.F(10, 22), 0.95f), r.F(0, 3), 0.6f);
        }
        for (int i = 0; i < 30; i++) {
            float a = r.F() * TAU, x = 960 + std::cos(a) * 980, y = 560 + std::sin(a) * 560;
            c.glow(14, "#7adcff"); Pieno(c, { { x - 8, y }, { x, y - r.F(20, 50) }, { x + 8, y } }, i % 3 ? "#7adcff" : "#c890ff"); c.noGlow();
        }
    }
    if (sc == 9) {   // mura del castello in alto
        c.fillStyle(lg(0, 0, 0, 230, { "#8a8478", "#a8a090", "#6a645a" })); c.fillRect(0, 0, W, 210);
        for (int row = 0; row < 12; row++) for (int k = 0; k < 42; k++) { c.strokeStyle(V("rgba(40,36,30,0.35)")); c.lineWidth(1); c.strokeRect(k * 48.f + (row % 2) * 24 - 24, row * 18.f, 48, 18); }
        for (int k = 0; k < 40; k++) c.fillStyle(V("#6a645a")), c.fillRect(k * 50.f, 0, 26, 24);
        for (int t = 0; t < 4; t++) { float x = 120 + t * 560; c.fillStyle(lg(x - 70, 0, x + 70, 0, { "#c8c0b0", "#8a8478", "#5a544a" })); c.fillRect(x - 70, 0, 140, 250); Pieno(c, { { x - 80, 0 }, { x + 80, 0 }, { x, -60 } }, "#6a3a3a"); }
        if (in.uscite[0]) { c.fillStyle(V("#1a140e")); c.beginPath(); c.ellipse(960, 210, 80, 110, 0, PI_, TAU); c.fill(); c.fillRect(880, 150, 160, 62); }
    }
    if (sc == 3 || in.canale >= 0) {   // fiume o canale
        float y0 = in.canale >= 0 ? in.canale - 40 : 200;
        c.fillStyle(lg(0, y0 - 50, 0, y0 + 50, { "#2a6a8a", "#4a9ab8", "#2a6a8a" }));
        c.beginPath(); c.moveTo(0, y0 - 40);
        for (int x = 0; x <= W; x += 40) c.lineTo((float)x, y0 - 44 + std::sin(x * 0.006f) * 14);
        for (int x = W; x >= 0; x -= 40) c.lineTo((float)x, y0 + 44 + std::sin(x * 0.006f + 1) * 14);
        c.closePath(); c.fill();
        for (int i = 0; i < 260; i++) { float x = r.F() * W, y = y0 + r.F(-34, 34); Linea(c, x, y, x + r.F(10, 26), y, rgba("#ffffff", r.F(0.1f, 0.35f)), 1.5f); }
        for (int x = 0; x < W; x += 14) { Dab(c, (float)x, y0 - 44 + std::sin(x * 0.006f) * 14, r.F(5, 10), Hsl(eh, es, el - 10, 0.9f), 0, 0.5f); Dab(c, (float)x, y0 + 44 + std::sin(x * 0.006f + 1) * 14, r.F(5, 10), Hsl(th, ts, tl - 10, 0.9f), 0, 0.5f); }
    }
    if (sc == 10) {   // dune
        for (int i = 0; i < 7; i++) {
            float x = r.F(0, W), y = r.F(150, 950);
            if (!Lontano(x, y, 200)) continue;
            c.fillStyle(rgs(x - 60, y - 40, 10, 220, { { 0, Hsl(eh, es, el + 8, 0.7f) }, { 1, Hsl(eh, es, el, 0) } })); c.beginPath(); c.ellipse(x, y, 220, 70, 0, 0, TAU); c.fill();
            c.fillStyle(V(Hsl(eh, es, el - 12, 0.35f))); c.beginPath(); c.ellipse(x + 60, y + 20, 160, 40, 0, 0, PI_); c.fill();
        }
    }
    if (sc == 8) {   // lastre di pietra antica
        for (int i = 0; i < 60; i++) {
            float x = r.F(150, 1770), y = r.F(260, 920);
            if (!Lontano(x, y, 90)) continue;
            Pieno(c, { { x - 40, y }, { x, y - 20 }, { x + 40, y }, { x, y + 20 } }, Hsl(40, 6, r.F(48, 62), 0.85f));
            Dab(c, x + r.F(-20, 20), y, r.F(6, 12), Hsl(100, 30, r.F(25, 35), 0.6f), 0, 0.5f);
        }
    }
    // 4. bordo: alberi, rocce o cristalli (lasciando liberi i passaggi delle uscite)
    auto Bordo = [&](float x, float y, float s) {
        c.save(); c.translate(x, y);
        R2 rr((unsigned)(x * 13 + y * 7));
        if (sc == 1 || sc == 5) { if (rr.F() < 0.5f) Abete(c, rr, s, false); else Latifoglia(c, rr, s, sc == 5 ? 70.f : 88 + rr.F(-12, 10), false); }
        else if (sc == 6) Abete(c, rr, s, true);
        else if (sc == 0 || sc == 2 || sc == 3 || sc == 8 || sc == 9) { if (rr.F() < 0.3f) Abete(c, rr, s, false); else if (rr.F() < 0.8f) Latifoglia(c, rr, s * 0.95f, 88 + rr.F(-14, 12), false); else Cespuglio(c, rr, s * 1.5f, 85, 0); }
        else if (sc == 7 || sc == 10) { if (rr.F() < 0.5f) { c.save(); c.scale(s * 0.9f, s * 0.9f); DipingiForma(c, sc == 7 ? F_PALMA : F_CACTUS, "", (unsigned)(x + y), 0); c.restore(); } else Roccia(c, rr, s * 0.9f, sc == 10 ? "#b8885a" : "#9a9488", "", 0); }
        else if (sc == 4) Roccia(c, rr, s, "#8a7a68", "", 0);
        else if (sc == 11) Cristalli(c, rr, s * 0.8f, rr.F() < 0.5f ? "#7adcff" : "#c890ff");
        c.restore();
    };
    bool cimeAlte = sc == 7 || sc == 4 || sc == 9 || sc == 11 || sc == 6;
    if (!cimeAlte && in.canale < 0 && sc != 3)
        for (float x = 40; x < W; x += r.F(130, 190)) { if (in.uscite[0] && x > 800 && x < 1120) continue; Bordo(x + r.F(-20, 20), r.F(120, 210), r.F(0.8f, 1.15f)); }
    for (float y = 300; y < 1000; y += r.F(150, 210)) {
        if (!(in.uscite[3] && y > 430 && y < 720) && y < 950) Bordo(r.F(20, 90), y, r.F(0.75f, 1.05f));
        if (!(in.uscite[1] && y > 430 && y < 720) && !(in.uscite[2] && y > 860)) Bordo(r.F(1830, 1900), y, r.F(0.75f, 1.05f));
    }
    // piccoli dettagli sparsi
    for (int i = 0; i < 40; i++) {
        float x = r.F(140, 1780), y = r.F(260, 960);
        if (!Lontano(x, y, 110)) continue;
        c.save(); c.translate(x, y);
        R2 rr((unsigned)i * 31u + in.seme);
        if (verde) { if (rr.F() < 0.5f) Cespuglio(c, rr, rr.F(0.6f, 1.0f), eh + rr.F(-10, 10), 0); else for (int k = 0; k < 14; k++) { c.fillStyle(V(k % 3 ? "#e8d040" : "#f0f0e0")); c.beginPath(); c.arc(rr.F(-30, 30), rr.F(-10, 10), 2.4f, 0, TAU); c.fill(); } }
        else if (sc == 6) Dab(c, 0, 0, rr.F(10, 20), "rgba(255,255,255,0.7)", 0, 0.4f);
        else Sasso(c, rr, 0, 0, rr.F(8, 18), rr.F(5, 10), sc == 10 ? "#b8885a" : sc == 11 ? "#4a4250" : "#9a9488");
        c.restore();
    }
    // 5. luce calda e vignettatura
    c.fillStyle(lgs(0, 0, W, H, { { 0, "rgba(255,225,160,0.16)" }, { 1, "rgba(40,30,80,0.14)" } })); c.fillRect(0, 0, W, H);
    c.fillStyle(rgs(W / 2.f, H / 2.f, 620, 1150, { { 0, "rgba(0,0,0,0)" }, { 1, sc == 11 ? "rgba(5,0,10,0.75)" : "rgba(10,8,0,0.5)" } })); c.fillRect(0, 0, W, H);
}

// ============================================================================================
// Texture condivise (si dipingono la prima volta che servono)
// ============================================================================================
struct Tex { Texture2D t{}; Spec s{}; };
std::map<std::string, Tex> gSprite;
std::map<std::string, Texture2D> gScene;
std::deque<std::string> gOrdineScene;
Texture2D gPale{}, gPergamena{};

const Tex& SpriteForma(int f, Color col, unsigned seme, int stato) {
    // le forme che non dipendono dal colore o dalla variante condividono la texture
    bool colore = f <= F_BAMBINO || f == F_TENDA || f == F_CRISTALLI || f == F_CASA_PAGLIA || f == F_CASA_TEGOLE;
    bool varia = f <= F_BAMBINO || f == F_ALBERO || f == F_ABETE || f == F_MELO || f == F_ROCCIA || f == F_FERRO || f == F_CARBONE || f == F_RAME || f == F_CESPUGLIO;
    std::string k = "f" + std::to_string(f) + (colore ? Esa(col) : "") + "|" + std::to_string(varia ? seme % 5 : 0) + "|" + std::to_string(stato);
    auto it = gSprite.find(k);
    if (it != gSprite.end()) return it->second;
    Tex t; t.s = SpecForma(f);
    std::string cs = Esa(col);
    t.t = Dipingi((int)(t.s.w * t.s.dens), (int)(t.s.h * t.s.dens), [&](K& c) { c.scale(t.s.dens, t.s.dens); c.translate(t.s.ax, t.s.ay); DipingiForma(c, f, cs, varia ? seme % 5 : 0, stato); });
    return gSprite[k] = t;
}
const Tex& SpriteNemico(int f, Color col) {
    std::string k = "n" + std::to_string(f) + Esa(col);
    auto it = gSprite.find(k);
    if (it != gSprite.end()) return it->second;
    Tex t; t.s = SpecNemico(f);
    std::string cs = Esa(col);
    t.t = Dipingi((int)(t.s.w * t.s.dens), (int)(t.s.h * t.s.dens), [&](K& c) { c.scale(t.s.dens, t.s.dens); c.translate(t.s.ax, t.s.ay); DipingiNemico(c, f, cs, 1); });
    return gSprite[k] = t;
}
const Tex& SpriteIcona(int ic, Color col) {
    std::string k = "i" + std::to_string(ic) + Esa(col);
    auto it = gSprite.find(k);
    if (it != gSprite.end()) return it->second;
    Tex t; t.s = { 96, 96, 48, 48, 1, 96, 96, 0 };
    std::string cs = Esa(col);
    t.t = Dipingi(96, 96, [&](K& c) { DipingiIcona(c, ic, cs); });
    return gSprite[k] = t;
}
const Tex& SpriteEroe(const Look& o, int passo, int braccio) {
    std::string k = "e" + o.veste + o.mantello + o.capelli + "|" + std::to_string(o.attrezzo) + o.colAttrezzo + "|" + std::to_string(passo) + "|" + std::to_string(braccio);
    auto it = gSprite.find(k);
    if (it != gSprite.end()) return it->second;
    Tex t; t.s = { 110, 130, 55, 118, 2, 46, 92, 0 };
    t.t = Dipingi((int)(t.s.w * t.s.dens), (int)(t.s.h * t.s.dens), [&](K& c) { c.scale(t.s.dens, t.s.dens); c.translate(t.s.ax, t.s.ay); Figura(c, o, passo, braccio); });
    return gSprite[k] = t;
}
// Disegna una texture con il piede in (x, y)
void Piede(const Tex& t, float x, float y, float scala = 1, bool flip = false, Color tinta = WHITE) {
    if (!t.t.id) return;
    float k = scala / t.s.dens;
    Rectangle src = { 0, 0, (float)(flip ? -t.t.width : t.t.width), (float)t.t.height };
    float ax = flip ? t.t.width - t.s.ax * t.s.dens : t.s.ax * t.s.dens;
    DrawTexturePro(t.t, src, { x - ax * k, y - t.s.ay * t.s.dens * k, t.t.width * k, t.t.height * k }, { 0, 0 }, 0, tinta);
}
void Icona(int ic, Color col, float x, float y, float dim) {
    const Tex& t = SpriteIcona(ic, col);
    DrawTexturePro(t.t, { 0, 0, 96, 96 }, { x - dim / 2, y - dim / 2, dim, dim }, { 0, 0 }, 0, WHITE);
}

// ============================================================================================
// IL GIOCO
// ============================================================================================
struct Oggetto { std::string nome; int tipo = 0, icona = 0; Color col{}; int valore = 0, prezzo = 0, negozio = 0; };
struct Ricetta { int ris = -1, q = 1, ing[3] = { -1, -1, -1 }, n[3] = { 0, 0, 0 }, cap = 1; };
struct Nemico { std::string nome; int forma = 0; Color col{}; int potenza = 1, oro = 0, lascia = -1, quantita = 0; bool grande = false; };
struct Capitolo { std::string nome, intro, musica; int partenza = 0; };
struct Luogo { std::string nome, musica; int capitolo = 1, scena = 0, ora = 0, seme = 0; Color erba{}, terreno{}; int uscite[4] = { -1, -1, -1, -1 }; };
struct Punto { std::string nome, frase; int luogo = 0, tipo = 0, forma = 0, oggetto = -1, quantita = 0, attrezzo = -1, costo = 0, usi = 0, nemico = -1, blocca = 0; float x = 0, y = 0, ricrescita = 0; Color colore{}; };
struct Missione { int capitolo = 1; std::string nome, testo, fine; int chi = -1, tipo = 0, oggetto = -1, quantita = 0, bersaglio = -1, richiede = -1, oro = 0, premio = -1, premioQ = 0; };
struct StatoPunto { int usati = 0; float attesa = 0; bool fatto = false; };

enum class Az { Niente, Cammina, Lavora, Combatte, Viaggia, Riposa };
enum class Pan { Niente, Dialogo, Mercato, Lavori, Zaino, Diario, Mappa, Capitolo, Fine };

class Querciaverde : public GiocoArcade {
public:
    const char* Nome() const override { return "querciaverde"; }
    std::string Titolo() const override { return "Il Regno di Querciaverde"; }
    bool SalvaQuandoVuoi() const override { return true; }
    std::string Tappa() const override {
        if (caps.empty()) return "Capitolo 1";
        return "Capitolo " + std::to_string(cap + 1) + " - " + (luogo >= 0 && luogo < (int)luoghi.size() ? luoghi[luogo].nome : caps[cap].nome);
    }
    std::vector<std::string> Aiuto() const override {
        return { "Clic sinistro: cammina, raccogli, parla, combatti, viaggia con le frecce dorate", "Levetta o frecce: muovono il puntatore; Spazio/A: clic; C/X: prossimo elemento",
                 "Clic destro, Z o B: chiude le finestre. Diario, mappa, lavori e zaino sono negli angoli",
                 "Le missioni '!' danno oro e oggetti: compi tutte quelle di un capitolo per andare avanti" };
    }
    Brano MusicaMenu() const override { return BranoDi(TestoDi(cfg["generale"], "musicaMenu", "Querciaverde - Tema")); }
    json Schema() const override { return SchemaQuerciaverde(); }
    json Predefiniti() const override { return DatiPredefiniti(); }
    std::vector<Brano> BraniPredefiniti() const override;
    int Tappe() const override { return std::max(1, cfg.contains("capitoli") && cfg["capitoli"].is_array() ? (int)cfg["capitoli"].size() : 1); }

    // --- dati letti dal progetto ---
    std::vector<Oggetto> ogg; std::vector<Ricetta> ric; std::vector<Nemico> nem; std::vector<Capitolo> caps;
    std::vector<Luogo> luoghi; std::vector<Punto> punti; std::vector<Missione> mis;
    std::vector<std::vector<int>> puntiDi;   // gli elementi di ogni localita'
    std::string firmaDati;
    int vitaMax = 10, energiaMax = 10, zainoBase = 20, forzaBase = 1, riposoAperto = 3;
    Look lookEroe;

    static int Rif(const json& j, const char* k, int n) { int v = IntDi(j, k, -1); return v >= 0 && v < n ? v : -1; }
    void Leggi() {
        std::string f = cfg.dump();
        if (f == firmaDati && !caps.empty()) return;
        firmaDati = f;
        ogg.clear(); ric.clear(); nem.clear(); caps.clear(); luoghi.clear(); punti.clear(); mis.clear();
        auto Arr = [&](const char* k) -> const json& { static const json vuoto = json::array(); return cfg.contains(k) && cfg[k].is_array() ? cfg[k] : vuoto; };
        for (auto& j : Arr("oggetti")) { Oggetto o; o.nome = TestoDi(j, "nome", "Oggetto"); o.tipo = std::max(0, std::min(7, IntDi(j, "tipo", 0))); o.icona = std::max(0, std::min(N_ICONE - 1, IntDi(j, "icona", 0)));
            o.col = ColoreDi(j, "colore", Hex(0x8a6a40)); o.valore = IntDi(j, "valore", 0); o.prezzo = std::max(0, IntDi(j, "prezzo", 1)); o.negozio = IntDi(j, "negozio", 0); ogg.push_back(o); }
        int NO = (int)ogg.size();
        for (auto& j : Arr("ricette")) { Ricetta r; r.ris = Rif(j, "risultato", NO); r.q = std::max(1, IntDi(j, "quantita", 1)); r.cap = IntDi(j, "capitolo", 1);
            const char* ki[3] = { "ing1", "ing2", "ing3" }; const char* kn[3] = { "n1", "n2", "n3" };
            for (int i = 0; i < 3; i++) { r.ing[i] = Rif(j, ki[i], NO); r.n[i] = r.ing[i] >= 0 ? std::max(1, IntDi(j, kn[i], 1)) : 0; }
            if (r.ris >= 0 && r.ing[0] >= 0) ric.push_back(r); }
        for (auto& j : Arr("nemici")) { Nemico n; n.nome = TestoDi(j, "nome", "Nemico"); n.forma = std::max(0, std::min(N_FORME_NEMICO - 1, IntDi(j, "forma", 0))); n.col = ColoreDi(j, "colore", Hex(0x7a7a80));
            n.potenza = std::max(1, IntDi(j, "potenza", 1)); n.oro = IntDi(j, "oro", 0); n.lascia = Rif(j, "lascia", NO); n.quantita = IntDi(j, "quantita", 0); n.grande = j.value("grande", false); nem.push_back(n); }
        for (auto& j : Arr("luoghi")) { Luogo l; l.nome = TestoDi(j, "nome", "Localita'"); l.capitolo = std::max(1, IntDi(j, "capitolo", 1)); l.scena = std::max(0, std::min(11, IntDi(j, "scena", 0)));
            l.ora = std::max(0, std::min(4, IntDi(j, "ora", 0))); l.seme = IntDi(j, "seme", 0); l.erba = ColoreDi(j, "erba", Hex(0x5a6a2e)); l.terreno = ColoreDi(j, "terreno", Hex(0x7a6a52));
            l.musica = TestoDi(j, "musica", "Querciaverde - Borgo"); luoghi.push_back(l); }
        int NL = (int)luoghi.size();
        if (NL == 0) { Luogo l; l.nome = "Radura"; l.erba = Hex(0x5a6a2e); l.terreno = Hex(0x7a6a52); luoghi.push_back(l); NL = 1; }
        { int i = 0; const char* ku[4] = { "nord", "est", "sud", "ovest" };
          for (auto& j : Arr("luoghi")) { for (int d = 0; d < 4; d++) luoghi[i].uscite[d] = Rif(j, ku[d], NL); i++; } }
        for (auto& j : Arr("capitoli")) { Capitolo c; c.nome = TestoDi(j, "nome", "Capitolo"); c.intro = TestoDi(j, "intro", ""); c.partenza = std::max(0, Rif(j, "partenza", NL)); c.musica = TestoDi(j, "musica", ""); caps.push_back(c); }
        if (caps.empty()) { Capitolo c; c.nome = "Capitolo"; caps.push_back(c); }
        for (auto& j : Arr("punti")) { Punto p; p.nome = TestoDi(j, "nome", ""); p.frase = TestoDi(j, "frase", ""); p.luogo = std::max(0, Rif(j, "luogo", NL));
            p.tipo = std::max(0, std::min(7, IntDi(j, "tipo", 0))); p.forma = std::max(0, std::min(N_FORME - 1, IntDi(j, "forma", 0)));
            p.oggetto = Rif(j, "oggetto", NO); p.quantita = std::max(0, IntDi(j, "quantita", 0)); p.attrezzo = Rif(j, "attrezzo", NO); p.costo = std::max(0, IntDi(j, "costo", 0));
            p.usi = std::max(0, IntDi(j, "usi", 0)); p.ricrescita = std::max(0.0f, NumDi(j, "ricrescita", 0)); p.nemico = Rif(j, "nemico", (int)nem.size()); p.blocca = std::max(0, std::min(4, IntDi(j, "blocca", 0)));
            p.x = Clamp(NumDi(j, "x", 960), 40, 1880); p.y = Clamp(NumDi(j, "y", 600), 120, 980); p.colore = ColoreDi(j, "colore", Hex(0x6a5a4a));
            if (p.tipo == 3 && p.nemico < 0) p.tipo = 0;
            punti.push_back(p); }
        int NP = (int)punti.size();
        for (auto& j : Arr("missioni")) { Missione m; m.capitolo = std::max(1, IntDi(j, "capitolo", 1)); m.nome = TestoDi(j, "nome", "Missione"); m.testo = TestoDi(j, "testo", ""); m.fine = TestoDi(j, "fine", "Grazie!");
            m.chi = Rif(j, "chi", NP); m.tipo = std::max(0, std::min(3, IntDi(j, "tipo", 0))); m.oggetto = Rif(j, "oggetto", NO); m.quantita = std::max(0, IntDi(j, "quantita", 0));
            m.bersaglio = Rif(j, "bersaglio", NP); m.oro = IntDi(j, "oro", 0); m.premio = Rif(j, "premio", NO); m.premioQ = IntDi(j, "premioQuantita", 0); mis.push_back(m); }
        { int i = 0; for (auto& j : Arr("missioni")) { mis[i].richiede = Rif(j, "richiede", (int)mis.size()); if (mis[i].richiede == i) mis[i].richiede = -1; i++; } }
        puntiDi.assign(NL, {});
        for (int i = 0; i < NP; i++) puntiDi[punti[i].luogo].push_back(i);
        const json& g = cfg["generale"];
        vitaMax = std::max(1, IntDi(g, "vita", 10)); energiaMax = std::max(1, IntDi(g, "energia", 10)); zainoBase = std::max(4, IntDi(g, "zaino", 20));
        forzaBase = std::max(0, IntDi(g, "forza", 1)); riposoAperto = IntDi(g, "riposo", 3);
        lookEroe = LookEroe(g, 0);
    }

    // --- stato della partita (si salva) ---
    Caso caso; uint64_t seme = 1, casoInizio = 0; Regista regista;
    int cap = 0, luogo = 0, oro = 0, vita = 10, energia = 10;
    float ex = 960, ey = 760;
    std::vector<int> inv, statoMis;
    std::vector<StatoPunto> sp;
    std::vector<char> visitati;
    int borsaLuogo = -1, borsaOro = 0; float borsaX = 0, borsaY = 0;
    float tempoCapitolo = 0, tRiposo = 0;
    // --- stato del momento (non si salva) ---
    Az az = Az::Niente; int azPunto = -1, azUscita = -1, azLuogo = -1; float azT = 0, azDurata = 0, dx = 0, dy = 0;
    Pan pan = Pan::Niente; int dlgPunto = -1, dlgMis = -1, dlgModo = 0; std::string dlgTesto, dlgPremio;
    float tPan = 0, tFade = 0; int passo = 0; float tPasso = 0; bool versoSx = false; int arnese = 0;
    Vector2 cur{ 960, 600 }; float tMouse = 0; bool padUsato = false;
    int sopra = -1, sopraUscita = -1, sopraBottone = -1;   // cosa c'e' sotto il puntatore
    float scorriM = 0, scorriV = 0;
    struct Avviso { std::string t; float vita; Color c; };
    std::vector<Avviso> avvisi;
    struct Volo { int o, q; Vector2 a, b; float t; };
    std::vector<Volo> voli;
    struct Numero { std::string t; Vector2 p; float vita; Color c; };
    std::vector<Numero> numeri;
    Particelle part;
    void Numero_(const Numero& q) {   // i numeri vicini si mettono uno sopra l'altro
        Numero n = q;
        for (int giro = 0; giro < 6; giro++) {
            bool vicino = false;
            for (auto& a : numeri) if (std::fabs(a.p.x - n.p.x) < 320 && std::fabs(a.p.y - n.p.y) < 36) vicino = true;
            if (!vicino) break;
            n.p.y -= 38;
        }
        numeri.push_back(n);
    }
    float tScossa = 0; int duelloEsito = 0;
    int autoPausa = 0; float autoT = 0;

    // ---------------- regole ----------------
    int Usati() const { int n = 0; for (size_t i = 0; i < inv.size(); i++) if (inv[i] > 0) n++; return n; }
    int Posti() const { int n = zainoBase; for (size_t i = 0; i < inv.size(); i++) if (inv[i] > 0 && ogg[i].tipo == 6) n += ogg[i].valore; return n; }
    int Migliore(int tipo) const { int b = -1; for (size_t i = 0; i < inv.size(); i++) if (inv[i] > 0 && ogg[i].tipo == tipo && (b < 0 || ogg[i].valore > ogg[b].valore)) b = (int)i; return b; }
    int Potenza() const { int a = Migliore(4), d = Migliore(5); return forzaBase + (a >= 0 ? ogg[a].valore : 0) + (d >= 0 ? ogg[d].valore : 0); }
    int Danno(int potenzaNemico) const {
        float k = 0.8f + 0.4f * Clamp((regista.sfida - 0.6f) / 1.0f, 0, 1);
        return std::max(1, (int)std::ceil(3.0f * potenzaNemico / std::max(1, Potenza()) * k));
    }
    bool Ha(int o, int q = 1) const { return o >= 0 && o < (int)inv.size() && inv[o] >= q; }
    bool CiSta(int o) const { return o < 0 || inv[o] > 0 || Usati() < Posti(); }
    float FattorePrezzo() const { uint64_t h = (seme ^ (uint64_t)(cap + 1) * 0x9E3779B97F4A7C15ull) * 0xBF58476D1CE4E5B9ull; return 0.9f + 0.25f * ((h >> 40) % 1000) / 1000.f; }
    int PrezzoCompra(int o) const { return std::max(1, (int)std::ceil(ogg[o].prezzo * FattorePrezzo() * (regista.sfida < 0.85f ? 0.9f : 1.0f))); }
    int PrezzoVendi(int o) const { return std::max(1, ogg[o].prezzo / 2); }
    bool InVendita(int o) const { return ogg[o].negozio > 0 && ogg[o].negozio <= cap + 1; }
    bool Visibile(int l) const { return l >= 0 && luoghi[l].capitolo <= cap + 1; }
    int Bloccata(int l, int d) const {   // l'ostacolo che chiude l'uscita d di l (-1 = libera)
        for (int i : puntiDi[l]) if (punti[i].tipo == 5 && punti[i].blocca == d + 1 && !sp[i].fatto) return i;
        return -1;
    }
    bool Presente(int i) const {   // l'elemento si vede (i nemici sconfitti spariscono, gli ostacoli superati cambiano)
        const Punto& p = punti[i];
        if (p.tipo == 3) return !sp[i].fatto;
        return true;
    }
    bool Disponibile(int m) const {
        const Missione& q = mis[m];
        return statoMis[m] == 0 && q.capitolo <= cap + 1 && (q.richiede < 0 || statoMis[q.richiede] == 3);
    }
    bool Pronta(int m) const {
        const Missione& q = mis[m];
        if (statoMis[m] == 2) return true;
        if (statoMis[m] != 1) return false;
        if (q.tipo == 0) return q.oggetto < 0 || Ha(q.oggetto, std::max(1, q.quantita));
        return false;
    }
    int Segno(int i) const {   // 2 = "?" da consegnare, 1 = "!" nuova missione, 0 = niente
        int s = 0;
        for (size_t m = 0; m < mis.size(); m++) {
            if ((mis[m].chi == i && Pronta((int)m)) || (mis[m].tipo == 3 && statoMis[m] == 1 && mis[m].bersaglio == i)) return 2;
            if (mis[m].chi == i && Disponibile((int)m)) s = 1;
        }
        return s;
    }
    std::string Progresso(int m) const {
        const Missione& q = mis[m];
        if (statoMis[m] == 2) return "Fatto! Torna da " + (q.chi >= 0 ? punti[q.chi].nome : std::string("chi te l'ha chiesto"));
        switch (q.tipo) {
        case 0: if (q.oggetto < 0) return ""; return ogg[q.oggetto].nome + " " + std::to_string(std::min(inv[q.oggetto], q.quantita)) + "/" + std::to_string(q.quantita) + (Pronta(m) ? "  - consegna a " + (q.chi >= 0 ? punti[q.chi].nome : std::string("?")) : "");
        case 1: return q.bersaglio >= 0 ? "Sconfiggi " + punti[q.bersaglio].nome + " (" + luoghi[punti[q.bersaglio].luogo].nome + ")" : "";
        case 2: return q.bersaglio >= 0 ? "Libera: " + punti[q.bersaglio].nome + " (" + luoghi[punti[q.bersaglio].luogo].nome + ")" : "";
        case 3: return q.bersaglio >= 0 ? "Parla con " + punti[q.bersaglio].nome + " (" + luoghi[punti[q.bersaglio].luogo].nome + ")" : "";
        }
        return "";
    }
    void Avvisa(const std::string& t, Color c = Hex(0xf0e0b0)) { if (avvisi.size() > 3) avvisi.erase(avvisi.begin()); avvisi.push_back({ t, 3.2f, c }); }
    void No(const std::string& t) { Avvisa(t, Hex(0xffb0a0)); Suona("qv_no", 0.7f); }
    void Aggiungi(int o, int q, Vector2 da) {
        if (o < 0 || q <= 0) return;
        inv[o] += q;
        voli.push_back({ o, q, da, { 440, 1000 }, 0 });
        Numero_({ "+" + std::to_string(q) + " " + ogg[o].nome, { da.x, da.y - 60 }, 1.6f, Hex(0xfff0b0) });
        Suona("qv_zaino", 0.6f);
    }
    void Guadagna(int n, Vector2 da) {
        if (n <= 0) return;
        oro += n; punteggio += n;
        Numero_({ "+" + std::to_string(n) + " oro", { da.x, da.y - 90 }, 1.6f, Hex(0xffd860) });
        Suona("qv_moneta", 0.7f);
    }
    void Checkpoint() { casoInizio = caso.s; puntiDiControllo++; }

    // ---------------- nuova partita ----------------
    void Prepara() override { Leggi(); PreparaSuoni(); }
    void Nuova(uint64_t s, int g) override {
        Leggi();
        seme = s; giocatore = g; caso.Semina(s ^ 0xC0E5ull); regista = Regista{};
        finito = vinto = false; punteggio = 0; tempo = 0; puntiDiControllo = 0;
        inv.assign(ogg.size(), 0); statoMis.assign(mis.size(), 0); sp.assign(punti.size(), StatoPunto{}); visitati.assign(luoghi.size(), 0);
        const json& gj = cfg["generale"];
        const char* ki[4] = { "inizio1", "inizio2", "inizio3", "inizio4" }; const char* kn[4] = { "n1", "n2", "n3", "n4" };
        for (int i = 0; i < 4; i++) { int o = Rif(gj, ki[i], (int)ogg.size()); if (o >= 0) inv[o] += std::max(0, IntDi(gj, kn[i], 1)); }
        oro = std::max(0, IntDi(gj, "oro", 30)); vita = vitaMax; energia = energiaMax;
        borsaLuogo = -1;
        avvisi.clear(); voli.clear(); numeri.clear(); part.v.clear();
        IniziaCapitolo(0, false);
    }
    void IniziaCapitolo(int k, bool premio) {
        cap = std::max(0, std::min((int)caps.size() - 1, k));
        tempoCapitolo = 0;
        Entra(caps[cap].partenza, -1);
        ex = 960; ey = 760;
        az = Az::Niente; pan = Pan::Capitolo; tPan = 0;
        if (premio) Suona("qv_capitolo");
        Checkpoint();
    }
    void IniziaDa(int tappa) override {
        int k = std::max(0, std::min((int)caps.size() - 1, tappa));
        // le missioni dei capitoli precedenti sono gia' compiute, con i loro premi
        for (size_t m = 0; m < mis.size(); m++)
            if (mis[m].capitolo <= k) {
                const Missione& q = mis[m];
                if (q.tipo == 0 && q.oggetto >= 0 && ogg[q.oggetto].tipo != 7) inv[q.oggetto] = std::max(0, inv[q.oggetto] - q.quantita);
                if ((q.tipo == 1 || q.tipo == 2) && q.bersaglio >= 0) sp[q.bersaglio].fatto = true;
                statoMis[m] = 3; oro += q.oro;
                if (q.premio >= 0) inv[q.premio] += q.premioQ;
                if (q.tipo == 1 && q.bersaglio >= 0 && punti[q.bersaglio].nemico >= 0) {
                    const Nemico& n = nem[punti[q.bersaglio].nemico];
                    oro += n.oro; if (n.lascia >= 0 && ogg[n.lascia].tipo != 7) inv[n.lascia] += n.quantita;
                }
            }
        // armi, armature e attrezzi che si sarebbero creati o comprati
        for (auto& r : ric) if (r.cap <= k && (ogg[r.ris].tipo >= 3 && ogg[r.ris].tipo <= 6) && inv[r.ris] == 0) inv[r.ris] = 1;
        for (size_t o = 0; o < ogg.size(); o++) if (InVendita((int)o) && ogg[o].negozio <= k && ogg[o].tipo >= 3 && ogg[o].tipo <= 6 && inv[o] == 0) inv[o] = 1;
        for (size_t o = 0; o < ogg.size(); o++) if (ogg[o].tipo == 7) inv[o] = 0;
        for (size_t l = 0; l < luoghi.size(); l++) if (luoghi[l].capitolo <= k) visitati[l] = 1;
        IniziaCapitolo(k, false);
        puntiDiControllo = 0;
    }
    void Entra(int l, int daDir) {
        luogo = std::max(0, std::min((int)luoghi.size() - 1, l));
        visitati[luogo] = 1;
        if (daDir >= 0) { ex = ARRIVO[(daDir + 2) % 4].x; ey = ARRIVO[(daDir + 2) % 4].y; }
        // a volte qualcuno ha perso una borsa di monete (cambia a ogni partita)
        if (borsaLuogo < 0 && caso.P(0.18f)) {
            borsaLuogo = luogo; borsaOro = caso.I(5, 12) + cap * 4;
            borsaX = caso.F(300, 1600); borsaY = caso.F(420, 880);
            for (int i : puntiDi[luogo]) if (std::fabs(punti[i].x - borsaX) < 120 && std::fabs(punti[i].y - borsaY) < 90) { borsaLuogo = -1; break; }
        }
        Musica();
    }
    void Musica() {
        std::string m = luoghi[luogo].musica;
        if (m.empty() && cap < (int)caps.size()) m = caps[cap].musica;
        SuonaMusica(BranoDi(m.empty() ? "Querciaverde - Borgo" : m));
    }

    // ---------------- interazione ----------------
    Rectangle Area(int i) const {   // area cliccabile di un elemento
        const Punto& p = punti[i];
        Spec s = p.tipo == 3 ? SpecNemico(nem[p.nemico].forma) : SpecForma(p.forma);
        float k = p.tipo == 3 && nem[p.nemico].grande ? 1.35f : 1;
        float hw = std::max(60.f, s.hw * k), hh = std::max(60.f, s.hh * k);
        return { p.x + s.hx - hw / 2, p.y - hh, hw, hh + 14 };
    }
    bool Interattivo(int i) const { return punti[i].tipo != 0 && Presente(i); }
    int UscitaVisibile(int d) const { int l = luoghi[luogo].uscite[d]; return Visibile(l) ? l : -1; }
    const Rectangle BARRA = { 360, 950, 1200, 124 };
    Vector2 Angolo(int k) const { return k == 0 ? Vector2{ 44, 44 } : k == 1 ? Vector2{ 1876, 44 } : k == 2 ? Vector2{ 44, 1036 } : Vector2{ 1876, 1036 }; }
    std::vector<int> Rapidi() const {   // cibi e cure nella barra in basso
        std::vector<int> v;
        for (size_t o = 0; o < inv.size(); o++) if (inv[o] > 0 && (ogg[o].tipo == 1 || ogg[o].tipo == 2)) v.push_back((int)o);
        if (v.size() > 5) v.resize(5);
        return v;
    }
    Rectangle Rapido(int k) const { return { 1190 + k * 70.f, 978, 62, 62 }; }
    bool BorsaQui() const { return borsaLuogo == luogo; }

    void Calcola() {
        sopra = -1; sopraUscita = -1; sopraBottone = -1;
        if (pan != Pan::Niente) return;
        for (int k = 0; k < 4; k++) if (Vector2Distance(cur, Angolo(k)) < 36) sopraBottone = k;
        auto r = Rapidi();
        for (size_t k = 0; k < r.size(); k++) if (CheckCollisionPointRec(cur, Rapido((int)k))) sopraBottone = 10 + (int)k;
        if (sopraBottone >= 0 || CheckCollisionPointRec(cur, BARRA)) return;
        for (int d = 0; d < 4; d++) if (UscitaVisibile(d) >= 0 && Vector2Distance(cur, FRECCIA[d]) < 56) { sopraUscita = d; return; }
        float best = -1;
        for (int i : puntiDi[luogo]) if (Interattivo(i) && CheckCollisionPointRec(cur, Area(i)) && punti[i].y > best) { best = punti[i].y; sopra = i; }
        if (BorsaQui() && Vector2Distance(cur, { borsaX, borsaY - 10 }) < 40) sopra = -2;
    }
    void Prossimo() {   // salta al prossimo elemento (tastiera e joypad)
        std::vector<Vector2> v;
        for (int i : puntiDi[luogo]) if (Interattivo(i)) { Rectangle a = Area(i); v.push_back({ a.x + a.width / 2, a.y + a.height * 0.55f }); }
        for (int d = 0; d < 4; d++) if (UscitaVisibile(d) >= 0) v.push_back(FRECCIA[d]);
        if (v.empty()) return;
        std::sort(v.begin(), v.end(), [](Vector2 a, Vector2 b) { return a.x + a.y * 0.3f < b.x + b.y * 0.3f; });
        float c0 = cur.x + cur.y * 0.3f;
        for (auto& q : v) if (q.x + q.y * 0.3f > c0 + 4) { cur = q; return; }
        cur = v[0];
    }
    void VaiA(float x, float y) { dx = Clamp(x, 90, 1830); dy = Clamp(y, 240, 935); az = Az::Cammina; }
    void Vai(int i) {
        const Punto& p = punti[i];
        Rectangle a = Area(i);
        float off = std::min(150.f, std::max(46.f, a.width * 0.42f));
        float lato = ex < p.x ? -1.f : 1.f;
        if (p.tipo == 7 && (p.forma == F_CASA_PAGLIA || p.forma == F_CASA_TEGOLE)) { VaiA(p.x - 70, p.y + 30); azPunto = i; azUscita = -1; return; }
        if (p.tipo == 5) { VaiA(p.x + lato * 40, p.y + 70); azPunto = i; azUscita = -1; return; }
        VaiA(p.x + lato * off, p.y + 14);
        azPunto = i; azUscita = -1;
    }
    void VaiUscita(int d) {
        int ost = Bloccata(luogo, d);
        if (ost >= 0) { No("La strada e' bloccata: " + punti[ost].nome); Vai(ost); return; }
        Vector2 q = FRECCIA[d];
        VaiA(q.x + (d == 1 ? -60.f : d == 3 ? 60.f : 0), q.y + (d == 0 ? 150.f : d == 2 ? -100.f : 40));
        azPunto = -1; azUscita = d;
    }
    void Clic() {
        if (az == Az::Lavora || az == Az::Combatte || az == Az::Viaggia || az == Az::Riposa) return;
        if (sopraBottone >= 0 && sopraBottone < 4) { Suona("qv_clic", 0.6f); Apri(sopraBottone == 0 ? Pan::Diario : sopraBottone == 1 ? Pan::Mappa : sopraBottone == 2 ? Pan::Lavori : Pan::Zaino); return; }
        if (sopraBottone >= 10) { auto r = Rapidi(); if (sopraBottone - 10 < (int)r.size()) Usa(r[sopraBottone - 10]); return; }
        if (sopra == -2) { VaiA(borsaX - 40, borsaY + 10); azPunto = -2; azUscita = -1; return; }
        if (sopra >= 0) { Vai(sopra); return; }
        if (sopraUscita >= 0) { VaiUscita(sopraUscita); return; }
        if (!CheckCollisionPointRec(cur, BARRA)) { VaiA(cur.x, cur.y); azPunto = -1; azUscita = -1; }
    }
    void Apri(Pan p) { pan = p; tPan = 0; scorriM = scorriV = 0; az = Az::Niente; }

    // Arrivato vicino all'elemento: che cosa succede
    void Esegui(int i) {
        if (i == -2) { if (BorsaQui()) { Guadagna(borsaOro, { borsaX, borsaY }); Avvisa("Hai trovato una borsa di monete!"); borsaLuogo = -1; } az = Az::Niente; return; }
        const Punto& p = punti[i];
        versoSx = p.x < ex;
        az = Az::Niente;
        switch (p.tipo) {
        case 1: {
            if (p.attrezzo >= 0 && !Ha(p.attrezzo)) { No("Ti serve: " + ogg[p.attrezzo].nome); return; }
            if (p.usi > 0 && sp[i].usati >= p.usi) { No(sp[i].attesa > 0 ? "Ricresce tra " + std::to_string((int)std::ceil(sp[i].attesa)) + " secondi" : "Qui non c'e' piu' niente"); return; }
            if (energia < p.costo) { No("Sei stanco: mangia qualcosa o riposa"); return; }
            if (!CiSta(p.oggetto)) { No("Lo zaino e' pieno: vendi qualcosa al mercato"); return; }
            Inizia(Az::Lavora, i, 1.1f); return;
        }
        case 2: ApriDialogo(i); return;
        case 3: {
            const Nemico& n = nem[p.nemico];
            if (Potenza() < n.potenza) { No(n.nome + " e' troppo forte: potenza " + std::to_string(n.potenza) + ", la tua " + std::to_string(Potenza()) + ". Cerca armi migliori!"); return; }
            if (vita <= Danno(n.potenza)) { No("Sei troppo ferito: mangia, bevi una pozione o riposa"); return; }
            Inizia(Az::Combatte, i, 1.7f); SuonaMusica(BranoDi(TestoDi(cfg["generale"], "musicaBattaglia", "Querciaverde - Battaglia"))); return;
        }
        case 4: Apri(Pan::Mercato); dlgPunto = i; return;
        case 5: {
            if (sp[i].fatto) return;
            if (p.attrezzo >= 0) {
                bool arnese2 = ogg[p.attrezzo].tipo == 3;
                if (!Ha(p.attrezzo, arnese2 ? 1 : std::max(1, p.costo))) { No("Per " + p.nome + " ti serve: " + (arnese2 ? "" : std::to_string(std::max(1, p.costo)) + " ") + ogg[p.attrezzo].nome); return; }
            }
            if (energia < 1) { No("Sei stanco: mangia qualcosa o riposa"); return; }
            Inizia(Az::Lavora, i, 1.6f); return;
        }
        case 6: {
            if (sp[i].fatto) { Avvisa("Il forziere e' vuoto"); return; }
            if (p.attrezzo >= 0 && !Ha(p.attrezzo)) { No("Il forziere e' chiuso: serve " + ogg[p.attrezzo].nome); return; }
            if (!CiSta(p.oggetto)) { No("Lo zaino e' pieno"); return; }
            Inizia(Az::Lavora, i, 0.8f); return;
        }
        case 7: Inizia(Az::Riposa, i, 1.8f); return;
        }
    }
    void Inizia(Az a, int i, float durata) { az = a; azPunto = i; azT = 0; azDurata = durata; duelloEsito = 0; }
    int ArneseDi(int i) const {   // che cosa tiene in mano l'eroe mentre lavora
        const Punto& p = punti[i];
        if (p.tipo == 3) { int a = Migliore(4); if (a < 0) return 4; int ic = ogg[a].icona; return ic == I_SPADA ? 1 : ic == I_LANCIA ? 5 : ic == I_ARCO ? 9 : ic == I_ASCIA ? 2 : 4; }
        int o = p.attrezzo;
        if (o < 0) return 0;
        switch (ogg[o].icona) { case I_ASCIA: return 2; case I_PICCONE: return 3; case I_FALCE: return 6; case I_CANNA: return 7; case I_SECCHIO: return 8; case I_PALA: return 10; }
        return 0;
    }
    void Completa(int i) {
        const Punto& p = punti[i];
        az = Az::Niente;
        switch (p.tipo) {
        case 1: {
            int q = p.quantita + (p.quantita > 0 && caso.F() < 0.2f ? 1 : 0);
            energia = std::max(0, energia - p.costo);
            if (p.usi > 0 && ++sp[i].usati >= p.usi) sp[i].attesa = p.ricrescita;
            Aggiungi(p.oggetto, q, { p.x, p.y - 40 });
            punteggio += 2 * q;
            break;
        }
        case 3: {
            const Nemico& n = nem[p.nemico];
            int d = Danno(n.potenza);
            vita = std::max(1, vita - d);
            sp[i].fatto = true; if (p.ricrescita > 0) sp[i].attesa = p.ricrescita;
            Guadagna(n.oro, { p.x, p.y });
            if (n.lascia >= 0 && n.quantita > 0 && CiSta(n.lascia)) Aggiungi(n.lascia, n.quantita, { p.x, p.y - 30 });
            Numero_({ "-" + std::to_string(d) + " vita", { ex, ey - 120 }, 1.6f, Hex(0xff7a6a) });
            punteggio += n.potenza * 25;
            part.Esplosione({ p.x, p.y - 50 }, n.col, 40, 260, 30, 0.8f);
            Suona("qv_vittoria"); regista.Bersaglio(); regista.Sparo();
            for (size_t m = 0; m < mis.size(); m++) if (statoMis[m] == 1 && mis[m].tipo == 1 && mis[m].bersaglio == i) { statoMis[m] = 2; Avvisa("Missione compiuta: torna da " + (mis[m].chi >= 0 ? punti[mis[m].chi].nome : std::string("chi te l'ha chiesta")), Hex(0xffe070)); }
            Musica();
            break;
        }
        case 5: {
            if (p.attrezzo >= 0 && ogg[p.attrezzo].tipo != 3) inv[p.attrezzo] = std::max(0, inv[p.attrezzo] - std::max(1, p.costo));
            energia = std::max(0, energia - 1);
            sp[i].fatto = true;
            part.Esplosione({ p.x, p.y - 40 }, Hex(0xc8a070), 50, 300, 40, 0.9f);
            Suona("qv_lavoro"); punteggio += 50;
            Avvisa(p.nome + ": la strada e' libera!", Hex(0xc0ffb0));
            for (size_t m = 0; m < mis.size(); m++) if (statoMis[m] == 1 && mis[m].tipo == 2 && mis[m].bersaglio == i) { statoMis[m] = 2; Avvisa("Missione compiuta: torna da " + (mis[m].chi >= 0 ? punti[mis[m].chi].nome : std::string("chi te l'ha chiesta")), Hex(0xffe070)); }
            break;
        }
        case 6: {
            sp[i].fatto = true;
            if (p.attrezzo >= 0 && ogg[p.attrezzo].tipo == 7) inv[p.attrezzo]--;
            Suona("qv_forziere");
            Guadagna(p.costo, { p.x, p.y });
            Aggiungi(p.oggetto, p.quantita, { p.x, p.y - 30 });
            break;
        }
        case 7: {
            int prezzo = std::min(oro, p.costo);
            oro -= prezzo; vita = vitaMax; energia = energiaMax;
            Avvisa(prezzo ? "Hai riposato (" + std::to_string(prezzo) + " oro): vita ed energia al massimo" : "Hai riposato: vita ed energia al massimo", Hex(0xc0e0ff));
            break;
        }
        }
    }
    void Usa(int o) {
        if (o < 0 || inv[o] <= 0) return;
        const Oggetto& q = ogg[o];
        if (q.tipo == 1) { if (energia >= energiaMax) { Avvisa("Non hai fame"); return; } energia = std::min(energiaMax, energia + std::max(1, q.valore)); }
        else if (q.tipo == 2) { if (vita >= vitaMax) { Avvisa("Stai benissimo"); return; } vita = std::min(vitaMax, vita + std::max(1, q.valore)); }
        else return;
        inv[o]--;
        Suona("qv_mangia");
        Numero_({ q.tipo == 1 ? "+" + std::to_string(q.valore) + " energia" : "+" + std::to_string(q.valore) + " vita", { ex, ey - 110 }, 1.4f, q.tipo == 1 ? Hex(0x9ad8ff) : Hex(0xff9aa0) });
    }

    // ---------------- dialoghi e missioni ----------------
    void ApriDialogo(int i) {
        dlgPunto = i; dlgMis = -1; dlgModo = 0; dlgPremio.clear();
        const Punto& p = punti[i];
        for (size_t m = 0; m < mis.size() && dlgMis < 0; m++) if ((mis[m].chi == i && Pronta((int)m)) || (mis[m].tipo == 3 && statoMis[m] == 1 && mis[m].bersaglio == i)) { dlgMis = (int)m; dlgModo = 2; }
        for (size_t m = 0; m < mis.size() && dlgMis < 0; m++) if (mis[m].chi == i && Disponibile((int)m)) { dlgMis = (int)m; dlgModo = 1; }
        for (size_t m = 0; m < mis.size() && dlgMis < 0; m++) if (mis[m].chi == i && statoMis[m] == 1) { dlgMis = (int)m; dlgModo = 0; }
        if (dlgModo == 2) dlgTesto = mis[dlgMis].tipo == 0 && mis[dlgMis].oggetto >= 0 ? "Hai portato " + std::to_string(mis[dlgMis].quantita) + " " + ogg[mis[dlgMis].oggetto].nome + "? Splendido!" : "Allora ce l'hai fatta!";
        else if (dlgMis >= 0) dlgTesto = mis[dlgMis].testo;
        else dlgTesto = p.frase.empty() ? "Salve, viandante." : p.frase;
        if (dlgMis >= 0) dlgPremio = PremioTesto(dlgMis);
        Apri(Pan::Dialogo);
        Suona("qv_clic", 0.5f);
    }
    std::string PremioTesto(int m) const {
        const Missione& q = mis[m];
        std::string s;
        if (q.oro > 0) s += std::to_string(q.oro) + " oro";
        if (q.premio >= 0 && q.premioQ > 0) s += (s.empty() ? "" : "  +  ") + std::to_string(q.premioQ) + " " + ogg[q.premio].nome;
        return s;
    }
    void Accetta() {
        if (dlgMis < 0 || statoMis[dlgMis] != 0) return;
        const Missione& q = mis[dlgMis];
        statoMis[dlgMis] = 1;
        if ((q.tipo == 1 || q.tipo == 2) && q.bersaglio >= 0 && sp[q.bersaglio].fatto && punti[q.bersaglio].ricrescita <= 0) statoMis[dlgMis] = 2;
        Suona("qv_missione");
        Avvisa("Nuova missione: " + q.nome, Hex(0xffe070));
        pan = Pan::Niente;
    }
    void Consegna() {
        if (dlgMis < 0) return;
        int m = dlgMis;
        const Missione& q = mis[m];
        if (!(Pronta(m) || (q.tipo == 3 && statoMis[m] == 1))) return;
        if (q.tipo == 0 && q.oggetto >= 0) inv[q.oggetto] = std::max(0, inv[q.oggetto] - q.quantita);
        statoMis[m] = 3;
        const Punto& p = punti[dlgPunto];
        Guadagna(q.oro, { p.x, p.y });
        if (q.premio >= 0 && q.premioQ > 0) Aggiungi(q.premio, q.premioQ, { p.x, p.y - 30 });
        punteggio += 100;
        Suona("qv_completa");
        dlgTesto = q.fine; dlgModo = 3;
        Checkpoint();
        // capitolo finito?
        bool tutte = true;
        for (size_t k = 0; k < mis.size(); k++) if (mis[k].capitolo == cap + 1 && statoMis[k] != 3) tutte = false;
        if (tutte) {
            regista.TappaFinita(tempoCapitolo, 900);
            if (cap + 1 >= (int)caps.size()) { finVicina = true; }
            else capitoloPronto = true;
        }
    }
    bool capitoloPronto = false, finVicina = false;
    void ChiudiDialogo() {
        pan = Pan::Niente;
        if (capitoloPronto) { capitoloPronto = false; punteggio += 500; IniziaCapitolo(cap + 1, true); }
        else if (finVicina) { finVicina = false; pan = Pan::Fine; tPan = 0; finito = vinto = true; punteggio += 2000; Suona("qv_capitolo"); }
    }
    // ---------------- mercato e lavori ----------------
    std::vector<int> DaComprare() const { std::vector<int> v; for (size_t o = 0; o < ogg.size(); o++) if (InVendita((int)o)) v.push_back((int)o); return v; }
    std::vector<int> DaVendere() const { std::vector<int> v; for (size_t o = 0; o < ogg.size(); o++) if (inv[o] > 0 && ogg[o].tipo != 7) v.push_back((int)o); return v; }
    bool Compra(int o) {
        int pr = PrezzoCompra(o);
        if (oro < pr) { No("Non hai abbastanza oro"); return false; }
        if (!CiSta(o)) { No("Lo zaino e' pieno"); return false; }
        oro -= pr; inv[o]++; Suona("qv_moneta", 0.6f);
        return true;
    }
    bool Vendi(int o) {
        if (inv[o] <= 0) return false;
        inv[o]--; oro += PrezzoVendi(o); Suona("qv_moneta", 0.5f);
        return true;
    }
    std::vector<int> RicetteAperte() const { std::vector<int> v; for (size_t r = 0; r < ric.size(); r++) if (ric[r].cap <= cap + 1) v.push_back((int)r); return v; }
    bool Fattibile(int r) const { for (int k = 0; k < 3; k++) if (ric[r].ing[k] >= 0 && !Ha(ric[r].ing[k], ric[r].n[k])) return false; return true; }
    bool Crea(int r) {
        if (!Fattibile(r)) { No("Mancano degli ingredienti"); return false; }
        const Ricetta& q = ric[r];
        // gli ingredienti finiti liberano posto: si controlla dopo averli tolti
        for (int k = 0; k < 3; k++) if (q.ing[k] >= 0) inv[q.ing[k]] -= q.n[k];
        if (!CiSta(q.ris)) { for (int k = 0; k < 3; k++) if (q.ing[k] >= 0) inv[q.ing[k]] += q.n[k]; No("Lo zaino e' pieno"); return false; }
        inv[q.ris] += q.q;
        Numero_({ "+" + std::to_string(q.q) + " " + ogg[q.ris].nome, { ex, ey - 120 }, 1.6f, Hex(0xfff0b0) });
        Suona("qv_lavoro"); punteggio += 10;
        return true;
    }
    // ---------------- mappa ----------------
    std::vector<int> Cammino(int da, int a) const {   // localita' da attraversare (uscite visibili e libere)
        std::vector<int> prec(luoghi.size(), -2); std::deque<int> c; c.push_back(da); prec[da] = -1;
        while (!c.empty()) {
            int l = c.front(); c.pop_front();
            if (l == a) break;
            for (int d = 0; d < 4; d++) { int n = luoghi[l].uscite[d]; if (Visibile(n) && prec[n] == -2 && Bloccata(l, d) < 0) { prec[n] = l; c.push_back(n); } }
        }
        std::vector<int> v;
        if (prec[a] == -2) return v;
        for (int l = a; l != -1; l = prec[l]) v.push_back(l);
        std::reverse(v.begin(), v.end());
        return v;
    }

    // ---------------- finestre: pulsanti ----------------
    struct Btn { Rectangle r; int id; std::string t; bool ok = true; };
    const Rectangle FIN = { 250, 100, 1420, 820 };
    std::vector<Btn> Bottoni() const {
        std::vector<Btn> b;
        float x = FIN.x, y = FIN.y, w = FIN.width;
        auto Chiudi = [&]() { b.push_back({ { x + w - 70, y + 16, 54, 54 }, 1, "X" }); };
        switch (pan) {
        case Pan::Dialogo: {
            Rectangle d = DlgRett();
            if (dlgModo == 1) { b.push_back({ { d.x + d.width - 520, d.y + d.height - 84, 240, 62 }, 10, "Accetta" }); b.push_back({ { d.x + d.width - 266, d.y + d.height - 84, 240, 62 }, 12, "Non ora" }); }
            else if (dlgModo == 2) b.push_back({ { d.x + d.width - 266, d.y + d.height - 84, 240, 62 }, 11, "Consegna" });
            else b.push_back({ { d.x + d.width - 266, d.y + d.height - 84, 240, 62 }, 12, dlgModo == 3 ? "Grazie!" : "Ciao" });
            break;
        }
        case Pan::Mercato: {
            Chiudi();
            auto c = DaComprare(), v = DaVendere();
            for (int k = 0; k < 12 && k + (int)scorriM < (int)c.size(); k++) { int o = c[k + (int)scorriM]; b.push_back({ { x + 40, y + 150 + k * 52.f, 640, 48 }, 100 + o, "", oro >= PrezzoCompra(o) }); }
            for (int k = 0; k < 12 && k + (int)scorriV < (int)v.size(); k++) { int o = v[k + (int)scorriV]; b.push_back({ { x + 740, y + 150 + k * 52.f, 640, 48 }, 300 + o, "" }); }
            if (c.size() > 12) { b.push_back({ { x + 610, y + 92, 34, 34 }, 60, "^" }); b.push_back({ { x + 650, y + 92, 34, 34 }, 61, "v" }); }
            if (v.size() > 12) { b.push_back({ { x + 1310, y + 92, 34, 34 }, 62, "^" }); b.push_back({ { x + 1350, y + 92, 34, 34 }, 63, "v" }); }
            break;
        }
        case Pan::Lavori: {
            Chiudi();
            auto r = RicetteAperte();
            int prima = (int)scorriM;
            for (int k = 0; k < 24 && k + prima < (int)r.size(); k++) { int q = r[k + prima]; b.push_back({ { x + 40 + (k % 2) * 680.f, y + 110 + (k / 2) * 56.f, 660, 52 }, 500 + q, "", Fattibile(q) }); }
            if (r.size() > 24) { b.push_back({ { x + w - 160, y + 26, 34, 34 }, 64, "^" }); b.push_back({ { x + w - 120, y + 26, 34, 34 }, 65, "v" }); }
            break;
        }
        case Pan::Zaino: {
            Chiudi();
            int k = 0;
            for (size_t o = 0; o < inv.size(); o++) if (inv[o] > 0) { b.push_back({ { x + 50 + (k % 10) * 132.f, y + 190 + (k / 10) * 132.f, 120, 120 }, 700 + (int)o, "" }); k++; }
            break;
        }
        case Pan::Diario: Chiudi(); break;
        case Pan::Mappa: {
            Chiudi();
            for (auto& n : NodiMappa()) if (visitati[n.l] && n.l != luogo) b.push_back({ { n.p.x - 120, n.p.y - 34, 240, 68 }, 900 + n.l, "" });
            break;
        }
        case Pan::Capitolo: b.push_back({ { 960 - 160, 840, 320, 70 }, 50, "Comincia" }); break;
        default: break;
        }
        return b;
    }
    Rectangle DlgRett() const { return { 330, 640, 1260, 300 }; }
    struct Nodo { int l; Vector2 p; };
    std::vector<Nodo> NodiMappa() const {   // le localita' del capitolo, disposte seguendo le uscite
        std::vector<Nodo> v;
        std::map<int, Vector2> pos;
        int start = caps[cap].partenza;
        std::deque<int> c; c.push_back(start); pos[start] = { 0, 0 };
        const Vector2 dir[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
        while (!c.empty()) {
            int l = c.front(); c.pop_front();
            for (int d = 0; d < 4; d++) {
                int n = luoghi[l].uscite[d];
                if (n < 0 || luoghi[n].capitolo != cap + 1 || pos.count(n)) continue;
                Vector2 q = Vector2Add(pos[l], dir[d]);
                for (auto& e : pos) if (Vector2Distance(e.second, q) < 0.1f) q.x += 0.5f;
                pos[n] = q; c.push_back(n);
            }
        }
        float mnx = 0, mxx = 0, mny = 0, mxy = 0;
        for (auto& e : pos) { mnx = std::min(mnx, e.second.x); mxx = std::max(mxx, e.second.x); mny = std::min(mny, e.second.y); mxy = std::max(mxy, e.second.y); }
        float sx = std::min(380.f, 1000 / std::max(1.f, mxx - mnx)), sy = std::min(170.f, 480 / std::max(1.f, mxy - mny));
        for (auto& e : pos) v.push_back({ e.first, { 960 + (e.second.x - (mnx + mxx) / 2) * sx, 540 + (e.second.y - (mny + mxy) / 2) * sy } });
        return v;
    }
    int BottoneSotto(const std::vector<Btn>& b) const { for (auto& q : b) if (CheckCollisionPointRec(cur, q.r)) return q.id; return -1; }
    void Premi(int id) {
        if (id == 1) { pan = Pan::Niente; Suona("qv_clic", 0.5f); return; }
        if (id == 10) { Accetta(); return; }
        if (id == 11) { Consegna(); return; }
        if (id == 12) { ChiudiDialogo(); return; }
        if (id == 50) { pan = Pan::Niente; Musica(); return; }
        if (id >= 60 && id <= 65) { float& s = id < 62 ? scorriM : id < 64 ? scorriV : scorriM; s = std::max(0.f, s + (id % 2 ? 1 : -1)); LimitaScorri(); return; }
        if (id >= 100 && id < 300) { Compra(id - 100); return; }
        if (id >= 300 && id < 500) { Vendi(id - 300); LimitaScorri(); return; }
        if (id >= 500 && id < 700) { Crea(id - 500); return; }
        if (id >= 700 && id < 900) { Usa(id - 700); return; }
        if (id >= 900) { Viaggia(id - 900); return; }
    }
    void LimitaScorri() {
        if (pan == Pan::Mercato) { scorriM = Clamp(scorriM, 0, std::max(0.f, (float)DaComprare().size() - 12)); scorriV = Clamp(scorriV, 0, std::max(0.f, (float)DaVendere().size() - 12)); }
        if (pan == Pan::Lavori) scorriM = Clamp(scorriM, 0, std::max(0.f, (float)((RicetteAperte().size() + 1) / 2 * 2) - 24));
    }
    void Viaggia(int l) {   // dalla mappa: si va subito se la strada e' aperta
        auto c = Cammino(luogo, l);
        if (c.size() < 2) { No("Non c'e' una strada aperta fino a li'"); return; }
        pan = Pan::Niente; az = Az::Viaggia; azLuogo = l; azUscita = -1; azT = 0;
        Suona("qv_viaggio", 0.7f);
    }

    // ---------------- aggiornamento ----------------
    void Aggiorna(float dt, const Pad& p) override {
        if (caps.empty()) return;
        tempo += dt; tempoCapitolo += dt; tPan += dt;
        for (size_t i = 0; i < sp.size(); i++)
            if (sp[i].attesa > 0) { sp[i].attesa -= dt; if (sp[i].attesa <= 0) { sp[i].attesa = 0; if (punti[i].tipo == 3) sp[i].fatto = false; else sp[i].usati = 0; } }
        for (auto& a : avvisi) a.vita -= dt;
        avvisi.erase(std::remove_if(avvisi.begin(), avvisi.end(), [](const Avviso& a) { return a.vita <= 0; }), avvisi.end());
        for (auto& n : numeri) { n.vita -= dt; n.p.y -= 40 * dt; }
        numeri.erase(std::remove_if(numeri.begin(), numeri.end(), [](const Numero& a) { return a.vita <= 0; }), numeri.end());
        for (auto& v : voli) v.t += dt * 1.4f;
        voli.erase(std::remove_if(voli.begin(), voli.end(), [](const Volo& a) { return a.t >= 1; }), voli.end());
        part.Aggiorna(dt);
        if (tScossa > 0) tScossa -= dt;
        regista.Osserva(dt, ex / 1920, ey / 1080, false);
        if (az == Az::Niente && pan == Pan::Niente && riposoAperto > 0 && energia < energiaMax) {
            tRiposo += dt;
            if (tRiposo >= 60.0f / riposoAperto) { tRiposo = 0; energia++; }
        }
        // puntatore
        if (p.mouseMosso) { cur = p.mouse; padUsato = false; }
        if (std::fabs(p.x) > 0.15f || std::fabs(p.y) > 0.15f) { cur.x += p.x * 1100 * dt; cur.y += p.y * 1100 * dt; padUsato = true; }
        cur.x = Clamp(cur.x, 0, 1920); cur.y = Clamp(cur.y, 0, 1080);
        if (p.saltoP && pan == Pan::Niente) { Prossimo(); padUsato = true; }
        bool clic = p.clicP || p.fuocoP, chiudi = p.bombaP || p.clicDxP;
        Calcola();
        if (finito) return;
        if (pan == Pan::Niente && (capitoloPronto || finVicina)) { ChiudiDialogo(); return; }
        if (pan != Pan::Niente) {
            if (pan == Pan::Mercato || pan == Pan::Lavori) {
                if (p.rotella != 0) { if (pan == Pan::Mercato && cur.x > FIN.x + 720) scorriV -= p.rotella; else scorriM -= p.rotella; LimitaScorri(); }
            }
            if (chiudi && pan != Pan::Capitolo) { if (pan == Pan::Dialogo) ChiudiDialogo(); else pan = Pan::Niente; return; }
            if (clic && tPan > 0.15f) { int id = BottoneSotto(Bottoni()); if (id >= 0) Premi(id); }
            return;
        }
        if (clic) Clic();
        switch (az) {
        case Az::Cammina: {
            Vector2 d{ dx - ex, dy - ey };
            float l = Vector2Length(d), v = 500 * dt;
            if (std::fabs(d.x) > 2) versoSx = d.x < 0;
            tPasso += dt; if (tPasso > 0.11f) { tPasso = 0; passo = (passo + 1) % 4; }
            if (l <= v) {
                ex = dx; ey = dy; az = Az::Niente; passo = 0;
                if (azPunto != -1) Esegui(azPunto);
                else if (azUscita >= 0) { int n = UscitaVisibile(azUscita); if (n >= 0) { az = Az::Viaggia; azLuogo = n; azT = 0; Suona("qv_viaggio", 0.6f); } }
            } else { ex += d.x / l * v; ey += d.y / l * v; }
            break;
        }
        case Az::Lavora: case Az::Combatte: case Az::Riposa: {
            float prima = azT;
            azT += dt;
            if (az != Az::Riposa) {
                float ritmo = az == Az::Combatte ? 0.42f : 0.36f;
                if ((int)(prima / ritmo) != (int)(azT / ritmo)) {
                    const Punto& q = punti[azPunto];
                    if (az == Az::Combatte) { Suona("qv_spada", 0.8f, Vis(0.9f, 1.1f)); tScossa = 0.15f; part.Esplosione({ (ex + q.x) / 2, q.y - 50 }, Hex(0xfff0a0), 10, 240, 6, 0.3f); }
                    else {
                        int a = ArneseDi(azPunto);
                        Suona(a == 2 ? "qv_legna" : a == 3 ? "qv_pietra" : a == 8 || a == 7 ? "qv_acqua" : "qv_raccogli", 0.7f, Vis(0.9f, 1.1f));
                        part.Esplosione({ q.x + (versoSx ? 30.f : -30.f), q.y - 40 }, q.tipo == 5 ? Hex(0xa08060) : Hex(0xd8c090), 8, 160, 5, 0.5f);
                    }
                }
            }
            if (azT >= azDurata) Completa(azPunto);
            break;
        }
        case Az::Viaggia: {
            azT += dt;
            if (azT >= 0.35f && azLuogo >= 0) {
                int da = -1;
                for (int d = 0; d < 4; d++) if (luoghi[luogo].uscite[d] == azLuogo) da = d;
                int dest = azLuogo; azLuogo = -1;
                if (da < 0) { Entra(dest, -1); ex = 960; ey = 700; }
                else Entra(dest, da);
            }
            if (azT >= 0.7f) az = Az::Niente;
            break;
        }
        default: break;
        }
    }

    // ---------------- pilota automatico (vetrina del menu e prove) ----------------
    std::vector<int> bisogno, riservato; int oroServe = 0;
    void Espandi(int o, int q, int prof) {
        if (o < 0 || q <= 0 || prof > 7) return;
        int libero = std::max(0, inv[o] - riservato[o]), prendi = std::min(libero, q);
        riservato[o] += prendi; q -= prendi;
        if (q <= 0) return;
        if (ogg[o].tipo == 3 && inv[o] > 0) return;
        bisogno[o] += q;
        for (int i = 0; i < (int)punti.size(); i++) if (punti[i].tipo == 1 && punti[i].oggetto == o && Visibile(punti[i].luogo)) { if (punti[i].attrezzo >= 0 && !Ha(punti[i].attrezzo)) Espandi(punti[i].attrezzo, 1, prof + 1); return; }
        for (int r : RicetteAperte()) if (ric[r].ris == o) { int volte = (q + ric[r].q - 1) / ric[r].q; for (int k = 0; k < 3; k++) if (ric[r].ing[k] >= 0) Espandi(ric[r].ing[k], volte * ric[r].n[k], prof + 1); return; }
        if (InVendita(o)) oroServe += PrezzoCompra(o) * q;
    }
    int Livello(int o) const {   // 0 posseduto, 1 si crea, 2 si compra, 9 impossibile
        if (o < 0 || inv[o] > 0) return 0;
        for (int r : RicetteAperte()) if (ric[r].ris == o) return 1;
        return InVendita(o) ? 2 : 9;
    }
    void Bisogni() {
        bisogno.assign(ogg.size(), 0); riservato.assign(ogg.size(), 0); oroServe = 0;
        std::vector<int> nemiciDaBattere;
        for (size_t m = 0; m < mis.size(); m++) {
            const Missione& q = mis[m];
            if (statoMis[m] != 1) continue;
            if (q.tipo == 0 && q.oggetto >= 0) Espandi(q.oggetto, q.quantita, 0);
            if (q.tipo == 1 && q.bersaglio >= 0 && punti[q.bersaglio].nemico >= 0) nemiciDaBattere.push_back(nem[punti[q.bersaglio].nemico].potenza);
            if (q.tipo == 2 && q.bersaglio >= 0 && punti[q.bersaglio].attrezzo >= 0) { int a = punti[q.bersaglio].attrezzo; Espandi(a, ogg[a].tipo == 3 ? 1 : std::max(1, punti[q.bersaglio].costo), 0); }
        }
        // armi e armature per il nemico piu' forte da battere
        int E = 0; for (int e : nemiciDaBattere) E = std::max(E, e);
        if (E > Potenza()) {
            int ba = Migliore(4), bd = Migliore(5), va = ba >= 0 ? ogg[ba].valore : 0, vd = bd >= 0 ? ogg[bd].valore : 0;
            int sa = -1, sd = -1, costo = 999;
            std::vector<int> armi = { -1 }, arm = { -1 };
            for (size_t o = 0; o < ogg.size(); o++) { if (ogg[o].tipo == 4 && ogg[o].valore > va && Livello((int)o) < 9) armi.push_back((int)o); if (ogg[o].tipo == 5 && ogg[o].valore > vd && Livello((int)o) < 9) arm.push_back((int)o); }
            for (int a : armi) for (int d : arm) {
                int p = forzaBase + (a >= 0 ? ogg[a].valore : va) + (d >= 0 ? ogg[d].valore : vd);
                if (p < E) continue;
                int c = (a >= 0 ? 1 + Livello(a) : 0) + (d >= 0 ? 1 + Livello(d) : 0);
                if (c < costo) { costo = c; sa = a; sd = d; }
            }
            if (sa >= 0) Espandi(sa, 1, 0);
            if (sd >= 0) Espandi(sd, 1, 0);
        }
    }
    int Mancano(int o) const { return o >= 0 && o < (int)bisogno.size() ? bisogno[o] : 0; }
    int Tieni(int o, int ba, int bd) const {   // quanti esemplari il pilota automatico non vende
        const Oggetto& q = ogg[o];
        int tieni = riservato[o];
        if (q.tipo == 3 || q.tipo == 6) tieni = std::max(tieni, 1);
        if ((q.tipo == 4 && o == ba) || (q.tipo == 5 && o == bd)) tieni = std::max(tieni, 1);
        if (q.tipo == 1 || q.tipo == 2) tieni = std::max(tieni, std::min(inv[o], 4));
        return tieni;
    }
    void AutoMercato() {
        Bisogni();
        bool pieno = Usati() >= Posti() - 2;
        int ba = Migliore(4), bd = Migliore(5);
        for (int giro = 0; giro < 2; giro++)
            for (int o : DaVendere()) {
                const Oggetto& q = ogg[o];
                int tieni = Tieni(o, ba, bd);
                bool vendi = pieno || oro < oroServe || q.tipo == 4 || q.tipo == 5;
                if (!vendi) continue;
                while (inv[o] > tieni && (pieno || oro < oroServe + 20 || q.tipo == 4 || q.tipo == 5)) { Vendi(o); pieno = Usati() >= Posti() - 2; }
            }
        for (int o : DaComprare()) while (Mancano(o) > 0 && oro >= PrezzoCompra(o) && CiSta(o)) { Compra(o); bisogno[o]--; }
        bool cibo = false; for (size_t o = 0; o < ogg.size(); o++) if (inv[o] > 0 && ogg[o].tipo == 1) cibo = true;
        if (!cibo) for (int o : DaComprare()) if (ogg[o].tipo == 1) { for (int k = 0; k < 3 && oro >= PrezzoCompra(o) + 10; k++) Compra(o); break; }
        bool cura = false; for (size_t o = 0; o < ogg.size(); o++) if (inv[o] > 0 && ogg[o].tipo == 2) cura = true;
        if (!cura) for (int o : DaComprare()) if (ogg[o].tipo == 2) { if (oro >= PrezzoCompra(o) + 40) Compra(o); break; }
    }
    bool AutoLavori() {
        Bisogni();
        bool fatto = false;
        for (int giro = 0; giro < 30; giro++) {
            bool uno = false;
            for (int r : RicetteAperte()) if (Mancano(ric[r].ris) > 0 && Fattibile(r)) { if (Crea(r)) { uno = fatto = true; Bisogni(); break; } }
            if (!uno) break;
        }
        return fatto;
    }
    Pad Automatico() override {
        Pad pd;
        if (caps.empty() || finito) return pd;
        if (pan != Pan::Niente) {
            if (++autoPausa < 20) return pd;
            autoPausa = 0;
            switch (pan) {
            case Pan::Capitolo: Premi(50); break;
            case Pan::Dialogo: if (dlgModo == 1) Accetta(); else if (dlgModo == 2) Consegna(); else ChiudiDialogo(); break;
            case Pan::Mercato: AutoMercato(); pan = Pan::Niente; break;
            case Pan::Lavori: AutoLavori(); pan = Pan::Niente; break;
            default: pan = Pan::Niente; break;
            }
            return pd;
        }
        if (az != Az::Niente) return pd;
        if (++autoPausa < 6) return pd;
        autoPausa = 0;
        Bisogni();
        // creare quello che serve, se si puo'
        for (int r : RicetteAperte()) if (Mancano(ric[r].ris) > 0 && Fattibile(r)) { Apri(Pan::Lavori); return pd; }
        // mangiare o curarsi
        if (energia <= 2) {
            int best = -1;
            for (size_t o = 0; o < inv.size(); o++) if (inv[o] > riservato[o] && ogg[o].tipo == 1 && (best < 0 || ogg[o].valore < ogg[best].valore)) best = (int)o;
            if (best >= 0) { Usa(best); return pd; }
        }
        int danno = 0;
        for (size_t m = 0; m < mis.size(); m++) if (statoMis[m] == 1 && mis[m].tipo == 1 && mis[m].bersaglio >= 0 && Visibile(punti[mis[m].bersaglio].luogo)) danno = std::max(danno, Danno(nem[punti[mis[m].bersaglio].nemico].potenza));
        if (danno > 0 && vita <= danno) for (size_t o = 0; o < inv.size(); o++) if (inv[o] > riservato[o] && ogg[o].tipo == 2) { Usa((int)o); return pd; }
        bool riposare = energia <= 2 || (danno > 0 && vita <= danno);
        bool pieno = Usati() >= Posti() - 1;
        bool compra = false;
        for (int o : DaComprare()) if (Mancano(o) > 0 && oro >= PrezzoCompra(o)) compra = true;
        bool soldi = oroServe > oro;
        // le localita' raggiungibili e la distanza
        std::vector<int> dist(luoghi.size(), -1), prec(luoghi.size(), -1);
        std::deque<int> c; c.push_back(luogo); dist[luogo] = 0;
        while (!c.empty()) { int l = c.front(); c.pop_front(); for (int d = 0; d < 4; d++) { int n = luoghi[l].uscite[d]; if (Visibile(n) && dist[n] < 0 && Bloccata(l, d) < 0) { dist[n] = dist[l] + 1; prec[n] = l; c.push_back(n); } } }
        float migliore = 1e9; int scelta = -1, sceltaL = -1;
        auto Proponi = [&](int i, int l, float prio) {
            float s = prio * 100 + dist[l] * 35 + (l == luogo && i >= 0 ? Vector2Distance({ ex, ey }, { punti[i].x, punti[i].y }) / 200 : 0);
            if (s < migliore) { migliore = s; scelta = i; sceltaL = l; }
        };
        for (size_t l = 0; l < luoghi.size(); l++) {
            if (dist[l] < 0) continue;
            if (!visitati[l]) Proponi(-1, (int)l, 7);
            if ((int)l == borsaLuogo) Proponi(-2, (int)l, 4);
            for (int i : puntiDi[l]) {
                const Punto& p = punti[i];
                if (!Presente(i)) continue;
                switch (p.tipo) {
                case 2: { int s = Segno(i); if (s == 2) Proponi(i, (int)l, 1); else if (s == 1) Proponi(i, (int)l, 2); break; }
                case 7: if (riposare) Proponi(i, (int)l, 0.5f); break;
                case 4: if (pieno) Proponi(i, (int)l, 1); else if (compra) Proponi(i, (int)l, 3); else if (soldi && oro < oroServe) { int ba = Migliore(4), bd = Migliore(5); for (int o : DaVendere()) if (inv[o] > Tieni(o, ba, bd)) { Proponi(i, (int)l, 5); break; } } break;
                case 3: {
                    const Nemico& n = nem[p.nemico];
                    bool bersaglio = false;
                    for (size_t m = 0; m < mis.size(); m++) if (statoMis[m] == 1 && mis[m].tipo == 1 && mis[m].bersaglio == i) bersaglio = true;
                    if (n.lascia >= 0 && Mancano(n.lascia) > 0 && !pieno) bersaglio = true;
                    if (bersaglio && Potenza() >= n.potenza && vita > Danno(n.potenza)) Proponi(i, (int)l, 3);
                    break;
                }
                case 5: {
                    if (sp[i].fatto) break;
                    bool missione = false;
                    for (size_t m = 0; m < mis.size(); m++) if (statoMis[m] == 1 && mis[m].tipo == 2 && mis[m].bersaglio == i) missione = true;
                    bool puo = p.attrezzo < 0 || Ha(p.attrezzo, ogg[p.attrezzo].tipo == 3 ? 1 : std::max(1, p.costo));
                    if (puo && energia >= 1) Proponi(i, (int)l, missione ? 3 : 6);
                    break;
                }
                case 6: if (!sp[i].fatto && (p.attrezzo < 0 || Ha(p.attrezzo)) && CiSta(p.oggetto)) Proponi(i, (int)l, p.oggetto >= 0 && Mancano(p.oggetto) > 0 ? 3 : 5); break;
                case 1: {
                    if (p.usi > 0 && sp[i].usati >= p.usi) break;
                    if (p.attrezzo >= 0 && !Ha(p.attrezzo)) break;
                    if (!CiSta(p.oggetto)) break;
                    bool utile = Mancano(p.oggetto) > 0;
                    bool vendibile = soldi && p.oggetto >= 0 && ogg[p.oggetto].prezzo >= 2;
                    bool cibo = p.oggetto >= 0 && ogg[p.oggetto].tipo == 1 && inv[p.oggetto] < 3;
                    if (energia < std::max(1, p.costo)) { if (utile) riposare = true; break; }
                    if (utile) Proponi(i, (int)l, 4);
                    else if (cibo && p.costo == 0) Proponi(i, (int)l, 6);
                    else if (vendibile) Proponi(i, (int)l, 6.5f);
                    break;
                }
                }
            }
        }
        if (riposare)   // stanco o ferito: prima si riposa
            for (size_t l = 0; l < luoghi.size(); l++) if (dist[l] >= 0) for (int i : puntiDi[l]) if (punti[i].tipo == 7) Proponi(i, (int)l, -1);
        if (scelta == -1 && sceltaL < 0) return pd;   // niente da fare: si aspetta che le risorse ricrescano
        if (sceltaL == luogo && scelta != -1) {
            if (scelta == -2) { VaiA(borsaX - 40, borsaY + 10); azPunto = -2; azUscita = -1; }
            else Vai(scelta);
            return pd;
        }
        // verso la localita' scelta: la prima uscita del cammino
        int l = sceltaL;
        while (l >= 0 && prec[l] != luogo) l = prec[l];
        for (int d = 0; d < 4; d++) if (luoghi[luogo].uscite[d] == l && l >= 0) { VaiUscita(d); return pd; }
        return pd;
    }

    // ---------------- disegno ----------------
    static const Tex& Ui(int k) {   // icone dell'interfaccia
        std::string key = "u" + std::to_string(k);
        auto it = gSprite.find(key);
        if (it != gSprite.end()) return it->second;
        Tex t; t.s = { 96, 96, 48, 48, 1, 96, 96, 0 };
        t.t = Dipingi(96, 96, [&](K& c) {
            c.translate(48, 48);
            auto Bordo = [&]() { c.strokeStyle(V("#1a1008")); c.lineWidth(3); c.stroke(); };
            switch (k) {
            case 0:   // diario
                c.fillStyle(lg(-30, 0, 30, 0, { "#8a3a2a", "#5a2014" })); c.beginPath(); c.rect(-30, -34, 60, 68); c.fill(); Bordo();
                c.fillStyle(V("#f0e0b8")); c.beginPath(); c.rect(-24, -28, 52, 58); c.fill();
                for (int i = 0; i < 6; i++) Linea(c, -16, -18 + i * 9.f, 20, -18 + i * 9.f, "#8a7050", 2);
                c.fillStyle(V("#c8a040")); c.beginPath(); c.rect(-30, -34, 8, 68); c.fill(); break;
            case 1:   // mappa
                for (int i = 0; i < 3; i++) { c.fillStyle(V(i % 2 ? "#d8c08a" : "#e8d4a0")); Via(c, { { -36 + i * 24.f, -26 + (i % 2) * 6.f }, { -12 + i * 24.f, -26 + ((i + 1) % 2) * 6.f }, { -12 + i * 24.f, 30 + ((i + 1) % 2) * 6.f }, { -36 + i * 24.f, 30 + (i % 2) * 6.f } }); c.fill(); Bordo(); }
                c.strokeStyle(V("#b03020")); c.lineWidth(3); c.beginPath(); c.moveTo(-26, 18); c.lineTo(-6, 0); c.lineTo(10, 10); c.lineTo(24, -12); c.stroke();
                Linea(c, 18, -18, 30, -6, "#b03020", 4); Linea(c, 30, -18, 18, -6, "#b03020", 4); break;
            case 2:   // lavori (martello)
                c.rotate(-0.7f);
                c.fillStyle(V("#8a5a30")); c.beginPath(); c.rect(-5, -10, 10, 50); c.fill(); Bordo();
                c.fillStyle(lg(0, -34, 0, -10, { "#d8e0e8", "#7a8490" })); c.beginPath(); c.rect(-26, -34, 52, 24); c.fill(); Bordo(); break;
            case 3: c.translate(-48, -48); DipingiIcona(c, I_ZAINO, "#8a5a30"); break;
            case 4: c.glow(8, "#ffd040"); c.fillStyle(rgs(-8, -8, 2, 34, { { 0, "#fff0a0" }, { 0.6f, "#f0c030" }, { 1, "#a07010" } })); c.beginPath(); c.arc(0, 0, 30, 0, TAU); c.fill(); c.noGlow(); Bordo();
                c.strokeStyle(V("#a07010")); c.lineWidth(3); c.beginPath(); c.arc(0, 0, 21, 0, TAU); c.stroke(); break;
            case 5: c.fillStyle(rgs(-10, -10, 2, 40, { { 0, "#ff9090" }, { 1, "#b01020" } })); c.beginPath(); c.moveTo(0, 32); c.bezierCurveTo(-44, 0, -30, -36, 0, -16); c.bezierCurveTo(30, -36, 44, 0, 0, 32); c.fill(); Bordo(); break;
            case 6: c.glow(8, "#80d0ff"); Pieno(c, { { 6, -38 }, { -22, 6 }, { -2, 6 }, { -8, 38 }, { 22, -8 }, { 2, -8 } }, "#7ac8ff"); c.noGlow(); Via(c, { { 6, -38 }, { -22, 6 }, { -2, 6 }, { -8, 38 }, { 22, -8 }, { 2, -8 } }); Bordo(); break;
            case 7: c.translate(-48, -48); DipingiIcona(c, I_SPADA, "#d8e0e8"); break;
            case 8:   // freccia dorata
                c.glow(10, "rgba(0,0,0,0.6)"); Pieno(c, { { 34, 0 }, { -22, -26 }, { -10, 0 }, { -22, 26 } }, "#f0c830"); c.noGlow();
                Pieno(c, { { 30, 0 }, { -14, -18 }, { -6, 0 } }, "#fff0a0"); Via(c, { { 34, 0 }, { -22, -26 }, { -10, 0 }, { -22, 26 } }); c.strokeStyle(V("#5a3a00")); c.lineWidth(2.5f); c.stroke(); break;
            case 9: c.translate(-48, -48); DipingiIcona(c, I_ZAINO, "#c8a040"); break;
            }
        });
        return gSprite[key] = t;
    }
    const Tex& Ritratto(int punto) const {   // busto in un cerchio (eroe = -1)
        Look o = punto < 0 ? lookEroe : LookPersona(punti[punto].forma, Esa(punti[punto].colore), (unsigned)punto % 5);
        if (punto >= 0 && punti[punto].forma == F_MERCANTE) o = LookPersona(F_MERCANTE, Esa(punti[punto].colore), (unsigned)punto % 5);
        std::string key = "r" + o.veste + o.capelli + o.pelle + std::to_string(o.copricapo) + o.mantello + (o.barba ? "b" : "");
        auto it = gSprite.find(key);
        if (it != gSprite.end()) return it->second;
        Tex t; t.s = { 160, 160, 80, 80, 1, 160, 160, 0 };
        t.t = Dipingi(160, 160, [&](K& c) {
            c.fillStyle(rgs(80, 60, 10, 90, { { 0, "#f0e0b0" }, { 1, "#a88a58" } })); c.beginPath(); c.arc(80, 80, 78, 0, TAU); c.fill();
            c.save(); c.beginPath(); c.arc(80, 80, 74, 0, TAU); c.clip();
            c.translate(76, 330); c.scale(3.0f, 3.0f);
            Look q = o; q.attrezzo = 0;
            Figura(c, q, -1, 0);
            c.restore();
            c.strokeStyle(V("#c8a050")); c.lineWidth(5); c.beginPath(); c.arc(80, 80, 76, 0, TAU); c.stroke();
        });
        return gSprite[key] = t;
    }
    Texture2D SfondoScena() {
        const Luogo& L = luoghi[luogo];
        InfoScena in;
        in.scena = L.scena; in.erba = L.erba; in.terreno = L.terreno; in.seme = (unsigned)L.seme + 1;
        for (int d = 0; d < 4; d++) in.uscite[d] = L.uscite[d] >= 0;
        for (int i : puntiDi[luogo]) { in.liberi.push_back({ punti[i].x, punti[i].y - 30 }); if (punti[i].forma == F_PONTE) in.canale = punti[i].y; }
        char k[200];
        std::snprintf(k, sizeof k, "%d|%s|%s|%u|%d%d%d%d|%.0f|%zu", in.scena, Esa(in.erba).c_str(), Esa(in.terreno).c_str(), in.seme, in.uscite[0], in.uscite[1], in.uscite[2], in.uscite[3], in.canale, in.liberi.size());
        std::string key = k;
        for (auto& q : in.liberi) key += "," + std::to_string((int)q.x) + ":" + std::to_string((int)q.y);
        auto it = gScene.find(key);
        if (it != gScene.end()) return it->second;
        Texture2D t = Dipingi(1920, 1080, [&](K& c) { DipingiScena(c, in); }, false);
        gScene[key] = t; gOrdineScene.push_back(key);
        while (gOrdineScene.size() > 6) { UnloadTexture(gScene[gOrdineScene.front()]); gScene.erase(gOrdineScene.front()); gOrdineScene.pop_front(); }
        return t;
    }
    void Pannello(Rectangle r, float alfa = 1) {
        if (!gPergamena.id)
            gPergamena = Dipingi(1000, 700, [](K& c) {
                R2 rr(77);
                c.fillStyle(V("#e8d6aa")); c.fillRect(0, 0, 1000, 700);
                for (int i = 0; i < 5000; i++) Dab(c, rr.F(0, 1000), rr.F(0, 700), rr.F(2, 9), Hsl(38, rr.F(30, 50), rr.F(62, 82), 0.18f), rr.F(0, 3));
                c.fillStyle(rgs(500, 350, 300, 640, { { 0, "rgba(120,80,30,0)" }, { 1, "rgba(120,80,30,0.45)" } })); c.fillRect(0, 0, 1000, 700);
            });
        DrawRectangleRounded({ r.x + 8, r.y + 10, r.width, r.height }, 0.03f, 8, ConAlfa(BLACK, 0.4f * alfa));
        DrawTexturePro(gPergamena, { 0, 0, 1000, 700 }, r, { 0, 0 }, 0, ConAlfa(WHITE, alfa));
        DrawRectangleLinesEx(r, 6, ConAlfa(Hex(0x5a3a1a), alfa));
        DrawRectangleLinesEx({ r.x + 9, r.y + 9, r.width - 18, r.height - 18 }, 2, ConAlfa(Hex(0xa8844a), alfa));
    }
    void Bottone(const Btn& b, bool sopraB) {
        Color fondo = !b.ok ? Hex(0x8a8070) : sopraB ? Hex(0x9a6a32) : Hex(0x6a4626);
        DrawRectangleRounded(b.r, 0.25f, 6, fondo);
        DrawRectangleRoundedLines(b.r, 0.25f, 6, Hex(0xe8c870));
        if (!b.t.empty()) Scritta(b.t, b.r.x + b.r.width / 2, b.r.y + b.r.height / 2, (int)std::min(34.f, b.r.height * 0.55f), b.ok ? Hex(0xfff0c8) : Hex(0xd8d0c0), 0);
    }
    float TestoInScatola(const std::string& t, float x, float y, float w, int dim, Color c) {   // testo a capo
        std::vector<std::string> righe; std::string riga, parola;
        auto Larg = [&](const std::string& s) { return dim >= 22 ? ::Ui::LarghezzaForte(s, dim) : ::Ui::Larghezza(s, dim); };
        auto Metti = [&]() { if (parola.empty()) return; std::string prova = riga.empty() ? parola : riga + " " + parola; if (Larg(prova) > w && !riga.empty()) { righe.push_back(riga); riga = parola; } else riga = prova; parola.clear(); };
        for (char ch : t) { if (ch == ' ') Metti(); else if (ch == '\n') { Metti(); righe.push_back(riga); riga.clear(); } else parola += ch; }
        Metti(); if (!riga.empty()) righe.push_back(riga);
        for (size_t i = 0; i < righe.size(); i++) Scritta(righe[i], x, y + i * dim * 1.25f, dim, c, -1, false);
        return righe.size() * dim * 1.25f;
    }

    void Disegna() override {
        if (caps.empty()) { ClearBackground(BLACK); return; }
        const Luogo& L = luoghi[luogo];
        float sx = tScossa > 0 ? Vis(-5, 5) : 0, sy = tScossa > 0 ? Vis(-4, 4) : 0;
        DrawTexture(SfondoScena(), (int)sx, (int)sy, WHITE);
        // acqua che luccica e lucciole
        if (L.scena == 7 || L.scena == 3) for (int i = 0; i < 18; i++) { float x = std::fmod(i * 137.f + tempo * 30, 1920.f), y = (L.scena == 7 ? 60 : 180) + std::fmod(i * 53.f, 110.f); Bagliore(x, y, 14 + 6 * std::sin(tempo * 3 + i), ConAlfa(WHITE, 0.25f)); }
        // --- elementi in ordine di profondita' ---
        struct D { float y; int tipo, i; };
        std::vector<D> lista;
        for (int i : puntiDi[luogo]) if (Presente(i)) lista.push_back({ punti[i].y, 0, i });
        lista.push_back({ ey, 1, -1 });
        if (BorsaQui()) lista.push_back({ borsaY, 2, -1 });
        std::sort(lista.begin(), lista.end(), [](const D& a, const D& b) { return a.y < b.y; });
        for (auto& d : lista) {
            if (d.tipo == 1) { DisegnaEroe(sx, sy); continue; }
            if (d.tipo == 2) { Icona(I_ZAINO, Hex(0xc8a040), borsaX + sx, borsaY - 22 + sy, 54); Bagliore(borsaX, borsaY - 24, 50 + 10 * std::sin(tempo * 4), ConAlfa(Hex(0xffe080), 0.4f)); continue; }
            int i = d.i;
            const Punto& p = punti[i];
            bool evid = (i == sopra);
            if (p.tipo == 3) {
                const Nemico& n = nem[p.nemico];
                float k = n.grande ? 1.35f : 1, resp = 1 + 0.02f * std::sin(tempo * 3 + i);
                bool flip = ex < p.x;
                float lx = 0;
                if (az == Az::Combatte && azPunto == i) lx = (flip ? 1 : -1) * 10 * std::fabs(std::sin(azT * 7));
                const Tex& t = SpriteNemico(n.forma, n.col);
                Piede(t, p.x + sx + lx, p.y + sy, k * resp, flip);
                if (evid) { BeginBlendMode(BLEND_ADDITIVE); Piede(t, p.x + sx + lx, p.y + sy, k * resp, flip, Color{ 255, 230, 200, 60 }); EndBlendMode(); }
                if (n.forma == N_SPIRITO) Bagliore(p.x, p.y - 80 * k, 120 * k, ConAlfa(n.col, 0.3f));
                // potenza sopra la testa
                Spec s = SpecNemico(n.forma);
                bool forte = Potenza() < n.potenza;
                Scritta(std::to_string(n.potenza), p.x + sx, p.y - s.hh * k - 26 + sy, 30, forte ? Hex(0xff6a5a) : Hex(0xfff0c0), 0);
                continue;
            }
            int stato = 0;
            if (p.tipo == 1 && p.usi > 0 && sp[i].usati >= p.usi) stato = 1;
            if (p.tipo == 5 && sp[i].fatto) stato = 2;
            if (p.tipo == 6 && sp[i].fatto) stato = 1;
            const Tex& t = SpriteForma(p.forma, p.colore, (unsigned)i, stato);
            bool flip = p.forma <= F_BAMBINO && p.forma != F_MERCANTE && ex < p.x;
            float scala = 1;
            if (p.tipo == 1 && az == Az::Lavora && azPunto == i) scala = 1 + 0.02f * std::sin(azT * 30);
            Piede(t, p.x + sx, p.y + sy, scala, flip);
            if (evid) { BeginBlendMode(BLEND_ADDITIVE); Piede(t, p.x + sx, p.y + sy, scala, flip, Color{ 255, 230, 200, 55 }); EndBlendMode(); }
            // dettagli vivi: fumo, fuoco, pale del mulino
            if ((p.forma == F_CASA_PAGLIA || p.forma == F_CASA_TEGOLE) && Vis() < 0.08f) {
                Vector2 c = p.forma == F_CASA_PAGLIA ? Comignolo(280, 180, 140, 120) : Comignolo(300, 200, 160, 130);
                Particella q; q.p = { p.x + c.x, p.y + c.y }; q.v = { Vis(5, 20), Vis(-50, -30) }; q.vita = q.durata = Vis(2.5f, 3.5f); q.r = Vis(8, 14); q.c = ConAlfa(Hex(0xe8e8e0), 0.35f); q.attrito = 1; q.additiva = false;
                part.Aggiungi(q);
            }
            if (p.forma == F_FALO) {
                for (int k = 0; k < 2; k++) { Particella q; q.p = { p.x + Vis(-14, 14), p.y - 14 }; q.v = { Vis(-10, 10), Vis(-90, -50) }; q.vita = q.durata = Vis(0.4f, 0.8f); q.r = Vis(5, 9); q.c = Vis() < 0.5f ? Hex(0xffa030) : Hex(0xffe070); part.Aggiungi(q); }
                Bagliore(p.x, p.y - 30, 110, ConAlfa(Hex(0xff9030), 0.45f));
            }
            if (p.forma == F_MULINO) {
                if (!gPale.id) gPale = Dipingi(420, 420, [](K& c) { c.translate(210, 210); for (int k = 0; k < 4; k++) { c.save(); c.rotate(k * TAU / 4); c.fillStyle(V("#5a3a1e")); c.fillRect(-5, -200, 10, 190); c.fillStyle(V("rgba(240,230,210,0.9)")); c.fillRect(6, -196, 46, 150); for (int q = 0; q < 6; q++) Linea(c, 6, -190 + q * 25.f, 52, -190 + q * 25.f, "#8a6a40", 2); c.restore(); } });
                Sprite(gPale, p.x + sx, p.y - 330 + sy, 0.8f, tempo * 25);
            }
            if (p.tipo == 2 || (p.tipo == 4)) {
                int s = p.tipo == 2 ? Segno(i) : 0;
                if (s) {
                    float b = std::sin(tempo * 4 + i) * 5;
                    Scritta(s == 2 ? "?" : "!", p.x + sx, p.y - 120 + b + sy, 54, s == 2 ? Hex(0xfff070) : Hex(0xffd820), 0);
                }
            }
            if (p.tipo == 6 && !sp[i].fatto) Bagliore(p.x, p.y - 30, 40 + 6 * std::sin(tempo * 3), ConAlfa(Hex(0xffe090), 0.3f));
        }
        // frecce dorate
        for (int d = 0; d < 4; d++) {
            if (UscitaVisibile(d) < 0) continue;
            bool chiusa = Bloccata(luogo, d) >= 0;
            float b = 6 * std::sin(tempo * 3);
            Vector2 q = FRECCIA[d];
            Vector2 dir = d == 0 ? Vector2{ 0, -1 } : d == 1 ? Vector2{ 1, 0 } : d == 2 ? Vector2{ 0, 1 } : Vector2{ -1, 0 };
            float rot = d == 0 ? -90.f : d == 1 ? 0.f : d == 2 ? 90.f : 180.f;
            float s = sopraUscita == d ? 0.78f : 0.66f;
            Sprite(Ui(8).t, q.x + dir.x * b, q.y + dir.y * b, s, rot, chiusa ? Color{ 150, 140, 130, 200 } : WHITE);
        }
        // ora del giorno
        DisegnaLuce(L.ora, L.scena);
        part.Disegna();
        for (auto& n : numeri) Scritta(n.t, n.p.x, n.p.y, 30, ConAlfa(n.c, std::min(1.f, n.vita * 1.5f)), 0);
        DisegnaInterfaccia();
        // viaggio e riposo: dissolvenze
        if (az == Az::Viaggia) { float a = azT < 0.35f ? azT / 0.35f : std::max(0.f, (0.7f - azT) / 0.35f); DrawRectangle(0, 0, 1920, 1080, ConAlfa(BLACK, a)); }
        if (az == Az::Riposa) {
            float a = std::sin(std::min(1.f, azT / azDurata) * PI_);
            DrawRectangle(0, 0, 1920, 1080, ConAlfa(Hex(0x05040a), a * 0.92f));
            Scritta("Riposi...", 960, 520, 64, ConAlfa(Hex(0xf0e0b0), a), 0);
        }
    }
    void DisegnaEroe(float sx, float sy) {
        Look o = lookEroe;
        int pass = 0, br = 0;
        float lx = 0;
        if (az == Az::Cammina) pass = passo;
        if (az == Az::Lavora && azPunto >= 0) { o.attrezzo = ArneseDi(azPunto); br = std::fmod(azT, 0.36f) < 0.18f ? 1 : 2; if (o.attrezzo == 8 || o.attrezzo == 7) br = 0; }
        if (az == Az::Combatte && azPunto >= 0) { o.attrezzo = ArneseDi(azPunto); br = std::fmod(azT, 0.42f) < 0.2f ? 1 : 2; lx = (versoSx ? -1 : 1) * 12 * std::fabs(std::sin(azT * 7.5f)); }
        int a = Migliore(4);
        if (o.attrezzo && a >= 0 && az == Az::Combatte) o.colAttrezzo = Esa(ogg[a].col);
        const Tex& t = SpriteEroe(o, pass, br);
        Piede(t, ex + sx + lx, ey + sy, 1, versoSx);
        if (Qui().ora == 3 || Qui().scena == 11) Bagliore(ex, ey - 40, 220, ConAlfa(Hex(0xffc070), 0.28f));
    }
    const Luogo& Qui() const { return luoghi[luogo]; }
    void DisegnaLuce(int ora, int scena) {
        if (ora == 1) { DrawRectangleGradientV(0, 0, 1920, 1080, ConAlfa(Hex(0xc8e0ff), 0.10f), ConAlfa(Hex(0xfff0d0), 0.04f)); }
        if (ora == 2) {
            BeginBlendMode(BLEND_MULTIPLIED); DrawRectangle(0, 0, 1920, 1080, Color{ 255, 206, 168, 255 }); EndBlendMode();
            Bagliore(1750, 80, 700, ConAlfa(Hex(0xff8a30), 0.25f));
        }
        if (ora == 3) {
            BeginBlendMode(BLEND_MULTIPLIED); DrawRectangle(0, 0, 1920, 1080, Color{ 92, 104, 168, 255 }); EndBlendMode();
            for (int i : puntiDi[luogo]) {
                const Punto& p = punti[i];
                if (p.forma == F_CASA_PAGLIA || p.forma == F_CASA_TEGOLE || p.forma == F_TORRE) Bagliore(p.x - 40, p.y - 110, 200, ConAlfa(Hex(0xffb050), 0.28f));
                if (p.forma == F_CRISTALLI || p.forma == F_FARO) Bagliore(p.x, p.y - 60, 140, ConAlfa(Hex(0x80d0ff), 0.3f));
            }
            Bagliore(ex, ey - 40, 260, ConAlfa(Hex(0xffc070), 0.3f));
            for (int i = 0; i < 14; i++) { float x = std::fmod(i * 331.f + std::sin(tempo * 0.4f + i) * 80, 1920.f), y = 200 + std::fmod(i * 197.f, 700.f) + std::cos(tempo * 0.6f + i) * 40; Bagliore(x, y, 16, ConAlfa(Hex(0xd0ff80), 0.5f + 0.4f * std::sin(tempo * 3 + i))); }
        }
        if (ora == 4 || scena == 5) {
            float f = ora == 4 ? 1 : 0.5f;
            for (int i = 0; i < 9; i++) {
                float x = std::fmod(i * 420.f + tempo * (14 + i * 3), 2400.f) - 240, y = 180 + i * 95.f;
                DrawEllipse((int)x, (int)y, 420, 70, ConAlfa(Hex(0xe8f0f0), 0.09f * f));
            }
        }
        if (scena == 11) { BeginBlendMode(BLEND_MULTIPLIED); DrawRectangle(0, 0, 1920, 1080, Color{ 150, 140, 170, 255 }); EndBlendMode(); Bagliore(ex, ey - 40, 300, ConAlfa(Hex(0xffc070), 0.3f)); }
    }

    std::string Suggerimento(std::string& riga2, Color& c2) const {
        riga2.clear(); c2 = Hex(0xf0e6c8);
        if (sopraBottone >= 0 && sopraBottone < 4) { const char* n[4] = { "Diario delle missioni", "Mappa del capitolo", "Lavori: crea oggetti", "Zaino e equipaggiamento" }; return n[sopraBottone]; }
        if (sopraBottone >= 10) { auto r = Rapidi(); int o = r[sopraBottone - 10]; riga2 = ogg[o].tipo == 1 ? "Mangia: +" + std::to_string(ogg[o].valore) + " energia" : "Bevi: +" + std::to_string(ogg[o].valore) + " vita"; return ogg[o].nome; }
        if (sopraUscita >= 0) {
            int ost = Bloccata(luogo, sopraUscita);
            if (ost >= 0) { riga2 = "Bloccata: " + punti[ost].nome; c2 = Hex(0xffa090); }
            return "Verso: " + luoghi[UscitaVisibile(sopraUscita)].nome;
        }
        if (sopra == -2) { riga2 = "Raccogli"; return "Borsa di monete"; }
        if (sopra < 0) return "";
        const Punto& p = punti[sopra];
        switch (p.tipo) {
        case 1:
            if (p.usi > 0 && sp[sopra].usati >= p.usi) { riga2 = sp[sopra].attesa > 0 ? "Ricresce tra " + std::to_string((int)std::ceil(sp[sopra].attesa)) + " s" : "Esaurito"; c2 = Hex(0xc0b8a8); }
            else if (p.attrezzo >= 0 && !Ha(p.attrezzo)) { riga2 = "Serve: " + ogg[p.attrezzo].nome; c2 = Hex(0xffa090); }
            else { riga2 = (p.oggetto >= 0 ? "+" + std::to_string(p.quantita) + " " + ogg[p.oggetto].nome : std::string("Raccogli")) + (p.costo ? "   energia -" + std::to_string(p.costo) : ""); if (energia < p.costo) c2 = Hex(0xffa090); }
            return p.nome;
        case 2: { int s = Segno(sopra); riga2 = s == 2 ? "Consegna la missione" : s == 1 ? "Nuova missione!" : "Parla"; if (s) c2 = Hex(0xffe070); return p.nome; }
        case 3: {
            const Nemico& n = nem[p.nemico];
            if (Potenza() < n.potenza) { riga2 = "Potenza " + std::to_string(n.potenza) + " (la tua " + std::to_string(Potenza()) + "): troppo forte!"; c2 = Hex(0xffa090); }
            else { riga2 = "Combatti: perderai circa " + std::to_string(Danno(n.potenza)) + " vita"; if (vita <= Danno(n.potenza)) c2 = Hex(0xffa090); }
            return n.nome;
        }
        case 4: riga2 = "Compra e vendi"; return p.nome;
        case 5:
            if (sp[sopra].fatto) { riga2 = "Superato"; return p.nome; }
            if (p.attrezzo >= 0) { bool a = ogg[p.attrezzo].tipo == 3; riga2 = "Serve: " + (a ? "" : std::to_string(std::max(1, p.costo)) + " ") + ogg[p.attrezzo].nome; if (!Ha(p.attrezzo, a ? 1 : std::max(1, p.costo))) c2 = Hex(0xffa090); }
            return p.nome;
        case 6: riga2 = sp[sopra].fatto ? "Vuoto" : p.attrezzo >= 0 && !Ha(p.attrezzo) ? "Chiuso: serve " + ogg[p.attrezzo].nome : "Apri"; return p.nome;
        case 7: riga2 = "Riposa: vita ed energia al massimo" + (p.costo ? " (" + std::to_string(p.costo) + " oro)" : std::string("")); return p.nome;
        }
        return p.nome;
    }
    void DisegnaInterfaccia() {
        // cornice di legno con i quattro pulsanti
        DrawRectangleLinesEx({ 0, 0, 1920, 1080 }, 14, Hex(0x3a2a18));
        DrawRectangleLinesEx({ 14, 14, 1892, 1052 }, 4, Hex(0xa8844a));
        for (int k = 0; k < 4; k++) {
            Vector2 a = Angolo(k);
            bool s = sopraBottone == k && pan == Pan::Niente;
            DrawCircleV(a, 38, Hex(0x3a2a18)); DrawCircleLines((int)a.x, (int)a.y, 38, Hex(0xc8a050)); DrawCircleLines((int)a.x, (int)a.y, 34, Hex(0x8a6a30));
            if (s) Bagliore(a.x, a.y, 60, ConAlfa(Hex(0xffe0a0), 0.35f));
            Sprite(Ui(k).t, a.x, a.y, 0.6f);
        }
        // missioni attive
        float y = 96;
        int n = 0;
        for (size_t m = 0; m < mis.size() && n < 4; m++) {
            if (statoMis[m] != 1 && statoMis[m] != 2) continue;
            bool ok = Pronta((int)m);
            DrawRectangleRounded({ 30, y - 6, 560, 64 }, 0.2f, 6, ConAlfa(Hex(0x1e140a), 0.72f));
            DrawRectangleRoundedLines({ 30, y - 6, 560, 64 }, 0.2f, 6, ConAlfa(Hex(0xa8844a), 0.8f));
            Scritta((ok ? "? " : "! ") + mis[m].nome, 46, y + 12, 24, ok ? Hex(0xfff070) : Hex(0xf0d890), -1, false);
            Scritta(Progresso((int)m), 46, y + 40, 18, Hex(0xe8e0d0), -1, false);
            y += 72; n++;
        }
        // barra in basso: ritratto, localita', oro, vita, energia, forza, zaino, cibo
        DrawRectangleRounded(BARRA, 0.18f, 8, ConAlfa(Hex(0x1e140a), 0.86f));
        DrawRectangleRoundedLines(BARRA, 0.18f, 8, Hex(0xa8844a));
        DrawTexturePro(Ritratto(-1).t, { 0, 0, 160, 160 }, { BARRA.x - 20, BARRA.y - 30, 150, 150 }, { 0, 0 }, 0, WHITE);
        Scritta(luoghi[luogo].nome, BARRA.x + 150, BARRA.y + 30, 32, Hex(0xf0d890), -1);
        Scritta("Cap. " + std::to_string(cap + 1) + " - " + caps[cap].nome, BARRA.x + 150, BARRA.y + 62, 18, Hex(0xc8b890), -1, false);
        float x = BARRA.x + 160, yy = BARRA.y + 96;
        auto Voce = [&](int ic, const std::string& t, Color c) { Sprite(Ui(ic).t, x + 14, yy, 0.34f); Scritta(t, x + 36, yy + 1, 26, c, -1); x += 44 + ::Ui::LarghezzaForte(t, 26) + 26; };
        Voce(4, std::to_string(oro), Hex(0xffe080));
        Voce(5, std::to_string(vita) + "/" + std::to_string(vitaMax), vita <= 3 ? Hex(0xff8a7a) : Hex(0xf0e6c8));
        Voce(6, std::to_string(energia) + "/" + std::to_string(energiaMax), energia <= 2 ? Hex(0xff8a7a) : Hex(0xf0e6c8));
        Voce(7, std::to_string(Potenza()), Hex(0xf0e6c8));
        Voce(3, std::to_string(Usati()) + "/" + std::to_string(Posti()), Usati() >= Posti() ? Hex(0xff8a7a) : Hex(0xf0e6c8));
        auto r = Rapidi();
        for (size_t k = 0; k < 5; k++) {
            Rectangle q = Rapido((int)k);
            DrawRectangleRounded(q, 0.2f, 4, ConAlfa(Hex(0x3a2a18), 0.9f));
            DrawRectangleRoundedLines(q, 0.2f, 4, sopraBottone == 10 + (int)k ? Hex(0xffe080) : Hex(0x8a6a30));
            if (k < r.size()) { Icona(ogg[r[k]].icona, ogg[r[k]].col, q.x + 31, q.y + 30, 54); Scritta(std::to_string(inv[r[k]]), q.x + 56, q.y + 50, 20, WHITE, 1); }
        }
        // oggetti in volo verso lo zaino
        for (auto& v : voli) {
            float t = v.t, e = t * t * (3 - 2 * t);
            Vector2 p = { v.a.x + (v.b.x - v.a.x) * e, v.a.y + (v.b.y - v.a.y) * e - std::sin(t * PI_) * 160 };
            Icona(ogg[v.o].icona, ogg[v.o].col, p.x, p.y, 64 * (1 - 0.3f * t));
        }
        // suggerimento accanto al puntatore
        if (pan == Pan::Niente && az != Az::Viaggia) {
            std::string r2; Color c2;
            std::string r1 = Suggerimento(r2, c2);
            if (!r1.empty()) {
                float w = (float)std::max(::Ui::LarghezzaForte(r1, 26), ::Ui::Larghezza(r2, 20)) + 36;
                float bx = std::min(cur.x + 28, 1900 - w), by = std::max(20.f, cur.y - 90);
                DrawRectangleRounded({ bx, by, w, r2.empty() ? 46.f : 78.f }, 0.2f, 6, ConAlfa(Hex(0x1e140a), 0.9f));
                DrawRectangleRoundedLines({ bx, by, w, r2.empty() ? 46.f : 78.f }, 0.2f, 6, Hex(0xa8844a));
                Scritta(r1, bx + 18, by + 24, 26, Hex(0xf0d890), -1);
                if (!r2.empty()) Scritta(r2, bx + 18, by + 56, 20, c2, -1, false);
            }
        }
        // avvisi
        for (size_t i = 0; i < avvisi.size(); i++) {
            const Avviso& a = avvisi[i];
            float al = std::min(1.f, a.vita * 2);
            float w = (float)::Ui::LarghezzaForte(a.t, 28) + 60, yA = 150 + i * 62.f;
            DrawRectangleRounded({ 960 - w / 2, yA - 26, w, 52 }, 0.4f, 8, ConAlfa(Hex(0x1e140a), 0.8f * al));
            Scritta(a.t, 960, yA, 28, ConAlfa(a.c, al), 0);
        }
        DisegnaFinestra();
        if (padUsato) { DrawCircleLines((int)cur.x, (int)cur.y, 18, Hex(0xffe080)); DrawCircleLines((int)cur.x, (int)cur.y, 17, Hex(0x3a2a18)); DrawCircleV(cur, 4, Hex(0xffe080)); }
    }

    // ---------------- finestre: disegno ----------------
    std::string Descrizione(int o) const {
        const Oggetto& q = ogg[o];
        switch (q.tipo) {
        case 1: return "Cibo: +" + std::to_string(q.valore) + " energia";
        case 2: return "Cura: +" + std::to_string(q.valore) + " vita";
        case 3: return "Attrezzo";
        case 4: return "Arma: potenza +" + std::to_string(q.valore);
        case 5: return "Armatura: potenza +" + std::to_string(q.valore);
        case 6: return "Zaino: +" + std::to_string(q.valore) + " posti";
        case 7: return "Oggetto di missione";
        }
        return "Risorsa";
    }
    void Titolo(const std::string& t, float y) { Scritta(t, FIN.x + FIN.width / 2, y, 46, Hex(0x4a2a10), 0, false); }
    void Riquadro(Rectangle r, bool sopraR, bool ok) {
        DrawRectangleRounded(r, 0.2f, 6, sopraR ? ConAlfa(Hex(0xfff4d0), 0.85f) : ConAlfa(Hex(0xf6e8c4), ok ? 0.55f : 0.3f));
        DrawRectangleRoundedLines(r, 0.2f, 6, sopraR ? Hex(0xb07a30) : ConAlfa(Hex(0x8a6a40), 0.6f));
    }
    void Fumetto(const std::string& r1, const std::string& r2) {   // descrizione accanto al puntatore, dentro le finestre
        float w = (float)std::max(::Ui::LarghezzaForte(r1, 24), ::Ui::Larghezza(r2, 20)) + 36;
        float bx = std::min(cur.x + 26, 1900 - w), by = std::max(20.f, cur.y - 84);
        DrawRectangleRounded({ bx, by, w, 72 }, 0.2f, 6, ConAlfa(Hex(0x1e140a), 0.92f));
        DrawRectangleRoundedLines({ bx, by, w, 72 }, 0.2f, 6, Hex(0xa8844a));
        Scritta(r1, bx + 18, by + 22, 24, Hex(0xf0d890), -1);
        Scritta(r2, bx + 18, by + 52, 20, Hex(0xe8e0d0), -1, false);
    }
    void DisegnaFinestra() {
        if (pan == Pan::Niente) return;
        auto b = Bottoni();
        int sotto = BottoneSotto(b);
        float al = std::min(1.f, tPan * 5);
        const Color INK = Hex(0x3a2210), INK2 = Hex(0x6a4a2a);
        float x = FIN.x, y = FIN.y, w = FIN.width;
        if (pan != Pan::Dialogo) DrawRectangle(0, 0, 1920, 1080, ConAlfa(BLACK, 0.45f * al));
        int tip = -1;   // oggetto sotto il puntatore (per la descrizione)
        switch (pan) {
        case Pan::Dialogo: {
            Rectangle d = DlgRett();
            Pannello(d, al);
            if (dlgPunto >= 0) {
                DrawTexturePro(Ritratto(dlgPunto).t, { 0, 0, 160, 160 }, { d.x + 24, d.y + 24, 180, 180 }, { 0, 0 }, 0, WHITE);
                Scritta(punti[dlgPunto].nome, d.x + 114, d.y + 230, 26, INK, 0, false);
            }
            float tx = d.x + 240, ty = d.y + 40;
            if (dlgMis >= 0 && dlgModo != 0) { Scritta(mis[dlgMis].nome, tx, ty, 32, Hex(0x8a2a10), -1, false); ty += 50; }
            ty += TestoInScatola(dlgTesto, tx, ty, d.width - 280, 26, INK) + 10;
            if (dlgMis >= 0 && (dlgModo == 1 || dlgModo == 2) && !dlgPremio.empty()) Scritta("Premio: " + dlgPremio, tx, d.y + d.height - 54, 24, Hex(0x7a5a10), -1, false);
            else if (dlgMis >= 0 && dlgModo == 0) Scritta(Progresso(dlgMis), tx, d.y + d.height - 54, 22, INK2, -1, false);
            break;
        }
        case Pan::Mercato: {
            Pannello(FIN, al);
            Titolo(dlgPunto >= 0 ? punti[dlgPunto].nome : std::string("Mercato"), y + 46);
            Scritta("Compra", x + 40, y + 108, 30, INK, -1, false);
            Scritta("Vendi", x + 740, y + 108, 30, INK, -1, false);
            Sprite(Ui(4).t, x + 60, y + 52, 0.4f); Scritta(std::to_string(oro) + " oro", x + 88, y + 52, 30, Hex(0x7a5a10), -1, false);
            Scritta("Posti nello zaino: " + std::to_string(Usati()) + "/" + std::to_string(Posti()), x + 1000, y + 108, 22, INK2, -1, false);
            for (auto& q : b) {
                if (q.id < 100 || q.id >= 500) continue;
                bool compra = q.id < 300; int o = q.id - (compra ? 100 : 300);
                Riquadro(q.r, sotto == q.id, q.ok);
                Icona(ogg[o].icona, ogg[o].col, q.r.x + 28, q.r.y + 24, 42);
                Scritta(ogg[o].nome, q.r.x + 60, q.r.y + 24, 24, q.ok ? INK : INK2, -1, false);
                if (inv[o] > 0) Scritta("x" + std::to_string(inv[o]), q.r.x + 420, q.r.y + 24, 20, INK2, -1, false);
                int pr = compra ? PrezzoCompra(o) : PrezzoVendi(o);
                Scritta(std::to_string(pr), q.r.x + q.r.width - 46, q.r.y + 24, 24, compra && !q.ok ? Hex(0xa03020) : Hex(0x7a5a10), 1, false);
                Sprite(Ui(4).t, q.r.x + q.r.width - 24, q.r.y + 24, 0.26f);
                if (sotto == q.id) tip = o;
            }
            Scritta("Clic su un oggetto per comprarlo o venderlo. Rotella per scorrere.", x + w / 2, y + FIN.height - 34, 20, INK2, 0, false);
            break;
        }
        case Pan::Lavori: {
            Pannello(FIN, al);
            Titolo("Lavori", y + 46);
            if (b.size() <= 1) Scritta("Ancora nessuna ricetta in questo capitolo.", x + w / 2, y + 300, 28, INK2, 0, false);
            for (auto& q : b) {
                if (q.id < 500 || q.id >= 700) continue;
                const Ricetta& r = ric[q.id - 500];
                Riquadro(q.r, sotto == q.id, q.ok);
                Icona(ogg[r.ris].icona, ogg[r.ris].col, q.r.x + 30, q.r.y + 26, 44);
                Scritta(ogg[r.ris].nome + (r.q > 1 ? " x" + std::to_string(r.q) : ""), q.r.x + 62, q.r.y + 26, 23, q.ok ? INK : INK2, -1, false);
                float ix = q.r.x + q.r.width - 18;
                for (int k = 2; k >= 0; k--) {
                    if (r.ing[k] < 0) continue;
                    std::string n = std::to_string(std::min(inv[r.ing[k]], r.n[k])) + "/" + std::to_string(r.n[k]);
                    int lw = ::Ui::Larghezza(n, 18);
                    Scritta(n, ix, q.r.y + 26, 18, Ha(r.ing[k], r.n[k]) ? Hex(0x2a6a20) : Hex(0xa03020), 1, false);
                    ix -= lw + 24;
                    Icona(ogg[r.ing[k]].icona, ogg[r.ing[k]].col, ix, q.r.y + 26, 34);
                    ix -= 26;
                }
                if (sotto == q.id) tip = r.ris;
            }
            Scritta("Clic su una ricetta per crearla: gli ingredienti escono dallo zaino.", x + w / 2, y + FIN.height - 34, 20, INK2, 0, false);
            break;
        }
        case Pan::Zaino: {
            Pannello(FIN, al);
            Titolo("Zaino", y + 46);
            int a = Migliore(4), d = Migliore(5);
            Scritta("Arma: " + (a >= 0 ? ogg[a].nome : std::string("a mani nude")), x + 50, y + 108, 24, INK, -1, false);
            Scritta("Armatura: " + (d >= 0 ? ogg[d].nome : std::string("nessuna")), x + 520, y + 108, 24, INK, -1, false);
            Scritta("Potenza " + std::to_string(Potenza()) + "   -   Posti " + std::to_string(Usati()) + "/" + std::to_string(Posti()), x + 50, y + 146, 22, INK2, -1, false);
            for (auto& q : b) {
                if (q.id < 700 || q.id >= 900) continue;
                int o = q.id - 700;
                Riquadro(q.r, sotto == q.id, true);
                Icona(ogg[o].icona, ogg[o].col, q.r.x + 60, q.r.y + 54, 80);
                Scritta(std::to_string(inv[o]), q.r.x + q.r.width - 12, q.r.y + q.r.height - 18, 24, INK, 1, false);
                if (sotto == q.id) tip = o;
            }
            for (int k = Usati(); k < std::min(Posti(), 40); k++) { Rectangle r = { x + 50 + (k % 10) * 132.f, y + 190 + (k / 10) * 132.f, 120, 120 }; DrawRectangleRoundedLines(r, 0.2f, 6, ConAlfa(Hex(0x8a6a40), 0.35f)); }
            Scritta("Clic su cibo o cure per usarli.", x + w / 2, y + FIN.height - 34, 20, INK2, 0, false);
            break;
        }
        case Pan::Diario: {
            Pannello(FIN, al);
            Titolo("Diario - Capitolo " + std::to_string(cap + 1) + ": " + caps[cap].nome, y + 46);
            float yy = y + 110;
            int fatte = 0, tot = 0;
            for (size_t m = 0; m < mis.size(); m++) {
                if (mis[m].capitolo != cap + 1) continue;
                tot++; if (statoMis[m] == 3) fatte++;
                if (yy > y + FIN.height - 100) continue;
                std::string seg = statoMis[m] == 3 ? "Fatta" : Pronta((int)m) ? "Da consegnare" : statoMis[m] == 1 ? "In corso" : Disponibile((int)m) ? "Nuova" : "Piu' avanti";
                Color c = statoMis[m] == 3 ? Hex(0x2a6a20) : statoMis[m] == 0 && !Disponibile((int)m) ? Hex(0x8a7a60) : Hex(0x8a2a10);
                Scritta((statoMis[m] == 3 ? "v  " : statoMis[m] == 0 ? "-  " : "!  ") + mis[m].nome, x + 60, yy, 28, c, -1, false);
                Scritta(seg, x + w - 60, yy, 22, c, 1, false);
                yy += 34;
                if (statoMis[m] == 1 || statoMis[m] == 2) { Scritta(Progresso((int)m), x + 100, yy, 22, INK2, -1, false); yy += 30; }
                else if (statoMis[m] == 0 && Disponibile((int)m) && mis[m].chi >= 0) { Scritta("Parla con " + punti[mis[m].chi].nome + " (" + luoghi[punti[mis[m].chi].luogo].nome + ")", x + 100, yy, 22, INK2, -1, false); yy += 30; }
                yy += 14;
            }
            Scritta(std::to_string(fatte) + " missioni compiute su " + std::to_string(tot) + "  -  Punti " + std::to_string(punteggio), x + w / 2, y + FIN.height - 34, 22, INK2, 0, false);
            break;
        }
        case Pan::Mappa: {
            Pannello(FIN, al);
            Titolo("Mappa - " + caps[cap].nome, y + 46);
            auto nodi = NodiMappa();
            std::map<int, Vector2> pos; for (auto& n : nodi) pos[n.l] = n.p;
            for (auto& n : nodi) for (int d = 0; d < 4; d++) {
                int m = luoghi[n.l].uscite[d];
                if (m < 0 || !pos.count(m) || m < n.l) continue;
                bool chiusa = Bloccata(n.l, d) >= 0;
                DrawLineEx(n.p, pos[m], 8, chiusa ? ConAlfa(Hex(0xa03020), 0.6f) : ConAlfa(Hex(0x8a6a40), 0.8f));
            }
            for (auto& n : nodi) {
                Rectangle r = { n.p.x - 120, n.p.y - 34, 240, 68 };
                bool vis = visitati[n.l] != 0, qui = n.l == luogo;
                bool s = sotto == 900 + n.l;
                DrawRectangleRounded(r, 0.35f, 8, qui ? Hex(0xf0c860) : s ? Hex(0xfff4d0) : vis ? Hex(0xf0e0b8) : Hex(0xc8b898));
                DrawRectangleRoundedLines(r, 0.35f, 8, qui ? Hex(0x8a2a10) : Hex(0x6a4a2a));
                Scritta(vis ? luoghi[n.l].nome : "???", n.p.x, n.p.y - (vis ? 6 : 0), 22, INK, 0, false);
                if (vis) {
                    bool mis1 = false;
                    for (int i : puntiDi[n.l]) if (Segno(i)) mis1 = true;
                    if (mis1) Scritta("!", r.x + r.width - 18, r.y + 14, 30, Hex(0xd88a00), 0, false);
                    Scritta(qui ? "Sei qui" : SCENE[luoghi[n.l].scena], n.p.x, n.p.y + 20, 16, INK2, 0, false);
                }
            }
            Scritta("Clic su una localita' gia' visitata per andarci.", x + w / 2, y + FIN.height - 34, 20, INK2, 0, false);
            break;
        }
        case Pan::Capitolo: {
            DrawRectangle(0, 0, 1920, 1080, ConAlfa(Hex(0x0a0806), 0.6f * al));
            Rectangle r = { 360, 180, 1200, 760 };
            Pannello(r, al);
            Scritta("Capitolo " + std::to_string(cap + 1), 960, 260, 40, ConAlfa(Hex(0x8a2a10), al), 0, false);
            Scritta(caps[cap].nome, 960, 330, 64, ConAlfa(Hex(0x3a2210), al), 0, false);
            DrawRectangle(660, 380, 600, 3, ConAlfa(Hex(0xa8844a), al));
            std::string intro = caps[cap].intro.empty() ? "Una nuova avventura ti aspetta." : caps[cap].intro;
            TestoInScatola(intro, 450, 440, 1020, 30, ConAlfa(INK, al));
            break;
        }
        case Pan::Fine: {
            DrawRectangle(0, 0, 1920, 1080, ConAlfa(Hex(0x0a0806), 0.7f * al));
            Bagliore(960, 420, 500 + 40 * std::sin(tempo * 2), ConAlfa(Hex(0xffd860), 0.3f * al));
            Icona(I_GHIANDA, Hex(0xf0c040), 960, 380, 220 * al);
            Scritta(TestoDi(cfg["generale"], "fine", "IL REGNO E' SALVO!"), 960, 580, 56, ConAlfa(Hex(0xffe080), al), 0);
            Scritta("Punti: " + std::to_string(punteggio) + "    Oro: " + std::to_string(oro), 960, 660, 34, ConAlfa(Hex(0xf0e6c8), al), 0);
            break;
        }
        default: break;
        }
        for (auto& q : b) if (!q.t.empty()) Bottone(q, sotto == q.id);
        if (tip >= 0) Fumetto(ogg[tip].nome, Descrizione(tip));
    }

    // ---------------- salvataggi ----------------
    std::string Stato() const override {
        json j;
        j["seme"] = std::to_string(seme); j["caso"] = std::to_string(caso.s); j["casoInizio"] = std::to_string(casoInizio);
        j["giocatore"] = giocatore; j["capitolo"] = cap; j["luogo"] = luogo; j["oro"] = oro; j["vita"] = vita; j["energia"] = energia;
        j["x"] = ex; j["y"] = ey; j["punteggio"] = punteggio; j["tempo"] = tempo; j["tempoCapitolo"] = tempoCapitolo; j["tRiposo"] = tRiposo;
        j["zaino"] = inv; j["missioni"] = statoMis;
        json e = json::array();
        for (size_t i = 0; i < sp.size(); i++) if (sp[i].usati || sp[i].attesa > 0 || sp[i].fatto) e.push_back({ (int)i, sp[i].usati, sp[i].attesa, sp[i].fatto ? 1 : 0 });
        j["elementi"] = e;
        std::string v; for (char c : visitati) v += c ? '1' : '0';
        j["visitati"] = v;
        j["borsa"] = { borsaLuogo, borsaOro, borsaX, borsaY };
        j["capitoloPronto"] = capitoloPronto; j["fineVicina"] = finVicina;
        j["regista"] = json::parse(regista.Json());
        return j.dump();
    }
    bool Ripristina(const std::string& t, std::string& errore) override {
        json j = json::parse(t, nullptr, false);
        if (!j.is_object() || !j.contains("capitolo") || !j.contains("zaino")) { errore = "dati del Regno di Querciaverde non validi"; return false; }
        uint64_t s, c, ci;
        try { s = std::stoull(j.value("seme", std::string("1"))); c = std::stoull(j.value("caso", std::string("1"))); ci = std::stoull(j.value("casoInizio", std::string("1"))); }
        catch (...) { errore = "stato del caso non valido"; return false; }
        bool vecchio = gSilenzio;
        gSilenzio = true;
        Nuova(s, j.value("giocatore", 0));
        gSilenzio = vecchio;
        int pc = puntiDiControllo;
        cap = std::max(0, std::min((int)caps.size() - 1, j.value("capitolo", 0)));
        luogo = std::max(0, std::min((int)luoghi.size() - 1, j.value("luogo", 0)));
        oro = std::max(0, j.value("oro", 0)); vita = std::max(1, std::min(vitaMax, j.value("vita", vitaMax))); energia = std::max(0, std::min(energiaMax, j.value("energia", energiaMax)));
        ex = j.value("x", 960.0f); ey = j.value("y", 760.0f);
        punteggio = j.value("punteggio", 0); tempo = j.value("tempo", 0.0f); tempoCapitolo = j.value("tempoCapitolo", 0.0f); tRiposo = j.value("tRiposo", 0.0f);
        if (j["zaino"].is_array()) for (size_t i = 0; i < inv.size() && i < j["zaino"].size(); i++) inv[i] = std::max(0, j["zaino"][i].is_number() ? j["zaino"][i].get<int>() : 0);
        if (j.contains("missioni") && j["missioni"].is_array()) for (size_t i = 0; i < statoMis.size() && i < j["missioni"].size(); i++) statoMis[i] = std::max(0, std::min(3, j["missioni"][i].is_number() ? j["missioni"][i].get<int>() : 0));
        sp.assign(punti.size(), StatoPunto{});
        if (j.contains("elementi") && j["elementi"].is_array())
            for (auto& q : j["elementi"]) {
                if (!q.is_array() || q.size() < 4) continue;
                int i = q[0].is_number() ? q[0].get<int>() : -1;
                if (i < 0 || i >= (int)sp.size()) continue;
                sp[i].usati = q[1].get<int>(); sp[i].attesa = q[2].get<float>(); sp[i].fatto = q[3].get<int>() != 0;
            }
        std::string v = j.value("visitati", std::string());
        for (size_t i = 0; i < visitati.size(); i++) visitati[i] = i < v.size() && v[i] == '1';
        visitati[luogo] = 1;
        if (j.contains("borsa") && j["borsa"].is_array() && j["borsa"].size() == 4) {
            borsaLuogo = j["borsa"][0].get<int>(); borsaOro = j["borsa"][1].get<int>(); borsaX = j["borsa"][2].get<float>(); borsaY = j["borsa"][3].get<float>();
            if (borsaLuogo >= (int)luoghi.size()) borsaLuogo = -1;
        } else borsaLuogo = -1;
        capitoloPronto = j.value("capitoloPronto", false); finVicina = j.value("fineVicina", false);
        if (j.contains("regista")) regista.DaJson(j["regista"].dump());
        caso.s = c; casoInizio = ci;
        puntiDiControllo = pc;
        az = Az::Niente; pan = Pan::Niente; azPunto = -1; azUscita = -1;
        avvisi.clear(); voli.clear(); numeri.clear(); part.v.clear();
        Musica();
        return true;
    }

    // ---------------- suoni ----------------
    void PreparaSuoni() {
        CreaSuono("qv_legna", 0.22f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.3f; return (0.7f * y + 0.5f * Onda(3, 140 * std::exp(-t * 8) * t)) * Inviluppo(t, 0.001f, 0.22f, 3); });
        CreaSuono("qv_pietra", 0.18f, [](float t) { return 0.3f * (Onda(0, 1900 * t) * 0.4f + Onda(3, 2870 * t) * 0.6f + Rumore() * 0.4f) * Inviluppo(t, 0, 0.18f, 3.5f); });
        CreaSuono("qv_raccogli", 0.24f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.15f; return (0.5f * y + 0.25f * Onda(3, (500 + 900 * t) * t)) * Inviluppo(t, 0.01f, 0.24f, 2); });
        CreaSuono("qv_acqua", 0.35f, [](float t) { float f = 600 + 500 * std::fabs(std::sin(t * 40)) + 700 * t; return 0.28f * Onda(3, f * t) * Inviluppo(t, 0.005f, 0.35f, 1.6f); });
        CreaSuono("qv_moneta", 0.4f, [](float t) { float f = t < 0.07f ? 1568 : 2093; return 0.26f * (Onda(3, f * t) + 0.4f * Onda(1, f * 2 * t)) * Inviluppo(t, 0, 0.4f, 1.6f); });
        CreaSuono("qv_zaino", 0.14f, [](float t) { return 0.35f * Onda(3, (900 - 2600 * t) * t) * Inviluppo(t, 0, 0.14f, 2); });
        CreaSuono("qv_missione", 0.7f, [](float t) { static const float m[] = { 587, 740, 880 }; int n = std::min(2, (int)(t * 7)); return 0.24f * (Onda(1, m[n] * t) + 0.3f * Onda(3, m[n] * 2 * t)) * Inviluppo(t, 0.005f, 0.7f, 1); });
        CreaSuono("qv_completa", 1.2f, [](float t) {
            static const float m[] = { 523, 659, 784, 659, 784, 1047 };
            static const float st[] = { 0, 0.12f, 0.24f, 0.36f, 0.48f, 0.6f };
            int n = 0; for (int k = 0; k < 6; k++) if (t >= st[k]) n = k;
            float tt = t - st[n];
            return 0.22f * (Onda(1, m[n] * t) + 0.5f * Onda(0, m[n] * t) * 0.3f + 0.3f * Onda(3, m[n] * 0.5f * t)) * Inviluppo(tt, 0.004f, n == 5 ? 0.6f : 0.13f, 1.2f); });
        CreaSuono("qv_spada", 0.3f, [](float t) { static float y = 0; y += (Rumore() - y) * (0.05f + t * 2); return (0.5f * y * Inviluppo(t, 0.02f, 0.14f, 1.5f) + 0.18f * (Onda(3, 2400 * t) + Onda(3, 3170 * t)) * Inviluppo(t, 0.06f, 0.3f, 2.5f)); });
        CreaSuono("qv_colpito", 0.22f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.2f; return (0.5f * Onda(3, (200 - 300 * t) * t) + 0.4f * y) * Inviluppo(t, 0, 0.22f, 2); });
        CreaSuono("qv_vittoria", 0.9f, [](float t) { static const float m[] = { 440, 554, 659, 880 }; int n = std::min(3, (int)(t * 6)); return 0.24f * (Onda(1, m[n] * t) + 0.4f * Onda(0, m[n] * t) * 0.4f) * Inviluppo(t, 0.004f, 0.9f, 1); });
        CreaSuono("qv_sconfitta", 1.0f, [](float t) { static const float m[] = { 440, 415, 392, 330 }; int n = std::min(3, (int)(t * 4)); return 0.24f * Onda(1, m[n] * t) * Inviluppo(t, 0.004f, 1.0f, 1); });
        CreaSuono("qv_mangia", 0.32f, [](float t) { static float y = 0; y += (Rumore() - y) * 0.4f; float g = std::fmod(t, 0.1f) < 0.04f ? 1.f : 0.15f; return 0.45f * y * g * Inviluppo(t, 0, 0.32f, 1.2f); });
        CreaSuono("qv_lavoro", 0.45f, [](float t) { float tt = std::fmod(t, 0.22f); return 0.28f * (Onda(3, 1320 * t) + 0.6f * Onda(3, 1980 * t) + 0.3f * Rumore()) * Inviluppo(tt, 0, 0.22f, 3); });
        CreaSuono("qv_forziere", 0.9f, [](float t) {
            float creak = t < 0.35f ? 0.25f * Onda(2, (90 + 40 * std::sin(t * 60)) * t) * Inviluppo(t, 0.02f, 0.35f, 1) : 0.f;
            float scint = t > 0.3f ? 0.16f * Onda(3, (1800 + 600 * std::sin(t * 30)) * t) * Inviluppo(t - 0.3f, 0.01f, 0.6f, 1.5f) : 0.f;
            return creak + scint; });
        CreaSuono("qv_clic", 0.05f, [](float t) { return 0.3f * Onda(3, 1400 * t) * Inviluppo(t, 0, 0.05f, 3); });
        CreaSuono("qv_viaggio", 0.6f, [](float t) { static float y = 0; y += (Rumore() - y) * (0.02f + 0.1f * std::sin(t * 5.2f)); return 0.6f * y * Inviluppo(t, 0.2f, 0.6f, 1); });
        CreaSuono("qv_capitolo", 2.0f, [](float t) {
            float a = Inviluppo(t, 0.3f, 2.0f, 1.2f);
            return 0.12f * a * (Onda(1, 294 * t) + Onda(1, 370 * t) + Onda(1, 440 * t) + 0.6f * Onda(3, 587 * t) + 0.5f * Onda(3, 147 * t)); });
        CreaSuono("qv_no", 0.3f, [](float t) { float f = t < 0.12f ? 233 : 196; return 0.22f * Onda(1, f * t) * Inviluppo(t, 0.004f, 0.3f, 1); });
    }
};

std::vector<Brano> Querciaverde::BraniPredefiniti() const {
    std::vector<Brano> b;
    b.push_back(CreaBrano("Querciaverde - Tema", 92, 8, {
        { "Flauto", 1, 0.32f, "D5:2 E5:1 F5:1 A5:3 G5:1 F5:2 E5:2 D5:4 C5:2 D5:1 E5:1 F5:2 G5:2 A5:3 G5:1 E5:4 A5:2 C6:2 D6:3 C6:1 A5:2 G5:2 F5:4 E5:2 F5:1 G5:1 A5:2 E5:2 D5:8" },
        { "Arpa", 3, 0.2f, "D4:1 A4:1 D5:1 A4:1 C4:1 G4:1 C5:1 G4:1 Bb3:1 F4:1 Bb4:1 F4:1 A3:1 E4:1 A4:1 E4:1" },
        { "Basso", 1, 0.36f, "D2:4 C2:4 Bb1:4 A1:4" },
        { "Tamburo", 4, 0.28f, "K.......S.....h.", true } }));
    b.push_back(CreaBrano("Querciaverde - Borgo", 108, 8, {
        { "Liuto", 0, 0.16f, "G4:1 B4:1 D5:1 B4:1 C5:1 E5:1 G5:1 E5:1 D5:1 F#5:1 A5:1 F#5:1 G5:2 D5:2" },
        { "Flauto", 1, 0.3f, "D5:2 G5:2 A5:1 B5:1 A5:2 G5:2 E5:2 D5:4 B4:2 C5:2 D5:1 E5:1 D5:2 B4:2 A4:2 G4:4 R:2" },
        { "Basso", 1, 0.34f, "G2:2 D2:2 C2:2 G1:2 D2:2 A1:2 G2:4" },
        { "Tamburo", 4, 0.3f, "K...h.K.S...h...", true } }));
    b.push_back(CreaBrano("Querciaverde - Bosco", 84, 8, {
        { "Flauto", 1, 0.3f, "E5:3 F#5:1 G5:2 B5:2 A5:3 G5:1 F#5:4 D5:3 E5:1 F#5:2 A5:2 G5:3 F#5:1 E5:4" },
        { "Arpa", 3, 0.18f, "E4:1 B4:1 E5:1 B4:1 D4:1 A4:1 D5:1 A4:1 C4:1 G4:1 C5:1 G4:1 B3:1 F#4:1 B4:1 F#4:1" },
        { "Basso", 3, 0.34f, "E2:8 D2:8 C2:8 B1:8" },
        { "Fruscio", 4, 0.16f, "....h.......h...", true } }));
    b.push_back(CreaBrano("Querciaverde - Miniera", 100, 8, {
        { "Campane", 3, 0.26f, "A4:2 C5:2 E5:2 D5:1 C5:1 B4:2 G4:2 A4:4 E5:2 F5:2 E5:2 D5:1 C5:1 B4:2 C5:2 A4:4" },
        { "Basso", 2, 0.24f, "A1:2 A1:2 E2:2 A1:2 F1:2 F1:2 G1:2 E1:2" },
        { "Martelli", 4, 0.34f, "K..oK..oK..oS.oo", true } }));
    b.push_back(CreaBrano("Querciaverde - Mare", 96, 8, {
        { "Flauto", 1, 0.3f, "C5:3 D5:1 E5:2 G5:2 A5:3 G5:1 E5:4 F5:3 E5:1 D5:2 C5:2 D5:6 R:2" },
        { "Onde", 3, 0.2f, "C4:1 G4:1 C5:1 G4:1 A3:1 E4:1 A4:1 E4:1 F3:1 C4:1 F4:1 C4:1 G3:1 D4:1 G4:1 D4:1" },
        { "Basso", 1, 0.34f, "C2:4 A1:4 F1:4 G1:4" },
        { "Tamburo", 4, 0.24f, "K.....h.K...S...", true } }));
    b.push_back(CreaBrano("Querciaverde - Palude", 72, 8, {
        { "Ombra", 3, 0.28f, "B4:3 C5:1 D5:4 F5:2 E5:2 D5:4 C5:3 B4:1 A4:4 G#4:2 A4:2 B4:4" },
        { "Pad", 1, 0.2f, "B2:8 G2:8 A2:8 F#2:8" },
        { "Gocce", 4, 0.18f, "h.......h...o...", true } }));
    b.push_back(CreaBrano("Querciaverde - Montagna", 88, 8, {
        { "Corno", 1, 0.32f, "D5:4 A4:2 D5:2 E5:3 F#5:1 E5:4 G5:4 F#5:2 E5:2 D5:4 C5:2 A4:2 D5:8" },
        { "Eco", 3, 0.16f, "D4:2 A4:2 D5:2 A4:2 C4:2 G4:2 C5:2 G4:2" },
        { "Basso", 1, 0.34f, "D2:8 C2:8 G1:8 D2:8" },
        { "Tamburo", 4, 0.3f, "K.......K...S...", true } }));
    b.push_back(CreaBrano("Querciaverde - Deserto", 104, 8, {
        { "Ney", 1, 0.3f, "E5:2 F5:1 G#5:1 A5:2 G#5:1 F5:1 E5:4 D5:2 E5:1 F5:1 E5:2 D5:1 C5:1 B4:2 C5:1 B4:1 A4:4" },
        { "Oud", 0, 0.14f, "A3:1 E4:1 A4:1 E4:1 A3:1 F4:1 A4:1 F4:1 G#3:1 E4:1 B4:1 E4:1 A3:1 E4:1 A4:2" },
        { "Basso", 1, 0.32f, "A1:4 F1:4 E1:4 A1:4" },
        { "Darbuka", 4, 0.34f, "K.hhS.h.K.hhS.hh", true } }));
    b.push_back(CreaBrano("Querciaverde - Castello", 80, 8, {
        { "Archi", 2, 0.16f, "D5:4 F5:4 E5:2 D5:2 C#5:4 D5:4 A4:4 Bb4:4 A4:2 G4:2 A4:8" },
        { "Organo", 1, 0.24f, "D3:8 Bb2:8 G2:8 A2:8" },
        { "Rintocco", 3, 0.2f, "D4:4 R:4 A3:4 R:4" },
        { "Timpani", 4, 0.32f, "K.......K.K.....", true } }));
    b.push_back(CreaBrano("Querciaverde - Battaglia", 140, 8, {
        { "Ottoni", 2, 0.2f, "A4:1 A4:1 C5:1 A4:1 E5:2 D5:1 C5:1 D5:1 D5:1 F5:1 D5:1 A5:2 G5:1 E5:1" },
        { "Basso", 0, 0.22f, "A1:1 A2:1 A1:1 A2:1 F1:1 F2:1 F1:1 F2:1 D2:1 D3:1 D2:1 D3:1 E1:1 E2:1 E1:1 E2:1" },
        { "Tamburi", 4, 0.42f, "K.KhS.hhK.KhS.Kh", true } }));
    return b;
}

}  // namespace

std::unique_ptr<GiocoArcade> CreaQuerciaverde() { return std::unique_ptr<GiocoArcade>(new Querciaverde()); }

}  // namespace arcade
