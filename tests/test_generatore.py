import json
import tempfile
import unittest
from pathlib import Path

from generatore.cli import main
from generatore.costruttore import genera_html
from generatore.specifica import ErroreSpecifica, carica, da_dizionario

ESEMPIO = Path(__file__).resolve().parent.parent / "esempi" / "gioco_base.json"


class TestSpecifica(unittest.TestCase):
    def test_carica_esempio(self):
        spec = carica(ESEMPIO)
        self.assertEqual(spec.titolo, "Caccia alle Stelle")
        self.assertEqual(spec.oggetti["quantita"], 12)

    def test_valori_predefiniti_uniti(self):
        spec = da_dizionario({"titolo": "Prova", "nemici": {"quantita": 5}})
        self.assertEqual(spec.nemici["quantita"], 5)
        self.assertEqual(spec.nemici["velocita"], 2)

    def test_titolo_obbligatorio(self):
        with self.assertRaises(ErroreSpecifica):
            da_dizionario({})

    def test_genere_non_supportato(self):
        with self.assertRaises(ErroreSpecifica):
            da_dizionario({"titolo": "X", "genere": "sparatutto"})

    def test_campo_sconosciuto(self):
        with self.assertRaises(ErroreSpecifica):
            da_dizionario({"titolo": "X", "boh": 1})


class TestCostruttore(unittest.TestCase):
    def test_html_contiene_titolo_e_config(self):
        pagina = genera_html(da_dizionario({"titolo": "Il <mio> gioco"}))
        self.assertIn("Il &lt;mio&gt; gioco", pagina)
        self.assertNotIn("{{CONFIG}}", pagina)
        self.assertIn('"tempo_limite": 60', pagina)

    def test_cli_genera_file(self):
        with tempfile.TemporaryDirectory() as cartella:
            self.assertEqual(main([str(ESEMPIO), "-o", cartella]), 0)
            self.assertTrue((Path(cartella) / "caccia_alle_stelle.html").exists())

    def test_cli_errore(self):
        with tempfile.TemporaryDirectory() as cartella:
            sbagliato = Path(cartella) / "x.json"
            sbagliato.write_text(json.dumps({"titolo": ""}), encoding="utf-8")
            self.assertEqual(main([str(sbagliato), "-o", cartella]), 1)


if __name__ == "__main__":
    unittest.main()
