# Testroute: missies en objectives nalopen zonder de campagne te spelen

*Voor de owner, 2026-07-25. Alle commando's hieronder zijn uit de bron geverifieerd
(`EclipseMissionSubsystem`, `EclipsePrepSubsystem`, `EclipseStrategySubsystem`,
`EclipseCampaignSubsystem`, `EclipseBaseSubsystem`, `EclipseEconomySubsystem`,
`EclipseSquadSubsystem`, `EclipseCommandModeComponent`). Ze zijn debug-tier per
GDD 14.5 en bestaan alleen in niet-Shipping builds.*

Console openen in een PIE- of game-sessie: **`~`** (tilde). Start de game met
`SPEEL_ECLIPSE.bat` in de repo-root.

## De volledige debug-commandolijst die er ís

| Commando | Wat het doet |
|---|---|
| `Eclipse.Prep.AutoLaunch` | lanceert de **geselecteerde** missie met standaard squad/loadout/insertion |
| `Eclipse.Mission.CompleteObjective <ObjectiveId>` | vinkt één objective af |
| `Eclipse.Mission.RaiseAlarm` | zet de alarm-latch van deze run (idempotent) |
| `Eclipse.Mission.ForceEnd <win\|lose>` | lost de debrief op |
| `Eclipse.Strategy.FlipRegion <RegionId>` | cyclet eigenaar Dominion → Contested → Player |
| `Eclipse.Campaign.AdvanceDay` | commit een dag-tick |
| `Eclipse.Campaign.GrantResource <Resource.Tag> <Amount>` | debug-resource erbij |
| `Eclipse.Campaign.ExportJson` | logt de hele campagne-state als JSON |
| `Eclipse.Roster.Kill <RosterIndex>` | debug-permadeath inclusief memorial |
| `Eclipse.Base.Report` | Hollow Point: slots, bouwstaten, ETA's, staf |
| `Eclipse.Base.Build <FacilityId> [SlotId]` | gevalideerde bouw-/upgrade-order |
| `Eclipse.Base.Vault` | rendert de begaanbare vault uit de state (of her-rendert) |
| `Eclipse.Economy.Report` | balansen + recente ledger-regels mét reden |
| `Eclipse.Squad.DumpOrders` | order-state per soldaat |
| `Eclipse.Command.Dump` | Command-Mode-state |
| `Eclipse.Events.Dump` | de laatste bus-feiten (handig als iets "niet lijkt te werken") |

## HET GAT: er is géén commando om een missie te SELECTEREN

`SelectMission(RegionId)` bestaat in C++ maar is niet als console-commando
geregistreerd. Daardoor is de route nu: **selecteer op het kaartscherm**, en gebruik
daarna `Eclipse.Prep.AutoLaunch`. Voor een echt snelle route mist er dus één schakel.

**Wat ik ga toevoegen** (staat in de rij, wacht op het build-slot):
`Eclipse.Strategy.SelectMission <RegionId>` — hetzelfde patroon als het bestaande
`Eclipse.Strategy.FlipRegion`, dus geen nieuw systeem, alleen de bestaande functie
blootgelegd in de debug-laag. Dan wordt de hele route console-only.

## M1.1 "Thirteen Bullets" van start tot debrief

De missie is gepind op **TransitCheckpoint** en heet intern `MT_M11`.

1. Start de game (`SPEEL_ECLIPSE.bat`). De campagne begint op dag 1.
2. Selecteer **Transit Checkpoint** op het kaartscherm. *(Straks:
   `Eclipse.Strategy.SelectMission TransitCheckpoint`.)*
3. Console: `Eclipse.Prep.AutoLaunch`
   → **Verwacht:** je zit in de missie, fase `Objectives`.
4. Console: `Eclipse.Mission.CompleteObjective Obj_M11_PatrolLeader`
   → **Verwacht:** het eerste doel vinkt af in de HUD.
5. Console: `Eclipse.Mission.CompleteObjective Obj_M11_Exfil`
   → **Verwacht:** de mandatory-set is compleet, fase gaat naar `Extraction`.
