#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
class ENGINE_DLL IPhysicsListener
{
public:
    virtual ~IPhysicsListener() = default;
    virtual void OnWake() = 0;
    virtual void OnSleep() = 0;
    virtual void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) = 0;
    virtual void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) = 0;
    virtual void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) = 0;
    virtual void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) = 0;

	// CCT move 중 수집된 Hit 알림. 물리 이벤트 큐에서 안전한 시점에 호출된다.
	virtual void OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit) = 0;
	virtual void OnCCTControllerHit(const PX_CCT_HIT_DATA& tHit) = 0;
	virtual void OnCCTObstacleHit(const PX_CCT_OBSTACLE_HIT_DATA& tHit) = 0;

	// CCT move 계산에 즉시 필요한 정책. 객체를 변경하지 말고 Behavior 플래그만 반환해야 한다.
	virtual PX_CCT_BEHAVIOR GetCCTShapeBehavior(CGameObject* pGameObject) const = 0;
	virtual PX_CCT_BEHAVIOR GetCCTControllerBehavior(CGameObject* pGameObject) const = 0;
	virtual PX_CCT_BEHAVIOR GetCCTObstacleBehavior(const void* pUserData) const = 0;
};

NS_END
