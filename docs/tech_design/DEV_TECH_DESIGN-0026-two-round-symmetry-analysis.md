# Technical Design / Analyse: Symmetrie-Untersuchung der Zwei-Runden-Regel

**Version:** 1.0
**Date:** 2026-06-01
**Author:** Friedemann Decker (KI-Agent unterstützt)
**Related Documents:** ADR-0026 (geplant), [ADR-0012](../adr/ADR-0012-matchmaking-and-tournament-architecture.md), [ADR-0008](../adr/ADR-0008-epic-scale-tournament-architecture.md)

---

## 1. Fragestellung

Bei der Konzeption von „Biotop" wurde festgelegt, dass ein **Match aus zwei Runden** besteht:

- **Runde 1:** Konfiguration `P_A` liegt links, `P_B` liegt rechts.
- **Runde 2:** Die Seiten werden getauscht — `P_A` rechts, `P_B` links.

Die ursprüngliche Begründung lautete: *Durch die unterschiedliche Position entstünden unterschiedliche Ausgangssituationen und damit potenziell unterschiedliche Siegchancen.*

Die zu klärende Frage, präzise formuliert:

> Zwei Spieler erstellen je eine Start-Konfiguration auf einem 8×8-Raster. Beide 8×8-Konfigurationen werden „nebeneinandergelegt" und zu einem **torusförmigen** 8×16-Spielfeld zusammengefügt. **Gibt es unterschiedliche Siegchancen, abhängig davon, ob eine Konfiguration links oder rechts liegt?**
>
> - Falls **ja** → die zwei Runden müssen bleiben.
> - Falls **nein** → die zweite Runde ist überflüssig.

Dieses Dokument beantwortet die Frage mit den Mitteln der **Diskreten Modellierung** (zellulärer Automat als diskretes dynamisches System, Symmetriegruppen, Äquivarianz, Invarianten) und leitet daraus eine Empfehlung ab.

**Ergebnis vorab:** Die Siegchancen sind **positionsunabhängig**. Die zweite Runde ist mathematisch redundant — sie verdoppelt jede Statistik exakt, kann aber kein Ranking verändern. Diese Aussage ist beweisbar und gilt, **solange** drei konkrete Voraussetzungen erfüllt sind (siehe §7).

---

## 2. Formales Modell

Wir modellieren eine Match-Runde als deterministischen **zellulären Automaten** auf einem Torus.

### 2.1. Konfigurationsraum

Das Spielfeld besteht aus `R = 8` Zeilen und `C = 16` Spalten. Da `sync_ghost_borders` (in `src/core/game_logic.c:59`) **alle vier Ränder** zyklisch verbindet (oben↔unten, links↔rechts, inkl. der Ecken), ist die Zellmenge ein diskreter Torus:

```
V = ℤ₈ × ℤ₁₆            (Zeilen mod 8, Spalten mod 16)
```

Jede Zelle trägt einen Zustand aus dem Alphabet

```
Σ = { DEAD, RED, BLUE }     (im Code: 0, 1, 2)
```

Eine **Konfiguration** ist eine Abbildung `s: V → Σ`. Der Konfigurationsraum ist `S = Σ^V`.

### 2.2. Nachbarschaft

Es gilt die **Moore-Nachbarschaft** (8 Nachbarn). Für `v ∈ V`:

```
N(v) = { v + d : d ∈ {-1,0,1}² \ {(0,0)} }     (Addition komponentenweise mod (8,16))
```

Entscheidend: Auf dem Torus ist `N(v) = v + N(0)` für **jede** Zelle `v` — es gibt keinen Rand, an dem die Nachbarschaft „abgeschnitten" würde. Diese Homogenität (Translations-Symmetrie der Zellmenge) ist später die tragende Eigenschaft.

### 2.3. Übergangsfunktion Φ

Die Regel (`update_generation`, `src/core/game_logic.c:161-167`) ist eine globale Abbildung `Φ: S → S`. Für eine Konfiguration `s` und eine Zelle `v` definieren wir die Nachbar-Zählungen

```
ρ(v) = #{ u ∈ N(v) : s(u) = RED }       (rote Nachbarn)
β(v) = #{ u ∈ N(v) : s(u) = BLUE }      (blaue Nachbarn)
n(v) = ρ(v) + β(v)                       (lebende Nachbarn gesamt)
```

