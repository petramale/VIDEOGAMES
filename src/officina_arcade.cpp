// Sezione "Gioco arcade" dell'officina: l'editor dei nuovi tipi di gioco (Stellaria, Sciame di Orione,
// Polvere da Sparo, Querciaverde). Ogni gioco descrive le sue parti modificabili con uno schema
// (categorie, elenchi ed i loro campi); qui diventano elenchi, numeri, colori, scelte, brani ed effetti.
// A destra un'anteprima dal vivo del livello scelto; F5 prova il gioco da quel livello.
#include "officina.h"
#include "arcade.h"
#include "audio.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

using nlohmann::json;

namespace {
// Il nome mostrato di un elemento di un elenco: il suo "nome", oppure "Ondata 3 - Nebulosa Rosa".
std::string NomeElemento(const json& schema, const json& dati, const json& cat, int i) {
    const json& lista = dati[cat["chiave"].get<std::string>()];
    const json& e = lista[i];
    if (e.contains("nome") && e["nome"].is_string() && !e["nome"].get<std::string>().empty()) return e["nome"].get<std::string>();
    std::string n = cat.value("elemento", std::string("Elemento")) + " " + std::to_string(i + 1);
    // il primo campo che rimanda a un altro elenco (es. il cielo dell'ondata) completa il nome
    for (auto& c : cat["campi"]) {
        if (c.value("tipo", "") != "rif") continue;
        std::string da = c.value("da", "");
        int k = e.value(c["chiave"].get<std::string>(), -1);
        if (dati.contains(da) && dati[da].is_array() && k >= 0 && k < (int)dati[da].size()) {
            const json& t = dati[da][k];
            if (t.contains("nome") && t["nome"].is_string()) n += " - " + t["nome"].get<std::string>();
        }
        break;
    }
    (void)schema;
    return n;
}
}

bool Officina::SezioneVisibile(int i) const {
    Sezione s = (Sezione)i;
    if (progetto.EArcade()) return s == Sezione::Home || s == Sezione::Arcade || s == Sezione::Audio || s == Sezione::Comandi;
    return s != Sezione::Arcade;
}

void Officina::SincronizzaArcade() {
    if (!progetto.EArcade()) return;
    if (tipoSchemaArcade != progetto.arcade) {
        auto g = arcade::Crea(progetto.arcade);
        schemaArcade = g ? g->Schema() : json::array();
        predefinitiArcade = g ? g->Predefiniti() : json::object();
        tipoSchemaArcade = progetto.arcade;
        catArcade = selArcade = 0;
    }
    if (datiArcadeTesto != progetto.arcadeDati || datiArcade.is_null()) {
        json d = json::parse(progetto.arcadeDati.empty() ? std::string("{}") : progetto.arcadeDati, nullptr, false);
        datiArcade = arcade::Unisci(predefinitiArcade, d.is_object() ? d : json::object());
        datiArcadeTesto = progetto.arcadeDati;
    }
}

void Officina::ArcadeCambiato() {
    Ricorda();
    progetto.arcadeDati = datiArcade.dump();
    datiArcadeTesto = progetto.arcadeDati;
    modificato = true;
}

// --- La sezione -------------------------------------------------------------------------------------

