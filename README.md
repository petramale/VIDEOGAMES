# Studio Videogiochi

Uno studio **stand-alone in C++** per creare videogiochi 2D per PC, con **Claude** come assistente creativo
in ogni sezione. Tutto ciò che serve è dentro il repository: solo librerie open-source, nessun programma esterno,
e lo studio crea **direttamente il file .exe** del gioco, senza compilatore.

Sviluppatore: **Mario Petramale** - Elvantis.

Il manuale d'uso completo è in [MANUALE.md](MANUALE.md).

## Avvio rapido con Embarcadero Dev-C++

1. Apri `StudioVideogiochi.dev` con Dev-C++ (testato per la 6.3 con TDM-GCC 9.2 a 64 bit).
2. Premi **F11** (Compila ed esegui). La prima compilazione include raylib e richiede un paio di minuti.
3. L'eseguibile viene creato in `bin\StudioVideogiochi.exe`: un solo file, senza DLL.

In alternativa, da riga di comando con il `bin` di TDM-GCC nel PATH: `mingw32-make`.

## L'officina

Lo studio si apre a tutta finestra, a risoluzione piena, con testi nitidi (carattere Outfit) e grafica dei giochi
disegnata in alta definizione.

- **In alto** il menu di sistema **Gioco** (Nuovo, Apri, **Salva gioco**, Salva con nome, Prova, Crea .exe,
  Stile grafico, Annulla, Ripeti, Musica dello studio, Chiave di Claude, Esci) e i menu della sezione aperta. La musica dello studio
  è spenta di serie (F7 o la nota in alto la accendono): gli effetti sonori restano sempre attivi.
- **Sotto** le schede delle sezioni (Alt+1 … Alt+9) e i pulsanti **Prova F5** e **Crea .exe**.
- **A destra** la chat con **Claude**, presente in ogni sezione: sa dove sei e cosa hai selezionato.
- **Aiuto sui pulsanti**: fermandosi con il mouse su un pulsante compare un riquadro che ne spiega la funzione.

| Sezione | Cosa si fa |
| --- | --- |
| Mondi | Editor delle mappe a blocchi: caselle, oggetti, selezione di blocchi (cancella, copia, incolla), porte fra mappe, scroll in 4 direzioni, telecamera, strati dello sfondo. Nella tavolozza ci sono anche i personaggi e gli oggetti della libreria del gioco. |
| Personaggi | Disegni e animi i personaggi (matita, gomma, secchio, linee, forme, simmetria, cipolla), importi immagini o fogli di sprite, scegli il comportamento (eroe, fermo, pattuglia, insegue, vola, torretta, boss, amico che parla) e i suoni. Ogni personaggio si salva nel gioco e si usa nelle mappe o come eroe. |
| Oggetti | Oggetti disegnati con la loro funzione: decorazione, da raccogliere, cura, chiave, porta, molla, cassa da spingere, interruttore, piattaforma mobile, pericolo, arrivo, proiettile. |
| Scenari | Sfondi a strati con parallasse, cielo, ora del giorno, pioggia e neve; le caselle del terreno disegnate a mano. |
| Audio | Musica a note su più tracce (piano roll), effetti sonori con sintetizzatore, import di WAV/OGG/MP3, suoni collegati agli eventi del gioco. |
| Logica | Regole "QUANDO … SE … ALLORA …": tocchi, raccolte, colpi, tasti, timer, variabili; azioni come suonare, mostrare un messaggio, cambiare mappa, dare punti. |
| Librerie | La libreria del gioco e 22 raccolte pronte (personaggi, oggetti, scenari, caselle, musiche, effetti) in pixel art fine o disegnate in HD. |
| Comandi | Tastiera, mouse e joypad: tasti da cambiare con un clic, zona morta, comandi extra legati alle regole, prova dal vivo. |
| Comportamenti | La mente delle creature: profili di carattere (coraggio, aggressività, prudenza, rancore…), quanto preferiscono ogni azione, reazioni e frasi secondo l'umore, a chi si applicano; l'indole degli oggetti; come cambia ogni partita (evento speciale, alleanze, veterani, meteo). Con il simulatore delle decisioni. |

Un gioco intero (mondo, libreria, regole, musica, immagini) è **un solo file `.gioco`** nella cartella `giochi`
accanto allo studio. Lo studio riapre da solo l'ultimo gioco e, se chiudi la finestra, salva il lavoro non salvato.

## Il gioco creato

Lo studio non usa un compilatore: copia sé stesso e attacca in coda il file del gioco.
L'eseguibile creato parte sempre così:

1. **intro** con il logo Elvantis, "Mario Petramale presenta", il titolo e la musica dell'intro;
2. **"Premi un tasto"**;
3. **menu**: 1 giocatore, 2 giocatori (a turni, con confronto dei punti), Opzioni (musica, effetti, audio, schermo intero), Crediti, Esci;
4. **il gioco**, con pausa (Esc: riprendi, ricomincia, musica, effetti, menu principale). Musica ed effetti
   si spengono separatamente anche con F7 e F6, nella prova (F5) come nel gioco creato.

