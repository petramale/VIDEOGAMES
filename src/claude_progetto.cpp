#include "claude_progetto.h"
#include "audio.h"
#include "claude.h"
#include "claude_mondo.h"
#include "progetto_json.h"
#include "raccolte.h"
#include "sfondo.h"
#include <algorithm>

using nlohmann::json;

namespace {

const char* CARATTERI = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

int Distanza(Color a, Color b) { return std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b) + std::abs(a.a - b.a); }


std::string Testo(const json& a, const char* k) {
    return a.contains(k) && a[k].is_string() ? a[k].get<std::string>() : std::string();
}

bool Uguale(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

template <typename T>
T* Trova(std::vector<T>& v, const std::string& nome) {
    for (auto& x : v) if (Uguale(x.nome, nome)) return &x;
    return nullptr;
}

// Un elemento nuovo o modificato: si parte dal JSON di quello che c'e' e si applicano i campi mandati.
// Un disegno nuovo sostituisce del tutto quello vecchio.
json Unisci(json base, const json& nuovo) {
    if (nuovo.contains("disegno")) base.erase("disegno");
    base.merge_patch(nuovo);
    return base;
}

void SistemaStile(Disegno& d, const ProgettoGioco& p) {
    if (p.stile == StileGrafico::Disegnato && !d.liscio && d.densita <= 2.01f && d.w <= 96 && d.h <= 96) DisegnoInHD(d);
}

void AggiungiRaccolta(ProgettoGioco& p, const Raccolta& r) {
    for (auto& x : r.personaggi) if (!p.Personaggio(x.nome)) p.personaggi.push_back(x);
    for (auto& x : r.oggetti) if (!p.Oggetto(x.nome)) p.oggetti.push_back(x);
    for (auto& x : r.scenari) if (!p.TrovaScenario(x.nome)) p.scenari.push_back(x);
    for (auto& x : r.caselle) if (!p.Casella(x.carattere)) p.caselle.push_back(x);
    for (auto& x : r.brani) if (!p.TrovaBrano(x.nome)) p.brani.push_back(x);
    for (auto& x : r.effetti) if (!p.TrovaEffetto(x.nome)) p.effetti.push_back(x);
}

} // namespace

// --- Disegni in righe e in HD ---------------------------------------------------------------

std::string DisegnoInRighe(const Disegno& d, int maxLato) {
    int passo = std::max(1, (std::max(d.w, d.h) + maxLato - 1) / maxLato);
    std::vector<Color> tav;
    auto Carattere = [&](Color c) -> char {
        if (c.a < 100) return '.';
        c.a = 255;
        int migliore = -1, dist = 1 << 30;
        for (size_t i = 0; i < tav.size(); i++) { int dd = Distanza(tav[i], c); if (dd < dist) { dist = dd; migliore = (int)i; } }
        if ((dist > 20 || migliore < 0) && tav.size() < 62) { tav.push_back(c); return CARATTERI[tav.size() - 1]; }
        return CARATTERI[migliore];
    };
    json anims = json::array();
    for (auto& a : d.animazioni) {
        json fots = json::array();
        for (size_t k = 0; k < a.fotogrammi.size() && k < 8; k++) {
            const Fotogramma& f = a.fotogrammi[k];
            json righe = json::array();
            for (int y = 0; y < f.h; y += passo) {
                std::string r;
                for (int x = 0; x < f.w; x += passo) r += Carattere(f.Prendi(x, y));
                righe.push_back(r);
            }
            fots.push_back(righe);
        }
        anims.push_back({ { "nome", a.nome }, { "fps", a.fps }, { "fotogrammi", fots } });
    }
    json pal = json::object();
    for (size_t i = 0; i < tav.size(); i++) pal[std::string(1, CARATTERI[i])] = ColoreInTesto(tav[i]);
    json j = { { "palette", pal }, { "animazioni", anims } };
    if (passo > 1) j["nota"] = "disegno ridotto di " + std::to_string(passo) + " volte per mostrarlo (nell'officina e' piu' grande)";
    return j.dump();
}

