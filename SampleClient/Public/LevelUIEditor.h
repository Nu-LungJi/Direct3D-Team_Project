#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelUIEditor final : public Engine::CLevel
{
private:
	explicit CLevelUIEditor();
	~CLevelUIEditor() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static Engine::UPtr<CLevelUIEditor> Create();

private:
	void Free() override;
};

NS_END

