#include "NvClothManager.h"
#include "DbgLineRender.h"
#include "NvClothCollisionEditorGUI.h"

#include <cmath>
#include <malloc.h>
#include <mutex>
#include <unordered_map>

// NvCloth 1.1.6 uses the PxShared version bundled with PhysX 4.0.
// Keep every NvCloth/foundation type inside this translation unit.
#pragma push_macro("new")
#undef new
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <foundation/PxErrors.h>
#include <foundation/PxQuat.h>
#include <NvCloth/Callbacks.h>
#include <NvCloth/Cloth.h>
#include <NvCloth/DxContextManagerCallback.h>
#include <NvCloth/Factory.h>
#include <NvCloth/PhaseConfig.h>
#include <NvCloth/Solver.h>
#include <NvClothExt/ClothFabricCooker.h>
#include <NvClothExt/ClothMeshDesc.h>
#pragma pop_macro("new")

NS_BEGIN(Engine)

namespace
{
	_bool IsFinite(const DirectX::XMFLOAT3& vValue)
	{
		return std::isfinite(vValue.x) &&
			std::isfinite(vValue.y) &&
			std::isfinite(vValue.z);
	}

	_bool IsFinite(const DirectX::XMFLOAT4& vValue)
	{
		return std::isfinite(vValue.x) &&
			std::isfinite(vValue.y) &&
			std::isfinite(vValue.z) &&
			std::isfinite(vValue.w);
	}

	_bool IsUnitInterval(const DirectX::XMFLOAT3& vValue)
	{
		return IsFinite(vValue) &&
			vValue.x >= 0.f && vValue.x <= 1.f &&
			vValue.y >= 0.f && vValue.y <= 1.f &&
			vValue.z >= 0.f && vValue.z <= 1.f;
	}

	_bool ValidateFabricDesc(const NVCLOTH_FABRIC_DESC& Desc)
	{
		if (Desc.vecPositions.size() < 3 ||
			Desc.vecPositions.size() >
				std::numeric_limits<uint32_t>::max() ||
			Desc.vecIndices.size() < 3 ||
			Desc.vecIndices.size() % 3 != 0 ||
			Desc.vecIndices.size() / 3 >
				std::numeric_limits<uint32_t>::max())
		{
			return false;
		}

		if (!Desc.vecInverseMasses.empty() &&
			Desc.vecInverseMasses.size() != Desc.vecPositions.size())
		{
			return false;
		}

		for (const auto& vPosition : Desc.vecPositions)
		{
			if (!IsFinite(vPosition))
				return false;
		}

		for (const auto fInverseMass : Desc.vecInverseMasses)
		{
			if (!std::isfinite(fInverseMass) || fInverseMass < 0.f)
				return false;
		}

		const auto iParticleCount =
			static_cast<uint32_t>(Desc.vecPositions.size());
		for (size_t i = 0; i < Desc.vecIndices.size(); i += 3)
		{
			const auto i0 = Desc.vecIndices[i];
			const auto i1 = Desc.vecIndices[i + 1];
			const auto i2 = Desc.vecIndices[i + 2];
			if (i0 >= iParticleCount ||
				i1 >= iParticleCount ||
				i2 >= iParticleCount ||
				i0 == i1 || i1 == i2 || i2 == i0)
			{
				return false;
			}
		}

		if (!IsFinite(Desc.vGravity))
			return false;

		const auto fGravityLengthSq =
			Desc.vGravity.x * Desc.vGravity.x +
			Desc.vGravity.y * Desc.vGravity.y +
			Desc.vGravity.z * Desc.vGravity.z;
		return std::isfinite(fGravityLengthSq) &&
			fGravityLengthSq > 1.e-12f;
	}

	_bool ValidateClothDesc(const NVCLOTH_CLOTH_DESC& Desc)
	{
		if (!Desc.hFabric ||
			Desc.vecPositions.empty() ||
			Desc.vecPositions.size() != Desc.vecInverseMasses.size() ||
			Desc.vecPositions.size() >
				std::numeric_limits<uint32_t>::max())
		{
			return false;
		}

		for (const auto& vPosition : Desc.vecPositions)
		{
			if (!IsFinite(vPosition))
				return false;
		}

		for (const auto fInverseMass : Desc.vecInverseMasses)
		{
			if (!std::isfinite(fInverseMass) || fInverseMass < 0.f)
				return false;
		}

		if (!IsFinite(Desc.vGravity) ||
			!IsFinite(Desc.tWind.vVelocity) ||
			!IsFinite(Desc.vDamping) ||
			!IsUnitInterval(Desc.vLinearInertia) ||
			!IsUnitInterval(Desc.vAngularInertia) ||
			!IsUnitInterval(Desc.vCentrifugalInertia) ||
			!std::isfinite(Desc.fSolverFrequency) ||
			!std::isfinite(Desc.fStiffnessFrequency) ||
			!std::isfinite(Desc.fPhaseStiffness) ||
			!std::isfinite(Desc.fPhaseStiffnessMultiplier) ||
			!std::isfinite(Desc.fCompressionLimit) ||
			!std::isfinite(Desc.fStretchLimit) ||
			!std::isfinite(Desc.tWind.fDragCoefficient) ||
			!std::isfinite(Desc.tWind.fLiftCoefficient) ||
			!std::isfinite(Desc.tWind.fFluidDensity) ||
			!std::isfinite(
				Desc.fMotionConstraintStiffness) ||
			!std::isfinite(
				Desc.fSelfCollisionDistance) ||
			!std::isfinite(
				Desc.fSelfCollisionStiffness))
		{
			return false;
		}

		return Desc.tWind.fDragCoefficient >= 0.f &&
			Desc.tWind.fDragCoefficient <= 1.f &&
			Desc.tWind.fLiftCoefficient >= 0.f &&
			Desc.tWind.fLiftCoefficient <= 1.f &&
			Desc.tWind.fFluidDensity >= 0.f &&
			Desc.fSolverFrequency > 0.f &&
			Desc.fStiffnessFrequency > 0.f &&
			Desc.fPhaseStiffness >= 0.f &&
			Desc.fPhaseStiffness <= 1.f &&
			Desc.fPhaseStiffnessMultiplier >= 0.f &&
			Desc.fPhaseStiffnessMultiplier <= 1.f &&
			Desc.fCompressionLimit > 0.f &&
			Desc.fStretchLimit > 0.f &&
			Desc.fMotionConstraintStiffness >= 0.f &&
			Desc.fMotionConstraintStiffness <= 1.f &&
			Desc.fSelfCollisionDistance >= 0.f &&
			Desc.fSelfCollisionStiffness >= 0.f &&
			Desc.fSelfCollisionStiffness <= 1.f &&
			Desc.vDamping.x >= 0.f && Desc.vDamping.x <= 1.f &&
			Desc.vDamping.y >= 0.f && Desc.vDamping.y <= 1.f &&
			Desc.vDamping.z >= 0.f && Desc.vDamping.z <= 1.f;
	}

	_bool ValidateAnimationConstraintDesc(
		const NVCLOTH_ANIMATION_CONSTRAINT_DESC& Desc)
	{
		if (Desc.vecTargetPositions.empty() ||
			Desc.vecTargetPositions.size() !=
				Desc.vecMaxDistances.size() ||
			Desc.vecSeparationCenters.size() !=
				Desc.vecSeparationRadii.size() ||
			(!Desc.vecSeparationCenters.empty() &&
				Desc.vecSeparationCenters.size() !=
					Desc.vecTargetPositions.size()))
		{
			return false;
		}

		for (size_t i = 0;
			i < Desc.vecTargetPositions.size();
			++i)
		{
			if (!IsFinite(Desc.vecTargetPositions[i]) ||
				!std::isfinite(
					Desc.vecMaxDistances[i]) ||
				Desc.vecMaxDistances[i] < 0.f)
			{
				return false;
			}
		}
		for (size_t i = 0;
			i < Desc.vecSeparationCenters.size();
			++i)
		{
			if (!IsFinite(
					Desc.vecSeparationCenters[i]) ||
				!std::isfinite(
					Desc.vecSeparationRadii[i]) ||
				Desc.vecSeparationRadii[i] < 0.f)
			{
				return false;
			}
		}
		return true;
	}

	_bool ValidateCollisionDesc(
		const NVCLOTH_COLLISION_DESC& Desc)
	{
		constexpr size_t MAX_COLLISION_SPHERES = 32;
		constexpr size_t MAX_COLLISION_CAPSULES = 32;
		constexpr size_t MAX_COLLISION_PLANES = 32;
		constexpr size_t MAX_COLLISION_CONVEXES = 32;

		if (Desc.vecSpheres.size() >
				MAX_COLLISION_SPHERES ||
			Desc.vecCapsules.size() >
				MAX_COLLISION_CAPSULES ||
			Desc.vecPlanes.size() >
				MAX_COLLISION_PLANES ||
			Desc.vecConvexes.size() >
				MAX_COLLISION_CONVEXES ||
			!std::isfinite(Desc.fCollisionMassScale) ||
			Desc.fCollisionMassScale < 0.f ||
			!std::isfinite(Desc.fFriction) ||
			Desc.fFriction < 0.f ||
			Desc.fFriction > 1.f)
		{
			return false;
		}

		for (const auto& Sphere : Desc.vecSpheres)
		{
			if (!IsFinite(Sphere.vCenter) ||
				!std::isfinite(Sphere.fRadius) ||
				Sphere.fRadius <= 0.f)
			{
				return false;
			}
		}

		for (const auto& Capsule : Desc.vecCapsules)
		{
			if (Capsule.iSphere0 >=
					Desc.vecSpheres.size() ||
				Capsule.iSphere1 >=
					Desc.vecSpheres.size() ||
				Capsule.iSphere0 == Capsule.iSphere1)
			{
				return false;
			}
		}

		for (const auto& Plane : Desc.vecPlanes)
		{
			if (!IsFinite(Plane.vNormal) ||
				!std::isfinite(Plane.fDistance))
			{
				return false;
			}

			const float fNormalLengthSq =
				Plane.vNormal.x * Plane.vNormal.x +
				Plane.vNormal.y * Plane.vNormal.y +
				Plane.vNormal.z * Plane.vNormal.z;
			if (!std::isfinite(fNormalLengthSq) ||
				std::abs(fNormalLengthSq - 1.f) >
					1.e-3f)
			{
				return false;
			}
		}

		const uint32_t iValidPlaneMask =
			Desc.vecPlanes.size() >= 32 ?
				std::numeric_limits<uint32_t>::max() :
				((1u <<
					static_cast<uint32_t>(
						Desc.vecPlanes.size())) - 1u);
		for (const auto& Convex : Desc.vecConvexes)
		{
			if (Convex.iPlaneMask == 0 ||
				(Convex.iPlaneMask &
					~iValidPlaneMask) != 0)
			{
				return false;
			}
		}

		return true;
	}

