#pragma once
#include "Engine_Defines.h"
#include "Client_Defines.h"
#include "BaseApp.h"
NS_BEGIN(Client)

class CMainApp final: public E::CBaseApp
{
private:
	CMainApp();
	~CMainApp() override;

private:
	//void FixedUpdate(E::_float fTimeDelta) override;
	//void Update(E::_float fTimeDelta) override;
	//HRESULT Render(E::_float fInterpolation) override;
	//void FrameStart(E::_float fTimeDelta) override;
	//void FrameEnd(E::_float fTimeDelta) override;

private:
	HRESULT Initialize();

public:
	static Engine::UPtr<CMainApp> Create();
};

NS_END
