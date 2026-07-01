
#pragma once
#include "Engine_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CResDynamicBuffer;
class ENGINE_DLL CParticle: public CGameObject
{
public:

private:
	explicit CParticle();
	CParticle(const CParticle& rhs);
	~CParticle();

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) ;


private:
	std::vector<VTX_FIRE_INSTANCED_DATA> m_vecInstancedData{};
	uint32_t m_iNumElements{ 3000 };
	SPtr<CResDynamicBuffer> m_pResInstancedBuffer{};

public:
	static UPtr<CParticle> Create();
	UPtr<CPrototype> Clone(void* pArg) ;
};

NS_END