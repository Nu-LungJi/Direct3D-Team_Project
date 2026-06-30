#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelLogo final : public Engine::CLevel
{
private:
	CLevelLogo();
	~CLevelLogo() override;

public:
	HRESULT Initialize() override;
	void Update(_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(_float fTimeDelta) override;

public:
	static UPtr<CLevelLogo> Create();

private:
	void Free() override;
};

NS_END