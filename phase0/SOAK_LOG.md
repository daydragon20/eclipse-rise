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
| 2026-07-27 13:35 | `b8693b9` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 13:42 | `b8693b9` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 13:47 | `bc0c670` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.StepListNamesBothDevicesAndAnExpectation |
| 2026-07-27 13:52 | `bc0c670` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest |
| 2026-07-27 13:54 | `bc0c670` | **GROEN** | 184 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 14:12 | `e862cc6` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 14:18 | `c3c46ae` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 14:23 | `c3c46ae` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 9 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 14:33 | `69b7c7f` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 9 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 14:47 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 14:52 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 14:57 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:00 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:04 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:08 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:11 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 2 beeld(en) VERANDERD -> stap 1, stap 2] |
| 2026-07-27 15:16 | `08d3c77` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 5 beeld(en) VERANDERD -> stap 1, stap 2, stap 4, stap 5, stap 6] |
| 2026-07-27 15:21 | `ea4714d` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:25 | `ea4714d` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:37 | `3a42a4a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:41 | `3a42a4a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:46 | `a1c9dcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:50 | `a1c9dcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:54 | `a1c9dcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 15:58 | `a1c9dcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:02 | `a1c9dcd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:06 | `c74b12a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:09 | `c74b12a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:12 | `c74b12a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:16 | `c74b12a` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:25 | `e3b1815` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:30 | `62dc487` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:34 | `62dc487` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:38 | `62dc487` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:42 | `f45b3ce` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:46 | `f45b3ce` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:49 | `f45b3ce` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:53 | `f45b3ce` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 16:57 | `f45b3ce` | **ROOD** | 184 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Playthrough.M11PlaysItselfFromLaunchToDebrief; opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:01 | `f45b3ce` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 8 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7, stap 9] |
| 2026-07-27 17:08 | `8f24a4e` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame [beeld: 7 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 5, stap 6, stap 7] |
| 2026-07-27 17:13 | `df9e2e2` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:18 | `df9e2e2` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:21 | `df9e2e2` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:24 | `df9e2e2` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:32 | `a6076e0` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:37 | `998a8fd` | **ROOD** | 184 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:43 | `998a8fd` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 17:44 | `998a8fd` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Feel.LocomotionBlendsBetweenDirections |
| 2026-07-27 17:48 | `998a8fd` | **ROOD** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 17:55 | `1618914` | **ROOD** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 18:06 | `339d4ab` | **ROOD** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 18:09 | `339d4ab` | **ROOD** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | opnameronde: 1 fout(en) in het frame |
| 2026-07-27 18:14 | `dae67f5` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:17 | `dae67f5` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:19 | `dae67f5` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:29 | `1c42756` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:33 | `1c42756` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:36 | `1c42756` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:40 | `1c42756` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:45 | `1dfec1e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:48 | `1dfec1e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:53 | `c1a31c3` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 18:57 | `c1a31c3` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:00 | `c1a31c3` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:05 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 2 beeld(en) VERANDERD -> stap 1, stap 2] |
| 2026-07-27 19:09 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:14 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:18 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:22 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:25 | `1db1fb8` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:33 | `3f95319` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:36 | `3f95319` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:39 | `3f95319` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:43 | `3f95319` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:50 | `d10918c` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:54 | `d10918c` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 19:57 | `993c8f6` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 20:01 | `993c8f6` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 20:04 | `79684ae` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-27 20:13 | `84686b2` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 4 beeld(en) VERANDERD -> stap 6, stap 7, stap 8, stap 9] |
| 2026-07-27 20:23 | `8e2d07d` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-27 20:42 | `814872e` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Audio.Subsystem.BusContract |
| 2026-07-27 20:45 | `814872e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-31 17:28 | `a3f8cd7` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 17:39 | `9e0a66e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 1 beeld(en) VERANDERD -> stap 6] |
| 2026-07-31 17:42 | `9e0a66e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-31 17:46 | `9e0a66e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 1 beeld(en) VERANDERD -> stap 6] |
| 2026-07-31 17:50 | `9e0a66e` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 | - |
| 2026-07-31 17:55 | `74f0ea1` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 1 beeld(en) VERANDERD -> stap 6] |
| 2026-07-31 17:59 | `74f0ea1` | **GROEN** | 185 tests / 0 gefaald / 0 niet gedraaid | 9 |  [beeld: 1 beeld(en) VERANDERD -> stap 6] |
| 2026-07-31 18:17 | `f01d8a6` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 9 | Eclipse.Feel.Input.DocumentedConsoleCommandsExist [beeld: 1 beeld(en) VERANDERD -> stap 6] |
| 2026-07-31 18:24 | `23d2915` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Feel.Input.DocumentedConsoleCommandsExist |
| 2026-07-31 18:27 | `23d2915` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Feel.Input.DocumentedConsoleCommandsExist |
| 2026-07-31 18:29 | `23d2915` | **ROOD** | 185 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Feel.Input.DocumentedConsoleCommandsExist |
| 2026-07-31 19:17 | `bc881f4` | **ROOD** | 193 tests / 2 gefaald / 0 niet gedraaid | 12 | Eclipse.Characters.VitalsFeed.ThresholdAccumulates; Eclipse.Characters.VitalsFeed.WiredToBus [beeld: 7 beeld(en) VERANDERD -> stap 1, stap 2, stap 3, stap 4, stap 6, stap 7, stap 8] |
| 2026-07-31 19:37 | `72cdc83` | **GROEN** | 193 tests / 0 gefaald / 0 niet gedraaid | 12 |  [beeld: 2 beeld(en) VERANDERD -> stap 1, stap 7] |
| 2026-07-31 19:44 | `ec3dcd9` | **GROEN** | 193 tests / 0 gefaald / 0 niet gedraaid | 12 |  [beeld: 3 beeld(en) VERANDERD -> stap 1, stap 4, stap 7] |
| 2026-07-31 20:07 | `f2b82e5` | **ROOD** | 194 tests / 1 gefaald / 0 niet gedraaid | 27 | Eclipse.Squad.Orders.OrderLinesNameThePerson [beeld: 3 beeld(en) VERANDERD -> stap 1, stap 4, stap 7] |
| 2026-07-31 20:18 | `d8d1bb3` | **GROEN** | 195 tests / 0 gefaald / 0 niet gedraaid | 27 |  [beeld: 2 beeld(en) VERANDERD -> stap 1, stap 4] |
| 2026-07-31 20:19 | `d8d1bb3` | **ROOD** | 195 tests / 0 gefaald / 0 niet gedraaid | 0 | validatie gaf geen uitslag |
| 2026-07-31 20:47 | `bfeb073` | **ROOD** | 208 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Command.StageB.NewRefusalsSpeakAndCanBeResolved |
| 2026-07-31 21:22 | `5fcb6e8` | **ROOD** | 225 tests / 2 gefaald / 0 niet gedraaid | 0 | Eclipse.Strategy.Lanes.SmugglerRouteIsAThirdOutcomeNotASecond; Eclipse.Strategy.Liberation.WiringM11CompletionFlipsNothing |
| 2026-07-31 21:27 | `73d28d3` | **GROEN** | 225 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 21:30 | `73d28d3` | **GROEN** | 225 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 21:41 | `342c8d5` | **GROEN** | 225 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 21:43 | `84c9e6e` | **GROEN** | 225 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 21:45 | `84c9e6e` | **GROEN** | 225 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
| 2026-07-31 21:48 | `0f89026` | **ROOD** | 226 tests / 1 gefaald / 0 niet gedraaid | 0 | Eclipse.Strategy.Lanes.ShippedBoardActuallyUsesTheThreeStatuses |
| 2026-07-31 21:54 | `0fa85fb` | **ROOD** | 226 tests / 0 gefaald / 0 niet gedraaid | 0 | validatie gaf geen uitslag |
| 2026-07-31 21:54 | `0fa85fb` | **GROEN** | 226 tests / 0 gefaald / 0 niet gedraaid | 0 | - |
