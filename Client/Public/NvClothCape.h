#pragma once

#include "Client_Defines.h"
#include "Engine_NvClothDefines.h"
#include "GameObject.h"
#include "NvClothCollisionRigData.h"

#include <array>

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComModelInstance;
class CComNvCloth;
class CResNvClothMesh;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CNvClothCape final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CNvClothCape, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hTarget{};
		StringID sResourceGroup{};
		StringID sModelResourceTag{};
		StringID sClothMeshResourceTag{};
		StringID sTargetModelComponentTag{};
		_string sAttachBoneName{};
		_float3 vLocalPosition{};
		_float fTeleportDistance{ 3.f };
		_bool bUseBackstop{ true };
		_bool bFlipBackstopNormal{};
		_float fBackstopRadius{ 5.f };
		_float fBackstopOffset{};
		_float fBackstopFullRatio{ 0.35f };
		_float fBackstopFadeEndRatio{ 0.9f };
		_float fBackstopFadeDepth{ 0.15f };
		_bool bUseVirtualParticles{};

		// 래그돌 Authoring 데이터를 망토 몸 충돌 리그의 공용 원본으로 사용한다.
		NVCLOTH_COLLISION_RIG_DESC
			tBodyCollisionRig{};
	};

private:
	CNvClothCape();
	CNvClothCape(const CNvClothCape& Prototype);
	~CNvClothCape() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& Context) override;
	HRESULT Render_Shadow(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& Context) override;

	static UPtr<CNvClothCape> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	struct BODY_COLLISION_BONE
	{
		_string sBoneName{};
		int32_t iBoneIndex{ -1 };
		_float fRadius{};
	};

	struct DEBUG_BODY_COLLISION_SHAPE
	{
		NVCLOTH_COLLISION_SHAPE_TYPE eType{
			NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE };
		_float4x4 SimulationPose{};
		_float3 vHalfExtents{};
		_float fRadius{};
		_float fHalfHeight{};
	};

private:
	_bool UpdateAttachment(_bool bUpdateSimulation);
	_bool ResolveAttachment();
	_bool UpdateBodyCollisions();
	_bool BuildBodyCollisionsFromRig(
		CComModelInstance& ModelInstance,
		_fmatrix TargetWorld,
		_fmatrix InverseSimulationWorld,
		NVCLOTH_COLLISION_DESC& OutDesc);
	void DebugDrawBodyCollisions();
	_bool UpdateAnimationConstraints(
		CComModelInstance& ModelInstance,
		_fmatrix AttachmentWorld,
		_bool bResetPreviousParticles);
	_bool GetTargetBoneMatrix(
		CComModelInstance& ModelInstance,
		int32_t iBoneIndex,
		_matrix& OutBoneMatrix) const;

private:
	CHandle m_hTarget{};
	_float4x4 m_ParentWorld{};
	StringID m_sTargetModelComponentTag{};
	_string m_sAttachBoneName{};
	int32_t m_iAttachBoneIndex{ -1 };
	_float m_fTeleportDistance{ 3.f };
	_float3 m_vPreviousAttachPosition{};
	_bool m_bAttachmentInitialized{};
	_bool m_bSimulationTransformInitialized{};
	_bool m_bAnimationConstraintInitialized{};
	std::vector<int32_t>
		m_ResolvedSkinBoneIndices{};
	std::vector<_float4x4>
		m_SkinBoneToSimulationMatrices{};
	NVCLOTH_ANIMATION_CONSTRAINT_DESC
		m_AnimationConstraintDesc{};
	std::array<BODY_COLLISION_BONE, 5>
		m_BodyCollisionBones{ {
			{ "Spine2", -1, 0.30f },
			{ "Spine3", -1, 0.32f },
			{ "Neck", -1, 0.18f },
			{ "LeftShoulder", -1, 0.22f },
			{ "RightShoulder", -1, 0.22f }
		} };
	_bool m_bContinuousBodyCollision{ true };
	_float m_fCollisionMassScale{ 1.f };
	_float m_fCollisionFriction{};
	_bool m_bUseBackstop{ true };
	_bool m_bFlipBackstopNormal{};
	_float m_fBackstopRadius{ 5.f };
	_float m_fBackstopOffset{};
	_float m_fBackstopFullRatio{ 0.35f };
	_float m_fBackstopFadeEndRatio{ 0.9f };
	_float m_fBackstopFadeDepth{ 0.15f };
	_bool m_bUseVirtualParticles{};
	NVCLOTH_COLLISION_RIG_DESC
		m_BodyCollisionRig{};
	std::vector<int32_t>
		m_CollisionRigBoneIndices{};
	_bool m_bDebugBodyCollisions{};
	NVCLOTH_COLLISION_DESC
		m_LastBodyCollisionDesc{};
	std::vector<DEBUG_BODY_COLLISION_SHAPE>
		m_DebugBodyCollisionShapes{};
	_bool m_bRenderCape{ true };
	SPtr<CResNvClothMesh> m_pClothMesh{};
	CComModelInstance* m_pComModelInstance{};
	CComNvCloth* m_pComNvCloth{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResVertexShader> m_pShadowVertexShader{};
	SPtr<CResVertexShader> m_pPointShadowVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};
};

NS_END
