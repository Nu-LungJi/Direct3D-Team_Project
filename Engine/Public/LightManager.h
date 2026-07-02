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

	VOID	Bind_EnviromentLight();
	VOID	Bind_DynamicLight();

private:
	ComPtr<ID3D11Device>		m_pDevice = { nullptr };
	ComPtr<ID3D11DeviceContext> m_pContext = { nullptr };

	std::vector<SPtr<CLight>>	m_LightList;

	ComPtr<ID3D11ShaderResourceView>	m_IrridianceSRV	= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_PreFilterSRV	= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_LUTSRV		= { nullptr };

public:
	static UPtr<CLightManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};
NS_END