#include "Combat/EclipseImpactMark.h"

#include "Components/StaticMeshComponent.h"
#include "Eclipse.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace EclipseImpactMark
{

namespace
{

/**
 * DE LIFT LANGS DE NORMAAL. Staat er tegen z-fighting met het oppervlak, en
 * `Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter` meet er op alle
 * twintig schoten op — verander dit getal niet zonder die test mee te nemen.
 */
constexpr float LiftCm = 1.0f;

/** Onderlinge afstand tussen ring en vulling, zodat twee doorschijnende vlakken niet vechten. */
constexpr float FillLiftCm = 0.5f;

/** Dikte van een vlak: 0,4 mm. Dun genoeg om als spoor te lezen en niet als blokje. */
constexpr float PlateThickness = 0.004f;

/**
 * DE ZON VAN HET DISTRICT.
 *
 * Een toon-master mengt licht- en schaduwkleur op N·L. Met een LightDir van nul is
 * die term nul en valt er niets te mengen — dat was in juli de enige parameter die
 * dit spoor niet zette terwijl elke andere gebruiker van het master hem wel zet.
 */
FVector SunTravel()
{
	return FRotator(-25.0f, 55.0f, 0.0f).Vector();
}

UStaticMesh* LoadShape(const TCHAR* Path)
{
	return LoadObject<UStaticMesh>(nullptr, Path);
}

/**
 * Een kindcomponent met een eigen schaal.
 *
 * De WORTEL van de actor blijft bewust op schaal 1 en draagt GEEN mesh. Dat is
 * geen stijlkeuze: kindcomponenten erven de schaal van hun ouder, dus zodra de
 * wortel de voetafdruk zou dragen (0,24 breed, 0,004 dik) zou de opstaande kern
 * die factor 0,004 in Z meekrijgen en tot een vliesje platgedrukt worden. De
 * wortel is een transformnaaf; alle maten zitten op de kinderen.
 */
UStaticMeshComponent* AddPart(AStaticMeshActor& Mark, UStaticMesh* Shape, const FName Name,
	const FVector& RelativeLocation, const FVector& Scale)
{
	if (Shape == nullptr)
	{
		return nullptr;
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(&Mark, Name);
	if (Part == nullptr)
	{
		return nullptr;
	}
	Part->SetupAttachment(Mark.GetRootComponent());
	Part->SetMobility(EComponentMobility::Movable);
	Part->RegisterComponent();
	Part->SetStaticMesh(Shape);
	Part->SetRelativeLocation(RelativeLocation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCastShadow(false);
	Part->SetAffectDistanceFieldLighting(false);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return Part;
}

/** Het toon-decal-master met masker; nullptr als er iets ontbreekt (dan blijft het deel ongekleurd). */
UMaterialInstanceDynamic* MakeDecalMaterial(UObject* Outer, const TCHAR* MaskPath,
	const FLinearColor& Color, float Emissive, float OpacityScale)
{
	UMaterialInterface* Master = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Art/M_EclipseToonDecal.M_EclipseToonDecal"));
	if (Master == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: M_EclipseToonDecal ontbreekt — dit deel krijgt geen materiaal."));
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Master, Outer);
	if (Mid == nullptr)
	{
		return nullptr;
	}
	// HET MASKER MET EEN NETTE TERUGVAL. T_impact_*_mask worden door
	// Tools/generate_impact_masks.py + Tools/import_impact_masks.py neergezet. Zijn
	// ze er niet, dan valt het spoor terug op de blob die er altijd al lag: kleiner
	// en zachter, maar zichtbaar — en de logregel zegt precies dát, zodat een
	// tegenvallende meting nooit als "de maat werkt niet" gelezen wordt terwijl in
	// werkelijkheid het masker ontbrak.
	UTexture* Mask = LoadObject<UTexture>(nullptr, MaskPath);
	if (Mask == nullptr)
	{
		Mask = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_blob_mask.T_blob_mask"));
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: masker %s ontbreekt — TERUGVAL op T_blob_mask. De randscherpte van deze meting is NIET het verscheepte recept."),
			MaskPath);
	}
	Mid->SetVectorParameterValue(TEXT("LitColor"), Color);
	Mid->SetVectorParameterValue(TEXT("ShadeColor"), Color * 0.45f);
	Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunTravel(), 0.0f)));
	Mid->SetScalarParameterValue(TEXT("EmissiveScale"), Emissive);
	// UVMode 1 = de UV's van de mesh zelf. Een vlak van de engine-kubus spant 0..1,
	// dus het masker rijdt precies één keer over het vlak. Op wereld-UV's zou het
	// masker over de grond schuiven en de vorm van het spoor van zijn PLEK afhangen.
	Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
	if (Mask != nullptr)
	{
		Mid->SetTextureParameterValue(TEXT("MaskTex"), Mask);
	}
	Mid->SetScalarParameterValue(TEXT("OpacityScale"), OpacityScale);
	if (UTexture* White = LoadObject<UTexture>(
		nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
	{
		Mid->SetTextureParameterValue(TEXT("AlbedoTex"), White);
	}
	return Mid;
}

/** Het opake toon-master voor de opstaande kern — die is een VOORWERP, geen decal. */
UMaterialInstanceDynamic* MakeSolidMaterial(UObject* Outer, const FLinearColor& Color, float Emissive)
{
	UMaterialInterface* Master = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
	if (Master == nullptr)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Master, Outer);
	if (Mid == nullptr)
	{
		return nullptr;
	}
	// De kern is het enige deel met een echte celband: hij heeft VOLUME, dus N·L
	// verschilt over zijn oppervlak. Een lichte en een donkere kant maken van een
	// bolletje een vorm — precies waar de stijlwet om vraagt.
	Mid->SetVectorParameterValue(TEXT("LitColor"), Color);
	Mid->SetVectorParameterValue(TEXT("ShadeColor"), Color * 0.38f);
	Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunTravel(), 0.0f)));
	Mid->SetScalarParameterValue(TEXT("EmissiveScale"), Emissive);
	Mid->SetScalarParameterValue(TEXT("UVMode"), 0.0f);
	return Mid;
}

} // namespace

