#include "pch.h"
#include "WorldNpc.h"
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
#include "DbgLineRender.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"

//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CWorldNpc::CWorldNpc()
{
}

CWorldNpc::~CWorldNpc()
{
}

void CWorldNpc::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CWorldNpc::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CWorldNpc::Initialize(void* pArg)
{
	auto NpcDesc = static_cast<WORLD_AGENT_DESC*>(pArg);
	ReadySound();
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 10;

	

	if (FAILED(Ready_Fsm(NpcDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}

	Ready_BBKeyValue(NpcDesc);

	_float3 vRot = NpcDesc->vRot;
	_matrix matRot = XMMatrixRotationX(XMConvertToRadians(vRot.x))
		* XMMatrixRotationY(XMConvertToRadians(vRot.y)) * XMMatrixRotationZ(XMConvertToRadians(vRot.z));
	_vector vFinalRot = XMQuaternionRotationMatrix(matRot);

	GetTransform().SetQuaternion(vFinalRot);
	GetTransform().SetScale(NpcDesc->vScale);

	if(nullptr != m_pComSphereCol)
		m_pComSphereCol->SetQueryEnabled(true);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);

	
	return S_OK;
}
void CWorldNpc::ReadySound()
{
	m_SoundTable["WatchOut"] = { "./Resources/SampleClient/Sound/NPC/watch-out-voice-352456.mp3",
	 "./Resources/SampleClient/Sound/NPC/HitHuman.wav" };
}
HRESULT CWorldNpc::Ready_Fsm(const _string& LevelTag)
{

	return S_OK;
}
void CWorldNpc::Ready_BBKeyValue(WORLD_AGENT_DESC* pDesc)
{
	auto pBB = Get_BlackBoard();

	pBB->Set_Value<_float3>(NPC_KEY::STARTPOS, pDesc->vStartPos);
	pBB->Set_Value<_float3>(NPC_KEY::ENDPOS, pDesc->vEndPos);
	pBB->Set_Value<_float>(NPC_KEY::SPEED, pDesc->fSpeed);
	pBB->Set_Value<AGENT_STATE>(NPC_KEY::STATE, AGENT_STATE::IDLE);

}
void CWorldNpc::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame)
	{
		SetPendingDestroy();
		return;
	}
	__super::PriorityUpdate(fTimeDelta);
}
int32_t CWorldNpc::Find_AnimIndex(const _string& AnimName)
{
	auto pModel = m_pComModelInstance->GetModel();
	if (nullptr == pModel) return -1;

	auto pAnims = pModel->GetAnimations();
	if (pAnims.empty()) return -1;

	for (size_t i = 0; i < pAnims.size(); ++i)
	{
		if (pAnims[i]->GetAnimName() == AnimName)
			return i;
	}

	return -1;
}
void CWorldNpc::Update(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	__super::Update(fTimeDelta);

}

void CWorldNpc::FixedUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;

	if(nullptr != m_pCharacterMotor)
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CWorldNpc::LateUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	__super::LateUpdate(fTimeDelta);

}


_bool CWorldNpc::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING || eType == PLAYER_SKILL_TYPE::ABRA)
		m_iHp = 0;

	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

	Damaged(eType);
	if (eType == PLAYER_SKILL_TYPE::ATTACK)
	{
		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
		{
			if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hUIController))
			{
				pUIController->AddFinisher(2.f);
			}
		}
	}
	return true;

}


void CWorldNpc::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

E::UPtr<CWorldNpc> CWorldNpc::Create()
{
	auto pInstance = E::ToUPtr(new CWorldNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWorldNpc");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWorldNpc::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWorldNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWorldNpc");
		return nullptr;
	}

	return pInstance;
}
