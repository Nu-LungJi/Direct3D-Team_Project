#include "pch.h"
#include "ResJson.h"
#include <fstream>

NS_USING(Engine)

CResJson::CResJson(const _string& sPath)
	: CResource{ sPath }
{
}

CResJson::~CResJson()
{
}

HRESULT CResJson::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;

	std::ifstream file(m_sPath);
	if (!file.is_open())
	{
		MSG_BOX_STR(_wstring{ L"CResJson Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		//std::wstring msg = L"CAssetJson Load Failed, Path: " + m_sPath;
		//MSG_BOX_STR(msg.c_str());
		return E_FAIL;
	}

	file >> m_json;

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResJson::Unload(const std::any& arg)
{
	m_json.clear();
	m_eState = STATE::UNLOAD;
	return S_OK;
}

SPtr<CResJson> CResJson::Create(const _string& sPath)
{
	return ToSPtr(new CResJson{ sPath });
}
