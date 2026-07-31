#include "UI/EclipseMissionHudWidget.h"

#include "Components/CanvasPanel.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/EclipseGameplayTags.h"
#include "Core/EclipseEventPayloads.h"
#include "Characters/EclipseCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/EclipseCommandModeComponent.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Characters/EclipsePlayerController.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Quests/EclipseMissionLogic.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "UI/EclipseHudReadoutLogic.h"
#include "Squad/EclipseSquadSubsystem.h"

namespace
{
	/**
	 * Master switch of the feel-gauntlet panel. Default OFF: a normal run shows
	 * the Phase 1 HUD it always showed and nothing else (the overlay is a
	 * measuring instrument, not a feature). F2/H stay live either way — a key
	 * press is the tester explicitly asking for a panel.
	 */
	TAutoConsoleVariable<int32> CVarEclipseGauntletOverlay(
		TEXT("Eclipse.Gauntlet.Overlay"),
		0,
		TEXT("1 = show the P2-02 R3 criteria panel on the debug HUD and arm its manual keys (F4-F8). Default off."),
		ECVF_Default);

	/**
	 * In-game test guide (phase0/INGAME_TESTGIDS.md; owner picked variant A —
	 * detect and tick off, never lock a control). Default OFF like every other
	 * debug tier: 1 opens the guide the moment the mission HUD mounts. F3 toggles
	 * it either way, following the F2/H convention that a keypress is the tester
	 * explicitly asking for a panel. Deliberately absent: Eclipse.Guide.Strict —
	 * variant B stays unbuilt until A proves its detection per control.
	 */
	TAutoConsoleVariable<int32> CVarEclipseGuideOverlay(
		TEXT("Eclipse.Guide.Overlay"),
		0,
		TEXT("1 = open the in-game test guide as soon as the mission HUD mounts (F3 toggles it at any time). Default off."),
		ECVF_Default);

	/** In-place refresh cadence, wall clock: a burst of order facts may not turn into a burst of string work. */
	constexpr double GauntletRefreshIntervalSeconds = 0.1;

	const FLinearColor ColourNeutral(0.85f, 0.85f, 0.85f);
	const FLinearColor ColourDim(0.42f, 0.42f, 0.42f);
	const FLinearColor ColourPass(0.45f, 0.90f, 0.50f);
	const FLinearColor ColourOpen(0.70f, 0.55f, 0.20f);
	const FLinearColor ColourFail(0.95f, 0.35f, 0.25f);

	/** Colour of a verdict row by status — the panel says pass/open/fail without the tester reading the words. */
	const FLinearColor& StatusColour(EclipseGauntletOverlay::EEclipseGauntletStatus Status)
	{
		switch (Status)
		{
		case EclipseGauntletOverlay::EEclipseGauntletStatus::Pass: return ColourPass;
		case EclipseGauntletOverlay::EEclipseGauntletStatus::Fail: return ColourFail;
		default:                                                   return ColourOpen;
		}
	}

	/**
	 * Debug text row. Name is deliberately not "AddTextRow": the base hub's own
	 * anonymous-namespace helper carries that name, and in a unity build both
	 * files share one translation unit — a default argument here would make every
	 * three-argument call over there ambiguous.
	 */
	UTextBlock* AddHudTextRow(UWidgetTree& Tree, UVerticalBox& Box, const FString& Text, const FLinearColor& Colour = ColourNeutral)
	{
		UTextBlock* Row = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(Text));
		Row->SetColorAndOpacity(FSlateColor(Colour));
		Box.AddChildToVerticalBox(Row);
		return Row;
	}
}

void UEclipseMissionHudWidget::LogUiReport() const
{

	UE_LOG(LogEclipse, Display, TEXT("UI: missie-HUD inViewport=%d"), IsInViewport() ? 1 : 0);
	UE_LOG(LogEclipse, Display, TEXT("UI:   munitieteller zichtbaarheid=%d tekst='%s'"),
		AmmoReadout != nullptr ? static_cast<int32>(AmmoReadout->GetVisibility()) : -1,
		AmmoReadout != nullptr ? *AmmoReadout->GetText().ToString() : TEXT("(bestaat niet)"));
	UE_LOG(LogEclipse, Display, TEXT("UI:   trefteken zichtbaarheid=%d"),
		HitMarker != nullptr ? static_cast<int32>(HitMarker->GetVisibility()) : -1);
	// HET KRUIS IN MATEN EN NIET IN TEKST. Het is sinds 31-07 geen glyph meer maar
	// vier balken, en juist de MAAT was het defect — een regel die "tekst='+'" meldt
	// zou het probleem niet eens kunnen noemen.
	UE_LOG(LogEclipse, Display, TEXT("UI:   richtkruis  zichtbaarheid=%d balken=%d arm=%.1f px gat=%.1f px dikte=%.1f px"),
		CrosshairRoot != nullptr ? static_cast<int32>(CrosshairRoot->GetVisibility()) : -1,
		CrosshairArms.Num(), LastCrosshairArmPx, LastCrosshairGapPx, LastCrosshairThicknessPx);
	// Per paneel: staat de vlag aan EN hoeveel regels hangen eronder.
	// Een open paneel zonder regels ziet er voor de speler net zo leeg
	// uit als een dicht paneel, en dat zijn twee verschillende bugs.
	UE_LOG(LogEclipse, Display, TEXT("UI:   F2 controls  open=%d"), bControlsVisible ? 1 : 0);
	UE_LOG(LogEclipse, Display, TEXT("UI:   F3 testgids   open=%d  regels=%d"),
		bGuideVisible ? 1 : 0, GuideRows.Num());
	UE_LOG(LogEclipse, Display, TEXT("UI:   H playtest    open=%d  regels=%d"),
		bPlaytestVisible ? 1 : 0, PlaytestRows.Num());
	UE_LOG(LogEclipse, Display, TEXT("UI:   R3-verdict    regels=%d"), GauntletRows.Num());

	// EN OORDELEN WAAR HET KAN. Een dump die je moet lezen is beter dan
	// niets, maar hij vangt alleen wat iemand toevallig naleest. Deze
	// drie gevallen zijn voor de speler kapot en horen dus op te vallen
	// zonder dat er iemand kijkt.
	const APawn* ReportBody = GetOwningPlayerPawn();
	const UEclipseHitscanWeaponComponent* ReportWeapon = ReportBody != nullptr
		? ReportBody->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;

	if (!IsInViewport())
	{
		UE_LOG(LogEclipse, Warning, TEXT("UI: FOUT — de missie-HUD hangt niet in de viewport; de speler ziet niets van dit alles."));
	}
	// Een wapen met een magazijn en een onzichtbare teller: dan schiet
	// je zonder te weten hoeveel je nog hebt.
	if (ReportWeapon != nullptr && ReportWeapon->GetMagazineSize() > 0
		&& AmmoReadout != nullptr && AmmoReadout->GetVisibility() == ESlateVisibility::Hidden)
	{
		UE_LOG(LogEclipse, Warning, TEXT("UI: FOUT — er is een wapen met een magazijn van %d, maar de munitieteller staat verborgen."),
			ReportWeapon->GetMagazineSize());
	}
	// Een wapen zonder richtkruis: dan schiet je zonder te weten WAAR je
	// richt. Dit is de owner-melding van 27-07 als vaste controle, zodat
	// hij niet nog eens twee dagen stil kan wegvallen.
	if (ReportWeapon != nullptr
		&& (CrosshairRoot == nullptr || CrosshairRoot->GetVisibility() == ESlateVisibility::Collapsed
			|| CrosshairArms.Num() != CrosshairArmCount))
	{
		UE_LOG(LogEclipse, Warning, TEXT("UI: FOUT — er is een wapen, maar er staat geen richtkruis; de speler kan niet zien waar hij richt."));
	}
	// EN EEN KRUIS DAT TE KLEIN IS OM TE VINDEN IS OOK KAPOT, en dat is de meting
	// die 31-07 pas op pixelniveau opviel: 7x9 px, 17 pixels inkt. Een controle die
	// alleen "staat hij er" vraagt, had daar nooit iets van gezegd.
	if (ReportWeapon != nullptr && LastCrosshairArmPx > 0.0f && LastCrosshairArmPx < 8.0f)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("UI: FOUT — het richtkruis heeft armen van %.1f px; dat is de maat waarop hij op 31-07 onvindbaar bleek."),
			LastCrosshairArmPx);
	}
	// Open zonder regels is voor de speler niet te onderscheiden van
	// dicht — en dat is precies waarom het apart gemeld hoort te worden.
	if (bGuideVisible && GuideRows.Num() == 0)
	{
		UE_LOG(LogEclipse, Warning, TEXT("UI: FOUT — de testgids staat open maar heeft nul regels; dat ziet er leeg uit terwijl hij aan staat."));
	}
	if (bPlaytestVisible && PlaytestRows.Num() == 0)
	{
		UE_LOG(LogEclipse, Warning, TEXT("UI: FOUT — het playtest-paneel staat open maar heeft nul regels."));
	}
}

