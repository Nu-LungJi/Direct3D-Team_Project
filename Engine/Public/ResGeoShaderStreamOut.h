#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResGeoShaderStreamOut final : public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResGeoShaderStreamOut, CResShader)

public:
	typedef struct tagDesc
	{

	}DESC;

public:
	ComPtr<ID3D11GeometryShader> GetGeometryShader() const { return m_pGeometryShader; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResGeoShaderStreamOut(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResGeoShaderStreamOut() override;

private:
	ComPtr<ID3D11GeometryShader> m_pGeometryShader{};

public:
	static SPtr<CResGeoShaderStreamOut> Create(const _string& sPath);
};

NS_END