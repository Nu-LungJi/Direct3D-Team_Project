#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResShader: public CResource
{
public:
	struct DESC {
		_string sEntryPoint;
		_string sTarget;
	};
public:
	DECLARE_DERIVED_TYPE(CResShader, CResource)

protected:
	explicit CResShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResShader() override;

public:
	void* GetBufferPointer() const { return m_pBlob->GetBufferPointer(); }
	size_t GetBufferSize() const { return m_pBlob->GetBufferSize(); }

protected:
	HRESULT CompileShader(const DESC* _desc = nullptr);

protected:
	_string m_sEntryPoint{};
	_string m_sTarget{};

	ComPtr<ID3DBlob> m_pBlob{};
	ComPtr<ID3DBlob> m_pErrorBlob{};

	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
};

NS_END