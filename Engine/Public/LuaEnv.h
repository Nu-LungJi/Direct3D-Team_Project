#pragma once
#include "Engine_Defines.h"
#include "ResLuaScript.h"
#include "ILuaScriptRelodable.h"

NS_BEGIN(Engine)
class CGameInstance;
class CLuaEnv: public CEngineBase, public ILuaScriptRelodable
{
protected:
	CLuaEnv();
	~CLuaEnv();

public:
	void LuaScriptRelod() override;

protected:
	virtual void LuadScriptLoad();

public:
	sol::protected_function CacheLuaFunction(const std::string& funcName);
	sol::protected_function GetCachedLuaFunction(const std::string& funcName) const;
	template<typename... Args>
	bool CallCacheLuaFunction(const std::string& funcName, Args&&... args)
	{
		if (!m_pGameInstance) return false;
		auto iter = m_mapCachedLuaFunctions.find(funcName);
		if (iter != m_mapCachedLuaFunctions.end())
		{
			return m_pGameInstance->LuaCallCacheFunction(iter->second, std::forward<Args>(args)...);
		}
		return false;
	}

protected:
	std::unordered_map<std::string, sol::protected_function> m_mapCachedLuaFunctions{};

public:
	void SetFuncOnCreateScript(const std::function<void(CLuaEnv*)>& func) { m_funcOnCreateScript = func; };
	void SetFuncOnLoadScript(const std::function<void(CLuaEnv*)>& func) { m_funcOnLoadScript = func; };
protected:
	std::function<void(CLuaEnv*)> m_funcOnCreateScript{};
	std::function<void(CLuaEnv*)> m_funcOnLoadScript{};

protected:
	SPtr<CResLuaScript> m_pResLuaScript{};
public:
	sol::environment& GetEnv() { return m_Environment; }
protected:
	sol::environment m_Environment{};

public:
	virtual HRESULT LateInitialize(SPtr<CResLuaScript> pLuaScript);

	// header 순환참조 피하기위함
private:
	CGameInstance* m_pGameInstance{};
public:
	static SPtr<CLuaEnv> Create();

protected:
	void Free() override;

};

NS_END
