#pragma once

//#include "Handle.h"

namespace Engine
{
	enum CollisionLayer {
		LAYER_PLAYER = (1 << 0),
		LAYER_TRIGGER = (1 << 1),
		LAYER_ENEMY = (1 << 2)
	};

	struct PHYSX_SYNC_DATA
	{
		_float3 vPos{};
		_float4 vQuat{};
	};

	struct PHYSIX_ON_COLLISION_DATA
	{
		bool hello;
	};

	struct PHYSIX_ON_TRIGGER_DATA
	{
		bool hello;
	};

	//struct PHYSIX_USER_DATA
	//{
	//	CHandle hObject;
	//};
}
