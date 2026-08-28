#include "pch.h"
#include "WorldAgent.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"

#include "CollBox.h"
#include "UIManager.h"
#include "UIController.h"

#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ClientEvents.h"

#include "UIManager.h"
#include "ComSound.h"

#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
NS_USING(Client)

CWorldAgent::CWorldAgent()
{
}


CWorldAgent::~CWorldAgent()
{
	// 구독해제
	// CGameInstance::Get().EventUnsubscribeAll(GetHandle());
}

void CWorldAgent::UpdateGUI()
{
	__super::UpdateGUI();
}

HRESULT CWorldAgent::InitializePrototype(void* pArg)
{
	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
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

HRESULT CWorldAgent::Initialize(void* pArg)
{
	auto WorldAgentDesc = static_cast<WORLD_AGENT_DESC*>(pArg);
	m_TargetHandle = WorldAgentDesc->TargetHandle;
	m_iHp = 1;

	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	{
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

		if (WorldAgentDesc->bPhyx)
		{

			{
				CComPxRigidBody::DESC Desc{};
				Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
				if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
					ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
				{
					MSG_BOX("Create Failed ComPxRigidBody Npc");
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
					MSG_BOX("Create Failed ComPxSphereCollider Npc");
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
					std::max(std::abs(WorldAgentDesc->vScale.x), std::abs(WorldAgentDesc->vScale.z));

				const _float3 vCenterOffset{
					WorldAgentDesc->vCCTCenterOffset.x ,
					WorldAgentDesc->vCCTCenterOffset.y,
					WorldAgentDesc->vCCTCenterOffset.z };
				Desc.fHeight = WorldAgentDesc->fCCTHeight;
				Desc.fRadius = WorldAgentDesc->fCCTRadius * fHorizontalScale;
				Desc.fStepOffset = WorldAgentDesc->fCCTStepOffset;
				Desc.vPosition = {
					WorldAgentDesc->vPos.x + vCenterOffset.x,
					WorldAgentDesc->vPos.y + vCenterOffset.y,
					WorldAgentDesc->vPos.z + vCenterOffset.z };
				Desc.tFilter = WorldAgentDesc->tFilter;
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
					WorldAgentDesc->vCCTCenterOffset.x * WorldAgentDesc->vScale.x,
					WorldAgentDesc->vCCTCenterOffset.y * std::abs(WorldAgentDesc->vScale.y),
					WorldAgentDesc->vCCTCenterOffset.z * WorldAgentDesc->vScale.z };
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
		}

		{
			CComBeHavior::BEHAVIOR_DESC Desc{};
			Desc.OwnerName = "Com_BT";
			Desc.resBeHaviorMajor = WorldAgentDesc->resBeHaviorMajor;
			Desc.resBeHaviorMinor = WorldAgentDesc->resBeHaviorMinor;
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
			Desc.sGroupTag = WorldAgentDesc->LevelTag;
			Desc.sResTag = WorldAgentDesc->ReSourceTag;

			if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
			{
				MessageBoxA(g_hWnd, WorldAgentDesc->ReSourceTag.c_str(), "hm", MB_OK);
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
		m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
		m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
		//m_pModelAnimator->Play_Anim(0, true,Randf(0.1f,1.f));
		m_vPos = WorldAgentDesc->vPos;
		GetTransform().SetPosition(XMLoadFloat3(&WorldAgentDesc->vPos));
		if(nullptr != m_pCharacterController)
			GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
		GetTransform().Update();

		auto* pBB = Get_BlackBoard();
		pBB->Set_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE, m_TargetHandle);
		
		if (!WorldAgentDesc->AnimName.empty())
		{
			pBB->Set_Value<_string>(PUBLIC_KEY::ANIMNAME, WorldAgentDesc->AnimName);
		}
		CGameInstance::Get().EventSubscribe<FAncientMagicStart>(GetHandle(), [=]() { Stuck(); });

	}
	return S_OK;
}

void CWorldAgent::Stuck()
{

}

void CWorldAgent::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)) || m_iHp <= 0)
		SetPendingDestroy();

	if (nullptr != m_pMoveIntent)
	{
		m_pMoveIntent->ClearMoveIntent();
		m_pMoveIntent->ClearFacingIntent();
	}
	__super::PriorityUpdate(fTimeDelta);
	if (nullptr != m_pCharacterMotor)
	{
		if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DROP) | ETOUI(CBTRoot::BTFLAG::DEAD) | ETOUI(CBTRoot::BTFLAG::DEBRIS)))
			m_pCharacterMotor->SetUseGravity(true);
		else m_pCharacterMotor->SetUseGravity(false);
	}
	m_pBeHavior->Update(fTimeDelta);
	
}

void CWorldAgent::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	if (nullptr != m_pComSound)
		m_pComSound->Update();
	Update_Animation(fTimeDelta);

	m_pBeHavior->AbortNode();
}

void CWorldAgent::Update_Animation(_float fTimeDelta)
{

	if (m_pComModelInstance->GetModel()->GetAnimations().empty())
		return;

	m_pModelAnimator->Update(fTimeDelta);

	if (m_bRootMotionTranslationActive && m_pMoveIntent)
	{
		const _float3 vRootMotionDelta = m_pModelAnimator->GetRootMotionDelta();
		_float3 vWorldDisplacement{};
		XMStoreFloat3(&vWorldDisplacement, XMVector3Rotate(XMLoadFloat3(&vRootMotionDelta) * m_fRootMotionTranslationScale, GetTransform().GetLoadedQuaternion()));
		m_pMoveIntent->AddExternalDisplacement(vWorldDisplacement);
	}

	if (m_bRootMotionRotationActive)
	{
		const _float4 vRootMotionRotationDelta = m_pModelAnimator->GetRootMotionRotationDelta();
		GetTransform().SetQuaternion(XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&vRootMotionRotationDelta), GetTransform().GetLoadedQuaternion())));
	}
}

