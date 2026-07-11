
#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelAnimEditor final : public Engine::CLevel
{
private:
	CLevelAnimEditor();
	~CLevelAnimEditor() override;

public:
	HRESULT Initialize() override;
	void Update(_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(_float fTimeDelta) override;



public:
	static UPtr<CLevelAnimEditor> Create();

private:
	int32_t iTestCount{1};
private:
	void Free() override;
};

NS_END
