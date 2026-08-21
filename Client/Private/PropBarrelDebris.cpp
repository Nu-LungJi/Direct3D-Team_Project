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
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
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
		// 파괴 순간 Dynamic 시뮬레이션으로 위치가 변하는지 확인하기 위한 테스트.
		// 파편을 생성 위치에 고정해 원본 배럴 위치와 직접 비교한다.
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

	// 위치 비교 테스트 중에는 파편을 Kinematic으로 유지한다.
	// Gravity, damping, WakeUp은 Dynamic 전용이라 여기서 호출하면
	// 파편 초기화 자체가 실패한다.

	return S_OK;
}

void CPropBarrelDebris::LateUpdate(_float fTimeDelta)
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

	MAPMESH_INSTANCE_DATA instanceData{};
	XMStoreFloat4x4(
		&instanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox worldBounds{};
	pModel->GetLocalBounds().Transform(
		worldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA occlusionData{};
	occlusionData.worldCenter = worldBounds.Center;
	occlusionData.worldExtents = worldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel, instanceData, occlusionData);
}

HRESULT CPropBarrelDebris::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	/*if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
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
			{ 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	pContext->RSSetState(previousRasterizer.Get());*/

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
