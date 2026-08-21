#include "pch.h"
#include "PropBarrel.h"
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

CPropBarrel::CPropBarrel() = default;

CPropBarrel::CPropBarrel(const CPropBarrel& prototype)
	: CGameObject{ prototype }
	, m_pResConvexGeometry{ prototype.m_pResConvexGeometry }
	, m_pResPhysXMaterial{ prototype.m_pResPhysXMaterial }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
	, m_sResourceGroup{ prototype.m_sResourceGroup }
{
}

HRESULT CPropBarrel::InitializePrototype(void*)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (!m_pResVertexShader || !m_pResPixelShader ||
		FAILED(m_pResVertexShader->Load()) || FAILED(m_pResPixelShader->Load()))
		return E_FAIL;

	m_pResConvexGeometry = CGameInstance::Get()
		.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
			"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A2.pxconvex",
			[]()
			{
				return CResPhysXConvexGeometry::CreateAndLoad(
					"./Resources/PhysX/Cooked/SM_Prop_Barrel_Breakable_A2.pxconvex");
			});
	m_pResPhysXMaterial = CResPhysXMaterial::CreateAndLoad({});
	return m_pResConvexGeometry && m_pResPhysXMaterial ? S_OK : E_FAIL;
}

HRESULT CPropBarrel::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetRotationEuler(pDesc->vInitialRotation);
	m_sResourceGroup = pDesc->sResourceGroup;
	m_vModelScale = pDesc->vInitialScale;
	m_vDebrisConvexScale = pDesc->vConvexScale;
	m_eState = BARREL_STATE::CREATED;
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
		desc.sResTag = "Static_Prop_Barrel_Resource";
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
		CComPxConvexCollider::DESC desc{};
		desc.pComPxRigidBody = m_pComPxRigidBody;
		desc.pResConvex = m_pResConvexGeometry;
		desc.pResMaterial = m_pResPhysXMaterial;
		desc.vScale = {
			std::max(std::abs(pDesc->vConvexScale.x), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.y), 0.001f),
			std::max(std::abs(pDesc->vConvexScale.z), 0.001f) };
		desc.tFilter = pDesc->tFilter;
		desc.vLocalOffset = {0.f, 0.f, 0.f};
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider",
			"ComPxConvexCollider", &desc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	//if (!m_pComPxRigidBody->SetGravityEnabled(false) ||
	//	!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
	//	!m_pComPxConvexCollider->SetQueryEnabled(false))
	//	return E_FAIL;

	return S_OK;
}

_bool CPropBarrel::DestroyBarrel()
{
	if (m_eState == BARREL_STATE::DESTROYED)
		return true;

	const _float3 vPosition = GetTransform().GetPosition();
	const _float4 vRotation = GetTransform().GetQuaternion();
	const _float3 vScale = GetTransform().GetScale();
	_float4 vDebrisRotation{};
	const _vector vRotateX90 = XMQuaternionRotationAxis(
		XMVectorSet(0.f, 1.f, 0.f, 0.f),
		XMConvertToRadians(-90.f));
	XMStoreFloat4(
		&vDebrisRotation,
		XMQuaternionNormalize(
			XMQuaternionMultiply(XMLoadFloat4(&vRotation), vRotateX90)));
	std::vector<CHandle> spawnedDebrisHandles{};
	spawnedDebrisHandles.reserve(12);

	for (uint32_t i = 1; i <= 12; ++i)
	{
		CPropBarrelDebris::DESC desc{};
		desc.sObjectTag = "PropBarrelDebris_" + std::to_string(i);
		desc.sResourceGroup = m_sResourceGroup;
		desc.sResourceTag =
			"Static_Prop_Barrel_Debris_Resource_" + std::to_string(i);

		desc.sConvexPath = "./Resources/PhysX/Cooked/";
		if (i < 10)
			desc.sConvexPath +=
				"SM_Prop_Barrel_Breakable_A_Fragment2_0" + std::to_string(i);
		else
			desc.sConvexPath +=
				"SM_Prop_Barrel_Breakable_A_Fragment2_" + std::to_string(i);
		desc.sConvexPath += ".pxconvex";

		desc.vInitialPosition = vPosition;
		desc.vInitialScale = vScale;
		desc.vConvexScale = m_vDebrisConvexScale;
		// 원본 GetTransform().GetQuaternion()을 기준으로 X축 +90도를 합성한다.
		desc.vInitialQuaternion = vDebrisRotation;

		const auto hDebris = CGameInstance::Get().AddGameObjectToLayer(
			m_sResourceGroup,
			PROTO_GAMEOBJECT::Prototype_GameObject_PropBarrelDebris,
			"PropBarrelDebris",
			&desc);
		if (!hDebris)
		{
			for (const CHandle& hSpawnedDebris : spawnedDebrisHandles)
			{
				if (auto* pDebris = CGameInstance::Get().GetGameObjectByHandle(
					hSpawnedDebris))
				{
					pDebris->SetPendingDestroy();
				}
			}
			return false;
		}

		spawnedDebrisHandles.push_back(*hDebris);
	}

	m_eState = BARREL_STATE::DESTROYED;
	// 원본 배럴과 Kinematic 파편의 위치가 겹치는지 확인하는 테스트 중이다.
	// 파편을 만든 뒤에도 원본 배럴을 제거하지 않고 함께 렌더링한다.
	return true;
}

void CPropBarrel::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::Separator();
	ImGui::Text("Prop Barrel State: %s",
		m_eState == BARREL_STATE::CREATED ? "Created" : "Destroyed");
	const _bool bCanDestroy =
		m_eState == BARREL_STATE::CREATED && !GetPendingDestroy();
	if (bCanDestroy && ImGui::Button("Destroy Prop Barrel"))
		m_bDestroyRequestedFromGUI = true;
	else if (!bCanDestroy)
		ImGui::TextDisabled("Destroy unavailable");
}

void CPropBarrel::Update(_float)
{
	if (!m_bDestroyRequestedFromGUI)
		return;

	m_bDestroyRequestedFromGUI = false;
	if (!DestroyBarrel())
		DEBUG_LOG("[PropBarrel] Deferred GUI destroy failed.\n");
}

void CPropBarrel::LateUpdate(_float fTimeDelta)
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

HRESULT CPropBarrel::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
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

UPtr<CPropBarrel> CPropBarrel::Create()
{
	auto pInstance = ToUPtr(new CPropBarrel{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPropBarrel::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPropBarrel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
