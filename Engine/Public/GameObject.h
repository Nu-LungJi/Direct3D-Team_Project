#pragma once
#include "Prototype.h"
#include "Component.h"
#include "ComTransform.h"
#include "ComCollider.h"
#include "Handle.h"
#include "IRenderable.h"
#include "IPhysicsListener.h"
#include "IPhysicsSync.h"
#include "TimeManager.h"

NS_BEGIN(Engine)

struct MODEL_INSTANCE_BATCH;
class CGameObjectManager;
class CGameObjectPoolManager;

// GameObjectManager가 제공하는 네 개의 Managed Update 단계 중 참여할 단계를 나타낸다.
// 비트 마스크이므로 여러 단계를 OR로 조합할 수 있다. ALL은 기존 네 단계 참여를 유지하는 기본값이고,
// NONE은 Manager에 등록된 채 모든 Managed Update만 제외하여 별도 전용 매니저가 직접 구동할 객체에 사용한다.
enum class GAMEOBJECT_UPDATE_LOOP : uint8_t
{
	NONE = 0,
	PRIORITY = 1u << 0,
	FIXED = 1u << 1,
	UPDATE = 1u << 2,
	LATE = 1u << 3,
	ALL = (1u << 4) - 1u
};

constexpr GAMEOBJECT_UPDATE_LOOP operator|(
	GAMEOBJECT_UPDATE_LOOP eLeft,
	GAMEOBJECT_UPDATE_LOOP eRight) noexcept
{
	return static_cast<GAMEOBJECT_UPDATE_LOOP>(
		static_cast<uint8_t>(eLeft) |
		static_cast<uint8_t>(eRight));
}

class ENGINE_DLL CGameObject : public CPrototype,
								public IRenderable,
								public IPhysicsListener,
								public IPhysicsSync
{
public:
	DECLARE_DERIVED_TYPE_WITH_BASES(
		CGameObject,
		CPrototype,
		IRenderable,
		IPhysicsListener,
		IPhysicsSync)
	// ENGINE_DLL 인애들은 반드시 명시적으로 복사 생성자, 복사 대입연산자 딜리트하거나 재정의해주어야함
	CGameObject& operator=(const CGameObject&) = delete;

public:
	typedef struct tagGameObjectDesc
	{
		CHandle __handle{};
		_string sObjectTag{};
	}GAMEOBJECT_DESC;

protected:
	explicit CGameObject();
	explicit CGameObject(const CGameObject& Prototype);
	~CGameObject();

public:
	virtual HRESULT Initialize(void* pArg);
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);
	virtual void UpdateGUI();

protected:
	// [LSY] GameObjectManager의 슬롯과 레이어에 등록이 모두 끝난 직후 한 번 호출된다.
	// Initialize 중에는 아직 Manager를 통한 자기 Handle/레이어 조회가 불가능하므로 등록 후처리는 여기서 수행한다.
	virtual void OnRegisteredToManager() {}

public:
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	virtual HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& Batch) { return S_OK; }

	virtual void SetInstanceModelNum(uint32_t iInstanceNum) {}
	bool HasRenderPass(RENDERPASS ePass) const override { return (m_RenderPassFlags & static_cast<uint32_t>(ePass)) != 0; };

	virtual _bool GetShadowBounds(BoundingBox& OutBounds) const override { return false; }
protected:
	uint32_t m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT);

public:
	CComTransform& GetTransform()				{ return *m_pComTransform;	}
	const CComTransform& GetTransform() const	{ return *m_pComTransform;	}

	
	CComCollider& GetCollider()					{ return *m_pComCollider;	}
	const CComCollider& GetCollider() const		{ return *m_pComCollider;	}

protected:
	std::vector<std::pair<StringID, UPtr<CComponent>>> m_Components{};
	std::unordered_map<StringID, size_t> m_ComponentsLookup{};
	CComTransform*	m_pComTransform{};

	CComCollider*	m_pComCollider{};

private:
	UPtr<CPrototype> CloneComponentProtoType(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg) const;

public:
	template<typename T>
	HRESULT AddComponentFromProto(const StringID& svGroupTag, const StringID& svPrototypetag, const StringID& svComponentTag,  void* pArg = nullptr, T** outPtr = nullptr)
	{
		auto pCom = CloneComponentProtoType(svGroupTag, svPrototypetag, pArg);
		if (!pCom)
		{
			return E_FAIL;
		}

		T* pCache = AddComponent(svComponentTag, static_uptr_cast<T>(std::move(pCom)));

		if (outPtr)
		{
			*outPtr = pCache;
		}

		return S_OK;
	}



	template<typename T>
	T* AddComponent(const StringID& tagComponent, UPtr<T> pComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter != m_ComponentsLookup.end())
		{
			return nullptr;
		}

		//pComponent->SetGameObject(this);

		T* pCache = pComponent.get();

		auto size = m_Components.size();
		std::pair<StringID, UPtr<CComponent>> a{ tagComponent, std::move(pComponent) };
		m_Components.push_back(std::move(a));
		m_ComponentsLookup.emplace(tagComponent, size);


		return pCache;
	}

	template<typename T>
	T* GetComponent(const StringID& tagComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter == m_ComponentsLookup.end())
		{
			return nullptr;
		}
		
		return Engine::Cast<T>(
			m_Components[iter->second].second.get());
	};



