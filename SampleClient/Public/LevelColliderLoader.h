#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Client)

class CLevelColliderLoader
{
public:
	static std::future<bool> Load();
	static HRESULT UnLoad();
};

NS_END
