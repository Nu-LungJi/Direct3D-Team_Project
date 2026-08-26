#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CResStaticModel;
class CResTexture2D;

// 모델의 머티리얼에서 메시별 텍스처를 한 번 조회해 보관하고,
// 반복되는 Draw에서 동일한 텍스처 조회 비용을 줄인다
class CMapMeshTextureCache final
{
public:
	// 픽셀 셰이더에 연속으로 바인딩하는 텍스처 슬롯 개수
	static constexpr size_t TEXTURE_COUNT = 4;
	// 배열 순서: Diffuse, Normal, SMRO(Metalness), Emissive
	using MESH_TEXTURE_SET = std::array<SPtr<CResTexture2D>, TEXTURE_COUNT>;
	// 모델의 메시 인덱스로 접근하는 텍스처 세트 배열
	using MODEL_TEXTURE_SETS = std::vector<MESH_TEXTURE_SET>;

public:
	// 캐시가 있으면 재사용하고, 없으면 모델 전체 메시의 텍스처 세트를 생성
	const MODEL_TEXTURE_SETS* GetOrCreateTextureSets(const SPtr<CResStaticModel>& model);
	// 지정한 메시의 텍스처 네 장을 픽셀 셰이더 슬롯 0번부터 바인딩한다
	HRESULT BindTextures(ID3D11DeviceContext* context, const MODEL_TEXTURE_SETS& textureSets, uint32_t meshIndex) const;
	// 모든 모델의 텍스처 캐시를 제거
	void ClearAll();
	// 스트리밍으로 해제되는 모델 하나의 캐시만 제거
	void EraseModel(const SPtr<CResStaticModel>& model);

private:
	// 모델의 특정 메시가 참조하는 재질에서 요청 타입의 첫 텍스처를 찾는다
	SPtr<CResTexture2D> GetMapMeshTexture(const SPtr<CResStaticModel>& model, uint32_t meshIndex, AI_TEXTURE_TYPE materialType);

private:
	// 모델별로 각 메시가 사용할 텍스처 세트를 캐싱
	std::unordered_map<SPtr<CResStaticModel>, MODEL_TEXTURE_SETS> m_ModelTextureSets;
};

NS_END
