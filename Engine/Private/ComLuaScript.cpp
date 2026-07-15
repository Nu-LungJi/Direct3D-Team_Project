#include "pch.h"
#include "ComLuaScript.h"
#include "GameInstance.h"
#include "LuaEnv.h"

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

	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}

	m_pLuaEnv = CLuaEnv::Create();
	if (!m_pLuaEnv)
	{
		MSG_BOX("CLuaEnv Failed");
		return E_FAIL;
	}

	m_funcScriptLoad = pDesc->funcScriptLoad;
	m_pLuaEnv->SetFuncOnCreateScript([this](CLuaEnv* luaEnv)
		{
			m_OnCreate = m_pLuaEnv->CacheLuaFunction("OnCreate");
			m_OnDestroy = m_pLuaEnv->CacheLuaFunction("OnDestroy");
			m_PriorityUpdate = m_pLuaEnv->CacheLuaFunction("PriorityUpdate");
			m_FixedUpdate = m_pLuaEnv->CacheLuaFunction("FixedUpdate");
			m_Update = m_pLuaEnv->CacheLuaFunction("Update");
			m_LateUpdate = m_pLuaEnv->CacheLuaFunction("LateUpdate");

			if (m_funcScriptCreate)
			{
				m_funcScriptCreate(this);
			}
		});

	m_funcScriptCreate = pDesc->funcScriptCreate;
	m_pLuaEnv->SetFuncOnLoadScript([this](CLuaEnv* luaEnv)
		{
			m_OnCreate = m_pLuaEnv->GetCachedLuaFunction("OnCreate");
			m_OnDestroy = m_pLuaEnv->GetCachedLuaFunction("OnDestroy");
			m_PriorityUpdate = m_pLuaEnv->GetCachedLuaFunction("PriorityUpdate");
			m_FixedUpdate = m_pLuaEnv->GetCachedLuaFunction("FixedUpdate");
			m_Update = m_pLuaEnv->GetCachedLuaFunction("Update");
			m_LateUpdate = m_pLuaEnv->GetCachedLuaFunction("LateUpdate");

			m_pLuaEnv->GetEnv()["self"] = this;
			m_pLuaEnv->GetEnv()["gameObject"] = GetGameObject();
			m_pLuaEnv->GetEnv()["transform"] = &GetGameObject()->GetTransform();

			if (m_funcScriptLoad)
			{
				m_funcScriptLoad(this);
			}
		});
	m_pLuaEnv->LateInitialize(pDesc->pResScript);
	
	return S_OK;
}

void CComLuaScript::PriorityUpdate(_float fTimeDelta)
{
	m_pLuaEnv->CallCacheLuaFunction("PriorityUpdate", fTimeDelta);
	//m_pLuaEnv->CallCacheLuaFunction("PriorityUpdate", fTimeDelta);
	//CGameInstance::Get().LuaCallCacheFunction(m_PriorityUpdate, fTimeDelta);
}

void CComLuaScript::FixedUpdate(_float fTimeDelta)
{
	//m_pLuaEnv->CallCacheLuaFunction("FixedUpdate", fTimeDelta);
	CGameInstance::Get().LuaCallCacheFunction(m_FixedUpdate, fTimeDelta);
}

void CComLuaScript::Update(_float fTimeDelta)
{
	//m_pLuaEnv->CallCacheLuaFunction("Update", fTimeDelta);
	CGameInstance::Get().LuaCallCacheFunction(m_Update, fTimeDelta);
}

void CComLuaScript::LateUpdate(_float fTimeDelta)
{
	//m_pLuaEnv->CallCacheLuaFunction("LateUpdate", fTimeDelta);
	CGameInstance::Get().LuaCallCacheFunction(m_LateUpdate, fTimeDelta);
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
	CComponent::Free();
}
