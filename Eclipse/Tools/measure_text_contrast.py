#!/usr/bin/env python3
"""Leest tekst op een frame af zoals een LEZER hem ziet: het slechtste punt.

WAAROM DIT BESTAAT, en waarom het de derde keer is dat het gebouwd wordt.

De HUD-ronde van 01-08 mat contrast eerst over een VAK (98e percentiel tekst
tegen 10e percentiel achtergrond) en kwam op 11 : 1 waar de inspecteur 1,39 : 1
mat. Een gemiddelde over een vak middelt precies weg waar witte letters een
felle balk RAKEN -- en dat ene punt is waar het lezen stukgaat. Deze meter
rapporteert daarom het SLECHTSTE achtergrondpunt onder de tekst, niet het
gemiddelde.

EN DE VALKUIL DIE DE VORIGE RONDE ZELF VOND: de "achtergrond" pal naast een
letter is de ANTIALIAS-RAND VAN DIEZELFDE LETTER. Die loopt per definitie van
achtergrond naar tekst, dus er zit altijd een pixel bij die exact even licht is
als de letter -- en dan meet elke tekst 1,0x : 1, ook tekst die aantoonbaar
leesbaar is. Vandaar de halo-uitsluiting van 2 px (--halo).

HOE DE LETTERS GEVONDEN WORDEN, zonder te weten welke kleur ze hebben.
Een top-hat: letters zijn DUNNE lichte structuren. Een morfologische opening
(min-filter gevolgd door max-filter) veegt alles weg wat dunner is dan de
kernel; wat daarna nog uitsteekt is de tekst. Een felle zonnebanner of een
lichte muur overleeft die opening en blijft dus gewoon ACHTERGROND -- precies
wat je wilt, want dat is het vlak waar de leesbaarheid op stukloopt.
Een drempel op absolute helderheid zou de banner als "tekst" tellen en het
defect wegpoetsen.

Grootheden: WCAG relatieve luminantie (sRGB-EOTF, Rec.709-weging) en
contrastverhouding (L_licht + 0,05) / (L_donker + 0,05). Let op: 4,5 : 1 is de
WCAG-drempel en NIET de projectnorm -- het project heeft die nooit aangenomen
(REFERENTIE_HUD_BORDERLANDS.md r49 vraagt "binnen een halve seconde afleesbaar,
getoetst op een gevechtsframe"). Noem er dus altijd bij welke van de twee je
gebruikt.

Gebruik:
  python measure_text_contrast.py <shot.png> naam=x0,y0,x1,y1 [naam2=...]
  python measure_text_contrast.py --zelftest
"""
from __future__ import annotations

import statistics
import sys

try:
    from PIL import Image, ImageChops, ImageFilter
except ImportError:  # pragma: no cover - zelfde melding als de andere meters
    sys.exit("Pillow ontbreekt: pip install Pillow")

LUMA = (0.2126, 0.7152, 0.0722)

# Kernels. 7 px opening: de letters op dit bord staan op korpsgrootte 12-18 en
# hun stokken zijn 1-3 px breed, dus ze overleven een opening van 7 niet. Een
# banner, een muur of een plaat wel.
OPEN_KERNEL = 7
# Hoeveel een pixel boven zijn eigen omgeving moet uitsteken om als letter te
# tellen (0-255, gammaruimte). Lager pakt de antialias-staart mee; die wordt
# door de halo alsnog uitgesloten, dus liever iets te laag dan te hoog.
TOPHAT_THRESHOLD = 25


def srgb_to_linear(channel_0_255: float) -> float:
    c = channel_0_255 / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def relative_luminance(rgb: tuple[int, int, int]) -> float:
    return sum(w * srgb_to_linear(c) for w, c in zip(LUMA, rgb))


def contrast(l_a: float, l_b: float) -> float:
    hi, lo = max(l_a, l_b), min(l_a, l_b)
    return (hi + 0.05) / (lo + 0.05)


def glyph_masks(img: Image.Image) -> tuple[Image.Image, Image.Image]:
    """(lichte dunne structuren, alle dunne structuren).

    TWEE maskers, en de tweede is de reparatie van de eerste poging. Een
    top-hat op alleen LICHT vindt geen letter die precies even licht is als de
    banner erachter -- en dat is nu net het defect dat gemeten moet worden. De
    meter viel daar letterlijk op om ("geen tekstpixels gevonden").

    Het donkere tegendeel (een sluitings-top-hat) vindt wel altijd iets, want
    elke regel op dit bord draagt een harde zwarte inktschaduw van 1 px
    (15.5). Die schaduw is dus de betrouwbare vindplaats van de tekst, ook waar
    de letter zelf onzichtbaar is.

    Het eerste masker bepaalt WAT de tekstkleur is, het tweede WAT er niet als
    achtergrond mag meetellen.
    """
    lum = img.convert("L")
    opened = lum.filter(ImageFilter.MinFilter(OPEN_KERNEL)).filter(ImageFilter.MaxFilter(OPEN_KERNEL))
    closed = lum.filter(ImageFilter.MaxFilter(OPEN_KERNEL)).filter(ImageFilter.MinFilter(OPEN_KERNEL))
    to_mask = lambda im: im.point(lambda v: 255 if v > TOPHAT_THRESHOLD else 0, mode="L")
    light = to_mask(ImageChops.subtract(lum, opened))
    dark = to_mask(ImageChops.subtract(closed, lum))
    return light, ImageChops.lighter(light, dark)


