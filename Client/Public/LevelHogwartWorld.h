#pragma once

#include "Client_Defines.h"
#include "Level.h"

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
	HRESULT SpawnTerrain(CHandle hPlayer);
	HRESULT SpawnFlyCamera();
	HRESULT SpawnUICamera();
	HRESULT SpawnPlayerCamera(CHandle hPlayer);
	HRESULT SpawnSkyBox();

public:
	static UPtr<CLevelHogwartWorld> Create();

private:
	_bool m_bCreatePlayScreenUI = false;

private:
	void Free() override;
};

NS_END