const FRecipe& Verscheept()
{
	// ==========================================================================
	// DE MAAT, EN WAAROM HET GEEN ENKEL GETAL IS.
	//
	// De vraag is: hoe groot moet een kogelspoor zijn om op 8-15 m te lezen? De
	// verleiding is één schaal te verdubbelen. Reken je het uit, dan blijkt dat de
	// verkeerde knop:
	//
	//   Een PLAT vlak van S cm op afstand D, bekeken vanaf ooghoogte h, projecteert
	//   op k*S/D pixels breed en k*S*h/D^2 pixels HOOG (k = pixels per radiaal). De
	//   hoogte valt dus met het KWADRAAT van de afstand, want de scherende hoek
	//   knijpt hem dicht. Bij S=9, h=160, D=1000, k=1000: 9 px breed en 1,4 px hoog
	//   — en dat is precies het 9x3-kadertje dat gemeten is.
	//
	//   Een OPSTAAND element van H cm projecteert op k*H/D pixels hoog. Lineair.
	//   Geen cosinus, want een verticaal vlak staat bij een scherende blik juist
	//   frontaal.
	//
	// Op 15 m levert 5 cm hoogte 3,3 px op. Om dat met een PLAT vlak te evenaren zou
	// de voetafdruk naar 46 cm moeten — een half stoeptegel per kogel. Dat is de
	// tweede eis uit de opdracht ("zonder de wereld te beplakken") en die verliest.
	//
	// Vandaar vier knoppen in plaats van één:
	//
	//   1. MAAT     9 -> 28 cm voetafdruk. Draagt het spoor van dichtbij, op muren,
	//               en van bovenaf. Bij 2,5 s levensduur en een vuurinterval van
	//               0,15 s liggen er hooguit een stuk of zeventien tegelijk, samen
	//               ongeveer 1 m2 — een paar procent van één lichtplek van de
	//               bouwer. Beplakken is dit niet.
	//   2. RAND     De blob-falloff eruit. T_blob_mask is pas VOL dekkend tot r=0,45
	//               en zakt daarna weg; op 9 cm bleef er 4 cm echte dekking over.
	//               De nieuwe maskers zijn hard en dekken tot r=0,91 (ring) en 0,81
	//               (vulling), gemeten door hun eigen generator. Dat is bijna een
	//               factor 2 in effectieve maat vóór er iets groter werd, plus vol
	//               contrast tot aan de rand in plaats van een flank die in de
	//               antialiasing valt. En het is de stijlwet zelf: cel + inktlijn.
	//   3. OPBOUW   Een kern van 18 cm die 11 cm boven het oppervlak uitsteekt. Dit
	//               is wat op afstand het werk doet, want het is het enige deel dat
	//               niet met de kijkhoek dichtknijpt. Half in de grond, dus het
	//               leest als een gloeiende spatkern en niet als een balletje.
	//   4. FELHEID  Emissie 10 -> 20. Op afstand bestaat het spoor bijna alleen uit
	//               GEDEELTELIJK bedekte pixels; feller tilt die boven de drempel
	//               en doet voor het oog hetzelfde. Kost geen centimeter grond.
	//
	// CONTRAST ZIT IN DE COMBINATIE, niet in een van de vier. De ring is bijna
	// zwart, de vulling en de kern zijn heet amber. Die twee grenzen aan elkaar, en
	// juist die grens is wat een vorm laat lezen: een hete vlek zonder rand vervloeit
	// met wat eromheen ligt, een zwarte ring op donker asfalt op zichzelf ook. Samen
	// dragen ze elkaar, en het maakt niet uit welke kant het wegdek op valt.
	//
	// ==========================================================================
	// WAT DE EERSTE METING HIERAAN VERANDERDE — en het is precies waarom je meet
	// in plaats van uitrekent.
	//
	// Ronde 1 stond op voetafdruk 24, vulling 15, kern 10x6, emissie 12, met een
	// dikke inktband van r 0,58 tot 0,96. De ladder gaf 45 / 22 / 10 px op 8 / 10 /
	// 15 m tegen een voorspelling van 123 / 68 / 24. Het KADER klopte wel (19x8 px
	// op 8 m tegen 23x10 voorspeld), dus het silhouet stond goed en het OPPERVLAK
	// niet — er veranderde maar 30% van wat er in dat kader paste. Twee oorzaken,
	// allebei zichtbaar op de uitsnede en geen van beide te bedenken geweest:
	//
	//   A. ER ZAT EEN GAT IN HET MIDDEN. `FillCm = 15` klonk als 15 cm heet, maar
	//      het maskert dekte tot r=0,50 — dus 7,5 cm. De binnenrand van de ring lag
	//      op 14 cm. Daartussen was het spoor doorzichtig. Een maat in centimeters
	//      op een quad is niet de maat van wat je ziet; het masker beslist dat mee.
	//   B. EEN HOLLE RING IS DE VERKEERDE VORM VOOR EEN SCHERENDE BLIK. De boven- en
	//      onderband van een platte ring worden door de cosinus subpixel; er blijven
	//      alleen twee flanken over. De ring betaalt volle breedte en levert een
	//      handvol pixels. Hij blijft — van dichtbij en van bovenaf is hij de
	//      inktlijn die de stijl vraagt — maar hij is nu een LIJN om een gevulde
	//      schijf heen, en niet zelf de vulling.
	// ==========================================================================
	static const FRecipe R;
	return R;
}

