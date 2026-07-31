# ANTWOORDEN VAN DE OWNER
*Nathan beantwoordt vragen met een knop op het dashboard; ze komen hier terecht.*
*Agents: LEES DIT ELKE SESSIE. Een antwoord hier is bindend en telt als owner-instructie.*
*Voeg nieuwe vragen toe aan `phase0/owner_questions.json` — dan verschijnen ze bij hem op het scherm.*

| Wanneer | Vraag | Antwoord | Toelichting |
|---|---|---|---|
| 2026-07-31 19:32 | O-7 | agent-kiest |  |
| 2026-07-31 19:33 | T-11 | true |  |
| 2026-07-31 19:34 | T-10 | gedaan |  |
| 2026-07-31 19:34 | T-2 | gedaan |  |
| 2026-07-31 19:48 | O-6 | A |  |
| 2026-07-31 19:48 | O-5 | volledig |  |
| 2026-07-31 19:48 | O-8 | vol |  |
| 2026-07-31 20:05 | O-6 | A | Via de chat, niet via de knop. De Borderlands-lock uit 15.5 blijft; geen her-lock, geen materiaal opnieuw, toon-master blijft. Deblokkeert de vormgeving van base en map (REFERENTIE_BASE_MAP 3). |
| 2026-07-31 20:10 | O-5 | volledig | Via de chat, niet via de knop. Het wapen gaat echt uit de karaktermesh: isoleren, los mesh per wapenfamilie, socket op hand_r, wisselogica eraan. Mijn advies was de goedkope tint-stap; de owner koos anders. |
| 2026-07-31 19:49 | T-7 | ja |  |
| 2026-07-31 20:12 | T-1 | nog-niet | Ik speel nog NIET. Eerst laat ik alle basisdingen bouwen waar je nu mee bezig bent. Fase 0 en 1 zijn hiermee afgesloten - wacht niet meer op mijn playtest, die staat niet langer in de weg. |
| 2026-07-31 20:12 | OBS-1 | wapen-omgekeerd | WAARNEMING: het wapen wordt OMGEKEERD vastgehouden. Klassieke socket-rotatiefout - de forward-as van de wapenmesh komt niet overeen met de orientatie van de socket op hand_r. Hoort bij O-5 volledig. Controleer de mesh-pivot en de socket-rotatie, niet de code. |
| 2026-07-31 20:12 | OBS-2 | geen-handen-1e-persoon | WAARNEMING: in first-person zie ik mijn handen niet. Klopt met wat al gemeten is - zodra het lichaam niet gerenderd wordt verdwijnt alles mee, inclusief het wapen. Een third-person game met een first-person modus heeft een APARTE armen-mesh nodig (first-person arms), of het lichaam moet zichtbaar blijven met de camera in het hoofd. Dat is een keuze, geen bug - leg hem voor als owner-vraag. |
| 2026-07-31 20:12 | OBS-3 | meer-wapens-beschikbaar | WAARNEMING: er is nu een tweede wapen maar het is weinig gedetailleerd, waarschijnlijk zelfgemaakt. Ik heb al VEEL wapens gedownload die je kunt gebruiken, en je mag ook zelf nieuwe assets ophalen. Bouw dus geen wapens meer met de hand als er een goed asset ligt. Let bij het kiezen op de fictiewet uit doc 20: geen aardse semiotiek, en alles door de toon-master. |
| 2026-07-31 20:12 | OBS-4 | hud-nog-niet-borderlands | WAARNEMING: de gegevens rond het scherm zijn nog niet Borderlands-achtig. Referentie met echte screenshots en element-voor-element vertaling staat nu in phase0/REFERENTIE_HUD_BORDERLANDS.md - inclusief Game UI Database en het officiele BL3-ontwerpproces. Waarschijnlijk was je hier al mee bezig. |