void UEclipseMissionHudWidget::RefreshAmmoReadout()
{
	if (AmmoReadout == nullptr)
	{
		return;
	}

	const APawn* Body = GetOwningPlayerPawn();
	const UEclipseHitscanWeaponComponent* Weapon = Body != nullptr
		? Body->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;

	// ALLE BESLISSINGEN KOMEN UIT DE PURE KERN (EclipseHudReadoutLogic). Deze functie
	// mag alleen nog aflezen en tekenen — dat is de laagscheiding waar de drie
	// defecten van 31-07 doorheen glipten: "toon HERLADEN in plaats van de kogels",
	// "plak de rijnaam op het scherm" en "zet hem 48 px van de rand" waren alledrie
	// beslissingen in een opmaakfunctie, en dus per constructie niet te toetsen
	// zonder de game te starten.
	EclipseHudReadout::FEclipseWeaponReadoutFacts Facts;
	if (Weapon != nullptr)
	{
		Facts.DisplayName = Weapon->GetActiveWeaponDisplayName();
		Facts.RowName = Weapon->GetActiveWeaponName();
		Facts.AmmoInMagazine = Weapon->GetAmmoInMagazine();
		Facts.MagazineSize = Weapon->GetMagazineSize();
		Facts.bReloading = Weapon->IsReloading();
		Facts.ReloadProgress = Weapon->GetReloadProgress();
		Facts.SlotCount = Weapon->GetSlotCount();
	}

	const EclipseHudReadout::FEclipseAmmoReadout Readout = EclipseHudReadout::ComposeAmmoReadout(Facts);

	// LUID DEGRADEREN, ÉÉN KEER (14.3.5). Een rij zonder DisplayName levert nu een
	// opgepoetste rijnaam op het scherm; dat is een noodverband en hoort gemeld te
	// worden, anders blijft het stil bestaan tot iemand het toevallig op een frame
	// ziet — precies hoe `Sidearm_Scrap` daar twee dagen kon staan.
	if (Weapon != nullptr && Facts.SlotCount > 1 && !EclipseHudReadout::DisplayNameCameFromData(Facts)
		&& !Facts.RowName.IsNone() && !ReportedMissingDisplayNames.Contains(Facts.RowName))
	{
		ReportedMissingDisplayNames.Add(Facts.RowName);
		UE_LOG(LogEclipse, Warning,
			TEXT("HUD: wapenrij '%s' heeft geen DisplayName — de speler ziet de opgepoetste rijnaam '%s'. Vul DT_Weapons.DisplayName."),
			*Facts.RowName.ToString(), *EclipseHudReadout::HumaniseRowName(Facts.RowName));
	}

	if (Readout.bHidden)
	{
		// Geen wapen, of een wapen met een oneindig magazijn: dan is er niets te
		// tellen en hoort er niets te staan. Een teller die "0 / 0" toont liegt.
		AmmoReadout->SetVisibility(ESlateVisibility::Hidden);
		SetVisibilityIfChanged(AmmoCapacity, false);
		SetVisibilityIfChanged(WeaponReadout, false);
		SetVisibilityIfChanged(ReloadReadout, false);
		SetVisibilityIfChanged(ReloadBarFill, false);
		SetVisibilityIfChanged(ReloadBarTrack, false);
		// HET PANEEL GAAT MEE. Een leeg kader zonder cijfers erin is erger dan geen
		// kader: het claimt schermruimte voor informatie die er niet is.
		SetVisibilityIfChanged(AmmoPanelInk, false);
		SetVisibilityIfChanged(AmmoPanelFill, false);
		return;
	}

	// DRIE VELDEN DIE NAAST ELKAAR BESTAAN — de reparatie van defect 1. Het
	// magazijngetal wordt hieronder niet meer aangeraakt door de herlaadtak.
	AmmoReadout->SetVisibility(ESlateVisibility::HitTestInvisible);
	AmmoReadout->SetText(FText::FromString(Readout.MagazineText));
	SetVisibilityIfChanged(AmmoPanelInk, true);
	SetVisibilityIfChanged(AmmoPanelFill, true);
	SetVisibilityIfChanged(AmmoCapacity, true);
	if (AmmoCapacity != nullptr)
	{
		AmmoCapacity->SetText(FText::FromString(Readout.CapacityText));
	}

	// Leeg is scherper dan laag: bij leeg doet de trekker niets, en dat hoort niet
	// als "bijna leeg" te lezen.
	const FLinearColor AmmoColour = Readout.bEmpty
		? FLinearColor(1.0f, 0.25f, 0.18f)
		: (Readout.bLow ? FLinearColor(1.0f, 0.55f, 0.15f) : FLinearColor(0.95f, 0.95f, 0.95f));
	AmmoReadout->SetColorAndOpacity(FSlateColor(AmmoColour));

	SetVisibilityIfChanged(WeaponReadout, !Readout.WeaponText.IsEmpty());
	if (WeaponReadout != nullptr && !Readout.WeaponText.IsEmpty())
	{
		WeaponReadout->SetText(Readout.WeaponText);
	}

	const bool bReloading = !Readout.ReloadText.IsEmpty();
	SetVisibilityIfChanged(ReloadReadout, bReloading);
	SetVisibilityIfChanged(ReloadBarTrack, bReloading);
	SetVisibilityIfChanged(ReloadBarFill, bReloading);
	if (bReloading)
	{
		if (ReloadReadout != nullptr)
		{
			ReloadReadout->SetText(Readout.ReloadText);
		}
		// DE VOORTGANG ALS BREEDTE. Het woord alleen zegt DAT je herlaadt; de balk
		// zegt WANNEER je weer kunt schieten, en dat is de informatie waar je in een
		// gevecht een besluit op neemt.
		if (UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(ReloadBarFill->Slot))
		{
			const FVector2D Size = FillSlot->GetSize();
			FillSlot->SetSize(FVector2D(ReloadBarWidthPx * Readout.ReloadProgress, Size.Y));
		}
	}
}

void UEclipseMissionHudWidget::SetVisibilityIfChanged(UWidget* Widget, bool bVisible)
{
	// SetVisibility ongeldigt de layout, en deze functie draait per frame per
	// element. Alleen bij een ECHTE verandering aanroepen scheelt vier
	// layout-invalidaties per frame — 12.4 geldt ook voor de HUD.
	if (Widget == nullptr)
	{
		return;
	}
	const ESlateVisibility Desired = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (Widget->GetVisibility() != Desired)
	{
		Widget->SetVisibility(Desired);
	}
}

void UEclipseMissionHudWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	// EEN KEER HARDOP WAT DE HUD DENKT DAT HIJ TOONT, en met opzet NA de refresh
	// van de vorige frame in plaats van ervoor.
	//
	// Eerste versie stond bovenaan RefreshAmmoReadout en meldde 'tekst=""' —
	// logisch, want daar had de functie zijn werk nog niet gedaan. Dat is meten
	// vóór de gebeurtenis, de spiegelbeeldfout van meten erna.
	//
	// Waarom dit er staat: op geen enkele opname van de speelronde is de
	// munitieteller te zien, en drie opnamemethodes pakken de UMG-laag geen van
	// alle mee. "Ik zie hem niet" is dus geen bewijs dat hij er niet is — deze
	// regel beantwoordt het zonder beeld.
	if (!bLoggedAmmoState && AmmoReadout != nullptr && GetWorld() != nullptr && GetWorld()->TimeSeconds > 3.0f)
	{
		bLoggedAmmoState = true;

		// DE MAAT DIE HET ECHT WORDT, en niet de maat die ik bedoelde.
		//
		// Deze regel bestaat omdat ik er twee opnamerondes op ben misgegaan: ik zette
		// de marge op 36 en mat 25 px op het frame, corrigeerde voor de DPI-schaal en
		// mat 55. Twee metingen die niet met één schaalfactor te rijmen zijn, dus
		// mijn model van de keten klopte niet — en dan is doorrekenen zinloos.
		//
		// Wat hier staat is de hele keten in één regel: de viewport, de DPI-schaal,
		// wat ik in het slot zet, en wat Slate er UITEINDELIJK van maakt
		// (GetCachedGeometry is de geometrie ná de layout, dus na alle schaling).
		// Daarmee is het verschil tussen "administratie" en "scherm" niet meer een
		// vermoeden maar een getal — precies waar deze hele laag op vastliep.
		int32 ViewX = 0;
		int32 ViewY = 0;
		if (const APlayerController* Owner = GetOwningPlayer())
		{
			Owner->GetViewportSize(ViewX, ViewY);
		}
		const float DpiScale = UWidgetLayoutLibrary::GetViewportScale(this);
		const FVector2D SlotMargin = CurrentTitleSafeMarginPx();
		const FGeometry& RootGeometry = Root != nullptr ? Root->GetCachedGeometry() : GetCachedGeometry();
		const FVector2D RootAbsolute = RootGeometry.GetAbsolutePosition();
		const FVector2D BlockAbsolute = AmmoBlock != nullptr
			? AmmoBlock->GetCachedGeometry().GetAbsolutePosition() : FVector2D::ZeroVector;
		const FVector2D BlockSize = AmmoBlock != nullptr
			? AmmoBlock->GetCachedGeometry().GetAbsoluteSize() : FVector2D::ZeroVector;
		UE_LOG(LogEclipse, Display,
			TEXT("HUD MARGE: viewport %dx%d, DPI-schaal %.3f, slotwaarde %.1f -> ECHT linksboven (%.1f, %.1f) px; munitieblok op (%.1f, %.1f) maat %.1fx%.1f -> rechtermarge %.1f px, ondermarge %.1f px"),
			ViewX, ViewY, DpiScale, SlotMargin.X,
			RootAbsolute.X, RootAbsolute.Y,
			BlockAbsolute.X, BlockAbsolute.Y, BlockSize.X, BlockSize.Y,
			ViewX - (BlockAbsolute.X + BlockSize.X), ViewY - (BlockAbsolute.Y + BlockSize.Y));
		UE_LOG(LogEclipse, Display,
			TEXT("HUD: munitieteller zichtbaarheid=%d tekst='%s' inViewport=%d"),
			static_cast<int32>(AmmoReadout->GetVisibility()),
			*AmmoReadout->GetText().ToString(),
			IsInViewport() ? 1 : 0);
	}

	Super::NativeTick(Geometry, DeltaSeconds);

	// Alleen werk als er iets te doven valt. Deze widget tikt toch al voor Slate;
	// een timer per treffer zou bij 6,67 schoten per seconde meer kosten dan dit.
	// De marge eerst: hij hangt aan de viewportmaat, en die kan tussen twee frames
	// veranderen (venster slepen, resolutiewissel).
	ApplySafeAreaLayout();
	RefreshAmmoReadout();
	// Het kruis reageert op de spreiding, dus hij hoort per frame na te kijken —
	// maar hij doet alleen werk als er iets veranderde (zie RefreshCrosshair).
	RefreshCrosshair();

	if (HitMarkerSecondsLeft > 0.0f)
	{
		HitMarkerSecondsLeft -= DeltaSeconds;
		if (HitMarkerSecondsLeft <= 0.0f && HitMarker != nullptr)
		{
			HitMarker->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (DamageIndicatorSecondsLeft > 0.0f)
	{
		DamageIndicatorSecondsLeft -= DeltaSeconds;
		if (DamageIndicatorSecondsLeft <= 0.0f && DamageIndicator != nullptr)
		{
			DamageIndicator->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UEclipseMissionHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// DE HELE BOOM STAAT HIER EN NIET IN NativeConstruct, en dat is de reparatie
	// van de oorzaak waar deze laag twee dagen op is vastgelopen.
	//
	// GEMETEN op 31-07 in de engine-bron (UserWidget.cpp): UUserWidget::RebuildWidget
	// pakt `WidgetTree->RootWidget` en maakt daar de Slate-boom van; is die leeg,
	// dan wordt het een SSpacer — een lege doos. PAS DAARNA roept OnWidgetRebuilt
	// NativePreConstruct en NativeConstruct aan. Deze widget zette zijn canvas als
	// RootWidget IN NativeConstruct, dus altijd één stap te laat: de Slate-boom was
	// op dat moment al genomen en bleef die lege doos. Alles wat er daarna in kwam —
	// richtkruis, munitieteller, trefteken, de F3-gids, de objectiveregels — werd
	// nooit getekend.
	//
	// Dat verklaart in één klap het rijtje meldingen dat als losse bugs is behandeld:
	// "ik kan niet zien waar ik richt", "F3 doet niets", "de HUD is niet te
	// fotograferen". En het verklaart waarom de logregels het tegendeel zeiden:
	// IsInViewport() en GetVisibility() lezen de UMG-administratie, niet het scherm.
	// Een instrument dat de vraag niet kan beantwoorden, antwoordde toch — precies
	// de vorm van fout waar de owner-regel "meten voor je concludeert" over gaat.
	//
	// NativeOnInitialized draait vanuit Initialize(), en dat gebeurt in CreateWidget
	// — dus vóór RebuildWidget. Bovendien precies één keer per instantie, wat past
	// bij een boom die niet per montage opnieuw hoeft.

	// CANVAS als wortel, met de bestaande tekstlijst linksboven erin (26-07).
	//
	// Voor trefferfeedback moet er iets in het MIDDEN van het scherm kunnen staan,
	// en dat kan een verticale doos niet. Een canvas kan het wel en verandert niets
	// aan de tekstregels: die krijgen gewoon hun eigen slot linksboven.
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MissionHudCanvas"));
	WidgetTree->RootWidget = Canvas;

	Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionHudRoot"));
	if (UCanvasPanelSlot* RootSlot = Canvas->AddChildToCanvas(Root))
	{
		RootSlot->SetAutoSize(true);
		RootSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		// De marge zelf wordt PER FRAME gezet (ApplySafeAreaLayout); hier staat alleen
		// een plek zodat het slot bestaat. Zie ApplySafeAreaLayout voor waarom dat
		// niet bij de bouw kan.
		RootSlot->SetPosition(FVector2D::ZeroVector);
	}

	// ---------------------------------------------------------------------------
	// DE SPELERLAAG — altijd, ook onder -EclipseShot.
	// ---------------------------------------------------------------------------
	//
	// Het richtkruis stond al vóór de poort, en dat was de correctie op een eerdere
	// fout: het kruis werd ONDER de uitstap gebouwd, bestond dus in geen enkele
	// opnameronde, en werd desondanks "klaar en getest" genoemd op grond van een
	// groene suite. Het beeldbewijs kón er per constructie niet zijn.
	//
	// Diezelfde redenering gold altijd al voor het trefteken, de munitieteller en
	// de richtingsindicator, en die stonden er wél achter. De poort is er om
	// DEBUGTEKST uit review-stills te houden — rijen, panelen, de gauntlet-teller.
	// Wat de speler tijdens het spelen afleest is spelbesturing en hoort net zo goed
	// op een reviewframe als het personage zelf.
	//
	// EN DE ABONNEMENTEN GAAN MEE (SubscribePlayerEvents, in NativeConstruct), want
	// dat is waar de vorige poging op strandde: de uitstap sloeg niet alleen de
	// constructie over maar ook Subscribe(). Een munitieteller zonder bron toont
	// eeuwig "30 / 30" en liegt dus harder dan een leeg scherm.
	BuildPlayerLayer();

	if (!IsDebugHudAllowed())
	{
		return;
	}

	// ---------------------------------------------------------------------------
	// DE DEBUGLAAG — alleen als de poort open staat.
	// ---------------------------------------------------------------------------

	LiveBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(LiveBox);

	BuildStaticPanels();
}

void UEclipseMissionHudWidget::NativeConstruct()
{
	using namespace EclipseGauntletOverlay;

	Super::NativeConstruct();

	// De BOOM staat in NativeOnInitialized (zie daar waarom). Hier staat alleen wat
	// per montage opnieuw moet: abonnementen, per-run-tellers en de eerste vulling.
	SubscribePlayerEvents();
	// Meteen één keer vullen. NativeTick doet het daarna elke frame, maar de eerste
	// frame ná het monteren is precies het frame dat een opnameronde vastlegt.
	RefreshAmmoReadout();

	if (!IsDebugHudAllowed())
	{
		// Review round: no rows, no panels, no console command — the DEBUG layer is
		// inert, so no debug text can appear in a review still. De spelerlaag
		// hierboven staat er wel, en dat is het hele punt van de splitsing.
		return;
	}

	// A mount is a fresh run: the widget is cached by the controller and
	// re-constructed per mission, exactly like the two automatic tallies reset per
	// run (UnregisterAll / Event.Mission.Started). The previous run's block was
	// already logged and archived in NativeDestruct, so nothing is lost — and
	// mixing two runs into one verdict would be worse than losing one.
	CleanPicks = 0;
	MisPicks = 0;
	ComfortAnswer = EEclipseGauntletAnswer::Unanswered;
	ConfidenceAnswer = EEclipseGauntletAnswer::Unanswered;
	PlaytestAnswers.Init(EEclipseGauntletAnswer::Unanswered, PlaytestQuestionCount);

	// Same reasoning for the guide: a new mission is a new walk through the list,
	// and the previous run's summary was archived on teardown.
	GuideProgress.Reset();
	bGuideSummaryEmitted = false;
	bGuideVisible = CVarEclipseGuideOverlay.GetValueOnGameThread() > 0;

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		// Three families, not the whole Event root: this HUD draws mission, squad
		// and command facts. Subscribing wider only bought it a full widget-tree
		// rebuild on every economy/base/story commit.
		for (const TCHAR* Family : { TEXT("Event.Mission"), TEXT("Event.Squad"), TEXT("Event.Command") })
		{
			EventHandles.Add(Bus->Subscribe(
				FGameplayTag::RequestGameplayTag(Family),
				FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionHudWidget::OnAnyFact)));
		}
	}

	// The gauntlet block on demand, so a session where the panel stayed closed can
	// still hand the owner his verdict input (phase0/FEEL_GAUNTLET_P2-02.md).
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Gauntlet.Summary")) == nullptr)
	{
		SummaryCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Gauntlet.Summary"),
			TEXT("Log + archive the R3-VERDICT INPUT block of the running feel gauntlet (SPEC-P2-02 R3)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]() { EmitVerdictSummary(); }),
			ECVF_Default);
	}

	// Active-device highlight is event-driven: Enhanced Input's device subsystem
	// broadcasts when the platform user switches hardware, so the overlay never
	// polls keys per frame.
	if (UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		DeviceChangedHandle = Devices->OnInputHardwareDeviceChangedNative.AddWeakLambda(this,
			[this](FPlatformUserId, FInputDeviceId) { RefreshDeviceHighlight(); });
	}

	// The guide CVar is also honoured LIVE, not only at mount. Reading it once
	// here loses the common case: -ExecCmds fires after the world is up, and with
	// -EclipseStartMission the mission (and this HUD) already exist by then — so
	// `-ExecCmds="Eclipse.Guide.Overlay 1"` set the variable and nothing opened,
	// which is exactly what the owner hit on 2026-07-25. A change sink makes the
	// order irrelevant, and it is the same event-driven shape as the device
	// listener above rather than a poll.
	if (IConsoleVariable* GuideVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Eclipse.Guide.Overlay")))
	{
		GuideVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateWeakLambda(this,
			[this](IConsoleVariable* Changed)
			{
				bGuideVisible = Changed->GetInt() > 0;
				ApplyPanelVisibility();
				RefreshGuideRows(/*bForce*/ true);
				Rebuild();
				UE_LOG(LogEclipse, Display, TEXT("Test guide: CVar set after mount -> panel %s (%d rows)."),
					bGuideVisible ? TEXT("OPEN") : TEXT("closed"), GuideRows.Num());
			}));
	}

	ApplyPanelVisibility();
	RefreshDeviceHighlight();
	RefreshPlaytestRows();
	RefreshGauntletRows(/*bForce*/ true);
	RefreshGuideRows(/*bForce*/ true);
	Rebuild();
}

void UEclipseMissionHudWidget::NativeDestruct()
{
	using namespace EclipseGauntletOverlay;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			for (FEclipseEventSubscriptionHandle& Handle : EventHandles)
			{
				Bus->Unsubscribe(Handle);
			}
		}
	}
	EventHandles.Reset();

	if (DeviceChangedHandle.IsValid())
	{
		if (UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
		{
			Devices->OnInputHardwareDeviceChangedNative.Remove(DeviceChangedHandle);
		}
		DeviceChangedHandle.Reset();
	}

	// Drop the sink with the widget: the CVar outlives this HUD, and a callback
	// pointing at a torn-down widget is a crash waiting for the next mission.
	if (IConsoleVariable* GuideVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Eclipse.Guide.Overlay")))
	{
		GuideVar->SetOnChangedCallback(FConsoleVariableDelegate());
	}

	if (SummaryCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(SummaryCommand);
		SummaryCommand = nullptr;
	}

	// Teardown is "closing the gauntlet" (mission end, level travel, quit): the
	// verdict block must survive it. Nothing measured or answered = nothing to
	// say, and a shot round is not a gauntlet at all. A half-walked test guide is
	// evidence too, so it counts as something to say.
	// EEN AUTOMATISCHE DRAAI IS GEEN SESSIE, en mag er ook geen achterlaten.
	//
	// GEMETEN op 27-07: deel 1 van de gids ("wat is er veranderd sinds je vorige
	// sessie") filtert op de datum van het NIEUWSTE eindrapport in Saved/Logs — en
	// dat rapport bleek door de SUITE zelf geschreven, om 10:14. De gids liet
	// daardoor alles van die dag weg, inclusief alle vijf de dingen die diezelfde
	// dag voor de owner waren gebouwd. Hij zou "niets sinds je vorige sessie" te
	// zien krijgen terwijl er vijf dingen te controleren waren.
	//
	// De filter is niet fout: een eindrapport HOORT een sessie te markeren. Wat
	// fout is, is dat de automatisering er een achterlaat. Gereedschap dat sporen
	// nalaat die op echt werk lijken, vervuilt precies het signaal dat het moet
	// bewaken — dezelfde vorm als de contracttest die ik vandaag zelf liet lekken.
	//
	// Vandaar hier en niet in de filter: de bron dicht in plaats van de lezer
	// leren omgaan met rommel.
	// -EclipseShotPlay staat er NIET voor de sier en was zelfs de belangrijkste:
	// het rapport van 10:14 dat de gids blind maakte, kwam uit de OPNAMERONDE, en
	// die draait als gewoon spel — geen commandlet, geen automation. Mijn eerste
	// versie van deze guard dekte alleen die twee en had dus precies de dader
	// gemist. Gevonden door te kijken hoe verify.ps1 de ronde start in plaats van
	// aan te nemen dat "automatisch" één ding betekent.
	const bool bAutomatedRun = GIsAutomationTesting || IsRunningCommandlet()
		|| FParse::Param(FCommandLine::Get(), TEXT("EclipseShotPlay"));
	if (!bAutomatedRun && IsDebugHudAllowed()
		&& (ComposeVerdict(GatherCriteria()).OpenCount < CriterionCount || GuideProgress.HasAnyProgress()))
	{
		EmitVerdictSummary();
	}

	Super::NativeDestruct();
}

bool UEclipseMissionHudWidget::IsDebugHudAllowed()
{
	return !FParse::Param(FCommandLine::Get(), TEXT("EclipseShot"));
}

void UEclipseMissionHudWidget::ShowDamageFrom(const FVector& ImpactPoint)
{
	const APlayerController* Owner = GetOwningPlayer();
	const APawn* MyPawn = Owner != nullptr ? Owner->GetPawn() : nullptr;
	if (MyPawn == nullptr || DamageIndicator == nullptr)
	{
		return;
	}

	// De hoek TUSSEN waar je kijkt en waar de klap vandaan kwam, in graden. Op de
	// KIJKRICHTING en niet op de lichaamsrichting: de indicator hangt aan je
	// scherm, en je scherm is je camera. Sinds het camera-relatieve model van
	// vanochtend lopen die twee bovendien uiteen.
	const FVector ToImpact = (ImpactPoint - MyPawn->GetActorLocation()).GetSafeNormal2D();
	const FVector Facing = Owner->GetControlRotation().Vector().GetSafeNormal2D();
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(
		FVector::CrossProduct(Facing, ToImpact).Z,
		FVector::DotProduct(Facing, ToImpact)));

	DamageIndicator->SetRenderTransformAngle(Angle);
	DamageIndicator->SetVisibility(ESlateVisibility::HitTestInvisible);
	// 1,2 s: lang genoeg om je hoofd om te draaien, kort genoeg om niet te blijven
	// hangen als je al reageerde. Ruimer dan de hitmarker, want die bevestigt iets
	// wat je zelf deed en deze vertelt je iets wat je nog niet wist.
	DamageIndicatorSecondsLeft = 1.2f;
}

