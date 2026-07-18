#include "pch.h"
#include "ResPhysXRTConvexGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

using namespace physx;
NS_USING(Engine)

CResPhysXRTConvexGeometry::CResPhysXRTConvexGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXRTConvexGeometry::~CResPhysXRTConvexGeometry() = default;

HRESULT CResPhysXRTConvexGeometry::Load(const std::any& arg)
{
	const auto* desc = std::any_cast<DESC>(&arg);
	if (!desc || !desc->pVertexData || desc->iVertexCount < 4 ||
		desc->iVertexStride < sizeof(_float3) ||
		desc->iPositionOffset > desc->iVertexStride - sizeof(_float3) ||
		desc->iVertexLimit < 8 || desc->iVertexLimit > 255)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	auto* physics = CGameInstance::Get().PxGetPhysics();
	if (!physics)
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	const auto* vertexBytes = static_cast<const std::byte*>(desc->pVertexData);
	PxConvexMeshDesc convexDesc{};
	convexDesc.points.count = desc->iVertexCount;
	convexDesc.points.stride = desc->iVertexStride;
	convexDesc.points.data = vertexBytes + desc->iPositionOffset;
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	convexDesc.vertexLimit = desc->iVertexLimit;

	if (!convexDesc.isValid())
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][RTConvex] Invalid mesh descriptor.\n");
		return E_FAIL;
	}

	PxCookingParams params{ physics->getTolerancesScale() };
	PxDefaultMemoryOutputStream outputBuffer{};
	PxConvexMeshCookingResult::Enum cookingResult{ PxConvexMeshCookingResult::eFAILURE };
	if (!PxCookConvexMesh(params, convexDesc, outputBuffer, &cookingResult))
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][RTConvex] Cooking failed.\n");
		return E_FAIL;
	}

	if (cookingResult == PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED)
		DEBUG_LOG("[PX][RTConvex] Polygon limit reached; the cooked hull was simplified.\n");
	else if (cookingResult == PxConvexMeshCookingResult::eNON_GPU_COMPATIBLE)
		DEBUG_LOG("[PX][RTConvex] Cooked hull is not GPU compatible.\n");

	PxDefaultMemoryInputData input{ outputBuffer.getData(), outputBuffer.getSize() };
	PxConvexMesh* convexMesh = physics->createConvexMesh(input);
	if (!convexMesh)
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][RTConvex] createConvexMesh failed.\n");
		return E_FAIL;
	}

	m_pConvexMesh = convexMesh;
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXRTConvexGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXRTConvexGeometry> CResPhysXRTConvexGeometry::Create()
{
	return ToSPtr(new CResPhysXRTConvexGeometry{ "" });
}

void CResPhysXRTConvexGeometry::Free()
{
	if (m_pConvexMesh)
	{
		m_pConvexMesh->release();
		m_pConvexMesh = nullptr;
	}
	CResPhysXGeometry::Free();
}
