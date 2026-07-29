#include "pch.h"
#include "NvClothCape.h"

#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComNvCloth.h"
#include "GameInstance.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResNvClothMesh.h"
#include "Resources.h"

NS_USING(Client)

namespace Client::NvClothCapeDetail
{
	_bool MakeRigidMatrix(
		_fmatrix Matrix,
		_matrix& OutRigidMatrix)
	{
		_vector vScale{};
		_vector qRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&qRotation,
			&vTranslation,
			Matrix))
		{
			return false;
		}

		qRotation = XMQuaternionNormalize(qRotation);
		OutRigidMatrix =
			XMMatrixRotationQuaternion(qRotation) *
			XMMatrixTranslationFromVector(vTranslation);
		return true;
	}
}

CNvClothCape::CNvClothCape()
	: CGameObject{}
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::CNvClothCape(
	const CNvClothCape& Prototype)
	: CGameObject{ Prototype },
	  m_pVertexShader{ Prototype.m_pVertexShader },
	  m_pPixelShader{ Prototype.m_pPixelShader }
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::~CNvClothCape()
{
}

HRESULT CNvClothCape::InitializePrototype(void*)
{
	m_pVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvCloth");
	m_pPixelShader =
		CGameInstance::Get().
		GetResourceFirst<CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"PS_NvCloth");
	return m_pVertexShader && m_pPixelShader ?
		S_OK : E_FAIL;
}

HRESULT CNvClothCape::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		pDesc->sResourceGroup.hash == 0 ||
		pDesc->sModelResourceTag.hash == 0 ||
		pDesc->sClothMeshResourceTag.hash == 0 ||
		pDesc->sTargetModelComponentTag.hash == 0 ||
		pDesc->sAttachBoneName.empty() ||
		!std::isfinite(pDesc->fTeleportDistance) ||
		pDesc->fTeleportDistance < 0.f ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	m_hTarget = pDesc->hTarget;
	m_sTargetModelComponentTag =
		pDesc->sTargetModelComponentTag;
	m_sAttachBoneName =
		pDesc->sAttachBoneName;
	m_fTeleportDistance =
		pDesc->fTeleportDistance;
	GetTransform().SetPosition(
		pDesc->vLocalPosition);

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = {
			TAG_RES_GRP_PERMANENT_BUFFER,
			TAG_RES_CBUFFER_OBJECT
		};
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject",
			&Desc,
			&m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->sResourceGroup;
		Desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ModelInstance",
			"ComModelInstance",
			&Desc,
			&m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	m_pClothMesh =
		CGameInstance::Get().
		GetResourceFirst<CResNvClothMesh>(
			pDesc->sResourceGroup,
			pDesc->sClothMeshResourceTag);
	if (!m_pClothMesh ||
		m_pClothMesh->GetSections().empty())
	{
		return E_FAIL;
	}

	{
		CComNvCloth::DESC Desc{};
		Desc.tFabric =
			m_pClothMesh->GetFabricDesc();
		Desc.tCloth.vGravity =
			{ 0.f, -9.81f, 0.f };
		Desc.tCloth.vDamping =
			{ 0.08f, 0.08f, 0.08f };
		Desc.tCloth.fSolverFrequency = 60.f;
		Desc.tCloth.fStiffnessFrequency = 60.f;
		Desc.tCloth.fPhaseStiffness = 0.9f;
		Desc.tCloth.fStretchLimit = 1.05f;
		Desc.tCloth.vLinearInertia =
			{ 1.f, 1.f, 1.f };
		Desc.tCloth.vAngularInertia =
			{ 0.8f, 0.8f, 0.8f };
		Desc.tCloth.vCentrifugalInertia =
			{ 0.8f, 0.8f, 0.8f };
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComNvCloth",
			"ComNvCloth",
			&Desc,
			&m_pComNvCloth)))
		{
			return E_FAIL;
		}
	}

	if (!ResolveAttachment() ||
		!UpdateAttachment(true) ||
		!UpdateBodyCollisions())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CNvClothCape::PriorityUpdate(_float)
{
}

void CNvClothCape::FixedUpdate(_float)
{
	if (UpdateAttachment(true))
		UpdateBodyCollisions();
}

void CNvClothCape::Update(_float)
{
}

void CNvClothCape::LateUpdate(_float)
{
	UpdateAttachment(false);
	CGameInstance::Get().AddRenderObject(
		RENDERGROUP::NONBLEND,
		this);
}