void Officina::Arcade(Rectangle r, float dt) {
    float s = Ui::S();
    if (!progetto.EArcade()) {
        Ui::TestoACapo("Questo gioco e' a mappe: per un gioco arcade usa Gioco > Nuovo gioco e scegli uno dei giochi arcade.", r.x + 30 * s, r.y + 30 * s, r.width - 60 * s, Ui::TestoChiaro(), Ui::T());
        return;
    }
    SincronizzaArcade();
    if (!schemaArcade.is_array() || schemaArcade.empty()) return;
    catArcade = std::max(0, std::min(catArcade, (int)schemaArcade.size() - 1));
    const json cat = schemaArcade[catArcade];
    const std::string chiave = cat["chiave"];
    bool lista = cat.value("lista", false);

    // --- colonna delle categorie ---
    Rectangle rc = { r.x, r.y, 210 * s, r.height };
    DrawRectangleRec(rc, Hex(0x0b0e26));
    Ui::Testo(arcade::TitoloDi(progetto.arcade), rc.x + 12 * s, rc.y + 12 * s, Ui::Accento(), Ui::TP());
    float y = rc.y + 44 * s;
    for (int i = 0; i < (int)schemaArcade.size(); i++) {
        const json& c = schemaArcade[i];
        std::string t = c["nome"];
        if (c.value("lista", false)) {
            std::string k = c["chiave"];
            if (datiArcade.contains(k) && datiArcade[k].is_array()) t += "  (" + std::to_string(datiArcade[k].size()) + ")";
        }
        if (Ui::Pulsante({ rc.x + 8 * s, y, rc.width - 16 * s, 40 * s }, t, i == catArcade) && i != catArcade) {
            catArcade = i; selArcade = 0; scorriCampiArcade = 0; scorriLista = 0;
        }
        y += 46 * s;
    }
    Ui::TestoACapo("Le modifiche si vedono subito nell'anteprima. F5 prova il gioco dal livello scelto; Crea .exe fa il gioco completo.",
                   rc.x + 12 * s, y + 10 * s, rc.width - 24 * s, Ui::TestoSpento(), Ui::TP());

    // --- elenco degli elementi ---
    float x0 = rc.x + rc.width;
    json* elem = &datiArcade[chiave];
    if (lista) {
        if (!datiArcade[chiave].is_array()) datiArcade[chiave] = json::array();
        json& arr = datiArcade[chiave];
        std::vector<std::string> nomi;
        for (int i = 0; i < (int)arr.size(); i++) nomi.push_back(NomeElemento(schemaArcade, datiArcade, cat, i));
        Rectangle rl = { x0, r.y, 270 * s, r.height };
        int mn = cat.value("min", 0), mx = cat.value("max", 999);
        bool modificabile = mn != mx;
        int az = Elenco(rl, cat["nome"].get<std::string>(), nomi, selArcade, [&](int i, Rectangle ic) {
            const json& e = arr[i];
            Color c = Hex(0x6a7aff);
            for (const char* k : { "chiaro", "alto", "colore", "luce" }) if (e.contains(k)) { c = arcade::ColoreDi(e, k, c); break; }
            DrawRectangleRec(ic, ConAlfa(c, 0.85f));
            Ui::TestoCentrato(std::to_string(i + 1), ic, Hex(0x101428), Ui::T());
        }, modificabile);
        if (az == 1 && (int)arr.size() < mx) {
            json nuovo = arr.empty() ? json::object() : arr[std::min(selArcade, (int)arr.size() - 1)];
            if (nuovo.contains("nome")) nuovo["nome"] = nuovo["nome"].get<std::string>() + " nuovo";
            arr.insert(arr.begin() + std::min((int)arr.size(), selArcade + 1), nuovo);
            selArcade++;
            ArcadeCambiato();
            Avvisa(cat.value("elemento", std::string("Elemento")) + " aggiunto dopo quello scelto: cambialo come vuoi");
        } else if (az == 2 && !arr.empty() && (int)arr.size() < mx) {
            arr.insert(arr.begin() + selArcade + 1, arr[selArcade]);
            selArcade++;
            ArcadeCambiato();
        } else if (az == 3 && (int)arr.size() > mn) {
            arr.erase(arr.begin() + selArcade);
            selArcade = std::max(0, selArcade - 1);
            ArcadeCambiato();
        } else if ((az == 1 || az == 2) && (int)arr.size() >= mx) Avvisa("Al massimo " + std::to_string(mx));
        else if (az == 3) Avvisa("Ne servono almeno " + std::to_string(mn));
        if (arr.empty()) return;
        selArcade = std::max(0, std::min(selArcade, (int)arr.size() - 1));
        elem = &arr[selArcade];
        x0 += rl.width;
    }

    // --- campi dell'elemento ---
    Rectangle rp = { x0, r.y, r.x + r.width - x0, r.height };
    float pad = 18 * s;
    std::string titolo = lista ? NomeElemento(schemaArcade, datiArcade, cat, selArcade) : cat["nome"].get<std::string>();
    Titolo({ rp.x + pad, rp.y + 12 * s, 0, 0 }, titolo);
    float hAiuto = Ui::TestoACapo(cat.value("aiuto", std::string("")), rp.x + pad, rp.y + 46 * s, rp.width - 2 * pad, Ui::TestoSpento(), Ui::TP());
    float top = rp.y + 54 * s + hAiuto;

    // anteprima dal vivo a destra (se c'e' spazio)
    bool conAnteprima = rp.width > 760 * s;
    float wa = conAnteprima ? std::min(rp.width * 0.46f, 640 * s) : 0;
    Rectangle ra = { rp.x + rp.width - wa - pad, top, wa, wa * 9 / 16 };
    Rectangle rf = { rp.x + pad, top, rp.width - 2 * pad - (conAnteprima ? wa + pad : 0), rp.y + rp.height - top - 10 * s };

    // campi (scorrono)
    const json& campi = cat["campi"];
    float hc = 36 * s, gap = 8 * s;
    float totale = campi.size() * (hc + gap);
    if (Ui::MouseSu(rf)) scorriCampiArcade -= GetMouseWheelMove() * 60 * s;
    scorriCampiArcade = Limita(scorriCampiArcade, 0, std::max(0.0f, totale - rf.height));
    Ui::Ritaglia((int)rf.x, (int)rf.y, (int)rf.width, (int)rf.height);
    float yy = rf.y - scorriCampiArcade;
    bool cambiato = false;
    std::string idBase = chiave + ":" + std::to_string(selArcade) + ":";
    for (auto& c : campi) {
        Rectangle f = { rf.x, yy, rf.width, hc };
        yy += hc + gap;
        if (f.y + f.height < rf.y || f.y > rf.y + rf.height) continue;
        std::string k = c["chiave"], nome = c.value("nome", k), tipo = c.value("tipo", "numero");
        json& v = (*elem)[k];
        std::string id = idBase + k;
        float etW = std::min(f.width * 0.45f, 300 * s);
        if (tipo == "numero") {
            float x = v.is_number() ? v.get<float>() : (float)c.value("min", 0.0);
            float passo = (float)c.value("passo", 1.0);
            if (Ui::Numero(id, f, nome, x, (float)c.value("min", 0.0), (float)c.value("max", 100.0), passo, c.value("formato", std::string("%.0f")).c_str())) {
                if (passo >= 1 && std::fabs(passo - std::round(passo)) < 1e-6f) v = (int)std::lround(x); else v = std::round(x * 1000) / 1000;
                cambiato = true;
            }
        } else if (tipo == "testo") {
            Ui::Testo(nome, f.x, f.y + (hc - Ui::TP()) / 2, Ui::TestoChiaro(), Ui::TP());
            std::string t = v.is_string() ? v.get<std::string>() : "";
            if (Ui::Campo(id, { f.x + etW, f.y, f.width - etW, hc }, t, 80)) { v = t; cambiato = true; }
        } else if (tipo == "colore") {
            Color col = arcade::ColoreDi(*elem, k.c_str(), GRAY);
            if (Ui::Colore(id, f, nome, col)) { v = arcade::Esa(col); cambiato = true; }
        } else if (tipo == "vero") {
            bool b = v.is_boolean() ? v.get<bool>() : false;
            if (Ui::Interruttore(f, nome, b)) { v = b; cambiato = true; }
        } else if (tipo == "scelta") {
            std::vector<std::string> voci;
            for (auto& x : c["voci"]) voci.push_back(x);
            int i = v.is_number() ? v.get<int>() : 0;
            i = std::max(0, std::min(i, (int)voci.size() - 1));
            if (Ui::Scelta(f, nome, i, voci)) { v = i; cambiato = true; }
        } else if (tipo == "rif") {
            std::string da = c.value("da", "");
            bool vuoto = c.contains("vuoto");
            std::vector<std::string> voci;
            if (vuoto) voci.push_back(c["vuoto"]);
            const json* cd = nullptr;
            for (auto& cc : schemaArcade) if (cc["chiave"] == da) cd = &cc;
            if (cd && datiArcade[da].is_array()) for (int i = 0; i < (int)datiArcade[da].size(); i++) voci.push_back(NomeElemento(schemaArcade, datiArcade, *cd, i));
            int i = (v.is_number() ? v.get<int>() : 0) + (vuoto ? 1 : 0);
            i = std::max(0, std::min(i, (int)voci.size() - 1));
            if (!voci.empty() && Ui::Scelta(f, nome, i, voci)) { v = i - (vuoto ? 1 : 0); cambiato = true; }
        } else if (tipo == "brano" || tipo == "effetto") {
            std::vector<std::string> voci = { tipo == "brano" ? "(nessuna)" : "(predefinito)" };
            if (tipo == "brano") for (auto& b : progetto.brani) voci.push_back(b.nome);
            else for (auto& e : progetto.effetti) voci.push_back(e.nome);
            std::string attuale = v.is_string() ? v.get<std::string>() : "";
            int i = 0;
            for (int q = 1; q < (int)voci.size(); q++) if (voci[q] == attuale) i = q;
            Rectangle fs = f;
            if (tipo == "effetto") fs.width -= 90 * s;
            if (Ui::Scelta(fs, nome, i, voci)) { v = i == 0 ? std::string("") : voci[i]; cambiato = true; }
            if (tipo == "effetto" && Ui::Pulsante({ f.x + f.width - 84 * s, f.y, 84 * s, hc }, "Ascolta")) {
                if (i > 0) { if (const EffettoSuono* e = progetto.TrovaEffetto(voci[i])) gAudio.SuonaEffetto(*e); }
                else arcade::Suona(k);
            }
        }
    }
    Ui::FineRitaglio();
    if (cambiato) ArcadeCambiato();

    // --- anteprima dal vivo ---
    if (conAnteprima) {
        AnteprimaArcade(ra, dt, lista && catArcade == 0 ? selArcade : 0);
        float by = ra.y + ra.height + 12 * s, bw = (ra.width - 10 * s) / 2;
        if (Ui::Pulsante({ ra.x, by, bw, 42 * s }, catArcade == 0 && lista ? "Prova da qui  F5" : "Prova il gioco  F5", true)) AvviaProvaArcade(catArcade == 0 && lista ? selArcade : 0);
        if (Ui::Pulsante({ ra.x + bw + 10 * s, by, bw, 42 * s }, "Crea .exe")) CreaEseguibile();
        by += 52 * s;
        if (Ui::Pulsante({ ra.x, by, bw, 36 * s }, "Valori predefiniti")) AzioneArcade("a:predefinito");
        if (Ui::Pulsante({ ra.x + bw + 10 * s, by, bw, 36 * s }, "Tutto come all'inizio")) AzioneArcade("a:tutto");
        by += 46 * s;
        Ui::TestoACapo("Anteprima: il pilota automatico gioca " + std::string(lista && catArcade == 0 ? "il livello scelto" : "il primo livello") +
                       " con i tuoi dati. La musica si scrive nella sezione Audio e si sceglie qui.", ra.x, by, ra.width, Ui::TestoSpento(), Ui::TP());
    }
}

