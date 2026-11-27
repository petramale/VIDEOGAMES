// La Mente: utility AI con memoria, emozioni e apprendimento di specie. Vedi mente.h.
#include "mente.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
inline float L01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float Lim(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
}

// --- Generatore -------------------------------------------------------------------------------------------------

uint64_t Mescola64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

void Generatore::Semina(uint64_t seme) {
    s = Mescola64(seme);
    if (s == 0) s = 0x9E3779B97F4A7C15ull;
}

uint64_t Generatore::Prossimo() {
    s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
    return s * 2685821657736338717ull;
}

float Generatore::Float() { return (float)((Prossimo() >> 40) / 16777216.0); }
float Generatore::Float(float lo, float hi) { return lo + (hi - lo) * Float(); }
int Generatore::Intero(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (int)(Prossimo() % (uint64_t)(hi - lo + 1));
}

uint64_t ImprontaTesto(const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
}

std::string Esadecimale64(uint64_t v) {
    char b[20];
    snprintf(b, sizeof b, "%08x%08x", (unsigned)(v >> 32), (unsigned)(v & 0xffffffffu));
    return b;
}

bool DaEsadecimale64(const std::string& t, uint64_t& v) {
    if (t.empty() || t.size() > 16) return false;
    uint64_t r = 0;
    for (char c : t) {
        int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return false;
        r = (r << 4) | (uint64_t)d;
    }
    v = r;
    return true;
}

// --- Azioni ------------------------------------------------------------------------------------------------

const char* NomeAzioneMente(AzioneMente a) {
    static const char* n[] = { "Pattuglia", "Insegui", "Attacca", "Aggira", "Cerca", "Fuggi", "Curati", "Chiama aiuto",
                               "Schiva", "Guardia", "Riposa", "Amico", "Combatti rivale" };
    int i = (int)a;
    return i >= 0 && i < NUM_AZIONI_MENTE ? n[i] : "?";
}

const char* ChiaveAzioneMente(AzioneMente a) {
    static const char* n[] = { "pattuglia", "insegui", "attacca", "aggira", "cerca", "fuggi", "curati", "chiama_aiuto",
                               "schiva", "guardia", "riposa", "amico", "rivale" };
    int i = (int)a;
    return i >= 0 && i < NUM_AZIONI_MENTE ? n[i] : "?";
}

const char* SpiegaAzioneMente(AzioneMente a) {
    switch (a) {
        case AzioneMente::Pattuglia: return "Gira per la sua zona quando non c'e' niente di meglio da fare.";
        case AzioneMente::Insegui: return "Corre verso l'eroe quando lo vede e non ha paura.";
        case AzioneMente::Attacca: return "Spara da lontano (se ha un proiettile) o fa uno scatto improvviso.";
        case AzioneMente::Aggira: return "Imboscata: tiene le distanze e scatta quando l'eroe e' vulnerabile.";
        case AzioneMente::Cerca: return "Va dove ha visto l'eroe l'ultima volta e si guarda intorno.";
        case AzioneMente::Fuggi: return "Scappa quando la paura supera il coraggio (ferito, solo, spaventato).";
        case AzioneMente::Curati: return "Con poca energia va a prendersi un oggetto Cura prima dell'eroe.";
        case AzioneMente::ChiamaAiuto: return "Grida: i compagni vicini accorrono dove ha visto l'eroe.";
        case AzioneMente::Schiva: return "Salta via quando l'eroe gli cade addosso o arriva un proiettile (lo impara).";
        case AzioneMente::Guardia: return "Torna al suo posto e lo difende.";
        case AzioneMente::Riposa: return "Si ferma a riprendere fiato quando e' stanco: un'occasione per l'eroe.";
        case AzioneMente::Amico: return "Se la fiducia e' alta segue l'eroe, non fa danni e lo aiuta.";
        case AzioneMente::Rivale: return "Attacca le creature di una specie rivale (o i nemici, se e' amico).";
        default: return "";
    }
}

// --- Profili -----------------------------------------------------------------------------------------------

ProfiloComportamento::ProfiloComportamento() {
    for (float& p : peso) p = 1;
    peso[(int)AzioneMente::Amico] = 0.6f;
}

