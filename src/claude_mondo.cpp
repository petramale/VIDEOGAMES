#include "claude_mondo.h"
#include "livello.h"
#include "sfondo.h"
#include "audio.h"
#include "json.hpp"
#include <cctype>

using nlohmann::json;

std::string PromptMondo() {
    std::string brani;
    for (size_t i = 0; i < Audio::Brani().size(); i++) brani += std::to_string(i) + " " + Audio::Brani()[i] + (i + 1 < Audio::Brani().size() ? ", " : "");
    std::string temi;
    for (size_t i = 0; i < Sfondo::Temi().size(); i++) temi += std::to_string(i) + " " + Sfondo::Temi()[i].nome + (i + 1 < Sfondo::Temi().size() ? ", " : "");
    return
        "Sei Claude, l'assistente creativo dello Studio Videogiochi (sviluppato da Mario Petramale). "
        "L'utente crea videogiochi 2D in pixel art e ti scrive in italiano. A ogni richiesta ricevi lo stato del MONDO in JSON "
        "(formato studio-videogiochi/2) e l'indice della mappa aperta nell'editor.\n\n"
        "RISPONDI SOLO con un oggetto JSON valido, senza testo prima o dopo e senza blocchi di codice:\n"
        "{\"risposta\": \"messaggio breve e amichevole in italiano: cosa hai fatto, oppure una domanda\", \"azioni\": [ ... ]}\n\n"
        "IL FORMATO DEL MONDO\n"
        "- Il mondo ha titolo, genere (platform, openworld, sparatutto, avventura, labirinto, puzzle), fisica (gravita 300-2000, "
        "salto 150-600, velocita 40-300, molla 250-900; standard 900/305/115/470), eroe (colori #rrggbb: pelle, capelli, maglia, mantello, "
        "pantaloni, scarpe) e un elenco di mappe.\n"
        "- Ogni mappa: nome, larghezza e altezza in caselle da 16 pixel (8-1000), vista \"laterale\" (con gravita') o \"alto\" (vista dall'alto, "
        "senza gravita'), musica (brano: " + brani + "), scenario {tema (" + temi + "), ora 0-1 (0.25 alba, 0.5 mezzogiorno, 0.75 tramonto, 0.9 notte), "
        "cicloGiorno, sfondoGenerato}, cielo opzionale {alto, basso} (colori), strati di sfondo, telecamera, partenza {x, y} in pixel (piedi dell'eroe), "
        "caselle, oggetti e testi dei cartelli.\n"
        "- caselle: 'altezza' stringhe lunghe 'larghezza'. Caratteri: ' ' vuoto, '#' terra con erba (dall'alto: siepe), '=' pietra/muro, "
        "'-' piattaforma attraversabile dal basso, '^' spine, '.' bersaglio delle casse (puzzle). Nella vista laterale le ultime righe sono il terreno; "
        "nella vista dall'alto il bordo della mappa deve essere di muri.\n"
        "- oggetti: {tipo, x, y} in pixel. Tipi: moneta, gemma, molla, forziere, lanterna (punto di controllo), slime (nemico che pattuglia), "
        "pipistrello (nemico volante), bandiera (arrivo), cuore (energia), cartello (numero = quale testo), porta, fantasma (insegue), "
        "torretta (spara), boss (tre fasi), cassa (puzzle). Una porta con \"mappa\": N porta alla mappa N (destX, destY = punto d'arrivo in pixel, "
        "-1 = partenza di quella mappa); senza \"mappa\" e' l'uscita del livello. Una bandiera con \"mappa\": N, dopo la vittoria, passa alla mappa N.\n"
        "- strati: {tipo: montagne|colline|foresta|nuvole|stelle|fascia|immagine, colore, parallasseX 0-1, parallasseY 0-1, altezza 0-1 "
        "(posizione verticale sullo schermo), ampiezza (pixel), seme, vento, primoPiano}. Se sfondoGenerato e' false si vedono solo cielo e strati.\n"
        "- telecamera: {modo: segui|stanze|automatica|fissa, zonaMortaX, zonaMortaY, anticipo, velocita (automatica, pixel/s), dirX, dirY (-1, 0, 1), zoom 0.5-3}.\n"
        "- testi: [{numero 1-9, it, en}] per i cartelli.\n\n"
        "LE AZIONI (usa solo quelle che servono, anche nessuna; vengono applicate in ordine)\n"
        "- {\"tipo\": \"mondo\", \"modifiche\": {...}}: cambia titolo, genere, fisica, eroe (anche solo alcuni campi).\n"
        "- {\"tipo\": \"mappa\", \"mappa\": N, \"modifiche\": {...}}: cambia i campi indicati della mappa N (gli altri restano). "
        "Per ridisegnare la mappa manda \"caselle\" completo (e se cambi le dimensioni anche larghezza e altezza). "
        "\"strati\", \"oggetti\" e \"testi\" se presenti sostituiscono l'elenco intero.\n"
        "- {\"tipo\": \"rettangolo\", \"mappa\": N, \"x0\", \"y0\", \"x1\", \"y1\", \"casella\": \"#\"}: riempie un rettangolo di caselle (coordinate in caselle).\n"
        "- {\"tipo\": \"aggiungi\", \"mappa\": N, \"oggetti\": [{\"tipo\": \"moneta\", \"cx\": 10, \"cy\": 5}, ...]}: aggiunge oggetti nella casella (cx, cy); "
        "per le porte puoi aggiungere mappa, destX, destY; per i cartelli numero.\n"
        "- {\"tipo\": \"rimuovi\", \"mappa\": N, \"caselle\": [[cx, cy], ...]} oppure {\"tipo\": \"rimuovi\", \"mappa\": N, \"oggettiTipo\": \"slime\"}: toglie oggetti.\n"
        "- {\"tipo\": \"nuovaMappa\", \"mappa\": {mappa completa}}: aggiunge una mappa in fondo all'elenco.\n"
        "- {\"tipo\": \"apri\", \"mappa\": N}: mostra all'utente la mappa N nell'editor.\n\n"
        "REGOLE\n"
        "- Nella vista laterale l'eroe salta circa 3 caselle in alto (piu' con salto alto) e 4-5 in lungo: ogni parte deve essere raggiungibile. "
        "Gli oggetti appoggiati (eroe, nemici a terra, forziere, bandiera, porta, cartello, lanterna, molla, torretta, boss) vanno sopra una casella solida.\n"
        "- Per piccole modifiche usa rettangolo, aggiungi e rimuovi invece di riscrivere tutte le caselle.\n"
        "- Se la richiesta non si puo' fare con queste azioni, dillo con gentilezza e proponi l'alternativa piu' vicina.";
}

