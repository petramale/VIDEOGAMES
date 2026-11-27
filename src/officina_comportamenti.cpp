// Sezione Comportamenti dell'officina (Alt+9): la mente di personaggi e oggetti e la varieta' della partita.
//
//   a sinistra  i profili di comportamento del gioco (nuovo, duplica, elimina, da un modello pronto)
//   al centro   le schede: Carattere, Azioni, Reazioni, Chi lo usa, Oggetti, Partita unica
//   a destra    il simulatore delle decisioni: si cambia la situazione e si vede quale azione sceglierebbe
//               la creatura e con quale punteggio (gli stessi calcoli del gioco, mente.cpp)
// Tutto quello che si crea qui si salva nel file .gioco e vale negli .exe creati.
#include "officina.h"
#include "audio.h"
#include "ui.h"
#include <algorithm>

namespace {

struct Colonna {
    Rectangle area;
    float y, s, scorri;
    Colonna(Rectangle a, float sc, float scorrimento) : area(a), y(a.y - scorrimento), s(sc), scorri(scorrimento) {}
    Rectangle Riga(float h) { Rectangle r = { area.x, y, area.width, h * s }; y += h * s + 6 * s; return r; }
    bool Vede(Rectangle r) const { return r.y >= area.y - 1 && r.y + r.height <= area.y + area.height + 1; }
    void Spazio(float h) { y += h * s; }
    void Sezione(const std::string& t) {
        y += 6 * s;
        Rectangle r = { area.x, y, area.width, 20 * s };
        if (Vede(r)) { Ui::Testo(t, area.x, y, Ui::Accento(), Ui::TP()); DrawLineEx({ area.x, y + 19 * s }, { area.x + area.width, y + 19 * s }, 1, Ui::Bordo()); }
        y += 26 * s;
    }
    // Testo a capo che si disegna solo se visibile.
    void Nota(const std::string& t, int dim) {
        float h = Ui::TestoACapo(t, area.x, y, area.width, Ui::TestoSpento(), dim, false);
        Rectangle r = { area.x, y, area.width, h };
        if (Vede(r)) Ui::TestoACapo(t, area.x, y, area.width, Ui::TestoSpento(), dim);
        y += h + 6 * s;
    }
    float Altezza() const { return y + scorri - area.y; }
};

const char* SCHEDE_COMP[] = { "Carattere", "Azioni", "Reazioni", "Chi lo usa", "Oggetti", "Partita unica" };
constexpr int NUM_SCHEDE_COMP = 6;

Color ColoreAzione(AzioneMente a) {
    switch (a) {
        case AzioneMente::Insegui: case AzioneMente::Attacca: case AzioneMente::Aggira: return Hex(0xff6b6b);
        case AzioneMente::Fuggi: case AzioneMente::Schiva: case AzioneMente::Curati: return Hex(0xffd23f);
        case AzioneMente::Amico: return Hex(0x7ee07e);
        case AzioneMente::ChiamaAiuto: case AzioneMente::Rivale: return Hex(0xff9f43);
        default: return Hex(0x5ec8ff);
    }
}

std::string NomeLiberoProfilo(const ProgettoGioco& p, const std::string& base) {
    if (!p.Profilo(base)) return base;
    for (int i = 2; i < 999; i++) {
        std::string n = base + " " + std::to_string(i);
        if (!p.Profilo(n)) return n;
    }
    return base + " nuovo";
}

} // namespace

// --- Azioni dai menu --------------------------------------------------------------------------------

