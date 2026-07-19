#include "pch.h"
#include "ResPhysXCapsuleGeometry.h"
#include "GameInstance.h"

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")


using namespace physx;

NS_USING(Engine)

CResPhysXCapsuleGeometry::CResPhysXCapsuleGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXCapsuleGeometry::~CResPhysXCapsuleGeometry()
{
}

HRESULT CResPhysXCapsuleGeometry::Load(const std::any& arg)
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
		m_pCapsuleGeometry = std::make_unique<PxCapsuleGeometry>(PxCapsuleGeometry(pDesc->fRadius, pDesc->fHalfHeight));
		m_eState = STATE::LOADED;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXCapsuleGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXCapsuleGeometry> CResPhysXCapsuleGeometry::Create()
{
	return ToSPtr(new CResPhysXCapsuleGeometry{ "" });
}

SPtr<CResPhysXCapsuleGeometry> CResPhysXCapsuleGeometry::CreateAndLoad(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXCapsuleGeometry{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}
