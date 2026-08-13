#include "pch.h"
#include "Weapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "Trail_CPU.h"
NS_USING(Client)

CWeapon::CWeapon()
	: CGameObject{}
{
}

CWeapon::~CWeapon()
{
}

void CWeapon::UpdateGUI()
{
	static _float4 weaponColor{};
	static _float3 weaponEmissiveColor{};
	static _float weaponEIntensity{};
	CGameObject::UpdateGUI();

	ImGui::Begin("WEAPON Trail");

	ImGui::ColorEdit4("Color", &weaponColor.x);
	ImGui::ColorEdit3("Emissive", &weaponEmissiveColor.x);
	ImGui::DragFloat("Emissive Intensity", &weaponEIntensity);

	if (ImGui::Button("Apply DashTrail")) {
		auto a = CGameInstance::Get().GetParticle("PlayerDashTrail1_CPU", "PlayerDashTrail1_CPU");
		static_cast<CTrail_CPU*>(a)->SetColor(weaponColor);
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(weaponEmissiveColor.x, weaponEmissiveColor.y, weaponEmissiveColor.z, weaponEIntensity));
	}

	ImGui::DragFloat("Distance", &m_fDistanceOffeset);

	ImGui::End();
}

HRESULT CWeapon::InitializePrototype(void* pArg)
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

HRESULT CWeapon::Initialize(void* pArg)
{
	
	auto pDesc = static_cast<WEAPON_DESC*>(pArg);
	m_iBoneSocketIndex = pDesc->iBoneIndex;
	m_ParentHandle	   = pDesc->ParentHandle;
	
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

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
		Desc.sGroupTag = pDesc->LevelTag;
		Desc.sResTag   = pDesc->WeaponName;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	XMStoreFloat4x4(&m_ParentMatrix, XMMatrixIdentity());
	//test = CGameInstance::Get().Parse_Command("PlayerDash.json");

	return S_OK;
}

void CWeapon::PriorityUpdate(E::_float fTimeDelta)
{
}

