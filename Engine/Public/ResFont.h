#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResFont : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResFont, CResource)

protected:
	explicit CResFont(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResFont() override;

public:
	SpriteBatch* GetSpriteBatch() const { return m_pBatch.get(); }

protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

	std::unique_ptr<SpriteBatch> m_pBatch{};
};

NS_END