void UEclipseMissionHudWidget::OnHitLanded(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Landed = Payload.GetPtr<FEclipseCombatEventPayload>();
	if (Landed == nullptr || HitMarker == nullptr)
	{
		return;
	}

	const APlayerController* Owner = GetOwningPlayer();
	const APawn* MyPawn = Owner != nullptr ? Owner->GetPawn() : nullptr;
	if (MyPawn == nullptr)
	{
		return;
	}

	// GERAAKT WORDEN: waar kwam het vandaan? (owner-opdracht 26-07 avond, punt 2.)
	// Hetzelfde feit draagt zowel de schutter als het slachtoffer, dus hier is
	// geen tweede mechanisme voor nodig.
	if (Landed->Victim.Get() == MyPawn)
	{
		ShowDamageFrom(Landed->Origin);
		return;
	}

	// Alleen schoten van de SPELER. Zonder deze poort licht je kruis op als je
	// squad iemand raakt, en dan bevestigt de marker niet meer jóuw schot — dan is
	// hij ruis in plaats van feedback.
	const AEclipseCharacter* Shooter = Cast<AEclipseCharacter>(Landed->Shooter.Get());
	if (Shooter == nullptr || Shooter != MyPawn)
	{
		return;
	}

	// Eigen VORM en KLEUR bij een kopschot (owner-opdracht 26-07 avond, punt 2).
	// Vorm én kleur, niet alleen kleur: kleur alleen is voor een deel van de
	// spelers geen onderscheid, en een X leest ook in je ooghoek anders dan een +.
	// De KEUZE staat in EclipseGauntletOverlay::MakeHitMarker, puur en toetsbaar;
	// hier staat alleen het tekenen. Zie die functie voor waarom vorm én kleur
	// verschillen en waarom de duur korter is dan het vuurinterval.
	const EclipseGauntletOverlay::FEclipseHitMarker Marker =
		EclipseGauntletOverlay::MakeHitMarker(Landed->bHeadshot);
	HitMarker->SetText(FText::FromString(Marker.Glyph));
	HitMarker->SetColorAndOpacity(FSlateColor(Marker.Colour));
	HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	HitMarkerSecondsLeft = Marker.Seconds;
}

void UEclipseMissionHudWidget::OnAnyFact(FGameplayTag /*EventTag*/, const FInstancedStruct& /*Payload*/)
{
	Rebuild();
	ApplyPanelVisibility();
	RefreshGauntletRows(/*bForce*/ false);

	// The guide's responsiveness row carries the live order measurement, so it
	// follows the same throttled, hidden-is-free path — never a rebuild.
	RefreshGuideRows(/*bForce*/ false);
}

void UEclipseMissionHudWidget::Rebuild()
{
	if (LiveBox == nullptr)
	{
		return;
	}
	LiveBox->ClearChildren();

	auto AddLine = [this](const FString& Text, const FLinearColor& Colour = ColourNeutral)
	{
		return AddHudTextRow(*WidgetTree, *LiveBox, Text, Colour);
	};

	const UGameInstance* GameInstance = GetGameInstance();
	if (const UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr)
	{
		const bool bAlarm = Mission->IsAlarmRaised();
		const bool bCasualty = Mission->HasAnyCasualtyThisRun();
		// The mode word and the two controls that matter come FIRST (13.2 finding):
		// a tester who boots into a run should never have to guess whether the game
		// has his input. The base hub says the mirror image of this line.
		AddLine(FString::Printf(TEXT("== MISSION ACTIVE  [%s]%s%s  —  WASD move · Q hold = Command Mode · F2 controls · F3 testgids =="),
			*UEnum::GetValueAsString(Mission->GetPhase()).RightChop(FString(TEXT("EEclipseMissionPhase::")).Len()),
			bAlarm ? TEXT("  ALARM") : TEXT(""),
			bCasualty ? TEXT("  CASUALTY") : TEXT("")));

		const TArray<FName>& Done = Mission->GetCompletedObjectiveIds();

		// Voided optionals through the SAME pure evaluation the debrief pays out
		// with (SPEC-P2-04), so the HUD and the wallet can never disagree — the
		// art review's finding was a completed optional that quietly stayed ticked
		// after the alarm/casualty latch had already killed its payout.
		TArray<FEclipseObjectiveDef> PaidOptionals;
		TArray<FName> MissedOptionals;
		EclipseMissionLogic::EvaluateOptionalObjectives(
			Mission->GetActiveObjectives(), Done, bAlarm, bCasualty, PaidOptionals, MissedOptionals);

		for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
		{
			const bool bDone = Done.Contains(Objective.ObjectiveId);
			const bool bVoidedNow = MissedOptionals.Contains(Objective.ObjectiveId);

			// An optional whose latch already tripped is lost whether it was
			// completed or not; the tester should see that while there is still
			// time to change plans, not first at debrief.
			const bool bConditionLost = Objective.bOptional
				&& ((Objective.bRequiresNoAlarm && bAlarm) || (Objective.bRequiresNoCasualties && bCasualty));

			FString Suffix;
			if (Objective.bOptional)
			{
				const TCHAR* Cause = Objective.bRequiresNoAlarm && bAlarm && Objective.bRequiresNoCasualties && bCasualty
					? TEXT("alarm + casualty")
					: (Objective.bRequiresNoAlarm && bAlarm ? TEXT("alarm") : TEXT("casualty"));
				Suffix = bVoidedNow
					? FString::Printf(TEXT("  (optional — VOID: %s, not paid)"), Cause)
					: bConditionLost
						? FString::Printf(TEXT("  (optional — already lost: %s)"), Cause)
						: FString(TEXT("  (optional)"));
			}

			AddLine(FString::Printf(TEXT("  [%s] %s%s"),
				bDone ? TEXT("X") : TEXT(" "), *Objective.Description.ToString(), *Suffix),
				bVoidedNow || bConditionLost ? ColourOpen : ColourNeutral);
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
		{
			AddLine(TEXT("-- squad orders --"));
			for (const FString& Line : Squad->GetOrderStateLines())
			{
				AddLine(FString::Printf(TEXT("  %s"), *Line));
			}
		}
	}

	// Command Mode debug lines (SPEC-P2-02 step 4 — the HUD stays a consumer;
	// the component owns the state). Rebuild is event-driven, so these refresh
	// on ModeEntered/Exited and every order fact.
	if (const APlayerController* Controller = GetOwningPlayer())
	{
		if (const UEclipseCommandModeComponent* Command = Controller->FindComponentByClass<UEclipseCommandModeComponent>())
		{
			for (const FString& Line : Command->GetDebugLines())
			{
				AddLine(Line);
			}
		}
	}
}

void UEclipseMissionHudWidget::BuildPlayerLayer()
{
	// VOLGORDE IS TEKENVOLGORDE op een canvas: wat later wordt toegevoegd, ligt
	// erbovenop. Kruis onderop, trefteken erover (anders verdwijnt de bevestiging
	// ín het kruis), teller rechtsonder, richtingsindicator als volvlaks laag
	// bovenop — die is verborgen tot je geraakt wordt en neemt dus niets weg.
	BuildCrosshair();
	BuildHitMarker();
	BuildAmmoReadout();
	BuildDamageIndicator();
}

void UEclipseMissionHudWidget::SubscribePlayerEvents()
{
	UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (Bus == nullptr)
	{
		// Luid, want dit is precies de stille fout die een verplaatst element
		// waardeloos maakt: het widget staat er, de bron niet, en het scherm toont
		// eeuwig de beginstand (14.3.5).
		UE_LOG(LogEclipse, Warning,
			TEXT("HUD: geen event-bus — het trefteken en de richtingsindicator krijgen nooit een feit te zien."));
		return;
	}

	// De treffer krijgt een EIGEN abonnement en niet de familie-route, want hij mag
	// geen volledige herbouw van de tekstlijst veroorzaken: er wordt tot 6,67 keer
	// per seconde geraakt, en OnAnyFact tekent alles opnieuw.
	//
	// Hetzelfde feit draagt schutter én slachtoffer, dus dit ene abonnement voedt
	// zowel het trefteken als de richtingsindicator (zie OnHitLanded).
	EventHandles.Add(Bus->Subscribe(
		EclipseTags::Event_Combat_HitLanded,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionHudWidget::OnHitLanded),
		FEclipseCombatEventPayload::StaticStruct()));
}

void UEclipseMissionHudWidget::BuildHitMarker()
{
	// De hitmarker: één tekstblok in het midden, onzichtbaar tot er iets geraakt
	// wordt. Tekst en geen afbeelding, en dat is een bewuste beperking — er ligt
	// geen hitmarker-textuur in het project, en er een verzinnen zou betekenen dat
	// ik iets teken. Een '+' die kort oplicht doet precies wat een hitmarker moet
	// doen: bevestigen DAT je raakte, zonder je blik van het doel te halen.
	HitMarker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HitMarker"));
	if (UCanvasPanelSlot* MarkerSlot = Canvas->AddChildToCanvas(HitMarker))
	{
		MarkerSlot->SetAutoSize(true);
		// Anker in het midden van het scherm; de offset centreert het teken zelf.
		MarkerSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerSlot->SetPosition(FVector2D::ZeroVector);
	}
	FSlateFontInfo MarkerFont = HitMarker->GetFont();
	MarkerFont.Size = 28;
	HitMarker->SetFont(MarkerFont);
	// Zelfde donkere rand als het kruis: een bevestiging die je op een lichte muur
	// niet ziet, bevestigt niets.
	ApplyLegibilityOutline(*HitMarker);
	HitMarker->SetVisibility(ESlateVisibility::Hidden);
}

