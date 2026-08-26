#include "pch.h"
#include "Edg_Spawn.h"
#include "EnderDragon.h"
#include "EnderDragon_State.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ClientEvents.h"
NS_USING(Client)
CEdg_Spawn::CEdg_Spawn()
{
}

CEdg_Spawn::~CEdg_Spawn()
{
}
HRESULT CEdg_Spawn::Initialize(const _string& strLevelTag)
{
	if(strLevelTag != MagicEnumToStringView(LEVEL::LAST_BOSS_RANROK))
		return S_OK;
	auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>("EDGWAYPT", "SPAWN");
	if (nullptr == pRes)
	{
		MSG_BOX("Load Failed Json To EDGWAYPT SPAWN");
		return E_FAIL;
	}
	auto json = pRes->Get_Json();
	JsonSaveLoadManager::LoadJsonTypeFloat3list(json,"SPAWN", m_PhasePos);

	return S_OK;
}
void CEdg_Spawn::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon)
		return;


	pDragon->Set_WingParticlesEnabled(false);
	pDragon->Set_StateFinished(false);
	
	m_Anims[ETOUI(EDG_SPAWN_NUMBER::SECOND)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Fly_Tucked_Loop_anm.bin"),.fBlend = 0.1f});
	//m_Anims[ETOUI(EDG_SPAWN_NUMBER::SECOND)].push_back(EDG_ANIM_FSM{ .iAnimIndex =
	//	pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Flap_anm.bin"),.fBlend = 0.5f});
	m_Anims[ETOUI(EDG_SPAWN_NUMBER::THIRD)].push_back(EDG_ANIM_FSM{.iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Fly_To_Hover_anm.bin"),.fBlend = 1.f});
	m_Anims[ETOUI(EDG_SPAWN_NUMBER::THIRD)].push_back(EDG_ANIM_FSM{.iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Taunt_Loop_anm.bin"),.fBlend = 1.f });

	
	pDragon->Get_Animator()->Play_Anim(0);
	pDragon->Set_HideOnBush(true);

}

void CEdg_Spawn::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;
	pDragon->Set_WingParticlesEnabled(true);
	
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	_float3 vLeftPos{}, vRightPos{};
	//좌우 무빙
	_float3 vPos = pDragon->GetTransform().GetPosition();
	_vector vDir = XMVector3Normalize(pDragon->GetTransform().GetState(STATE::RIGHT));
	XMStoreFloat3(&vLeftPos, XMLoadFloat3(&vPos)  + -vDir * 15.f);
	XMStoreFloat3(&vRightPos, XMLoadFloat3(&vPos) + vDir * 15.f);

	pBB->Set_Value<_float3>(EDG_KEY::LPATROL, vLeftPos);
	pBB->Set_Value<_float3>(EDG_KEY::RPATROL, vRightPos);
	pDragon->Set_HideOnBush(false);

	auto pSoundManager = CGameInstance::Get().GetSoundManager();

	if (m_iSound != INVALID_SOUND_ID)
	{
		pSoundManager->Stop(m_iSound);
		m_iSound = INVALID_SOUND_ID;
	}
}

void CEdg_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	//이거 뺴야지 나중에
	//if (false == pDragon->Is_StateFinished()) return;
	//카메라랑 샤바샤바 하고 전환

	switch (m_eSpawn)
	{
	case EDG_SPAWN_NUMBER::FIRST:
		_float4x4 mat{};
		XMStoreFloat4x4(&mat, pDragon->GetTransform().GetLoadedWorldMatrix());
		CGameInstance::Get().Spawn("SpawnSmoke.json", mat);
		CGameInstance::Get().EventPublish(FRequestPlayerCameraShake{
			.fIntensity = 1.f,
			.fDuration = 2.5f,
			.fFrequency = 25.f
		});

		E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/LastBossRanrok/Ambient/Ranrock_Spawn.wav", SOUND_PLAY_DESC{
		.sBusID = SOUND_BUS::SFX,
		.fVolume = 0.7f,
		.fPitch = 1.f,
		.iPriority = 64,
		.bLoop = false
			});
		m_eSpawn = EDG_SPAWN_NUMBER::SECOND;
		break;
	case EDG_SPAWN_NUMBER::SECOND:
		MoveSpawn(pDragon, fTimeDelta);
		break;
	case EDG_SPAWN_NUMBER::THIRD:
		//m_fSpawnTick += fTimeDelta;
		//if(m_fSpawnTick >= 2.f)
		Play_Anim(pDragon, fTimeDelta);
		break;
	case EDG_SPAWN_NUMBER::FOUR:
		pDragonFsm->Request_State(MON_STATE::COMBAT);
		break;
	}
	//pDragonFsm->Request_State(MON_STATE::COMBAT);
}

