#pragma once
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResTexture: public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResTexture, CResource)

protected:
	explicit CResTexture(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTexture() override;

protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

};

NS_END