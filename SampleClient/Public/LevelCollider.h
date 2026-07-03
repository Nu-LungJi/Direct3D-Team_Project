#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelCollider final : public Engine::CLevel
{
private:
	CLevelCollider();
	~CLevelCollider() override;

public:
	HRESULT Initialize() override;
	void Update(_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(_float fTimeDelta) override;

public:
	static UPtr<CLevelCollider> Create();

private:
	void Free() override;
};

NS_END