#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResDynamicBuffer final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResDynamicBuffer, CResource)
public:
	typedef struct tagDesc
	{
		D3D11_BUFFER_DESC desc{};
		D3D11_SUBRESOURCE_DATA subResource{};
	} DESC;
private:
	explicit CResDynamicBuffer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResDynamicBuffer() override;

public:
	ComPtr<ID3D11Buffer> GetBuffer() const { return m_pBuffer; }

	ComPtr<ID3D11Buffer>& GetBufferRef() { return m_pBuffer; }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11Buffer> m_pBuffer{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResDynamicBuffer> Create();
};

NS_END