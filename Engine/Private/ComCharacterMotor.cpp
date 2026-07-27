#include "pch.h"
#include "ComCharacterMotor.h"

#include "ComCharacterMoveIntent.h"
#include "GameObject.h"

NS_USING(Engine)

CComCharacterMotor::CComCharacterMotor()
{
}

CComCharacterMotor::CComCharacterMotor(const CComCharacterMotor& rhs)
	: CComponent{ rhs }
	, m_fGravity{ rhs.m_fGravity }
	, m_fJumpVelocity{ rhs.m_fJumpVelocity }
	, m_fMinMoveDistance{ rhs.m_fMinMoveDistance }
	, m_bUseGravity{ rhs.m_bUseGravity }
	, m_bSyncTransform{ rhs.m_bSyncTransform }
{
}

CComCharacterMotor::~CComCharacterMotor()
{
}

HRESULT CComCharacterMotor::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || !pDesc->pMoveIntent || !pDesc->pCharacterController)
		return E_FAIL;

	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	if (pDesc->pMoveIntent->GetGameObject() != m_pGameObject ||
		pDesc->pCharacterController->GetGameObject() != m_pGameObject)
		return E_FAIL;

	m_pMoveIntent = pDesc->pMoveIntent;
	m_pCharacterController = pDesc->pCharacterController;
	m_fGravity = pDesc->fGravity;
	m_fJumpVelocity = pDesc->fJumpVelocity;
	m_fMinMoveDistance = std::max(0.f, pDesc->fMinMoveDistance);
	m_bUseGravity = pDesc->bUseGravity;
	m_bSyncTransform = pDesc->bSyncTransform;
	m_bGrounded = m_pCharacterController->IsGrounded();

	return S_OK;
}

void CComCharacterMotor::FixedUpdate(_float fFixedTimeDelta)
{
	if (!m_pMoveIntent || !m_pCharacterController || fFixedTimeDelta <= 0.f)
		return;

	_float3 vWarpPosition{};
	if (m_pMoveIntent->ConsumeWarpRequest(vWarpPosition))
	{
		m_pMoveIntent->ClearExternalDisplacement();
		m_pCharacterController->SetPosition(vWarpPosition);
		m_vVelocity = {};
		m_bGrounded = false;
		m_eLastCollisionFlag = PX_CCT_COLLISION_FLAG::NONE;

		if (m_pGameObject)
			m_pGameObject->GetTransform().SetPosition(vWarpPosition);

		return;
	}

	const CComCharacterMoveIntent::OUTPUT& tOutput = m_pMoveIntent->GetOutput();
	if (tOutput.bMoveRequested)
	{
		m_vVelocity.x = tOutput.vMoveDirection.x * tOutput.fMoveSpeed;
		m_vVelocity.z = tOutput.vMoveDirection.z * tOutput.fMoveSpeed;
	}
	else
	{
		m_vVelocity.x = 0.f;
		m_vVelocity.z = 0.f;
	}

	if (m_pMoveIntent->HasJumpRequest())
	{
		if (m_bGrounded)
			m_vVelocity.y = m_fJumpVelocity;

		m_pMoveIntent->ConsumeJumpRequest();
	}

	if (m_bUseGravity)
		m_vVelocity.y += m_fGravity * fFixedTimeDelta;

	_float3 vDisplacement{
		m_vVelocity.x * fFixedTimeDelta,
		m_vVelocity.y * fFixedTimeDelta,
		m_vVelocity.z * fFixedTimeDelta };

	_float3 vExternalDisplacement{};
	if (m_pMoveIntent->ConsumeExternalDisplacement(vExternalDisplacement))
	{
		vDisplacement.x += vExternalDisplacement.x;
		vDisplacement.y += vExternalDisplacement.y;
		vDisplacement.z += vExternalDisplacement.z;
	}

	m_eLastCollisionFlag = m_pCharacterController->Move(
		vDisplacement,
		fFixedTimeDelta,
		m_fMinMoveDistance);

	m_bGrounded = m_pCharacterController->IsGrounded();
	if (m_bGrounded && m_vVelocity.y < 0.f)
		m_vVelocity.y = 0.f;
	if (m_pCharacterController->IsCollidingUp() && m_vVelocity.y > 0.f)
		m_vVelocity.y = 0.f;

	if (m_bSyncTransform && m_pGameObject)
		m_pGameObject->GetTransform().SetPosition(m_pCharacterController->GetPosition());

	if (tOutput.bFacingRequested && m_pGameObject)
	{
		auto& Transform = m_pGameObject->GetTransform();
		const _float fTargetYaw = std::atan2(
			tOutput.vFacingDirection.x,
			tOutput.vFacingDirection.z);
		const _vector vTargetQuaternion = XMQuaternionRotationAxis(
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			fTargetYaw);

		if (tOutput.bImmediateFacing)
		{
			Transform.SetQuaternion(vTargetQuaternion);
		}
		else
		{
			const _vector vCurrentQuaternion = Transform.GetLoadedQuaternion();
			_float fDot = std::abs(XMVectorGetX(XMQuaternionDot(
				vCurrentQuaternion, vTargetQuaternion)));
			fDot = std::clamp(fDot, 0.f, 1.f);
			const _float fAngle = 2.f * std::acos(fDot);
			const _float fMaxStep = XMConvertToRadians(tOutput.fTurnSpeed) * fFixedTimeDelta;
			const _float fRatio = fAngle <= std::numeric_limits<_float>::epsilon()
				? 1.f
				: std::min(1.f, fMaxStep / fAngle);

			Transform.SetQuaternion(XMQuaternionSlerp(
				vCurrentQuaternion, vTargetQuaternion, fRatio));
		}
	}
}

void CComCharacterMotor::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Text("Velocity: %.3f, %.3f, %.3f", m_vVelocity.x, m_vVelocity.y, m_vVelocity.z);
	ImGui::Text("Grounded: %s", m_bGrounded ? "true" : "false");
	ImGui::DragFloat("Gravity", &m_fGravity, 0.1f);
	ImGui::DragFloat("Jump Velocity", &m_fJumpVelocity, 0.1f, 0.f);
	ImGui::Checkbox("Use Gravity", &m_bUseGravity);
	ImGui::Checkbox("Sync Transform", &m_bSyncTransform);
}

UPtr<CComCharacterMotor> CComCharacterMotor::Create()
{
	auto pInstance = ToUPtr(new CComCharacterMotor{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComCharacterMotor");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComCharacterMotor::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComCharacterMotor{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComCharacterMotor");
		return nullptr;
	}

	return pInstance;
}

void CComCharacterMotor::Free()
{
	m_pMoveIntent = nullptr;
	m_pCharacterController = nullptr;
	CComponent::Free();
}
