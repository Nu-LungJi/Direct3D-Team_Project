#pragma once
#include "DragonSKill.h"
#include "SkillTarget.h"
NS_BEGIN(Engine)
class CComPxRigidBody;
class CComPxSphereCollider;
NS_END

NS_BEGIN(Client)
class CEdgRandomBall : public CDragonSkill,  public CSkillTarget
{
public:
	DECLARE_DERIVED_TYPE(CEdgRandomBall, CDragonSkill)

	enum class COLOR {YELLOW, PURPLE, RED, END};
protected:
	explicit CEdgRandomBall();
	explicit CEdgRandomBall(const CEdgRandomBall& rhs);
	~CEdgRandomBall() override;

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
private:
	_bool			Check_Table(PLAYER_SKILL_TYPE eType)override;
	void			Ball(_float fTimeDelta);
	_bool			Sweep(_vector vNextPos);

public:
	static E::UPtr<CEdgRandomBall> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	CComPxRigidBody* m_pComRigidBody{};
	CComPxSphereCollider* m_pComSphereCollider{};
	COLOR			m_eColor = COLOR::END;
	uint32_t		m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
};

NS_END

