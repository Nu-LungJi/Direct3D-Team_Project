#include "pch.h"
#include "ShopNpc.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "ResModel.h"
#include "UIManager.h"

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
}

void CShopNpc::OpenShop()
{
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
