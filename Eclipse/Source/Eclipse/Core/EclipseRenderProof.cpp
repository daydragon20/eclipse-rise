#include "Core/EclipseRenderProof.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Containers/Ticker.h"
#include "Eclipse.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Widgets/SWindow.h"

namespace EclipseRenderProof
{

bool TraceFreeSight(const UWorld& World, const FVector& From, const AActor& Target,
	const TArray<const AActor*>& Ignore, FString& OutBlocker, float& OutBlockDistance)
{
	OutBlocker.Reset();
	OutBlockDistance = -1.0f;

	const FVector To = Target.GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseFreeSight), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(&Target);
	for (const AActor* Skip : Ignore)
	{
		if (Skip != nullptr)
		{
			Params.AddIgnoredActor(Skip);
		}
	}

	FHitResult Hit;
	// ECC_Visibility en niet ECC_Camera: de vraag is wat het OOG tegenhoudt.
	if (!World.LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params))
	{
		return true;
	}

	const AActor* Blocker = Hit.GetActor();
	const FString Tag = (Blocker != nullptr && Blocker->Tags.Num() > 0)
		? Blocker->Tags[0].ToString() : FString(TEXT("-"));
	OutBlocker = FString::Printf(TEXT("%s(%s)"),
		Blocker != nullptr ? *Blocker->GetName() : TEXT("?"), *Tag);
	OutBlockDistance = static_cast<float>(FVector::Dist(From, Hit.ImpactPoint));
	return false;
}

namespace
{

/** Frames tussen een wijziging en de opname erna. De wereld-kanarie bewijst dat dit genoeg is. */
constexpr int32 SettleFrames = 8;
/** Per kanaal, 0-255. Onder deze stap telt een pixel als onveranderd. */
constexpr int32 ChannelThreshold = 6;
/** Halve zijde van het meetvenster rond de geprojecteerde plek van de verdachte. */
constexpr int32 RoiHalfSize = 260;

struct FDiff
{
	int64 Changed = 0;
	int64 ChangedInRoi = 0;
	int32 Strongest = 0;
	int32 MinX = 0, MinY = 0, MaxX = -1, MaxY = -1;
	double SumX = 0.0, SumY = 0.0;
	int64 Total = 0;
	bool bValid = false;

	float Promille() const
	{
		return Total > 0 ? 1000.0f * static_cast<float>(Changed) / static_cast<float>(Total) : 0.0f;
	}
};

/**
 * Verschil tussen twee opnames van hetzelfde beeld.
 *
 * `Roi` is het venster rond de plek waar de verdachte op het scherm hoort te
 * landen. Dat tweede getal bestaat omdat het eerste een keer bedrogen heeft: de
 * eerste ronde van dit harnas meldde 1346 veranderde pixels voor de positieve
 * controle, en dat bleek de engine-melding "Preparing Shaders (2)" te zijn die
 * tussen de twee opnames verdween. Een getal over het HELE frame kan niet zien
 * of het over de verdachte gaat; een getal over het venster wel.
 */
FDiff Compare(const TArray<FColor>& A, const TArray<FColor>& B, const FIntVector& Size, const FIntRect& Roi)
{
	FDiff D;
	D.Total = static_cast<int64>(Size.X) * static_cast<int64>(Size.Y);
	if (D.Total <= 0 || A.Num() != B.Num() || static_cast<int64>(A.Num()) < D.Total)
	{
		D.Total = 0;
		return D;
	}
	D.bValid = true;
	D.MinX = Size.X;
	D.MinY = Size.Y;
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const int32 Index = Y * Size.X + X;
			const FColor& Pa = A[Index];
			const FColor& Pb = B[Index];
			const int32 Delta = FMath::Max3(
				FMath::Abs(static_cast<int32>(Pa.R) - static_cast<int32>(Pb.R)),
				FMath::Abs(static_cast<int32>(Pa.G) - static_cast<int32>(Pb.G)),
				FMath::Abs(static_cast<int32>(Pa.B) - static_cast<int32>(Pb.B)));
			if (Delta > ChannelThreshold)
			{
				++D.Changed;
				D.Strongest = FMath::Max(D.Strongest, Delta);
				D.MinX = FMath::Min(D.MinX, X);
				D.MaxX = FMath::Max(D.MaxX, X);
				D.MinY = FMath::Min(D.MinY, Y);
				D.MaxY = FMath::Max(D.MaxY, Y);
				D.SumX += X;
				D.SumY += Y;
				if (Roi.Contains(FIntPoint(X, Y)))
				{
					++D.ChangedInRoi;
				}
			}
		}
	}
	return D;
}

bool CaptureWindow(TArray<FColor>& OutPixels, FIntVector& OutSize)
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}
	UGameViewportClient* ViewportClient = GEngine != nullptr ? GEngine->GameViewport : nullptr;
	TSharedPtr<SWindow> Window = ViewportClient != nullptr ? ViewportClient->GetWindow() : nullptr;
	if (!Window.IsValid())
	{
		return false;
	}
	OutPixels.Reset();
	OutSize = FIntVector::ZeroValue;
	return FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), OutPixels, OutSize)
		&& OutSize.X > 0 && OutSize.Y > 0;
}

