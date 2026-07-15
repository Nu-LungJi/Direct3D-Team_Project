#include "pch.h"
#include "LuaEnv.h"

NS_USING(Engine)
CLuaEnv::CLuaEnv()
{
}
CLuaEnv::~CLuaEnv()
{
}
void CLuaEnv::LuaScriptRelod()
{
	LuadScriptLoad();
}

void CLuaEnv::LuadScriptLoad()
{
	m_Environment = CGameInstance::Get().LuaCreateEnvironment();

	m_Environment["__ScriptPath"] = m_pResLuaScript->GetPath();

	CGameInstance::Get().LuaScriptExecute(m_pResLuaScript->GetSource(), m_Environment, m_pResLuaScript->GetPath());

	for (auto& [k, v] : m_mapCachedLuaFunctions)
	{
		CacheLuaFunction(k);
	}

	m_Environment["self"] = this;

	if (m_funcOnLoadScript)
	{
		m_funcOnLoadScript(this);
	}
}

sol::protected_function CLuaEnv::CacheLuaFunction(const std::string& funcName)
{
	auto f = CGameInstance::Get().LuaCacheFunction(m_Environment, funcName);
	m_mapCachedLuaFunctions[funcName] = f;
	return f;
}

sol::protected_function CLuaEnv::GetCachedLuaFunction(const std::string& funcName) const
{
	auto iter = m_mapCachedLuaFunctions.find(funcName);
	if (iter != m_mapCachedLuaFunctions.end())
	{
		return iter->second;
	}
	return sol::protected_function();
}

HRESULT  CLuaEnv::LateInitialize(SPtr<CResLuaScript> pLuaScript)
{
	if (!pLuaScript)
	{
		return E_FAIL;
	}
	m_pResLuaScript = pLuaScript;
	m_pGameInstance = &CGameInstance::Get();

	CGameInstance::Get().LuaRegisterComponent(m_pResLuaScript->GetPath(), this);

	LuadScriptLoad();

	if (m_funcOnCreateScript)
	{
		m_funcOnCreateScript(this);
	}
	return S_OK;
}

SPtr<CLuaEnv> CLuaEnv::Create()
{
	auto pInstance = ToSPtr(new CLuaEnv{});
	return pInstance;
}

void CLuaEnv::Free()
{
	CGameInstance::Get().LuaUnregisterComponent(m_pResLuaScript->GetPath(), this);
	CEngineBase::Free();
}