Dann ist `Φ(s)(v)` definiert als:

```
Falls s(v) ≠ DEAD  (Überleben):
    Φ(s)(v) = s(v)        wenn n(v) ∈ {2, 3}     (Farbe bleibt erhalten)
    Φ(s)(v) = DEAD        sonst

Falls s(v) = DEAD  (Geburt):
    Φ(s)(v) = ( ρ(v) > β(v) ? RED : BLUE )   wenn n(v) = 3
    Φ(s)(v) = DEAD                            sonst
```

Das ist exakt der Code. Zwei Merkmale sind für die Analyse zentral:

1. **Überlebensregel:** Hängt nur von `n(v)` ab (farbblind), die überlebende Zelle behält ihre Farbe.
2. **Geburtsregel:** Eine Geburt erfordert **genau `n(v) = 3`** lebende Nachbarn. Die Farbe des Neugeborenen ist die Mehrheitsfarbe.

---

## 3. Die beiden Runden als eine Transformation

### 3.1. Zwei Symmetrie-Operatoren

**Verschiebung (Shift).** Für eine Verschiebung `δ ∈ ℤ₈ × ℤ₁₆` definieren wir `σ_δ: S → S` durch

```
(σ_δ s)(v) = s(v − δ)
```

Uns interessiert die horizontale Halb-Verschiebung um 8 Spalten: `δ* = (0, 8)`. Da `2 · 8 = 16 ≡ 0 (mod 16)`, ist `σ_δ*` eine **Involution**: zweimal angewandt ergibt sich die Identität (eine halbe Torus-Umrundung hin und zurück).

**Farbtausch (Color Swap).** Sei `κ: Σ → Σ` mit `κ(DEAD)=DEAD`, `κ(RED)=BLUE`, `κ(BLUE)=RED`. Punktweise auf Konfigurationen fortgesetzt ergibt das `χ: S → S` mit `(χ s)(v) = κ(s(v))`. Auch `χ` ist eine Involution.

**Die kombinierte Transformation.** Wir definieren

```
T = χ ∘ σ_δ*     (erst um 8 Spalten verschieben, dann Farben tauschen)
```

`σ_δ*` wirkt auf Positionen, `χ` auf Werte — sie kommutieren, also `T = χ ∘ σ_δ* = σ_δ* ∘ χ`. Auch `T` ist eine Involution (`T² = id`).

### 3.2. Behauptung A — die Startkonfigurationen sind T-verwandt

Im Code (`run_isolated_match`, `src/core/game_logic.c:210-230`) gilt die feste Konvention: **das linke 8×8-Pattern wird RED, das rechte BLUE.** Runde 2 entsteht dadurch, dass `main_hyper.c:70-74` die Argumente vertauscht:

```c
// Match 1: i (Left=RED) vs j (Right=BLUE)
MatchResult res1 = run_isolated_match(competitors[i].cells, competitors[j].cells, ...);
// Match 2: j (Left=RED) vs i (Right=BLUE)
MatchResult res2 = run_isolated_match(competitors[j].cells, competitors[i].cells, ...);
```

Daraus folgt: Beim Seitentausch werden **Position UND Farbe gemeinsam** getauscht.

- **`s₁`** (Runde 1): `P_A` als RED in Spalten 0–7, `P_B` als BLUE in Spalten 8–15.
- **`s₂`** (Runde 2): `P_B` als RED in Spalten 0–7, `P_A` als BLUE in Spalten 8–15.

> **Behauptung A:** `s₂ = T(s₁)`.

**Beweis.** Wir berechnen `T(s₁) = χ(σ_δ*(s₁))`.
- `σ_δ*` verschiebt jede Spalte `c` nach `c + 8 (mod 16)` unter Beibehaltung der inneren Pattern-Orientierung:
  - `P_A` (Spalten 0–7, RED) → Spalten 8–15, weiterhin RED.
  - `P_B` (Spalten 8–15, BLUE) → Spalten 16–23 ≡ 0–7, weiterhin BLUE.
- `χ` tauscht anschließend die Farben:
  - Spalten 8–15: `P_A`, jetzt BLUE.
  - Spalten 0–7: `P_B`, jetzt RED.

Das ist Zelle für Zelle genau `s₂`. ∎

