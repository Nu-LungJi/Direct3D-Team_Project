#pragma once

//#include "Handle.h"
namespace Engine
{
	class CGameObject;
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

	struct PHYSIX_RAYCAST_RESULT
	{
		_bool bHit{ false };
		_float3 vHitpos{}; // 충돌 지점
		_float3 vHitNormal{}; // 충돌 표면의 법선 벡터
		_float fDistance{}; // 시작점으로부터의 거리
		CGameObject* pGameObject{};
	};

	//struct PHYSIX_USER_DATA
	//{
	//	CHandle hObject;
	//};
}
