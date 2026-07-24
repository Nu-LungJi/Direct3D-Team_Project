#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelLogo final : public Engine::CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelLogo, CLevel)

private:
	explicit CLevelLogo();
	~CLevelLogo() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static Engine::UPtr<CLevelLogo> Create();

private:
	void Free() override;
};

NS_END