6. Console: `Eclipse.Mission.ForceEnd win`
   → **Verwacht, en dit is de kern van de test:**
   - **+50 credits en +25 materiaal** (check met `Eclipse.Economy.Report`; de
     ledger-regels dragen de reden `MissionReward`)
   - ~~**+20 materiaal extra** als niemand neerging~~ — **KOMT NIET (gemeten 26-07).**
     Die bonus hangt aan het optionele objective `Obj_M11_NoCasualties`, en de
     debrief betaalt alleen optionals uit die in `CompletedObjectiveIds` staan.
     Niets voltooit dat objective: er is geen vak om binnen te lopen en geen
     doelwit om neer te halen — het is een *voorwaarde*, geen taak. De speelronde
     bevestigt het: nul gewonden, 25 materiaal in plaats van 45. Wil je hem toch
     zien uitbetalen, dan moet je hem er zelf bij voltooien met
     `Eclipse.Mission.CompleteObjective Obj_M11_NoCasualties`.
   - **dag 1 → dag 2** (elke missie kost een dag, winst of verlies)
   - **Transit Checkpoint verandert NIET van eigenaar** — M1.1 verzet geen grens;
     dat doet pas M1.3 via de liberation-instantie
   - de **story-beat** staat gezet, dus het checkpoint biedt de missie niet meer aan

### De bonus zien falen (interessanter dan hem zien slagen)

Herhaal 1-5, maar laat onderweg iemand neergaan (of gebruik
`Eclipse.Roster.Kill <index>` vóór de debrief), en dan `ForceEnd win`:
→ **Verwacht:** de 50 C en 25 M komen wél, de **+20 M niet**. De bonus is een latch
over de hele run: wie neerging telt mee, óók als hij daarna gestabiliseerd is.

> **LET OP (26-07): deze proef onderscheidt op dit moment niets.** De +20 komt óók
> niet als er géén gewonden vallen, want het objective wordt nooit voltooid (zie
> hierboven). Wil je de latch echt zien werken, voltooi hem dan expliciet met
> `Eclipse.Mission.CompleteObjective Obj_M11_NoCasualties` — mét gewonden vervalt
> hij dan, zonder gewonden betaalt hij +20. Dát is de proef die wél iets zegt.

### Het verliespad

Bij stap 4 direct `Eclipse.Mission.ForceEnd lose`:
→ **Verwacht:** geen rewards, **geen** story-beat, dag kost tóch een dag, regio
onaangeraakt, en het checkpoint biedt M1.1 **opnieuw** aan — verliezen is geen muur.

## Alarm en de ghost-bonus

`Eclipse.Mission.RaiseAlarm` zet de alarm-latch. Die is idempotent (tweemaal
aanroepen doet niets extra) en **faalt de missie niet** — hij kost alleen bonussen
die stilte eisen. M1.1 heeft nog geen alarm-afhankelijke bonus; die komt met M1.2's
ghost-optional.

> **LET OP (26-07): dit pad is in M1.1 NIET te testen, ook al staat dat er eerder.**
> Twee redenen, allebei gemeten. Er is geen optional die stilte eist, dus er valt
> niets te laten vervallen. En "gemist" verschijnt alleen voor een optional die
> je eerst hebt *voltooid* — de debrief slaat optionals over die niet in
> `CompletedObjectiveIds` staan, dus een niet-voltooide optional verdwijnt
> sowieso stilletjes. Wil je de gemist-melding zien, dan moet je in M1.1 het
> casualty-objective expliciet voltooien én iemand laten neergaan; met alarm lukt
> het pas als M1.2 zijn ghost-optional meebrengt.
>
> **En het alarm zelf gaat tijdens spelen nooit af** — de enige aanroep is dit
> console-commando. Zie de owner-lijst; koppelen is een ontwerpbeslissing.

## Als iets niet lijkt te werken

`Eclipse.Events.Dump` is bijna altijd het snelste antwoord: als een objective niet
afvinkt of een reward niet aankomt, laat de feiten-stroom zien of het event wél
gevallen is. Geen event = de actie is niet gebeurd; wél event maar geen effect = de
consument hangt. Dat scheelt raden.

De verwachte waarden hierboven zijn geen aannames: ze staan als asserts in
`Eclipse.Missions.M11GauntletOnShippedData` en `Eclipse.Missions.M11LossKeepsStoryCold`,
die op elke groene bar meelopen tegen precies deze data.
