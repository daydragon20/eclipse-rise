#include "Squad/EclipseBreachPoint.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AEclipseBreachPoint::AEclipseBreachPoint()
{
	PrimaryActorTick.bCanEverTick = false; // een markering denkt nergens over na (12.4)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AEclipseBreachPoint::GetStackLocation(int32 SlotIndex, int32 SlotCount) const
{
	const int32 Count = FMath::Max(1, SlotCount);
	const int32 Index = FMath::Clamp(SlotIndex, 0, Count - 1);

	// Symmetrisch rond de as: bij vier plekken staan de offsets op -1.5, -0.5,
	// +0.5, +1.5 keer de spreiding. Bij één plek is de offset 0 en staat hij
	// precies naast het kozijn.
	const float Centered = static_cast<float>(Index) - (static_cast<float>(Count) - 1.0f) * 0.5f;

	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	return GetActorLocation() - Forward * StackDistanceCm + Right * (Centered * StackSpreadCm);
}

FVector AEclipseBreachPoint::GetEntryLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * EntryDepthCm;
}

AEclipseBreachPoint* AEclipseBreachPoint::FindNearest(const UWorld* World, const FVector& NearLocation, float MaxRangeCm)
{
	if (World == nullptr || MaxRangeCm <= 0.0f)
	{
		return nullptr;
	}

	AEclipseBreachPoint* Nearest = nullptr;
	float NearestDistanceSquared = FMath::Square(MaxRangeCm);
	for (TActorIterator<AEclipseBreachPoint> It(const_cast<UWorld*>(World)); It; ++It)
	{
		AEclipseBreachPoint* Candidate = *It;
		if (Candidate == nullptr)
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), NearLocation);
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Candidate;
		}
	}
	return Nearest;
}
