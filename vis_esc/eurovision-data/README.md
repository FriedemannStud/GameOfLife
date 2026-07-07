# Eurovision Dataset

Structured JSON files covering the Eurovision Song Contest from **1956 to 2026**.

---

## Files

| File | Rows | Description |
|---|---|---|
| `entries.json` | 1,830 | One record per song entry |
| `votes_final.json` | 38,115 | One record per country-to-country vote in the grand final |
| `groups.json` | 7 groups | Curated country groups for ESC participation stories (group key → `{ label, members[], story }`). **Complete** — all seven groups |
| `country_attributes.json` | 1 obj/country | Per-country reference attributes (languages, cultural region, government over time, wars). **Scaffold** — 4 example countries so far |
| `bias_matrix.json` | 2,363 pairs | Directed vote bias per country pair vs. a leave-one-out null model, with permutation p-values. **Derived** — rebuilt by `scripts/build_bias_matrix.py` |
| `bias_seriation.json` | 7 blocks | Louvain community ordering of the 52 countries for the bias-matrix view. **Derived** — same generator |

---


## entries.json

One row per song entry (1956–2026). Includes both finalists and semi-final eliminations from 2004 onward.

| Field | Type | Notes |
|---|---|---|
| `performance_id` | string | Unique join key, format `ISO2_YEAR`, e.g. `"SE_2024"`. **1956 only**: uses `ISO2_YEAR_ID` (e.g. `"CH_1956_0"`) because countries entered two songs that year |
| `year` | integer | e.g. `2024` |
| `country_iso2` | string | ISO 3166-1 alpha-2 code, e.g. `"SE"`. Historical codes are preserved: Yugoslavia = `"YU"`, Serbia & Montenegro = `"CS"` |
| `country` | string | Country name as it appeared at the time, e.g. `"Yugoslavia"` |
| `country_modern` | string | Current official country name, e.g. `"Serbia"`. **null** for dissolved states — Yugoslavia (`YU`) and Serbia & Montenegro (`CS`) have no single modern successor |
| `artist` | string | e.g. `"ABBA"` |
| `song` | string | e.g. `"Waterloo"` |
| `language_sung` | string | Language(s) performed in, e.g. `"Swedish"`. **Multiple languages** are pipe-separated: `"English\|French\|Hebrew"`. Never null |
| `is_finalist` | boolean | `true` if the entry performed in the grand final. Always `true` before 2004 (no semis existed). Also `true` for the Big 5 (FR, DE, ES, IT, GB) and the host country, who qualify automatically |
| `final_running_order` | integer | Performance slot in the grand final, e.g. `14`. **null** if `is_finalist` is `false` |
| `final_place` | integer | Final ranking, e.g. `1`. **null** if `is_finalist` is `false` |
| `final_total_points` | integer | Total points received in the final, e.g. `365`. **null** if `is_finalist` is `false`, or if year is 1956 (scores were never officially published) |
| `final_jury_points` | integer | Points from national juries only, e.g. `243`. **null** if `is_finalist` is `false`, or if year is before 2016 — the jury/televote split wasn't introduced until then |
| `final_televote_points` | integer | Points from public televoting only, e.g. `122`. Same null conditions as `final_jury_points` |

---

## votes_final.json

One row per country-to-country vote pair in the grand final. Starts from 1957 (no vote data for 1956).

| Field | Type | Notes |
|---|---|---|
| `year` | integer | e.g. `2024` |
| `from_country_iso2` | string | Country casting the vote, e.g. `"SE"`. `"WLD"` from 2023 onward = the combined rest-of-world televote via the Eurovision app |
| `from_country` | string | e.g. `"Sweden"` |
| `to_performance_id` | string | Links to `performance_id` in `entries.json`, e.g. `"IT_2024"` |
| `to_country_iso2` | string | e.g. `"IT"` |
| `to_country` | string | e.g. `"Italy"` |
| `total_points` | integer | Points awarded, e.g. `12`. Classic 1–8, 10, 12 scale. From 2016 onward this is the combined jury + televote score |
| `jury_points` | integer | Jury component only, e.g. `12`. **null** before 2016 — the split wasn't published |
| `televote_points` | integer | Televote component only, e.g. `8`. **null** before 2016 |