void UEclipseMissionHudWidget::BuildAmmoReadout()
{
	// DE MUNITIEHOEK. Vier elementen die samen één blok vormen, en dat is de
	// reparatie van defect 1 en 4 tegelijk.
	//
	// WAT ER STOND: één tekstblok, AutoSize, verankerd rechtsonder en uitgelijnd op
	// zijn eigen rechteronderhoek, op (-48, -32). Twee dingen mis:
	//
	//  1. HET DROEG TWEE FEITEN. Tijdens een herlaadbeurt werd de hele tekst
	//     vervangen door "HERLADEN", dus precies wanneer je wilt weten hoeveel er
	//     straks in zit, stond het er niet.
	//
	//  2. AUTOSIZE + RECHTSUITGELIJND LOOPT ÉÉN FRAME ACHTER. GEZIEN op
	//     HUD_wapen_E_na_wissel.png: `Sidearm_Scrap` liep tot beeldkolom 1278 van
	//     1279 en de "12 / 12" stond helemaal buiten beeld. De oorzaak is niet de
	//     offset (die zou op 1232 uitkomen) maar dat de slot-geometrie de GEWENSTE
	//     MAAT VAN DE VORIGE LAYOUT gebruikt: wordt de tekst langer, dan wordt hij
	//     één frame lang geplaatst alsof hij nog kort was, en groeit hij rechts het
	//     beeld uit. Elke wapenwissel levert dus zo'n frame op.
	//
	// Vandaar een VAST KADER met een eigen breedte in plaats van AutoSize. Wat er
	// ook in komt te staan, het blok kan zijn eigen rand niet meer uit — een langere
	// wapennaam kan het probleem niet opnieuw maken.
	AmmoBlock = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AmmoBlock"));
	if (UCanvasPanelSlot* BlockSlot = Canvas->AddChildToCanvas(AmmoBlock))
	{
		BlockSlot->SetAutoSize(false);
		BlockSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		BlockSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		BlockSlot->SetSize(FVector2D(AmmoBlockWidthPx, AmmoBlockHeightPx));
		BlockSlot->SetPosition(FVector2D::ZeroVector); // per frame gezet, zie ApplySafeAreaLayout
	}
	AmmoBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// HET PANEEL ERACHTER — de eerste stap van de Borderlands-vormtaal (O-8 = "vol",
	// REFERENTIE_HUD_BORDERLANDS.md §1: "dikke inktranden om alles" en §4.2: "doe
	// ÉÉN element helemaal af in die taal en pas het daarna pas op de rest toe").
	//
	// De munitieteller is met opzet dat ene element: hij is het grootste getal op het
	// scherm en dus de plek waar een verkeerde richting het snelst zichtbaar is.
	//
	// EN HIJ DIENT EEN LEESBAARHEIDSDOEL, niet alleen een stijldoel — dat is de harde
	// tegeneis uit §3. GEMETEN op de frames van vanavond: de teller staat rechtsonder
	// pal boven de GELE WEGMARKERING (luminantie tot 195), en dat is precies de
	// ondergrond waarop lichte tekst wegvalt. Een donker vlak eronder maakt het
	// contrast onafhankelijk van waar de speler toevallig staat. Zou stijl en
	// leesbaarheid hier botsen, dan wint leesbaarheid; hier wijzen ze dezelfde kant op.
	//
	// Vlakke kleuren, geen textuur: er ligt geen HUD-paneeltextuur in het project en
	// er een verzinnen zou betekenen dat ik ga tekenen.
	// LET OP DE KLEURRUIMTE — en dit is geen theorie maar een gemeten fout van
	// vanavond. Eerste versie zette het vlak op FLinearColor(0.055, 0.058, 0.075).
	// Dat LEEK donker en werd op het frame (60, 62, 71), terwijl de wereld eronder
	// (45, 45, 56) was: het paneel maakte de hoek LICHTER in plaats van donkerder,
	// precies het tegenovergestelde van waarvoor het er staat.
	//
	// Oorzaak: Slate behandelt een FLinearColor als LINEAIR en zet hem naar sRGB voor
	// het scherm. Lineair 0,055 is sRGB 0,25 — dus 64 van de 255, niet 14.
	// FromSRGBColor doet de omrekening in de goede richting, en dan staat er in de
	// code het getal dat je op het frame terugmeet.
	//
	// Dezelfde klasse fout als de rest van dit dossier: een waarde in de
	// administratie is niet de waarde op het scherm, en alleen nameten scheelt dat.
	AmmoPanelInk = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AmmoPanelInk"));
	AmmoPanelInk->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(5, 5, 7, 255)));
	if (UCanvasPanelSlot* InkSlot = AmmoBlock->AddChildToCanvas(AmmoPanelInk))
	{
		InkSlot->SetAutoSize(false);
		InkSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		// De inktrand steekt aan alle kanten 3 px uit onder het vlak erop; dat IS de
		// dikke lijn uit §15.5, alleen dan in Slate in plaats van als post-effect op
		// 3D-geometrie (die haalt een widget per definitie nooit).
		InkSlot->SetOffsets(FMargin(-3.0f, -3.0f, -3.0f, -3.0f));
	}

	AmmoPanelFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AmmoPanelFill"));
	// Niet zwart maar heel donker blauwgrijs met 82 % dekking: het paneel hoort de
	// wereld te dempen, niet weg te snijden. Volle dekking leest als een gat in beeld.
	// sRGB 18/19/24 ligt ver onder de donkerste ondergrond die hier gemeten is (26),
	// dus het paneel is op ELKE plek in de wijk het donkerste vlak — ook boven de
	// gele wegmarkering van 195 waar de teller anders wegvalt.
	AmmoPanelFill->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(18, 19, 24, 209)));
	if (UCanvasPanelSlot* FillSlot = AmmoBlock->AddChildToCanvas(AmmoPanelFill))
	{
		FillSlot->SetAutoSize(false);
		FillSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FillSlot->SetOffsets(FMargin(0.0f));
	}

	// Alles binnen het blok is RECHTS uitgelijnd op dezelfde lijn, zodat "7" en "30"
	// op dezelfde plek eindigen. Een teller die verspringt terwijl je hem afleest is
	// erger dan geen teller.
	auto AddToBlock = [this](UTextBlock* Text, float RightPx, float BottomPx)
	{
		if (UCanvasPanelSlot* Slot = AmmoBlock->AddChildToCanvas(Text))
		{
			Slot->SetAutoSize(true);
			Slot->SetAnchors(FAnchors(1.0f, 1.0f));
			Slot->SetAlignment(FVector2D(1.0f, 1.0f));
			Slot->SetPosition(FVector2D(-RightPx, -BottomPx));
		}
	};

	// HET MAGAZIJNGETAL IS HET GROOTSTE GETAL OP HET SCHERM
	// (REFERENTIE_HUD_BORDERLANDS.md §1 regel 3). Dat is geen versiering maar de
	// hiërarchie: één ding is het belangrijkst, de rest wijkt.
	AmmoReadout = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoReadout"));
	FSlateFontInfo AmmoFont = AmmoReadout->GetFont();
	AmmoFont.Size = 46;
	AmmoFont.TypefaceFontName = TEXT("Bold");
	AmmoReadout->SetFont(AmmoFont);
	ApplyLegibilityOutline(*AmmoReadout, 2);
	AddToBlock(AmmoReadout, /*RightPx*/ 46.0f, /*BottomPx*/ 0.0f);

	// De magazijnMAAT kleiner en gedimd ernaast: hij verandert nooit, dus hij hoort
	// niet mee te vechten om je blik.
	AmmoCapacity = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoCapacity"));
	FSlateFontInfo CapacityFont = AmmoCapacity->GetFont();
	CapacityFont.Size = 20;
	AmmoCapacity->SetFont(CapacityFont);
	AmmoCapacity->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.74f)));
	ApplyLegibilityOutline(*AmmoCapacity);
	AddToBlock(AmmoCapacity, /*RightPx*/ 0.0f, /*BottomPx*/ 6.0f);

	// De wapennaam erboven — de LEESBARE naam uit de data (defect 3).
	WeaponReadout = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeaponReadout"));
	FSlateFontInfo WeaponFont = WeaponReadout->GetFont();
	WeaponFont.Size = 18;
	WeaponReadout->SetFont(WeaponFont);
	WeaponReadout->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.90f)));
	ApplyLegibilityOutline(*WeaponReadout);
	AddToBlock(WeaponReadout, /*RightPx*/ 0.0f, /*BottomPx*/ 54.0f);

	// EN DE HERLAADREGEL ERBOVEN, met een eigen plek. Dit is defect 1: hij staat
	// NAAST de kogels en niet in plaats van de kogels.
	ReloadReadout = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReloadReadout"));
	FSlateFontInfo ReloadFont = ReloadReadout->GetFont();
	ReloadFont.Size = 16;
	ReloadReadout->SetFont(ReloadFont);
	ReloadReadout->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.72f, 0.20f)));
	ApplyLegibilityOutline(*ReloadReadout);
	AddToBlock(ReloadReadout, /*RightPx*/ 0.0f, /*BottomPx*/ 92.0f);

	// De voortgangsbalk: een donkere goot met een oplopende vulling erin. Twee
	// afbeeldingen met een vlakke kleur — geen textuur, dus er wordt niets getekend
	// wat er niet ligt.
	ReloadBarTrack = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ReloadBarTrack"));
	ReloadBarTrack->SetColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.06f, 0.85f));
	if (UCanvasPanelSlot* TrackSlot = AmmoBlock->AddChildToCanvas(ReloadBarTrack))
	{
		TrackSlot->SetAutoSize(false);
		TrackSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		TrackSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		TrackSlot->SetSize(FVector2D(ReloadBarWidthPx, 6.0f));
		TrackSlot->SetPosition(FVector2D(0.0f, -84.0f));
	}

	ReloadBarFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ReloadBarFill"));
	ReloadBarFill->SetColorAndOpacity(FLinearColor(1.0f, 0.72f, 0.20f, 1.0f));
	if (UCanvasPanelSlot* FillSlot = AmmoBlock->AddChildToCanvas(ReloadBarFill))
	{
		FillSlot->SetAutoSize(false);
		// LINKS verankerd binnen de goot, want de vulling groeit naar rechts en moet
		// dus aan zijn linkerkant vastzitten. Rechts verankeren zou hem laten
		// krimpen vanaf de verkeerde kant en dat leest als aftellen in plaats van
		// vollopen.
		FillSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		FillSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		FillSlot->SetSize(FVector2D(0.0f, 6.0f));
		FillSlot->SetPosition(FVector2D(-ReloadBarWidthPx, -84.0f));
	}

	SetVisibilityIfChanged(ReloadReadout, false);
	SetVisibilityIfChanged(ReloadBarTrack, false);
	SetVisibilityIfChanged(ReloadBarFill, false);
}

void UEclipseMissionHudWidget::ApplySafeAreaLayout()
{
	// DE MARGE HOORT PER FRAME GEZET TE WORDEN EN NIET BIJ DE BOUW, en dat is een
	// reparatie op mijn eigen eerste versie die ik alleen door NAMETEN gevonden heb.
	//
	// DE METING, in drie opnamerondes op 31-07:
	//   slotwaarde 12 -> 9 px op het frame
	//   slotwaarde 36 -> 25 px
	//   slotwaarde 54 -> 55 px
	// De eerste twee schalen met ~0,70; de derde met 1,02. Eén schaalfactor kan die
	// drie niet verklaren, en dat betekende dat mijn model van de keten fout was —
	// niet dat er een getal bijgesteld moest worden.
	//
	// WAT ER ECHT GEBEURT. Slot-posities staan in Slate-eenheden en worden met de
	// DPI-schaal van de viewport vermenigvuldigd; op deze machine is dat 0,666
	// (Windows staat op 150 %). Maar die schaal is bij NativeOnInitialized nog NIET
	// de definitieve: de viewport heeft zijn maat dan nog niet. Ik bakte dus een
	// getal in dat berekend was met een verkeerde schaal, en de fout verschilde per
	// ronde omdat het moment verschilde.
	//
	// Dit is exact dezelfde klasse fout als waar deze hele laag op vastliep
	// (RebuildWidget vóór NativeConstruct): iets vastleggen op een moment waarop de
	// bron er nog niet is. De oplossing is dezelfde vorm — niet één keer vroeg, maar
	// nakijken zolang het kan veranderen. Bovendien MOET dat: een speler die zijn
	// venster sleept of van resolutie wisselt, hoort zijn marge mee te zien gaan.
	//
	// Gratis als er niets veranderde, dus dit kost geen frame (12.4).
	const FVector2D Margin = CurrentTitleSafeMarginPx();
	if (Margin.Equals(LastAppliedSafeMarginPx, 0.5))
	{
		return;
	}
	LastAppliedSafeMarginPx = Margin;

	if (Root != nullptr)
	{
		if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(Root->Slot))
		{
			RootSlot->SetPosition(Margin);
		}
	}
	if (AmmoBlock != nullptr)
	{
		if (UCanvasPanelSlot* BlockSlot = Cast<UCanvasPanelSlot>(AmmoBlock->Slot))
		{
			BlockSlot->SetPosition(FVector2D(-Margin.X, -Margin.Y));
		}
	}
	UE_LOG(LogEclipse, Verbose, TEXT("HUD: titel-veilige marge gezet op %.1f slot-eenheden."), Margin.X);
}

FVector2D UEclipseMissionHudWidget::CurrentTitleSafeMarginPx() const
{
	// De schermmaat uit de viewport en niet uit de widget-geometrie: bij
	// NativeOnInitialized bestaat de Slate-geometrie nog niet, en 0x0 zou de marge
	// op zijn ondergrens vastzetten zonder dat iets dat meldt.
	FVector2D Size(1280.0, 720.0);
	if (const APlayerController* Owner = GetOwningPlayer())
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		Owner->GetViewportSize(SizeX, SizeY);
		if (SizeX > 0 && SizeY > 0)
		{
			Size = FVector2D(SizeX, SizeY);
		}
	}
	// GEDEELD DOOR DE DPI-SCHAAL, en dat is een reparatie op mijn eigen eerste
	// versie — gevonden door na te meten in plaats van aan te nemen.
	//
	// GEMETEN op de opnameronde van 31-07 21:41: ik zette de marge op 36 px en op
	// het frame stond hij op 25. UMG rekent slot-posities in SLATE-EENHEDEN en
	// vermenigvuldigt die met de DPI-schaal van de viewport; de standaardcurve van
	// de engine geeft op een korte zijde van 720 een schaal van 0,666, en
	// 36 x 0,666 = 24. De marge kwam dus stelselmatig te krap uit, en precies op de
	// kleine vensters waar hij het hardst nodig is.
	//
	// Dit is dezelfde kloof als waar deze hele laag op vastliep: een getal in de
	// administratie is niet hetzelfde als een getal op het scherm. Delen door de
	// schaal maakt van "36" een belofte in SCHERMPIXELS in plaats van in
	// Slate-eenheden.
	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	return EclipseHudReadout::TitleSafeMarginPx(Size) / Scale;
}

