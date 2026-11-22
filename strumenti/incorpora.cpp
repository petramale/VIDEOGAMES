// Trasforma file binari (immagini, suoni...) in un header C++ da includere nello studio,
// cosi' l'eseguibile non ha bisogno di file esterni.
//
//   g++ -O2 -o incorpora strumenti/incorpora.cpp
//   incorpora src/risorse_dati.h LOGO_PNG risorse/elvantis_logo_800.png ICONA_PNG risorse/elvantis_icona.png
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 4 || (argc - 2) % 2 != 0) {
        std::fprintf(stderr, "Uso: incorpora uscita.h NOME1 file1 [NOME2 file2 ...]\n");
        return 1;
    }
    std::ofstream out(argv[1]);
    out << "// Generato da strumenti/incorpora.cpp: non modificare a mano.\n#pragma once\n\n";
    for (int i = 2; i + 1 < argc; i += 2) {
        std::ifstream f(argv[i + 1], std::ios::binary);
        if (!f) { std::fprintf(stderr, "Non trovo %s\n", argv[i + 1]); return 1; }
        std::vector<unsigned char> dati((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        out << "// " << argv[i + 1] << "\nstatic const unsigned char " << argv[i] << "[" << dati.size() << "] = {";
        for (size_t k = 0; k < dati.size(); k++) {
            if (k % 20 == 0) out << "\n    ";
            out << (int)dati[k] << ",";
        }
        out << "\n};\n\n";
    }
    return 0;
}
