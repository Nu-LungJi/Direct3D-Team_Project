#include "pch.h"
#include "PropBarrelDebris.h"

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

CPropBarrelDebris::CPropBarrelDebris() = default;

CPropBarrelDebris::CPropBarrelDebris(const CPropBarrelDebris& prototype)
	: CGameObject{ prototype }
	, m_pResConvexGeometry{ prototype.m_pResConvexGeometry }
	, m_pResPhysXMaterial{ prototype.m_pResPhysXMaterial }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CPropBarrelDebris::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;


	//m_pResConvexGeometry = CGameInstance::Get()
	//	.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
	//		"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A_Fragment_01.pxconvex",
	//		[]()
	//		{
	//			return CResPhysXConvexGeometry::CreateAndLoad(
	//				"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A_Fragment_01.pxconvex");
	//		});
	
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
	return /*m_pResConvexGeometry && */m_pResPhysXMaterial ? S_OK : E_FAIL;
}

HRESULT CPropBarrelDebris::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetQuaternion(pDesc->vInitialQuaternion);
	m_vModelScale = pDesc->vInitialScale;
	m_fDissolveDelay = std::max(pDesc->fDissolveDelay, 0.f);
	m_fDissolveDuration = std::max(pDesc->fDissolveDuration, 0.f);
	m_fLifeElapsed = 0.f;
	m_fDissolveIntensity = 0.f;
	m_bDissolving = false;
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
		
		desc.sResTag = pDesc->sResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance",
			"ComModelInstance", &desc, &m_pComModelInstance)))
			return E_FAIL;
	}

	{
		CComPxRigidBody::DESC desc{};
		desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		desc.fMass = std::max(pDesc->fMass, 0.001f);
		desc.vPosition = pDesc->vInitialPosition;
		desc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &desc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		
		m_pResConvexGeometry = CGameInstance::Get()
			.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
				pDesc->sConvexPath,
				[path = pDesc->sConvexPath]()
				{
					return CResPhysXConvexGeometry::CreateAndLoad(path);
				});
		if (!m_pResConvexGeometry)
			return E_FAIL;

		CComPxConvexCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResConvex = m_pResConvexGeometry;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.vScale = {
			std::max(std::abs(pDesc->vConvexScale.x), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.y), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.z), 0.001f) };
		desc.tFilter = pDesc->tFilter;
		//desc.vLocalOffset = { 0.f, 0.f, 1.25f };
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider",
			"ComPxConvexCollider", &desc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(true) ||
		!m_pComPxRigidBody->SetLinearDamping(0.1f) ||
		!m_pComPxRigidBody->SetAngularDamping(0.2f) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(5.f) ||
		!m_pComPxRigidBody->SetLinearVelocity(pDesc->vInitialLinearVelocity) ||
		!m_pComPxRigidBody->SetAngularVelocity(
			pDesc->vInitialAngularVelocityRadians) ||
		!m_pComPxRigidBody->WakeUp())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CPropBarrelDebris::Update(_float fTimeDelta)
{
	if (GetPendingDestroy())
		return;

	m_fLifeElapsed += std::max(fTimeDelta, 0.f);
	if (!m_bDissolving && m_fLifeElapsed >= m_fDissolveDelay)
		m_bDissolving = true;

	if (!m_bDissolving)
		return;

	if (m_fDissolveDuration <= 0.f)
	{
		m_fDissolveIntensity = 1.f;
		SetPendingDestroy();
		return;
	}

	m_fDissolveIntensity = std::clamp(
		(m_fLifeElapsed - m_fDissolveDelay) / m_fDissolveDuration,
		0.f, 1.f);
	if (m_fDissolveIntensity >= 1.f)
		SetPendingDestroy();
}

void CPropBarrelDebris::LateUpdate(_float fTimeDelta)
{
	UpdatePhysicData();
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

HRESULT CPropBarrelDebris::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX&,
	const MODEL_INSTANCE_BATCH& batch)
{
	return m_pComModelInstance
		? m_pComModelInstance->RenderDynamicInstances(pContext, batch)
		: E_FAIL;
}

HRESULT CPropBarrelDebris::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;
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
		return E_FAIL;
	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT), 1,
		m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	ComPtr<ID3D11RasterizerState> previousRasterizer{};
	pContext->RSGetState(previousRasterizer.GetAddressOf());
	const auto noCullRasterizer = CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	if (noCullRasterizer)
		pContext->RSSetState(noCullRasterizer->GetRasterizerState().Get());

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

UPtr<CPropBarrelDebris> CPropBarrelDebris::Create()
{
	auto pInstance = ToUPtr(new CPropBarrelDebris{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPropBarrelDebris::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPropBarrelDebris{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
