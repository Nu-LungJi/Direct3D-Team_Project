#include "pch.h"
#include "ResPhysXSphereGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")


using namespace physx;

NS_USING(Engine)

CResPhysXSphereGeometry::CResPhysXSphereGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXSphereGeometry::~CResPhysXSphereGeometry()
{
}

HRESULT CResPhysXSphereGeometry::Load(const std::any& arg)
{
	auto pDesc = std::any_cast<DESC>(&arg);
	if (!pDesc)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	{
		m_pSphereGeometry = std::make_unique<PxSphereGeometry>(PxSphereGeometry(pDesc->fRadius));

		m_eState = STATE::LOADED;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXSphereGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXSphereGeometry> CResPhysXSphereGeometry::Create()
{
	return ToSPtr(new CResPhysXSphereGeometry{ "" });
}

SPtr<CResPhysXSphereGeometry> CResPhysXSphereGeometry::CreateAndLoad(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXSphereGeometry{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}
