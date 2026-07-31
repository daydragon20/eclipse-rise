#include "UI/EclipseHudReadoutLogic.h"

#include "Internationalization/Text.h"

namespace EclipseHudReadout
{

FVector2D TitleSafeMarginPx(const FVector2D& ViewportSizePx)
{
	// De marge is per as EVEN GROOT IN PIXELS en niet per as een eigen fractie.
	//
	// Dat is een bewuste afwijking van de televisienorm, en de reden staat in de
	// meting: 5 % van 1280 is 64 px en 5 % van 720 is 36 px, dus de linkerrand zou
	// bijna twee keer zo ruim worden als de bovenrand. Een HUD-hoek met een
	// zichtbaar andere horizontale dan verticale inspringing leest als een fout, ook
	// als beide "binnen de norm" zijn. De KORTSTE as bepaalt daarom de marge, precies
	// zoals Unreal's eigen DPI-curve de kortste as pakt.
	const double Shortest = FMath::Min(ViewportSizePx.X, ViewportSizePx.Y);
	const double Margin = FMath::Clamp(Shortest * TitleSafeFraction,
		static_cast<double>(MinSafeMarginPx), static_cast<double>(MaxSafeMarginPx));
	return FVector2D(Margin, Margin);
}

FString HumaniseRowName(FName RowName)
{
	if (RowName.IsNone())
	{
		return FString();
	}
	FString Text = RowName.ToString();
	// Alleen scheidingstekens. Zie de header voor waarom hier niets slimmers hoort.
	Text.ReplaceInline(TEXT("_"), TEXT(" "));
	Text.ReplaceInline(TEXT("-"), TEXT(" "));
	Text.TrimStartAndEndInline();
	return Text;
}

bool DisplayNameCameFromData(const FEclipseWeaponReadoutFacts& Facts)
{
	return !Facts.DisplayName.IsEmpty();
}

FEclipseAmmoReadout ComposeAmmoReadout(const FEclipseWeaponReadoutFacts& Facts)
{
	FEclipseAmmoReadout Out;

	// GEEN MAGAZIJN IS GEEN TELLER. Een wapen met een oneindig magazijn heeft niets
	// te tellen, en "0 / 0" is dan geen lege teller maar een leugen.
	if (Facts.MagazineSize <= 0)
	{
		Out.bHidden = true;
		return Out;
	}

	const int32 Ammo = FMath::Clamp(Facts.AmmoInMagazine, 0, Facts.MagazineSize);
	Out.MagazineText = FString::Printf(TEXT("%d"), Ammo);
	Out.CapacityText = FString::Printf(TEXT("/ %d"), Facts.MagazineSize);
	Out.AmmoText = FString::Printf(TEXT("%d / %d"), Ammo, Facts.MagazineSize);

	// Onder een derde kleurt hij, en dat is het punt waarop je moet BESLUITEN of je
	// herlaadt of doorschiet. Een getal alleen haal je in een vuurgevecht niet van
	// het scherm; kleur wel.
	Out.bLow = Ammo * 3 <= Facts.MagazineSize;
	Out.bEmpty = Ammo <= 0;

	// DE NAAM ALLEEN ALS ER IETS TE WISSELEN VALT. Met één wapen in je handen is de
	// naam ruis: je weet wat je vasthoudt, want er is niets anders.
	if (Facts.SlotCount > 1)
	{
		Out.WeaponText = DisplayNameCameFromData(Facts)
			? Facts.DisplayName
			: FText::FromString(HumaniseRowName(Facts.RowName));
	}

	// HET HERLADEN STAAT ERNAAST EN NIET ERVOOR. Dit is defect 1 in één regel: de
	// munitietekst hierboven is al gezet en wordt hieronder niet meer aangeraakt.
	if (Facts.bReloading)
	{
		Out.ReloadText = NSLOCTEXT("EclipseHud", "Reloading", "RELOADING");
		Out.ReloadProgress = FMath::Clamp(Facts.ReloadProgress, 0.0f, 1.0f);
	}

	return Out;
}

float SpreadDegreesToGapPx(float SpreadDegrees, float ViewportHeightPx, float VerticalFovDegrees)
{
	// Beschermen tegen onzin-invoer VÓÓR de tangens: een FOV van 0 of 180 laat de
	// noemer naar 0 of oneindig lopen, en dan komt er een gat van miljoenen pixels
	// of van nul uit — allebei stil, allebei fout.
	const float Fov = FMath::Clamp(VerticalFovDegrees, 10.0f, 170.0f);
	const float Height = FMath::Max(ViewportHeightPx, 1.0f);
	const float Spread = FMath::Clamp(SpreadDegrees, 0.0f, 45.0f);

	const float PixelsPerTan = (Height * 0.5f) / FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f));
	return FMath::Tan(FMath::DegreesToRadians(Spread)) * PixelsPerTan;
}

