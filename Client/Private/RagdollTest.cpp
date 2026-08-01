#include "pch.h"
#include "RagdollTest.h"

#include "ComPxRagdoll.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "PhysXRagdollData.h"

NS_USING(Client)

namespace
{
	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		return XMMatrixRotationQuaternion(
			XMLoadFloat4(&vRotation)) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}

	_float3 GetMatrixPosition(FXMMATRIX Matrix)
	{
		_float3 vPosition{};
		XMStoreFloat3(&vPosition, Matrix.r[3]);
		return vPosition;
	}
}

CRagdollTest::CRagdollTest()
	: CGameObject{}
{
}

CRagdollTest::CRagdollTest(
	const CRagdollTest& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CRagdollTest::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CRagdollTest::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().Update();

	CComPxRagdoll::DESC tRagdollComponentDesc{};
	tRagdollComponentDesc.tRagdoll =
		MakeTestRagdollDesc();
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PHYSX,
		ES_EngineProtoPhysXComponent::
			Prototype_Component_ComPxRagdoll,
		"ComPxRagdoll",
		&tRagdollComponentDesc,
		&m_pComPxRagdoll)) ||
		!m_pComPxRagdoll)
	{
		return E_FAIL;
	}

	return S_OK;
}

void CRagdollTest::FixedUpdate(_float fTimeDelta)
{
	if (m_pComPxRagdoll)
		m_pComPxRagdoll->SyncKinematicPoseFromOwner();
}

void CRagdollTest::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	DrawDebugRagdoll();
}