---

## groups.json

Curated country **groups** for the ESC participation stories, keyed by a stable
group key. A small `_meta` object holds documentation; the data lives under
`groups`. Loaded at runtime by `sankey.html` (`loadGroups()`) to build the
selection-panel checkboxes; the group keys are identical to the panel's
`data-group` values, so they must not be renamed.

| Field | Type | Notes |
|---|---|---|
| *key* | string | Stable group key (`ewg_founding`, …). Identical to the `data-group` values in `sankey.html`; never displayed |
| `label` | string | German UI string shown on the checkbox. Non-empty |
| `members` | string[] | ISO2 codes joining `entries.json.country_iso2`. May include historical `YU`/`CS` |
| `story` | `null` \| string | **Reserved anchor** for the future per-group data-story (ADR-0006). `null` until a story is written; do not repurpose |

### The seven groups

| Key | Label | Rationale |
|---|---|---|
| `ewg_founding` | EWG-Gründungsmitglieder | The six founding members of the European Economic Community |
| `eastern_bloc` | Ehemaliger Ostblock | The former Eastern Bloc states, most arriving in a single 1990s wave |
| `nordic` | Nordische Länder | The five Nordic countries |
| `yugoslavia` | Jugoslawien & Nachfolgestaaten | Yugoslavia (`YU`, 1961–1992) fragments into six entrants over 15 years; Serbia & Montenegro (`CS`) splits a second time |
| `caucasus` | Kaukasus | The Caucasus arrives late (2006–2008) — the geographically most contested edge of "Europe": Armenia, Georgia and Azerbaijan |
| `beyond_europe` | Jenseits Europas | Participants outside geographic Europe — Israel (Asia), Morocco (Africa, single 1980 entry), Australia (Oceania) and transcontinental Turkey |
| `mediterranean` | Mittelmeer-Süderweiterung | Mediterranean southern enlargement of the 1970s–80s: Malta, Greece, Turkey and Cyprus join within a decade |

Some countries appear in more than one group (e.g. Turkey in both `beyond_europe`
and `mediterranean`); the visualisation resolves such overlaps once via
`effectiveMembers`.

---

## country_attributes.json

Per-country reference attributes, **keyed by ISO2** (joins `entries.json.country_iso2`).
A small `_meta` object holds documentation; the data lives under `countries`.

**Design principle — static vs. time-varying.** Attributes that never change are
plain scalars/lists (`languages`, `cultural_region`). Attributes that change over
time are **lists of intervals** with explicit `from`/`to`, so you can ask "what was
X in year Y". Everything is stored at full-year resolution; decade aggregation is
done in code, never pre-baked here.

