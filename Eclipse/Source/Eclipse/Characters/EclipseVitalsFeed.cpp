#include "Characters/EclipseVitalsFeed.h"

namespace EclipseVitalsFeed
{
	EEclipseStance ResolveStance(const FEclipseVitalsSnapshot& Snapshot)
	{
		// Volgorde = voorrang; zie de afweging in de header.
		if (Snapshot.bInCover)
		{
			return EEclipseStance::InCover;
		}
		if (Snapshot.bCrouched)
		{
			return EEclipseStance::Crouched;
		}
		if (Snapshot.bSprinting)
		{
			return EEclipseStance::Sprinting;
		}
		return EEclipseStance::Standing;
	}

	FEclipseVitalsDecision Decide(const FEclipseVitalsSnapshot& Previous, const FEclipseVitalsSnapshot& Current, bool bHasPrevious)
	{
		FEclipseVitalsDecision Decision;

		const EEclipseStance CurrentStance = ResolveStance(Current);

		// Het EERSTE monster: niets is veranderd, er was alleen nog niets bekend.
		// Vorige-waarden gelijk aan de huidige, zodat een consument die klakkeloos
		// een verschil uitrekent op nul uitkomt in plaats van op "van 0 naar 100"
		// — dat laatste zou bij spawn als genezing over het scherm rollen.
		if (!bHasPrevious)
		{
			Decision.bShouldBroadcast = true;
			Decision.Payload.Health = Current.Health;
			Decision.Payload.MaxHealth = Current.MaxHealth;
			Decision.Payload.PreviousHealth = Current.Health;
			Decision.Payload.Stance = CurrentStance;
			Decision.Payload.PreviousStance = CurrentStance;
			Decision.Payload.bDowned = Current.bDowned;
			Decision.Payload.bInitial = true;
			return Decision;
		}

		const EEclipseStance PreviousStance = ResolveStance(Previous);

		// MaxHealth telt mee onder dezelfde vlag: de balk verandert van lengte als
		// het maximum schuift, ook als het huidige getal blijft staan (ApplyTuning
		// doet precies dat). Eén vlag omdat de HUD er één ding mee doet — de
		// gezondheidsuitlezing opnieuw zetten.
		const bool bHealthChanged =
			FMath::Abs(Current.Health - Previous.Health) >= HealthEpsilon
			|| FMath::Abs(Current.MaxHealth - Previous.MaxHealth) >= HealthEpsilon;
		const bool bStanceChanged = CurrentStance != PreviousStance;
		const bool bDownedChanged = Current.bDowned != Previous.bDowned;

		if (!bHealthChanged && !bStanceChanged && !bDownedChanged)
		{
			// De hele reden dat deze laag bestaat: hier gaat er NIETS de bus op.
			return Decision;
		}

		Decision.bShouldBroadcast = true;
		Decision.Payload.Health = Current.Health;
		Decision.Payload.MaxHealth = Current.MaxHealth;
		Decision.Payload.PreviousHealth = Previous.Health;
		Decision.Payload.Stance = CurrentStance;
		Decision.Payload.PreviousStance = PreviousStance;
		Decision.Payload.bDowned = Current.bDowned;
		Decision.Payload.bHealthChanged = bHealthChanged;
		Decision.Payload.bStanceChanged = bStanceChanged;
		Decision.Payload.bDownedChanged = bDownedChanged;
		return Decision;
	}

	FEclipseVitalsDecision FEclipseVitalsTracker::Submit(const FEclipseVitalsSnapshot& Current)
	{
		FEclipseVitalsDecision Decision = Decide(LastBroadcast, Current, bHasBroadcast);
		if (Decision.bShouldBroadcast)
		{
			// Alleen bijwerken als het feit ook echt vertrekt — anders zou een
			// verandering onder de drempel het ijkpunt verschuiven en nooit
			// opstapelen tot iets zichtbaars.
			LastBroadcast = Current;
			bHasBroadcast = true;
			++BroadcastCount;
		}
		return Decision;
	}

	void FEclipseVitalsTracker::Reset()
	{
		LastBroadcast = FEclipseVitalsSnapshot();
		bHasBroadcast = false;
		BroadcastCount = 0;
	}
}
