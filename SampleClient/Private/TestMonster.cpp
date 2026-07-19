#include "pch.h"
#include "TestMonster.h"

#include "GameInstance.h"
#include "ComPxCharacterController.h"
#include "ComLocomotion.h"
#include "ComCharacterMotor.h"
#include "Resources.h"
#include "DbgLineRender.h"
#include "TestPhysXBall.h"

NS_USING(Client)

CTestMonster::CTestMonster() = default;
CTestMonster::~CTestMonster() = default;

HRESULT CTestMonster::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad(CResPhysXMaterial::DESC{});
		Desc.vPosition = pDesc->vInitialPos;
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}

	{
		CComLocomotion::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComLocomotion,
			"ComLocomotion", &Desc, &m_pLocomotion)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pLocomotion = m_pLocomotion;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	GetTransform().SetPosition(m_pCharacterController->GetPosition());
	GetTransform().Update();
	m_hTarget = pDesc->hTarget;
	return S_OK;
}

void CTestMonster::PriorityUpdate(_float fTimeDelta)
{
	TryFireAtTarget(fTimeDelta);

	m_fDirectionChangeTimer -= fTimeDelta;
	if (m_fDirectionChangeTimer <= 0.f)
	{
		const _float fAngle = Randf(0.f, XM_2PI);
		m_vMoveDirection = { std::sin(fAngle), 0.f, std::cos(fAngle) };
		m_fDirectionChangeTimer = Randf(1.f, 3.f);
	}

	m_pLocomotion->SetMoveIntent(m_vMoveDirection, 2.f);
}

void CTestMonster::TryFireAtTarget(_float fTimeDelta)
{
	m_fAttackCooldown = std::max(0.f, m_fAttackCooldown - fTimeDelta);

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return;

	const _float3 vPosition = m_pCharacterController->GetPosition();
	const _float3 vTargetPosition = pTarget->GetTransform().GetPosition();
	_float3 vToTarget{
		vTargetPosition.x - vPosition.x,
		vTargetPosition.y - vPosition.y,
		vTargetPosition.z - vPosition.z };
	const _float fDistanceSq =
		vToTarget.x * vToTarget.x + vToTarget.y * vToTarget.y + vToTarget.z * vToTarget.z;
	constexpr _float fAttackRange = 10.f;
	if (fDistanceSq > fAttackRange * fAttackRange ||
		fDistanceSq <= std::numeric_limits<_float>::epsilon() || m_fAttackCooldown > 0.f)
	{
		return;
	}

	const _float fInvDistance = 1.f / std::sqrt(fDistanceSq);
	vToTarget.x *= fInvDistance;
	vToTarget.y *= fInvDistance;
	vToTarget.z *= fInvDistance;

	constexpr _float fRadius = 0.12f;
	constexpr _float fSpawnDistance = 1.f;
	constexpr _float fProjectileSpeed = 8.f;
	CTestPhysXBall::DESC Desc{};
	Desc.sObjectTag = "TestEnemyProjectileSphere";
	Desc.vInitialPos = {
		vPosition.x + vToTarget.x * fSpawnDistance,
		vPosition.y + vToTarget.y * fSpawnDistance,
		vPosition.z + vToTarget.z * fSpawnDistance };
	Desc.vInitialVelocity = {
		vToTarget.x * fProjectileSpeed,
		vToTarget.y * fProjectileSpeed,
		vToTarget.z * fProjectileSpeed };
	Desc.fRadius = fRadius;
	Desc.fLifetime = 5.f;
	Desc.bUseGravity = true;
	Desc.fRestitution = 0.8f;
	Desc.tFilter.iLayer = ETOUI(COLLISION_LAYER::ENEMY_PROJECTILE);
	Desc.tFilter.iSimulationMask =
		ETOUI(COLLISION_LAYER::WORLD_STATIC) |
		ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
		ETOUI(COLLISION_LAYER::PLAYER_BODY);
	Desc.tFilter.iQueryMask = PX_ALL_LAYERS;

	if (CGameInstance::Get().AddGameObjectToLayer(
		"SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXBall", "02_PROJECTILES", &Desc))
	{
		m_fAttackCooldown = 0.5f;
	}
}

void CTestMonster::FixedUpdate(_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CTestMonster::LateUpdate(_float)
{
	GetTransform().SetPosition(m_pCharacterController->GetPosition());
	GetTransform().Update();

	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		const _float3 vPosition = GetTransform().GetPosition();

		pDbgLineRender->SetColor({ 1.f, 0.f, 0.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddCapsule(
			0.5f,
			1.f,
			XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
}

HRESULT CTestMonster::Render(ID3D11DeviceContext*, const RENDER_CTX&)
{
	return S_OK;
}

void CTestMonster::OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA&)
{
	DEBUG_LOG_STR(std::string("[PX][TestMonster] Collision Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
	TryDestroyByProjectile(pObj);
}

void CTestMonster::OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit)
{
	TryDestroyByProjectile(tHit.pGameObject);
}

void CTestMonster::TryDestroyByProjectile(CGameObject* pObject)
{
	if (!pObject)
		return;

	const std::string sObjectTag{ pObject->GetObjectTag() };
	if (sObjectTag.starts_with("TestPlayerProjectile"))
		SetPendingDestroyCascade();
}

void CTestMonster::OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA&)
{
	DEBUG_LOG_STR(std::string("[PX][TestMonster] Collision Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

UPtr<CTestMonster> CTestMonster::Create()
{
	auto pInstance = ToUPtr(new CTestMonster{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CTestMonster::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CTestMonster{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