void UEclipseMissionHudWidget::BuildDamageIndicator()
{
	// De richtingsindicator komt uit een pack die al in het project ligt en die
	// niemand aanriep (`Screen_Damage_Indicator`, gevonden 26-07). Een
	// Blueprint-widget, dus laden via zijn gegenereerde klasse.
	//
	// Ontbreekt hij, dan blijft de rest van de HUD gewoon werken en zegt hij dat
	// één keer — dit is decoratie die je mist, geen systeem dat stukgaat (14.3.5).
	UClass* IndicatorClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/Screen_Damage_Indicator/UI/WBP_DamageIndicator.WBP_DamageIndicator_C"));
	if (IndicatorClass == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("HUD: WBP_DamageIndicator niet gevonden — je ziet niet uit welke richting je geraakt wordt (14.3.5)."));
		return;
	}

	DamageIndicator = CreateWidget<UUserWidget>(GetOwningPlayer(), IndicatorClass);
	if (DamageIndicator == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("HUD: WBP_DamageIndicator kon niet worden aangemaakt (geen eigenaar-controller?)."));
		return;
	}

	if (UCanvasPanelSlot* IndicatorSlot = Canvas->AddChildToCanvas(DamageIndicator))
	{
		// Vult het hele scherm en draait om zijn midden: de pack tekent zijn
		// eigen pijl/rand, wij bepalen alleen de hoek.
		IndicatorSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		IndicatorSlot->SetOffsets(FMargin(0.0f));
	}
	DamageIndicator->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	DamageIndicator->SetVisibility(ESlateVisibility::Hidden);
}

void UEclipseMissionHudWidget::BuildCrosshair()
{
	// HET RICHTKRUIS. Owner-melding 27-07: "ik kan niet zien waar ik richt."
	// Nagekeken en de melding klopt volledig — de enige treffer op 'Crosshair' in
	// het hele project was EMouseCursor::Crosshairs, de VORM van de
	// muisaanwijzer, en die staat in het veld juist uit. Er is dus nooit een
	// richtkruis geweest, terwijl de startbat er al die tijd naar vroeg.
	//
	// Dit maakt ook de hitmarker pas af: die zat op een kruis dat niet bestond,
	// dus een treffer verscheen op een plek die je verder nergens aan kon
	// herkennen. Zelfde constructie als de hitmarker en om dezelfde reden tekst:
	// er ligt geen richtkruis-textuur in het project en er een verzinnen zou
	// betekenen dat ik ga tekenen.
	//
	// EERST HET KRUIS, DAN DE HITMARKER: de canvas tekent in volgorde van
	// toevoegen, dus de hitmarker hoort erná zodat een treffer OVER het kruis
	// oplicht in plaats van eronder te verdwijnen.
	// GEEN GLYPH MEER, MAAR GEOMETRIE — en dat is de reparatie van defect 5.
	//
	// GEMETEN op alle elf HUD-frames van 31-07 20:36: het kruis was een tekstglyph
	// van 7x9 px met 17 pixels boven luminantie 200. Dat is 0,0018 % van een beeld
	// van 1280x720. De donkere rand die hier eerder is aangebracht ZIT ER ECHT (de
	// donkerste pixel in het middenvak meet 0,0 op elk frame), dus het is geen
	// contrastprobleem meer — het is een MAAT-probleem, en dat is een ander soort
	// fout dan waar de vorige ronde op mikte.
	//
	// Tweede vondst uit dezelfde meting, en die kon alleen op pixelniveau opvallen:
	// de inkt van de '+' stond op elk frame op y = -5..+3 ten opzichte van het
	// schermmidden, dus het kruis hing 1 px TE HOOG. Een tekstglyph is gecentreerd
	// op zijn regelvak en niet op zijn eigen inkt, en een vizier dat niet op het
	// richtpunt staat is precies zo fout als een vizier dat je niet ziet.
	//
	// Vier balken rond een gat lossen beide op: de maat is een getal in plaats van
	// een fonteigenschap, en het middelpunt is per constructie het middelpunt.
	// Dit is ook wat REFERENTIE_HUD_BORDERLANDS.md §2 vraagt (vorm per wapentype).
	// Tekenen doe ik hiermee niet: het zijn vlakke kleuren, geen textuur.
	CrosshairRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrosshairRoot"));
	if (UCanvasPanelSlot* RootSlot = Canvas->AddChildToCanvas(CrosshairRoot))
	{
		RootSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		RootSlot->SetOffsets(FMargin(0.0f));
	}
	CrosshairRoot->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Per arm twee lagen: een donkere onderlaag die één pixel aan alle kanten
	// uitsteekt, en de lichte balk erop. Dat is dezelfde redenering als de
	// font-outline op de teksten — een teken dat op een lichte ondergrond wegvalt,
	// wijst nergens naar — maar nu op geometrie, waar een font-outline niet bestaat.
	CrosshairArms.Reset();
	CrosshairShadows.Reset();
	for (int32 Arm = 0; Arm < CrosshairArmCount; ++Arm)
	{
		UImage* Shadow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Shadow->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		CrosshairRoot->AddChildToCanvas(Shadow);
		CrosshairShadows.Add(Shadow);
	}
	for (int32 Arm = 0; Arm < CrosshairArmCount; ++Arm)
	{
		UImage* Bar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		// Niet zuiver wit: tegen de oranje horizon en het lichte asfalt van het
		// district verdwijnt wit. Een lichte koele tint met volle dekking blijft op
		// beide leesbaar.
		Bar->SetColorAndOpacity(FLinearColor(0.90f, 0.95f, 1.0f, 1.0f));
		CrosshairRoot->AddChildToCanvas(Bar);
		CrosshairArms.Add(Bar);
	}

	// EN EEN PUNT IN HET MIDDEN — met een eigen donkere rand, want zonder die rand
	// bestond hij alleen in de code.
	//
	// GEZIEN op HUD_kruis_ondergrond_1.png (31-07 21:5x), op 5x uitvergroot: van het
	// middenpunt was GEEN pixel te vinden. Hij stond op 2x2 px met alfa 0,55 — op een
	// lichte ondergrond is dat per constructie niets. En juist bij heupvuur staan de
	// vier balken 29 px uit elkaar, dus dan is het middenpunt het ENIGE dat het
	// richtpunt nog aanwijst.
	//
	// Dat het punt "gedimd" moest zijn was een aanname van mij en geen eis: hij mag
	// het doelwit niet BEDEKKEN, en dat regel je met MAAT (3 px), niet met alfa.
	CrosshairCentreShadow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CrosshairCentreShadow"));
	CrosshairCentreShadow->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	CrosshairRoot->AddChildToCanvas(CrosshairCentreShadow);

	CrosshairCentre = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CrosshairCentre"));
	CrosshairCentre->SetColorAndOpacity(FLinearColor(0.90f, 0.95f, 1.0f, 1.0f));
	CrosshairRoot->AddChildToCanvas(CrosshairCentre);

	RefreshCrosshair();
}

void UEclipseMissionHudWidget::RefreshCrosshair()
{
	if (CrosshairRoot == nullptr || CrosshairArms.Num() != CrosshairArmCount)
	{
		return;
	}

	const APawn* Body = GetOwningPlayerPawn();
	const UEclipseHitscanWeaponComponent* Weapon = Body != nullptr
		? Body->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;

	// GEEN WAPEN, GEEN KRUIS. Een vizier boven een lege hand wijst nergens naar.
	const bool bShow = Weapon != nullptr;
	if (CrosshairRoot->GetVisibility() != (bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed))
	{
		CrosshairRoot->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow)
	{
		return;
	}

	int32 ViewportX = 1280;
	int32 ViewportY = 720;
	if (const APlayerController* Owner = GetOwningPlayer())
	{
		Owner->GetViewportSize(ViewportX, ViewportY);
	}

	// DE SPREIDING DIE NU GELDT, uit dezelfde functie waar het SCHOT hem vandaan
	// haalt. Niet uit het profiel.
	//
	// Hier stond `bAiming ? GetAimSpreadDegrees() : GetHipSpreadDegrees()`, en dat
	// was fout op precies de manier waar ik in de header van GetCurrentSpreadDegrees
	// tegen waarschuwde: ik had de formule aan de wapenkant netjes op één plek gezet
	// en hem hier alsnog nagebouwd. GEZIEN op de frames van 31-07 22:0x: in derde
	// persoon (mikkend) stond het kruis 32 px wijd en in eerste persoon (heup) 58 px,
	// terwijl er in geen van beide standen geschoten werd — het kruis toonde dus de
	// spreiding die je ZOU krijgen als je aan het vuren was, niet die van je volgende
	// schot. Dat volgende schot is per "eerste schot is zuiver" juist zuiver.
	//
	// Met GetCurrentSpreadDegrees trekt het kruis dicht zodra je de trekker een halve
	// seconde loslaat, en dat is geen cosmetiek: dat IS de terugkoppeling waar de
	// zuiver-eerste-schot-regel om vraagt. Zonder die terugkoppeling is de regel
	// onzichtbaar en dus onbespeelbaar.
	const bool bAiming = Body != nullptr && Body->IsA<AEclipseCharacter>()
		&& Cast<AEclipseCharacter>(Body)->IsAiming();
	const float Spread = Weapon->GetCurrentSpreadDegrees();
	// De FOV bepaalt alleen de PROJECTIE van die hoek op pixels; mikken zoomt in, dus
	// dezelfde hoek dekt dan meer beeld. Dat is de enige reden dat bAiming hier nog
	// staat.
	const float Fov = bAiming ? 64.0f : 80.0f;

	EclipseHudReadout::FEclipseCrosshairLayout Layout = EclipseHudReadout::ComposeCrosshair(
		Spread, Weapon->GetPelletsPerShot(), Weapon->GetAimSpreadDegrees(),
		static_cast<float>(ViewportY), Fov);

	// DEZELFDE DPI-CORRECTIE ALS BIJ DE MARGE, en om dezelfde reden: ComposeCrosshair
	// rekent in SCHERMPIXELS (de spreidingshoek projecteert op schermpixels), maar een
	// canvas-slot krijgt SLATE-EENHEDEN. Zonder deze deling werd een balk van 3 px op
	// 720p als 2 px getekend — GEMETEN op de ronde van 21:41 — en dat is precies de
	// dunte waar het oude kruis op wegviel. Het gat mag NIET meeschalen op een andere
	// manier dan de rest, anders belooft het kruis een andere spreiding dan de kogel.
	const float DpiScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	Layout.ArmLengthPx /= DpiScale;
	Layout.ThicknessPx /= DpiScale;
	Layout.GapPx /= DpiScale;

	// Niets doen als er niets veranderde: dit draait per frame, en een layout-invalidatie
	// per frame per balk is precies het soort HUD-kosten dat 12.4 verbiedt.
	if (FMath::IsNearlyEqual(Layout.ArmLengthPx, LastCrosshairArmPx, 0.05f)
		&& FMath::IsNearlyEqual(Layout.GapPx, LastCrosshairGapPx, 0.05f)
		&& FMath::IsNearlyEqual(Layout.ThicknessPx, LastCrosshairThicknessPx, 0.05f))
	{
		return;
	}
	LastCrosshairArmPx = Layout.ArmLengthPx;
	LastCrosshairGapPx = Layout.GapPx;
	LastCrosshairThicknessPx = Layout.ThicknessPx;

	// Volgorde: boven, onder, links, rechts. Haken laten de verticale balken weg —
	// dat is wat een shotgun-vizier in Borderlands doet en het leest meteen anders.
	const bool bBrackets = Layout.Shape == EclipseHudReadout::EEclipseCrosshairShape::Brackets;
	for (int32 Arm = 0; Arm < CrosshairArmCount; ++Arm)
	{
		const bool bVertical = Arm < 2;
		const float Sign = (Arm % 2 == 0) ? -1.0f : 1.0f;
		const bool bArmVisible = !(bBrackets && bVertical);

		const FVector2D Size = bVertical
			? FVector2D(Layout.ThicknessPx, Layout.ArmLengthPx)
			: FVector2D(Layout.ArmLengthPx, Layout.ThicknessPx);
		// Het midden van de balk ligt op gat + halve armlengte vanaf het schermmidden.
		const float Offset = Sign * (Layout.GapPx + Layout.ArmLengthPx * 0.5f);
		const FVector2D Position = bVertical ? FVector2D(0.0f, Offset) : FVector2D(Offset, 0.0f);

		auto Place = [this](UImage* Image, const FVector2D& InSize, const FVector2D& InPosition, bool bIsVisible)
		{
			if (Image == nullptr)
			{
				return;
			}
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot))
			{
				Slot->SetAutoSize(false);
				Slot->SetAnchors(FAnchors(0.5f, 0.5f));
				Slot->SetAlignment(FVector2D(0.5f, 0.5f));
				Slot->SetSize(InSize);
				Slot->SetPosition(InPosition);
			}
			SetVisibilityIfChanged(Image, bIsVisible);
		};

		// De schaduw is aan alle kanten één pixel groter; daarmee staat er rondom de
		// hele balk een donkere lijn in plaats van alleen aan twee zijden.
		Place(CrosshairShadows[Arm], Size + FVector2D(2.0f, 2.0f), Position, bArmVisible);
		Place(CrosshairArms[Arm], Size, Position, bArmVisible);
	}

	// Het middenpunt schaalt mee met de dikte van de balken, zodat hij op een groot
	// scherm niet terugvalt naar de onvindbare maat waar dit dossier over ging.
	const float CentrePx = FMath::Max(3.0f, Layout.ThicknessPx);
	auto PlaceCentre = [this](UImage* Image, float SizePx)
	{
		if (Image == nullptr)
		{
			return;
		}
		if (UCanvasPanelSlot* CentreSlot = Cast<UCanvasPanelSlot>(Image->Slot))
		{
			CentreSlot->SetAutoSize(false);
			CentreSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CentreSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CentreSlot->SetSize(FVector2D(SizePx, SizePx));
			CentreSlot->SetPosition(FVector2D::ZeroVector);
		}
		SetVisibilityIfChanged(Image, true);
	};
	PlaceCentre(CrosshairCentreShadow, CentrePx + 2.0f);
	PlaceCentre(CrosshairCentre, CentrePx);
}

