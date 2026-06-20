# Poster für die Uni-Messe — Vorschlagsliste „Forschungsfragen"

**Projekt:** Biotop (kompetitives Conway's Game of Life)
**Stand:** 2026-06-19
**Zweck:** Sammlung möglicher Leitfragen für ein Messeposter. Zielpublikum: fachfremde, interessierte Laien und Uni-Angehörige. Dieses Dokument ist die Brainstorming-Grundlage; in einem nächsten Schritt werden 2–3 Fragen zu konkreten Poster-Sektionen ausgearbeitet.

---

## Gestaltungsprinzip

Ein gutes Messeposter lebt von **einer** großen Leitfrage plus 2–3 Unterfragen, die man in 30 Sekunden im Vorbeigehen erfassen kann und die einen Gesprächsaufhänger am Stand bieten. Ein Poster sollte nicht mehr als ~3 Fragen tragen.

---

## Richtung 1 — Programmierung: „Von handmade C zu Vibe-Code & verteilten Systemen"

Erfahrungsgemäß die für ein Uni-Publikum spannendste Achse, weil sie eine *Erzählung* hat (eine Person + KI baut ein überraschend großes System).

- **F1.1 „Wie weit kommt ein Erstsemester, wenn Mensch und KI als Team programmieren?"**
  Stark, weil ehrlich und persönlich. Zeigbar: Codezeilen-Statistik, die `// KI-Agent unterstützt`-Konvention, ein Vorher/Nachher-Beispiel. Reizt fachfremde *und* Lehrende zum Gespräch.

- **F1.2 „Warum schreibt man die Simulation in C — und das Drumherum in Python?"**
  Die „richtiges Werkzeug für die richtige Schicht"-Story: hot loop in C (O3, OpenMP, Ghost-Border, Double-Buffering) vs. Backend-Komfort in FastAPI. Gut für Performance-Zahlen (Generationen/Sekunde).

- **F1.3 „Vom Einzelspiel zum Turnier: Was macht ein System ‚verteilt'?"**
  Architekturdiagramm als Posterherz: GUI → Editor → Backend → Matchmaker → Hyper-Worker → DB. Erklärt Begriffe wie asynchron, Worker, Container/Docker an einem konkreten Beispiel.

- **F1.4 „Kann man KI-generiertem Code vertrauen — und wie prüft man das?"**
  Verbindet zu Test-/ADR-/Review-Kultur (Unit-Tests, „zero warnings", ADRs, code-review). Aktuelles Thema, das jeden interessiert.

## Richtung 2 — Game of Life als Thema

- **F2.1 „Komplexität aus vier Regeln: Wie entsteht ‚Leben' aus dem Nichts?"**
  Der Klassiker und visuell der dankbarste Aufhänger. Live-Kiosk am Stand = Selbstläufer. Begriffe: Emergenz, zelluläre Automaten.

- **F2.2 „Rot gegen Blau: Was ändert sich, wenn aus einer Simulation ein Wettkampf wird?"**
  Das eigentliche Alleinstellungsmerkmal (Biotop = kompetitives GoL). Frage: Gibt es eine „beste" Startformation, oder schlägt jede Strategie eine andere (Schere-Stein-Papier)?

- **F2.3 „Gibt es Muster, die (fast) immer gewinnen?"**
  Direkt aus den Turnierdaten/Elo-Ranking belegbar — echte Daten aus dem eigenen System statt Lehrbuch. Sehr posterfreundlich (Heatmap der 8×8-Startmuster, Top-Patterns).

- **F2.4 „Ist Game of Life Spielzeug oder Wissenschaft?"**
  Brücke zu Turing-Vollständigkeit, Selbstreplikation, Anwendungen. Gut für „warum sollte mich das interessieren?"-Besucher.

## Richtung 3 — Zusätzliche Vorschläge

- **F3.1 „Wie misst man Stärke fair? Schach-Mathematik (Elo) für Zellmuster."**
  Brückenthema, das fast jeder kennt (Schachzahlen) und das die O(N²)-Round-Robin-/Ranking-Logik zugänglich macht. Sehr gesprächsfreundlich.

- **F3.2 „Wie wird aus dem Publikum ein Mitspieler?"** *(Interaktions-/Mensch-Maschine-Fokus)*
  Stellt den Web-Editor + Kiosk-Leaderboard in den Mittelpunkt: Besucher malen am Stand ein Muster, es tritt im Live-Turnier an, Name erscheint im Ranking. **Potenziell stärkster Magnet** — ein Poster, das man *bedient*, nicht nur liest.

- **F3.3 „Was kostet Geschwindigkeit? Ein Rechenkern, viele Kerne (Parallelisierung)."**
  Greifbare Erklärung von OpenMP/Hyper-Worker: ein Turnier sequentiell vs. parallel. Mit echtem Speedup-Balken.

- **F3.4 „Wie entscheidet man als Team, wie Software gebaut wird?"** *(Meta/Prozess)*
  Die ADR-Kultur als Thema: Entscheidungen werden dokumentiert, nicht nur getroffen. Spricht besonders Lehrende/Methodiker an.

- **F3.5 „Simuliertes Ökosystem: Was hat ‚Biotop' mit echter Biologie zu tun?"**
  Interdisziplinärer Köder für fachfremde Besucher (Räuber-Beute, Stabilität, Dominanz) — bewusst metaphorisch gehalten.

---

## Empfehlung für die Auswahl

Ein Poster trägt nicht mehr als ~3 Fragen. Eine runde Kombination, die alle drei Richtungen mit je einer Frage abdeckt:

1. **F2.1** als visueller Einstieg (Emergenz, Live-Kiosk zieht Leute an),
2. **F3.2** als interaktiver Kern (Publikum spielt mit — Alleinstellungsmerkmal),
3. **F1.1** als persönliche Story (Erstsemester + KI baut verteiltes System — der „Wow, das habt *ihr* gemacht?"-Effekt).

Roter Faden: *Faszination → Mitmachen → Wie ist das entstanden?*

---

## Nächster Schritt

2–3 Fragen auswählen; je Frage ausarbeiten zu: Kernaussage, Visualisierung/Diagramm, 2–3 Stichpunkte, Gesprächsaufhänger für den Stand.