void DisegnoInHD(Disegno& d) {
    auto Uguali = [](Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; };
    for (auto& a : d.animazioni)
        for (auto& f : a.fotogrammi) {
            Fotogramma g;
            g.Nuovo(f.w * 2, f.h * 2);
            for (int y = 0; y < f.h; y++)
                for (int x = 0; x < f.w; x++) {
                    Color P = f.Prendi(x, y), A = f.Prendi(x, y - 1), B = f.Prendi(x + 1, y), C = f.Prendi(x - 1, y), D = f.Prendi(x, y + 1);
                    Color e1 = P, e2 = P, e3 = P, e4 = P;
                    if (Uguali(C, A) && !Uguali(C, D) && !Uguali(A, B)) e1 = A;
                    if (Uguali(A, B) && !Uguali(A, C) && !Uguali(B, D)) e2 = B;
                    if (Uguali(D, C) && !Uguali(D, B) && !Uguali(C, A)) e3 = C;
                    if (Uguali(B, D) && !Uguali(B, A) && !Uguali(D, C)) e4 = D;
                    g.Metti(x * 2, y * 2, e1); g.Metti(x * 2 + 1, y * 2, e2); g.Metti(x * 2, y * 2 + 1, e3); g.Metti(x * 2 + 1, y * 2 + 1, e4);
                }
            f = g;
        }
    d.w *= 2; d.h *= 2;
    d.densita *= 2;
    d.liscio = true;
    d.Invalida();
}

// --- Istruzioni per Claude ------------------------------------------------------------------------

