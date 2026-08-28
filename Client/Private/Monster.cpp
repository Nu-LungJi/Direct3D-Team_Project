#include "pch.h"
#include "Monster.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Mon_Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "Player_Magic_Bullet.h"

#include "CollBox.h"
#include "UIManager.h"
#include "UIController.h"

#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ClientEvents.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "UIManager.h"
#include "ComSound.h"
NS_USING(Client)

CMonster::CMonster()
{
}


CMonster::~CMonster()
{
	// 구독해제
	// CGameInstance::Get().EventUnsubscribeAll(GetHandle());
}

void CMonster::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
	ImGui::DragFloat("EE", &m_fIntensive, 0.1f,0.f,100.f);
	ImGui::DragFloat("DD", &m_fDissolve, 0.1f, 0.f, 1.f);
	ImGui::DragFloat3("ff", reinterpret_cast<_float*>(&m_fEMissiveColor), 0.1f,0.f, 1.f);
	ImGui::Text("NoramlAtt : %d", m_iNormalHitCnt);
	
	ImGui::Text("bPending : %s", m_bPending == true ? "TRUE" : "FALSE");
	ImGui::Text("Pending AttType : %s", MagicEnumToStringView(m_PendingMonTable.eAttType).data());
	ImGui::Text("Pending HitType : %s", MagicEnumToStringView(m_PendingMonTable.eHitType).data());

	ImGui::Separator();
	ImGui::Text("ActiveHit : %s", m_bActiveHit == true ? "TRUE" : "FALSE");
	ImGui::Text("ActiveHit AttType : %s", MagicEnumToStringView(m_ActiveMonTable.eAttType).data());
	ImGui::Text("ActiveHit HitType : %s", MagicEnumToStringView(m_ActiveMonTable.eHitType).data());
	ImGui::Separator();
	ImGui::Text("Current Attack:"); ImGui::SameLine();
	ImGui::Text(MagicEnumToStringView(m_eAttType).data());

	if (nullptr != m_pBeHavior)
		ImGui::Text("BeHavior Att : %s", Check_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK)) == true ? "ENABLE" : "DISABLE");

	ImGui::Text(m_CurEffectName.c_str());
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
			ImGui::PushID(i);
			ImGui::Text(Flags[i].pName); ImGui::SameLine();
			ImGui::Text(true == m_pBeHavior->Check_Flag(Flags[i].iValue) ? ": TRUE" : " FALSE");
			ImGui::SameLine();
			if (ImGui::Button("Invert"))
			{
				m_pBeHavior->Set_Flag(Flags[i].iValue, FLAGTYPE::INVERT);
			}
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

