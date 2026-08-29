#pragma once
#include "InteractiveNpc.h"

NS_BEGIN(Client)
class CNpcRagdollController;

// 대사와 시네마틱은 CInteractiveNpc의 흐름을 사용하고,
// OPEN_SHOP 대사 액션에서 지팡이 상점 UI를 여는 상점 전용 NPC.
class CShopNpc final : public CInteractiveNpc
{
public:
	struct DESC : public CInteractiveNpc::DESC
	{
		// false면 일반 2D 상점, true면 NPC 기준 월드 패널 상점을 연다.
		_bool WorldSpaceShop{};
		_float3 ShopPanelPositionOffset{ 0.f, 1.6f, 1.2f };
		_float3 ShopPanelRotationOffsetDegrees{};
	};

public:
	DECLARE_DERIVED_TYPE(CShopNpc, CInteractiveNpc)

private:
	CShopNpc() = default;
	CShopNpc(const CShopNpc& prototype);
	~CShopNpc() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	void UpdateGUI() override;
	_bool Check_Table(PLAYER_SKILL_TYPE eType) override;
	_bool CanBePlayerCombatTarget() const override;
	_bool TryGetSkillTargetPosition(_float3& OutPosition) const override;
	_bool RequestRagdollActivation(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	_bool ResetRagdoll();
	_bool IsRagdollActive() const;

	static E::UPtr<CShopNpc> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

protected:
	void OpenShop() override;
	void PrepareDialogueCamera(const _string& cinematicName) override;
	_bool KeepDialogueCameraOnFinish() const override
	{
		return m_bWandBoxPresentationPending || m_bWandBoxPresentationActive;
	}

private:
	void SpawnWandBoxAtFirstHandShot();
	void SuspendGameplayForRagdoll();

	UPtr<CNpcRagdollController> m_pRagdollController{};
	_bool m_bRagdollGameplaySuspended{};
	_bool m_bWorldSpaceShop{};
	_float3 m_vShopPanelPositionOffset{ 0.f, 1.6f, 1.2f };
	_float3 m_vShopPanelRotationOffsetDegrees{};
	int32_t m_iDebugAnimationIndex{};
	_bool m_bDebugAnimationLoop{ true };
	_float m_fDebugAnimationSpeed{ 1.f };
	_float m_fDebugAnimationBlend{ 0.1f };
	_bool m_bDebugRootMotion{};
	_bool m_bDebugRootMotionRotation{};
	_float3 m_vDebugEntrancePosition{};
	_float3 m_vDebugEntranceControllerPosition{};
	_float4 m_qDebugEntranceRotation{ 0.f, 0.f, 0.f, 1.f };
	uint32_t m_iSpeechMouthMorph{ UINT32_MAX };
	int32_t m_iSpeechFacialAnimation{ -1 };
	_float m_fSpeechMorphTime{};
	_bool m_bSpeechMorphApplied{};
	_bool m_bSpeechUpperAnimationPlaying{};
	_bool m_bSpeechJawChannel{};
	_bool m_bSpeechLowerTeethChannel{};
	_bool m_bSpeechTongueChannel{};
	int32_t m_iWandBoxOpenAnimation{ -1 };
	_bool m_bWandBoxPresentationPending{};
	_bool m_bWandBoxPresentationActive{};
	_bool m_bWandBoxCameraStarted{};
	_float m_fWandBoxCameraElapsed{};
	_bool m_bWandShopOpenedByPresentation{};
	_bool m_bWandPresentationOwnsTimePause{};
	_bool m_bWandPurchaseDialoguePending{};
	E::CHandle m_hWandBox{};
	// Hand-socket local transform. The former NPC-root position
	// (-1.2, 3.7, 0.8) is represented by the hand bone itself now.
	_float3 m_vWandBoxLocalPosition{ -0.1f, 0.f, 0.1f };
	_float3 m_vWandBoxLocalRotation{ 46.231f, 93.596f, -168.686f };
	_float3 m_vWandBoxLocalScale{ 3.f, 3.f, 3.f };
	_float m_fWandBoxAnimationElapsed{};
	_bool m_bWandBoxAnimationPaused{};
	int32_t m_iWandBoxAttachBoneIndex{ -1 };
};

NS_END
