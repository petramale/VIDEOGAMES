# Manuale d'uso - Studio Videogiochi

Studio Videogiochi di **Mario Petramale - Elvantis**. Questo manuale spiega come creare un gioco 2D
dall'inizio alla fine: dal primo disegno all'eseguibile da regalare o vendere.

## 1. Primo avvio

1. Compila con Dev-C++ (F11) oppure avvia `bin\StudioVideogiochi.exe`.
2. Dopo il logo Elvantis si apre l'**officina** sulla pagina **Home**.
3. Lo studio riapre l'ultimo gioco su cui hai lavorato; la prima volta ne crea uno nuovo ("Il mio gioco", platform, pixel art fine).

Per usare Claude: menu **Gioco → Chiave di Claude…**, incolla la chiave API di Anthropic e conferma.
La chiave resta solo nel file `claude_chiave.txt` accanto allo studio e non finisce mai dentro i giochi creati.

## 2. Com'è fatta l'officina

- **Barra dei menu**: il menu **Gioco** è sempre presente; accanto ci sono i menu della sezione aperta e **Aiuto**.
  In alto a destra lo stato di Claude e l'altoparlante (clic o F8 per spegnere tutto l'audio).
- **Schede delle sezioni**: Home, Mondi, Personaggi, Oggetti, Scenari, Audio, Logica, Librerie, Comandi
  (anche con **Alt+1 … Alt+8**). A destra **Prova F5** e **Crea .exe**.
- **Chat di Claude** a destra (F2 la apre o la chiude, F3 porta il cursore nella chat).

### Il menu Gioco

| Voce | Tasto | Cosa fa |
| --- | --- | --- |
| Nuovo gioco | Ctrl+N | Sceglie genere (platform, open world, sparatutto, avventura, labirinto, puzzle), stile grafico e titolo |
| Apri | Ctrl+O | Apre un `.gioco` salvato o uno dei mondi d'esempio |
| Salva gioco | Ctrl+S | Salva in `giochi\<titolo>.gioco` |
| Salva con nome | Ctrl+Maiusc+S | Salva una copia con un nome nuovo |
| Prova il gioco | F5 | Gioca subito; Esc o F5 torna all'officina, R ricomincia, F1 mostra le collisioni |
| Crea il gioco .exe | Ctrl+E | Crea l'eseguibile accanto allo studio |
| Stile grafico | | Passa da **pixel art fine** a **disegnato in alta definizione** (e viceversa) |
| Annulla / Ripeti | Ctrl+Z / Ctrl+Y | Vale anche per le modifiche fatte da Claude |
| Torna alla home, Esci | | |

Se chiudi la finestra senza salvare, lo studio salva da solo il lavoro (senza sovrascrivere altri giochi).

Puoi anche **trascinare file sulla finestra**: un `.gioco` o un `.json` si apre, un'immagine PNG/JPG diventa
fotogrammi del personaggio aperto (un foglio di sprite viene diviso da solo), uno strato dello scenario o un personaggio nuovo;
un WAV/OGG/MP3 diventa un brano o un effetto.

## 3. Mondi

L'editor delle mappe. Ogni mondo ha una o più **mappe** (schede in alto, `+` per aggiungerne una, clic destro per rinominarla).

