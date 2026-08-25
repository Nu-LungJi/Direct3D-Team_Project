#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CResStaticModel;

// 현재 로드된 청크의 맵 메시 인스턴스를 모델과 렌더 기능별로 수집한다.
// GPU 리소스는 소유하지 않으며 청크 구성이 변경될 때만 다시 만들어진다.
class CMapMeshInstanceBatchCollector final
{
public:
	// 같은 모델과 같은 렌더 기능을 하나의 인스턴싱 배치로 묶는 키
	using MODEL_RENDER_KEY = std::pair<SPtr<CResStaticModel>, EMapMeshRenderFeature>;

	// MODEL_RENDER_KEY를 unordered_map에서 사용할 수 있도록 결합 해시를 만든다
	struct MODEL_RENDER_KEY_HASH
	{
		size_t operator()(const MODEL_RENDER_KEY& key) const noexcept
		{
			const auto& [model, renderFeature] = key;
			const size_t modelHash = std::hash<SPtr<CResStaticModel>>{}(model);
			const size_t featureHash = std::hash<uint32_t>{}(static_cast<uint32_t>(renderFeature));
			return modelHash ^ (featureHash << 1);
		}
	};

	// 현재 상주 장면의 모델,렌더 기능별 인스턴스 배치 저장소
	using BATCH_MAP = std::unordered_map<MODEL_RENDER_KEY, MAPMESH_INSTANCE_BATCH, MODEL_RENDER_KEY_HASH>;

public:
	// 인스턴스와 컬링 데이터를 같은 배치의 동일한 인덱스에 추가
	HRESULT AddInstance(const SPtr<CResStaticModel>& model, EMapMeshRenderFeature renderFeature, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData);
	// 상주 장면을 다시 만들거나 해제할 때 모든 배치를 비운다.
	void ClearBatches();

	// PrepareDrawPacket과 통계 계산에서 수집 상태를 조회한다
	_bool IsEmpty() const { return m_Batches.empty(); }
	size_t GetBatchCount() const { return m_Batches.size(); }
	uint32_t GetInstanceCount() const;
	const BATCH_MAP& GetBatches() const { return m_Batches; }

private:
	// 다음 청크 변경 전까지 유지되는 CPU 인스턴스 배치 저장소다.
	BATCH_MAP m_Batches;
};

NS_END
