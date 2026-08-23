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

#define FROXELX	320
#define FROXELY	180
#define FROXELZ	96

class CRenderer final : public CEngineBase {
private:
	CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CRenderer() override;

public:
	HRESULT		Initialize();

private:		// Initialize
	HRESULT		InitializeShaderResource();
	HRESULT		InitializeBackBuffer();
	HRESULT		InitializeOffscreen();
	HRESULT		InitializeFullscreen();
	HRESULT		InitializeBaseTarget();
	HRESULT		InitializeTargetPBR();
	HRESULT		InitializeBlendTarget();
	HRESULT		InitializePostProcess();
	HRESULT		InitializeUserInterface();
	HRESULT		InitializeGFSDK_SSAO();
	HRESULT		InitializeFSR2_2();
	HRESULT		InitializeBloom();
	HRESULT		InitializeVolumetricEffect();
	HRESULT		InitializeUI3D();

public:			// Update
	VOID		Update(_float fTimeDelta);
	VOID		UpdateGUI();

private:		// GUI Update
	VOID		RendererGUI();

public:			// Render
	HRESULT		Draw();
	VOID		FrameEnd();

private:		// Render Setting
	HRESULT		Render_Shadow();
	HRESULT		Render_DepthMap();
	HRESULT		Render_NonAlpha();
	HRESULT		Render_Decal();
	HRESULT		Render_HBAO();
	HRESULT		Render_Lighting();
	HRESULT		Render_Alpha();
	HRESULT		Render_Effect();
	HRESULT		Render_VolumetricEffect();
	HRESULT		Render_PostProcess();
	HRESULT		Render_UI3D();
	HRESULT		Render_UserInterface();
	HRESULT		Render_FullScreen();

private:		// Render Object
	HRESULT		RenderPriority();
	HRESULT		RenderNonBlend();
	HRESULT		RenderNonBlend_Instanced();
	HRESULT		RenderMapMesh();
	HRESULT		RenderBlend();
	HRESULT		RenderLight();
	HRESULT		RenderSkybox();
	HRESULT		RenderEffect();
	HRESULT		RenderCollider();
	HRESULT		RenderUI3D();
	HRESULT		RenderUI();

private:		// Volumetric Effect Pass Render
	HRESULT		Update_VolumetricConstantBuffer();
	HRESULT		Render_VolumetricCloud();
	HRESULT		Render_LightIntegration();
	HRESULT		Render_FroxelZAccumulation();
	HRESULT		Render_TemporalBlend();
	HRESULT		Render_VolumetricComposite();

private:		// PostProcess Pass Render
	HRESULT		Render_PostProcess_Focusing();
	HRESULT		Render_PostProcess_LensFlare();
	HRESULT		Render_PostProcess_Bloom();
	HRESULT		Render_PostProcess_Filter();
	
private:		// Bloom Helper Function
	HRESULT		Update_TexelSize(_float _Width, _float _Height);
	HRESULT		Render_BrightPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, uint32_t _ScreenX, uint32_t _ScreenY);
	HRESULT		Render_VerticalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY);
	HRESULT		Render_HorizontalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY);

	HRESULT		Render_UpSampleCombinePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _HalfBloomTex, const SPtr<CResDynamicTexture2D>& _QuarterBloomTex, uint32_t _ScreenX, uint32_t _ScreenY);
	HRESULT		Render_DownSamplePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _SrcTex, uint32_t _ScreenX, uint32_t _ScreenY);
	HRESULT		Render_CombinedPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY);

private:		// Camera Setting / Render Constext Setting
	HRESULT		Bind_CameraAttribute(CCameraObject* _ActiveCam);
	HRESULT		Reset_RenderContext(RENDERPASS _Pass, CCameraObject* _ActiveCam);

public:			// PostProcess Effect Function
	VOID		Render_ChromaticRing(XMVECTOR _WorldPosition, _float _Duration, _float _Scale);
	VOID		Set_ChromaticRingOpacity(_float _Opacity) { m_fChromaticRingAlpha = _Opacity; }
	VOID		Apply_OutlineEffect(std::optional<CHandle> _OutlineTargetHandle) { m_pOutlineTargetHandle = _OutlineTargetHandle; }
	VOID		Clear_OutlineEffect();
	
private:		// Unbind Shader Resource / Shader / UAV / Render Target
	VOID		Unbind_Resources();

