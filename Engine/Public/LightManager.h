#pragma once
#include "Engine_Defines.h"
#include "Light.h"
#include "ResComputeShader.h"
#include "ResGeometryShader.h"
#include "ResViewPort.h"
NS_BEGIN(Engine)

struct LightData {
	CLight* LightOBJ;
	_float Distance;
};

struct SHADOW_ARRAY_2D {
	ComPtr<ID3D11Texture2D>			 TexBuffer{};
	ComPtr<ID3D11ShaderResourceView> SRV{};
	ComPtr<ID3D11DepthStencilView>	 DSVList[MAX_SHADOW_LIGHT_COUNT];
};

struct SHADOW_ARRAY_CUBE {
	ComPtr<ID3D11Texture2D>			 TexBuffer{};
	ComPtr<ID3D11ShaderResourceView> SRV{};
	ComPtr<ID3D11DepthStencilView>	 DSVList[MAX_SHADOW_LIGHT_COUNT];
};

class ENGINE_DLL CLightManager final : public CEngineBase {
private:
	CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CLightManager() override;

public:
	HRESULT	Initialize_LightManager();

	VOID	Update(_float fTimeDelta);
	VOID	UpdateGUI();
	HRESULT	Capture_ShadowMap();
	HRESULT	Render_ObjectShadow();
	HRESULT	Render_ObjectNonShadow();

	VOID	Bind_DynamicLight();

	std::optional<CHandle> Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	std::optional<CHandle> Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range);
	std::optional<CHandle> Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);

	VOID	Clear_DynamicLightList()							{ m_LightHandleList.clear(); }

	HRESULT	AddShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);

	const	SPtr<CResDynamicTexture2D>& Get_CombinedResource()	{ return m_pUAVComBinedOutput; }

	VOID	Bind_ShadowResource();
	VOID	UnBind_ShadowResource();
	HRESULT Render_ShadowInstanced(const ComPtr<ID3D11DeviceContext>& pContext, const E::RENDER_CTX& ctx, LIGHT_TYPE _LType, _bool _bStaticBatch);

	VOID	Update_ActiveLights();
	VOID	Update_LightData();
	VOID	Allocate_ShadowSlot();

public:
	_bool	IsInFrustum(CLight* _LightOBJ);

	HRESULT Reset_EffectLight(const std::optional<CHandle>& _Handle);
	HRESULT Reset_AllEffectLight();

	HRESULT Transform_EffectLight(const std::optional<CHandle>& _Handle, XMFLOAT3 _Position);
	HRESULT Transform_EffectLight(const std::optional<CHandle>& _Handle, XMVECTOR _Position);
private:
	HRESULT	Generate_ShadowArray2D(SHADOW_ARRAY_2D& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT	Generate_ShadowArrayCube(SHADOW_ARRAY_CUBE& _SHAR, uint32_t _ResolutionX, uint32_t _ResolutionY);

public:
	HRESULT	Initialize_EffectLight(uint32_t _PoolSize);
	std::optional<CHandle> Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color, _float _Range, _float _LifeTime, _float3 _Velocity);


private:
	std::vector<std::optional<CHandle>>				m_pEffectLightPool{};
	uint32_t							m_iEffectLightPoolSize{};
	uint32_t							m_iLastAllocatedIndex{};
#ifdef _DEBUG
public:
	HRESULT	Initialize_DebugRender();
	HRESULT Render_DebugIcon();

private:
	SPtr<CResVertexShader>	m_pResDebugVertexShader			= { nullptr };
	SPtr<CResPixelShader>	m_pResDebugPixelShader			= { nullptr };

	SPtr<CResQuadTexBuffer>	m_pResLightTexBuffer			= { nullptr };

	// Light 위치 나타내는 용 아이콘 텍스쳐
	SPtr<CResTexture2D>		m_pResDirectionalLightTexture2D = { nullptr };
	SPtr<CResTexture2D>		m_pResPointLightTexture2D		= { nullptr };
	SPtr<CResTexture2D>		m_pResSpotLightTexture2D		= { nullptr };

#endif

private:
	ComPtr<ID3D11Device>				m_pDevice = { nullptr };
	ComPtr<ID3D11DeviceContext>			m_pContext = { nullptr };

	std::vector<std::optional<CHandle>>	m_LightHandleList;

	ComPtr<ID3D11ShaderResourceView>	m_pIrridianceSRV = { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pPreFilterSRV = { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pLUTSRV = { nullptr };

	SPtr<CResCBuffer>					m_pLightConstantBuffer{ };

	SPtr<CResVertexShader>				m_pResVertexShader = { nullptr };
	SPtr<CResPixelShader>				m_pResPixelShader = { nullptr };

	SPtr<CResVertexShader>				m_pPointLightVS = { nullptr };
	SPtr<CResVertexShader>				m_pDirectionalLightVS = { nullptr };

	SPtr<CResVertexShader>				m_pInstancedPointLightVS = { nullptr };
	SPtr<CResVertexShader>				m_pInstancedDirectionalLightVS = { nullptr };

	SPtr<CResGeometryShader>			m_pPointLightGS = { nullptr };
	SPtr<CResPixelShader>				m_pPointLightPS = { nullptr };

	SPtr<CResComputeShader>				m_pShadowComputeShader = { nullptr };
	SPtr<CResComputeShader>				m_pNonShadowComputeShader = { nullptr };
	SPtr<CResDynamicTexture2D>			m_pUAVComBinedOutput = { nullptr };
	SPtr<CResViewPort>					m_pDirectionalShadowViewPort{};
	SPtr<CResViewPort>					m_pPointShadowViewPort{};

	std::vector<CGameObject*>			m_pRenderable_StaticObjectList{};
	std::vector<CGameObject*>			m_pRenderable_DynamicObjectList{};

	std::vector<std::optional<CHandle>>	m_pActiveShadowLightList{};

	CB_LIGHT							m_pLightConstantVariable{};

	SHADOW_ARRAY_2D						m_pStaticDirectionalShadowList{};
	SHADOW_ARRAY_2D						m_pDynamicDirectionalShadowList{};

	SHADOW_ARRAY_CUBE					m_pStaticPointShadowList{};
	SHADOW_ARRAY_CUBE					m_pDynamicPointShadowList{};

public:
	static UPtr<CLightManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};
NS_END