void SavePng(const TArray<FColor>& Pixels, const FIntVector& Size, const FString& Name)
{
	if (Pixels.Num() <= 0 || Size.X <= 0 || Size.Y <= 0)
	{
		return;
	}
	TArray<FColor> Opaque = Pixels;
	for (FColor& Pixel : Opaque)
	{
		Pixel.A = 255;
	}
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("GRONDPROEF")
		/ (Name + TEXT(".png"));
	const FImageView Image(Opaque.GetData(), Size.X, Size.Y);
	FImageUtils::SaveImageByExtension(*Path, Image);
}

/** Zet een plaat neer met exact het recept van een grondvlak: engine-kubus, dun, geen botsing. */
AStaticMeshActor* SpawnPlate(UWorld& World, const FVector& Location, const FVector& Scale,
	UMaterialInterface* Material, const TCHAR* Tag)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube == nullptr)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
	if (Actor == nullptr)
	{
		return nullptr;
	}
	Actor->SetMobility(EComponentMobility::Movable);
	Actor->GetStaticMeshComponent()->SetStaticMesh(Cube);
	Actor->SetActorScale3D(Scale);
	if (Material != nullptr)
	{
		Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
	}
	Actor->GetStaticMeshComponent()->SetCastShadow(false);
	Actor->SetActorEnableCollision(false);
	Actor->Tags.Add(Tag);
	return Actor;
}

UMaterialInterface* LoadUnmistakableMaterial(UObject* Outer, FString& OutName)
{
	if (UMaterialInterface* Emissive = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial")))
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Emissive, Outer))
		{
			Mid->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(6.0f, 0.0f, 6.0f, 1.0f));
			OutName = TEXT("EmissiveMeshMaterial(magenta)");
			return Mid;
		}
	}
	if (UMaterialInterface* Basic = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		OutName = TEXT("BasicShapeMaterial");
		return Basic;
	}
	OutName = TEXT("GEEN");
	return nullptr;
}

/** Eén proef = twee opnames van hetzelfde beeld met precies één ding ertussen verborgen. */
struct FTrial
{
	FString Name;
	FString Note;
	/** Zet iets neer vlak voor de proef; dat ding is dan meteen de verdachte. */
	TFunction<AActor*(UWorld&)> Setup;
	TArray<TWeakObjectPtr<AActor>> Hide;
	/** Waar de verdachte op het scherm hoort te landen; leeg = geen venster, alleen het hele frame. */
	TWeakObjectPtr<AActor> RoiAnchor;
	FVector RoiWorld = FVector::ZeroVector;
	bool bHasRoiWorld = false;
	FDiff Result;
	bool bMeasured = false;
	FString Where;
	/** Hoeveel er werkelijk verborgen is. Nul mag alleen bij de ruisvloer — zie LogTrial. */
	int32 HideCount = 0;
	bool bNoiseFloor = false;
};

struct FGroundProof : public TSharedFromThis<FGroundProof>
{
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<ACameraActor> ProofCam;
	TWeakObjectPtr<AActor> Target;
	TArray<TWeakObjectPtr<AActor>> Planes;
	TArray<TWeakObjectPtr<AActor>> Marks;
	/** -EclipseSpoorProef: eerst een salvo, dan de INSLAGSPOREN meten in plaats van de grondvlakken. */
	bool bMarkMode = false;
	int32 ShotTicks = 0;
	float CamHeight = 700.0f;

	TArray<FTrial> Trials;
	TArray<FTrial> Done;
	int32 Stage = 0;
	int32 TrialIndex = 0;
	int32 Sub = 0;
	int32 Wait = 0;
	double ArmedAt = 0.0;
	FString ViewName = TEXT("SPELER");

	// De stiltepoort: pas meten als twee opeenvolgende opnames identiek zijn.
	int32 GateSub = 0;
	int32 GateStreak = 0;
	int32 GateTries = 0;

	TArray<FColor> ShotA;
	TArray<FColor> ShotB;
	FIntVector SizeA = FIntVector::ZeroValue;
	FIntVector SizeB = FIntVector::ZeroValue;
	TWeakObjectPtr<AActor> Temp;

	FTSTicker::FDelegateHandle Handle;
};

void CollectPlanes(UWorld& World, TArray<TWeakObjectPtr<AActor>>& Out)
{
	Out.Reset();
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr)
		{
			continue;
		}
		if (Actor->Tags.Contains(TEXT("Deco_Blob"))
			|| Actor->Tags.Contains(TEXT("Deco_LampPool"))
			|| Actor->Tags.Contains(TEXT("Deco_Pool")))
		{
			Out.Add(Actor);
		}
	}
}

void CollectMarks(UWorld& World, TArray<TWeakObjectPtr<AActor>>& Out)
{
	Out.Reset();
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (*It != nullptr && It->Tags.Contains(TEXT("Eclipse_ImpactMark")))
		{
			Out.Add(*It);
		}
	}
}

/**
 * STILTEPOORT. Twee opnames zonder enige wijziging moeten identiek zijn voordat
 * er iets gemeten wordt.
 *
 * Dit staat er niet voor de netheid. De eerste ronde van dit harnas leverde een
 * "positieve controle" van 1346 pixels op die volledig uit de engine-melding
 * "Preparing Shaders (2)" bestond — een overlay die tussen twee opnames
 * verdween. Elk getal uit een frame dat uit zichzelf verandert is waardeloos,
 * en welke van de twee verklaringen je eruit leest is dan een kwestie van geluk.
 */
