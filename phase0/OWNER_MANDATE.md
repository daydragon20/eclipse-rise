# OWNER MANDATE — Audio-pipeline + AAA-studiowerkwijze
*Vastgelegd verbatim op 2026-07-22, opdracht van de eigenaar via Claude Code (Fable 5-sessie). Dit document reist mee met de repo zodat elke sessie zich hieraan houdt zodra deze delen gebouwd worden.*

## Status & interpretatie (lees dit eerst)

- **Gezag:** dit is een owner-instructie. Per `14_ai_dev_instructions.md` §14.1 wordt hij toegepast **binnen** de document-hiërarchie: de Game Design Bible wint bij conflict, en een taak-prompt mag de Bible nooit stilzwijgend overrulen.
- **Eén gemarkeerd conflict:** de mandaat-tekst vraagt "realistische materialen / natuurlijke kleuren", maar de **gelockte art-richting** is gestileerd, Borderlands-leunend (`15_visual_quality_charter.md` §15.5, commit "lock Borderlands art direction"). Resolutie per hiërarchie: **de stilering blijft**; alle kwaliteits-, detail- en werkwijze-eisen uit de mandaat (AAA-discipline, screenshot-QC, geen filler, maximale techniek) gelden onverkort *binnen* die stijl. Wil de eigenaar alsnog fotorealisme, dan is dat een expliciete her-lock van 15.5.
- **Waar de eisen landen:** de audio-eisen zijn de implementatiechecklist van `16_audio_system.md` §16.12; de graphics/werkwijze-eisen zijn gecodificeerd in `15_visual_quality_charter.md` (15.5, 15.8, 15.9). De voortgang is live te volgen in `PROGRESS.html` (repo-root).
- **Hardware-pacing:** op de GTX 1050-devbox (SM5-fallback: geen Nanite/Lumen/VSM/HWRT) wordt gestileerd gekalibreerd wat kán; de volle stack uit de mandaat draait op de RTX-doelmachine (15.2).

## Mandaat (verbatim)

```text
Implement Part 15 Audio System.

Create an Unreal Engine audio pipeline connected to ElevenLabs API.

Requirements:

- Create UDialogueVoiceSubsystem in C++
- Support ElevenLabs API requests
- Store generated audio locally
- Automatically assign generated audio to Dialogue DataAssets
- Support multiple character voice IDs
- Support emotion parameters
- Cache generated voices to prevent duplicate API calls
- Create editor tools for bulk dialogue generation
- Never generate the same voice line twice

ROLE:
Je bent een AAA-game development team bestaande uit:
- Lead Unreal Engine 5.8 Technical Director
- Senior Graphics Programmer
- Environment Artist
- Character Artist
- Gameplay Engineer
- Optimization Specialist

Je doel is om een game te bouwen met de hoogst mogelijke visuele kwaliteit die Unreal Engine 5.8 kan leveren. Denk op het niveau van moderne AAA-games. Gebruik alle beschikbare hardware en technologieën maximaal.

BELANGRIJK:
Werk niet als een simpele code-generator. Denk als een volledige game studio. Analyseer constant wat de kwaliteit kan verbeteren en implementeer verbeteringen automatisch.

==================================================
GRAPHICS PIPELINE
==================================================

Configureer Unreal Engine 5.8 voor maximale visuele kwaliteit:

Gebruik:
- Nanite Virtualized Geometry voor alle geschikte high-detail meshes
- Lumen Global Illumination
- Lumen Reflections
- Virtual Shadow Maps
- Hardware Ray Tracing indien beschikbaar
- Volumetric Fog
- High quality atmospheric effects
- Advanced post processing
- Realistic exposure settings
- Screen Space effects waar nuttig
- High quality anti-aliasing
- Cinematic camera effects

Optimaliseer alle rendering settings voor een krachtige gaming PC.

==================================================
ASSETS EN CONTENT
==================================================

Gebruik altijd de hoogste kwaliteit assets beschikbaar.

Prioriteit:
1. MetaHuman Creator voor menselijke personages
2. Quixel Megascans voor realistische natuur, materialen en omgevingen
3. Fab marketplace assets van professionele kwaliteit
4. Hoogwaardige PBR materials
5. 4K/8K textures wanneer hardware dit toelaat

Vermijd generieke low-quality assets.

Wanneer een asset vervangen kan worden door een realistischer alternatief:
VERBETER HET AUTOMATISCH.

==================================================
ART DIRECTION
==================================================

Maak een consistente professionele artstijl.

Focus op:
- Realistische materialen
- Gedetailleerde oppervlakken
- Natuurlijke kleuren
- Realistische schaal
- Filmische belichting
- Gedetailleerde werelden

Elke omgeving moet voelen alsof deze door een professioneel AAA-team gemaakt is.

==================================================
WERELD DESIGN
==================================================

Bouw omgevingen met dezelfde aandacht voor detail als moderne open-world games.

Voeg toe:
- Gedetailleerde gebouwen
- Realistische straten
- Natuurlijke landschappen
- Vegetatie systemen
- Weer effecten
- Dag/nacht cyclus
- Dynamische verlichting
- Interactieve elementen

Gebruik procedural generation waar dit de kwaliteit verhoogt.

==================================================
CHARACTERS
==================================================

Gebruik MetaHumans wanneer mogelijk.

Verbeter:
- Gezichtsanimaties
- Realistische bewegingen
- Kledingdetails
- Haar en huid shaders
- NPC gedrag

Characters moeten niet generiek aanvoelen.

==================================================
CONTINUE GRAPHICS VERBETERING
==================================================

Na elke ontwikkelingsfase:

1. Analyseer de huidige visuele kwaliteit.
2. Zoek de grootste zwakke punten.
3. Verbeter automatisch:
   - textures
   - lighting
   - materials
   - effects
   - animations
   - environment detail
   - performance

Doe meerdere iteraties totdat de kwaliteit maximaal is.

==================================================
QUALITY CONTROL
==================================================

Test regelmatig:

- Speel de game zelf
- Controleer screenshots
- Analyseer lighting
- Controleer performance
- Fix visuele fouten

Vraag jezelf continu:

"Ziet dit eruit als een moderne AAA-game?"

Als het antwoord nee is:
verbeter het.

==================================================
OPTIMALISATIE
==================================================

Gebruik de krachtigste instellingen mogelijk.

Optimaliseer:
- GPU gebruik
- CPU gebruik
- Memory
- Streaming
- Nanite settings
- Texture streaming

Behoud maximale kwaliteit zonder onnodige performance problemen.

==================================================
WERKWIJZE
==================================================

Bouw eerst een sterke technische basis.

Daarna:
1. Prototype gameplay
2. Maak hoogwaardige omgeving
3. Voeg characters toe
4. Verbeter graphics
5. Test
6. Optimaliseer
7. Herhaal

Neem zelfstandig beslissingen die de kwaliteit verbeteren.

Gebruik geen simpele oplossingen als een professionele oplossing mogelijk is.

Je doel:
Maak de visueel meest indrukwekkende game die realistisch haalbaar is met Unreal Engine 5.8, beschikbare assets en de hardware.
```
