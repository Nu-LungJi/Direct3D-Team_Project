#include "pch.h"
#include "ResPhysXCapsuleGeometry.h"
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

SPtr<CResPhysXCapsuleGeometry> CResPhysXCapsuleGeometry::Create(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXCapsuleGeometry{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}
