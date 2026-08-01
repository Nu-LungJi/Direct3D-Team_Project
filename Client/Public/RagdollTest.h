#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComPxRagdoll;
struct PX_RAGDOLL_DESC;
NS_END

NS_BEGIN(Client)

class CRagdollTest final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CRagdollTest, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
	};

private:
	CRagdollTest();
	CRagdollTest(const CRagdollTest& Prototype);
	~CRagdollTest() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

private:
	static PX_RAGDOLL_DESC MakeTestRagdollDesc();
	void DrawDebugRagdoll() const;

private:
	CComPxRagdoll* m_pComPxRagdoll{};
	_float3 m_vActivationLinearVelocity{};
	_float3 m_vActivationAngularVelocityDegrees{};

public:
	static UPtr<CRagdollTest> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
