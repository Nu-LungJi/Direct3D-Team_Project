#include "pch.h"
#include "WiggenweldPotion.h"

#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "ResRasterizerState.h"
#include "Resources.h"

NS_USING(Client)

CWiggenweldPotion::CWiggenweldPotion() = default;

CWiggenweldPotion::CWiggenweldPotion(const CWiggenweldPotion& prototype)
	: CGameObject{ prototype }
	, m_pResConvexGeometry{ prototype.m_pResConvexGeometry }
	, m_pResPhysXMaterial{ prototype.m_pResPhysXMaterial }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CWiggenweldPotion::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	m_pResConvexGeometry = CGameInstance::Get()
		.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
			"./Resources/PhysX/Cooked/SM_Potion_Wiggenweld.pxconvex",
			[]()
			{
				return CResPhysXConvexGeometry::CreateAndLoad(
					"./Resources/PhysX/Cooked/SM_Potion_Wiggenweld.pxconvex");
			});
	CResPhysXMaterial::DESC materialDesc{};
	materialDesc.fStaticFriction = 0.4f;
	materialDesc.fDynamicFriction = 0.35f;
	materialDesc.fRestitution = 0.f;
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad(materialDesc);
	return m_pResConvexGeometry && m_pResPhysXMaterial ? S_OK : E_FAIL;
}

HRESULT CWiggenweldPotion::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_vModelScale = pDesc->vInitialScale;
	GetTransform().SetScale(m_vModelScale);
	GetTransform().Update();

	{
		CComConstantBuffer::DESC desc{};
		desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject", &desc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComStaticModelInstance::DESC desc{};
		desc.sGroupTag = pDesc->sResourceGroup;
		desc.sResTag = "Static_WiggenweldPotion_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
			return E_FAIL;
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		desc.fMass = std::max(pDesc->fMass, 0.001f);
		desc.vPosition = pDesc->vInitialPosition;
		desc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &desc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		CComPxConvexCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResConvex = m_pResConvexGeometry;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.vScale = {
			std::max(std::abs(pDesc->vConvexScale.x), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.y), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.z), 0.001f) };
		desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider",
			"ComPxConvexCollider", &desc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(false) ||
		!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(false))
		return E_FAIL;

	m_eState = STATE::HELD;
	return S_OK;
}

void CWiggenweldPotion::LateUpdate(_float fTimeDelta)
{
	if (m_eState == STATE::DROPPED)
	{
		UpdatePhysicData();
		m_fDroppedElapsed += std::max(fTimeDelta, 0.f);
		if (m_fDroppedElapsed >= POTION_DISSOLVE_DELAY)
		{
			m_bDissolving = true;
			m_fDissolveIntensity = std::clamp(
				(m_fDroppedElapsed - POTION_DISSOLVE_DELAY) /
				POTION_DISSOLVE_DURATION,
				0.f, 1.f);
			if (m_fDissolveIntensity >= 1.f)
			{
				SetPendingDestroyCascade();
				return;
			}
		}
	}

	GetTransform().Update();
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (m_bDissolving || !CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	CGameInstance::Get().Add_Instance(
		m_pComModelInstance,
		*GetTransform().GetCombinedWorldMatrix());
}

HRESULT CWiggenweldPotion::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CWiggenweldPotion::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	ComPtr<ID3D11RasterizerState> previousRasterizer{};
	pContext->RSGetState(previousRasterizer.GetAddressOf());
	const auto noCullRasterizer = CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	if (noCullRasterizer)
		pContext->RSSetState(noCullRasterizer->GetRasterizerState().Get());

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!m_bRenderConfirmed)
	{
		const auto renderLog = std::format(
			"[PlayerPotion] Render reached. meshes={}, scale=({}, {}, {}).\n",
			pModel->Get_NumMeshes(), m_vModelScale.x, m_vModelScale.y,
			m_vModelScale.z);
		DEBUG_LOG(renderLog.c_str());
		m_bRenderConfirmed = true;
	}
	for (uint32_t meshIndex = 0; meshIndex < pModel->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[meshIndex];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset{};
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, meshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f,
			{ 1.f, 0.25f, 0.05f }, m_fDissolveIntensity, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	pContext->RSSetState(previousRasterizer.Get());

	return S_OK;
}

_bool CWiggenweldPotion::SetHeldPose(_fmatrix worldMatrix)
{
	if (m_eState != STATE::HELD || !m_pComPxRigidBody)
		return false;

	_vector scale{};
	_vector rotation{};
	_vector translation{};
	if (!XMMatrixDecompose(&scale, &rotation, &translation, worldMatrix))
		return false;

	_float3 vPosition{};
	_float4 vRotation{};
	XMStoreFloat3(&vPosition, translation);
	XMStoreFloat4(&vRotation, XMQuaternionNormalize(rotation));
	GetTransform().SetScale(m_vModelScale);
	GetTransform().SetPosition(vPosition);
	GetTransform().SetQuaternion(vRotation);
	return m_pComPxRigidBody->SetPose(vPosition, vRotation);
}

_bool CWiggenweldPotion::Drop(
	const _float3& vLinearVelocity,
	const _float3& vAngularVelocity)
{
	if (m_eState != STATE::HELD || !m_pComPxRigidBody || !m_pComPxConvexCollider)
		return false;

	const _float3 vPosition = GetTransform().GetPosition();
	const _float4 vRotation = GetTransform().GetQuaternion();
	if (!m_pComPxRigidBody->SetPose(vPosition, vRotation) ||
		!m_pComPxConvexCollider->SetSimulationEnabled(true) ||
		!m_pComPxConvexCollider->SetQueryEnabled(true) ||
		!m_pComPxRigidBody->SetKinematic(false) ||
		!m_pComPxRigidBody->SetGravityEnabled(true))
		return false;

	if (!m_pComPxRigidBody->SetLinearDamping(0.08f) ||
		!m_pComPxRigidBody->SetAngularDamping(0.8f) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(1.5f) ||
		!m_pComPxRigidBody->SetLinearVelocity(vLinearVelocity) ||
		!m_pComPxRigidBody->SetAngularVelocity(vAngularVelocity))
		return false;

	m_pComPxRigidBody->WakeUp();
	m_eState = STATE::DROPPED;
	m_fDroppedElapsed = 0.f;
	m_fDissolveIntensity = 0.f;
	m_bDissolving = false;
	return true;
}

UPtr<CWiggenweldPotion> CWiggenweldPotion::Create()
{
	auto pInstance = ToUPtr(new CWiggenweldPotion{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CWiggenweldPotion::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CWiggenweldPotion{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
