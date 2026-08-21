#pragma once
#include "PhysXCollisionProxyObject.h"
NS_BEGIN(Client)

class CCoin final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CCoin, E::CPhysXCollisionProxyObject)

private:
	CCoin() = default;
	CCoin(const CCoin&) = default;
	~CCoin() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CCoin> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bParticleSpawned{ false };
	_bool m_bCollected{ false };
	uint32_t m_iParticleID{ INVALID_PARTICLE_OWNER_ID };
};

NS_END
