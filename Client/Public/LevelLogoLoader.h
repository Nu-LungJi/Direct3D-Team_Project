#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Client)

class CLevelLogoLoader
{
public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();

private:
	static _bool UILoad();
};

NS_END
