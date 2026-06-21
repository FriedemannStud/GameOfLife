# BIOTOP – Conways Game of Life als Wettkampf
### Poster-Entwurf (druckfertige Vorlage)

**Projekt:** Biotop (kompetitives Conways Game of Life) · **Kurs:** WIAI25
**Stand:** 2026-06-21
**Grundlage:** [../260619_Poster_Forschungsfragen_Vorschlaege.md](../260619_Poster_Forschungsfragen_Vorschlaege.md) · [../260619_Poster_Sektionen_Ausarbeitung.md](../260619_Poster_Sektionen_Ausarbeitung.md)
**Richtlinie:** Uni Bamberg, *„Poster wirkungsvoll gestalten – Weniger ist mehr!"* (siehe `../Poster_wirkungsvoll_gestalten.pdf`)

> Roter Faden: **Faszination → Mitmachen → Wie ist das entstanden?**
> Drei Forschungsfragen: **F2.1** (Einstieg) · **F3.2** (interaktiver Kern) · **F1.1** (persönliche Story).

---

## 0 · Design-Vorgaben (verbindlich, aus der Uni-Richtlinie)

| Vorgabe | Umsetzung im Entwurf |
|---|---|
| **Format** | **DIN A0 hochkant** (841 × 1189 mm). |
| **„Weniger ist mehr"** | Pro Sektion *eine* Headline + *ein* Kernsatz + max. 3 Stichpunkte. Kein Fließtext. |
| **Schriftgrößen (Minimum)** | Titel **78 pt fett** · Autor **72 pt fett** · Zwischenüberschriften **36 pt** · Text **36 pt** · Bildunterschriften/Quellen kleiner erlaubt. |
| **Schriftart** | serifenlos: **Arial** oder **Tahoma** (konsistent durchgängig). |
| **„Klar"** | Wichtigstes **ins Zentrum** → die Mitmach-Station (F3.2) steht in der Poster-Mitte, nicht unten. |
| **Kontrast** | kräftige Farben, hochauflösende Vektorgrafiken (alle Diagramme als SVG). |
| **Lesbarkeit** | Inhalt aus **1–2 m** klar erkennbar. |

**Farb-Palette (konsistent über alle Grafiken):**
Team Rot `#D7263D` · Team Blau `#1B6CA8` · Aktion/„Los" Grün `#0F8B6C` · Text `#1A1A1A` · neutrale Zelle `#2E3440` · tote Zelle `#EAEEF4`.

### Anpassung gegenüber dem Vorschlag (bewusste Entscheidung)
Der Vorschlag skizzierte ein **Querformat** mit drei Spalten (links/Mitte/rechts).
Die Uni-Richtlinie verlangt jedoch **A0 hochkant**. Deshalb wird der rote Faden hier
**vertikal von oben nach unten** geführt (Faszination oben → Mitmachen Mitte → Story unten).
Das erfüllt zugleich die Regel *„Wichtigstes ins Zentrum"*: die interaktive Mitmach-Station
(F3.2, das Alleinstellungsmerkmal) sitzt in der optischen Poster-Mitte.

---

## 1 · Layout-Skizze (A0 hochkant)

```
╔══════════════════════════════════════════════════════════╗
║   B I O T O P  –  Game of Life als Wettkampf      (78 pt)  ║  Titel
║   Erstsemester: Friedemann Decker + KI-Agenten: Gemini CLI, Claude CLI  (72 pt)  ║  Autor
╠══════════════════════════════════════════════════════════╣
║  EINSTIEG · Worum geht es?                          (36pt) ║  kurze Einführung
║  Rot gegen Blau: zwei Teams kämpfen um das Spielfeld.      ║  (Uni-Pflichtinhalt)
╠══════════════════════════════════════════════════════════╣
║  ① F2.1  FASZINATION                                       ║
║     „Komplexität aus vier Regeln"                          ║
║     [ f21_vier_regeln.svg ]                                ║
║     [ f21_gleiter_sequenz.svg ]   [ kiosk_live.png ]       ║  ← Live-Kiosk daneben
╠══════════════════════════════════════════════════════════╣
║  ② F3.2  MITMACHEN   ◄── ZENTRUM, größte Fläche            ║
║     „Werde vom Zuschauer zum Mitspieler"                   ║
║     [ f32_mitmach_ablauf.svg ]                             ║
║     [ qr_editor.png ]   [ editor_tablet.png ] [ leaderboard.png ]
╠══════════════════════════════════════════════════════════╣
║  ◊ Daten-Einschub (schmaler Streifen)                     ║
║     „Wie viele Zellen gewinnen? – Dichte-Optimum"         ║
║     [ f23_winrate_vs_cells.svg ]  (umgekehrte U-Kurve)     ║
╠══════════════════════════════════════════════════════════╣
║  ③ F1.1  ENTSTEHUNG                                        ║
║     „Wie weit kommt ein Erstsemester mit KI?"              ║
║     [ f11_architektur_3schichten.svg ]                     ║
║     [ f11_kennzahlen.svg ]      [ code_vignette.png ]      ║
╠══════════════════════════════════════════════════════════╣
║  Fußzeile: Namen · WIAI25 · QR zum Projekt        (klein)  ║
╚══════════════════════════════════════════════════════════╝
```

