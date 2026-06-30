#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResSamplerState final: public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResSamplerState, CResource)


private:
	explicit CResSamplerState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResSamplerState() override;

public:
	ComPtr<ID3D11SamplerState> GetSamplerState() const { return m_Sampler; }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11SamplerState> m_Sampler{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResSamplerState> Create();
};

NS_END