def measure(
    img: Image.Image,
    box: tuple[int, int, int, int],
    halo: int = 2,
    text_rgb: tuple[int, int, int] | None = None,
) -> dict:
    x0, y0, x1, y1 = box
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(img.width, x1), min(img.height, y1)
    if x1 <= x0 or y1 <= y0:
        raise ValueError(f"lege rechthoek na clampen: {box} op {img.width}x{img.height}")

    # Marge meenemen bij het maskeren, zodat het filter aan de rand van het vak
    # niet zijn eigen randeffect meet, en daarna terugsnijden.
    pad = OPEN_KERNEL + halo + 1
    wide = (max(0, x0 - pad), max(0, y0 - pad), min(img.width, x1 + pad), min(img.height, y1 + pad))
    crop_wide = img.crop(wide).convert("RGB")

    light, thin = glyph_masks(crop_wide)
    halo_mask = thin.filter(ImageFilter.MaxFilter(2 * halo + 1)) if halo > 0 else thin

    inner = (x0 - wide[0], y0 - wide[1], x1 - wide[0], y1 - wide[1])
    rgb = crop_wide.crop(inner).tobytes()
    l_bytes = light.crop(inner).tobytes()
    t_bytes = thin.crop(inner).tobytes()
    h_bytes = halo_mask.crop(inner).tobytes()

    text_l: list[float] = []
    back_l: list[float] = []
    clipped = 0
    for i, (is_light, is_thin, is_halo) in enumerate(zip(l_bytes, t_bytes, h_bytes)):
        px = (rgb[3 * i], rgb[3 * i + 1], rgb[3 * i + 2])
        lum = relative_luminance(px)
        if is_light:
            text_l.append(lum)
        if not is_thin and not is_halo:
            back_l.append(lum)
            if max(px) >= 255:
                clipped += 1

    if not back_l:
        raise ValueError(f"geen achtergrondpixels over in {box} na halo-uitsluiting")

    # De TEKSTwaarde is de mediaan van de letterkernen: de randen zijn per
    # definitie lichter of donkerder dan de letter zelf.
    #
    # LEEG is hier geen fout maar een MEETUITSLAG: geen enkele letter steekt
    # boven zijn ondergrond uit. Dan komt de waarde uit de geauthorde inktkleur
    # (--text), want de vraag blijft geldig -- sterker nog, dat is precies het
    # geval waarin hij het hardst gesteld moet worden.
    if text_l:
        # 90e PERCENTIEL en niet de mediaan. Slate zet elke letter met
        # antialiasing, dus de helft van de "letterpixels" is een RAMP van
        # ondergrond naar inkt. De mediaan daarvan ligt halverwege en maakt de
        # tekst systematisch donkerder dan hij is (gemeten op de zelftest:
        # mediaan 0,4355 waar de geauthorde inkt op 0,785 zit). Wat een lezer
        # ziet is de KERN van de letter.
        text_l.sort()
        text_median = text_l[min(len(text_l) - 1, int(0.90 * len(text_l)))]
        text_source = f"90e perc. van {len(text_l)} letterpixels"
    elif text_rgb is not None:
        text_median = relative_luminance(text_rgb)
        text_source = f"GEEN letter stak boven zijn ondergrond uit -- geauthorde inkt {text_rgb} aangenomen"
    else:
        raise ValueError(
            f"geen tekstpixels gevonden in {box} en geen --text= opgegeven -- "
            "de letters zijn niet lichter dan hun ondergrond, geef de geauthorde inktkleur mee"
        )
    # En het SLECHTSTE achtergrondpunt: de pixel die qua luminantie het dichtst
    # bij de tekst ligt. Daar loopt het lezen stuk, en nergens anders.
    worst_bg = min(back_l, key=lambda l: abs(l - text_median))
    return {
        "n_text": len(text_l),
        "text_source": text_source,
        "n_back": len(back_l),
        "text_l": text_median,
        "bg_min": min(back_l),
        "bg_max": max(back_l),
        "bg_mean": sum(back_l) / len(back_l),
        "bg_stdev": statistics.pstdev(back_l),
        "bg_worst": worst_bg,
        "clipped_frac": clipped / len(back_l),
        "contrast_worst": contrast(text_median, worst_bg),
        "contrast_mean": contrast(text_median, sum(back_l) / len(back_l)),
    }


