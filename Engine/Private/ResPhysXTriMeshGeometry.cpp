#include "pch.h"
#include "ResPhysXTriMeshGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

#include "PhysXCookedMeshFile.h"

using namespace physx;
NS_USING(Engine)

CResPhysXTriMeshGeometry::CResPhysXTriMeshGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXTriMeshGeometry::~CResPhysXTriMeshGeometry() = default;

HRESULT CResPhysXTriMeshGeometry::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<uint8_t> cookedData{};
	if (FAILED(PhysXCookedMeshFile::Read(
		m_sPath,
		PhysXCookedMeshFile::TYPE::TRIANGLE_MESH,
		cookedData)) ||
		FAILED(CreateFromCookedData(cookedData.data(), cookedData.size())))
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][TriMesh] Failed to load cooked mesh file.\n");
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXTriMeshGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

HRESULT CResPhysXTriMeshGeometry::CreateFromCookedData(
	uint8_t* pData,
	size_t iDataSize)
{
	if (!pData || iDataSize == 0 ||
		iDataSize > std::numeric_limits<PxU32>::max() ||
		m_pTriMesh)
		return E_INVALIDARG;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxDefaultMemoryInputData input{ pData, static_cast<PxU32>(iDataSize) };
	m_pTriMesh = pPhysics->createTriangleMesh(input);
	if (!m_pTriMesh)
	{
		DEBUG_LOG("[PX][TriMesh] createTriangleMesh failed.\n");
		return E_FAIL;
	}

	return S_OK;
}

SPtr<CResPhysXTriMeshGeometry> CResPhysXTriMeshGeometry::Create(const _string& sPath)
{
	return ToSPtr(new CResPhysXTriMeshGeometry{ sPath });
}

SPtr<CResPhysXTriMeshGeometry> CResPhysXTriMeshGeometry::CreateAndLoad(const _string& sPath)
{
	auto pInstance = Create(sPath);
	if (!pInstance || FAILED(pInstance->Load()))
		return nullptr;

	return pInstance;
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
