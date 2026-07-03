#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelMapEditor final : public Engine::CLevel
{
private:
	CLevelMapEditor();
	~CLevelMapEditor() override;

public:
	HRESULT Initialize() override;
	void Update(_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(_float fTimeDelta) override;

public:
	static UPtr<CLevelMapEditor> Create();

private:
	void Free() override;
};

NS_END