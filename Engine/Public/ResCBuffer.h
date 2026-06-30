#pragma once
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResCBuffer final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResCBuffer, CResource)

public:
	typedef struct tagCBufferDesc
	{
		UINT byteWidth = 0;                     // 버퍼 크기
		D3D11_USAGE usage = D3D11_USAGE_DYNAMIC;   // 버퍼 사용 방식
		UINT bindFlags = D3D11_BIND_CONSTANT_BUFFER;
		UINT cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;    // CPU 접근 플래그
		const void* pInitData = nullptr;        // 초기 데이터 포인터
	} CBUFFER_DESC;


private:
	explicit CResCBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResCBuffer() override;

public:
	ComPtr<ID3D11Buffer> GetCBuffer() const { return m_pCBuffer; }
	ComPtr<ID3D11Buffer>& GetCBufferRef() { return m_pCBuffer; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	HRESULT CreateBuffer(const CBUFFER_DESC& desc);

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
	ComPtr<ID3D11Buffer> m_pCBuffer{};   // 실제 GPU CB

	UINT m_byteWidth{};
	D3D11_USAGE m_usage{};
	UINT m_bindFlags{};
	UINT m_cpuAccessFlags{};

public:
	static SPtr<CResCBuffer> Create();
};

NS_END