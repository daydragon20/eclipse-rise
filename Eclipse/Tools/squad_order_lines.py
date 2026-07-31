"""De regels die de squad zegt bij een order — EEN bron, twee lezers.

Waarom dit een eigen bestand is en geen blok in create_phase1_content.py: dat
script schrijft de HELE Phase-1-dataset, inclusief DT_Weapons en
DA_CharacterTuning. Zolang er een tweede bouwer aan het wapenwerk zit (O-5), zet
het draaien van dat script diens afstelling terug naar de scriptwaarden — en dat
merk je pas als het wapen weer anders voelt.

Dus: de rijen staan hier, create_phase1_content.py importeert ze, en
author_squad_order_lines.py schrijft ALLEEN DT_SquadOrderDefs. Wie alleen barks
wil bijwerken, raakt geen enkel ander asset aan.

De rijnamen zijn niet vrij te kiezen. Drie soorten:
  · de ordernaam uit EEclipseSquadOrder      (ack + weigering per ordertype)
  · de reden uit EEclipseOrderRefusalReason  (alleen de redenen die iets ANDERS
    betekenen dan hun ordertype — zie EclipseSquadOrderLogic::RefusalPoolRowName)
  · een overgangsnaam voor wat er KLAARSTAAT (Event.Squad.OrderQueued)

Ontbreekt een rij, dan valt de code luid terug op de stockzin "Copy." (14.3.5) —
hoorbaar fout in plaats van stil fout.
"""

SQUAD_ORDER_ROWS = [
    # --- SPEC-P1-06: de vier orders van Phase 1 --------------------------
    {"Name": "MoveTo",
     "AcknowledgeLines": ["On it.", "Moving.", "Copy - relocating."],
     "RefusalLines": ["No route, boss.", "Can't get there from here.", "That path's blocked."]},
    {"Name": "FocusTarget",
     "AcknowledgeLines": ["Target marked.", "On your mark.", "Engaging."],
     "RefusalLines": ["Can't see the target.", "No shot from here.", "Target's gone."]},
    {"Name": "Hold",
     "AcknowledgeLines": ["Holding.", "Anchored.", "Not moving."],
     "RefusalLines": ["Can't hold here."]},
    {"Name": "Regroup",
     "AcknowledgeLines": ["Falling back to you.", "Coming in.", "Regrouping."],
     "RefusalLines": ["No way back to you.", "Route's cut."]},

    # Een RIJ en geen veld (owner-beslissing 26-07, optie 1). Een neergeschoten
    # soldaat weigerde tot die dag met de zin van het ORDERTYPE: vroeg je hem te
    # verplaatsen, dan zei hij "No route, boss." en wees hij je op een
    # routeprobleem dat er niet was. AcknowledgeLines blijft leeg: wie neer ligt
    # bevestigt niets.
    {"Name": "Downed",
     "AcknowledgeLines": [],
     "RefusalLines": ["I'm hit - can't move.", "I'm down, boss.", "Need a medic, not an order."]},

    # --- SPEC-P2-02 Stage B: de rest van de 8.4-tabel --------------------
    {"Name": "Suppress",
     "AcknowledgeLines": ["Suppressing.", "Putting rounds on it.", "Heads down - firing."],
     "RefusalLines": ["No angle on that spot.", "Can't see it from here.", "Something's in the way."]},
    {"Name": "Flank",
     "AcknowledgeLines": ["Route's up - say go.", "I've got a way around. Your call.", "Line's plotted, boss."],
     "RefusalLines": ["No way around them.", "That flank's closed.", "Nothing but open ground that side."]},
    {"Name": "Breach",
     "AcknowledgeLines": ["Stacking up.", "On the door.", "Moving to breach."],
     "RefusalLines": ["Can't reach the stack.", "No path to that door."]},
    {"Name": "UseAbility",
     "AcknowledgeLines": ["Working it.", "On it.", "Doing what I'm good at."],
     "RefusalLines": ["Nothing for me to do here.", "Wrong moment for that."]},
    {"Name": "SyncStrike",
     "AcknowledgeLines": ["Taking mine.", "Going quiet.", "Mine's down."],
     "RefusalLines": ["Can't make that work."]},

    # De drie NIEUWE redenen. Elk zegt WELK probleem het is, niet dat er een is —
    # dat is het verschil tussen een antwoord en een muur.
    {"Name": "NoBreachPoint",
     "AcknowledgeLines": [],
     "RefusalLines": ["Nothing to breach here.", "There's no door, boss.", "Show me a way in and I'll take it."]},
    {"Name": "NoTargetsMarked",
     "AcknowledgeLines": [],
     "RefusalLines": ["Nothing marked.", "Mark them first, boss.", "I've got no targets."]},
    {"Name": "NotConcealed",
     "AcknowledgeLines": [],
     "RefusalLines": ["They're looking right at me.", "Too late for quiet.", "I'm made - can't do it silent."]},

    # Wat er KLAARSTAAT (Event.Squad.OrderQueued). Deze spreken uit de ACK-pool:
    # er is niets geweigerd, er wacht iets. Ook de negatieve overgangen krijgen
    # een regel - een voorstel dat stil verdampt is precies de stilte die 9.5
    # verbiedt, ook al is jouw nietsdoen geen weigering.
    {"Name": "FlankProposed",
     "AcknowledgeLines": ["Route's up. Say go.", "Ready when you are.", "Waiting on your word."],
     "RefusalLines": []},
    {"Name": "FlankApproved",
     "AcknowledgeLines": ["Moving.", "Going wide.", "On my way around."],
     "RefusalLines": []},
    {"Name": "FlankExpired",
     "AcknowledgeLines": ["Window's gone.", "Too slow - they moved.", "That line's closed now."],
     "RefusalLines": []},
    {"Name": "SyncStrikeMark",
     "AcknowledgeLines": ["Marked.", "Got him.", "That one's mine."],
     "RefusalLines": []},
    {"Name": "SyncStrikeFull",
     "AcknowledgeLines": ["That's four already.", "Hands are full, boss.", "No room for another."],
     "RefusalLines": []},
    {"Name": "SyncStrikePruned",
     "AcknowledgeLines": ["Lost one.", "He's moved off.", "Mark's gone."],
     "RefusalLines": []},
    {"Name": "BreachEntry",
     "AcknowledgeLines": ["Going in.", "Through.", "Now."],
     "RefusalLines": []},
    {"Name": "BreachAborted",
     "AcknowledgeLines": ["Can't get to the door.", "I'm cut off.", "Not making the stack."],
     "RefusalLines": []},
]
