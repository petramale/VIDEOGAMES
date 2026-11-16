#include "periferiche.h"
#include "sistema.h"
#include <fstream>
#include <sstream>

Periferiche gInput;

const char* NomeAzione(Azione a) {
    static const char* nomi[] = { "Sinistra", "Destra", "Su", "Giu'", "Salto", "Conferma", "Indietro", "Pausa" };
    return nomi[(int)a];
}

const char* NomeTasto(int k) {
    if (k == 0) return "-";
    if (k >= KEY_A && k <= KEY_Z) return TextFormat("%c", 'A' + (k - KEY_A));
    if (k >= KEY_ZERO && k <= KEY_NINE) return TextFormat("%c", '0' + (k - KEY_ZERO));
    if (k >= KEY_F1 && k <= KEY_F12) return TextFormat("F%d", k - KEY_F1 + 1);
    switch (k) {
        case KEY_SPACE: return "Spazio";
        case KEY_ENTER: return "Invio";
        case KEY_ESCAPE: return "Esc";
        case KEY_BACKSPACE: return "Backspace";
        case KEY_TAB: return "Tab";
        case KEY_LEFT: return "Freccia sx";
        case KEY_RIGHT: return "Freccia dx";
        case KEY_UP: return "Freccia su";
        case KEY_DOWN: return "Freccia giu'";
        case KEY_LEFT_SHIFT: return "Shift sx";
        case KEY_RIGHT_SHIFT: return "Shift dx";
        case KEY_LEFT_CONTROL: return "Ctrl sx";
        case KEY_RIGHT_CONTROL: return "Ctrl dx";
        case KEY_LEFT_ALT: return "Alt";
        case KEY_P: return "P";
        case KEY_KP_0: return "Num 0";
        case KEY_KP_ENTER: return "Num Invio";
    }
    return TextFormat("Tasto %d", k);
}

const char* NomePulsante(int p) {
    switch (p) {
        case GAMEPAD_BUTTON_LEFT_FACE_UP: return "Croce su";
        case GAMEPAD_BUTTON_LEFT_FACE_RIGHT: return "Croce dx";
        case GAMEPAD_BUTTON_LEFT_FACE_DOWN: return "Croce giu'";
        case GAMEPAD_BUTTON_LEFT_FACE_LEFT: return "Croce sx";
        case GAMEPAD_BUTTON_RIGHT_FACE_UP: return "Y / Triangolo";
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return "B / Cerchio";
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN: return "A / Croce";
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT: return "X / Quadrato";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1: return "LB / L1";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1: return "RB / R1";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_2: return "LT / L2";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_2: return "RT / R2";
        case GAMEPAD_BUTTON_MIDDLE_LEFT: return "Select";
        case GAMEPAD_BUTTON_MIDDLE_RIGHT: return "Start";
        case GAMEPAD_BUTTON_LEFT_THUMB: return "Levetta sx";
        case GAMEPAD_BUTTON_RIGHT_THUMB: return "Levetta dx";
    }
    return "-";
}

