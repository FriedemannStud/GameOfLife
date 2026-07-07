---
title: "Die Neuen pflegen den Klüngel — und gewinnen nichts"
frage: "Lohnt sich Block-Loyalität beim ESC: Welche Ländergruppen schanzen sich Punkte zu, und bringt es ihnen etwas?"
auswahl:
  group: yugoslavia
  # weitere Anker je Beweisstück: caucasus + nordic (Beweisstück 1), ewg_founding + eastern_bloc (Gegenprobe)
  group_secondary: ewg_founding
decades: ["1956–1969", "1990–1999", "2000–2009", "2010–2019", "2020–2026"]
belege:
  - quelle: "votes_final.json"
    befund: "38.115 gerichtete Land-zu-Land-Wertungen 1957–2026. Klüngel je Gruppe als Bias des Nullmodells (Bias = ΣO/ΣE über die in bias_matrix.json erfassten Innen-Paare; das Nullmodell entfernt Geber-Spendierfreude und Empfänger-Stärke, per Jahr normiert): yugoslavia ×3,81 (O=2341, E=615), caucasus ×2,24 (O=414, E=185), nordic ×1,92 (O=4342, E=2259), eastern_bloc ×1,15 (O=9630, E=8349), ewg_founding ×1,10 (O=4270, E=3890). Alle fünf Gruppen-Bias sind im Permutationstest signifikant (4000 Ziehungen, seed 7; ΣO der Gruppe gegen die Nullverteilung; scripts/group_bias_pvalues.py): durchweg p<0,001 — yugoslavia/caucasus/nordic/eastern_bloc übertreffen jede der 4000 Ziehungen (p=0,00025), ewg_founding alle bis auf zwei (p=0,00075). Zum Vergleich der frühere rohe Treuefaktor (Gruppenschnitt ÷ Gesamtschnitt 3,06): yugoslavia ×2,97, caucasus ×2,22, nordic ×1,76, eastern_bloc ×1,23, ewg_founding ×0,98 — er überschätzt starke und unterschätzt schwache Punkter, der Bias korrigiert das."
  - quelle: "votes_final.json"
    befund: "Insider-Anteil der von Gruppenmitgliedern erhaltenen Punkte: eastern_bloc 34,5 %, yugoslavia 29,2 %, nordic 22,6 %, ewg_founding 19,0 %, caucasus 9,7 % (nur drei Länder, daher geringes internes Volumen)."
  - quelle: "entries.json"
    befund: "∅ final_place unter Finalisten gesamt 11,3 (n=1475). Je Gruppe: ewg_founding 10,1 (21 Siege, 342 Finalbeiträge), nordic 11,5 (14 Siege, 250), caucasus 11,5 (1 Sieg, 35), eastern_bloc 12,5 (9 Siege, 268), yugoslavia 12,8 (2 Siege, 110)."
  - quelle: "entries.json (feldgrößen-korrigiert)"
    befund: "Faires Erfolgsmaß je Gruppe, normiert auf Gelegenheit und Epoche, sodass Gruppengröße und Teilnahmedauer nicht mehr durchschlagen. Normierter Rang = Mittel von (Feldgröße−Platz)/(Feldgröße−1) über alle Finalauftritte (1=Sieger, 0=Letzter; Feldmittel aller Finalisten ≈0,51): caucasus 0,574, ewg_founding 0,537, eastern_bloc 0,504, nordic 0,483, yugoslavia 0,466. Dieses Maß speist das payoff-Diagramm (ADR-0011/GROUP_PAYOFF); eastern_bloc ist berechnet, wird dort aber nicht gezeigt."
  - quelle: "story_facts.py group"
    befund: "Eintrittszeitpunkte: ewg_founding und nordic sind 1956er-/Früh-Teilnehmer (alle 7 Dekaden präsent); yugoslavia-Nachfolger ab 1993 (HR, SI), caucasus komplett 2006–2008 (AM 2006, GE 2007, AZ 2008), eastern_bloc-Welle überwiegend 2000–2009 (AL 2004, BG 2005, BY 2004)."
recherche: []
steps:
  - section: "„Die wählen doch eh nur ihre Nachbarn.\""
    stage: sankey
    select: { allGrouped: true, except: [yugoslavia] }
  - section: "Klüngel-Könige"
    stage: sankey
    select: { groups: [yugoslavia, caucasus, nordic] }
  - section: "Klüngel-Könige"
    stage: map
    select: { groups: [yugoslavia, caucasus, nordic] }
    caption: "Klüngel-Könige"
  - section: "Die Datenbasis"
    stage: measure
    select: {}
  - section: "Beweis"
    stage: matrix
    select: { groups: [yugoslavia, caucasus] }
  - section: "Fun Fact - Belarus"
    stage: matrix
    select: { countries: [RU, BY, UA] }
  - section: "Gegenprobe"
    stage: matrix
    select: { groups: [ewg_founding] }
    beat: payoff-reveal
  - section: "Also — lohnt sich der Klüngel?"
    stage: payoff
    select: {}
status: final
---

## „Die wählen doch eh nur ihre Nachbarn."

Diesen Satz hörst du jedes Jahr im Mai, auf dem
Sofa, im Büro, im Netz. Er ist der Lieblingsvorwurf gegen den ESC: Nicht das Lied
gewinne, sondern die Landkarte. Aber stimmt das überhaupt? Und falls ja — hilft es
denen, die so klüngeln, am Ende auch?

## Klüngel-Könige

