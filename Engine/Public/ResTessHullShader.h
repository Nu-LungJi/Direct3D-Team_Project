#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResTessHullShader final : public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResTessHullShader, CResShader)

public:
	ComPtr<ID3D11HullShader> GetTessHullShader() const { return m_pTessHullShader; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResTessHullShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTessHullShader() override;

private:
	ComPtr<ID3D11HullShader> m_pTessHullShader{};

public:
	static SPtr<CResTessHullShader> Create(const _string& sPath);
};

NS_END