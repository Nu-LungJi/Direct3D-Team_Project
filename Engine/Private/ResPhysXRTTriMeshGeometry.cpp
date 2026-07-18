#include "pch.h"
#include "ResPhysXRTTriMeshGeometry.h"
#include "GameInstance.h"
#include "ResPhysXGeometry.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

CResPhysXRTTriMeshGeometry::CResPhysXRTTriMeshGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXRTTriMeshGeometry::~CResPhysXRTTriMeshGeometry()
{
}

HRESULT CResPhysXRTTriMeshGeometry::Load(const std::any& arg)
{
	auto desc = std::any_cast<DESC>(&arg);
	if (!desc || !desc->pVertexData || !desc->pIndexData ||
		desc->iVertexCount < 3 || desc->iIndexCount < 3 ||
		desc->iIndexCount % 3 != 0 ||
		desc->iVertexStride < sizeof(_float3) ||
		desc->iPositionOffset > desc->iVertexStride - sizeof(_float3))
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	auto* physics = CGameInstance::Get().PxGetPhysics();
	if (!physics)
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	const auto* vertexBytes = static_cast<const std::byte*>(desc->pVertexData);
	PxTriangleMeshDesc meshDesc{};
	meshDesc.points.count = desc->iVertexCount;
	meshDesc.points.stride = desc->iVertexStride;
	meshDesc.points.data = vertexBytes + desc->iPositionOffset;
	meshDesc.triangles.count = desc->iIndexCount / 3;
	meshDesc.triangles.data = desc->pIndexData;

	if (desc->eIndexFormat == INDEX_FORMAT::UINT16)
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
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][TriMesh] Invalid mesh descriptor.\n");
		return E_FAIL;
	}

	PxCookingParams params{ physics->getTolerancesScale() };
#ifdef _DEBUG
	if (!PxValidateTriangleMesh(params, meshDesc))
		DEBUG_LOG("[PX][TriMesh] Mesh validation warning. Cooking will still be attempted.\n");
#endif

	PxDefaultMemoryOutputStream outputBuffer{};
	PxTriangleMeshCookingResult::Enum cookingResult{ PxTriangleMeshCookingResult::eFAILURE };
	if (!PxCookTriangleMesh(params, meshDesc, outputBuffer, &cookingResult))
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][TriMesh] Cooking failed.\n");
		return E_FAIL;
	}

	if (cookingResult == PxTriangleMeshCookingResult::eLARGE_TRIANGLE)
		DEBUG_LOG("[PX][TriMesh] Cooking succeeded with a large-triangle warning.\n");

	PxDefaultMemoryInputData readBuffer{ outputBuffer.getData(), outputBuffer.getSize() };
	PxTriangleMesh* triMesh = physics->createTriangleMesh(readBuffer);
	if (!triMesh)
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][TriMesh] createTriangleMesh failed.\n");
		return E_FAIL;
	}

	m_pTriMesh = triMesh;

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXRTTriMeshGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXRTTriMeshGeometry> CResPhysXRTTriMeshGeometry::Create()
{
	return ToSPtr(new CResPhysXRTTriMeshGeometry{ "" });
}

void CResPhysXRTTriMeshGeometry::Free()
{
	if (m_pTriMesh)
	{
		m_pTriMesh->release();
		m_pTriMesh = nullptr;
	}
	CResPhysXGeometry::Free();
}
