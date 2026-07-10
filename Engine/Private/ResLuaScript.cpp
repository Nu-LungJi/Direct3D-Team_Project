#include "pch.h"
#include "ResLuaScript.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResLuaScript::Load(const std::any& arg)
{
	auto desc = std::any_cast<CResLuaScript::DESC>(&arg);

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	{
		m_eState = STATE::LOADING;

		std::ifstream file(GetPath(), std::ios::binary);
		if (!file.is_open())
			return E_FAIL;

		m_Source.assign(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());

		if (CGameInstance::Get().LuaCompile(m_Source))
		{
			MSG_BOX("Lua Compile Failed See Log");
			m_Source = "";
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResLuaScript::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResLuaScript::CResLuaScript(const _string& sPath)
	: CResLua{ sPath }
{
}

CResLuaScript::~CResLuaScript()
{
}

SPtr<CResLuaScript> CResLuaScript::Create(const _string& sPath)
{
	return ToSPtr(new CResLuaScript{ sPath });
}

SPtr<CResLuaScript> CResLuaScript::CreateAndLoad(const _string& sPath)
{
	auto pRes = CResLuaScript::Create(sPath);
	if (FAILED(pRes->Load()))
	{
		return nullptr;
	}
	return pRes;
}
