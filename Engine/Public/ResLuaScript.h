#pragma once
#include "ResLua.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResLuaScript final : public CResLua
{
public:
	struct DESC {
	};
public:
	DECLARE_DERIVED_TYPE(CResLuaScript, CResLua)

protected:
	CResLuaScript(const _string& sPath);
	~CResLuaScript() override;

public:
	const std::string& GetSource() const { return m_Source; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	std::string m_Source;
	sol::load_result m_Compiled;

public:
	static SPtr<CResLuaScript> Create(const _string& sPath);
	static SPtr<CResLuaScript> CreateAndLoad(const _string& sPath);
};

NS_END
