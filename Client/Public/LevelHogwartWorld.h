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
	HRESULT SpawnPropBarrelBlock();
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
	HRESULT SpawnPhysicsDoor(
		const _float3& vPosition,
		const _float3& vRotationEulerDegrees,
		const _float3& vScale);
	HRESULT SpawnAccioActivity(
		CHandle hPlayer,
		const _float3& vOrigin,
		_float fYawDegrees,
		_float fUniformScale);
	void DespawnRuntimeObjects(std::vector<CHandle>& Handles);
	void PruneInvalidRuntimeHandles(std::vector<CHandle>& Handles);
	void UpdateRuntimeActivitySpawnShortcut();
	void RequestSummonersCourtSpawn(CHandle hPlayer);
	void UpdateRequestedSummonersCourtSpawn();
	HRESULT SpawnSummonersCourtIfNeeded(CHandle hPlayer);
	void RequestSummonersCourtDespawn();
	void UpdateRequestedSummonersCourtDespawn();
	void ChangeBGM(
		const _string& strSoundPath,
		_float fTargetVolume,
		_float fFadeDuration = 1.f);

	void UpdateDebugWarp();

	/*----------- 광윤 추가 -----------*/
	HRESULT Initialize_VolumetricFog();
	HRESULT Initialize_EnviromentLight();
	HRESULT Initialize_LoopEffect();
	/*---------------------------------*/
	HRESULT SpanwWorldAgent();
	HRESULT SpawnNpcPlacements(CHandle hPlayer, const _string& Path);


public:
	static UPtr<CLevelHogwartWorld> Create();

private:
	_bool m_bCreatePlayScreenUI = false;
	_bool m_bPropBarrelBlockSpawned{};
	_bool m_bQuestCreated = false;

	CHandle m_hDebugPlayer{};
	std::optional<CHandle> m_hPendingSummonersCourtPlayer{};
	_bool m_bSummonersCourtDespawnRequested{};
	std::vector<CHandle> m_AccioActivityHandles{};
	std::vector<CHandle> m_CoinCollisionHandles{};
	SOUND_ID m_iBGM{ INVALID_SOUND_ID };
	_string m_strCurrentBGMPath{};

private:
	void Free() override;
};

NS_END
