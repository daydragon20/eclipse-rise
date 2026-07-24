# Imports the ElevenLabs-generated staging audio from Saved/AudioStaging
# (generator: Tools/generate_audio_assets.py — cached, re-runnable) into
# /Game/Audio/{SFX,Music} as SoundWaves, plus basic SoundCues and attenuation
# defaults per GDD 16.2 (playback goes through the audio systems/bus later;
# this only creates the assets).
#
# ============================ WHEN TO RUN ============================
# NOT NOW. Do NOT run while the owner's UnrealEditor.exe is open (Live
# Coding session): check `tasklist | findstr UnrealEditor` first. This
# script is staged for a later round when the editor is free. Then either:
#   headless:  UnrealEditor-Cmd <project.uproject> -run=pythonscript \
#                -script="<repo>\Eclipse\Tools\import_generated_audio.py" \
#                -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash
#   in-editor: owner-idle editor, Window > Output Log > Python:
#                exec(open(r"<repo>\Eclipse\Tools\import_generated_audio.py").read())
# =====================================================================
#
# Driven by AudioStaging/manifest.json (single source of truth): only entries
# with status "ok" import; "suspect"/"needs_conversion" are skipped with a log
# line, never a crash (missing-asset fallback rule).

import json
import os
import unreal

STAGING = os.path.join(
    unreal.SystemLibrary.get_project_saved_directory(), "AudioStaging"
)
MANIFEST = os.path.join(STAGING, "manifest.json")

DEST = {"SFX": "/Game/Audio/SFX", "Music": "/Game/Audio/Music"}

# 16.2 defaults: world SFX get a shared attenuation; UI + music stay 2D.
ATT_SFX = ("ATT_SFX_Default", 400.0, 3500.0)   # inner radius, falloff
ATT_AMB = ("ATT_Ambient_Bed", 1200.0, 9000.0)  # layer-1 beds carry further (16.7)


def is_ui(name):
    return "_UI_" in name


def is_ambient(name):
    return "_Amb_" in name


if not os.path.isfile(MANIFEST):
    raise RuntimeError(
        f"manifest missing: {MANIFEST} (run Tools/generate_audio_assets.py first)")

with open(MANIFEST, "r", encoding="utf-8") as f:
    manifest = json.load(f)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def make_attenuation(asset_name, inner, falloff):
    path = f"{DEST['SFX']}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    att = asset_tools.create_asset(
        asset_name, DEST["SFX"], unreal.SoundAttenuation,
        unreal.SoundAttenuationFactory())
    if att is None:
        unreal.log_warning(f"could not create attenuation {asset_name}")
        return None
    settings = unreal.SoundAttenuationSettings()
    settings.set_editor_property("attenuation_shape_extents",
                                 unreal.Vector(inner, 0.0, 0.0))
    settings.set_editor_property("falloff_distance", falloff)
    att.set_editor_property("attenuation", settings)
    unreal.EditorAssetLibrary.save_loaded_asset(att)
    return att


att_sfx = make_attenuation(*ATT_SFX)
att_amb = make_attenuation(*ATT_AMB)

imported = []
for name, entry in sorted(manifest.get("assets", {}).items()):
    if entry.get("status") != "ok":
        unreal.log_warning(f"skip {name}: status={entry.get('status')}")
        continue
    src = os.path.join(STAGING, entry["file"].replace("/", os.sep))
    if not os.path.isfile(src):
        unreal.log_warning(f"skip {name}: staging file missing ({src})")
        continue
    dest = DEST.get(entry.get("category"), DEST["SFX"])

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])

    paths = list(task.get_editor_property("imported_object_paths"))
    if not paths:
        unreal.log_warning(f"import produced nothing for {name}")
        continue
    wave_path = paths[0]
    unreal.log(f"imported: {wave_path}")
    imported.append(wave_path)

    wave = unreal.load_asset(wave_path)
    if wave is None:
        continue

    # Ambient beds loop at the wave level (16.7 layer-1 never-silent floor).
    if is_ambient(name):
        wave.set_editor_property("looping", True)
        unreal.EditorAssetLibrary.save_loaded_asset(wave)

    # Basic SoundCue wrappers for world SFX; UI and music stay plain 2D waves.
    if entry.get("category") == "SFX" and not is_ui(name):
        cue_name = f"Cue_{name}"
        cue_path = f"{dest}/{cue_name}"
        if not unreal.EditorAssetLibrary.does_asset_exist(cue_path):
            factory = unreal.SoundCueFactoryNew()
            # UE 5.8: 'initial_sound_wave' is protected/deprecated - the
            # factory takes the array property now.
            factory.set_editor_property("initial_sound_waves", [wave])
            cue = asset_tools.create_asset(cue_name, dest, unreal.SoundCue, factory)
            if cue is not None:
                att = att_amb if is_ambient(name) else att_sfx
                if att is not None:
                    cue.set_editor_property("attenuation_settings", att)
                unreal.EditorAssetLibrary.save_loaded_asset(cue)
                unreal.log(f"cue: {cue_path}")

if not imported:
    raise RuntimeError("nothing imported — check AudioStaging + manifest status")
unreal.log(f"done: {len(imported)} waves into /Game/Audio")