const std::vector<ProfiloComportamento>& ModelliProfilo() {
    static std::vector<ProfiloComportamento> v;
    if (!v.empty()) return v;
    auto P = [&](const char* nome, const char* desc, Carattere c, float appr, float var) {
        ProfiloComportamento p;
        p.nome = nome; p.descrizione = desc; p.car = c; p.apprendimento = appr; p.variabilita = var;
        p.autore = "Modello";
        v.push_back(p);
        return &v.back();
    };
    using A = AzioneMente;
    P("Istintivo", "Equilibrato: insegue se vede l'eroe, scappa se e' ferito, impara un po'.", Carattere{}, 0.3f, 0.25f);
    {
        auto* p = P("Lupo di branco", "Caccia in gruppo: chiama i compagni, aggira la preda, attacca quando e' in vantaggio.",
                    Carattere{ 0.65f, 0.6f, 0.45f, 0.9f, 0.5f, 0.5f, 0.3f }, 0.4f, 0.3f);
        p->peso[(int)A::ChiamaAiuto] = 2; p->peso[(int)A::Aggira] = 1.6f; p->peso[(int)A::Amico] = 0.3f;
    }
    {
        auto* p = P("Guardiano rancoroso", "Difende il suo posto e non dimentica chi lo ha colpito.",
                    Carattere{ 0.85f, 0.55f, 0.5f, 0.2f, 0.95f, 0.15f, 0.15f }, 0.3f, 0.15f);
        p->peso[(int)A::Guardia] = 2.4f; p->peso[(int)A::Fuggi] = 0.4f; p->peso[(int)A::Pattuglia] = 0.6f; p->peso[(int)A::Amico] = 0.1f;
    }
    {
        auto* p = P("Codardo furbo", "Scappa appena e' in pericolo, si cura, torna quando l'eroe e' debole.",
                    Carattere{ 0.15f, 0.45f, 0.8f, 0.5f, 0.3f, 0.6f, 0.4f }, 0.5f, 0.3f);
        p->peso[(int)A::Fuggi] = 1.8f; p->peso[(int)A::Curati] = 2; p->peso[(int)A::Insegui] = 0.7f; p->fuga = 0.6f;
    }
    {
        auto* p = P("Cacciatore paziente", "Segue le tracce, tende imboscate, non molla facilmente.",
                    Carattere{ 0.7f, 0.65f, 0.7f, 0.2f, 0.7f, 0.9f, 0.2f }, 0.5f, 0.2f);
        p->peso[(int)A::Aggira] = 2; p->peso[(int)A::Cerca] = 2; p->peso[(int)A::Insegui] = 0.8f;
    }
    {
        auto* p = P("Abitante cordiale", "Un personaggio che parla: si fida di chi lo aiuta, ha paura di chi lo colpisce.",
                    Carattere{ 0.3f, 0.05f, 0.6f, 0.8f, 0.6f, 0.5f, 0.1f }, 0.2f, 0.3f);
        p->peso[(int)A::Insegui] = 0; p->peso[(int)A::Attacca] = 0; p->peso[(int)A::Aggira] = 0; p->peso[(int)A::Rivale] = 0;
        p->peso[(int)A::Amico] = 1.5f; p->peso[(int)A::Guardia] = 1.5f; p->chiamaAiuto = false;
        p->fraseAmico = "Sei un vero amico! Tieni gli occhi aperti, piu' avanti e' pericoloso.";
        p->fraseNeutra = "Ciao, viaggiatore.";
        p->fraseOstile = "Stai lontano da me!";
        p->frasePaura = "Aiuto! Non farmi del male!";
    }
    {
        auto* p = P("Bestia selvatica", "Imprevedibile e aggressiva, ma si stanca presto.",
                    Carattere{ 0.6f, 0.9f, 0.1f, 0.1f, 0.4f, 0.3f, 0.85f }, 0.25f, 0.45f);
        p->peso[(int)A::Riposa] = 1.6f; p->peso[(int)A::ChiamaAiuto] = 0.2f; p->peso[(int)A::Insegui] = 1.4f;
    }
    {
        auto* p = P("Boss che impara", "Studia il giocatore: impara in fretta a schivare e a cambiare tattica.",
                    Carattere{ 0.95f, 0.75f, 0.6f, 0.4f, 0.8f, 0.6f, 0.35f }, 0.9f, 0.2f);
        p->peso[(int)A::Fuggi] = 0.2f; p->peso[(int)A::Amico] = 0; p->peso[(int)A::Schiva] = 1.8f; p->peso[(int)A::Riposa] = 0.5f;
    }
    return v;
}

