// Il motore con la Mente e il Destino della partita.
//   - Destino: dal seme della partita nascono l'evento speciale, le relazioni fra specie, le variabili
//     casuali e, per ogni mappa, carattere delle creature, veterani, oggetti ricollocati, meteo.
//     Tutto e' deterministico: stesso seme = stessa partita (e il caricamento ricostruisce uguale).
//   - Mente: a ogni pensiero (10 al secondo) la creatura guarda intorno (Contesto), aggiorna le emozioni
//     e sceglie un'azione (mente.cpp); qui l'azione diventa movimento.
//   - Progressi di scena: cosa e' stato raccolto, aperto, acceso in ogni mappa lasciata.
#include "gioco.h"
#include "audio.h"
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace {

bool Sovrappone(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }

bool StessoNome(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

bool Collezionabile(TipoOggetto t) { return t == TipoOggetto::Moneta || t == TipoOggetto::Gemma || t == TipoOggetto::Cuore; }

const Sapere& SapereVuoto() { static Sapere s; return s; }

} // namespace

// --- Destino della partita ----------------------------------------------------------------------------

void Gioco::PreparaDestino() {
    entita.clear(); oggetti.clear();
    mappeStato.clear(); relazioni.clear(); fiduciaIniziale.clear();
    contatoreCreati = 0; pieta = 0;
    eventoPartita.clear();
    vistaMolt = velocitaMolt = puntiPartitaMolt = 1;
    rng.Semina(semePartita ^ 0x5DEECE66Dull);
    const VarietaPartita& v = lib.varieta;
    if (semePartita == 0 || v.livello <= 0) return;
    Generatore r;
    r.Semina(semePartita);
    int L = v.livello;
    // Un evento speciale (a volte nessuno: anche la normalita' e' una variazione).
    float pEvento = r.Float();
    if (v.eventi && pEvento < 0.15f + 0.15f * L) {
        std::vector<const EventoPartita*> ammessi;
        for (auto& ev : EventiPartita())
            if (std::find(v.eventiVietati.begin(), v.eventiVietati.end(), ev.chiave) == v.eventiVietati.end()) ammessi.push_back(&ev);
        if (!ammessi.empty()) eventoPartita = ammessi[r.Intero(0, (int)ammessi.size() - 1)]->chiave;
    }
    if (eventoPartita == "luna_rossa") puntiPartitaMolt = 1.5f;
    else if (eventoPartita == "nebbia") vistaMolt = 0.65f;
    else if (eventoPartita == "festa") puntiPartitaMolt = 1.2f;
    else if (eventoPartita == "calma") velocitaMolt = 0.85f;
    // Relazioni fra le specie nemiche: rivali (si combattono) o alleate (rispondono ai richiami).
    std::vector<int> specie;
    for (size_t i = 0; i < lib.personaggi.size(); i++) {
        TipoComportamento t = lib.personaggi[i].comportamento.tipo;
        if (t != TipoComportamento::Giocatore && t != TipoComportamento::Parla && !StessoNome(lib.personaggi[i].nome, lib.eroe)) specie.push_back((int)i);
    }
    for (size_t a = 0; a < specie.size(); a++)
        for (size_t b = a + 1; b < specie.size(); b++) {
            float x = r.Float();
            if (!v.relazioni) continue;
            if (x < 0.05f * L) relazioni[{ specie[a], specie[b] }] = -1;
            else if (x < 0.05f * L + 0.3f) relazioni[{ specie[a], specie[b] }] = 1;
        }
    // Fiducia iniziale degli abitanti.
    for (size_t i = 0; i < lib.personaggi.size(); i++)
        if (lib.personaggi[i].comportamento.tipo == TipoComportamento::Parla) {
            float f = r.Float(-0.2f, 0.4f);
            if (!v.relazioni) f = 0.1f;
            if (eventoPartita == "festa") f += 0.4f;
            fiduciaIniziale[(int)i] = Limita(f, -1, 1);
        }
    // Variabili casuali della Logica (prima che la mappa parta: le regole "inizio" le vedono gia').
    for (auto& vc : v.variabiliCasuali) {
        float x = r.Float(vc.minimo, vc.massimo);
        if (vc.intero) x = std::floor(x + 0.5f);
        lib.variabili[vc.nome] = x;
    }
}

void Gioco::DestinoMappa(int indice) {
    const VarietaPartita& v = lib.varieta;
    bool varia = semePartita != 0 && v.livello > 0;
    Generatore r;
    r.Semina(Mescola64(semePartita ^ (0xA24BAED4963EE407ull * (uint64_t)(indice + 1))));
    int L = v.livello;
    int ostili = 0;
    for (auto& e : entita)
        if (e.personaggio && lib.personaggi[e.def].comportamento.tipo != TipoComportamento::Parla && lib.personaggi[e.def].comportamento.tipo != TipoComportamento::Boss) ostili++;
    int maxSpeciali = std::max(1, ostili / 5), speciali = 0;
    for (auto& e : entita) {
        if (!e.personaggio || !e.mente.attiva) continue;
        const ProfiloComportamento& pr = profiliDef[e.def];
        TipoComportamento t = lib.personaggi[e.def].comportamento.tipo;
        bool boss = t == TipoComportamento::Boss, abitante = t == TipoComportamento::Parla;
        Carattere c = pr.car;
        if (varia && v.carattere) c = CarattereDellaPartita(pr.car, pr.variabilita * L / 2.0f, r);
        float x = varia ? r.Float() : 1;
        if (varia && v.veterani && !boss && !abitante && speciali < maxSpeciali) {
            if (x < 0.05f * L) {
                e.veterano = 1; speciali++;
                e.energia += std::max(1, e.energia / 2); e.energiaMax = e.energia;
                e.puntiMolt = 2; e.tinta = Hex(0xffd890);
                c.coraggio = std::min(1.0f, c.coraggio + 0.2f);
            } else if (x < 0.1f * L) {
                e.veterano = -1; speciali++;
                e.tinta = Hex(0xc8dcff);
                c.coraggio *= 0.5f;
            }
        }
        if (eventoPartita == "luna_rossa") c.aggressivita = std::min(1.0f, c.aggressivita + 0.25f);
        if (eventoPartita == "calma") c.aggressivita = std::max(0.0f, c.aggressivita - 0.15f);
        if (eventoPartita == "caccia") c.socievolezza = std::min(1.0f, c.socievolezza + 0.3f);
        e.mente.car = c;
    }
    if (!varia) return;
    // Oggetti da raccogliere che si scambiano di posto: solo fra posizioni gia' raggiungibili.
    std::vector<int> racc;
    for (size_t i = 0; i < entita.size(); i++) {
        const Entita& e = entita[i];
        if (e.personaggio || e.uid >= 1000000) continue;
        const OggettoDef& d = lib.oggetti[e.def];
        if ((d.funzione == FunzioneOggetto::Raccogli || d.funzione == FunzioneOggetto::Cura) && !d.gravita) racc.push_back((int)i);
    }
    if (v.ricolloca && racc.size() > 1) {
        int scambi = (int)(racc.size() * 0.12f * L + 0.5f);
        for (int k = 0; k < scambi; k++) {
            Entita& a = entita[racc[r.Intero(0, (int)racc.size() - 1)]];
            Entita& b = entita[racc[r.Intero(0, (int)racc.size() - 1)]];
            std::swap(a.pos, b.pos); std::swap(a.origine, b.origine);
        }
    }
    std::vector<int> base;
    for (size_t i = 0; i < oggetti.size(); i++) if (Collezionabile(oggetti[i].tipo)) base.push_back((int)i);
    if (v.ricolloca && base.size() > 1) {
        int scambi = (int)(base.size() * 0.12f * L + 0.5f);
        for (int k = 0; k < scambi; k++) {
            Oggetto& a = oggetti[base[r.Intero(0, (int)base.size() - 1)]];
            Oggetto& b = oggetti[base[r.Intero(0, (int)base.size() - 1)]];
            std::swap(a.pos, b.pos); std::swap(a.origine, b.origine);
        }
    }
    // Il tesoro nascosto: un oggetto da raccogliere vale cinque volte tanto.
    if (eventoPartita == "tesoro" && !racc.empty()) {
        Entita& t = entita[racc[r.Intero(0, (int)racc.size() - 1)]];
        t.valoreMolt = 5; t.tinta = Hex(0xffe27a);
    }
    // Meteo e ora del giorno, solo all'aperto e dove il designer non ha gia' deciso l'atmosfera.
    if (v.meteo && Laterale() && cicloGiorno && pioggia == 0 && buio < 0.05f && !acqua) {
        if (r.Probabile(0.06f * L)) pioggia = r.Probabile(0.75f) ? 1 : 2;
        sfondo.ora = std::fmod(sfondo.ora + r.Float(-0.08f, 0.08f) * L + 1.0f, 1.0f);
    }
}

// --- Progressi di scena -------------------------------------------------------------------------------

void Gioco::RicordaMappa() {
    if (entita.empty() && oggetti.empty()) return;
    StatoMappa& sm = mappeStato[mappaAttuale];
    sm = StatoMappa{};
    for (const Entita& e : entita) {
        if (e.uid < 0 || e.uid >= 1000000) continue;
        if (e.personaggio) {
            // i nemici tornano, ma un boss sconfitto resta sconfitto
            if (lib.personaggi[e.def].comportamento.tipo == TipoComportamento::Boss && (!e.attivo || e.morte > 0)) sm.tolti.insert(e.uid);
            continue;
        }
        const OggettoDef& d = lib.oggetti[e.def];
        if (!e.attivo && !(d.indole.attiva && d.indole.ricarica > 0)) sm.tolti.insert(e.uid);
        if (e.aperta) sm.aperti.insert(e.uid);
        if (e.acceso) sm.accesi.insert(e.uid);
    }
    size_t iniziali = livello.Oggetti().size();
    for (size_t i = 0; i < oggetti.size() && i < iniziali; i++)
        if (!oggetti[i].attivo && Collezionabile(oggetti[i].tipo)) sm.base.insert((int)i);
}

void Gioco::ApplicaStatoMappa(int indice) {
    auto it = mappeStato.find(indice);
    if (it == mappeStato.end()) return;
    const StatoMappa& sm = it->second;
    for (Entita& e : entita) {
        if (sm.tolti.count(e.uid)) e.attivo = false;
        if (sm.aperti.count(e.uid)) { e.aperta = true; e.anim = "aperta"; }
        if (sm.accesi.count(e.uid)) { e.acceso = true; e.anim = "accesa"; }
    }
    for (int i : sm.base) if (i >= 0 && i < (int)oggetti.size()) oggetti[i].attivo = false;
}

// --- Mente in azione -------------------------------------------------------------------------------------

int Gioco::Relazione(int a, int b) const {
    if (a == b) return 1;
    auto it = relazioni.find({ std::min(a, b), std::max(a, b) });
    return it == relazioni.end() ? 0 : it->second;
}

bool Gioco::Ostile(const Entita& e) const {
    if (!e.attivo || !e.personaggio || e.morte > 0) return false;
    TipoComportamento t = lib.personaggi[e.def].comportamento.tipo;
    if (t == TipoComportamento::Parla || t == TipoComportamento::Giocatore) return false;
    return !(e.mente.attiva && e.mente.azione == AzioneMente::Amico);
}

Contesto Gioco::ContestoDi(const Entita& e, int i, float dist, float vista, bool vede) {
    const Comportamento& c = lib.personaggi[e.def].comportamento;
    const Mente& m = e.mente;
    Contesto k;
    k.tempo = tempo;
    k.distanza = dist / std::max(1.0f, vista);
    k.vede = vede ? 1.0f : 0;
    k.energia = Limita((float)e.energia / std::max(1, e.energiaMax), 0, 1);
    k.energiaEroe = Limita((float)energia / std::max(1, energiaMax), 0, 1);
    k.abitante = c.tipo == TipoComportamento::Parla;
    k.boss = c.tipo == TipoComportamento::Boss;
    k.puoSparare = (c.tipo == TipoComportamento::Spara || c.tipo == TipoComportamento::Boss || !c.proiettile.empty()) ? 1.0f : 0;
    k.lontanoCasa = Limita(std::sqrt((e.pos.x - e.origine.x) * (e.pos.x - e.origine.x) + (e.pos.y - e.origine.y) * (e.pos.y - e.origine.y)) / 200, 0, 1);
    k.memoria = (m.ultimoVisto.x >= 0 && m.tVisto < 15) ? 1.0f : 0;
    k.chiuso = e.bloccato ? 1.0f : 0;
    k.pieta = Limita(pieta * 2, 0, 1);
    Entita& me = entita[i];
    me.bersaglioCura = me.bersaglioRivale = -1;
    float migliorCura = 260 * 260, migliorRivale = vista * vista;
    bool amico = m.azione == AzioneMente::Amico;
    int alleati = 0;
    for (size_t j = 0; j < entita.size(); j++) {
        if ((int)j == i) continue;
        const Entita& o = entita[j];
        if (!o.attivo || o.morte > 0) continue;
        float ddx = o.pos.x - e.pos.x, ddy = o.pos.y - e.pos.y, d2 = ddx * ddx + ddy * ddy;
        if (o.personaggio) {
            if (!Ostile(o) && !amico) continue;
            if (!k.abitante && Ostile(o) && d2 < 200 * 200 && Relazione(e.def, o.def) == 1) alleati++;
            bool rivale = amico ? Ostile(o) : (Relazione(e.def, o.def) == -1);
            if (rivale && d2 < migliorRivale) { migliorRivale = d2; me.bersaglioRivale = (int)j; }
        } else if (k.energia < 1) {
            const OggettoDef& d = lib.oggetti[o.def];
            if (d.funzione == FunzioneOggetto::Cura && !o.rotto && d2 < migliorCura) { migliorCura = d2; me.bersaglioCura = (int)j; }
        }
    }
    k.alleati = (float)std::min(3, alleati);
    k.cura = me.bersaglioCura >= 0 ? 1.0f : 0;
    k.rivale = me.bersaglioRivale >= 0 ? 1.0f : 0;
    Rectangle b = BoxEntita(e);
    if (Laterale() && eroe.vel.y > 30 && std::fabs(eroe.pos.x - e.pos.x) < b.width * 0.5f + 16 && eroe.pos.y < b.y + 4 && eroe.pos.y > b.y - 80)
        k.eroeSopra = 1;
    for (const Proiettile& q : proiettili) {
        if (!q.amico) continue;
        float ddx = e.pos.x - q.pos.x, ddy = (e.pos.y - b.height / 2) - q.pos.y;
        if (ddx * ddx + ddy * ddy < 80 * 80 && ddx * q.vel.x + ddy * q.vel.y > 0) { k.proiettile = 1; break; }
    }
    return k;
}

void Gioco::ChiamaAiuto(int i) {
    Entita& e = entita[i];
    e.mente.tRichiamo = 8;
    e.tGrido = 1.2f;
    for (size_t j = 0; j < entita.size(); j++) {
        if ((int)j == i) continue;
        Entita& o = entita[j];
        if (!o.mente.attiva || !Ostile(o) || Relazione(e.def, o.def) != 1) continue;
        float ddx = o.pos.x - e.pos.x, ddy = o.pos.y - e.pos.y;
        if (ddx * ddx + ddy * ddy > 300 * 300) continue;
        // chi risponde corre a cercare l'eroe, ma non richiama a sua volta subito (niente catene infinite)
        o.mente.allerta = 1;
        o.mente.ultimoVisto = { eroe.pos.x, eroe.pos.y };
        o.mente.tVisto = 0;
        o.mente.minaccia = std::min(1.0f, o.mente.minaccia + 0.2f);
        o.mente.tRichiamo = std::max(o.mente.tRichiamo, 5.0f);
        o.mente.Ricorda(TipoRicordo::Richiamo, tempo, eroe.pos, 0.6f);
    }
}

void Gioco::DannoFraCreature(int j, int danno) {
    Entita& t = entita[j];
    if (!t.attivo || t.morte > 0 || t.lampeggio > 0) return;
    t.energia -= danno;
    t.lampeggio = 0.4f;
    if (t.mente.attiva) t.mente.minaccia = std::min(1.0f, t.mente.minaccia + 0.2f);
    Rectangle b = BoxEntita(t);
    particelle.Scintille({ b.x + b.width / 2, b.y + b.height / 2 }, Hex(0xffa060));
    if (t.energia <= 0) {
        t.morte = 0.01f;
        particelle.Esplosione({ b.x + b.width / 2, b.y + b.height / 2 }, Hex(0xffd23f));
        gAudio.Suona(Effetto::Esplosione, 0.4f);
        std::string nome = t.nome;
        Evento("sconfitto", nome, "", j);
    }
}

void Gioco::ReazioneColpo(int i) {
    Entita& e = entita[i];
    if (!e.personaggio || !e.mente.attiva) return;
    Mente& m = e.mente;
    bool abitante = lib.personaggi[e.def].comportamento.tipo == TipoComportamento::Parla;
    m.Ricorda(TipoRicordo::ColpitoDaEroe, tempo, eroe.pos, 1);
    m.minaccia = std::min(1.0f, m.minaccia + 0.35f);
    m.fiducia = std::max(-1.0f, m.fiducia - (abitante ? 0.6f : 0.45f));
    m.tColpoEroe = 0;
    m.allerta = 1;
    m.ultimoVisto = { eroe.pos.x, eroe.pos.y };
    m.tVisto = 0;
    if (m.azione == AzioneMente::Amico) m.tAzione = 99;   // tradito: ci ripensa subito
}

void Gioco::ImparaMorte(int i) {
    Entita& e = entita[i];
    if (!e.personaggio) return;
    const PersonaggioDef& p = lib.personaggi[e.def];
    if (e.mente.attiva && e.def < (int)profiliDef.size() && profiliDef[e.def].imparaSpecie && causaColpo > 0) {
        const ProfiloComportamento& pr = profiliDef[e.def];
        Sapere& s = sapere[p.nome];
        s.Impara(s.schiacciato, causaColpo == 1 ? 1.0f : 0.0f, pr.apprendimento);
        s.Impara(s.colpito, causaColpo == 2 ? 1.0f : 0.0f, pr.apprendimento);
        s.esperienze++;
    }
    bool ostile = Ostile(e) || e.morte > 0;
    if (lib.personaggi[e.def].comportamento.tipo == TipoComportamento::Parla) ostile = false;
    for (size_t j = 0; j < entita.size(); j++) {
        if ((int)j == i) continue;
        Entita& o = entita[j];
        if (!o.attivo || !o.personaggio || !o.mente.attiva || o.morte > 0) continue;
        float ddx = o.pos.x - e.pos.x, ddy = o.pos.y - e.pos.y;
        if (ddx * ddx + ddy * ddy > 220 * 220) continue;
        bool abitante = lib.personaggi[o.def].comportamento.tipo == TipoComportamento::Parla;
        if (abitante && ostile) {
            // l'eroe ha sconfitto un nemico vicino a un abitante: guadagna fiducia
            o.mente.fiducia = std::min(1.0f, o.mente.fiducia + 0.25f);
            o.mente.Ricorda(TipoRicordo::Aiutato, tempo, eroe.pos, 0.8f);
        } else if (o.def == e.def) {
            o.mente.minaccia = std::min(1.0f, o.mente.minaccia + 0.25f);
            o.mente.allerta = 1;
            o.mente.Ricorda(TipoRicordo::CompagnoCaduto, tempo, e.pos, 0.8f);
        }
    }
}

void Gioco::AggiornaConMente(Entita& e, int i, float dt, Rectangle eroeBox) {
    const PersonaggioDef& p = lib.personaggi[e.def];
    const Comportamento& c = p.comportamento;
    const ProfiloComportamento& pr = profiliDef[e.def];
    Mente& m = e.mente;
    Rectangle b0 = BoxEntita(e);
    float w = b0.width, h = b0.height;
    float v = c.velocita * e.velMolt * velocitaMolt;
    float dx = eroe.pos.x - e.pos.x, dy = (eroe.pos.y - 10) - (e.pos.y - h / 2);
    float dist = std::sqrt(dx * dx + dy * dy);
    float vista = std::max(24.0f, c.vista * vistaMolt * (buio > 0.3f ? 0.8f : 1.0f));
    bool vede = dist < vista && !vinto && !finito;
    bool gravita = c.gravita && Laterale();
    bool abitante = c.tipo == TipoComportamento::Parla, boss = c.tipo == TipoComportamento::Boss;
    if (e.tGrido > 0) e.tGrido -= dt;

    Contesto k = ContestoDi(e, i, dist, vista, vede);
    AggiornaEmozioni(m, pr, k, dt);
    if (vede) { m.ultimoVisto = { eroe.pos.x, eroe.pos.y }; m.tVisto = 0; m.Ricorda(TipoRicordo::VistoEroe, tempo, eroe.pos, 0.3f); }

    std::string eventoDopo;   // gli eventi delle regole si lanciano alla fine (possono creare entita')
    m.tPensa -= dt;
    if (m.tPensa <= 0) {
        m.tPensa = std::max(0.02f, m.tPensa + 0.1f);
        AzioneMente prima = m.azione;
        Decidi(m, pr, pr.imparaSpecie ? sapere[p.nome] : SapereVuoto(), k, rng);
        if (m.azione != prima) {
            e.bloccato = false;
            switch (m.azione) {
                case AzioneMente::ChiamaAiuto: ChiamaAiuto(i); eventoDopo = "chiama_aiuto"; break;
                case AzioneMente::Fuggi: eventoDopo = "fugge"; break;
                case AzioneMente::Schiva: {
                    m.tSchiva = 1.2f + (1 - m.car.prudenza);
                    float via = dx > 0 ? -1.0f : 1.0f;
                    if (gravita) { if (e.aTerra) { e.vel.y = -std::max(c.salto, 220.0f); e.vel.x = via * std::max(v, 40.0f) * 1.6f; } }
                    else e.vel = { via * std::max(v, 40.0f) * 2, -std::max(v, 40.0f) };
                    break;
                }
                case AzioneMente::Amico:
                    if (!e.amicoAnnunciato) {
                        e.amicoAnnunciato = true;
                        particelle.Scintille({ b0.x + b0.width / 2, b0.y }, Hex(0xff8ac0));
                        eventoDopo = "diventa_amico";
                    }
                    break;
                case AzioneMente::Attacca: if (k.puoSparare <= 0) m.tScatto = 0.6f; break;
                default: break;
            }
        }
    }

    // --- l'azione diventa movimento ---
    auto Cammina = [&](float verso, float vel, bool evitaBurroni) {
        if (std::fabs(verso) < 0.01f || vel <= 0) { e.vel.x = 0; return; }
        e.dir = verso > 0 ? 1 : -1;
        int davanti = (int)std::floor((e.pos.x + e.dir * (w / 2 + 2)) / TILE);
        int sotto = (int)std::floor((e.pos.y + 2) / TILE);
        bool vuoto = e.aTerra && !livello.Solido(davanti, sotto) && !livello.Piattaforma(davanti, sotto);
        bool muro = livello.Solido(davanti, (int)std::floor((e.pos.y - 4) / TILE));
        if (muro && e.aTerra && c.salto > 0) { e.vel.y = -c.salto; muro = false; }
        if ((vuoto && evitaBurroni) || muro) { e.vel.x = 0; e.bloccato = true; return; }
        e.bloccato = false;
        e.vel.x = e.dir * vel;
    };
    auto Vola = [&](Vector2 t, float vel) {
        float ddx = t.x - e.pos.x, ddy = t.y - e.pos.y, l = std::sqrt(ddx * ddx + ddy * ddy);
        if (l < 2 || vel <= 0) { e.vel = { 0, 0 }; return; }
        e.vel = { ddx / l * vel, ddy / l * vel };
        if (std::fabs(ddx) > 1) e.dir = ddx > 0 ? 1 : -1;
    };
    auto Verso = [&](Vector2 t, float vel, bool evita) {
        if (gravita) Cammina(std::fabs(t.x - e.pos.x) > 4 ? t.x - e.pos.x : 0, vel, evita);
        else Vola(t, vel);
    };
    auto Fermo = [&]() { e.vel.x = 0; if (!gravita) e.vel.y = 0; };
    Vector2 eroeP = { eroe.pos.x, eroe.pos.y - 8 };

    if (boss && m.azione != AzioneMente::Schiva) MovimentoClassico(e, i, dt, TipoComportamento::Boss);
    else switch (m.azione) {
        case AzioneMente::Pattuglia:
            if (abitante) { Fermo(); break; }
            MovimentoClassico(e, i, dt, c.tipo == TipoComportamento::Vola ? TipoComportamento::Vola :
                                        c.tipo == TipoComportamento::Spara ? TipoComportamento::Spara :
                                        c.tipo == TipoComportamento::Fermo ? TipoComportamento::Fermo : TipoComportamento::Pattuglia);
            break;
        case AzioneMente::Insegui:
            Verso(eroeP, v * (1 + 0.3f * m.car.aggressivita), eroe.pos.y <= e.pos.y + 4);
            if (gravita && e.aTerra && c.salto > 0 && dy < -24 && std::fabs(dx) < 90) e.vel.y = -c.salto;
            break;
        case AzioneMente::Attacca:
            if (k.puoSparare > 0) {
                e.dir = dx > 0 ? 1 : -1;
                Fermo();
                e.timer -= dt;
                if (vede && e.timer <= 0) {
                    e.timer = c.attaccoOgni * (1.25f - 0.5f * m.car.aggressivita) * (1 + pieta);
                    SparaDa(i, "", 1);
                }
            } else if (m.tScatto > 0) Verso(eroeP, v * 2, false);   // scatto improvviso
            else Fermo();
            break;
        case AzioneMente::Aggira: {
            float D = vista * 0.55f;
            bool vulnerabile = (eroe.dir > 0) == (dx > 0) || !eroe.aTerra || eroe.lampeggio > 0;
            if (m.tScatto <= 0 && vulnerabile && dist < D * 1.3f && m.tAzione > 0.8f) m.tScatto = 0.9f;
            if (m.tScatto > 0) Verso(eroeP, v * 1.7f, false);
            else if (dist < D * 0.8f) Verso({ e.pos.x - dx, e.pos.y - dy }, v * 0.9f, true);
            else if (dist > D * 1.2f) Verso(eroeP, v * 0.8f, true);
            else { Fermo(); e.dir = dx > 0 ? 1 : -1; }
            break;
        }
        case AzioneMente::Cerca:
            if (m.ultimoVisto.x < 0) { Fermo(); break; }
            if (std::fabs(m.ultimoVisto.x - e.pos.x) > 10 || (!gravita && std::fabs(m.ultimoVisto.y - e.pos.y) > 10)) Verso(m.ultimoVisto, v * 0.8f, true);
            else { Fermo(); if (std::fmod(m.tAzione, 1.6f) < dt) e.dir = -e.dir; }   // si guarda intorno
            if (m.tVisto > 6 + 6 * m.car.curiosita) m.ultimoVisto = { -1, -1 };
            if (e.bloccato && std::fmod(m.tAzione, 1.0f) < dt) m.ultimoVisto = { -1, -1 };
            break;
        case AzioneMente::Fuggi:
            Verso({ e.pos.x - dx * 4, e.pos.y - (gravita ? 0 : dy * 4) }, v * 1.15f, true);
            if (gravita && e.bloccato && e.aTerra && c.salto > 0) e.vel.y = -c.salto;
            break;
        case AzioneMente::Curati:
            if (e.bersaglioCura >= 0 && e.bersaglioCura < (int)entita.size() && entita[e.bersaglioCura].attivo) {
                Entita& cura = entita[e.bersaglioCura];
                Verso(cura.pos, v * 1.1f, true);
                if (Sovrappone(BoxEntita(e), BoxEntita(cura))) {
                    const OggettoDef& d = lib.oggetti[cura.def];
                    e.energia = std::min(e.energiaMax, e.energia + std::max(1, d.valore));
                    cura.attivo = false;
                    if (d.indole.attiva && d.indole.ricarica > 0) cura.tRicarica = d.indole.ricarica;
                    Rectangle cb = BoxEntita(cura);
                    particelle.Scintille({ cb.x + cb.width / 2, cb.y + cb.height / 2 }, Hex(0xff5c8a));
                    m.paura *= 0.5f;
                    m.tAzione = 99;
                }
            } else Fermo();
            break;
        case AzioneMente::ChiamaAiuto:
            Fermo();
            e.dir = dx > 0 ? 1 : -1;
            if (m.tAzione > 0.5f) m.tAzione = 99;   // gridato: si torna a decidere
            break;
        case AzioneMente::Schiva:
            if (gravita) { if (e.aTerra) e.vel.x *= 1 - std::min(1.0f, dt * 6); }
            else e.vel = { e.vel.x * (1 - std::min(1.0f, dt * 3)), e.vel.y * (1 - std::min(1.0f, dt * 3)) };
            break;
        case AzioneMente::Guardia:
            if (std::fabs(e.pos.x - e.origine.x) > 6 || (!gravita && std::fabs(e.pos.y - e.origine.y) > 6)) Verso(e.origine, v * 0.7f, true);
            else { Fermo(); if (vede) e.dir = dx > 0 ? 1 : -1; }
            break;
        case AzioneMente::Riposa:
            Fermo();
            break;
        case AzioneMente::Amico: {
            Vector2 t = { eroe.pos.x - eroe.dir * 28, gravita ? eroe.pos.y : eroe.pos.y - 24 };
            float d = std::fabs(t.x - e.pos.x) + (gravita ? 0 : std::fabs(t.y - e.pos.y));
            if (e.bersaglioRivale >= 0 && e.bersaglioRivale < (int)entita.size()) {
                Entita& r = entita[e.bersaglioRivale];   // aiuta l'eroe contro i nemici vicini
                Verso(r.pos, v * 1.2f, true);
                if (e.tContatto <= 0 && Sovrappone(BoxEntita(e), BoxEntita(r))) {
                    e.tContatto = 1;
                    int j = e.bersaglioRivale;
                    bool primaViva = r.morte <= 0;
                    DannoFraCreature(j, 1);
                    if (primaViva && j < (int)entita.size() && entita[j].morte > 0)
                        punteggio += (int)(lib.personaggi[entita[j].def].comportamento.punti * 0.5f);
                    return FinePersonaggio(entita[i], i, dt, eroeBox);
                }
            } else if (d > 20) Verso(t, std::max(v, 50.0f) * 1.2f, false);
            else { Fermo(); e.dir = eroe.dir; }
            if (gravita && e.aTerra && c.salto > 0 && eroe.pos.y < e.pos.y - 30 && std::fabs(dx) < 60) e.vel.y = -c.salto;
            break;
        }
        case AzioneMente::Rivale:
            if (e.bersaglioRivale >= 0 && e.bersaglioRivale < (int)entita.size()) {
                Entita& r = entita[e.bersaglioRivale];
                Verso(r.pos, v * 1.1f, true);
                if (e.tContatto <= 0 && Sovrappone(BoxEntita(e), BoxEntita(r))) {
                    e.tContatto = 1;
                    DannoFraCreature(e.bersaglioRivale, 1);
                    return FinePersonaggio(entita[i], i, dt, eroeBox);
                }
            } else Fermo();
            break;
        default: Fermo(); break;
    }
    // Gli abitanti parlano quando l'eroe e' vicino (se non scappano).
    if (abitante) {
        if (dist < 36 && m.azione != AzioneMente::Fuggi && !FraseUmore(m, pr, c.dialogo).empty()) {
            dialogo = i; e.dir = dx > 0 ? 1 : -1;
            if (m.azione != AzioneMente::Amico) Fermo();
        }
    }
    std::string nome = e.nome;
    FinePersonaggio(e, i, dt, eroeBox);
    if (!eventoDopo.empty()) Evento(eventoDopo, nome, "", i);
}

// --- Indole degli oggetti --------------------------------------------------------------------------------

void Gioco::AggiornaIndole(Entita& e, int i, float dt) {
    (void)i;
    const OggettoDef& d = lib.oggetti[e.def];
    const IndoleOggetto& ind = d.indole;
    bool rotto = ind.usura > 0 && e.carica <= 0.001f;
    if (rotto && ind.ricarica > 0) {
        e.tRicarica -= dt;
        if (e.tRicarica <= 0) {
            e.carica = 1; e.usi = 0; e.forza = 1; rotto = false;
            Rectangle b = BoxEntita(e);
            particelle.Scintille({ b.x + b.width / 2, b.y + b.height / 2 }, Hex(0xb8f0ff));
        }
    }
    bool dorme = false;
    if (d.funzione == FunzioneOggetto::Pericolo) {
        if (ind.notturno && !(sfondo.Notte() > 0.4f || buio > 0.3f)) dorme = true;
        if (ind.risveglio > 0) {
            float ddx = eroe.pos.x - e.pos.x, ddy = eroe.pos.y - e.pos.y;
            if (ddx * ddx + ddy * ddy < 56 * 56) e.tSveglio += dt;
            else e.tSveglio = std::max(0.0f, e.tSveglio - dt * 0.5f);
            if (e.tSveglio < ind.risveglio) dorme = true;
        }
    }
    e.rotto = rotto || dorme;
}

// --- Pensieri visibili (F9) e prove -------------------------------------------------------------------------

void Gioco::DisegnaPensieri(float cx, float cy, bool barra) const {
    // Le scritte hanno sempre la stessa grandezza sullo schermo, qualunque sia lo zoom del gioco.
    // Le etichette sulle creature si disegnano nel mondo (con lo zoom), la barra in basso sopra all'HUD.
    extern float gScalaHD;
    float f = std::max(1.0f, gScalaHD * (barra ? 1.0f : zoom));
    float fs = std::max(3.0f, 13.0f / f), sp = fs / 10;
    auto Etichetta = [&](const std::string& t, float x, float y, Color c) {
        Vector2 m = MeasureTextEx(GetFontDefault(), t.c_str(), fs, sp);
        DrawRectangleRec({ x - m.x / 2 - fs * 0.3f, y - fs * 0.1f, m.x + fs * 0.6f, fs * 1.15f }, Hex(0x000000, 170));
        DrawTextEx(GetFontDefault(), t.c_str(), { x - m.x / 2, y }, fs, sp, c);
    };
    for (const Entita& e : entita) {
        if (barra) break;
        if (!e.attivo || e.morte > 0) continue;
        Rectangle b = BoxEntita(e);
        float x = b.x + b.width / 2 - cx, y = b.y - cy;
        if (x < -60 || x > VW + 60 || y < -40 || y > VH + 40) continue;
        if (e.personaggio && e.mente.attiva) {
            if (e.tGrido > 0) {   // "!" del richiamo
                float a = Limita(e.tGrido, 0, 1);
                DrawCircleV({ x, y - 8 }, 5, ConAlfa(Hex(0xffd23f), a));
                DrawRectangle((int)x - 1, (int)y - 12, 2, 5, ConAlfa(Hex(0x402000), a));
                DrawRectangle((int)x - 1, (int)y - 6, 2, 2, ConAlfa(Hex(0x402000), a));
            }
            if (e.mente.azione == AzioneMente::Amico) {   // un cuoricino: e' dalla parte dell'eroe
                Color c = Hex(0xff6aa8);
                DrawCircleV({ x - 1.5f, y - 6 }, 2, c); DrawCircleV({ x + 1.5f, y - 6 }, 2, c);
                DrawTriangle({ x - 3.4f, y - 5.4f }, { x, y - 1.5f }, { x + 3.4f, y - 5.4f }, c);
            }
            if (mostraPensieri) Etichetta(Pensiero(e.mente), x, y - 14 - fs, Hex(0xb8ffcf));
        } else if (!e.personaggio && mostraPensieri && e.def >= 0 && e.def < (int)lib.oggetti.size() && lib.oggetti[e.def].indole.attiva) {
            Etichetta(TextFormat("carica %.0f%%%s", e.carica * 100, e.rotto ? " (fermo)" : ""), x, y - 4 - fs, Hex(0xffe0a0));
        }
    }
    if (barra && mostraPensieri) {
        std::string t = TextFormat("seme %s   evento: %s   pieta' %.2f", Esadecimale64(semePartita).c_str(),
                                   eventoPartita.empty() ? "nessuno" : NomeEventoPartita().c_str(), pieta);
        Vector2 m = MeasureTextEx(GetFontDefault(), t.c_str(), fs, sp);
        Etichetta(t, 4 + m.x / 2 + fs * 0.3f, 19, Hex(0xb8ffcf));   // sotto la barra in alto
    }
}

void Gioco::RicollegaRisorse() {
    if (!conLibreria) { livello.caselleLibreria.clear(); return; }
    std::string sc = mappaAttuale >= 0 && mappaAttuale < (int)mondo.mappe.size() ? mondo.mappe[mappaAttuale].scenarioLibreria : "";
    livello.caselleLibreria = lib.CaselleScenario(sc);
}

std::string Gioco::NomeEventoPartita() const {
    for (auto& ev : EventiPartita()) if (eventoPartita == ev.chiave) return ev.nome;
    return "";
}

std::vector<std::string> Gioco::PensieriCreature() const {
    std::vector<std::string> v;
    for (const Entita& e : entita)
        if (e.attivo && e.personaggio && e.mente.attiva) v.push_back(e.nome + ": " + Pensiero(e.mente));
    return v;
}

std::string Gioco::Firma() const {
    std::string s = TextFormat("mappa %d eroe %.1f,%.1f en %d vite %d punti %d monete %d gemme %d\n", mappaAttuale, eroe.pos.x, eroe.pos.y,
                               energia, vite, punteggio, monete, gemme);
    for (auto& v : variabili) s += TextFormat("var %s=%.2f\n", v.first.c_str(), v.second);
    for (auto& v : inventario) s += TextFormat("inv %s=%d\n", v.first.c_str(), v.second);
    for (const Entita& e : entita)
        s += TextFormat("%d %s att %d pos %.1f,%.1f en %d %s min %.2f fid %.2f car %.2f ap %d acc %d\n", e.uid, e.nome.c_str(), e.attivo ? 1 : 0,
                        e.pos.x, e.pos.y, e.energia, e.mente.attiva ? NomeAzioneMente(e.mente.azione) : "-", e.mente.minaccia, e.mente.fiducia,
                        e.carica, e.aperta ? 1 : 0, e.acceso ? 1 : 0);
    for (size_t i = 0; i < oggetti.size(); i++) s += TextFormat("b%d %d %.1f,%.1f\n", (int)i, oggetti[i].attivo ? 1 : 0, oggetti[i].pos.x, oggetti[i].pos.y);
    return s;
}
