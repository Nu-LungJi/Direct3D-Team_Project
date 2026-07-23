#include "pch.h"
#include "TestDynamic.h"

#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr char TEST_DYNAMIC_CONVEX_PATH[] =
		"./Resources/PhysX/Cooked/SM_oil_barrel_0001.pxconvex";
}

CTestDynamic::CTestDynamic()
	: CGameObject{}
{
}

CTestDynamic::CTestDynamic(const CTestDynamic& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CTestDynamic::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
		return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CTestDynamic::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	const auto* desc = static_cast<DESC*>(pArg);
	GetTransform().SetPosition(desc->vInitialPosition);
	GetTransform().SetRotationEuler(desc->vInitialRotation);
	GetTransform().SetScale(desc->vInitialScale);

	{
		CComConstantBuffer::DESC bufferDesc{};
		bufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
			&bufferDesc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComStaticModelInstance::DESC modelDesc{};
		modelDesc.sGroupTag = "LEVEL_CREATURE";
		//"LEVEL_CREATURE", "Static_OilBarrel_Resource"
		modelDesc.sResTag = "Static_OilBarrel_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&modelDesc, &m_pComModelInstance)))
			return E_FAIL;
	}

	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		rigidBodyDesc.fMass = std::max(desc->fMass, 0.001f);
		rigidBodyDesc.vPosition = desc->vInitialPosition;
		rigidBodyDesc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody",
			&rigidBodyDesc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		auto convexResource = CGameInstance::Get()
			.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
				TEST_DYNAMIC_CONVEX_PATH,
				[]() { return CResPhysXConvexGeometry::CreateAndLoad(TEST_DYNAMIC_CONVEX_PATH); });
		if (!convexResource)
			return E_FAIL;

		CComPxConvexCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResConvex = std::move(convexResource);
		colliderDesc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		colliderDesc.vScale = {
			std::max(std::abs(desc->vConvexScale.x), 0.001f),
			std::max(std::abs(desc->vConvexScale.y), 0.001f),
			std::max(std::abs(desc->vConvexScale.z), 0.001f)
		};
		colliderDesc.tFilter = desc->tFilter;
		if (!colliderDesc.pResMaterial || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider", "ComPxConvexCollider",
			&colliderDesc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(true))
		return E_FAIL;

	return S_OK;
}

void CTestDynamic::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestDynamic::Update(E::_float fTimeDelta)
{
}

void CTestDynamic::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel->HasLocalBounds())
		return;

	MAPMESH_INSTANCE_DATA InstanceData{};
	XMStoreFloat4x4(
		&InstanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox WorldBounds{};
	pModel->GetLocalBounds().Transform(
		WorldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA OcclusionData{};
	OcclusionData.worldCenter = WorldBounds.Center;
	OcclusionData.worldExtents = WorldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel,
		InstanceData,
		OcclusionData);
}

HRESULT CTestDynamic::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto model = m_pComModelInstance->GetModel();
	if (!model)
		return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(
			viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, i);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

_bool CTestDynamic::ApplyPushForce(const _float3& vDirection, _float fStrength)
{
	if (!m_pComPxRigidBody || fStrength <= 0.f)
		return false;

	const _vector vDirectionVector = XMLoadFloat3(&vDirection);
	if (XMVectorGetX(XMVector3LengthSq(vDirectionVector)) <= FLT_EPSILON)
		return false;

	_float3 vNormalizedDirection{};
	XMStoreFloat3(&vNormalizedDirection, XMVector3Normalize(vDirectionVector));
	return m_pComPxRigidBody->AddForce({
		vNormalizedDirection.x * fStrength,
		vNormalizedDirection.y * fStrength,
		vNormalizedDirection.z * fStrength });
}

E::UPtr<CTestDynamic> CTestDynamic::Create()
{
	auto instance = E::ToUPtr(new CTestDynamic{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CTestDynamic");
		return nullptr;
	}
	return instance;
}

E::UPtr<E::CPrototype> CTestDynamic::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTestDynamic{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CTestDynamic");
		return nullptr;
	}
	return instance;
}
