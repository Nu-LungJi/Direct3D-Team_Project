#include "pch.h"
#include "ComLuaScript.h"
#include "GameInstance.h"

NS_USING(Engine)

void CComLuaScript::UpdateGUI()
{
}

CComLuaScript::CComLuaScript()
{
}

CComLuaScript::~CComLuaScript()
{
}

HRESULT CComLuaScript::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc->pResScript)
	{
		MSG_BOX("Lua Com Need Source");
		return E_FAIL;
	}
	m_pResLuaScript = pDesc->pResScript;
	m_funcScriptLoad = pDesc->funcScriptLoad;
	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}

	CGameInstance::Get().LuaRegisterComponent(m_pResLuaScript->GetPath(), this);

	LoadScript();

	if (m_OnCreate.valid())
		m_OnCreate();
	return S_OK;
}

void CComLuaScript::PriorityUpdate(_float fTimeDelta)
{
	if (m_PriorityUpdate.valid())
	{
		auto result = m_PriorityUpdate(fTimeDelta);
		if (!result.valid())
		{
			sol::error err = result;
			std::string errorMsg = "[Lua Script Error] " + std::string(err.what()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
		}
	}
}

void CComLuaScript::FixedUpdate(_float fTimeDelta)
{
	if (m_FixedUpdate.valid())
	{
		auto result = m_FixedUpdate(fTimeDelta);
		if (!result.valid())
		{
			sol::error err = result;
			std::string errorMsg = "[Lua Script Error] " + std::string(err.what()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
		}
	}
}

void CComLuaScript::Update(_float fTimeDelta)
{
	if (m_Update.valid())
	{
		auto result = m_Update(fTimeDelta);
		if (!result.valid())
		{
			sol::error err = result;
			std::string errorMsg = "[Lua Script Error] " + std::string(err.what()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
		}
	}
}

void CComLuaScript::LateUpdate(_float fTimeDelta)
{
	if (m_LateUpdate.valid())
	{
		auto result = m_LateUpdate(fTimeDelta);
		if (!result.valid())
		{
			sol::error err = result;
			std::string errorMsg = "[Lua Script Error] " + std::string(err.what()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
		}
	}
}

void CComLuaScript::Reload()
{
	// 환경을 새로 덮어쓰고 다시 로드
	LoadScript();
}

void CComLuaScript::LoadScript()
{
	m_Environment = CGameInstance::Get().LuaCreateEnvironment();

	CGameInstance::Get().LuaSetValue(
		m_Environment,
		"__ScriptPath",
		m_pResLuaScript->GetPath());

	CGameInstance::Get().LuaScriptExecute(m_pResLuaScript->GetSource(), m_Environment, m_pResLuaScript->GetPath());

	CacheFunctions("OnCreate", m_OnCreate);
	CacheFunctions("OnDestroy", m_OnDestroy);

	CacheFunctions("PriorityUpdate", m_PriorityUpdate);
	CacheFunctions("FixedUpdate", m_FixedUpdate);
	CacheFunctions("Update", m_Update);
	CacheFunctions("LateUpdate", m_LateUpdate);

	m_Environment["self"] = this;
	m_Environment["gameObject"] = GetGameObject();
	m_Environment["transform"] = &GetGameObject()->GetTransform();

	if (m_funcScriptLoad)
	{
		m_funcScriptLoad(this);
	}
}

void CComLuaScript::CacheFunctions(_string_view name, sol::protected_function& out)
{
	sol::object obj = m_Environment[name];
	if (obj.valid() && obj.get_type() == sol::type::function)
		out = obj.as<sol::protected_function>();
}

UPtr<CComLuaScript> CComLuaScript::Create()
{
	auto pInstance = ToUPtr(new CComLuaScript{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComLuaScript");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComLuaScript::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComLuaScript{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComLuaScript");
		return nullptr;
	}
	return pInstance;
}

void CComLuaScript::Free()
{
	// 프로토타입 등록된애들도 프리가 불림
	if (m_pResLuaScript)
	{
		CGameInstance::Get().LuaUnregisterComponent(m_pResLuaScript->GetPath(), this);
	}
	CComponent::Free();
}