public:			// Shader Resource Generator
	TEXTURE3D	Generate_Texture3D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight, uint32_t _TexDepth);
	SPtr<CResDynamicTexture2D>	Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_UnorderedAccessView(const StringID& _sResTag, DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResViewPort>			Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);

	HRESULT		Generate_Texture2DArray(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT		Generate_ShadowCubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT		Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT		Generate_ShadowMapOutput(ID3D11UnorderedAccessView** _ShadowUAV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _ShadowSRV, uint32_t _LTYPE, uint32_t _ResolutionX, uint32_t _ResolutionY);
	
public:			// Append Render Queue
	HRESULT		AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);

public:			// Extra Function
	HRESULT		Reset_DefaultShader(RENDERGROUP _Group);

public:			// Volumetric Fog
	const CB_VLFOG	Get_VolumetricFogOption()							{ return m_pFogInfo; }
	VOID			Set_VolumetricFogOption(const CB_VLFOG& _FogOption) { m_pFogInfo = _FogOption; }

	_float			Get_HaltonSequence(uint32_t _FrameIndex, uint32_t _Base);

public:
	VOID			Apply_RadialBlur(_float _Intensity) { m_fBlurIntensity = _Intensity; }
	
private:
	ComPtr<ID3D11Device>		m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

	std::array<std::vector<IRenderable*>, ETOUI(RENDERGROUP::END)> m_pRenderObject{};
	RENDER_CTX					m_pRenderContext{};
	SPtr<CResRasterizerState>	m_pRasterizer{};