	template<typename T>
	nv::cloth::Range<const T> MakeRange(
		const std::vector<T>& Values)
	{
		if (Values.empty())
			return {};

		return {
			Values.data(),
			Values.data() + Values.size()
		};
	}

#ifdef _DEBUG
	void BuildProceduralTestCloth(
		NVCLOTH_FABRIC_DESC& OutFabricDesc,
		NVCLOTH_CLOTH_DESC& OutClothDesc)
	{
		constexpr uint32_t GRID_X = 11;
		constexpr uint32_t GRID_Z = 11;
		constexpr float CLOTH_SIZE = 4.f;
		constexpr _float3 ORIGIN{ 8.f, 12.f, 8.f };

		OutFabricDesc = {};
		OutFabricDesc.bUseGeodesicTether = false;
		OutFabricDesc.vecPositions.reserve(GRID_X * GRID_Z);
		OutFabricDesc.vecInverseMasses.reserve(GRID_X * GRID_Z);
		OutFabricDesc.vecIndices.reserve(
			(GRID_X - 1) * (GRID_Z - 1) * 6);

		for (uint32_t z = 0; z < GRID_Z; ++z)
		{
			const auto fZ =
				static_cast<float>(z) /
				static_cast<float>(GRID_Z - 1);
			for (uint32_t x = 0; x < GRID_X; ++x)
			{
				const auto fX =
					static_cast<float>(x) /
					static_cast<float>(GRID_X - 1);
				OutFabricDesc.vecPositions.push_back({
					ORIGIN.x + (fX - 0.5f) * CLOTH_SIZE,
					ORIGIN.y,
					ORIGIN.z + fZ * CLOTH_SIZE
				});
				OutFabricDesc.vecInverseMasses.push_back(
					z == 0 ? 0.f : 1.f);
			}
		}

		for (uint32_t z = 0; z + 1 < GRID_Z; ++z)
		{
			for (uint32_t x = 0; x + 1 < GRID_X; ++x)
			{
				const auto i0 = z * GRID_X + x;
				const auto i1 = i0 + 1;
				const auto i2 = i0 + GRID_X;
				const auto i3 = i2 + 1;
				OutFabricDesc.vecIndices.insert(
					OutFabricDesc.vecIndices.end(),
					{ i0, i2, i1, i1, i2, i3 });
			}
		}

		OutClothDesc = {};
		OutClothDesc.vecPositions = OutFabricDesc.vecPositions;
		OutClothDesc.vecInverseMasses =
			OutFabricDesc.vecInverseMasses;
		OutClothDesc.fSolverFrequency = 120.f;
		OutClothDesc.fStiffnessFrequency = 120.f;
		OutClothDesc.vDamping = { 0.1f, 0.1f, 0.1f };
	}
#endif

	class CNvClothAllocator final : public physx::PxAllocatorCallback
	{
	public:
		void* allocate(
			size_t size,
			const char*,
			const char*,
			int) override
		{
			return _aligned_malloc(size, 16);
		}

		void deallocate(void* pMemory) override
		{
			_aligned_free(pMemory);
		}
	};

	const char* GetNvClothErrorName(physx::PxErrorCode::Enum eCode)
	{
		switch (eCode)
		{
		case physx::PxErrorCode::eNO_ERROR:
			return "NO_ERROR";
		case physx::PxErrorCode::eDEBUG_INFO:
			return "DEBUG_INFO";
		case physx::PxErrorCode::eDEBUG_WARNING:
			return "DEBUG_WARNING";
		case physx::PxErrorCode::eINVALID_PARAMETER:
			return "INVALID_PARAMETER";
		case physx::PxErrorCode::eINVALID_OPERATION:
			return "INVALID_OPERATION";
		case physx::PxErrorCode::eOUT_OF_MEMORY:
			return "OUT_OF_MEMORY";
		case physx::PxErrorCode::eINTERNAL_ERROR:
			return "INTERNAL_ERROR";
		case physx::PxErrorCode::eABORT:
			return "ABORT";
		case physx::PxErrorCode::ePERF_WARNING:
			return "PERF_WARNING";
		default:
			return "UNKNOWN";
		}
	}

	class CNvClothErrorCallback final : public physx::PxErrorCallback
	{
	public:
		void reportError(
			physx::PxErrorCode::Enum eCode,
			const char* pMessage,
			const char* pFile,
			int iLine) override
		{
			char szBuffer[2048]{};
			sprintf_s(
				szBuffer,
				"[NvCloth][%s] %s (%s:%d)\n",
				GetNvClothErrorName(eCode),
				pMessage ? pMessage : "",
				pFile ? pFile : "",
				iLine);
			OutputDebugStringA(szBuffer);
		}
	};

	class CNvClothAssertHandler final : public nv::cloth::PxAssertHandler
	{
	public:
		void operator()(
			const char* pExpression,
			const char* pFile,
			int iLine,
			bool& bIgnore) override
		{
			bIgnore = false;

			char szBuffer[2048]{};
			sprintf_s(
				szBuffer,
				"[NvCloth][ASSERT] %s (%s:%d)\n",
				pExpression ? pExpression : "",
				pFile ? pFile : "",
				iLine);
			OutputDebugStringA(szBuffer);

#ifdef _DEBUG
			if (IsDebuggerPresent())
				__debugbreak();
#endif
		}
	};

	class CNvClothDxContext final
		: public nv::cloth::DxContextManagerCallback
	{
	public:
		CNvClothDxContext(
			ID3D11Device* pDevice,
			ID3D11DeviceContext* pContext)
			: m_pDevice{ pDevice },
			  m_pContext{ pContext }
		{
		}

		void acquireContext() override
		{
			m_ContextMutex.lock();
		}

		void releaseContext() override
		{
			m_ContextMutex.unlock();
		}

		ID3D11Device* getDevice() const override
		{
			return m_pDevice.Get();
		}

		ID3D11DeviceContext* getContext() const override
		{
			return m_pContext.Get();
		}

		bool synchronizeResources() const override
		{
			return false;
		}

	private:
		std::recursive_mutex m_ContextMutex{};
		ComPtr<ID3D11Device> m_pDevice{};
		ComPtr<ID3D11DeviceContext> m_pContext{};
	};
}

struct CNvClothManager::IMPLEMENTATION
{
	struct FABRIC_RECORD
	{
		nv::cloth::Fabric* pFabric{};
		std::vector<int32_t> vecPhaseTypes{};
		std::vector<uint32_t> vecIndices{};
		NVCLOTH_FABRIC_INFO Info{};
		uint32_t iClothReferenceCount{};

		~FABRIC_RECORD()
		{
			if (pFabric)
			{
				pFabric->decRefCount();
				pFabric = nullptr;
			}
		}
	};

	struct CLOTH_RECORD
	{
		nv::cloth::Cloth* pCloth{};
		uint64_t iFabricHandle{};
		ID3D11Buffer* pGpuParticleBuffer{};
		uint32_t iGpuParticleOffset{};
		ComPtr<ID3D11ShaderResourceView> pGpuParticleSRV{};
		ComPtr<ID3D11Buffer> pCpuParticleBuffer{};
		ComPtr<ID3D11ShaderResourceView> pCpuParticleSRV{};
		uint32_t iCpuParticleCapacity{};
		_bool bCpuParticleBufferDirty{ true };
		std::vector<_float3> vecDebugPositions{};
		std::vector<float> vecInverseMasses{};
		std::vector<float> vecMotionConstraintRadii{};
		std::vector<uint32_t> vecIndices{};
		std::vector<physx::PxVec4> vecCollisionSpheres{};
		std::vector<uint32_t> vecCollisionCapsules{};
		std::vector<physx::PxVec4> vecCollisionPlanes{};
		std::vector<uint32_t> vecCollisionConvexes{};
		_float3 vTranslation{};
		_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };

