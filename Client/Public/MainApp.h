#pragma once
#include "Engine_Defines.h"
#include "Client_Defines.h"
#include "BaseApp.h"

NS_BEGIN(Client)

class CCinematicEditor;

class CMainApp final: public E::CBaseApp
{
private:
	CMainApp();
	~CMainApp() override;

private:
	HRESULT Initialize();

protected:
	void FrameStart(_float fTimeDelta) override;

private:
	E::UPtr<CCinematicEditor> m_pCinematicEditor{};

public:
	static Engine::UPtr<CMainApp> Create();
};

NS_END
