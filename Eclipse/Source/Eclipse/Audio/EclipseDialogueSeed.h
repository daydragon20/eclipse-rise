#pragma once

#include "CoreMinimal.h"
#include "Audio/EclipseDialogueTypes.h"

/**
 * Dialogue-database seed parsing (GDD 16.12). The seed file
 * (Content/Audio/DialogueSeed.json) is the plain-text source the bulk commandlet
 * materialises into UEclipseCharacterVoiceData assets — so the Phase 1 dialogue
 * database is authorable in a diffable text file instead of only through the
 * editor UI, and a fresh clone can bootstrap voice assets headlessly.
 *
 * Pure logic, engine-light (14.3.2): no editor or actor dependency, so the
 * parse rules are provable in the headless test suite without touching assets.
 */

/** One line row parsed from the seed (maps onto FEclipseDialogueLine). */
struct FEclipseVoiceSeedLine
{
	FName LineId;
	EEclipseVoiceEmotion Emotion = EEclipseVoiceEmotion::Neutral;
	FString Text;
};

/** One voice asset to materialise (maps onto UEclipseCharacterVoiceData). */
struct FEclipseVoiceSeedEntry
{
	/** Asset name under /Game/Audio (e.g. DA_Voice_SquadA). */
	FString AssetName;
	FName CharacterId;
	FString DisplayName;
	FString ElevenLabsVoiceId;
	/** Empty = keep the class default model. */
	FString ModelId;
	TArray<FEclipseVoiceSeedLine> Lines;
};

namespace EclipseDialogueSeed
{
	/**
	 * Parse seed JSON into entries. Returns false only for structurally unusable
	 * input (not JSON / no "voices" array). Recoverable data problems — missing
	 * required fields, unknown emotions, duplicate ids — skip or default the
	 * offending row and are reported in OutProblems, never dropped silently
	 * (the never-silent spirit of 9.5 applied to tooling; 14.3.5).
	 */
	ECLIPSE_API bool Parse(const FString& JsonText, TArray<FEclipseVoiceSeedEntry>& OutEntries, TArray<FString>& OutProblems);
}
