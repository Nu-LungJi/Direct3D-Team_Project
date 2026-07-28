#include "pch.h"
#include "Monster.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Mon_Weapon.h"
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
			m_Effects[ETOUI(key)] = CGameInstance::Get().Parse_Command(value);
	}

	
	ImGui::Text("bPending : %s", m_bPending == true ? "TRUE" : "FALSE");
	ImGui::Text("Pending AttType : %s", MagicEnumToStringView(m_PendingMonTable.eAttType).data());
	ImGui::Text("Pending HitType : %s", MagicEnumToStringView(m_PendingMonTable.eHitType).data());

	ImGui::Separator();
	ImGui::Text("ActiveHit : %s", m_bActiveHit == true ? "TRUE" : "FALSE");
	ImGui::Text("ActiveHit AttType : %s", MagicEnumToStringView(m_ActiveMonTable.eAttType).data());
	ImGui::Text("ActiveHit HitType : %s", MagicEnumToStringView(m_ActiveMonTable.eHitType).data());

}

HRESULT CMonster::InitializePrototype(void* pArg)
{
	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}


	return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	m_bDonMove = MonDesc->bDonMove;
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
	
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DROP)))
	{
		m_pCharacterMotor->SetUseGravity(true);
	}else m_pCharacterMotor->SetUseGravity(false);
		
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
HRESULT CMonster::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResPixelShader)
		return E_FAIL;

	const auto& vs = m_pResVertexCPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0 || iInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iInstanceCount)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
		return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pModel || !pCPUBonePaletteBuffer)
		return E_FAIL;

	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> combinedPalette(iInstanceCount * 512, identity);
	for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
	{
		const auto& combinedMatrices = Batch.CombinedBoneMatrices[instanceIndex];
		if (combinedMatrices.empty() || combinedMatrices.size() > 512)
			return E_FAIL;

		// DirectXMath로 계산한 CPU Combined 행렬을 VS의 t7 행렬 규약에 맞춘다.
		// CPU 원본은 다른 CPU 기능에서도 사용하므로 업로드 복사본만 전치한다.
		for (uint32_t boneIndex = 0; boneIndex < static_cast<uint32_t>(combinedMatrices.size()); ++boneIndex)
		{
			XMStoreFloat4x4(
				&combinedPalette[instanceIndex * 512 + boneIndex],
				XMMatrixTranspose(
					XMLoadFloat4x4(&combinedMatrices[boneIndex])));
		}
	}

	// CPU가 계산한 CombinedBone palette는 batch당 한 번만 갱신한다.
	ID3D11ShaderResourceView* nullPaletteSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &nullPaletteSRV);
	if (FAILED(pCPUBonePaletteBuffer->UpdateData(
		combinedPalette.data(),
		static_cast<uint32_t>(combinedPalette.size() * sizeof(_float4x4)))))
		return E_FAIL;



	if (FAILED(Bind_InstanceBuffer(pContext)))
		return E_FAIL;
	ID3D11ShaderResourceView* cpuBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	if (!cpuBonePaletteSRV)
		return E_FAIL;

	ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
	if (!skinBonesSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &cpuBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &skinBonesSRV);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);

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
HRESULT CMonster::Bind_InstanceBuffer(ID3D11DeviceContext* pContext)
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

_bool CMonster::Activate_PendingHit()
{
	if (!m_bPending)
		return false;

	if (m_bActiveHit)
		return false;

	m_ActiveMonTable = m_PendingMonTable;
	m_bActiveHit = true;

	m_PendingMonTable = {};
	Clear_PendingHit();
	m_bPending = false;

	return true;
}

void CMonster::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return;

	MON_HIT_INFO HitInfo{};

	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::ADD);
	HitInfo.eAttType = m_eAttType;
	HitInfo.eHitType = eType;
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		HitInfo.iPriority = 5.f;
		break;
	case PLAYER_SKILL_TYPE::ACCIO:
		HitInfo.iPriority = 10.f;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		HitInfo.iPriority = 15.f;
		break;
	case PLAYER_SKILL_TYPE::DESCENDO:
		HitInfo.iPriority = 20.f;
		break;
	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		HitInfo.iPriority = 25.f;
		break;
	case PLAYER_SKILL_TYPE::PROTEGO:
		HitInfo.iPriority = 8.f;
		break;
	}

	if (m_bActiveHit && HitInfo.eHitType == PLAYER_SKILL_TYPE::ATTACK)
	{
		m_PendingMonTable = HitInfo;
		m_bPending = true;
	}
	//현재 pending 가중치보다 낮으면 리턴
	if (m_bPending && HitInfo.iPriority < m_PendingMonTable.iPriority)
		return;
	//현재 잠금된거보다 낮아도 거부
	if (m_bActiveHit &&HitInfo.iPriority <m_ActiveMonTable.iPriority)
		return;
	//새 피격상태 전달
	m_PendingMonTable = HitInfo;
	m_bPending = true;
	
	//우선순위 잠금
	m_ActiveMonTable = HitInfo;
	m_bActiveHit = true;
}

_bool CMonster::Is_Grounded()
{
	return m_pCharacterController->IsGrounded();
}

void CMonster::RunningSkill(_float fTimeDelta)
{
	if (m_bSkill && !m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK)))
	{
		_float fCurrRatio = m_pModelAnimator->GetPlayAnimRatio();

		if (m_eAttType != ATTMON::END && fCurrRatio >= m_fSkillRatio.x && fCurrRatio < m_fSkillRatio.y)
		{
			CGameInstance::Get().Spawn(m_Effects[ETOUI(m_eAttType)], *m_pComTransform->GetWorldMatrix());
			m_eAttType = ATTMON::END;
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
		Check_Table(PLAYER_SKILL_TYPE::ATTACK);
	}
	if (CGameInstance::Get().KeyDown(DIK_Z))
	{
		Check_Table(PLAYER_SKILL_TYPE::ACCIO);
	}
	else if (CGameInstance::Get().KeyDown(DIK_X))
	{
		Check_Table(PLAYER_SKILL_TYPE::DEPULSO);
	}
	else if (CGameInstance::Get().KeyDown(DIK_C))
	{
		Check_Table(PLAYER_SKILL_TYPE::DESCENDO);
	}
	else if (CGameInstance::Get().KeyDown(DIK_V))
	{
		m_PendingMonTable.eHitType = PLAYER_SKILL_TYPE::DEFAULT;
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
		m_PendingMonTable.eHitType = PLAYER_SKILL_TYPE::DEFAULT;
	}
	if (m_iHp <= 0.f)
	{
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEAD), FLAGTYPE::ADD);
	}

	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::HIT)))
		m_fEmissive = 0;

	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE)))
	{
		if (auto pSrc = CGameInstance::Get().GetGameObjectByHandleT<CMon_Weapon>(m_Partes[ETOUI(PARTES::WEAPON)]))
		{
			if (pSrc->Weapon_CallBack())
				m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE), FLAGTYPE::DEL);
		}
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

