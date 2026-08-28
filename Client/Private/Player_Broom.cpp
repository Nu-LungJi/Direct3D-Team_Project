#include "pch.h"
#include "Player_Broom.h"

#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "Resources.h"
#include "ResModelBone.h"
#include "ResTexture2D.h"

NS_USING(Client)

CPlayer_Broom::~CPlayer_Broom()
{
	if (m_iSpeedLineEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().StopEffect(m_iSpeedLineEffectID);
}

HRESULT CPlayer_Broom::InitializePrototype(void* pArg)
{
	m_pVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	m_pPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (!m_pVertexShader || !m_pPixelShader ||
		FAILED(m_pVertexShader->Load()) || FAILED(m_pPixelShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CPlayer_Broom::Initialize(void* pArg)
{
	auto* pDesc = static_cast<BROOM_DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	m_hParent = pDesc->hParent;
	m_iSocketBoneIndex = pDesc->iSocketBoneIndex;
	m_bVisible = pDesc->bVisible;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CComConstantBuffer::DESC bufferDesc{};
	bufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer",
		"ComCBufferPerObject", &bufferDesc, &m_pObjectBuffer)))
		return E_FAIL;

	CComModelInstance::DESC modelDesc{};
	modelDesc.sGroupTag = pDesc->sLevelTag;
	modelDesc.sResTag = pDesc->sResourceTag;
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance",
		"ComCModelIntance", &modelDesc, &m_pModelInstance)))
		return E_FAIL;

	// 이 모델은 애니메이션 클립이 없으므로 Animator가 기본 포즈를 계산해 주지 않는다.
	// 로드된 로컬 본 행렬로 바인드 포즈의 Combined Bone 팔레트를 한 번 구성한다.
	const auto& bones = m_pModelInstance->GetModel()->GetBones();
	auto& combinedBones = m_pModelInstance->Get_CombinedBoneMatrices();
	combinedBones.resize(bones.size());
	for (size_t i = 0; i < bones.size(); ++i)
	{
		if (!bones[i])
			return E_FAIL;
		bones[i]->Update_CombinedTransformationMatrix(bones, XMMatrixIdentity());
		combinedBones[i] = *bones[i]->Get_CombinedTransformationMatrixPtr();
	}

	XMStoreFloat4x4(&m_ParentMatrix, XMMatrixIdentity());
	GetTransform().SetScale(pDesc->vScale);
	// 원본 빗자루의 길이 축(Y)을 플레이어 진행 축(Z)에 맞춘다.
	GetTransform().SetRotationEuler({ 12.f, 0.f, 0.f });
	GetTransform().SetPosition(_float3{ 0.f, m_fCurrentHeightOffset, -0.5f });

	m_iBroomEndBoneIndex = m_pModelInstance->GetModel()->Get_BoneIndex("thistleSocket");
	return S_OK;
}

void CPlayer_Broom::PriorityUpdate(_float fTimeDelta)
{
}

void CPlayer_Broom::Update(_float fTimeDelta)
{
	const _float fBlendRatio = 1.f - std::exp(
		-m_fHeightBlendResponse * std::max(fTimeDelta, 0.f));
	m_fCurrentHeightOffset = std::lerp(
		m_fCurrentHeightOffset, m_fTargetHeightOffset, fBlendRatio);
	GetTransform().SetPosition(_float3{ 0.f, m_fCurrentHeightOffset, -0.5f });


}

void CPlayer_Broom::SetMovementRatio(_float fRatio)
{
	// 정지 시 -1, 출발 구간에서는 약 -0.7, 달리기 속도에 도달하면 -0.5를 유지한다.
	m_fTargetHeightOffset = std::lerp(-1.f, -0.5f, std::clamp(fRatio, 0.f, 1.f));
}

void CPlayer_Broom::SetBoostEffectRatio(_float fRatio)
{
	m_fBoostEffectRatio = std::clamp(fRatio, 0.f, 1.f);
}

_bool CPlayer_Broom::TryBuildAttachmentParentWorld(
	_matrix& OutParentWorld) const
{
	auto* pPlayer = CGameInstance::Get().GetGameObjectByHandle(m_hParent);
	if (!pPlayer)
		return false;

	auto* pPlayerModel = pPlayer->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pPlayerModel)
		return false;

	const auto& boneMatrices = pPlayerModel->Get_CombinedBoneMatrices();
	if (m_iSocketBoneIndex < 0 ||
		static_cast<size_t>(m_iSocketBoneIndex) >= boneMatrices.size())
	{
		return false;
	}

	_matrix socketMatrix = XMLoadFloat4x4(&boneMatrices[m_iSocketBoneIndex]);
	for (uint32_t i = 0; i < 3; ++i)
		socketMatrix.r[i] = XMVector3Normalize(socketMatrix.r[i]);

	OutParentWorld =
		socketMatrix * pPlayer->GetTransform().GetLoadedWorldMatrix();
	return true;
}

