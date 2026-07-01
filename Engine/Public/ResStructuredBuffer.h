#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResStructuredBuffer final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResStructuredBuffer, CResource)
public:
	typedef struct tagDesc
	{
		uint32_t iNumElements = 0;          // 파티클 최대 개수 (예: 1000)
		uint32_t iStructureByteStride = 0;  // 구조체 1개의 크기 (sizeof(VTX_DROP_BLOCK_INSTANCED_DATA))
		void* pInitialData = nullptr;    // 초기 데이터 배열 포인터 (생략 시 nullptr)
	} DESC;
private:
	explicit CResStructuredBuffer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResStructuredBuffer() override;

public:
	ComPtr<ID3D11Buffer> GetBuffer() const { return m_pBuffer; }
	ComPtr<ID3D11Buffer>& GetBufferRef() { return m_pBuffer; }
	ComPtr<ID3D11ShaderResourceView>  GetSRV() const { return m_pSRV; }
	ComPtr<ID3D11UnorderedAccessView> GetUAV() const { return m_pUAV; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11Buffer> m_pBuffer{};
	ComPtr<ID3D11ShaderResourceView>  m_pSRV = nullptr;
	ComPtr<ID3D11UnorderedAccessView> m_pUAV = nullptr;
private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResStructuredBuffer> Create();
};

NS_END