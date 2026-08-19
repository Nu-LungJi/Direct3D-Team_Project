#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgPulse : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgPulse, CDragonSkill)

protected:
	explicit CEdgPulse();
	explicit CEdgPulse(const CEdgPulse& rhs);
	~CEdgPulse() override;

public:
	HRESULT			InitializePrototype(void* pArg = nullptr) override;
	HRESULT			Initialize(void* pArg) override;
	void			PriorityUpdate(E::_float fTimeDelta) override;
	void			FixedUpdate(E::_float fTimeDelta) override;
	void			Update(E::_float fTimeDelta) override;
	void			LateUpdate(E::_float fTimeDelta) override;
public:
	void			Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos = XMVectorSet(0, 0, 0, 1)) override;
	void			Cancle() override;
	const _float3&	Get_ClosestPointToPlayer() const { return m_vClosestPointToPlayer; }

private:
	void			ResetValue()override;
	void			Pulse(_float fTimeDelta);
	void			PulseOverlap(_vector vCenter);
private:
	_bool			m_bPlayerHit{ false };
	_bool			m_bWorldStaticHit{ false };
	_float3			m_vClosestPointToPlayer{};
	uint32_t		m_iBurstParticleOwnerId{ INVALID_PARTICLE_OWNER_ID };
public:
	static E::UPtr<CEdgPulse> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