HRESULT CMonster::InitializePrototype(void* pArg)
{
	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResVertexGPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced");
	if (!m_pResVertexGPUSkinningInstancedShader || FAILED(m_pResVertexGPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pAnimComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation");
	if (!m_pAnimComputeShader || FAILED(m_pAnimComputeShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}


	return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	m_bDonMove = MonDesc->bDonMove;
	m_bSpawn = MonDesc->bSpawn;
	m_TargetHandle = MonDesc->TargetHandle;
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	{
		{
			CComPxRigidBody::DESC Desc{};
			Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
			if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
				ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
			{
				MSG_BOX("Create Failed ComPxRigidBody");
				return E_FAIL;
			}
		}

		{
			CComPxSphereCollider::DESC Desc{};
			Desc.pComPxRigidBody = m_pComRigidBody;
			Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
			Desc.bIsTrigger = false;
			Desc.tFilter = PX_FILTER_DESC{
				.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
				.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
				//.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
			};
			Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
			if (!Desc.pResMaterial ||
				FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
					"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
			{
				MSG_BOX("Create Failed ComPxSphereCollider");
				return E_FAIL;
			}
			if (!m_pComSphereCol->SetQueryEnabled(false))
				return E_FAIL;
		}

		//피직스
		{
			CComPxCharacterController::DESC Desc{};
			Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
			const _float fHorizontalScale =
				std::max(std::abs(MonDesc->vScale.x), std::abs(MonDesc->vScale.z));
			const _float fVerticalScale = std::abs(MonDesc->vScale.y);
			const _float3 vCenterOffset{
				MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
				MonDesc->vCCTCenterOffset.y * fVerticalScale,
				MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
			Desc.fHeight = MonDesc->fCCTHeight * fVerticalScale;
			Desc.fRadius = MonDesc->fCCTRadius * fHorizontalScale;
			Desc.fStepOffset = MonDesc->fCCTStepOffset;
			Desc.vPosition = {
				MonDesc->vPos.x + vCenterOffset.x,
				MonDesc->vPos.y + vCenterOffset.y,
				MonDesc->vPos.z + vCenterOffset.z };
			Desc.tFilter = MonDesc->tFilter;
			if (FAILED(AddComponentFromProto(
				ES_EngineProtoMajorType::PHYSX,
				ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
				"ComPxCharacterController", &Desc, &m_pCharacterController)))
			{
				return E_FAIL;
			}
		}
		//캐릭컨트롤러
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
		//캐릭 모터
		{
			CComCharacterMotor::DESC Desc{};
			Desc.pMoveIntent = m_pMoveIntent;
			Desc.pCharacterController = m_pCharacterController;
			Desc.fGravity = -9.81f;
			Desc.vControllerCenterOffset = {
				MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
				MonDesc->vCCTCenterOffset.y * std::abs(MonDesc->vScale.y),
				MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
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
		CComSound::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComSound,
			"Com_Sound",
			&Desc,
			&m_pComSound)))
		{
			return E_FAIL;
		}

		{
			CComBeHavior::BEHAVIOR_DESC Desc{};
			Desc.OwnerName = "Com_BT";
			Desc.resBeHaviorMajor = MonDesc->resBeHaviorMajor;
			Desc.resBeHaviorMinor = MonDesc->resBeHaviorMinor;
			Desc.LoadPath = MonDesc->BeHaviorTag;
			if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
			{
				return E_FAIL;
			};
		}

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
		auto* pBB = Get_BlackBoard();
		pBB->Set_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE, m_TargetHandle);
		CGameInstance::Get().EventSubscribe<FAncientMagicStart>(GetHandle(), [=]() { Stuck(); });
		GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
		GetTransform().Update();
		m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
		//m_pModelAnimator->Play_Anim(0, false);
	}
	return S_OK;
}

void CMonster::Stuck()
{

}
void CMonster::PriorityUpdate(E::_float fTimeDelta)
{
	Activate_PendingHit();
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_0))
		m_pMoveIntent->RequestWarp(_float3(20, 20, 20));
	
	m_pMoveIntent->ClearMoveIntent();
	m_pMoveIntent->ClearFacingIntent();
	__super::PriorityUpdate(fTimeDelta);
	
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DROP)| ETOUI(CBTRoot::BTFLAG::DEAD) | ETOUI(CBTRoot::BTFLAG::DEBRIS)))
		m_pCharacterMotor->SetUseGravity(true);
	else m_pCharacterMotor->SetUseGravity(false);
		
	Flag_Check(fTimeDelta);
	m_pCharacterMotor->SetGravity(-9.8f);
	m_pBeHavior->Update(fTimeDelta);
	
}

void CMonster::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	if (m_pComSound)
		m_pComSound->Update();
	Update_Animation(fTimeDelta);

	EmissiveFadeOut(fTimeDelta);
	m_pBeHavior->AbortNode();
	Update_HurtBox();
}

