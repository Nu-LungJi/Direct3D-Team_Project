#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelBossCharlesRookwood final : public CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelBossCharlesRookwood, CLevel)

private:
	explicit CLevelBossCharlesRookwood();
	~CLevelBossCharlesRookwood() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static UPtr<CLevelBossCharlesRookwood> Create();

private:
	HRESULT SpawnFlyCamera();
	HRESULT SpawnUICamera();
	HRESULT SpawnStaticCollision();
	HRESULT SpawnMonster(std::optional<CHandle> hPlayer);

	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();

private:
	void Free() override;
};

NS_END