std::string PromptProgetto() {
    std::string stili;
    for (size_t i = 0; i < StiliBrano().size(); i++) stili += (i ? ", " : "") + StiliBrano()[i];
    std::string raccolte;
    for (int i = 0; i < NumeroRaccolte(); i++) raccolte += (i ? ", " : "") + std::string(NomeRaccolta(i));
    std::string eventi, azioni;
    for (size_t i = 0; i < TipiEvento().size(); i++) eventi += (i ? ", " : "") + TipiEvento()[i] + " (" + NomiEvento()[i] + ")";
    for (size_t i = 0; i < TipiAzione().size(); i++) azioni += (i ? ", " : "") + TipiAzione()[i] + " (" + NomiAzione()[i] + ")";
    return PromptMondo() +
        "\n\nIL GIOCO COMPLETO E LA SUA LIBRERIA\n"
        "Oltre al mondo ricevi la LIBRERIA del gioco (personaggi, oggetti, scenari, caselle, brani, effetti), le REGOLE della logica, le variabili "
        "e la SEZIONE dello studio in cui si trova l'utente (mondi, personaggi, oggetti, scenari, audio, logica, librerie, comandi) con l'elemento aperto: "
        "rispondi pensando a quella sezione. Nelle mappe gli oggetti possono avere come tipo il NOME di un personaggio o di un oggetto della libreria "
        "(es. {\"tipo\":\"Fiammaros\",\"cx\":30,\"cy\":10}): e' cosi' che si mettono nel mondo i personaggi creati. L'eroe del gioco e' il personaggio indicato in \"eroe\".\n\n"
        "AZIONI SULLA LIBRERIA (crea se il nome non c'e', altrimenti modifica solo i campi mandati)\n"
        "- {\"tipo\":\"personaggio\",\"personaggio\":{\"nome\":\"Drago\",\"comportamento\":{\"tipo\":\"giocatore|fermo|pattuglia|insegue|vola|spara|boss|parla\","
        "\"velocita\":40,\"salto\":0,\"energia\":3,\"danno\":1,\"vista\":120,\"attaccoOgni\":2,\"proiettile\":\"nome di un oggetto con funzione proiettile\","
        "\"gravita\":true,\"calpestabile\":true,\"dialogo\":\"cosa dice (parla)\",\"punti\":100},\"suoni\":{\"salto\":\"effetto\",\"colpito\":\"effetto\",\"attacco\":\"effetto\",\"sconfitto\":\"effetto\"},"
        "\"disegno\":{...}}}\n"
        "- {\"tipo\":\"oggetto\",\"oggetto\":{\"nome\":\"Chiave d'oro\",\"funzione\":\"decorazione|raccogli|cura|chiave|porta|molla|cassa|interruttore|piattaforma|pericolo|arrivo|proiettile\","
        "\"valore\":10,\"richiede\":\"per le porte: nome della chiave o dell'interruttore\",\"solido\":false,\"gravita\":false,\"velocita\":40,\"distanza\":64,"
        "\"suono\":\"effetto\",\"variabile\":\"variabile da aumentare\",\"disegno\":{...}}}\n"
        "- {\"tipo\":\"casella\",\"casella\":{\"carattere\":\"#\",\"nome\":\"Erba\",\"disegno\":{...}}}: aspetto di un tipo di casella ('#','=','-','^','.'); "
        "il disegno e' 32x32 con l'animazione \"superficie\" (casella con il vuoto sopra) e \"interno\".\n"
        "- {\"tipo\":\"scenario\",\"scenario\":{\"nome\":\"Tramonto\",\"tema\":0,\"ora\":0.74,\"cicloGiorno\":false,\"sfondoGenerato\":true,\"pioggia\":0,"
        "\"cielo\":{\"alto\":\"#rrggbb\",\"basso\":\"#rrggbb\"},\"strati\":[...]}} (pioggia: 0 niente, 1 pioggia, 2 neve).\n"
        "- {\"tipo\":\"brano\",\"brano\":{\"nome\":\"Tema\",\"bpm\":120,\"battute\":8,\"tracce\":[{\"nome\":\"Melodia\",\"strumento\":0,\"volume\":0.5,\"batteria\":false,"
        "\"note\":[[inizio,durata,altezza],...]}]}}: note in sedicesimi (16 per battuta), altezza MIDI (60 = do centrale); strumenti 0 onda quadra, "
        "1 triangolo, 2 dente di sega, 3 seno, 4 batteria (altezza 36 cassa, 38 rullante, 42 charleston). Per un brano composto in automatico: "
        "{\"tipo\":\"brano\",\"brano\":{\"nome\":\"X\",\"stile\":\"epico\",\"seme\":7,\"battute\":8}} con stile fra " + stili + ".\n"
        "- {\"tipo\":\"effetto\",\"effetto\":{\"nome\":\"Laser\",\"onda\":0,\"f0\":880,\"f1\":220,\"durata\":0.25,\"volume\":0.5,\"duty\":0.5,\"vibrato\":0,\"vibratoHz\":0,\"rumore\":0}} "
        "(frequenze in Hz da f0 a f1, rumore 0-1 per esplosioni).\n"
        "- {\"tipo\":\"regola\",\"regola\":{\"chi\":\"eroe|gioco|nome di personaggio o oggetto\",\"evento\":\"...\",\"conChi\":\"...\",\"variabile\":\"\",\"confronto\":\">=\",\"valore\":0,"
        "\"azioni\":[{\"tipo\":\"...\",\"testo\":\"...\",\"numero\":0}]}} aggiunge una regola (con \"indice\": N la sostituisce). Eventi: " + eventi + ". "
        "conChi: per tocca/raccoglie/colpisce/vicino e' il nome dell'altro; per energia la soglia; per tasto il nome del tasto (A-Z, SPAZIO, INVIO, SU, GIU, SINISTRA, DESTRA); "
        "per joypad il numero del pulsante; per ogni i secondi; per variabile il nome. Azioni: " + azioni + ". "
        "testo e' il nome dell'effetto, del brano, del messaggio, della variabile, della mappa, del personaggio o oggetto da creare (numero = quanti), dell'animazione, "
        "il colore #rrggbb; numero e' la quantita'. La condizione facoltativa (variabile confronto valore) deve essere vera perche' la regola scatti.\n"
        "- {\"tipo\":\"togliRegola\",\"indice\":N}\n"
        "- {\"tipo\":\"variabile\",\"nome\":\"chiavi\",\"valore\":0}: valore iniziale di una variabile (le variabili si vedono nel gioco).\n"
        "- {\"tipo\":\"eroe\",\"nome\":\"personaggio\"}: chi fa da eroe (\"\" = l'eroe disegnato dal motore).\n"
        "- {\"tipo\":\"suonoEvento\",\"evento\":\"salto|moneta|gemma|colpo|molla|forziere|esplosione|vittoria|passo\",\"effetto\":\"nome effetto (\\\"\\\" = quello di serie)\"}\n"
        "- {\"tipo\":\"usa\",\"mappa\":N,\"scenario\":\"nome\",\"brano\":\"nome\"}: scenario e musica della libreria per la mappa N.\n"
        "- {\"tipo\":\"elimina\",\"categoria\":\"personaggi|oggetti|scenari|caselle|brani|effetti\",\"nome\":\"...\"}\n"
        "- {\"tipo\":\"raccolta\",\"nome\":\"...\"}: aggiunge alla libreria una raccolta pronta fra: " + raccolte + ".\n\n"
        "I DISEGNI (pixel art)\n"
        "\"disegno\":{\"palette\":{\"a\":\"#2a1a0a\",\"b\":\"#d8344a\",...},\"animazioni\":[{\"nome\":\"fermo\",\"fps\":6,\"fotogrammi\":[[\"riga\",\"riga\",...],...]}]}. "
        "Ogni fotogramma e' un elenco di righe tutte lunghe uguali; '.' e' trasparente, ogni altro carattere e' un colore della palette. "
        "Misure: 32x32 per personaggi e oggetti grandi come una casella (il disegno appare largo 16 pixel del mondo, con il doppio dei dettagli), "
        "fino a 64x48 per un boss o un drago, 16x16 per oggetti piccoli. Piedi in basso al centro. Usa un contorno scuro, 3 toni per colore (luce, base, ombra) "
        "e guarda a DESTRA (il motore specchia il disegno). Animazioni che il motore usa: personaggi \"fermo\", \"corsa\", \"salto\", \"caduta\", \"colpito\", "
        "\"attacco\", \"vittoria\"; oggetti \"fermo\" (la prima animazione), porte \"chiusa\" e \"aperta\", interruttori \"spenta\" e \"accesa\", molle \"scatta\". Al massimo 4 animazioni con 2-4 fotogrammi ciascuna: "
        "la risposta deve restare sotto i 40.000 caratteri. Se lo stile del gioco e' \"disegnato\" lo studio ingrandisce e ammorbidisce da solo i tuoi disegni.\n"
        "Se l'utente chiede di modificare il disegno dell'elemento aperto, ricevi il suo disegno attuale nello stato: rimandalo completo con le modifiche.";
}

