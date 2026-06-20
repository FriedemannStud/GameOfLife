# Poster-Auswertung — Studienlauf mit 3.000 Mustern

**Stand:** 2026-06-19
**Datensatz:** `biotope_study`, 3.000 zufällig erzeugte 8×8-Startmuster
**Verfahren:** vollständiges Round-Robin (jeder gegen jeden), **8.997.000 Matches**
**Wichtige Regel:** Jedes Match bricht **hart nach 1000 Generationen** ab; der Zellstand bei Generation 1000 ist das Endergebnis (kein „Unentschieden durch Zeitablauf" — die Dominanz zu Gen 1000 entscheidet).

Reproduktion: `MONGODB_DB=biotope_study python3 backend/scripts/perf_tournament.py`

---

## Teil A — Strategie: „Gibt es Muster, die fast immer gewinnen?" (F2.3)

### Kernbefund: Mehr Startzellen → höhere Win-Rate

Bei großem Stichprobenumfang zeigt sich ein **klarer, monotoner Zusammenhang**: Je mehr lebende Zellen ein Startmuster hat, desto höher seine Gewinnquote.

| Startzellen | Ø Win-Rate |
|---|---|
| 2 | 26,3 % |
| 6 | 32,3 % |
| 10 | 49,8 % |
| 14 | 58,4 % |
| 18 | 62,8 % |
| 24 | 64,4 % |

Die Kurve steigt von ~26 % (2 Zellen) auf ~64 % (24 Zellen) und kreuzt die 50 %-Marke bei **rund 10 Zellen**. → **Grafik:** `260619_F2.3_winrate_vs_cells_study.svg`

> **Wichtige Einordnung — Anekdote vs. Gesetz:**
> Bei den nur **29 menschlichen** Mustern (`biotope_db`) sahen die Top-Muster *dünn besetzt* aus („voller ist nicht besser"). Der große Studienlauf zeigt: Das war ein **Kleine-Stichproben-Effekt**. Bei 3.000 Mustern wird das wahre Muster sichtbar — im 2-Team-Conway zahlt sich Masse aus. **Das ist die eigentliche Poster-Geschichte: Erst die Skalierung trennt Zufall von Gesetz.**

### Zweitbefund: Dünne Muster *leben* länger, *gewinnen* aber seltener
Die rechnerisch teuersten (langlebigsten) Muster sind auffällig **zellarm** (Top: 5 Zellen, 714 Generationen bis zur Ruhe), während die *Gewinner* zellreich sind. Lebensdauer und Erfolg sind also zwei verschiedene Dinge.

### Heatmap: bei Zufall keine „guten Gegenden"
`260619_F2.3_heatmap_study.svg` (jetzt 3.000 Muster) ist nahezu gleichverteilt — anders als die strukturierte Menschen-Heatmap. Bei reinem Zufall mittelt sich jede Position weg; den Unterschied macht **nicht wo**, sondern **wie viele** Zellen man setzt.

---

## Teil B — Performance: „Was kostet ein Turnier?" (F3.3 / F1.3)

Diese Daten stammen aus dem Performance-Monitor (`performance_metrics`, ADR-0034).

| Metrik | Wert |
|---|---|
| Wettbewerber | 3.000 |
| Matches (jeder gegen jeden) | **8.997.000** |
| Rechenzeit (Wall-Clock) | **308 s** (~5 min) |
| CPU-Zeit gesamt | 5.997 s |
| **Parallel-Speedup** | **19,5×** (auf 20 Kernen) |
| Durchsatz | 29.215 Matches/s (34 µs/Match) |
| Nie stabilisiert (lief bis Gen 1000) | **0 (0,0 %)** |

**Poster-taugliche Aussagen:**
- *Quadratisches Wachstum:* „Jeder gegen jeden" heißt — verdreifacht man die Teilnehmer, **verneunfacht** sich die Arbeit (O(N²)). 3.000 Muster = schon ~9 Mio. Spiele.
- *Viele Kerne, viel schneller:* 20 Prozessorkerne erledigen die ~100 Minuten Rechenarbeit in ~5 Minuten — ein fast 20-facher Speedup (OpenMP).
- *Stabilität als Kostenfaktor:* Jedes Spiel läuft maximal 1000 Generationen. Muster, die sich **nie beruhigen**, würden jedes ihrer Spiele bis zum Limit zwingen — der teuerste Fall. Bei Zufallsmustern trat das **kein einziges Mal** auf (alle ruhten spätestens bei Gen 714). Genau das überwacht der Live-Monitor während der Messe für die echten Einreichungen.

---

## Verwendbare Dateien

| Zweck | Datei |
|---|---|
| Win-Rate-vs-Zellzahl-Kurve | `260619_F2.3_winrate_vs_cells_study.svg` |
| Belegungs-Heatmap (Zufall, 3000) | `260619_F2.3_heatmap_study.svg` |
| Top-5-Seeds (Zufall, 3000) | `260619_F2.3_top_seeds_study.svg` |
| Mensch-vs-Zufall-Vergleich | `260619_F2.3_Datenvergleich_db_vs_study.md` |

> **Hinweis:** Dies ist der **3.000er-Zwischenstand**. Der 10.000er-Lauf läuft anschließend; eine analoge Auswertung folgt in `260619_Poster_Auswertung_study_10000.md`. Erwartung: Die Win-Rate-vs-Zellzahl-Kurve sollte sich bestätigen und glätten; Performance ~57 min Wall-Clock bei ~100 Mio. Matches.
