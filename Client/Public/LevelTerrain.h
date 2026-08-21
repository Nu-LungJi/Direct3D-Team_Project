#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)
class CLevelTerrain final : public Engine::CLevel
{

private:
	explicit CLevelTerrain();
	~CLevelTerrain() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;

public:
	void Picking();
private:
	void		Resources();
	void		Objects();
	void		BeHaviors();

	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();
	HRESULT InitializeJointTests(
		CHandle hPlayer,
		const std::array<CHandle, 6>& hOilBarrels);
	HRESULT InitializeCamerasAndLighting(
		const std::optional<CHandle>& hPlayer);
	HRESULT InitializePathPlaybackTests();
	HRESULT InitializeTombBossBulletTest(CHandle hPlayer);
	HRESULT SpawnConfringoBulletTest();
	HRESULT InitializeOilBarrelPool();
	HRESULT InitializeAccioActivityTest();
	HRESULT SpawnAccioBalls();
	HRESULT SpawnAccioActivityObjects();
	void UpdateAccioActivityTestGUI();
	void ResetAccioBalls();
	void DrawSelectedAccioBallDebug();
	_bool PushSelectedAccioBallTowardPlayer();
	void ApplyAccioBallMotionTuning();

	HRESULT SpawnMonster(const std::optional<CHandle>& hPlayer);
public:
	static Engine::UPtr<CLevelTerrain> Create();

private:
	_float3		m_fPos{};
	_bool		m_bSpawn{ false };
	int32_t  m_iResourceSelect{ 0 }, m_iObjSelect{ 0 };
	const	_string m_strLevelName = { "LEVEL_CREATURE" };
	_string	m_SelectResourceTag{}, m_SelectObjecteTag{}, m_SelectFileName{}, m_SelectFilePath{};

	std::map<_string, _string>		m_BeHaviorJsonList;
	std::vector<CHandle>				m_MedDebrisHandles{};

private:
	_bool m_bCreatePlayScreenUI{ false };
	CHandle m_hPlayer{};
	CHandle m_hPropBarrel{};
	_float3 m_vPropBarrelSpawnPosition{ 5.f, 5.f, 5.f };
	std::array<CHandle, 6> m_hAccioBalls{};
	int32_t m_iSelectedAccioBall{};
	_float m_fAccioBallPushTorque{ 20.f };
	_float m_fAccioBallSelectedMass{ 3.f };
	_float m_fAccioBallIdleMass{ 1.f };
	_float m_fAccioBallLinearDamping{ 0.4f };
	_float m_fAccioBallAngularDamping{ 0.8f };
	_float m_fTombBossBulletSpawnYawDegrees{};
	_float m_fConfringoBulletSpeed{ 35.f };
	_float m_fConfringoBulletLifeTime{ 5.f };
	_float m_fConfringoBulletRadius{ 0.25f };
	_float m_fConfringoBulletCurveAmplitude{ 0.35f };
	_float m_fConfringoBulletCurveFrequency{ 1.75f };
	_float m_fConfringoBulletTrailSpacing{ 0.14f };
	_bool m_bConfringoBulletDebugDraw{ true };
	_float3 m_vOilBarrelPoolSpawnPosition{ 10.f, 100.f, 10.f };

private:
	HRESULT InitializeMyMagicSquareStep();
private:
	void Free() override;
};

NS_END

