#include "pch.h"
#include "TmbGurdianDead.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
NS_USING(Client)

CTmbGurdianDead::CTmbGurdianDead()
	: CGameObject{}
{
}

CTmbGurdianDead::~CTmbGurdianDead()
{
}

void CTmbGurdianDead::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::Text(
		"Activated: %s",
		m_bActivated ? "TRUE" : "FALSE");
	ImGui::Text(
		"Socket Attached: %s",
		m_bSocketAttached ? "TRUE" : "FALSE");
	ImGui::Text(
		"Render Enabled: %s",
		m_bRenderEnabled ? "TRUE" : "FALSE");
}

_bool CTmbGurdianDead::ActivatePhysics()
{
	if (m_bActivated)
		return true;

	if (!m_pComPxRigidBody ||
		!m_pComPxConvexCollider)
	{
		return false;
	}

	const _float3 vPosition =
		GetTransform().GetPosition();
	const _float4 vRotation =
		GetTransform().GetQuaternion();

	if (!m_pComPxRigidBody->SetPose(
			vPosition,
			vRotation) ||
		!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}) ||
		!m_pComPxConvexCollider
			->SetSimulationEnabled(true) ||
		!m_pComPxConvexCollider
			->SetQueryEnabled(true) ||
		!m_pComPxRigidBody
			->SetGravityEnabled(true) ||
		!m_pComPxRigidBody->WakeUp())
	{
		m_pComPxConvexCollider
			->SetSimulationEnabled(false);
		m_pComPxConvexCollider
			->SetQueryEnabled(false);
		m_pComPxRigidBody
			->SetGravityEnabled(false);
		m_pComPxRigidBody->PutToSleep();
		return false;
	}

	m_bSocketAttached = false;
	m_bActivated = true;
	return true;
}

_bool CTmbGurdianDead::ApplyBonePose(
	_fmatrix matSocketWorld,
	_fmatrix matInverseBind)
{
	if (m_bActivated)
		return false;

	if (!m_pComPxRigidBody)
		return false;

	const _matrix matWorld =
		matInverseBind *
		matSocketWorld;

	_vector vScale{};
	_vector vRotation{};
	_vector vPosition{};
	if (!XMMatrixDecompose(
		&vScale,
		&vRotation,
		&vPosition,
		matWorld))
	{
		return false;
	}

	_float3 vWorldScale{};
	_float4 vWorldRotation{};
	_float3 vWorldPosition{};
	XMStoreFloat3(&vWorldScale, vScale);
	XMStoreFloat4(
		&vWorldRotation,
		XMQuaternionNormalize(vRotation));
	XMStoreFloat3(&vWorldPosition, vPosition);

	GetTransform().SetPosition(vWorldPosition);
	GetTransform().SetQuaternion(vWorldRotation);
	GetTransform().SetScale(vWorldScale);
	GetTransform().Update();

	m_bSocketAttached = true;
	return m_pComPxRigidBody->SetPose(
		vWorldPosition,
		vWorldRotation);
}

HRESULT CTmbGurdianDead::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTmbGurdianDead::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	const auto* pDesc =
		static_cast<TMBGURDIAN_DEAD_DESC*>(pArg);

	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().SetQuaternion(pDesc->vInitialQuaternion);
	GetTransform().SetScale(pDesc->vInitialScale);
	GetTransform().Update();

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->sResourceGroup;
		Desc.sResTag = pDesc->DebrisResTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		Desc.fMass = std::max(pDesc->fMass, 0.001f);
		Desc.vPosition = pDesc->vInitialPosition;
		Desc.vRotation = pDesc->vInitialQuaternion;
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody",
			&Desc,
			&m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	{
		const std::string sConvexPath =
			pDesc->DebrisConvex;
		if (sConvexPath.empty())
			return E_INVALIDARG;

		auto pConvexResource = CGameInstance::Get()
			.GetOrCreateResourceByPath<
				CResPhysXConvexGeometry>(
				sConvexPath,
				[sConvexPath]()
				{
					return CResPhysXConvexGeometry::
						CreateAndLoad(sConvexPath);
				});
		if (!pConvexResource)
			return E_FAIL;

		CComPxConvexCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComPxRigidBody;
		Desc.pResConvex = std::move(pConvexResource);
		Desc.pResMaterial =
			CResPhysXMaterial::CreateAndLoad({});
		Desc.vScale = {
			std::max(
				std::abs(pDesc->vConvexScale.x),
				0.001f),
			std::max(
				std::abs(pDesc->vConvexScale.y),
				0.001f),
			std::max(
				std::abs(pDesc->vConvexScale.z),
				0.001f)
		};
		Desc.tFilter = pDesc->tFilter;
		if (!Desc.pResMaterial ||
			FAILED(AddComponentFromProto(
				"PHYSX",
				"Prototype_Component_ComPxConvexCollider",
				"ComPxConvexCollider",
				&Desc,
				&m_pComPxConvexCollider)))
		{
			return E_FAIL;
		}
	}

	if (!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(false) ||
		!m_pComPxRigidBody->SetGravityEnabled(false) ||
		!m_pComPxRigidBody->PutToSleep())
	{
		return E_FAIL;
	}

	m_bActivated = false;
	return S_OK;
}

void CTmbGurdianDead::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTmbGurdianDead::Update(E::_float fTimeDelta)
{
}

void CTmbGurdianDead::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bRenderEnabled)
		return;

	if (!m_bActivated && !m_bSocketAttached)
		return;

	if (m_bActivated)
		UpdatePhysicData();
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTmbGurdianDead::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

	uint32_t   iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];


		ID3D11Buffer* vertexBuffers[] = {
			  viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
		   viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
		   0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);   // EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}


E::UPtr<CTmbGurdianDead> CTmbGurdianDead::Create()
{
	auto pInstance = E::ToUPtr(new CTmbGurdianDead{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTmbGurdianDead");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTmbGurdianDead::Clone(void* pArg)
{
	auto   pInstance = E::ToUPtr(new CTmbGurdianDead{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTmbGurdianDead");
		return nullptr;
	}

	return pInstance;
}
