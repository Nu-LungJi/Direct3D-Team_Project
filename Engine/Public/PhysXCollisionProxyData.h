#pragma once
#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr const char* PX_COLLISION_SAVE_ROOT = "./Resources/json/Collisions/";

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

struct PX_COLLISION_PROXY_BOX final : public ISerializable
{
	uint64_t iID{};
	std::string sName{};
	std::string sGroup{ "DEFAULT" };
	E::_float3 vPosition{};
	E::_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	E::_float3 vSize{ 1.f, 0.2f, 1.f };
	_bool bEnabled{ true };

	void Serialize(E::ISerializer& serializer) const override
	{
		serializer.Write("ID", iID);
		serializer.Write("Name", sName);
		serializer.Write("Group", sGroup);
		serializer.Write("Position", vPosition);
		serializer.Write("Rotation", vRotation);
		serializer.Write("Size", vSize);
		serializer.Write("Enabled", bEnabled);
	}

	void Deserialize(E::IDeserializer& deserializer) override
	{
		deserializer.Read("ID", iID);
		deserializer.Read("Name", sName);
		deserializer.Read("Group", sGroup);
		deserializer.Read("Position", vPosition);
		deserializer.Read("Rotation", vRotation);
		deserializer.Read("Size", vSize);
		deserializer.Read("Enabled", bEnabled);
	}
};

struct PX_COLLISION_PROXY_FILE final : public ISerializable
{
	uint32_t iVersion{ 1 };
	std::vector<PX_COLLISION_PROXY_BOX> boxes{};

	void Serialize(E::ISerializer& serializer) const override
	{
		serializer.Write("Version", iVersion);
		serializer.Write("Boxes", boxes);
	}

	void Deserialize(E::IDeserializer& deserializer) override
	{
		deserializer.Read("Version", iVersion);
		deserializer.Read("Boxes", boxes);
	}
};

NS_END
