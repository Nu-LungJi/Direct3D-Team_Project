#pragma once

#include "GameObject.h"
#include "LightPlacementData.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLightPlacementObject final : public CGameObject
{
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		std::string sLightFileName{};
		std::string sPlacementGroup{};
		const LIGHT_PLACEMENT_FILE* pLightData{};
	};

public:
	DECLARE_DERIVED_TYPE(CLightPlacementObject, CGameObject)

private:
	CLightPlacementObject() = default;
	CLightPlacementObject(
		const CLightPlacementObject&) = default;
	~CLightPlacementObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CLightPlacementObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	HRESULT LoadLightData(
		const DESC& desc,
		LIGHT_PLACEMENT_FILE& outData) const;
	HRESULT SpawnLights(
		const LIGHT_PLACEMENT_FILE& data);
	void RemoveSpawnedLights();

private:
	std::vector<CHandle> m_SpawnedLightHandles{};
	std::string m_sPlacementGroup{};

private:
	void Free() override;
};

NS_END
