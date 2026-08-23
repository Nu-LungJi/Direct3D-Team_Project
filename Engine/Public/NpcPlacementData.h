#pragma once

#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

enum class NPC_RUNTIME_TYPE : uint8_t
{
	CPU_ACTOR_COMBAT,
	CPU_ACTOR_AMBIENT,
	GPU_CROWD_AMBIENT,
	END
};

struct NPC_PLACEMENT_DESC final : public ISerializable
{
	uint64_t iPlacementId{};
	_string sPrototypeGroupTag{};
	_string sPrototypeTag{};
	_string sLayerTag{ "NPC" };
	_string sModelGroupTag{};
	_string sModelResourceTag{};
	_string sBehaviorMajorTag{};
	_string sBehaviorMinorTag{};
	_float3 vPosition{};
	_float3 vRotation{};
	_float3 vScale{ 1.f, 1.f, 1.f };
	_float3 vPatrolStartPosition{};
	_float3 vPatrolEndPosition{};
	NPC_RUNTIME_TYPE eRuntimeType{ NPC_RUNTIME_TYPE::CPU_ACTOR_AMBIENT };
	_bool bCastShadow{ true };
	_float fVisibleDistance{ 150.f };
	_float fAnimationUpdateDistance{ 80.f };
	_float fAIUpdateDistance{ 60.f };
	uint32_t iCrowdCount{ 1 };
	_float fCrowdRadius{ 1.f };
	uint32_t iRandomSeed{};

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iPlacementId, sPrototypeGroupTag, sPrototypeTag, sLayerTag,
			sModelGroupTag, sModelResourceTag,
			sBehaviorMajorTag, sBehaviorMinorTag, vPosition, vRotation, vScale,
			vPatrolStartPosition, vPatrolEndPosition,
			eRuntimeType, bCastShadow, fVisibleDistance, fAnimationUpdateDistance,
			fAIUpdateDistance, iCrowdCount, fCrowdRadius, iRandomSeed);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iPlacementId, sPrototypeGroupTag, sPrototypeTag, sLayerTag,
			sModelGroupTag, sModelResourceTag,
			sBehaviorMajorTag, sBehaviorMinorTag, vPosition, vRotation, vScale,
			vPatrolStartPosition, vPatrolEndPosition,
			eRuntimeType, bCastShadow, fVisibleDistance, fAnimationUpdateDistance,
			fAIUpdateDistance, iCrowdCount, fCrowdRadius, iRandomSeed);
	}
};

struct NPC_PLACEMENT_FILE final : public ISerializable
{
	uint32_t iVersion{ 1 };
	std::vector<NPC_PLACEMENT_DESC> Placements{};
	        
	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iVersion, Placements);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iVersion, Placements);
	}
};

NS_END
