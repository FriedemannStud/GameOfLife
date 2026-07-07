---
title: "Wie der Zerbruch des Ostblocks das ESC-Feld verdoppelte"
frage: "Wie hat das Ende des Kalten Krieges das ESC-Teilnehmerfeld verwandelt?"
auswahl:
  group: eastern_bloc
  # zweiter Anker für die Gegenprobe (YU → Nachfolger):
  group_secondary: yugoslavia
decades: ["1980–1989", "1990–1999", "2000–2009", "2010–2019"]
belege:
  - quelle: "story_facts.py transitions + group eastern_bloc"
    befund: "1980er: 23 aktiv, 3 neu (CY IS MA), 0 davon östlich. Neueinsteiger der GRUPPE eastern_bloc (= das bei dieser Auswahl sichtbare Ostblock-Band): 1990er 10 (EE HR HU LT MK PL RO RU SI SK), 2000er 7 (AL BG BY CZ LV RS UA), Summe 17 von 28 Neueinsteigern beider Dekaden. Feld wächst 23 → 47. ACHTUNG Abgrenzung: die breitere Kategorie 'postkommunistisch/östlich' zählt zusätzlich Kaukasus (AM AZ GE), Jugoslawien-Nachfolger (BA CS ME) und Moldau (MD) mit — 1990er dann 11, 2000er 13, Summe 24. Diese Zusatzländer gehören NICHT zur Gruppe eastern_bloc und erscheinen bei deren Auswahl nicht im Band; der Prosatext zählt deshalb nur die 17 echten Gruppenmitglieder."
  - quelle: "story_facts.py group eastern_bloc"
    befund: "Einziges Mitglied mit Eintrittsdekade vor 1990 ist Jugoslawien (YU, erst 1961, zuletzt 1992). Alle übrigen treten ab 1993 ein."
  - quelle: "story_facts.py group yugoslavia"
    befund: "YU zuletzt 1992; HR/SI ab 1993, BA 1993, MK 1998, CS 2004–2005, RS/ME ab 2007."
  - quelle: "story_facts.py transitions + group yugoslavia"
    befund: "Jugoslawien-Band: nach den 2000ern keine neuen Mitglieder mehr; einziger im Band sichtbarer Abgang ist BA nach 2016 (gone-node 2010er→2020er). Letzte Teilnahmen der Gruppenmitglieder laut entries.json: CS 2005, BA 2016, MK 2022, SI 2025; 2026 noch aktiv: HR, RS, ME. ABGRENZUNG: die Ostblock-Abgänge SK 2012, HU 2019, BY 2020, RU 2021 gehören zur Gruppe eastern_bloc, NICHT zu yugoslavia — sie sind erst im Schluss (kombinierte Auswahl eastern_bloc + yugoslavia) im Band sichtbar."
recherche: []
steps:
  - section: "Wo man singt ..."
    stage: sankey
    select: { countries: [YU] }
  - section: "Warum das zählt"
    stage: sankey
    select: { allGrouped: true, except: [yugoslavia] }
  - section: "Beweis 1"
    stage: sankey
    select: { group: eastern_bloc }
    beat: count-23-47
  - section: "Beweis 1"
    stage: map
    select: { group: eastern_bloc }
    beat: wall-fall
    caption: "Dieselbe Verdopplung, diesmal im Raum: der Osten füllt die Karte."
  - section: "Beweis 2"
    stage: sankey
    select: { group: yugoslavia }
    beat: yu-hinge
  - section: "Schluss"
    stage: sankey
    select: { groups: [eastern_bloc, yugoslavia] }
  - section: "Die Datenbasis"
    stage: sankey
    select: { groups: [eastern_bloc, yugoslavia] }
status: fertig
---

## Wo man singt ...

*"Wo man singt, da lass dich ruhig nieder. Böse Menschen kennen keine Lieder."*

Ein Land. Mehr als drei Jahrzehnte lang schickte der gesamte kommunistische Osten
genau ein Land zum Eurovision Song Contest: **Jugoslawien**, das blockfreie. Und
dann? Keine zwanzig Jahre später hatte sich das Starterfeld verdoppelt — und fast
jeder Neue kam aus genau dem Osten, der vorher gefehlt hatte. Ein Wettbewerb, der
sich gern für unpolitisch hält, hat den Fall des Eisernen Vorhangs
mitgeschrieben. Startplatz um Startplatz.

## Warum das zählt

