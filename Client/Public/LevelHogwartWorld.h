#pragma once

#include "Client_Defines.h"
#include "Level.h"
NS_BEGIN(Engine)
class CTerrain;
NS_END
NS_BEGIN(Client)

class CLevelHogwartWorld final : public CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelHogwartWorld, CLevel)

private:
	explicit CLevelHogwartWorld();
	~CLevelHogwartWorld() override = default;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;

private:
	std::optional<CHandle> SpawnPlayer();
	HRESULT SpawnPlayerCape(CHandle hPlayer);
	HRESULT SpawnTerrain(std::optional<CHandle> hPlayer);
	HRESULT SpawnNaviMesh(class E::CTerrain* pTerrain);
	HRESULT SpawnFlyCamera();
	HRESULT SpawnUICamera();
	HRESULT SpawnPlayerCamera(CHandle hPlayer);
	HRESULT SpawnSkyBox();
	HRESULT SpawnWater();
	HRESULT SpawnMonster(std::optional<CHandle> hPlayer);
	HRESULT SpawnStaticCollision();
	HRESULT SpawnCoinCollision();
	HRESULT SpawnLightPlacement();

	/*----------- 광윤 추가 -----------*/
	HRESULT Initialize_VolumetricFog();
	HRESULT Initialize_EnviromentLight();
	HRESULT Initialize_LoopEffect();
	/*---------------------------------*/
	HRESULT SpanwAnimal();
	HRESULT SpawnNpcPlacements(CHandle hPlayer, const _string& Path);
public:
	static UPtr<CLevelHogwartWorld> Create();

private:
	_bool m_bCreatePlayScreenUI = false;

private:
	void Free() override;
};

NS_END
