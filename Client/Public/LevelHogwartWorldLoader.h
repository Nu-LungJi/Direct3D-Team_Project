#pragma once

#include "Engine_Defines.h"
#include "Level_Defines.h"
NS_BEGIN(Client)

class CLevelHogwartWorldLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/LastHogwartWorld";
	static constexpr const char* TERRAIN_PATH = "./Resources/json/MapSaved/LastHogwartWorld/terrain/terrain.json";

public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();

private:
	static HRESULT LoadPlayerResources();
	static HRESULT LoadPlayerCape();
	static _bool UILoad_InWorker();

	static HRESULT MonsterLoad_InWorker();

	static HRESULT NpcLoad_InWorker();
	static HRESULT AnimatedObjectLoad_InWorker();
	static HRESULT WorldAgentLoad_InWorker();

	static HRESULT LoadCollsion_InWorker();
	static HRESULT LoadHogsmeade_ExtraAsset();
private:
	constexpr static LEVEL CURR_LEVEL = LEVEL::HOGWART_WORLD;
};

NS_END