| Field | Type | Notes |
|---|---|---|
| `languages` | string[] | Spoken/official languages as ISO 639-1 codes, e.g. `["de"]`. Distinct from `entries.json.language_sung` (the *song's* language) |
| `cultural_region` | string | One value from the **cultural-region vocabulary** below |
| `government` | object[] | Interval list; each `{ from, to, form, note? }`. `form` from the **government-form vocabulary**. `to: null` = still in effect |
| `wars` | object[] | Event/interval list; each `{ from, to, name, role? }`. `to: null` = ongoing. Empty `[]` means "none recorded", not "unknown" |

Interval objects share the same shape across attributes: `from` (integer year,
inclusive), `to` (integer year inclusive, or `null` for open/ongoing).

### Controlled vocabularies

Keep these closed sets consistent — they drive colour scales and filters, so free
text would fragment them. Extend the vocabulary here (and in the validator) before
introducing a new value.

**`cultural_region`**

| Value | Meaning |
|---|---|
| `nordeuropa` | Nordic countries |
| `westeuropa` | Western Europe |
| `mitteleuropa` | Central Europe |
| `suedeuropa` | Southern Europe |
| `osteuropa` | Eastern Europe |
| `suedosteuropa` | South-Eastern Europe / Balkans |
| `kaukasus` | Caucasus |
| `naher_osten` | Near East |
| `nordafrika` | North Africa |
| `ozeanien` | Oceania |

**`government.form`**

| Value | Meaning |
|---|---|
| `parlamentarische_monarchie` | Parliamentary (constitutional) monarchy |
| `parlamentarische_republik` | Parliamentary republic |
| `praesidialrepublik` | Presidential republic |
| `semipraesidiale_republik` | Semi-presidential republic |
| `sozialistische_republik` | Socialist (one-party) republic |
| `fuerstentum` | Principality |

**`wars.role`** — the country's primary military posture in that war (a
simplification of contested histories). The same war appears under each
belligerent ESC country with a mirrored role, e.g. Cyprus 1974: `CY`
`verteidigend` / `TR` `angreifend`.

| Value | Meaning |
|---|---|
| `verteidigend` | Defends against an attack/invasion on its own (claimed) territory |
| `angreifend` | Launched the offensive / invaded or attacked another territory |
| `zwischenstaatlich` | Mutual interstate war, symmetric or disputed initiation |
| `buergerkrieg` | Internal civil war on the country's own territory |
| `zerfallskonflikt` | War of state disintegration / post-imperial separatism |
| `kolonialkrieg` | Metropole fighting an independence/decolonisation movement |

Scope (see `_meta.wars_scope` in the JSON): only conflicts where the country is a
**direct belligerent** (interstate, territorial, and full-scale
civil/disintegration wars), with war dates overlapping the **ESC era 1956–2026**.
Coalition/expeditionary interventions abroad and low-intensity internal
insurgencies are excluded.

### Validation

`scripts/validate_data.py` validates **both** `groups.json` and
`country_attributes.json` in one run. For attributes it checks: every key is a
known ISO2 code, every interval is well-formed (`from <= to`, or `to` null), and
`cultural_region` / `government.form` are drawn from the vocabularies above. For
groups it checks: `label` is a non-empty string, `story` is `null` or a string,
and every member is a known ISO2 code. Run it before committing changes to either
file.

---

## bias_matrix.json (derived)

Directed country-to-country **vote bias** measured against a null model, for the
academic bias-matrix view. **Do not hand-edit** — regenerate with
`python scripts/build_bias_matrix.py` whenever `votes_final.json` changes. A
`_meta` object documents the method; the data lives in parallel arrays indexed by
`pairs`.

**Null model.** For each year, every giver's actual point budget is distributed
under the hypothesis of *no identity bias* — proportional to each receiver's
general appeal, measured **leave-one-out** (points the receiver got from everyone
*except* this giver, so a giver cannot inflate its own expectation). This removes
the two confounds that distort raw point averages: a giver's **generosity** and a
receiver's **general strength**. Per-year normalisation also absorbs the
scoring-system changes 1957–2026 (see *Scoring rules* below), so eras are
comparable without rescaling.

| Field | Type | Notes |
|---|---|---|
| `pairs` | `[giver, receiver][]` | ISO2 ordered pairs; the index used by every other array. Directed — `[SE, NO]` ≠ `[NO, SE]` |
| `O` | number[] | **Observed** total points the giver gave the receiver across all shared finals |
| `E` | number[] | **Expected** total points under the null model |
| `bias` | number\|null | `O / E`. `1.0` = exactly as expected, `>1` = bias toward, `<1` = bias against. `null` if `E = 0` |
| `log2bias` | number\|null | `log2(O / E)` — the symmetric value to colour a diverging matrix on |
| `p` | number[] | One-sided **permutation** probability that bias ≥ observed under the null (4,000 draws, fixed seed). Small `p` = significant klüngel |