// Il gioco che si muove da solo nel riquadro, rifatto quando cambiano i dati o il livello scelto.
void Officina::AnteprimaArcade(Rectangle ra, float dt, int tappa) {
    DrawRectangleRec({ ra.x - 3, ra.y - 3, ra.width + 6, ra.height + 6 }, Ui::Bordo());
    if (tAnteprimaArcade > 0) tAnteprimaArcade -= dt;
    bool rifai = !anteprimaArcade || tappa != tappaAnteprima || (datiAnteprima != progetto.arcadeDati && tAnteprimaArcade <= 0);
    if (rifai) {
        if (!anteprimaArcade || anteprimaArcade->Nome() != progetto.arcade) {
            delete anteprimaArcade;
            anteprimaArcade = arcade::Crea(progetto.arcade).release();
        }
        if (!anteprimaArcade) return;
        arcade::gSilenzio = true;
        json d = json::parse(progetto.arcadeDati.empty() ? std::string("{}") : progetto.arcadeDati, nullptr, false);
        anteprimaArcade->Configura(d.is_object() ? d : json::object(), &progetto);
        anteprimaArcade->Prepara();
        anteprimaArcade->Nuova(777, 0);
        anteprimaArcade->IniziaDa(tappa);
        arcade::gSilenzio = false;
        datiAnteprima = progetto.arcadeDati;
        tappaAnteprima = tappa;
        tAnteprimaArcade = 0.6f;   // non si rifa' a ogni tasto premuto
        if (rtArcade.id == 0 || rtArcade.texture.width != anteprimaArcade->L()) {
            if (rtArcade.id) UnloadRenderTexture(rtArcade);
            rtArcade = LoadRenderTexture(anteprimaArcade->L(), anteprimaArcade->A());
        }
        SetTextureFilter(rtArcade.texture, anteprimaArcade->PixelArt() ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
    }
    if (!anteprimaArcade) return;
    arcade::gSilenzio = true;
    anteprimaArcade->Aggiorna(std::min(dt, 1 / 30.0f), anteprimaArcade->Automatico());
    if (anteprimaArcade->finito) { anteprimaArcade->Nuova(778, 0); anteprimaArcade->IniziaDa(tappa); }
    arcade::gSilenzio = false;
    BeginTextureMode(rtArcade);
    ClearBackground(BLACK);
    anteprimaArcade->Disegna();
    EndTextureMode();
    DrawTexturePro(rtArcade.texture, { 0, 0, (float)rtArcade.texture.width, -(float)rtArcade.texture.height }, ra, { 0, 0 }, 0, WHITE);
}

void Officina::AzioneArcade(const std::string& c) {
    if (!progetto.EArcade()) return;
    SincronizzaArcade();
    if (c == "a:prova") AvviaProvaArcade(catArcade == 0 ? selArcade : 0);
    else if (c == "a:tutto") {
        datiArcade = predefinitiArcade;
        ArcadeCambiato();
        Avvisa("Tutti i dati del gioco sono tornati come all'inizio (Ctrl+Z per annullare)");
    } else if (c == "a:predefinito") {
        const json& cat = schemaArcade[catArcade];
        std::string k = cat["chiave"];
        if (!predefinitiArcade.contains(k)) return;
        if (cat.value("lista", false)) {
            if (predefinitiArcade[k].is_array() && selArcade < (int)predefinitiArcade[k].size() && selArcade < (int)datiArcade[k].size())
                datiArcade[k][selArcade] = predefinitiArcade[k][selArcade];
            else return;
        } else datiArcade[k] = predefinitiArcade[k];
        ArcadeCambiato();
        Avvisa("Valori predefiniti rimessi (Ctrl+Z per annullare)");
    }
}

// --- Prova (F5) ---------------------------------------------------------------------------------------

void Officina::AvviaProvaArcade(int tappa) {
    delete provaArcade;
    provaArcade = arcade::Crea(progetto.arcade).release();
    if (!provaArcade) { Avvisa("Tipo di gioco sconosciuto: " + progetto.arcade); return; }
    json d = json::parse(progetto.arcadeDati.empty() ? std::string("{}") : progetto.arcadeDati, nullptr, false);
    provaArcade->Configura(d.is_object() ? d : json::object(), &progetto);
    provaArcade->Prepara();
    uint64_t seme = Mescola64((uint64_t)GetRandomValue(1, 1 << 30) * 2654435761ull ^ (uint64_t)(GetTime() * 1000));
    provaArcade->Nuova(seme ? seme : 1, 0);
    provaArcade->IniziaDa(tappa);
    tappaProvaArcade = tappa;
    if (rtArcade.id == 0 || rtArcade.texture.width != provaArcade->L()) {
        if (rtArcade.id) UnloadRenderTexture(rtArcade);
        rtArcade = LoadRenderTexture(provaArcade->L(), provaArcade->A());
    }
    SetTextureFilter(rtArcade.texture, provaArcade->PixelArt() ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
    tProva = 0;
    Ui::TogliFuoco();
    gAudio.Suona(Effetto::Conferma);
}

void Officina::FrameProvaArcade(float dt) {
    tProva += dt;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    int dim = std::max(12, (int)(sh / 60));
    float hb = dim * 1.7f;
    int L = provaArcade->L(), A = provaArcade->A();
    float s = std::min(sw / L, (sh - hb) / A);
    Rectangle area = { (sw - L * s) / 2, (sh - hb - A * s) / 2, L * s, A * s };
    if (tProva > 0.2f && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F5))) {
        delete provaArcade;
        provaArcade = nullptr;
        arcade::FermaMusica();
        gAudio.ScegliBrano(0);
        tappaAnteprima = -1;   // l'anteprima riparte (le texture potrebbero essere state ridipinte)
        return;
    }
    if (IsKeyPressed(KEY_R) || (provaArcade->finito && IsKeyPressed(KEY_ENTER))) {
        provaArcade->Nuova((uint64_t)GetRandomValue(1, 1 << 30), 0);
        provaArcade->IniziaDa(tappaProvaArcade);
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) { tappaProvaArcade = std::min(provaArcade->Tappe() - 1, tappaProvaArcade + 1); provaArcade->IniziaDa(tappaProvaArcade); }
    if (IsKeyPressed(KEY_PAGE_UP)) { tappaProvaArcade = std::max(0, tappaProvaArcade - 1); provaArcade->IniziaDa(tappaProvaArcade); }
    provaArcade->Aggiorna(std::min(dt, 1 / 20.0f), arcade::LeggiPad(area, L, A));
    BeginTextureMode(rtArcade);
    ClearBackground(BLACK);
    provaArcade->Disegna();
    EndTextureMode();
    ClearBackground(BLACK);
    DrawTexturePro(rtArcade.texture, { 0, 0, (float)L, -(float)A }, area, { 0, 0 }, 0, WHITE);
    DrawRectangle(0, (int)(sh - hb), (int)sw, (int)hb, Hex(0x0b0e22, 230));
    std::string t = "PROVA  " + provaArcade->Tappa() + "    Esc o F5: torna allo studio   R: ricomincia   Pag Su / Pag Giu: livello precedente / successivo   F7 musica   F6 effetti";
    DrawText(t.c_str(), 12, (int)(sh - dim * 1.35f), dim, Hex(0xc8d0f0));
    postoAudio = { sw - hb - 4, sh - hb + 2, hb - 4, hb - 4 };
}
