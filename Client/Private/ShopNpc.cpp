#include "pch.h"
#include "ShopNpc.h"
#include "NpcRagdollController.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"
#include "ResModelBone.h"
#include "UIManager.h"
#include "AnimatedWorldObject.h"
#include "BTRoot.h"
NS_USING(Client)

CShopNpc::CShopNpc(const CShopNpc& prototype)
	: CInteractiveNpc(prototype)
{
}

CShopNpc::~CShopNpc() = default;

HRESULT CShopNpc::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	auto* pDesc = static_cast<DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_bWorldSpaceShop = pDesc->WorldSpaceShop;
	m_vShopPanelPositionOffset = pDesc->ShopPanelPositionOffset;
	m_vShopPanelRotationOffsetDegrees =
		pDesc->ShopPanelRotationOffsetDegrees;
	m_vDebugEntrancePosition = pDesc->vPos;
	m_vDebugEntranceControllerPosition = m_pCharacterController
		? m_pCharacterController->GetPosition()
		: pDesc->vPos;
	XMStoreFloat4(
		&m_qDebugEntranceRotation,
		XMQuaternionRotationRollPitchYaw(
			XMConvertToRadians(pDesc->vRot.x),
			XMConvertToRadians(pDesc->vRot.y),
			XMConvertToRadians(pDesc->vRot.z)));
	GetTransform().SetQuaternion(m_qDebugEntranceRotation);
	GetTransform().Update();
	if (m_pCharacterMotor)
		m_pCharacterMotor->SetUseGravity(true);
	if (m_pModelAnimator)
	{
		m_iSpeechMouthMorph = m_pModelAnimator->FindMorphTargetIndex("jaw_drop");
		if (m_iSpeechMouthMorph == UINT32_MAX)
			m_iSpeechMouthMorph = m_pModelAnimator->FindMorphTargetIndex("jaw_drop_mid");
		m_iSpeechFacialAnimation = Find_AnimIndex(
			"AN_BODY__DialogueTalk__HU_STN_STND_Conv_Talk.bin");
		m_iWandBoxOpenAnimation = Find_AnimIndex(
			"AN_BODY__WandSelection__Clip21_Ollivander.bin");
		if (m_pComModelInstance && m_pComModelInstance->GetModel())
		{
			static constexpr const char* HAND_BONE_CANDIDATES[] =
			{
				"SKT_FX_RightHandSocket",
				"SKT_RightHandSocket",
				"RightHand",
				"SKT_RightHand",
				"RightHandWandSocket"
			};
			for (const char* boneName : HAND_BONE_CANDIDATES)
			{
				m_iWandBoxAttachBoneIndex =
					m_pComModelInstance->GetModel()->Get_BoneIndex(boneName);
				if (m_iWandBoxAttachBoneIndex >= 0)
					break;
			}
		}
		if (m_iSpeechFacialAnimation >= 0 && m_pComModelInstance &&
			m_pComModelInstance->GetModel())
		{
			const auto& model = m_pComModelInstance->GetModel();
			const auto& animations = model->GetAnimations();
			if (static_cast<size_t>(m_iSpeechFacialAnimation) < animations.size())
			{
				const auto& facialAnim = animations[m_iSpeechFacialAnimation];
				const auto& bones = model->GetBones();
				for (size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
				{
					if (!bones[boneIndex] ||
						!facialAnim->GetChannelByBoneIndex(static_cast<uint32_t>(boneIndex)))
						continue;
					const _string boneName = bones[boneIndex]->GetBoneName();
					if (boneName == "jawC" || boneName.find("jaw") != _string::npos)
						m_bSpeechJawChannel = true;
					if (boneName == "teeth_lwr")
						m_bSpeechLowerTeethChannel = true;
					if (boneName.find("tongue") != _string::npos)
						m_bSpeechTongueChannel = true;
				}
			}
		}
	}

	m_pRagdollController = CNpcRagdollController::Create(*this);
	if (!m_pRagdollController)
		return E_FAIL;

	return S_OK;
}

