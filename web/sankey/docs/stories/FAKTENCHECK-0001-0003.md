# Faktencheck-Bericht — Data Stories 0001 / 0002 / 0003

- **Prüfer:** Fact-Desk
- **Datum:** 2026-06-28
- **Standard:** NYT (jede Tatsachenbehauptung gegen Primärquelle geprüft)
- **Primärquellen:** `eurovision-data/{entries,votes_final,groups,bias_matrix}.json`
- **Reproduziert mit:** `story_facts.py`, `scripts/build_bias_matrix.py`, `scripts/group_bias_pvalues.py`

## Gesamturteil

Alle drei Stories sind außergewöhnlich gut belegt. Sämtliche tragenden Zahlen
wurden unabhängig nachgerechnet und stimmen — inklusive der 4.000er-Permutations-
tests (seed 7), die für diesen Bericht neu liefen. Es wurde **eine** sachliche
Korrektur nötig (Story 0001, Datenbasis-Satz), dazu zwei kleinere Präzisierungen;
**alle drei sind am 2026-06-28 eingearbeitet** (siehe Abschnitt „Eingearbeitete
Korrekturen"). Stories 0002 und 0003: keine Beanstandung.

---

## Story 0001 — „Zerbruch des Ostblocks"

| Behauptung | Daten | Status |
|---|---|---|
| Nur **ein** Ostland vor 1990 (Jugoslawien), „mehr als drei Jahrzehnte" | YU einziger kommunistischer Teilnehmer, 1961–1992 (31 J.) | ✅ |
| 1980er: **23 Länder**, 3 Neuzugänge, keiner östlich | count=23; Neu: CY, IS, MA | ✅ |
| 1990er: **12 neue**, **10** im Ostblock-Band | 12 Neue; davon 10 in `eastern_bloc` (BA, MT außen) | ✅ |
| 2000er: von **16** Neuen **7** östlich | 16 Neue; 7 (AL BG BY CZ LV RS UA) | ✅ |
| **17 von 28** Neueinsteigern Ostblock | 10+7=17 / 12+16=28 | ✅ |
| Feld wächst **23 → 47** | 1980er 23, 2000er 47 | ✅ |
| YU letztes Mal **1992**; HR/SI **1993**; RS/ME **2007** | exakt so | ✅ |
| Bosnien scheidet **nach 2016** aus; Nordmazedonien **2022**; Slowenien **2025** | BA last 2016, MK 2022, SI 2025 | ✅ |
| Gruppen: Ostblock **18** (mit YU), Jugoslawien **8** | groups.json: 18 / 8 | ✅ |

**Befunde (vor Korrektur):**

1. 🔴 *Sachlich* — Datenbasis-Satz „1.830 Wettbewerbsbeiträge … **je ein Datensatz
   pro Land und Jahr**." Falsch für 1956: Beim Eröffnungswettbewerb sangen alle 7
   Länder **je zwei** Titel → 14 Datensätze brechen die Regel. Die Zahl 1.830 ist
   korrekt, aber es ist „ein Datensatz pro **Beitrag**" (so auch CLAUDE.md:
   *one record per song entry*).
2. 🟡 *Präzision* — „**Bosnien folgt**" (nach Kroatien/Slowenien 1993): BA
   debütierte **ebenfalls 1993**, nicht später.
3. 🟡 *Präzision* — „**baltischen Nachbarn**" im 1990er-Block: in den 1990ern sind
   nur EE + LT baltisch dabei; **Lettland kommt erst in den 2000ern** (dort bereits
   korrekt genannt). Der Plural „Nachbarn" für die 1990er war leicht überdehnt.

---

## Story 0002 — „Krieger sollen nicht singen!"

