// Piccola interfaccia "immediata" per gli editor a risoluzione piena:
// pulsanti, campi di testo, numeri, interruttori, scelte e colori.
// Ogni funzione disegna il controllo e nello stesso momento ne gestisce mouse e tastiera.
#pragma once
#include "comune.h"
#include <string>
#include <vector>

namespace Ui {

// Da chiamare all'inizio di ogni fotogramma con la scala dell'interfaccia (1 = finestra alta 900 pixel).
void Inizia(float scala);
float S();                         // scala attuale
int T();                           // dimensione del testo normale
int TP();                          // testo piccolo

// Colori del tema dello studio.
Color Fondo();      Color Pannello();   Color Bordo();   Color Accento();
Color TestoChiaro(); Color TestoSpento(); Color Selezione();

// Area "modale" (un menu a tendina aperto, una finestra): i controlli sotto non ricevono il mouse.
void ImpostaModale(Rectangle r);
void ApriModale();   void ChiudiModale();   // dentro la finestra modale i controlli funzionano
bool MouseSu(Rectangle r);
bool Cliccato(Rectangle r, int tasto = MOUSE_BUTTON_LEFT);

void Testo(const std::string& t, float x, float y, Color c, int dim = 0);
void TestoCentrato(const std::string& t, Rectangle r, Color c, int dim = 0);
int Larghezza(const std::string& t, int dim = 0);
void TestoForte(const std::string& t, float x, float y, Color c, int dim = 0);   // grassetto, per i titoli
int LarghezzaForte(const std::string& t, int dim = 0);
// Testo a capo dentro una larghezza; restituisce l'altezza usata.
float TestoACapo(const std::string& t, float x, float y, float larghezza, Color c, int dim = 0, bool disegna = true);

bool Pulsante(Rectangle r, const std::string& testo, bool attivo = false, bool abilitato = true);
// Campo di testo: true quando l'utente conferma (Invio o clic altrove) un valore cambiato.
bool Campo(const std::string& id, Rectangle r, std::string& testo, size_t massimo = 200);
// Numero con - e +, rotella del mouse e clic per scriverlo.
bool Numero(const std::string& id, Rectangle r, const std::string& etichetta, float& v, float minimo, float massimo,
            float passo, const char* formato = "%.0f");
bool NumeroIntero(const std::string& id, Rectangle r, const std::string& etichetta, int& v, int minimo, int massimo);
bool Interruttore(Rectangle r, const std::string& etichetta, bool& v);
// Scelta fra voci con frecce < >.
bool Scelta(Rectangle r, const std::string& etichetta, int& v, const std::vector<std::string>& voci);
// Colore "#rrggbb" con campione.
bool Colore(const std::string& id, Rectangle r, const std::string& etichetta, Color& c);

bool StaScrivendo();               // un campo di testo ha la tastiera
void TogliFuoco();
void DaiFuoco(const std::string& id);   // il campo con questo id riceve la tastiera (testo vuoto)
std::string Fuoco();

}
