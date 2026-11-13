// Funzioni che parlano direttamente con il sistema operativo.
// Stanno in un file separato perche' windows.h e raylib.h usano gli stessi nomi
// (Rectangle, DrawText, CloseWindow...) e non possono essere inclusi insieme.
#pragma once
#include <string>

std::string PercorsoEseguibile();
std::string CartellaEseguibile();
std::string LeggiVariabileAmbiente(const char* nome);

// Richiesta HTTPS POST in un thread separato, per non bloccare la finestra.
// Stato: 0 nessuna richiesta, 1 in corso, 2 completata, 3 errore.
void HttpAvviaPost(const std::string& host, const std::string& percorso,
                   const std::string& intestazioni, const std::string& corpo);
int HttpStato();
std::string HttpRisposta();   // corpo della risposta, oppure descrizione dell'errore

// Su Linux un file deve essere marcato come eseguibile; su Windows non serve.
void RendiEseguibile(const std::string& percorso);