void CWorldAgent::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	if (nullptr != m_pCharacterController)
	{
		const _float3 vControllerPosition = m_pCharacterController->GetPosition();
		GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	}

	GetTransform().Update();

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());

		return;
	}
}
HRESULT CWorldAgent::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
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

	const auto& meshes = pModel->GetMeshes();
	const auto& materials = pModel->GetMaterials();
	const uint32_t meshCount = std::min<uint32_t>(
		pModel->Get_NumMeshes(), static_cast<uint32_t>(meshes.size()));
	for (uint32_t iMeshIndex = 0; iMeshIndex < meshCount; ++iMeshIndex)
	{
		const auto& mesh = meshes[iMeshIndex];
		if (!mesh)
			continue;
		const uint32_t materialIndex = mesh->Get_MaterialIndex();
		if (materialIndex >= materials.size() || !materials[materialIndex])
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
		// 올리밴더의 5번 머리카락 카드만 알파 컷을 낮춰 가닥 밀도를 높인다.
		// 다른 캐릭터와 속눈썹 머티리얼은 기존 0.35 기준을 유지한다.
		const _bool bGerbold =
			Batch.Key.modelTag == StringID{ "Model_Resource_NPC_GerboldOllivander" };
		const _bool bGerboldHair = bGerbold && materialIndex == 5u;
		const _bool bGerboldOpaqueBody =
			bGerbold && materialIndex >= 6u && materialIndex <= 8u;
		const _float fAlphaClipThreshold =
			bGerboldOpaqueBody ? 0.f : (bGerboldHair ? 0.12f : 0.35f);
		m_pComModelInstance->Bind_Materials(
			pContext, _float3{ 1,1,1 }, 0, { 1.f, 1.f, 1.f },
			m_fDissolve, 1.f, 1.f, 1.f, 1.f, 1.f, fAlphaClipThreshold);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);

	return S_OK;

}
HRESULT CWorldAgent::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
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
HRESULT CWorldAgent::Bind_InstanceBuffer(ID3D11DeviceContext* pContext)
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
HRESULT CWorldAgent::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
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
bool CWorldAgent::GetShadowBounds(BoundingBox& OutBounds) const
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
_bool CWorldAgent::OnQueryHit(int32_t iDamage)
{
	if (iDamage <= 0 || m_iHp <= 0)
		return false;

	m_iHp -= iDamage;

	if (m_iHp <= 0)

	return true;
}
void CWorldAgent::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	if (nullptr == pObj)
		return;

}

SOUND_ID  CWorldAgent::Play_Sound(const MONSOUND& MonSound)
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

void CWorldAgent::Get_SoundKey(_string& CurSoundName)
{
	_string Key = "";
	if (ImGui::BeginCombo("SoundTable", CurSoundName.c_str()))
	{
		for (auto& [key, value] : m_SoundTable)
		{
			_bool bSelect = key == CurSoundName;
			if (ImGui::Selectable(key.c_str(), bSelect))
			{
				CurSoundName = key;
				break;
			}

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	return;
}

const _float4x4* CWorldAgent::Get_CombineBoneMatrix(int32_t iBoneIndex)
{
	if (iBoneIndex >= m_pComModelInstance->Get_CombinedBoneMatrices().size() || iBoneIndex < 0)
		return nullptr;

	return &m_pComModelInstance->Get_CombinedBoneMatrices()[iBoneIndex];
}

CComAnimator* CWorldAgent::Get_Animator()
{
	return m_pModelAnimator;
}

CComCharacterMoveIntent* CWorldAgent::Get_MoveIntent()
{
	return m_pMoveIntent;
}

CBTBlackBoard* CWorldAgent::Get_BlackBoard()
{
	if (nullptr == m_pBeHavior) return nullptr;
	return m_pBeHavior->Get_Blackboard();
}
int32_t CWorldAgent::Find_AnimIndex(const _string& AnimName)
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
void CWorldAgent::Damaged(PLAYER_SKILL_TYPE eType)
{
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		GET_SINGLE(UIManager)->CreateDamageFont(5, GetHandle(), false);
		m_iHp -= 5.f;
		break;
	case PLAYER_SKILL_TYPE::ACCIO:
		GET_SINGLE(UIManager)->CreateDamageFont(10, GetHandle(), true);
		m_iHp -= 10.f;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		GET_SINGLE(UIManager)->CreateDamageFont(15, GetHandle(), true);
		m_iHp -= 15.f;
		break;
	case PLAYER_SKILL_TYPE::DESCENDO:
		GET_SINGLE(UIManager)->CreateDamageFont(20, GetHandle(), true);
		m_iHp -= 20.f;
		break;
	case PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING:
		GET_SINGLE(UIManager)->CreateDamageFont(25, GetHandle(), true);
		m_iHp -= 25.f;
		break;
	case PLAYER_SKILL_TYPE::PROTEGO:
		m_iHp -= 8.f;
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		m_iHp -= 25.f;
		break;
	case PLAYER_SKILL_TYPE::ABRA:
		m_iHp -= 50.f;
		GET_SINGLE(UIManager)->CreateDamageFont(25, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::CONFRIGO:
		m_iHp -= 18.f;
		GET_SINGLE(UIManager)->CreateDamageFont(18, GetHandle(), true);
		break;
	case PLAYER_SKILL_TYPE::BOMBARDA:
		m_iHp -= 18.f;
		GET_SINGLE(UIManager)->CreateDamageFont(28, GetHandle(), true);
		break;

	}
}


