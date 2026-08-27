#include "pch.h"
#include "AccioBall.h"

#include "AccioActivity_Base.h"
#include "ComConstantBuffer.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXMaterial.h"
#include "ResPhysXSphereGeometry.h"
#include "Resources.h"

NS_USING(Client)

CAccioBall::CAccioBall() = default;

CAccioBall::CAccioBall(const CAccioBall& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
	, m_eColor{ prototype.m_eColor }
{
}

HRESULT CAccioBall::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CAccioBall::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	// [LSY] 일반 렌더와 외곽선용 Depth 렌더를 모두 지원한다.
	m_RenderPassFlags =
		ETOUI(RENDERPASS::DEFAULT) |
		ETOUI(RENDERPASS::DEPTH);

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	GetTransform().SetScale(pDesc->vInitialScale);
	GetTransform().Update();
	m_vInitialPosition = pDesc->vInitialPosition;
	m_vInitialRotation = GetTransform().GetQuaternion();
	m_eColor = pDesc->eColor;
	SetRollingTuning(
		pDesc->fRollingTorque,
		pDesc->fMaxRollAngularSpeed);
	SetPullTuning(
		pDesc->fMaxPullAcceleration,
		pDesc->fMaxPullLinearSpeed,
		pDesc->fPullSlowRadius);
	m_fAutoSleepLinearSpeed = std::max(
		pDesc->fAutoSleepLinearSpeed, 0.f);
	m_fAutoSleepAngularSpeed = std::max(
		pDesc->fAutoSleepAngularSpeed, 0.f);
	m_fAutoSleepDelay = std::max(pDesc->fAutoSleepDelay, 0.f);

	const _float fScale = std::max({
		std::abs(pDesc->vInitialScale.x),
		std::abs(pDesc->vInitialScale.y),
		std::abs(pDesc->vInitialScale.z),
		0.001f });
	m_fSphereRadius = std::max(pDesc->fSphereRadius * fScale, 0.001f);

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject", &desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sResourceGroup;
		desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		desc.fMass = std::max(pDesc->fMass, 0.001f);
		desc.vPosition = pDesc->vInitialPosition;
		desc.vRotation = GetTransform().GetQuaternion();
		desc.bSendSleepNotifies = true;
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &desc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	{
		CResPhysXMaterial::DESC materialDesc{};
		materialDesc.fStaticFriction = std::max(
			pDesc->fStaticFriction,
			0.f);
		materialDesc.fDynamicFriction = std::max(
			pDesc->fDynamicFriction,
			0.f);
		materialDesc.fRestitution = std::clamp(
			pDesc->fRestitution,
			0.f,
			1.f);
		m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad(materialDesc);
		m_pResSphereGeometry = CResPhysXSphereGeometry::CreateAndLoad(
			{ .fRadius = m_fSphereRadius });

		CComPxSphereCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.pResSphereGeo = m_pResSphereGeometry;
		desc.bIsTrigger = false;
		desc.tFilter = pDesc->tFilter;
		if (!m_pResPhysXMaterial || !m_pResSphereGeometry ||
			FAILED(AddComponentFromProto(
				"PHYSX", "Prototype_Component_ComPxSphereCollider",
				"ComPxSphereCollider", &desc, &m_pComPxSphereCollider)))
		{
			return E_FAIL;
		}
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(true) ||
		!m_pComPxRigidBody->SetLinearDamping(pDesc->fLinearDamping) ||
		!m_pComPxRigidBody->SetAngularDamping(pDesc->fAngularDamping) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(2.f) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CAccioBall::FixedUpdate(_float fTimeDelta)
{
	if (m_hController != CHandle{})
	{
		auto* pController = CGameInstance::Get().GetGameObjectByHandle(
			m_hController);
		if (!pController || pController->GetPendingDestroy())
		{
			const CHandle hController = m_hController;
			ReleaseControl(hController);
		}
		else
		{
			// [LSY] FixedUpdate가 한 렌더 프레임에 여러 번 실행되어도 이전
			// LateUpdate Transform이 아닌 현재 PhysX Pose로 당김 방향을 계산한다.
			const _float3 vBallPosition = m_pComPxRigidBody ?
				m_pComPxRigidBody->GetPosition() : GetTransform().GetPosition();
			const _float3 vControllerPosition =
				pController->GetTransform().GetPosition();
			ApplyPullMotion({
				vControllerPosition.x - vBallPosition.x,
				0.f,
				vControllerPosition.z - vBallPosition.z
			});
			// [LSY] 제어 중에는 자동 수면 판정을 하지 않는다. 힘 적용과 같은
			// FixedUpdate에서 Sleep에 들어가면 당김이 끊기거나 떨릴 수 있다.
			return;
		}
	}

	if (!m_pComPxRigidBody || m_pComPxRigidBody->IsSleeping())
	{
		m_fAutoSleepElapsed = 0.f;
		return;
	}

	const _float3 linearVelocity = m_pComPxRigidBody->GetLinearVelocity();
	const _float3 angularVelocity = m_pComPxRigidBody->GetAngularVelocity();
	const _float linearSpeedSq = XMVectorGetX(XMVector3LengthSq(
		XMLoadFloat3(&linearVelocity)));
	const _float angularSpeedSq = XMVectorGetX(XMVector3LengthSq(
		XMLoadFloat3(&angularVelocity)));

	const _bool bBelowSleepSpeed =
		linearSpeedSq <= m_fAutoSleepLinearSpeed * m_fAutoSleepLinearSpeed &&
		angularSpeedSq <= m_fAutoSleepAngularSpeed * m_fAutoSleepAngularSpeed;
	if (!bBelowSleepSpeed)
	{
		m_fAutoSleepElapsed = 0.f;
		return;
	}

	m_fAutoSleepElapsed += std::max(fTimeDelta, 0.f);
	if (m_fAutoSleepElapsed < m_fAutoSleepDelay)
		return;

	// [LSY] 충돌 직후의 반발은 유지하고, 저속 미세 이동만 수면 상태로 끊는다.
	if (m_pComPxRigidBody->PutToSleep())
		m_bSettled = true;
	m_fAutoSleepElapsed = 0.f;
}

void CAccioBall::LateUpdate(_float)
{
	SyncRenderPoseFromRigidBody();
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		// [LSY] 인스턴싱 비활성화 시에도 MapMesh의 스텐실 정책을 유지한다.
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND_MAPMESH, this);
		return;
	}

	// [LSY] 움직이는 공은 정적 MapMesh 상주 데이터가 아니다. 매 프레임 갱신되는
	// Model Instance Manager 경로로 제출하고, 선택 외곽선의 Depth만 Render() 폴백을 사용한다.
	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		*GetTransform().GetCombinedWorldMatrix());
}