bool QuietGate(FGroundProof& S, const TCHAR* Label)
{
	if (S.GateSub == 0)
	{
		S.Wait = SettleFrames;
		S.GateSub = 1;
		return false;
	}
	if (S.GateSub == 1)
	{
		if (--S.Wait > 0)
		{
			return false;
		}
		CaptureWindow(S.ShotA, S.SizeA);
		S.Wait = SettleFrames;
		S.GateSub = 2;
		return false;
	}
	if (--S.Wait > 0)
	{
		return false;
	}
	++S.GateTries;
	int64 Changed = -1;
	if (CaptureWindow(S.ShotB, S.SizeB) && S.SizeA == S.SizeB && S.SizeA.X > 0)
	{
		const FDiff D = Compare(S.ShotA, S.ShotB, S.SizeA, FIntRect());
		Changed = D.Changed;
	}
	if (Changed == 0)
	{
		++S.GateStreak;
	}
	else
	{
		S.GateStreak = 0;
		UE_LOG(LogEclipse, Display,
			TEXT("[GRONDPROEF STILTEPOORT %s] poging %d: het beeld verandert nog uit zichzelf (%lld pixels) — nog niet meten."),
			Label, S.GateTries, Changed);
	}
	S.GateSub = 0;
	if (S.GateStreak >= 2)
	{
		UE_LOG(LogEclipse, Display,
			TEXT("[GRONDPROEF STILTEPOORT %s] stil na %d pogingen: twee opeenvolgende opnames zijn identiek (0 pixels)."),
			Label, S.GateTries);
		S.GateStreak = 0;
		S.GateTries = 0;
		return true;
	}
	if (S.GateTries >= 40)
	{
		UE_LOG(LogEclipse, Error,
			TEXT("[GRONDPROEF STILTEPOORT %s] NOOIT STIL na %d pogingen — alles hierna is besmet met beeldruis en telt niet als meting."),
			Label, S.GateTries);
		S.GateStreak = 0;
		S.GateTries = 0;
		return true;
	}
	return false;
}

void LogTrial(const FTrial& Trial, const FString& ViewName)
{
	// EEN LEGE VERZAMELING VERBERGEN VERANDERT NUL PIXELS, en die nul ziet er
	// precies zo uit als "de verdachte rendert niet". Dat is vanavond echt
	// gebeurd: een vlag die stil niet parste liet deze proef met nul sporen
	// draaien en er rolde een keurige 0 uit. Sindsdien is nul-verbergen een
	// FOUT en geen uitslag.
	if (!Trial.bNoiseFloor && Trial.HideCount == 0)
	{
		UE_LOG(LogEclipse, Error,
			TEXT("[GRONDPROEF %s/%s] LEGE VERZAMELING — er was niets om te verbergen, dus deze proef meet NIETS. Elke 0 hier is betekenisloos."),
			*ViewName, *Trial.Name);
		return;
	}
	if (!Trial.Result.bValid)
	{
		UE_LOG(LogEclipse, Warning, TEXT("[GRONDPROEF %s/%s] GEEN GELDIGE OPNAME — deze proef meet niets."),
			*ViewName, *Trial.Name);
		return;
	}
	const FDiff& D = Trial.Result;
	if (D.Changed == 0)
	{
		UE_LOG(LogEclipse, Display,
			TEXT("[GRONDPROEF %s/%s] veranderd=0 heelframe, 0 in venster, van %lld pixels — dit ding draagt NIETS bij aan het beeld. %s %s"),
			*ViewName, *Trial.Name, D.Total, *Trial.Where, *Trial.Note);
		return;
	}
	UE_LOG(LogEclipse, Display,
		TEXT("[GRONDPROEF %s/%s] veranderd=%lld heelframe (%.2f promille), %lld in venster; sterkste=%d/255 zwaartepunt=(%.0f,%.0f) kader=(%d,%d)-(%d,%d). %s %s"),
		*ViewName, *Trial.Name, D.Changed, D.Promille(), D.ChangedInRoi, D.Strongest,
		D.SumX / static_cast<double>(D.Changed), D.SumY / static_cast<double>(D.Changed),
		D.MinX, D.MinY, D.MaxX, D.MaxY, *Trial.Where, *Trial.Note);
}

