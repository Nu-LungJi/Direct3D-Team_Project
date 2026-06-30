#pragma once
#include "ResFont.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResFontCustom final : public CResFont
{
public:
	DECLARE_DERIVED_TYPE(CResFontCustom, CResFont)

public:
	const SpriteFont* GetFont() const { return m_pFont.get(); }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResFontCustom(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResFontCustom() override;

private:
	std::unique_ptr<SpriteFont> m_pFont{};

public:
	static SPtr<CResFontCustom> Create(const _string& sPath);
};

NS_END