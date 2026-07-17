#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CGameObject;

enum class PX_CCT_BEHAVIOR : uint8_t
{
	NONE = 0,
	CAN_RIDE = 1 << 0,
	SLIDE = 1 << 1,
	USER_DEFINED_RIDE = 1 << 2
};

struct PX_CCT_HIT_DATA
{
	CGameObject* pGameObject{};
	_float3 vWorldPosition{};
	_float3 vWorldNormal{};
	_float3 vMoveDirection{};
	_float fMoveLength{};
};

struct PX_CCT_OBSTACLE_HIT_DATA
{
	const void* pUserData{};
	_float3 vWorldPosition{};
	_float3 vWorldNormal{};
	_float3 vMoveDirection{};
	_float fMoveLength{};
};

class ENGINE_DLL IPxCharacterControllerListener
{
public:
	virtual ~IPxCharacterControllerListener() = default;

	virtual void OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit) {}
	virtual void OnCCTControllerHit(const PX_CCT_HIT_DATA& tHit) {}
	virtual void OnCCTObstacleHit(const PX_CCT_OBSTACLE_HIT_DATA& tHit) {}

	virtual PX_CCT_BEHAVIOR GetCCTShapeBehavior(CGameObject* pGameObject) const
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}

	virtual PX_CCT_BEHAVIOR GetCCTControllerBehavior(CGameObject* pGameObject) const
	{
		return PX_CCT_BEHAVIOR::NONE;
	}

	virtual PX_CCT_BEHAVIOR GetCCTObstacleBehavior(const void* pUserData) const
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}
};

NS_END
