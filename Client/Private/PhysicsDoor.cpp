#include "pch.h"
#include "PhysicsDoor.h"

#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxBoxCollider.h"
#include "ComPxD6Joint.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"
#include "Player.h"

NS_USING(Client)

CPhysicsDoor::CPhysicsDoor() = default;

CPhysicsDoor::CPhysicsDoor(const CPhysicsDoor& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CPhysicsDoor::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) ||
		FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CPhysicsDoor::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		pDesc->vHalfExtents.x <= 0.f ||
		pDesc->vHalfExtents.y <= 0.f ||
		pDesc->vHalfExtents.z <= 0.f ||
		pDesc->fMass <= 0.f ||
		pDesc->fAngularDamping < 0.f ||
		pDesc->fTwistDriveStiffness < 0.f ||
		pDesc->fTwistDriveDamping < 0.f ||
		pDesc->fTwistDriveForceLimit < 0.f ||
		pDesc->fCCTPushForce < 0.f ||
		pDesc->fHingeBlockerHalfWidth <= 0.f ||
		pDesc->fHingeBlockerDepthPadding < 0.f ||
		pDesc->fPassageBarrierHalfDepth <= 0.f ||
		pDesc->fPassageOpenAngleDegrees <= 0.f ||
		pDesc->vPassageTriggerHalfExtents.x <= 0.f ||
		pDesc->vPassageTriggerHalfExtents.y <= 0.f ||
		pDesc->vPassageTriggerHalfExtents.z <= 0.f ||
		pDesc->fLowerLimitDegrees >= pDesc->fUpperLimitDegrees ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	m_vHalfExtents = pDesc->vHalfExtents;
	m_vInitialPosition = pDesc->vInitialPosition;
	m_fLowerLimitDegrees = pDesc->fLowerLimitDegrees;
	m_fUpperLimitDegrees = pDesc->fUpperLimitDegrees;
	m_fTwistDriveStiffness = pDesc->fTwistDriveStiffness;
	m_fTwistDriveDamping = pDesc->fTwistDriveDamping;
	m_fTwistDriveForceLimit = pDesc->fTwistDriveForceLimit;
	m_fCCTPushForce = pDesc->fCCTPushForce;
	m_fPassageOpenAngleDegrees = pDesc->fPassageOpenAngleDegrees;
	m_eHingeSide = pDesc->eHingeSide;
	m_vPassageTriggerHalfExtents = pDesc->vPassageTriggerHalfExtents;
	m_vPassageBarrierHalfExtents = {
		m_vHalfExtents.x,
		m_vHalfExtents.y,
		pDesc->fPassageBarrierHalfDepth
	};

	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_vInitialRotation = GetTransform().GetQuaternion();
	GetTransform().Update();

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = {
			TAG_RES_GRP_PERMANENT_BUFFER,
			TAG_RES_CBUFFER_OBJECT
		};
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject",
			&desc,
			&m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sModelResourceGroup;
		desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_StaticModelInstance",
			"ComModelInstance",
			&desc,
			&m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		desc.fMass = pDesc->fMass;
		desc.vPosition = m_vInitialPosition;
		desc.vRotation = m_vInitialRotation;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody",
			&desc,
			&m_pComPxDoorRigidBody)))
		{
			return E_FAIL;
		}
	}

	{
		auto pMaterial = CResPhysXMaterial::CreateAndLoad({
			.fStaticFriction = 0.7f,
			.fDynamicFriction = 0.6f,
			.fRestitution = 0.05f
		});
		auto pGeometry = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = m_vHalfExtents
		});

		CComPxBoxCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxDoorRigidBody;
		desc.pResMaterial = pMaterial;
		desc.pResBoxGeo = pGeometry;
		desc.tFilter = pDesc->tDoorFilter;
		desc.iShapeSubIndex = ETOUI(SHAPE_SUB_INDEX::DOOR_LEAF);
		if (!pMaterial ||
			!pGeometry ||
			FAILED(AddComponentFromProto(
				ES_EngineProtoMajorType::PHYSX,
				ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
				"ComPxBoxCollider",
				&desc,
				&m_pComPxDoorCollider)))
		{
			return E_FAIL;
		}
	}

	if (!CreateHingeBlocker(*pDesc))
		return E_FAIL;
	if (!CreatePassageTrigger(*pDesc))
		return E_FAIL;
	if (!CreatePassageBarrier(*pDesc))
		return E_FAIL;

	if (!m_pComPxDoorRigidBody->SetGravityEnabled(false) ||
		!m_pComPxDoorRigidBody->SetAngularDamping(
			std::max(pDesc->fAngularDamping, 0.f)) ||
		!m_pComPxDoorRigidBody->SetMaxDepenetrationVelocity(1.5f) ||
		!m_pComPxDoorRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	return S_OK;
}