void CNvClothCape::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::PushID(this);

	auto vLocalPosition =
		GetTransform().GetPosition();
	if (ImGui::DragFloat3(
		"Cape Local Offset",
		&vLocalPosition.x,
		0.005f))
	{
		GetTransform().SetPosition(vLocalPosition);
		m_bSimulationTransformInitialized = false;
	}

	if (ImGui::TreeNode("Body Collision"))
	{
		ImGui::Checkbox(
			"Continuous Collision",
			&m_bContinuousBodyCollision);
		ImGui::DragFloat(
			"Collision Mass Scale",
			&m_fCollisionMassScale,
			0.1f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"Collision Friction",
			&m_fCollisionFriction,
			0.01f,
			0.f,
			1.f);

		for (auto& Binding : m_BodyCollisionBones)
		{
			ImGui::DragFloat(
				Binding.sBoneName.c_str(),
				&Binding.fRadius,
				0.005f,
				0.01f,
				1.f);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}

_bool CNvClothCape::ResolveAttachment()
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	m_iAttachBoneIndex =
		pModelInstance->GetModel()->
		Get_BoneIndex(m_sAttachBoneName.c_str());
	if (m_iAttachBoneIndex < 0)
		return false;

	const auto& SkinBoneNames =
		m_pClothMesh->GetSkinBoneNames();
	m_ResolvedSkinBoneIndices.resize(
		SkinBoneNames.size(),
		-1);
	m_SkinBoneToSimulationMatrices.resize(
		SkinBoneNames.size());
	size_t iResolvedSkinBoneCount{};
	for (size_t i = 0;
		i < SkinBoneNames.size();
		++i)
	{
		m_ResolvedSkinBoneIndices[i] =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				SkinBoneNames[i].c_str());
		if (m_ResolvedSkinBoneIndices[i] >= 0)
			++iResolvedSkinBoneCount;
	}

	char szLog[256]{};
	sprintf_s(
		szLog,
		"[NvClothCape] Skin bones resolved: %zu / %zu. "
		"Particles without a matching bone use their Rest Pose.\n",
		iResolvedSkinBoneCount,
		SkinBoneNames.size());
	DEBUG_LOG(szLog);

	for (auto& Binding : m_BodyCollisionBones)
	{
		Binding.iBoneIndex =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				Binding.sBoneName.c_str());
		if (Binding.iBoneIndex < 0)
			return false;
	}

	return true;
}

_bool CNvClothCape::UpdateAnimationConstraints(
	CComModelInstance& ModelInstance,
	_fmatrix AttachmentWorld,
	_bool bResetPreviousParticles)
{
	if (!m_pComNvCloth ||
		!m_pClothMesh ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto& Bindings =
		m_pClothMesh->GetParticleSkinBindings();
	const auto& RestPositions =
		m_pClothMesh->GetFabricDesc().vecPositions;
	if (Bindings.empty() ||
		Bindings.size() != RestPositions.size() ||
		m_ResolvedSkinBoneIndices.size() !=
			m_pClothMesh->GetSkinBoneNames().size() ||
		m_SkinBoneToSimulationMatrices.size() !=
			m_ResolvedSkinBoneIndices.size())
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseAttachmentWorld =
		XMMatrixInverse(
			&vDeterminant,
			AttachmentWorld);
	if (!std::isfinite(XMVectorGetX(vDeterminant)) ||
		std::fabs(XMVectorGetX(vDeterminant)) <=
			FLT_EPSILON)
	{
		return false;
	}

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	const _matrix TargetWorld =
		pTarget->GetTransform().
		GetLoadedCombinedWorldMatrix();
	for (size_t i = 0;
		i < m_ResolvedSkinBoneIndices.size();
		++i)
	{
		const int32_t iTargetBoneIndex =
			m_ResolvedSkinBoneIndices[i];
		if (iTargetBoneIndex < 0)
		{
			XMStoreFloat4x4(
				&m_SkinBoneToSimulationMatrices[i],
				XMMatrixIdentity());
			continue;
		}

		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			ModelInstance,
			iTargetBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		_matrix BoneWorldRigid{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			BoneMatrix * TargetWorld,
			BoneWorldRigid))
		{
			return false;
		}

		XMStoreFloat4x4(
			&m_SkinBoneToSimulationMatrices[i],
			BoneWorldRigid *
				InverseAttachmentWorld);
	}

	auto& Desc = m_AnimationConstraintDesc;
	Desc.vecTargetPositions.resize(Bindings.size());
	Desc.vecMaxDistances.resize(Bindings.size());
	for (size_t i = 0;
		i < Bindings.size();
		++i)
	{
		_vector vTarget = XMVectorZero();
		float fTotalWeight{};
		for (const auto& Influence :
			Bindings[i].Influences)
		{
			if (Influence.fWeight <= 0.f ||
				Influence.iSourceBoneIndex >=
					m_SkinBoneToSimulationMatrices.size() ||
				m_ResolvedSkinBoneIndices[
					Influence.iSourceBoneIndex] < 0)
			{
				continue;
			}

			vTarget +=
				XMVector3TransformCoord(
					XMLoadFloat3(
						&Influence.vBoneLocalPosition),
					XMLoadFloat4x4(
						&m_SkinBoneToSimulationMatrices[
							Influence.iSourceBoneIndex])) *
				Influence.fWeight;
			fTotalWeight += Influence.fWeight;
		}

		if (fTotalWeight > FLT_EPSILON)
			vTarget /= fTotalWeight;
		else
			vTarget = XMLoadFloat3(&RestPositions[i]);

		XMStoreFloat3(
			&Desc.vecTargetPositions[i],
			vTarget);
		Desc.vecMaxDistances[i] =
			Bindings[i].fMaxDistance;
	}

	Desc.bUpdateFixedParticles = true;
	Desc.bResetPreviousParticles =
		bResetPreviousParticles ||
		!m_bAnimationConstraintInitialized;
	if (!m_pComNvCloth->
		SetAnimationConstraints(Desc))
	{
		return false;
	}

	m_bAnimationConstraintInitialized = true;
	return true;
}

