# Poster-Sektionen — Ausarbeitung F2.1 / F3.2 / F1.1

**Projekt:** Biotop (kompetitives Conway's Game of Life)
**Stand:** 2026-06-19
**Grundlage:** [260619_Poster_Forschungsfragen_Vorschlaege.md](260619_Poster_Forschungsfragen_Vorschlaege.md)
**Zielpublikum:** fachfremde, interessierte Laien und Uni-Angehörige

Roter Faden des Posters: **Faszination → Mitmachen → Wie ist das entstanden?**
Leserichtung: links der Hingucker (F2.1), Mitte die Mitmach-Station (F3.2), rechts die Story (F1.1).

---

## Sektion 1 (links) — F2.1
### „Komplexität aus vier Regeln: Wie entsteht ‚Leben' aus dem Nichts?"

**Rolle im Poster:** visueller Einstieg, zieht Besucher an. Sollte fast ohne Text funktionieren.

**Kernaussage (eine Headline + ein Satz):**
> Vier simple Regeln, kein Plan, kein Steuermann — und trotzdem entstehen Gleiter, Oszillatoren und Chaos. Das nennt man *Emergenz*.

**Die vier Regeln (als Icon-Reihe, nicht als Fließtext):**
1. Eine lebende Zelle mit **2 oder 3** Nachbarn überlebt.
2. Mit **weniger als 2** stirbt sie (Einsamkeit).
3. Mit **mehr als 3** stirbt sie (Überbevölkerung).
4. Eine tote Zelle mit **genau 3** Nachbarn wird geboren.

**Visualisierung:**
- Großes 3×3-Schema, das eine Regel Schritt für Schritt zeigt (Vorher → Nachher).
- Eine kleine Bildfolge eines **Gleiters** (5 Generationen nebeneinander), der über das Gitter „wandert" — der klassische Aha-Effekt.
- **Live-Element:** der laufende Kiosk-Bildschirm direkt neben dem Poster. Das bewegte Bild ist hier stärker als jedes gedruckte Diagramm.

**Stichpunkte (max. 3):**
- *Emergenz:* komplexes Verhalten aus einfachen lokalen Regeln, ohne zentrale Steuerung.
- *Zellulärer Automat:* jede Zelle „schaut" nur auf ihre 8 Nachbarn — mehr nicht.
- Erfunden 1970 von John Conway; bis heute Forschungsgegenstand (sogar Turing-vollständig — man kann darin „rechnen").

**Gesprächsaufhänger für den Stand:**
- „Erkennen Sie, welche dieser vier Formen sich bewegt und welche an Ort und Stelle blinkt?"
- „Niemand hat dem Gleiter gesagt, dass er laufen soll — er *entsteht* aus den Regeln. Wo kennen Sie so etwas aus der Natur?"

---

## Sektion 2 (Mitte) — F3.2
### „Wie wird aus dem Publikum ein Mitspieler?"

**Rolle im Poster:** interaktiver Kern und Alleinstellungsmerkmal. Diese Sektion ist eine *Anleitung zum Mitmachen*, kein Erklärtext.

**Kernaussage:**
> Malen Sie in 30 Sekunden Ihr eigenes 8×8-Muster — es tritt sofort im Live-Turnier an, und Ihr Name erscheint im Ranking.

**Der Mitmach-Ablauf (als nummerierte Schritte mit Pfeilen):**
1. **Malen** — am Tablet/Editor ein 8×8-Startmuster zeichnen (Team Rot oder Blau).
2. **Absenden** — das Muster geht an den Server (`POST /api/v1/submit_config`).
3. **Kämpfen** — der Matchmaker lässt es im Hintergrund gegen alle anderen Muster antreten.
4. **Ranken** — Ergebnis fließt in die Elo-Wertung; der eigene Name taucht auf dem Leaderboard auf.

**Visualisierung:**
- Großer, klarer **QR-Code / Pfeil zum Editor-Tablet** am Stand.
- Mini-Architekturpfeil, der den Weg eines Musters zeigt: `Editor → Backend → Matchmaker → Hyper-Worker → Leaderboard`. Bewusst stark vereinfacht (Details kommen in Sektion 3).
- Foto/Screenshot des **Kiosk-Leaderboards** mit Beispielnamen.

**Stichpunkte (max. 3):**
- Das Poster ist hier eine **Station, kein Plakat** — man bedient es.
- Jedes eingereichte Muster wird *automatisch* und *fair* gegen alle bewertet — niemand muss zuschauen, bis das Match fertig ist (asynchron).
- Rückkehrende Besucher können denselben Namen erneut beanspruchen und ihren Platz verteidigen.

**Gesprächsaufhänger für den Stand:**
- „Trauen Sie sich zu, das aktuelle Top-Muster zu schlagen?"
- „Was glauben Sie — gewinnt eher ein dichter Klumpen oder ein lockeres Muster?" (überleitend zu F2.3 / Strategie)

---

## Sektion 3 (rechts) — F1.1
### „Wie weit kommt ein Erstsemester, wenn Mensch und KI als Team programmieren?"

**Rolle im Poster:** die persönliche Story und der „Wow, das habt *ihr* gemacht?"-Effekt. Schließt den roten Faden ab.

**Kernaussage:**
> Hinter der Spielerei steckt ein komplettes verteiltes System — gebaut im 1. Semester, im Team aus Mensch und KI-Agent.

**Visualisierung:**
- Das **echte Architekturdiagramm** (aus `docs/260530_ARCHITECTURE_DIAGRAM.md`), aber für Laien entschärft: drei Schichten farbig.
  - *Rechenkern (C):* GUI · Headless · Hyper-Worker
  - *Server (Python):* FastAPI · Matchmaker-Worker · MongoDB
  - *Mitmachen (Web):* Editor · Kiosk
- Eine kleine **Code-Vignette** mit der projektweiten Konvention `// KI-Agent unterstützt` — macht die Mensch-KI-Zusammenarbeit *sichtbar*.
- Eine **Kennzahl-Leiste** (Stand 2026-06-19, aus dem Repo gezählt):
  - **≈ 11.500 Zeilen eigener Code** (C ~4.200 · Python ~1.700 · Web ~5.700)
  - **3 native C-Programme** (GUI · Headless · Hyper-Worker)
  - **14 automatisierte Tests** (5 in C, 9 in Python)
  - **31 dokumentierte Architektur-Entscheidungen** (ADRs)
  - *Hinweis: die eingebettete Bibliothek cJSON (~3.500 Zeilen) ist hier bewusst nicht mitgezählt.*

**Stichpunkte (max. 3):**
- *Werkzeug für jede Schicht:* Geschwindigkeit in C, Komfort in Python, Zugänglichkeit im Web.
- *KI als Teampartner, nicht als Autopilot:* jede KI-unterstützte Stelle ist im Code markiert; Entscheidungen werden in ADRs begründet, Code wird getestet (Ziel: null Compiler-Warnungen).
- *Verteiltes System:* mehrere Programme laufen gleichzeitig in Containern (Docker) und reden über das Netz miteinander.

**Gesprächsaufhänger für den Stand:**
- „Welcher Teil ist wohl von Hand geschrieben, welcher mit KI entstanden?"
- „Würden Sie KI-generiertem Code vertrauen? Wir zeigen, wie wir ihn prüfen." (Brücke zu F1.4, falls vertieft gewünscht)

---

## Layout-Skizze (Querformat A0)

```
+-----------------------+-----------------------+-----------------------+
|   F2.1  FASZINATION    |   F3.2  MITMACHEN      |   F1.1  ENTSTEHUNG     |
|                        |                        |                        |
| 4 Regeln als Icons     | 1-2-3-4 Mitmach-Ablauf | 3-Schichten-Diagramm   |
| Gleiter-Bildfolge      | QR/Pfeil zum Editor    | // KI-Agent unterstützt|
| [Live-Kiosk daneben]   | Leaderboard-Foto       | Kennzahlen-Leiste      |
|                        |                        |                        |
| "Leben aus 4 Regeln"   | "Werde Mitspieler"     | "1. Semester + KI"     |
+-----------------------+-----------------------+-----------------------+
   Titelzeile oben durchgehend:  BIOTOP — Game of Life als Wettkampf
   Fußzeile: Namen · Kurs (WIAI25) · QR zum Projekt
```

---

## Daten-Einschub F2.3 — „Gibt es Muster, die (fast) immer gewinnen?"

**Echte Turnierdaten aus der laufenden MongoDB (Stand 2026-06-19, 29 gewertete Einreichungen).**
Quelle/Reproduktion: `python3 backend/scripts/top_patterns.py` (gleiche Filter/Sortierung wie `GET /api/leaderboard`).

**Top 5 Start­muster (8×8, █ = lebende Zelle):**

| Rang | Name | Win-Rate | Bilanz | Ø-stabile Gen. | Zellen |
|---|---|---|---|---|---|
| 1 | Schlanke Gazelle | 76,8 % | 16-11-1 | 140,6 | 17/64 |
| 2 | Mose | 73,2 % | 16-9-3 | 323,1 | 16/64 |
| 3 | Wilde Hilde | 71,4 % | 17-6-5 | 428,4 | 14/64 |
| 4 | Brummender Brauner | 71,4 % | 17-6-5 | 529,0 | 18/64 |
| 5 | Mose | 69,6 % | 18-3-7 | 191,2 | 8/64 |

```
   #1 Schl. Gazelle   #3 Wilde Hilde      #5 Mose
   ····█···           █·█·█·█·            ········
   ·██··█··           ·█·█·█··            ········
   ·██·█···           ··█·█···            ···███··
   ···█····           ···█····            ··█···█·
   █·····██           ···█·█··            ···███··
   ·█······           ···██···            ········
   ··█···█·           ········            ········
   ··█·██··           ········            ········
```

**Auswertbare Beobachtungen fürs Poster (laienverständlich):**
- *„Voller ist nicht besser":* die Top-Muster nutzen nur **8–18 von 64** Feldern — kein Spitzenmuster ist dicht gefüllt. Ein lockeres Muster schlägt den Klumpen.
- *Zwei Wege zum Sieg:* Platz 1 stabilisiert sich **früh** (~141 Gen.), Platz 4 lebt **lange** (~529 Gen.) — unterschiedliche Strategien führen zu ähnlich hohen Quoten.
- *Selbst ein Mini-Muster gewinnt:* Platz 5 hat nur **8 Zellen** und liegt trotzdem bei ~70 %.
- Aufhänger am Stand: „Welches dieser fünf würden *Sie* tippen — und warum gewinnt das schlanke gegen das volle?"

**Druckfertige Grafiken (vektoriell, skalieren verlustfrei auf A0):**
- `260619_F2.3_heatmap_db.svg` — **aggregierte Heatmap** (Menschen-Daten): welche Felder Gewinner bevorzugt belegen (win-rate-gewichtet über alle 29 Einreichungen). Deutliche Hotspots in der oberen Bildmitte → es gibt „gute Gegenden" auf dem Brett.
- `260619_F2.3_top_seeds_db.svg` — die **Top-5-Seed-Kacheln** als saubere 8×8-Grids mit Name + Win-Rate.
- Erzeugt mit `python3 backend/scripts/pattern_heatmap.py [N]` (Dateiname trägt den DB-Tag `_db` / `_study`).
- **Vergleich Mensch vs. Zufall:** siehe [260619_F2.3_Datenvergleich_db_vs_study.md](260619_F2.3_Datenvergleich_db_vs_study.md) — die zwei Heatmaps nebeneinander sind ein starker eigener Hingucker (Struktur vs. Rauschen).

> **Hinweis zur Aktualität:** Die Daten ändern sich mit jeder Turnier-Epoche (Worker läuft alle 60 s). Vor dem Druck **beide Skripte erneut laufen lassen** (`top_patterns.py` für die Tabelle, `pattern_heatmap.py` für die SVGs) und Tabelle/Grafiken aktualisieren.

---

## Offene Punkte für die nächste Iteration

- ~~Konkrete **Kennzahlen** für Sektion 3 ermitteln~~ ✅ erledigt 2026-06-19 (siehe Kennzahl-Leiste). Vor dem Druck ggf. aktualisieren, falls noch Code dazukommt.
- ~~Entscheiden, ob **F2.3** als Daten-Einschub mitläuft~~ ✅ Daten gezogen & Einschub erstellt (2026-06-19). Noch offen: Platzierung im Layout (Vorschlag: schmaler Streifen unter Sektion 1 oder 2) und ob als Tabelle oder grafische Heatmap gedruckt.
- Hingucker-Bild für den Gleiter final auswählen (gedruckte Bildfolge vs. nur Live-Kiosk).
- Tablet/QR-Logistik am Stand klären (WLAN, Editor-URL im LAN — siehe `docs/MOBILE_LAN_TESTING.md`).