| Behauptung | Daten | Status |
|---|---|---|
| Baseline 2016+: **n=257**, Schnitt **+0,9**, Median **−3** | n=257, mean +0,9, median −3 | ✅ |
| Sieger 1958 **27** Pkt, 2016 **534** | FR 1958 = 27; UA 2016 = 534 | ✅ |
| UA 2016: Jury **211** / Tele **323**, Sieg | place 1, 211/323 | ✅ |
| UA 2018: Platz **17**, Jury **11** / Tele **119** | exakt | ✅ |
| UA 2022: Jury **192** / Tele **439**, Δ **+247**, Sieg | exakt | ✅ |
| IL 2024 **größter je gemessener** Ausschlag, Jury 52 / Tele 323, Δ **+271** | +271 ist Allzeit-Maximum | ✅ |
| IL 2025: **60** gegen **297** (Δ +237) | exakt | ✅ |
| RU 2016: Tele **361**, Jury **130**, Δ **+231**; bis 2021 auf **−4** | exakt; RU 2021 Δ −4 | ✅ |
| YU **Platz 1 (1989)** → **Platz 21, 1 Pkt (1991)** | place 1 (137) → 21 (1) | ✅ |
| FR siegt im Algerienkrieg **1958, 1960, 1962** | alle drei place 1 (Krieg bis 19.3.1962) | ✅ |
| PT 1964 **letzte Plätze, 0 Punkte** | place 13 = geteilter **letzter** Platz, 0 Pkt | ✅ |
| RU 2022 **ausgeschlossen**, nicht weggestimmt | belegt mit EBU-Quelle (recherche) | ✅ |

**Hinweis (kein Fehler):** Die Frontmatter-Integritätsregel (IL 2023 = +8, kein
Kriegsjahr; erstes volles Kriegsjahr 2024) ist im Prosatext korrekt eingehalten —
Israel taucht nur mit 2024/2025 auf. **Keine Beanstandung.**

---

## Story 0003 — „Die Neuen pflegen den Klüngel"

| Behauptung | Daten | Status |
|---|---|---|
| **38.115** Wertungen, 1957–2026 | exakt | ✅ |
| Bias: YU **×3,81** (O 2341/E 615), höchster | ΣO/ΣE = 3,81 | ✅ |
| Kaukasus **×2,24** (O 414/E 185), zweithöchster | 2,24 | ✅ |
| Nordic ×1,92 · Ostblock ×1,15 · EWG **×1,10** (niedrigster) | 1,92 / 1,15 / 1,10 | ✅ |
| Belarus: UA→BY **50** vs **7,8** = **×6,4**; RU→BY **41** vs **6,7** = **×6,1** | ×6,43 / ×6,12 | ✅ |
| „beide heißesten Zellen im Trio zeigen auf Belarus" | Top 2 von 6 = UA→BY, RU→BY | ✅ |
| p<0,001 / „keine der 4.000 Ziehungen" | nachgerechnet: YU/CAU/NOR/EB **p=0,00025**, EWG **0,00075** | ✅ |
| ∅ Platz gesamt **11,3** (n=1475); Feldmittel norm. Rang ≈**0,51** | 11,35 / 0,510 | ✅ |
| Norm. Rang: CAU **0,57** > EWG **0,54** > Rest unter Schnitt | 0,574 / 0,537; NOR 0,483, YU 0,466, EB 0,504 | ✅ |
| Siege/Platz je Gruppe (EWG 21 Siege/10,1 … YU 2/12,8) | alle 5 exakt | ✅ |
| Insider-Anteil 34,5 / 29,2 / 22,6 / 19,0 / 9,7 % | alle 5 auf 0,1 % exakt | ✅ |

**Keine Beanstandung.** Die methodisch heikelsten Aussagen (Nullmodell-Bias,
Permutations-p-Werte, feldgrößen-korrigierter Rang) sind vollständig
reproduzierbar und korrekt.

---

## Eingearbeitete Korrekturen (Story 0001, 2026-06-28)

1. **Datenbasis:** „je ein Datensatz pro Land und Jahr" → „je ein Datensatz pro
   **Beitrag**".
2. **Beweis 1:** „… und die **baltischen Nachbarn**" → „darunter **Estland** und
   **Litauen**, dazu **Kroatien**, **Polen**, **Rumänien** und **Russland**".
3. **Beweis 2:** „**Kroatien** und **Slowenien** 1993, **Bosnien** folgt" →
   „**Kroatien**, **Slowenien** und **Bosnien** 1993".