_bool CNvClothCape::GetTargetBoneMatrix(
	CComModelInstance& ModelInstance,
	int32_t iBoneIndex,
	_matrix& OutBoneMatrix) const
{
	if (iBoneIndex < 0 ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto iIndex =
		static_cast<size_t>(iBoneIndex);
	const auto& CombinedBones =
		ModelInstance.Get_CombinedBoneMatrices();
	if (iIndex < CombinedBones.size())
	{
		OutBoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iIndex]);
		return true;
	}

	const auto& Bones =
		ModelInstance.GetModel()->GetBones();
	if (iIndex >= Bones.size() ||
		!Bones[iIndex])
	{
		return false;
	}

	OutBoneMatrix =
		Bones[iIndex]->
		Get_CombinedTransformationMatrix();
	return true;
}

_bool CNvClothCape::UpdateBodyCollisions()
{
	if (!m_pComNvCloth)
		return false;

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseSimulationWorld =
		XMMatrixInverse(
			&vDeterminant,
			SimulationWorld);
	const float fDeterminant =
		XMVectorGetX(vDeterminant);
	if (!std::isfinite(fDeterminant) ||
		std::abs(fDeterminant) <= 1.e-8f)
	{
		return false;
	}

	const _matrix TargetWorld =
		pTarget->GetTransform().
		GetLoadedCombinedWorldMatrix();

	NVCLOTH_COLLISION_DESC CollisionDesc{};
	CollisionDesc.bContinuousCollision =
		m_bContinuousBodyCollision;
	CollisionDesc.fCollisionMassScale =
		m_fCollisionMassScale;
	CollisionDesc.fFriction =
		m_fCollisionFriction;
	CollisionDesc.vecSpheres.reserve(
		m_BodyCollisionBones.size());

	for (const auto& Binding :
		m_BodyCollisionBones)
	{
		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			*pModelInstance,
			Binding.iBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		const _vector vWorldPosition =
			XMVector3TransformCoord(
				XMVectorZero(),
				BoneMatrix * TargetWorld);
		const _vector vLocalPosition =
			XMVector3TransformCoord(
				vWorldPosition,
				InverseSimulationWorld);

		NVCLOTH_COLLISION_SPHERE Sphere{};
		XMStoreFloat3(
			&Sphere.vCenter,
			vLocalPosition);
		Sphere.fRadius = Binding.fRadius;
		CollisionDesc.vecSpheres.push_back(
			Sphere);
	}

	CollisionDesc.vecCapsules = {
		{ 0, 1 },
		{ 1, 2 },
		{ 1, 3 },
		{ 1, 4 }
	};

	return m_pComNvCloth->SetCollisions(
		CollisionDesc);
}

