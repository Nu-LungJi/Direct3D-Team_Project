#include "pch.h"
#include "ResPhysXBoxGeometry.h"
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

CResPhysXBoxGeometry::CResPhysXBoxGeometry(const _string& sPath)
	: CResPhysXGeometry{ sPath }
{
}

CResPhysXBoxGeometry::~CResPhysXBoxGeometry()
{
}

HRESULT CResPhysXBoxGeometry::Load(const std::any& arg)
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
		m_pBoxGeometry = std::make_unique<PxBoxGeometry>(PxBoxGeometry(
			pDesc->vHalfExtents.x,
			pDesc->vHalfExtents.y,
			pDesc->vHalfExtents.z));

		m_eState = STATE::LOADED;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXBoxGeometry::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXBoxGeometry> CResPhysXBoxGeometry::Create()
{
	return ToSPtr(new CResPhysXBoxGeometry{ "" });
}

SPtr<CResPhysXBoxGeometry> CResPhysXBoxGeometry::Create(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXBoxGeometry{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}
