---
title: "Krieger sollen nicht singen! — Wie der ESC Kriegsländer belohnt, statt sie zu bestrafen"
frage: Bekommen Länder, die Krieg führen, beim ESC weniger Punkte — oder im Gegenteil mehr?
auswahl:
  countries: [UA, IL, RU]
decades: ["2010–2019", "2020–2026", "1990–1999", "1956–1969"]
belege:
  - quelle: entries.json (final_jury_points / final_televote_points, abgeleitet)
    befund: >-
      Basislinie aller Final-Auftritte ab 2016 (n=257): mittleres Publikums-Übergewicht
      Δ(Televote − Jury) ≈ +0,9, Median −3. Kriegsjahr-Auftritte liegen am äußersten
      oberen Ende — 4 der 8 größten Publikums-über-Jury-Ausschläge überhaupt sind
      Kriegsjahre: IL 2024 +271, UA 2022 +247, IL 2025 +237, RU 2016 +231.
  - quelle: entries.json (Ukraine, Kriegsjahre 2014–2026)
    befund: >-
      Televote durchweg ≫ Jury: 2016 Jury 211 / Televote 323 (Sieg); 2018 Jury 11 /
      Televote 119 (Platz 17); 2022 Jury 192 / Televote 439 (Sieg).
  - quelle: country_attributes.json (wars/role) + entries.json (Russland)
    befund: >-
      RU als role 'angreifend' (Russisch-Ukrainischer Krieg ab 2014): 2016 Jury 130 /
      Televote 361 (Δ +231), 2021 Jury 104 / Televote 100 (Δ −4); letzter Auftritt 2021.
  - quelle: entries.json (final_place, Vor-Televote-Ära)
    befund: >-
      YU 1989 Platz 1 (137 P) → 1991 Platz 21 (1 P), danach kein Auftritt mehr.
      FR siegt während des Algerienkriegs dreimal (1958, 1960, 1962); PT 1964 Platz 13
      (0 P) während des Kolonialkriegs.
recherche:
  - fakt: >-
      Russland wurde 2022 nicht weggestimmt, sondern von der EBU ausgeschlossen
      (Statement vom 25.02.2022); die Suspendierung der russischen Mitglieder wurde
      am 26.05.2022 wirksam. In der Codebase endet RU nur faktisch mit 2021.
    quelle: "EBU, »Statement on Russia in the Eurovision Song Contest 2022«, ebu.ch"
    abgerufen: 2026-06-24
# ── Visualisierungs-Design (Ergebnis der /grill-me-Sitzung) ────────────────
# Eine neue Visualisierung trägt den Beweis, zwei Overlays auf den bestehenden
# Views liefern Auf- und Abspann. Trichter: breites Kriegsfeld → Trio.
visualisierung:
  traeger_evidenz: vote-dumbbell
  # NEUE Stage: Hantel Jury●——●Televote je Kriegsjahr-Auftritt, davor die blasse
  # Δ-Wolke aller 257 Final-Auftritte 2016+ mit Median-(-3)-Linie. Highlight je
  # Step. Trio-Friedensjahre liegen IN der Wolke (IL-Friedensjahre negativ:
  # 2016 −113, 2017 −29, 2021 −53 — derselbe Sänger, Krieg vs. Frieden).
  vote_stage:
    baseline: "alle 257 Δ (Tele−Jury) 2016+, Median −3 markiert"
    highlight: "Kriegsjahre des im Step gewählten Landes als obere Ausreißer"
    integritaet: "alle Kriegsjahre zeigen, nicht nur die Spitzen; IL 2023 (+8) ist
      kein Kriegsjahr-Effekt — Contest Mai, Gazakrieg ab Oktober; erstes volles
      Kriegsjahr ist 2024"
  # Bestehende Sankey: Kriegsjahr-SEGMENTE der Bänder markiert, binär nach der in
  # der Dekade aktiven Rolle gefärbt. Doppelrolle: Auftakt (Breite) UND Gegenprobe
  # (FR/PT/YU rot in den frühen Dekaden, während die Vote-Wolke dort leer ist).
  sankey_overlay:
    war_segments: "wars[] from/to × Dekadengrenzen → markierte Segmente"
    rolle_binaer: { blau: verteidigend, rot: "alles übrige (angreifend/zwischen-\
      staatlich/zerfall/bürgerkrieg/kolonial)" }
    schluss: "RU verlässt das Feld über den bestehenden gone-node 2021 — Caption
      trägt ›ausgeschlossen, nicht weggestimmt‹"
  # Bestehende Map: Panorama der Kriegsparteien, je Step-Fokusjahr nach Binär-Rolle
  # neu gefärbt — die Karte atmet mit der Erzählung.
  map_overlay:
    color_by: "Binär-Rolle im focus-Jahr des Steps"
  neue_codeflaeche:
    - "dritte .is-stage (#vote) + setStage-Zweig; Vote-Dumbbell-Render"
    - "Sankey-Kriegssegment-Overlay (wars[] × Dekade)"
    - "Map-Recolor je focus-Jahr; select-Modus belligerents:true (wars[] nicht leer)"
    - "Frontmatter-Felder stage:vote, focus:, highlight:; STORY_SOURCES['0002']"
  hinweis: "Major Feature → vor Implementierung ADR → DEV_SPEC → DEV_TECH_DESIGN →
    DEV_TASKS (CLAUDE.md); steps unten sind die Design-Vorlage, noch nicht lauffähig."
