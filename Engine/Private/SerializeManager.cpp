#include "pch.h"
#include "SerializeManager.h"

#include "SerDeTestCase.h"

NS_USING(Engine)

CSerializeManager::CSerializeManager()
{
}

CSerializeManager::~CSerializeManager()
{
}

void CSerializeManager::UpdateGUI()
{

}

HRESULT CSerializeManager::Initialize()
{
	return S_OK;
}

UPtr<CSerializeManager> CSerializeManager::Create()
{
	auto pInstance = ToUPtr(new CSerializeManager);
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}