const FRecipe& Nulmeting()
{
	static const FRecipe R = []
	{
		FRecipe N;
		N.Naam = TEXT("nulmeting-9cm-plat");
		N.FootprintCm = 9.0f;
		N.FillCm = 0.0f;
		N.CoreCm = 0.0f;
		N.CoreRiseCm = 0.0f;
		N.OpacityScale = 1.0f;
		N.RingMask = TEXT("/Game/Art/Decals/T_blob_mask.T_blob_mask");
		N.RingColor = FLinearColor(1.00f, 0.62f, 0.22f, 1.0f);
		N.HotEmissive = 10.0f;
		return N;
	}();
	return R;
}

AStaticMeshActor* Spawn(UWorld& World, const FVector& ImpactPoint, const FVector& Normal,
	const FRecipe& Recipe)
{
	UStaticMesh* Cube = LoadShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: /Engine/BasicShapes/Cube.Cube laadde NIET — geen spoor mogelijk."));
		return nullptr;
	}

	const FVector SafeNormal = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	const FVector Spot = ImpactPoint + SafeNormal * LiftCm;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;   // een inslag hoort nooit in een save
	AStaticMeshActor* Mark = World.SpawnActor<AStaticMeshActor>(
		Spot, FRotationMatrix::MakeFromZ(SafeNormal).Rotator(), Params);
	if (Mark == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Inslagspoor: SpawnActor gaf niets terug op %s."),
			*Spot.ToCompactString());
		return nullptr;
	}

	Mark->SetMobility(EComponentMobility::Movable);
	Mark->SetActorEnableCollision(false);
	// De wortel draagt geen mesh en geen schaal — zie AddPart. Wat er wel op moet:
	// niets aan het licht, want in een unlit wijk zou een schaduw van een inslag
	// meer kosten dan hij oplevert.
	if (UStaticMeshComponent* Hub = Mark->GetStaticMeshComponent())
	{
		Hub->SetStaticMesh(nullptr);
		Hub->SetCastShadow(false);
		Hub->SetAffectDistanceFieldLighting(false);
	}

	int32 Delen = 0;

	// 1. DE INKTRING — de buitenmaat en de rand.
	if (UStaticMeshComponent* Ring = AddPart(*Mark, Cube, TEXT("SpoorInkt"), FVector::ZeroVector,
		FVector(Recipe.FootprintCm * 0.01f, Recipe.FootprintCm * 0.01f, PlateThickness)))
	{
		// De nulmeting heeft geen vulling en dus ook geen ring om iets omheen te
		// zetten: daar IS dit vlak het hele spoor, en het krijgt de hete kleur.
		const bool bAlleenDitVlak = Recipe.FillCm <= 0.0f;
		if (UMaterialInstanceDynamic* Mid = MakeDecalMaterial(Mark,
			Recipe.RingMask,
			bAlleenDitVlak ? Recipe.HotColor : Recipe.RingColor,
			bAlleenDitVlak ? Recipe.HotEmissive : 1.0f,
			Recipe.OpacityScale))
		{
			Ring->SetMaterial(0, Mid);
		}
		++Delen;
	}

	// 2. DE HETE VULLING binnen de ring.
	if (Recipe.FillCm > 0.0f)
	{
		if (UStaticMeshComponent* Fill = AddPart(*Mark, Cube, TEXT("SpoorGloed"),
			FVector(0.0f, 0.0f, FillLiftCm),
			FVector(Recipe.FillCm * 0.01f, Recipe.FillCm * 0.01f, PlateThickness)))
		{
			if (UMaterialInstanceDynamic* Mid = MakeDecalMaterial(Mark, Recipe.FillMask,
				Recipe.HotColor, Recipe.HotEmissive, Recipe.OpacityScale))
			{
				Fill->SetMaterial(0, Mid);
			}
			++Delen;
		}
	}

	// 3. DE OPSTAANDE KERN — het enige deel dat niet met de kijkhoek dichtknijpt.
	if (Recipe.CoreCm > 0.0f && Recipe.CoreRiseCm > 0.0f)
	{
		UStaticMesh* Bol = LoadShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (Bol == nullptr)
		{
			// Nette terugval: zonder bol wordt het een kubusje. Lelijker, even hoog,
			// en er staat een regel in het log — nooit stil een ander ding meten.
			Bol = Cube;
			UE_LOG(LogEclipse, Warning,
				TEXT("Inslagspoor: /Engine/BasicShapes/Sphere ontbreekt — de kern wordt een kubus."));
		}
		// De engine-bol is 100 cm in doorsnede (straal 50). Het middelpunt zakt met
		// LiftCm mee omlaag zodat het precies op het OPPERVLAK ligt: de onderste
		// helft zit in de grond en wat je ziet is een koepel van CoreRiseCm hoog.
		if (UStaticMeshComponent* Core = AddPart(*Mark, Bol, TEXT("SpoorKern"),
			FVector(0.0f, 0.0f, -LiftCm),
			FVector(Recipe.CoreCm * 0.01f, Recipe.CoreCm * 0.01f, Recipe.CoreRiseCm * 0.02f)))
		{
			if (UMaterialInstanceDynamic* Mid = MakeSolidMaterial(Mark,
				Recipe.HotColor * 1.15f, Recipe.HotEmissive))
			{
				Core->SetMaterial(0, Mid);
			}
			++Delen;
		}
	}

	Mark->SetLifeSpan(Recipe.LifeSeconds);
	Mark->Tags.Add(TEXT("Eclipse_ImpactMark"));

	// DE MAAT IN HET LOG, want een spoor waarvan de maat niet in het log staat is
	// niet te koppelen aan de pixels die het harnas telt. De bounding box is het
	// eerlijke getal: hij dekt ring, vulling én kern, ongeacht hoe de schaal over de
	// componenten verdeeld is.
	const FBox Omvang = Mark->GetComponentsBoundingBox(true);
	UE_LOG(LogEclipse, Verbose,
		TEXT("Inslagspoor RECEPT '%s': %d delen, voetafdruk %.0f cm, kern %.0f cm x %.0f cm hoog, omvang %s."),
		Recipe.Naam, Delen, Recipe.FootprintCm, Recipe.CoreCm, Recipe.CoreRiseCm,
		*Omvang.GetSize().ToCompactString());

	return Mark;
}

} // namespace EclipseImpactMark
