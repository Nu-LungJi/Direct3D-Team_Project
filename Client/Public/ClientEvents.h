#pragma once
#include "Client_Defines.h"
#include "UI_Enums.h"
#include <string>

NS_BEGIN(Client)


struct FRequestPlayerCameraShake
{
	// 0~1
	_float fIntensity{ 1.f };

	_float fDuration{ 0.3f };

	// 초당 흔들리는 횟수
	_float fFrequency{ 18.f };
};

struct FPlayerDied
{
	CHandle hPlayer{};
	_float fLevelBgmFadeDuration{ 1.f };
};

struct FAncientMagicStart {};

struct FQuestUIGroupChanged
{
	QUEST_UI_GROUP Group{ QUEST_UI_GROUP::NONE };
	_bool Active{ false };
	// Empty text uses the UIController's default text for the group.
	std::string QuestText{};
	// Battle-zone quests can update the minimap content and quest widget
	// independently while continuing to use the same quest group.
	_bool UpdateMinimap{ true };
	_bool UpdateQuestWidget{ true };
};

NS_END
