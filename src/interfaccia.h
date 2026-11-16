// Elementi d'interfaccia disegnati alla risoluzione della finestra (non a quella del gioco),
// condivisi tra lo studio e i giochi creati.
#pragma once
#include "comune.h"

// Pulsante altoparlante nel rettangolo r (in pixel della finestra): clic o tasto F8 per
// spegnere/accendere tutto l'audio. Restituisce true se lo stato e' cambiato.
// 'fileOpzioni' e' il file dove ricordare la scelta (vuoto = non salvare).
bool PulsanteAudio(Rectangle r, const std::string& fileOpzioni);
// Posizioni comode: in alto o in basso a destra dentro l'area di gioco.
Rectangle AngoloAudio(Rectangle area, bool inBasso);
// Fuori dall'immagine del gioco, nelle bande laterali o sopra, se c'e' spazio; altrimenti in alto a destra.
Rectangle PostoAudioLibero(Rectangle area);

// Opzioni audio (muto, volumi) lette e scritte in un piccolo file di testo.
void CaricaOpzioniAudio(const std::string& file);
void SalvaOpzioniAudio(const std::string& file);

// Musica di sottofondo dello studio (non quella dei giochi): spenta di serie.
// Si accende e spegne col pulsante a nota o con F7, senza toccare gli effetti sonori.
extern bool gMusicaStudio;
bool PulsanteMusica(Rectangle r, const std::string& fileOpzioni);

// Musica e effetti dei giochi (prova F5 e giochi creati), separati: F7 la musica, F6 gli effetti, F8 tutto.
extern bool gMusicaGioco;
void CambiaMusicaGioco(const std::string& fileOpzioni);
void CambiaEffetti(const std::string& fileOpzioni);
void AvvisoAudio(const std::string& testo);
// Pulsanti nota ed effetti a sinistra dell'altoparlante 'altoparlante' (gestiscono anche F7 e F6).
void PulsantiAudioGioco(Rectangle altoparlante, const std::string& fileOpzioni);