std::string StatoMondoPerClaude(const Mondo& m, int mappaAperta) {
    return "Mappa aperta nell'editor: " + std::to_string(mappaAperta) + " (\"" +
           (mappaAperta >= 0 && mappaAperta < (int)m.mappe.size() ? m.mappe[mappaAperta].nome : std::string()) + "\")\n" +
           "Mondo:\n" + m.Json(true);
}

namespace {

int IndiceMappa(const json& a, const Mondo& m, int aperta) {
    int i = aperta;
    if (a.contains("mappa") && a["mappa"].is_number_integer()) i = a["mappa"].get<int>();
    return (i >= 0 && i < (int)m.mappe.size()) ? i : -1;
}

int Intero(const json& a, const char* k, int predefinito) {
    if (a.contains(k) && a[k].is_number()) return (int)std::lround(a[k].get<double>());
    return predefinito;
}

bool Applica(Mondo& m, int aperta, const json& a, std::string& fatto, std::string& errore, int& apri) {
    if (!a.is_object()) { errore = "azione non valida"; return false; }
    std::string tipo = a.value("tipo", std::string());
    if (tipo == "mondo") {
        if (!a.contains("modifiche") || !a["modifiche"].is_object()) { errore = "mondo: mancano le modifiche"; return false; }
        json j = json::parse(m.Json(true));
        json mod = a["modifiche"];
        mod.erase("mappe");   // le mappe si cambiano con le loro azioni
        j.merge_patch(mod);
        Mondo nuovo;
        if (!nuovo.DaJson(j.dump(), errore)) return false;
        nuovo.fisica.gravita = Limita(nuovo.fisica.gravita, 300, 2000);
        nuovo.fisica.salto = Limita(nuovo.fisica.salto, 150, 600);
        nuovo.fisica.velocita = Limita(nuovo.fisica.velocita, 40, 300);
        nuovo.fisica.molla = Limita(nuovo.fisica.molla, 250, 900);
        m = nuovo;
        fatto = "impostazioni del mondo";
        return true;
    }
    if (tipo == "mappa") {
        int i = IndiceMappa(a, m, aperta);
        if (i < 0) { errore = "mappa inesistente"; return false; }
        if (!a.contains("modifiche") || !a["modifiche"].is_object()) { errore = "mappa: mancano le modifiche"; return false; }
        json j = json::parse(MappaInJson(m.mappe[i]));
        json mod = a["modifiche"];
        // Le voci dello scenario si accettano anche fuori da "scenario" (Claude a volte le mette li').
        for (const char* k : { "tema", "ora", "cicloGiorno", "sfondoGenerato" })
            if (mod.contains(k)) { mod["scenario"][k] = mod[k]; mod.erase(k); }
        // Caselle nuove con dimensioni diverse: le dimensioni le danno le righe.
        if (mod.contains("caselle") && mod["caselle"].is_array()) {
            if (!mod.contains("altezza")) mod["altezza"] = (int)mod["caselle"].size();
            if (!mod.contains("larghezza")) {
                size_t w = 0;
                for (auto& r : mod["caselle"]) if (r.is_string()) w = std::max(w, r.get<std::string>().size());
                mod["larghezza"] = (int)w;
            }
        }
        j.merge_patch(mod);
        Mappa nuova;
        if (!MappaDaJson(j.dump(), nuova, errore)) return false;
        m.mappe[i] = nuova;
        std::string cosa;
        for (auto it = mod.begin(); it != mod.end(); ++it) cosa += (cosa.empty() ? "" : ", ") + it.key();
        fatto = "mappa \"" + nuova.nome + "\" (" + cosa + ")";
        return true;
    }
    if (tipo == "rettangolo") {
        int i = IndiceMappa(a, m, aperta);
        if (i < 0) { errore = "mappa inesistente"; return false; }
        std::string c = a.value("casella", a.value("carattere", std::string(" ")));
        if (c.empty()) c = " ";
        if (std::string(" #=-^.").find(c[0]) == std::string::npos) { errore = "casella sconosciuta '" + c + "'"; return false; }
        Mappa& mp = m.mappe[i];
        int x0 = Intero(a, "x0", 0), y0 = Intero(a, "y0", 0), x1 = Intero(a, "x1", x0), y1 = Intero(a, "y1", y0);
        if (x0 > x1) std::swap(x0, x1);
        if (y0 > y1) std::swap(y0, y1);
        x0 = std::max(0, x0); y0 = std::max(0, y0); x1 = std::min(mp.larghezza - 1, x1); y1 = std::min(mp.altezza - 1, y1);
        if (x0 > x1 || y0 > y1) { errore = "rettangolo fuori dalla mappa"; return false; }
        for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) mp.ImpostaCasella(x, y, c[0]);
        fatto = TextFormat("caselle '%c' da %d,%d a %d,%d", c[0], x0, y0, x1, y1);
        return true;
    }
    if (tipo == "aggiungi") {
        int i = IndiceMappa(a, m, aperta);
        if (i < 0) { errore = "mappa inesistente"; return false; }
        if (!a.contains("oggetti") || !a["oggetti"].is_array()) { errore = "aggiungi: manca l'elenco degli oggetti"; return false; }
        Mappa& mp = m.mappe[i];
        int n = 0;
        for (auto& o : a["oggetti"]) {
            if (!o.is_object()) continue;
            OggettoMappa nuovo;
            nuovo.tipo = o.value("tipo", std::string());
            TipoOggetto t;
            if (!TipoDaNome(nuovo.tipo, t)) { errore = "oggetto sconosciuto \"" + nuovo.tipo + "\""; return false; }
            nuovo.tipo = ChiaveOggetto(t);
            int cx = Intero(o, "cx", -1), cy = Intero(o, "cy", -1);
            if (cx < 0 || cy < 0 || cx >= mp.larghezza || cy >= mp.altezza) { errore = nuovo.tipo + " fuori dalla mappa"; return false; }
            Vector2 p = PosizioneInCasella(nuovo.tipo, cx, cy, mp.vista);
            nuovo.x = p.x; nuovo.y = p.y;
            nuovo.numero = Intero(o, "numero", 0);
            nuovo.mappa = Intero(o, "mappa", -1);
            if (o.contains("destX") && o["destX"].is_number()) nuovo.destX = o["destX"].get<float>();
            if (o.contains("destY") && o["destY"].is_number()) nuovo.destY = o["destY"].get<float>();
            mp.oggetti.push_back(nuovo);
            n++;
        }
        fatto = std::to_string(n) + " oggetti aggiunti";
        return true;
    }
    if (tipo == "rimuovi") {
        int i = IndiceMappa(a, m, aperta);
        if (i < 0) { errore = "mappa inesistente"; return false; }
        Mappa& mp = m.mappe[i];
        size_t prima = mp.oggetti.size();
        if (a.contains("oggettiTipo") && a["oggettiTipo"].is_string()) {
            std::string t = a["oggettiTipo"].get<std::string>();
            for (auto& ch : t) ch = (char)std::tolower((unsigned char)ch);
            std::vector<OggettoMappa> resto;
            for (auto& o : mp.oggetti) if (o.tipo != t && t != "tutti") resto.push_back(o);
            mp.oggetti = resto;
        }
        if (a.contains("caselle") && a["caselle"].is_array())
            for (auto& c : a["caselle"]) {
                if (!c.is_array() || c.size() < 2 || !c[0].is_number() || !c[1].is_number()) continue;
                int cx = c[0].get<int>(), cy = c[1].get<int>();
                std::vector<OggettoMappa> resto;
                for (auto& o : mp.oggetti) if ((int)(o.x / TILE) != cx || (int)((o.y - 1) / TILE) != cy) resto.push_back(o);
                mp.oggetti = resto;
            }
        fatto = std::to_string(prima - mp.oggetti.size()) + " oggetti tolti";
        return true;
    }
    if (tipo == "nuovaMappa") {
        if (!a.contains("mappa") || !a["mappa"].is_object()) { errore = "nuovaMappa: manca la mappa"; return false; }
        Mappa nuova;
        if (!MappaDaJson(a["mappa"].dump(), nuova, errore)) return false;
        if (m.mappe.size() >= 64) { errore = "troppe mappe"; return false; }
        m.mappe.push_back(nuova);
        apri = (int)m.mappe.size() - 1;
        fatto = "nuova mappa \"" + nuova.nome + "\"";
        return true;
    }
    if (tipo == "apri") {
        int i = IndiceMappa(a, m, -1);
        if (i < 0) { errore = "mappa inesistente"; return false; }
        apri = i;
        fatto = "";
        return true;
    }
    errore = "azione sconosciuta \"" + tipo + "\"";
    return false;
}

} // namespace

