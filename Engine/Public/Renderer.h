#pragma once
#include "Engine_Defines.h"
#include "IRenderable.h"
#include "HizBuffer.h"

NS_BEGIN(Engine)
class CGameObject;
class CResOffscreenTexture;
class CResDynamicTexture2D;

class CMyGFSDK_SSAO;
class CMyFSR2_2;

class CRenderer final : public CEngineBase
{
private:
	CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CRenderer() override;

public:
	VOID		UpdateGUI();
	VOID		Update(_float fTimeDelta);

public:
	HRESULT Initialize();

	HRESULT AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);



public:
	HRESULT	Reset_DefaultShader(RENDERGROUP _Group);



private:
	HRESULT InitializeShaderResource();
	HRESULT InitializeBackBuffer();
	HRESULT InitializeOffscreen();
	HRESULT InitializeShadow();
	HRESULT InitializeFullscreen();

	HRESULT InitializeBaseTarget();
	HRESULT InitializeTargetPBR();
	HRESULT InitializeBlendTarget();

	HRESULT InitializePostProcess();
	HRESULT	InitializeUserInterface();
	HRESULT InitializeGFSDK_SSAO();
	HRESULT InitializeFSR2_2();
	HRESULT InitializeBloom();
	HRESULT InitializeVolumetricEffect();

	HRESULT InitializeUI3D();

public:
	HRESULT Draw();
	void FrameEnd();
	const CHizBuffer* GetPrevHizBuffer() const { return m_bHasPrevHizBuffer ? m_pPrevHizBuffer.get() : nullptr; }
	_bool HasPrevHizBuffer() const { return m_bHasPrevHizBuffer && m_pPrevHizBuffer != nullptr; }

public:
	void DrawPlayerInvenUIPass() { m_bDrawPlayerInvenUIPass = true; }

public:
	SPtr<CResDynamicTexture2D>	Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_UnorderedAccessView(const StringID& _sResTag, DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResViewPort>			Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);

	HRESULT	Generate_Texture2DArray(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT	Generate_ShadowCubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT Generate_CubeMapList(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT	Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT Generate_ShadowMapOutput(ID3D11UnorderedAccessView** _ShadowUAV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _ShadowSRV, uint32_t _LTYPE, uint32_t _ResolutionX, uint32_t _ResolutionY);

	HRESULT	Generate_CubeMap(ID3D11ShaderResourceView** _SRV, ID3D11Texture2D** _TextureArray, uint32_t _Resolution, uint32_t _MipLevels);
	HRESULT Generate_CubeMapFace(ID3D11RenderTargetView** _RTV, ID3D11Texture2D* _Texture, uint32_t _FaceIndex, uint32_t _MipLevel);

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
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetEffect{};			// Effect
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPostProcess{};	// PostProcess
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetUI{};				// UI
	SPtr<CResDynamicTexture2D>	m_pOffScreenTex2D{};				// Combined
	SPtr<CResDynamicTexture2D>  m_pResDynTexTargetLight{};
	SPtr<CResDynamicTexture2D>  m_pResDynTexTargetUI3D{};			// 3DUI

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetHBAO{};			// HBAO

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_HalfScaleA{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_HalfScaleB{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_QuarterScaleA{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_QuarterScaleB{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloomResult{};	

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetVolumetric{};		// Volumetric
	SPtr<CResDynamicTexture2D>	m_pResDynTexUAVVolumetric{};

	SPtr<CResDynamicTexture2D> m_pResDynTexTargetPreviousRenderView{};

private:
	SPtr<CResVertexShader>		m_pOffScreenVertexShader{};
	SPtr<CResPixelShader>		m_pOffScreenPixelShader{};

	SPtr<CResVertexShader>		m_pPBRVertexShader{};
	SPtr<CResPixelShader>		m_pPBRPixelShader{};

	SPtr<CResVertexShader>		m_pResVertexShader{};
	SPtr<CResPixelShader>		m_pResPixelShader{};

	SPtr<CResVertexShader>		m_pBlendVertexShader{};
	SPtr<CResPixelShader>		m_pBlendPixelShader{};

	SPtr<CResVertexShader>		m_pUI3DVertexShader{};
	SPtr<CResPixelShader>		m_pUI3DPixelShader{};

	SPtr<CResPixelShader>		m_pBrightPassPS{};
	SPtr<CResPixelShader>		m_pVerticalBlurPS{};
	SPtr<CResPixelShader>		m_pHorizontalBlurPS{};
	SPtr<CResPixelShader>		m_pBloomPassPS{};
	SPtr<CResPixelShader>		m_pUpSamplePS{};
	SPtr<CResPixelShader>		m_pDownSamplePS{};

	SPtr<CResCBuffer>			m_pBloomCBuffer{};

	SPtr<CResComputeShader>		m_pVolumetricComputeShader{};

private:
	ComPtr<ID3D11Texture2D>				m_pBackBufferTexture{};
	ComPtr<ID3D11ShaderResourceView>	m_pBackBufferSRV{};

private:
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetShadow{};
	SPtr<CResViewPort>			m_pShadowViewPort{};

private:
	SPtr<CResDynamicTexture2D> m_pDeathScreenRedFilterTex2D{};
	SPtr<CResVertexShader> m_pDeathScreenRedFilterVS{};
	SPtr<CResPixelShader> m_pDeathScreenRedFilterPS{};

private:
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV{};
	ComPtr<ID3D11DepthStencilView> m_pBackBufferDSV{};
	SPtr<CResViewPort> m_pBackBufferViewPort{};

private:
	ComPtr<ID3D11ShaderResourceView>	m_pIrridianceMapSRV{};
	ComPtr<ID3D11ShaderResourceView>	m_pPreFilteredMapSRV{};
	ComPtr<ID3D11ShaderResourceView>	m_pBRDFLookUpMapSRV{};

	ComPtr<ID3D11Texture2D>				m_pIrridianceTex2D{ };
	ComPtr<ID3D11Texture2D>				m_pPreFilteredTex2D{ };

private:
	SPtr<CResVertexShader> m_pFullscreenVS{};
	SPtr<CResPixelShader> m_pFullscreenPS{};
	SPtr<CResVIBuffer> m_pFullscreenVIBuffer{};

	SPtr<CResPixelShader>	m_pPostProcessPS{};

	ComPtr<ID3D11ShaderResourceView>	m_pLUTTexture = { nullptr };
	ComPtr<ID3D11UnorderedAccessView>	m_pUAVVolumetric = { nullptr };

private:
	SPtr<CResViewPort>		m_pHalfViewPort{};
	SPtr<CResViewPort>		m_pQuarterViewPort{};

private:
	UPtr<CMyGFSDK_SSAO> m_pGFSDK_SSAO{};
	UPtr<CMyFSR2_2> m_pFSR2_2{};

private:
	HRESULT Render_Shadow();
	HRESULT	Render_DepthMap();
	HRESULT	Render_NonAlpha();
	HRESULT	Render_Alpha();
	HRESULT	Render_Effect();
	HRESULT	Render_VolumetricEffect();
	HRESULT Render_OffScreen();
	HRESULT Render_UserInterface();

	HRESULT Render_Lighting();
	HRESULT Render_UI3D();

	HRESULT Render_PostProcess();
	HRESULT Render_PostProcess_Bloom();
	HRESULT	Render_PostProcess_Filter();

	HRESULT	Bind_CameraAttribute(CCameraObject* _ActiveCam);
	HRESULT	Bind_VolumetricFog();
	HRESULT Reset_RenderContext(RENDERPASS _Pass, CCameraObject* _ActiveCam);

	_float	NoiseHash(uint32_t _X, uint32_t _Y, uint32_t _Z);
	inline _float saturate(_float val) { return val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val); }
	HRESULT Render_FullScreen();

	VOID	Unbind_Resources();

	VOID	Render_Quad();

	ComPtr<ID3D11ShaderResourceView>	Create_Texture2D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	ComPtr<ID3D11ShaderResourceView>	Create_Texture3D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0, uint32_t _TexDepth = 0);

	VOID	PostProcessGUI();
	VOID	VolumetricFogGUI();

	HRESULT Initialize_Debugging();
	HRESULT	Render_Debugging();


	// Bloom Helper Function
	HRESULT	Update_TexelSize(_float _Width, _float _Height);
	HRESULT Render_BrightPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture);
	HRESULT	Render_VerticalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture);
	HRESULT	Render_HorizontalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture);

	HRESULT Render_UpSampleCombinePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _HalfBloomTex, const SPtr<CResDynamicTexture2D>& _QuarterBloomTex);
	HRESULT Render_DownSamplePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _SrcTex);
	HRESULT Render_CombinedPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, const SPtr<CResDynamicTexture2D>& _BlurPassTexture);

