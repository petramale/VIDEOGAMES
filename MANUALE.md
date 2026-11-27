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
  In alto a destra lo stato di Claude, la **nota** (musica dello studio, F7) e l'altoparlante (clic o F8 per spegnere tutto l'audio).

**Musica dello studio**: è spenta di serie. La nota in alto a destra, il tasto F7 o il menu Gioco → Musica dello studio
la accendono o la spengono. Gli effetti sonori restano attivi, e la musica suona comunque quando provi il gioco (F5)
o ascolti un brano nella sezione Audio. I giochi creati hanno la loro musica, regolabile dal menu Opzioni del gioco.
- **Schede delle sezioni**: Home, Mondi, Personaggi, Oggetti, Scenari, Audio, Logica, Librerie, Comandi, Comportamenti
  (anche con **Alt+1 … Alt+9**). A destra **Prova F5** e **Crea .exe**.
- **Chat di Claude** a destra (F2 la apre o la chiude, F3 porta il cursore nella chat).
- **Aiuto sui pulsanti**: fermati con il mouse su un pulsante, una scelta, un campo o una casella della tavolozza
  e dopo un attimo compare un riquadro che spiega a cosa serve. Vale in tutte le sezioni. Se il nome di una scelta
  è troppo lungo e si vede accorciato con "...", il riquadro lo mostra per intero.

### Il menu Gioco

| Voce | Tasto | Cosa fa |
| --- | --- | --- |
| Nuovo gioco | Ctrl+N | Sceglie genere (platform, open world, sparatutto, avventura, labirinto, puzzle), stile grafico e titolo |
| Apri | Ctrl+O | Apre un `.gioco` salvato o uno dei mondi d'esempio |
| Salva gioco | Ctrl+S | Salva in `giochi\<titolo>.gioco` |
| Salva con nome | Ctrl+Maiusc+S | Salva una copia con un nome nuovo |
| Prova il gioco | F5 | Gioca subito; Esc o F5 torna all'officina, R ricomincia, F1 mostra le collisioni, **F7 musica** e **F6 effetti sonori** si spengono separatamente (anche con i pulsanti in basso a destra) |
| Crea il gioco .exe | Ctrl+E | Crea l'eseguibile accanto allo studio |
| Stile grafico | | Passa da **pixel art fine** a **disegnato in alta definizione** (e viceversa) |
| Annulla / Ripeti | Ctrl+Z / Ctrl+Y | Vale anche per le modifiche fatte da Claude |
| Musica dello studio | F7 | Accende o spegne la musica di sottofondo dello studio (spenta di serie) |
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

### Selezionare, cancellare, copiare

- **Un oggetto o un personaggio**: strumento **Seleziona (V)** e clic sull'oggetto. Si sposta trascinandolo,
  **Canc** (o Backspace) lo cancella, **Ctrl+C** e poi **Ctrl+V** lo duplicano. Le sue proprietà compaiono a destra.
- **Un gruppo di blocchi**: con **Seleziona (V)** clicca su un punto senza oggetti e **trascina**: compare un
  riquadro azzurro. Poi **Canc** cancella tutte le caselle e gli oggetti dentro il riquadro, **Ctrl+C** li copia
  e **Ctrl+V** li incolla dove c'è il mouse. Gli stessi comandi sono anche pulsanti nel pannello a destra
  ("Blocchi selezionati"). **Esc** toglie la selezione.
- **Cancellare a mano**: strumento **Gomma (E)** oppure il **tasto destro** sulla mappa con qualunque strumento.
- Attenzione: cliccando una casella o un oggetto della tavolozza lo strumento torna Pennello, per disegnare subito.
- Tutto si annulla con **Ctrl+Z**.

## 4. Personaggi

A sinistra l'elenco (Nuovo, Duplica, Elimina), al centro l'**editor di disegno**, a destra le proprietà.

**Strumenti di disegno**: Matita (P), Gomma (E), Secchio (F), Linea (L), Rettangolo (R), Ellisse (O), Contagocce (I), Sposta (M).
`[` e `]` cambiano la punta, X scambia i due colori, `,` e `.` sfogliano i fotogrammi.
Tasto sinistro = colore 1, destro = colore 2. Opzioni: forme piene, simmetria, cipolla (vedi il fotogramma precedente), griglia.

**Colori**: quadrato colore/luminosità, barra della tinta, codice esadecimale, opacità e tavolozza (colori base + colori già usati).

