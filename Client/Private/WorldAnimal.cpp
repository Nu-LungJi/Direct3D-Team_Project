#include "pch.h"
#include "WorldAnimal.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CWorldAnimal::CWorldAnimal()
{
}


CWorldAnimal::~CWorldAnimal()
{
}

void CWorldAnimal::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CWorldAnimal::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CWorldAnimal::Initialize(void* pArg)
{
	auto WorldAgentDesc = static_cast<WORLD_AGENT_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}

	m_iHp = m_iMaxHp = 10;

	_float3 vRot = WorldAgentDesc->vRot;
	_matrix matRot = XMMatrixRotationX(XMConvertToRadians(vRot.x))
		* XMMatrixRotationY(XMConvertToRadians(vRot.y)) * XMMatrixRotationZ(XMConvertToRadians(vRot.z));
	_vector vFinalRot = XMQuaternionRotationMatrix(matRot);

	GetTransform().SetQuaternion(vFinalRot);
	GetTransform().SetScale(WorldAgentDesc->vScale);
	
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);

	auto pBB = Get_BlackBoard();

	pBB->Set_Value<_float3>(NPC_KEY::STARTPOS, WorldAgentDesc->vStartPos);
	pBB->Set_Value<_float3>(NPC_KEY::ENDPOS, WorldAgentDesc->vEndPos);
	pBB->Set_Value<_float>(NPC_KEY::SPEED, WorldAgentDesc->fSpeed);
	pBB->Set_Value<AGENT_STATE>(NPC_KEY::STATE, AGENT_STATE::IDLE);
	return S_OK;
}

void CWorldAnimal::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
}

void CWorldAnimal::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);


}

void CWorldAnimal::FixedUpdate(E::_float fTimeDelta)
{
	if(nullptr != m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CWorldAnimal::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}

void CWorldAnimal::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

E::UPtr<CWorldAnimal> CWorldAnimal::Create()
{
	auto pInstance = E::ToUPtr(new CWorldAnimal{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWorldAnimal");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWorldAnimal::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWorldAnimal{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWorldAnimal");
		return nullptr;
	}

	return pInstance;
}
