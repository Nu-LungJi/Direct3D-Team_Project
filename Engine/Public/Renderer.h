#pragma once
#include "Engine_Defines.h"
#include "IRenderable.h"
#include "HizBuffer.h"

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

	ComPtr<ID3D11Texture2D>          m_pBackBufferTexture{};
	ComPtr<ID3D11ShaderResourceView> m_pBackBufferSRV{};

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
	HRESULT Render_ShadowMap();
	HRESULT	Render_DepthMap();
	HRESULT	Render_NonAlpha();
	HRESULT	Render_Alpha();
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
#ifdef _DEBUG
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
#endif

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



//------------------------------------- 대성 추가 오클루전 컬링용 ------------------------------
private:
	UPtr<CHizBuffer> m_pCurrentHizBuffer = {}; // 이번 프레임에서 새로 만든 자료
	UPtr<CHizBuffer> m_pPrevHizBuffer = {}; // 컬링에 사용할 자료
	_bool m_bHasPrevHizBuffer = false;

private:
	HRESULT InitializeHizBuffer();
	HRESULT BuildCurrentHizBuffer(); // 다 그려진 후 depth를 Hiz버퍼에 copy & mipChain 구성
	HRESULT UpdatePrevHizCpuMips(); // prev Hi-Z의 모든 mip을 CPU vector로 복사한다 // 임시 테스트용

	_bool IsOcclusionCulledCPU(const IRenderable* pRenderObject) const; // 렌더러블 오브젝트 오클루전 컬링 검사 (CPU로 검사)
	_float SampleHizCpuDepth(uint32_t mip, uint32_t x, uint32_t y) const; // mip에서 depth값을 가져옴(CPU)


	void DrawOcclusionBoundsDebug(const IRenderable* pRenderObject, const _float4& color) const;


	struct HIZ_CPU_MIP
	{
		std::vector<float> depths{}; // mip depth 캐싱
		uint32_t width = 0; // mip width
		uint32_t height = 0; // mip height
	};
	std::vector<HIZ_CPU_MIP> m_HizCpuMips{};

	// 디버깅GUI용
	uint32_t m_iHizCpuTested = 0;
	uint32_t m_iHizCpuCulled = 0;
	_bool m_bDrawOcclusionBounds = false;

	// Mip선택이 잘 이루어지고 있는지 확인용GUI
	static constexpr uint32_t HIZ_DEBUG_MAX_MIPS = 16;
	mutable uint32_t m_HizSelectedMipCounts[HIZ_DEBUG_MAX_MIPS] = {};
	mutable uint32_t m_iHizSelectedMipOverflow = 0;
//--------------------------------------------------------------------------------------------

public:
	static UPtr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
