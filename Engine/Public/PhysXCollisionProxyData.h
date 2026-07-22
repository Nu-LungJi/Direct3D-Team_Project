#pragma once
#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr const char* PX_COLLISION_SAVE_ROOT = "./Resources/json/Collisions/";
inline constexpr const char* PX_COLLISION_PROXY_PROTOTYPE_GROUP = "COLLISION_PROXY";

inline std::string MakePxCollisionFilePath(std::string fileName)
{
	if (fileName.ends_with(".json"))
		fileName.resize(fileName.size() - 5);
	if (fileName.empty())
		fileName = "Default";

	for (char& ch : fileName)
	{
		switch (ch)
		{
		case '/': case '\\': case ':': case '*': case '?':
		case '"': case '<': case '>': case '|': case '.':
			ch = '_';
			break;
		default:
			break;
		}
	}

	return std::string{ PX_COLLISION_SAVE_ROOT } + fileName + ".json";
}

enum class PX_COLLISION_PROXY_ACTOR_TYPE : uint32_t
{
	STATIC,
	DYNAMIC,
	KINEMATIC
};

enum class PX_COLLISION_PROXY_SHAPE_TYPE : uint32_t
{
	BOX,
	SPHERE,
	CAPSULE,
	CONVEX_MESH,
	TRIANGLE_MESH
};

struct PX_COLLISION_PROXY_SHAPE final : public ISerializable
{
	uint64_t iID{};
	std::string sName{};
	PX_COLLISION_PROXY_SHAPE_TYPE eType{ PX_COLLISION_PROXY_SHAPE_TYPE::BOX };
	_float3 vLocalPosition{};
	_float4 vLocalRotation{ 0.f, 0.f, 0.f, 1.f };
	_float3 vSize{ 1.f, 0.2f, 1.f };
	_float3 vScale{ 1.f, 1.f, 1.f };
	_float fRadius{ 0.5f };
	_float fHalfHeight{ 0.5f };
	std::string sCookedResourcePath{};
	uint32_t iLayer{ PX_DEFAULT_LAYER };
	uint32_t iSimulationMask{ PX_ALL_LAYERS };
	uint32_t iQueryMask{ PX_ALL_LAYERS };
	_bool bTrigger{};
	_bool bSimulationEnabled{ true };
	_bool bQueryEnabled{ true };
	_bool bEnabled{ true };

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iID, sName, eType, vLocalPosition, vLocalRotation,
			vSize, vScale, fRadius, fHalfHeight, sCookedResourcePath,
			iLayer, iSimulationMask, iQueryMask, bTrigger,
			bSimulationEnabled, bQueryEnabled, bEnabled);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iID, sName, eType, vLocalPosition, vLocalRotation,
			vSize, vScale, fRadius, fHalfHeight, sCookedResourcePath,
			iLayer, iSimulationMask, iQueryMask, bTrigger,
			bSimulationEnabled, bQueryEnabled, bEnabled);
	}
};

struct PX_COLLISION_PROXY_ACTOR final : public ISerializable
{
	uint64_t iID{};
	std::string sName{};
	std::string sPrototypeTag{};
	PX_COLLISION_PROXY_ACTOR_TYPE eType{ PX_COLLISION_PROXY_ACTOR_TYPE::STATIC };
	_float3 vPosition{};
	_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	_float fMass{ 1.f };
	_bool bGravity{ true };
	_bool bEnabled{ true };
	std::vector<PX_COLLISION_PROXY_SHAPE> shapes{};

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iID, sName, sPrototypeTag, eType, vPosition, vRotation,
			fMass, bGravity, bEnabled, shapes);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iID, sName, sPrototypeTag, eType, vPosition, vRotation,
			fMass, bGravity, bEnabled, shapes);
	}
};

struct PX_COLLISION_PROXY_FILE final : public ISerializable
{
	uint32_t iVersion{ 3 };
	std::vector<PX_COLLISION_PROXY_ACTOR> actors{};

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iVersion, actors);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iVersion, actors);
	}
};

NS_END
