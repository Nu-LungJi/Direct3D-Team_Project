#pragma once

#include "GameObject.h"
#include "PhysXCollisionProxyData.h"

NS_BEGIN(Engine)
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)

class CMapCollisionProxyObject final : public E::CGameObject
{
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		std::string sCollisionFileName{};
	};

public:
	DECLARE_DERIVED_TYPE(CMapCollisionProxyObject, CGameObject)

private:
	CMapCollisionProxyObject() = default;
	~CMapCollisionProxyObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	static E::UPtr<CMapCollisionProxyObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	E::CComPxRigidBody* m_pComPxRigidBody{};
};

NS_END
