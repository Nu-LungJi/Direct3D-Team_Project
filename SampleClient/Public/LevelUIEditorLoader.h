#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Client)

class LevelUIEditorLoader
{
public:
	static std::future<bool> Load();
	static HRESULT UnLoad();
};

NS_END
