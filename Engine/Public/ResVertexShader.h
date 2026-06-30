#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResVertexShader final: public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResVertexShader, CResShader)

public:
	ComPtr<ID3D11VertexShader> GetVertexShader() const { return m_pVertexShader; }
	ComPtr<ID3D11InputLayout> GetInputLayout() const { return m_pInputLayout; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	ComPtr<ID3D11VertexShader> m_pVertexShader{};
	ComPtr<ID3D11InputLayout> m_pInputLayout{};
	std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputLayout{};

private:
	explicit CResVertexShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResVertexShader() override;

public:
	static SPtr<CResVertexShader> Create(const _string& sPath);
};

NS_END
