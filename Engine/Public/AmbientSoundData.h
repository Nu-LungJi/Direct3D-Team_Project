#pragma once

#include "ISerializable.h"
#include "SerializerInterface.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

struct AMBIENT_SOUND_3D_DATA final : public ISerializable
{
	uint64_t iID{};
	_string sName{};
	_string sSoundPath{};
	_float3 vPosition{};
	_float fMinDistance{ 1.f };
	_float fMaxDistance{ 30.f };
	_float fVolume{ 1.f };
	_float fPitch{ 1.f };
	_float fFadeInDuration{};
	_float fFadeOutDuration{};
	int32_t iPriority{ 128 };
	SOUND_BUS_ID sBusID{ SOUND_MASTER_BUS_ID };
	SOUND_3D_ROLLOFF eRolloff{ SOUND_3D_ROLLOFF::INVERSE };
	SOUND_LOAD_TYPE eLoadType{ SOUND_LOAD_TYPE::SAMPLE };
	_bool bLoop{ true };
	_bool bAutoPlay{ true };
	_bool bEnabled{ true };

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iID, sName, sSoundPath, vPosition,
			fMinDistance, fMaxDistance, fVolume, fPitch,
			fFadeInDuration, fFadeOutDuration, iPriority,
			sBusID, eRolloff, eLoadType, bLoop, bAutoPlay, bEnabled);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iID, sName, sSoundPath, vPosition,
			fMinDistance, fMaxDistance, fVolume, fPitch,
			fFadeInDuration, fFadeOutDuration, iPriority,
			sBusID, eRolloff, eLoadType, bLoop, bAutoPlay, bEnabled);
	}
};

struct AMBIENT_SOUND_2D_DATA final : public ISerializable
{
	uint64_t iID{};
	_string sName{};
	_string sSoundPath{};
	_float fVolume{ 1.f };
	_float fPitch{ 1.f };
	_float fFadeInDuration{};
	_float fFadeOutDuration{};
	int32_t iPriority{ 128 };
	SOUND_BUS_ID sBusID{ SOUND_MASTER_BUS_ID };
	SOUND_LOAD_TYPE eLoadType{ SOUND_LOAD_TYPE::SAMPLE };
	_bool bLoop{ true };
	_bool bAutoPlay{ true };
	_bool bEnabled{ true };

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iID, sName, sSoundPath,
			fVolume, fPitch, fFadeInDuration, fFadeOutDuration,
			iPriority, sBusID, eLoadType,
			bLoop, bAutoPlay, bEnabled);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iID, sName, sSoundPath,
			fVolume, fPitch, fFadeInDuration, fFadeOutDuration,
			iPriority, sBusID, eLoadType,
			bLoop, bAutoPlay, bEnabled);
	}
};

NS_END
