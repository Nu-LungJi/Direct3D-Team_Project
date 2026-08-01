#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "PhysXCollisionProxyData.h"
#include <mutex>
#include <shared_mutex>
namespace physx {
	class PxActor;
	class PxFoundation;
	class PxPhysics;
	class PxScene;
	class PxDefaultCpuDispatcher;
	class PxPvd;
	class PxCooking;
	class PxControllerManager;
	class PxShape;
	class PxCudaContextManager;
}

NS_BEGIN(Engine)
class CPhysxManagerListener;
class CPhysXCollisionProxyEditor;
class CPhysXCookingEditor;
class CRagdollEditorGUI;
class CGameObject;
struct PX_CCT_HIT_DATA;
struct PX_CCT_OBSTACLE_HIT_DATA;
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
	void SetCollisionLayerNames(std::vector<std::pair<uint32_t, std::string>> layerNames);
	_bool EditCollisionLayerGUI(const char* pLabel, uint32_t& iLayer) const;
	_bool EditCollisionLayerMaskGUI(const char* pLabel, uint32_t& iMask) const;
	const std::vector<std::pair<uint32_t, std::string>>& GetCollisionLayerNames() const
	{
		return m_CollisionLayerNames;
	}
	std::vector<CHandle> CreateCollisionProxyObjects(
		const PX_COLLISION_PROXY_FILE& data, std::string_view layerName);
	std::vector<CHandle> CreateCollisionProxyObjectsFromFile(
		std::string collisionFileName, std::string_view layerName);

public:
	//_bool RayCast(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, PX_RAYCAST_RESULT& outResult) const;
	//_bool RayCastMultiple(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, std::vector<PX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit = 10)const;
	_bool RayCast(const PX_RAYCAST_DESC& tDesc, PX_RAYCAST_RESULT& outResult) const;
	_bool RayCastMultiple(const PX_RAYCAST_DESC& tDesc, std::vector<PX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit = 10) const;

	_bool Sweep(const PX_SWEEP_DESC& tDesc, PX_SWEEP_RESULT& outResult) const;
	_bool SweepMultiple(const PX_SWEEP_DESC& tDesc, std::vector<PX_SWEEP_RESULT>& outVecResult, uint32_t iMaxHit = 10) const;

	_bool Overlap(const PX_OVERLAP_DESC& tDesc, PX_OVERLAP_RESULT& outResult) const;
	_bool OverlapMultiple(const PX_OVERLAP_DESC& tDesc, std::vector<PX_OVERLAP_RESULT>& outVecResult, uint32_t iMaxHit = 10) const;

private:
	void UpdateDebugRender(_float fTimeDelta);

public:
	void StepSimulation(float fixedDeltaTime);

public:
	physx::PxScene* GetScene() const { return m_pScene; }
	physx::PxPhysics* GetPhysics() const { return m_pPhysics; }
	physx::PxControllerManager* GetControllerManager() const { return m_pControllerManager; }

public:
	_bool RegisterActor(const physx::PxActor* pActor, const PX_ACTOR_USER_DATA& userData);
	void UnregisterActor(const physx::PxActor* pActor);
	std::optional<PX_ACTOR_USER_DATA> FindActorUserData(const physx::PxActor* pActor) const;
	CGameObject* FindGameObject(const physx::PxActor* pActor) const;

	_bool RegisterShape(const physx::PxShape* pShape, const PX_SHAPE_USER_DATA& userData);
	void UnregisterShape(const physx::PxShape* pShape);
	std::optional<PX_SHAPE_USER_DATA> FindShapeUserData(const physx::PxShape* pShape) const;

	void QueueCCTShapeHit(const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit);
	void QueueCCTControllerHit(const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit);
	void QueueCCTObstacleHit(const CHandle& hOwner, const PX_CCT_OBSTACLE_HIT_DATA& tHit);

private:
	void SyncPhysicsToComponents();
	

private:
	physx::PxControllerManager* m_pControllerManager{};
	physx::PxFoundation* m_pFoundation{};
	physx::PxPhysics* m_pPhysics{};
	physx::PxScene* m_pScene{};
	physx::PxDefaultCpuDispatcher* m_pCpuDispatcher{};
	physx::PxCudaContextManager* m_pCudaContextManager{};
	physx::PxPvd* m_pPvd{};

private:
	UPtr<CPhysxManagerListener> m_pListener{};
	UPtr<CPhysXCollisionProxyEditor> m_pCollisionProxyEditor{};
	UPtr<CPhysXCookingEditor> m_pCookingEditor{};
	UPtr<CRagdollEditorGUI> m_pRagdollEditor{};

private:
	mutable std::shared_mutex m_UserDataRegistryMutex{};
	std::unordered_map<const physx::PxActor*, PX_ACTOR_USER_DATA> m_ActorUserDataRegistry{};
	std::unordered_map<const physx::PxShape*, PX_SHAPE_USER_DATA> m_ShapeUserDataRegistry{};
	std::vector<std::pair<uint32_t, std::string>> m_CollisionLayerNames{};

private:
	_bool m_bDbgRender{ false };
	_bool m_bGpuSimulationEnabled{ false };
public:
	static UPtr<CPhysXManager> Create();

private:
	void Free() override;
};
NS_END
