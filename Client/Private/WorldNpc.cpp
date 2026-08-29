#include "pch.h"
#include "WorldNpc.h"
#include "NpcRagdollController.h"
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

CWorldNpc::CWorldNpc(const CWorldNpc& Prototype)
	: CWorldAgent{ Prototype }
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
	if (m_pRagdollController)
		m_pRagdollController->UpdateGUI();
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
	GetTransform().Update();

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

	// CWorldAgent는 사망 NPC를 즉시 PendingDestroy 처리한다.
	// 그보다 먼저 랙돌 전환을 예약하고 일반 AI 루프 진입을 막는다.
	const _bool bDeathRequested =
		m_iHp <= 0 ||
		(m_pBeHavior &&
			m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)));
	if (bDeathRequested && m_pRagdollController &&
		!m_pRagdollController->IsTransitioning())
	{
		m_pRagdollController->RequestFromCurrentMotion();
	}

	if (m_pRagdollController &&
		m_pRagdollController->PrePriorityUpdate())
	{
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
	if (!IsRagdollActive())
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

	if (m_pRagdollController &&
		m_pRagdollController->PreFixedUpdate())
	{
		return;
	}

	if(nullptr != m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);

	if (m_pRagdollController)
		m_pRagdollController->PostFixedUpdate();
}
void CWorldNpc::LateUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;

	// 활성 랙돌에서는 CWorldAgent::LateUpdate가 CCT 발 위치로 Transform을
	// 다시 덮지 않도록 렌더 등록만 수행한다.
	if (IsRagdollActive())
	{
		GetTransform().Update();
		if (m_pComModelInstance &&
			m_pModelAnimator &&
			m_pComModelInstance->GetModel() &&
			!m_pComModelInstance->GetModel()->GetAnimations().empty())
		{
			CGameInstance::Get().Add_Instance(
				m_pComModelInstance,
				m_pModelAnimator,
				*GetTransform().GetCombinedWorldMatrix());
		}
		return;
	}

	__super::LateUpdate(fTimeDelta);

}

_bool CWorldNpc::RequestRagdollActivation(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	return m_pRagdollController &&
		m_pRagdollController->RequestActivation(
			vLinearVelocity,
			vAngularVelocityRadians);
}

_bool CWorldNpc::ResetRagdoll()
{
	return m_pRagdollController &&
		m_pRagdollController->Reset();
}

_bool CWorldNpc::IsRagdollActive() const
{
	return m_pRagdollController &&
		m_pRagdollController->IsActive();
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
