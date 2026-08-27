#pragma once

#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

struct ANIMATED_OBJECT_PLACEMENT_DESC final : public ISerializable
{
	uint64_t iPlacementId{};
	_string sPrototypeGroupTag{};
	_string sPrototypeTag{};
	_string sLayerTag{ "AnimatedObject" };
	_string sModelGroupTag{};
	_string sModelResourceTag{};
	_string sAnimationName{};
	_float3 vPosition{};
	_float3 vRotation{};
	_float3 vScale{ 1.f, 1.f, 1.f };
	_bool bAutoPlay{ true };
	_bool bLoop{ true };
	_bool bCastShadow{ true };
	_float fAnimationSpeed{ 1.f };
	_float fStartRatio{};
	_float fVisibleDistance{ 150.f };

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iPlacementId, sPrototypeGroupTag, sPrototypeTag, sLayerTag,
			sModelGroupTag, sModelResourceTag, sAnimationName,
			vPosition, vRotation, vScale, bAutoPlay, bLoop, bCastShadow,
			fAnimationSpeed, fStartRatio, fVisibleDistance);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iPlacementId, sPrototypeGroupTag, sPrototypeTag, sLayerTag,
			sModelGroupTag, sModelResourceTag, sAnimationName,
			vPosition, vRotation, vScale, bAutoPlay, bLoop, bCastShadow,
			fAnimationSpeed, fStartRatio, fVisibleDistance);
	}
};

struct ANIMATED_OBJECT_PLACEMENT_FILE final : public ISerializable
{
	uint32_t iVersion{ 1 };
	std::vector<ANIMATED_OBJECT_PLACEMENT_DESC> Placements{};

	void Serialize(ISerializer& serializer) const override { WRITE_ALL(serializer, iVersion, Placements); }
	void Deserialize(IDeserializer& deserializer) override { READ_ALL(deserializer, iVersion, Placements); }
};

NS_END
