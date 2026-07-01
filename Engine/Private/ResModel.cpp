#include "pch.h"
#include "ResModel.h"
#include <fstream>

NS_USING(Engine)

CResModel::CResModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResModel::~CResModel()
{
}

HRESULT CResModel::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;

	{

	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

SPtr<CResModel> CResModel::Create(const _string& sPath)
{
	return ToSPtr(new CResModel{ sPath });
}
