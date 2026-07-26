"""Maak de pack-skeletten onderling COMPATIBEL, zodat anim-arme lichamen lenen.

Het probleem (locomotie-audit punt 3): vijf van de negen lichamen hebben geen
zijwaartse loopcyclus, omdat hun pack er geen levert. Ze schuiven daardoor
zijwaarts met een vooruit-cyclus onder hun voeten.

De code zei tot vandaag dat lenen onmogelijk was:

    "MESH AND ANIMS MUST COME FROM THE SAME PATH. Every pack ships its OWN COPY
     of UE4_Mannequin_Skeleton ... a different USkeleton asset"

Dat is waar op ASSETNIVEAU en het is de verkeerde conclusie. Nagemeten: alle acht
de packs dragen een kopie van hetzelfde `UE4_Mannequin_Skeleton`. Verschillende
assets, dezelfde botten. Precies waar UE5 `CompatibleSkeletons` voor heeft: je
vertelt een skelet dat het de animaties van een ander mag afspelen.

Dit script legt die verbanden. Daarna kan `setup_character_data.py` een
donorpack aanwijzen voor de takes die een lichaam zelf niet heeft.

DONOREN zijn expliciet en niet geraden. Elk anim-arm pack krijgt de donor die er
qua silhouet het dichtst bij zit — een soldaat leent van een soldaat, een
warrior van een warrior. Dat is geen kosmetiek: een loopcyclus draagt de houding
van het personage, en een zwaargepantserde pas onder een slank lichaam valt op.

Draaien:

    UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript ^
        -script="Eclipse\\Tools\\link_compatible_skeletons.py" -unattended -nullrhi
"""

import unreal

eal = unreal.EditorAssetLibrary

SKELETON = "Meshes/UE4_Mannequin_Skeleton"

# doelpack -> donorpack (het pack waarvan het de takes mag afspelen)
DONORS = {
    "SciFiGirl": "SciFiCharacter",
    "SciFiCharacterPack/SciFiGirl": "SciFiCharacterPack/SciFiSoldier",
    "SciFiSoldier02": "SciFiCharacter",
    "SciFiSoldier03": "SciFiCharacter",
    "SciFiWarrior02": "SciFiCharacterPack/SciFiWarrior",
}

linked = 0
for target_pack, donor_pack in DONORS.items():
    target_path = f"/Game/{target_pack}/{SKELETON}"
    donor_path = f"/Game/{donor_pack}/{SKELETON}"

    target = eal.load_asset(target_path)
    donor = eal.load_asset(donor_path)
    if target is None or donor is None:
        unreal.log_warning(
            f"COMPAT {target_pack}: skelet ontbreekt "
            f"(doel {'ok' if target else 'MISSING'}, donor {'ok' if donor else 'MISSING'})")
        continue

    already = [str(s.get_path_name()) for s in target.get_editor_property("compatible_skeletons")]
    if donor.get_path_name() in already:
        unreal.log(f"COMPAT {target_pack}: al gekoppeld aan {donor_pack}")
        continue

    target.add_compatible_skeleton(donor)

    # modify() + save_loaded_asset, en NIET save_asset(pad). Dat laatste schreef
    # niets: het asset stond niet als gewijzigd gemarkeerd, dus de save sloeg hem
    # over — en meldde geen fout. Dit script rapporteerde daardoor "5 koppelingen"
    # terwijl er nul op schijf stonden, en de runtime wees 2948 animaties af.
    #
    # Vandaar ook de controle hieronder: opnieuw laden en TELLEN. Een tool die
    # zegt dat hij iets deed zonder het na te kijken, is precies de stille fout
    # die hij moest opruimen.
    target.modify()
    if not eal.save_loaded_asset(target):
        unreal.log_warning(f"COMPAT {target_pack}: opslaan MISLUKT")
        continue

    check = eal.load_asset(target_path)
    stored = check.get_editor_property("compatible_skeletons") if check else []
    if len(stored) == 0:
        unreal.log_warning(f"COMPAT {target_pack}: na opslaan nog steeds leeg — de koppeling houdt niet")
        continue

    linked += 1
    unreal.log(f"COMPAT {target_pack} <- {donor_pack} (nagekeken: {len(stored)} in de lijst)")

unreal.log(f"COMPAT klaar: {linked} nieuwe koppelingen van {len(DONORS)}")
