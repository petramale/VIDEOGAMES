# Studio Videogiochi

Uno studio **stand-alone in C++** per creare videogiochi 2D per PC, con **Claude** come assistente creativo.
Tutto ciò che serve è dentro il repository: solo librerie open-source, nessun programma esterno,
e lo studio crea **direttamente il file .exe** del gioco, senza compilatore.

Sviluppatore: **Mario Petramale** - Elvantis.

Architettura e piano completi: [Generatore di videogiochi C++: architettura e piano](https://claude.ai/artifact/4h33WsyU7LhHuhYiennan9).

## Avvio rapido con Embarcadero Dev-C++

1. Apri `StudioVideogiochi.dev` con Dev-C++ (testato per la 6.3 con TDM-GCC 9.2 a 64 bit).
2. Premi **F11** (Compila ed esegui). La prima compilazione include raylib e richiede un paio di minuti.
3. L'eseguibile viene creato in `bin\StudioVideogiochi.exe`: un solo file, senza DLL.

In alternativa, da riga di comando con il `bin` di TDM-GCC nel PATH: `mingw32-make`.

## Le 7 categorie, una per voce di menu

| # | Categoria | Cosa provare |
| --- | --- | --- |
| 1 | Sfondi e scenari | Paesaggio a parallasse con 4 strati di montagne, nuvole, foresta, stelle e lucciole. Su/Giù cambia l'ora del giorno, T cambia tema (valle, deserto, ghiacci, vulcani). |
| 2 | Personaggi e animazioni | Eroe animato a stati (fermo, corsa, salto, caduta, atterraggio, vittoria, colpito) con mantello che ondeggia e deformazione elastica. C cambia aspetto. Accanto, uno sprite in pixel art nel formato usato da Claude. |
| 3 | Oggetti interattivi | Monete che ruotano, gemme luminose, molle, forzieri che sputano tesori, lanterne-checkpoint, slime e pipistrelli. Ogni oggetto ha il suo nome sopra. |
| 4 | Effetti e musica | Sintetizzatore chiptune: 11 effetti generati dal codice e una colonna sonora originale (melodia con eco, basso, arpeggi, pad, batteria). Rullo di note e oscilloscopio in tempo reale. |
| 5 | Logica di gioco | Livello completo "La valle delle gemme": salto variabile, coyote time, collisioni con tile e piattaforme, telecamera che anticipa, nemici, spine, vite, checkpoint, vittoria con fuochi d'artificio. F1 mostra i riquadri di collisione. **C crea il gioco .exe.** |
| 6 | Librerie | Le librerie incluse, con versione, licenza e cartella (elenco anche in `librerie/librerie.json`). |
| 7 | Crea con Claude | Descrivi uno sprite o un livello: Claude lo crea, lo studio lo controlla e lo mostra. Un livello si gioca subito con F5, F3 lo apre nell'editor. F2 carica una risposta d'esempio, utile senza internet. |
| 8 | Editor + chat Claude | Disegni il livello con mouse o joypad (terreno, piattaforme, spine, oggetti, nemici), cambi tema e ora, Tab lo provi subito, Ctrl+S salva, Ctrl+E crea il .exe. A destra la **chat con Claude** modifica il progetto aperto (livello, scenario, eroe, fisica) con anteprima immediata. F4 apre il pannello audio: trascina sulla finestra file **WAV, OGG o MP3** per la musica o per i singoli effetti; finiscono dentro l'eseguibile creato. |
| 9 | Comandi e periferiche | Tastiera, mouse e joypad: per ogni azione due tasti e un pulsante del joypad, da cambiare con un clic; prova dal vivo, zona morta delle levette. Salvato in `comandi.cfg`, vale anche per i giochi creati. |

Comandi comuni: frecce o WASD per muoversi, Spazio per saltare, M accende o spegne la musica,
F11 schermo intero, Esc torna al menu. Funzionano anche il mouse (nei giochi: sinistro cammina verso
il puntatore, destro salta) e il joypad. La finestra si apre grande, al 90% dello schermo.

## Creare l'eseguibile di un gioco

Lo studio non usa un compilatore: copia sé stesso e attacca in coda i dati del gioco.
All'avvio, l'eseguibile trova i dati e parte direttamente con il gioco.

- Dallo studio: categoria 5, tasto **C**. Accanto allo studio compare `LaValleDelleGemme.exe`.
- Da riga di comando:
  ```
  StudioVideogiochi.exe --crea MioGioco.exe esempi\la_valle_delle_gemme.txt "Il mio gioco"
  ```

Il file creato si può copiare su qualsiasi PC Windows e funziona senza installare nulla.

## Collegare Claude

Serve una chiave API di Anthropic (da [console.anthropic.com](https://console.anthropic.com)).
Mettila nella variabile d'ambiente `ANTHROPIC_API_KEY` oppure in un file `claude_chiave.txt`
accanto a `StudioVideogiochi.exe` (il file è escluso da git). Il modello predefinito è
`claude-sonnet-5-5`; per cambiarlo scrivi il nome in `claude_modello.txt`.

Claude è l'unica parte che usa internet: tutto il resto dello studio funziona offline.

## Altre funzioni

- `StudioVideogiochi.exe --esporta-audio cartella` salva tutti gli effetti e la musica in file `.wav`.

## Struttura

```
StudioVideogiochi.dev   progetto per Embarcadero Dev-C++
Makefile                compilazione con mingw32-make (o make su Linux)
src/
  main.cpp              menu dello studio, presentazione con il logo, avvio dei giochi creati
  editor.*              editor dei livelli con la chat di Claude
  progetto.*            progetto di un gioco (livello, scenario, eroe, fisica, file audio)
  periferiche.*         tastiera, mouse, joypad e mappatura dei comandi
  risorse_dati.h        logo e icona incorporati (generato da strumenti/incorpora.cpp)
  sfondo.*              1. sfondi e scenari
  personaggio.*         2. personaggi e animazioni
  pixelart.*            sprite in pixel art (formato JSON di Claude)
  livello.*, gioco.*    3 e 5. oggetti, fisica, collisioni, nemici, telecamera
  particelle.*          effetti visivi
  audio.*               4. sintetizzatore e colonna sonora
  librerie.*            6. elenco delle librerie
  claude.*              7. dialogo con Claude
  pacchetto.*           costruttore dell'eseguibile
  sistema.*             funzioni di Windows (WinHTTP, percorso dell'eseguibile)
esempi/                 livelli di esempio in formato testo
risorse/                logo Elvantis e icona dell'eseguibile (studio.rc)
strumenti/incorpora.cpp trasforma file binari in array C++
librerie/               raylib 5.5 e nlohmann/json con le loro licenze
```

## Formato dei livelli

Un livello è un testo: ogni carattere è un tile da 16 pixel.

| Carattere | Significato | Carattere | Significato |
| --- | --- | --- | --- |
| `#` | terra con erba | `o` | moneta |
| `=` | pietra | `g` | gemma |
| `-` | piattaforma attraversabile dal basso | `m` | molla |
| `^` | spine | `c` | forziere |
| `P` | partenza | `k` | lanterna (checkpoint) |
| `F` | bandiera d'arrivo | `e`, `b` | slime, pipistrello |

## Licenze

Le librerie incluse hanno licenze permissive (zlib, MIT): i giochi creati si possono distribuire
e vendere liberamente. I testi delle licenze sono nella cartella `librerie/`.
