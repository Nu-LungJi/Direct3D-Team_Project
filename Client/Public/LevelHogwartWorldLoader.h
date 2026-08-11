#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Client)

class CLevelHogwartWorldLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/HogwartWorld";
	static constexpr const char* TERRAIN_PATH = "./Resources/json/MapSaved/HogwartWorld/terrain/terrain.json";

public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();

private:
	static HRESULT LoadPlayerResources();
	static HRESULT LoadPlayerCape();
	static _bool UILoad_InWorker();

private:
	constexpr static LEVEL CURR_LEVEL = LEVEL::HOGWART_WORLD;
};

NS_END
