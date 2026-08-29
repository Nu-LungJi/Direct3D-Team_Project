#include "pch.h"
#include "Troll.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "CollBox.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ComPxBoxCollider.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"
#include "UIController.h"
#include "UIManager.h"
#include "TrollWeapon.h"
//FSM
#include "Mon_State.h"
#include "MON_Default.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
#include "Mon_Dead.h"
#include "Mon_Godae.h"
#include "Troll_Hit.h"
#include "Troll_Combat.h"
#include "Troll_Spawn.h"
#include "Troll_Grogy.h"
#include "PropBarrel.h"
NS_USING(Client)

CTroll::CTroll()
{
}


CTroll::~CTroll()
{
}

void CTroll::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);

}

HRESULT CTroll::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CTroll::Initialize(void* pArg)
{
	auto MonDesc = static_cast<TROLL_DESC*>(pArg);
	// 트롤의 큰 몸통에 맞춰 이동용 Capsule을 확대한다. 중심 오프셋은
	// 전체 반높이와 같게 두어 CCT 바닥과 모델 원점(발)을 일치시킨다.
	// 리소스 로더의 4배 PreTransform을 적용한 바인드 메시가 약
	// 14.6 높이, 6.2 깊이이므로 몸통 기준으로 그 크기에 맞춘다.
	MonDesc->fCCTRadius = 3.1f;
	MonDesc->fCCTHeight = 8.4f;
	MonDesc->vCCTCenterOffset = {
		0.f,
		MonDesc->fCCTHeight * 0.5f + MonDesc->fCCTRadius,
		0.f };
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}

	// 트롤은 기본 몬스터 컬링 박스보다 메시가 훨씬 크므로 몸 전체와
	// 위로 솟는 애니메이션까지 포함하도록 로컬 박스를 넉넉하게 잡는다.
	if (m_pComCollider && m_pComCollider->Get() &&
		m_pComCollider->Get()->GetCollType() == CollType::Box)
	{
		static_cast<CCollBox*>(m_pComCollider->Get())->SetLocalBoundingBox(
			{ 0.f, 2.f, 0.f },
			{ 4.f, 6.f, 4.f });
	}

	m_iHp = m_iMaxHp = 300;

	CTrollWeapon::TROLL_WEAPON_DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.ParentHandle = GetHandle();
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHand");
	WeaponDesc.WeaponName = MonDesc->WeaponResourceName;
	WeaponDesc.LevelTag = MonDesc->LevelTag;
	WeaponDesc.vScale = MonDesc->vWeaponScale;
	WeaponDesc.vOwnerScale = MonDesc->vScale;
	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(MonDesc->LevelTag, MonDesc->WeaponProtoName, "03_Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon To Troll");
		return E_FAIL;
	}
	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();

	if (FAILED(Ready_Fsm(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}
	if (FAILED(Ready_Skill(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Skill");
		return E_FAIL;
	}

	Ready_BBKeyValue();

	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	m_eMonType = MONSTER_TYPE::BOSS;
	m_pModelAnimator->Play_Anim(0, false);
	ReadySound();
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	m_pComSphereCol->SetQueryEnabled(true);
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Chest");
	m_tDefaultCCTFilter = m_pCharacterController->GetFilter();
	if (FAILED(InitializeChargeCollider()))
		return E_FAIL;
	return S_OK;
}

HRESULT CTroll::InitializeChargeCollider()
{
	// CMonster의 전투 HurtBox 강체는 매 프레임 SKT_Chest 본의 위치와
	// 회전을 따라간다. 같은 키네마틱 강체에 돌진용 Shape를 붙이면
	// 상체가 앞으로 기울 때 배럴 파괴 판정도 애니메이션을 따라간다.
	CComPxBoxCollider::DESC Desc{};
	Desc.pComPxRigidBody = m_pComRigidBody;
	Desc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
		.vHalfExtents = { 4.f, 4.5f, 3.f } });
	Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
	Desc.vLocalOffset = { 0.f, -1.5f, 0.f };
	Desc.iShapeSubIndex = CHARGE_BODY_SHAPE_INDEX;
	Desc.tFilter = {
		.iLayer = ETOUI(COLLISION_LAYER::ENEMY_BODY),
		.iSimulationMask = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
		.iQueryMask = ETOUI(COLLISION_LAYER::NONE),
		.iNotifyFlags =
			PX_NOTIFY_TOUCH_FOUND |
			PX_NOTIFY_CONTACT_POINTS
	};

	if (!Desc.pResBoxGeo || !Desc.pResMaterial ||
		FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
			"ComPxTrollChargeBodyCollider",
			&Desc,
			&m_pChargeBodyCollider)))
	{
		return E_FAIL;
	}

	return m_pChargeBodyCollider->SetSimulationEnabled(false) &&
		m_pChargeBodyCollider->SetQueryEnabled(false)
		? S_OK
		: E_FAIL;
}

