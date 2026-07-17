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
	HRESULT Initialize();

public:
	static Engine::UPtr<CMainApp> Create();
};

NS_END
