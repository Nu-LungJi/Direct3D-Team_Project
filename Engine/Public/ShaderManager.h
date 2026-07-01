#pragma once
#include "Engine_Defines.h"
#include "ResCBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CShaderManager final : public CEngineBase {
private:
	CShaderManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CShaderManager() override;

public:
	template<typename T>
	HRESULT		Bind_ConstantBuffer(T _Argument, SPtr<CResCBuffer> _Buffer);

private:
	ComPtr<ID3D11Device>		m_pDevice	= { nullptr };
	ComPtr<ID3D11DeviceContext> m_pContext	= { nullptr };

public:
	static UPtr<CShaderManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
template<typename T>
inline HRESULT CShaderManager::Bind_ConstantBuffer(T _Argument, SPtr<CResCBuffer> _Buffer) {
    D3D11_MAPPED_SUBRESOURCE MRES;
    if (SUCCEEDED(m_pContext->Map(_Buffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
    {
        T CBStructure = _Argument;
        memcpy(MRES.pData, &CBStructure, sizeof(T));
        m_pContext->Unmap(_Buffer->GetCBuffer().Get(), 0);
    }
	return S_OK;
}