#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CResQuadColBuffer;
NS_END
NS_BEGIN(Client)

class CTestGuizmo final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestGuizmo, CGameObject)

private:
	CTestGuizmo();
	~CTestGuizmo() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	SPtr< CResQuadColBuffer> m_pResCubeCol{};

public:
	static E::UPtr<CTestGuizmo> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END