void CMonster::Update_Animation(_float fTimeDelta)
{
	if (m_pComModelInstance->GetModel()->GetAnimations().empty())
		return;

	m_pModelAnimator->Update(fTimeDelta);

	if (m_bRootMotionTranslationActive && m_pMoveIntent)
	{
		const _float3 vRootMotionDelta = m_pModelAnimator->GetRootMotionDelta();
		_float3 vWorldDisplacement{};
		XMStoreFloat3(&vWorldDisplacement,XMVector3Rotate(XMLoadFloat3(&vRootMotionDelta) * m_fRootMotionTranslationScale,GetTransform().GetLoadedQuaternion()));
		m_pMoveIntent->AddExternalDisplacement(vWorldDisplacement);
	}

	if (m_bRootMotionRotationActive)
	{
		const _float4 vRootMotionRotationDelta =m_pModelAnimator->GetRootMotionRotationDelta();
		GetTransform().SetQuaternion(XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&vRootMotionRotationDelta),GetTransform().GetLoadedQuaternion())));
	}
}

void CMonster::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	const _float3 vControllerPosition = m_pCharacterController->GetPosition();
	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();

	if (m_bHide) return;

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;	
	
	BoundingBox WorldBounds{};

	if (GetShadowBounds(WorldBounds)) {
		/*애니메이션 포즈가 Collider 밖으로 나가는 상황과 카메라 경계에서 깜빡이는 현상 방지*/
		constexpr _float CullPadding = 1.f;

		WorldBounds.Extents.x += CullPadding;
		WorldBounds.Extents.y += CullPadding;
		WorldBounds.Extents.z += CullPadding;

		UpdateRenderVisibility(WorldBounds);
	}
	else {

	}

	if (!ShouldSubmitRenderInstance())
		return;


	if (!pModel->GetAnimations().empty())
	{
	
		uint32_t iDissolveBits{};
		static_assert(sizeof(iDissolveBits) == sizeof(m_fDissolve));
		memcpy(&iDissolveBits, &m_fDissolve, sizeof(iDissolveBits));
		CGameInstance::Get().Add_Instance(
			m_pComModelInstance,
			m_pModelAnimator,
			*GetTransform().GetCombinedWorldMatrix(),
			iDissolveBits);

		return;
	}
}
HRESULT CMonster::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	const auto eEvaluationMode = static_cast<CComAnimator::EVALUATION_MODE>(Batch.Key.iEvaluationMode);
	if (eEvaluationMode == CComAnimator::EVALUATION_MODE::GPU)
		return Render_Instanced_GPU(pContext, ctx, Batch);

	return Render_Instanced_CPU(pContext, ctx, Batch);
}

HRESULT CMonster::Render_Instanced_CPU(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResPixelShader)
		return E_FAIL;

	const auto& vs = m_pResVertexCPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0 || iInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iInstanceCount)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
		return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pModel || !pCPUBonePaletteBuffer)
		return E_FAIL;

	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> combinedPalette(iInstanceCount * 512, identity);
	for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
	{
		const auto& combinedMatrices = Batch.CombinedBoneMatrices[instanceIndex];
		if (combinedMatrices.empty() || combinedMatrices.size() > 512)
			return E_FAIL;

		// DirectXMath로 계산한 CPU Combined 행렬을 VS의 t7 행렬 규약에 맞춘다.
		// CPU 원본은 다른 CPU 기능에서도 사용하므로 업로드 복사본만 전치한다.
		for (uint32_t boneIndex = 0; boneIndex < static_cast<uint32_t>(combinedMatrices.size()); ++boneIndex)
		{
			XMStoreFloat4x4(
				&combinedPalette[instanceIndex * 512 + boneIndex],
				XMMatrixTranspose(
					XMLoadFloat4x4(&combinedMatrices[boneIndex])));
		}
	}

	// CPU가 계산한 CombinedBone palette는 batch당 한 번만 갱신한다.
	ID3D11ShaderResourceView* nullPaletteSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &nullPaletteSRV);
	if (FAILED(pCPUBonePaletteBuffer->UpdateData(
		combinedPalette.data(),
		static_cast<uint32_t>(combinedPalette.size() * sizeof(_float4x4)))))
		return E_FAIL;



	if (FAILED(Bind_InstanceBuffer(pContext)))
		return E_FAIL;
	ID3D11ShaderResourceView* cpuBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	if (!cpuBonePaletteSRV)
		return E_FAIL;

	ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
	if (!skinBonesSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &cpuBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &skinBonesSRV);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		// Dissolve is supplied per instance through GPU_ANIM_INSTANCE_DATA::iFlags.
		m_pComModelInstance->Bind_Materials(pContext, m_fEMissiveColor, m_fIntensive, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);

	return S_OK;

}