Damit ist Runde 2 **keine neue Ausgangssituation**, sondern das Bild von Runde 1 unter der Transformation `T`. Es bleibt zu zeigen, dass die Dynamik diese Verwandtschaft über alle Generationen hinweg bewahrt.

---

## 4. Zwei Lemmata über die Übergangsfunktion

### 4.1. Lemma 1 — Translations-Äquivarianz (hier wird der Torus gebraucht)

> Für jede Verschiebung `σ_δ` gilt `Φ ∘ σ_δ = σ_δ ∘ Φ`, d. h. `Φ(σ_δ s) = σ_δ(Φ(s))`.

**Beweis.** `Φ(s)(v)` hängt nur ab von (a) dem eigenen Zustand `s(v)` und (b) der Multimenge der Nachbarzustände `{ s(u) : u ∈ N(v) }`. Für die verschobene Konfiguration gilt `(σ_δ s)(v) = s(v − δ)`, und für die Nachbarn `(σ_δ s)(u) = s(u − δ)`. Da `N(v) = v + N(0)` auf dem **ganzen** Torus gilt, ist

```
{ u − δ : u ∈ N(v) } = N(v) − δ = (v − δ) + N(0) = N(v − δ).
```

Die lokalen Daten, die `Φ(σ_δ s)(v)` bestimmen, sind also identisch mit den lokalen Daten, die `Φ(s)(v − δ)` bestimmen. Folglich

```
Φ(σ_δ s)(v) = Φ(s)(v − δ) = (σ_δ Φ(s))(v)     für alle v. ∎
```

**Warum der Torus entscheidend ist:** Der Schritt `N(v) = v + N(0)` gilt *nur*, weil das Spielfeld randlos und homogen ist. Auf einem **begrenzten** Feld mit fixem toten Rahmen wäre die Nachbarschaft am Rand „beschnitten", `N(v)` wäre nicht mehr translations-kovariant, und Lemma 1 würde an den Rändern brechen. (→ §7, §8.)

### 4.2. Lemma 2 — Farbtausch-Äquivarianz (hier wird die Parität gebraucht)

> Es gilt `Φ ∘ χ = χ ∘ Φ`, d. h. `Φ(χ s) = χ(Φ(s))`.

**Beweis.** Sei `v` fest. Für `s` seien die Zählungen `ρ, β, n`; für `χs` seien sie `ρ', β', n'`. Da `χ` punktweise RED↔BLUE tauscht, gilt `ρ' = β`, `β' = ρ` und `n' = n`. Außerdem `(χs)(v) = κ(s(v))`.

**Fall `s(v) ≠ DEAD` (Überleben).** Dann `(χs)(v) = κ(s(v)) ≠ DEAD`. Das Überleben entscheidet sich an `n' = n`:
- `n ∈ {2,3}`: `Φ(χs)(v) = (χs)(v) = κ(s(v))`, und `χ(Φ(s))(v) = κ(s(v))`. Gleich. ✓
- `n ∉ {2,3}`: beide sterben → `DEAD = κ(DEAD)`. ✓

**Fall `s(v) = DEAD` (Geburt).** Dann `(χs)(v) = DEAD`. Die Geburt entscheidet sich an `n`:
- `n ≠ 3`: beide bleiben `DEAD`. ✓
- `n = 3`: `Φ(s)(v) = (ρ > β ? RED : BLUE)` und `Φ(χs)(v) = (ρ' > β' ? RED : BLUE) = (β > ρ ? RED : BLUE)`.
  - Teilfall `ρ > β`: `Φ(s)(v) = RED`, also `κ(Φ(s)(v)) = BLUE`. `Φ(χs)(v)`: `β > ρ` ist falsch → `BLUE`. Gleich. ✓
  - Teilfall `ρ < β`: `Φ(s)(v) = BLUE`, also `κ(Φ(s)(v)) = RED`. `Φ(χs)(v)`: `β > ρ` ist wahr → `RED`. Gleich. ✓
  - Teilfall `ρ = β`: **unmöglich**, denn `ρ + β = n = 3` ist **ungerade** ⇒ `ρ ≠ β`.

