#pragma once
#include "Engine_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CParticle : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CParticle, CEngineBase)


protected:
	explicit CParticle();
	CParticle(const CParticle& rhs);
	virtual ~CParticle();
public:
	virtual HRESULT Initialize(void* pArg) = 0;
	virtual void PriorityUpdate(E::_float fTimeDelta) = 0;
	virtual void Update(E::_float fTimeDelta) = 0;
	virtual void LateUpdate(E::_float fTimeDelta) = 0;
	virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) = 0;
	virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) = 0;

public:
	PARTICLE_TYPE &Get_Type() { return m_eType; }
	HRESULT LoadParticleTexture(std::pair<StringID, StringID> textureId);

protected:
	PARTICLE_TYPE m_eType;
	SPtr<class CResTexture2D> m_pParticleTexture;
	SPtr<class CResPixelShader> m_pResPixelShader{};
	SPtr<class CResVertexShader> m_pResVertexShader{};
};

NS_END