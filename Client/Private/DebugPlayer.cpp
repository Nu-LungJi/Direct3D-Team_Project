#include"pch.h"
#include "GameInstance.h"
#include "DebugPlayer.h"
#include "Collider.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "Resources.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "DbgLineRender.h"

NS_USING(Client)

CDebugPlayer::CDebugPlayer()
{
}

CDebugPlayer::~CDebugPlayer()
{
}

HRESULT CDebugPlayer::Initialize(void* pArg)
{
	auto		pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

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
	//	Desc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = {0.5f, 0.5f, 0.5f} });
	//	Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
	//	if (FAILED(AddComponentFromProto("PHYSX", "Prototype_Component_ComPxBoxCollider", "ComPxBoxCollider", &Desc, &m_pComPxBoxCollider)))
	//	{
	//		return E_FAIL;
	//	};
	//}

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad(CResPhysXMaterial::DESC{});
		Desc.tFilter = pDesc->tFilter;
		//Desc.fStepOffset = 0.f;
		//Desc.fSlopeLimit = 1.f;	
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pComCharacterController)))
		{
			return E_FAIL;
		};
	}

	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pComMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pComMoveIntent;
		Desc.pCharacterController = m_pComCharacterController;
		Desc.fGravity = -9.81f;
		Desc.fJumpVelocity = 5.f;
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pComCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	m_pComMoveIntent->RequestWarp(pDesc->vInitialPosition);

	return S_OK;
}
void CDebugPlayer::PriorityUpdate(E::_float fTimeDelta)
{
	auto* pPlayerCamera = CGameInstance::Get().GetActiveCamera("DebugPlayerCamera");
	if (!pPlayerCamera)
	{
		m_pComMoveIntent->ClearMoveIntent();
		return;
	}

	
	// 실제 콘텐츠에서는 BT가 이 입력 코드 대신 이동 의도만 Locomotion에 전달한다.
	_float fForwardIntent{};
	_float fRightIntent{};
	if (CGameInstance::Get().KeyPressing(DIK_W) || CGameInstance::Get().KeyPressing(DIK_UP))
		fForwardIntent += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_S) || CGameInstance::Get().KeyPressing(DIK_DOWN))
		fForwardIntent -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_D) || CGameInstance::Get().KeyPressing(DIK_RIGHT))
		fRightIntent += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_A) || CGameInstance::Get().KeyPressing(DIK_LEFT))
		fRightIntent -= 1.f;

	_float3 vCameraForward{};
	_float3 vCameraRight{};
	XMStoreFloat3(&vCameraForward, pPlayerCamera->GetTransform().GetState(STATE::LOOK));
	XMStoreFloat3(&vCameraRight, pPlayerCamera->GetTransform().GetState(STATE::RIGHT));
	vCameraForward.y = 0.f;
	vCameraRight.y = 0.f;

	const _float fForwardLengthSq =
		vCameraForward.x * vCameraForward.x + vCameraForward.z * vCameraForward.z;
	const _float fRightLengthSq =
		vCameraRight.x * vCameraRight.x + vCameraRight.z * vCameraRight.z;
	if (fForwardLengthSq > std::numeric_limits<_float>::epsilon())
	{
		const _float fInvLength = 1.f / std::sqrt(fForwardLengthSq);
		vCameraForward.x *= fInvLength;
		vCameraForward.z *= fInvLength;
	}
	if (fRightLengthSq > std::numeric_limits<_float>::epsilon())
	{
		const _float fInvLength = 1.f / std::sqrt(fRightLengthSq);
		vCameraRight.x *= fInvLength;
		vCameraRight.z *= fInvLength;
	}

	const _float3 vMoveDirection{
		vCameraForward.x * fForwardIntent + vCameraRight.x * fRightIntent,
		0.f,
		vCameraForward.z * fForwardIntent + vCameraRight.z * fRightIntent };

	if (vMoveDirection.x != 0.f || vMoveDirection.z != 0.f)
		m_pComMoveIntent->SetMoveIntent(vMoveDirection, 5.f);
	else
		m_pComMoveIntent->ClearMoveIntent();

	if (CGameInstance::Get().KeyDown(DIK_SPACE))
		m_pComMoveIntent->RequestJump();

	if (CGameInstance::Get().KeyDown(DIK_R))
	{
		m_pComCharacterController->SetPosition({ 5.f, 5.f, 5.f });
		m_pComCharacterMotor->SetVelocity({});
	}
}
void CDebugPlayer::FixedUpdate(_float fTimeDelta)
{
	m_pComCharacterMotor->FixedUpdate(fTimeDelta);
}



void CDebugPlayer::Update(E::_float fTimeDelta)
{
	for (auto iter = m_Projectiles.begin(); iter != m_Projectiles.end();)
	{
		auto* pProjectile = CGameInstance::Get().GetGameObjectByHandle(iter->hProjectile);
		if (!pProjectile)
		{
			iter = m_Projectiles.erase(iter);
			continue;
		}

		iter->fRemainingTime -= fTimeDelta;
		if (iter->fRemainingTime <= 0.f)
		{
			pProjectile->SetPendingDestroyCascade();
			iter = m_Projectiles.erase(iter);
			continue;
		}

		++iter;
	}
}

void CDebugPlayer::LateUpdate(E::_float fTimeDelta)
{
	//m_pComPhysX->UpdateSyncedDataToTransform(m_pComTransform);
	GetTransform().Update();

	// 플레이어 Transform을 먼저 확정한 뒤 같은 프레임의 카메라 View를 갱신한다.


	if (auto* pCamera = Cast<CDebugPlayerThirdPersonCamera>(CGameInstance::Get().GetActiveCamera("DebugPlayerCamera")))
	{
		pCamera->UpdateFollow();
	}

	// PhysX render buffer와 무관하게 현재 게임오브젝트 Transform을 즉시 시각화한다.
	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		const _float3 vPosition = GetTransform().GetPosition();

		pDbgLineRender->SetColor({ 1.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddCapsule(
			0.5f,
			1.f,
			XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));
		pDbgLineRender->AddCross(vPosition, 0.15f);

		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
}

HRESULT CDebugPlayer::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CDebugPlayer::OnWake()
{
}

void CDebugPlayer::OnSleep()
{
	int x = 0;
}

void CDebugPlayer::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestCharacter] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CDebugPlayer::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestCharacter] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CDebugPlayer::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestCharacter] Trigger Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CDebugPlayer::OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][TestCharacter] Trigger Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CDebugPlayer> CDebugPlayer::Create()
{
	auto pInstance = E::ToUPtr(new CDebugPlayer{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CDebugPlayer");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CDebugPlayer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CDebugPlayer{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CDebugPlayer");
		return nullptr;
	}

	return pInstance;
}
