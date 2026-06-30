#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResViewPort final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResViewPort, CResource)

public:
	const D3D11_VIEWPORT& GetViewPort() const { return m_ViewPort; }

private:
	explicit CResViewPort();
	~CResViewPort() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	D3D11_VIEWPORT m_ViewPort{};

public:
	static SPtr<CResViewPort> Create();
};

NS_END