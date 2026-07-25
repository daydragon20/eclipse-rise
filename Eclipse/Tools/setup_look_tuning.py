"""Zet de look-waarden op DA_CharacterTuning die niet meer bij de code passen.

Waarom dit script bestaat (feel-audit S3-nasleep, 2026-07-25): een opgeslagen
DataAsset wint van de C++-default voor elk veld dat het asset al KENT. Toen
`bEnableLegacyInputScales` uitging, verdween een verborgen factor 2.5 uit
AddYawInput/AddPitchInput. Voor de STICK is dat de bedoeling — 240 gr/s hoort
240 gr/s te zijn — maar voor de MUIS is `MouseLookScale` een kale schaal zonder
eenheid, en daar is de eerlijke keuze het gedrag ongewijzigd laten. De CDO staat
daarom op 2.5; dit script trekt het asset mee, want anders wint de 1.0 die er in
2026-07-22 in gebakken werd en wordt de muis 2,5x trager.

Draaien (commandlet-slot):
  UnrealEditor-Cmd.exe <project> -run=pythonscript -script="Tools/setup_look_tuning.py"
    -unattended -nopause -nosplash

Idempotent: schrijft alleen als de waarde afwijkt, en zegt per veld wat het deed.
"""

import unreal

ASSET_PATH = "/Game/Data/DA_CharacterTuning"

# Veld -> gewenste waarde. Houd dit gelijk aan de CDO-defaults in
# EclipseCharacterTypes.h; AEclipseCharacter::ApplyTuning waarschuwt luid zodra
# de twee uiteenlopen, dus een vergeten rij hier wordt vanzelf zichtbaar.
WANTED = {
    "mouse_look_scale": 2.5,
}

editor_asset = unreal.EditorAssetLibrary

if not editor_asset.does_asset_exist(ASSET_PATH):
    raise RuntimeError(f"{ASSET_PATH} bestaat niet — draai eerst create_phase1_content.py")

tuning = editor_asset.load_asset(ASSET_PATH)
changed = False

for prop, wanted in WANTED.items():
    current = tuning.get_editor_property(prop)
    if abs(float(current) - float(wanted)) < 1e-4:
        unreal.log(f"[look-tuning] {prop} staat al op {wanted} — ongemoeid gelaten")
        continue
    tuning.set_editor_property(prop, wanted)
    changed = True
    unreal.log_warning(f"[look-tuning] {prop}: {current} -> {wanted}")

if changed:
    if not editor_asset.save_asset(ASSET_PATH):
        raise RuntimeError(f"opslaan van {ASSET_PATH} mislukt")
    unreal.log(f"[look-tuning] {ASSET_PATH} opgeslagen")
else:
    unreal.log("[look-tuning] niets te doen")