const std::vector<EventoPartita>& EventiPartita() {
    static const std::vector<EventoPartita> v = {
        { "luna_rossa", "Luna rossa", "I nemici sono piu' aggressivi, ma valgono piu' punti." },
        { "nebbia", "Nebbia", "Le creature vedono meno lontano." },
        { "festa", "Giorno di festa", "Gli abitanti si fidano di piu' e le monete valgono di piu'." },
        { "caccia", "Stagione della caccia", "Le creature si chiamano a vicenda piu' spesso." },
        { "calma", "Giornata calma", "Le creature sono piu' lente e tranquille." },
        { "tesoro", "Tesoro nascosto", "In ogni mappa un oggetto brilla: vale cinque volte tanto." },
    };
    return v;
}

// --- Sapere ------------------------------------------------------------------------------------------------

float Sapere::Schiva() const { return 0.6f * L01(schiacciato * 1.3f); }
float Sapere::EvitaColpi() const { return 0.6f * L01(colpito * 1.3f); }
float Sapere::Distanza() const { return 0.6f * L01((schiacciato + colpito) * 0.5f + spariEroe * 0.3f); }

void Sapere::Impara(float& valore, float bersaglio, float apprendimento) {
    // Media mobile: il valore si avvicina al bersaglio di una frazione; con 0 non cambia mai.
    valore = L01(valore + (bersaglio - valore) * L01(apprendimento) * 0.35f);
}

// --- Mente -------------------------------------------------------------------------------------------------

void Mente::Ricorda(TipoRicordo t, float quando, Vector2 dove, float forza) {
    // Due ricordi uguali e vicini nel tempo si fondono (non si riempie la memoria di "visto l'eroe").
    for (int i = 0; i < numRicordi; i++) {
        Ricordo& r = ricordi[i];
        if (r.tipo == t && quando - r.quando < 2.0f) { r.quando = quando; r.dove = dove; r.forza = std::min(1.0f, r.forza + forza * 0.5f); return; }
    }
    ricordi[prossimo] = { t, quando, dove, L01(forza) };
    prossimo = (prossimo + 1) % 8;
    if (numRicordi < 8) numRicordi++;
}

int Mente::Conta(TipoRicordo t, float dopo) const {
    int n = 0;
    for (int i = 0; i < numRicordi; i++) if (ricordi[i].tipo == t && ricordi[i].quando >= dopo && ricordi[i].forza > 0.05f) n++;
    return n;
}

void Mente::Sbiadisci(float ora) {
    // Il rancore fa durare di piu' i ricordi brutti.
    for (int i = 0; i < numRicordi; i++) {
        Ricordo& r = ricordi[i];
        float vita = (r.tipo == TipoRicordo::ColpitoDaEroe ? 30 + 90 * car.rancore : 25);
        float eta = ora - r.quando;
        if (eta > vita) r.forza = 0;
    }
}

Carattere CarattereDellaPartita(const Carattere& b, float variabilita, Generatore& rng) {
    float v = Lim(variabilita, 0, 1) * 0.5f;
    auto V = [&](float x) { return L01(x + rng.Float(-v, v)); };
    Carattere c;
    c.coraggio = V(b.coraggio); c.aggressivita = V(b.aggressivita); c.prudenza = V(b.prudenza);
    c.socievolezza = V(b.socievolezza); c.rancore = V(b.rancore); c.curiosita = V(b.curiosita);
    c.imprevedibilita = V(b.imprevedibilita);
    return c;
}

