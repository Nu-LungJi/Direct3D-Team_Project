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
	struct ACCIO_ACTIVITY_SET_DESC
	{
		// [LSY] 모든 세트 구성요소는 이 기준점과 Yaw를 공유한다.
		_float3 vOrigin{ 27.f, 5.f, 100.f };
		_float fYawDegrees{};
		// [LSY] 실제 경기장과 떨어져 있는 개별 파츠 확인용 샘플 배치 여부다.
		_bool bSpawnDetachedPartSamples{ true };
	};

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
	HRESULT InitializePhysicsDoorTest();
	HRESULT InitializeTombBossBulletTest(CHandle hPlayer);
	HRESULT SpawnConfringoBulletTest();
	HRESULT InitializeOilBarrelPool();
	HRESULT InitializeAccioActivityTest();
	HRESULT SpawnAccioActivitySet(const ACCIO_ACTIVITY_SET_DESC& desc);
	void UpdateAccioActivityTestGUI();
	void ResetAccioBalls();
	_bool PushSelectedAccioBallTowardPlayer();
	void ApplyAccioBallMotionTuning();
	_float3 MakePropBarrelSpawnPosition() const;

	HRESULT SpawnMonster(const std::optional<CHandle>& hPlayer);

	HRESULT SpawnStaticCollision();
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
	CHandle m_hPhysicsDoor{};
	_float m_fPhysicsDoorTestTorque{ 800.f };
	_float3 m_vPropBarrelSpawnPosition{ 20.f, 5.f, 20.f };
	CHandle m_hAccioActivityBase{};
	CHandle m_hAccioActivityNpc{};
	std::array<CHandle, 6> m_hAccioBalls{};
	_float3 m_vAccioActivitySetOrigin{ 27.f, 5.f, 100.f };
	_float m_fAccioActivitySetYawDegrees{ 18.f };
	int32_t m_iSelectedAccioBall{};
	_float m_fAccioBallPushTorque{ 20.f };
	_float m_fAccioBallMaxRollAngularSpeed{ 6.f };
	_float m_fAccioBallMaxPullAcceleration{ 32.f };
	_float m_fAccioBallMaxPullLinearSpeed{ 20.f };
	_float m_fAccioBallPullSlowRadius{ 4.f };
	_float m_fAccioBallMass{ 5.f };
	_float m_fAccioBallLinearDamping{ 0.6f };
	_float m_fAccioBallAngularDamping{ 0.7f };
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

