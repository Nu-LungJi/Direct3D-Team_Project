#pragma once

#include "Engine_NvClothDefines.h"
#include "Engine_Struct_Vertex.h"
#include "ResDynamicVIBuffer.h"
#include "Resource.h"

#include <array>
#include <limits>

NS_BEGIN(Engine)

class ENGINE_DLL CResNvClothMesh final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResNvClothMesh, CResource)

	struct DESC
	{
		// 모델 원본 좌표계를 엔진 좌표계로 맞추는 선행 변환이다.
		_matrix PreTransformMatrix;

		// Rest Pose 메시를 이 본의 로컬 좌표계로 변환해 시뮬레이션한다.
		// 런타임에 망토를 부착하는 본과 동일하게 설정하는 것이 원칙이다.
		_string sSimulationAnchorBone{};

		// 미지정 시 가장 작은 메시를 저폴리 시뮬레이션 메시로 선택한다.
		uint32_t iSimulationMeshIndex{
			std::numeric_limits<uint32_t>::max() };

		// 미지정 시 가장 큰 메시를 하이폴리 렌더 메시로 선택한다.
		uint32_t iRenderMeshIndex{
			std::numeric_limits<uint32_t>::max() };

		// 이 거리 안의 저폴리 중복 정점을 하나의 Cloth 파티클로 합친다.
		_float fWeldTolerance{ 1.e-5f };

		// 시뮬레이션 메시 상단에서 고정할 높이 비율이다.
		_float fFixedTopRatio{ 0.08f };

		// 상단에서 하단으로 증가하는 Motion Constraint의 최대 반경 배율이다.
		// 1이면 Rest Pose 망토 높이까지 움직일 수 있다.
		_float fMaxDistanceScale{ 1.f };
	};

	struct PARTICLE_SKIN_INFLUENCE
	{
		uint32_t iSourceBoneIndex{
			std::numeric_limits<uint32_t>::max() };
		_float fWeight{};

		// Source Rest Pose에서 해당 본의 로컬 좌표로 변환한 위치다.
		_float3 vBoneLocalPosition{};
	};

	struct PARTICLE_SKIN_BINDING
	{
		std::array<PARTICLE_SKIN_INFLUENCE, 4>
			Influences{};
		_float fMaxDistance{};
	};

	struct SECTION
	{
		// 원본 모델의 텍스처 및 머티리얼을 다시 바인딩하기 위한 식별값이다.
		uint32_t iSourceMeshIndex{};
		uint32_t iMaterialIndex{};

		// 하이폴리 정점의 Cloth 매핑 정보와 원본 인덱스를 보관한다.
		SPtr<CResDynamicVIBuffer> pVIBuffer{};
	};

private:
	explicit CResNvClothMesh(const _string& sPath);
	~CResNvClothMesh() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	const NVCLOTH_FABRIC_DESC& GetFabricDesc() const
	{
		return m_tFabricDesc;
	}

	const std::vector<SECTION>& GetSections() const
	{
		return m_Sections;
	}

	uint32_t GetParticleCount() const
	{
		return static_cast<uint32_t>(
			m_tFabricDesc.vecPositions.size());
	}

	const std::vector<_string>& GetSkinBoneNames() const
	{
		return m_SkinBoneNames;
	}

	const std::vector<PARTICLE_SKIN_BINDING>&
	GetParticleSkinBindings() const
	{
		return m_ParticleSkinBindings;
	}

	static SPtr<CResNvClothMesh> Create(const _string& sPath);

private:
	// NvCloth Fabric 생성에 필요한 저폴리 파티클과 위상 데이터다.
	NVCLOTH_FABRIC_DESC m_tFabricDesc{};

	// 시뮬레이션 파티클을 따라 변형될 하이폴리 렌더 섹션들이다.
	std::vector<SECTION> m_Sections{};

	// Source Skeleton의 본 이름과 Weld된 입자별 스킨 바인딩이다.
	// 런타임에 이름으로 Target Skeleton을 매핑해 현재 자세를 따라간다.
	std::vector<_string> m_SkinBoneNames{};
	std::vector<PARTICLE_SKIN_BINDING>
		m_ParticleSkinBindings{};
};

NS_END
