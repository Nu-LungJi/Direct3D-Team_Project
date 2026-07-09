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

public:
	HRESULT Initialize();

	HRESULT AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);

private:
	HRESULT InitializeShaderResource();
	HRESULT InitializeBackBuffer();
	HRESULT InitializeOffscreen();
	HRESULT InitializeShadow();
	HRESULT InitializeFullscreen();

	HRESULT InitializeBaseTarget();
	HRESULT InitializeTargetPBR();
	HRESULT InitializeBlendTarget();

	HRESULT InitilizePostProcess();
	HRESULT InitializeGFSDK_SSAO();
	HRESULT InitializeBloom();
	
	
public:
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
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetSMRO{};			// SMRO
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetEmissive{};		// Emissive
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetDepth{};			// Depth

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPBR{};			// PBR
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPostProcess{};	// PostProcess
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBrightPass{};		// Bloom SwapRTV
	SPtr<CResDynamicTexture2D>	m_pOffScreenTex2D{};				// Combined
	
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetHBAO{};			// HBAO

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBlurPass{};		// BlurPass
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloomPass{};		// BloomPass

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetVolumetric{};		// Volumetric

private:
	SPtr<CResVertexShader>		m_pOffScreenVertexShader{};
	SPtr<CResPixelShader>		m_pOffScreenPixelShader{};

	SPtr<CResVertexShader>		m_pPBRVertexShader{};
	SPtr<CResPixelShader>		m_pPBRPixelShader{};

	SPtr<CResVertexShader>		m_pResVertexShader{};
	SPtr<CResPixelShader>		m_pResPixelShader{};

	SPtr<CResVertexShader>		m_pBlendVertexShader{};
	SPtr<CResPixelShader>		m_pBlendPixelShader{};

	SPtr<CResPixelShader>		m_pBrightPassPixelShader{};
	SPtr<CResPixelShader>		m_pVerticalBlurPixelShader{};
	SPtr<CResPixelShader>		m_pBloomPassPixelShader{};

	SPtr<CResComputeShader>		m_pVolumetricComputeShader{};

private:
	ComPtr<ID3D11Texture2D>				m_pBackBufferTexture{};
	ComPtr<ID3D11ShaderResourceView>	m_pBackBufferSRV{};

private:
	SPtr<CResDynamicTexture2D> m_pShadowTex2D{};
	SPtr<CResViewPort> m_pShadowViewPort{};

private:
	SPtr<CResDynamicTexture2D> m_pDeathScreenRedFilterTex2D{};
	SPtr<CResVertexShader> m_pDeathScreenRedFilterVS{};
	SPtr<CResPixelShader> m_pDeathScreenRedFilterPS{};

private:
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV{};
	ComPtr<ID3D11DepthStencilView> m_pBackBufferDSV{};
	SPtr<CResViewPort> m_pBackBufferViewPort{};

private:
	SPtr<CResDynamicTexture2D> m_pLastTex2DBeforeFullScreenDraw{};

private:
	SPtr<CResVertexShader> m_pFullscreenVS{};
	SPtr<CResPixelShader> m_pFullscreenPS{};
	SPtr<CResVIBuffer> m_pFullscreenVIBuffer{};

	SPtr<CResPixelShader>	m_pPostProcessPS{};

private:	// PostProcess Variable
	_float m_pBloomIntensity		{ 0.f };
	_float m_pDistortionIntensity	{ 0.f };
	_float m_pChromaticIntensity	{ 0.f };
	_float m_pVignetteIntensity		{ 0.f };
	_float m_pVignetteSmoothness	{ 0.f };

	ComPtr<ID3D11ShaderResourceView>	m_pLUTTexture = { nullptr };

private:
	UPtr<CMyGFSDK_SSAO> m_pGFSDK_SSAO{};

private:
	HRESULT Render_Shadow();
	HRESULT	Render_DepthMap();
	HRESULT	Render_NonAlpha();
	HRESULT	Render_Alpha();
	HRESULT	Render_Volumetric();
	HRESULT Render_OffScreen();
	HRESULT Render_UserInterface();

	HRESULT Render_Lighting();

	HRESULT Render_PostProcess();
	HRESULT Render_PostProcess_Bloom();
	HRESULT	Render_PostProcess_Filter();

	HRESULT	Bind_CameraAttribute(CCameraObject* _ActiveCam);
	HRESULT Reset_RenderContext(RENDERPASS _Pass, CCameraObject* _ActiveCam);

	HRESULT Render_FullScreen();



	VOID	Unbind_Resources();

	SPtr<CResDynamicTexture2D>	Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResViewPort>			Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);

	VOID	PostProcessGUI();

	HRESULT Initialize_Debugging();
	HRESULT	Render_Debugging();

private:
	XMFLOAT4X4					m_fDebugWorldMatrix[9];
	SPtr<CResVertexShader>		m_pDebugVertexShader = { nullptr };
	SPtr<CResPixelShader>		m_pDebugPixelShader  = { nullptr };
	SPtr<CResQuadTexBuffer>		m_pDebugBuffer		 = { nullptr };
	
	std::vector<SPtr<CResDynamicTexture2D>>	m_pResDynTexTargetList;

	_bool						m_bRenderable = { false };

private:
	HRESULT RenderPriority();
	HRESULT RenderNonBlend();
	HRESULT	Render_HBAO();
	HRESULT RenderBlend();
	HRESULT RenderLight();
	HRESULT RenderSkybox();
	HRESULT RenderCollider();
	HRESULT RenderParticle();
	HRESULT RenderUI();

	
private:
	_bool			ApplyFilter = { false };		// 필터 적용 ON-OFF
	RENDER_CTX		RenderContext = {};

	SPtr<CResRasterizerState>	Rasterizer{};
public:
	static UPtr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
