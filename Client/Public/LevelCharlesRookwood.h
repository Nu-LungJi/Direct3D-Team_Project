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
	HRESULT SpawnDebugPlayerCamera(std::optional<CHandle> hDebugPlayer);
	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();
	std::optional<CHandle> SpawnDebugPlayer();
	HRESULT SpawnStaticCollision();

private:
	_bool m_bCreatePlayScreenUI{ false };

private:
	void Free() override;
};

NS_END