private:
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetDiffuse{};		// Diffuse
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetNormal{};			// Normal
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetSMRO{};			// SMRO
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetEmissive{};		// Emissive
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetDepth{};			// Depth
	ComPtr<ID3D11DepthStencilView> m_pDecalReadOnlyDSV{};

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPBR{};			// PBR
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetEffect{};			// Effect
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetPostProcess{};	// PostProcess
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetUI{};				// UI
	SPtr<CResDynamicTexture2D>	m_pOffScreenTex2D{};				// Combined
	SPtr<CResDynamicTexture2D>  m_pResDynTexTargetUI3D{};			// 3DUI
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetFocusingDepthMap{};	// DepthMap

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetHBAO{};			// HBAO

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetLensFlare{};

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_HalfScaleA{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_HalfScaleB{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_QuarterScaleA{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloom_QuarterScaleB{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetBloomResult{};

	SPtr<CResDynamicTexture2D>	m_pResDynTexTargetVolumetric{};		// Volumetric

	SPtr<CResDynamicTexture2D>  m_pResDynTexTargetPreviousRenderView{};

private:
	SPtr<CResVertexShader>		m_pOffScreenVertexShader{};
	SPtr<CResPixelShader>		m_pOffScreenPixelShader{};

	SPtr<CResVertexShader>		m_pPBRVertexShader{};

	SPtr<CResVertexShader>		m_pResVertexShader{};
	SPtr<CResPixelShader>		m_pResPixelShader{};

	SPtr<CResVertexShader>		m_pBlendVertexShader{};
	SPtr<CResPixelShader>		m_pBlendPixelShader{};

	SPtr<CResVertexShader>		m_pUI3DVertexShader{};
	SPtr<CResPixelShader>		m_pUI3DPixelShader{};

	SPtr<CResComputeShader>		m_pBrightPassComputeShader{};
	SPtr<CResComputeShader>		m_pVerticalBlurComputeShader{};
	SPtr<CResComputeShader>		m_pHorizontalBlurComputeShader{};
	SPtr<CResComputeShader>		m_pBloomPassComputeShader{};
	SPtr<CResComputeShader>		m_pUpSampleComputeShader{};
	SPtr<CResComputeShader>		m_pDownSampleComputeShader{};

	SPtr<CResCBuffer>			m_pBloomCBuffer{};
	SPtr<CResCBuffer>			m_pVolumetricFroxelCBuffer{};
	SPtr<CResCBuffer>			m_pVolumetricVFogCBuffer{};
	SPtr<CResCBuffer>			m_pVolumetricCSMCBuffer{};
	SPtr<CResCBuffer>			m_pVolumetricCloudCBuffer{};
	SPtr<CResCBuffer>			m_pLensFlareCBuffer{};

	SPtr<CResComputeShader>		m_pLensFlareComputeShader{};
	SPtr<CResComputeShader>		m_pPostProcessComputeShader{};

private:
	ComPtr<ID3D11Texture2D>				m_pBackBufferTexture{};
	ComPtr<ID3D11ShaderResourceView>	m_pBackBufferSRV{};

private:
	SPtr<CResVertexShader> m_pDeathScreenRedFilterVS{};
	SPtr<CResPixelShader> m_pDeathScreenRedFilterPS{};

private:
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV{};
	ComPtr<ID3D11DepthStencilView> m_pBackBufferDSV{};
	SPtr<CResViewPort> m_pBackBufferViewPort{};

private:
	SPtr<CResVertexShader>	m_pFullscreenVS{};
	SPtr<CResPixelShader>	m_pFullscreenPS{};
	SPtr<CResVIBuffer>		m_pFullscreenVIBuffer{};

private:		// FSR
	UPtr<CMyGFSDK_SSAO> m_pGFSDK_SSAO{};
	UPtr<CMyFSR2_2> m_pFSR2_2{};

private:
	_bool			m_bApplyEnvLight	= { true };		// 환경광 ON-OFF
	_bool			m_bApplyFilter		= { true };		// 필터 적용 ON-OFF
	_bool			m_bApplyVolumetric	= { false };	// 볼류메트릭 효과 ON-OFF
	_bool			m_bApplyShadow		= { false };	// 그림자 ON-OFF

private:		// ChromaticRing
	_float2			m_fScreenPosition{};
	_float			m_fExpandDuration{};
	_float			m_fCurrentLifeTime{};
	_float			m_fRingScale{};
	_float			m_fDeltaTime{};
	_float			m_fTimeAccumulation{};
	_float			m_fChromaticRingAlpha{};

private:		// Volumetric Fog
	SPtr<CResComputeShader>		m_pLightIntegrationCS{};
	SPtr<CResComputeShader>		m_pFroxelAccumulationCS{};
	SPtr<CResComputeShader>		m_pTemporalBlendedCS{};
	SPtr<CResPixelShader>		m_pVolumetricCompositePS{};

	SPtr<CResComputeShader>		m_pVolumetricCloudCS{};

	TEXTURE3D					m_pVoxelLighting{};
	TEXTURE3D					m_pVoxelAccumulated{};
	TEXTURE3D					m_pBlendedVolumeTex{};
	TEXTURE3D					m_pPreviousVolumeTex{};

	SPtr<CResDynamicTexture2D>	m_pVolumetricCloudTex{};

	CB_VLFOG					m_pFogInfo{};
	CB_ENVLIGHT					m_pEnvLight{};

	ComPtr<ID3D11ShaderResourceView>	m_pCSMShadowMapTexture	= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pBlueNoiseTexture		= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pVolumeTexture		= { nullptr };

	XMMATRIX					m_mShadowLightViewProj{};
	XMMATRIX					m_mPreviousCamViewProj{};

private:		// PostProcess
	std::optional<CHandle>				m_pOutlineTargetHandle{};

	ComPtr<ID3D11ShaderResourceView>	m_pLookUpTableTexture{};
	ComPtr<ID3D11ShaderResourceView>	m_pSRVIrradianceMap{};
	ComPtr<ID3D11ShaderResourceView>	m_pSRVPreFilteredMap{};
	ComPtr<ID3D11ShaderResourceView>	m_pSRVBRDFLookUpMap{};

	_float	m_fBlurIntensity{};

public:			// Hi-Z Fuction
	const CHizBuffer* GetPrevHizBuffer() const { return m_bHasPrevHizBuffer ? m_pPrevHizBuffer.get() : nullptr; }
	_bool		HasPrevHizBuffer() const { return m_bHasPrevHizBuffer && m_pPrevHizBuffer != nullptr; }

private:		// Hi-Z buffer ownership
	HRESULT		InitializeHizBuffer();
	HRESULT		BuildCurrentHizBuffer(); // 다 그려진 후 depth를 Hiz버퍼에 copy & mipChain 구성

private:		// Hi-Z Variable
	UPtr<CHizBuffer> m_pCurrentHizBuffer = {}; // 이번 프레임에서 새로 만든 자료
	UPtr<CHizBuffer> m_pPrevHizBuffer	 = {}; // 컬링에 사용할 자료
	_bool			 m_bHasPrevHizBuffer = false;

#ifdef _DEBUG
private:		// Debugging
	HRESULT		Initialize_Debugging();
	HRESULT		Render_Debugging();

private:
	XMFLOAT4X4					m_fDebugWorldMatrix[9];
	SPtr<CResVertexShader>		m_pDebugVertexShader = { nullptr };
	SPtr<CResPixelShader>		m_pDebugPixelShader  = { nullptr };
	SPtr<CResQuadTexBuffer>		m_pDebugRenderBuffer = { nullptr };

	std::vector<SPtr<CResDynamicTexture2D>>	m_pResDynTexTargetList{};

	_bool						m_bRenderDebugScreen = { false };
#endif

public:
	static UPtr<CRenderer> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