_bool CNvClothCape::UpdateAttachment(
	_bool bUpdateSimulation)
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	if (m_iAttachBoneIndex < 0 &&
		!ResolveAttachment())
	{
		return false;
	}

	const auto iBoneIndex =
		static_cast<size_t>(m_iAttachBoneIndex);
	const auto& CombinedBones =
		pModelInstance->Get_CombinedBoneMatrices();
	_matrix BoneMatrix{};
	if (iBoneIndex < CombinedBones.size())
	{
		BoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iBoneIndex]);
	}
	else
	{
		const auto& Bones =
			pModelInstance->GetModel()->GetBones();
		if (iBoneIndex >= Bones.size() ||
			!Bones[iBoneIndex])
		{
			return false;
		}
		BoneMatrix =
			Bones[iBoneIndex]->
			Get_CombinedTransformationMatrix();
	}

	_matrix AttachmentWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		BoneMatrix *
			pTarget->GetTransform().
			GetLoadedCombinedWorldMatrix(),
		AttachmentWorld))
	{
		return false;
	}

	XMStoreFloat4x4(
		&m_ParentWorld,
		AttachmentWorld);
	GetTransform().SetParentWorldMatrix(
		m_ParentWorld);
	GetTransform().Update();
	m_bAttachmentInitialized = true;

	if (!bUpdateSimulation ||
		!m_pComNvCloth)
	{
		return true;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vScale{};
	_vector qRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&qRotation,
		&vTranslation,
		SimulationWorld))
	{
		return false;
	}

	_float3 vCurrentPosition{};
	_float4 vCurrentRotation{};
	XMStoreFloat3(
		&vCurrentPosition,
		vTranslation);
	XMStoreFloat4(
		&vCurrentRotation,
		XMQuaternionNormalize(qRotation));

	const float fDistance =
		XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&vCurrentPosition) -
			XMLoadFloat3(
				&m_vPreviousAttachPosition)));
	const _bool bTeleport =
		!m_bSimulationTransformInitialized ||
		fDistance > m_fTeleportDistance;
	if (!m_pComNvCloth->SetSimulationTransform(
		vCurrentPosition,
		vCurrentRotation,
		bTeleport))
	{
		return false;
	}

	if (!UpdateAnimationConstraints(
		*pModelInstance,
		AttachmentWorld,
		bTeleport))
	{
		return false;
	}

	m_vPreviousAttachPosition =
		vCurrentPosition;
	m_bSimulationTransformInitialized = true;
	return true;
}

HRESULT CNvClothCape::Render(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& Context)
{
	if (!pContext ||
		!m_pComCBufferPerObject ||
		!m_pComModelInstance ||
		!m_pComNvCloth ||
		!m_pClothMesh ||
		!m_pVertexShader ||
		!m_pPixelShader)
	{
		return E_FAIL;
	}

	NVCLOTH_GPU_PARTICLE_VIEW ParticleView{};
	if (!m_pComNvCloth->GetGpuParticleView(
		ParticleView) ||
		ParticleView.iParticleCount !=
			m_pClothMesh->GetParticleCount())
	{
		return E_FAIL;
	}

	CB_PER_OBJECT PerObject{};
	PerObject.matWorld =
		*GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&PerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() *
			Context.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext,
		&PerObject,
		sizeof(PerObject))))
	{
		return E_FAIL;
	}

	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(
		m_pVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		m_pVertexShader->GetVertexShader().Get(),
		nullptr,
		0);
	pContext->PSSetShader(
		m_pPixelShader->GetPixelShader().Get(),
		nullptr,
		0);
	pContext->VSSetShaderResources(
		9,
		1,
		&ParticleView.pSRV);

	const auto& Sections =
		m_pClothMesh->GetSections();
	for (uint32_t i = 0;
		i < Sections.size();
		++i)
	{
		const auto& Section = Sections[i];
		if (!Section.pVIBuffer)
			continue;

		ID3D11Buffer* pVertexBuffer =
			Section.pVIBuffer->
				GetVertexBuffer().Get();
		const uint32_t iVertexStride =
			Section.pVIBuffer->
				GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0,
			1,
			&pVertexBuffer,
			&iVertexStride,
			&iOffset);
		pContext->IASetIndexBuffer(
			Section.pVIBuffer->
				GetIndexBuffer().Get(),
			Section.pVIBuffer->
				GetIndexFormat(),
			0);
		pContext->IASetPrimitiveTopology(
			Section.pVIBuffer->
				GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(
			pContext,
			Section.iSourceMeshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext,
			{ 1.f, 1.f, 1.f },
			0.f,
			{ 1.f, 1.f, 1.f },
			0.f,
			1.f);
		pContext->DrawIndexed(
			Section.pVIBuffer->
				GetNumIndices(),
			0,
			0);
	}

	ID3D11ShaderResourceView* pNullSRV{};
	pContext->VSSetShaderResources(
		9,
		1,
		&pNullSRV);
	return S_OK;
}

UPtr<CNvClothCape> CNvClothCape::Create()
{
	auto pInstance =
		ToUPtr(new CNvClothCape{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CNvClothCape::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CNvClothCape{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}
