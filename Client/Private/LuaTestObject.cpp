#include "pch.h"
#include "LuaTestObject.h"

#include "ComLuaScript.h"
#include "GameInstance.h"
#include "ResLuaScript.h"
#include "Resources.h"

NS_USING(Client)

CLuaTestObject::CLuaTestObject() = default;

CLuaTestObject::CLuaTestObject(const CLuaTestObject& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CLuaTestObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CComLuaScript::DESC Desc{};
	Desc.pResScript = CGameInstance::Get().GetResourceFirst<CResLuaScript>(
		ES_EngineResMajorType::PERMANENT_LUA,
		ES_EngineResLuaScript::LUA_TEST);
	if (!Desc.pResScript)
		return E_FAIL;

	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::LUA,
		ES_EngineProtoComponent::Prototype_Component_ComLuaScript,
		"ComLuaScript",
		&Desc,
		&m_pComLuaScript)))
	{
		return E_FAIL;
	}

	return S_OK;
}

void CLuaTestObject::PriorityUpdate(_float fTimeDelta)
{
	m_pComLuaScript->PriorityUpdate(fTimeDelta);
}

void CLuaTestObject::FixedUpdate(_float fTimeDelta)
{
	m_pComLuaScript->FixedUpdate(fTimeDelta);
}

void CLuaTestObject::Update(_float fTimeDelta)
{
	m_pComLuaScript->Update(fTimeDelta);
}

void CLuaTestObject::LateUpdate(_float fTimeDelta)
{
	m_pComLuaScript->LateUpdate(fTimeDelta);
	GetTransform().Update();
}

UPtr<CLuaTestObject> CLuaTestObject::Create()
{
	auto pInstance = ToUPtr(new CLuaTestObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CLuaTestObject");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CLuaTestObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CLuaTestObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CLuaTestObject");
		return nullptr;
	}

	return pInstance;
}