Der letzte Punkt ist der eigentliche Kern: Weil eine Geburt **genau 3** Nachbarn verlangt und 3 ungerade ist, kann es **niemals einen Gleichstand** `ρ = β` geben. Der asymmetrische Tie-Breaker im Code (`>` statt `≥`, der bei Gleichstand BLUE bevorzugen würde) wird **nie ausgelöst**. Die Regel ist deshalb in ihrem tatsächlich erreichbaren Verhalten **vollständig farbsymmetrisch** — es gibt keinen versteckten Vorteil für RED oder BLUE. ∎

---

## 5. Hauptsatz und Folgerung

### 5.1. Satz (T-Äquivarianz der Dynamik)

> `Φ ∘ T = T ∘ Φ`.

**Beweis.** Mit `T = χ ∘ σ_δ*`:

```
Φ ∘ T = Φ ∘ χ ∘ σ_δ*
      = χ ∘ Φ ∘ σ_δ*     (Lemma 2)
      = χ ∘ σ_δ* ∘ Φ     (Lemma 1)
      = T ∘ Φ.   ∎
```

### 5.2. Korollar (generationenweise Kopplung)

Seien `s₁(0) = s₁`, `s₂(0) = s₂ = T(s₁)` die Startkonfigurationen und `s_k(g+1) = Φ(s_k(g))` ihre Entwicklung. Dann gilt für **alle** `g ≥ 0`:

```
s₂(g) = T( s₁(g) ).
```

**Beweis durch Induktion über `g`.**
- *Basis* `g = 0`: `s₂(0) = T(s₁(0))` nach Behauptung A.
- *Schritt:* `s₂(g+1) = Φ(s₂(g)) = Φ(T(s₁(g))) = T(Φ(s₁(g))) = T(s₁(g+1))` (Satz 5.1). ∎

Runde 2 ist also zu **jedem Zeitpunkt** nur das `T`-Bild von Runde 1 — eine um 8 Spalten verschobene, farbgetauschte Kopie. Es entsteht keinerlei neue Information.

### 5.3. Konsequenz für Populationen und Siegchancen

Wir definieren die Populations-Funktionale `pop_RED(s) = #{v : s(v)=RED}` und `pop_BLUE(s)` analog. Ihr Verhalten unter den Operatoren:

```
σ_δ (Bijektion der Positionen):   pop_RED(σ_δ s) = pop_RED(s),   pop_BLUE(σ_δ s) = pop_BLUE(s)
χ   (Farbtausch):                  pop_RED(χ s)   = pop_BLUE(s),  pop_BLUE(χ s)   = pop_RED(s)
⇒  unter T:                        pop_RED(T s)   = pop_BLUE(s),  pop_BLUE(T s)   = pop_RED(s)
```

Am Endzeitpunkt `g*` (beide Runden erreichen ihn identisch, siehe §6):

```
pop_RED(s₂(g*)) = pop_BLUE(s₁(g*))
pop_BLUE(s₂(g*)) = pop_RED(s₁(g*))
```

Übersetzt auf die **Spieler**:

| | Runde 1 (Seite/Farbe) | Endpopulation | Runde 2 (Seite/Farbe) | Endpopulation |
|---|---|---|---|---|
| Spieler `P_A` | links / RED | `a := pop_RED(s₁(g*))` | rechts / BLUE | `pop_BLUE(s₂(g*)) = a` |
| Spieler `P_B` | rechts / BLUE | `b := pop_BLUE(s₁(g*))` | links / RED | `pop_RED(s₂(g*)) = b` |

> **Jeder Spieler endet in beiden Runden mit exakt derselben Population** — `P_A` immer mit `a`, `P_B` immer mit `b`, **unabhängig davon, ob er links oder rechts startet.** Die Differenz `a − b` ist identisch, der Sieger identisch, ein Unentschieden bleibt ein Unentschieden.

**Damit ist die Ausgangsfrage beantwortet: Es gibt _keine_ positionsabhängigen Siegchancen. Die Seite (links/rechts) ist für das Ergebnis kausal irrelevant.**

---

## 6. Auswirkung auf die Turnier-Buchhaltung

Die Redundanz schlägt bis in die Wertung durch (`main_hyper.c:76-108`). Für den Sieger gilt nach §5.3:

```
res2.winner == RED   ⟺   res1.winner == BLUE
res2.winner == BLUE  ⟺   res1.winner == RED
res2.winner == DRAW  ⟺   res1.winner == DRAW
```