void CTroll::UpdateChargeColliderState()
{
	if (!m_pFsm || !m_pChargeBodyCollider || !m_pCharacterController)
		return;

	const _bool bShouldEnable =
		m_pFsm->GetCurState() == MON_STATE::SPAWN;
	if (bShouldEnable == m_bChargeBodyColliderEnabled)
		return;

	PX_FILTER_DESC CCTFilter = m_tDefaultCCTFilter;
	if (bShouldEnable)
	{
		// 돌진 중에는 세로 CCT가 잠든 배럴을 먼저 막지 않게 하고,
		// 애니메이션 본을 추종하는 Box만 실제 접촉을 처리한다.
		CCTFilter.iSimulationMask &=
			~ETOUI(COLLISION_LAYER::WORLD_DYNAMIC);
		CCTFilter.iQueryMask &=
			~ETOUI(COLLISION_LAYER::WORLD_DYNAMIC);

		if (!m_pCharacterController->SetFilter(CCTFilter) ||
			!m_pChargeBodyCollider->SetSimulationEnabled(true))
		{
			m_pCharacterController->SetFilter(m_tDefaultCCTFilter);
			m_pChargeBodyCollider->SetSimulationEnabled(false);
			DEBUG_LOG("[Troll] Failed to enable charge body collision.\n");
			return;
		}
	}
	else
	{
		if (!m_pChargeBodyCollider->SetSimulationEnabled(false) ||
			!m_pCharacterController->SetFilter(CCTFilter))
		{
			DEBUG_LOG("[Troll] Failed to restore collision after charge.\n");
			return;
		}
	}

	m_bChargeBodyColliderEnabled = bShouldEnable;
}
void CTroll::ReadySound()
{
	m_SoundTable["ChageReady"] = { "./Resources/SampleClient/Sound/Troll/Charge/troll_charge_attack_hit.wav", };
	m_SoundTable["ChageLoop"] = { "./Resources/SampleClient/Sound/Troll/Charge/vo_troll_charge_attack_loop.wav", };
	m_SoundTable["ChageHit"] = { "./Resources/SampleClient/Sound/Troll/Charge/troll_charge_attack_hit.wav", };

	m_SoundTable["Swing"] = { "./Resources/SampleClient/Sound/Troll/Club/troll_club_swing.wav", };
	m_SoundTable["Walk"] = { 
		"./Resources/SampleClient/Sound/Troll/Footsteps/Troll_Foot_Impact_91691935.wav", 
	"./Resources/SampleClient/Sound/Troll/Footsteps/Troll_Foot_Impact_902343883.wav", 
	"./Resources/SampleClient/Sound/Troll/Footsteps/Troll_Foot_Impact_241239946.wav",  };

	
}
HRESULT CTroll::Ready_Fsm(const _string& LevelTag)
{
	CMon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Mon_FSM", "Mon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CTroll_Spawn::Create(LevelTag))) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CTroll_Combat::Create())) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::HIT, CTroll_Hit::Create(this))) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::GROGGY, CTrollGroggy::Create(this))) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::GODAE, CMon_Godae::Create("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Cmbt_Idle_01_anm.bin", this))) return E_FAIL;

	if (false == m_pFsm->Add_State(MON_STATE::DEAD, CMon_Dead::Create("AN_SK_Troll_ArmoredTroll_CMB_Master_LOD0_Skeleton_Trl_Rct_KnckDn_Bck_anm.bin", this))) return E_FAIL;
	if (false == m_pFsm->Add_State(MON_STATE::NOTHING, CMon_Default::Create())) return E_FAIL;

	if (false == m_pFsm->Initialize_State(MON_STATE::NOTHING)) return E_FAIL;


	return S_OK;
}
HRESULT CTroll::Ready_Skill(const _string& LevelTag)
{
	if (ETOUI(TROLL_SKILL::END) > ETOUI(ATTMON::END))
		return E_FAIL;

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(TROLL_SKILL::DOLJIN);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(TROLL_SKILL::SMASH);
	//////////////////////파티클 넣는곳/////////////////////////
	m_EffectNames[ETOUI(TROLL_SKILL::DOLJIN)] = "Doljin";
	m_EffectNames[ETOUI(TROLL_SKILL::SMASH)] = "TrollSmash.json";
	////////////////////////////////////////////////////////////

	int32_t iBone{};
	iBone = m_pComModelInstance->GetModel()->Get_BoneIndex("prop_two_hand_carry");
	m_SkillHandle[ETOUI(TROLL_SKILL::SMASH)] = TROLL_SKILL_INFO{.iBoneIndex = iBone,.NameTag = "Smash"};
	
	return S_OK;
}
void CTroll::Ready_BBKeyValue()
{
	auto* pBB = Get_BlackBoard();
	pBB->Set_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE, m_TargetHandle);
}
void CTroll::PriorityUpdate(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_F2))
	{
		m_pFsm->Request_State(MON_STATE::SPAWN);
	}
	if (nullptr != m_pFsm)
	{
		if(m_pFsm->GetCurState() == MON_STATE::NOTHING)
			m_pFsm->Request_State(MON_STATE::SPAWN);
	}
	if (m_bEndGame)
	{
		SetPendingDestroy();
		GET_SINGLE(UIManager)->CreateFadeInSceneChange(3.f, 2.f, LEVEL::LAST_BOSS_RANROK);
		return;
	}
	Flag_Check(fTimeDelta);
	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
	UpdateChargeColliderState();
}