private:
	XMFLOAT4X4					m_fDebugWorldMatrix[9];
	SPtr<CResVertexShader>		m_pDebugVertexShader = { nullptr };
	SPtr<CResPixelShader>		m_pDebugPixelShader = { nullptr };
	SPtr<CResQuadTexBuffer>		m_pDebugBuffer = { nullptr };

	std::vector<SPtr<CResDynamicTexture2D>>	m_pResDynTexTargetList;

	_bool						m_bRenderable = { false };

	ComPtr<ID3D11ShaderResourceView>	BlueNoiseTexture = { nullptr };
	ComPtr<ID3D11ShaderResourceView>	VolumeTexture = { nullptr };
	ComPtr<ID3D11UnorderedAccessView>	VolumeUAV = { nullptr };

private:
	HRESULT RenderPriority();
	HRESULT RenderNonBlend();
	HRESULT RenderNonBlend_Instanced();
	HRESULT	Render_HBAO();
	HRESULT RenderBlend();
	HRESULT RenderLight();
	HRESULT RenderSkybox();
	HRESULT RenderEffect();
	HRESULT RenderCollider();
	HRESULT RenderUI3D();
	HRESULT RenderUI();


private:
	_bool			ApplyFilter = { false };		// 필터 적용 ON-OFF
	_bool			ApplyVolumetric = { true };		// 볼류메트릭 효과 ON-OFF
	RENDER_CTX		RenderContext = {};
	_bool bApplyShadow = { true };
	XMMATRIX	ShadowLightVP{};
	SPtr<CResRasterizerState>	Rasterizer{};



// Hi-Z buffer ownership
private:
	UPtr<CHizBuffer> m_pCurrentHizBuffer = {}; // 이번 프레임에서 새로 만든 자료
	UPtr<CHizBuffer> m_pPrevHizBuffer = {}; // 컬링에 사용할 자료
	_bool m_bHasPrevHizBuffer = false;

private:
	HRESULT InitializeHizBuffer();
	HRESULT BuildCurrentHizBuffer(); // 다 그려진 후 depth를 Hiz버퍼에 copy & mipChain 구성

	_float			TimeAccumulation{};

	_float	m_fFogIntensity{};
	_float3	m_fFogColor{ 1.f, 1.f, 1.f };
	_float	m_fFogMaxHeight{};
	_float	m_fFogStartPos{};
	_float	m_fFogEndPos{};
	_float	m_fFogDensity{};

public:
	static UPtr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END