HRESULT CMonster::Render_Instanced_GPU(ID3D11DeviceContext* pContext, const E::RENDER_CTX&, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pAnimComputeShader || !m_pResVertexGPUSkinningInstancedShader || !m_pResPixelShader)
		return E_INVALIDARG;

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0)
		return S_OK;
	if (iInstanceCount > 512)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)) ||
		FAILED(m_pComModelInstance->Bind_GPUAnimationSRVs_CS(pContext)) ||
		FAILED(Bind_InstanceBuffer_CS(pContext)) ||
		FAILED(Bind_FinalBoneUAV_CS(pContext)))
	{
		Unbind_GPUAnimation_CS(pContext);
		return E_FAIL;
	}

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(), nullptr, 0);
	pContext->Dispatch(iInstanceCount, 1, 1);
	Unbind_GPUAnimation_CS(pContext);

	if (FAILED(Bind_InstanceBuffer(pContext)) ||
		FAILED(Bind_FinalBoneSRV_VS(pContext)) ||
		FAILED(m_pComModelInstance->Bind_GPUSkinBones_VS(pContext)))
	{
		Unbind_GPUAnimation_VS(pContext);
		return E_FAIL;
	}

	const auto& vs = m_pResVertexGPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	if (!pModel)
	{
		Unbind_GPUAnimation_VS(pContext);
		return E_FAIL;
	}

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			continue;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			Unbind_GPUAnimation_VS(pContext);
			return E_FAIL;
		}
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);

		ID3D11Buffer* pSkinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &pSkinningCB);

		ID3D11Buffer* pVertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT iStride = mesh->GetVertexStride();
		const UINT iOffset = 0;
		pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &iStride, &iOffset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		// Dissolve is supplied per instance through GPU_ANIM_INSTANCE_DATA::iFlags.
		m_pComModelInstance->Bind_Materials(pContext, m_fEMissiveColor, m_fIntensive, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	Unbind_GPUAnimation_VS(pContext);
	return S_OK;
}

HRESULT CMonster::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	if (!pBuffer || !pBuffer->GetSRV())
		return E_FAIL;
	ID3D11ShaderResourceView* pSRV = pBuffer->GetSRV().Get();
	pContext->CSSetShaderResources(6, 1, &pSRV);
	return S_OK;
}

HRESULT CMonster::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	if (!pBuffer || !pBuffer->GetUAV())
		return E_FAIL;
	ID3D11ShaderResourceView* pNullSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &pNullSRV);
	ID3D11UnorderedAccessView* pUAV = pBuffer->GetUAV().Get();
	pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);
	return S_OK;
}

HRESULT CMonster::Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	if (!pBuffer || !pBuffer->GetSRV())
		return E_FAIL;
	ID3D11ShaderResourceView* pSRV = pBuffer->GetSRV().Get();
	pContext->VSSetShaderResources(7, 1, &pSRV);
	return S_OK;
}

void CMonster::Unbind_GPUAnimation_CS(ID3D11DeviceContext* pContext)
{
	ID3D11ShaderResourceView* pNullSRVs[7]{};
	pContext->CSSetShaderResources(0, 7, pNullSRVs);
	ID3D11UnorderedAccessView* pNullUAV = nullptr;
	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
	pContext->CSSetShader(nullptr, nullptr, 0);
}

