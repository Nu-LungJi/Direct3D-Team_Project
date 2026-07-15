#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include "ComCollider.h"

NS_BEGIN(Engine)
class CComLuaScript;
NS_END
NS_BEGIN(Client)

class CTestCollider final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestCollider, CGameObject)

public:
	struct DESC: public CGameObject::GAMEOBJECT_DESC
	{
		_bool bIsController{ false };
		StringID CollGroupID{};
		std::vector<std::pair<StringID, CComCollider::DESC>> collInfos{};
	};

private:
	CTestCollider();
	~CTestCollider() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	//SPtr<CCollider> m_pCollider{};

private:
	std::vector<CComCollider*> m_vecComCollider{};
	StringID m_CollGroupID{};
	_bool m_bController{};
	CComLuaScript* m_pComLuaScript{};
public:
	static E::UPtr<CTestCollider> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
