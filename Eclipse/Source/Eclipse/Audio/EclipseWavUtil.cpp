#include "Audio/EclipseWavUtil.h"

namespace
{
	void AppendUInt16LE(TArray<uint8>& Out, uint16 Value)
	{
		Out.Add(static_cast<uint8>(Value & 0xFF));
		Out.Add(static_cast<uint8>((Value >> 8) & 0xFF));
	}

	void AppendUInt32LE(TArray<uint8>& Out, uint32 Value)
	{
		Out.Add(static_cast<uint8>(Value & 0xFF));
		Out.Add(static_cast<uint8>((Value >> 8) & 0xFF));
		Out.Add(static_cast<uint8>((Value >> 16) & 0xFF));
		Out.Add(static_cast<uint8>((Value >> 24) & 0xFF));
	}

	void AppendFourCC(TArray<uint8>& Out, const ANSICHAR* FourCC)
	{
		Out.Append(reinterpret_cast<const uint8*>(FourCC), 4);
	}

	uint16 ReadUInt16LE(const TArray<uint8>& Buffer, int32 Offset)
	{
		return static_cast<uint16>(static_cast<uint16>(Buffer[Offset]) | (static_cast<uint16>(Buffer[Offset + 1]) << 8));
	}

	uint32 ReadUInt32LE(const TArray<uint8>& Buffer, int32 Offset)
	{
		return static_cast<uint32>(Buffer[Offset])
			| (static_cast<uint32>(Buffer[Offset + 1]) << 8)
			| (static_cast<uint32>(Buffer[Offset + 2]) << 16)
			| (static_cast<uint32>(Buffer[Offset + 3]) << 24);
	}

	bool IsFourCCAt(const TArray<uint8>& Buffer, int32 Offset, const ANSICHAR* FourCC)
	{
		return FMemory::Memcmp(Buffer.GetData() + Offset, FourCC, 4) == 0;
	}
}

namespace EclipseWavUtil
{
	TArray<uint8> WrapPcm16ToWav(const TArray<uint8>& Pcm, int32 SampleRate, int32 Channels)
	{
		TArray<uint8> Out;
		if (SampleRate <= 0 || Channels <= 0)
		{
			return Out;
		}

		// Half a sample cannot be played; drop a dangling byte instead of writing
		// a header whose sample count lies (see header comment).
		const uint32 DataNumBytes = static_cast<uint32>(Pcm.Num() - (Pcm.Num() % 2));
		const uint32 BytesPerFrame = static_cast<uint32>(Channels) * (PcmBitsPerSample / 8);

		Out.Reserve(WavHeaderNumBytes + static_cast<int32>(DataNumBytes));
		AppendFourCC(Out, "RIFF");
		AppendUInt32LE(Out, 36 + DataNumBytes); // remaining header bytes + payload
		AppendFourCC(Out, "WAVE");
		AppendFourCC(Out, "fmt ");
		AppendUInt32LE(Out, 16); // fmt chunk payload size for plain PCM
		AppendUInt16LE(Out, 1);  // audio format 1 = linear PCM
		AppendUInt16LE(Out, static_cast<uint16>(Channels));
		AppendUInt32LE(Out, static_cast<uint32>(SampleRate));
		AppendUInt32LE(Out, static_cast<uint32>(SampleRate) * BytesPerFrame); // byte rate
		AppendUInt16LE(Out, static_cast<uint16>(BytesPerFrame));              // block align
		AppendUInt16LE(Out, static_cast<uint16>(PcmBitsPerSample));
		AppendFourCC(Out, "data");
		AppendUInt32LE(Out, DataNumBytes);
		Out.Append(Pcm.GetData(), static_cast<int32>(DataNumBytes));
		return Out;
	}

	bool ParseWav(const TArray<uint8>& Wav, FEclipseWavInfo& OutInfo)
	{
		OutInfo = FEclipseWavInfo();
		if (Wav.Num() < WavHeaderNumBytes || !IsFourCCAt(Wav, 0, "RIFF") || !IsFourCCAt(Wav, 8, "WAVE"))
		{
			return false;
		}

		bool bHaveFmt = false;
		bool bHaveData = false;

		// int64 offset math: chunk sizes are untrusted uint32s from disk; int32
		// arithmetic could wrap on a corrupt size and walk out of bounds.
		int64 Offset = 12;
		while (Offset + 8 <= Wav.Num())
		{
			const int32 ChunkStart = static_cast<int32>(Offset);
			const uint32 ChunkSize = ReadUInt32LE(Wav, ChunkStart + 4);

			if (IsFourCCAt(Wav, ChunkStart, "fmt ") && ChunkSize >= 16 && Offset + 8 + 16 <= Wav.Num())
			{
				const uint16 AudioFormat = ReadUInt16LE(Wav, ChunkStart + 8);
				OutInfo.NumChannels = ReadUInt16LE(Wav, ChunkStart + 10);
				OutInfo.SampleRate = static_cast<int32>(ReadUInt32LE(Wav, ChunkStart + 12));
				OutInfo.BitsPerSample = ReadUInt16LE(Wav, ChunkStart + 22);
				// This pipeline only ever writes/plays linear PCM16 (header comment);
				// anything else must read as "not ours", not as a half-parsed file.
				bHaveFmt = AudioFormat == 1 && OutInfo.BitsPerSample == PcmBitsPerSample;
			}
			else if (IsFourCCAt(Wav, ChunkStart, "data"))
			{
				OutInfo.DataOffset = ChunkStart + 8;
				// Clamp to the real buffer: a truncated file must yield playable-or-false, never an overread.
				OutInfo.DataNumBytes = static_cast<int32>(FMath::Min<int64>(ChunkSize, static_cast<int64>(Wav.Num()) - OutInfo.DataOffset));
				bHaveData = true;
			}

			if (bHaveFmt && bHaveData)
			{
				break;
			}
			Offset += 8 + static_cast<int64>(ChunkSize) + (ChunkSize & 1); // RIFF chunks are 2-byte aligned
		}

		return bHaveFmt && bHaveData && OutInfo.SampleRate > 0 && OutInfo.NumChannels > 0 && OutInfo.DataNumBytes >= 0;
	}

	TArrayView<const uint8> GetPcmView(const TArray<uint8>& Wav, const FEclipseWavInfo& Info)
	{
		if (Info.DataOffset < 0 || Info.DataNumBytes < 0 || Info.DataOffset + Info.DataNumBytes > Wav.Num())
		{
			return TArrayView<const uint8>();
		}
		return MakeArrayView(Wav.GetData() + Info.DataOffset, Info.DataNumBytes);
	}
}