steps:
  - section: "Krieger sollen nicht singen!"
    stage: map
    select: { belligerents: true }
    focus: all
    caption: "So viele, die singen, führen Krieg — links alle Krieger von 1956 bis 2026."
  - section: "Warum das zählt"
    stage: sankey
    select: { belligerents: true }
    focus: all
    beat: war-segments
  - section: "Datenbasis"
    stage: vote
    select: {}
    focus: 2016
    beat: baseline
  - section: "Ups... 1"
    stage: vote
    select: { countries: [UA] }
    focus: 2022
    beat: war-spikes
  - section: "Ups... 2"
    stage: vote
    select: { countries: [IL, RU] }
    focus: 2024
    beat: war-spikes

  - section: "Also, bestraft der ESC die Krieger?"
    stage: sankey
    select: { countries: [RU] }
    focus: 2021
    beat: war-segments
status: fertig
---

## Krieger sollen nicht singen!

Klingt vernünftig, oder? Wer am Morgen Raketen abfeuert, soll am Abend nicht in Pailletten um ESC-Punkte betteln — eine Frage des Anstands.

Nur: Die Zahlen des Eurovision Song Contest pfeifen auf den Anstand. Länder im Krieg werden nicht abgestraft. Sie werden **belohnt**. Aber nur von der einen Hälfte des Saals. Die Profi-Jurys bleiben ungerührt. Das Publikum dagegen verwandelt den Wettbewerb in eine stille Abstimmung über Mitgefühl — und merkt es selbst kaum.

## Warum das zählt

Der ESC will ein unpolitisches Fest sein. Genau deshalb verrät er so viel. Er ist
ein Seismograf: Was Europa gerade bewegt, schlägt im Punktetableau aus.

Seit 2016 stehen Jury- und Publikumsstimmen getrennt im Protokoll — und seitdem siehst du etwas, das vorher in einer einzigen Zahl verschwand: Beide bewerten den Krieg völlig verschieden. Die Frage ist also nicht mehr, *ob* ein Kriegsland Punkte bekommt. Sondern *wer* sie ihm gibt — die Institution oder die Straße.

## Datenbasis

Zwei Dateien tragen alles: `entries.json` (ein Datensatz je Beitrag
1956–2026, mit `final_place` und ab 2016 `final_jury_points` plus
`final_televote_points`) und das Feld `wars[]` aus `country_attributes.json`, das
jedem Land seine Kriege der ESC-Ära mit Jahren und einer `role` zuordnet —
verteidigend, angreifend, zwischenstaatlich. 

Wichtiger Hinweis: Rohpunkte über die Jahrzehnte zu vergleichen, geht schief. Ein Sieger holte 1958 ganze 27 Punkte, 2016 waren es 534. Belastbar sind nur die **Platzierung** und der Abstand **Televote minus Jury** ab 2016. Genau dieses Δ ist unser Maß. Über alle 257 Final-Auftritte seit 2016 liegt es im Schnitt bei +0,9, im Median bei −3.

Heißt: **Im Normalfall sind sich Jury und Publikum ziemlich einig**.

## Ups... 1

Und dann kommt die **Ukraine**. Seit Kriegsbeginn 2014 kippt sie dieses Gleichgewicht
regelmäßig. Die eigentliche Geschichte steckt im Riss zwischen beiden. 2016 holt »1944« den Sieg: 211 von der Jury, 323 vom Publikum. 2018, ein schwaches Jahr, Platz 17 — die Jury gibt **11** Punkte, das Publikum **119**. Und 2022, wenige Wochen nach dem russischen Großangriff, türmt das Publikum **439** Punkte auf, gegen 192 der Jury. Ein Δ von +247. Das trägt kein Lied der Welt. Das trägt das Mitgefühl im Wohnzimmer.

## Ups... 2

Der größte Publikums-über-Jury-Ausschlag, der je gemessen
wurde, gehört **Israel** — mitten im Gazakrieg. 2024: 52 Jury-, 323
Publikumspunkte, Δ +271. 2025: 60 gegen 297. Die Jurys bleiben eiskalt, das
Publikum glüht. Dasselbe Muster wie bei der Ukraine — das Motiv lassen die Daten
offen.

**Jetzt wird es unbequem**. Denn die saubere Erzählung »Verteidiger gut, Angreifer
böse« stimmt nicht. Selbst der **Angreifer Russland** kassiert 2016 vom Publikum 361 Punkte (Jury 130, Δ +231), ein Stimmenblock, der bis 2021 auf magere −4
zusammenschmilzt.

Bleibt die Gegenprobe — die liefert die Vergangenheit. Vor dem Televote war
Krieg im Punkte-Tableau unsichtbar. Jugoslawien stürzte zwar mitten im Zerfall
von Platz 1 (1989) auf Platz 21 mit **einem** einzigen Punkt (1991). Aber
**Frankreich**? Gewann während des Algerienkriegs gleich dreimal — 1958, 1960, 1962.
Und **Portugal** landete während seines Kolonialkriegs auf den letzten Plätzen, 1964
mit null Punkten. Die reine Jury-Ära verband Song und Schlachtfeld einfach nicht.

## Also, bestraft der ESC die Krieger?

 Nein! Die Antwort ist sogar ein doppeltes Nein. Die Jury bleibt neutral bis kühl. Und das Publikum belohnt ausgerechnet den, der in den Schlagzeilen steht — egal ob angegriffen (Ukraine) oder Kriegspartei (Russland). Die Politisierung des Wettbewerbs ist ein Kind des Televotes: Erst die Fernbedienung gab dem Saal eine Stimme, lauter als die der Fachjurys.

Und bezeichnend bleibt, wie ein "Krieger" am Ende doch verschwand. **Russland** wurde nach 2021 nicht "weggestimmt". Es wurde 2022 von der EBU **ausgeschlossen**.
