// Categoria 4: effetti sonori e temi musicali.
// Un piccolo sintetizzatore "chiptune" genera tutti i suoni dal nulla:
// onde quadre, triangolari, a dente di sega e rumore, con inviluppi, vibrato ed eco.
// La colonna sonora e' composta nel codice (melodia, arpeggi, basso, batteria, pad).
#pragma once
#include "comune.h"
#include <atomic>

enum class Effetto { Salto, Moneta, Gemma, Colpo, Molla, Forziere, Esplosione, Vittoria, Passo, Menu, Conferma, Totale };

struct EventoNota {
    int canale;       // 0 melodia, 1 basso, 2 arpeggio, 3 batteria
    float inizio, durata;
    int midi;
};

class Audio {
public:
    void Prepara();          // genera suoni e musica (non serve la scheda audio)
    void Inizializza();      // prepara e avvia l'uscita audio
    bool EsportaWav(const std::string& cartella); // salva effetti e brano in file .wav
    void Chiudi();
    void Aggiorna(float dt); // senza scheda audio fa comunque avanzare il brano (per la grafica)
    void Suona(Effetto e, float volume = 1.0f);
    static const char* NomeEffetto(Effetto e);

    // Musiche ed effetti da file (WAV, OGG, MP3, QOA). 'estensione' es. ".ogg".
    // I dati devono restare validi finche' sono in uso: l'Audio ne tiene una copia.
    bool CaricaMusica(const std::string& estensione, const std::string& dati);
    bool CaricaEffetto(Effetto e, const std::string& estensione, const std::string& dati);
    // Applica i file di un progetto: "musica.ogg" e "effetto_<Nome>.wav".
    void ApplicaFile(const std::vector<std::pair<std::string, std::string>>& file);
    void RipristinaMusica();               // torna alla colonna sonora generata
    void RipristinaEffetto(Effetto e);     // torna all'effetto generato
    bool MusicaDaFile() const { return musicaFile; }
    bool EffettoDaFile(Effetto e) const { return effettoFile[(int)e]; }
    void FermaTutto();                     // ripristina tutto ai suoni generati

    void MusicaAttiva(bool attiva);
    bool MusicaAttiva() const { return musicaAttiva; }
    float volumeMusica = 0.55f;

    // Per la visualizzazione (oscilloscopio e rullo di note).
    float TempoBrano() const;
    float DurataBrano() const { return durata; }
    float Campione(long indice) const;       // campione del brano, indice in frame
    long PosizioneFrame() const { return (long)posizione.load(); }
    const std::vector<EventoNota>& Note() const { return note; }
    float Battito() const { return 60.0f / bpm; }
    int Frequenza() const { return 44100; }

private:
    static void Callback(void* buffer, unsigned int frame);
    void ComponiBrano();

    Sound suoni[(int)Effetto::Totale]{};
    std::vector<float> campioniEffetti[(int)Effetto::Totale];
    Music musica{};
    bool musicaFile = false;
    std::string datiMusica;
    bool effettoFile[(int)Effetto::Totale] = {};
    bool preparato = false;
    AudioStream flusso{};
    std::vector<float> brano;
    std::vector<EventoNota> note;
    std::atomic<unsigned long> posizione{ 0 };
    bool musicaAttiva = false;
    bool pronto = false;
    float durata = 0;
    float bpm = 132;
};

extern Audio gAudio;
