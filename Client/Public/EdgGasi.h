#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgGasi : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgGasi, CDragonSkill)

protected:
	explicit CEdgGasi();
	explicit CEdgGasi(const CEdgGasi& rhs);
	~CEdgGasi() override;

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
	void			ResetValue()override;
	void			MoveGasi(_float fTimeDelta);
	_bool			MoveSweep(_vector vNextPos);
public:
	static E::UPtr<CEdgGasi> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

};

NS_END

