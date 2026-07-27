# Soak-logboek

Elke draai van `Eclipse\Tools\verify.ps1` schrijft hier een regel — GROEN en ROOD.
Zonder de rode nachten is `drie nachten achtereen` niet te bewijzen.

> **De ROOD-regel van 27-07 07:38 is met opzet veroorzaakt.** Ik heb er een rij uit
> `EventCatalog.md` gehaald om te controleren dat dit logboek ook bij een rode bar
> schrijft — anders zou een reeks groene regels nooit iets bewijzen. Die rij staat
> er weer in (34/34). Hij blijft hier staan in plaats van gewist te worden: een
> logboek waar de mislukkingen uit verdwijnen zodra ze ongelegen komen, is precies
> het soort bewijs dat niets waard is.

| Wanneer | Commit | Uitslag | Suite | Opnames | Waarop het viel |
|---|---|---|---|---|---|
| 2026-07-27 07:36 | `1eced40` | **GROEN** | 180 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 07:38 | `1eced40` | **ROOD** | 180 tests / 0 gefaald / 0 niet gedraaid | 0 | event-catalogus klopt niet |
| 2026-07-27 07:41 | `1eced40` | **GROEN** | 180 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 07:47 | `eff53b8` | **GROEN** | 180 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 07:50 | `cf97725` | **GROEN** | 180 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 07:58 | `644edfa` | **GROEN** | 180 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:07 | `896ea9a` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:13 | `ef96138` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:18 | `4619de5` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 08:30 | `54fed33` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:35 | `531bf71` | **ROOD** | 181 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Playthrough.EnemiesEngageWhenYouWalkIntoTheirRange |
| 2026-07-27 08:39 | `531bf71` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:47 | `31c06e1` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 08:51 | `250ae38` | **ROOD** | 181 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Playthrough.EnemiesEngageWhenYouWalkIntoTheirRange |
| 2026-07-27 08:54 | `250ae38` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 09:01 | `5b81d52` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 9 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 09:05 | `f6f581e` | **ROOD** | 181 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Playthrough.EnemiesEngageWhenYouWalkIntoTheirRange |
| 2026-07-27 09:08 | `f6f581e` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 09:17 | `55ffaf6` | **GROEN** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 09:33 | `8382a9d` | **ROOD** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 6 beeld(en) VERANDERD -> stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 09:38 | `8382a9d` | **ROOD** | 181 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 09:47 | `79e9054` | **GROEN** | 182 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 09:50 | `79e9054` | **ROOD** | 182 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Mission.Playthrough.HubStaysClickableAfterDeath |
| 2026-07-27 10:01 | `79e9054` | **GROEN** | 182 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 10:04 | `6933fa7` | **ROOD** | 182 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 7 beeld(en) VERANDERD -> stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 10:11 | `ceed908` | **ROOD** | 183 tests / 0 gefaald / 0 niet gedraaid | 0 | validatie gaf geen uitslag |
| 2026-07-27 10:23 | `263541e` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 10:27 | `263541e` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 10:37 | `bcbce69` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 10:43 | `d0aaf04` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest; opnameronde: 1 fout(en) in het frame [beeld: 7 beeld(en) VERANDERD -> stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 10:49 | `406edd2` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest; opnameronde: 1 fout(en) in het frame [beeld: 9 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 10:58 | `cb48bfb` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest; opnameronde: 1 fout(en) in het frame [beeld: 9 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 11:10 | `5ffc8ae` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 11:14 | `114cfed` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 11:20 | `2c5b9c4` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 11:24 | `8466469` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 11:30 | `ecd5bcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 11:37 | `907509c` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 11:49 | `9ede3a0` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 8 beeld(en) VERANDERD -> stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 11:55 | `5680703` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 12:01 | `77befb0` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 3 fout(en) in het frame [beeld: 2 beeld(en) VERANDERD -> stap 8, stap 9] |
| 2026-07-27 12:05 | `77befb0` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 3 fout(en) in het frame |
| 2026-07-27 12:15 | `b179692` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 3 fout(en) in het frame |
| 2026-07-27 12:27 | `b673902` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 12:34 | `4dd040a` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Audio.Subsystem.BusContract |
| 2026-07-27 12:37 | `4dd040a` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 12:43 | `42de670` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