void Officina::AzioneComportamenti(const std::string& c) {
    auto& v = progetto.profili;
    if (c == "m:nuovo" || c == "m:modello") {
        Ricorda();
        ProfiloComportamento p = c == "m:modello" ? ModelliProfilo()[std::min(modelloScelto, (int)ModelliProfilo().size() - 1)] : ModelliProfilo()[0];
        p.nome = NomeLiberoProfilo(progetto, c == "m:modello" ? p.nome : "Nuovo profilo");
        p.autore = "";
        v.push_back(p);
        selProfilo = (int)v.size() - 1;
        Avvisa("Profilo " + p.nome + ": cambia il carattere e assegnalo ai personaggi nella scheda \"Chi lo usa\"");
    } else if (c == "m:duplica" && selProfilo < (int)v.size()) {
        Ricorda();
        ProfiloComportamento p = v[selProfilo];
        p.nome = NomeLiberoProfilo(progetto, p.nome);
        v.insert(v.begin() + selProfilo + 1, p);
        selProfilo++;
    } else if (c == "m:elimina" && selProfilo < (int)v.size()) {
        Ricorda();
        std::string nome = v[selProfilo].nome;
        int usati = 0;
        for (auto& p : progetto.personaggi) if (p.profilo == nome) { p.profilo.clear(); usati++; }
        v.erase(v.begin() + selProfilo);
        selProfilo = std::max(0, selProfilo - 1);
        Avvisa(usati ? TextFormat("Profilo eliminato: %d personaggi tornano al comportamento istintivo", usati) : "Profilo eliminato");
    } else if (c == "m:prova") {
        AvviaProva();
        if (provando) { prova.mostraPensieri = true; Avvisa("Prova con i pensieri delle creature (F9 per nasconderli)"); }
    } else if (c == "m:tutti" && selProfilo < (int)v.size()) {
        Ricorda();
        int n = 0;
        for (auto& p : progetto.personaggi)
            if (p.comportamento.tipo != TipoComportamento::Giocatore && p.comportamento.tipo != TipoComportamento::Parla && p.nome != progetto.eroe) { p.profilo = v[selProfilo].nome; n++; }
        Avvisa(TextFormat("Profilo %s dato a %d nemici", v[selProfilo].nome.c_str(), n));
    }
}

// --- La sezione --------------------------------------------------------------------------------------