protected:
	HRESULT DelComponent(const StringID& tagComponent);

public:
	_string_view GetObjectTag() const { return m_sObjectTag; }
	void SetObjectTag(_string_view sObjectTag) { m_sObjectTag = sObjectTag; }
	void SetTimeDomain(TIME_DOMAIN eTimeDomain) { m_eTimeDomain = eTimeDomain; }
	TIME_DOMAIN GetTimeDomain() const { return m_eTimeDomain; }
protected:
	_string m_sObjectTag{};
	TIME_DOMAIN m_eTimeDomain{ TIME_DOMAIN::SCALED };


private:
	CHandle m_ObjectHandle{};
public:
	const CHandle& GetHandle() const { return m_ObjectHandle; }

protected:
	void Free() override;

public:
	// true는 파괴 요청만 큐에 등록하며 실제 해제는 안전한 FrameEnd에서 수행한다.
	// false는 FrameEnd가 삭제 배치를 수집하기 전까지만 요청 취소로 인정된다.
	// 단, DelLayer처럼 Manager가 논리 컨테이너에서 먼저 분리한 확정 파괴 요청은 취소할 수 없다.
	void SetPendingDestroy(_bool b = true);
	// GameObject 계층 제거 후 남은 호환 API다. 현재 객체 하나에만 동일 요청을 전달한다.
	void SetPendingDestroyCascade(_bool b = true);
	_bool GetPendingDestroy() const { return m_bPendingDestroy; }
private:
	void CommitPendingDestroy();
	_bool m_bPendingDestroy{ false };
	_bool m_bPendingDestroyCommitted{ false };

public:
	// 전체 Managed Update 참여 여부를 제어하는 런타임 master switch다.
	// 각 dispatch 직전에 검사하므로 단계별 배열을 재구축하지 않아도 즉시 반영되며 Pool 활성/비활성에도 사용한다.
	void SetManagedUpdateEnabled(_bool bEnabled);
	// GameObject 계층 제거 후 남은 호환 API다. 현재 객체 하나에만 동일 요청을 전달한다.
	void SetManagedUpdateEnabledCascade(_bool bEnabled);
	_bool IsManagedUpdateEnabled() const { return m_bManagedUpdateEnabled; }

	// Managed Update가 켜진 상태에서 참여할 세부 루프를 반환한다.
	// 마스크는 프로토타입 생성자에서 고정하고 복제 객체가 그대로 물려받는 정적 분류값이다.
	// 등록 후 변경해도 Manager의 단계별 배열은 즉시 재구성되지 않으므로 런타임 상태 제어에는 사용하지 않는다.
	GAMEOBJECT_UPDATE_LOOP GetUpdateLoopMask() const { return m_eUpdateLoopMask; }
protected:
	// 파생 프로토타입 생성자에서만 설정하는 용도다. 정의되지 않은 상위 비트는 구현에서 제거한다.
	void SetUpdateLoopMask(GAMEOBJECT_UPDATE_LOOP eMask);
	virtual void OnManagedUpdateEnabled() {}
	virtual void OnManagedUpdateDisabled() {}
	virtual _bool OnAcquireFromPool(void* pArg) { return true; }
	virtual void OnReleaseToPool() {}
private:
	// Pool Manager를 거치지 않은 직접 호출은 Manager의 Handle 상태와 어긋나므로 차단한다.
	_bool AcquireFromPool(void* pArg = nullptr);
	void ReleaseToPool();
	_bool m_bManagedUpdateEnabled{ true };
	GAMEOBJECT_UPDATE_LOOP m_eUpdateLoopMask{ GAMEOBJECT_UPDATE_LOOP::ALL };
	friend class CGameObjectManager;
	friend class CGameObjectPoolManager;

	// IPhysicsListener
public:
	void OnWake() override {}
	void OnSleep() override {}
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override {}
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override {}
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override {}
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override {}
	void OnJointBreak(const PX_ON_JOINT_BREAK_DATA& tData) override {}

	// CCT Hit 알림은 move 중 즉시 호출되지 않고 물리 이벤트 Dispatch 시점에 전달된다.
	void OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit) override {}
	void OnCCTControllerHit(const PX_CCT_HIT_DATA& tHit) override {}
	void OnCCTObstacleHit(const PX_CCT_OBSTACLE_HIT_DATA& tHit) override {}

	// CCT 이동 계산에 즉시 사용되므로 상태 변경 없이 정책값만 반환해야 한다.
	PX_CCT_BEHAVIOR GetCCTShapeBehavior(CGameObject* pGameObject) const override
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}
	PX_CCT_BEHAVIOR GetCCTControllerBehavior(CGameObject* pGameObject) const override
	{
		// [LSY] CCT가 다른 CCT 위에 멈추지 않고 캡슐 표면을 따라 미끄러지게 한다.
		return PX_CCT_BEHAVIOR::SLIDE;
	}
	PX_CCT_BEHAVIOR GetCCTObstacleBehavior(const void* pUserData) const override
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}

	// IPhysicsSync
public:
	void SyncActivePhysXData(const PX_SYNC_DATA& syncData) override;
	void InvalidatePhysXSyncData();
	virtual void UpdatePhysicData();
private:
	PX_SYNC_DATA m_PhysXSyncData{};
	_bool m_bPhysXSynced{ false };
};

NS_END
