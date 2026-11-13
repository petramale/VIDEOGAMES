"""Lettura e validazione della descrizione di un gioco (file JSON)."""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path

GENERI_SUPPORTATI = {"raccolta"}


class ErroreSpecifica(ValueError):
    """La descrizione del gioco non è valida."""


@dataclass
class Specifica:
    titolo: str
    genere: str = "raccolta"
    larghezza: int = 800
    altezza: int = 600
    colore_sfondo: str = "#1d2b53"
    giocatore: dict = field(default_factory=lambda: {"colore": "#ffec27", "velocita": 4})
    oggetti: dict = field(default_factory=lambda: {"colore": "#00e436", "quantita": 10})
    nemici: dict = field(default_factory=lambda: {"colore": "#ff004d", "quantita": 3, "velocita": 2})
    tempo_limite: int = 60

    def come_dizionario(self) -> dict:
        return dict(self.__dict__)


def da_dizionario(dati: dict) -> Specifica:
    if not isinstance(dati, dict):
        raise ErroreSpecifica("La specifica deve essere un oggetto JSON.")
    titolo = dati.get("titolo")
    if not isinstance(titolo, str) or not titolo.strip():
        raise ErroreSpecifica("Il campo 'titolo' è obbligatorio.")

    spec = Specifica(titolo=titolo.strip())
    for chiave, valore in dati.items():
        if chiave == "titolo":
            continue
        if not hasattr(spec, chiave):
            raise ErroreSpecifica(f"Campo sconosciuto: '{chiave}'.")
        predefinito = getattr(spec, chiave)
        if isinstance(predefinito, dict):
            if not isinstance(valore, dict):
                raise ErroreSpecifica(f"Il campo '{chiave}' deve essere un oggetto.")
            valore = {**predefinito, **valore}
        setattr(spec, chiave, valore)

    if spec.genere not in GENERI_SUPPORTATI:
        raise ErroreSpecifica(
            f"Genere '{spec.genere}' non supportato. Disponibili: {', '.join(sorted(GENERI_SUPPORTATI))}."
        )
    for chiave in ("larghezza", "altezza", "tempo_limite"):
        valore = getattr(spec, chiave)
        if not isinstance(valore, int) or valore <= 0:
            raise ErroreSpecifica(f"Il campo '{chiave}' deve essere un intero positivo.")
    return spec


def carica(percorso: str | Path) -> Specifica:
    try:
        dati = json.loads(Path(percorso).read_text(encoding="utf-8"))
    except json.JSONDecodeError as errore:
        raise ErroreSpecifica(f"JSON non valido: {errore}") from errore
    return da_dizionario(dati)