_bool CPlayer_Broom::TryBuildAttachedWorldMatrix(
	_matrix& OutWorld) const
{
	_matrix ParentWorld{};
	if (!TryBuildAttachmentParentWorld(ParentWorld))
		return false;

	const auto& Transform = GetTransform();
	const _matrix LocalWorld =
		XMMatrixScalingFromVector(Transform.GetLoadedScale()) *
		XMMatrixRotationQuaternion(Transform.GetLoadedQuaternion()) *
		XMMatrixTranslationFromVector(Transform.GetLoadedPostion());
	OutWorld = LocalWorld * ParentWorld;
	return true;
}

void CPlayer_Broom::LateUpdate(_float fTimeDelta)
{
	_matrix ParentWorld{};
	if (!TryBuildAttachmentParentWorld(ParentWorld))
		return;

	XMStoreFloat4x4(&m_ParentMatrix, ParentWorld);
	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();

	auto* pPlayer = CGameInstance::Get().GetGameObjectByHandle(m_hParent);
	if (!pPlayer)
		return;

	const _bool bBoostEffectActive = m_bVisible && m_fBoostEffectRatio > 0.05f;
	if (bBoostEffectActive)
	{
		if (m_iSpeedLineEffectID == INVALID_EFFECT_INSTANCE_ID)
		{
			m_iSpeedLineEffectID = CGameInstance::Get().PlayEffect(
				"Broom_Boost_SpeedLines", *pPlayer->GetTransform().GetWorldMatrix());
		}
		else
		{
			CGameInstance::Get().SetEffectWorldMatrix(
				m_iSpeedLineEffectID, *pPlayer->GetTransform().GetWorldMatrix());
		}
	}
	else
	{
		if (m_iSpeedLineEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().StopEffect(m_iSpeedLineEffectID);
			m_iSpeedLineEffectID = INVALID_EFFECT_INSTANCE_ID;
		}
	}

	if (!m_bVisible)
		return;
	//이펙트
	{
		if (m_iBroomEndBoneIndex >= 0)
		{
			const auto& broomBoneMatrices = m_pModelInstance->Get_CombinedBoneMatrices();

			if (static_cast<size_t>(m_iBroomEndBoneIndex) < broomBoneMatrices.size())
			{
				m_fSpawnTime += fTimeDelta;

				if (m_fSpawnTime >= 0.08f)
				{
					m_fSpawnTime -= 0.08f;

					_matrix matBroomEndWorld = XMLoadFloat4x4(&broomBoneMatrices[m_iBroomEndBoneIndex]) * GetTransform().GetLoadedCombinedWorldMatrix();

					_float4x4 effectWorld{};
					XMStoreFloat4x4(&effectWorld, matBroomEndWorld);

					CGameInstance::Get().Spawn("BroomParticle.json", effectWorld);
				}
			}
		}
	}
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
}

HRESULT CPlayer_Broom::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pModelInstance || !m_pObjectBuffer)
		return E_FAIL;

	CB_PER_OBJECT objectBuffer{};
	objectBuffer.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&objectBuffer.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pObjectBuffer->MapDiscard(pContext, &objectBuffer, sizeof(objectBuffer))))
		return E_FAIL;

	pContext->VSSetConstantBuffers(0, 1, m_pObjectBuffer->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pObjectBuffer->GetAdressOfBuffer());
	pContext->IASetInputLayout(m_pVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pPixelShader->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

	for (uint32_t i = 0; i < pModel->Get_NumMeshes(); ++i)
	{
		const auto& mesh = pModel->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		if (FAILED(m_pModelInstance->Bind_BoneMatrices(pContext, i)))
			return E_FAIL;
		m_pModelInstance->Bind_Textures(pContext, i);
	
		if (auto white = CGameInstance::Get().GetResourceFirst<CResTexture2D>(
			"DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE"))
			pContext->PSSetShaderResources(0, 1, white->GetSRV().GetAddressOf());
		m_pModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

HRESULT CPlayer_Broom::Render_Shadow(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pModelInstance || !m_pObjectBuffer)
		return E_FAIL;

	CB_PER_OBJECT objectBuffer{};
	objectBuffer.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&objectBuffer.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pObjectBuffer->MapDiscard(pContext, &objectBuffer, sizeof(objectBuffer))))
		return E_FAIL;

	pContext->VSSetConstantBuffers(0, 1, m_pObjectBuffer->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pObjectBuffer->GetAdressOfBuffer());
	auto pModel = m_pModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

	for (const auto& mesh : pModel->GetMeshes())
	{
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const uint32_t stride = mesh->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}
	return S_OK;
}

bool CPlayer_Broom::GetShadowBounds(BoundingBox& outBounds) const
{
	return false;
}

UPtr<CPlayer_Broom> CPlayer_Broom::Create()
{
	auto pInstance = ToUPtr(new CPlayer_Broom{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPlayer_Broom::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPlayer_Broom{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