float PunteggioAzione(const Mente& m, const ProfiloComportamento& p, const Sapere& s, const Contesto& c, AzioneMente a) {
    const Carattere& k = m.car;
    float coraggioso = 1 - m.paura;
    float u = 0;
    switch (a) {
        case AzioneMente::Pattuglia:
            u = 0.22f + 0.18f * k.curiosita * (1 - c.vede) - 0.25f * m.allerta * c.memoria;
            break;
        case AzioneMente::Insegui:
            u = c.vede * (0.35f + 0.55f * k.aggressivita) * coraggioso * (1 - 0.6f * m.stanchezza) * (1 - 0.5f * s.Distanza())
                + 0.15f * m.minaccia * coraggioso * c.vede;
            u *= 1 - 0.6f * c.pieta;
            if (c.abitante) u = 0;
            break;
        case AzioneMente::Attacca:
            if (c.puoSparare > 0) u = c.vede * (c.distanza > 0.2f ? 1.0f : 0.4f) * (0.4f + 0.5f * k.aggressivita) * (1 - 0.5f * m.paura);
            else u = c.vede * (c.distanza < 0.35f ? 1.0f : 0) * k.aggressivita * 0.7f * coraggioso * (1 - m.stanchezza);
            u *= 1 - 0.5f * c.pieta;
            if (c.abitante) u = 0;
            break;
        case AzioneMente::Aggira:
            u = c.vede * (c.distanza > 0.15f ? 1.0f : 0.3f) * (0.25f * k.prudenza + 0.25f * k.aggressivita + 0.6f * s.Distanza()) * coraggioso;
            if (c.abitante) u = 0;
            break;
        case AzioneMente::Cerca:
            u = (1 - c.vede) * c.memoria * (0.25f + 0.45f * k.curiosita) * (0.4f + 0.6f * m.allerta);
            if (c.abitante) u *= 0.2f;
            break;
        case AzioneMente::Fuggi: {
            float ostile = c.abitante ? L01(-m.fiducia) * 0.9f : 0;
            u = m.paura * (1.15f - c.energia) * (c.vede > 0 ? 1.0f : 0.35f) + ostile * c.vede + (c.energia < p.fuga ? 0.2f * c.vede : 0);
            u *= 1 - 0.7f * c.chiuso;   // spalle al muro: si combatte
            break;
        }
        case AzioneMente::Curati:
            u = c.cura * (1 - c.energia) * (0.6f + 0.6f * k.prudenza) * 1.3f;
            break;
        case AzioneMente::ChiamaAiuto:
            u = (p.chiamaAiuto ? 1.0f : 0) * c.vede * (c.alleati > 0 ? 1.0f : 0) * (0.3f + 0.7f * k.socievolezza) *
                (0.3f + m.minaccia + 0.3f * m.allerta) * (m.tRichiamo <= 0 ? 1.0f : 0) * 0.8f;
            break;
        case AzioneMente::Schiva:
            u = (m.tSchiva <= 0 ? 1.0f : 0) * (c.eroeSopra * (0.15f + 2 * s.Schiva()) * (0.5f + k.prudenza) +
                                               c.proiettile * (0.1f + 2 * s.EvitaColpi()) * (0.5f + k.prudenza));
            break;
        case AzioneMente::Guardia:
            u = 0.08f + c.lontanoCasa * (1 - c.vede) * (0.3f + 0.5f * k.prudenza) + (c.abitante ? 0.25f : 0);
            break;
        case AzioneMente::Riposa:
            u = m.stanchezza * 0.85f * (1 - 0.5f * c.vede) * (c.boss ? 0.5f : 1.0f);
            break;
        case AzioneMente::Amico:
            u = L01(m.fiducia - 0.35f) * 1.8f * (0.4f + k.socievolezza);
            if (c.boss) u = 0;
            break;
        case AzioneMente::Rivale:
            u = c.rivale * (0.3f + 0.6f * k.aggressivita) * coraggioso * (c.vede > 0 && m.fiducia < 0.4f ? 0.6f : 1.0f);
            break;
        default: break;
    }
    return std::max(0.0f, u) * std::max(0.0f, p.peso[(int)a]);
}

