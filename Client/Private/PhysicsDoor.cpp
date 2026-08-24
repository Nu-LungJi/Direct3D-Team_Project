#include "pch.h"
#include "PhysicsDoor.h"

#include "ComPxBoxCollider.h"
#include "ComPxD6Joint.h"
#include "ComPxRigidBody.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

NS_USING(Client)

CPhysicsDoor::CPhysicsDoor() = default;

CPhysicsDoor::CPhysicsDoor(const CPhysicsDoor& prototype)
	: CGameObject{ prototype }
{
}

HRESULT CPhysicsDoor::InitializePrototype(void*)
{
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
		pDesc->fLowerLimitDegrees >= pDesc->fUpperLimitDegrees ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	m_vHalfExtents = pDesc->vHalfExtents;
	m_vInitialPosition = pDesc->vInitialPosition;
	m_fLowerLimitDegrees = pDesc->fLowerLimitDegrees;
	m_fUpperLimitDegrees = pDesc->fUpperLimitDegrees;
	m_eHingeSide = pDesc->eHingeSide;

	GetTransform().SetPosition(m_vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_vInitialRotation = GetTransform().GetQuaternion();
	GetTransform().Update();

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
			&m_pComPxRigidBody)))
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
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResMaterial = pMaterial;
		desc.pResBoxGeo = pGeometry;
		desc.tFilter = pDesc->tFilter;
		if (!pMaterial ||
			!pGeometry ||
			FAILED(AddComponentFromProto(
				ES_EngineProtoMajorType::PHYSX,
				ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider,
				"ComPxBoxCollider",
				&desc,
				&m_pComPxBoxCollider)))
		{
			return E_FAIL;
		}
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(false) ||
		!m_pComPxRigidBody->SetAngularDamping(
			std::max(pDesc->fAngularDamping, 0.f)) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(5.f) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	return S_OK;
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
	if (!m_pComPxRigidBody)
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

	_float3 vWorldHingePosition{};
	XMStoreFloat3(
		&vWorldHingePosition,
		XMVector3TransformCoord(
			XMLoadFloat3(&vLocalHingePosition),
			matBodyRotation) +
		XMLoadFloat3(&m_vInitialPosition));

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
	desc.pRigidBodyB = m_pComPxRigidBody;
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
	DrawDebugDoor();
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

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepth);
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
}

_bool CPhysicsDoor::ApplyOpeningTorque(_float fTorque)
{
	return m_pComPxRigidBody &&
		m_pComPxRigidBody->AddTorque({ 0.f, fTorque, 0.f });
}

_bool CPhysicsDoor::ResetDoor()
{
	return m_pComPxD6Joint &&
		m_pComPxD6Joint->ResetWorldAnchoredRigidBodyToPlacement();
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
