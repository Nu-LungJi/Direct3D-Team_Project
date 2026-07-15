#pragma once
#include "Engine_Defines.h"
#include "Light.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLightManager final : public CEngineBase {
private:
	CLightManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CLightManager() override;

public:
	HRESULT	Initialize_LightManager();
	VOID	Update(_float fTimeDelta);
	VOID	UpdateGUI();

	VOID	Bind_EnviromentLight();
	VOID	Bind_DynamicLight();
	
	VOID	Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	VOID	Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range);
	VOID	Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);

	VOID	Clear_DynamicLightList() { m_LightHandleList.clear(); }

private:
	ComPtr<ID3D11Device>				m_pDevice		= { nullptr };
	ComPtr<ID3D11DeviceContext>			m_pContext		= { nullptr };

	std::vector<CHandle>				m_LightHandleList;

	ComPtr<ID3D11ShaderResourceView>	m_IrridianceSRV	= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_PreFilterSRV	= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_LUTSRV		= { nullptr };

public:
	static UPtr<CLightManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};
NS_END
