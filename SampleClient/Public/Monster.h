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

	}GOBLINE_DESC;
private:
	CMonster();
	~CMonster() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	CComBeHavior* m_pComBT{ nullptr };
public:
	static E::UPtr<CMonster> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END


