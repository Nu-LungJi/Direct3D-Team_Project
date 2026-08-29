#include "pch.h"
#include "WorldAnimal.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CWorldAnimal::CWorldAnimal()
{
}


CWorldAnimal::~CWorldAnimal()
{
}

void CWorldAnimal::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CWorldAnimal::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	m_pResVertexGPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced");
	if (!m_pResVertexGPUSkinningInstancedShader ||
		FAILED(m_pResVertexGPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}

	m_pAnimComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation");
	if (!m_pAnimComputeShader || FAILED(m_pAnimComputeShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CWorldAnimal::Initialize(void* pArg)
{
	auto WorldAgentDesc = static_cast<WORLD_AGENT_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}

	// 월드 동물은 애니메이션 pose 계산과 skinning을 모두 GPU 경로로 처리한다.
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::GPU);

	m_iHp = m_iMaxHp = 10;

	_float3 vRot = WorldAgentDesc->vRot;
	_matrix matRot = XMMatrixRotationX(XMConvertToRadians(vRot.x))
		* XMMatrixRotationY(XMConvertToRadians(vRot.y)) * XMMatrixRotationZ(XMConvertToRadians(vRot.z));
	_vector vFinalRot = XMQuaternionRotationMatrix(matRot);

	GetTransform().SetQuaternion(vFinalRot);
	GetTransform().SetScale(WorldAgentDesc->vScale);
	
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);

	auto pBB = Get_BlackBoard();

	pBB->Set_Value<_float3>(NPC_KEY::STARTPOS, WorldAgentDesc->vStartPos);
	pBB->Set_Value<_float3>(NPC_KEY::ENDPOS, WorldAgentDesc->vEndPos);
	pBB->Set_Value<_float>(NPC_KEY::SPEED, WorldAgentDesc->fSpeed);
	pBB->Set_Value<AGENT_STATE>(NPC_KEY::STATE, AGENT_STATE::IDLE);
	return S_OK;
}

void CWorldAnimal::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
}

void CWorldAnimal::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);


}

void CWorldAnimal::FixedUpdate(E::_float fTimeDelta)
{
	if(nullptr != m_pCharacterMotor)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CWorldAnimal::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}

HRESULT CWorldAnimal::Render_Instanced(
	ID3D11DeviceContext* pContext,
	const E::RENDER_CTX&,
	const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pAnimComputeShader ||
		!m_pResVertexGPUSkinningInstancedShader || !m_pResPixelShader)
	{
		return E_INVALIDARG;
	}

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0)
		return S_OK;
	if (iInstanceCount > 512)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)) ||
		FAILED(m_pComModelInstance->Bind_GPUAnimationSRVs_CS(pContext)) ||
		FAILED(Bind_InstanceBuffer_CS(pContext)) ||
		FAILED(Bind_FinalBoneUAV_CS(pContext)))
	{
		Unbind_GPUAnimation_CS(pContext);
		return E_FAIL;
	}

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(), nullptr, 0);
	pContext->Dispatch(iInstanceCount, 1, 1);
	Unbind_GPUAnimation_CS(pContext);

	if (FAILED(Bind_InstanceBuffer(pContext)) ||
		FAILED(Bind_FinalBoneSRV_VS(pContext)) ||
		FAILED(m_pComModelInstance->Bind_GPUSkinBones_VS(pContext)))
	{
		Unbind_GPUAnimation_VS(pContext);
		return E_FAIL;
	}

	const auto& vs = m_pResVertexGPUSkinningInstancedShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(
		Batch.Key.modelGroup, Batch.Key.modelTag);
	if (!pModel)
	{
		Unbind_GPUAnimation_VS(pContext);
		return E_FAIL;
	}

	const auto& meshes = pModel->GetMeshes();
	const auto& materials = pModel->GetMaterials();
	const uint32_t iMeshCount = std::min<uint32_t>(
		pModel->Get_NumMeshes(), static_cast<uint32_t>(meshes.size()));

	for (uint32_t iMeshIndex = 0; iMeshIndex < iMeshCount; ++iMeshIndex)
	{
		const auto& mesh = meshes[iMeshIndex];
		if (!mesh)
			continue;

		const uint32_t iMaterialIndex = mesh->Get_MaterialIndex();
		if (iMaterialIndex >= materials.size() || !materials[iMaterialIndex])
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			continue;

		E::GPU_SKIN_MESH_CONSTANTS constants{};
		constants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		constants.iVertexCount = mesh->GetNumVertices();
		constants.iSkinBoneCount = skinRange.iSkinBoneCount;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(
			m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0,
			D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			Unbind_GPUAnimation_VS(pContext);
			return E_FAIL;
		}
		memcpy(mapped.pData, &constants, sizeof(constants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);

		ID3D11Buffer* pSkinningCBuffer = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &pSkinningCBuffer);

		ID3D11Buffer* pVertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT iStride = mesh->GetVertexStride();
		const UINT iOffset = 0;
		pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &iStride, &iOffset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext, _float3{ 1.f, 1.f, 1.f }, 0.f,
			_float3{ 1.f, 1.f, 1.f }, m_fDissolve, 1.f);
		pContext->DrawIndexedInstanced(
			mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	Unbind_GPUAnimation_VS(pContext);
	return S_OK;
}

HRESULT CWorldAnimal::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(
		TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	if (!pBuffer || !pBuffer->GetSRV())
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pBuffer->GetSRV().Get();
	pContext->CSSetShaderResources(6, 1, &pSRV);
	return S_OK;
}

HRESULT CWorldAnimal::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(
		TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	if (!pBuffer || !pBuffer->GetUAV())
		return E_FAIL;

	ID3D11ShaderResourceView* pNullSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &pNullSRV);
	ID3D11UnorderedAccessView* pUAV = pBuffer->GetUAV().Get();
	pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);
	return S_OK;
}

HRESULT CWorldAnimal::Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext)
{
	auto pBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(
		TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	if (!pBuffer || !pBuffer->GetSRV())
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pBuffer->GetSRV().Get();
	pContext->VSSetShaderResources(7, 1, &pSRV);
	return S_OK;
}

void CWorldAnimal::Unbind_GPUAnimation_CS(ID3D11DeviceContext* pContext)
{
	ID3D11ShaderResourceView* pNullSRVs[7]{};
	pContext->CSSetShaderResources(0, 7, pNullSRVs);
	ID3D11UnorderedAccessView* pNullUAV = nullptr;
	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);
	pContext->CSSetShader(nullptr, nullptr, 0);
}

void CWorldAnimal::Unbind_GPUAnimation_VS(ID3D11DeviceContext* pContext)
{
	ID3D11ShaderResourceView* pNullSRVs[4]{};
	pContext->VSSetShaderResources(6, 4, pNullSRVs);
}

void CWorldAnimal::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

E::UPtr<CWorldAnimal> CWorldAnimal::Create()
{
	auto pInstance = E::ToUPtr(new CWorldAnimal{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWorldAnimal");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWorldAnimal::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWorldAnimal{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWorldAnimal");
		return nullptr;
	}

	return pInstance;
}
