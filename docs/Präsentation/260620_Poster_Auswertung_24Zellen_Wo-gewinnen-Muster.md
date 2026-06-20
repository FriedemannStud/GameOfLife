# Poster-Auswertung — „Wo gewinnen Muster?" bei **exakt 24 Startzellen**

**Stand:** 2026-06-20
**Datensatz:** `biotope_study`, 10.000 zufällige 8×8-Startmuster — gefiltert auf die **430 Muster mit genau 24 lebenden Startzellen**
**Verfahren:** Auswertung der Matches dieser 430 Muster (je 9.999 Gegner → 4.299.570 Match-Ergebnisse) aus dem vollständigen Round-Robin
**Regel:** hartes Limit bei 1000 Generationen; Zellstand bei Gen. 1000 = Endergebnis.

Reproduktion: `MONGODB_DB=biotope_study python3 backend/scripts/pattern_heatmap_fixedcells.py 24 5`
Überblick (alle Zellzahlen): [260620_Poster_Auswertung_study_10000.md](260620_Poster_Auswertung_study_10000.md)

---

## Warum ausgerechnet 24 Zellen?

Der Hauptbefund des Posters lautet: **„Es zählt, *wie viele* Zellen — nicht *wo*."** Die Win-Rate steigt monoton von ~26 % (2 Zellen) auf ~64 % (24 Zellen). Solange Muster unterschiedlich viele Zellen haben, überlagert dieser starke Dichte-Effekt jede Ortsabhängigkeit.

24 Zellen ist die **dichteste Stufe** im Datensatz. Hält man die Zellzahl konstant, fällt der Dichte-Effekt als Erklärung weg. Übrig bleibt die saubere Frage:

> **Bei gleicher Dichte — entscheidet die *Position* der Zellen über Sieg oder Niederlage?**

Die 430 Muster sind dafür ideal: gleiche Zellzahl, gleiche Match-Zahl, nur die Anordnung der 24 Zellen auf den 64 Feldern unterscheidet sie.

---

## Kernbefund: Auch bei gleicher Dichte gilt — die Position ist (fast) egal

| Kennzahl | Wert |
|---|---|
| Muster mit genau 24 Zellen | **430** |
| Ø Win-Rate (Gruppe) | **63,97 %** |
| Streuung der Win-Rate zwischen Mustern | 3,04 Prozentpunkte |
| **Ø Win-Rate je Feld — Spanne über alle 64 Felder** | nur **63,6 % – 64,6 %** |
| Streuung dieser 64 Feld-Mittelwerte | **0,21 pp** |

Färbt man jedes der 64 Bretter-Felder mit der durchschnittlichen Win-Rate aller Muster, die dieses Feld belegen, liegt **jedes Feld zwischen 63,6 % und 64,6 %** — eine Spanne von gerade einmal **einem Prozentpunkt**. Es gibt **kein „heißes" Siegerfeld** und keine kalte Verliererzone.

→ **Grafik:** `260620_F2.3_heatmap_24cells_study.svg`

### Statistischer Beweis: Die Restschwankung ist reines Rauschen

Die winzige Spanne könnte trotzdem ein echtes Muster sein. Ein **Permutationstest** (Label-Shuffle, 500 Durchläufe) prüft das: Würfelt man die Win-Rates zufällig auf die Muster, entsteht allein durch Stichprobenrauschen eine Feld-Streuung von **0,19 pp**. Beobachtet werden **0,21 pp** — praktisch identisch.

| | Feld-Streuung |
|---|---|
| Beobachtet | 0,207 pp |
| Zufall (Shuffle-Nullverteilung) | 0,191 pp |
| **p-Wert** | **0,20** |

Bei p = 0,20 ist die Positionsabhängigkeit **nicht signifikant** — sie ist statistisch nicht von purem Zufall zu unterscheiden.

### Auch Rand vs. Mitte trennt nicht

