#pragma once
#include "Client_Defines.h"

NS_BEGIN(Client)


struct FRequestPlayerCameraShake
{
	// 0~1
	_float fIntensity{ 1.f };

	_float fDuration{ 0.3f };

	// 초당 흔들리는 횟수
	_float fFrequency{ 18.f };
};

struct FAcientMagicStart {};

NS_END