std::string StatoProgettoPerClaude(const ProgettoGioco& p, int mappaAperta, const std::string& sezione, const std::string& selezionato) {
    json j;
    j["sezione"] = sezione;
    if (!selezionato.empty()) j["elementoAperto"] = selezionato;
    j["stile"] = p.stile == StileGrafico::Disegnato ? "disegnato" : "pixel";
    j["eroe"] = p.eroe;
    json pers = json::array();
    for (auto& x : p.personaggi) {
        json e = PersonaggioJson(x);
        json anim = json::array();
        for (auto& a : x.disegno.animazioni) anim.push_back(a.nome);
        e["disegno"] = { { "larghezza", x.disegno.w / x.disegno.densita }, { "altezza", x.disegno.h / x.disegno.densita }, { "animazioni", anim } };
        if (Uguale(x.nome, selezionato)) e["disegnoCompleto"] = json::parse(DisegnoInRighe(x.disegno, 48));
        pers.push_back(e);
    }
    j["personaggi"] = pers;
    json ogg = json::array();
    for (auto& x : p.oggetti) {
        json e = OggettoJson(x);
        e["disegno"] = { { "larghezza", x.disegno.w / x.disegno.densita }, { "altezza", x.disegno.h / x.disegno.densita } };
        if (Uguale(x.nome, selezionato)) e["disegnoCompleto"] = json::parse(DisegnoInRighe(x.disegno, 48));
        ogg.push_back(e);
    }
    j["oggetti"] = ogg;
    json sc = json::array();
    for (auto& x : p.scenari) sc.push_back(ScenarioJson(x));
    j["scenari"] = sc;
    json cas = json::array();
    for (auto& x : p.caselle) cas.push_back({ { "carattere", std::string(1, x.carattere) }, { "nome", x.nome } });
    j["caselle"] = cas;
    json br = json::array();
    for (auto& x : p.brani) {
        if (Uguale(x.nome, selezionato) && x.file.empty()) { br.push_back(BranoJson(x)); continue; }
        br.push_back({ { "nome", x.nome }, { "bpm", x.bpm }, { "battute", x.battute }, { "tracce", (int)x.tracce.size() }, { "daFile", !x.file.empty() } });
    }
    j["brani"] = br;
    json ef = json::array();
    for (auto& x : p.effetti) { json e = EffettoJson(x); e.erase("dati"); ef.push_back(e); }
    j["effetti"] = ef;
    json re = json::array();
    for (auto& x : p.regole) re.push_back(RegolaJson(x));
    j["regole"] = re;
    j["variabili"] = p.variabili;
    j["suoniEventi"] = p.suoniEventi;
    return "Libreria, regole e sezione aperta:\n" + j.dump() + "\n\n" + StatoMondoPerClaude(p.mondo, mappaAperta);
}