Significance comes from a permutation test (Gumbel-top-k weighted sampling without
replacement) that preserves each giver's exact point multiset and the year's
finalist set, so it respects receiver strength rather than re-flagging strong
receivers. `p = (count of draws ≥ observed + 1) / (draws + 1)`, so the floor at
4,000 draws is `≈ 0.00025`; true p-values below that are censored at the floor.

## bias_seriation.json (derived)

A **community ordering** of the 52 countries so the bias matrix renders as clean
diagonal blocks instead of a scattered field. Built by running **Louvain**
community detection on the undirected graph whose edges are the significant,
positive, substantive bias relations from `bias_matrix.json` (edge weight =
summed `log2(bias)`). Same generator, rebuilt alongside the matrix.

| Field | Type | Notes |
|---|---|---|
| `order` | string[] | The 52 ISO2 codes in row/column order. Feed this to the matrix axes |
| `communities` | object[] | The diagonal blocks, ordered by internal cohesion (tightest klüngel first) |
| `communities[].start_index` / `end_index` | integer | Inclusive bounds into `order`, for drawing block separators |
| `communities[].suggested_label` | string | The dominant curated-group key (`yugoslavia`, …) or `—` / `ungebunden`; a hint, meant to be renamed by a human |
| `communities[].curated_tags` | object | Count of curated-group memberships in the block, so the auto label is traceable |
| `communities[].members[]` | object | `{ iso, name, within_strength }`, ordered by intra-block klüngel strength |

`_meta` records the achieved `modularity` and the `chosen_seed`: Louvain is
stochastic, so the generator keeps the best of many seeded runs and stores the
winning seed, making the partition deterministically reproducible. Countries with
no significant klüngel edge (e.g. Morocco) collect in a trailing `ungebunden`
block so the matrix still spans all 52 participants.

---

## Scoring rules over the years

Comparing scores across eras requires knowing how the voting system changed.

| Period | Who voted | Jury/televote split published |
|---|---|---|
| 1956–1996 | National juries only | n/a |
| 1997–2008 | Public televote only | n/a |
| 2009–2015 | 50/50 jury + televote | No — only the combined total was published |
| 2016–present | 50/50 jury + televote | Yes — jury and televote scores published separately |

**Before 1975**: Each country's jury awarded points on varying scales across years — not directly comparable to later scores.

**1975**: The 12-point scale standardised — each country awards 1–8, 10, and 12 points to their top 10 songs. Used ever since.

**2016**: Jury and televote scores published separately for the first time. `final_jury_points` and `final_televote_points` are only non-null from this year onward.

**2023**: Non-participating countries (Rest of World) can vote via the Eurovision app. Their combined vote appears as `from_country_iso2 = "WLD"` in `votes_final.json`.

---

## Tips for working with the data

- **Joining files**: use `performance_id` as the key. `votes_final.to_performance_id` → `entries.performance_id`.
- **Filtering to finalists only**: `entries.json` includes semi-final eliminations (from 2004). Filter on `is_finalist == true` to get only grand final participants.
- **Jury/televote analysis**: only valid from 2016. `final_jury_points` and `final_televote_points` are null for all earlier years.
- **Multilingual songs**: `language_sung` uses `|` as a separator. Split on `|` to get individual languages. Example: `"English|French|Hebrew"` → `["English", "French", "Hebrew"]`.
- **Historical countries**: Yugoslavia (`YU`) and Serbia & Montenegro (`CS`) appear with their historical ISO codes. `country_modern` is null for these rows.
- **1956**: no vote data, no official scores, and each country entered two songs. Treat it as a special case.

---

## Data sources

| Source | Coverage |
|---|---|
| [josago97/EurovisionDataset](https://github.com/josago97/EurovisionDataset) | 1956–2024 |
| [EurovisionWorld](https://eurovisionworld.com) | 2025–2026 |

**For educational use only.** Do not redistribute or publish this dataset online.


