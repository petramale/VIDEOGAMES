# Generatore di Videogiochi

Un software che **crea videogiochi a partire da una semplice descrizione**.
Scrivi in un file JSON come vuoi il gioco (titolo, colori, numero di nemici, tempo...)
e il generatore produce un gioco completo, giocabile subito nel browser.

## Obiettivo

Rendere la creazione di un videogioco accessibile anche a chi non programma:
descrivi il gioco, il software lo costruisce. Si parte da un genere semplice
("raccolta": raccogli oggetti ed evita i nemici) e si aggiungono nuovi generi nel tempo.

## Avvio rapido

Serve solo **Python 3.10 o superiore**, nessuna libreria da installare.

```bash
git clone https://github.com/petramale/videogames.git
cd videogames
python3 -m generatore esempi/gioco_base.json
```

Il gioco viene creato in `output/caccia_alle_stelle.html`: aprilo con un doppio clic
e gioca con le frecce o con WASD.

Per eseguire i test:

```bash
python3 -m unittest discover -s tests -v
```

## Come si descrive un gioco

```json
{
  "titolo": "Caccia alle Stelle",
  "genere": "raccolta",
  "colore_sfondo": "#1d2b53",
  "giocatore": { "colore": "#ffec27", "velocita": 4 },
  "oggetti":   { "colore": "#00e436", "quantita": 12 },
  "nemici":    { "colore": "#ff004d", "quantita": 3, "velocita": 2 },
  "tempo_limite": 60
}
```

Solo `titolo` è obbligatorio: tutti gli altri campi hanno un valore predefinito.

## Struttura del progetto

```
videogames/
├── generatore/              # il software
│   ├── cli.py               # comando da terminale
│   ├── specifica.py         # legge e controlla il file JSON del gioco
│   ├── costruttore.py       # trasforma la specifica in un gioco HTML
│   └── modelli/             # un modello di gioco per ogni genere
│       └── raccolta.html
├── esempi/                  # descrizioni di giochi pronte da provare
├── tests/                   # test automatici
└── .github/workflows/       # i test girano su GitHub a ogni modifica
```

## Perché queste tecnologie

- **Python per il generatore**: facile da leggere e da imparare, già presente su
  molti computer, e non richiede librerie esterne.
- **HTML5 + JavaScript (canvas) per i giochi generati**: il risultato è un solo file
  che funziona in qualsiasi browser, su computer e telefono, senza installare nulla
  e facile da condividere.

Se in futuro serviranno giochi più complessi (fisica, sprite, audio, 3D), i modelli
potranno usare un motore come Phaser o Godot senza cambiare il resto del generatore.

## Prossimi passi

1. Aggiungere nuovi generi (piattaforme, sparatutto spaziale, labirinto).
2. Supporto per immagini e suoni personalizzati.
3. Un'interfaccia grafica per creare il gioco senza scrivere JSON.
4. Generazione del gioco a partire da una descrizione a parole, con l'aiuto di un modello AI.