def report(name: str, m: dict) -> None:
    print(
        f"{name}: contrast SLECHTSTE PUNT {m['contrast_worst']:.2f} : 1"
        f"  (gemiddeld vlak {m['contrast_mean']:.2f} : 1)"
    )
    print(
        f"    tekst L={m['text_l']:.4f} ({m['text_source']}) · ondergrond"
        f" L {m['bg_min']:.4f}-{m['bg_max']:.4f}, gem {m['bg_mean']:.4f},"
        f" spreiding {m['bg_stdev']:.4f}, geclipt {100 * m['clipped_frac']:.2f}%"
        f" ({m['n_back']} px)"
    )


def zelftest() -> int:
    """Bewijst dat deze meter TWEE KANTEN OP kan.

    Een meter die alleen ooit hoge getallen heeft gegeven is niet te
    onderscheiden van `print(10.77)`. Twee beelden met dezelfde tekst: een op
    een plaat en een op een fel vlak. Zakt het onderscheid, dan is elk oordeel
    over een echt frame waardeloos.
    """
    from PIL import ImageDraw

    INK = (237, 230, 212)

    def frame(bg: tuple[int, int, int]) -> Image.Image:
        img = Image.new("RGB", (240, 60), bg)
        d = ImageDraw.Draw(img)
        # Mét de inktschaduw van 1 px, want die staat op elke regel van dit
        # project en de meter leunt erop om onzichtbare tekst terug te vinden.
        for text, y in (("AMMO 24 / 120", 8), ("SPIRE-GATED 2d", 30)):
            d.text((9, y + 1), text, fill=(0, 0, 0))
            d.text((8, y), text, fill=INK)
        return img

    box = (0, 0, 240, 60)
    op_plaat = measure(frame((18, 19, 24)), box, text_rgb=INK)
    op_fel = measure(frame((236, 232, 220)), box, text_rgb=INK)
    report("zelftest tekst op donkere plaat", op_plaat)
    report("zelftest tekst op fel vlak    ", op_fel)

    fouten = []
    if op_plaat["contrast_worst"] < 8.0:
        fouten.append(f"tekst op een plaat moet ruim boven 8 : 1 komen, kreeg {op_plaat['contrast_worst']:.2f}")
    if op_fel["contrast_worst"] > 1.5:
        fouten.append(f"tekst op een fel vlak moet richting 1 : 1 zakken, kreeg {op_fel['contrast_worst']:.2f}")
    # En de halo-valkuil zelf. Het dunne-structuurmasker vangt de dikke
    # antialias-rand al; de halo is de tweede lijn voor de FLAUWE staart die
    # net onder de drempel blijft. Zonder halo hoort de uitslag dus meetbaar
    # SLECHTER te zijn -- gebeurt dat niet, dan doet --halo niets en is de
    # uitsluiting een dood knopje.
    zonder_halo = measure(frame((18, 19, 24)), box, halo=0, text_rgb=INK)
    if zonder_halo["contrast_worst"] >= op_plaat["contrast_worst"]:
        fouten.append(
            f"halo-uitsluiting doet niets: zonder {zonder_halo['contrast_worst']:.2f} : 1,"
            f" met {op_plaat['contrast_worst']:.2f} : 1"
        )
    print(f"    (controleproef: dezelfde plaat zonder halo-uitsluiting = {zonder_halo['contrast_worst']:.2f} : 1)")

    if fouten:
        for f in fouten:
            print(f"ZELFTEST GEZAKT: {f}")
        return 1
    print("zelftest: de meter onderscheidt leesbaar van onleesbaar.")
    return 0


def main(argv: list[str]) -> int:
    if not argv or argv[0] == "--zelftest":
        return zelftest()

    path, *rects = argv
    halo = 2
    # De geauthorde inkt van de schermlaag (InkBone, FLinearColor 0.93/0.90/0.83
    # -> sRGB). Alleen gebruikt als GEEN enkele letter boven zijn ondergrond
    # uitsteekt; zie measure().
    text_rgb = (248, 244, 236)
    keep = []
    for arg in rects:
        if arg.startswith("--halo="):
            halo = int(arg.split("=", 1)[1])
        elif arg.startswith("--text="):
            hexval = arg.split("=", 1)[1].lstrip("#")
            text_rgb = tuple(int(hexval[i : i + 2], 16) for i in (0, 2, 4))
        else:
            keep.append(arg)

    img = Image.open(path).convert("RGB")
    print(f"{path} ({img.width}x{img.height}, halo {halo} px)")
    for spec in keep:
        name, _, coords = spec.partition("=")
        box = tuple(int(v) for v in coords.split(","))
        report(name, measure(img, box, halo=halo, text_rgb=text_rgb))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
