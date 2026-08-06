#include "pch.h"
#include "Edg_Spawn.h"
#include "EnderDragon.h"
#include "EnderDragon_State.h"
NS_USING(Client)
CEdg_Spawn::CEdg_Spawn()
{
}

CEdg_Spawn::~CEdg_Spawn()
{
}

void CEdg_Spawn::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon)
		return;
}

void CEdg_Spawn::Exit(CStateMachine* pStateMachine)
{
}

void CEdg_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	CEnderDragon_State* pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	
	if (nullptr == pDragonFsm)
		return;
	pDragonFsm->Request_State(EDG_STATE::COMBAT);

}
SPtr<CEdg_Spawn> CEdg_Spawn::Create()
{
	auto pInstance = ToSPtr(new CEdg_Spawn{});
	if (!pInstance)
	{
		MSG_BOX("Failed to create CEdg_Spawn");
		return nullptr;
	}

	return pInstance;
}
