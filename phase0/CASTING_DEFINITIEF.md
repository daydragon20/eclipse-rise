# CASTING — DEFINITIEF

*Gegenereerd door `Tools/dump_casting_definitief.py` op 2026-08-01 10:09. **Afgeleid, niet de bron.***

De castingtab is uit het dashboard verwijderd omdat alle keuzes vastliggen en de poort ze bewaakt. Dit is wat er gekozen is.

**De bron blijft JSON — bewerk dít bestand nooit met de hand:**

| Bestand | Wat het is |
|---|---|
| `phase0/O16_KEUZE.json` | De tien slots die Nathan zelf koos. **Wint van alles.** |
| `phase0/CASTING_KEUZE.json` | Posities uit de shortlist (per rol een top-2) |
| `phase0/CASTING_RESOLVED.json` | De opgeloste binding — output van `resolve_casting_choice.py` |

**Vóór elke generatiebatch:**

```
python Eclipse/Tools/resolve_casting_choice.py
python Eclipse/Tools/check_voice_resolves.py     # exit 0 = de enige toestemming
```

Faalt de poort: genereer niets, meld welke rol hangt, stop. **Val nooit terug op een fallback-stem** — dat maakt de bar groen terwijl die regels stilte worden.

---

## Nathans eigen keuzes — 10 slots (O-16)

Deze winnen van de afgeleide binding.

| Rol | Slot | Stem | voice_id |
|---|---|---|---|
| `dominion_conscript` | B | Miguel - American English - Clear and Youthful | `6UZ6Y6OSl14UA2aOxuMM` |
| `eclipse_fighter` | B | Michael - Gruff and Serious | `iPYqcpY8j8Ap4jUf28I3` |
| `eclipse_fighter` | C | Beth - Bold and Smooth Narrator | `y3UNfL9XC5Bb5htg8B0q` |
| `eclipse_fighter` | D | Arric - Steady and Deadpan | `AOLUVLMs1jzrUpQeAea8` |
| `iron_chorus_emissary` | - | Marcus K - Calm Documentary Narrator | `3H55HGnNE1XjYxigHSAS` |
| `kaine` | - | Eryn – Strong Female Military Commander | `wa4sQVgbDDzUDEzJwch3` |
| `kaya` | - | Veronica - Sassy and Energetic | `ejl43bbp2vjkAFGSmAMa` |
| `petra` | - | Blue - Low, Calm with Grit  | `YGWwh1G8pUwWmJyCCpma` |
| `torren` | - | Onyx – Calm & Friendly Podcast Narrator | `An7r4HL8TAw73K5uSMpy` |
| `veil_operative` | B | Madison Ray - Sharp News Anchor | `FyrYFW3P9GUxA348YGWu` |

---

## Alle rollen — 20 met 40 finalisten

| Rol | Tier | Finalisten |
|---|---|---|
| `dominion_conscript` | Tier 1 - barks | Harry, Liam |
| `eclipse_fighter` | Tier 1 - barks | Roger, Matilda |
| `veil_operative` | Tier 1 - barks | River, Eric |
| `aegis` | Tier 2 | Daniel, Alice |
| `brick` | Tier 4 | Adam, Brian |
| `dex` | Tier 2 + 4 | Liam, Will |
| `iron_chorus_emissary` | Tier 2 - blokkeert M1.5 | Marcus K - Calm Documentary Narrator, Steve - Deep & Authoritative |
| `kaya` | Tier 2 | Veronica - Sassy and Energetic, Matilda |
| `mara` | Tier 2 + 4 | Matilda, Lily |
| `petra` | Tier 2 - blokkeert M1.8 | Blue - Low, Calm with Grit , Mother - Strong, Warm  & Calm |
| `reyes` | Tier 4 | Alice, Bella |
| `sela` | Tier 2 | Dr. Kendall, Bella |
| `threx` | Tier 2 | Eric, Chris |
| `torren` | Tier 2 | Onyx – Calm & Friendly Podcast Narrator, Onyx – Calm & Friendly Podcast Narrator |
| `voss_f` | Tier 2 - elke scene | Laura, Matilda |
| `voss_m` | Tier 2 - elke scene | Callum, Roger |
| `whisper` | Tier 2 | Dylan Malc - Calm & Educational, River |
| `callis` | systemisch | Victor - Deep, Dark Psychology, Uyi - Nigerian American Male  |
| `kaine` | Act 2-4 | Eryn – Strong Female Military Commander, Glinda - Confident, Sly, Sultry |
| `vex` | Act 3-4 | Bill, Victor - Deep, Dark Psychology |

---

## ⚠ Licentiecontrole open — 4 stemmen

`19_voice_production.md` §19.1 eist dat de commerciële licentie van een Voice-Library-stem **met de hand op de stemkaart** gelezen wordt; dat veld bestaat niet in de API. Dit is owner-werk en staat als kaart **O-20** op het dashboard. **Genereer niets voor die check rond is** — een stem zonder licentie moet je later vervangen, en dan herbetaal je élke regel van dat personage.

- **['iron_chorus_emissary', 'Marcus K - Calm Documentary Narrator']** ``
- **['iron_chorus_emissary', 'Steve - Deep & Authoritative']** ``
- **['petra', 'Blue - Low, Calm with Grit ']** ``
- **['petra', 'Mother - Strong, Warm  & Calm']** ``

---

## Conflicten

- Eerste keuze: **0** — geen; elke rol heeft een eigen stem.
- Inclusief reserves: **9** — reserves mogen elkaar overlappen zolang de *gekozen* stem uniek is; `check_voice_resolves.py` toetst alleen die laatste.

**Waarom uniekheid telt:** de stem-ID zit in de cachesleutel. Twee personages op dezelfde stem is niet 'lijkt op elkaar' maar letterlijk dezelfde stem — en later wisselen herbetaalt elke regel van dat personage.
