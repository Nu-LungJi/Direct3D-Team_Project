#pragma once
#include "Client_Defines.h"
#include "Level.h"
#include "Handle.h"
NS_BEGIN(Client)

class CLevelCharlesRookwood final : public CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelCharlesRookwood, CLevel)

private:
	explicit CLevelCharlesRookwood();
	~CLevelCharlesRookwood() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static UPtr<CLevelCharlesRookwood> Create();

private:
	HRESULT SpawnFlyCamera();
	HRESULT SpawnUICamera();

	HRESULT SpawnMonster(std::optional<CHandle> hPlayer);
	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();
	HRESULT SpawnPlayerCape(CHandle hPlayer);

	HRESULT SpawnStaticCollision();
	HRESULT SpawnLightPlacement();
	HRESULT SpawnBridge();
	void ToggleGPUShadowDebugObject();
	HRESULT SpawnMyMagicStepController();

	HRESULT SpawnSkyBox();

private:
	HRESULT PlayBGM();
	HRESULT StopBGM(_float fDuration = 1.f);
	void SubscribePlayerDeath(const CHandle& hPlayer);

	/*----------- 광윤 추가 -----------*/
	HRESULT Initialize_VolumetricFog();
	HRESULT Initialize_EnviromentLight();
	/*---------------------------------*/
private:
	SOUND_ID m_bmgID{ INVALID_SOUND_ID };
	CHandle m_hPlayer{};
	CHandle m_hBridgeCRW{};
	uint64_t m_iPlayerDeathListenerID{};
	_float3 m_vBridgeOriginalPosition{};
	_bool m_bGPUShadowDebugObjectVisible{ false };

private:
	_bool m_bCreatePlayScreenUI{ false };
	_bool m_bInitialQuestShown{ false };
	_bool m_bInitialQuestMinimapActivated{ false };
	_float m_fInitialQuestDelay{};
	_float m_fInitialQuestMinimapDelay{};
	static constexpr _float INITIAL_QUEST_DELAY = 10.f;
	static constexpr _float INITIAL_QUEST_APPEAR_DURATION = 0.3f;

private:
	void Free() override;
};

NS_END
