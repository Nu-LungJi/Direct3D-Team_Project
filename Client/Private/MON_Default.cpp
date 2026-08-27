#include "pch.h"
#include "Mon_Default.h"
#include "Monster.h"
NS_USING(Client)
CMon_Default::CMon_Default()
{
}

CMon_Default::~CMon_Default()
{
}
HRESULT CMon_Default::Initialize()
{

	return S_OK;
}
void CMon_Default::Enter(CStateMachine* pStateMachine)
{
	CMonster* pMon = pStateMachine->GetOwner<CMonster>();

	if (nullptr == pMon) return;

}

void CMon_Default::Exit(CStateMachine* pStateMachine)
{
	auto pMonster = pStateMachine->GetOwner<CMonster>();
	if (nullptr == pMonster) return;


}

void CMon_Default::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CMon_Default::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
}


SPtr<CMon_Default> CMon_Default::Create()
{
	auto pInstance = ToSPtr(new CMon_Default{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CMon_Default");
		return nullptr;
	}

	return pInstance;
}
