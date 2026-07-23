# Cleans the chosen FD_WarningSigns_V1 albedos into opaque sign placards for
# the toon pipeline (15.8 warning-sign pass). The pack is decal MIs over photo
# cutouts: the RGB under the alpha is green-screen/sky backing, so referencing
# the pack textures directly renders green cards on an opaque toon plane
# (audit 2026-07-23). This crops each sign to its alpha bounds and composites
# it over a dark steel plate so the placard reads as a mounted sign board.
#
# Chain (same two-step discipline as generate_decals.py):
#   1. UnrealEditor-Cmd ... -run=pythonscript -script=Tools/import_warning_signs.py
#      (first run exports the raw pack albedos to Saved/WarningSignStaging/raw)
#   2. python Eclipse/Tools/prepare_warning_signs.py          (this file, Pillow)
#   3. UnrealEditor-Cmd ... -script=Tools/import_warning_signs.py again
#      (imports the cleaned T_sign_*_diff.png into /Game/Art/Decals)
#
# Provenance: FD_WarningSigns_V1 (Fab free pack, machine-local; not committed).

import os

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # Eclipse/
STAGING = os.path.join(ROOT, "Saved", "WarningSignStaging")
RAW = os.path.join(STAGING, "raw")

# Pack index -> placard name (chosen from the 60-sign contact sheet, 15.8
# round: STOP for the gate portal, radiation for the crossing lamp, TOXIC for
# the west wall by Entry_Main). Curation pass 2026-07-23 adds four (see
# phase0/ASSET_CURATION.md): route arrow (checkpoint wayfinding), labor
# (worker sign, Underworks yards), blast (munitions/demolition), reactor
# (heavy-rust radiation triangle, exclusion zone).
SIGNS = {
    "09": "stop", "17": "radiation", "30": "toxic",
    "36": "route", "50": "labor", "51": "blast", "53": "reactor",
}

PLATE = (55, 55, 58)  # dark steel backing, tinted by the toon MID's cel bands
OUT_SIZE = 1024
MARGIN = 0.05  # sign-relative border of plate around the cutout

# Value normalization (15.8 art-fix round): the raw pack photos sit at wildly
# different exposures, so half the placards rendered as near-black plates
# (radiation/TOXIC, review shot 00012) while STOP read fine. Every cutout is
# scaled so its 99th-percentile max-channel hits NORM_TARGET - bright sign
# marks land near-white on the uniform dark plate, pushing placard value up
# toward (never onto) the yellow wall strips' emissive budget. The scale is
# uniform over RGB: value-only, hue stays with the artwork/palette (15.5).
NORM_TARGET = 245
NORM_MAX_SCALE = 2.4  # safety: never torture a placard more than this


def srgb_to_linear(v: float) -> float:
    v /= 255.0
    return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4


if not os.path.isdir(RAW):
    raise RuntimeError(f"raw staging missing: {RAW} (run Tools/import_warning_signs.py headless first)")

for index, name in sorted(SIGNS.items()):
    src = os.path.join(RAW, f"T_Albedo_WarningSigns_V1_{index}.png")
    if not os.path.isfile(src):
        raise RuntimeError(f"raw albedo missing: {src}")
    image = Image.open(src).convert("RGBA")
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise RuntimeError(f"{src}: fully transparent")
    # Square crop around the sign with a plate margin; clamp to the source.
    cx, cy = (bbox[0] + bbox[2]) / 2, (bbox[1] + bbox[3]) / 2
    half = max(bbox[2] - bbox[0], bbox[3] - bbox[1]) * (1.0 + 2.0 * MARGIN) / 2
    box = (int(max(0, cx - half)), int(max(0, cy - half)),
           int(min(image.width, cx + half)), int(min(image.height, cy + half)))
    sign = image.crop(box)
    # Normalize the cutout's value before compositing (see NORM_TARGET above):
    # p99 of max(R,G,B) over visible pixels -> scale to the shared anchor.
    visible = [max(p[0], p[1], p[2]) for p in sign.getdata() if p[3] > 8]
    visible.sort()
    p99 = max(1, visible[int(len(visible) * 0.99)] if visible else 255)
    scale = max(1.0, min(NORM_TARGET / p99, NORM_MAX_SCALE))
    if scale > 1.001:
        r, g, b, a = sign.split()
        lut = [min(255, int(v * scale + 0.5)) for v in range(256)]
        sign = Image.merge("RGBA", (r.point(lut), g.point(lut), b.point(lut), a))
    plate = Image.new("RGBA", sign.size, PLATE + (255,))
    plate.alpha_composite(sign)
    placard = plate.convert("RGB").resize((OUT_SIZE, OUT_SIZE), Image.LANCZOS)
    dest = os.path.join(STAGING, f"T_sign_{name}_diff.png")
    placard.save(dest)
    # Measured linear average, for the gain bookkeeping — the builder's per-sign
    # AlbedoGain is 1/mean so texturing never re-meters the dusk exposure.
    small = placard.resize((64, 64))
    mean = sum(srgb_to_linear(sum(p) / 3.0) for p in small.getdata()) / (64 * 64)
    print(f"{dest}: p99 {p99} scale {scale:.2f} linear-mean {mean:.3f} (1/mean = {1.0 / mean:.1f})")

print("done — run Tools/import_warning_signs.py headless to import.")
