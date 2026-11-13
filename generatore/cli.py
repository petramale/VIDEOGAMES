"""Interfaccia a riga di comando: python -m generatore esempi/gioco_base.json"""

from __future__ import annotations

import argparse
import sys

from generatore.costruttore import genera_file
from generatore.specifica import ErroreSpecifica, carica


def main(argomenti: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="genera-gioco",
        description="Genera un videogioco HTML5 a partire da una descrizione JSON.",
    )
    parser.add_argument("specifica", help="percorso del file JSON che descrive il gioco")
    parser.add_argument("-o", "--uscita", default="output", help="cartella di destinazione (predefinita: output)")
    opzioni = parser.parse_args(argomenti)

    try:
        spec = carica(opzioni.specifica)
    except (OSError, ErroreSpecifica) as errore:
        print(f"Errore: {errore}", file=sys.stderr)
        return 1

    percorso = genera_file(spec, opzioni.uscita)
    print(f"Gioco generato: {percorso}")
    print("Aprilo con un doppio clic per giocare nel browser.")
    return 0
