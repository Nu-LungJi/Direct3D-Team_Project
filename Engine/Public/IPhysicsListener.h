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
    virtual void OnCollisionEnter(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info) = 0;
    virtual void OnCollisionExit(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info) = 0;
    virtual void OnTriggerEnter(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info) = 0;
    virtual void OnTriggerExit(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info) = 0;
};

NS_END