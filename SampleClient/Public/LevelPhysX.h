#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelPhysX final : public Engine::CLevel
{
private:
	CLevelPhysX();
	~CLevelPhysX() override;

public:
	HRESULT Initialize() override;
	void Update(_float fTimeDelta) override;
	void UpdateGUI() override;
	HRESULT Render() override;
	void FrameStart(_float fTimeDelta) override;

public:
	static UPtr<CLevelPhysX> Create();

private:
	void Free() override;
};

NS_END