		~CLOTH_RECORD()
		{
			delete pCloth;
			pCloth = nullptr;
		}
	};

	CNvClothAllocator Allocator{};
	CNvClothErrorCallback ErrorCallback{};
	CNvClothAssertHandler AssertHandler{};
	std::unique_ptr<CNvClothDxContext> pDxContext{};
	ComPtr<ID3D11Device> pDevice{};
	ComPtr<ID3D11DeviceContext> pContext{};
	nv::cloth::Factory* pFactory{};
	nv::cloth::Solver* pSolver{};
	NVCLOTH_BACKEND eBackend{ NVCLOTH_BACKEND::DX11 };
	mutable std::mutex StateMutex{};
	std::unordered_map<
		uint64_t,
		std::unique_ptr<FABRIC_RECORD>> Fabrics{};
	std::unordered_map<
		uint64_t,
		std::unique_ptr<CLOTH_RECORD>> Cloths{};
	uint64_t iNextFabricHandle{ 1 };
	uint64_t iNextClothHandle{ 1 };
	_bool bDebugDraw{};
	_bool bDebugConstraintWeights{};
	_bool bSolverErrorLogged{};
#ifdef _DEBUG
	NVCLOTH_FABRIC_HANDLE hTestFabric{};
	NVCLOTH_CLOTH_HANDLE hTestCloth{};
#endif

	~IMPLEMENTATION()
	{
		if (pSolver)
		{
			for (auto& [iHandle, pRecord] : Cloths)
			{
				if (pRecord && pRecord->pCloth)
					pSolver->removeCloth(pRecord->pCloth);
			}
		}
		Cloths.clear();

		delete pSolver;
		pSolver = nullptr;

		Fabrics.clear();

		if (pFactory)
		{
			NvClothDestroyFactory(pFactory);
			pFactory = nullptr;
		}

		pDxContext.reset();
	}
};

CNvClothManager::CNvClothManager()
{
}

CNvClothManager::~CNvClothManager()
{
}

HRESULT CNvClothManager::Initialize(
	ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext,
	NVCLOTH_BACKEND eBackend)
{
	if (!pDevice || !pContext)
		return E_INVALIDARG;

	m_pImpl = std::make_unique<IMPLEMENTATION>();
	m_pImpl->pDevice = pDevice;
	m_pImpl->pContext = pContext;
	m_pImpl->eBackend = eBackend;

	nv::cloth::InitializeNvCloth(
		&m_pImpl->Allocator,
		&m_pImpl->ErrorCallback,
		&m_pImpl->AssertHandler,
		nullptr);

	if (eBackend == NVCLOTH_BACKEND::DX11)
	{
		if (!NvClothCompiledWithDxSupport())
		{
			OutputDebugStringA(
				"[NvCloth] The loaded library has no DX11 support.\n");
			return E_FAIL;
		}

		m_pImpl->pDxContext =
			std::make_unique<CNvClothDxContext>(
				pDevice,
				pContext);
		m_pImpl->pFactory =
			NvClothCreateFactoryDX11(
				m_pImpl->pDxContext.get());
	}
	else
	{
		m_pImpl->pFactory = NvClothCreateFactoryCPU();
	}

	if (!m_pImpl->pFactory ||
		m_pImpl->pFactory->getPlatform() !=
			(eBackend == NVCLOTH_BACKEND::DX11 ?
				nv::cloth::Platform::DX11 :
				nv::cloth::Platform::CPU))
	{
		OutputDebugStringA(
			"[NvCloth] Failed to create the requested factory.\n");
		return E_FAIL;
	}

	m_pImpl->pSolver = m_pImpl->pFactory->createSolver();
	if (!m_pImpl->pSolver)
	{
		OutputDebugStringA(
			"[NvCloth] Failed to create the requested solver.\n");
		return E_FAIL;
	}

	m_pCollisionEditor =
		CNvClothCollisionEditorGUI::Create();
	if (!m_pCollisionEditor)
		return E_FAIL;

	DEBUG_LOG(
		eBackend == NVCLOTH_BACKEND::DX11 ?
			"[NvCloth] DX11 backend initialized.\n" :
			"[NvCloth] CPU backend initialized.\n");

#ifdef _DEBUG
	NVCLOTH_FABRIC_DESC TestFabricDesc{};
	NVCLOTH_CLOTH_DESC TestClothDesc{};
	BuildProceduralTestCloth(TestFabricDesc, TestClothDesc);

	NVCLOTH_FABRIC_INFO TestInfo{};
	if (FAILED(CreateFabric(
			TestFabricDesc,
			m_pImpl->hTestFabric,
			&TestInfo)) ||
		!m_pImpl->hTestFabric ||
		TestInfo.iParticleCount !=
			TestFabricDesc.vecPositions.size())
	{
		OutputDebugStringA(
			"[NvCloth] Fabric cooking self-test failed.\n");
		return E_FAIL;
	}

	TestClothDesc.hFabric = m_pImpl->hTestFabric;
	if (FAILED(CreateCloth(
			TestClothDesc,
			m_pImpl->hTestCloth)) ||
		!m_pImpl->hTestCloth)
	{
		OutputDebugStringA(
			"[NvCloth] Procedural cloth creation failed.\n");
		return E_FAIL;
	}

	StepSimulation(1.f / 60.f);
	std::vector<_float3> vecTestParticles{};
	if (!GetClothParticles(
			m_pImpl->hTestCloth,
			vecTestParticles) ||
		vecTestParticles.size() !=
			TestClothDesc.vecPositions.size())
	{
		OutputDebugStringA(
			"[NvCloth] Solver self-test failed.\n");
		return E_FAIL;
	}

	if (!ReleaseCloth(m_pImpl->hTestCloth) ||
		!ReleaseFabric(m_pImpl->hTestFabric))
	{
		OutputDebugStringA(
			"[NvCloth] Failed to release self-test resources.\n");
		return E_FAIL;
	}
	m_pImpl->hTestCloth = {};
	m_pImpl->hTestFabric = {};

	DEBUG_LOG(
		"[NvCloth] Fabric and solver self-test passed.\n");
#endif

	return S_OK;
}

_bool CNvClothManager::IsInitialized() const
{
	return m_pImpl && m_pImpl->pFactory;
}

NVCLOTH_BACKEND CNvClothManager::GetBackend() const
{
	return m_pImpl ?
		m_pImpl->eBackend :
		NVCLOTH_BACKEND::DX11;
}

HRESULT CNvClothManager::CreateFabric(
	const NVCLOTH_FABRIC_DESC& Desc,
	NVCLOTH_FABRIC_HANDLE& OutHandle,
	NVCLOTH_FABRIC_INFO* pOutInfo)
{
	ZoneScopedN("NvCloth_CreateFabric");

	OutHandle = {};
	if (pOutInfo)
		*pOutInfo = {};

	if (!IsInitialized() || !ValidateFabricDesc(Desc))
	{
		OutputDebugStringA(
			"[NvCloth] Fabric descriptor is invalid.\n");
		return E_INVALIDARG;
	}

	const auto fGravityLength = std::sqrt(
		Desc.vGravity.x * Desc.vGravity.x +
		Desc.vGravity.y * Desc.vGravity.y +
		Desc.vGravity.z * Desc.vGravity.z);
	const physx::PxVec3 vGravity{
		Desc.vGravity.x / fGravityLength,
		Desc.vGravity.y / fGravityLength,
		Desc.vGravity.z / fGravityLength
	};

	std::vector<physx::PxVec3> vecPositions{};
	vecPositions.reserve(Desc.vecPositions.size());
	for (const auto& vPosition : Desc.vecPositions)
	{
		vecPositions.emplace_back(
			vPosition.x,
			vPosition.y,
			vPosition.z);
	}

	nv::cloth::ClothMeshDesc MeshDesc{};
	MeshDesc.points.data = vecPositions.data();
	MeshDesc.points.count =
		static_cast<physx::PxU32>(vecPositions.size());
	MeshDesc.points.stride = sizeof(physx::PxVec3);

	MeshDesc.triangles.data = Desc.vecIndices.data();
	MeshDesc.triangles.count =
		static_cast<physx::PxU32>(Desc.vecIndices.size() / 3);
	MeshDesc.triangles.stride = sizeof(uint32_t) * 3;

	if (!Desc.vecInverseMasses.empty())
	{
		MeshDesc.invMasses.data = Desc.vecInverseMasses.data();
		MeshDesc.invMasses.count =
			static_cast<physx::PxU32>(
				Desc.vecInverseMasses.size());
		MeshDesc.invMasses.stride = sizeof(float);
	}

	if (!MeshDesc.isValid())
		return E_INVALIDARG;

	nv::cloth::Vector<int32_t>::Type vecPhaseTypes{};
	std::scoped_lock Lock{ m_pImpl->StateMutex };
	auto* pFabric = NvClothCookFabricFromMesh(
		m_pImpl->pFactory,
		MeshDesc,
		vGravity,
		&vecPhaseTypes,
		Desc.bUseGeodesicTether);
	if (!pFabric)
	{
		OutputDebugStringA(
			"[NvCloth] Fabric cooking failed.\n");
		return E_FAIL;
	}

	auto pRecord =
		std::make_unique<IMPLEMENTATION::FABRIC_RECORD>();
	pRecord->pFabric = pFabric;
	pRecord->vecPhaseTypes.assign(
		vecPhaseTypes.begin(),
		vecPhaseTypes.end());
	pRecord->vecIndices = Desc.vecIndices;
	pRecord->Info.iParticleCount = pFabric->getNumParticles();
	pRecord->Info.iPhaseCount = pFabric->getNumPhases();
	pRecord->Info.iConstraintCount =
		pFabric->getNumRestvalues();
	pRecord->Info.iTetherCount = pFabric->getNumTethers();
	pRecord->Info.iTriangleCount = pFabric->getNumTriangles();

	uint64_t iHandle = m_pImpl->iNextFabricHandle++;
	while (iHandle == 0 || m_pImpl->Fabrics.contains(iHandle))
		iHandle = m_pImpl->iNextFabricHandle++;

	const auto Info = pRecord->Info;
	m_pImpl->Fabrics.emplace(iHandle, std::move(pRecord));
	OutHandle.iValue = iHandle;
	if (pOutInfo)
		*pOutInfo = Info;

	return S_OK;
}