---

## 2 · Kopf des Posters (Uni-Pflichtinhalt: Einführung + Fragestellung)

**Titel (78 pt fett):** BIOTOP – Conways Game of Life als Wettkampf
**Autorzeile (72 pt fett):** *[Name(n)]* · 1. Semester · Kurs WIAI25

**Kurze Einführung (ein Satz, 36 pt):**
> *Conways Game of Life* erzeugt aus vier einfachen Regeln verblüffend komplexe Muster. **Biotop** macht
> daraus einen Wettkampf: **Team Rot und Team Blau** kämpfen auf einem Gitter um die Vorherrschaft.

**Leitfrage des Posters (eine, prägnant):**
> *Wie entsteht aus vier Regeln ein faszinierendes Spiel?*

---

## 3 · Sektion ① – F2.1 · FASZINATION
### „Komplexität aus vier Regeln: Wie entsteht Ordnung ganz von selbst?"

**Rolle:** visueller Einstieg, zieht Besucher an – funktioniert fast ohne Text.

**Kernaussage (Headline + ein Satz):**
> Vier simple Regeln – und trotzdem entstehen Gleiter, Oszillatoren und Chaos. Das nennt man **Emergenz**.

**Visualisierung:**
- `f21_vier_regeln.svg` – die vier Regeln als **Vorher → Nachher**-Icons (Überleben · Einsamkeit · Überbevölkerung · Geburt), mit hervorgehobener Fokuszelle.
- **Live-Element:** Foto/Screenshot des laufenden Kiosk daneben → `kiosk_live.png`. Das bewegte Bild am Stand ist stärker als jedes gedruckte Diagramm.