void UEclipseMissionHudWidget::ApplyLegibilityOutline(UTextBlock& Text, int32 OutlineSizePx)
{
	// EEN ECHTE RAND RONDOM, en niet de slagschaduw waar ik mee begon.
	//
	// De schaduw was één verschoven kopie: op het uitvergrote beeld liep de donkere
	// rand alleen langs de onder- en rechterkant, en dan blijft de linkerbovenhoek
	// licht-op-licht. Half opgelost is hier niet opgelost — de eis is dat het teken
	// op ELKE ondergrond staat, niet op de helft ervan.
	//
	// FFontOutlineSettings tekent de rand aan alle kanten, in de font-rasterisatie
	// zelf. Eén pixel: genoeg om te scheiden, klein genoeg om een kruis van 9 px
	// niet in een blok te veranderen.
	FSlateFontInfo Font = Text.GetFont();
	Font.OutlineSettings.OutlineSize = OutlineSizePx;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	Text.SetFont(Font);
}

void UEclipseMissionHudWidget::BuildStaticPanels()
{
	using namespace EclipseGauntletOverlay;

	// The controller caches this widget across missions, so a second mount runs
	// this again: drop the previous mount's row pointers or the refreshes would
	// write into orphaned widgets that are no longer in the tree.
	GauntletRows.Reset();
	PlaytestRows.Reset();
	GuideRows.Reset();
	MouseKeyboardCells.Reset();
	ControllerCells.Reset();

	// ---- R3 criteria panel: one row per verdict line, never rebuilt ----------
	GauntletPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(GauntletPanel);
	GauntletRows.Reserve(VerdictLineCount);
	for (int32 Line = 0; Line < VerdictLineCount; ++Line)
	{
		GauntletRows.Add(AddHudTextRow(*WidgetTree, *GauntletPanel, FString()));
	}
	AddHudTextRow(*WidgetTree, *GauntletPanel,
		TEXT("F4 schone pick · F5 mis-pick · F6 comfort · F7 vertrouwen · F8 nieuwe beat · F2 besturing · F3 testgids · H playtest"),
		ColourDim);

	// ---- Control overview (F2): label column + the two device columns --------
	ControlsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(ControlsPanel);
	AddHudTextRow(*WidgetTree, *ControlsPanel, TEXT("== BESTURING (F2) =="));

	UHorizontalBox* Table = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ControlsPanel->AddChildToVerticalBox(Table);

	auto AddColumn = [this, Table](const FString& Header, TArray<TObjectPtr<UTextBlock>>* OutCells)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* Slot = Table->AddChildToHorizontalBox(Column))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
		}
		UTextBlock* HeaderCell = AddHudTextRow(*WidgetTree, *Column, Header);
		if (OutCells != nullptr)
		{
			OutCells->Add(HeaderCell);
		}
		return Column;
	};

	UVerticalBox* ActionColumn = AddColumn(TEXT("ACTIE"), nullptr);
	UVerticalBox* MouseKeyboardColumn = AddColumn(TEXT("MUIS + TOETSENBORD"), &MouseKeyboardCells);
	UVerticalBox* ControllerColumn = AddColumn(TEXT("CONTROLLER"), &ControllerCells);

	for (const FEclipseControlRow& Row : GetControlRows())
	{
		AddHudTextRow(*WidgetTree, *ActionColumn, Row.Action);
		MouseKeyboardCells.Add(AddHudTextRow(*WidgetTree, *MouseKeyboardColumn, Row.MouseKeyboard));
		ControllerCells.Add(AddHudTextRow(*WidgetTree, *ControllerColumn, Row.Controller));
	}

	// ---- 13.2 playtest checklist (H) ----------------------------------------
	PlaytestPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(PlaytestPanel);
	for (int32 Line = 0; Line < PlaytestQuestionCount + 2; ++Line)
	{
		PlaytestRows.Add(AddHudTextRow(*WidgetTree, *PlaytestPanel, FString()));
	}
	AddHudTextRow(*WidgetTree, *PlaytestPanel, TEXT("toggle per regel: 6 7 8 9 0  (onbeantwoord -> goed -> slecht)"), ColourDim);

	// ---- In-game test guide (F3): fixed rows, refreshed in place ------------
	GuidePanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(GuidePanel);
	GuideRows.Reserve(EclipseTestGuide::GuidePanelLineCount);
	for (int32 Line = 0; Line < EclipseTestGuide::GuidePanelLineCount; ++Line)
	{
		GuideRows.Add(AddHudTextRow(*WidgetTree, *GuidePanel, FString()));
	}
}

bool UEclipseMissionHudWidget::IsGauntletPanelVisible() const
{
	return CVarEclipseGauntletOverlay.GetValueOnGameThread() > 0;
}

void UEclipseMissionHudWidget::ApplyPanelVisibility()
{
	// SetVisibility invalidates layout, so only ever call it on a real change —
	// this runs on every fact.
	auto Apply = [](UVerticalBox* Panel, bool bVisible)
	{
		if (Panel == nullptr)
		{
			return;
		}
		const ESlateVisibility Desired = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
		if (Panel->GetVisibility() != Desired)
		{
			Panel->SetVisibility(Desired);
		}
	};

	Apply(GauntletPanel, IsGauntletPanelVisible());
	Apply(ControlsPanel, bControlsVisible);
	Apply(PlaytestPanel, bPlaytestVisible);
	Apply(GuidePanel, IsGuidePanelVisible());
}

void UEclipseMissionHudWidget::RefreshGauntletRows(bool bForce)
{
	using namespace EclipseGauntletOverlay;

	if (GauntletRows.IsEmpty() || !IsGauntletPanelVisible())
	{
		return; // hidden panel = zero work per fact
	}

	const double NowWallSeconds = FPlatformTime::Seconds();
	if (!bForce && (NowWallSeconds - LastGauntletRefreshWallSeconds) < GauntletRefreshIntervalSeconds)
	{
		return; // a four-soldier broadcast order arrives as four facts in one frame
	}
	LastGauntletRefreshWallSeconds = NowWallSeconds;

	const FEclipseGauntletVerdict Verdict = ComposeVerdict(GatherCriteria());
	for (int32 Line = 0; Line < GauntletRows.Num() && Line < Verdict.Lines.Num(); ++Line)
	{
		UTextBlock* Row = GauntletRows[Line];
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(Verdict.Lines[Line]));

		// Line 0 is the title, the last line the tally; in between, one criterion
		// each — the same order ComposeVerdict guarantees.
		const bool bCriterionRow = Line >= 1 && Line <= CriterionCount;
		Row->SetColorAndOpacity(FSlateColor(bCriterionRow
			? StatusColour(Verdict.Statuses[Line - 1])
			: (Line == 0 ? ColourNeutral
				: (Verdict.FailedCount > 0 ? ColourFail : (Verdict.OpenCount > 0 ? ColourOpen : ColourPass)))));
	}
}

void UEclipseMissionHudWidget::RefreshPlaytestRows()
{
	using namespace EclipseGauntletOverlay;

	const TArray<FString> Lines = ComposePlaytestBlock(PlaytestAnswers);
	for (int32 Line = 0; Line < PlaytestRows.Num() && Line < Lines.Num(); ++Line)
	{
		if (PlaytestRows[Line] != nullptr)
		{
			PlaytestRows[Line]->SetText(FText::FromString(Lines[Line]));
		}
	}
}

FString UEclipseMissionHudWidget::DescribeOrderRoundTrip() const
{
	// The same wall-clock meter criterion 1 is judged on (the squad layer owns it);
	// the guide only quotes it, so the two panels can never disagree. Empty means
	// "not measured", never "fine so far".
	const UWorld* World = GetWorld();
	const UEclipseSquadSubsystem* Squad = World != nullptr ? World->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	if (Squad == nullptr)
	{
		return FString();
	}

	const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats& Stats = Squad->GetOrderRoundTripStats();
	if (Stats.SampleCount <= 0)
	{
		return FString();
	}
	constexpr double BarSeconds = EclipseSquadOrderLogic::FEclipseOrderRoundTripStats::BarSeconds;
	return FString::Printf(TEXT("%d/%d binnen %.2f s (slechtste %.3f s)"),
		Stats.WithinBarCount, Stats.SampleCount, BarSeconds, Stats.WorstSeconds);
}