void Officina::Comportamenti(Rectangle r, float dt) {
    (void)dt;
    float s = Ui::S();
    auto& profili = progetto.profili;
    Rectangle sx = { r.x, r.y, 300 * s, r.height };
    std::vector<std::string> nomi;
    for (auto& p : profili) nomi.push_back(p.nome);
    int az = Elenco(sx, "PROFILI DI COMPORTAMENTO", nomi, selProfilo, [&](int i, Rectangle q) {
        // Icona: la "faccia" del carattere (rossa se aggressivo, gialla se pauroso, verde se socievole).
        const Carattere& c = profili[i].car;
        Color col = c.aggressivita > 0.6f ? Hex(0xff6b6b) : c.coraggio < 0.3f ? Hex(0xffd23f) : c.socievolezza > 0.6f ? Hex(0x7ee07e) : Hex(0x9aa4ff);
        float cx = q.x + q.width / 2, cy = q.y + q.height / 2, u = q.height / 16;
        DrawCircleV({ cx, cy }, 6 * u, col);
        DrawCircleV({ cx - 2.2f * u, cy - 1.5f * u }, 1.1f * u, Hex(0x0d0f24));
        DrawCircleV({ cx + 2.2f * u, cy - 1.5f * u }, 1.1f * u, Hex(0x0d0f24));
        float b = c.aggressivita > 0.6f ? -1.0f : 1.0f;
        DrawLineEx({ cx - 2.5f * u, cy + 2.5f * u }, { cx, cy + (2.5f + b) * u }, 0.8f * u, Hex(0x0d0f24));
        DrawLineEx({ cx, cy + (2.5f + b) * u }, { cx + 2.5f * u, cy + 2.5f * u }, 0.8f * u, Hex(0x0d0f24));
    });
    if (az == 1) AzioneComportamenti("m:nuovo");
    if (az == 2) AzioneComportamenti("m:duplica");
    if (az == 3) AzioneComportamenti("m:elimina");

    Rectangle resto = { sx.x + sx.width, r.y, r.width - sx.width, r.height };
    // Schede in alto.
    float th = 36 * s, tx = resto.x + 16 * s;
    DrawRectangleRec({ resto.x, resto.y, resto.width, th + 12 * s }, Hex(0x0b0d22));
    for (int i = 0; i < NUM_SCHEDE_COMP; i++) {
        float w = Ui::Larghezza(SCHEDE_COMP[i], Ui::TP()) + 30 * s;
        if (Ui::Pulsante({ tx, resto.y + 6 * s, w, th }, SCHEDE_COMP[i], schedaComp == i)) { schedaComp = i; scorriComp = 0; }
        tx += w + 6 * s;
    }
    if (Ui::Pulsante({ resto.x + resto.width - 190 * s, resto.y + 6 * s, 180 * s, th }, "Prova con i pensieri", true)) AzioneComportamenti("m:prova");
    Rectangle corpo = { resto.x, resto.y + th + 12 * s, resto.width, resto.height - th - 12 * s };

    bool perProfilo = schedaComp <= 3;
    Rectangle sim = { corpo.x + corpo.width - 330 * s, corpo.y, 330 * s, corpo.height };
    Rectangle centro = perProfilo ? Rectangle{ corpo.x, corpo.y, corpo.width - sim.width, corpo.height } : corpo;
    Rectangle area = { centro.x + 24 * s, centro.y + 14 * s, std::min(centro.width - 48 * s, 760 * s), centro.height - 24 * s };

    if (perProfilo && profili.empty()) {
        float y = area.y + 30 * s;
        Ui::TestoForte("La mente delle creature", area.x, y, WHITE, (int)(22 * s));
        y += 40 * s;
        y += Ui::TestoACapo("Ogni nemico e personaggio ha gia' una mente \"istintiva\" ricavata dal suo tipo: vede, ricorda, ha paura, chiama i compagni "
                            "e impara come lo sconfiggi. Qui crei i PROFILI di comportamento per decidere tu il carattere: coraggio, aggressivita', "
                            "prudenza, rancore... e quali azioni preferisce. Poi assegni il profilo ai personaggi.",
                            area.x, y, area.width, Ui::TestoChiaro(), Ui::T()) + 20 * s;
        std::vector<std::string> modelli;
        for (auto& m : ModelliProfilo()) modelli.push_back(m.nome);
        Ui::Scelta({ area.x, y, 420 * s, 32 * s }, "Modello", modelloScelto, modelli);
        if (Ui::Pulsante({ area.x + 430 * s, y, 200 * s, 32 * s }, "Crea dal modello", true)) AzioneComportamenti("m:modello");
        y += 40 * s;
        Ui::TestoACapo(ModelliProfilo()[std::min(modelloScelto, (int)modelli.size() - 1)].descrizione, area.x, y, area.width, Ui::TestoSpento(), Ui::TP());
        y += 40 * s;
        Ui::TestoACapo("Oppure chiedi a Claude: \"crea un profilo per un lupo che caccia in branco e dallo ai lupi\".", area.x, y, area.width, Ui::TestoSpento(), Ui::TP());
        return;
    }

    selProfilo = std::min(selProfilo, std::max(0, (int)profili.size() - 1));
    ProfiloComportamento* pp = profili.empty() ? nullptr : &profili[selProfilo];
    ProfiloComportamento primaP = pp ? *pp : ProfiloComportamento{};
    VarietaPartita primaV = progetto.varieta;
    selOggettoInd = std::min(selOggettoInd, std::max(0, (int)progetto.oggetti.size() - 1));
    IndoleOggetto primaI = progetto.oggetti.empty() ? IndoleOggetto{} : progetto.oggetti[selOggettoInd].indole;
    bool cambiato = false;

    if (Ui::MouseSu(area)) scorriComp = std::max(0.0f, scorriComp - GetMouseWheelMove() * 50 * s);
    Colonna col(area, s, scorriComp);
    Ui::Ritaglia((int)area.x, (int)area.y, (int)area.width, (int)area.height);
    auto Num = [&](const std::string& id, const std::string& et, float& v, float lo, float hi, float passo, const char* f = "%.2f") {
        Rectangle q = col.Riga(30);
        if (col.Vede(q) && Ui::Numero(id, q, et, v, lo, hi, passo, f)) cambiato = true;
    };
    // Tratto del carattere: numero + barra colorata con le parole agli estremi.
    auto Tratto = [&](const std::string& id, const std::string& et, float& v, const char* poco, const char* tanto) {
        Rectangle q = col.Riga(30);
        float wn = q.width * 0.55f;
        if (col.Vede(q)) {
            if (Ui::Numero(id, { q.x, q.y, wn, q.height }, et, v, 0, 1, 0.05f, "%.2f")) cambiato = true;
            Rectangle b = { q.x + wn + 12 * s, q.y + q.height / 2 - 4 * s, q.width - wn - 12 * s, 8 * s };
            DrawRectangleRounded(b, 1, 6, Hex(0x1a1e44));
            DrawRectangleRounded({ b.x, b.y, b.width * Limita(v, 0, 1), b.height }, 1, 6, Ui::Accento());
            // Clic sulla barra = valore diretto.
            Rectangle presa = { b.x, q.y, b.width, q.height };
            if (Ui::MouseSu(presa) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float nv = std::round(Limita((GetMouseX() - b.x) / b.width, 0, 1) * 20) / 20;
                if (nv != v) { v = nv; cambiato = true; }
            }
            Ui::Testo(poco, b.x, b.y + 10 * s, Ui::TestoSpento(), (int)(11 * s));
            Ui::Testo(tanto, b.x + b.width - Ui::Larghezza(tanto, (int)(11 * s)), b.y + 10 * s, Ui::TestoSpento(), (int)(11 * s));
        }
    };
    auto Leva = [&](const std::string& et, bool& v) {
        Rectangle q = col.Riga(28);
        if (col.Vede(q) && Ui::Interruttore(q, et, v)) cambiato = true;
    };
    auto CampoTesto = [&](const std::string& id, const std::string& et, std::string& v) {
        Rectangle q = col.Riga(18);
        if (col.Vede(q)) Ui::Testo(et, q.x, q.y, Ui::TestoSpento(), Ui::TP());
        q = col.Riga(30);
        if (col.Vede(q) && Ui::Campo(id, q, v, 300)) cambiato = true;
    };

    if (perProfilo && pp) {
        ProfiloComportamento& p = *pp;
        Rectangle q = col.Riga(30);
        if (col.Vede(q)) Ui::TestoForte(p.nome, q.x, q.y, WHITE, (int)(22 * s));
        if (schedaComp == 0) {
            col.Sezione("NOME E DESCRIZIONE");
            std::string nome = p.nome;
            q = col.Riga(30);
            if (col.Vede(q) && Ui::Campo("pc_nome", q, nome, 40) && !nome.empty() && nome != p.nome) {
                if (progetto.Profilo(nome)) Avvisa("C'e' gia' un profilo chiamato " + nome);
                else {
                    Ricorda();
                    for (auto& x : progetto.personaggi) if (x.profilo == p.nome) x.profilo = nome;
                    p.nome = nome;
                    primaP.nome = nome;
                    modificato = true;
                }
            }
            q = col.Riga(30);
            if (col.Vede(q) && Ui::Campo("pc_desc", q, p.descrizione, 200)) cambiato = true;
            col.Sezione("CARATTERE");
            Tratto("pc_cor", "Coraggio", p.car.coraggio, "pauroso", "temerario");
            Tratto("pc_agg", "Aggressivita'", p.car.aggressivita, "pacifico", "feroce");
            Tratto("pc_pru", "Prudenza", p.car.prudenza, "avventato", "cauto");
            Tratto("pc_soc", "Socievolezza", p.car.socievolezza, "solitario", "di branco");
            Tratto("pc_ran", "Rancore", p.car.rancore, "perdona", "non dimentica");
            Tratto("pc_cur", "Curiosita'", p.car.curiosita, "pigro", "esploratore");
            Tratto("pc_imp", "Imprevedibilita'", p.car.imprevedibilita, "regolare", "sorprendente");
            col.Sezione("COME CAMBIA");
            Tratto("pc_app", "Apprendimento", p.apprendimento, "mai", "in fretta");
            col.Nota("Quanto la specie impara dal giocatore: se lo schiacci spesso imparera' a schivare i salti, se spari a tenersi lontano. "
                     "Le tendenze apprese hanno un limite: il gioco resta sempre vincibile.", (int)(13 * s));
            Tratto("pc_var", "Variabilita'", p.variabilita, "sempre uguale", "ogni volta diverso");
            col.Nota("Quanto cambia il carattere da una partita all'altra (dal seme della partita).", (int)(13 * s));
            Num("pc_fuga", "Pensa a fuggire sotto l'energia", p.fuga, 0, 1, 0.05f);
            Num("pc_dur", "Secondi prima di cambiare idea", p.durataMinima, 0.1f, 3, 0.1f, "%.1f");
        } else if (schedaComp == 1) {
            col.Sezione("QUANTO PREFERISCE OGNI AZIONE");
            col.Nota("0 = non la fa mai, 1 = normale, fino a 3 = la preferisce. Ogni decimo di secondo la creatura da' un punteggio a ogni azione "
                     "secondo la situazione e il carattere, lo moltiplica per questo peso e sceglie il piu' alto. A destra vedi il calcolo dal vivo.", (int)(13 * s));
            for (int i = 0; i < NUM_AZIONI_MENTE; i++) {
                AzioneMente a = (AzioneMente)i;
                q = col.Riga(30);
                if (col.Vede(q)) {
                    DrawRectangleRec({ q.x, q.y + 4 * s, 5 * s, q.height - 8 * s }, ColoreAzione(a));
                    float wn = q.width * 0.42f;
                    if (Ui::Numero(std::string("pc_w") + ChiaveAzioneMente(a), { q.x + 12 * s, q.y, wn, q.height }, NomeAzioneMente(a), p.peso[i], 0, 3, 0.1f, "%.1f")) cambiato = true;
                    Ui::TestoACapo(SpiegaAzioneMente(a), q.x + wn + 24 * s, q.y + 2 * s, q.width - wn - 24 * s, Ui::TestoSpento(), (int)(12 * s));
                }
            }
        } else if (schedaComp == 2) {
            col.Sezione("REAZIONI");
            Leva("Chiama aiuto quando vede l'eroe", p.chiamaAiuto);
            Leva("Impara insieme agli altri della stessa specie", p.imparaSpecie);
            Leva("Ricorda quello che ha imparato nella partita dopo", p.ricordaPartite);
            col.Nota("Con \"ricorda nella partita dopo\" il gioco creato tiene un file .memoria accanto all'exe: la specie diventa piu' furba partita dopo partita.", (int)(13 * s));
            col.Sezione("COSA DICE (PERSONAGGI CHE PARLANO)");
            col.Nota("La frase cambia con la fiducia verso l'eroe: aiutalo e diventa amico, colpiscilo e avra' paura o rancore. Vuoto = la frase del personaggio.", (int)(13 * s));
            CampoTesto("pc_fa", "Se e' amico", p.fraseAmico);
            CampoTesto("pc_fn", "Se e' neutrale", p.fraseNeutra);
            CampoTesto("pc_fo", "Se e' ostile", p.fraseOstile);
            CampoTesto("pc_fp", "Se ha paura", p.frasePaura);
        } else if (schedaComp == 3) {
            col.Sezione("PERSONAGGI CHE USANO QUESTO PROFILO");
            col.Nota("Clicca un personaggio per dargli (o togliergli) questo profilo. Chi non ne ha uno usa la mente istintiva del suo tipo.", (int)(13 * s));
            for (auto& x : progetto.personaggi) {
                if (x.comportamento.tipo == TipoComportamento::Giocatore || x.nome == progetto.eroe) continue;
                q = col.Riga(30);
                if (!col.Vede(q)) continue;
                bool usa = x.profilo == p.nome;
                std::string et = x.nome + "  (" + NomeComportamento(x.comportamento.tipo) + ")";
                if (!usa && !x.profilo.empty()) et += "  - ora: " + x.profilo;
                if (Ui::Pulsante(q, et, usa)) {
                    Ricorda();
                    x.profilo = usa ? "" : p.nome;
                    modificato = true;
                }
            }
            q = col.Riga(32);
            if (col.Vede(q) && Ui::Pulsante({ q.x, q.y, 280 * s, q.height }, "Dallo a tutti i nemici")) AzioneComportamenti("m:tutti");
        }
    } else if (schedaComp == 4) {
        col.Sezione("INDOLE DEGLI OGGETTI");
        col.Nota("Anche gli oggetti possono cambiare durante la partita: una molla si consuma, un pericolo dorme finche' non ti avvicini, "
                 "una piattaforma diventa piu' veloce ogni volta che la usi.", (int)(13 * s));
        if (progetto.oggetti.empty()) col.Nota("Il gioco non ha ancora oggetti.", Ui::TP());
        else {
            std::vector<std::string> on;
            for (auto& o : progetto.oggetti) on.push_back(o.nome + (o.indole.attiva ? "  *" : ""));
            Rectangle q = col.Riga(32);
            if (col.Vede(q)) Ui::Scelta(q, "Oggetto", selOggettoInd, on);
            OggettoDef& o = progetto.oggetti[selOggettoInd];
            q = col.Riga(18);
            if (col.Vede(q)) Ui::Testo(std::string("Funzione: ") + NomeFunzione(o.funzione), q.x, q.y, Ui::TestoSpento(), Ui::TP());
            IndoleOggetto& in = o.indole;
            Leva("Ha un'indole (cambia durante la partita)", in.attiva);
            if (in.attiva) {
                Num("oi_usura", "Usura a ogni uso", in.usura, 0, 1, 0.05f);
                col.Nota("0.25 = si rompe al quarto uso; 0 = non si consuma mai.", (int)(12 * s));
                Num("oi_ric", "Torna come nuovo dopo (s)", in.ricarica, 0, 300, 1, "%.0f");
                col.Nota("Vale anche per monete e cure raccolte: ricrescono dopo questi secondi (0 = mai).", (int)(12 * s));
                Num("oi_ada", "Con l'uso si indebolisce (-) / rinforza (+)", in.adattamento, -1, 1, 0.1f, "%.1f");
                Leva("Pericolo notturno (attivo solo al buio)", in.notturno);
                Num("oi_sve", "Pericolo: si sveglia dopo (s) con l'eroe vicino", in.risveglio, 0, 10, 0.5f, "%.1f");
            }
        }
    } else if (schedaComp == 5) {
        VarietaPartita& v = progetto.varieta;
        col.Sezione("OGNI PARTITA E' DIVERSA");
        col.Nota("A ogni partita il gioco sceglie un seme e da li' ricava tutto: carattere delle creature, un evento speciale, il tempo, "
                 "chi e' alleato di chi. Le mappe restano quelle che hai disegnato: cambia come le vivi.", (int)(13 * s));
        Leva("Mente delle creature (IA adattiva)", v.mente);
        std::vector<std::string> livelli = { "spenta", "leggera", "media", "forte" };
        Rectangle q = col.Riga(30);
        if (col.Vede(q) && Ui::Scelta(q, "Quanta varieta'", v.livello, livelli)) cambiato = true;
        Leva("Il carattere delle creature cambia un po'", v.carattere);
        Leva("Qualche veterano piu' forte o creatura timida", v.veterani);
        Leva("Monete, gemme e cure si scambiano di posto", v.ricolloca);
        Leva("Ora del giorno e pioggia a caso", v.meteo);
        Leva("Un evento speciale per partita", v.eventi);
        Leva("Specie alleate e rivali, fiducia degli abitanti", v.relazioni);
        int seme = (int)std::min<uint32_t>(v.semeFisso, 999999);
        q = col.Riga(30);
        if (col.Vede(q) && Ui::NumeroIntero("pv_seme", q, "Seme fisso (0 = ogni volta nuovo)", seme, 0, 999999)) { v.semeFisso = (uint32_t)seme; cambiato = true; }
        col.Nota("Con un seme fisso ogni partita e' identica: utile per provare un livello o per una sfida uguale per tutti.", (int)(12 * s));
        if (v.eventi) {
            col.Sezione("EVENTI CHE POSSONO CAPITARE");
            for (auto& ev : EventiPartita()) {
                bool ammesso = std::find(v.eventiVietati.begin(), v.eventiVietati.end(), ev.chiave) == v.eventiVietati.end();
                q = col.Riga(28);
                if (col.Vede(q)) {
                    Ui::Aiuto(ev.descrizione);
                    if (Ui::Interruttore({ q.x, q.y, q.width * 0.45f, q.height }, ev.nome, ammesso)) {
                        if (ammesso) v.eventiVietati.erase(std::remove(v.eventiVietati.begin(), v.eventiVietati.end(), std::string(ev.chiave)), v.eventiVietati.end());
                        else v.eventiVietati.push_back(ev.chiave);
                        cambiato = true;
                    }
                    Ui::TestoACapo(ev.descrizione, q.x + q.width * 0.48f, q.y + 4 * s, q.width * 0.52f, Ui::TestoSpento(), (int)(12 * s));
                }
            }
        }
        col.Sezione("VARIABILI A CASO");
        col.Nota("A inizio partita ricevono un valore a caso fra minimo e massimo (es. \"tesoro\" da 1 a 5): le regole della Logica le possono usare.", (int)(12 * s));
        int togli = -1;
        for (int i = 0; i < (int)v.variabiliCasuali.size(); i++) {
            VariabileCasuale& x = v.variabiliCasuali[i];
            q = col.Riga(30);
            if (!col.Vede(q)) continue;
            float w4 = (q.width - 40 * s) / 4;
            if (Ui::Campo("pv_vn" + std::to_string(i), { q.x, q.y, w4 * 1.3f, q.height }, x.nome, 24)) cambiato = true;
            if (Ui::Numero("pv_vmin" + std::to_string(i), { q.x + w4 * 1.35f, q.y, w4, q.height }, "", x.minimo, -99999, 99999, 1, "%g")) cambiato = true;
            if (Ui::Numero("pv_vmax" + std::to_string(i), { q.x + w4 * 2.4f, q.y, w4, q.height }, "", x.massimo, -99999, 99999, 1, "%g")) cambiato = true;
            if (Ui::Interruttore({ q.x + w4 * 3.45f, q.y, w4 * 0.55f + 4 * s, q.height }, "int", x.intero)) cambiato = true;
            if (Ui::Pulsante({ q.x + q.width - 34 * s, q.y, 34 * s, q.height }, "x")) togli = i;
        }
        if (togli >= 0) { v.variabiliCasuali.erase(v.variabiliCasuali.begin() + togli); cambiato = true; }
        q = col.Riga(30);
        if (col.Vede(q) && Ui::Pulsante({ q.x, q.y, 240 * s, q.height }, "+ Variabile a caso")) {
            VariabileCasuale x;
            x.nome = "tesoro" + (v.variabiliCasuali.empty() ? std::string() : std::to_string(v.variabiliCasuali.size() + 1));
            x.minimo = 1; x.massimo = 5;
            v.variabiliCasuali.push_back(x);
            cambiato = true;
        }
    }
    Ui::FineRitaglio();
    scorriComp = std::min(scorriComp, std::max(0.0f, col.Altezza() - area.height));

    // Simulatore delle decisioni.
    if (perProfilo && pp) {
        DrawRectangleRec(sim, Ui::Pannello());
        DrawLineEx({ sim.x, sim.y }, { sim.x, sim.y + sim.height }, 1, Ui::Bordo());
        Rectangle sa = { sim.x + 12 * s, sim.y + 10 * s, sim.width - 24 * s, sim.height - 20 * s };
        if (Ui::MouseSu(sa)) scorriSim = std::max(0.0f, scorriSim - GetMouseWheelMove() * 50 * s);
        Colonna cs(sa, s, scorriSim);
        Ui::Ritaglia((int)sa.x, (int)sa.y, (int)sa.width, (int)sa.height);
        cs.Sezione("SIMULATORE: COSA FAREBBE?");
        Contesto& c = simContesto;
        c.abitante = simAbitante;
        auto N = [&](const char* id, const char* et, float& v, float lo, float hi, float passo) {
            Rectangle q = cs.Riga(26);
            if (cs.Vede(q)) Ui::Numero(id, q, et, v, lo, hi, passo, "%.2f");
        };
        auto L = [&](const char* et, float& v) {
            Rectangle q = cs.Riga(26);
            bool b = v > 0.5f;
            if (cs.Vede(q) && Ui::Interruttore(q, et, b)) v = b ? 1.0f : 0.0f;
        };
        L("Vede l'eroe", c.vede);
        N("sim_dist", "Distanza (viste)", c.distanza, 0, 3, 0.1f);
        N("sim_en", "Sua energia", c.energia, 0, 1, 0.1f);
        N("sim_ene", "Energia dell'eroe", c.energiaEroe, 0, 1, 0.1f);
        N("sim_all", "Compagni vicini", c.alleati, 0, 5, 1);
        L("Cura vicina", c.cura);
        L("Proiettile in arrivo", c.proiettile);
        L("Eroe che gli salta sopra", c.eroeSopra);
        L("Rivale vicino", c.rivale);
        L("Ricorda dove era l'eroe", c.memoria);
        L("Puo' sparare", c.puoSparare);
        { Rectangle q = cs.Riga(26); if (cs.Vede(q)) Ui::Interruttore(q, "E' un abitante che parla", simAbitante); }
        N("sim_min", "Minaccia sentita", simMinaccia, 0, 1, 0.1f);
        N("sim_pau", "Paura", simPaura, 0, 1, 0.1f);
        N("sim_fid", "Fiducia verso l'eroe", simFiducia, -1, 1, 0.1f);
        // Il calcolo: identico al gioco, senza rumore.
        Mente m;
        m.attiva = true;
        m.car = pp->car;
        m.minaccia = simMinaccia; m.paura = simPaura; m.fiducia = simFiducia;
        m.azione = AzioneMente::Totale;
        if (c.memoria > 0.5f) m.ultimoVisto = { 0, 0 };
        Sapere sap;
        float pt[NUM_AZIONI_MENTE], massimo = 0.001f;
        int migliore = 0;
        for (int i = 0; i < NUM_AZIONI_MENTE; i++) {
            pt[i] = PunteggioAzione(m, *pp, sap, c, (AzioneMente)i);
            if (pt[i] > pt[migliore]) migliore = i;
            massimo = std::max(massimo, pt[i]);
        }
        cs.Sezione("PUNTEGGI");
        Rectangle q = cs.Riga(26);
        if (cs.Vede(q)) Ui::TestoForte(std::string("Sceglie: ") + NomeAzioneMente((AzioneMente)migliore), q.x, q.y, ColoreAzione((AzioneMente)migliore), Ui::T());
        for (int i = 0; i < NUM_AZIONI_MENTE; i++) {
            q = cs.Riga(18);
            if (!cs.Vede(q)) continue;
            float lw = 100 * s;
            Ui::Testo(NomeAzioneMente((AzioneMente)i), q.x, q.y, i == migliore ? WHITE : Ui::TestoSpento(), (int)(12 * s));
            Rectangle b = { q.x + lw, q.y + 3 * s, q.width - lw - 44 * s, q.height - 6 * s };
            DrawRectangleRec(b, Hex(0x1a1e44));
            DrawRectangleRec({ b.x, b.y, b.width * Limita(pt[i] / massimo, 0, 1), b.height }, i == migliore ? ColoreAzione((AzioneMente)i) : Fade(ColoreAzione((AzioneMente)i), 0.45f));
            Ui::Testo(TextFormat("%.2f", pt[i]), b.x + b.width + 6 * s, q.y, Ui::TestoSpento(), (int)(12 * s));
        }
        cs.Nota("Nel gioco si aggiungono l'isteresi (resta sull'azione in corso), un pizzico di sorpresa e cio' che la specie ha imparato.", (int)(12 * s));
        Ui::FineRitaglio();
        scorriSim = std::min(scorriSim, std::max(0.0f, cs.Altezza() - sa.height));
    }

    if (cambiato) {
        // Una voce di annullamento con lo stato di prima (la modifica e' gia' fatta sui controlli).
        ProgettoGioco v = progetto;
        if (pp && selProfilo < (int)v.profili.size()) v.profili[selProfilo] = primaP;
        v.varieta = primaV;
        if (!v.oggetti.empty() && selOggettoInd < (int)v.oggetti.size()) v.oggetti[selOggettoInd].indole = primaI;
        annulla.push_back(v);
        if (annulla.size() > 30) annulla.erase(annulla.begin());
        ripeti.clear();
        modificato = true;
    }
}
