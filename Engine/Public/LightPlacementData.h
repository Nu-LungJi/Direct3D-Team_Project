#pragma once

#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr const char* LIGHT_PLACEMENT_SAVE_ROOT =
	"./Resources/json/Lights/";

inline std::string MakeLightPlacementGroupName(std::string fileName)
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

	return fileName;
}

inline std::string MakeLightPlacementFilePath(std::string fileName)
{
	return std::string{ LIGHT_PLACEMENT_SAVE_ROOT } +
		MakeLightPlacementGroupName(std::move(fileName)) +
		".json";
}

struct LIGHT_PLACEMENT_ENTRY final : public ISerializable
{
	std::string sName{};
	std::string sAlias{};
	LIGHT_TYPE eType{ LIGHT_TYPE::POINT };
	_float3 vPosition{};
	_float3 vDirection{ 0.f, -1.f, 0.f };
	_float3 vColor{ 1.f, 1.f, 1.f };
	_float fIntensity{ 10.f };
	_float fRange{ 10.f };
	_float fInnerAttenuation{ 20.f };
	_float fOuterAttenuation{ 30.f };
	_bool bActive{ true };
	_bool bCastShadow{ true };

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, sName, sAlias, eType, vPosition, vDirection,
			vColor, fIntensity, fRange, fInnerAttenuation,
			fOuterAttenuation, bActive, bCastShadow);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, sName, sAlias, eType, vPosition, vDirection,
			vColor, fIntensity, fRange, fInnerAttenuation,
			fOuterAttenuation, bActive, bCastShadow);
	}
};

struct LIGHT_PLACEMENT_FILE final : public ISerializable
{
	uint32_t iVersion{ 1 };
	std::vector<LIGHT_PLACEMENT_ENTRY> lights{};

	void Serialize(ISerializer& serializer) const override
	{
		WRITE_ALL(serializer, iVersion, lights);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		READ_ALL(deserializer, iVersion, lights);
	}
};

NS_END