_float3 CPhysicsDoor::CalculateHingeWorldPosition(
	const _float3& vDoorPosition,
	const _float4& vDoorRotation) const
{
	const _float fHingeX = m_eHingeSide == HINGE_SIDE::LEFT
		? -m_vHalfExtents.x
		: m_vHalfExtents.x;
	const _float3 vLocalHingePosition{ fHingeX, 0.f, 0.f };
	const _matrix matDoorRotation = XMMatrixRotationQuaternion(
		XMLoadFloat4(&vDoorRotation));

	_float3 vWorldHingePosition{};
	XMStoreFloat3(
		&vWorldHingePosition,
		XMVector3TransformCoord(
			XMLoadFloat3(&vLocalHingePosition),
			matDoorRotation) +
		XMLoadFloat3(&vDoorPosition));
	return vWorldHingePosition;
}

_bool CPhysicsDoor::IsPlayerPassageTriggerEvent(
	CGameObject* pGameObject,
	const PX_ON_TRIGGER_DATA& tData) const
{
	return tData.bSelfIsTrigger &&
		tData.iSelfShapeSubIndex == ETOUI(SHAPE_SUB_INDEX::PASSAGE_TRIGGER) &&
		pGameObject &&
		pGameObject->Is<CPlayer>();
}

_bool CPhysicsDoor::CreateHingeBlocker(const DESC& desc)
{
	m_vHingeWorldPosition = CalculateHingeWorldPosition(
		m_vInitialPosition,
		m_vInitialRotation);
	m_vHingeBlockerHalfExtents = {
		desc.fHingeBlockerHalfWidth,
		m_vHalfExtents.y,
		m_vHalfExtents.z + desc.fHingeBlockerDepthPadding
	};

	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::STATIC;
		rigidBodyDesc.vPosition = m_vHingeWorldPosition;
		rigidBodyDesc.vRotation = m_vInitialRotation;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody_DoorHingeBlocker",
			&rigidBodyDesc,
			&m_pComPxHingeBlockerRigidBody)))
		{
			return false;
		}
	}

	auto pMaterial = CResPhysXMaterial::CreateAndLoad({
		.fStaticFriction = 0.7f,
		.fDynamicFriction = 0.6f,
		.fRestitution = 0.f
	});
	auto pGeometry = CResPhysXBoxGeometry::CreateAndLoad({
		.vHalfExtents = m_vHingeBlockerHalfExtents
	});

	CComPxBoxCollider::DESC colliderDesc{};
	colliderDesc.pComPxRigidBody = m_pComPxHingeBlockerRigidBody;
	colliderDesc.pResMaterial = pMaterial;
	colliderDesc.pResBoxGeo = pGeometry;
	colliderDesc.tFilter = desc.tCCTBlockerFilter;
	colliderDesc.iShapeSubIndex = ETOUI(SHAPE_SUB_INDEX::HINGE_BLOCKER);
	if (!pMaterial || !pGeometry ||
		FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
			"ComPxBoxCollider_DoorHingeBlocker",
			&colliderDesc,
			&m_pComPxHingeBlockerCollider)))
	{
		return false;
	}

	return true;
}