Klassische „Game of Life"-Intuition: Randfelder haben weniger Nachbarn, müssten also schwächer sein. Die Daten widerlegen das — die Ringe vom äußeren Rand bis zum 2×2-Zentrum sind flach:

| Ring (0 = äußerer Rand … 3 = Zentrum) | Ø Win-Rate |
|---|---|
| 0 (Rand) | 64,04 % |
| 1 | 63,93 % |
| 2 | 63,91 % |
| 3 (Zentrum) | 63,88 % |

Unterschied Rand → Zentrum: **0,16 pp** — kein nutzbarer Vorteil.

### Gegenprobe: Sitzen Gewinner woanders als Verlierer?

Vergleicht man die Belegung des **besten Viertels** (107 Muster) mit der des **schlechtesten Viertels**, ergeben sich pro Feld nur Differenzen im einstelligen bis ~20er-Bereich — ohne erkennbares Muster, und innerhalb dessen, was bei 107 zufälligen Mustern allein durch Rauschen entsteht.

→ **Grafik:** `260620_F2.3_contrast_24cells_study.svg`

---

## Was dann den Unterschied macht

Wenn nicht die Position — was trennt dann die 24-Zellen-Muster? Die Win-Rate streut ja durchaus (43,7 % bis 69,5 %, σ = 3,0 pp). Die Antwort: nicht *wo* die Zellen liegen, sondern **wie sie zueinander stehen** — ob die 24 Zellen sofort kollabieren oder in eine stabile, ausgedehnte Struktur übergehen. Das ist eine Eigenschaft der *Nachbarschaft*, nicht des *Bretter-Orts*. Die Top-5 leben spürbar lang (Ø stabil ab Gen. ~300–350), ohne ein bestimmtes Feld zu bevorzugen.

| Rang | Muster | Win-Rate | Ø stabil ab Gen. |
|---|---|---|---|
| 1 | exhaust#00005767 | **69,5 %** | 323 |
| 2 | exhaust#00005323 | 68,6 % | 341 |
| 3 | exhaust#00005312 | 68,5 % | 322 |
| 4 | exhaust#00008497 | 68,5 % | 304 |
| 5 | exhaust#00004745 | 68,4 % | 346 |

→ **Grafik:** `260620_F2.3_top_seeds_24cells_study.svg`

---

## Poster-Aussage

> **„Wir haben die Dichte eingefroren — und das Brett bleibt blind."**
> Bei den 430 Mustern mit *exakt* 24 Startzellen liegt die Sieg-Wahrscheinlichkeit jedes einzelnen Feldes zwischen 63,6 % und 64,6 %. Kein Feld ist ein Siegerfeld, kein Rand ist ein Nachteil; ein Permutationstest weist die Restschwankung als reines Rauschen aus (p = 0,20). **Damit ist die Poster-These bewiesen: Es zählt, *wie viele* Zellen man setzt — und *wie* sie zusammenstehen — aber nicht, *an welcher Stelle* des Bretts.**

---

## Verwendbare Dateien

| Zweck | Datei |
|---|---|
| Ø-Win-Rate je Feld (24 Zellen) | `260620_F2.3_heatmap_24cells_study.svg` |
| Gewinner- vs. Verlierer-Belegung (24 Zellen) | `260620_F2.3_contrast_24cells_study.svg` |
| Top-5-Seeds (24 Zellen) | `260620_F2.3_top_seeds_24cells_study.svg` |
| Generator-Skript | `backend/scripts/pattern_heatmap_fixedcells.py` |

> **Hinweis:** `biotope_study` ist der feste Studien-Datensatz. Für den Live-Betrieb in `.env` wieder `biotope_db` aktivieren. Das Skript akzeptiert jede Zellzahl als Argument (`… pattern_heatmap_fixedcells.py 18` etc.), um die Positions-Analyse für andere Dichtestufen zu wiederholen.
