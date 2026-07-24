#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Client)
class CLevelBossCharlesRookwoodLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/TombBoss";
public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();
};
NS_END
