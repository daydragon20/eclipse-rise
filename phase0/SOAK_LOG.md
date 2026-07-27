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
