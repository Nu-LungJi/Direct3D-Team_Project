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
	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}

	m_Environment = CGameInstance::Get().LuaCreateEnvironment();

	CGameInstance::Get().LuaSetValue(
		m_Environment,
		"__ScriptPath",
		m_pResLuaScript->GetPath());

	CGameInstance::Get().LuaScriptExecute(m_pResLuaScript->GetSource(), m_Environment);

	CacheFunctions("OnCreate", m_OnCreate);
	CacheFunctions("OnDestroy", m_OnDestroy);

	CacheFunctions("PriorityUpdate", m_PriorityUpdate);
	CacheFunctions("FixedUpdate", m_FixedUpdate);
	CacheFunctions("Update", m_Update);
	CacheFunctions("LateUpdate", m_LateUpdate);

	m_Environment["self"] = this;
	m_Environment["gameObject"] = GetGameObject();
	m_Environment["transform"] = GetGameObject()->GetTransform();

	if (m_OnCreate.valid())
		m_OnCreate();
	return S_OK;
}

void CComLuaScript::PriorityUpdate(_float fTimeDelta)
{
	if (m_PriorityUpdate.valid())
		m_PriorityUpdate(fTimeDelta);
}

void CComLuaScript::FixedUpdate(_float fTimeDelta)
{
	if (m_FixedUpdate.valid())
		m_FixedUpdate(fTimeDelta);
}

void CComLuaScript::Update(_float fTimeDelta)
{
	if (m_Update.valid())
		m_Update(fTimeDelta);
}

void CComLuaScript::LateUpdate(_float fTimeDelta)
{
	if (m_LateUpdate.valid())
		m_LateUpdate(fTimeDelta);
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