void CEdg_Spawn::SpawnSkill(CEnderDragon* pDragon, const _string& strName)
{
	m_iEffectID = CGameInstance::Get().PlayEffect(strName, *pDragon->GetTransform().GetWorldMatrix(), _vector{},
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (effectId != m_iEffectID)
				return;
			m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
		});
}

_bool CEdg_Spawn::MoveSpawn(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pMoveIntent = pDragon->Get_MoveIntent();
	if (nullptr == pMoveIntent)return false;
	
	if (m_PhasePos.size() == 1)
	{
		Play_AnimMoveSpawn(pDragon, fTimeDelta);
	}

	if (m_PhasePos.empty())
	{
		CGameInstance::Get().StopEffect(m_iEffectID);
		SpawnSkill(pDragon, "RanrokStaySmoke");
		m_fSpawnTick = 0.f;
		m_eSpawn = EDG_SPAWN_NUMBER::THIRD;
		m_bNext = false;
		return true;
	}
	m_fSpawnTick += fTimeDelta;

	Effect(pDragon, fTimeDelta);
	
	_vector vNextPos = XMLoadFloat3(&m_PhasePos.front());
	_vector vCurPos = XMLoadFloat3(&pDragon->GetTransform().GetPosition());

	_vector vToNext = vNextPos - vCurPos;
	if (!m_bNext)
	{
		XMStoreFloat3(&m_vLastDir, XMVector3Normalize(pDragon->GetTransform().GetState(STATE::LOOK)));
		XMStoreFloat3(&m_vNextDir, XMVector3Normalize(vNextPos - vCurPos));
		m_bNext = true;
	}

	_float fDot = XMVectorGetX(XMVector3Dot(XMVector3Normalize(vToNext), XMLoadFloat3(&m_vNextDir)));
	_float fDist = XMVectorGetX(XMVector3Length(vNextPos - vCurPos));

	if (fDist <= 0.5f || fDot < 0.f)
	{
		m_PhasePos.pop_front();
		m_bNext = false;
		m_fTick = 0.f;
		if (m_PhasePos.empty())
		{
			pMoveIntent->ClearMoveIntent();

			CGameInstance::Get().StopEffect(m_iEffectID);
			//SpawnSkill(pDragon, "RanrokStaySmoke");
		

			m_fSpawnTick = 0.f;
			m_eSpawn = EDG_SPAWN_NUMBER::THIRD;
			m_bNext = false;
			return true;
		}
		vNextPos = XMLoadFloat3(&m_PhasePos.front());

		XMStoreFloat3(&m_vNextDir,XMVector3Normalize(vNextPos - vCurPos));
	}
	
		m_fTick += fTimeDelta;
		_float t = std::min(m_fTick /0.5f,1.f);

		_float3 vLerpDir{};
		XMStoreFloat3(&vLerpDir, XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vLastDir), XMLoadFloat3(&m_vNextDir), t)));
		
		pMoveIntent->SetMoveIntent(vLerpDir, 25.f);
		if (m_PhasePos.size() <= 1)
		{
			if (!m_bEffectStop)
			{
				_matrix mat{};
				_float4x4 LastMat{};
				mat = pDragon->GetTransform().GetLoadedWorldMatrix();
				
				mat.r[3] += XMLoadFloat3(&m_vNextDir) * 35.f;
				XMStoreFloat4x4(&LastMat, mat);
				CGameInstance::Get().Spawn("SpawnSmoke.json", LastMat);
				m_bEffectStop = true;
			}
			
			pMoveIntent->SetFacingIntentImmediate(m_vNextDir);
		}
		else
			pMoveIntent->SetFacingIntent(vLerpDir, 30.f);
	
	return false;
}
void CEdg_Spawn::Effect(CEnderDragon* pDragon, _float fTimeDelta)
{
	m_fAngle += 180.f * fTimeDelta * 2.f;

	_matrix matWorld = XMMatrixRotationZ(XMConvertToRadians(m_fAngle)) * pDragon->GetTransform().GetLoadedWorldMatrix();

	auto TransformTrailPoint = [&matWorld](const _float3& localPoint)
		{
			_float3 worldPoint{};
		
			XMStoreFloat3(&worldPoint, XMVector3TransformCoord(XMLoadFloat3(&localPoint), matWorld));
			return worldPoint;
		};
	
	_float3 vstart{};
	_float3 vend{};
	
	vstart = TransformTrailPoint({ 0.f, 4.5f, 0.f });
	vend = TransformTrailPoint({ 0.f, 2.5f, 0.f });
	CGameInstance::Get().AddTrailPoint("RanrokTrail1", "RanrokTrail1", pDragon->GetHandle(), vstart, vend);
	
	vstart = TransformTrailPoint({ 0.f, 2.5f, -3.f });
	vend = TransformTrailPoint({ 0.f, 0.5f, -3.f });
	CGameInstance::Get().AddTrailPoint("RanrokTrail2", "RanrokTrail2", pDragon->GetHandle(), vstart, vend);
	
	vstart = TransformTrailPoint({ 0.f, 2.5f, 3.f });
	vend = TransformTrailPoint({ 0.f, 0.5f, 3.f });
	CGameInstance::Get().AddTrailPoint("RanrokTrail3", "RanrokTrail3", pDragon->GetHandle(), vstart, vend);
	
	vstart = TransformTrailPoint({ 0.f, 0.5f, -2.f });
	vend = TransformTrailPoint({ 0.f, -1.5f, -2.f });
	CGameInstance::Get().AddTrailPoint("RanrokTrail4", "RanrokTrail4", pDragon->GetHandle(), vstart, vend);
	
	vstart = TransformTrailPoint({ 0.f, 0.5f, 2.f });
	vend = TransformTrailPoint({ 0.f, -1.5f, 2.f });
	CGameInstance::Get().AddTrailPoint("RanrokTrail5", "RanrokTrail5", pDragon->GetHandle(), vstart, vend);
	
	if (m_fSpawnTick > 0.1f)
	{
		m_iEffectID = CGameInstance::Get().PlayEffect("RanrokMoveSmoke", *pDragon->GetTransform().GetWorldMatrix(), _vector{},
			[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
			{
				if (effectId != m_iEffectID)
					return;
				m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
			});
		m_fSpawnTick = 0.f;
	}
	else
	{
		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *pDragon->GetTransform().GetWorldMatrix());
	}
}

