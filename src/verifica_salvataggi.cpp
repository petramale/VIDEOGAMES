// Prova automatica dei salvataggi (StudioVideogiochi --verifica-salvataggi).
//
// Per ogni genere (e per un gioco con profili, indole e varieta' accese):
//   1. si gioca qualche secondo con un seme fisso, si salva su disco (scrittura atomica);
//   2. si rilegge, si ripristina in un gioco nuovo e la "firma" (eroe, entita', menti, oggetti) deve essere uguale;
//   3. si gioca ancora con gli stessi comandi su tutti e due: le partite devono restare uguali (determinismo);
//   4. file troncato, CRC sbagliato, versione futura, salvataggio vecchio (v0), copia di riserva .bak,
//      gioco cambiato dopo il salvataggio: nessun crash, errori chiari, riserva usata quando serve.
#include "comune.h"
#include "platform_tre.h"
#include "gioco.h"
#include "progetto_gioco.h"
#include "salvataggio.h"
#include <cstdio>
#include <fstream>
#include <string>

namespace {

int errori = 0;

void Prova(const std::string& nome, bool ok, const std::string& extra = "") {
    if (!ok) { TraceLog(LOG_ERROR, "Salvataggi: %s FALLITO %s", nome.c_str(), extra.c_str()); errori++; }
    else TraceLog(LOG_INFO, "Salvataggi: %s ok", nome.c_str());
}

std::string Leggi(const std::string& f) {
    std::ifstream in(f, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void Scrivi(const std::string& f, const std::string& t) {
    std::ofstream out(f, std::ios::binary);
    out << t;
}

// Prima riga diversa fra due firme (per capire cosa non torna).
std::string Differenza(const std::string& a, const std::string& b) {
    size_t i = 0, ia = 0, ib = 0;
    while (true) {
        size_t fa = a.find('\n', ia), fb = b.find('\n', ib);
        std::string ra = a.substr(ia, fa == std::string::npos ? std::string::npos : fa - ia);
        std::string rb = b.substr(ib, fb == std::string::npos ? std::string::npos : fb - ib);
        if (ra != rb) return TextFormat("riga %d: [%s] / [%s]", (int)i, ra.c_str(), rb.c_str());
        if (fa == std::string::npos || fb == std::string::npos) return "lunghezze diverse";
        ia = fa + 1; ib = fb + 1; i++;
    }
}

Comandi ComandoAlPasso(int i) {
    Comandi c;
    c.asse = (i / 90) % 4 == 3 ? -1.0f : 1.0f;
    c.salto = c.saltoPremuto = i % 37 == 0;
    c.asseY = (i / 70) % 3 == 1 ? -1.0f : (i / 70) % 3 == 2 ? 1.0f : 0.0f;
    return c;
}

void ProvaGioco(const std::string& nome, const ProgettoGioco& progetto, const std::string& cartella) {
    std::string errore, avviso;
    Gioco a;
    a.semePartita = 0x5EED1234ABCDull;
    if (!a.CaricaProgetto(progetto, errore)) { Prova(nome + ": carica", false, errore); return; }
    for (int i = 0; i < 600; i++) a.Aggiorna(1 / 60.0f, ComandoAlPasso(i));
    MetaSalvataggio meta;
    meta.titolo = nome;
    meta.impronta = ImprontaTesto(progetto.Json(true));
    meta.punteggio = a.punteggio;
    meta.tempo = a.tempo;
    std::string file = cartella + "/" + NomeFileDaTitolo(nome) + ".sav";
    std::string contenuto = ImbustaSalvataggio(ComponiSalvataggio(a.StatoJson(), meta));
    Prova(nome + ": scrittura atomica", ScriviAtomico(file, contenuto, errore), errore);
    // Un secondo salvataggio: il primo diventa la riserva .bak.
    Prova(nome + ": secondo salvataggio", ScriviAtomico(file, contenuto, errore), errore);
    Prova(nome + ": riserva .bak", !Leggi(file + ".bak").empty());

    std::string json;
    InfoSalvataggio info;
    bool letto = LeggiSalvataggio(file, json, info);
    Prova(nome + ": lettura", letto && info.valido && !info.daRiserva, info.errore);
    if (!letto) return;
    Prova(nome + ": intestazione", info.titolo == nome && info.versione == VERSIONE_SALVATAGGIO && info.impronta == meta.impronta);
    Gioco b;
    bool ripristinato = b.Ripristina(progetto, json, errore, avviso);
    Prova(nome + ": ripristino", ripristinato, errore);
    if (!ripristinato) return;
    Prova(nome + ": nessuna correzione", avviso.empty(), avviso);
    std::string fa = a.Firma(), fb = b.Firma();
    Prova(nome + ": stato uguale dopo il caricamento", fa == fb, fa == fb ? "" : Differenza(fa, fb));
    std::string primaDivergenza;
    for (int i = 600; i < 1200; i++) {
        Comandi c = ComandoAlPasso(i);
        a.Aggiorna(1 / 60.0f, c); b.Aggiorna(1 / 60.0f, c);
        if (primaDivergenza.empty() && a.Firma() != b.Firma()) primaDivergenza = TextFormat("fotogramma %d dopo: ", i - 600) + Differenza(a.Firma(), b.Firma());
    }
    fa = a.Firma(); fb = b.Firma();
    Prova(nome + ": stessa partita 10 s dopo il caricamento", fa == fb && primaDivergenza.empty(), primaDivergenza);
    TraceLog(LOG_INFO, "Salvataggi: %s: %d entita', salvataggio di %d KB", nome.c_str(), b.NumeroEntita(), (int)(contenuto.size() / 1024));

    // File troncato: si usa la riserva.
    Scrivi(file, contenuto.substr(0, contenuto.size() / 2));
    letto = LeggiSalvataggio(file, json, info);
    Prova(nome + ": troncato -> riserva", letto && info.daRiserva);
    // Troncato senza riserva: errore chiaro.
    std::remove((file + ".bak").c_str());
    letto = LeggiSalvataggio(file, json, info);
    Prova(nome + ": troncato senza riserva", !letto && !info.errore.empty(), info.errore);
    // Un byte cambiato: CRC sbagliato.
    std::string rotto = contenuto;
    size_t k = rotto.find("\"punteggio\"");
    if (k != std::string::npos) rotto[k + 1] = 'P';
    Scrivi(file, rotto);
    letto = LeggiSalvataggio(file, json, info);
    Prova(nome + ": CRC sbagliato", !letto && info.errore.find("controllo") != std::string::npos, info.errore);
    // Gioco cambiato dopo il salvataggio: niente crash, al massimo correzioni.
    ProgettoGioco cambiato = progetto;
    if (!cambiato.mondo.mappe.empty() && cambiato.mondo.mappe[0].oggetti.size() > 2) cambiato.mondo.mappe[0].oggetti.erase(cambiato.mondo.mappe[0].oggetti.begin() + 1);
    std::string j2;
    SbustaSalvataggio(contenuto, j2, errore);
    MigraSalvataggio(j2, errore);
    Gioco c;
    bool rip = c.Ripristina(cambiato, j2, errore, avviso);
    Prova(nome + ": gioco cambiato", rip, errore);
    for (int i = 0; i < 120 && rip; i++) c.Aggiorna(1 / 60.0f, ComandoAlPasso(i));
}

} // namespace

int VerificaSalvataggi() {
    errori = 0;
    std::string cartella = "prova_salvataggi";
    MakeDirectory(cartella.c_str());
    // Busta e migrazioni.
    {
        std::string j = "{\"formato\":\"elvantis-salvataggio\",\"versione\":1,\"partita\":{}}", fuori, errore;
        Prova("busta: CRC noto", Crc32("123456789") == 0xCBF43926u);
        Prova("busta: andata e ritorno", SbustaSalvataggio(ImbustaSalvataggio(j), fuori, errore) && fuori == j, errore);
        Prova("busta: testo qualsiasi rifiutato", !SbustaSalvataggio("ciao", fuori, errore));
        std::string futuro = "{\"formato\":\"elvantis-salvataggio\",\"versione\":99,\"partita\":{}}";
        Prova("versione futura rifiutata", !MigraSalvataggio(futuro, errore) && !errore.empty(), errore);
        std::string vecchio = "{\"formato\":\"elvantis-salvataggio\",\"versione\":0,\"mappa\":0,\"punteggio\":120,\"vite\":2}";
        int v = -1;
        bool ok = MigraSalvataggio(vecchio, errore, &v);
        Prova("migrazione v0 -> v1", ok && v == 0 && vecchio.find("\"versione\":1") != std::string::npos && vecchio.find("parziale") != std::string::npos, errore + " " + vecchio);
        std::string altro = "{\"formato\":\"altro\",\"versione\":1}";
        Prova("non e' un salvataggio", !MigraSalvataggio(altro, errore));
    }
    for (int g = 0; g < (int)Genere::Totale; g++) {
        ProgettoGioco p = NuovoProgetto((Genere)g, StileGrafico::PixelFine, std::string("Salva ") + NomeGenere((Genere)g));
        ProvaGioco(NomeGenere((Genere)g), p, cartella);
    }
    // I tre giochi di piattaforme: personaggi della libreria con la mente, oggetti, regole.
    ProvaGioco("Nora e Bullone", piatt::CreaNoraEBullone(), cartella);
    ProvaGioco("Nilo", piatt::CreaNilo(), cartella);
    // Un gioco con tutto acceso: profili, indole degli oggetti, variabili a caso, livello forte.
    {
        ProgettoGioco p = piatt::CreaLumina();
        p.profili.push_back(ModelliProfilo()[1]);
        p.profili.push_back(ModelliProfilo()[3]);
        int n = 0;
        for (auto& x : p.personaggi)
            if (x.comportamento.tipo != TipoComportamento::Giocatore && x.nome != p.eroe) x.profilo = p.profili[n++ % 2].nome;
        for (auto& o : p.oggetti) {
            if (o.funzione == FunzioneOggetto::Molla) { o.indole.attiva = true; o.indole.usura = 0.34f; o.indole.ricarica = 3; }
            if (o.funzione == FunzioneOggetto::Raccogli) { o.indole.attiva = true; o.indole.ricarica = 4; }
            if (o.funzione == FunzioneOggetto::Pericolo) { o.indole.attiva = true; o.indole.risveglio = 1; }
        }
        p.varieta.livello = 3;
        p.varieta.variabiliCasuali.push_back({ "tesoro", 1, 5, true });
        ProvaGioco("Lumina con profili e indole", p, cartella);
    }
    TraceLog(errori ? LOG_ERROR : LOG_INFO, "Salvataggi: %s (%d errori)", errori ? "CI SONO ERRORI" : "tutto bene", errori);
    return errori;
}
