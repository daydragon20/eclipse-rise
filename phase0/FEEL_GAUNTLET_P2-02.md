# Feel-gauntlet P2-02 Stage A — het R3-verdict (owner, ~20 min)
*Draaiboek voor de SPEC-P2-02 locked-decision-3-criteria. Dit verdict beslist of
Stage B (nieuwe verbs, camera, UI-polish) mag starten. Speel op de lelijkste
build — dat is expres (14.5): polish mag een "true" alleen beter maken, nooit
een verdict fabriceren.*

## Besturing (debug-bindings, staan ook op de HUD)
- **Command Mode:** houd **Q** (toetsenbord) of **LB** (pad) ingedrukt → wereld
  vertraagt naar 30%. Loslaten = normale tijd. Jij beweegt/kijkt gewoon door.
- **Soldaat kiezen (alleen tijdens de hold):** **Tab**/scroll-omhoog/**RB** =
  volgende · scroll-omlaag/**LT** = vorige · **E**/**X** = pak de soldaat onder
  je richtkruis. Geen selectie = order gaat naar iedereen (zoals Phase 1).
  (De "held"-teller op de HUD ververst alleen op events — dat is debug-grade,
  geen bug.)
- **Orders:** 1-4 / D-pad zoals altijd. **Stance:** Alt vasthouden (KB) of
  **Y** togglen (pad, tijdens de hold).

## Meet dit (streng zijn — een gefaald criterium = verdict "false")
1. **Order-round-trip:** 10 orders achter elkaar in Command Mode → elk antwoord
   (ack-bark of weigering) binnen 1 seconde ECHTE tijd. Doel: **10/10**.
2. **Targeting-leesbaarheid:** 10 orders onder vuur op een bedoelde soldaat +
   doel → landt hij bij de juiste in één keer? Doel: **≥9/10** (mis-picks = falsified).
3. **Dilatatie-comfort:** speel 3 encounter-beats. Voelt instappen als *sneller
   denken* of als *een menu openen*? Geen desoriëntatie/misselijkheid; loslaten
   hervat schoon (geen pop, geen opgegeten input).
4. **Vertrouwen onder dilatatie:** geen enkele stille orderfout terwijl vertraagd
   (elke order krijgt hoorbaar/leesbaar antwoord — let er actief op).
5. **Gebruiks-trek:** ga daarna vrij spelen. Stap je uit jezelf minstens 1× per
   encounter-beat de mode in? (Een ongebruikte mode is een gefaalde mode.)

## Telemetrie aflezen (voor je notities)
- Console: `Eclipse.Command.Dump` (mode-status, dilatatie, selectie, ordertelling).
- Elke keer dat je de mode verlaat logt `Event.Command.ModeExited` hoe lang je
  hem vasthield en hoeveel orders je gaf — dat is de gebruiks-trek-meting.

## Verdict doorgeven
Zeg tegen de agent: **"R3-verdict: true"** of **"R3-verdict: false"** + per
criterium kort wat je zag. Het verdict + de telemetrie worden vastgelegd in
phase0/EXECUTION_PLAN.md §2 (S4-statusregel).

Bij **false**: STOP — geen polish, geen Stage B. De vooraf geautoriseerde
fallback-ladder (spec-amendement, in volgorde): (a) dilatatie 0.5, (b) volledige
pauze als default, (c) hold-to-order zonder dilatatie.
