#include"pch.h"
#include "GameInstance.h"
#include "TestCharacter.h"
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
#include "ComPxCharacterController.h"

NS_USING(Client)

CTestCharacter::CTestCharacter()
{
}

CTestCharacter::~CTestCharacter()
{
}

HRESULT CTestCharacter::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	//{
	//	CComPxRigidBody::DESC Desc{};
	//	Desc.eType = CComPxRigidBody::TYPE::STATIC;
	//	if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody", &Desc, &m_pComPxRigidBody)))
	//	{
	//		return E_FAIL;
	//	};
	//}

	//{
	//	CComPxBoxCollider::DESC Desc{};
	//	Desc.pComPxRigidBody = m_pComPxRigidBody;
	//	Desc.pResBoxGeo = CResPhysXBoxGeometry::Create({ .vHalfExtents = {0.5f, 0.5f, 0.5f} });
	//	Desc.pResMaterial = CResPhysXMaterial::Create({});
	//	if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
	//	{
	//		return E_FAIL;
	//	};
	//}

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::Create(CResPhysXMaterial::DESC{});
		//Desc.fStepOffset = 0.f;
		//Desc.fSlopeLimit = 1.f;	
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pComCharacterController)))
		{
			return E_FAIL;
		};
	}

	return S_OK;
}
void CTestCharacter::PriorityUpdate(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyDown(DIK_R))
	{
		m_pComCharacterController->SetPosition({ 5.f, 5.f, 5.f });
	}
}
void CTestCharacter::FixedUpdate(_float fTimeDelta)
{
	// 1. 이동 방향 계산 (입력 처리)
	XMFLOAT3 vDir = { 0.f, 0.f, 0.f };
	if (CGameInstance::Get().KeyPressing(DIK_UP)) vDir.z += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_DOWN)) vDir.z -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_LEFT)) vDir.x -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_RIGHT)) vDir.x += 1.f;

	// 2. 이동 벡터 생성 (가로)
	XMFLOAT3 vMove = { 0.f, 0.f, 0.f };
	if (vDir.x != 0.f || vDir.z != 0.f)
	{
		float fLength = sqrtf(vDir.x * vDir.x + vDir.z * vDir.z);
		vDir.x /= fLength;
		vDir.z /= fLength;

		float fSpeed = 5.0f;
		vMove.x = vDir.x * fSpeed * fTimeDelta;
		vMove.z = vDir.z * fSpeed * fTimeDelta;
	}

	// 3. 중력 적용 (세로)
	// 입력이 없어도 중력은 항상 적용되어야 함!
	float fGravity = -9.81f; // 중력 가속도
	vMove.y = fGravity * fTimeDelta;

	// 4. 컨트롤러 이동 (입력이 없어도 vMove.y 때문에 아래로 이동 시도)
	// PhysX가 바닥(지형)을 감지하면 여기서 자동으로 멈추게 됨
	m_pComCharacterController->Move(vMove, fTimeDelta);

}



void CTestCharacter::Update(E::_float fTimeDelta)
{
	
}

void CTestCharacter::LateUpdate(E::_float fTimeDelta)
{
	//m_pComPhysX->UpdateSyncedDataToTransform(m_pComTransform);
	GetTransform().Update();
	//CGameInstance::Get().AddColliderGroup("Coll_TestPhysX", m_pComCollider->Get());
	//m_pComCollider->Get()->Transform(GetTransform().GetLoadedCombinedWorldMatrix());
}

HRESULT CTestCharacter::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CTestCharacter::OnWake()
{
}

void CTestCharacter::OnSleep()
{
	int x = 0;
}

void CTestCharacter::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
}

void CTestCharacter::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
}

void CTestCharacter::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}

void CTestCharacter::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}

E::UPtr<CTestCharacter> CTestCharacter::Create()
{
	auto pInstance = E::ToUPtr(new CTestCharacter{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestCharacter");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestCharacter::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestCharacter{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestCharacter");
		return nullptr;
	}

	return pInstance;
}
