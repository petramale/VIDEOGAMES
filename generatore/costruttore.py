"""Trasforma una Specifica in un file HTML giocabile nel browser."""

from __future__ import annotations

import html
import json
from importlib import resources
from pathlib import Path

from generatore.specifica import Specifica


def genera_html(spec: Specifica) -> str:
    modello = resources.files("generatore.modelli").joinpath(f"{spec.genere}.html").read_text(encoding="utf-8")
    configurazione = json.dumps(spec.come_dizionario(), ensure_ascii=False).replace("</", "<\\/")
    return modello.replace("{{TITOLO}}", html.escape(spec.titolo)).replace("{{CONFIG}}", configurazione)


def genera_file(spec: Specifica, cartella_uscita: str | Path) -> Path:
    cartella = Path(cartella_uscita)
    cartella.mkdir(parents=True, exist_ok=True)
    nome = "".join(c if c.isalnum() else "_" for c in spec.titolo.lower()).strip("_") or "gioco"
    destinazione = cartella / f"{nome}.html"
    destinazione.write_text(genera_html(spec), encoding="utf-8")
    return destinazione