void CShopNpc::PriorityUpdate(E::_float fTimeDelta)
{
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
	m_bRagdollGameplaySuspended = false;

	__super::PriorityUpdate(fTimeDelta);
	// 상점 NPC는 행동 트리 상태와 관계없이 바닥을 따라가야 한다.
	if (m_pCharacterMotor)
		m_pCharacterMotor->SetUseGravity(true);
}

void CShopNpc::FixedUpdate(E::_float fTimeDelta)
{
	if (m_pRagdollController &&
		m_pRagdollController->PreFixedUpdate())
	{
		return;
	}

	__super::FixedUpdate(fTimeDelta);
	if (m_pRagdollController)
		m_pRagdollController->PostFixedUpdate();
}

void CShopNpc::UpdateGUI()
{
	__super::UpdateGUI();
	if (m_pRagdollController)
		m_pRagdollController->UpdateGUI();

	if (!ImGui::CollapsingHeader(
		"Shop NPC Animation Test", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	if (!m_pComModelInstance || !m_pModelAnimator ||
		!m_pComModelInstance->GetModel())
	{
		ImGui::TextDisabled("Animator or model is not ready.");
		return;
	}

	const auto& animations =
		m_pComModelInstance->GetModel()->GetAnimations();
	if (animations.empty())
	{
		ImGui::TextDisabled("No animation clips.");
		return;
	}

	m_iDebugAnimationIndex = std::clamp(
		m_iDebugAnimationIndex, 0,
		static_cast<int32_t>(animations.size()) - 1);
	const _string& currentName =
		animations[m_iDebugAnimationIndex]->GetAnimName();
	if (ImGui::BeginCombo("Animation Clip", currentName.c_str()))
	{
		for (int32_t i = 0; i < static_cast<int32_t>(animations.size()); ++i)
		{
			const _bool selected = i == m_iDebugAnimationIndex;
			if (ImGui::Selectable(
				animations[i]->GetAnimName().c_str(), selected))
				m_iDebugAnimationIndex = i;
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Checkbox("Loop", &m_bDebugAnimationLoop);
	ImGui::DragFloat(
		"Speed", &m_fDebugAnimationSpeed, 0.05f, 0.f, 3.f, "%.2f");
	ImGui::DragFloat(
		"Blend", &m_fDebugAnimationBlend, 0.01f, 0.f, 1.f, "%.2f sec");
	ImGui::Checkbox("Apply Root Motion", &m_bDebugRootMotion);
	ImGui::SameLine();
	ImGui::Checkbox("Apply Root Rotation", &m_bDebugRootMotionRotation);

	if (ImGui::Button("Play Selected"))
	{
		SetRootMotionActive(m_bDebugRootMotion);
		SetRootMotionRotationActive(m_bDebugRootMotionRotation);
		m_pModelAnimator->Play_Anim(
			m_iDebugAnimationIndex,
			m_bDebugAnimationLoop,
			m_fDebugAnimationBlend);
		m_pModelAnimator->GetCurAnimState().fSpeed =
			m_fDebugAnimationSpeed;
		m_pModelAnimator->SetPlay(true);
	}
	ImGui::SameLine();
	if (ImGui::Button(
		m_pModelAnimator->GetPlay() ? "Pause" : "Resume"))
		m_pModelAnimator->SetPlay(!m_pModelAnimator->GetPlay());

	// 재생 중에도 속도 슬라이더가 즉시 반영되게 한다.
	if (m_pModelAnimator->GetCurAnimState().IsValid())
		m_pModelAnimator->GetCurAnimState().fSpeed =
			m_fDebugAnimationSpeed;

	ImGui::Text("Clip: %d / %zu", m_iDebugAnimationIndex, animations.size());
	ImGui::Text("Ratio: %.3f", m_pModelAnimator->GetPlayAnimRatio());

	ImGui::Separator();
	ImGui::TextUnformatted("Entrance Cinematic Test");
	ImGui::Text("Start: %.3f, %.3f, %.3f",
		m_vDebugEntrancePosition.x,
		m_vDebugEntrancePosition.y,
		m_vDebugEntrancePosition.z);
	if (ImGui::Button("Reset NPC To Entrance"))
	{
		CancelDialogue();
		SetRootMotionActive(false);
		SetRootMotionRotationActive(false);
		if (m_pCharacterController)
			m_pCharacterController->SetPosition(m_vDebugEntranceControllerPosition);
		GetTransform().SetPosition(m_vDebugEntrancePosition);
		GetTransform().SetQuaternion(m_qDebugEntranceRotation);
		GetTransform().Update();
	}
	ImGui::SameLine();
	if (ImGui::Button("Test Full Entrance"))
	{
		SetRootMotionActive(false);
		SetRootMotionRotationActive(false);
		if (m_pCharacterController)
			m_pCharacterController->SetPosition(m_vDebugEntranceControllerPosition);
		GetTransform().SetPosition(m_vDebugEntrancePosition);
		GetTransform().SetQuaternion(m_qDebugEntranceRotation);
		GetTransform().Update();
		RestartDialogueForTest();
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop Entrance"))
		CancelDialogue();

	if (ImGui::Button("Open in Animation Editor"))
		CGameInstance::Get().SetAnimationEditorTarget(GetHandle());

	ImGui::Separator();
	ImGui::TextUnformatted("CloseUp Cinematic Test (No Transform Move)");
	if (ImGui::Button("Test ShopNpcDialogueCloseUp"))
		PlayDialogueCameraOnlyForTest("ShopNpcDialogueCloseUp");
	ImGui::SameLine();
	if (ImGui::Button("Stop CloseUp Camera"))
		StopDialogueCameraOnlyForTest();
	ImGui::TextDisabled("NPC and player position/rotation are not changed.");
	if (ImGui::Button("Test Return Visit Dialogue"))
		RestartDialogueAtIndexForTest(4u);
	ImGui::SameLine();
	ImGui::TextDisabled("Return lines -> open wand shop");
	if (ImGui::Button("Test Wand Box Camera"))
		PlayDialogueCameraOnlyForTest("ShopNpcWandBox");
	ImGui::SameLine();
	if (ImGui::Button("Stop Wand Box Camera"))
		StopDialogueCameraOnlyForTest();
	ImGui::Separator();
	ImGui::TextUnformatted("Speech Face Diagnostics");
	ImGui::Text("jaw_drop morph: %s",
		m_iSpeechMouthMorph != UINT32_MAX ? "found" : "missing");
	ImGui::Text("DialogueTalk face layer: %s",
		m_iSpeechFacialAnimation >= 0 ? "found" : "missing");
	ImGui::Text("Channels - jaw: %s / lower teeth: %s / tongue: %s",
		m_bSpeechJawChannel ? "yes" : "no",
		m_bSpeechLowerTeethChannel ? "yes" : "no",
		m_bSpeechTongueChannel ? "yes" : "no");
	ImGui::Separator();
	ImGui::TextUnformatted("Wand Box Local Transform");
	_bool boxTransformChanged = false;
	boxTransformChanged |= ImGui::DragFloat3(
		"Box Position", &m_vWandBoxLocalPosition.x, 0.01f);
	boxTransformChanged |= ImGui::DragFloat3(
		"Box Rotation", &m_vWandBoxLocalRotation.x, 1.f);
	boxTransformChanged |= ImGui::DragFloat3(
		"Box Scale", &m_vWandBoxLocalScale.x, 0.01f, 0.01f, 10.f);
	if (boxTransformChanged && m_hWandBox.IsValid())
	{
		if (auto* box = Cast<CAnimatedWorldObject>(
			CGameInstance::Get().GetGameObjectByHandle(m_hWandBox)))
		{
			box->ApplyTransform(m_vWandBoxLocalPosition,
				m_vWandBoxLocalRotation, m_vWandBoxLocalScale);
		}
	}
}

void CShopNpc::OpenShop()
{
	if (m_iWandBoxOpenAnimation >= 0)
	{
		m_bWandBoxPresentationPending = true;
		return;
	}

	auto* pUIManager = GET_SINGLE(UIManager);
	if (m_bWorldSpaceShop)
	{
		pUIManager->OpenWandShopWorld(
			GetHandle(),
			m_vShopPanelPositionOffset,
			m_vShopPanelRotationOffsetDegrees);
		return;
	}

	pUIManager->OpenWandShop();
}

void CShopNpc::PrepareDialogueCamera(const _string& cinematicName)
{
	if (cinematicName != "ShopNpcDialogueCloseUp")
		return;

	constexpr _float3 closeUpPosition{ 124.677f, 0.717f, -87.211f };
	const _float3 controllerOffset{
		m_vDebugEntranceControllerPosition.x - m_vDebugEntrancePosition.x,
		m_vDebugEntranceControllerPosition.y - m_vDebugEntrancePosition.y,
		m_vDebugEntranceControllerPosition.z - m_vDebugEntrancePosition.z
	};
	if (m_pCharacterController)
	{
		m_pCharacterController->SetPosition({
			closeUpPosition.x + controllerOffset.x,
			closeUpPosition.y + controllerOffset.y,
			closeUpPosition.z + controllerOffset.z
		});
	}
	GetTransform().SetPosition(closeUpPosition);
	GetTransform().SetQuaternion(
		XMQuaternionRotationRollPitchYaw(
			0.f, XMConvertToRadians(131.699f), 0.f));
	GetTransform().Update();

	// NPC 앞에 세워 NPC를 바라보게 한다. NPC Y가 0.717이므로 로컬
	// 오프셋 1.683을 적용해 플레이어의 최종 월드 Y를 2.4로 맞춘다.
	PlacePlayerFacingNpc({ 1.2f, 1.683f, 2.2f });
}

void CShopNpc::SpawnWandBoxAtFirstHandShot()
{
	if (m_hWandBox.IsValid())
		return;

	m_fWandBoxAnimationElapsed = 0.f;
	m_bWandBoxAnimationPaused = false;
	CAnimatedWorldObject::DESC boxDesc{};
	boxDesc.sObjectTag = "Ollivander_WandBox_Full_Selection";
	boxDesc.sModelGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	boxDesc.sModelResourceTag =
		"Model_Resource_Ollivander_WandBox_Full_Selection";
	boxDesc.sAnimationName =
		"AN_CCL_Activity_WandSelection_Clip12_WandBox_anm.bin";
	boxDesc.bLoop = false;
	boxDesc.fAnimationSpeed = 1.f;
	boxDesc.fDissolveAppearDuration = 0.5f;
	boxDesc.ParentHandle = GetHandle();
	boxDesc.iParentBoneIndex = m_iWandBoxAttachBoneIndex;
	boxDesc.bLockLocalRotation = true;
	boxDesc.vPosition = m_vWandBoxLocalPosition;
	boxDesc.vRotation = m_vWandBoxLocalRotation;
	boxDesc.vScale = m_vWandBoxLocalScale;
	if (const auto box = CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_AnimatedWorldObject,
			"03_WandBox",
			&boxDesc))
	{
		m_hWandBox = *box;
		if (auto* wandBox = Cast<CAnimatedWorldObject>(
			CGameInstance::Get().GetGameObjectByHandle(m_hWandBox)))
		{
			// Re-apply the complete socket-local pose after registration so the
			// first hand-shot frame includes the authored Euler rotation as well.
			wandBox->ApplyTransform(
				m_vWandBoxLocalPosition,
				m_vWandBoxLocalRotation,
				m_vWandBoxLocalScale);
		}
	}
}

void CShopNpc::Update(E::_float fTimeDelta)
{
	if (!IsRagdollActive())
		__super::Update(fTimeDelta);

	GetTransform().Update();
	if (m_pRagdollController)
		m_pRagdollController->UpdatePoseBridge();
	if (IsRagdollActive())
	{
		SuspendGameplayForRagdoll();
		return;
	}

	auto* uiManager = GET_SINGLE(UIManager);
	if (uiManager->ConsumeWandPurchaseCompleted())
		m_bWandPurchaseDialoguePending = true;

	if (!m_pModelAnimator)
		return;

	// WandBox의 Com_Transform 로컬 값은 손 뼈의 움직임과 관계없이
	// 항상 지정된 Position / Rotation / Scale로 고정한다.
	if (m_hWandBox.IsValid())
	{
		if (auto* wandBox = Cast<CAnimatedWorldObject>(
			CGameInstance::Get().GetGameObjectByHandle(m_hWandBox)))
		{
			wandBox->ApplyTransform(
				m_vWandBoxLocalPosition,
				{ 46.231f, 93.596f, -168.686f },
				m_vWandBoxLocalScale);
		}
	}

	if (m_bWandBoxPresentationPending)
	{
		m_bWandBoxPresentationPending = false;
		m_bWandBoxPresentationActive = true;
		m_bWandBoxCameraStarted = false;
		m_fWandBoxCameraElapsed = 0.f;
		m_bWandShopOpenedByPresentation = false;
		GetTransform().SetQuaternion(
			XMQuaternionRotationRollPitchYaw(
				0.f, XMConvertToRadians(180.f), 0.f));
		GetTransform().Update();
		m_pModelAnimator->Play_Anim(m_iWandBoxOpenAnimation, false, 0.15f);
	}

	if (m_bWandBoxPresentationActive)
	{
		if (!m_bWandBoxAnimationPaused && m_hWandBox.IsValid())
		{
			m_fWandBoxAnimationElapsed +=
				E::CGameInstance::Get().GetUnscaledDelta();
			if (m_fWandBoxAnimationElapsed >= 0.5f)
			{
				if (auto* box = Cast<CAnimatedWorldObject>(
					CGameInstance::Get().GetGameObjectByHandle(m_hWandBox)))
				{
					box->SetAnimationPaused(true);
					m_bWandBoxAnimationPaused = true;
				}
			}
		}

		const _float wandOpenRatio = m_pModelAnimator->GetPlayAnimRatio();
		// JSON의 0초 CloseUp 키에서 1초 상자 키로 이어지도록 열리기 직전에 시작한다.
		if (!m_bWandBoxCameraStarted && wandOpenRatio >= 0.65f)
		{
			PlayDialogueCameraOnlyForTest("ShopNpcWandBox");
			SpawnWandBoxAtFirstHandShot();
			m_bWandBoxCameraStarted = true;
			m_fWandBoxCameraElapsed = 0.f;
		}
		if (!m_bWandBoxCameraStarted && m_pModelAnimator->GetFinish())
		{
			PlayDialogueCameraOnlyForTest("ShopNpcWandBox");
			SpawnWandBoxAtFirstHandShot();
			m_bWandBoxCameraStarted = true;
			m_fWandBoxCameraElapsed = 0.f;
		}
		if (m_bWandBoxCameraStarted && !m_bWandShopOpenedByPresentation)
			m_fWandBoxCameraElapsed += E::CGameInstance::Get().GetUnscaledDelta();
		if (!m_bWandShopOpenedByPresentation && m_bWandBoxCameraStarted &&
			m_fWandBoxCameraElapsed >= 8.f)
		{
			E::TIME_SCALE_REQUEST_DESC pauseDesc{};
			pauseDesc.fTargetScale = 0.f;
			pauseDesc.fBlendIn = 0.f;
			pauseDesc.fMaxUnscaledDuration = 600.f;
			pauseDesc.fSafetyBlendOut = 0.15f;
			pauseDesc.sTag = "ShopNpc_WandBoxRevealPause";
			m_bWandPresentationOwnsTimePause =
				E::CGameInstance::Get().BeginTimeScale(pauseDesc);
			// ShopNpcWandBox의 마지막 카메라 포즈를 기준으로 패널을
			// 카메라 전방 및 화면 오른쪽에 배치한다. 카메라 키가 수정되어도
			// NPC 로컬 오프셋을 런타임에 다시 계산해 같은 구도를 유지한다.
			_float3 panelOffset{ -2.f, 1.5f, 0.2f };
			if (auto* activeCamera = E::CGameInstance::Get().GetActiveCamera())
			{
				constexpr _float PANEL_DISTANCE = 3.f;
				const _vector cameraPosition =
					activeCamera->GetTransform().GetLoadedPostion();
				const _vector cameraLook = XMVector3Normalize(
					activeCamera->GetTransform().GetState(E::STATE::LOOK));
				const _vector cameraRight = XMVector3Normalize(
					activeCamera->GetTransform().GetState(E::STATE::RIGHT));
				const _vector panelPosition =
					cameraPosition + cameraLook * PANEL_DISTANCE +
					cameraRight * 1.05f;
				const _vector panelDelta =
					panelPosition - GetTransform().GetLoadedPostion();
				const _vector npcRight = XMVector3Normalize(
					GetTransform().GetState(E::STATE::RIGHT));
				const _vector npcUp = XMVector3Normalize(
					GetTransform().GetState(E::STATE::UP));
				const _vector npcLook = XMVector3Normalize(
					GetTransform().GetState(E::STATE::LOOK));

				panelOffset = {
					XMVectorGetX(XMVector3Dot(panelDelta, npcRight)),
					XMVectorGetX(XMVector3Dot(panelDelta, npcUp)),
					XMVectorGetX(XMVector3Dot(panelDelta, npcLook))
				};
			}
			GET_SINGLE(UIManager)->OpenWandShopWorld(
				GetHandle(), panelOffset, { 0.f, 180.f, 0.f }, 0.22f);
			m_bWandShopOpenedByPresentation = true;
		}
		else if (m_bWandShopOpenedByPresentation &&
			!GET_SINGLE(UIManager)->IsWandShopOpen())
		{
			if (m_bWandPresentationOwnsTimePause)
				E::CGameInstance::Get().EndTimeScale(
					"ShopNpc_WandBoxRevealPause", 0.15f);
			m_bWandPresentationOwnsTimePause = false;
			m_bWandBoxPresentationActive = false;
			StopDialogueCameraOnlyForTest();
			if (auto* box = CGameInstance::Get().GetGameObjectByHandle(m_hWandBox))
				box->SetPendingDestroy();
			m_hWandBox = {};
		}
	}

	// 구매용 E 홀드가 완료되어 상점 연출이 정리되면, 추가 입력 없이
	// 기존 대화 페이드 및 CloseUp 시네마틱으로 후속 대화를 시작한다.
	if (m_bWandPurchaseDialoguePending &&
		!uiManager->IsWandShopOpen() &&
		!m_bWandBoxPresentationActive &&
		!IsTalking())
	{
		m_bWandPurchaseDialoguePending = false;
		RestartDialogueAtIndexForTest(6u);
	}

	const _bool talking = IsTalking() && GetState() == STATE::TALKING &&
		IsDialogueSpeechActive();
	if (!talking)
	{
		m_fSpeechMorphTime = 0.f;
		if (m_bSpeechMorphApplied)
		{
			m_pModelAnimator->ClearMorphPreview();
			m_bSpeechMorphApplied = false;
		}
		if (m_bSpeechUpperAnimationPlaying)
		{
			m_pModelAnimator->Stop_UpperAnim(0.12f);
			m_bSpeechUpperAnimationPlaying = false;
		}
		return;
	}

	if (!m_bSpeechUpperAnimationPlaying && m_iSpeechFacialAnimation >= 0 &&
		m_pModelAnimator->Set_UpperBodyRootBone("face", 1))
	{
		m_pModelAnimator->Play_UpperAnim(
			m_iSpeechFacialAnimation, true, 0.1f);
		m_pModelAnimator->SetUpperAnimationFadeOutDuration(0.12f);
		m_bSpeechUpperAnimationPlaying = true;
	}
	if (m_iSpeechMouthMorph == UINT32_MAX)
		return;

	m_fSpeechMorphTime += fTimeDelta;
	// 서로 다른 주기의 파형과 짧은 휴지 구간을 섞어 기계적인 반복을 줄인다.
	const _float syllable = std::abs(std::sin(m_fSpeechMorphTime * 10.7f));
	const _float variation = 0.65f + 0.35f * std::sin(m_fSpeechMorphTime * 3.1f + 0.8f);
	const _bool pause = std::fmod(m_fSpeechMorphTime, 2.35f) > 2.05f;
	const _float weight = pause ? 0.03f : std::clamp(syllable * variation, 0.04f, 0.72f);
	m_pModelAnimator->SetMorphPreview(m_iSpeechMouthMorph, weight);
	m_bSpeechMorphApplied = true;
}

void CShopNpc::LateUpdate(E::_float fTimeDelta)
{
	if (!IsRagdollActive())
	{
		__super::LateUpdate(fTimeDelta);
		return;
	}

	// CWorldAgent::LateUpdate의 CCT 발 위치 동기화를 건너뛰고,
	// 랙돌이 기록한 본 행렬과 기존 오브젝트 월드를 그대로 렌더한다.
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
}

_bool CShopNpc::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType != PLAYER_SKILL_TYPE::ABRA || !CanBePlayerCombatTarget())
		return false;

	// 랙돌 준비가 성공한 뒤에만 사망 상태로 바꾼다. 준비 실패 상태에서
	// HP부터 0이 되면 CWorldAgent의 일반 PendingDestroy 경로로 빠질 수 있다.
	if (!m_pRagdollController ||
		!m_pRagdollController->RequestFromCurrentMotion())
	{
		return false;
	}

	// 다음 포즈 브리지에서 현재 애니메이션 자세를 랙돌 시작 자세로 넘긴다.
	m_iHp = 0;
	return true;
}

_bool CShopNpc::CanBePlayerCombatTarget() const
{
	return m_iHp > 0 && !IsRagdollActive();
}

_bool CShopNpc::TryGetSkillTargetPosition(_float3& OutPosition) const
{
	if (m_pCharacterController)
	{
		// Transform은 발 위치이므로 CCT 중심을 사용해 빔이 몸통으로 향하게 한다.
		OutPosition = m_pCharacterController->GetPosition();
		return true;
	}

	OutPosition = GetTransform().GetPosition();
	OutPosition.y += 1.2f;
	return true;
}

void CShopNpc::SuspendGameplayForRagdoll()
{
	if (m_bRagdollGameplaySuspended)
		return;

	// 대화 카메라, 입력 잠금, 선택지와 상점 UI가 랙돌 뒤에 남지 않게 정리한다.
	CancelDialogue();
	StopDialogueCameraOnlyForTest();
	auto* pUIManager = GET_SINGLE(UIManager);
	if (pUIManager->IsWandShopOpen())
		pUIManager->CloseWandShop();

	if (m_bWandPresentationOwnsTimePause)
	{
		E::CGameInstance::Get().EndTimeScale(
			"ShopNpc_WandBoxRevealPause", 0.15f);
	}
	m_bWandPresentationOwnsTimePause = false;
	m_bWandBoxPresentationPending = false;
	m_bWandBoxPresentationActive = false;
	m_bWandBoxCameraStarted = false;
	m_bWandShopOpenedByPresentation = false;
	m_bWandPurchaseDialoguePending = false;
	m_fWandBoxCameraElapsed = 0.f;
	m_fWandBoxAnimationElapsed = 0.f;
	m_bWandBoxAnimationPaused = false;
	if (auto* pWandBox = CGameInstance::Get().
		GetGameObjectByHandle(m_hWandBox))
	{
		pWandBox->SetPendingDestroy();
	}
	m_hWandBox = {};

	if (m_pModelAnimator)
	{
		m_pModelAnimator->ClearMorphPreview();
		if (m_bSpeechUpperAnimationPlaying)
			m_pModelAnimator->Stop_UpperAnim(0.f);
	}
	m_bSpeechMorphApplied = false;
	m_bSpeechUpperAnimationPlaying = false;
	m_bRagdollGameplaySuspended = true;
}

_bool CShopNpc::RequestRagdollActivation(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocityRadians)
{
	return m_pRagdollController &&
		m_pRagdollController->RequestActivation(
			vLinearVelocity,
			vAngularVelocityRadians);
}

_bool CShopNpc::ResetRagdoll()
{
	return m_pRagdollController &&
		m_pRagdollController->Reset();
}

_bool CShopNpc::IsRagdollActive() const
{
	return m_pRagdollController &&
		m_pRagdollController->IsActive();
}

E::UPtr<CShopNpc> CShopNpc::Create()
{
	auto pInstance = E::ToUPtr(new CShopNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CShopNpc");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CShopNpc::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CShopNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CShopNpc");
		return nullptr;
	}
	return pInstance;
}