- **Tavolozza a sinistra**: le caselle (terra, pietra, piattaforma, spine…), la **libreria del gioco** (i tuoi personaggi e oggetti) e gli oggetti classici.
- **Mappa al centro**: rotella e frecce scorrono, Ctrl+rotella fa lo zoom, tasto centrale o Spazio+trascina sposta la vista. Minimappa in basso a destra.
- **Strumenti in basso**: Pennello (B), Gomma (E), Riempi (F), Rettangolo (R), Seleziona (V), Contagocce (I); Griglia, Sfondo, Inquadratura.
- **Pannello a destra**: *Proprietà* (della mappa o dell'oggetto selezionato: misura, vista laterale o dall'alto, tema, ora del giorno, musica, telecamera, partenza dell'eroe), *Mondo* (titolo, genere, fisica), *Strati* (strati dello sfondo e decorazioni).
- Le **porte** si collegano scegliendo nelle Proprietà la mappa d'arrivo e poi il punto d'arrivo.
- Ctrl+C / Ctrl+V copiano e incollano la selezione, Canc la cancella.

## 4. Personaggi

A sinistra l'elenco (Nuovo, Duplica, Elimina), al centro l'**editor di disegno**, a destra le proprietà.

**Strumenti di disegno**: Matita (P), Gomma (E), Secchio (F), Linea (L), Rettangolo (R), Ellisse (O), Contagocce (I), Sposta (M).
`[` e `]` cambiano la punta, X scambia i due colori, `,` e `.` sfogliano i fotogrammi.
Tasto sinistro = colore 1, destro = colore 2. Opzioni: forme piene, simmetria, cipolla (vedi il fotogramma precedente), griglia.

**Colori**: quadrato colore/luminosità, barra della tinta, codice esadecimale, opacità e tavolozza (colori base + colori già usati).

**Animazioni**: anteprima che si muove, velocità (fps), ciclo. Il motore usa questi nomi:
`fermo`, `corsa`, `salto`, `caduta`, `colpito`, `attacco`, `vittoria`. Se un'animazione manca usa `fermo`.

**Fotogrammi** (striscia in basso): + Vuoto, + Copia, sposta a sinistra/destra, Elimina.

**Proprietà**: nome, **È l'eroe** (diventa il personaggio che guidi), **Metti in mappa**, comportamento
(Eroe, Fermo, Pattuglia, Insegue l'eroe, Vola, Torretta che spara, Boss, Parla), velocità, salto, energia, danno,
vista, cadenza d'attacco, punti, gravità, se si sconfigge saltandoci sopra, proiettile e suoni (salto, colpito, attacco, sconfitto).

Esempio con Claude: *"Disegna un drago verde che sputa fuoco, con animazioni di volo e attacco, come boss"*.
Il drago viene salvato nel gioco: lo usi nelle mappe, come eroe o come boss, e resta nel file `.gioco`.

## 5. Oggetti

Come i personaggi, ma con una **funzione**: Decorazione, Da raccogliere (punti), Cura, Chiave, Porta (si apre con…),
Molla, Cassa da spingere, Interruttore/leva, Piattaforma mobile, Pericolo, Arrivo (vittoria), Proiettile.
Nomi di animazione usati dal motore: porte `chiusa`/`aperta`, interruttori `spenta`/`accesa`, molle `scatta`, forzieri `chiuso`/`aperto`.

## 6. Scenari

- Scheda **Scenari**: sfondi a strati (montagne, colline, foresta, nuvole, stelle, fasce, immagini importate) con parallasse,
  colori del cielo, ora del giorno, pioggia e neve. Anteprima in alta definizione con un terreno d'esempio.
- Scheda **Caselle**: disegni le caselle del terreno (solida, piattaforma, pericolo, decorazione…) con lo stesso editor di disegno.

## 7. Audio

- **Musica**: piano roll a più tracce (melodia, basso, arpeggio, batteria…), strumenti, tempo, ascolto dal vivo,
  "Componi" crea un brano in uno stile (Allegro, Epico, Calmo, Misterioso, Spaziale, Battaglia). Si importano anche OGG/MP3/WAV.
- **Effetti**: sintetizzatore (onda, frequenze, durata, vibrato, rumore) con forma d'onda, effetti pronti, esporta in WAV.
- **Suoni degli eventi**: quale effetto suona per salto, moneta, colpo…; e la musica di ogni mappa.

## 8. Logica

Le **regole** si leggono come frasi: **QUANDO** *chi* *evento* (con chi) **SE** *variabile confronto valore* **ALLORA** *azioni*.

Eventi: inizia la mappa, tocca, raccoglie, colpisce, viene sconfitto, ha energia sotto, si avvicina a, viene ferito,
salta, tasto premuto, clic del mouse, pulsante del joypad, ogni tot secondi, variabile cambia.

Azioni: suona l'effetto, metti/ferma la musica, mostra il messaggio, aumenta/imposta la variabile, dai punti, cuori,
energia, vai alla mappa, crea, togli di mezzo, cambia animazione, velocità, fai saltare, spara, colora, apri le porte,
scuoti lo schermo, spingi indietro l'eroe, vittoria, game over.

A destra le **variabili** del gioco con il loro valore iniziale.

## 9. Librerie

In alto la **libreria del gioco** (tutto quello che il gioco contiene, per categoria). Sotto le **22 raccolte pronte**:
aggiungi un singolo elemento o l'intera raccolta. Ogni raccolta esiste in pixel art fine e in alta definizione.

## 10. Comandi

Per ogni azione (sinistra, destra, su, giù, salto, conferma, indietro, pausa) due tasti e un pulsante del joypad:
clic sulla casella e premi il nuovo tasto. Mouse sì/no, zona morta delle levette, **prova dal vivo**.
Le regole con evento "tasto premuto" o "pulsante del joypad" compaiono qui come comandi extra.
I comandi sono salvati in `comandi.cfg` e valgono anche per i giochi creati.

## 11. Claude in ogni sezione

Claude vede l'intero gioco, sa in quale sezione sei e cosa hai selezionato. Puoi chiedere di creare o cambiare
personaggi, oggetti, caselle, scenari, brani, effetti, regole, variabili, mappe; di scegliere l'eroe; di aggiungere una raccolta.
Ogni risposta viene **controllata** prima di entrare nel gioco: le parti sbagliate vengono scartate e mostrate,
il resto si applica e si annulla con Ctrl+Z. Dopo la risposta l'officina ti porta a ciò che è stato creato.

Esempi:
- *"Fammi un platform completo con un drago come boss finale"*
- *"Aggiungi piattaforme e 10 monete fino alla bandiera"*
- *"Quando l'eroe raccoglie 5 chiavi apri la porta del castello e suona una fanfara"*
- *"Componi una musica misteriosa per la grotta"*

## 12. Creare l'eseguibile

**Crea .exe** (o Ctrl+E) controlla il gioco e crea `<Titolo>.exe` accanto allo studio. Il gioco creato parte con:

1. intro con il logo Elvantis, "Mario Petramale presenta", il titolo e la musica;
2. "Premi un tasto";
3. menu: **1 giocatore**, **2 giocatori (a turni)**, **Opzioni** (musica, effetti, audio, schermo intero), **Crediti**, **Esci**;
4. il gioco; **Esc** apre la pausa (Riprendi, Ricomincia, Menu principale).

Con 2 giocatori ognuno gioca la sua partita e alla fine si confrontano i punti.
L'eseguibile è un solo file: si copia su qualsiasi PC Windows e funziona senza installare nulla.

## 13. Tasti utili

| Tasto | Dove | Cosa fa |
| --- | --- | --- |
| F1 | officina | aiuto |
| F2 / F3 | officina | apri-chiudi la chat / scrivi a Claude |
| F5 | ovunque | prova il gioco |
| F8 | ovunque | audio acceso/spento |
| F11 | ovunque | schermo intero |
| Alt+1…8 | officina | cambia sezione |
| Ctrl+S, Ctrl+O, Ctrl+N, Ctrl+E | officina | salva, apri, nuovo, crea .exe |
| Ctrl+Z, Ctrl+Y | officina | annulla, ripeti |

## 14. File e cartelle

| Percorso | Contenuto |
| --- | --- |
| `giochi\*.gioco` | i tuoi giochi (un file ciascuno, con immagini e suoni dentro) |
| `giochi_recenti.txt` | gli ultimi giochi aperti |
| `immagini\` | immagini importate nelle mappe |
| `claude_chiave.txt` | la chiave di Claude (non condividerla) |
| `comandi.cfg`, `studio.opzioni` | comandi e opzioni audio |