void CMonster::Unbind_GPUAnimation_VS(ID3D11DeviceContext* pContext)
{
	ID3D11ShaderResourceView* pNullSRVs[4]{};
	pContext->VSSetShaderResources(6, 4, pNullSRVs);
}
HRESULT CMonster::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount = static_cast<uint32_t>(Instances.size());

	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer = pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	/*
	 * 이전 Batch에서 VS/CS에 연결되어 있을 수 있으므로
	 * Map 전에 SRV를 해제한다.
	 */
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->CSSetShaderResources(6, 1, &pNullSRV);

	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;
	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}
HRESULT CMonster::Bind_InstanceBuffer(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}

/*----------- 광윤 추가 -----------*/
HRESULT CMonster::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx){
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	const auto model = m_pComModelInstance->GetModel();
	if (!model)	return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* pSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(0, 4, pSRVs);

	return S_OK;
}
bool CMonster::GetShadowBounds(BoundingBox& OutBounds) const
{
	if (!m_pComCollider || !m_pComCollider->Get())	return false;

	CCollider* pCollider = m_pComCollider->Get();

	if (pCollider->GetCollType() != CollType::Box)	return false;

	const auto* pBox = static_cast<const CCollBox*>(pCollider);

	pBox->GetLocalBoundingBox().Transform(OutBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	OutBounds.Extents.x *= 1.25f;
	OutBounds.Extents.y *= 1.25f;
	OutBounds.Extents.z *= 1.25f;

	return true;
}
/*---------------------------------*/

void CMonster::Find_Target()
{
	auto pPhysX =CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX)return;

	PX_OVERLAP_DESC Desc{};
	// 몬스터 주변 탐색 범위
	Desc.tGeometry = {.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = 2000.f};
	Desc.tPose = {.vPosition = GetTransform().GetPosition()};

	// 플레이어 본체와 NPC 본체만 검색
	Desc.tFilter = {.iQueryMask =ETOUI(COLLISION_LAYER::PLAYER_BODY) |ETOUI(COLLISION_LAYER::NPC_BODY),
		.hIgnoreGameObject = GetHandle(),
		.bQueryStatic = false,
		.bQueryDynamic = true,
		.bIncludeTrigger = false
	};

	std::vector<PX_OVERLAP_RESULT> Results{};
	if (!pPhysX->OverlapMultiple(Desc,	Results,32))
	{
		m_TargetHandle = {};
		return;
	}

	CGameObject* pLastTarget = nullptr;
	_float fMaxDist = FLT_MAX;

	 _vector vPos =GetTransform().GetState(STATE::POSITION);

	for (const auto& Result : Results)
	{
		CGameObject* pTarget = Result.pGameObject;

		if (nullptr == pTarget || pTarget->GetPendingDestroy())
			continue;

		const _vector vTargetPos = pTarget->GetTransform().GetState(STATE::POSITION);

		const _float fDis =	XMVectorGetX(XMVector3LengthSq(vTargetPos - vPos));

		if (fDis < fMaxDist)
		{
			fMaxDist = fDis;
			pLastTarget = pTarget;
		}
	}

	if (nullptr != pLastTarget)
	{
		auto* pBB = Get_BlackBoard();
		m_TargetHandle = pLastTarget->GetHandle();
		if( nullptr != pBB)
			pBB->Set_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE, m_TargetHandle);

	}
	else
		m_TargetHandle = {};
}

