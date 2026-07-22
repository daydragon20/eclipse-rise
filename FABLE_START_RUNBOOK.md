# FABLE START — het kaartje voor de andere computer

*Wat je zometeen doet om Claude Code op het juiste (Max / Fable 5) account te
laten draaien. De losse startknop is `start_fable.bat` (in deze repo én bedoeld
voor `C:\Dev\ECLIPSE_SECRETS\`). Zie ook `MIGRATION_TO_STRONG_PC.md` voor de
volledige dag-één-migratie.*

---

## De 4 stappen (in volgorde)

1. **Kopieer de login van de USB naar de vault.**
   Kopieer `ECLIPSE_SECRETS\fable-config\.credentials.json` van de **USB** over
   `C:\Dev\ECLIPSE_SECRETS\fable-config\.credentials.json` op de PC
   (overschrijf de oude — dit ververst de Max-login).
   *Staat `C:\Dev\ECLIPSE_SECRETS` nog niet op de PC? Kopieer dan eerst de hele
   map `ECLIPSE_SECRETS` van de USB naar `C:\Dev\`.*

2. **Dubbelklik `start_fable.bat`.**
   Ligt in `C:\Dev\ECLIPSE_SECRETS\` (kopieer 'm daarheen vanuit deze repo als
   hij daar nog niet staat). Hij zet `CLAUDE_CONFIG_DIR` naar de vault, gaat naar
   de repo-map (`C:\Dev\ECLIPSE_GDD`) als die bestaat — anders `C:\` — en start
   `claude --model fable --effort max --dangerously-skip-permissions`.
   Het script **controleert eerst** of de config-map en `.credentials.json`
   aanwezig zijn en zegt het meteen als er iets ontbreekt.

3. **Herstart VS Code** *als* je Claude daar via de fable-config gebruikt.
   Sluit VS Code helemaal af en open opnieuw, zodat de nieuwe
   `CLAUDE_CONFIG_DIR` / credentials worden opgepikt. (Gebruik je alleen het
   losse Claude Code-venster van `start_fable.bat`? Dan hoeft dit niet.)

4. **Test.** Stel een simpele vraag (bv. *"Op welk account draai ik?"*) en typ
   dan **`/model`** → moet **Fable 5** tonen. Zo niet: zie hieronder.

---

## Als `/model` geen Fable 5 toont / je op het verkeerde account zit

- Controleer dat `start_fable.bat` echt de vault gebruikt: bovenin het venster
  print hij `Config-dir : C:\Dev\ECLIPSE_SECRETS\fable-config`.
- Controleer dat `C:\Dev\ECLIPSE_SECRETS\fable-config\.credentials.json` bestaat
  en de **verse** kopie van de USB is (stap 1).
- **Fallback-login** (config-dir-route hapert): kopieer de losse
  `claude-credentials.json` van de USB naar
  `C:\Users\<jij>\.claude\.credentials.json`.
- **Duurzame fallback:** toegang tot de `rocadelobv@gmail.com`-inbox →
  wachtwoord-reset op claude.ai → normale `/login` in het venster.

---

## Waarom dit los staat van je eigen Claude-account

`start_fable.bat` zet **alleen voor dat venster** `CLAUDE_CONFIG_DIR` naar de
vault. Een eventueel eigen (Pro-)Claude-account op deze PC gebruikt de standaard
config-map (`C:\Users\<jij>\.claude`) en blijft hier volledig los van — je kunt
dus beide naast elkaar hebben zonder dat ze elkaar overschrijven.
