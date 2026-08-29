#include "pch.h"
#include "ShopNpc.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"
#include "ResModelBone.h"
#include "UIManager.h"
#include "AnimatedWorldObject.h"

NS_USING(Client)

CShopNpc::CShopNpc(const CShopNpc& prototype)
	: CInteractiveNpc(prototype)
{
}

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
	return S_OK;
}

void CShopNpc::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
	// 상점 NPC는 행동 트리 상태와 관계없이 바닥을 따라가야 한다.
	if (m_pCharacterMotor)
		m_pCharacterMotor->SetUseGravity(true);
}

void CShopNpc::UpdateGUI()
{
	__super::UpdateGUI();

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
	__super::Update(fTimeDelta);
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
					cameraRight * 0.75f;
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
				GetHandle(), panelOffset, { 0.f, 180.f, 0.f }, 0.4f);
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
