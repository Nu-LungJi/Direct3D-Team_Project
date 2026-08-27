#pragma once

#include "PhysXCollisionProxyObject.h"
#include "Handle.h"
NS_BEGIN(Client)

class CTriggerCRW_BridgeFix final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CTriggerCRW_BridgeFix, E::CPhysXCollisionProxyObject)

private:
	CTriggerCRW_BridgeFix() = default;
	CTriggerCRW_BridgeFix(const CTriggerCRW_BridgeFix&) = default;
	~CTriggerCRW_BridgeFix() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTriggerCRW_BridgeFix> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bSpawned{ false };
	_bool m_bQuestAdvanced{ false };
	_float m_fFixElapsedTime{};
	std::optional<CHandle> m_hTiggeredPlayer{};
};

NS_END