void CAccioBall::SyncRenderPoseFromRigidBody()
{
	if (!m_pComPxRigidBody)
		return;

	// [LSY] Sleep 직후 충돌로 다시 Wake 되는 프레임에는 Active Actor 기반
	// 동기화 캐시가 이전 Pose를 유지할 수 있으므로 실제 PhysX Pose를 사용한다.
	const _float3 vPosition = m_pComPxRigidBody->GetPosition();
	const _float4 vRotation = m_pComPxRigidBody->GetRotation();
	GetTransform().SetPosition(vPosition);
	GetTransform().SetQuaternion(vRotation);
}

HRESULT CAccioBall::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CAccioBall::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
	{
		return E_FAIL;
	}

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
	{
		return E_FAIL;
	}
	pContext->VSSetConstantBuffers(
		0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	if (ctx.pass == RENDERPASS::DEFAULT)
	{
		pContext->PSSetConstantBuffers(
			0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	if (ctx.pass == RENDERPASS::DEPTH)
	{
		pContext->PSSetShader(nullptr, nullptr, 0);
	}
	else
	{
		pContext->PSSetShader(
			m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	for (uint32_t meshIndex = 0; meshIndex < pModel->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset{};
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(
			mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		if (ctx.pass == RENDERPASS::DEFAULT)
		{
			m_pComModelInstance->Bind_Textures(pContext, meshIndex);
			m_pComModelInstance->Bind_Materials(
				pContext, { 1.f, 1.f, 1.f }, 0.f,
				{ 1.f, 1.f, 1.f }, 0.f, 1.f);
		}
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

void CAccioBall::OnWake()
{
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;
}

void CAccioBall::OnSleep()
{
	m_bSettled = true;
	m_fAutoSleepElapsed = 0.f;
}

_bool CAccioBall::ApplyImpulse(const _float3& vImpulse)
{
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;
	return m_pComPxRigidBody && m_pComPxRigidBody->AddImpulse(vImpulse);
}

_bool CAccioBall::ApplyTorque(const _float3& vTorque)
{
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;
	return m_pComPxRigidBody && m_pComPxRigidBody->AddTorque(vTorque);
}

_bool CAccioBall::ApplyRollingTorque(
	const _float3& vTorqueAxis)
{
	if (!m_pComPxRigidBody || m_fMaxRollAngularSpeed <= 0.f)
		return false;

	const _vector loadedAxis = XMLoadFloat3(&vTorqueAxis);
	if (XMVectorGetX(XMVector3LengthSq(loadedAxis)) <= FLT_EPSILON)
		return false;

	const _vector normalizedAxis = XMVector3Normalize(loadedAxis);
	const _float3 angularVelocity = m_pComPxRigidBody->GetAngularVelocity();
	const _float fAxisAngularSpeed = XMVectorGetX(XMVector3Dot(
		XMLoadFloat3(&angularVelocity), normalizedAxis));

	if (fAxisAngularSpeed >= m_fMaxRollAngularSpeed)
		return true;

	// [LSY] 제한 속도의 65%부터 토크를 부드럽게 줄여 급격한 속도 고정을 피한다.
	constexpr _float fFadeStartRatio = 0.65f;
	const _float fSpeedRatio = std::clamp(
		fAxisAngularSpeed / m_fMaxRollAngularSpeed,
		0.f,
		1.f);
	const _float fFadeRatio = std::clamp(
		(fSpeedRatio - fFadeStartRatio) / (1.f - fFadeStartRatio),
		0.f,
		1.f);
	const _float fSmoothFade = fFadeRatio * fFadeRatio *
		(3.f - 2.f * fFadeRatio);
	const _float fAppliedTorque = m_fRollingTorque * (1.f - fSmoothFade);

	_float3 normalizedTorqueAxis{};
	XMStoreFloat3(&normalizedTorqueAxis, normalizedAxis);
	return ApplyTorque({
		normalizedTorqueAxis.x * fAppliedTorque,
		normalizedTorqueAxis.y * fAppliedTorque,
		normalizedTorqueAxis.z * fAppliedTorque
	});
}

_bool CAccioBall::ApplyPullMotion(const _float3& vToTarget)
{
	if (!m_pComPxRigidBody)
		return false;
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;

	const _vector loadedToTarget = XMVectorSet(
		vToTarget.x,
		0.f,
		vToTarget.z,
		0.f);
	const _float fDistance = XMVectorGetX(XMVector3Length(loadedToTarget));
	if (fDistance <= FLT_EPSILON)
		return true;

	const _vector pullDirection = loadedToTarget / fDistance;
	const _vector torqueAxis = XMVector3Normalize(XMVector3Cross(
		XMVectorSet(0.f, 1.f, 0.f, 0.f),
		pullDirection));
	_float3 storedTorqueAxis{};
	XMStoreFloat3(&storedTorqueAxis, torqueAxis);
	const _bool bTorqueApplied = ApplyRollingTorque(storedTorqueAxis);

	_float fTargetSpeedRatio = 1.f;
	if (m_fPullSlowRadius > FLT_EPSILON)
	{
		const _float fDistanceRatio = std::clamp(
			fDistance / m_fPullSlowRadius,
			0.f,
			1.f);
		fTargetSpeedRatio = fDistanceRatio * fDistanceRatio *
			(3.f - 2.f * fDistanceRatio);
	}

	const _vector desiredVelocity = pullDirection *
		(m_fMaxPullLinearSpeed * fTargetSpeedRatio);
	const _float3 currentVelocity = m_pComPxRigidBody->GetLinearVelocity();
	const _vector horizontalVelocity = XMVectorSet(
		currentVelocity.x,
		0.f,
		currentVelocity.z,
		0.f);
	const _vector velocityError = desiredVelocity - horizontalVelocity;

	// [LSY] 목표 속도와의 차이를 가속도로 바꾸고 상한을 둔다.
	// 질량을 곱해 Force로 전달하므로 선택 중 질량이 바뀌어도 조작감은 유지된다.
	constexpr _float fVelocityResponse = 5.f;
	_vector acceleration = velocityError * fVelocityResponse;
	const _float fAccelerationLength = XMVectorGetX(
		XMVector3Length(acceleration));
	if (fAccelerationLength > m_fMaxPullAcceleration &&
		fAccelerationLength > FLT_EPSILON)
	{
		acceleration = XMVector3Normalize(acceleration) *
			m_fMaxPullAcceleration;
	}

	_float3 force{};
	XMStoreFloat3(
		&force,
		acceleration * std::max(m_pComPxRigidBody->GetMass(), 0.001f));
	const _bool bForceApplied = m_pComPxRigidBody->AddForce(force);
	return bTorqueApplied && bForceApplied;
}

_bool CAccioBall::CanAcquireControl(const CHandle& hController) const
{
	if (hController == CHandle{} || GetPendingDestroy())
		return false;

	if (const auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity))
	{
		if (!pActivity->CanControlBall(hController, GetHandle()))
			return false;
	}

	if (m_hController == CHandle{} || m_hController == hController)
		return true;

	const auto* pCurrentController = CGameInstance::Get().
		GetGameObjectByHandle(m_hController);
	return !pCurrentController || pCurrentController->GetPendingDestroy();
}

_bool CAccioBall::TryAcquireControl(const CHandle& hController)
{
	if (!CanAcquireControl(hController))
		return false;

	if (m_hController != CHandle{} && m_hController != hController)
		m_hController = CHandle{};

	m_hController = hController;
	if (m_pComPxRigidBody)
		m_pComPxRigidBody->WakeUp();
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;

	if (auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity))
	{
		pActivity->NotifyBallControlAcquired(hController, GetHandle());
	}
	return true;
}

_bool CAccioBall::ReleaseControl(const CHandle& hController)
{
	if (hController == CHandle{} || m_hController != hController)
		return false;

	m_hController = CHandle{};
	if (auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(m_hActivity))
	{
		pActivity->NotifyBallControlReleased(hController, GetHandle());
	}
	return true;
}

_bool CAccioBall::IsControlledBy(const CHandle& hController) const
{
	return hController != CHandle{} && m_hController == hController;
}

void CAccioBall::SetRollingTuning(
	_float fRollingTorque,
	_float fMaxRollAngularSpeed)
{
	m_fRollingTorque = std::max(fRollingTorque, 0.f);
	m_fMaxRollAngularSpeed = std::max(fMaxRollAngularSpeed, 0.1f);
}

void CAccioBall::SetPullTuning(
	_float fMaxPullAcceleration,
	_float fMaxPullLinearSpeed,
	_float fPullSlowRadius)
{
	m_fMaxPullAcceleration = std::max(fMaxPullAcceleration, 0.f);
	m_fMaxPullLinearSpeed = std::max(fMaxPullLinearSpeed, 0.f);
	m_fPullSlowRadius = std::max(fPullSlowRadius, 0.f);
}

_bool CAccioBall::SetMotionTuning(
	_float fMass,
	_float fLinearDamping,
	_float fAngularDamping)
{
	return m_pComPxRigidBody &&
		m_pComPxRigidBody->SetMass(std::max(fMass, 0.001f)) &&
		m_pComPxRigidBody->SetLinearDamping(std::max(fLinearDamping, 0.f)) &&
		m_pComPxRigidBody->SetAngularDamping(std::max(fAngularDamping, 0.f));
}

_bool CAccioBall::ResetToInitialPose()
{
	m_hController = CHandle{};
	m_bSettled = false;
	m_fAutoSleepElapsed = 0.f;
	if (!m_pComPxRigidBody ||
		!m_pComPxRigidBody->SetPose(m_vInitialPosition, m_vInitialRotation) ||
		!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return false;
	}

	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetQuaternion(m_vInitialRotation);
	GetTransform().Update();
	return true;
}

UPtr<CAccioBall> CAccioBall::Create()
{
	auto pInstance = ToUPtr(new CAccioBall{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioBall::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
