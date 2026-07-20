#include "pch.h"
#include "ResPhysXTriMeshGeometry.h"
#include "GameInstance.h"
#include "ResPhysXGeometry.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

using namespace physx;

NS_USING(Engine)

CResPhysXTriMeshGeometry::CResPhysXTriMeshGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXTriMeshGeometry::~CResPhysXTriMeshGeometry()
{
}

HRESULT CResPhysXTriMeshGeometry::Load(const std::any& arg)
{
	auto desc = std::any_cast<DESC>(&arg);
	if (!desc)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	{
        auto* physics = CGameInstance::Get().PxGetPhysics();
        auto* scene = CGameInstance::Get().PxGetScene();

        physx::PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = (physx::PxU32)desc->pVecVertices->size();
        meshDesc.points.stride = sizeof(physx::PxVec3);
        meshDesc.points.data = desc->pVecVertices->data();

        meshDesc.triangles.count = (physx::PxU32)desc->pVecTriangles->size();
        meshDesc.triangles.stride = 3 * sizeof(uint32_t);
        meshDesc.triangles.data = desc->pVecTriangles->data();

        physx::PxCookingParams params(physics->getTolerancesScale());

        physx::PxDefaultMemoryOutputStream outputBuffer;
        physx::PxTriangleMeshCookingResult::Enum cookingResult;
        _bool bSuccess = PxCookTriangleMesh(params, meshDesc, outputBuffer, &cookingResult);
        if (!bSuccess)
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResPhysXTriMesh Cooking Failed");
            return E_FAIL;
        }
        physx::PxDefaultMemoryInputData readBuffer(outputBuffer.getData(), outputBuffer.getSize());
        m_pTriMesh = physics->createTriangleMesh(readBuffer);
        m_eState = STATE::LOADED;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXTriMeshGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXTriMeshGeometry> CResPhysXTriMeshGeometry::Create()
{
	return ToSPtr(new CResPhysXTriMeshGeometry{ "" });
}

void CResPhysXTriMeshGeometry::Free()
{
	if (m_pTriMesh)
	{
		m_pTriMesh->release();
		m_pTriMesh = nullptr;
	}
	CResPhysXGeometry::Free();
}