**Stichpunkte (max. 3):**
- **Emergenz:** komplexes Verhalten aus einfachen lokalen Regeln, ohne zentrale Steuerung.
- **Zellulärer Automat:** jede Zelle „schaut" nur auf ihre 8 Nachbarn – mehr nicht.
- Erfunden 1970 von John Conway; bis heute Forschungsgegenstand (sogar **Turing-vollständig** – man kann darin „rechnen").

**Gesprächsaufhänger für den Stand:**
- „Erkennen Sie, welche dieser Formen sich *bewegt* und welche an Ort und Stelle *blinkt*?"


---

## 4 · Sektion ② – F3.2 · MITMACHEN  *(Zentrum des Posters)*
### „Erschaffen Sie das stärkste Biotop!"

**Rolle:** interaktiver Kern und Alleinstellungsmerkmal – eine **Anleitung zum Mitmachen**, kein Erklärtext. Diese Sektion bekommt die größte Fläche und steht in der Poster-Mitte.

**Kernaussage:**
> Malen Sie in **30 Sekunden** Ihr eigenes 8×8-Muster – es tritt sofort im Live-Turnier an,
> und Ihr Name erscheint in der Rangliste.

**Visualisierung:**
- `f32_mitmach_ablauf.svg` – der 4-Schritt-Ablauf **Malen → Absenden → Kämpfen → Ranken**, darunter die vereinfachte Pipeline `Editor → Backend → Matchmaker → Hyper-Worker → Leaderboard`.
- `qr_editor.png` – großer **QR-Code / Pfeil zum Editor-Tablet** am Stand.
- `editor_tablet.png` – Screenshot des Editors mit gezeichnetem 8×8-Muster.
- `leaderboard.png` – Screenshot des **Kiosk-Leaderboards** mit Beispielnamen.

**Stichpunkte (max. 3):**

**Gesprächsaufhänger für den Stand:**
- „Trauen Sie sich zu, das aktuelle **Top-Muster** zu schlagen?"

---

## 5 · Daten-Einschub ◊ – „Wie dicht sollte ein Startmuster sein?" *(schmaler Streifen, echte Turnierdaten)*
### Möglicherweise gibt es ein **Dichte-Optimum**

**Rolle:** echte Daten aus dem eigenen System (nicht Lehrbuch) – belegt die Strategie-Frage aus F3.2.

**Kernaussage:**
> Weder fast leer noch randvoll: Startmuster mit **24–32 von 64 Zellen** gewinnen am häufigsten.

**Visualisierung:**
- `f23_winrate_vs_cells.svg` – die **Win-Rate über die Startzellzahl** als Kurve: eine klare
  **umgekehrte U-Form** mit hervorgehobenem Sweet-Spot-Band (24–32 Zellen) und Peak-Markierung.

**Stichpunkte (max. 3):**
**Zahlen-Einschub (Frage 2 aus Zwischenbericht #2):**
> Wie viele verschiedene Muster gibt es unter der Fair-Play-Grenze (max. 24 Zellen)?
> Σ C(64,k) für k=0..24 ≈ **553 Billiarden** – über 25-mal so viele, wie es Ameisen auf der ganzen Erde gibt.
> Stures Durchprobieren (Brute Force) chancenlos.

**Gesprächsaufhänger für den Stand:**


> **Warum diese Aussage präziser ist:** Die frühere Auswertung (Studie 2–24 Zellen) sah nur den
> *aufsteigenden Ast* der Kurve und legte „voller ist nicht besser" nahe. Die Vollbereichs-Studie
> (2–64) zeigt: der Trend **kippt** ab ~⅓ Belegung – es gibt ein Optimum, kein „je lockerer, desto besser".
>
> **Methoden-/Aktualitäts-Hinweis:** Quelle ist die Studien-Datenbank `biotope_study_uniform`
> (10.000 gleichverteilte Zufallsmuster, eigenes Turnier). Kurve neu erzeugen mit
> `python3 docs/Präsentation/poster_entwurf/make_winrate_curve.py` (liest die DB direkt).

---

## 6 · Sektion ③ – F1.1 · ENTSTEHUNG
### „Wie weit kommt ein Erstsemester, wenn Mensch und KI als Team programmieren?"

**Rolle:** die persönliche Story und der „Wow, das habt *ihr* gemacht?"-Effekt – schließt den roten Faden.

**Kernaussage:**
> Hinter BIOTOP steckt ein komplettes **verteiltes System**.

**Visualisierung:**
- `f11_architektur_3schichten.svg` – die Architektur für Laien entschärft, drei farbige Schichten:
  *Mitmachen (Web)* · *Server (Python)* · *Rechenkern (C)*.
- `f11_kennzahlen.svg` – die Kennzahl-Leiste (siehe unten).
- `code_vignette.png` – kleine **Code-Vignette** mit der projektweiten Konvention `// KI-Agent unterstützt` (macht die Mensch-KI-Zusammenarbeit sichtbar).

**Kennzahl-Leiste (aus dem Repo gezählt, Stand 2026-06-20):**

| Kennzahl | Wert |
|---|---|
| Zeilen eigener Code | **≈ 13.300** *(C ≈ 4.400 · Python ≈ 3.100 · Web ≈ 5.800)* |
| native C-Programme | **3** (GUI · Headless · Hyper-Worker) |
| automatisierte Tests | **17** (6 in C, 11 in Python) |
| dokumentierte Architektur-Entscheidungen (ADRs) | **35** |

*Hinweis: die eingebettete Bibliothek cJSON (~3.500 Zeilen) ist bewusst **nicht** mitgezählt.*

**Stichpunkte (max. 3):**
- **Werkzeug für jede Schicht:** Geschwindigkeit in C, Komfort in Python, Zugänglichkeit im Web.
- **KI als Teampartner, nicht Autopilot:** Entscheidungen werden in Architektur-Notizen (ADRs) begründet, Code wird getestet (Ziel: **null Compiler-Warnungen**).
- **Verteiltes System:** mehrere Programme laufen gleichzeitig in Containern (Docker) und reden übers Netz.



---

## 7 · Asset-Manifest

### Vektorgrafiken – generiert (in diesem Ordner)
Erzeugt durch `python3 make_poster_svgs.py` (Gitter/Gleiter mit echter Conway-Logik berechnet):

| Datei | Sektion | Inhalt |
|---|---|---|
| `f21_vier_regeln.svg` | F2.1 | vier Regeln als Vorher→Nachher-Icons |
| `f32_mitmach_ablauf.svg` | F3.2 | 4-Schritt-Ablauf + Pipeline |
| `f23_winrate_vs_cells.svg` | Daten-Einschub | Win-Rate-vs-Zellzahl-Kurve (Dichte-Optimum) – erzeugt von `make_winrate_curve.py` aus `biotope_study_uniform` |
| `f11_architektur_3schichten.svg` | F1.1 | 3-Schichten-Architektur |
| `f11_kennzahlen.svg` | F1.1 | Kennzahl-Leiste |

### Snapshots – vom Entwickler erstellt ✅ (geliefert 2026-06-20)

| Dateiname | Sektion | Gelieferter Inhalt | Maße | Druck-Hinweis |
|---|---|---|---|---|
| `kiosk_live.png` | F2.1 | Multicam 2×2 „LIVE BATTLES", kräftige Rot/Blau-Fronten | 1302×868 | sehr gut |
| `editor_tablet.png` | F3.2 | Editor mit Muster (22/24 Zellen, Team Blau), Name *Pumati*, „AB IN DIE ARENA" | 539×842 | bei kleiner/mittlerer Platzierung gut; nicht großflächig drucken |
| `leaderboard.png` | F3.2 | Global Leaderboard, deckt sich mit der F2.3-Tabelle | 1292×788 | gut |
| `qr_editor.png` | F3.2 | gültiger QR-Code | 477×477 | dank Fehlerkorrektur problemlos skalierbar |
| `code_vignette.png` | F1.1 | `// KI-Agent unterstützt` über `init_world` (TEAM_RED/BLUE/DEAD) | 606×434 | bei kleiner/mittlerer Platzierung gut |

> **Optional vor dem Finaldruck:** `editor_tablet.png` und `code_vignette.png` könnten für sehr
> großflächige Platzierung höher aufgelöst neu aufgenommen werden – bei der im Layout vorgesehenen
> mittleren Größe sind sie aus 1–2 m aber unkritisch.

### Zusammengesetzter Druck-Proof
`poster_a0.html` – fügt alle SVGs + PNGs im finalen A0-Hochkant-Layout zusammen (Arial,
Schriftgrößen ≥ Uni-Mindestwerte, F3.2 im Zentrum). **Im Browser öffnen → Drucken →
„Als PDF speichern", Papier A0 hochkant, Ränder: keine, Hintergrundgrafiken: an.**

---

## 8 · Druck-Checkliste

- [ ] A0 **hochkant**, serifenlose Schrift durchgängig (Arial/Tahoma).
- [ ] Schriftgrößen ≥ Minimum (Titel 78 / Autor 72 / Überschriften & Text 36 pt).
- [ ] Autorzeile mit echten Namen gefüllt (Platzhalter `[Name(n)]` in `poster.md` **und** `poster_a0.html` ersetzt).
- [ ] F2.3-Daten frisch gezogen (Tabelle + Heatmap-SVGs) und Zahlen geprüft.
- [ ] Kennzahl-Leiste vor Druck noch einmal aus dem Repo verifiziert (LOC/Tests/ADRs).
- [x] Alle 5 PNG-Snapshots erstellt (2026-06-20).
- [ ] `poster_a0.html` im Browser geprüft (passt auf eine A0-Seite? sonst Layout leicht straffen).
- [ ] QR-Code getestet (führt im Stand-LAN wirklich zum Editor).
- [ ] Kontrast aus 1–2 m geprüft (Probedruck A4 als Mini-Test).
