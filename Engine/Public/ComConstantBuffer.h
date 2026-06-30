#pragma once

#include "Component.h"
#include "ResCBuffer.h"

NS_BEGIN(Engine)
//class CResCBuffer;
class ENGINE_DLL CComConstantBuffer : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		std::pair<StringID, StringID> cBufferId;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComConstantBuffer, CComponent)

public:
	virtual void UpdateGUI();

private:
	explicit CComConstantBuffer();
	~CComConstantBuffer() override;

public:
	ID3D11Buffer* GetBuffer() const { return m_pResCBuffer->GetCBufferRef().Get(); }
	ID3D11Buffer** GetAdressOfBuffer() const { return m_pResCBuffer->GetCBufferRef().GetAddressOf(); }
	HRESULT MapDiscard(ID3D11DeviceContext* pContext, void* pData, size_t size);

private:
	HRESULT Initialize(void* pArg) override;

private:
	SPtr<CResCBuffer> m_pResCBuffer{};

public:
	static UPtr<CComConstantBuffer> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END