/** Draait de proevenlijst af. Geeft true zodra de lijst leeg is. */
bool RunTrials(FGroundProof& S)
{
	UWorld* World = S.World.Get();
	APlayerController* PC = S.PC.Get();
	if (World == nullptr || PC == nullptr || !S.Trials.IsValidIndex(S.TrialIndex))
	{
		return true;
	}
	FTrial& Trial = S.Trials[S.TrialIndex];

	switch (S.Sub)
	{
	case 0:
		if (Trial.Setup)
		{
			AActor* Spawned = Trial.Setup(*World);
			S.Temp = Spawned;
			Trial.Hide.Reset();
			if (Spawned != nullptr)
			{
				Trial.Hide.Add(Spawned);
				Trial.RoiAnchor = Spawned;
			}
		}
		Trial.HideCount = 0;
		for (const TWeakObjectPtr<AActor>& Weak : Trial.Hide)
		{
			Trial.HideCount += Weak.IsValid() ? 1 : 0;
		}
		S.Wait = SettleFrames;
		S.Sub = 1;
		return false;

	case 1:
		if (--S.Wait > 0)
		{
			return false;
		}
		if (!CaptureWindow(S.ShotA, S.SizeA))
		{
			UE_LOG(LogEclipse, Warning, TEXT("[GRONDPROEF %s/%s] opname A mislukt."), *S.ViewName, *Trial.Name);
			S.Sub = 0;
			++S.TrialIndex;
			return false;
		}
		S.Sub = 2;
		return false;

	case 2:
		for (const TWeakObjectPtr<AActor>& Weak : Trial.Hide)
		{
			if (AActor* Actor = Weak.Get())
			{
				Actor->SetActorHiddenInGame(true);
			}
		}
		S.Wait = SettleFrames;
		S.Sub = 3;
		return false;

	default:
	{
		if (--S.Wait > 0)
		{
			return false;
		}
		// HET VENSTER: waar hoort de verdachte op het scherm te staan? Zonder dit
		// kan een verandering ergens anders in het frame doorgaan voor bewijs.
		FIntRect Roi;
		FVector2D Screen = FVector2D::ZeroVector;
		bool bProjected = false;
		FVector Anchor = FVector::ZeroVector;
		if (const AActor* AnchorActor = Trial.RoiAnchor.Get())
		{
			Anchor = AnchorActor->GetActorLocation();
			bProjected = true;
		}
		else if (Trial.bHasRoiWorld)
		{
			Anchor = Trial.RoiWorld;
			bProjected = true;
		}
		if (bProjected)
		{
			bProjected = PC->ProjectWorldLocationToScreen(Anchor, Screen);
		}
		if (bProjected)
		{
			Roi = FIntRect(
				FMath::RoundToInt(Screen.X) - RoiHalfSize, FMath::RoundToInt(Screen.Y) - RoiHalfSize,
				FMath::RoundToInt(Screen.X) + RoiHalfSize, FMath::RoundToInt(Screen.Y) + RoiHalfSize);
			Trial.Where = FString::Printf(TEXT("[verdachte projecteert op (%.0f,%.0f), venster %d px]"),
				Screen.X, Screen.Y, 2 * RoiHalfSize);
		}
		else
		{
			Trial.Where = TEXT("[geen enkele plek om een venster op te hangen — alleen het hele frame telt]");
		}

		if (CaptureWindow(S.ShotB, S.SizeB) && S.SizeA == S.SizeB)
		{
			Trial.Result = Compare(S.ShotA, S.ShotB, S.SizeA, Roi);
			Trial.bMeasured = true;
			SavePng(S.ShotA, S.SizeA, FString::Printf(TEXT("%s_%s_A"), *S.ViewName, *Trial.Name));
			SavePng(S.ShotB, S.SizeB, FString::Printf(TEXT("%s_%s_B"), *S.ViewName, *Trial.Name));
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("[GRONDPROEF %s/%s] opname B mislukt of andere maat (%dx%d vs %dx%d)."),
				*S.ViewName, *Trial.Name, S.SizeA.X, S.SizeA.Y, S.SizeB.X, S.SizeB.Y);
		}
		for (const TWeakObjectPtr<AActor>& Weak : Trial.Hide)
		{
			if (AActor* Actor = Weak.Get())
			{
				Actor->SetActorHiddenInGame(false);
			}
		}
		if (AActor* Tijdelijk = S.Temp.Get())
		{
			Tijdelijk->Destroy();
		}
		S.Temp = nullptr;
		LogTrial(Trial, S.ViewName);
		S.Done.Add(Trial);
		S.Sub = 0;
		++S.TrialIndex;
		return false;
	}
	}
}

/** Waar staat de camera echt naar te kijken? Een gezette view target is geen bewijs van een verzette camera. */
void LogViewPoint(FGroundProof& S, const TCHAR* Label)
{
	APlayerController* PC = S.PC.Get();
	if (PC == nullptr)
	{
		return;
	}
	FVector Loc = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(Loc, Rot);
	const AActor* View = PC->GetViewTarget();
	FString Doel = TEXT("geen doelvlak");
	if (const AActor* T = S.Target.Get())
	{
		FVector2D Screen = FVector2D::ZeroVector;
		const bool bOn = PC->ProjectWorldLocationToScreen(T->GetActorLocation(), Screen);
		Doel = FString::Printf(TEXT("doelvlak projecteert op (%.0f,%.0f) inbeeld=%d afstand=%.0f"),
			Screen.X, Screen.Y, bOn ? 1 : 0, FVector::Dist(Loc, T->GetActorLocation()));
	}
	UE_LOG(LogEclipse, Display,
		TEXT("[GRONDPROEF ZICHTPUNT %s] camera op (%.0f,%.0f,%.0f) kijkt (pitch=%.0f yaw=%.0f); viewtarget=%s; %s"),
		Label, Loc.X, Loc.Y, Loc.Z, Rot.Pitch, Rot.Yaw,
		View != nullptr ? *View->GetName() : TEXT("?"), *Doel);
}