Welche gesamteuropäische Einrichtung hat das geteilte *und* das
vereinte Europa erlebt? Es gibt nicht viele — und der ESC ist eine davon.
Wer hier dazugehört, entscheidet sich nicht in Brüssel, sondern auf der Bühne.
Der **Teilnehmerstrom** ist ein Seismograf: Er zeigt, wann sich der Kontinent
öffnet und wann er sich wieder schließt. Das alles steht in den Beitrittsdaten —
lange bevor irgendjemand über Gründe redet.

## Die Datenbasis

Eines musst du wissen: Die Daten sagen dir verlässlich *wer* und
*wann*. Über das *Warum* schweigen sie. Und so hält es auch dieser Text.

Woher kommen die Zahlen? 
Aus `entries.json`: 1.830 Wettbewerbsbeiträge von 1956
bis 2026, je ein Datensatz pro Beitrag.
Das Skript `story_facts.py` rechnet aus diesen Beiträgen für jeden Dekaden-Übergang aus, wer neu hinzukommt, wer bleibt, wer verschwindet — mit denselben Dekadengrenzen, die auch das Sankey-Diagramm zeichnet.
Die Gruppen „Ehemaliger Ostblock" (18 Länder,Jugoslawien eingerechnet) und „Jugoslawien & Nachfolgestaaten" (8) kommen aus `groups.json`. 

## Beweis 1

Bis in die 1980er passiert wenig: 23 Länder, stabil, und von den drei Neuzugängen
des Jahrzehnts kommt kein einziges aus dem Osten. Dann bricht der Damm. In den
1990ern kommen zwölf neue Länder dazu — und zehn davon liegen im Ostblock-Band,
darunter **Estland** und **Litauen**, dazu **Kroatien**, **Polen**, **Rumänien** und
**Russland**. In den 2000ern füllt sich dasselbe Band weiter: von 16 Neuzugängen
gehören sieben hinein, unter ihnen **Ukraine**, **Serbien**, **Bulgarien** und **Lettland**. Zähl beide Jahrzehnte zusammen, dann
stellt allein der ehemalige Ostblock **17 von 28 Neueinsteigern** — kein anderer
Teil Europas trägt auch nur annähernd so viel bei. Das Feld wächst von 23 auf 47,
und der breite Neueinsteiger-Block, der die 1990er und 2000er aufbläht, ist zum
größten Teil östlich. Im Sankey-Diagramm kannst du es nicht übersehen.

## Beweis 2

Der ganze Übergang hängt an einem Scharnier, und das heißt **Jugoslawien**. Das
einzige Ostland der frühen Jahrzehnte — und es nimmt 1992 zum letzten Mal teil.
Exakt in dem Moment, in dem seine Nachfolger auf die Bühne treten.
**Kroatien**, **Slowenien** und **Bosnien** 1993, später **Serbien** und **Montenegro**. Schau
dir das im Diagramm an: Ein Strang endet, und direkt daneben beginnen mehrere
weitere. Der Zerfall eines Staates, übersetzt in Teilnehmerflüsse.

Aber ist eine Öffnung für immer? Nein — und das zeigt sogar das jugoslawische Band
selbst. Nach den 2000ern kommt kein neuer Strang mehr hinzu, und bald verliert das
Band wieder Stränge ein: **Bosnien** scheidet nach 2016 aus dem Feld aus; auch
die Nachzügler bleiben nicht — **Nordmazedonien** ist 2022 zum letzten Mal dabei,
**Slowenien** 2025. Warum? Das verraten die Teilnahmedaten nicht. Sie sagen dir nur:
Sie gehen. Die Öffnung der 1990er war eben keine Einbahnstraße.

## Schluss

Also: Hat der Fall des Eisernen Vorhangs das ESC-Feld verwandelt? Die Daten lassen
keinen Zweifel. Aus einem einzigen östlichen Teilnehmer wurden binnen zwanzig
Jahren Dutzende, und die Verdopplung des Wettbewerbs war fast ausschließlich ihr
Werk. Der ESC hat die europäische Wiedervereinigung nicht kommentiert — er hat sie
vollzogen, Startplatz um Startplatz. Und doch zeigen dieselben Daten die andere
Seite: Beitritt ist umkehrbar, und seit den 2010ern überwiegen im Osten die
**Abgänge**. 
Eine Frage bleibt, und der Datensatz beantwortet sie nicht: Ist dieser
Rückzug bloß Ermüdung — oder die erste Linie eines neuen Grabens?