void CMonster::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	if (nullptr == pObj)
		return;

	
	if (auto pPlayerMagicBullet = Cast<CPlayer_Magic_Bullet>(pObj))
	{
		Check_Table(PLAYER_SKILL_TYPE::ATTACK);
	}
	
}
_bool CMonster::Activate_PendingHit()
{
	if (!m_bPending)return false;

	_bool bSameHit = m_bActiveHit &&
		m_ActiveMonTable.eAttType == m_PendingMonTable.eAttType &&
		m_ActiveMonTable.eHitType == m_PendingMonTable.eHitType;

	if (!bSameHit)
		++m_iHitCnt;

	m_ActiveMonTable = m_PendingMonTable;
	m_bActiveHit = true;

	m_PendingMonTable = {};
	m_bPending = false;

	return true;
}

void CMonster::ReActiveTable()
{
	m_PendingMonTable = {};
	m_bPending = false;

	m_ActiveMonTable = {}; 
	m_bActiveHit = false;
	m_iHitCnt = 0;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::DEL);
}

_bool CMonster::Is_Grounded()
{
	return m_pCharacterController->IsGrounded();
}


uint32_t CMonster::Find_SkillNum(ATTMON eType)
{
	auto iter = m_MonSkillLists.find(eType);
	
	if (iter == m_MonSkillLists.end())
		return UINT_MAX;
	return iter->second;

}

_bool CMonster::Check_Flag(uint32_t iFlag)
{
	return m_pBeHavior->Check_Flag(iFlag);
}

SOUND_ID  CMonster::Play_Sound(const MONSOUND& MonSound)
{
	auto iter = m_SoundTable.find(MonSound.SoundKey);

	if (iter == m_SoundTable.end() || iter->second.empty())
		return  INVALID_SOUND_ID;

	auto& SoundPaths = iter->second;

	int32_t iSoundIndex = Engine::RandInt(0, static_cast<int32_t>(SoundPaths.size()) - 1);

	SOUND_3D_DESC Sounds = MonSound.str3DSound;
	Sounds.vPosition = GetTransform().GetPosition();

	auto id = CGameInstance::Get().GetSoundManager()->Play3D(
		SoundPaths[iSoundIndex],
		Sounds,
		MonSound.SoundPlay
	);
	if (id == INVALID_SOUND_ID)
	{
		MSG_BOX("INVALID_SOUND_ID");
	}
	return id;
}

void CMonster::Skill_Finished()
{
	m_eAttType = ATTMON::END;
	m_CurEffectName.clear();
	m_eLastSkillTable = ATTMON::END;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT) | ETOUI(CBTRoot::BTFLAG::ATTACK) | ETOUI(CBTRoot::BTFLAG::ENDHIT) |ETOUI(CBTRoot::BTFLAG::THROW),FLAGTYPE::DEL);
}

void CMonster::Get_SoundKey(_string& CurSoundName)
{
	_string Key = "";
	if (ImGui::BeginCombo("SoundTable",CurSoundName.c_str()))
	{
		for (auto&[key, value] : m_SoundTable)
		{
			_bool bSelect = key == CurSoundName;
			if (ImGui::Selectable(key.c_str(), bSelect))
			{
				CurSoundName = key;
				break;
			}

			if(bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	return;
}

const _float4x4* CMonster::Get_CombineBoneMatrix(int32_t iBoneIndex)
{
	if (iBoneIndex >= m_pComModelInstance->Get_CombinedBoneMatrices().size() || iBoneIndex < 0)
		return nullptr;

	return &m_pComModelInstance->Get_CombinedBoneMatrices()[iBoneIndex];
}

CComAnimator* CMonster::Get_Animator()
{
	return m_pModelAnimator;
}

CComCharacterMoveIntent* CMonster::Get_MoveIntent()
{
	return m_pMoveIntent;
}

CBTBlackBoard* CMonster::Get_BlackBoard()
{
	if (nullptr == m_pBeHavior) return nullptr;
	return m_pBeHavior->Get_Blackboard();
}
int32_t CMonster::Find_AnimIndex(const _string& AnimName)
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
void CMonster::Damaged(PLAYER_SKILL_TYPE eType)
{
	int32_t iRand = 0;
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		iRand = RandInt(3, 10);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), false);
		break;
	case PLAYER_SKILL_TYPE::ACCIO:
		iRand = RandInt(8, 15);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		iRand = RandInt(11, 20);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::DESCENDO:
		iRand = RandInt(15, 25);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING:
		iRand = RandInt(20, 28);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::PROTEGO:
		m_iHp -= 8.f;
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		iRand = RandInt(23, 28);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::ABRA:
		iRand = RandInt(40, 65);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::CONFRIGO:
		iRand = RandInt(9, 23);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::BOMBARDA:
		iRand = RandInt(16, 28);
		GET_SINGLE(UIManager)->CreateDamageFont(iRand, GetHandle(), true);
		break;

	}
	m_iHp -= iRand;
}

