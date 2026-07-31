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

	HRESULT SpawnStaticCollision();
	HRESULT SpawnLightPlacement();
	HRESULT SpawnBridge();
	HRESULT SpawnMyMagicStepController();

private:
	_bool m_bCreatePlayScreenUI{ false };

private:
	void Free() override;
};

NS_END
