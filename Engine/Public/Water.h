#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CResQuadTexBuffer;
class CResVertexShader;
class CResPixelShader;
class CResTexture2D;
class CResSamplerState;
class CResBlendState;

class ENGINE_DLL CWater : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CWater, CGameObject)

	struct WATER_DESC : public CGameObject::GAMEOBJECT_DESC
	{

	};

private:
	CWater();
	CWater(const CWater& prototype);
	~CWater() override;

private:
	struct CB_WATER
	{
		_float4 waterColor = _float4(0.1f, 0.5f, 0.6f, 0.85f);  // 물 기본 색상 및 투명도 (RGB, A)
		_float2 scrollSpeed1 = _float2(0.5f, 0.3f); // 첫 번째 노멀 맵 스크롤 속도
		_float2 scrollSpeed2 = _float2(-0.4f, 0.6f); // 두 번째 노멀 맵 스크롤 속도
		_float  time = 0.f;         // 누적 시간 (Shader Animation 용)
		_float  waveIntensity = 100.0f;// 파도 요동 / 노멀 강도
		_float2 padding;      // 16바이트 배수 맞추기용 패딩
	};
	static_assert(sizeof(CB_WATER) % 16 == 0);

public:
	HRESULT InitializePrototype(void* Arg = nullptr) override;
	HRESULT Initialize(void* Arg = nullptr) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* context, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

private:
	_float m_fTime = 0.f;

private:
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComConstantBuffer* m_pComCBufferWater{};

	SPtr<CResQuadTexBuffer> m_pQuadTexBuffer{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};

	SPtr<CResTexture2D> m_pNormalTex0{};
	SPtr<CResTexture2D> m_pNormalTex1{};
	SPtr<CResSamplerState> m_pSamplerState{};
	SPtr<CResBlendState> m_pBlendState{};

public:
	static UPtr<CWater> Create();
	UPtr<CPrototype> Clone(void* pArg = nullptr) override;
};

NS_END
