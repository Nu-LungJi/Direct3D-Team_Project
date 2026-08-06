#pragma once
#include "Client_Defines.h"
#include "Level.h"
#include "Handle.h"
NS_BEGIN(Client)

class CLevelLastBossRanrok final : public CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelLastBossRanrok, CLevel)

private:
	explicit CLevelLastBossRanrok();
	~CLevelLastBossRanrok() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static UPtr<CLevelLastBossRanrok> Create();

private:
	HRESULT SpawnFlyCamera();
	HRESULT SpawnUICamera();

	HRESULT SpawnMonster(std::optional<CHandle> hPlayer);
	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();
	HRESULT SpawnPlayerCape(CHandle hPlayer);

	HRESULT SpawnStaticCollision();
	HRESULT SpawnLightPlacement();

	HRESULT SpawnSkyBox();

private:
	HRESULT PlayBGM();
	HRESULT StopBGM(_float fDuration = 1.f);
	void SubscribePlayerDeath(const CHandle& hPlayer);
private:
	SOUND_ID m_bmgID{ INVALID_SOUND_ID };
	CHandle m_hPlayer{};
	uint64_t m_iPlayerDeathListenerID{};

private:
	_bool m_bCreatePlayScreenUI{ false };

private:
	void Free() override;
};

NS_END
