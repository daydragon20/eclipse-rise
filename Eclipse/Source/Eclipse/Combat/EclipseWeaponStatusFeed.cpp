#include "Combat/EclipseWeaponStatusFeed.h"

namespace EclipseWeaponStatusFeed
{
	bool IsEmpty(const FEclipseWeaponSnapshot& Snapshot)
	{
		// Zie de header: geen magazijn is iets anders dan een leeg magazijn.
		return Snapshot.MagazineSize > 0 && Snapshot.AmmoInMagazine <= 0;
	}

	namespace
	{
		/** De velden die in ELK uitgaand feit horen te staan, verandering of niet. */
		void FillState(FEclipseWeaponStatusPayload& Payload, const FEclipseWeaponSnapshot& Current)
		{
			Payload.AmmoInMagazine = Current.AmmoInMagazine;
			Payload.MagazineSize = Current.MagazineSize;
			Payload.SpareMagazines = Current.SpareMagazines;
			Payload.bReloading = Current.bReloading;
			// Buiten een beurt is voortgang betekenisloos, en "betekenisloos" hoort
			// als 0 op de bus en niet als de laatst gemeten 0,97 — anders staat er na
			// een geslaagde herlaadbeurt een balk die bijna vol is.
			Payload.ReloadProgress = Current.bReloading ? Current.ReloadProgress : 0.0f;
			Payload.ReloadSecondsRemaining = Current.bReloading ? Current.ReloadSecondsRemaining : 0.0f;
			Payload.ReloadSecondsTotal = Current.ReloadSecondsTotal;
			Payload.bEmpty = IsEmpty(Current);
			Payload.WeaponRowName = Current.WeaponRowName;
			Payload.WeaponDisplayName = Current.WeaponDisplayName;
			Payload.ActiveSlot = Current.ActiveSlot;
			Payload.SlotCount = Current.SlotCount;
			Payload.FireMode = Current.FireMode;
		}
	}

	FEclipseWeaponStatusDecision Decide(const FEclipseWeaponSnapshot& Previous, const FEclipseWeaponSnapshot& Current, bool bHasPrevious)
	{
		FEclipseWeaponStatusDecision Decision;

		// Het EERSTE monster: niets is veranderd, er was alleen nog niets bekend.
		// PreviousAmmo gelijk aan het huidige, zodat een consument die klakkeloos een
		// verschil uitrekent op nul uitkomt in plaats van op "van 0 naar 30" — dat
		// laatste zou bij het oppakken als een herlaadbeurt over het scherm rollen.
		if (!bHasPrevious)
		{
			Decision.bShouldBroadcast = true;
			FillState(Decision.Payload, Current);
			Decision.Payload.PreviousAmmoInMagazine = Current.AmmoInMagazine;
			Decision.Payload.bInitial = true;
			return Decision;
		}

		// EEN ANDER WAPEN IS EEN ANDER WAPEN, en de sleutel beslist dat — niet de
		// leestekst (zie de header). Het slot telt mee omdat twee slots dezelfde rij
		// kunnen dragen: twee keer dezelfde sidearm is een geldige loadout, en dan is
		// de wissel alleen aan het slotnummer te zien. Zonder dat zou zo'n wissel
		// stil zijn en zou de HUD het verkeerde magazijn blijven tonen.
		const bool bWeaponChanged =
			Current.WeaponRowName != Previous.WeaponRowName
			|| Current.ActiveSlot != Previous.ActiveSlot
			|| Current.SlotCount != Previous.SlotCount;

		// MagazineSize telt onder dezelfde vlag als het magazijn zelf: allebei
		// veranderen wat "23 / 30" zegt, en de schermlaag doet er één ding mee.
		const bool bAmmoChanged =
			Current.AmmoInMagazine != Previous.AmmoInMagazine
			|| Current.MagazineSize != Previous.MagazineSize
			|| Current.SpareMagazines != Previous.SpareMagazines;

		const bool bReloadStateChanged = Current.bReloading != Previous.bReloading;

		// VOORTGANG TELT ALLEEN BINNEN ÉÉN DOORLOPENDE BEURT. Bij een omslag draagt
		// bReloadStateChanged het nieuws; zou voortgang daar meetellen, dan zou het
		// einde van een beurt (0,97 -> 0) als "voortgang" gemarkeerd worden en zou de
		// balk terugspringen in plaats van te verdwijnen. Monotoon betekent monotoon.
		const bool bReloadProgressed =
			Current.bReloading && Previous.bReloading && !bReloadStateChanged
			&& FMath::Abs(Current.ReloadProgress - Previous.ReloadProgress) >= ReloadProgressEpsilon;

		const bool bEmptyChanged = IsEmpty(Current) != IsEmpty(Previous);
		const bool bFireModeChanged = Current.FireMode != Previous.FireMode;

		if (!bWeaponChanged && !bAmmoChanged && !bReloadStateChanged && !bReloadProgressed
			&& !bEmptyChanged && !bFireModeChanged)
		{
			// De hele reden dat deze laag bestaat: hier gaat er NIETS de bus op.
			return Decision;
		}

		Decision.bShouldBroadcast = true;
		FillState(Decision.Payload, Current);
		Decision.Payload.PreviousAmmoInMagazine = Previous.AmmoInMagazine;
		Decision.Payload.bAmmoChanged = bAmmoChanged;
		Decision.Payload.bWeaponChanged = bWeaponChanged;
		Decision.Payload.bReloadStateChanged = bReloadStateChanged;
		Decision.Payload.bReloadProgressed = bReloadProgressed;
		Decision.Payload.bEmptyChanged = bEmptyChanged;
		Decision.Payload.bFireModeChanged = bFireModeChanged;
		return Decision;
	}

	FEclipseWeaponStatusDecision FEclipseWeaponStatusTracker::Submit(const FEclipseWeaponSnapshot& Current)
	{
		FEclipseWeaponStatusDecision Decision = Decide(LastBroadcast, Current, bHasBroadcast);
		if (Decision.bShouldBroadcast)
		{
			// Alleen bijwerken als het feit ook echt vertrekt — anders zou een
			// voortgangsstap onder de drempel het ijkpunt verschuiven en zou de balk
			// nooit een stap halen.
			LastBroadcast = Current;
			bHasBroadcast = true;
			++BroadcastCount;
		}
		return Decision;
	}

	void FEclipseWeaponStatusTracker::Reset()
	{
		LastBroadcast = FEclipseWeaponSnapshot();
		bHasBroadcast = false;
		BroadcastCount = 0;
	}
}