void CRagdollTest::UpdateGUI()
{
	CGameObject::UpdateGUI();

	if (!m_pComPxRagdoll)
		return;

	ImGui::Separator();
	ImGui::TextUnformatted("Ragdoll Test");
	ImGui::Text(
		"State: %s",
		m_pComPxRagdoll->IsRagdollActive()
			? "Dynamic"
			: "Kinematic");
	ImGui::DragFloat3(
		"Initial Linear Velocity",
		&m_vActivationLinearVelocity.x,
		0.1f);
	ImGui::DragFloat3(
		"Initial Angular Velocity (Deg/s)",
		&m_vActivationAngularVelocityDegrees.x,
		1.f);

	if (ImGui::Button("Activate Ragdoll"))
	{
		const _float3 vAngularVelocityRadians{
			XMConvertToRadians(
				m_vActivationAngularVelocityDegrees.x),
			XMConvertToRadians(
				m_vActivationAngularVelocityDegrees.y),
			XMConvertToRadians(
				m_vActivationAngularVelocityDegrees.z)
		};
		m_pComPxRagdoll->ActivateRagdoll(
			m_vActivationLinearVelocity,
			vAngularVelocityRadians);
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset Kinematic"))
		m_pComPxRagdoll->ResetToKinematicPose();
}

PX_RAGDOLL_DESC CRagdollTest::MakeTestRagdollDesc()
{
	PX_RAGDOLL_DESC tDesc{};
	tDesc.sSkeletonTag = "TEST_SYNTHETIC_HUMANOID";

	const uint32_t iRagdollLayer =
		ETOUI(COLLISION_LAYER::RAGDOLL);
	const uint32_t iRagdollSimulationMask =
		ETOUI(COLLISION_LAYER::WORLD_STATIC) |
		ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
		ETOUI(COLLISION_LAYER::RAGDOLL);

	auto MakeShape =
		[iRagdollLayer, iRagdollSimulationMask](
			std::string sName,
			PX_RAGDOLL_SHAPE_TYPE eType)
		{
			PX_RAGDOLL_SHAPE_DESC tShape{};
			tShape.sName = std::move(sName);
			tShape.eType = eType;
			tShape.iLayer = iRagdollLayer;
			tShape.iSimulationMask =
				iRagdollSimulationMask;
			return tShape;
		};

	auto AddCapsuleBody =
		[&tDesc, &MakeShape](
			const std::string& sBodyName,
			const _float3& vPosition,
			_float fMass,
			_float fRadius,
			_float fHalfHeight,
			const _float4& vShapeRotation)
		{
			PX_RAGDOLL_BODY_DESC tBody{};
			tBody.sBodyName = sBodyName;
			tBody.sBoneName =
				std::string{ "TEST_" } + sBodyName;
			tBody.vBoneToActorPosition = vPosition;
			tBody.fMass = fMass;

			auto tShape = MakeShape(
				sBodyName + "Capsule",
				PX_RAGDOLL_SHAPE_TYPE::CAPSULE);
			tShape.fRadius = fRadius;
			tShape.fHalfHeight = fHalfHeight;
			tShape.vLocalRotation = vShapeRotation;
			tBody.Shapes.emplace_back(
				std::move(tShape));
			tDesc.Bodies.emplace_back(
				std::move(tBody));
		};

	auto AddD6Joint =
		[&tDesc](
			const std::string& sJointName,
			const std::string& sParentBodyName,
			const std::string& sChildBodyName,
			const _float3& vParentLocalPosition,
			const _float3& vChildLocalPosition,
			_float fTwistLowerDegrees,
			_float fTwistUpperDegrees,
			_float fSwingYDegrees,
			_float fSwingZDegrees,
			_bool bHinge)
		{
			PX_RAGDOLL_D6_JOINT_DESC tJoint{};
			tJoint.sJointName = sJointName;
			tJoint.sParentBodyName =
				sParentBodyName;
			tJoint.sChildBodyName =
				sChildBodyName;
			tJoint.vParentLocalPosition =
				vParentLocalPosition;
			tJoint.vChildLocalPosition =
				vChildLocalPosition;
			tJoint.fTwistLowerDegrees =
				fTwistLowerDegrees;
			tJoint.fTwistUpperDegrees =
				fTwistUpperDegrees;
			tJoint.fSwingYDegrees =
				fSwingYDegrees;
			tJoint.fSwingZDegrees =
				fSwingZDegrees;
			if (bHinge)
			{
				tJoint.eSwingYMotion =
					PX_RAGDOLL_D6_MOTION::LOCKED;
				tJoint.eSwingZMotion =
					PX_RAGDOLL_D6_MOTION::LOCKED;
			}
			tDesc.Joints.emplace_back(
				std::move(tJoint));
		};

	{
		PX_RAGDOLL_BODY_DESC tBody{};
		tBody.sBodyName = "Pelvis";
		tBody.sBoneName = "TEST_Pelvis";
		tBody.vBoneToActorPosition = { 0.f, 0.95f, 0.f };
		tBody.fMass = 12.f;

		auto tShape = MakeShape(
			"PelvisBox",
			PX_RAGDOLL_SHAPE_TYPE::BOX);
		tShape.vHalfExtents = { 0.45f, 0.375f, 0.25f };
		tBody.Shapes.emplace_back(std::move(tShape));
		tDesc.Bodies.emplace_back(std::move(tBody));
	}

	{
		PX_RAGDOLL_BODY_DESC tBody{};
		tBody.sBodyName = "Chest";
		tBody.sBoneName = "TEST_Chest";
		tBody.vBoneToActorPosition = { 0.f, 1.7f, 0.f };
		tBody.fMass = 15.f;

		auto tShape = MakeShape(
			"ChestBox",
			PX_RAGDOLL_SHAPE_TYPE::BOX);
		tShape.vHalfExtents = { 0.55f, 0.375f, 0.3f };
		tBody.Shapes.emplace_back(std::move(tShape));
		tDesc.Bodies.emplace_back(std::move(tBody));
	}

	{
		PX_RAGDOLL_BODY_DESC tBody{};
		tBody.sBodyName = "Head";
		tBody.sBoneName = "TEST_Head";
		tBody.vBoneToActorPosition = { 0.f, 2.45f, 0.f };
		tBody.fMass = 4.f;

		auto tShape = MakeShape(
			"HeadSphere",
			PX_RAGDOLL_SHAPE_TYPE::SPHERE);
		tShape.fRadius = 0.35f;
		tBody.Shapes.emplace_back(std::move(tShape));
		tDesc.Bodies.emplace_back(std::move(tBody));
	}

	constexpr _float SIN_COS_45 = 0.70710678f;
	const _float4 vHorizontalCapsuleRotation{
		0.f,
		0.f,
		SIN_COS_45,
		SIN_COS_45
	};
	const _float4 vVerticalCapsuleRotation{
		0.f,
		0.f,
		0.f,
		1.f
	};

	AddCapsuleBody(
		"LeftUpperArm",
		{ -0.95f, 1.75f, 0.f },
		3.f,
		0.18f,
		0.37f,
		vHorizontalCapsuleRotation);
	AddCapsuleBody(
		"LeftLowerArm",
		{ -1.7f, 1.75f, 0.f },
		2.f,
		0.14f,
		0.32f,
		vHorizontalCapsuleRotation);
	AddCapsuleBody(
		"RightUpperArm",
		{ 0.95f, 1.75f, 0.f },
		3.f,
		0.18f,
		0.37f,
		vHorizontalCapsuleRotation);
	AddCapsuleBody(
		"RightLowerArm",
		{ 1.7f, 1.75f, 0.f },
		2.f,
		0.14f,
		0.32f,
		vHorizontalCapsuleRotation);

	AddCapsuleBody(
		"LeftUpperLeg",
		{ -0.28f, 0.25f, 0.f },
		7.f,
		0.2f,
		0.38f,
		vVerticalCapsuleRotation);
	AddCapsuleBody(
		"LeftLowerLeg",
		{ -0.28f, -0.65f, 0.f },
		5.f,
		0.16f,
		0.38f,
		vVerticalCapsuleRotation);
	AddCapsuleBody(
		"RightUpperLeg",
		{ 0.28f, 0.25f, 0.f },
		7.f,
		0.2f,
		0.38f,
		vVerticalCapsuleRotation);
	AddCapsuleBody(
		"RightLowerLeg",
		{ 0.28f, -0.65f, 0.f },
		5.f,
		0.16f,
		0.38f,
		vVerticalCapsuleRotation);

	{
		PX_RAGDOLL_D6_JOINT_DESC tJoint{};
		tJoint.sJointName = "PelvisToChest";
		tJoint.sParentBodyName = "Pelvis";
		tJoint.sChildBodyName = "Chest";
		tJoint.vParentLocalPosition = { 0.f, 0.375f, 0.f };
		tJoint.vChildLocalPosition = { 0.f, -0.375f, 0.f };
		tJoint.fTwistLowerDegrees = -15.f;
		tJoint.fTwistUpperDegrees = 15.f;
		tJoint.fSwingYDegrees = 20.f;
		tJoint.fSwingZDegrees = 20.f;
		tDesc.Joints.emplace_back(std::move(tJoint));
	}

	{
		PX_RAGDOLL_D6_JOINT_DESC tJoint{};
		tJoint.sJointName = "ChestToHead";
		tJoint.sParentBodyName = "Chest";
		tJoint.sChildBodyName = "Head";
		tJoint.vParentLocalPosition = { 0.f, 0.4f, 0.f };
		tJoint.vChildLocalPosition = { 0.f, -0.35f, 0.f };
		tJoint.fTwistLowerDegrees = -30.f;
		tJoint.fTwistUpperDegrees = 30.f;
		tJoint.fSwingYDegrees = 30.f;
		tJoint.fSwingZDegrees = 25.f;
		tDesc.Joints.emplace_back(std::move(tJoint));
	}

	AddD6Joint(
		"ChestToLeftUpperArm",
		"Chest",
		"LeftUpperArm",
		{ -0.55f, 0.05f, 0.f },
		{ 0.4f, 0.f, 0.f },
		-50.f,
		50.f,
		65.f,
		55.f,
		false);
	AddD6Joint(
		"LeftElbow",
		"LeftUpperArm",
		"LeftLowerArm",
		{ -0.4f, 0.f, 0.f },
		{ 0.35f, 0.f, 0.f },
		-110.f,
		110.f,
		5.f,
		5.f,
		true);
	AddD6Joint(
		"ChestToRightUpperArm",
		"Chest",
		"RightUpperArm",
		{ 0.55f, 0.05f, 0.f },
		{ -0.4f, 0.f, 0.f },
		-50.f,
		50.f,
		65.f,
		55.f,
		false);
	AddD6Joint(
		"RightElbow",
		"RightUpperArm",
		"RightLowerArm",
		{ 0.4f, 0.f, 0.f },
		{ -0.35f, 0.f, 0.f },
		-110.f,
		110.f,
		5.f,
		5.f,
		true);

	AddD6Joint(
		"PelvisToLeftUpperLeg",
		"Pelvis",
		"LeftUpperLeg",
		{ -0.28f, -0.35f, 0.f },
		{ 0.f, 0.35f, 0.f },
		-35.f,
		35.f,
		50.f,
		35.f,
		false);
	AddD6Joint(
		"LeftKnee",
		"LeftUpperLeg",
		"LeftLowerLeg",
		{ 0.f, -0.45f, 0.f },
		{ 0.f, 0.45f, 0.f },
		-5.f,
		125.f,
		5.f,
		5.f,
		true);
	AddD6Joint(
		"PelvisToRightUpperLeg",
		"Pelvis",
		"RightUpperLeg",
		{ 0.28f, -0.35f, 0.f },
		{ 0.f, 0.35f, 0.f },
		-35.f,
		35.f,
		50.f,
		35.f,
		false);
	AddD6Joint(
		"RightKnee",
		"RightUpperLeg",
		"RightLowerLeg",
		{ 0.f, -0.45f, 0.f },
		{ 0.f, 0.45f, 0.f },
		-5.f,
		125.f,
		5.f,
		5.f,
		true);

	return tDesc;
}

void CRagdollTest::DrawDebugRagdoll() const
{
	if (!m_pComPxRagdoll)
		return;

	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const auto& tRagdoll =
		m_pComPxRagdoll->GetRagdollDesc();
	const _float4 vPreviousColor = pDebug->GetColor();
	const auto ePreviousDepthMode =
		pDebug->GetDepthMode();

	pDebug->SetDepthTest(true);

	for (size_t i = 0; i < tRagdoll.Bodies.size(); ++i)
	{
		const auto& tBody = tRagdoll.Bodies[i];
		_float4x4 ActorWorldFloat4x4{};
		if (!m_pComPxRagdoll->GetBodyWorldMatrix(
			i,
			ActorWorldFloat4x4))
		{
			continue;
		}
		const _matrix ActorWorld =
			XMLoadFloat4x4(&ActorWorldFloat4x4);

		_float4 vBodyColor{
			1.f, 0.9f, 0.2f, 1.f
		};
		if (tBody.sBodyName == "Pelvis")
		{
			vBodyColor = {
				0.2f, 1.f, 0.2f, 1.f
			};
		}
		else if (tBody.sBodyName == "Chest")
		{
			vBodyColor = {
				0.2f, 0.8f, 1.f, 1.f
			};
		}
		else if (tBody.sBodyName.find("Arm") !=
			std::string::npos)
		{
			vBodyColor = {
				1.f, 0.45f, 0.15f, 1.f
			};
		}
		else if (tBody.sBodyName.find("Leg") !=
			std::string::npos)
		{
			vBodyColor = {
				0.45f, 0.45f, 1.f, 1.f
			};
		}
		pDebug->SetColor(vBodyColor);

		for (const auto& tShape : tBody.Shapes)
		{
			const _matrix ShapeWorld =
				MakePoseMatrix(
					tShape.vLocalPosition,
					tShape.vLocalRotation) *
				ActorWorld;

			switch (tShape.eType)
			{
			case PX_RAGDOLL_SHAPE_TYPE::BOX:
				pDebug->AddBox(
					tShape.vHalfExtents,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
				pDebug->AddSphere(
					tShape.fRadius,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
				pDebug->AddCapsule(
					tShape.fRadius,
					tShape.fHalfHeight,
					ShapeWorld);
				break;
			}
		}

		pDebug->AddAxis(0.25f, ActorWorld);
	}

	const auto FindBodyIndex =
		[&tRagdoll](const std::string& sBodyName)
		-> size_t
		{
			const auto Iter = std::ranges::find_if(
				tRagdoll.Bodies,
				[&sBodyName](
					const PX_RAGDOLL_BODY_DESC& tBody)
				{
					return tBody.sBodyName == sBodyName;
				});
			return Iter != tRagdoll.Bodies.end()
				? static_cast<size_t>(
					std::distance(
						tRagdoll.Bodies.begin(),
						Iter))
				: std::numeric_limits<size_t>::max();
		};

	pDebug->SetColor({ 1.f, 0.2f, 0.2f, 1.f });
	for (const auto& tJoint : tRagdoll.Joints)
	{
		const size_t iParent =
			FindBodyIndex(tJoint.sParentBodyName);
		const size_t iChild =
			FindBodyIndex(tJoint.sChildBodyName);
		if (iParent == std::numeric_limits<size_t>::max() ||
			iChild == std::numeric_limits<size_t>::max())
			continue;

		_float4x4 ParentActorWorldFloat4x4{};
		_float4x4 ChildActorWorldFloat4x4{};
		if (!m_pComPxRagdoll->GetBodyWorldMatrix(
			iParent,
			ParentActorWorldFloat4x4) ||
			!m_pComPxRagdoll->GetBodyWorldMatrix(
				iChild,
				ChildActorWorldFloat4x4))
		{
			continue;
		}
		const _matrix ParentActorWorld =
			XMLoadFloat4x4(
				&ParentActorWorldFloat4x4);
		const _matrix ChildActorWorld =
			XMLoadFloat4x4(
				&ChildActorWorldFloat4x4);
		const _matrix ParentJointWorld =
			MakePoseMatrix(
				tJoint.vParentLocalPosition,
				tJoint.vParentLocalRotation) *
			ParentActorWorld;
		const _matrix ChildJointWorld =
			MakePoseMatrix(
				tJoint.vChildLocalPosition,
				tJoint.vChildLocalRotation) *
			ChildActorWorld;

		const _float3 vParentJointPosition =
			GetMatrixPosition(ParentJointWorld);
		const _float3 vChildJointPosition =
			GetMatrixPosition(ChildJointWorld);
		pDebug->AddLine(
			vParentJointPosition,
			vChildJointPosition);
		pDebug->AddCross(vParentJointPosition, 0.08f);
		pDebug->AddCross(vChildJointPosition, 0.08f);
		pDebug->AddAxis(0.2f, ParentJointWorld);
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepthMode);
}

UPtr<CRagdollTest> CRagdollTest::Create()
{
	auto pInstance = ToUPtr(new CRagdollTest{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CRagdollTest");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CRagdollTest::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CRagdollTest{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CRagdollTest");
		return nullptr;
	}

	return pInstance;
}