void BuildTopViewTrials(FGroundProof& S, UWorld& World)
{
	AActor* TargetActor = S.Target.Get();
	APlayerController* PC = S.PC.Get();
	if (TargetActor == nullptr || PC == nullptr)
	{
		return;
	}
	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector TargetScale = TargetActor->GetActorScale3D();
	UMaterialInterface* OwnMaterial = nullptr;
	if (const AStaticMeshActor* AsMesh = Cast<AStaticMeshActor>(TargetActor))
	{
		if (const UStaticMeshComponent* Comp = AsMesh->GetStaticMeshComponent())
		{
			OwnMaterial = Comp->GetMaterial(0);
		}
	}
	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	// 1. RUISVLOER.
	{
		FTrial T;
		T.Name = TEXT("1_RUISVLOER");
		T.bNoiseFloor = true;
		T.Note = TEXT("niets verborgen; de ruis van de methode zelf");
		S.Trials.Add(MoveTemp(T));
	}
	// 2. WERELD-KANARIE — het wegdek onder de camera. Dit is de controleproef die
	//    bewijst dat de methode 3D-GEOMETRIE ziet en niet alleen UI-tekst: de
	//    eerste ronde van dit harnas werd juist daardoor misleid.
	{
		FVector Down = TargetLoc;
		TArray<const AActor*> Ignore;
		Ignore.Add(PC->GetPawn());
		Ignore.Add(TargetActor);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseGroundCanary), false);
		Params.AddIgnoredActor(TargetActor);
		if (APawn* Pawn = PC->GetPawn())
		{
			Params.AddIgnoredActor(Pawn);
		}
		FHitResult Hit;
		AActor* Road = nullptr;
		if (World.LineTraceSingleByChannel(Hit, TargetLoc + FVector(0, 0, 50.0f),
			TargetLoc - FVector(0, 0, 400.0f), ECC_Visibility, Params))
		{
			Road = Hit.GetActor();
			Down = Hit.ImpactPoint;
		}
		if (Road != nullptr)
		{
			// HOE DIEP LIGT DE VERDACHTE IN HET WEGDEK? Een spoor dat 1 cm boven het
			// NULVLAK ligt, ligt niet per se boven het OPPERVLAK: het asfalt is een
			// blok met dikte. Zonder dit getal is "hij tekent niet" niet te scheiden
			// van "hij zit in de weg begraven".
			UE_LOG(LogEclipse, Display,
				TEXT("[GRONDPROEF HOOGTE] verdachte op z=%.2f, wegdek-oppervlak eronder op z=%.2f -> steekt %.2f cm uit."),
				TargetLoc.Z, Down.Z, TargetLoc.Z - Down.Z);
			FTrial T;
			T.Name = TEXT("2_KANARIE_WEGDEK");
			T.Note = FString::Printf(TEXT("het wegdek onder het doelvlak (%s) — moet ver boven de ruisvloer uitkomen"),
				*Road->GetName());
			T.Hide.Add(Road);
			T.RoiAnchor = Road;
			T.bHasRoiWorld = true;
			T.RoiWorld = Down;
			S.Trials.Add(MoveTemp(T));
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("[GRONDPROEF] geen wegdek onder het doelvlak gevonden — wereld-kanarie vervalt."));
		}
	}
	// 3. ONMISKENBAAR VLAK op exact de plek en maat van het doelvlak.
	{
		FTrial T;
		T.Name = TEXT("3_CONTROLE_ZICHTBAAR");
		T.Note = TEXT("onmiskenbaar vlak op DEZELFDE plek en maat als het doelvlak");
		T.Setup = [TargetLoc, TargetScale](UWorld& W) -> AActor*
		{
			FString MatName;
			UMaterialInterface* Mat = LoadUnmistakableMaterial(&W, MatName);
			UE_LOG(LogEclipse, Display, TEXT("[GRONDPROEF] controleplaat materiaal=%s"), *MatName);
			return SpawnPlate(W, TargetLoc + FVector(0, 0, 6.0f), TargetScale, Mat, TEXT("Proef_Controle"));
		};
		S.Trials.Add(MoveTemp(T));
	}
	// 4. NEGATIEVE CONTROLE — hetzelfde vlak, ver achter de camera.
	{
		const FVector Behind = CamLoc - CamRot.Vector() * 1200.0f;
		FTrial T;
		T.Name = TEXT("4_CONTROLE_BUITEN_BEELD");
		T.Note = TEXT("hetzelfde onmiskenbare vlak ACHTER de camera; hoort op de ruisvloer te liggen");
		T.Setup = [Behind, TargetScale](UWorld& W) -> AActor*
		{
			FString MatName;
			UMaterialInterface* Mat = LoadUnmistakableMaterial(&W, MatName);
			return SpawnPlate(W, Behind, TargetScale, Mat, TEXT("Proef_Buiten"));
		};
		S.Trials.Add(MoveTemp(T));
	}
	// 5. HET RECEPT — ZELFDE PLEK, ZELFDE MAAT, ZELFDE FRAME als proef 3; alleen
	//    het materiaal verschilt. Dat is het enige paar dat "dit materiaal levert
	//    geen pixels" kan scheiden van "dit exemplaar staat verkeerd of verstopt".
	{
		FTrial T;
		T.Name = TEXT("5_RECEPT_EIGEN_MATERIAAL");
		T.Note = TEXT("verse plaat op DEZELFDE plek als proef 3, met HET EIGEN materiaal van het grondvlak");
		T.Setup = [TargetLoc, TargetScale, OwnMaterial](UWorld& W) -> AActor*
		{
			return SpawnPlate(W, TargetLoc + FVector(0, 0, 6.0f), TargetScale,
				OwnMaterial, TEXT("Proef_Recept"));
		};
		S.Trials.Add(MoveTemp(T));
	}
	// 6. DE VERDACHTE — het echte exemplaar uit de wereld.
	{
		FTrial T;
		T.Name = TEXT("6_DOELVLAK");
		T.Note = S.bMarkMode
			? TEXT("het ECHTE inslagspoor van het wapen (9x9 cm)")
			: TEXT("het ECHTE grondvlak van de bouwer");
		T.Hide.Add(S.Target);
		T.RoiAnchor = S.Target;
		S.Trials.Add(MoveTemp(T));
	}
	// 7. DE HELE LAAG.
	{
		FTrial T;
		T.Name = TEXT("7_HELE_LAAG");
		T.Note = S.bMarkMode
			? TEXT("alle levende inslagsporen tegelijk verborgen")
			: TEXT("alle grondvlakken tegelijk verborgen");
		T.Hide = S.bMarkMode ? S.Marks : S.Planes;
		S.Trials.Add(MoveTemp(T));
	}
	// 9-10. DE HOOGTELADDER. Proef 5 zet de verse plaat 6 cm boven de verdachte, en
	//    dat is precies genoeg verschil om twee heel andere diagnoses te verwarren:
	//    "dit exemplaar tekent niet" en "dit exemplaar ligt begraven in het asfalt".
	//    Deze twee zetten dezelfde plaat met hetzelfde materiaal op EXACT de hoogte
	//    van de verdachte en 2 cm erboven. Waar de reeks omslaat, staat het antwoord.
	for (const float Lift : { 0.0f, 2.0f })
	{
		FTrial T;
		T.Name = FString::Printf(TEXT("9_RECEPT_OP_PLUS%.0fCM"), Lift);
		T.Note = FString::Printf(
			TEXT("zelfde plaat en materiaal als proef 5, maar %.0f cm boven de verdachte in plaats van 6"), Lift);
		T.Setup = [TargetLoc, TargetScale, OwnMaterial, Lift](UWorld& W) -> AActor*
		{
			return SpawnPlate(W, TargetLoc + FVector(0, 0, Lift), TargetScale,
				OwnMaterial, TEXT("Proef_Ladder"));
		};
		S.Trials.Add(MoveTemp(T));
	}
	// 8. DE ANDERE LAAG erbij, zodat één ronde beide vragen beantwoordt.
	{
		FTrial T;
		T.Name = TEXT("8_ANDERE_LAAG");
		T.Note = S.bMarkMode
			? TEXT("alle grondvlakken van de bouwer tegelijk verborgen")
			: TEXT("alle levende inslagsporen tegelijk verborgen");
		T.Hide = S.bMarkMode ? S.Planes : S.Marks;
		S.Trials.Add(MoveTemp(T));
	}
}