Il file creato si può copiare su qualsiasi PC Windows e funziona senza installare nulla.

## Collegare Claude

Serve una chiave API di Anthropic (da [console.anthropic.com](https://console.anthropic.com)).
Si incolla dal menu **Gioco → Chiave di Claude** (viene salvata in `claude_chiave.txt` accanto allo studio,
file escluso da git e mai copiato nei giochi creati), oppure si mette nella variabile d'ambiente `ANTHROPIC_API_KEY`.
Il modello predefinito è `claude-opus-5-5`; per cambiarlo scrivi il nome in `claude_modello.txt`.

Claude è l'unica parte che usa internet: tutto il resto dello studio funziona offline.

## Riga di comando

```
StudioVideogiochi.exe MioGioco.gioco                    apre un gioco
StudioVideogiochi.exe --crea MioGioco.exe livello.txt "Titolo"
StudioVideogiochi.exe --esporta-audio cartella          effetti e musica in .wav
StudioVideogiochi.exe --esporta-mondi cartella          i mondi d'esempio in JSON
StudioVideogiochi.exe --verifica-mondi                  prove automatiche (mondi, progetti, risposte di Claude)
```

## Struttura

```
StudioVideogiochi.dev   progetto per Embarcadero Dev-C++
Makefile                compilazione con mingw32-make (o make su Linux)
MANUALE.md              manuale d'uso
src/
  main.cpp              avvio dello studio, intro Elvantis, flusso dei giochi creati (menu, 2 giocatori)
  officina.*            l'officina: menu di sistema, schede, chat con Claude, file .gioco
  officina_sezioni.cpp  sezioni Mondi, Personaggi, Oggetti, Scenari
  officina_audio.cpp    sezione Audio (musica, effetti, suoni degli eventi)
  officina_logica.cpp   sezioni Logica, Librerie, Comandi
  officina_comportamenti.cpp  sezione Comportamenti (profili, indole, partita unica, simulatore)
  editor_disegno.*      editor di disegno e animazione (personaggi, oggetti, caselle)
  editor_mondo.*        editor delle mappe (dentro la sezione Mondi)
  progetto_gioco.*      il file .gioco: libreria, regole, stile grafico
  raccolte.*            le 22 raccolte pronte
  claude_progetto.*     Claude sull'intero gioco (personaggi, oggetti, brani, regole…)
  claude_mondo.*        Claude sulle mappe
  gioco*.cpp, gioco.h   motore: fisica, collisioni, telecamera, libreria, regole, disegno HD
  mente.*               la mente delle creature (utility AI, emozioni, memoria, apprendimento), generatore deterministico
  gioco_mente.cpp       mente e indole nel motore, destino della partita (seme, eventi, relazioni)
  salvataggio.*         salvataggi dei giochi creati: CRC, scrittura atomica, versioni, ripristino
  verifica_salvataggi.cpp  prova automatica dei salvataggi (--verifica-salvataggi)
  mondo.*               mondi e mappe in JSON
  ui.*                  controlli dell'interfaccia (pulsanti, campi, scelte, colori)
  aiuto.cpp             le spiegazioni dei pulsanti (aiuto che compare fermandosi col mouse)
  audio.*               sintetizzatore, brani ed effetti
  periferiche.*         tastiera, mouse, joypad e mappatura dei comandi
  pacchetto.*           costruttore dell'eseguibile
  sistema.*             funzioni di Windows (WinHTTP, percorso dell'eseguibile)
  risorse_dati.h        logo e icona incorporati; caratteri_dati.h il carattere Outfit; licenze_dati.h le licenze
esempi/                 mondi e livelli d'esempio
risorse/                logo Elvantis, icona, carattere Outfit (licenza OFL)
strumenti/incorpora.cpp trasforma file binari in array C++ (logo, carattere, licenze)
licenses/               licenza dello studio e avvisi delle librerie open source
LICENSE.txt             licenza di Studio Videogiochi (proprietaria, Elvantis - Mario Petramale)
librerie/               raylib 5.5 e nlohmann/json con le loro licenze
```

## Licenze

Studio Videogiochi è **software proprietario** di Elvantis - Mario Petramale: non è software libero.
La licenza (in inglese) è in [LICENSE.txt](LICENSE.txt). In breve: lo studio si usa sui propri computer per creare
giochi, ma non si copia, non si rivende e non si modifica.

I **giochi creati** si possono distribuire e vendere, purché restino l'intro Elvantis, i crediti e gli avvisi
delle licenze, e senza estrarre il motore o le risorse incluse. I disegni, le musiche e le mappe originali
restano di chi li ha creati.

Le librerie open source incluse (raylib, GLFW, miniaudio, stb, dr_libs, nlohmann/json, carattere Outfit e altre)
hanno licenze permissive (zlib, MIT, MIT-0, pubblico dominio, OFL) che ne permettono l'uso in software proprietario.
I loro avvisi sono nella cartella [licenses/](licenses/README.txt) e, in ogni gioco creato, in **Crediti → L**.