void UEclipseMissionHudWidget::RefreshGuideRows(bool bForce)
{
	if (GuideRows.IsEmpty() || !IsGuidePanelVisible())
	{
		return; // hidden guide = zero work per fact
	}

	const double NowWallSeconds = FPlatformTime::Seconds();
	if (!bForce && (NowWallSeconds - LastGuideRefreshWallSeconds) < GauntletRefreshIntervalSeconds)
	{
		return;
	}
	LastGuideRefreshWallSeconds = NowWallSeconds;

	const int32 ActiveStep = GuideProgress.GetActiveIndex();
	// The look numbers come from the same tuning asset the controller reads, so the
	// panel can never advertise a value the game is not actually using.
	FString LookSummary;
	if (const AEclipsePlayerController* Controller = Cast<AEclipsePlayerController>(GetOwningPlayer()))
	{
		LookSummary = Controller->DescribeLookTuning();
	}
	const TArray<FString> Lines = EclipseTestGuide::ComposeGuidePanelLines(GuideProgress, DescribeOrderRoundTrip(), LookSummary);
	for (int32 Line = 0; Line < GuideRows.Num() && Line < Lines.Num(); ++Line)
	{
		UTextBlock* Row = GuideRows[Line];
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(Lines[Line]));

		// Line 0 is the header, the last line the tally; in between one step each,
		// in the order ComposeGuidePanelLines guarantees. The active step is the
		// only bright row — settled steps dim so the eye lands on what to do next.
		const int32 StepIndex = Line - 1;
		const bool bStepRow = StepIndex >= 0 && StepIndex < EclipseTestGuide::GetGuideStepCount();
		Row->SetColorAndOpacity(FSlateColor(bStepRow
			? (StepIndex == ActiveStep ? ColourNeutral : ColourDim)
			: ColourNeutral));
	}
}

void UEclipseMissionHudWidget::ToggleGuidePanel()
{
	bGuideVisible = !bGuideVisible;
	ApplyPanelVisibility();
	RefreshGuideRows(/*bForce*/ true);
	// Audible on the log, because "F3 does nothing" and "F3 opened a panel you
	// cannot see" are different bugs and looked identical to the owner.
	UE_LOG(LogEclipse, Display, TEXT("Test guide: panel %s (%d rows built)."),
		bGuideVisible ? TEXT("OPEN") : TEXT("closed"), GuideRows.Num());
}

void UEclipseMissionHudWidget::ConfirmGuideStep()
{
	if (!IsGuidePanelVisible() || !GuideProgress.ConfirmActive())
	{
		return; // a closed guide steals no key; a finished guide has nothing to settle
	}
	OnGuideStepSettled();
}

void UEclipseMissionHudWidget::SkipGuideStep()
{
	if (!IsGuidePanelVisible() || !GuideProgress.RejectActive())
	{
		return;
	}
	OnGuideStepSettled();
}


void UEclipseMissionHudWidget::OnGuideStepSettled()
{
	using namespace EclipseGauntletOverlay;

	// Deel 3 IS the 13.2 checklist, so answering it in the guide answers it in the
	// playtest block: one set of books on the owner's gate question.
	const TArray<EclipseTestGuide::EEclipseGuideStepState>& States = GuideProgress.GetStates();
	for (int32 Step = 0; Step < States.Num(); ++Step)
	{
		const int32 Question = EclipseTestGuide::GetPlaytestQuestionIndex(Step);
		if (Question == INDEX_NONE || !PlaytestAnswers.IsValidIndex(Question))
		{
			continue;
		}
		switch (States[Step])
		{
		case EclipseTestGuide::EEclipseGuideStepState::Confirmed:
			PlaytestAnswers[Question] = EEclipseGauntletAnswer::Good;
			break;
		case EclipseTestGuide::EEclipseGuideStepState::Rejected:
			PlaytestAnswers[Question] = EEclipseGauntletAnswer::Bad;
			break;
		default:
			break; // Pending/Skipped leave the row unanswered — never a fabricated verdict
		}
	}
	RefreshPlaytestRows();
	RefreshGuideRows(/*bForce*/ true);

	// Finishing the list archives it through the gauntlet's existing summary path
	// — one mechanism, one file. Once per mount: further keys have nothing to add.
	if (GuideProgress.IsComplete() && !bGuideSummaryEmitted)
	{
		bGuideSummaryEmitted = true;
		EmitVerdictSummary();
	}
}

void UEclipseMissionHudWidget::RefreshDeviceHighlight()
{
	// UInputDeviceSubsystem::GetMostRecentlyUsedHardwareDevice is Enhanced Input's
	// own answer to "what is the player holding right now" (Engine/Source/Runtime/
	// Engine/Classes/GameFramework/InputDeviceSubsystem.h); its
	// OnInputHardwareDeviceChangedNative delegate is what re-runs this function.
	// Missing subsystem (commandlet/dedicated server) = keyboard column marked.
	bool bGamepadActive = false;
	const APlayerController* Controller = GetOwningPlayer();
	if (const UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		if (Controller != nullptr)
		{
			const FHardwareDeviceIdentifier Hardware = Devices->GetMostRecentlyUsedHardwareDevice(Controller->GetPlatformUserId());
			bGamepadActive = Hardware.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad;
		}
	}

	auto ApplyColumn = [](TArray<TObjectPtr<UTextBlock>>& Cells, bool bActive, const FString& HeaderText)
	{
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			if (Cells[Index] == nullptr)
			{
				continue;
			}
			Cells[Index]->SetColorAndOpacity(FSlateColor(bActive ? ColourNeutral : ColourDim));
			if (Index == 0)
			{
				Cells[Index]->SetText(FText::FromString(bActive
					? FString::Printf(TEXT(">> %s <<"), *HeaderText)
					: HeaderText));
			}
		}
	};

	ApplyColumn(MouseKeyboardCells, !bGamepadActive, TEXT("MUIS + TOETSENBORD"));
	ApplyColumn(ControllerCells, bGamepadActive, TEXT("CONTROLLER"));
}

void UEclipseMissionHudWidget::ToggleControlsPanel()
{
	bControlsVisible = !bControlsVisible;
	ApplyPanelVisibility();
	if (bControlsVisible)
	{
		RefreshDeviceHighlight();
	}
}

void UEclipseMissionHudWidget::TogglePlaytestPanel()
{
	bPlaytestVisible = !bPlaytestVisible;
	ApplyPanelVisibility();
	if (bPlaytestVisible)
	{
		RefreshPlaytestRows();
	}
}

void UEclipseMissionHudWidget::NoteTargetingPick(bool bCleanPick)
{
	if (!IsGauntletPanelVisible())
	{
		return; // a closed panel steals no input
	}
	bCleanPick ? ++CleanPicks : ++MisPicks;
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CycleComfortAnswer()
{
	using namespace EclipseGauntletOverlay;

	if (!IsGauntletPanelVisible())
	{
		return;
	}
	ComfortAnswer = CycleAnswer(ComfortAnswer);
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CycleConfidenceAnswer()
{
	using namespace EclipseGauntletOverlay;

	if (!IsGauntletPanelVisible())
	{
		return;
	}
	ConfidenceAnswer = CycleAnswer(ConfidenceAnswer);
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::MarkEncounterBeat()
{
	if (!IsGauntletPanelVisible())
	{
		return;
	}

	// The tally lives in the component (it owns ModeEntered); the overlay only
	// tells it a beat ended — including an empty one, which is the measurement
	// that fails criterion 5.
	APlayerController* Controller = GetOwningPlayer();
	if (UEclipseCommandModeComponent* Command = Controller != nullptr ? Controller->FindComponentByClass<UEclipseCommandModeComponent>() : nullptr)
	{
		Command->BeginNextEncounterBeat(/*bCountEmptyBeat*/ true);
	}
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CyclePlaytestAnswer(int32 QuestionIndex)
{
	using namespace EclipseGauntletOverlay;

	if (!bPlaytestVisible || !PlaytestAnswers.IsValidIndex(QuestionIndex))
	{
		return;
	}
	PlaytestAnswers[QuestionIndex] = CycleAnswer(PlaytestAnswers[QuestionIndex]);
	RefreshPlaytestRows();
}

EclipseGauntletOverlay::FEclipseGauntletCriteria UEclipseMissionHudWidget::GatherCriteria() const
{
	using namespace EclipseGauntletOverlay;

	FEclipseGauntletCriteria Criteria;

	// Criterion 1 from the squad layer: it measured the round trips, wall clock.
	if (const UWorld* World = GetWorld())
	{
		if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
		{
			const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats& Stats = Squad->GetOrderRoundTripStats();
			Criteria.RoundTripSamples = Stats.SampleCount;
			Criteria.RoundTripWithinBar = Stats.WithinBarCount;
			Criteria.RoundTripWorstSeconds = Stats.WorstSeconds;
			Criteria.RoundTripBarSeconds = EclipseSquadOrderLogic::FEclipseOrderRoundTripStats::BarSeconds;
		}
	}

	// Criterion 5 from the Command Mode component: it owns the entries.
	if (const APlayerController* Controller = GetOwningPlayer())
	{
		if (const UEclipseCommandModeComponent* Command = Controller->FindComponentByClass<UEclipseCommandModeComponent>())
		{
			Criteria.EntriesThisBeat = Command->GetCommandEntriesThisBeat();
			Criteria.ClosedBeatCount = Command->GetObservedEncounterBeatCount();
			Criteria.AverageEntriesPerBeat = Command->GetAverageEntriesPerBeat();
		}
	}

	// Criteria 2-4: the tester's own answers, the only state this widget owns.
	Criteria.CleanPicks = CleanPicks;
	Criteria.MisPicks = MisPicks;
	Criteria.Comfort = ComfortAnswer;
	Criteria.Confidence = ConfidenceAnswer;
	return Criteria;
}

void UEclipseMissionHudWidget::EmitVerdictSummary()
{
	using namespace EclipseGauntletOverlay;

	const FEclipseGauntletVerdict Verdict = ComposeVerdict(GatherCriteria());

	TArray<FString> Block = Verdict.Lines;
	Block.Add(FString());
	Block.Append(ComposePlaytestBlock(PlaytestAnswers));

	// The test guide rides along in the same block and the same file (spec §4: the
	// guide is a mode of this overlay, not a second archive). A guide nobody walked
	// adds nothing — an empty section would only dilute the verdict above it.
	if (GuideProgress.HasAnyProgress())
	{
		Block.Add(FString());
		Block.Append(EclipseTestGuide::ComposeGuideSummaryBlock(GuideProgress));
	}

	for (const FString& Line : Block)
	{
		UE_LOG(LogEclipse, Display, TEXT("[R3] %s"), *Line);
	}

	// Saved/Logs keeps the block even when the session's console scroll is gone
	// (owner requirement: the verdict input may not be lost on shutdown).
	const FString Path = FPaths::ProjectLogDir() /
		FString::Printf(TEXT("EclipseGauntletR3_%s.txt"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString Body = FString::Join(Block, TEXT("\r\n")) + TEXT("\r\n");
	if (!FFileHelper::SaveStringToFile(Body, *Path))
	{
		// A missing archive must not be a crash and must not be silent (14.3.5).
		UE_LOG(LogEclipse, Warning, TEXT("Gauntlet: could not write the R3 block to %s — the log above is the record."), *Path);
		return;
	}
	UE_LOG(LogEclipse, Display, TEXT("Gauntlet: R3 block archived at %s"), *Path);
}
