#include "pch.h"
#include "Model_Instance_Manager.h"

#include "ComAnimator.h"
#include "AnimationObject.h"
#include "ComModelInstance.h"
#include "ComStaticModelInstance.h"
#include "ComStaticModelInstance.h"
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

	const auto eAnimatorMode = pAnimator->GetEvaluationMode();
	// CPU 단독 모드는 구현 보존용이다. 소환·배치는 CPU+GPU 스키닝 경로로 정규화한다.
	const uint32_t iEvaluationMode = static_cast<uint32_t>(eAnimatorMode == CComAnimator::EVALUATION_MODE::CPU? CComAnimator::EVALUATION_MODE::CPU_GPU: eAnimatorMode);
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, false, iEvaluationMode);
	if (!pBatch)
		return;

	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
		pObject->SetInstanceModelNum(iBatchInstanceIndex);

	pBatch->Instances.push_back(InstanceData);
	pBatch->CombinedBoneMatrices.push_back(pModelInstance->Get_CombinedBoneMatrices());
	++m_iTotalInstanceCount;
	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}


void CModel_Instance_Manager::Add_Instance(CComStaticModelInstance* pModelInstance,  const _float4x4& WorldMatrix, uint32_t iFlags)
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

	InstanceData.iAnimIndex = INVALID_ANIM_INDEX;

	InstanceData.iFlags = iFlags;

	InstanceData.fTrackPosition = 0.f;

	InstanceData.iRootBoneIndex = 0;



	Add_Instance(pModelInstance, InstanceData);
}


void CModel_Instance_Manager::Add_Instance( CComModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance)
		return;

	const _bool bStaticModel = (InstanceData.iAnimIndex == INVALID_ANIM_INDEX);
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, bStaticModel);

	if (!pBatch)
		return;


	
	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	// Instance 踰덊?�瑜????�� ?꾪빐 ??�떆 Object???깅줉????�쨾????�떎.
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
	{
		pObject->SetInstanceModelNum(iBatchInstanceIndex);
	}
	pBatch->Instances.push_back(InstanceData);
	++m_iTotalInstanceCount;



	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}

void CModel_Instance_Manager::Add_Part_Instance(CComStaticModelInstance* pModelInstance, const GPU_PART_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance || !pModelInstance->GetModel())
		return;

	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Part_Batch(pModelInstance);
	if (!pBatch)
		return;

	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	pBatch->PartInstances.push_back(InstanceData);
	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}
void CModel_Instance_Manager::Add_Instance(CComStaticModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance)
		return;

	const _bool bStaticModel = (InstanceData.iAnimIndex == INVALID_ANIM_INDEX);
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, bStaticModel);

	if (!pBatch)
		return;



	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	// Instance 踰덊?�瑜????�� ?꾪빐 ??�떆 Object???깅줉????�쨾????�떎.
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
	{
		pObject->SetInstanceModelNum(iBatchInstanceIndex);
	}
	pBatch->Instances.push_back(InstanceData);
	++m_iTotalInstanceCount;



	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}



MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Batch(CComModelInstance* pModelInstance, _bool bStaticModel, uint32_t iEvaluationMode)
{
	if (!pModelInstance)
		return nullptr;

	const auto& pModel =pModelInstance->GetModel();

	if (!pModel)
		return nullptr;

	MODEL_INSTANCE_KEY Key{};

	Key.modelGroup = pModelInstance->Get_GroupTag();

	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = bStaticModel;
	Key.iEvaluationMode = iEvaluationMode;

	auto Iter = m_InstanceBatches.find(Key);

	if (Iter != m_InstanceBatches.end())
	{
		return Iter->second.get();
	}

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();

	if (!pBatch)
		return nullptr;

	pBatch->Key =Key;
	pBatch->bModelStatic = bStaticModel;


	pBatch->Instances.reserve(16);
	pBatch->CombinedBoneMatrices.reserve(16);

	MODEL_INSTANCE_BATCH* pBatchRaw =pBatch.get();

	m_InstanceBatches.emplace(Key,std::move(pBatch));

	return pBatchRaw;
}

MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Part_Batch(CComStaticModelInstance* pModelInstance)
{
	MODEL_INSTANCE_KEY Key{};
	Key.modelGroup = pModelInstance->Get_GroupTag();
	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = true;

	auto iter = m_InstanceBatches.find(Key);
	if (iter != m_InstanceBatches.end())
		return iter->second.get();

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();
	pBatch->Key = Key;
	pBatch->bModelStatic = true;
	pBatch->PartInstances.reserve(16);
	auto* pBatchRaw = pBatch.get();
	m_InstanceBatches.emplace(Key, std::move(pBatch));
	return pBatchRaw;
}

MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Batch(CComStaticModelInstance* pModelInstance, _bool bStaticModel)
{
	if (!pModelInstance)
		return nullptr;

	const auto& pModel = pModelInstance->GetModel();

	if (!pModel)
		return nullptr;

	MODEL_INSTANCE_KEY Key{};

	Key.modelGroup = pModelInstance->Get_GroupTag();

	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = bStaticModel;

	auto Iter = m_InstanceBatches.find(Key);

	if (Iter != m_InstanceBatches.end())
	{
		return Iter->second.get();
	}

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();

	if (!pBatch)
		return nullptr;

	pBatch->Key = Key;
	pBatch->bModelStatic = bStaticModel;

	// ?몄뒪??�뒪 媛쒖???????�?�?
	pBatch->Instances.reserve(16);
	pBatch->CombinedBoneMatrices.reserve(16);

	MODEL_INSTANCE_BATCH* pBatchRaw = pBatch.get();

	m_InstanceBatches.emplace(Key, std::move(pBatch));

	return pBatchRaw;
}
void CModel_Instance_Manager::Clear_Frame()
{
	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches)
	{
		if (!pBatch)
			continue;

		pBatch->Instances.clear();
		pBatch->PartInstances.clear();
		pBatch->CombinedBoneMatrices.clear();

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
		if (!pBatch || (pBatch->Instances.empty() && pBatch->PartInstances.empty()))
			continue;

		auto* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle);
		if (pObject == nullptr)
			return E_FAIL;

		if (FAILED(pObject->Render_Instanced(pContext, ctx, *pBatch)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CModel_Instance_Manager::Render_ShadowInstanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, _bool bStaticBatch){
	
	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches) {
		if (!pBatch || (pBatch->Instances.empty()))		continue;

		if (pBatch->bModelStatic != bStaticBatch)
			continue;

		auto pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle);
		if (nullptr == pObject) continue;

		if (FAILED(pObject->Render_ShadowInstanced(pContext, ctx, *pBatch)))	return E_FAIL;
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