_bool CPhysicsDoor::CreatePassageTrigger(const DESC& desc)
{
	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::STATIC;
		rigidBodyDesc.vPosition = m_vInitialPosition;
		rigidBodyDesc.vRotation = m_vInitialRotation;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody_DoorPassageTrigger",
			&rigidBodyDesc,
			&m_pComPxPassageTriggerRigidBody)))
		{
			return false;
		}
	}

	auto pMaterial = CResPhysXMaterial::CreateAndLoad({});
	auto pGeometry = CResPhysXBoxGeometry::CreateAndLoad({
		.vHalfExtents = m_vPassageTriggerHalfExtents
	});

	CComPxBoxCollider::DESC colliderDesc{};
	colliderDesc.pComPxRigidBody = m_pComPxPassageTriggerRigidBody;
	colliderDesc.pResMaterial = pMaterial;
	colliderDesc.pResBoxGeo = pGeometry;
	colliderDesc.tFilter = desc.tPassageTriggerFilter;
	colliderDesc.iShapeSubIndex = ETOUI(SHAPE_SUB_INDEX::PASSAGE_TRIGGER);
	colliderDesc.bIsTrigger = true;
	if (!pMaterial || !pGeometry ||
		FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
			"ComPxBoxCollider_DoorPassageTrigger",
			&colliderDesc,
			&m_pComPxPassageTriggerCollider)))
	{
		return false;
	}

	return true;
}

_bool CPhysicsDoor::CreatePassageBarrier(const DESC& desc)
{
	if (!m_pComPxPassageTriggerRigidBody)
		return false;

	auto pMaterial = CResPhysXMaterial::CreateAndLoad({});
	auto pGeometry = CResPhysXBoxGeometry::CreateAndLoad({
		.vHalfExtents = m_vPassageBarrierHalfExtents
	});

	CComPxBoxCollider::DESC colliderDesc{};
	// [LSY] Trigger와 차단면은 같은 고정 Pose를 사용하므로 Actor를 공유한다.
	// Shape별 Query/Trigger 플래그는 독립적이라 차단면만 안전하게 끌 수 있다.
	colliderDesc.pComPxRigidBody = m_pComPxPassageTriggerRigidBody;
	colliderDesc.pResMaterial = pMaterial;
	colliderDesc.pResBoxGeo = pGeometry;
	colliderDesc.tFilter = desc.tCCTBlockerFilter;
	colliderDesc.iShapeSubIndex = ETOUI(SHAPE_SUB_INDEX::PASSAGE_BARRIER);
	if (!pMaterial || !pGeometry ||
		FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
			"ComPxBoxCollider_DoorPassageBarrier",
			&colliderDesc,
			&m_pComPxPassageBarrierCollider)))
	{
		return false;
	}

	m_bPassageBarrierEnabled = true;
	return true;
}

void CPhysicsDoor::OnRegisteredToManager()
{
	if (!CreateHingeJoint())
	{
		DEBUG_LOG(
			"[PhysicsDoor] Failed to create the D6 hinge joint.\n");
	}
}