Fast das Vierfache. So viel mehr Punkte schieben sich die ex-jugoslawischen Länder
zu, als ihnen ganz ohne Klüngel zustünden — Bias **×3,81**. Nicht roh gezählt,
sondern fair gerechnet: gemessen an dem, was Geberlaune und Form erwarten lassen.
Du hast also recht, wenn du beim ESC den
Nachbarschafts-Klüngel witterst: Es gibt ihn wirklich. Und er hat ein Gesicht — fast
durchweg das der **Neuankömmlinge**: die ex-jugoslawischen Länder, das
Kaukasus-Trio, Teile des ehemaligen Ostblocks. Die alte Klüngel-Tradition halten allein
die fünf Nordländer hoch.

## Die Datenbasis

Reden wir kurz darüber, worauf das hier fußt. Grundlage ist `votes_final.json`: 38.115 Wertungen von Land zu Land, aus jedem Finale von 1957 bis 2026, jede mit Geber, Empfänger und Punktzahl. Drei Dinge schaue ich mir an. Erstens die Treue — aber nicht roh. Ein simpler Gruppenschnitt belohnt Länder, die ohnehin viel verteilen oder kassieren. Also rechne ich mit einem *Nullmodell*: Es schätzt, wie viele Punkte sich eine Gruppe ganz ohne Klüngel zuschieben würde — das **E**, der Erwartungswert — und hält die Wirklichkeit daneben, das **O**. Der **Bias** (O geteilt durch E) ist, was nach Abzug von Geberlaune und Form übrig bleibt: der nackte Klüngel. Zweitens die Abhängigkeit — der Anteil der Punkte, den eine Gruppe sich selbst gibt. Und drittens der Ertrag: ob am Ende ein guter Platz herauskommt, aus `final_place` und den Siegen in `entries.json`. Die fünf Gruppen kommen aus `groups.json`. Was die Daten zeigen: *wer wem wie viel*
gibt. Warum — dazu schweigen sie. Und so halte ich es auch.

## Beweis

Fangen wir mit dem an, was den Vorwurf stützt — und schauen gleich, *wer* da
klüngelt. Ganz oben die Nachzügler. „**Jugoslawien & Nachfolgestaaten**": Bias **×3,81** — fast das Vierfache dessen, was das Nullmodell ohne Klüngel erwartet (O **2.341**, E **615**), der höchste Wert im ganzen Feld.

Direkt dahinter der **Kaukasus** — Armenien, Georgien,
Aserbaidschan: Bias **×2,24** (O **414**, E **185**), der zweitstärkste im Feld.
Zufall? Ausgeschlossen — in 4.000 Permutationen erreicht keine einzige Ziehung
diese Bündelung (p<0,001).

## Fun Fact - Belarus

Sechsfach. So viel mehr, als ihm überhaupt zusteht, bekommt Belarus von seinen
Nachbarn — und das ist keine rohe Zahl, das ist der ehrliche Wert. Die Matrix
rechnet nämlich fair: Sie schätzt erst, wie viele Punkte ein Land normalerweise
bekäme — wie großzügig der Geber, wie stark der Empfänger im Schnitt ist. Das ist
das **E**, der Erwartungswert. Daneben stellt sie die Wirklichkeit, das **O**. Und
was darüber hinausragt, ist der **Bias** — der nackte Klüngel, von dem das
Land-ist-eben-beliebt schon abgezogen ist.

Schau jetzt in die Matrix. Die beiden heißesten Zellen im Trio zeigen beide auf
Belarus. Ukraine → Belarus: **50** Punkte, wo das Modell nur **7,8** erwartet —
Bias **×6,4**. Russland → Belarus: **41** statt **6,7** — Bias **×6,1**. Sechs- bis
sechseinhalbfach über dem, was Form und Zufall hergeben.

„Seine Freunde kann man sich aussuchen, seine Geschwister nicht." Russland,
Belarus, die Ukraine: drei ostslawische Brudervölker, jahrzehntelang im selben
politischen Wohnzimmer. Warum bekommt der Kleinste so viel mehr, als ihm zusteht? Sympathie? Verbundenheit? Das Gefälle zeigt dir die Matrix. Den Grund behält sie für sich.

## Gegenprobe

Das ist der eigentliche Schlag: Die EWG-Gründer, Belgien, Deutschland, Frankreich und die anderen Ur-Teilnehmer, klüngeln so gut wie *gar nicht*. Ihr Bias liegt bei **×1,10** — kaum über dem, was das Nullmodell ganz ohne Klüngel erwartet, und der niedrigste Wert im Feld.

## Also — lohnt sich der Klüngel?

Als Titelmaschine: nein. 

Und der Platz? Wir rechnen den Rang auf die Anzahl der teilnehmenden Länder um — 1 für den Sieger, 0 für den Letzten. Nur der Kaukasus, der zweitstärkste Klüngler von allen, liegt mit **0,57** vorn, knapp vor den EWG-Gründern mit **0,54**. Alle anderen Klüngel-Könige sortieren sich unterhalb des durchschnittlichen Ranges ein.

Fazit: Wer spät kommt und am Rand steht, rückt zusammen. Den Sieger? Den bestimmt am Ende doch der ganze Kontinent. Bleibt die eine Frage, die dir die Daten nicht abnehmen: Ist diese Treue kühle Strategie — oder einfach das, was zwangsläufig passiert, wenn Länder sich Nachbarschaft, Sprache und Geschichte teilen?
