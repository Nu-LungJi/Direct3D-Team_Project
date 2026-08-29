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
	if (auto* pSoundManager =
		E::CGameInstance::Get().GetSoundManager())
	{
		pSoundManager->Stop(m_iSoundID);
	}

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

	_string soundPath{};
	uint32_t iRand = RandInt(0, 3);
	switch (iRand) {
	case 0:
		soundPath = "./Resources/SampleClient/Sound/NPC/NpcTalk1.mp3";
		break;
	case 1:
		soundPath = "./Resources/SampleClient/Sound/NPC/NpcTalk2.mp3";

		break;
	case 2:
		soundPath = "./Resources/SampleClient/Sound/NPC/NpcTalk3.mp3";
		break;
	case 3:
		soundPath = "./Resources/SampleClient/Sound/NPC/NpcTalk4.mp3";
		break;
	default:
		break;
	}
	m_iSoundID = CGameInstance::Get().GetSoundManager()->Play3D(
		soundPath,
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = 5.f,
			.fMaxDistance = 30.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.75f,
			.fPitch = 1.f,
			.iPriority = 96,
			.bLoop = true
		});

	return S_OK;
}
HRESULT CWorldNpc::Ready_Fsm(const _string& LevelTag)
{
	//CMon_State::DESC Desc{};
	//if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Mon_FSM", "Mon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	//
	//
	//if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CWorldNpc_Spawn::Create(LevelTag))) return E_FAIL;
	//
	//if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CWorldNpc_Combat::Create(LevelTag))) return E_FAIL;
	//
	//if (false == m_pFsm->Add_State(MON_STATE::HIT, CWorldNpc_Hit::Create(LevelTag, this))) return E_FAIL;
	//if (false == m_pFsm->Add_State(MON_STATE::DEAD, CWorldNpc_Dead::Create())) return E_FAIL;
	//
	//if (false == m_pFsm->Initialize_State(MON_STATE::SPAWN)) return E_FAIL;


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

	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (nullptr == pSoundManager || m_iSoundID == INVALID_SOUND_ID)
		return;

	if (!pSoundManager->IsValidSound(m_iSoundID) ||
		!pSoundManager->IsPlaying(m_iSoundID))
	{
		m_iSoundID = INVALID_SOUND_ID;
		return;
	}

	pSoundManager->Set3DAttributes(
		m_iSoundID,
		GetTransform().GetPosition());

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