_bool CPhysicsDoor::CreateHingeJoint()
{
	if (m_pComPxD6Joint)
		return true;
	if (!m_pComPxDoorRigidBody)
		return false;

	const _float fHingeX = m_eHingeSide == HINGE_SIDE::LEFT
		? -m_vHalfExtents.x
		: m_vHalfExtents.x;
	const _float3 vLocalHingePosition{ fHingeX, 0.f, 0.f };

	// D6의 Twist는 Joint Local X축 회전이다. Local X를 문의 세로축 Y에
	// 맞추기 위해 Joint Frame을 Z축으로 90도 돌린다.
	const _matrix matLocalHingeRotation = XMMatrixRotationZ(XM_PIDIV2);
	const _matrix matBodyRotation = XMMatrixRotationQuaternion(
		XMLoadFloat4(&m_vInitialRotation));
	const _matrix matWorldHingeRotation =
		matLocalHingeRotation * matBodyRotation;

	const _float3 vWorldHingePosition = CalculateHingeWorldPosition(
		m_vInitialPosition,
		m_vInitialRotation);

	_float4 vLocalHingeRotation{};
	_float4 vWorldHingeRotation{};
	XMStoreFloat4(
		&vLocalHingeRotation,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(matLocalHingeRotation)));
	XMStoreFloat4(
		&vWorldHingeRotation,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(matWorldHingeRotation)));

	CComPxD6Joint::DESC desc{};
	desc.pRigidBodyB = m_pComPxDoorRigidBody;
	desc.bPreserveCurrentPose = false;
	desc.tLocalFrameA.vPosition = vWorldHingePosition;
	desc.tLocalFrameA.vRotation = vWorldHingeRotation;
	desc.tLocalFrameB.vPosition = vLocalHingePosition;
	desc.tLocalFrameB.vRotation = vLocalHingeRotation;
	desc.eMotions[
		static_cast<size_t>(CComPxD6Joint::AXIS::TWIST)] =
		CComPxD6Joint::MOTION::LIMITED;
	desc.tTwistLimit.fLowerDegrees = m_fLowerLimitDegrees;
	desc.tTwistLimit.fUpperDegrees = m_fUpperLimitDegrees;
	desc.tTwistLimit.tResponse.fRestitution = 0.05f;
	desc.tTwistLimit.tResponse.fBounceThreshold = 1.f;
	// [LSY] 닫힌 Joint 상대 자세를 목표로 하는 스프링 Drive다.
	// 플레이어가 밀기를 멈추면 Stiffness가 문을 원위치로 복귀시키고,
	// Damping이 복귀 과정의 진동을 억제한다.
	desc.tDrives[
		static_cast<size_t>(CComPxD6Joint::DRIVE::TWIST)] = {
		.fStiffness = m_fTwistDriveStiffness,
		.fDamping = m_fTwistDriveDamping,
		.fForceLimit = m_fTwistDriveForceLimit,
		.bAcceleration = false
	};
	desc.bCollisionEnabled = false;
	desc.bVisualizationEnabled = true;
	desc.iJointSubIndex = 0u;

	m_pComPxD6Joint = CGameInstance::Get().AddPxJoint<CComPxD6Joint>(
		*this,
		"ComPxD6Joint_DoorHinge",
		desc);
	return m_pComPxD6Joint != nullptr;
}

void CPhysicsDoor::LateUpdate(_float)
{
	UpdatePhysicData();
	GetTransform().Update();
	UpdatePassageState();
	DrawDebugDoor();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		*GetTransform().GetCombinedWorldMatrix());
}

