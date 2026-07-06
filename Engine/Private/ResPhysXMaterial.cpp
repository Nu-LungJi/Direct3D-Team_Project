#include "pch.h"
#include "ResPhysXMaterial.h"
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

CResPhysXMaterial::CResPhysXMaterial(const _string& sPath)
	: CResource{ sPath }
{
}

CResPhysXMaterial::~CResPhysXMaterial()
{
}

HRESULT CResPhysXMaterial::Load(const std::any& arg)
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
		if (!physics)
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		m_pMaterial = physics->createMaterial(
			desc->fStaticFriction,
			desc->fDynamicFriction,
			desc->fRestitution
		);

		if (!m_pMaterial)
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		m_pMaterial->setFrictionCombineMode(
			static_cast<physx::PxCombineMode::Enum>(desc->eFrictionCombine)
		);

		m_pMaterial->setRestitutionCombineMode(
			static_cast<physx::PxCombineMode::Enum>(desc->eRestitutionCombine)
		);
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResPhysXMaterial::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResPhysXMaterial> CResPhysXMaterial::Create()
{
	return ToSPtr(new CResPhysXMaterial{ "" });
}

SPtr<CResPhysXMaterial> CResPhysXMaterial::Create(const DESC& desc)
{
	auto pInstance = ToSPtr(new CResPhysXMaterial{ "" });;
	if (FAILED(pInstance->Load(desc)))
	{
		return nullptr;
	}
	return pInstance;
}

void CResPhysXMaterial::Free()
{
	if (m_pMaterial)
	{
		m_pMaterial->release();
		m_pMaterial = nullptr;
	}
	CResource::Free();
}
