# Poster-Auswertung — Studienlauf mit 10.000 Mustern

**Stand:** 2026-06-20
**Datensatz:** `biotope_study`, 10.000 zufällig erzeugte 8×8-Startmuster
**Verfahren:** vollständiges Round-Robin (jeder gegen jeden), **99.990.000 Matches**
**Regel:** Jedes Match bricht **hart nach 1000 Generationen** ab; der Zellstand bei Generation 1000 ist das Endergebnis.

Reproduktion: `MONGODB_DB=biotope_study python3 backend/scripts/perf_tournament.py`
Vorgänger-Auswertung: [260619_Poster_Auswertung_study_3000.md](260619_Poster_Auswertung_study_3000.md)

---

## Teil A — Strategie: „Gibt es Muster, die fast immer gewinnen?" (F2.3)

### Kernbefund bestätigt: Mehr Startzellen → höhere Win-Rate

Der bei 3.000 Mustern gefundene Zusammenhang **bestätigt sich bei 10.000 fast deckungsgleich** — ein starkes Zeichen, dass es kein Zufall, sondern ein stabiles Gesetz ist:

| Startzellen | Ø Win-Rate (3.000) | Ø Win-Rate (10.000) |
|---|---|---|
| 2 | 26,3 % | **25,5 %** |
| 6 | 32,3 % | 31,1 % |
| 10 | 49,8 % | 48,6 % |
| 14 | 58,4 % | 58,3 % |
| 18 | 62,8 % | 62,2 % |
| 24 | 64,4 % | 64,0 % |

Die Kurve steigt monoton von ~26 % (2 Zellen) auf ~64 % (24 Zellen) und kreuzt die 50 %-Marke bei **rund 10–11 Zellen**. → **Grafik:** `260620_F2.3_winrate_vs_cells_study10k.svg`

> **Poster-Geschichte:** „Bei 29 Menschen-Mustern sah es so aus, als gewinne das schlanke Muster. Bei 10.000 Mustern zeigt sich das Gesetz: Wer mehr Zellen setzt, gewinnt im Schnitt häufiger. **Erst die Skalierung trennt Anekdote von Gesetz.**" Die fast identische Kurve bei 3.000 und 10.000 ist der visuelle Beweis dafür.

### Zweitbefund bestätigt: dünn = langlebig, aber nur Mittelmaß
Die 5 rechnerisch teuersten (langlebigsten) Muster haben alle **nur 5–7 Zellen** und liegen bei ~50–52 % Win-Rate. Die *Gewinner* dagegen sind zellreich (Top 1–3: 22–24 Zellen, ~69 %). Lebensdauer ≠ Erfolg.

### Top-Win-Rate sinkt mit der Feldgröße
Die höchste erreichte Win-Rate sinkt, je größer das Feld wird: 76,8 % (29 Menschen) → 70,7 % (1.000) → 70,3 % (3.000) → **69,6 % (10.000)**. Mit mehr Konkurrenz nähert sich auch das beste Muster der „Dichte-Obergrenze" von ~64–70 %; ein Muster, das *jeden* schlägt, gibt es nicht.

### Heatmap
`260619_F2.3_heatmap_study.svg` / `260619_F2.3_top_seeds_study.svg` (neu auf 10.000 erzeugt) — bei Zufall nahezu gleichverteilt: den Unterschied macht **wie viele**, nicht **wo**.

---

## Teil B — Performance: „Was kostet ein Turnier mit 10.000?" (F3.3 / F1.3)

Quelle: Performance-Monitor (`performance_metrics`, ADR-0034).

| Metrik | 3.000 | **10.000** |
|---|---|---|
| Matches | 8.997.000 | **99.990.000** |
| Wall-Clock | 308 s (5,1 min) | **2.918 s (48,6 min)** |
| CPU-Zeit | 5.997 s | 57.538 s (16 h) |
| Parallel-Speedup | 19,5× | **19,7×** (20 Kerne) |
| Durchsatz | 29.215 Matches/s | **34.271 Matches/s** |
| Zeit pro Match | 34,2 µs | **29,2 µs** |
| Nie stabilisiert | 0 % | **0 %** |

**Poster-taugliche Aussagen:**
- *Quadratisch, aber beherrschbar:* 100 Millionen Spiele in unter 50 Minuten auf einem Rechner — durch Parallelisierung (fast 20× schneller dank 20 Kernen, OpenMP).
- *Größer = effizienter pro Match:* Obwohl 11× mehr Matches anfielen, stieg die Wall-Clock nur um das ~9,5-fache. Pro Match wurde es **schneller** (34→29 µs), weil größere Stapel die Rechenpipeline besser auslasten. Skalierung zahlt sich aus.
- *Kein Performance-Killer im Zufall:* Auch bei 100 Mio. Matches lief **kein einziges** bis zum 1000-Generationen-Limit (alle ruhten ≤ 708 Gen.). Genau diese Kennzahl (`pct_never_stabilized`) überwacht der Live-Monitor während der Messe für die echten Einreichungen — ein menschliches „Nie-Ruhe"-Muster fiele dort sofort auf.

---

## Verwendbare Dateien

| Zweck | Datei |
|---|---|
| Win-Rate-vs-Zellzahl-Kurve (10k) | `260620_F2.3_winrate_vs_cells_study10k.svg` |
| Win-Rate-vs-Zellzahl-Kurve (3k, Vergleich) | `260619_F2.3_winrate_vs_cells_study.svg` |
| Belegungs-Heatmap (Zufall, 10k) | `260619_F2.3_heatmap_study.svg` |
| Top-5-Seeds (Zufall, 10k) | `260619_F2.3_top_seeds_study.svg` |
| Mensch-vs-Zufall-Vergleich | `260619_F2.3_Datenvergleich_db_vs_study.md` |

> **Hinweis:** Die `*_study.svg`-Heatmap/Seeds wurden auf den 10.000er-Stand neu erzeugt (die 3.000er-Versionen liegen in der Git-Historie, Commit `77be329`). `biotope_study` ist ein fester Studien-Datensatz; `biotope_db` (echte Menschen) ändert sich pro Turnier-Epoche. Für den Live-Betrieb in `.env` wieder `biotope_db` aktivieren.
