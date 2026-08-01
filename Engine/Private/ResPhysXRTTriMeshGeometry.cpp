#include "pch.h"
#include "ResPhysXRTTriMeshGeometry.h"
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
	_bool IsValidDesc(const CResPhysXRTTriMeshGeometry::DESC& desc)
	{
		return desc.pVertexData &&
			desc.pIndexData &&
			desc.iVertexCount >= 3 &&
			desc.iIndexCount >= 3 &&
			desc.iIndexCount % 3 == 0 &&
			desc.iVertexStride >= sizeof(_float3) &&
			desc.iPositionOffset <= desc.iVertexStride - sizeof(_float3);
	}
}

CResPhysXRTTriMeshGeometry::CResPhysXRTTriMeshGeometry(const _string& sPath)
	: CResPhysXTriMeshGeometry{ sPath }
{
}

CResPhysXRTTriMeshGeometry::~CResPhysXRTTriMeshGeometry() = default;

HRESULT CResPhysXRTTriMeshGeometry::Load(const std::any& arg)
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

HRESULT CResPhysXRTTriMeshGeometry::CookToMemory(
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
	PxTriangleMeshDesc meshDesc{};
	meshDesc.points.count = desc.iVertexCount;
	meshDesc.points.stride = desc.iVertexStride;
	meshDesc.points.data = pVertexBytes + desc.iPositionOffset;
	meshDesc.triangles.count = desc.iIndexCount / 3;
	meshDesc.triangles.data = desc.pIndexData;

	if (desc.eIndexFormat == INDEX_FORMAT::UINT16)
	{
		meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;
		meshDesc.triangles.stride = sizeof(uint16_t) * 3;
	}
	else
	{
		meshDesc.triangles.stride = sizeof(uint32_t) * 3;
	}

	if (!meshDesc.isValid())
	{
		DEBUG_LOG("[PX][TriMesh] Invalid mesh descriptor.\n");
		return E_INVALIDARG;
	}

	PxCookingParams params{ pPhysics->getTolerancesScale() };
	params.buildGPUData = true;
#ifdef _DEBUG
	if (!PxValidateTriangleMesh(params, meshDesc))
		DEBUG_LOG("[PX][TriMesh] Mesh validation warning. Cooking will still be attempted.\n");
#endif

	PxDefaultMemoryOutputStream output{};
	PxTriangleMeshCookingResult::Enum result{ PxTriangleMeshCookingResult::eFAILURE };
	if (!PxCookTriangleMesh(params, meshDesc, output, &result))
	{
		DEBUG_LOG("[PX][TriMesh] Cooking failed.\n");
		return E_FAIL;
	}

	if (result == PxTriangleMeshCookingResult::eLARGE_TRIANGLE)
		DEBUG_LOG("[PX][TriMesh] Cooking succeeded with a large-triangle warning.\n");

	outCookedData.assign(output.getData(), output.getData() + output.getSize());
	return outCookedData.empty() ? E_FAIL : S_OK;
}

HRESULT CResPhysXRTTriMeshGeometry::CookToFile(
	const DESC& desc,
	const _string& sOutputPath)
{
	std::vector<uint8_t> cookedData{};
	if (FAILED(CookToMemory(desc, cookedData)))
		return E_FAIL;

	return PhysXCookedMeshFile::Write(
		sOutputPath,
		PhysXCookedMeshFile::TYPE::TRIANGLE_MESH,
		cookedData.data(),
		cookedData.size());
}

SPtr<CResPhysXRTTriMeshGeometry> CResPhysXRTTriMeshGeometry::Create()
{
	return ToSPtr(new CResPhysXRTTriMeshGeometry{ "" });
}

SPtr<CResPhysXRTTriMeshGeometry> CResPhysXRTTriMeshGeometry::CreateAndLoad(const DESC& desc)
{
	auto pInstance = Create();
	if (!pInstance || FAILED(pInstance->Load(desc)))
		return nullptr;

	return pInstance;
}
