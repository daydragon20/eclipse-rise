# Gemini-beeldprompts — hoe ECLIPSE er straks uit gaat zien

Tien prompts om doelbeelden te genereren: de vier vaste review-camera's die de
dev-agent altijd schiet, plus zes andere kanten van de game.

**Hoe je ze gebruikt:** plak de **stijlbasis** hieronder, en daarachter één van de
tien scène-prompts. De stijlbasis houdt alle beelden in dezelfde look — anders
krijg je tien losse plaatjes in plaats van één game.

Deze beelden zijn *richtbeelden*, geen screenshots. Ze horen te tonen waar Fase 2
naartoe werkt, zodat je de echte shots ernaast kan leggen.

---

## De stijlbasis (altijd meesturen)

```
Stylized cel-shaded 3D game render in the Borderlands family of style: bold shapes,
strong readable silhouettes, hand-painted surface character, crisp black ink outlines
on silhouettes and interior seams, visible brush-stroke hatching in the shadows.
Punchy graphic-novel colour, not photoreal. Rendered in Unreal Engine 5 with soft
global illumination, volumetric smog, filmic tonemapping, subtle bloom and a light
film grain.

World: Kessara, the industrial underworld of a fallen interstellar civilisation,
occupied by the Dominion regime. Everything reads lived-in, ruled and worth
liberating: grime, wear, checkpoints, propaganda, layered damage history.

Palette: warm sodium gold (#FB9B35) as the dominant light, rusted oxide and salmon
concrete facades, worker teal accents, amber signal strips, resistance red for rebel
marks. Dusk lighting, low sun at a shallow angle, heavy industrial haze catching
light shafts.

Third-person action game framing at squad-command distance — the viewer reads the
whole space, not a close-up. 16:9.
```

---

## De vier vaste review-camera's

### 1 · Controlepost-compound (review-camera 1)
```
A Dominion checkpoint compound at the district entrance. Heavy concrete blast walls
in oxide and salmon tones, a lowered vehicle barrier, sandbag emplacements, sodium
floodlights throwing hard gold pools onto wet asphalt. Regime propaganda posters with
a stylized watching-eye emblem plastered on the west facade, hazard chevrons painted
on the ground. Two armoured enforcers in gold-trimmed gear standing guard. Smog haze
softens the far wall. Low dusk sun rakes across the facade from the left.
```

### 2 · Warehouse-straat (review-camera 2)
```
A narrow industrial street between corrugated-steel warehouses, seen down its length.
Loading docks with steel plating and gantry trusses overhead, oil stains and rust
bleeding across the asphalt, stacked crates and rusted barrels along the walls.
Resistance stencils — an eclipse symbol in red — sprayed on a shutter. Amber signal
strips glowing along the dock edge. Hanging smog, light shafts cutting between the
buildings, a distant factory skyline closing off the far end.
```

### 3 · Cover-veld (review-camera 3)
```
An open contested yard used as a firefight arena: scattered concrete barriers,
overturned road blockers, rubble piles and a wrecked transport providing cover in a
readable rhythm across the space. Blast scoring on the ground, twisted rebar, drifting
ash and embers. A perimeter wall with hazard striping behind. Late dusk light, long
shadows, the whole layout legible at a glance from a raised third-person camera.
```

### 4 · Overzicht district (review-camera 4)
```
A wide elevated overview of the whole occupied district: the checkpoint compound, the
warehouse street and the contested yard all readable in one frame, connected by a main
artery with painted lane markings. Beyond the perimeter wall, a dense skyline of
factory hulks, smokestacks and crane gantries fades into smog — no visible world edge,
the city continues to the horizon. Sodium lights speckle the depth. Dusk sky with a
low sun behind the stacks.
```

---

## Zes andere kanten van de game

### 5 · De skyline van Kessara
```
The Kessara skyline seen from the district: dozens of stacked factory hulks,
smokestacks venting into a heavy dusk sky, crane gantries silhouetted against the low
sun, sodium-lit window strips glowing in layered depth. Monumental industrial scale
that dwarfs the streets below. Layered silhouettes fading through atmospheric haze,
strong graphic separation between foreground, midground and horizon.
```

### 6 · Squad-portret (MetaHuman-kwaliteit)
```
Character portrait of a resistance squad of four standing in a loading bay, lit by one
sodium lamp. Distinct silhouettes and roles: an assault trooper in scavenged plating,
a medic with a field kit, a sniper with a long rifle slung, a heavy in salvaged armour.
Layered scavenged gear reading as improvised, not uniform — patched fabric, mismatched
plating, resistance red armbands. Stylized faces with clear expression, cel-shaded skin
with ink outlines, groom-based hair. Cinematic three-quarter framing, shallow depth.
```

### 7 · Command Mode (het tijdvertragings-moment)
```
The tactical Command Mode moment: the world slowed to a crawl, seen from a raised
third-person camera. A rebel squad frozen mid-advance across a contested street,
tracer rounds hanging in the air as bright amber streaks, dust and debris suspended.
A cool desaturated overlay across the world while the selected soldier and the order
markers stay fully saturated — golden order arcs and target reticles drawn over the
scene as clean graphic UI. Time itself looks held.
```

### 8 · Vuurgevecht op straat
```
An intense firefight in the warehouse street: rebel squad behind concrete cover on the
left, Dominion enforcers advancing under a gantry on the right. Muzzle flashes throwing
hard gold light onto nearby surfaces, tracer streaks, sparks ricocheting off steel,
smoke curling through light shafts, shell casings mid-air. Chunks blown out of a
barrier. Kinetic and readable — the viewer instantly sees who is where.
```

### 9 · Hollow Point — de verzetsbasis ondergronds
```
The interior of a hidden resistance base carved into an old maintenance vault beneath
the district. Rough concrete and exposed rebar, cabling strung along the ceiling, a
war-planning table lit from above by a single warm work lamp, bunks and salvaged
crates in the shadows, a wall of maps and photographs marked with red string. Cold
blue light leaking down a stairwell contrasts with the warm contraband glow inside.
Cramped, improvised, defiant.
```

### 10 · Nacht bij de propaganda-muur
```
Night in the occupied district at a towering propaganda wall: an enormous Dominion
banner with the stylized watching-eye emblem, backlit by cold surveillance floodlights.
Across its lower half a huge resistance eclipse symbol has been sprayed in red, paint
still running. A lone rebel figure walks away in silhouette, small against the wall.
Rain-slicked asphalt reflecting sodium gold and cold white in long streaks. Heavy smog,
strong graphic contrast between the regime's cold light and the resistance's warm red.
```

---

## Kleine tips

- Vraag om **16:9** en zet er "no text, no watermark, no UI" bij als Gemini letters
  in het beeld begint te plakken.
- Wil je een variant vergelijken (zoals de westgevel-A/B), genereer dan twee beelden
  met één verschil in de prompt — verander nooit twee dingen tegelijk.
- Bewaar de beelden in `progress_media/` met een duidelijke naam
  (`doel_cam1_checkpoint.png`), dan kan de dev-agent ze naast de echte shots leggen.
