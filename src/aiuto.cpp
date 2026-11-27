// Help contestuale: la spiegazione che compare fermando il mouse su un pulsante o un controllo.
// La chiave e' il testo del pulsante (o l'etichetta del controllo).
#include "ui.h"
#include <map>

namespace Ui {

static const std::map<std::string, const char*>& Dizionario() {
    static const std::map<std::string, const char*> d = {
        // --- Barra e officina ---
        { "Prova F5", "Prova il gioco subito, cosi' com'e' adesso. Esc o F5 per tornare all'officina." },
        { "Prova il gioco F5", "Prova il gioco subito, cosi' com'e' adesso. Esc o F5 per tornare all'officina." },
        { "PROVA (F5)", "Prova il gioco partendo dalla mappa aperta. Esc o F5 per tornare all'editor." },
        { "Crea .exe", "Crea il file .exe del gioco accanto allo studio: parte con l'intro Elvantis e il menu, e si copia su qualsiasi PC." },
        { "Crea il gioco", "Crea il nuovo gioco con il genere, lo stile e il titolo scelti." },
        { "Nuovo gioco", "Comincia un gioco nuovo: scegli genere, stile grafico e titolo (Ctrl+N)." },
        { "Apri un gioco", "Apre un gioco salvato o uno dei mondi d'esempio (Ctrl+O)." },
        { "Apri", "Apre l'elemento scelto." },
        { "Salva gioco", "Salva tutto il gioco nel file .gioco della cartella giochi (Ctrl+S)." },
        { "Salva", "Salva la modifica." },
        { "Inserisci la chiave", "Incolla la chiave API di Anthropic per parlare con Claude. Resta solo su questo PC, mai dentro i giochi." },
        { "Invia", "Manda il messaggio a Claude (anche con Invio). Claude vede il gioco e la sezione in cui sei." },
        { "Pulisci", "Cancella la conversazione con Claude (le modifiche gia' fatte restano)." },
        { "X", "Chiude o toglie l'elemento." },
        { "x", "Toglie questa voce." },
        { "OK", "Conferma." },
        { "...", "Claude sta pensando: aspetta la risposta." },
        { "Claude", "Parla con Claude per modificare il mondo con le parole." },
        { "Claude...", "Claude sta pensando: aspetta la risposta." },
        // --- Elenchi (personaggi, oggetti, scenari...) ---
        { "+ Nuovo", "Crea un elemento nuovo e lo seleziona." },
        { "+ Nuova", "Crea un'animazione nuova (parte dal fotogramma attuale)." },
        { "Duplica", "Fa una copia dell'elemento selezionato, con un nome nuovo." },
        { "Elimina", "Elimina l'elemento selezionato (si recupera con Annulla, Ctrl+Z)." },
        { "Su", "Sposta su l'elemento selezionato." },
        { "Giu'", "Sposta giu' l'elemento selezionato." },
        { "Sposta su", "Sposta questa regola piu' in alto: le regole si controllano dall'alto in basso." },
        { "Sposta giu'", "Sposta questa regola piu' in basso." },
        { "Togli", "Toglie l'elemento selezionato." },
        // --- Personaggi e oggetti ---
        { "E' l'eroe", "Questo personaggio e' l'eroe guidato dal giocatore." },
        { "Usa come eroe", "Fa diventare questo personaggio l'eroe guidato dal giocatore." },
        { "Metti in mappa", "Mette questo elemento nella mappa aperta, vicino alla partenza dell'eroe." },
        { "Metti nella mappa aperta", "Mette questo elemento nella mappa aperta." },
        { "Torna al disegno del motore", "Toglie il disegno personalizzato e torna al disegno predefinito." },
        { "Disegna questa casella", "Apre l'editor di disegno per dare a questa casella un aspetto tuo." },
        { "Comportamento", "Come si muove e agisce il personaggio nel gioco." },
        { "Tipo", "Il tipo di comportamento o di elemento." },
        { "Funzione", "Che cosa fa l'oggetto nel gioco: da raccogliere, chiave, porta, molla, pericolo, arrivo..." },
        { "Velocita'", "Quanto veloce si muove (pixel al secondo)." },
        { "Salto", "Quanto in alto salta." },
        { "Energia", "Quanti colpi servono per sconfiggerlo." },
        { "Gravita'", "Se e' acceso, cade e cammina sul terreno; se e' spento, vola." },
        { "Proiettile", "L'oggetto che spara." },
        // --- Editor di disegno ---
        { "Matita", "Disegna un pixel alla volta (P). Sinistro: colore 1, destro: colore 2." },
        { "Gomma", "Cancella i pixel (E)." },
        { "Secchio", "Riempie una zona dello stesso colore (F)." },
        { "Linea", "Disegna una linea dritta trascinando (L)." },
        { "Rettang.", "Disegna un rettangolo trascinando (R). Con \"Pieno\" acceso e' pieno." },
        { "Ellisse", "Disegna un cerchio o un'ellisse trascinando (O)." },
        { "Contagocce", "Prende il colore da un pixel del disegno (I)." },
        { "Sposta", "Sposta tutto il fotogramma trascinando (M)." },
        { "Pieno", "Rettangoli ed ellissi pieni invece che solo il bordo." },
        { "Simmetria", "Quello che disegni a sinistra si ripete a destra, come allo specchio." },
        { "Cipolla", "Mostra in trasparenza il fotogramma precedente, per animare bene." },
        { "Griglia", "Mostra o nasconde la griglia." },
        { "Ridimensiona", "Cambia la misura del disegno con larghezza e altezza scelte qui a sinistra." },
        { "Specchia", "Gira il fotogramma da sinistra a destra." },
        { "Annulla", "Annulla l'ultima modifica (Ctrl+Z)." },
        { "Ripeti", "Rifa' la modifica annullata (Ctrl+Y)." },
        { "Vuoto", "Rende trasparente il colore scelto (gomma)." },
        { "Riproduci", "Fa partire l'anteprima dell'animazione." },
        { "Pausa", "Ferma l'anteprima dell'animazione." },
        { "Ciclo", "Se e' acceso l'animazione ricomincia da capo; se e' spento si ferma all'ultimo fotogramma." },
        { "fps", "Velocita' dell'animazione: fotogrammi al secondo." },
        { "+ Vuoto", "Aggiunge un fotogramma vuoto dopo quello scelto." },
        { "+ Copia", "Aggiunge una copia del fotogramma scelto, da modificare." },
        { "+", "Aggiunge o aumenta." },
        { "-", "Diminuisce." },
        { "<", "Indietro (la scelta precedente)." },
        { ">", "Avanti (la scelta successiva)." },
        { "Copia", "Aggiunge una copia del fotogramma scelto." },
        { "Opacita' %", "Quanto e' coprente il colore: 100 pieno, 0 trasparente." },
        // --- Scenari ---
        { "Usa nella mappa aperta", "La mappa aperta usera' questo elemento (scenario o musica) nel gioco." },
        { "+ Strato", "Aggiunge uno strato allo sfondo (montagne, nuvole, foresta, immagine...)." },
        { "+ Aggiungi strato", "Aggiunge uno strato allo sfondo della mappa." },
        { "Tema", "Il paesaggio di base: valle, deserto, ghiacci, vulcani..." },
        { "Ora del giorno", "0 e' notte, 0.5 mezzogiorno, 0.75 tramonto." },
        { "Il giorno passa", "Il cielo cambia lentamente dal giorno alla notte mentre giochi." },
        { "Ciclo giorno e notte", "Il cielo cambia lentamente dal giorno alla notte mentre giochi." },
        { "Paesaggio generato", "Disegna montagne, colline e alberi in automatico. Spento: solo il cielo e i tuoi strati." },
        { "Cielo personalizzato", "Scegli tu i due colori del cielo." },
        { "Cielo in alto", "Colore del cielo in alto." },
        { "Cielo in basso", "Colore del cielo all'orizzonte." },
        { "Meteo", "Sereno, pioggia o neve." },
        { "Scenario", "Lo scenario della libreria usato come sfondo di questa mappa. \"(sfondo della mappa)\" usa lo sfondo semplice." },
        { "Parallasse X", "Quanto lo strato segue la telecamera in orizzontale: 0 fermo (lontano), 1 come il terreno (vicino)." },
        { "Parallasse Y", "Quanto lo strato segue la telecamera in verticale." },
        { "Altezza (0-1)", "A che altezza dello schermo sta lo strato." },
        { "Colore", "Il colore." },
        { "Davanti all'eroe", "Lo strato si disegna davanti al gioco (primo piano)." },
        { "Davanti a tutto", "Lo strato si disegna davanti a tutto." },
        { "Ripetuta in orizzontale", "L'immagine si ripete all'infinito verso destra e sinistra." },
        { "Scala", "Ingrandisce o rimpicciolisce l'immagine." },
        { "Forma (seme)", "Cambia la forma dello strato generato (numeri diversi, forme diverse)." },
        { "Ampiezza", "Quanto sono alte le montagne o le colline dello strato." },
        { "Vento", "Velocita' delle nuvole." },
        // --- Mondi ---
        { "Pennello", "Disegna caselle o mette oggetti nella mappa (B)." },
        { "Riempi", "Riempie una zona di caselle uguali (F)." },
        { "Rettangolo", "Riempie un rettangolo di caselle trascinando (R)." },
        { "Seleziona", "Seleziona un oggetto per spostarlo o cambiarne le proprieta' (V)." },
        { "Sfondo", "Mostra lo sfondo vero dietro la mappa." },
        { "Inquadratura", "Mostra il rettangolo di quello che si vede nel gioco." },
        { "Larghezza", "Larghezza in caselle." },
        { "Altezza", "Altezza in caselle." },
        { "Vista", "Laterale: con la gravita' (platform). Dall'alto: si cammina in 4 direzioni (open world, avventura)." },
        { "Musica", "La musica di questa mappa." },
        { "Modo", "Come si muove la telecamera: segue l'eroe, a stanze, scorre da sola o ferma." },
        { "Zona morta X", "Quanto l'eroe si muove in orizzontale prima che la telecamera lo segua." },
        { "Zona morta Y", "Quanto l'eroe si muove in verticale prima che la telecamera lo segua." },
        { "Anticipo", "Quanto la telecamera guarda avanti nella direzione in cui corre l'eroe." },
        { "Zoom", "Ingrandimento della telecamera nel gioco." },
        { "X (pixel)", "Posizione orizzontale in pixel." },
        { "Y (pixel)", "Posizione verticale in pixel." },
        { "Mappa", "La mappa di arrivo della porta." },
        { "Scegli il punto d'arrivo sulla mappa", "Clicca poi sulla mappa d'arrivo il punto dove comparira' l'eroe." },
        { "Arrivo alla partenza della mappa", "L'eroe arriva alla partenza della mappa d'arrivo." },
        { "Duplica (Ctrl+C, V)", "Copia l'oggetto selezionato." },
        { "Elimina (Canc)", "Toglie l'oggetto selezionato dalla mappa." },
        { "Elimina la decorazione", "Toglie l'immagine dalla mappa." },
        { "Torna alle proprieta' della mappa", "Deseleziona e mostra le proprieta' della mappa." },
        { "Genere", "Il tipo di gioco: platform, open world, sparatutto, avventura, labirinto, puzzle." },
        { "Giocatori", "Quanti giocatori possono giocare." },
        { "Mappa iniziale", "La mappa da cui parte il gioco." },
        { "Direzione X", "Verso dove scorre la telecamera in orizzontale." },
        { "Direzione Y", "Verso dove scorre la telecamera in verticale." },
        { "Laterale (gravita')", "Vista di lato, con la gravita'." },
        { "Dall'alto", "Vista dall'alto, ci si muove in 4 direzioni." },
        // --- Audio ---
        { "Ascolta", "Fa sentire il brano o l'effetto." },
        { "Ferma", "Ferma l'ascolto." },
        { "Esporta WAV", "Salva il suono in un file .wav accanto allo studio." },
        { "WAV", "Salva il suono in un file .wav accanto allo studio." },
        { "Usa nella mappa", "Usa questo brano come musica della mappa aperta nell'editor dei mondi." },
        { "Cancella i blocchi", "Cancella le caselle e gli oggetti dentro il riquadro scelto. Si annulla con Ctrl+Z." },
        { "Copia i blocchi", "Copia le caselle e gli oggetti del riquadro: Ctrl+V li incolla dove c'e' il mouse." },
        { "Deseleziona", "Toglie la scelta." },
        { "A caso", "Crea un effetto sonoro a caso: premi piu' volte finche' ti piace." },
        { "Tempo", "Velocita' del brano in battiti al minuto." },
        { "Battute", "Lunghezza del brano in battute." },
        { "Volume", "Volume." },
        { "Onda", "Il timbro del suono: quadra, triangolo, dente di sega, rumore..." },
        // --- Logica ---
        { "+ Aggiungi", "Aggiunge una nuova regola o variabile." },
        { "+ Aggiungi un'azione", "Aggiunge un'altra cosa da fare quando la regola scatta." },
        { "Attiva", "Se e' spenta, la regola non scatta." },
        { "Chi", "Chi fa scattare la regola: l'eroe, il gioco o un personaggio/oggetto." },
        { "Evento", "Quando scatta la regola: tocca, raccoglie, colpisce, tasto premuto, ogni tot secondi..." },
        { "Con chi / che cosa", "Con chi o che cosa succede l'evento (lascia vuoto per \"qualsiasi\")." },
        { "Tasto", "Il tasto che fa scattare la regola." },
        { "Pulsante", "Il pulsante del joypad che fa scattare la regola." },
        { "Ogni (secondi)", "Ogni quanti secondi scatta la regola." },
        { "Energia sotto", "Scatta quando l'energia scende sotto questo valore." },
        { "Suono", "L'effetto sonoro da suonare." },
        // --- Librerie ---
        { "Aggiungi tutto", "Aggiunge al gioco tutti gli elementi di questa raccolta." },
        // --- Comandi ---
        { "Ripristina i predefiniti", "Rimette i tasti e i pulsanti di serie." },
        { "Zona morta delle levette", "Quanto bisogna spostare la levetta del joypad prima che l'eroe si muova." },
    };
    return d;
}

const char* Spiegazione(const std::string& etichetta) {
    // Spazi doppi ridotti a uno, poi senza la parte finale tra parentesi ("Pennello (B)" -> "Pennello").
    std::string t;
    for (char ch : etichetta) if (!(ch == ' ' && !t.empty() && t.back() == ' ')) t += ch;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    auto& d = Dizionario();
    auto it = d.find(t);
    if (it != d.end()) return it->second;
    size_t p = t.rfind(" (");
    if (p != std::string::npos && !t.empty() && t.back() == ')') {
        it = d.find(t.substr(0, p));
        if (it != d.end()) return it->second;
    }
    if (t.rfind("Aggiungi tutto", 0) == 0) return d.at("Aggiungi tutto");
    return nullptr;
}

} // namespace Ui
