#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Client)
class CLevelPercivalLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/Tomb12345";

	static std::future<bool> Load();
	static std::future<bool> UnLoad();
};
NS_END
