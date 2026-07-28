#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

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

	static UPtr<CNvClothCape> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	struct BODY_COLLISION_BONE
	{
		_string sBoneName{};
		int32_t iBoneIndex{ -1 };
		_float fRadius{};
	};

private:
	_bool UpdateAttachment(_bool bUpdateSimulation);
	_bool ResolveAttachment();
	_bool UpdateBodyCollisions();
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
	std::array<BODY_COLLISION_BONE, 5>
		m_BodyCollisionBones{ {
			{ "Spine2", -1, 0.30f },
			{ "Spine3", -1, 0.32f },
			{ "Neck", -1, 0.18f },
			{ "LeftShoulder", -1, 0.22f },
			{ "RightShoulder", -1, 0.22f }
		} };
	_bool m_bContinuousBodyCollision{ true };
	_float m_fCollisionMassScale{ 10.f };
	_float m_fCollisionFriction{ 0.2f };
	SPtr<CResNvClothMesh> m_pClothMesh{};
	CComModelInstance* m_pComModelInstance{};
	CComNvCloth* m_pComNvCloth{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};
};

NS_END
