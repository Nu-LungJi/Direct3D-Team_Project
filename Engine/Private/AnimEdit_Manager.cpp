#include "pch.h"
#include "AnimEdit_Manager.h"
#include  "GameObject.h"

NS_USING(Engine)

CAnimEdit_Manager::CAnimEdit_Manager()
{
}
CAnimEdit_Manager::~CAnimEdit_Manager()
{

}

void CAnimEdit_Manager::Update(_float fTimeDelta)
{

}

HRESULT CAnimEdit_Manager::Initilize()
{

	// 반드시 TestModel이 먼저 생성된 후에 CAnimEdit_Manager를 생성해야 한다.
	// if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestModel","02_TestModel", &Desc))
	

	if (auto layer = CGameInstance::Get().GetGameObjectLayer("LEVEL_TEST"))
	{
		if (!layer->empty())
		{
			auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(layer->front());
			if (pSampleObj)
			{
				m_pTestModel = pSampleObj;
			}
		}
	}




	return S_OK;
}



UPtr<CAnimEdit_Manager> CAnimEdit_Manager::Create()
{
	auto pInstance = UPtr<CAnimEdit_Manager>(new CAnimEdit_Manager{});
	if (FAILED(pInstance->Initilize()))
	{
		MSG_BOX("CAnimEdit_Manager Create Failed");
		return nullptr;
	}
	return pInstance;
}
