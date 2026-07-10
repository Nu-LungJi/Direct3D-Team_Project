#include "pch.h"
#include "LuaManager.h"
#include "GameInstance.h"

#include "ComLuaScript.h"
#include "GameObject.h"
#include "ComTransform.h"
NS_USING(Engine)

CLuaManager::CLuaManager()
{
}

CLuaManager::~CLuaManager()
{
}

void CLuaManager::UpdateGUI()
{

	ImGui::Begin("CLuaManager");
	
	ImGui::End();

}

HRESULT CLuaManager::Initialize()
{
	// Lua 기본 라이브러리
	m_Lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::utf8
	);
	sol::protected_function tostring = m_Lua["tostring"];
	m_Lua.set_function("print",
		[tostring](sol::variadic_args args) mutable
		{
			std::ostringstream oss;
			oss << "[LuaManager] ";

			bool first = true;

			

			for (auto value : args)
			{
				if (!first)
					oss << '\t';

				first = false;

				auto result = tostring(value);

				if (result.valid())
					oss << result.get<std::string>();
				else
					oss << "<error>";
			}

			oss << '\n';

			OutputDebugStringA(oss.str().c_str());
		});

	m_Lua.script(R"(
		print("Hello Lua")
	)");

	auto result = m_Lua.safe_script("return 10 + 20");
	if (result.valid())
	{
		int value = result.get<int>();
	}
	
	m_Lua.new_usertype<CComLuaScript>(
		"ComLuaScript",
		sol::meta_function::to_string,
		[](CComLuaScript&)
		{
			return std::string("ComLuaScript");
		}
	);
	m_Lua.new_usertype<CGameObject>(
		"GameObject",
		"GetObjectTag", &CGameObject::GetObjectTag,
		"GetTypeString", &CGameObject::GetTypeString,
		sol::meta_function::to_string,
		[](CGameObject& obj)
		{
			return std::string("GameObject(") + std::string{ obj.GetObjectTag() } + ")";
		}
	);

	m_Lua.new_usertype<CComTransform>(
		"ComTrnasform",
		"GetPosition", &CComTransform::GetPosition,
		sol::meta_function::to_string,
		[](CComTransform& obj)
		{
			return std::string("ComTransform");
		}
	);
	return S_OK;
}

bool CLuaManager::HasFunction(sol::environment& env, std::string_view function) const
{
	sol::object obj = env[std::string(function)];

	return obj.valid() &&
		obj.get_type() == sol::type::function;
}

HRESULT CLuaManager::Execute(const std::string& script, const sol::environment& env)
{
	auto result = m_Lua.safe_script(script, env);
	if (!result.valid())
	{
		return E_FAIL;
	}

	//int value = result.get<int>();
	return S_OK;
}

// object 하나당 하나만들기 
sol::environment CLuaManager::CreateEnvironment()
{
	return sol::environment(m_Lua, sol::create, m_Lua.globals());
}


HRESULT CLuaManager::Compile(const std::string& script)
{
	sol::load_result result = m_Lua.load(script);

	if (!result.valid())
	{
		sol::error err = result;
		auto msg = std::string("[LuaError]") + err.what();
		OutputDebugStringA(msg.c_str());
		OutputDebugStringA("\n");

		return E_FAIL;
	}

	return S_OK;
}

void CLuaManager::EnvDump(const sol::environment& env) const
{
	OutputDebugStringA("========== Lua Environment ==========\n");

	sol::table table = env;

	for (auto& kv : table)
	{
		sol::object key = kv.first;
		sol::object value = kv.second;

		if (!key.is<std::string>())
			continue;

		std::string line = key.as<std::string>();
		line += " : ";
		line += sol::type_name(value.lua_state(), value.get_type());
		line += "\n";

		OutputDebugStringA(line.c_str());
	}

	OutputDebugStringA("=====================================\n");
}

void CLuaManager::EnvClear(sol::environment& env)
{
	sol::table table = env;

	for (auto& kv : table)
	{
		if (kv.first.is<std::string>())
			env[kv.first.as<std::string>()] = sol::lua_nil;
	}
}
UPtr<CLuaManager> CLuaManager::Create()
{
	auto pInstance = UPtr<CLuaManager>(new CLuaManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

