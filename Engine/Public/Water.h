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
class CResDepthStencilState;

class ENGINE_DLL CWater : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CWater, CGameObject)

	struct WATER_DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vPosition{};
		_float2 vSize{ 8000.f, 8000.f };
		_float4 vWaterColor{ 0.012f, 0.055f, 0.16f, 0.9f };
		_float4 vShallowColor{ 0.018f, 0.10f, 0.24f, 1.f };
		_float4 vDeepColor{ 0.002f, 0.012f, 0.055f, 1.f };
		_float4 vReflectionColor{ 0.10f, 0.20f, 0.38f, 1.f };
		_float2 vScrollSpeed1{ 0.025f, 0.015f };
		_float2 vScrollSpeed2{ -0.018f, 0.022f };
		_float fWaveIntensity = 0.95f;
		_float fUVScale = 0.018f;
		_float fSecondaryNormalScale = 2.7f;
		_float fFollowSnap = 50.f;
		_bool bFollowCamera = true;
	};

private:
	CWater();
	CWater(const CWater& prototype);
	~CWater() override;

private:
	struct CB_WATER
	{
		_float4 waterColor{};
		_float4 shallowColor{};
		_float4 deepColor{};
		_float4 reflectionColor{};
		_float2 scrollSpeed1{};
		_float2 scrollSpeed2{};
		_float time{};
		_float waveIntensity{};
		_float uvScale{};
		_float secondaryNormalScale{};
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
	_float m_fSeaLevel = 0.f;
	_float m_fFollowSnap = 50.f;
	_bool m_bFollowCamera = true;
	CB_WATER m_WaterData{};

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
	SPtr<CResDepthStencilState> m_pDepthStencilState{};

public:
	static UPtr<CWater> Create();
	UPtr<CPrototype> Clone(void* pArg = nullptr) override;
};

NS_END
