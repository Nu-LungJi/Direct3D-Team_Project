#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Client)
class CLevelPercivalLoader
{
public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();
};
NS_END
