#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CLuaManager final : public CEngineBase
{
private:
	CLuaManager();
	~CLuaManager();

public:
	void UpdateGUI();

public:

private:
	HRESULT Initialize();

public:
	bool HasFunction(
		sol::environment& env,
		std::string_view function) const;
	HRESULT Execute(const std::string& script, const sol::environment& env);
	sol::environment CreateEnvironment();
	template<typename... Args>
	HRESULT Call(sol::environment& env, std::string_view function, Args&&... args);

	template<typename T>
	void SetValue( sol::environment& env, std::string_view name, T&& value)
	{
		env[std::string(name)] = std::forward<T>(value);
	}

	template<typename T>
	bool GetValue( sol::environment& env, std::string_view name, T& value)
	{
		sol::object obj = env[std::string(name)];

		if (!obj.valid())
			return false;

		if (!obj.is<T>())
			return false;

		value = obj.as<T>();
		return true;
	}

	HRESULT Compile(const std::string& script);

	template<typename Ret, typename... Args>
	HRESULT Call(sol::environment& env, std::string_view function, Ret& ret, Args&&... args)
	{
		sol::object obj = env[std::string(function)];

		if (!obj.valid())
			return S_FALSE;

		sol::protected_function func =
			obj.as<sol::protected_function>();

		auto result =
			func(std::forward<Args>(args)...);

		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(err.what());
			return E_FAIL;
		}

		ret = result.get<Ret>();

		return S_OK;
	}

	bool IsEnvValid(const sol::environment& env) const { return env.valid(); }
	bool HasValue(const sol::environment& env, std::string_view name) const
	{
		sol::object obj = env[std::string(name)];
		return obj.valid() && obj.get_type() != sol::type::lua_nil;
	}
	void RemoveValue(sol::environment& env, std::string_view name) { env[std::string(name)] = sol::lua_nil; }
	void EnvDump(const sol::environment& env) const;
	void EnvClear(sol::environment& env);

private:
	// 엔진에서 사용하는 유일한 VM
	sol::state m_Lua;

public:
	static UPtr<CLuaManager> Create();
};

NS_END

template<typename... Args>
inline HRESULT CLuaManager::Call(
	sol::environment& env,
	std::string_view function,
	Args&&... args)
{
	sol::object obj = env[std::string(function)];

	if (!obj.valid() || obj.get_type() != sol::type::function)
		return S_FALSE;   // 함수가 없으면 정상적으로 무시

	sol::protected_function func = obj.as<sol::protected_function>();

	sol::protected_function_result result =
		func(std::forward<Args>(args)...);

	if (!result.valid())
	{
		sol::error err = result;
		OutputDebugStringA((std::string(err.what()) + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}
