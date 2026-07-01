#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CResComputeShader;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResStructuredBuffer;
NS_END

NS_BEGIN(Client)
class  CParticle : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CParticle, CGameObject);

	// 셰이더와 데이터 주파수를 맞추기 위한 상수 버퍼 구조체 (유지)
	struct CB_ParticleUpdate
	{
		float    g_fTimeDelta;
		uint32_t g_iNumInstances;
		uint32_t g_iBehaviorType;
		float    g_fPadding;
	};

private:
	explicit CParticle();
	virtual ~CParticle() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

private:

	uint32_t m_iNumElements{ 3000 };
	SPtr< CResStructuredBuffer> m_pParticleStructuredBuffer = nullptr;

	SPtr< CResComputeShader> m_pResComputeShader = nullptr;

	SPtr< CResTexture2D> m_pParticleTexture = nullptr; 
	SPtr< CResVertexShader> m_pResVertexShader = nullptr;
	SPtr< CResPixelShader> m_pResPixelShader = nullptr;
	SPtr< CResSamplerState> m_pResSamplerState = nullptr;

	CComConstantBuffer* m_pComCBuffer = nullptr;

public:
	static UPtr<CParticle> Create();
	UPtr<CPrototype> Clone(void* pArg);
};

NS_END