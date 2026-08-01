# Opdracht — schrijf act 1 t/m 4 uit en zet de audio klaar

Lees eerst `STATUS.md`, `JOUW_ACTIES.md` en `phase0/SCRIPT_PRODUCTION_PLAN.md`.

Nathan heeft op 01-08 het dashboard uitgebreid: alle keuzes die hij moet maken
staan nu ín het dashboard, niet meer in markdown. Twee nieuwe plekken:

- **tab Casting** — bovenaan het blok **O-16**, tien slots, per slot een voorstel
  én een alternatief met fragment. Zijn keuze landt in `phase0/O16_KEUZE.json`
  als `{"rol:slot": {"voice_id": ..., "stem": ...}}`. Dit is een ANDER bestand
  dan `CASTING_KEUZE.json` — dat bewaart posities voor `resolve_casting_choice.py`
  en mag je niet met stem-id's vervuilen.
- **tab Geluidslaag** — credits, ledger, en elk gegenereerd fragment op schijf.
  Plus de O-14-knop, die schrijft `phase0/VOSS_TWEEDE_STEM_AANVRAAG.json`.

Werk die twee bestanden dus in, en meld nieuwe owner-keuzes voortaan als kaart
in het dashboard — niet als losse regel in een markdown-document.

---

## De stand van alle owner-vragen

| Kaart | Stand | Wat jij ermee doet |
|---|---|---|
| **O-16** casting | **BESLIST — de voorstelkolom** | Lees `phase0/O16_KEUZE.json`. Wijkt een slot af, dan wint dat bestand. Verwerk en draai de poort. |
| **O-14** Voss-kosten | **BESLIST — één stem** (`een-stem`) | Genereer act 1 met ÉÉN Voss-stem. Bespaart 20.570. De tweede stem alleen als `VOSS_TWEEDE_STEM_AANVRAAG.json` bestaat. **Nog open:** of de vier persoonlijkheidsassen ook gehalveerd worden (`varianten`, ~8.900) — vraag dat apart, Nathan heeft er niets over gezegd. |
| **O-17** ambient-burgers | **BESLIST — advies volgen** | De drie Kessara-burgers lenen een bestaande stem; de Dominion-officier krijgt een eigen. Maakt 12.833 credits en 8 scènes vrij. |
| **O-18** creditbedrag | **BESLIST — optie 3** | Bedrag én meetdatum in de statuskaart. |
| **O-4** ijkmissie M1.1 | **Lost zichzelf op** | Wacht op O-16. Zodra de poort groen is: genereer M1.1 en laat Nathan hem horen. |
| **O-12** verkeerd model | **WACHT OP NATHAN** | Advies: alle 51 opnieuw op `eleven_v3` (2.470 credits, 2% van budget). Vraag het als één kaart, genereer niets vooruit. |
| **O-19** kaine gewijzigd | **WACHT OP NATHAN** | Eén vraag: was de wijziging van kaine `[2]` → `[5,8]` bewust? Verwerk pas na bevestiging — casting is onomkeerbaar. |
| **O-15** vault-sfeer | **WACHT OP NATHAN** | Hij moet kijken, begin bij de barakken (00034). Blokkeert niets. |
| **T-8** MetaHuman | **WACHT OP NATHAN** | ~5 minuten owner-werk. Blokkeert niets. |

Drie kaarten wachten dus nog op hem, en **geen enkele blokkeert het schrijven**.

---

## STAP 1 — de poort staat GROEN (gemeten 01-08)

Nathan heeft alle tien O-16-slots vastgelegd. `resolve_casting_choice.py` legt
`phase0/O16_KEUZE.json` sinds 01-08 over de afgeleide binding heen — een keuze
per rol+slot wint van de shortlist-afleiding, en vult de slots die de shortlist
niet kon dekken. Uitslag:

```
OK: every speaker resolves to a role or is a declared uncast speaker,
    and no two speakers share a voice id.                      exit 0
```

Opgelost: `eclipse_fighter:B` → Michael (was Matilda, botste met Mara),
`dominion_conscript:B` → Miguel (was Liam, botste met Dex), `veil_operative:B`
→ Madison Ray (was Eric, botste met Threx), en de lege slots `C`/`D` → Beth en
Arric. Voor de emissaris koos Nathan **Marcus K**, niet Spartan.

