#pragma once
#include "Engine_Defines.h"
#include "Light.h"
#include "ResComputeShader.h"
#include "ResViewPort.h"
NS_BEGIN(Engine)

class ENGINE_DLL CLightManager final : public CEngineBase {
private:
	CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CLightManager() override;

public:
	HRESULT	Initialize_LightManager();
	
	VOID	Update(_float fTimeDelta);
	VOID	UpdateGUI();
	HRESULT	Render_ShadowMap();
	HRESULT	Render_ObjectShadow(const ComPtr<ID3D11ShaderResourceView>& _Diffuse, const ComPtr<ID3D11ShaderResourceView>& _Normal, const ComPtr<ID3D11ShaderResourceView>& _SMRO,
		const ComPtr<ID3D11ShaderResourceView>& _Emissive, const ComPtr<ID3D11ShaderResourceView> _Ambient, const ComPtr<ID3D11ShaderResourceView> _Depth);

	VOID	Bind_EnviromentLight();
	VOID	Bind_DynamicLight();
	
	VOID	Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	VOID	Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range);
	VOID	Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);

	VOID	Clear_DynamicLightList() { m_LightHandleList.clear(); }

	HRESULT	Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);

	const SPtr<CResDynamicTexture2D>& Get_CombinedResource() { return m_pUAVComBinedOutput; }

#ifdef _DEBUG
public:
	HRESULT	Initialize_DebugRender();
	HRESULT Render_DebugIcon();
#endif
private:
	ComPtr<ID3D11Device>				m_pDevice				= { nullptr };
	ComPtr<ID3D11DeviceContext>			m_pContext				= { nullptr };

	std::vector<CHandle>				m_LightHandleList;

	ComPtr<ID3D11ShaderResourceView>	m_pIrridianceSRV		= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pPreFilterSRV			= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pLUTSRV				= { nullptr };


	SPtr<CResVertexShader>				m_pResVertexShader		= { nullptr };
	SPtr<CResPixelShader>				m_pResPixelShader		= { nullptr };
	SPtr<CResQuadTexBuffer>				m_pResLightTexBuffer	= { nullptr };

	SPtr<CResComputeShader>				m_pShadowComputeShader	= { nullptr };
	SPtr<CResComputeShader>				m_pPBRComputeShader		= { nullptr };
	SPtr<CResDynamicTexture2D>			m_pUAVComBinedOutput	= { nullptr };
	SPtr<CResViewPort>					m_pShadowViewPort{};

	std::vector<CGameObject*>			m_pRenderable_StaticObjectList{};
	std::vector<CGameObject*>			m_pRenderable_DynamicObjectList{};

	std::vector<ID3D11DepthStencilView*>	m_pShadowMapList;
	ComPtr<ID3D11Texture2D>					m_pShadowTextureArray = { nullptr };
	ComPtr<ID3D11ShaderResourceView>		m_pShadowSRV = { nullptr };

	// Light 위치 나타내는 용 아이콘 텍스쳐
	SPtr<CResTexture2D>		m_pResDirectionalLightTexture2D = { nullptr };
	SPtr<CResTexture2D>		m_pResPointLightTexture2D		= { nullptr };
	SPtr<CResTexture2D>		m_pResSpotLightTexture2D		= { nullptr };

public:
	static UPtr<CLightManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};
NS_END
