#include "pch.h"
#include "Model_Instance_Manager.h"

#include "ComAnimator.h"
#include "AnimationObject.h"
#include "ComModelInstance.h"
#include "ResModel.h"

NS_USING(Engine)

CModel_Instance_Manager::CModel_Instance_Manager(): CEngineBase{}
{
}

CModel_Instance_Manager::~CModel_Instance_Manager()
{
	m_ActiveBatches.clear();
	m_InstanceBatches.clear();
}

HRESULT CModel_Instance_Manager::Initialize()
{
	m_InstanceBatches.clear();
	m_ActiveBatches.clear();

	m_iTotalInstanceCount = 0;

	return S_OK;
}

void CModel_Instance_Manager::Add_Instance(CComModelInstance* pModelInstance,CComAnimator* pAnimator,const _float4x4& WorldMatrix,uint32_t iFlags)
{
	if (!pModelInstance ||!pAnimator)
	{
		return;
	}

	const auto& pModel =pModelInstance->GetModel();

	if (!pModel)
		return;

	const auto& AnimState =pAnimator->GetCurAnimState();

	if (AnimState.iAnimIndex < 0)
		return;

	const uint32_t iAnimationCount = (uint32_t) pModel->GetAnimations().size();

	const uint32_t iAnimIndex = (uint32_t)AnimState.iAnimIndex;

	if (iAnimIndex >= iAnimationCount)
		return;

	GPU_ANIM_INSTANCE_DATA InstanceData{};

	InstanceData.WorldMatrix = WorldMatrix;

	InstanceData.iAnimIndex = iAnimIndex;

	InstanceData.iFlags = iFlags;

	InstanceData.fTrackPosition = AnimState.fTrackPosition;

	InstanceData.iRootBoneIndex = pAnimator->GetRootBoneIndex();
	
	if (pAnimator->IsBlending())
	{
		const auto& PrevAnimState = pAnimator->GetPrevAnimState();
		InstanceData.iPrevAnimIndex = static_cast<uint32_t>(PrevAnimState.iAnimIndex);
		InstanceData.fPrevTrackPosition = PrevAnimState.fTrackPosition;
		InstanceData.fBlendWeight = pAnimator->GetBlendWeight();
		InstanceData.bBlending = 1;
	}

	Add_Instance(pModelInstance, InstanceData);
}


void CModel_Instance_Manager::Add_Instance(CComModelInstance* pModelInstance,  const _float4x4& WorldMatrix, uint32_t iFlags)
{
	if (!pModelInstance)
	{
		return;
	}

	const auto& pModel = pModelInstance->GetModel();

	if (!pModel)
		return;


	GPU_ANIM_INSTANCE_DATA InstanceData{};

	InstanceData.WorldMatrix = WorldMatrix;

	InstanceData.iAnimIndex = -1;

	InstanceData.iFlags = iFlags;

	InstanceData.fTrackPosition = -1;

	InstanceData.iRootBoneIndex =-1;



	Add_Instance(pModelInstance, InstanceData);
}


void CModel_Instance_Manager::Add_Instance( CComModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance)
		return;

	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance);

	if (InstanceData.iAnimIndex == -1) {
		pBatch->bModelStatic = true;
	}
	else {
		pBatch->bModelStatic = false;

	}

	if (!pBatch)
		return;


	
	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	// Instance 번호를 알기 위해 다시 Object에 등록을 해줘야 한다.
	CGameInstance::Get().GetGameObjectByHandleT<CAnimationObject>(pBatch->ObjectHandle)->SetInstanceModelNum(m_iTotalInstanceCount);
	pBatch->Instances.push_back(InstanceData);
	++m_iTotalInstanceCount;



	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}



MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Batch( CComModelInstance* pModelInstance)
{
	if (!pModelInstance)
		return nullptr;

	const auto& pModel =pModelInstance->GetModel();

	if (!pModel)
		return nullptr;

	MODEL_INSTANCE_KEY Key{};

	Key.modelGroup = pModelInstance->Get_GroupTag();

	Key.modelTag = pModelInstance->Get_ResTag();

	auto Iter = m_InstanceBatches.find(Key);

	if (Iter != m_InstanceBatches.end())
	{
		return Iter->second.get();
	}

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();

	if (!pBatch)
		return nullptr;

	pBatch->Key =Key;

	// 인스턴스 개수 대충 넣은거
	pBatch->Instances.reserve(16);

	MODEL_INSTANCE_BATCH* pBatchRaw =pBatch.get();

	m_InstanceBatches.emplace(Key,std::move(pBatch));

	return pBatchRaw;
}

void CModel_Instance_Manager::Clear_Frame()
{
	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches)
	{
		if (!pBatch)
			continue;

		pBatch->Instances.clear();

		pBatch->bActiveThisFrame = false;
	}

	m_ActiveBatches.clear();

	m_iTotalInstanceCount = 0;
}

void CModel_Instance_Manager::UpdateGUI()
{
	if (!ImGui::Begin("Model Instance Manager"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Registered Batch Count : %zu",m_InstanceBatches.size());

	ImGui::Text("Active Batch Count : %zu",m_ActiveBatches.size());

	ImGui::Text("Total Instance Count : %u",m_iTotalInstanceCount);

	ImGui::Separator();

	uint32_t iBatchIndex = 0;

	for (const MODEL_INSTANCE_BATCH* pBatch :m_ActiveBatches)
	{
		if (!pBatch)
			continue;

		ImGui::PushID(
			static_cast<int32_t>(
				iBatchIndex));

		const uint32_t iInstanceCount =
			static_cast<uint32_t>(
				pBatch->Instances.size());

		char szTreeLabel[64]{};

		sprintf_s(
			szTreeLabel,
			"Batch %u",
			iBatchIndex);

		if (ImGui::TreeNode(szTreeLabel))
		{
			

			ImGui::Text(
				"Instance Count : %u",
				iInstanceCount);

			ImGui::Text(
				"Vector Capacity : %zu",
				pBatch->Instances.capacity());

			ImGui::Text(
				"Active : %s",
				pBatch->bActiveThisFrame
				? "True"
				: "False");

			ImGui::TreePop();
		}

		ImGui::PopID();

		++iBatchIndex;
	}

	ImGui::End();

}

HRESULT CModel_Instance_Manager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	

	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches)
	{
		if (!pBatch || pBatch->Instances.empty() )
			continue;

		if (!pBatch->bModelStatic)
		{
			auto* pAnimationObject = CGameInstance::Get().GetGameObjectByHandleT<CAnimationObject>(pBatch->ObjectHandle);

			if (pAnimationObject == nullptr)
				return E_FAIL;

			if (FAILED(pAnimationObject->Render_Instanced(pContext, ctx, *pBatch)))
				return E_FAIL;
		}
		else {
			auto* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle);

			if (pObject == nullptr)
				return E_FAIL;

			if (FAILED(pObject->Render(pContext, ctx, *pBatch)))
				return E_FAIL;
		}


	}

	return S_OK;
}

UPtr<CModel_Instance_Manager> CModel_Instance_Manager::Create()
{
	auto pInstance =ToUPtr(new CModel_Instance_Manager{});

	if (FAILED( pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CModel_Instance_Manager");

		return nullptr;
	}

	return pInstance;
}