void CEdg_Spawn::Play_Anim(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	auto pMove = pDragon->Get_MoveIntent();
	if (nullptr == pMove) return;

	auto pTarget = pDragon->Get_Target();
	if (nullptr == pTarget) return;
	//////////사 운 드
	if (!m_bSound)
	{
		MONSOUND Sound_Desc{};
		_float3 vPos = pDragon->GetTransform().GetPosition();
		Sound_Desc.SoundKey = "WingDefault";
		Sound_Desc.bOnlyOne = false;
		Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = true, };
		Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f,.eRolloff = SOUND_3D_ROLLOFF::LINEAR };
		m_iSound = pDragon->Play_Sound(Sound_Desc);
		m_bSound = true;
	}
	if (m_iSound != INVALID_SOUND_ID)
	{
		CGameInstance::Get().GetSoundManager()->Set3DAttributes(
			m_iSound,
			pDragon->GetTransform().GetPosition()
		);
	}
	///////////////////////////////////////////////
	_float3 vTargetPos = pTarget->GetTransform().GetPosition();
	_float3 vSrcPos = pDragon->GetTransform().GetPosition();
	_float3 vDis{};
	XMStoreFloat3(&vDis, XMVector3Normalize(XMLoadFloat3(&vTargetPos) - XMLoadFloat3(&vSrcPos)));
	_float3 vLerp{};

	pDragon->Set_HideOnBush(false);
	if (!m_Anims[ETOUI(m_eSpawn)].empty())
	{
		if (m_Anims[ETOUI(m_eSpawn)].size() == 2)
		{
			XMStoreFloat3(&vLerp, XMVectorLerp(pDragon->GetTransform().GetState(STATE::LOOK), XMLoadFloat3(&vDis), 0.8f));
			pMove->SetFacingIntent(vLerp, 45.f);
		}
		

		if (m_Anims[ETOUI(m_eSpawn)].size() == 1)
		{
			if (!m_bSoundH && pAnimator->GetPlayAnimRatio() >= 0.4f)
			{
				if (!m_bSoundH)
				{
					MONSOUND Sound_Desc{};
					_float3 vPos = pDragon->GetTransform().GetPosition();
					Sound_Desc.SoundKey = "Houling";
					Sound_Desc.bOnlyOne = false;
					Sound_Desc.SoundPlay = SOUND_PLAY_DESC{ .fVolume = 0.8f,.bLoop = false, };
					Sound_Desc.str3DSound = SOUND_3D_DESC{ .vPosition = vPos ,.fMinDistance = 1.f, .fMaxDistance = 200.f,.eRolloff = SOUND_3D_ROLLOFF::LINEAR };
					m_bSoundH = true;
					pDragon->Play_Sound(Sound_Desc);
					auto* pModelInstance = pDragon->GetComponent<CComModelInstance>("ComCModelIntance");
					if (nullptr != pModelInstance)
					{
						const int32_t iMouthBoneIndex = pModelInstance->GetModel()->Get_BoneIndex("SKT_Mouth");
						const _float4x4* pMouthBoneMatrix = pDragon->Get_CombineBoneMatrix(iMouthBoneIndex);

						if (nullptr != pMouthBoneMatrix)
						{
							_matrix dragonWorld = pDragon->GetTransform().GetLoadedWorldMatrix();
							_matrix mouthWorld = XMLoadFloat4x4(pMouthBoneMatrix) * dragonWorld;

							_vector vLook = XMVector3Normalize(pDragon->GetTransform().GetState(STATE::LOOK));
							_vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

							if (fabsf(XMVectorGetX(XMVector3Dot(vLook, vWorldUp))) > 0.999f)
								vWorldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

							_vector vRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vLook));
							_vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

							_matrix roarWorld = XMMatrixIdentity();
							roarWorld.r[0] = XMVectorSetW(vRight, 0.f);
							roarWorld.r[1] = XMVectorSetW(vUp, 0.f);
							roarWorld.r[2] = XMVectorSetW(vLook, 0.f);
							roarWorld.r[3] = XMVectorSetW(mouthWorld.r[3], 1.f);

							_float4x4 roarWorldData{};
							XMStoreFloat4x4(&roarWorldData, roarWorld);

							m_iRoarEffectID = CGameInstance::Get().PlayEffect(
								"DragonRoar",
								roarWorldData,
								XMVectorZero(),
								[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
								{
									if (effectId != m_iRoarEffectID)
										return;

									CGameInstance::Get().Set_RadialBlurIntensity(0.f);
									m_iRoarEffectID = INVALID_EFFECT_INSTANCE_ID;
								});

							if (m_iRoarEffectID != INVALID_EFFECT_INSTANCE_ID)
								CGameInstance::Get().Set_RadialBlurIntensity(6.3f);
							CGameInstance::Get().EventPublish(FRequestPlayerCameraShake{
								.fIntensity = 1.f,
								.fDuration = 2.5f,
								.fFrequency = 25.f
								});
						}
					}
		 
				}
			}
		}
		if (pAnimator->GetFinish())
		{
			m_Anims[ETOUI(m_eSpawn)].pop_front();

		}
			
	}
	

	if(m_Anims[ETOUI(m_eSpawn)].empty())
	{
		m_eSpawn = EDG_SPAWN_NUMBER::FOUR;
		return;
	}
	
	EDG_ANIM_FSM EdgAnim = m_Anims[ETOUI(m_eSpawn)].front();

	pAnimator->Play_Anim(EdgAnim.iAnimIndex, false, EdgAnim.fBlend);

}
void CEdg_Spawn::Play_AnimMoveSpawn(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	auto pMove = pDragon->Get_MoveIntent();
	if (nullptr == pMove) return;

	auto pTarget = pDragon->Get_Target();
	if (nullptr == pTarget) return;

	pDragon->Set_HideOnBush(false);
	if (!m_Anims[ETOUI(m_eSpawn)].empty())
	{
		if (pAnimator->GetFinish())
		{
			m_Anims[ETOUI(m_eSpawn)].pop_front();

		}

	}
	if (m_Anims[ETOUI(m_eSpawn)].empty())
	{
		return;
	}

	EDG_ANIM_FSM EdgAnim = m_Anims[ETOUI(m_eSpawn)].front();

	pAnimator->Play_Anim(EdgAnim.iAnimIndex, true, EdgAnim.fBlend);
}
SPtr<CEdg_Spawn> CEdg_Spawn::Create(const _string& strLevelTag)
{
	auto pInstance = ToSPtr(new CEdg_Spawn{});
	if (FAILED(pInstance->Initialize(strLevelTag)))
	{
		MSG_BOX("Failed to create CEdg_Spawn");
		return nullptr;
	}

	return pInstance;
}