Im Code erhält Spieler `i` aus Runde 1 Punkte für `res1.winner == RED` (er war RED) und aus Runde 2 Punkte für `res2.winner == BLUE` (er war BLUE). Wegen der Äquivalenz oben sind **beide Bedingungen exakt gleichwertig** — Spieler `i` bekommt aus Runde 2 dieselbe Punktzahl wie aus Runde 1. Dasselbe gilt für `j`.

**Folge:** Runde 2 multipliziert **jede** Turnier-Statistik exakt mit dem Faktor 2 — `total_score`, `wins`, `draws`, `losses` und `sum_stable_gen` (denn `g*` ist identisch). Eine Konstante × 2 über alle Teilnehmer verändert keine relative Reihenfolge. Das **Ranking ist invariant.**

**Weitere Invarianten** (alle aus Korollar 5.2, da `T` eine Bijektion ist):
- **Terminierung:** `run_isolated_match` stoppt bei einem Fixpunkt `Φ(s)=s`. Wegen `s₂(g)=T(s₁(g))` und Bijektivität von `T` ist `Φ(s₂(g))=s₂(g) ⟺ Φ(s₁(g))=s₁(g)`. Beide Runden werden im **selben** Generationsschritt `g*` stabil.
- **Highlight-Metrik (`activity_sum`):** Die Zahl der pro Generation geänderten Zellen ist unter der Bijektion `T` erhalten. Die als „spannend" gewerteten Aktivitäts-Summen beider Runden sind identisch — die zweite Runde liefert nur Duplikat-Highlights mit vertauschten RED/BLUE-Seeds.

### Zur Beobachtung „spiegelsymmetrischer Spielverläufe"

Die empirische Beobachtung — immer spiegelbildliche Verläufe mit gleichem Ergebnis — ist durch Korollar 5.2 **erzwungen, nicht zufällig.** Eine Präzisierung: Die exakte Beziehung ist eine **Translation um eine halbe Torusbreite plus Farbtausch**, nicht eine echte Spiegelung (Reflexion). Da bei der Halb-Verschiebung die linke und die rechte Hälfte ihre Plätze tauschen und zugleich die Farben kippen, *wirkt* das auf das Auge wie eine Links-rechts-Spiegelung mit Farbwechsel; geometrisch korrekt ist es eine zyklische Verschiebung. (Eine echte Spiegelung läge nur vor, wenn die Patterns zusätzlich in sich links-rechts-symmetrisch wären.)

---

## 7. Voraussetzungen — wovon das Ergebnis abhängt

Das Resultat ist kein allgemeines „Seitentausch ist immer egal", sondern ruht auf **genau drei Pfeilern**. Fällt einer, kann die Position wieder relevant werden:

1. **Torus-Topologie (Homogenität).** Trägt Lemma 1. `sync_ghost_borders` macht das Feld randlos. Auf einem begrenzten Feld mit fixem Rand bräche Lemma 1 an den Kanten — die Halb-Verschiebung wäre dann keine Symmetrie mehr.
2. **Farbsymmetrische Regel.** Trägt Lemma 2. Sie hält *nur*, weil Geburten ausschließlich bei `n = 3` (ungerade) stattfinden, sodass nie ein Gleichstand `ρ = β` entsteht und der asymmetrische Tie-Breaker nie greift.
3. **Positionsunabhängige Regel.** Ebenfalls Voraussetzung für Lemma 1: Die Regel `Φ` darf nicht von absoluten Koordinaten abhängen (kein Ressourcen-Gradient, keine unterschiedlichen Regeln je Hälfte).

> **Tripwire / Regressionswarnung:** Wird künftig die Topologie geändert (begrenztes Feld), die Geburtsregel um gerade Nachbarzahlen erweitert, ein farb- oder positionsabhängiger Bias eingeführt, **dann ist diese Analyse ungültig und die zweite Runde muss erneut geprüft / reaktiviert werden.**

---

## 8. Gegenbeispiele — wann die Position _doch_ zählen würde

