#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComBeHavior;
NS_END

NS_BEGIN(Client)
class CMonster : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMonster, CGameObject)
public:
	typedef struct tagGoblnedesc : CGameObject::GAMEOBJECT_DESC
	{
		_string SocketName{};
	}MONSTER_DESC;
private:
	CMonster();
	~CMonster() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:

	void Set_Damage(int32_t iDamage) { m_iHp -= iDamage; }
private:
	CComBeHavior*				m_pComBT{ nullptr };
	int32_t						m_iHp{};
	_bool						m_bDead{ false };

	_string						m_SocketName{};
public:
	static E::UPtr<CMonster> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END


