#pragma once

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

struct ID3D11ShaderResourceView;

namespace Engine
{
	struct NVCLOTH_FABRIC_HANDLE
	{
		uint64_t iValue{};

		constexpr explicit operator bool() const
		{
			return iValue != 0;
		}
	};

	struct NVCLOTH_CLOTH_HANDLE
	{
		uint64_t iValue{};

		constexpr explicit operator bool() const
		{
			return iValue != 0;
		}
	};

	struct NVCLOTH_FABRIC_DESC
	{
		// Cloth particle positions in the cloth object's local space.
		std::vector<DirectX::XMFLOAT3> vecPositions{};

		// Triangle-list indices. The count must be a multiple of three.
		std::vector<uint32_t> vecIndices{};

		// Zero fixes a particle in place. A positive value makes it dynamic.
		// When empty, every particle is treated as dynamic.
		std::vector<float> vecInverseMasses{};

		// The cooker uses this direction to classify vertical/horizontal phases.
		DirectX::XMFLOAT3 vGravity{ 0.f, -1.f, 0.f };
		bool bUseGeodesicTether{ true };
	};

	struct NVCLOTH_FABRIC_INFO
	{
		uint32_t iParticleCount{};
		uint32_t iPhaseCount{};
		uint32_t iConstraintCount{};
		uint32_t iTetherCount{};
		uint32_t iTriangleCount{};
	};

	struct NVCLOTH_CLOTH_DESC
	{
		NVCLOTH_FABRIC_HANDLE hFabric{};

		// Initial particle positions. They may differ from the positions
		// used to cook the shared Fabric, but the particle count must match.
		std::vector<DirectX::XMFLOAT3> vecPositions{};
		std::vector<float> vecInverseMasses{};

		DirectX::XMFLOAT3 vGravity{ 0.f, -9.81f, 0.f };
		DirectX::XMFLOAT3 vDamping{ 0.05f, 0.05f, 0.05f };
		DirectX::XMFLOAT3 vLinearInertia{ 1.f, 1.f, 1.f };
		DirectX::XMFLOAT3 vAngularInertia{ 1.f, 1.f, 1.f };
		DirectX::XMFLOAT3 vCentrifugalInertia{ 1.f, 1.f, 1.f };
		float fSolverFrequency{ 120.f };
		float fStiffnessFrequency{ 120.f };
		float fPhaseStiffness{ 1.f };
		float fPhaseStiffnessMultiplier{ 1.f };
		float fCompressionLimit{ 1.f };
		float fStretchLimit{ 1.f };
		float fMotionConstraintStiffness{ 1.f };
	};

	struct NVCLOTH_ANIMATION_CONSTRAINT_DESC
	{
		// Current animation targets in Cloth simulation-local space.
		// Both arrays must contain exactly one value per Cloth particle.
		std::vector<DirectX::XMFLOAT3> vecTargetPositions{};
		std::vector<float> vecMaxDistances{};

		// Fixed particles are moved to their animation targets before
		// simulation. Reset previous positions only on the first frame or
		// after a teleport so normal attachment movement keeps its inertia.
		bool bUpdateFixedParticles{ true };
		bool bResetPreviousParticles{};
	};

	struct NVCLOTH_COLLISION_SPHERE
	{
		// Center and radius in the Cloth simulation's local space.
		DirectX::XMFLOAT3 vCenter{};
		float fRadius{};
	};

	struct NVCLOTH_COLLISION_CAPSULE
	{
		// Indices into NVCLOTH_COLLISION_DESC::vecSpheres.
		uint32_t iSphere0{};
		uint32_t iSphere1{};
	};

	struct NVCLOTH_COLLISION_DESC
	{
		std::vector<NVCLOTH_COLLISION_SPHERE> vecSpheres{};
		std::vector<NVCLOTH_COLLISION_CAPSULE> vecCapsules{};
		bool bContinuousCollision{ true };
		float fCollisionMassScale{ 10.f };
		float fFriction{ 0.2f };
	};

	struct NVCLOTH_GPU_PARTICLE_VIEW
	{
		// Borrowed SRV. It remains valid while the cloth handle is alive.
		// The view starts at the cloth's current-particle slice, so shaders
		// can read particle i from byte offset i * 16.
		ID3D11ShaderResourceView* pSRV{};
		uint32_t iParticleCount{};
	};
}
