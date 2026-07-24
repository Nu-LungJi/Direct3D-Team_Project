#include "pch.h"
#include "Monster.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
NS_USING(Client)

CMonster::CMonster()
{
}


CMonster::~CMonster()
{
}

void CMonster::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
	ImGui::DragFloat("EE", &ff, 0, 1);
	ImGui::DragFloat3("ff", reinterpret_cast<_float*>(&m_f), 0, 100);

	
	ImGui::Text("ReLoad Data");
	for (auto& [key, value] : m_ParticleData)
	{
		if (ImGui::Button(MagicEnumToStringView(key).data()))
			test[ETOUI(key)] = CGameInstance::Get().Parse_Command(value);
	}
}

HRESULT CMonster::InitializePrototype(void* pArg)
{

	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	//m_pResVertexShader = CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	//m_pResPixelShader = CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResVertexInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced");
	if (!m_pResVertexInstancedShader || FAILED(m_pResVertexInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}


	m_pAnimComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation");
	if (FAILED(m_pAnimComputeShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	return S_OK;
}

void CMonster::PriorityUpdate(E::_float fTimeDelta)
{
	m_pMoveIntent->ClearMoveIntent();
	m_pMoveIntent->ClearFacingIntent();
	CGameInstance::Get().AddColliderGroup("CollMonster", m_pComCollider->Get());
	m_pComCollider->Get()->Transform(GetTransform().GetLoadedCombinedWorldMatrix());
	__super::PriorityUpdate(fTimeDelta);
	if (CGameInstance::Get().KeyDown(DIK_1))
		Set_Damage(10);
	Flag_Check(fTimeDelta);
	m_pBeHavior->Update(fTimeDelta);
	RunningSkill(fTimeDelta);
}

void CMonster::Update(E::_float fTimeDelta)
{
	//파티클 테스트용
	if (CGameInstance::Get().KeyDown(DIK_0))
		m_bSkill = false;
	__super::Update(fTimeDelta);

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0)
		m_pModelAnimator->Update(fTimeDelta);

	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::HIT)))
		m_fEmissive = 0;

	EmissiveFadeOut(fTimeDelta);
	m_pBeHavior->AbortNode();
}

void CMonster::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	const _float3 vControllerPosition = m_pCharacterController->GetPosition();
	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	IsHit();
	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());

		return;
	}
}

