#include "pch.h"
#include "BossTMB.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Mon_Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "StarBurst.h"
NS_USING(Client)

CBossTMB::CBossTMB()
{
}

CBossTMB::~CBossTMB()
{
}

void CBossTMB::UpdateGUI()
{
	__super::UpdateGUI();

	if (ImGui::TreeNode("Flag"))
	{
		struct GuiView
		{
			uint32_t iValue{};
			const _char* pName{};
		};
#define X(name, value) value, #name,
		const GuiView Flags[] = { BTFLAG_M };
#undef X

		for (uint32_t i = 0; i < std::size(Flags); ++i)
		{
			ImGui::Text(Flags[i].pName); ImGui::SameLine();
			ImGui::Text(true == m_pBeHavior->Check_Flag(Flags[i].iValue) ? ": TRUE" : " FALSE");
		}
			
		ImGui::TreePop();
	}
}

HRESULT CBossTMB::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CBossTMB::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 100;

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.vPosition = MonDesc->vPos;
		Desc.tFilter = MonDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	CComBeHavior::BEHAVIOR_DESC Desc{};
	Desc.OwnerName = "Com_BT";
	Desc.LoadPath = MonDesc->BeHaviorTag;
	if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
	{
		return E_FAIL;
	};
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = MonDesc->ReSourceTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}

	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		};
	}

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(BOSSTOMB_SKILL::SPAWN);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(BOSSTOMB_SKILL::STUMP);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(BOSSTOMB_SKILL::BLUST_START);
	m_MonSkillLists[ATTMON::SLOT3] = ETOUI(BOSSTOMB_SKILL::BLUST_END);
	m_MonSkillLists[ATTMON::SLOT4] = ETOUI(BOSSTOMB_SKILL::BALL);
	m_MonSkillLists[ATTMON::SLOT5] = ETOUI(BOSSTOMB_SKILL::BALL_BREAK);


	m_MonSkillLists[ATTMON::SKIP] = ETOUI(BOSSTOMB_SKILL::SKIP);
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::SPAWN)] = "Boss_Appear";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::STUMP)] = "Boss_GroundCrash";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_START)] = "BossAoeBlustStart";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_END)] = "BossAoeBlustEnd";

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();

	m_pComTransform->SetRotation(XMVectorSet(MonDesc->vRot.x, MonDesc->vRot.y, MonDesc->vRot.z, 0.f), MonDesc->fAngle);
	m_pComTransform->SetScale(XMVectorSet(MonDesc->vScale.x, MonDesc->vScale.y, MonDesc->vScale.z, 0));
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);


	m_eMonType = MONSTER_TYPE::BOSS;

	m_eAttType = ATTMON::END;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);

	return S_OK;
}

void CBossTMB::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

}

void CBossTMB::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bDonMove)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CBossTMB::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	if (CGameInstance::Get().KeyDown(DIK_0)) {
		CBoss_StarBurst::STARBURST_DESC desc{};
		desc.pTargetHandle = m_TargetHandle;
		desc.vStartPosition = { GetTransform().GetPosition() };
		CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossStarBurst, "BossStarBurst", &desc);
	}
}

void CBossTMB::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
}

void CBossTMB::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (m_eAttType != eType) {
		
		uint32_t iSkillNum = Find_SkillNum(eType);
		if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(BOSSTOMB_SKILL::END))
			return;
		//if (m_EffectNames[iSkillNum] == "")
		//	return;

		m_CurEffectName = m_EffectNames[iSkillNum];
		m_eAttType = eType;
		m_fSkillRatio = fSkillRatio;

	}
}

_string CBossTMB::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(BOSSTOMB_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<BOSSTOMB_SKILL>(pValue->second)).data();
}

E::UPtr<CBossTMB> CBossTMB::Create()
{
	auto pInstance = E::ToUPtr(new CBossTMB{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBossTMB");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CBossTMB::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBossTMB{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBossTMB");
		return nullptr;
	}

	return pInstance;
}
