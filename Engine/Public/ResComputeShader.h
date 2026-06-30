#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResComputeShader final : public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResComputeShader, CResShader)

public:
	ComPtr<ID3D11ComputeShader> GetComputeShader() const { return m_pComputeShader; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResComputeShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResComputeShader() override;

private:
	ComPtr<ID3D11ComputeShader> m_pComputeShader{};

public:
	static SPtr<CResComputeShader> Create(const _string& sPath);
};

NS_END