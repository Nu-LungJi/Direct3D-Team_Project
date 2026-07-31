#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Client)
class CLevelCharlesRookwoodLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/Tomb12345";

	static std::future<bool> Load();
	static std::future<bool> UnLoad();
private:
	static HRESULT MonsterLoad_InWorker();

private:
	static _bool UILoad();
};
NS_END