void CTroll::Update(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	__super::Update(fTimeDelta);

	if (m_fTick > 3.f)
	{
		Find_Target();
		m_fTick = 0.f;
	}
}

void CTroll::Stuck()
{
	if (nullptr == m_pFsm) return;
	m_pFsm->Request_State(MON_STATE::GODAE);
}

void CTroll::FixedUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CTroll::LateUpdate(E::_float fTimeDelta)
{
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}

void CTroll::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CTroll::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;

	return *pbFinished;
}
_string CTroll::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(TROLL_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<TROLL_SKILL>(pValue->second)).data();
}

_bool CTroll::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (m_iHp <= 0 && m_pFsm->GetCurState() == MON_STATE::DEAD)
		return false;

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

	if (true == BreakSkillType(eType) && false == m_bIsBreak)
	{
		if(eType == PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING)
		{
			m_pFsm->Request_State(MON_STATE::GROGGY);
		}
		else
		{
			m_pFsm->Request_State(MON_STATE::HIT);
		}

		m_PendingMonTable.eAttType = m_eAttType;
		m_PendingMonTable.eHitType = eType;
		m_bIsBreak = true;
		m_bPending = true;
	}

	return true;

}

void CTroll::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	auto pbEffect = Get_BlackBoard()->Get_Value<_bool>(EDG_KEY::EDGEFFECT);
	if (nullptr == pbEffect) return;

	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(TROLL_SKILL::END))
		return;

	if (*pbEffect)
	{
		_float4x4 mat;
		_matrix BoneMat = XMMatrixIdentity();
		int32_t iBoneIndex = m_SkillHandle[ETOUI(iSkillNum)].iBoneIndex;
		if (-1 != iBoneIndex)
		{
			BoneMat = XMLoadFloat4x4(Get_CombineBoneMatrix(iBoneIndex));
			for (uint32_t i = 0; i < 3; ++i)
				BoneMat.r[i] = XMVector3Normalize(BoneMat.r[i]);
		}

		XMStoreFloat4x4(&mat, BoneMat * GetTransform().GetLoadedWorldMatrix());
		CGameInstance::Get().Spawn(m_EffectNames[iSkillNum], mat);
		Get_BlackBoard()->Set_Value<_bool>(EDG_KEY::EDGEFFECT, false);
	}
	Set_Damage(static_cast<TROLL_SKILL>(iSkillNum));
	m_CurEffectName.clear();
	m_eAttType = eType;
	m_eLastSkillTable = m_eAttType = eType;

}
void CTroll::Flag_Check(_float fTimeDelta)
{
	if (m_iHp <= 0)
		m_pFsm->Request_State(MON_STATE::DEAD);
}
void CTroll::Destory_Child()
{
	auto PartesHandle = m_Partes[ETOUI(PARTES::WEAPON)];
	
	auto pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CTrollWeapon>(PartesHandle);
	if (nullptr != pWeapon)
		pWeapon->Set_Dead();
	
}
void CTroll::OnCollisionEnter(
	CGameObject* pObj,
	const PX_ON_COLLISION_DATA& info)
{
	if (info.iSelfShapeSubIndex != CHARGE_BODY_SHAPE_INDEX)
		return;

	if (auto* pBarrel = Cast<CPropBarrel>(pObj))
		pBarrel->DestroyBarrel();
}
const _float CTroll::Get_Damage()
{
	return m_fDamage;
}
void CTroll::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CTroll::BreakSkillType(PLAYER_SKILL_TYPE eType)
{
	uint32_t iSkillNumber = Find_SkillNum(m_eAttType);
	//파훼 됨?
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ACCIO:
		//if (ETOUI(DRAGON_SKILL::FIREBALL) == iSkillNumber)
		//	return true;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		break;
	case PLAYER_SKILL_TYPE::DESCENDO:
		break;
	case PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING:
		return true;
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		return true;
		break;
	case PLAYER_SKILL_TYPE::ABRA:
		return true;
		break;
	}
	return false;
}

void CTroll::Set_Damage(TROLL_SKILL eType)
{
	switch(eType)
	{
	case TROLL_SKILL::DOLJIN:
		m_fDamage = 100.f;
		break;
	case TROLL_SKILL::SMASH:
		m_fDamage = 30.f;
		break;
	}
}

E::UPtr<CTroll> CTroll::Create()
{
	auto pInstance = E::ToUPtr(new CTroll{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTroll");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTroll::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTroll{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTroll");
		return nullptr;
	}

	return pInstance;
}
