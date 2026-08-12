#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgRandomBall : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgRandomBall, CDragonSkill)

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
	void			Active(EDG_ACSKT_DESC& SkillTable) override;
	void			Cancle() override;
private:
	void			Ball(_float fTimeDelta);
	_bool			Sweep(_vector vNextPos);
public:
	static E::UPtr<CEdgRandomBall> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

