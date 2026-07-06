#include "pch.h"
#include "ResPhysXSphereGeometry.h"
#include "GameInstance.h"

#ifdef _DEBUG
// 라이브러리 설정 전후로 매크로 잠시 해제
#undef new
#endif

#include "PxPhysicsAPI.h"

#ifdef _DEBUG
#define new DBG_NEW
#endif


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

SPtr<CResPhysXSphereGeometry> CResPhysXSphereGeometry::Create(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXSphereGeometry{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}