**Animazioni**: anteprima che si muove, velocità (fps), ciclo. Il motore usa questi nomi:
`fermo`, `corsa`, `salto`, `caduta`, `colpito`, `attacco`, `sparo` (l'eroe appena spara), `vittoria`. Se un'animazione manca usa `fermo`.
Le altre animazioni (con il nome che vuoi) si avviano dalla Logica con l'azione "cambia animazione".

**Gestore delle immagini** (pulsante **Carica immagini…** sotto l'elenco delle animazioni, oppure trascina i file
sull'editor): carica **PNG, BMP, JPG, GIF (anche animate), TGA, QOI**, uno o più file insieme.
- Più file (es. `corsa_01.png`, `corsa_02.png`…) diventano fotogrammi in ordine di nome.
- Un **foglio di sprite** si divide in **colonne × righe**: lo studio le indovina da solo guardando gli spazi vuoti
  fra i disegni, e puoi correggerle; la griglia gialla mostra il taglio. Le celle vuote si saltano.
- Una **GIF animata** dà tutti i suoi fotogrammi.
- **Animazione**: quella aperta, una di quelle del motore (fermo, corsa, salto, caduta, colpito, attacco, sparo,
  vittoria, aperta/chiusa, accesa/spenta) o una nuova con il nome che scrivi. "Aggiungi in coda" o "Sostituisci".
- **Misura**: allarga il disegno fino a 256 pixel, rimpicciolisci le immagini nel disegno, oppure porta il disegno
  alla misura delle immagini. **Allinea**: piedi in basso (per i personaggi), al centro, in alto a sinistra.
- **Sfondo trasparente**: BMP e JPG non hanno trasparenza; questa scelta toglie il colore dell'angolo in alto a sinistra.
- **Velocità (fps)** e anteprima animata prima di importare. Dopo l'importazione ogni fotogramma si ritocca
  con gli strumenti di disegno e si riordina con < e > nella striscia dei fotogrammi.

**Fotogrammi** (striscia in basso, su tutta la larghezza): in alto i pulsanti + Vuoto, + Copia, < > (sposta), Elimina
(su schermi piccoli diventano +, Copia, <, >, X); sotto la sequenza dei fotogrammi, che scorre con la rotella
o con la barra se è lunga. Il fotogramma scelto resta sempre in vista.

**Proprietà**: nome, **È l'eroe** (diventa il personaggio che guidi), **Metti in mappa**, comportamento
(Eroe, Fermo, Pattuglia, Insegue l'eroe, Vola, Torretta che spara, Boss, Parla), velocità, salto, energia, danno,
vista, cadenza d'attacco, punti, gravità, se si sconfigge saltandoci sopra, proiettile e suoni (salto, colpito, attacco, sconfitto),
**Luce ed effetti** (vedi il capitolo 7b).

Esempio con Claude: *"Disegna un drago verde che sputa fuoco, con animazioni di volo e attacco, come boss"*.
Il drago viene salvato nel gioco: lo usi nelle mappe, come eroe o come boss, e resta nel file `.gioco`.

## 5. Oggetti

Come i personaggi, ma con una **funzione**: Decorazione, Da raccogliere (punti), Cura, Chiave, Porta (si apre con…),
Molla, Cassa da spingere, Interruttore/leva, Piattaforma mobile, Pericolo, Arrivo (vittoria), Proiettile.
Nomi di animazione usati dal motore: porte `chiusa`/`aperta`, interruttori `spenta`/`accesa`, molle `scatta`, forzieri `chiuso`/`aperto`.
Anche gli oggetti hanno **Luce ed effetti** (una lanterna appesa, un cristallo che pulsa, una torcia: capitolo 7b)
e il **Gestore delle immagini** per caricare i loro disegni.

## 5b. Animazioni (lo studio di animazione)

Scheda **Animazioni** nella barra in alto (icona della pellicola, anche nella Home). Vale per **personaggi e oggetti**.

1. **A sinistra** scegli il personaggio o l'oggetto (sotto il nome: quante animazioni ha).
2. **A destra** l'elenco delle sue animazioni con la miniatura; **+ Nuova**, **Duplica**, **Elimina**.
3. **Carica immagini (PNG, JPG, BMP, GIF)…**: si apre il **Gestore delle immagini**. Puoi scegliere più file
   (un file = un fotogramma), un **foglio di sprite** (righe × colonne trovate da sole, puoi correggerle),
   o una **GIF animata** (ogni quadro diventa un fotogramma). Sfondo trasparente dal colore dell'angolo,
   misura, allineamento e velocità; **Importa** e i fotogrammi entrano nell'animazione scelta.
4. **Al centro** l'**anteprima che si muove** (Pausa / Riproduci) su un fondo a scacchi, con la linea del terreno
   e una **croce gialla** = il **punto d'appoggio** (i piedi). **Trascina il disegno** con il mouse per mettere i piedi
   sulla croce; oppure usa "Sposta a destra / Sposta in basso" e **Rimetti al centro**.
5. **In basso la striscia dei fotogrammi** (la timeline): clic per vederne uno, **trascinali per cambiare l'ordine**
   (una linea gialla mostra dove finiscono), **Duplica**, **Elimina**, **Inverti ordine**, **Specchia**, **+ immagini**.
6. **Si usa per**: a quale azione del gioco va l'animazione. Personaggi: fermo, corsa, salto, caduta, colpito,
   attacco, sparo, vittoria. Oggetti: fermo, aperta/chiusa (porte), accesa/spenta (interruttori), scatta (molle),
   aperto/chiuso (forzieri). Con "(nome mio)" la scegli tu e la avvii dalla Logica con "anima".
7. **Velocità (fps)** e **Si ripete (ciclo)**. **Ritocca i pixel** porta all'editor di disegno.

Claude è nella chat a destra: *"fai l'animazione di corsa con 6 fotogrammi"*.

## 6. Scenari

- Scheda **Scenari**: sfondi a strati (montagne, colline, foresta, nuvole, stelle, fasce, immagini importate) con parallasse,
  colori del cielo, ora del giorno, pioggia e neve. Anteprima in alta definizione con un terreno d'esempio.
- Ogni mappa usa **uno** scenario. Uno scenario nuovo (o duplicato) viene usato subito nella mappa aperta;
  per cambiare scenario a una mappa usa "Usa nella mappa aperta" oppure la scelta **Scenario** nelle Proprietà della mappa
  (sezione Mondi). Con "(sfondo della mappa)" la mappa torna al suo sfondo semplice.
- Scheda **Caselle**: disegni le caselle del terreno (solida, piattaforma, pericolo, decorazione…) con lo stesso editor di disegno.

## 7. Audio

- **Musica**: piano roll a più tracce (melodia, basso, arpeggio, batteria…), strumenti, tempo, ascolto dal vivo,
  "Componi" crea un brano in uno stile (Allegro, Epico, Calmo, Misterioso, Spaziale, Battaglia).
- **Carica musica (MP3, WAV, OGG, MIDI)**: il grande pulsante in cima alla scheda Musica (oppure trascina il file nella finestra).
  Puoi scegliere più file insieme. Ogni brano caricato compare nell'**elenco a sinistra** (sotto il nome c'è il tipo:
  "MP3 caricato", "da MIDI", "a note" e "in uso" se un livello lo usa): sceglilo e premi **Ascolta** / **Ferma**.
  Per un brano caricato, sotto c'è **In quali livelli suona**: accendi l'interruttore dei livelli che devono usarlo.
  Nei giochi arcade la musica di ogni livello/settore/missione, del boss e del menu si sceglie nella scheda
  **Gioco arcade**, nel campo **Musica**, con **Ascolta** e **Carica…** per prendere subito un MP3/WAV/OGG/MIDI tuo.
  - **MP3, OGG, WAV, FLAC, XM, MOD**: il file entra nel gioco così com'è e suona anche nell'eseguibile creato.
    Lo usi come musica di una mappa ("Usa nella mappa aperta") o dalla Logica ("metti la musica").
  - **MIDI (.mid)**: le note diventano un **brano a note** che vedi e modifichi nel rullo, nota per nota:
    fino a 6 tracce (la batteria del canale 10 e le 5 parti con più note), il tempo del file e
    ogni strumento General MIDI trasformato nell'onda più simile (pianoforte → triangolo, organo e ottoni → quadra,
    chitarre e archi → dente di sega, flauti e pad → seno). Puoi cambiare strumento, volume e note come in ogni brano.
- **Effetti**: sintetizzatore (onda, frequenze, durata, vibrato, rumore) con forma d'onda, **40 effetti pronti**
  (i 20 arcade e i 20 speciali: grande esplosione, tuono, vento, fulmine, teletrasporto, incantesimo, campanella, gong,
  fiamma accesa, lanterna, luce che si spegne, raggio laser, scudo, power-up, livello superato, allarme, cigolio,
  splash, bolle, battito del cuore), esporta in WAV. **Carica effetto…** importa WAV, OGG, MP3.
- **Suoni degli eventi**: quale effetto suona per salto, moneta, colpo…; e la musica di ogni mappa.

## 7b. Luci ed effetti speciali (l'effetto lanterna)

**Come funziona la lanterna di Lia** (Lumina): lo scenario della mappa è **buio** (Scenari → *Buio*, da 0 a 1).
Al buio il motore copre la scena con un velo scuro e apre dei "buchi" di luce morbida intorno a chi porta una luce:
l'eroe ha di serie una **lanterna** calda che tremola come una fiamma, i nemici hanno gli occhi che brillano,
monete, chiavi, punti di controllo accesi e proiettili fanno un po' di luce. La lanterna che si vede in mano a Lia
è disegnata nel suo sprite; la luce intorno la fa il motore.

**Luce ed effetti** (nelle proprietà di ogni personaggio e oggetto):
- **Luce**: *Automatica* (come sopra), *Luce propria* o *Nessuna luce*.
- Con la luce propria scegli **colore**, **raggio**, **intensità**, **tremolio** (fiamma che vibra: candele, torce,
  lanterne), **pulsazione** (respira lentamente: cristalli, magie), **alone colorato** (tinge l'aria intorno,
  si vede anche di giorno) e **altezza** della luce sopra i piedi. L'anteprima mostra la luce al buio.
- **Scia**: scia di luce, scintille, fumo, stelline, bolle, fiammelle, foglie, neve o cuori, con il suo colore:
  esce dietro a chi si muove (un razzo, una fata, un fantasma).

**Nella Logica** (azioni nuove):
- **effetto speciale**: scintille, esplosione, grande esplosione (con scossa e lampo), fumo, stelle, cuori, magia,
  fuochi d'artificio, bolle, polvere, fulmine, teletrasporto, anello di luce, pioggia d'oro; il numero è la grandezza.
  Esce su chi ha fatto scattare la regola (o su tutti quelli che si chiamano come "chi"), altrimenti sull'eroe.
- **lampo sullo schermo**: un colore che copre tutto per un attimo (il numero è la durata in secondi).
- **accendi/spegni la luce**: accende, spegne o colora (rossa, verde, blu, gialla, bianca, viola, azzurra)
  la luce dell'eroe o di chi fa scattare la regola; il numero è il raggio.
- **buio della mappa**: porta piano piano il buio al valore scelto (0 giorno pieno, 1 buio pesto).
- **rallentatore**: per qualche secondo tutto va al 40% della velocità.

Esempi: *quando l'eroe raccoglie la Lanterna → accendi la luce gialla 160*; *quando il Boss viene sconfitto →
effetto grande esplosione, lampo bianco 0.3, buio della mappa 0*; *quando l'eroe tocca l'Interruttore →
buio della mappa 0.9, suona "Tuono"*.

## 8. Logica

Le **regole** si leggono come frasi: **QUANDO** *chi* *evento* (con chi) **SE** *variabile confronto valore* **ALLORA** *azioni*.

Eventi: inizia la mappa, tocca, raccoglie, colpisce, viene sconfitto, ha energia sotto, si avvicina a, viene ferito,
salta, tasto premuto, clic del mouse, pulsante del joypad, ogni tot secondi, variabile cambia.

Azioni: suona l'effetto, metti/ferma la musica, mostra il messaggio, aumenta/imposta la variabile, dai punti, cuori,
energia, vai alla mappa, crea, togli di mezzo, cambia animazione, velocità, fai saltare, spara, colora, apri le porte,
scuoti lo schermo, spingi indietro l'eroe, vittoria, game over, effetto speciale, lampo sullo schermo,
accendi/spegni la luce, buio della mappa, rallentatore (capitolo 7b).

A destra le **variabili** del gioco con il loro valore iniziale.

## 9. Librerie

In alto la **libreria del gioco** (tutto quello che il gioco contiene, per categoria). Sotto le **23 raccolte pronte** (nuova: *Suoni speciali*):
aggiungi un singolo elemento o l'intera raccolta. Ogni raccolta esiste in pixel art fine e in alta definizione.

## 10. Comandi

Per ogni azione (sinistra, destra, su, giù, salto, conferma, indietro, pausa) due tasti e un pulsante del joypad:
clic sulla casella e premi il nuovo tasto. Mouse sì/no, zona morta delle levette, **prova dal vivo**.
Le regole con evento "tasto premuto" o "pulsante del joypad" compaiono qui come comandi extra.
I comandi sono salvati in `comandi.cfg` e valgono anche per i giochi creati.

## 11. Comportamenti: la mente delle creature

La sezione **Comportamenti** (**Alt+9**) decide come pensano nemici, personaggi e oggetti. Non serve toccarla:
ogni personaggio ha già una mente **istintiva** ricavata dal suo tipo. Qui la rendi tua.

**Come pensa una creatura.** Dieci volte al secondo guarda cosa ha intorno (vede l'eroe? è ferita? ci sono compagni,
una cura, un proiettile in arrivo?), sente delle **emozioni** (minaccia, paura, fiducia verso l'eroe, stanchezza)
e ricorda gli **ultimi fatti** (dove ha visto l'eroe, chi l'ha colpita, chi l'ha risparmiata). Poi dà un punteggio a
13 azioni (pattuglia, insegui, attacca, aggira, cerca, fuggi, curati, chiama aiuto, schiva, guardia, riposa, amico,
rivale) e sceglie la migliore. Non cambia idea a scatti: resta sull'azione in corso per un attimo.
Le creature della **stessa specie imparano insieme** come le sconfiggi: se le schiacci saltando impareranno a
schivare i salti, se spari a tenersi a distanza. L'apprendimento ha un limite fisso: il gioco resta sempre vincibile,
e se l'eroe è in difficoltà le creature allentano un po' (*pietà*).

A sinistra ci sono i **profili** (+ Nuovo, Duplica, Elimina, oppure **Crea dal modello**: Istintivo, Lupo di branco,
Guardiano rancoroso, Codardo furbo, Cacciatore paziente, Abitante cordiale…). Al centro le schede:

| Scheda | Cosa si sceglie |
| --- | --- |
| **Carattere** | coraggio, aggressività, prudenza, socievolezza, rancore, curiosità, imprevedibilità (da 0 a 1); quanto impara, quanto cambia da una partita all'altra, sotto quale energia pensa a fuggire |
| **Azioni** | il peso di ogni azione: 0 = mai, 1 = normale, fino a 3 = la preferita |
| **Reazioni** | chiama aiuto, impara con la specie, **ricorda nella partita dopo**; le frasi degli abitanti secondo l'umore (amico, neutrale, ostile, impaurito) |
| **Chi lo usa** | clic su un personaggio per dargli il profilo; "Dallo a tutti i nemici" |
| **Oggetti** | l'**indole** degli oggetti: usura (una molla si rompe dopo qualche salto), ricarica (torna come nuova, anche monete e cure ricrescono), si rinforza o si indebolisce con l'uso, pericoli notturni o che si svegliano quando ti avvicini |
| **Partita unica** | come cambia ogni partita (vedi sotto) |

A destra il **simulatore**: cambi la situazione (distanza, energia, compagni, paura…) e vedi subito quale azione
sceglierebbe la creatura e con che punteggio, con gli stessi calcoli del gioco. **Prova con i pensieri** avvia il
gioco mostrando sopra ogni creatura cosa sta pensando (nella prova e nei giochi creati si accendono e spengono con **F9**).
Anche dalla sezione **Personaggi** puoi scegliere il **Carattere** di un personaggio e aprirlo qui.

Le regole della **Logica** conoscono la mente: eventi *diventa amico*, *chiama aiuto*, *fugge*; azioni *fiducia* e *minaccia*
(da -1 a 1) per cambiare l'umore di una creatura.

Esempi per Claude in questa sezione:
- *"Crea un profilo per lupi che cacciano in branco e dallo ai lupi"*
- *"Rendi il mercante diffidente: si fida solo se lo aiuti"*
- *"Fai che le molle si consumino dopo tre salti e tornino dopo 20 secondi"*

### Ogni partita è diversa

Ogni partita parte da un **seme** e da lì ricava tutto, sempre in modo coerente: le mappe restano quelle che hai
disegnato, cambia come le vivi. Nella scheda **Partita unica** scegli quanta varietà (spenta, leggera, media, forte) e cosa può cambiare:
carattere delle creature, **veterani** più forti o creature timide, monete e cure che si scambiano di posto,
ora del giorno e pioggia, specie **alleate o rivali** (si attaccano fra loro), fiducia iniziale degli abitanti, e un
**evento speciale** per partita: *Luna rossa*, *Nebbia*, *Giorno di festa*, *Stagione della caccia*, *Giornata calma*,
*Tesoro nascosto* (puoi vietare quelli che non vuoi). Le **variabili a caso** ricevono un valore fra minimo e massimo
a inizio partita e le regole le possono usare. Con un **seme fisso** ogni partita è identica (utile per provare un livello).

### Salvare e caricare nei giochi creati

I giochi creati salvano la partita **completa**: mappa ed eroe, energia, punti, inventario, variabili, porte aperte e oggetti
raccolti in ogni mappa, ogni creatura con le sue emozioni e i suoi ricordi, ciò che le specie hanno imparato, il seme
e l'evento della partita. Caricando, la partita riprende esattamente da dove era.

- Menu principale: **Continua** (il salvataggio più recente) e **Carica partita**.
- Pausa (**Esc**): **Salva partita** e **Carica partita**, con **tre spazi** che mostrano mappa, punti, tempo e data.
- **Salvataggio automatico** a ogni cambio di mappa, a ogni punto di controllo e quando chiudi la finestra.
- I file sono accanto all'exe (`<Gioco>.slot1.sav` … `slot3`, `<Gioco>.auto.sav`); se la cartella non è scrivibile,
  nella cartella dell'utente (`%APPDATA%\Elvantis`). Le specie che "ricordano nella partita dopo" usano `<Gioco>.memoria`.
- **Sicurezza**: ogni salvataggio ha un codice di controllo; si scrive prima un file temporaneo e il precedente resta come
  copia `.bak`. Un file rovinato o troncato non si carica e si usa la copia. Un salvataggio di una versione più nuova del
  gioco non si carica; uno vecchio viene aggiornato. Se hai cambiato il gioco dopo aver salvato, la partita si carica lo stesso
  e un avviso dice cosa è stato sistemato.

## 12. Claude in ogni sezione

Claude vede l'intero gioco, sa in quale sezione sei e cosa hai selezionato. Puoi chiedere di creare o cambiare
personaggi, oggetti, caselle, scenari, brani, effetti, regole, variabili, mappe, profili di comportamento, indole degli oggetti, varietà della partita; di scegliere l'eroe; di aggiungere una raccolta.
Ogni risposta viene **controllata** prima di entrare nel gioco: le parti sbagliate vengono scartate e mostrate,
il resto si applica e si annulla con Ctrl+Z. Dopo la risposta l'officina ti porta a ciò che è stato creato.

Esempi:
- *"Fammi un platform completo con un drago come boss finale"*
- *"Aggiungi piattaforme e 10 monete fino alla bandiera"*
- *"Quando l'eroe raccoglie 5 chiavi apri la porta del castello e suona una fanfara"*
- *"Componi una musica misteriosa per la grotta"*

## 13. Creare l'eseguibile

**Crea .exe** (o Ctrl+E) controlla il gioco e crea `<Titolo>.exe` accanto allo studio. Il gioco creato parte con:

1. intro con il logo Elvantis, "Mario Petramale presenta", il titolo e la musica;
2. "Premi un tasto";
3. menu: **1 giocatore**, **2 giocatori (a turni)**, **Opzioni** (musica, effetti, audio, schermo intero), **Crediti**, **Esci**;
4. il gioco; **Esc** apre la pausa (Riprendi, Salva partita, Carica partita, Ricomincia, Musica, Effetti sonori, Menu principale).
   Nel menu c'è anche **Continua** quando esiste un salvataggio (vedi sezione 11).

Nel gioco creato musica ed effetti sonori si spengono **separatamente**: **F7** la musica, **F6** gli effetti, **F8** tutto
l'audio, oppure con i tre pulsanti in basso a destra, il menu Opzioni o la pausa. La scelta resta salvata accanto al gioco.

Con 2 giocatori ognuno gioca la sua partita e alla fine si confrontano i punti.
L'eseguibile è un solo file: si copia su qualsiasi PC Windows e funziona senza installare nulla.

**Licenza.** I giochi creati si possono regalare e vendere, a patto di non togliere l'intro Elvantis, i crediti
e gli avvisi delle licenze, e di non estrarre il motore o le risorse incluse. Nei **Crediti** del gioco il tasto
**L** mostra la licenza di Studio Videogiochi e quelle delle librerie open source. I disegni, le musiche e le
mappe che crei sono tuoi; immagini e suoni importati devono essere tuoi o usabili liberamente.
La licenza completa dello studio è in `LICENSE.txt`, gli avvisi delle librerie nella cartella `licenses`.

## 14. I tre grandi giochi di piattaforme

Nel menu **Gioco → Apri** ci sono tre giochi completi, in fondo all'elenco (**Grandi giochi di piattaforme**).
Si aprono come qualsiasi altro gioco: si provano con **F5**, si cambiano in tutte le sezioni, si salvano con
**Ctrl+S** e si trasformano in .exe con **Ctrl+E**. Ognuno ha 6 mondi con 3 livelli e un guardiano (24 mappe),
30 nemici e 6 boss disegnati apposta, 8 musiche, effetti sonori propri e due eroi (il secondo è quello del giocatore 2).

| Gioco | Stile | Eroi | Mondi |
| --- | --- | --- | --- |
| **Lumina: le Lanterne del Mondo Spento** | disegnato HD | Lia e la lucciola Pip | Bosco delle Lucciole, Miniere di Cristallo, Città dei Tetti, Ghiacciaio dell'Aurora, Fonderia dei Vapori, Torre dell'Eclissi |
| **Nora e Bullone: gli Ingranaggi del Tempo** | pixel art fine | Nora e il robot Bullone | Preistoria, Antico Egitto, Medioevo, Caraibi, Far West, Anno 3000 |
| **Nilo e il Regno Capovolto** | pixel art fine | Nilo e il pinguino Fiocco | Isole del Vento, Giardino dei Giganti, Abissi di Corallo, Fabbrica dei Giocattoli, Cuore del Vulcano, Castello Sottosopra |

Come si gioca: frecce o levetta per correre, **Spazio** salta (premuto di nuovo in aria fa il **doppio salto**),
**X** lancia (raggio di luce, chiave inglese, stelle...). Le **lanterne / i lampioni / le bandierine** sono punti di
controllo: toccati, da lì si riparte. Le **chiavi** aprono le porte chiuse; il cancello alla fine del livello del
guardiano si apre solo dopo averlo sconfitto. Le **molle** (fiori, funghi) fanno saltare altissimo.

Novità del motore usate da questi giochi, disponibili anche nei tuoi:
- **Buio** (Scenari → Buio, da 0 a 1): si vede solo intorno all'eroe, alle monete, alle lanterne e ai colpi. In *Lumina* ogni regno è al buio.
- **Sott'acqua** (Scenari → Sott'acqua): si nuota premendo più volte il salto, si affonda piano (*Abissi di Corallo*).
- **Doppio salto** (Personaggi → l'eroe → Salti in aria).
- **Punto di controllo** (Oggetti → funzione "Punto di controllo").
- **Caselle legate a uno scenario**: ogni mondo ha il suo terreno; nell'editor dei Mondi si vedono le caselle dello scenario della mappa.
- **Meteo** nuovi: lucciole, braci, polvere, bolle, foglie, polline, coriandoli, magia.
- **Secondo eroe** per il giocatore 2.

Gli stessi giochi si possono creare anche da riga di comando:
`StudioVideogiochi --crea-platform A|B|C file.gioco [Gioco.exe]` (A Lumina, B Nora e Bullone, C Nilo).

## 15. I giochi arcade

### Creare un tuo gioco arcade (in 4 passi)

1. Premi **Giochi arcade** nella Home (o la scheda **Gioco arcade** in alto), poi **+ Crea un gioco arcade nuovo**.
   (È la stessa finestra di **Gioco → Nuovo gioco** con il genere **Arcade**.)
2. Scrivi il **titolo**, scegli il **tipo**: *Sparatutto a ondate*, *Sparatutto a formazioni*,
   *Corri e spara (run-and-gun)* o *Avventura isometrica*; lascia "Gioco nuovo (1 livello da riempire)"
   e premi **Crea il gioco**. Nasce un gioco tuo con un livello già giocabile e tutto il materiale del tipo
   (nemici, boss, armi, sfondi, musiche, suoni).
3. Nella scheda **Gioco arcade**: a sinistra scegli la parte (Ondate/Livelli/Missioni, Nemici, Boss, Armi, Colori,
   Effetti sonori…), al centro l'elemento (**+ Nuovo** aggiunge un livello copiando l'ultimo, **Duplica**, **Elimina**),
   a destra i suoi valori (numeri con − e +, colori, scelte); l'anteprima cambia subito. Il pulsante
   **Come si usa?** riapre la guida.
4. **F5** per giocare, **Ctrl+S** per salvare il tuo gioco (file `.gioco`), **Crea .exe** per il gioco finito.
   Musica: il campo **Musica** di ogni livello (o settore/missione/località) con **Ascolta** e **Carica…** per un
   MP3/WAV/OGG/MIDI tuo; tutte le musiche e gli effetti anche nella scheda **Audio**. Puoi anche chiedere a Claude nella chat:
   *"aggiungi tre ondate sempre più difficili"*.

Con "Esempio completo da cambiare" (o "Apri e modifica" su un esempio) parti invece da uno dei quattro giochi pronti.

### Mettere i tuoi personaggi, paesaggi, suoni e musiche

Tutto si fa nella scheda **Gioco arcade**, scegliendo a sinistra la parte e al centro l'elemento:

| Cosa | Dove | Campo |
|---|---|---|
| L'eroe / la navicella | **Eroi e regole** (o Caccia/Cannone e regole, Eroe e regole) | **Grafica** |
| I nemici | **Nemici** (Insetti, Alieni) | **Grafica** |
| I boss | **Boss** | **Grafica** |
| Le persone e le cose delle scene (avventura) | **Elementi delle scene** | **Grafica** |
| Il paesaggio | **Ambienti** / **Settori** / **Cieli** / **Località** | **Sfondo** |
| La musica | Missioni / Settori / Cieli / Capitoli / Boss | **Musica** (Ascolta, Carica…) |
| I suoni | **Effetti sonori** | ogni suono: Ascolta o un tuo effetto della scheda Audio |

- **Grafica → Carica immagini…**: scegli una o più immagini (PNG, JPG, BMP, GIF animata, foglio di sprite);
  si apre il Gestore delle immagini, premi **Importa**. Nasce un personaggio con il nome dell'elemento e il gioco
  lo disegna al posto del disegno originale (la miniatura compare sotto il nome del campo).
  **Animazioni** apre lo studio di animazione: il gioco usa da solo le animazioni `fermo`, `corsa`, `salto`,
  `sparo`/`attacco` e `colpito` quando ci sono. Puoi anche scegliere con **<** e **>** un personaggio o un
  oggetto disegnato da te nelle schede **Personaggi** e **Oggetti** (che nei giochi arcade sono visibili).
  "(disegno del gioco)" rimette il disegno originale.
- **Sfondo → Carica immagine…**: l'immagine diventa uno scenario e lo sfondo di quel livello. **Modifica** apre la
  scheda **Scenari** (strati, cielo, movimento). Puoi anche scegliere uno scenario disegnato da te.
- I colori, i numeri e i comportamenti restano validi: la grafica cambia solo l'aspetto.

### I quattro esempi

Sono quattro giochi completi, scritti per lo Studio e modificabili in ogni parte. Si aprono dalla scheda
**Gioco arcade** ("Apri e modifica") o da **Gioco → Apri** (elenco **Giochi arcade**). Hanno lo stesso flusso dei giochi creati: intro Elvantis, menu con 1 o 2 giocatori (a turni),
opzioni, crediti, salvataggi; si giocano con tastiera, mouse o joypad.

| Gioco | Genere | Contenuto |
| --- | --- | --- |
| **Stellaria: l'Invasione dei Prismi** | sparatutto a ondate, disegnato HD | 30 ondate in 6 cieli, alieni di cristallo, disco bonus, potenziamenti, combo di colore |
| **Sciame di Orione** | sparatutto a formazioni, disegnato HD | 15 livelli in 5 settori, insetti che entrano in volo e si tuffano, un boss ogni 3 livelli, armi bonus |
| **Polvere da Sparo: Commando del West** | run-and-gun in pixel art fine | 10 missioni con boss, due pistoleri (giocatore 2), ostaggi che danno armi, carro corazzato "Bisonte", dinamite |
| **Il Regno di Querciaverde** | avventura isometrica dipinta | 12 capitoli e 60 località, missioni "!", mercato, ricette, combattimenti, zaino, mappa |

**Come si gioca a Querciaverde.** Tutto si fa con un clic: sugli alberi, le rocce, i campi e i pesci per
raccogliere (serve l'attrezzo giusto e costa energia), sulle persone con il **!** per avere una missione e con
il **?** per consegnarla, sui nemici per combattere (vinci se la tua **potenza**, data da arma e armatura, è almeno
quella scritta sopra di loro; perdi un po' di vita), sulle **frecce dorate** per cambiare località, sulla locanda
per riposare. Nei quattro angoli ci sono **Diario**, **Mappa** (viaggio rapido), **Lavori** (ricette) e **Zaino**;
nella barra in basso ritratto, oro, vita, energia, potenza, posti nello zaino e il cibo da usare al volo.
Un capitolo finisce quando tutte le sue missioni sono compiute. Si salva **in qualsiasi momento** dal menu di
pausa (Esc). Clic destro, **Z** o **B** chiudono le finestre; con il joypad la levetta muove il puntatore e
**X/Y** salta all'elemento successivo.

**Ogni partita è diversa.** Il seme della partita cambia le formazioni, i nemici, le capsule, i prezzi del
mercato e le borse di monete perse. Il **regista** (IA adattiva) osserva come giochi (precisione, vite perse,
tempo) e regola la sfida fra facile e difficile; in Querciaverde cambia quanta vita costano i combattimenti e i prezzi.

**Modificarli nello Studio.** La sezione **Gioco arcade** mostra le categorie del gioco aperto (per Querciaverde:
Capitoli, Località, Elementi delle scene, Missioni, Oggetti, Ricette, Nemici, Eroe e regole, Effetti sonori).
Ogni elemento si aggiunge, duplica, elimina e cambia; a destra l'**anteprima** mostra il pilota automatico che
gioca con i tuoi dati. **Prova da qui (F5)** parte dal livello/capitolo scelto, **Crea .exe** fa il gioco completo.
La musica dei giochi è nella sezione **Audio** (si riscrive nota per nota) e si sceglie nei campi "Musica";
gli effetti sonori si sostituiscono con quelli creati in Audio. Anche **Claude** può cambiare i dati
("aggiungi un capitolo nel deserto", "rendi i boss più forti").

Da riga di comando: `StudioVideogiochi --crea-arcade stellaria|sciame|polvere|querciaverde Gioco.exe [base.exe [progetto.gioco]]`;
`--prova-arcade nome secondi` fa giocare il pilota automatico e controlla i salvataggi.

## 16. Tasti utili

| Tasto | Dove | Cosa fa |
| --- | --- | --- |
| F1 | officina | aiuto |
| F2 / F3 | officina | apri-chiudi la chat / scrivi a Claude |
| F5 | ovunque | prova il gioco |
| F7 | officina | musica dello studio accesa/spenta (gli effetti restano) |
| F7 / F6 | prova del gioco e giochi creati | musica / effetti sonori del gioco, separati |
| F8 | ovunque | audio acceso/spento |
| F11 | ovunque | schermo intero |
| Alt+1…9 | officina | cambia sezione |
| F9 | prova del gioco e giochi creati | mostra/nasconde i pensieri delle creature |
| Ctrl+S, Ctrl+O, Ctrl+N, Ctrl+E | officina | salva, apri, nuovo, crea .exe |
| Ctrl+Z, Ctrl+Y | officina | annulla, ripeti |
| Canc / Backspace | Mondi | cancella l'oggetto o i blocchi selezionati |
| Ctrl+C, Ctrl+V | Mondi | copia e incolla l'oggetto o i blocchi selezionati |
| Esc | Mondi | toglie la selezione |

## 17. File e cartelle

| Percorso | Contenuto |
| --- | --- |
| `giochi\*.gioco` | i tuoi giochi (un file ciascuno, con immagini e suoni dentro) |
| `giochi_recenti.txt` | gli ultimi giochi aperti |
| `immagini\` | immagini importate nelle mappe |
| `claude_chiave.txt` | la chiave di Claude (non condividerla) |
| `comandi.cfg`, `studio.opzioni` | comandi e opzioni audio |
