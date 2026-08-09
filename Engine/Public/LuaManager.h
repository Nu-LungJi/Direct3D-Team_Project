#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CLuaWatcher;
class CResLuaScript;
class CLuaScriptInstance;

class ENGINE_DLL CLuaManager final : public CEngineBase
{
private:
	CLuaManager();
	~CLuaManager();

public:
	void UpdateGUI();
	void Update(_float fTimeDelta);

	SPtr<CLuaScriptInstance> CreateScriptInstance(
		const SPtr<CResLuaScript>& pScript,
		const _string& sDebugName = {});

	// 매니저 전역 VM에서 일회성 코드를 실행한다.
	// 객체별 스크립트는 CreateScriptInstance()를 사용한다.
	HRESULT Execute(
		const std::string& script,
		const std::string& chunkName = "InlineScript");

	template<typename T>
	void SetValue(std::string_view name, T&& value)
	{
		m_Lua.globals()[name] = std::forward<T>(value);
	}

	template<typename T>
	bool GetValue(std::string_view name, T& outValue)
	{
		sol::object object = m_Lua.globals()[name];
		if (!object.valid() || !object.is<T>())
			return false;

		outValue = object.as<T>();
		return true;
	}

	HRESULT Compile(const std::string& script);

	void RegisterExtension(std::function<void(sol::state&)> extensionFunc)
	{
		extensionFunc(m_Lua);
	}

	template<typename T>
	void RegisterType()
	{
		m_TypeRegistry[StringID{ T::StaticType }] = [this](CEngineBase* pBase) -> sol::object
		{
			T* pCasted = Cast<T>(pBase);
			if (!pCasted)
				return sol::nil;

			return sol::make_object(m_Lua, pCasted);
		};
	}

	void UpdateHotReload();
	void OnFileChanged(const std::string& path);

private:
	HRESULT Initialize();
	HRESULT Initialize_PrintBinding();
	HRESULT Initialize_RegistType();
	HRESULT Initialize_DebuggerBinding();
	HRESULT Initialize_MathBinding();
	HRESULT Initialize_GameInstanceBindnig();
	HRESULT Initialize_ClassBindnig();
	HRESULT Initialize_DefineBinding();

	friend class CLuaScriptInstance;

	sol::table CreateTable();
	HRESULT ExecuteModule(
		const std::string& sSource,
		const std::string& sChunkName,
		const sol::table& Context,
		sol::environment& OutEnvironment,
		sol::table& OutExports);

private:
	std::unordered_map<StringID, std::function<sol::object(CEngineBase*)>> m_TypeRegistry{};
	sol::state m_Lua{};
	std::vector<WPtr<CLuaScriptInstance>> m_ScriptInstances{};
	UPtr<CLuaWatcher> m_pLuaWatcher{};

public:
	static UPtr<CLuaManager> Create();

protected:
	void Free() override;
};

NS_END
