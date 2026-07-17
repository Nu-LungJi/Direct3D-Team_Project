#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
class ENGINE_DLL IPhysicsSync
{
public:
    virtual ~IPhysicsSync() = default;
    virtual void SyncActivePhysXData(const PX_SYNC_DATA& syncData) = 0;
};

NS_END