void CWeapon::Update(E::_float fTimeDelta)
{
	if (CGameInstance::Get().KeyDown(DIK_SPACE)) {
		m_fSpwanPos = m_pComTransform->GetPosition();
		m_bDashing = !m_bDashing;
	}
	_float3 vstart1, vend1;
	_float3 vstart2, vend2;


	//vstart1 = vend;
	//vend1 = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y-1.f , m_pComTransform->GetPosition().z );
	//vstart2 = vend1;
	//vend2= _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y-1.5f , m_pComTransform->GetPosition().z );
	//CGameInstance::Get().AddTrailPoint("PlayerDashSmoke_CPU", "PlayerDashSmoke_CPU", vstart1, vend1	);
	//CGameInstance::Get().AddTrailPoint("PlayerDashTrail2_CPU", "PlayerDashTrail2_CPU", vstart2, vend2);

	if (CGameInstance::Get().KeyPressing(DIK_HOME))
		m_pComTransform->GoUp(fTimeDelta * 15);
	if (CGameInstance::Get().KeyPressing(DIK_END))
		m_pComTransform->GoDown(fTimeDelta * 15);
	if (CGameInstance::Get().KeyPressing(DIK_UP))
		m_pComTransform->GoStraight(fTimeDelta * 15);
	if (CGameInstance::Get().KeyPressing(DIK_LEFT))
		m_pComTransform->GoRight(fTimeDelta * -15);
	if (CGameInstance::Get().KeyPressing(DIK_DOWN))
		m_pComTransform->GoBackward(fTimeDelta * 15);
	if (CGameInstance::Get().KeyPressing(DIK_RIGHT))
		m_pComTransform->GoRight(fTimeDelta * 15);
	if (CGameInstance::Get().KeyUp(DIK_RSHIFT))
		m_iCurrentEffectID = CGameInstance::Get().Spawn(test, *m_pComTransform->GetWorldMatrix());


	if (CGameInstance::Get().KeyPressing(DIK_7)) {
		//auto b = CGameInstance::Get().GetParticle("PLAYERFLARE_CPU", "PLAYERFLARE_CPU");
	}

	
	_float4 fpos = _float4(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y + 2.5f, m_pComTransform->GetPosition().z ,1);
	_vector pos = XMVectorSet(fpos.x, fpos.y, fpos.z, fpos.w);
	if (m_bDashing) {
		_vector lastSpawnPos = XMVectorSet(m_fSpwanPos.x, m_fSpwanPos.y, m_fSpwanPos.z,1.f);
		_float distance = XMVectorGetX(
			XMVector3Length(pos - lastSpawnPos));
		_float3 vstart, vend;
		vstart = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y + 2.5f, m_pComTransform->GetPosition().z);
		vend = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y - 2.5f, m_pComTransform->GetPosition().z);
		CGameInstance::Get().AddTrailPoint("Player_Attack_Trail_CPU", "Player_Attack_Trail_CPU", GetHandle(), vstart, vend);
		//_float3 deltaPos;
		//XMStoreFloat3(&deltaPos, lastSpawnPos - pos);
		if (distance > m_fDistanceOffeset) {
			m_iCurrentEffectID = CGameInstance::Get().PlayEffect(
				"PlayerDashSmoke",
				*m_pComTransform->GetWorldMatrix(), pos,
				[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
				{
					if (effectId != m_iCurrentEffectID)
						return;

					m_iCurrentEffectID = INVALID_EFFECT_INSTANCE_ID;

					if (reason == EFFECT_FINISH_REASON::NATURAL)
						OnDashEffectFinished();
					else if (reason == EFFECT_FINISH_REASON::STOPPED)
						OnDashEffectStopped();

				});
;
			m_fSpwanPos = m_pComTransform->GetPosition();
			m_bDashEffectSpawned = true;
		}
	}


	//if (CGameInstance::Get().KeyPressing(DIK_P))
	//	m_pComTransform->AddRotation(XMVectorSet(0,0,1,0), fTimeDelta * 5);


	Weapon_Throw(fTimeDelta);
}

void CWeapon::LateUpdate(E::_float fTimeDelta)
{
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
	{
		if (!m_bThrow)
		{
			if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
			{
				if (pModel->Get_CombinedBoneMatrices().size() >= m_iBoneSocketIndex)
				{
					_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneSocketIndex]);
					for (uint32_t i = 0; i < 3; ++i)
					{
						Par.r[i] = XMVector3Normalize(Par.r[i]);
					}
					XMStoreFloat4x4(&m_ParentMatrix, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));
				}
			}
		}
		if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
		{
			if (!m_bThrow && pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
			{
				m_bThrow = true;
				XMStoreFloat3(&m_vLook, pBT->GetGameObject()->GetTransform().GetState(STATE::LOOK));
				XMStoreFloat4x4(&m_ParentMatrix, (XMLoadFloat4x4(&m_ParentMatrix)));
			}
			else if(m_bThrow && !pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
				m_bThrow = false;
		}
	}
	
	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CWeapon::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}

void CWeapon::Weapon_Throw(_float fTimeDelta)
{
	if (!m_bThrow)
		return;
	m_fAngle = 30.f;
	_vector vTargetLook = XMVector3Normalize(XMLoadFloat3(&m_vLook));
	
	_matrix Rot = XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMVectorSet(0,0,1, 0), XMConvertToRadians(m_fAngle)));
	_matrix matRot = Rot * XMLoadFloat4x4(&m_ParentMatrix);
	matRot.r[3] += vTargetLook * 15.f * fTimeDelta;
	XMStoreFloat4x4(&m_ParentMatrix, matRot);

}

void CWeapon::OnDashEffectFinished()
{
	m_bDashEffectSpawned = false;
}

void CWeapon::OnDashEffectStopped()
{
}

E::UPtr<CWeapon> CWeapon::Create()
{
	auto pInstance = E::ToUPtr(new CWeapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWeapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWeapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWeapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWeapon");
		return nullptr;
	}

	return pInstance;
}
