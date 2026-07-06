#pragma once
#include "Engine_Defines.h"
namespace physx {
	class PxFoundation;
	class PxPhysics;
	class PxScene;
	class PxDefaultCpuDispatcher;
	class PxPvd;
	class PxCooking;
}

NS_BEGIN(Engine)
class CPhysxManagerListener;
class ENGINE_DLL CPhysXManager final: public CEngineBase
{
private:
	CPhysXManager();
	~CPhysXManager() override;

private:
	HRESULT Initialize();

public:
	void Update(_float fTimeDelta);
	void UpdateGUI();

private:
	void UpdateDebugRender(_float fTimeDelta);

public:
	void StepSimulation(float fixedDeltaTime);

public:
	physx::PxScene* GetScene() const { return m_pScene; }
	physx::PxPhysics* GetPhysics() const { return m_pPhysics; }


private:
	void SyncPhysicsToComponents();
	

private:
	physx::PxFoundation* m_pFoundation{};
	physx::PxPhysics* m_pPhysics{};
	physx::PxScene* m_pScene{};
	physx::PxDefaultCpuDispatcher* m_pCpuDispatcher{};
	physx::PxPvd* m_pPvd{};

private:
	UPtr<CPhysxManagerListener> m_pListener{};

private:
	_bool m_bDbgRender{ true };
public:
	static UPtr<CPhysXManager> Create();

private:
	void Free() override;
};
NS_END
