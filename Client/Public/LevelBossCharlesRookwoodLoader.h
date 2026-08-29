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

private:
	static HRESULT		LoadPlayerCape();
	static HRESULT		MonsterLoad_InWorker();
	static HRESULT		LoadBossCharlesRookwood_ExtraAsset();
private:
	static _bool UILoad();
};
NS_END