void CMonster::Update_HurtBox()
{
	_bool bHurtBoxUpdated{ false };

	// GPU 전용 평가는 CPU CombinedBoneMatrices를 매 프레임 갱신하지 않는다.
	// 오래된 본 행렬로 HurtBox를 움직이는 대신 CCT 위치 fallback을 사용한다.
	if (m_iColliderBoneIndex >= 0 && m_pComModelInstance &&
		m_pModelAnimator &&
		m_pModelAnimator->GetEvaluationMode() != CComAnimator::EVALUATION_MODE::GPU)
	{
		const auto& CombinedBones =m_pComModelInstance->Get_CombinedBoneMatrices();

		const size_t iBoneIndex =
			static_cast<size_t>(m_iColliderBoneIndex);

		if (iBoneIndex < CombinedBones.size())
		{
			const _matrix HurtBoxWorld =
				XMLoadFloat4x4(&CombinedBones[iBoneIndex]) *
				GetTransform().GetLoadedCombinedWorldMatrix();

			_vector vScale{};
			_vector vRotation{};
			_vector vTranslation{};

			if (XMMatrixDecompose(
				&vScale,
				&vRotation,
				&vTranslation,
				HurtBoxWorld))
			{
				_float4 vHurtBoxRotation{};

				// 계산한 위치를 멤버에 저장
				XMStoreFloat3(
					&m_vHurtBoxPosition,
					vTranslation);

				XMStoreFloat4(
					&vHurtBoxRotation,
					XMQuaternionNormalize(vRotation));

				bHurtBoxUpdated = m_pComRigidBody->SetKinematicTarget(m_vHurtBoxPosition,vHurtBoxRotation);
			}
		}
	}

	if (!bHurtBoxUpdated)
	{
		m_vHurtBoxPosition =
			m_pCharacterController->GetPosition();

		m_pComRigidBody->SetKinematicTarget(
			m_vHurtBoxPosition,
			GetTransform().GetQuaternion());
	}
}


void CMonster::Flag_Check(_float fTimeDelta)
{
	//이미시브
	if (m_fIntensive <= 0.f && Check_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
	{
		StartEmissive();
		m_bWork = true;
	}

	//뭔말알?
	if (m_iHp <= 0.f)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEAD), FLAGTYPE::ADD);

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::HIT)))
		m_fIntensive = 0;

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::ENDHIT)))
		Skill_Finished();

	if (!Check_Flag(ETOUI(CBTRoot::BTFLAG::LOOP)) && m_bSkillLoop)
	{
		if (m_iCurEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().StopEffect(m_iCurEffectID);
		m_bSkillLoop = false;
	}
	
}
void CMonster::EmissiveFadeOut(_float fTimeDelta)
{
	if (m_fIntensive > 0.f &&  !Check_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
	{
		m_bWork = true;
		m_fTimeTick += fTimeDelta;

		_float t = m_fTimeTick / 0.5f;

		m_fIntensive = std::lerp(m_fPreEmissive, 0, t);
		if (t >= 1.f)
		{
			m_bWork = false;
			m_fTimeTick = m_fIntensive = 0;
		}

	}

}