Wer die ursprüngliche Design-Absicht („Position erzeugt unterschiedliche Situationen") tatsächlich realisieren will, muss **gezielt einen der drei Pfeiler brechen**:

- **Begrenztes Feld** (toter Rahmen statt Torus): Die Halb-Verschiebung ist keine Symmetrie mehr; Randeffekte unterscheiden links/rechts. Zwei Runden würden dann echte unterschiedliche Situationen testen.
- **Asymmetrischer Versatz:** Beide Patterns nicht in benachbarten 8er-Blöcken, sondern mit einem Offset, der **kein** Vielfaches der halben Torusbreite ist. Dann ist `s₂` kein `T`-Bild von `s₁` mehr.
- **Asymmetrische Regel:** z. B. ein Geburts-/Überlebens-Schwellwert, der für RED und BLUE verschieden ist, oder eine Geburt bei gerader Nachbarzahl mit echtem Tie-Breaker. Dann bräche Lemma 2.

Alle drei sind jedoch **Eingriffe ins Spieldesign**, nicht in die Turnier-Buchhaltung. Sie würden die Spielbalance verändern und müssten als eigene Design-Entscheidung (ADR) behandelt werden.

---

## 9. Antwort und Empfehlung

**Antwort auf die Ausgangsfrage:** Bei torusförmigem 8×16-Feld und der aktuellen, farbsymmetrischen Regel sind die Siegchancen **vollständig positionsunabhängig**. Es ist gleichgültig, ob eine Konfiguration links oder rechts liegt. Nach dem eigenen Entscheidungskriterium („falls identisch → zweite Runde überflüssig") folgt: **Die zweite Runde wird nicht benötigt.**

Optionen für das Vorgehen:

| Option | Beschreibung | Bewertung |
|---|---|---|
| **A — Runde 2 entfernen** *(empfohlen)* | In `main_hyper.c` pro Paarung nur noch ein Match (`run_isolated_match(i, j)`) rechnen. Die Punktevergabe entsprechend halbieren/anpassen. | **Halbiert die Rechenzeit** des O(N²)-Turniers (von `N(N−1)` auf `N(N−1)/2` Matches) bei **identischem Ranking**. Sauberste Konsequenz aus dem Beweis. |
| **B — Runde 2 als „Versicherung" behalten** | Code unverändert lassen, aber im Quelltext/ADR als *aktuell redundant* dokumentieren — Schutz, falls künftig ein Pfeiler aus §7 fällt. | Minimaler Aufwand, aber dauerhafte Verdopplung der Rechenkosten ohne aktuellen Nutzen. Nur sinnvoll, wenn eine Regeländerung konkret geplant ist. |
| **C — Position bewusst relevant machen** | Einen Pfeiler aus §7/§8 gezielt brechen (begrenztes Feld, asymmetrischer Versatz oder asymmetrische Regel), um die ursprüngliche Design-Absicht zu erfüllen. | Erfüllt die ursprüngliche Intention, ist aber ein **Spieldesign-Eingriff** mit Balance-Folgen — eigene Entscheidung, eigener ADR. |

**Empfehlung:** **Option A** (Runde 2 entfernen), kombiniert mit dem ausdrücklichen **Tripwire-Vermerk** aus §7: Sollte die Topologie oder die Farbsymmetrie der Regel jemals geändert werden, ist die zweite Runde neu zu bewerten. Option B nur, falls eine solche Regeländerung bereits konkret auf der Roadmap steht.

Die Umsetzung (Code-Änderung in `main_hyper.c` und die zugehörige Punktelogik) sowie die formale Entscheidung sollten in einem nachgelagerten **ADR-0026** ratifiziert werden — dieses Analyse-Dokument liefert dessen Begründung.

---

## Anhang: Code-Referenzen

| Aussage | Fundstelle |
|---|---|
| Torus-Wrapping aller vier Ränder | `src/core/game_logic.c:59` (`sync_ghost_borders`) |
| Übergangsregel (Überleben/Geburt) | `src/core/game_logic.c:161-167` |
| Geburt nur bei `n = 3`, Mehrheitsfarbe | `src/core/game_logic.c:164-165` |
| Links = RED, rechts = BLUE (Platzierung) | `src/core/game_logic.c:219-230` (`run_isolated_match`) |
| Zwei Runden mit Seitentausch | `src/apps/hyper/main_hyper.c:70-74` |
| Punktevergabe pro Runde | `src/apps/hyper/main_hyper.c:76-108` |
| Determinismus (kein `rand()` im Match) | `run_isolated_match` initialisiert nur aus Patterns |
