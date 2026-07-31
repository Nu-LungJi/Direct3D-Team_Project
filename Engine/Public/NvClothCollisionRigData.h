#pragma once

#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr uint32_t NVCLOTH_COLLISION_RIG_VERSION = 1u;
inline constexpr const char* NVCLOTH_COLLISION_RIG_ROOT =
	"NvClothCollisionRig";
inline constexpr const char* NVCLOTH_COLLISION_RIG_SAVE_ROOT =
	"./Resources/NvCloth/CollisionRigs";

enum class NVCLOTH_COLLISION_SHAPE_TYPE : uint8_t
{
	SPHERE,
	CAPSULE,
	BOX
};

struct NVCLOTH_COLLISION_SHAPE_DESC final :
	public ISerializable
{
	uint64_t iID{};
	std::string sName{ "ClothCollisionShape" };
	std::string sBoneName{};
	NVCLOTH_COLLISION_SHAPE_TYPE eType{
		NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE };

	// Shape pose relative to the selected animation bone's rigid pose.
	_float3 vLocalPosition{};
	_float4 vLocalRotation{ 0.f, 0.f, 0.f, 1.f };

	_float3 vHalfExtents{ 0.2f, 0.2f, 0.2f };
	_float fRadius{ 0.2f };
	// Capsule segment half length. The capsule local axis is +Y.
	_float fHalfHeight{ 0.25f };
	// Per-shape outward inflation. Unlike a global scale, this does not
	// enlarge unrelated collision shapes.
	_float fMargin{};
	_bool bEnabled{ true };

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			iID,
			sName,
			sBoneName,
			eType,
			vLocalPosition,
			vLocalRotation,
			vHalfExtents,
			fRadius,
			fHalfHeight,
			fMargin,
			bEnabled);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			iID,
			sName,
			sBoneName,
			eType,
			vLocalPosition,
			vLocalRotation,
			vHalfExtents,
			fRadius,
			fHalfHeight,
			fMargin,
			bEnabled);
	}
};

struct NVCLOTH_COLLISION_RIG_DESC final :
	public ISerializable
{
	uint32_t iVersion{ NVCLOTH_COLLISION_RIG_VERSION };
	std::string sSkeletonTag{};
	std::vector<NVCLOTH_COLLISION_SHAPE_DESC> Shapes{};

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			iVersion,
			sSkeletonTag,
			Shapes);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			iVersion,
			sSkeletonTag,
			Shapes);
	}
};

NS_END
