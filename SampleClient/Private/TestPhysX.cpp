#include"pch.h"
#include "GameInstance.h"
#include "TestPhysX.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "TestPhysXBox.h"
#include "TestPhysXBall.h"
#include "TestPhysXCapsule.h"
#include "Resources.h"
#include "TestPhysXTerrain.h"

NS_USING(Client)

CTestPhysX::CTestPhysX()
{
}

CTestPhysX::~CTestPhysX()
{
}

HRESULT CTestPhysX::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::STATIC;
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxBoxCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResBoxGeo = CResPhysXBoxGeometry::Create({ .vHalfExtents = {0.5f, 0.5f, 0.5f} });
		Desc.pResMaterial = CResPhysXMaterial::Create({});
		if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		};
	}

    return S_OK;
}

void CTestPhysX::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPhysX::Update(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		// spawn
		auto pos = CGameInstance::Get().GetActiveCamera()->GetTransform().GetPosition();
		
		if (Randf(0.f, 1.f) < 0.5f)
		{
			{
				//"SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox"
				CTestPhysXBall::DESC Desc{
					.vInitialPos = pos
				};
				Desc.sObjectTag = "TestPhysXBall";
				if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBall",
					"00_OBJECTS", &Desc)))
				{
					//return E_FAIL;
				}
			}
		}
		else if (Randf(0.f, 1.f) < 0.5f)
		{
			
			{
				//"SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox"
				CTestPhysXCapsule::DESC Desc{
					.vInitialPos = pos
				};
				Desc.sObjectTag = "TestPhysXCapsule";
				if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXCapsule",
					"00_OBJECTS", &Desc)))
				{
					//return E_FAIL;
				}
			}
		}
		else
		{
			{
				//"SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox"
				CTestPhysXBox::DESC Desc{
					.vInitialPos = pos
				};
				Desc.sObjectTag = "TestPhysXBox";
				if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox",
					"00_OBJECTS", &Desc)))
				{
					//return E_FAIL;
				}
			}
		}
	}

	if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		if (auto pCam = CGameInstance::Get().GetActiveCamera())
		{
			const auto& [ori, Dir] = pCam->GetRay();
			if(false)
			{
				PHYSIX_RAYCAST_RESULT outResult;
				if (CGameInstance::Get().PxRayCast(ori, Dir, 10.f, outResult))
				{
					if (!outResult.pGameObject->IsA(CTestPhysXTerrain::StaticType)
						&& !outResult.pGameObject->IsA(CTestPhysX::StaticType))
					{
						outResult.pGameObject->SetPendingDestroyCascade();
					}
				}
			}

			{
				std::vector< PHYSIX_RAYCAST_RESULT> vecOutResult{};
				if (CGameInstance::Get().PxRayCastMultiple(ori, Dir, 10.f, vecOutResult))
				{
					for (auto& result : vecOutResult)
					{
						if (!result.pGameObject->IsA(CTestPhysXTerrain::StaticType)
							&& !result.pGameObject->IsA(CTestPhysX::StaticType))
						{
							result.pGameObject->SetPendingDestroyCascade();
						}
					}
				}
			}
		}
	}
}

void CTestPhysX::LateUpdate(E::_float fTimeDelta)
{
	//m_pComPhysX->UpdateSyncedDataToTransform(m_pComTransform);
	GetTransform().Update();
	//CGameInstance::Get().AddColliderGroup("Coll_TestPhysX", m_pComCollider->Get());
	//m_pComCollider->Get()->Transform(GetTransform().GetLoadedCombinedWorldMatrix());
}

HRESULT CTestPhysX::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    return S_OK;
}

void CTestPhysX::OnWake()
{
}

void CTestPhysX::OnSleep()
{
	int x = 0;
}

void CTestPhysX::OnCollisionEnter(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info)
{
}

void CTestPhysX::OnCollisionExit(CGameObject* pObj, const PHYSIX_ON_COLLISION_DATA& info)
{
}

void CTestPhysX::OnTriggerEnter(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info)
{
}

void CTestPhysX::OnTriggerExit(CGameObject* pObj, const PHYSIX_ON_TRIGGER_DATA& info)
{
}

E::UPtr<CTestPhysX> CTestPhysX::Create()
{
	auto pInstance = E::ToUPtr(new CTestPhysX{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPhysX");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPhysX::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPhysX{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPhysX");
		return nullptr;
	}

	return pInstance;
}