AzioneMente Decidi(Mente& m, const ProfiloComportamento& p, const Sapere& s, const Contesto& c, Generatore& rng) {
    int migliore = (int)m.azione;
    float top = -1;
    bool puoCambiare = m.tAzione >= std::max(0.2f, p.durataMinima);
    for (int i = 0; i < NUM_AZIONI_MENTE; i++) {
        float u = PunteggioAzione(m, p, s, c, (AzioneMente)i);
        if (p.peso[i] <= 0) { m.punteggi[i] = 0; continue; }
        if (i == (int)m.azione) u += 0.15f;                              // isteresi
        u += rng.Float(-1, 1) * m.car.imprevedibilita * 0.08f;           // sorpresa
        m.punteggi[i] = std::max(0.0f, u);
        if (u > top) { top = u; migliore = i; }
    }
    // Le reazioni istantanee (schivare) non aspettano la durata minima.
    bool urgente = migliore == (int)AzioneMente::Schiva;
    if (migliore != (int)m.azione && (puoCambiare || urgente)) { m.azione = (AzioneMente)migliore; m.tAzione = 0; }
    return m.azione;
}

void AggiornaEmozioni(Mente& m, const ProfiloComportamento& p, const Contesto& c, float dt) {
    (void)p;
    const Carattere& k = m.car;
    m.tAzione += dt;
    if (m.tRichiamo > 0) m.tRichiamo -= dt;
    if (m.tSchiva > 0) m.tSchiva -= dt;
    if (m.tScatto > 0) m.tScatto -= dt;
    if (m.tRivale > 0) m.tRivale -= dt;
    m.tColpoEroe += dt;
    m.tVisto += dt;
    // La minaccia torna lentamente a zero; il rancore la fa durare di piu'.
    m.minaccia = L01(m.minaccia - dt * 0.1f * (1.2f - 0.6f * k.rancore));
    if (c.vede > 0) m.allerta = 1;
    else m.allerta = L01(m.allerta - dt * 0.12f);
    // La paura insegue un bersaglio: minaccia e ferite pesano di piu' se il coraggio e' poco.
    float bers = m.minaccia * (1 - k.coraggio) * 1.2f + (1 - c.energia) * (1 - k.coraggio) * 0.7f;
    if (c.abitante && c.vede > 0) bers += L01(-m.fiducia) * 0.5f;
    bers = L01(bers - 0.4f * c.alleati * k.socievolezza * 0.3f);
    m.paura += (bers - m.paura) * std::min(1.0f, dt * 1.5f);
    // Stanchezza: sale correndo, scende riposando.
    bool corre = m.azione == AzioneMente::Insegui || m.azione == AzioneMente::Fuggi || m.tScatto > 0;
    if (corre) m.stanchezza = L01(m.stanchezza + dt * 0.045f);
    else m.stanchezza = L01(m.stanchezza - dt * (m.azione == AzioneMente::Riposa ? 0.3f : 0.06f));
    // Fiducia: torna verso quella di base (il rancore rallenta il perdono).
    m.fiducia += (m.fiduciaBase - m.fiducia) * std::min(1.0f, dt * 0.01f * (1.1f - k.rancore));
    // Risparmiato: l'eroe gli e' vicino, lo ha colpito in passato ma ora non lo fa da un po'.
    if (c.vede > 0 && c.distanza < 0.6f && m.tColpoEroe > 5 && m.Conta(TipoRicordo::ColpitoDaEroe) > 0 && m.azione != AzioneMente::Insegui && m.azione != AzioneMente::Attacca)
        m.fiducia += dt * 0.06f * (0.2f + k.socievolezza) * (1.2f - k.rancore);
    m.fiducia = Lim(m.fiducia, -1, 1);
    m.Sbiadisci(c.tempo);
}

std::string FraseUmore(const Mente& m, const ProfiloComportamento& p, const std::string& frase) {
    if (!m.attiva) return frase;
    const std::string* f = nullptr;
    if (m.paura > 0.5f && !p.frasePaura.empty()) f = &p.frasePaura;
    else if (m.fiducia > 0.4f && !p.fraseAmico.empty()) f = &p.fraseAmico;
    else if (m.fiducia < -0.3f && !p.fraseOstile.empty()) f = &p.fraseOstile;
    else if (!p.fraseNeutra.empty() && frase.empty()) f = &p.fraseNeutra;
    return f ? *f : frase;
}

std::string Pensiero(const Mente& m) {
    char b[160];
    snprintf(b, sizeof b, "%s  min %.1f  pau %.1f  fid %+.1f  sta %.1f", NomeAzioneMente(m.azione), m.minaccia, m.paura, m.fiducia, m.stanchezza);
    return b;
}