Draai de poort opnieuw vóór elke generatiebatch:

```
python Eclipse/Tools/resolve_casting_choice.py
python Eclipse/Tools/check_voice_resolves.py
```

Exitcode 0 blijft de **enige** toestemming om audio te genereren. Faalt hij:
genereer niets, rapporteer welke rol/slot hangt, stop. Val nooit terug op een
fallback-stem om de bar groen te krijgen — dat verbergt stilte in plaats van hem
op te lossen.

**Wat nu nog tussen jou en de eerste batch staat:** de licentiecontrole hieronder
(owner-werk), en O-17 — de drie ambient-burgers en de Dominion-officier staan nog
als `uncast` en houden 12.833 credits / 8 scènes tegen. Dat besluit is genomen
(zie tabel): burgers lenen een bestaande stem, de officier krijgt een eigen.

Voor `eclipse_fighter:C` en `:D` blijft de ROL `eclipse_fighter` tot ná het
vastleggen (RULING L1-R30) — de rolsplitsing naar `iron_chorus_fighter` is het
tweede deel en is daarna gratis, want de cachesleutel hangt aan de stem-id.

**Licentiecontrole is owner-werk.** Negen van de tien voorstellen komen uit de
Voice Library en `19_voice_production.md` eist dat de commerciële licentie met de
hand op de stemkaart gelezen wordt. Zet de directe links klaar als kaart in het
dashboard. Genereer niets voor die check rond is.

---

## STAP 2 — schrijf ALLE dialogen. Dit is de hoofdopdracht.

Dit kost geen credits en wacht nergens op. Nathan wil dat je hier doorgaat.

**L1 — beat-sheets acts 2, 3 en 4.** De 34 resterende van de 42 missies (26
verhaalmissies M2.1–M4.7 + 8 loyaliteitsmissies). `phase0/beats/` bevat nu alleen
act 1. Gepland 01/08–11/08 en nog niet begonnen. story-architect, act voor act,
continuïteitsaudit aan het eind: elke twist geplant én betaald, geen weesdraad,
geen tegenstrijdigheid met `02_story_bible.md`.

**L2 — dialoog, elke regel van elke scène.** Act 1 heeft 71 scènes; acts 2–4 nul.
Lagen locken **per act**, niet per project: zodra act 2's beats staan mag je
daarop schrijven terwijl L1 nog aan act 3 werkt. dialogue-writer parallel, één
missie per agent, critic-gate per scène, `SCRIPT_FORMAT`-conform. Escaleer een
gat in de beats naar story-architect — geen stille rewrite.

Doel: acts 2–4 **script-compleet**. Dat is de opdracht, ook al krijgen ze geen stem.

---

## STAP 3 — audio, alleen als stap 1 groen is

Tier-volgorde: Tier 1 barks → Tier 2 act 1 → Tier 3 muziek → Tier 4 hub →
Tier 5 SFX. Budget **131.000**, vervalt 21/08, **harde stop generatie 19/08**.
Act 1 kost met één Voss-stem ~80.000 in plaats van ~100.000.

Acts 2–4 krijgen **geen** stem — die worden script-compleet afgemaakt op 20–21/08.
Werk `VOICE_LEDGER.md` bij na elke batch; de tab Geluidslaag meet bestanden op
schijf en zal van het ledger afwijken als er iets misgaat. Dat verschil is het
signaal, niet de fout.

---

## Werkregels

- Werk `progress_data.js` bij na elke iteratie. Nooit `PROGRESS.html`,
  `DASHBOARD.html` of `progress_auto.js` — die zijn van het dashboard.
- Commit per afgeronde stap.
- Bug: `phase0/DEBUG_DISCIPLINE.md`. Drie iteraties zonder diagnose = escaleren.
- Bewerk bestanden met de Edit-tool, niet met een script, tenzij de wijziging
  herhaald of berekend is.
- Owner-consent: geen installs, downloads of security-prompts zonder akkoord.
  Nathan heeft geen adminrechten — queue dat als owner-actie.
