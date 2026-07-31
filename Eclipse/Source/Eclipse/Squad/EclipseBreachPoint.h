#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EclipseBreachPoint.generated.h"

/**
 * Een geauthord breekpunt: de deur of muur waar een Breach-order op slaat
 * (GDD 8.4 "Breach — Door/wall — Synchronized entry"; SPEC-P2-02 Stage B).
 *
 * Waarom een ACTOR en geen zoekactie op geometrie: "waar kun je naar binnen"
 * is een ontwerpbeslissing en geen meetbare eigenschap van een muur. Een
 * automatische deurdetector zou in een graybox precies zo vaak op een raamkozijn
 * of een pilaar aanslaan, en dan breekt de squad ergens in waar de missie niets
 * heeft. Een geplaatste actor zegt: HIER is een ingang.
 *
 * Afwezigheid is de normale toestand van een missie zonder deuren, en die
 * degradeert (14.3.5): geen punt binnen bereik = de order weigert met
 * `NoBreachPoint` en de soldaat zegt waarom. Geen crash, geen stilte, en
 * niets dat de missie hoeft te weten.
 *
 * De ORIENTATIE draagt de betekenis: +X (de voorkant van de actor) wijst naar
 * BINNEN. Stapelen gebeurt aan de buitenkant, de doorgang ligt erachter.
 */
UCLASS()
class ECLIPSE_API AEclipseBreachPoint : public AActor
{
	GENERATED_BODY()

public:
	AEclipseBreachPoint();

	/**
	 * Hoe ver vóór de deur de squad zich opstelt. Buiten, dus tegen de
	 * kijkrichting in: wie op de drempel stapelt, staat al in de vuurlijn.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Breach", meta = (ClampMin = 50.0))
	float StackDistanceCm = 220.0f;

	/** Zijdelingse afstand tussen twee stapelplekken, zodat vier lichamen niet in één capsule kruipen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Breach", meta = (ClampMin = 30.0))
	float StackSpreadCm = 110.0f;

	/** Hoe ver voorbij de deur "binnen" is — het punt waar iedereen tegelijk heen gaat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Breach", meta = (ClampMin = 50.0))
	float EntryDepthCm = 350.0f;

	/**
	 * Waar soldaat SlotIndex van SlotCount gaat staan wachten.
	 *
	 * De rij staat naast de deur en niet ervoor, en de spreiding is symmetrisch
	 * rond de as: met twee man staat er één links en één rechts, met vier man
	 * twee aan elke kant. Eén man staat precies naast het kozijn.
	 */
	FVector GetStackLocation(int32 SlotIndex, int32 SlotCount) const;

	/** Het punt binnen waar de gesynchroniseerde entry naartoe gaat. */
	FVector GetEntryLocation() const;

	/**
	 * Het dichtstbijzijnde breekpunt bij een plek, binnen MaxRangeCm — of null.
	 *
	 * Hier en niet in de controller, want zowel het FEIT (is er een punt in de
	 * buurt?) als de UITVOERING (naar welk punt dan?) moeten dezelfde vraag
	 * stellen. Twee zoekers zouden precies één keer een ander punt kiezen, en dan
	 * accepteert de order op punt A terwijl de soldaat naar punt B loopt.
	 */
	static AEclipseBreachPoint* FindNearest(const UWorld* World, const FVector& NearLocation, float MaxRangeCm);
};
