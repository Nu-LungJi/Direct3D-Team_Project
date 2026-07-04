#pragma once
#include "Engine_Defines.h"
#include "IRenderable.h"
NS_BEGIN(Engine)
class CGameObject;
class CResOffscreenTexture;
class CResDynamicTexture2D;

class CMyGFSDK_SSAO;

class CRenderer final : public CEngineBase
{
private:
	CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CRenderer() override;

public:
	VOID	 UpdateGUI();
	VOID	 PostProcessGUI();

public:
	HRESULT Initialize();
private:
	HRESULT InitializeOffscreen();
	HRESULT InitializeShadow();
	HRESULT InitializeFullscreen();

	HRESULT InitializeTargetDiffuse();
	HRESULT InitializeTargetNormal();

	HRESULT InitilizePostProcess();
	HRESULT InitializeGFSDK_SSAO();

public:
	HRESULT AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);
	HRESULT Draw();
	void FrameEnd();

public:
	void DrawPlayerInvenUIPass() { m_bDrawPlayerInvenUIPass = true; }
private:
	_bool m_bDrawPlayerInvenUIPass{ false };

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
	std::array<std::vector<IRenderable*>, ETOUI(RENDERGROUP::END)> m_RenderObject{};

private:
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetDiffuse{};		// Diffuse
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetNormal{};			// Normal
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPostProcess{};	// PostProcess
	SPtr<CResDynamicTexture2D>	m_pOffScreenTex2D{};				// Combined
	
	//SPtr<CResDynamicTexture2D> m_pResDynTex

	SPtr<CResVertexShader>		m_pOffScreenVertexShader{};
	SPtr<CResPixelShader>		m_pOffScreenPixelShader{};

private:
	SPtr<CResDynamicTexture2D> m_pShadowTex2D{};
	SPtr<CResViewPort> m_pShadowVP{};

private:
	SPtr<CResDynamicTexture2D> m_pDeathScreenRedFilterTex2D{};
	SPtr<CResVertexShader> m_pDeathScreenRedFilterVS{};
	SPtr<CResPixelShader> m_pDeathScreenRedFilterPS{};

private:
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV{};
	ComPtr<ID3D11DepthStencilView> m_pBackBufferDSV{};
	SPtr<CResViewPort> m_pBackBufferVP{};

private:
	SPtr<CResDynamicTexture2D> m_pLastTex2DBeforeFullScreenDraw{};

private:
	SPtr<CResVertexShader> m_pFullscreenVS{};
	SPtr<CResPixelShader> m_pFullscreenPS{};
	SPtr<CResVIBuffer> m_pFullscreenVIBuffer{};

	SPtr<CResPixelShader>	m_pPostProcessPS{};

private:	// PostProcess Variable
	_float m_pDistortionIntensity	{ 0.f };
	_float m_pChromaticIntensity	{ 0.f };
	_float m_pVignetteIntensity		{ 0.f };
	_float m_pVignetteSmoothness	{ 0.f };

	ComPtr<ID3D11ShaderResourceView>	m_pLUTTexture = { nullptr };

private:
	UPtr<CMyGFSDK_SSAO> m_pGFSDK_SSAO{};

private:
	HRESULT Render_ShadowMap(RENDER_CTX& ctx);
	HRESULT	Render_DiffuseNormal(RENDER_CTX& ctx);
	HRESULT Render_OffScreen(RENDER_CTX& ctx);

	HRESULT Render_FullScreen();

#ifdef _DEBUG
	HRESULT Initialize_Debugging();
	HRESULT	Render_Debugging(const RENDER_CTX& ctx);

private:
	XMFLOAT4X4					m_fDebugWorldMatrix[8];
	SPtr<CResVertexShader>		m_pDebugVertexShader = { nullptr };
	SPtr<CResPixelShader>		m_pDebugPixelShader  = { nullptr };
	SPtr<CResQuadTexBuffer>		m_pDebugBuffer		 = { nullptr };
	
	std::vector<SPtr<CResDynamicTexture2D>>	m_pResDynTexTargetList;

	_bool						m_bRenderable = { false };
#endif

private:
	HRESULT RenderPriority(const RENDER_CTX& ctx);
	HRESULT RenderNonBlend(const RENDER_CTX& ctx);
	HRESULT RenderBlend(const RENDER_CTX& ctx);
	HRESULT RenderSkybox(const RENDER_CTX& ctx);
	HRESULT RenderCollider(const RENDER_CTX& ctx);
	HRESULT RenderParticle(const RENDER_CTX& ctx);
	HRESULT RenderPostProcess(const RENDER_CTX& ctx);
	HRESULT RenderUI(const RENDER_CTX& ctx);

	
	HRESULT	Bind_CameraAttribute(CCameraObject* _ActiveCam, CCameraObject* _ShadowCam);
	
private:
	_bool	ApplyFilter = { false };		// 필터 적용 ON-OFF

public:
	static UPtr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END