HRESULT CMonster::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}
	const auto& vs = m_pResVertexShader;
	//!m_pComModelInstance->GetModel()->GetAnimations().empty()
	//? m_pResVertexShader
	//: m_pResVertexNonAnimShader;

	const auto& ps = m_pResPixelShader;
	//!m_pComModelInstance->GetModel()->GetAnimations().empty()
	//? m_pResPixelShader
	//: m_pResPixelNonAnimShader;


	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
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
			if (!m_pComModelInstance->GetModel()->GetAnimations().empty())
				if (FAILED(m_pComModelInstance->Bind_BoneMatrices(pContext, i))) {
					return E_FAIL;
				}
		}

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}
	return S_OK;
}
HRESULT CMonster::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext)
		return E_INVALIDARG;

	const uint32_t iInstanceCount = Batch.Instances.size();

	if (iInstanceCount == 0)
		return S_OK;



	if (!m_pAnimComputeShader || !m_pAnimComputeShader->GetComputeShader())
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Compute Shader
	// -------------------------------------------------

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
	{
		return E_FAIL;
	}

	// CS t0 ~ t5
	if (FAILED(m_pComModelInstance->Bind_GPUAnimationSRVs_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS t6
	if (FAILED(Bind_InstanceBuffer_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS u0
	if (FAILED(Bind_FinalBoneUAV_CS(pContext)))
	{
		return E_FAIL;
	}

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(), nullptr, 0);

	/*
	 * 한 Thread Group = 한 인스턴스라는 전제.
	 */
	pContext->Dispatch(iInstanceCount, 1, 1);

	if (FAILED(Unbind_AnimationCompute(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Vertex Shader용 SRV
	// -------------------------------------------------

	// VS t6 = InstanceData
	if (FAILED(Bind_InstanceBuffer_VS(pContext)))
	{
		return E_FAIL;
	}

	// VS t7 = Compute 결과 FinalBoneMatrix
	if (FAILED(Bind_FinalBoneSRV_VS(pContext)))
	{
		return E_FAIL;
	}
	if (FAILED(m_pComModelInstance->Bind_GPUSkinBones_VS(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Graphics Shader
	// -------------------------------------------------

	const auto& vs = m_pResVertexInstancedShader;

	const auto& ps = m_pResPixelShader;

	if (!vs || !ps)
		return E_FAIL;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());

	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);

	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);

	const uint32_t iNumMeshes = pModel->Get_NumMeshes();

	for (uint32_t iMeshIndex = 0; iMeshIndex < iNumMeshes; ++iMeshIndex)
	{
		const auto& viBuffer = pModel->GetMeshes()[iMeshIndex];

		if (!viBuffer)
			continue;

		ID3D11Buffer* vertexBuffers[] =
		{
			viBuffer->GetVertexBuffer().Get()
		};

		UINT strides[] =
		{
			viBuffer->GetVertexStride()
		};

		UINT offsets[] =
		{
			0
		};

		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);

		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		E::GPU_SKIN_MESH_CONSTANTS skinConstants{};
		skinConstants.iSkinBoneOffset = pModel->Get_GPUMeshSkinRange(iMeshIndex).iSkinBoneOffset;
		D3D11_MAPPED_SUBRESOURCE mappedResource{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return E_FAIL;
		memcpy(mappedResource.pData, &skinConstants, sizeof(skinConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* pSkinMeshCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &pSkinMeshCB);

		//"R": 1.0,
		//	"G" : 0.933333,
		//	"B" : 0.592157,
		//1.2f, 0.7f, 0.f
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 0.585,0.685,1 }, m_fEmissive, { 1.f, 1.f, 1.f }, 0.f, 1.f);

		pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	if (FAILED(Unbind_AnimationVS(pContext)))
	{
		return E_FAIL;
	}

	return S_OK;
}
HRESULT CMonster::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount = static_cast<uint32_t>(Instances.size());

	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer = pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	/*
	 * 이전 Batch에서 VS/CS에 연결되어 있을 수 있으므로
	 * Map 전에 SRV를 해제한다.
	 */
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->CSSetShaderResources(6, 1, &pNullSRV);

	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;
	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}
HRESULT CMonster::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	pContext->CSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}
HRESULT CMonster::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11UnorderedAccessView* pUAV = pStructuredBuffer->GetUAV().Get();

	if (!pUAV)
		return E_FAIL;

	// 이전 Draw에서 FinalBone 버퍼가 VS의 SRV로
	// 연결되어 있었다면 먼저 연결 해제
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->VSSetShaderResources(7, 1, &pNullSRV);

	// CS의 u0 슬롯에 출력 UAV 연결
	pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);

	return S_OK;
}
HRESULT CMonster::Unbind_AnimationCompute(ID3D11DeviceContext* pContext)
{
	// CS t0 ~ t6 SRV 해제
	ID3D11ShaderResourceView* pNullSRVs[7] =
	{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	pContext->CSSetShaderResources(0, 7, pNullSRVs);

	// CS u0 UAV 해제
	ID3D11UnorderedAccessView* pNullUAV = nullptr;

	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);

	// Compute Shader 자체도 해제
	pContext->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}
HRESULT CMonster::Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}
HRESULT CMonster::Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t7 슬롯에 Compute 결과 연결
	pContext->VSSetShaderResources(7, 1, &pSRV);

	return S_OK;
}
HRESULT CMonster::Unbind_AnimationVS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return E_INVALIDARG;

	ID3D11ShaderResourceView* pNullSRVs[3]{};

	pContext->VSSetShaderResources(6, 3, pNullSRVs);

	return S_OK;
}
void CMonster::RunningSkill(_float fTimeDelta)
{
	if (m_bSkill && !m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK)))
	{
		_float fCurrRatio = m_pModelAnimator->GetPlayAnimRatio();

		if (m_MonTable.eAttType != ATTMON::END && fCurrRatio >= m_fSkillRatio.x && fCurrRatio < m_fSkillRatio.y)
		{
			CGameInstance::Get().Spawn(test[ETOUI(m_MonTable.eAttType)], *m_pComTransform->GetWorldMatrix());
			m_MonTable.eAttType = ATTMON::END;
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK), FLAGTYPE::ADD);
			
		}
		if (fCurrRatio >= 1.f)
		{
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK), FLAGTYPE::DEL);
			m_bSkill = false;
		}
			
	}
}
void CMonster::IsHit()
{
	if (CGameInstance::Get().KeyDown(DIK_2))
	{
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::ADD);
	}
	if (CGameInstance::Get().KeyDown(DIK_Z))
	{
		m_MonTable.eHitType = HITMON::HIT_1;
	}
	else if (CGameInstance::Get().KeyDown(DIK_X))
	{
		m_MonTable.eHitType = HITMON::HIT_2;
	}
	else if (CGameInstance::Get().KeyDown(DIK_C))
	{
		m_MonTable.eHitType = HITMON::HIT_3;
	}
	else if (CGameInstance::Get().KeyDown(DIK_V))
	{
		m_MonTable.eHitType = HITMON::END;
	}

}
void CMonster::Flag_Check(_float fTimeDelta)
{
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::HIT) | ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
	{
		m_bEmissive = true;
		StartEmissive();
	}
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::ABORT)))
	{
		m_MonTable.eHitType = HITMON::END;
	}
	if (m_iHp <= 0.f)
	{
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEAD), FLAGTYPE::ADD);
	}
}
void CMonster::EmissiveFadeOut(_float fTimeDelta)
{
	if (m_bEmissive)
	{
		m_bWork = true;
		m_fTimeTick += fTimeDelta;

		_float t = m_fTimeTick / 0.5f;

		m_fEmissive = std::lerp(m_fPreEmissive, 0, t);
		if (t >= 1.f)
		{
			m_bWork = m_bEmissive = false;
			m_fTimeTick = m_fEmissive = 0;
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE), FLAGTYPE::DEL);
		}

	}

}