Mappatura Mappatura::Predefinita() {
    Mappatura m;
    auto Imposta = [&](Azione a, int k1, int k2, int p) { m.tasti[(int)a][0] = k1; m.tasti[(int)a][1] = k2; m.pulsanti[(int)a] = p; };
    Imposta(Azione::Sinistra, KEY_LEFT, KEY_A, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    Imposta(Azione::Destra, KEY_RIGHT, KEY_D, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    Imposta(Azione::Su, KEY_UP, KEY_W, GAMEPAD_BUTTON_LEFT_FACE_UP);
    Imposta(Azione::Giu, KEY_DOWN, KEY_S, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    Imposta(Azione::Salto, KEY_SPACE, KEY_UP, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    Imposta(Azione::Conferma, KEY_ENTER, KEY_SPACE, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    Imposta(Azione::Indietro, KEY_ESCAPE, 0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    Imposta(Azione::Pausa, KEY_P, 0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
    return m;
}

static std::string FileComandi() { return CartellaEseguibile() + "/comandi.cfg"; }

void Periferiche::Carica() {
    std::ifstream f(FileComandi());
    if (!f) return;
    std::string riga;
    while (std::getline(f, riga)) {
        std::istringstream ss(riga);
        std::string chiave;
        ss >> chiave;
        if (chiave == "mouse") ss >> m.mouseNelGioco;
        else if (chiave == "zona_morta") ss >> m.zonaMorta;
        else
            for (int a = 0; a < NUM_AZIONI; a++)
                if (chiave == TextFormat("azione%d", a)) ss >> m.tasti[a][0] >> m.tasti[a][1] >> m.pulsanti[a];
    }
}

void Periferiche::Salva() const {
    std::ofstream f(FileComandi());
    f << "# Comandi dello Studio Videogiochi: azione tasto1 tasto2 pulsante_joypad\n";
    for (int a = 0; a < NUM_AZIONI; a++)
        f << "azione" << a << " " << m.tasti[a][0] << " " << m.tasti[a][1] << " " << m.pulsanti[a]
          << "   # " << NomeAzione((Azione)a) << "\n";
    f << "mouse " << m.mouseNelGioco << "\nzona_morta " << m.zonaMorta << "\n";
}

const char* Periferiche::NomeJoypad() const {
    return joypad >= 0 ? GetGamepadName(joypad) : "nessun joypad collegato";
}

float Periferiche::Asse() const {
    if (std::fabs(levettaX) > m.zonaMorta) return levettaX;
    return (Tenuto(Azione::Destra) ? 1.0f : 0.0f) - (Tenuto(Azione::Sinistra) ? 1.0f : 0.0f);
}

void Periferiche::Aggiorna(float scala, Vector2 margine) {
    for (int a = 0; a < NUM_AZIONI; a++) precedente[a] = tenuto[a];

    // Il primo joypad collegato diventa quello attivo (anche se viene inserito a gioco avviato).
    joypad = -1;
    for (int i = 0; i < 4; i++) if (IsGamepadAvailable(i)) { joypad = i; break; }
    levettaX = levettaY = 0;
    if (joypad >= 0) {
        levettaX = GetGamepadAxisMovement(joypad, GAMEPAD_AXIS_LEFT_X);
        levettaY = GetGamepadAxisMovement(joypad, GAMEPAD_AXIS_LEFT_Y);
    }

    for (int a = 0; a < NUM_AZIONI; a++) {
        bool tasto = (m.tasti[a][0] && IsKeyDown(m.tasti[a][0])) || (m.tasti[a][1] && IsKeyDown(m.tasti[a][1]));
        bool pulsante = joypad >= 0 && m.pulsanti[a] && IsGamepadButtonDown(joypad, m.pulsanti[a]);
        bool levetta = false;
        if (joypad >= 0) {
            if (a == (int)Azione::Sinistra) levetta = levettaX < -m.zonaMorta;
            if (a == (int)Azione::Destra) levetta = levettaX > m.zonaMorta;
            if (a == (int)Azione::Su) levetta = levettaY < -m.zonaMorta - 0.2f;
            if (a == (int)Azione::Giu) levetta = levettaY > m.zonaMorta + 0.2f;
        }
        tenuto[a] = tasto || pulsante || levetta;
        if (tasto) ultima = Periferica::Tastiera;
        if (pulsante || levetta) ultima = Periferica::Joypad;
    }

    Vector2 p = GetMousePosition();
    Vector2 nuovo = { (p.x - margine.x) / scala, (p.y - margine.y) / scala };
    Vector2 d = GetMouseDelta();
    mouseMosso = d.x != 0 || d.y != 0;
    mouse = nuovo;
    if (mouseMosso || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ultima = Periferica::Mouse;
}