_bool CNvClothManager::ReleaseFabric(
	NVCLOTH_FABRIC_HANDLE Handle)
{
	if (!m_pImpl || !Handle)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Fabrics.find(Handle.iValue);
	if (Iter == m_pImpl->Fabrics.end() ||
		Iter->second->iClothReferenceCount != 0)
	{
		return false;
	}

	m_pImpl->Fabrics.erase(Iter);
	return true;
}

_bool CNvClothManager::GetFabricInfo(
	NVCLOTH_FABRIC_HANDLE Handle,
	NVCLOTH_FABRIC_INFO& OutInfo) const
{
	OutInfo = {};
	if (!m_pImpl || !Handle)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Fabrics.find(Handle.iValue);
	if (Iter == m_pImpl->Fabrics.end())
		return false;

	OutInfo = Iter->second->Info;
	return true;
}

HRESULT CNvClothManager::CreateCloth(
	const NVCLOTH_CLOTH_DESC& Desc,
	NVCLOTH_CLOTH_HANDLE& OutHandle)
{
	ZoneScopedN("NvCloth_CreateCloth");

	OutHandle = {};
	if (!IsInitialized() || !ValidateClothDesc(Desc))
		return E_INVALIDARG;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	if (!m_pImpl->pSolver)
		return E_FAIL;

	const auto FabricIter =
		m_pImpl->Fabrics.find(Desc.hFabric.iValue);
	if (FabricIter == m_pImpl->Fabrics.end() ||
		!FabricIter->second->pFabric ||
		FabricIter->second->Info.iParticleCount !=
			Desc.vecPositions.size() ||
		FabricIter->second->Info.iPhaseCount >
			std::numeric_limits<uint16_t>::max())
	{
		return E_INVALIDARG;
	}

	std::vector<physx::PxVec4> vecParticles{};
	vecParticles.reserve(Desc.vecPositions.size());
	for (size_t i = 0; i < Desc.vecPositions.size(); ++i)
	{
		const auto& vPosition = Desc.vecPositions[i];
		vecParticles.emplace_back(
			vPosition.x,
			vPosition.y,
			vPosition.z,
			Desc.vecInverseMasses[i]);
	}

	auto* pCloth = m_pImpl->pFactory->createCloth(
		nv::cloth::Range<const physx::PxVec4>{
			vecParticles.data(),
			vecParticles.data() + vecParticles.size() },
		*FabricIter->second->pFabric);
	if (!pCloth)
	{
		OutputDebugStringA(
			"[NvCloth] Cloth creation failed.\n");
		return E_FAIL;
	}

	pCloth->setGravity({
		Desc.vGravity.x,
		Desc.vGravity.y,
		Desc.vGravity.z });
	pCloth->setWindVelocity({
		Desc.tWind.vVelocity.x,
		Desc.tWind.vVelocity.y,
		Desc.tWind.vVelocity.z });
	pCloth->setDragCoefficient(
		Desc.tWind.fDragCoefficient);
	pCloth->setLiftCoefficient(
		Desc.tWind.fLiftCoefficient);
	pCloth->setFluidDensity(
		Desc.tWind.fFluidDensity);
	pCloth->setDamping({
		Desc.vDamping.x,
		Desc.vDamping.y,
		Desc.vDamping.z });
	pCloth->setLinearInertia({
		Desc.vLinearInertia.x,
		Desc.vLinearInertia.y,
		Desc.vLinearInertia.z });
	pCloth->setAngularInertia({
		Desc.vAngularInertia.x,
		Desc.vAngularInertia.y,
		Desc.vAngularInertia.z });
	pCloth->setCentrifugalInertia({
		Desc.vCentrifugalInertia.x,
		Desc.vCentrifugalInertia.y,
		Desc.vCentrifugalInertia.z });
	pCloth->setSolverFrequency(Desc.fSolverFrequency);
	pCloth->setStiffnessFrequency(Desc.fStiffnessFrequency);
	pCloth->setMotionConstraintStiffness(
		Desc.fMotionConstraintStiffness);
	pCloth->setSelfCollisionDistance(
		Desc.fSelfCollisionDistance);
	pCloth->setSelfCollisionStiffness(
		Desc.fSelfCollisionStiffness);
	if (Desc.fSelfCollisionDistance > 0.f &&
		Desc.fSelfCollisionStiffness > 0.f)
	{
		// [LSY] 초기 형상에서 이미 가까운 이웃 정점은 Self Collision
		// 대상으로 보지 않게 해 메시 자체가 부풀거나 떨리는 현상을 막는다.
		pCloth->setRestPositions(
			nv::cloth::Range<const physx::PxVec4>{
				vecParticles.data(),
				vecParticles.data() +
					vecParticles.size() });
	}

	std::vector<nv::cloth::PhaseConfig> vecPhases(
		static_cast<size_t>(
			FabricIter->second->Info.iPhaseCount));
	for (uint32_t i = 0; i < vecPhases.size(); ++i)
	{
		auto& Phase = vecPhases[i];
		Phase.mPhaseIndex = static_cast<uint16_t>(i);
		Phase.mStiffness = Desc.fPhaseStiffness;
		Phase.mStiffnessMultiplier =
			Desc.fPhaseStiffnessMultiplier;
		Phase.mCompressionLimit = Desc.fCompressionLimit;
		Phase.mStretchLimit = Desc.fStretchLimit;
	}
	pCloth->setPhaseConfig(
		nv::cloth::Range<const nv::cloth::PhaseConfig>{
			vecPhases.data(),
			vecPhases.data() + vecPhases.size() });

	if (Desc.bUseVirtualParticles)
	{
		const auto& vecTriangleIndices =
			FabricIter->second->vecIndices;
		std::vector<uint32_t>
			vecVirtualParticleIndices{};
		vecVirtualParticleIndices.reserve(
			vecTriangleIndices.size() / 3 * 4);

		for (size_t i = 0;
			i + 2 < vecTriangleIndices.size();
			i += 3)
		{
			const uint32_t i0 =
				vecTriangleIndices[i];
			const uint32_t i1 =
				vecTriangleIndices[i + 1];
			const uint32_t i2 =
				vecTriangleIndices[i + 2];

			// A fully fixed triangle cannot be moved by a collision response,
			// so it does not need an additional collision sample.
			if (Desc.vecInverseMasses[i0] <= 0.f &&
				Desc.vecInverseMasses[i1] <= 0.f &&
				Desc.vecInverseMasses[i2] <= 0.f)
			{
				continue;
			}

			vecVirtualParticleIndices.insert(
				vecVirtualParticleIndices.end(),
				{ i0, i1, i2, 0u });
		}

		if (!vecVirtualParticleIndices.empty())
		{
			using VIRTUAL_PARTICLE_INDICES =
				const uint32_t[4];
			const auto* pIndices =
				reinterpret_cast<
					const uint32_t(*)[4]>(
						vecVirtualParticleIndices.data());
			const size_t iVirtualParticleCount =
				vecVirtualParticleIndices.size() / 4;
			const physx::PxVec3 Weights[]{
				physx::PxVec3{
					1.f / 3.f,
					1.f / 3.f,
					1.f / 3.f }
			};
			pCloth->setVirtualParticles(
				nv::cloth::Range<
					VIRTUAL_PARTICLE_INDICES>{
						pIndices,
						pIndices +
							iVirtualParticleCount },
				nv::cloth::Range<
					const physx::PxVec3>{
						Weights,
						Weights + 1 });
		}
	}

	auto pRecord =
		std::make_unique<IMPLEMENTATION::CLOTH_RECORD>();
	pRecord->pCloth = pCloth;
	pRecord->iFabricHandle = Desc.hFabric.iValue;
	pRecord->vecDebugPositions = Desc.vecPositions;
	pRecord->vecInverseMasses = Desc.vecInverseMasses;
	pRecord->vecIndices = FabricIter->second->vecIndices;

	uint64_t iHandle = m_pImpl->iNextClothHandle++;
	while (iHandle == 0 || m_pImpl->Cloths.contains(iHandle))
		iHandle = m_pImpl->iNextClothHandle++;

	m_pImpl->pSolver->addCloth(pCloth);
	m_pImpl->Cloths.emplace(iHandle, std::move(pRecord));
	++FabricIter->second->iClothReferenceCount;
	OutHandle.iValue = iHandle;
	return S_OK;
}

_bool CNvClothManager::ReleaseCloth(
	NVCLOTH_CLOTH_HANDLE Handle)
{
	if (!m_pImpl || !Handle)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto ClothIter = m_pImpl->Cloths.find(Handle.iValue);
	if (ClothIter == m_pImpl->Cloths.end())
		return false;

	auto& Record = *ClothIter->second;
	if (m_pImpl->pSolver && Record.pCloth)
		m_pImpl->pSolver->removeCloth(Record.pCloth);

	const auto FabricIter =
		m_pImpl->Fabrics.find(Record.iFabricHandle);
	if (FabricIter != m_pImpl->Fabrics.end() &&
		FabricIter->second->iClothReferenceCount != 0)
	{
		--FabricIter->second->iClothReferenceCount;
	}

	m_pImpl->Cloths.erase(ClothIter);
	return true;
}

