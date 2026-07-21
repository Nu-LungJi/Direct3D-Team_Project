#include "pch.h"
#include "ResPhysXConvexGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

#include "PhysXCookedMeshFile.h"

using namespace physx;
NS_USING(Engine)

CResPhysXConvexGeometry::CResPhysXConvexGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXConvexGeometry::~CResPhysXConvexGeometry() = default;

HRESULT CResPhysXConvexGeometry::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<uint8_t> cookedData{};
	if (FAILED(PhysXCookedMeshFile::Read(
		m_sPath,
		PhysXCookedMeshFile::TYPE::CONVEX_MESH,
		cookedData)) ||
		FAILED(CreateFromCookedData(cookedData.data(), cookedData.size())))
	{
		m_eState = STATE::LOADFAIL;
		DEBUG_LOG("[PX][Convex] Failed to load cooked mesh file.\n");
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXConvexGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

HRESULT CResPhysXConvexGeometry::CreateFromCookedData(
	uint8_t* pData,
	size_t iDataSize)
{
	if (!pData || iDataSize == 0 ||
		iDataSize > std::numeric_limits<PxU32>::max() ||
		m_pConvexMesh)
		return E_INVALIDARG;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	if (!pPhysics)
		return E_FAIL;

	PxDefaultMemoryInputData input{ pData, static_cast<PxU32>(iDataSize) };
	m_pConvexMesh = pPhysics->createConvexMesh(input);
	if (!m_pConvexMesh)
	{
		DEBUG_LOG("[PX][Convex] createConvexMesh failed.\n");
		return E_FAIL;
	}

	return S_OK;
}

SPtr<CResPhysXConvexGeometry> CResPhysXConvexGeometry::Create(const _string& sPath)
{
	return ToSPtr(new CResPhysXConvexGeometry{ sPath });
}

SPtr<CResPhysXConvexGeometry> CResPhysXConvexGeometry::CreateAndLoad(const _string& sPath)
{
	auto pInstance = Create(sPath);
	if (!pInstance || FAILED(pInstance->Load()))
		return nullptr;

	return pInstance;
}

void CResPhysXConvexGeometry::Free()
{
	if (m_pConvexMesh)
	{
		m_pConvexMesh->release();
		m_pConvexMesh = nullptr;
	}
	CResPhysXGeometry::Free();
}
