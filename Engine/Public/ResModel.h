
#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResModel final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModel, CResource)

private:
	explicit CResModel(const _string& sPath);
	~CResModel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

public:
	static SPtr<CResModel> Create(const _string& sPath);
};

NS_END