#pragma once

#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResJson final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResJson, CResource)

private:
	explicit CResJson(const _string& sPath);
	~CResJson() override;

public:
	const nlohmann::json& Get_Json() const { return m_json; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	nlohmann::json m_json{};

public:
	static SPtr<CResJson> Create(const _string& sPath);
};

NS_END