// --- Applicare la risposta ---------------------------------------------------------------------------

EsitoProgetto ApplicaRispostaProgetto(ProgettoGioco& p, int mappaAperta, const std::string& testo) {
    EsitoProgetto e;
    std::string sorgente = Claude::EstraiJson(testo);
    json j = json::parse(sorgente, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        // Risposta solo a parole: va bene lo stesso, la mostriamo.
        e.risposta = testo;
        return e;
    }
    e.risposta = Testo(j, "risposta");
    if (!j.contains("azioni") || !j["azioni"].is_array()) return e;
    ProgettoGioco copia = p;
    json azioniMondo = json::array();
    for (auto& a : j["azioni"]) {
        if (!a.is_object()) continue;
        std::string tipo = Testo(a, "tipo"), errore;
        try {
            if (tipo == "personaggio" && a.contains("personaggio") && a["personaggio"].is_object()) {
                std::string nome = Testo(a["personaggio"], "nome");
                PersonaggioDef* c = Trova(copia.personaggi, nome);
                PersonaggioDef n;
                json base = c ? PersonaggioJson(*c) : json::object();
                if (!PersonaggioDaJson(Unisci(base, a["personaggio"]), n, errore)) { e.scartate.push_back("personaggio " + nome + ": " + errore); continue; }
                if (a["personaggio"].contains("disegno")) SistemaStile(n.disegno, copia);
                n.autore = c ? c->autore : "Claude";
                if (c) { *c = n; e.fatte.push_back("personaggio " + n.nome + " modificato"); }
                else { copia.personaggi.push_back(n); e.fatte.push_back("nuovo personaggio " + n.nome); }
                e.sezione = "personaggi"; e.nome = n.nome;
            } else if (tipo == "oggetto" && a.contains("oggetto") && a["oggetto"].is_object()) {
                std::string nome = Testo(a["oggetto"], "nome");
                OggettoDef* c = Trova(copia.oggetti, nome);
                OggettoDef n;
                json base = c ? OggettoJson(*c) : json::object();
                if (!OggettoDaJson(Unisci(base, a["oggetto"]), n, errore)) { e.scartate.push_back("oggetto " + nome + ": " + errore); continue; }
                if (a["oggetto"].contains("disegno")) SistemaStile(n.disegno, copia);
                n.autore = c ? c->autore : "Claude";
                if (c) { *c = n; e.fatte.push_back("oggetto " + n.nome + " modificato"); }
                else { copia.oggetti.push_back(n); e.fatte.push_back("nuovo oggetto " + n.nome); }
                e.sezione = "oggetti"; e.nome = n.nome;
            } else if (tipo == "casella" && a.contains("casella") && a["casella"].is_object()) {
                const json& jc = a["casella"];
                std::string car = Testo(jc, "carattere");
                if (car.size() != 1 || std::string("#=-^.").find(car[0]) == std::string::npos) { e.scartate.push_back("casella: carattere non valido"); continue; }
                CasellaDef n;
                n.carattere = car[0];
                n.nome = Testo(jc, "nome");
                if (!jc.contains("disegno") || !DisegnoDaJson(jc["disegno"], n.disegno, errore)) { e.scartate.push_back("casella " + car + ": " + errore); continue; }
                SistemaStile(n.disegno, copia);
                bool c = false;
                for (auto& x : copia.caselle) if (x.carattere == n.carattere) { x = n; c = true; }
                if (!c) copia.caselle.push_back(n);
                e.fatte.push_back("casella '" + car + "' disegnata");
                e.sezione = "scenari"; e.nome = n.nome;
            } else if (tipo == "scenario" && a.contains("scenario") && a["scenario"].is_object()) {
                std::string nome = Testo(a["scenario"], "nome");
                Scenario* c = Trova(copia.scenari, nome);
                Scenario n;
                json base = c ? ScenarioJson(*c) : json::object();
                json patch = a["scenario"];
                base.merge_patch(patch);
                if (!ScenarioDaJson(base, n, errore)) { e.scartate.push_back("scenario " + nome + ": " + errore); continue; }
                if (c) { n.autore = c->autore; *c = n; e.fatte.push_back("scenario " + n.nome + " modificato"); }
                else { n.autore = "Claude"; copia.scenari.push_back(n); e.fatte.push_back("nuovo scenario " + n.nome); }
                e.sezione = "scenari"; e.nome = n.nome;
            } else if (tipo == "brano" && a.contains("brano") && a["brano"].is_object()) {
                const json& jb = a["brano"];
                std::string nome = Testo(jb, "nome");
                Brano n;
                if (jb.contains("stile") && !jb.contains("tracce")) {
                    int stile = 0;
                    if (jb["stile"].is_number()) stile = jb["stile"].get<int>();
                    else for (size_t i = 0; i < StiliBrano().size(); i++) if (Uguale(StiliBrano()[i], Testo(jb, "stile"))) stile = (int)i;
                    int seme = jb.contains("seme") && jb["seme"].is_number() ? jb["seme"].get<int>() : GetRandomValue(1, 9999);
                    int battute = jb.contains("battute") && jb["battute"].is_number() ? jb["battute"].get<int>() : 8;
                    n = ComponiBrano(nome.empty() ? "Brano" : nome, seme, stile, (int)Limita((float)battute, 2, 64));
                } else if (!BranoDaJson(jb, n, errore)) { e.scartate.push_back("brano " + nome + ": " + errore); continue; }
                Brano* c = Trova(copia.brani, n.nome);
                n.autore = "Claude";
                if (c) { *c = n; e.fatte.push_back("brano " + n.nome + " riscritto"); }
                else { copia.brani.push_back(n); e.fatte.push_back("nuovo brano " + n.nome); }
                e.sezione = "audio"; e.nome = n.nome;
            } else if (tipo == "effetto" && a.contains("effetto") && a["effetto"].is_object()) {
                std::string nome = Testo(a["effetto"], "nome");
                EffettoSuono* c = Trova(copia.effetti, nome);
                EffettoSuono n;
                json base = c ? EffettoJson(*c) : json::object();
                base.merge_patch(a["effetto"]);
                if (!EffettoDaJson(base, n, errore)) { e.scartate.push_back("effetto " + nome + ": " + errore); continue; }
                if (c) { *c = n; e.fatte.push_back("effetto " + n.nome + " modificato"); }
                else { n.autore = "Claude"; copia.effetti.push_back(n); e.fatte.push_back("nuovo effetto " + n.nome); }
                e.sezione = "audio"; e.nome = n.nome;
            } else if (tipo == "regola" && a.contains("regola")) {
                Regola r;
                if (!RegolaDaJson(a["regola"], r, errore)) { e.scartate.push_back("regola: " + errore); continue; }
                int i = a.contains("indice") && a["indice"].is_number() ? a["indice"].get<int>() : -1;
                if (i >= 0 && i < (int)copia.regole.size()) { copia.regole[i] = r; e.fatte.push_back("regola " + std::to_string(i + 1) + " cambiata"); }
                else { copia.regole.push_back(r); e.fatte.push_back("nuova regola: quando " + r.chi + " " + r.evento + (r.conChi.empty() ? "" : " " + r.conChi)); }
                e.sezione = "logica";
            } else if (tipo == "togliRegola") {
                int i = a.contains("indice") && a["indice"].is_number() ? a["indice"].get<int>() : -1;
                if (i < 0 || i >= (int)copia.regole.size()) { e.scartate.push_back("togliRegola: indice non valido"); continue; }
                copia.regole.erase(copia.regole.begin() + i);
                e.fatte.push_back("regola " + std::to_string(i + 1) + " tolta");
            } else if (tipo == "variabile") {
                std::string nome = Testo(a, "nome");
                if (nome.empty()) { e.scartate.push_back("variabile senza nome"); continue; }
                copia.variabili[nome] = a.contains("valore") && a["valore"].is_number() ? a["valore"].get<float>() : 0;
                e.fatte.push_back("variabile " + nome);
            } else if (tipo == "eroe") {
                std::string nome = Testo(a, "nome");
                if (!nome.empty() && !copia.Personaggio(nome)) { e.scartate.push_back("eroe: non c'e' il personaggio " + nome); continue; }
                copia.eroe = nome.empty() ? "" : copia.Personaggio(nome)->nome;
                e.fatte.push_back(nome.empty() ? "eroe classico" : "l'eroe ora e' " + copia.eroe);
            } else if (tipo == "suonoEvento") {
                std::string ev = Testo(a, "evento"), ef = Testo(a, "effetto");
                if (!ef.empty() && !copia.TrovaEffetto(ef)) { e.scartate.push_back("suonoEvento: non c'e' l'effetto " + ef); continue; }
                if (ef.empty()) copia.suoniEventi.erase(ev); else copia.suoniEventi[ev] = ef;
                e.fatte.push_back("suono per " + ev);
            } else if (tipo == "usa") {
                int i = a.contains("mappa") && a["mappa"].is_number() ? a["mappa"].get<int>() : mappaAperta;
                if (i < 0 || i >= (int)copia.mondo.mappe.size()) { e.scartate.push_back("usa: mappa non valida"); continue; }
                Mappa& m = copia.mondo.mappe[i];
                if (a.contains("scenario")) {
                    std::string sc = Testo(a, "scenario");
                    if (!sc.empty() && !copia.TrovaScenario(sc)) { e.scartate.push_back("usa: non c'e' lo scenario " + sc); continue; }
                    m.scenarioLibreria = sc;
                }
                if (a.contains("brano")) {
                    std::string b = Testo(a, "brano");
                    if (!b.empty() && !copia.TrovaBrano(b)) { e.scartate.push_back("usa: non c'e' il brano " + b); continue; }
                    m.brano = b;
                }
                e.fatte.push_back("scenario e musica della mappa " + m.nome);
            } else if (tipo == "elimina") {
                std::string cat = Testo(a, "categoria"), nome = Testo(a, "nome");
                size_t prima = 0, dopo = 0;
                auto Togli = [&](auto& v) { prima = v.size(); v.erase(std::remove_if(v.begin(), v.end(), [&](const auto& x) { return Uguale(x.nome, nome); }), v.end()); dopo = v.size(); };
                if (cat == "personaggi") Togli(copia.personaggi);
                else if (cat == "oggetti") Togli(copia.oggetti);
                else if (cat == "scenari") Togli(copia.scenari);
                else if (cat == "caselle") Togli(copia.caselle);
                else if (cat == "brani") Togli(copia.brani);
                else if (cat == "effetti") Togli(copia.effetti);
                if (prima == dopo) { e.scartate.push_back("elimina: non trovo " + nome); continue; }
                e.fatte.push_back(nome + " eliminato");
            } else if (tipo == "raccolta") {
                std::string nome = Testo(a, "nome");
                int i = -1;
                for (int k = 0; k < NumeroRaccolte(); k++) if (Uguale(NomeRaccolta(k), nome)) i = k;
                if (i < 0) { e.scartate.push_back("raccolta sconosciuta: " + nome); continue; }
                AggiungiRaccolta(copia, CreaRaccolta(i, copia.stile));
                e.fatte.push_back("raccolta " + std::string(NomeRaccolta(i)) + " aggiunta");
                e.sezione = "librerie";
            } else {
                azioniMondo.push_back(a);   // azioni sul mondo: le applica claude_mondo
            }
        } catch (const std::exception& ex) {
            e.scartate.push_back(tipo + ": " + ex.what());
        }
    }
    // Il mondo si controlla conoscendo i nomi della libreria (anche quelli appena creati).
    std::string problema = copia.Controlla();
    if (!azioniMondo.empty()) {
        json r = { { "risposta", "" }, { "azioni", azioniMondo } };
        EsitoClaude em = ApplicaRispostaClaude(copia.mondo, mappaAperta, r.dump());
        for (auto& f : em.fatte) e.fatte.push_back(f);
        for (auto& s : em.scartate) e.scartate.push_back(s);
        if (em.apriMappa >= 0) e.apriMappa = em.apriMappa;
        if (em.modificato && e.sezione.empty()) e.sezione = "mondi";
    }
    problema = copia.Controlla();
    if (!problema.empty()) {
        // Qualcosa non torna (es. una mappa usa un brano eliminato): si scarta tutto.
        e.scartate.push_back("risultato non valido: " + problema);
        e.fatte.clear();
        return e;
    }
    if (!e.fatte.empty()) {
        p = copia;
        e.modificato = true;
    }
    return e;
}