EsitoClaude ApplicaRispostaClaude(Mondo& mondo, int mappaAperta, const std::string& jsonRisposta) {
    EsitoClaude e;
    json j = json::parse(jsonRisposta, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        // Testo intorno al JSON (spiegazioni, ```json ...```): si prende il primo oggetto completo.
        size_t a = jsonRisposta.find('{'), b = jsonRisposta.rfind('}');
        if (a != std::string::npos && b != std::string::npos && b > a) j = json::parse(jsonRisposta.substr(a, b - a + 1), nullptr, false);
    }
    if (j.is_discarded() || !j.is_object()) {
        // Solo testo: Claude ha risposto a parole, senza modifiche.
        std::string t = jsonRisposta;
        while (!t.empty() && std::isspace((unsigned char)t.back())) t.pop_back();
        e.risposta = t.empty() ? "Non ho capito la risposta di Claude, puoi riprovare?" : t;
        return e;
    }
    if (j.contains("risposta") && j["risposta"].is_string()) e.risposta = j["risposta"].get<std::string>();
    if (!j.contains("azioni") || !j["azioni"].is_array()) return e;
    Mondo prova = mondo;   // tutto su una copia: se il risultato non e' valido non si tocca nulla
    for (auto& a : j["azioni"]) {
        std::string fatto, errore;
        int apri = -1;
        try {
            if (Applica(prova, mappaAperta, a, fatto, errore, apri)) {
                if (!fatto.empty()) e.fatte.push_back(fatto);
                if (apri >= 0) e.apriMappa = apri;
            } else e.scartate.push_back(errore);
        } catch (const std::exception& ex) {
            e.scartate.push_back(std::string("errore: ") + ex.what());
        }
    }
    std::string problema = prova.Controlla();
    if (!problema.empty()) {
        e.scartate.push_back("il risultato non era valido (" + problema + "): nessuna modifica");
        e.fatte.clear();
        e.apriMappa = -1;
        return e;
    }
    if (!e.fatte.empty()) { mondo = prova; e.modificato = true; }
    return e;
}
