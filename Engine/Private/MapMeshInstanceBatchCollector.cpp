#include "pch.h"
#include "MapMeshInstanceBatchCollector.h"

NS_USING(Engine)

HRESULT CMapMeshInstanceBatchCollector::AddInstance(const SPtr<CResStaticModel>& model, EMapMeshRenderFeature renderFeature, 
	const MAPMESH_INSTANCE_DATA& instanceData,
	MAPMESH_OCCLUSION_DATA& occlusionData)
{
	if (model == nullptr)
		return E_INVALIDARG;

	// 같은 모델과 렌더 기능은 동일한 인스턴스 배열에 연속해서 저장한다
	auto& batch = m_Batches[MODEL_RENDER_KEY{ model, renderFeature }];
	// 컬링 결과가 원래 인스턴스를 다시 찾을 수 있도록 배치 내부 위치를 기록한다
	occlusionData.instanceIndex = static_cast<uint32_t>(batch.instances.size());
	batch.instances.push_back(instanceData);
	batch.occlusionData.push_back(occlusionData);

	return S_OK;
}

void CMapMeshInstanceBatchCollector::ClearBatches()
{
	m_Batches.clear();
}

uint32_t CMapMeshInstanceBatchCollector::GetInstanceCount() const
{
	uint32_t instanceCount = 0;
	for (const auto& batchEntry : m_Batches)
	{
		instanceCount += static_cast<uint32_t>(batchEntry.second.instances.size());
	}

	return instanceCount;
}