bool Tick(TSharedPtr<FGroundProof> State, float /*Delta*/)
{
	FGroundProof& S = *State;
	UWorld* World = S.World.Get();
	if (World == nullptr)
	{
		return false;
	}

	switch (S.Stage)
	{
	case 0:
	{
		// Ruim bezinken: shaders, streaming en belichting moeten klaar zijn. De
		// stiltepoort hierna controleert dat ook echt in plaats van het te hopen.
		if (FPlatformTime::Seconds() - S.ArmedAt < 12.0)
		{
			return true;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC == nullptr)
		{
			return true;
		}
		S.PC = PC;

		// EERST SCHIETEN, want zonder treffers is er geen spoor om te meten. Twaalf
		// graden omlaag, exact de richting die de opnameronde gebruikt: recht vooruit
		// loopt elk schot zijn bereik uit over een lege weg en raakt niets.
		// Het wapen bewaakt zijn eigen vuurinterval, dus dit levert een handvol
		// schoten op en geen veertig.
		if (S.bMarkMode && S.ShotTicks < 45)
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UEclipseHitscanWeaponComponent* Weapon = Pawn->FindComponentByClass<UEclipseHitscanWeaponComponent>())
				{
					const FVector Origin = Pawn->GetPawnViewLocation();
					const FRotator AimedDown = PC->GetControlRotation() + FRotator(-12.0f, 0.0f, 0.0f);
					Weapon->Fire(Origin, AimedDown.Vector(), TEXT("GrondProef"));
				}
			}
			++S.ShotTicks;
			return true;
		}

		// Ruis uit de methode halen: temporele AA en bewegingsonscherpte maken twee
		// opeenvolgende frames per definitie verschillend, en dan meet de
		// verschilmeting het rooster in plaats van het vlak.
		PC->ConsoleCommand(TEXT("r.AntiAliasingMethod 0"));
		PC->ConsoleCommand(TEXT("r.MotionBlurQuality 0"));
		PC->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
		PC->ConsoleCommand(TEXT("Eclipse.Guide.Overlay 0"));
		UGameplayStatics::SetGamePaused(World, true);

		CollectPlanes(*World, S.Planes);
		CollectMarks(*World, S.Marks);
		UE_LOG(LogEclipse, Display, TEXT("[GRONDPROEF SPOREN] %d levende inslagsporen na %d vuurticks."),
			S.Marks.Num(), S.ShotTicks);

		FVector CamLoc = FVector::ZeroVector;
		FRotator CamRot = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		int32 Blob = 0, Pool = 0;
		for (const TWeakObjectPtr<AActor>& Weak : S.Planes)
		{
			if (const AActor* Actor = Weak.Get())
			{
				Blob += Actor->Tags.Contains(TEXT("Deco_Blob")) ? 1 : 0;
				Pool += Actor->Tags.Contains(TEXT("Deco_LampPool")) ? 1 : 0;
			}
		}
		UE_LOG(LogEclipse, Display,
			TEXT("[GRONDPROEF INVENTARIS] %d grondvlakken (%d Deco_Blob, %d Deco_LampPool). Camera op (%.0f,%.0f,%.0f)."),
			S.Planes.Num(), Blob, Pool, CamLoc.X, CamLoc.Y, CamLoc.Z);

		// VRIJ ZICHT VANUIT DE SPELER (meting A, toegepast op de grondvlakken).
		TArray<const AActor*> Ignore;
		Ignore.Add(PC->GetPawn());
		int32 Vrij = 0;
		int32 InBeeldEnVrij = 0;
		FString Voorbeelden;
		for (const TWeakObjectPtr<AActor>& Weak : S.Planes)
		{
			const AActor* Actor = Weak.Get();
			if (Actor == nullptr)
			{
				continue;
			}
			FString Blocker;
			float BlockDist = -1.0f;
			const bool bFree = TraceFreeSight(*World, CamLoc, *Actor, Ignore, Blocker, BlockDist);
			FVector2D Screen = FVector2D::ZeroVector;
			const bool bOnScreen = PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), Screen);
			const FVector ToPlane = Actor->GetActorLocation() - CamLoc;
			const bool bInFront = FVector::DotProduct(ToPlane.GetSafeNormal(), CamRot.Vector()) > 0.0f;
			Vrij += bFree ? 1 : 0;
			if (bFree && bOnScreen && bInFront)
			{
				++InBeeldEnVrij;
				if (Voorbeelden.Len() < 500)
				{
					Voorbeelden += FString::Printf(TEXT(" [%s %s scherm=(%.0f,%.0f) afstand=%.0f schaal=%.1fx%.1f]"),
						*Actor->GetName(), Actor->Tags.Num() > 0 ? *Actor->Tags[0].ToString() : TEXT("-"),
						Screen.X, Screen.Y, ToPlane.Size(),
						Actor->GetActorScale3D().X, Actor->GetActorScale3D().Y);
				}
			}
		}
		UE_LOG(LogEclipse, Display,
			TEXT("[GRONDPROEF ZICHTLIJN-SPELER] %d van %d vlakken hebben vrij zicht; %d daarvan staan OOK op het scherm en voor de camera.%s"),
			Vrij, S.Planes.Num(), InBeeldEnVrij, *Voorbeelden);

		LogViewPoint(S, TEXT("SPELER"));
		S.Stage = 1;
		return true;
	}

	case 1:
		if (QuietGate(S, TEXT("SPELER")))
		{
			// KIES HET DOELVLAK NU AL, zodat het spelerzicht al een venster heeft:
			// het grootste vlak met vrij zicht dat ook op het scherm staat.
			APlayerController* PC = S.PC.Get();
			FVector CamLoc = FVector::ZeroVector;
			FRotator CamRot = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			TArray<const AActor*> Ignore;
			Ignore.Add(PC->GetPawn());
			AActor* Best = nullptr;
			float BestScore = -1.0f;
			const TArray<TWeakObjectPtr<AActor>>& Kandidaten = S.bMarkMode ? S.Marks : S.Planes;
			for (const TWeakObjectPtr<AActor>& Weak : Kandidaten)
			{
				AActor* Actor = Weak.Get();
				if (Actor == nullptr)
				{
					continue;
				}
				FString Blocker;
				float BlockDist = -1.0f;
				if (!TraceFreeSight(*World, CamLoc, *Actor, Ignore, Blocker, BlockDist))
				{
					continue;
				}
				FVector2D Screen = FVector2D::ZeroVector;
				if (!PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), Screen))
				{
					continue;
				}
				const FVector ToPlane = Actor->GetActorLocation() - CamLoc;
				if (FVector::DotProduct(ToPlane.GetSafeNormal(), CamRot.Vector()) <= 0.0f)
				{
					continue;
				}
				// Groot en dichtbij wint: dat levert de meeste pixels en dus de
				// scherpste meting. Schaal 1.0 = 100 cm.
				const FVector Scale = Actor->GetActorScale3D();
				const float Score = static_cast<float>(Scale.X * Scale.Y) / FMath::Max(1.0f, static_cast<float>(ToPlane.Size()));
				if (Score > BestScore)
				{
					BestScore = Score;
					Best = Actor;
				}
			}
			S.Target = Best;
			if (Best != nullptr)
			{
				UE_LOG(LogEclipse, Display,
					TEXT("[GRONDPROEF DOELVLAK] %s tag=%s op (%.0f,%.0f,%.0f) schaal=(%.2f,%.2f,%.3f), %.0f cm van de speler, vrij zicht."),
					*Best->GetName(), Best->Tags.Num() > 0 ? *Best->Tags[0].ToString() : TEXT("-"),
					Best->GetActorLocation().X, Best->GetActorLocation().Y, Best->GetActorLocation().Z,
					Best->GetActorScale3D().X, Best->GetActorScale3D().Y, Best->GetActorScale3D().Z,
					FVector::Dist(CamLoc, Best->GetActorLocation()));
			}
			else
			{
				UE_LOG(LogEclipse, Warning,
					TEXT("[GRONDPROEF DOELVLAK] geen enkel grondvlak staat vrij en in beeld vanaf de speler."));
			}

			S.ViewName = TEXT("SPELER");
			S.Trials.Reset();
			S.TrialIndex = 0;
			S.Sub = 0;
			BuildTopViewTrials(S, *World);
			S.Stage = 2;
		}
		return true;

	case 2:
		if (RunTrials(S))
		{
			S.Stage = 3;
		}
		return true;

	case 3:
	{
		// EEN CAMERA DIE RECHT OP HET VLAK KIJKT. En hij wordt gezet met het spel
		// LOSGELATEN: de eerste ronde zette de view target terwijl het spel op
		// pauze stond, de cameramanager tickte niet, en alle "BOVEN"-proeven
		// draaiden in werkelijkheid nog vanuit het spelerzicht. Een gezette
		// view target is geen bewijs van een verzette camera — daarom staat er
		// hierna een ZICHTPUNT-regel die zegt waar hij echt staat.
		APlayerController* PC = S.PC.Get();
		AActor* Best = S.Target.Get();
		if (PC == nullptr || Best == nullptr)
		{
			S.Stage = 7;
			return true;
		}
		// HOOGTE UIT DE MAAT VAN DE VERDACHTE. Een vaste 700 cm is prima boven een
		// lichtplek van 7 m, maar zet een inslagspoor van 9 cm op acht pixels — en
		// dan meet je opnieuw niet wat je denkt te meten. Acht keer de breedte laat
		// het onderwerp ongeveer een achtste van het beeld vullen.
		const float Breedte = 100.0f * FMath::Max(Best->GetActorScale3D().X, Best->GetActorScale3D().Y);
		S.CamHeight = FMath::Clamp(Breedte * 8.0f, 60.0f, 700.0f);
		const FVector CamLoc = Best->GetActorLocation() + FVector(0, 0, S.CamHeight);
		ACameraActor* Cam = World->SpawnActor<ACameraActor>(CamLoc, FRotator(-90.0f, 0.0f, 0.0f));
		if (Cam == nullptr)
		{
			S.Stage = 7;
			return true;
		}
		S.ProofCam = Cam;
		UGameplayStatics::SetGamePaused(World, false);
		PC->SetViewTarget(Cam);
		S.Wait = SettleFrames * 3;
		S.Stage = 4;
		return true;
	}

	case 4:
	{
		if (--S.Wait > 0)
		{
			return true;
		}
		UGameplayStatics::SetGamePaused(World, true);
		LogViewPoint(S, TEXT("BOVEN"));

		// Verifieer dat de camera ECHT verzet is; anders is de bovenproef een
		// tweede spelerproef met een misleidende naam.
		APlayerController* PC = S.PC.Get();
		AActor* Best = S.Target.Get();
		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(Loc, Rot);
		const float Afstand = Best != nullptr
			? static_cast<float>(FVector::Dist(Loc, Best->GetActorLocation() + FVector(0, 0, S.CamHeight)))
			: 1e9f;
		if (Afstand > 50.0f)
		{
			UE_LOG(LogEclipse, Error,
				TEXT("[GRONDPROEF] de camera staat %.0f cm van waar hij hoort — de bovenproef zou het spelerzicht opnieuw meten en wordt overgeslagen."),
				Afstand);
			S.Stage = 7;
			return true;
		}
		S.Stage = 5;
		return true;
	}

	case 5:
		if (QuietGate(S, TEXT("BOVEN")))
		{
			S.ViewName = TEXT("BOVEN");
			S.Trials.Reset();
			S.TrialIndex = 0;
			S.Sub = 0;
			BuildTopViewTrials(S, *World);
			S.Stage = 6;
		}
		return true;

	case 6:
		if (RunTrials(S))
		{
			S.Stage = 7;
		}
		return true;

	default:
	{
		UE_LOG(LogEclipse, Display, TEXT("[GRONDPROEF SAMENVATTING] %d proeven gedraaid:"), S.Done.Num());
		for (const FTrial& T : S.Done)
		{
			UE_LOG(LogEclipse, Display,
				TEXT("[GRONDPROEF SAMENVATTING]   %-26s heelframe=%lld venster=%lld sterkste=%d"),
				*T.Name, T.Result.Changed, T.Result.ChangedInRoi, T.Result.Strongest);
		}
		UGameplayStatics::SetGamePaused(World, false);
		UE_LOG(LogEclipse, Display, TEXT("[GRONDPROEF] klaar — beelden in Saved/Screenshots/GRONDPROEF."));
		FPlatformMisc::RequestExit(false);
		return false;
	}
	}
}

} // namespace

