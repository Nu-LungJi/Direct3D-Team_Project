#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResTessDomainShader final : public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResTessDomainShader, CResShader)

public:
	ComPtr<ID3D11DomainShader> GetTessDomainShader() const { return m_pTessDomainShader; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResTessDomainShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTessDomainShader() override;

private:
	ComPtr<ID3D11DomainShader> m_pTessDomainShader{};

public:
	static SPtr<CResTessDomainShader> Create(const _string& sPath);
};

NS_END