#include "pch.h"
#include "ResPhysXRTConvexGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

#include "PhysXCookedMeshFile.h"

using namespace physx;
NS_USING(Engine)

namespace
{
	_bool IsValidDesc(const CResPhysXRTConvexGeometry::DESC& desc)
	{
		return desc.pVertexData &&
			desc.iVertexCount >= 4 &&
			desc.iVertexStride >= sizeof(_float3) &&
			desc.iPositionOffset <= desc.iVertexStride - sizeof(_float3) &&
			desc.iVertexLimit >= 8 &&
			desc.iVertexLimit <= 255;
	}
}

CResPhysXRTConvexGeometry::CResPhysXRTConvexGeometry(const _string& sPath)
	: CResPhysXConvexGeometry{ sPath }
{
}

CResPhysXRTConvexGeometry::~CResPhysXRTConvexGeometry() = default;

HRESULT CResPhysXRTConvexGeometry::Load(const std::any& arg)
{
	const auto* pDesc = std::any_cast<DESC>(&arg);
	if (!pDesc)
		return E_INVALIDARG;

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<uint8_t> cookedData{};
	if (FAILED(CookToMemory(*pDesc, cookedData)) ||
		FAILED(CreateFromCookedData(cookedData.data(), cookedData.size())))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXRTConvexGeometry::CookToMemory(
	const DESC& desc,
	std::vector<uint8_t>& outCookedData)
{
	outCookedData.clear();
	if (!IsValidDesc(desc))
		return E_INVALIDARG;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	const auto* pVertexBytes = static_cast<const std::byte*>(desc.pVertexData);
	PxConvexMeshDesc meshDesc{};
	meshDesc.points.count = desc.iVertexCount;
	meshDesc.points.stride = desc.iVertexStride;
	meshDesc.points.data = pVertexBytes + desc.iPositionOffset;
	meshDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	meshDesc.vertexLimit = desc.iVertexLimit;

	if (!meshDesc.isValid())
	{
		DEBUG_LOG("[PX][RTConvex] Invalid mesh descriptor.\n");
		return E_INVALIDARG;
	}

	PxCookingParams params{ pPhysics->getTolerancesScale() };
	PxDefaultMemoryOutputStream output{};
	PxConvexMeshCookingResult::Enum result{ PxConvexMeshCookingResult::eFAILURE };
	if (!PxCookConvexMesh(params, meshDesc, output, &result))
	{
		DEBUG_LOG("[PX][RTConvex] Cooking failed.\n");
		return E_FAIL;
	}

	if (result == PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED)
		DEBUG_LOG("[PX][RTConvex] Polygon limit reached; the cooked hull was simplified.\n");
	else if (result == PxConvexMeshCookingResult::eNON_GPU_COMPATIBLE)
		DEBUG_LOG("[PX][RTConvex] Cooked hull is not GPU compatible.\n");

	outCookedData.assign(output.getData(), output.getData() + output.getSize());
	return outCookedData.empty() ? E_FAIL : S_OK;
}

HRESULT CResPhysXRTConvexGeometry::CookToFile(
	const DESC& desc,
	const _string& sOutputPath)
{
	std::vector<uint8_t> cookedData{};
	if (FAILED(CookToMemory(desc, cookedData)))
		return E_FAIL;

	return PhysXCookedMeshFile::Write(
		sOutputPath,
		PhysXCookedMeshFile::TYPE::CONVEX_MESH,
		cookedData.data(),
		cookedData.size());
}

SPtr<CResPhysXRTConvexGeometry> CResPhysXRTConvexGeometry::Create()
{
	return ToSPtr(new CResPhysXRTConvexGeometry{ "" });
}

SPtr<CResPhysXRTConvexGeometry> CResPhysXRTConvexGeometry::CreateAndLoad(const DESC& desc)
{
	auto pInstance = Create();
	if (!pInstance || FAILED(pInstance->Load(desc)))
		return nullptr;

	return pInstance;
}
