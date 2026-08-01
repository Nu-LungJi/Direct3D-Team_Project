#pragma once
#include "Engine_Defines.h"
#include "Client_Defines.h"
#include "BaseApp.h"
NS_BEGIN(Client)

class CMainAppLoader
{
public:
	static HRESULT Load();

private:
	static HRESULT Load_Particle_Resources();
	static HRESULT Load_PhysX_Resource();
	static HRESULT Create_ActionNode();
	static HRESULT Load_UIStaitc_Resource();
	static HRESULT Initialize_Sound();
};

NS_END