HRESULT CPhysicsDoor::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CPhysicsDoor::Render(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& ctx)
{
	if (CGameInstance::Get().IsInstancingEnabled())
		return S_OK;

	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
	{
		return E_FAIL;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

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
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	for (uint32_t meshIndex = 0;
		meshIndex < pModel->Get_NumMeshes();
		++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* pVertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t iStride = mesh->GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0, 1, &pVertexBuffer, &iStride, &iOffset);
		pContext->IASetIndexBuffer(
			mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, meshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext,
			{ 1.f, 1.f, 1.f }, 0.f,
			{ 1.f, 1.f, 1.f }, 0.f,
			1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

void CPhysicsDoor::DrawDebugDoor()
{
	if (!m_bDebugDraw)
		return;

	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE ePreviousDepth = pDebug->GetDepthMode();
	const _matrix matWorld = GetTransform().GetLoadedWorldMatrix();

	pDebug->SetDepthTest(false);
	pDebug->SetColor({ 0.95f, 0.5f, 0.1f, 1.f });
	pDebug->AddBox(m_vHalfExtents, matWorld);

	if (m_pComPxHingeBlockerCollider)
	{
		const _matrix matHingeBlockerWorld =
			XMMatrixRotationQuaternion(XMLoadFloat4(&m_vInitialRotation)) *
			XMMatrixTranslation(
				m_vHingeWorldPosition.x,
				m_vHingeWorldPosition.y,
				m_vHingeWorldPosition.z);
		pDebug->SetColor({ 0.1f, 0.85f, 1.f, 1.f });
		pDebug->AddBox(
			m_vHingeBlockerHalfExtents,
			matHingeBlockerWorld);
	}

	if (m_pComPxPassageTriggerCollider)
	{
		const _matrix matPassageTriggerWorld =
			XMMatrixRotationQuaternion(XMLoadFloat4(&m_vInitialRotation)) *
			XMMatrixTranslation(
				m_vInitialPosition.x,
				m_vInitialPosition.y,
				m_vInitialPosition.z);
		pDebug->SetColor({ 0.2f, 1.f, 0.35f, 1.f });
		pDebug->AddBox(
			m_vPassageTriggerHalfExtents,
			matPassageTriggerWorld);
	}

	if (m_pComPxPassageBarrierCollider)
	{
		const _matrix matPassageBarrierWorld =
			XMMatrixRotationQuaternion(XMLoadFloat4(&m_vInitialRotation)) *
			XMMatrixTranslation(
				m_vInitialPosition.x,
				m_vInitialPosition.y,
				m_vInitialPosition.z);
		pDebug->SetColor(m_bPassageBarrierEnabled
			? _float4{ 1.f, 0.15f, 0.8f, 1.f }
			: _float4{ 0.35f, 0.15f, 0.3f, 1.f });
		pDebug->AddBox(
			m_vPassageBarrierHalfExtents,
			matPassageBarrierWorld);
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepth);
}

void CPhysicsDoor::OnTriggerEnter(
	CGameObject* pGameObject,
	const PX_ON_TRIGGER_DATA& tData)
{
	if (!IsPlayerPassageTriggerEvent(pGameObject, tData))
		return;

	// [LSY] 플레이어가 문 통과 감지 영역에 들어온 시점이다.
	// 문 열기 안내, 상호작용 상태 시작 등 진입 로직은 이 분기에 추가한다.
	m_bPlayerInsidePassageTrigger = true;
	DEBUG_LOG("[PhysicsDoor] Player entered the passage trigger.\n");
}

void CPhysicsDoor::OnTriggerExit(
	CGameObject* pGameObject,
	const PX_ON_TRIGGER_DATA& tData)
{
	if (!IsPlayerPassageTriggerEvent(pGameObject, tData))
		return;

	// [LSY] 플레이어가 문 통과 감지 영역을 완전히 벗어난 시점이다.
	// 문 닫기, 다음 구역 활성화 등 이탈 완료 로직은 이 분기에 추가한다.
	m_bPlayerInsidePassageTrigger = false;
	DEBUG_LOG("[PhysicsDoor] Player exited the passage trigger.\n");
}

_bool CPhysicsDoor::SetReturnDrivePaused(_bool bPaused)
{
	if (m_bReturnDrivePaused == bPaused)
		return true;
	if (!m_pComPxD6Joint)
		return false;

	CComPxD6Joint::DRIVE_DESC driveDesc{
		.fStiffness = m_fTwistDriveStiffness,
		.fDamping = m_fTwistDriveDamping,
		.fForceLimit = m_fTwistDriveForceLimit,
		.bAcceleration = false
	};
	if (bPaused)
		driveDesc.fStiffness = 0.f;
	if (!m_pComPxD6Joint->SetDrive(
		CComPxD6Joint::DRIVE::TWIST,
		driveDesc))
	{
		return false;
	}

	// [LSY] 플레이어가 열린 문을 통과하는 동안 복귀 Drive만 잠시 멈춘다.
	// 움직이는 문짝의 Query는 항상 유지하고 고정 차단면만 별도로 제어한다.
	m_bReturnDrivePaused = bPaused;
	return true;
}

_bool CPhysicsDoor::SetPassageBarrierEnabled(_bool bEnabled)
{
	if (m_bPassageBarrierEnabled == bEnabled)
		return true;
	if (!m_pComPxPassageBarrierCollider ||
		!m_pComPxPassageBarrierCollider->SetQueryEnabled(bEnabled))
	{
		return false;
	}

	m_bPassageBarrierEnabled = bEnabled;
	return true;
}

void CPhysicsDoor::UpdatePassageState()
{
	const _float fAbsoluteAngle = fabsf(GetOpeningAngleDegrees());
	const _bool bShouldOpenPassage =
		m_bPlayerInsidePassageTrigger &&
		fAbsoluteAngle >= m_fPassageOpenAngleDegrees;
	const _bool bShouldResumeReturnDrive =
		m_bReturnDrivePaused &&
		(!m_bPlayerInsidePassageTrigger ||
		 fAbsoluteAngle < m_fPassageOpenAngleDegrees * 0.5f);

	if (bShouldOpenPassage)
	{
		SetReturnDrivePaused(true);
		SetPassageBarrierEnabled(false);
	}
	else if (bShouldResumeReturnDrive)
		SetReturnDrivePaused(false);

	// [LSY] 플레이어가 차단면과 겹친 상태에서는 다시 켜지 않는다.
	// 통과가 끝나고 문이 충분히 닫힌 뒤에만 다음 진입을 차단한다.
	if (!m_bPassageBarrierEnabled &&
		!m_bPlayerInsidePassageTrigger &&
		fAbsoluteAngle < m_fPassageOpenAngleDegrees * 0.5f)
	{
		SetPassageBarrierEnabled(true);
	}
}

void CPhysicsDoor::UpdateGUI()
{
	CGameObject::UpdateGUI();

	ImGui::Separator();
	ImGui::TextUnformatted("D6 Physics Door");
	ImGui::Text(
		"Hinge: %s | Angle: %.2f deg",
		m_eHingeSide == HINGE_SIDE::LEFT ? "Left" : "Right",
		GetOpeningAngleDegrees());
	ImGui::Checkbox("Debug Door Shape", &m_bDebugDraw);
	ImGui::Text(
		"Passage: %s | Return Drive: %s | Barrier: %s",
		m_bPlayerInsidePassageTrigger ? "Player Inside" : "Empty",
		m_bReturnDrivePaused ? "Paused" : "Enabled",
		m_bPassageBarrierEnabled ? "Blocking" : "Open");
	ImGui::DragFloat(
		"Passage Open Angle",
		&m_fPassageOpenAngleDegrees,
		1.f,
		1.f,
		90.f,
		"%.1f deg");
	ImGui::DragFloat(
		"CCT Push Force",
		&m_fCCTPushForce,
		10.f,
		0.f,
		5000.f,
		"%.0f");
	ImGui::DragFloat(
		"Test Torque",
		&m_fTestTorque,
		10.f,
		0.f,
		10000.f,
		"%.0f");

	if (ImGui::Button("Push Open"))
		ApplyOpeningTorque(m_fTestTorque);
	ImGui::SameLine();
	if (ImGui::Button("Push Close"))
		ApplyOpeningTorque(-m_fTestTorque);
	ImGui::SameLine();
	if (ImGui::Button("Reset Door"))
		ResetDoor();

	if (ImGui::Button("Apply Transform As Door Placement"))
	{
		SetPlacement(
			GetTransform().GetPosition(),
			GetTransform().GetRotationEuler());
	}
}

_bool CPhysicsDoor::ApplyOpeningTorque(_float fTorque)
{
	return m_pComPxDoorRigidBody &&
		m_pComPxDoorRigidBody->AddTorque({ 0.f, fTorque, 0.f });
}

_bool CPhysicsDoor::ApplyCCTPush(const PX_CCT_HIT_DATA& tHit)
{
	const _bool bPushableShape =
		tHit.iOtherShapeSubIndex == ETOUI(SHAPE_SUB_INDEX::DOOR_LEAF) ||
		tHit.iOtherShapeSubIndex == ETOUI(SHAPE_SUB_INDEX::PASSAGE_BARRIER);
	if (!bPushableShape ||
		m_fCCTPushForce <= 0.f ||
		tHit.fMoveLength <= std::numeric_limits<_float>::epsilon())
	{
		return false;
	}

	_float3 vPushDirection = tHit.vMoveDirection;
	vPushDirection.y = 0.f;
	const _float fDirectionLengthSq =
		vPushDirection.x * vPushDirection.x +
		vPushDirection.z * vPushDirection.z;
	if (fDirectionLengthSq <= std::numeric_limits<_float>::epsilon())
		return false;

	const _float fInverseDirectionLength = 1.f / sqrtf(fDirectionLengthSq);
	vPushDirection.x *= fInverseDirectionLength;
	vPushDirection.z *= fInverseDirectionLength;

	const _float fLeverX =
		tHit.vWorldPosition.x - m_vHingeWorldPosition.x;
	const _float fLeverZ =
		tHit.vWorldPosition.z - m_vHingeWorldPosition.z;
	const _float fTorqueY =
		(fLeverZ * vPushDirection.x -
		 fLeverX * vPushDirection.z) *
		m_fCCTPushForce;
	if (fabsf(fTorqueY) <= std::numeric_limits<_float>::epsilon())
		return false;

	return ApplyOpeningTorque(fTorqueY);
}

_bool CPhysicsDoor::SetPlacement(
	const _float3& vPosition,
	const _float3& vRotationEulerDegrees)
{
	if (!m_pComPxDoorRigidBody ||
		!m_pComPxHingeBlockerRigidBody ||
		!m_pComPxPassageTriggerRigidBody ||
		!m_pComPxD6Joint)
	{
		return false;
	}
	if (!SetReturnDrivePaused(false))
		return false;

	_float4 vRotation{};
	XMStoreFloat4(
		&vRotation,
		XMQuaternionNormalize(
			XMQuaternionRotationRollPitchYaw(
				XMConvertToRadians(vRotationEulerDegrees.x),
				XMConvertToRadians(vRotationEulerDegrees.y),
				XMConvertToRadians(vRotationEulerDegrees.z))));

	const _float3 vHingeWorldPosition = CalculateHingeWorldPosition(
		vPosition,
		vRotation);

	const _float3 vPreviousDoorPosition =
		m_pComPxDoorRigidBody->GetPosition();
	const _float4 vPreviousDoorRotation =
		m_pComPxDoorRigidBody->GetRotation();
	const _float3 vPreviousBlockerPosition =
		m_pComPxHingeBlockerRigidBody->GetPosition();
	const _float4 vPreviousBlockerRotation =
		m_pComPxHingeBlockerRigidBody->GetRotation();
	const _float3 vPreviousPassageTriggerPosition =
		m_pComPxPassageTriggerRigidBody->GetPosition();
	const _float4 vPreviousPassageTriggerRotation =
		m_pComPxPassageTriggerRigidBody->GetRotation();

	// [LSY] 동적 문짝만 옮기면 기존 월드 앵커가 다시 끌어당기므로
	// 조인트가 문짝 Pose와 월드 앵커를 함께 재배치하도록 한다.
	if (!m_pComPxD6Joint->RelocateWorldAnchoredRigidBody(
			vPosition,
			vRotation))
	{
		return false;
	}

	if (!m_pComPxHingeBlockerRigidBody->SetPose(
			vHingeWorldPosition,
			vRotation))
	{
		m_pComPxD6Joint->RelocateWorldAnchoredRigidBody(
			vPreviousDoorPosition,
			vPreviousDoorRotation);
		m_pComPxHingeBlockerRigidBody->SetPose(
			vPreviousBlockerPosition,
			vPreviousBlockerRotation);
		return false;
	}

	if (!m_pComPxPassageTriggerRigidBody->SetPose(vPosition, vRotation))
	{
		m_pComPxD6Joint->RelocateWorldAnchoredRigidBody(
			vPreviousDoorPosition,
			vPreviousDoorRotation);
		m_pComPxHingeBlockerRigidBody->SetPose(
			vPreviousBlockerPosition,
			vPreviousBlockerRotation);
		m_pComPxPassageTriggerRigidBody->SetPose(
			vPreviousPassageTriggerPosition,
			vPreviousPassageTriggerRotation);
		return false;
	}

	m_vInitialPosition = vPosition;
	m_vInitialRotation = vRotation;
	m_vHingeWorldPosition = vHingeWorldPosition;

	GetTransform().SetPosition(vPosition);
	GetTransform().SetRotationEuler(vRotationEulerDegrees);
	GetTransform().Update();
	return true;
}

_bool CPhysicsDoor::ResetDoor()
{
	if (!m_pComPxD6Joint ||
		!SetReturnDrivePaused(false) ||
		!SetPassageBarrierEnabled(true))
		return false;

	return m_pComPxD6Joint->RelocateWorldAnchoredRigidBody(
		m_vInitialPosition,
		m_vInitialRotation);
}

_float CPhysicsDoor::GetOpeningAngleDegrees() const
{
	return m_pComPxD6Joint
		? m_pComPxD6Joint->GetTwistAngleDegrees()
		: 0.f;
}

UPtr<CPhysicsDoor> CPhysicsDoor::Create()
{
	auto pInstance = ToUPtr(new CPhysicsDoor{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPhysicsDoor::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPhysicsDoor{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