FEclipseCrosshairLayout ComposeCrosshair(
	float SpreadDegrees, int32 PelletsPerShot, float AimSpreadDegrees,
	float ViewportHeightPx, float VerticalFovDegrees)
{
	FEclipseCrosshairLayout Out;

	// DE VORM ZEGT WAT JE VASTHOUDT (REFERENTIE_HUD_BORDERLANDS.md §2: "het is
	// gratis leesbaarheid"). Twee scheidslijnen, allebei uit het wapenprofiel en
	// geen van beide uit een handmatige tabel per wapen — een tabel is incompleet
	// zodra er een wapen bij komt.
	if (PelletsPerShot > 1)
	{
		Out.Shape = EEclipseCrosshairShape::Brackets;
	}
	else if (AimSpreadDegrees > 0.0f && AimSpreadDegrees <= 0.25f)
	{
		// Een wapen dat gemikt binnen een kwart graad blijft, is een precisiewapen.
		// De DMR zit op 0,15; de AR op 0,6.
		Out.Shape = EEclipseCrosshairShape::Precision;
	}

	const float GapFromSpread = SpreadDegreesToGapPx(SpreadDegrees, ViewportHeightPx, VerticalFovDegrees);

	// EEN ONDERGRENS OP HET GAT, want een kruis dat bij lage spreiding dichtklapt tot
	// één blok is geen kruis meer — dan staat je doelwit onder je eigen vizier. 4 px
	// is precies de maat waarop het gat op 720p nog als gat leest.
	Out.GapPx = FMath::Clamp(GapFromSpread, 4.0f, ViewportHeightPx * 0.25f);

	switch (Out.Shape)
	{
	case EEclipseCrosshairShape::Brackets:
		// Kort en dik: hagel vraagt geen punt maar een gebied.
		Out.ArmLengthPx = 9.0f;
		Out.ThicknessPx = 4.0f;
		break;
	case EEclipseCrosshairShape::Precision:
		// Lang en dun, met een groter gat: op afstand telt de PLEK, niet de balk.
		Out.ArmLengthPx = 14.0f;
		Out.ThicknessPx = 2.0f;
		Out.GapPx = FMath::Max(Out.GapPx, 7.0f);
		break;
	default:
		Out.ArmLengthPx = 10.0f;
		Out.ThicknessPx = 3.0f;
		break;
	}

	// SCHALEN MET DE SCHERMHOOGTE. Op 720p is een balk van 10 px goed zichtbaar; op
	// 1440p is diezelfde balk half zo groot in beeld en verdwijnt hij weer — precies
	// de klasse fout die het oude kruis van 7x9 px was, alleen dan door resolutie in
	// plaats van door glyphmaat.
	const float Scale = FMath::Clamp(ViewportHeightPx / 720.0f, 1.0f, 3.0f);
	Out.ArmLengthPx *= Scale;
	Out.ThicknessPx *= Scale;
	Out.GapPx *= Scale;

	return Out;
}

} // namespace EclipseHudReadout
