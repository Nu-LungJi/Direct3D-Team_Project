#pragma once
#include "GameObject.h"
#include "PhysXCollisionProxyData.h"

NS_BEGIN(Engine)

class CComPxRigidBody;

class ENGINE_DLL CPhysXCollisionProxyObject : public CGameObject
{
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		std::string sCollisionFileName{};
		const PX_COLLISION_PROXY_FILE* pCollisionData{};
	};

public:
	DECLARE_DERIVED_TYPE(CPhysXCollisionProxyObject, CGameObject)

protected:
	CPhysXCollisionProxyObject() = default;
	CPhysXCollisionProxyObject(const CPhysXCollisionProxyObject&) = default;
	~CPhysXCollisionProxyObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

public:
	static UPtr<CPhysXCollisionProxyObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	HRESULT LoadCollisionData(const DESC& desc, PX_COLLISION_PROXY_FILE& outData) const;
	HRESULT BuildCollision(const PX_COLLISION_PROXY_FILE& data);

private:
	std::vector<CComPxRigidBody*> m_pComPxRigidBodies{};
};

NS_END
