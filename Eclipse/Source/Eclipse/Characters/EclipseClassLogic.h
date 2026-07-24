#pragma once

#include "CoreMinimal.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Strategy/EclipseCampaignTypes.h"

/**
 * Class resolution + stabilize-window math as pure logic (GDD 14.3.2,
 * SPEC-P2-01). No engine actor headers: the subsystems load tables and gather
 * world facts; this file only computes — headless-testable forever.
 *
 * Design law (GDD 12.3): a class is data over the one shared body. The kit
 * below is everything a class may change; the movement/health/order contract
 * is deliberately absent from it.
 */
namespace EclipseClassLogic
{
	/** A soldier's resolved class kit (plain data; the wrapper applies it to spawned bodies). */
	struct FEclipseResolvedClassKit
	{
		/** Resolved class row id; NAME_None = classless Recruit (GDD 4.2.3 default). */
		FName ClassId;

		FText DisplayName;

		/** DT_Weapons row; NAME_None = platform default. */
		FName WeaponRow;

		/** DT_BodyDefs row; NAME_None = shared squad body pool. */
		FName BodyDefOverride;

		FGameplayTag SignatureVerb;

		/** 0 = this soldier cannot stabilize (data decides, never an is-Medic branch). */
		float StabilizeWindowSeconds = 0.0f;

		float KillzoneRangeCm = 0.0f;

		FName BarkSet;

		/** Per-class order modulation (GDD 9.5) — all zero/false for the classless kit. */
		float OrderPushDistanceCm = 0.0f;
		float CoverLaneBias = 0.0f;
		bool bAutoTriage = false;
	};

	/**
	 * Resolve the kit for (record, class row). Row may be null — a ClassId
	 * without a row degrades to the classless kit with the id preserved for
	 * display/logs, never a crash (GDD 14.3.5).
	 */
	ECLIPSE_API FEclipseResolvedClassKit ResolveClassKit(const FEclipseSoldierRecord& Record, const FEclipseClassDefRow* ClassRow);

	/**
	 * Is a stabilize attempt at NowSeconds still inside the window that opened
	 * when the soldier went down (GDD 4.2.5)? Window <= 0 means "cannot
	 * stabilize" regardless of timing; the attempt exactly at the window edge
	 * still succeeds (the dramatic save is inclusive by design).
	 */
	ECLIPSE_API bool IsStabilizeWindowOpen(double DownedAtSeconds, double NowSeconds, float WindowSeconds);
}