void ArmGroundProof(UWorld& World)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("EclipseGrondProef")))
	{
		return;
	}

	TSharedPtr<FGroundProof> State = MakeShared<FGroundProof>();
	State->World = &World;
	State->ArmedAt = FPlatformTime::Seconds();
	// Strifind en niet FParse::Param. GEMETEN, niet aangenomen: met
	// `-EclipseGrondProef -EclipseSpoorProef` naast elkaar op de regel gaf
	// FParse::Param op de eerste WEL aan en op de tweede NIET, en de proef draaide
	// vrolijk door met nul sporen — een lege verzameling verbergen verandert nul
	// pixels, en dat had zo als "het spoor rendert niet" de geschiedenis in gekund.
	// Een vlag die stil op false blijft staan is precies het soort meetfout waar dit
	// dossier al twee keer op is gestrand, dus de stand gaat het log in.
	State->bMarkMode = FCString::Strifind(FCommandLine::Get(), TEXT("EclipseSpoorProef")) != nullptr;
	State->Handle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([State](float Delta) { return Tick(State, Delta); }), 0.0f);

	UE_LOG(LogEclipse, Display,
		TEXT("[GRONDPROEF] gewapend, spoormodus=%d. Dit is een MEETharnas voor DEBUG_DISCIPLINE 4.3 en repareert niets."),
		State->bMarkMode ? 1 : 0);
}

} // namespace EclipseRenderProof
