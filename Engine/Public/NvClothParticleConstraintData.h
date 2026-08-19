#pragma once

#include "ISerializable.h"
#include "SerializerInterface.h"

#include <limits>

NS_BEGIN(Engine)

inline constexpr uint32_t NVCLOTH_PARTICLE_CONSTRAINT_VERSION = 1u;
inline constexpr const char* NVCLOTH_PARTICLE_CONSTRAINT_ROOT =
	"NvClothParticleConstraint";

// [LSY] Simulation Source Mesh의 정점 순서에 대응하는 Cloth 이동 가중치다.
// 0은 완전 고정, 1은 해당 Cloth의 최대 이동 반경을 모두 허용한다.
struct NVCLOTH_PARTICLE_CONSTRAINT_DATA final :
	public ISerializable
{
	uint32_t iVersion{
		NVCLOTH_PARTICLE_CONSTRAINT_VERSION };
	uint32_t iSimulationMeshIndex{
		std::numeric_limits<uint32_t>::max() };
	uint32_t iSourceVertexCount{};
	uint64_t iMeshSignature{};
	std::vector<_float> Weights{};

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			iVersion,
			iSimulationMeshIndex,
			iSourceVertexCount,
			iMeshSignature,
			Weights);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			iVersion,
			iSimulationMeshIndex,
			iSourceVertexCount,
			iMeshSignature,
			Weights);
	}
};

NS_END