_bool CNvClothManager::GetClothParticles(
	NVCLOTH_CLOTH_HANDLE Handle,
	std::vector<_float3>& OutPositions) const
{
	ZoneScopedN("NvCloth_GetParticles_CPUReadback");

	OutPositions.clear();
	if (!m_pImpl || !Handle)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() || !Iter->second->pCloth)
		return false;

	const auto& Cloth =
		static_cast<const nv::cloth::Cloth&>(*Iter->second->pCloth);
	const auto Particles = Cloth.getCurrentParticles();
	OutPositions.reserve(Particles.size());
	for (const auto& Particle : Particles)
	{
		OutPositions.push_back({
			Particle.x,
			Particle.y,
			Particle.z });
	}
	return true;
}

_bool CNvClothManager::GetClothRenderParticleView(
	NVCLOTH_CLOTH_HANDLE Handle,
	NVCLOTH_RENDER_PARTICLE_VIEW& OutView)
{
	ZoneScopedN("NvCloth_GetRenderParticleView");

	OutView = {};
	if (!m_pImpl ||
		!Handle ||
		!m_pImpl->pDevice ||
		!m_pImpl->pContext)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	const auto iParticleCount =
		Record.pCloth->getNumParticles();
	if (iParticleCount == 0)
		return false;

	if (m_pImpl->eBackend == NVCLOTH_BACKEND::CPU)
	{
		if (!Record.pCpuParticleBuffer ||
			!Record.pCpuParticleSRV ||
			Record.iCpuParticleCapacity !=
				iParticleCount)
		{
			Record.pCpuParticleSRV.Reset();
			Record.pCpuParticleBuffer.Reset();
			Record.iCpuParticleCapacity = 0;

			D3D11_BUFFER_DESC BufferDesc{};
			BufferDesc.ByteWidth =
				iParticleCount *
				static_cast<uint32_t>(
					sizeof(physx::PxVec4));
			BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			BufferDesc.BindFlags =
				D3D11_BIND_SHADER_RESOURCE;
			BufferDesc.CPUAccessFlags =
				D3D11_CPU_ACCESS_WRITE;
			BufferDesc.MiscFlags =
				D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			if (FAILED(m_pImpl->pDevice->CreateBuffer(
				&BufferDesc,
				nullptr,
				Record.pCpuParticleBuffer.GetAddressOf())))
			{
				return false;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
			SrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			SrvDesc.ViewDimension =
				D3D11_SRV_DIMENSION_BUFFEREX;
			SrvDesc.BufferEx.FirstElement = 0;
			SrvDesc.BufferEx.NumElements =
				BufferDesc.ByteWidth /
				sizeof(uint32_t);
			SrvDesc.BufferEx.Flags =
				D3D11_BUFFEREX_SRV_FLAG_RAW;
			if (FAILED(
				m_pImpl->pDevice->CreateShaderResourceView(
					Record.pCpuParticleBuffer.Get(),
					&SrvDesc,
					Record.pCpuParticleSRV.GetAddressOf())))
			{
				Record.pCpuParticleBuffer.Reset();
				return false;
			}

			Record.iCpuParticleCapacity =
				iParticleCount;
			Record.bCpuParticleBufferDirty = true;
		}

		if (Record.bCpuParticleBufferDirty)
		{
			const auto& Cloth =
				static_cast<const nv::cloth::Cloth&>(
					*Record.pCloth);
			const auto Particles =
				Cloth.getCurrentParticles();
			if (Particles.size() != iParticleCount)
				return false;

			D3D11_MAPPED_SUBRESOURCE Mapped{};
			if (FAILED(m_pImpl->pContext->Map(
				Record.pCpuParticleBuffer.Get(),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped)))
			{
				return false;
			}

			auto* pDestination =
				static_cast<physx::PxVec4*>(
					Mapped.pData);
			for (uint32_t i = 0;
				i < iParticleCount;
				++i)
			{
				pDestination[i] = Particles[i];
			}
			m_pImpl->pContext->Unmap(
				Record.pCpuParticleBuffer.Get(),
				0);
			Record.bCpuParticleBufferDirty = false;
		}

		OutView.pSRV = Record.pCpuParticleSRV.Get();
		OutView.iParticleCount = iParticleCount;
		return OutView.pSRV != nullptr;
	}

	if (!m_pImpl->pDxContext)
		return false;

	const auto GpuParticles =
		Record.pCloth->getGpuParticles();
	if (!GpuParticles.mBuffer)
	{
		return false;
	}

	const auto iParticleOffsetBytes =
		reinterpret_cast<uintptr_t>(
			GpuParticles.mCurrent);
	if (iParticleOffsetBytes %
		sizeof(physx::PxVec4) != 0)
	{
		return false;
	}

	const auto iParticleOffset =
		static_cast<uint32_t>(
			iParticleOffsetBytes /
			sizeof(physx::PxVec4));
	D3D11_BUFFER_DESC BufferDesc{};
	GpuParticles.mBuffer->GetDesc(&BufferDesc);
	const uint64_t iRequiredBytes =
		(static_cast<uint64_t>(iParticleOffset) +
			iParticleCount) *
		sizeof(physx::PxVec4);
	if (iRequiredBytes > BufferDesc.ByteWidth)
		return false;

	if (!Record.pGpuParticleSRV ||
		Record.pGpuParticleBuffer !=
			GpuParticles.mBuffer ||
		Record.iGpuParticleOffset !=
			iParticleOffset)
	{
		Record.pGpuParticleSRV.Reset();

		D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
		SrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		SrvDesc.ViewDimension =
			D3D11_SRV_DIMENSION_BUFFEREX;
		SrvDesc.BufferEx.FirstElement =
			iParticleOffset *
			(sizeof(physx::PxVec4) / sizeof(uint32_t));
		SrvDesc.BufferEx.NumElements =
			iParticleCount *
			(sizeof(physx::PxVec4) / sizeof(uint32_t));
		SrvDesc.BufferEx.Flags =
			D3D11_BUFFEREX_SRV_FLAG_RAW;

		if (FAILED(
			m_pImpl->pDxContext->getDevice()->
			CreateShaderResourceView(
				GpuParticles.mBuffer,
				&SrvDesc,
				Record.pGpuParticleSRV.GetAddressOf())))
		{
			Record.pGpuParticleBuffer = nullptr;
			Record.iGpuParticleOffset = 0;
			return false;
		}

		Record.pGpuParticleBuffer =
			GpuParticles.mBuffer;
		Record.iGpuParticleOffset =
			iParticleOffset;
	}

	OutView.pSRV = Record.pGpuParticleSRV.Get();
	OutView.iParticleCount = iParticleCount;
	return OutView.pSRV != nullptr;
}

_bool CNvClothManager::SetClothTransform(
	NVCLOTH_CLOTH_HANDLE Handle,
	const _float3& vTranslation,
	const _float4& vRotation,
	_bool bTeleport)
{
	ZoneScopedN("NvCloth_SetClothTransform");

	if (!m_pImpl ||
		!Handle ||
		!IsFinite(vTranslation) ||
		!IsFinite(vRotation))
	{
		return false;
	}

	const _vector qRotation = XMLoadFloat4(&vRotation);
	const float fLengthSq =
		XMVectorGetX(XMQuaternionLengthSq(qRotation));
	if (!std::isfinite(fLengthSq) ||
		fLengthSq <= 1.e-8f)
	{
		return false;
	}

	_float4 vNormalizedRotation{};
	XMStoreFloat4(
		&vNormalizedRotation,
		XMQuaternionNormalize(qRotation));

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	const physx::PxVec3 Translation{
		vTranslation.x,
		vTranslation.y,
		vTranslation.z };
	const physx::PxQuat Rotation{
		vNormalizedRotation.x,
		vNormalizedRotation.y,
		vNormalizedRotation.z,
		vNormalizedRotation.w };

	if (bTeleport)
	{
		Record.pCloth->teleportToLocation(
			Translation,
			Rotation);
		Record.pCloth->clearInertia();
		Record.pCloth->clearInterpolation();
		Record.vecCollisionSpheres.clear();
	}
	else
	{
		Record.pCloth->setTranslation(Translation);
		Record.pCloth->setRotation(Rotation);
	}

	Record.vTranslation = vTranslation;
	Record.vRotation = vNormalizedRotation;
	return true;
}

_bool CNvClothManager::SetClothWind(
	NVCLOTH_CLOTH_HANDLE Handle,
	const NVCLOTH_WIND_DESC& Desc)
{
	ZoneScopedN("NvCloth_SetClothWind");

	if (!m_pImpl ||
		!Handle ||
		!IsFinite(Desc.vVelocity) ||
		!std::isfinite(Desc.fDragCoefficient) ||
		!std::isfinite(Desc.fLiftCoefficient) ||
		!std::isfinite(Desc.fFluidDensity) ||
		Desc.fDragCoefficient < 0.f ||
		Desc.fDragCoefficient > 1.f ||
		Desc.fLiftCoefficient < 0.f ||
		Desc.fLiftCoefficient > 1.f ||
		Desc.fFluidDensity < 0.f)
	{
		return false;
	}

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto* pCloth = Iter->second->pCloth;
	pCloth->setWindVelocity({
		Desc.vVelocity.x,
		Desc.vVelocity.y,
		Desc.vVelocity.z });
	pCloth->setDragCoefficient(
		Desc.fDragCoefficient);
	pCloth->setLiftCoefficient(
		Desc.fLiftCoefficient);
	pCloth->setFluidDensity(
		Desc.fFluidDensity);
	return true;
}

_bool CNvClothManager::SetClothSelfCollision(
	NVCLOTH_CLOTH_HANDLE Handle,
	_float fDistance,
	_float fStiffness)
{
	ZoneScopedN("NvCloth_SetSelfCollision");

	if (!m_pImpl ||
		!Handle ||
		!std::isfinite(fDistance) ||
		!std::isfinite(fStiffness) ||
		fDistance < 0.f ||
		fStiffness < 0.f ||
		fStiffness > 1.f)
	{
		return false;
	}

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto* pCloth = Iter->second->pCloth;
	if (fDistance > 0.f &&
		fStiffness > 0.f &&
		pCloth->getNumRestPositions() == 0)
	{
		const auto& Record = *Iter->second;
		if (Record.vecDebugPositions.size() !=
			Record.vecInverseMasses.size())
		{
			return false;
		}

		std::vector<physx::PxVec4> vecRestPositions{};
		vecRestPositions.reserve(
			Record.vecDebugPositions.size());
		for (size_t i = 0;
			i < Record.vecDebugPositions.size();
			++i)
		{
			const auto& vPosition =
				Record.vecDebugPositions[i];
			vecRestPositions.emplace_back(
				vPosition.x,
				vPosition.y,
				vPosition.z,
				Record.vecInverseMasses[i]);
		}
		pCloth->setRestPositions(
			nv::cloth::Range<const physx::PxVec4>{
				vecRestPositions.data(),
				vecRestPositions.data() +
					vecRestPositions.size() });
	}
	pCloth->setSelfCollisionDistance(fDistance);
	pCloth->setSelfCollisionStiffness(fStiffness);
	return true;
}

_bool CNvClothManager::SetClothCollisions(
	NVCLOTH_CLOTH_HANDLE Handle,
	const NVCLOTH_COLLISION_DESC& Desc)
{
	ZoneScopedN("NvCloth_SetClothCollisions");

	if (!m_pImpl ||
		!Handle ||
		!ValidateCollisionDesc(Desc))
	{
		return false;
	}

	std::vector<physx::PxVec4> vecSpheres{};
	vecSpheres.reserve(Desc.vecSpheres.size());
	for (const auto& Sphere : Desc.vecSpheres)
	{
		vecSpheres.emplace_back(
			Sphere.vCenter.x,
			Sphere.vCenter.y,
			Sphere.vCenter.z,
			Sphere.fRadius);
	}

	std::vector<uint32_t> vecCapsules{};
	vecCapsules.reserve(Desc.vecCapsules.size() * 2);
	for (const auto& Capsule : Desc.vecCapsules)
	{
		vecCapsules.push_back(Capsule.iSphere0);
		vecCapsules.push_back(Capsule.iSphere1);
	}

	std::vector<physx::PxVec4> vecPlanes{};
	vecPlanes.reserve(Desc.vecPlanes.size());
	for (const auto& Plane : Desc.vecPlanes)
	{
		vecPlanes.emplace_back(
			Plane.vNormal.x,
			Plane.vNormal.y,
			Plane.vNormal.z,
			Plane.fDistance);
	}

	std::vector<uint32_t> vecConvexes{};
	vecConvexes.reserve(Desc.vecConvexes.size());
	for (const auto& Convex : Desc.vecConvexes)
		vecConvexes.push_back(Convex.iPlaneMask);

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	auto* pCloth = Record.pCloth;
	const _bool bTopologyChanged =
		Record.vecCollisionSpheres.size() !=
			vecSpheres.size() ||
		Record.vecCollisionCapsules != vecCapsules ||
		Record.vecCollisionPlanes.size() !=
			vecPlanes.size() ||
		Record.vecCollisionConvexes != vecConvexes;

	if (bTopologyChanged)
	{
		// Removing planes first also removes convex masks referencing them.
		// NvCloth 1.1.6 can underflow its internal delta when convexes are
		// explicitly shrunk before their planes.
		pCloth->setPlanes(
			{},
			0,
			pCloth->getNumPlanes());
		pCloth->setCapsules(
			{},
			0,
			pCloth->getNumCapsules());
		pCloth->setSpheres(
			MakeRange(vecSpheres),
			0,
			pCloth->getNumSpheres());
		pCloth->setCapsules(
			MakeRange(vecCapsules),
			0,
			0);
		pCloth->setPlanes(
			MakeRange(vecPlanes),
			0,
			0);
		pCloth->setConvexes(
			MakeRange(vecConvexes),
			0,
			0);
		pCloth->clearInterpolation();
	}
	else
	{
		if (!vecSpheres.empty())
		{
			pCloth->setSpheres(
				MakeRange(Record.vecCollisionSpheres),
				MakeRange(vecSpheres));
		}
		if (!vecPlanes.empty())
		{
			pCloth->setPlanes(
				MakeRange(Record.vecCollisionPlanes),
				MakeRange(vecPlanes));
		}
	}

	pCloth->enableContinuousCollision(
		Desc.bContinuousCollision);
	pCloth->setCollisionMassScale(
		Desc.fCollisionMassScale);
	pCloth->setFriction(Desc.fFriction);

	Record.vecCollisionSpheres =
		std::move(vecSpheres);
	Record.vecCollisionCapsules =
		std::move(vecCapsules);
	Record.vecCollisionPlanes =
		std::move(vecPlanes);
	Record.vecCollisionConvexes =
		std::move(vecConvexes);
	return true;
}

_bool CNvClothManager::SetClothAnimationConstraints(
	NVCLOTH_CLOTH_HANDLE Handle,
	const NVCLOTH_ANIMATION_CONSTRAINT_DESC& Desc)
{
	ZoneScopedN("NvCloth_SetAnimationConstraints");

	if (!m_pImpl ||
		!Handle ||
		!ValidateAnimationConstraintDesc(Desc))
	{
		return false;
	}

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	auto* pCloth = Record.pCloth;
	const uint32_t iParticleCount =
		pCloth->getNumParticles();
	if (Desc.vecTargetPositions.size() !=
			iParticleCount ||
		Record.vecInverseMasses.size() !=
			iParticleCount)
	{
		return false;
	}

	// Motion constraints keep every particle inside an animation-following
	// sphere. Radius zero pins the upper cape; larger radii leave the lower
	// cape progressively freer for simulation.
	auto Constraints =
		pCloth->getMotionConstraints();
	if (Constraints.size() != iParticleCount)
		return false;

	for (uint32_t i = 0;
		i < iParticleCount;
		++i)
	{
		const auto& vTarget =
			Desc.vecTargetPositions[i];
		Constraints[i] = physx::PxVec4{
			vTarget.x,
			vTarget.y,
			vTarget.z,
			Desc.vecMaxDistances[i] };
	}

	if (m_pImpl->bDebugConstraintWeights)
	{
		Record.vecMotionConstraintRadii =
			Desc.vecMaxDistances;
	}
	else if (!Record.vecMotionConstraintRadii.empty())
	{
		Record.vecMotionConstraintRadii.clear();
	}

	if (Desc.vecSeparationCenters.empty())
	{
		if (pCloth->getNumSeparationConstraints() != 0)
			pCloth->clearSeparationConstraints();
	}
	else
	{
		auto SeparationConstraints =
			pCloth->getSeparationConstraints();
		if (SeparationConstraints.size() !=
			iParticleCount)
		{
			return false;
		}

		for (uint32_t i = 0;
			i < iParticleCount;
			++i)
		{
			const auto& vCenter =
				Desc.vecSeparationCenters[i];
			SeparationConstraints[i] =
				physx::PxVec4{
					vCenter.x,
					vCenter.y,
					vCenter.z,
					Desc.vecSeparationRadii[i] };
		}
	}

	if (!Desc.bUpdateFixedParticles)
		return true;

	// NvCloth documents writable current particles as the mechanism for
	// moving animation attachment points. Only zero-inverse-mass particles
	// are overwritten; dynamic particles remain solver-owned.
	{
		auto CurrentParticles =
			pCloth->getCurrentParticles();
		if (CurrentParticles.size() != iParticleCount)
			return false;

		for (uint32_t i = 0;
			i < iParticleCount;
			++i)
		{
			if (Record.vecInverseMasses[i] > 0.f)
				continue;

			const auto& vTarget =
				Desc.vecTargetPositions[i];
			CurrentParticles[i] = physx::PxVec4{
				vTarget.x,
				vTarget.y,
				vTarget.z,
				0.f };
		}
	}

	if (Desc.bResetPreviousParticles)
	{
		auto PreviousParticles =
			pCloth->getPreviousParticles();
		if (PreviousParticles.size() != iParticleCount)
			return false;

		for (uint32_t i = 0;
			i < iParticleCount;
			++i)
		{
			if (Record.vecInverseMasses[i] > 0.f)
				continue;

			const auto& vTarget =
				Desc.vecTargetPositions[i];
			PreviousParticles[i] = physx::PxVec4{
				vTarget.x,
				vTarget.y,
				vTarget.z,
				0.f };
		}
		pCloth->clearInterpolation();
	}

	return true;
}

_bool CNvClothManager::ResetClothParticlesToPositions(
	NVCLOTH_CLOTH_HANDLE Handle,
	const std::vector<_float3>& Positions)
{
	ZoneScopedN("NvCloth_ResetClothParticlesToPositions");

	if (!m_pImpl || !Handle || Positions.empty())
		return false;

	for (const auto& Position : Positions)
	{
		if (!IsFinite(Position))
			return false;
	}

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	auto* pCloth = Record.pCloth;
	const uint32_t iParticleCount =
		pCloth->getNumParticles();
	if (Positions.size() != iParticleCount ||
		Record.vecInverseMasses.size() != iParticleCount)
	{
		return false;
	}

	{
		auto CurrentParticles =
			pCloth->getCurrentParticles();
		if (CurrentParticles.size() != iParticleCount)
			return false;

		for (uint32_t i = 0; i < iParticleCount; ++i)
		{
			const auto& Position = Positions[i];
			CurrentParticles[i] = physx::PxVec4{
				Position.x,
				Position.y,
				Position.z,
				Record.vecInverseMasses[i] };
		}
	}

	{
		auto PreviousParticles =
			pCloth->getPreviousParticles();
		if (PreviousParticles.size() != iParticleCount)
			return false;

		for (uint32_t i = 0; i < iParticleCount; ++i)
		{
			const auto& Position = Positions[i];
			PreviousParticles[i] = physx::PxVec4{
				Position.x,
				Position.y,
				Position.z,
				Record.vecInverseMasses[i] };
		}
	}

	pCloth->clearInertia();
	pCloth->clearInterpolation();
	return true;
}

_bool CNvClothManager::SetClothVirtualParticles(
	NVCLOTH_CLOTH_HANDLE Handle,
	_bool bEnabled)
{
	ZoneScopedN("NvCloth_SetVirtualParticles");

	if (!m_pImpl || !Handle)
		return false;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	const auto Iter = m_pImpl->Cloths.find(Handle.iValue);
	if (Iter == m_pImpl->Cloths.end() ||
		!Iter->second ||
		!Iter->second->pCloth)
	{
		return false;
	}

	auto& Record = *Iter->second;
	auto* pCloth = Record.pCloth;
	if (!bEnabled)
	{
		pCloth->setVirtualParticles({}, {});
		return pCloth->getNumVirtualParticles() == 0;
	}

	std::vector<uint32_t> vecVirtualParticleIndices{};
	vecVirtualParticleIndices.reserve(
		Record.vecIndices.size() / 3 * 4);
	for (size_t i = 0;
		i + 2 < Record.vecIndices.size();
		i += 3)
	{
		const uint32_t i0 = Record.vecIndices[i];
		const uint32_t i1 = Record.vecIndices[i + 1];
		const uint32_t i2 = Record.vecIndices[i + 2];
		if (i0 >= Record.vecInverseMasses.size() ||
			i1 >= Record.vecInverseMasses.size() ||
			i2 >= Record.vecInverseMasses.size())
		{
			return false;
		}

		if (Record.vecInverseMasses[i0] <= 0.f &&
			Record.vecInverseMasses[i1] <= 0.f &&
			Record.vecInverseMasses[i2] <= 0.f)
		{
			continue;
		}

		vecVirtualParticleIndices.insert(
			vecVirtualParticleIndices.end(),
			{ i0, i1, i2, 0u });
	}

	if (vecVirtualParticleIndices.empty())
	{
		pCloth->setVirtualParticles({}, {});
		return true;
	}

	using VIRTUAL_PARTICLE_INDICES =
		const uint32_t[4];
	const auto* pIndices =
		reinterpret_cast<const uint32_t(*)[4]>(
			vecVirtualParticleIndices.data());
	const size_t iVirtualParticleCount =
		vecVirtualParticleIndices.size() / 4;
	const physx::PxVec3 Weights[]{
		physx::PxVec3{
			1.f / 3.f,
			1.f / 3.f,
			1.f / 3.f }
	};
	pCloth->setVirtualParticles(
		nv::cloth::Range<VIRTUAL_PARTICLE_INDICES>{
			pIndices,
			pIndices + iVirtualParticleCount },
		nv::cloth::Range<const physx::PxVec3>{
			Weights,
			Weights + 1 });
	return pCloth->getNumVirtualParticles() ==
		iVirtualParticleCount;
}

void CNvClothManager::StepSimulation(
	_float fFixedTimeDelta)
{
	ZoneScopedN("NvCloth_StepSimulation");

	if (!m_pImpl ||
		!m_pImpl->pSolver ||
		!std::isfinite(fFixedTimeDelta) ||
		fFixedTimeDelta <= 0.f)
	{
		return;
	}

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	_bool bSimulationBegun{};
	{
		ZoneScopedN("NvCloth_BeginSimulation");
		bSimulationBegun =
			m_pImpl->pSolver->beginSimulation(
				fFixedTimeDelta);
	}
	if (!bSimulationBegun)
		return;

	const auto iChunkCount =
		m_pImpl->pSolver->getSimulationChunkCount();
	{
		ZoneScopedN("NvCloth_SimulateChunks");
		for (int i = 0; i < iChunkCount; ++i)
			m_pImpl->pSolver->simulateChunk(i);
	}
	{
		ZoneScopedN("NvCloth_EndSimulation");
		m_pImpl->pSolver->endSimulation();
	}

	if (m_pImpl->eBackend == NVCLOTH_BACKEND::CPU)
	{
		for (auto& [iHandle, pRecord] :
			m_pImpl->Cloths)
		{
			if (pRecord)
				pRecord->bCpuParticleBufferDirty = true;
		}
	}

	if (m_pImpl->pSolver->hasError())
	{
		if (!m_pImpl->bSolverErrorLogged)
		{
			OutputDebugStringA(
				"[NvCloth] Solver entered an unrecoverable error state.\n");
			m_pImpl->bSolverErrorLogged = true;
		}
		return;
	}

	if (!m_pImpl->bDebugDraw)
		return;

	{
		ZoneScopedN("NvCloth_DebugParticleReadback");
		for (auto& [iHandle, pRecord] : m_pImpl->Cloths)
		{
			if (!pRecord || !pRecord->pCloth)
				continue;

			const auto& Cloth =
				static_cast<const nv::cloth::Cloth&>(
					*pRecord->pCloth);
			const auto Particles =
				Cloth.getCurrentParticles();
			pRecord->vecDebugPositions.resize(
				Particles.size());
			for (uint32_t i = 0;
				i < Particles.size();
				++i)
			{
				const physx::PxVec3 vLocal{
					Particles[i].x,
					Particles[i].y,
					Particles[i].z };
				const physx::PxQuat qRotation{
					pRecord->vRotation.x,
					pRecord->vRotation.y,
					pRecord->vRotation.z,
					pRecord->vRotation.w };
				const physx::PxVec3 vWorld =
					qRotation.rotate(vLocal) +
					physx::PxVec3{
						pRecord->vTranslation.x,
						pRecord->vTranslation.y,
						pRecord->vTranslation.z };
				pRecord->vecDebugPositions[i] = {
					vWorld.x,
					vWorld.y,
					vWorld.z };
			}
		}
	}
}

void CNvClothManager::RenderDebug(
	CDbgLineRender& DbgLineRender) const
{
	ZoneScopedN("NvCloth_DebugLineSubmit");

	if (!m_pImpl)
		return;

	std::scoped_lock Lock{ m_pImpl->StateMutex };
	if (!m_pImpl->bDebugDraw)
		return;

	const auto OldColor = DbgLineRender.GetColor();
	const auto OldDepthMode = DbgLineRender.GetDepthMode();
	DbgLineRender.SetDepthTest(false);

	for (const auto& [iHandle, pRecord] : m_pImpl->Cloths)
	{
		if (!pRecord ||
			pRecord->vecDebugPositions.empty() ||
			pRecord->vecIndices.empty())
		{
			continue;
		}

		const _bool bDrawConstraintWeights =
			m_pImpl->bDebugConstraintWeights &&
			pRecord->vecMotionConstraintRadii.size() ==
				pRecord->vecDebugPositions.size();
		if (bDrawConstraintWeights)
		{
			const float fMaxRadius =
				*std::max_element(
					pRecord->vecMotionConstraintRadii.begin(),
					pRecord->vecMotionConstraintRadii.end());
			const float fSafeMaxRadius =
				std::max(fMaxRadius, FLT_EPSILON);

			const auto DrawWeightedEdge =
				[&DbgLineRender,
					pRecord = pRecord.get(),
					fSafeMaxRadius](
						uint32_t i0,
						uint32_t i1)
			{
				if (i0 >= pRecord->vecDebugPositions.size() ||
					i1 >= pRecord->vecDebugPositions.size())
				{
					return;
				}

				const float fWeight =
					std::clamp(
						(pRecord->vecMotionConstraintRadii[i0] +
							pRecord->vecMotionConstraintRadii[i1]) *
							0.5f / fSafeMaxRadius,
						0.f,
						1.f);
				const _float4 vColor{
					1.f - fWeight,
					std::min(1.f, fWeight * 2.f),
					fWeight,
					1.f };
				DbgLineRender.AddLine(
					pRecord->vecDebugPositions[i0],
					pRecord->vecDebugPositions[i1],
					vColor);
			};

			for (size_t i = 0;
				i + 2 < pRecord->vecIndices.size();
				i += 3)
			{
				const uint32_t i0 =
					pRecord->vecIndices[i];
				const uint32_t i1 =
					pRecord->vecIndices[i + 1];
				const uint32_t i2 =
					pRecord->vecIndices[i + 2];
				DrawWeightedEdge(i0, i1);
				DrawWeightedEdge(i1, i2);
				DrawWeightedEdge(i2, i0);
			}
		}
		else
		{
			DbgLineRender.SetColor({ 0.f, 0.8f, 1.f, 1.f });
			DbgLineRender.AddTriangleMesh(
				pRecord->vecDebugPositions.data(),
				static_cast<uint32_t>(
					pRecord->vecDebugPositions.size()),
				pRecord->vecIndices.data(),
				static_cast<uint32_t>(
					pRecord->vecIndices.size() / 3));
		}

		DbgLineRender.SetColor({ 1.f, 0.2f, 0.1f, 1.f });
		const auto iParticleCount = std::min(
			pRecord->vecDebugPositions.size(),
			pRecord->vecInverseMasses.size());
		for (size_t i = 0; i < iParticleCount; ++i)
		{
			if (pRecord->vecInverseMasses[i] == 0.f)
				DbgLineRender.AddCross(
					pRecord->vecDebugPositions[i],
					0.08f);
		}

		const _matrix ClothWorld =
			XMMatrixRotationQuaternion(
				XMLoadFloat4(
					&pRecord->vRotation)) *
			XMMatrixTranslation(
				pRecord->vTranslation.x,
				pRecord->vTranslation.y,
				pRecord->vTranslation.z);

		DbgLineRender.SetColor(
			{ 0.2f, 1.f, 0.2f, 1.f });
		for (const auto& Sphere :
			pRecord->vecCollisionSpheres)
		{
			DbgLineRender.AddSphere(
				Sphere.w,
				XMMatrixTranslation(
					Sphere.x,
					Sphere.y,
					Sphere.z) *
				ClothWorld);
		}

		for (size_t i = 0;
			i + 1 <
				pRecord->vecCollisionCapsules.size();
			i += 2)
		{
			const auto iSphere0 =
				pRecord->vecCollisionCapsules[i];
			const auto iSphere1 =
				pRecord->vecCollisionCapsules[i + 1];
			if (iSphere0 >=
					pRecord->vecCollisionSpheres.size() ||
				iSphere1 >=
					pRecord->vecCollisionSpheres.size())
			{
				continue;
			}

			const auto& Sphere0 =
				pRecord->vecCollisionSpheres[
					iSphere0];
			const auto& Sphere1 =
				pRecord->vecCollisionSpheres[
					iSphere1];
			_float3 vWorld0{};
			_float3 vWorld1{};
			XMStoreFloat3(
				&vWorld0,
				XMVector3TransformCoord(
					XMVectorSet(
						Sphere0.x,
						Sphere0.y,
						Sphere0.z,
						1.f),
					ClothWorld));
			XMStoreFloat3(
				&vWorld1,
				XMVector3TransformCoord(
					XMVectorSet(
						Sphere1.x,
						Sphere1.y,
						Sphere1.z,
						1.f),
					ClothWorld));
			DbgLineRender.AddLine(
				vWorld0,
				vWorld1);
		}
	}

	DbgLineRender.SetColor(OldColor);
	DbgLineRender.SetDepthMode(OldDepthMode);
}

void CNvClothManager::UpdateGUI()
{
	if (m_pCollisionEditor)
		m_pCollisionEditor->UpdateGUI();

	if (!ImGui::Begin("NvCloth Manager"))
	{
		ImGui::End();
		return;
	}

	if (!m_pImpl)
	{
		ImGui::TextDisabled("NvCloth manager is not initialized.");
		ImGui::End();
		return;
	}

	if (ImGui::Button(
		"Open Collision Rig Editor") &&
		m_pCollisionEditor)
	{
		m_pCollisionEditor->Open();
	}
	ImGui::Separator();

	std::scoped_lock Lock{ m_pImpl->StateMutex };

	const _bool bInitialized =
		m_pImpl->pFactory &&
		m_pImpl->pSolver &&
		(m_pImpl->eBackend == NVCLOTH_BACKEND::CPU ||
			m_pImpl->pDxContext);
	ImGui::TextColored(
		bInitialized ?
			ImVec4{ 0.35f, 0.9f, 0.45f, 1.f } :
			ImVec4{ 0.95f, 0.35f, 0.3f, 1.f },
		bInitialized ? "Initialized" : "Not Initialized");
	ImGui::SameLine();
	ImGui::TextDisabled(
		m_pImpl->eBackend == NVCLOTH_BACKEND::DX11 ?
			"| DX11 Backend" :
			"| CPU Backend");
	ImGui::SameLine();
	ImGui::Checkbox("Debug Draw Cloth", &m_pImpl->bDebugDraw);
	if (m_pImpl->bDebugDraw)
	{
		ImGui::SameLine();
		ImGui::Checkbox(
			"Constraint Weights",
			&m_pImpl->bDebugConstraintWeights);
	}
	else
	{
		m_pImpl->bDebugConstraintWeights = false;
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Summary");
	if (ImGui::BeginTable(
		"NvClothSummary",
		2,
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Item");
		ImGui::TableSetupColumn("Count");
		ImGui::TableHeadersRow();

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted("Fabric Records");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%zu", m_pImpl->Fabrics.size());

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted("Cloth Records");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%zu", m_pImpl->Cloths.size());

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted("Solver Cloths");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text(
			"%u",
			m_pImpl->pSolver ?
				m_pImpl->pSolver->getNumCloths() :
				0u);

		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader(
		"Fabrics",
		ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (m_pImpl->Fabrics.empty())
		{
			ImGui::TextDisabled("No fabric records.");
		}
		else if (ImGui::BeginTable(
			"NvClothFabrics",
			7,
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("Handle");
			ImGui::TableSetupColumn("Particles");
			ImGui::TableSetupColumn("Phases");
			ImGui::TableSetupColumn("Constraints");
			ImGui::TableSetupColumn("Tethers");
			ImGui::TableSetupColumn("Triangles");
			ImGui::TableSetupColumn("Cloth Refs");
			ImGui::TableHeadersRow();

			std::vector<uint64_t> FabricHandles{};
			FabricHandles.reserve(
				m_pImpl->Fabrics.size());
			for (const auto& [iHandle, pRecord] :
				m_pImpl->Fabrics)
			{
				FabricHandles.push_back(iHandle);
			}
			std::sort(
				FabricHandles.begin(),
				FabricHandles.end());

			for (const auto iHandle : FabricHandles)
			{
				const auto Iter =
					m_pImpl->Fabrics.find(iHandle);
				if (Iter == m_pImpl->Fabrics.end() ||
					!Iter->second)
				{
					continue;
				}

				const auto& Record = *Iter->second;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(
					"%llu",
					static_cast<unsigned long long>(
						iHandle));
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(
					"%u",
					Record.Info.iParticleCount);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text(
					"%u",
					Record.Info.iPhaseCount);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text(
					"%u",
					Record.Info.iConstraintCount);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text(
					"%u",
					Record.Info.iTetherCount);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text(
					"%u",
					Record.Info.iTriangleCount);
				ImGui::TableSetColumnIndex(6);
				ImGui::Text(
					"%u",
					Record.iClothReferenceCount);
			}

			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader(
		"Cloths",
		ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (m_pImpl->Cloths.empty())
		{
			ImGui::TextDisabled("No cloth records.");
		}
		else if (ImGui::BeginTable(
			"NvClothCloths",
			9,
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("Handle");
			ImGui::TableSetupColumn("Fabric");
			ImGui::TableSetupColumn("Particles");
			ImGui::TableSetupColumn("Virtual");
			ImGui::TableSetupColumn("Triangles");
			ImGui::TableSetupColumn("Spheres");
			ImGui::TableSetupColumn("Capsules");
			ImGui::TableSetupColumn("Render SRV");
			ImGui::TableSetupColumn("Particle Offset");
			ImGui::TableHeadersRow();

			std::vector<uint64_t> ClothHandles{};
			ClothHandles.reserve(
				m_pImpl->Cloths.size());
			for (const auto& [iHandle, pRecord] :
				m_pImpl->Cloths)
			{
				ClothHandles.push_back(iHandle);
			}
			std::sort(
				ClothHandles.begin(),
				ClothHandles.end());

			for (const auto iHandle : ClothHandles)
			{
				const auto Iter =
					m_pImpl->Cloths.find(iHandle);
				if (Iter == m_pImpl->Cloths.end() ||
					!Iter->second)
				{
					continue;
				}

				const auto& Record = *Iter->second;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(
					"%llu",
					static_cast<unsigned long long>(
						iHandle));
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(
					"%llu",
					static_cast<unsigned long long>(
						Record.iFabricHandle));
				ImGui::TableSetColumnIndex(2);
				ImGui::Text(
					"%u",
					Record.pCloth ?
						Record.pCloth->getNumParticles() :
						0u);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text(
					"%u",
					Record.pCloth ?
						Record.pCloth->
							getNumVirtualParticles() :
						0u);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text(
					"%zu",
					Record.vecIndices.size() / 3);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text(
					"%zu",
					Record.vecCollisionSpheres.size());
				ImGui::TableSetColumnIndex(6);
				ImGui::Text(
					"%zu",
					Record.vecCollisionCapsules.size() /
						2);
				ImGui::TableSetColumnIndex(7);
				const _bool bRenderViewReady =
					m_pImpl->eBackend ==
						NVCLOTH_BACKEND::DX11 ?
						static_cast<_bool>(
							Record.pGpuParticleSRV) :
						static_cast<_bool>(
							Record.pCpuParticleSRV);
				ImGui::TextColored(
					bRenderViewReady ?
						ImVec4{
							0.35f,
							0.9f,
							0.45f,
							1.f } :
						ImVec4{
							0.8f,
							0.8f,
							0.8f,
							1.f },
					bRenderViewReady ?
						"Ready" : "Not Created");
				ImGui::TableSetColumnIndex(8);
				ImGui::Text(
					"%u",
					m_pImpl->eBackend ==
						NVCLOTH_BACKEND::DX11 ?
						Record.iGpuParticleOffset :
						0u);
			}

			ImGui::EndTable();
		}
	}

	ImGui::End();
}

UPtr<CNvClothManager> CNvClothManager::Create(
	ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext,
	NVCLOTH_BACKEND eBackend)
{
	auto pInstance = ToUPtr(new CNvClothManager{});
	if (FAILED(pInstance->Initialize(
		pDevice,
		pContext,
		eBackend)))
		return nullptr;

	return pInstance;
}

void CNvClothManager::Free()
{
	m_pCollisionEditor.reset();
	m_pImpl.reset();
	CEngineBase::Free();
}

NS_END
