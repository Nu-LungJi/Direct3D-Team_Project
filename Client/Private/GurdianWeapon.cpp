#include "pch.h"
#include "GurdianWeapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "Trail_CPU.h"

NS_USING(Client)

CGurdianWeapon::CGurdianWeapon()
	: CMon_Weapon{}
{
}

CGurdianWeapon::~CGurdianWeapon()
{
}

void CGurdianWeapon::UpdateGUI()
{
	CMon_Weapon::UpdateGUI();

}

HRESULT CGurdianWeapon::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
		return E_FAIL;

	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGurdianWeapon::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CGurdianWeapon::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bDead)
	{
		Dead_Parent(fTimeDelta);
		return;
	}

	__super::PriorityUpdate(fTimeDelta);

}

void CGurdianWeapon::Update(E::_float fTimeDelta)
{
	if (m_bDead)
		return;

	
	__super::Update(fTimeDelta);

	Weapon_Throw(fTimeDelta);

}

void CGurdianWeapon::LateUpdate(E::_float fTimeDelta)
{
	if (m_bDead)
		return;
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
	{
		if (!m_bThrow)
		{
			if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
			{
				if (pModel->Get_CombinedBoneMatrices().size() > m_iBoneSocketIndex)
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
			else if (m_bThrow && !pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
				m_bThrow = false;

			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)))
			{
				m_bDead = true;
				
			}
			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE)))
			{
				Weapon_CallBack();
				pBT->Set_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE), FLAGTYPE::DEL);
			}
		}
	}

	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/
}

void CGurdianWeapon::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}
void CGurdianWeapon::Dead_Parent(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	_float t = m_fTick / 3.f;
	m_fDissolveintensity = 1 + (0 - 1) * t;
	if (t >= 1.f)
	{
		m_fDissolveintensity = 0.f;
		m_fTick = 0.f;
		SetPendingDestroy();
	
	}
}
/*---------------------------------*/
void CGurdianWeapon::Weapon_Throw(_float fTimeDelta)
{
	if (!m_bThrow)
		return;
	m_fAngle = 30.f;
	_vector vTargetLook = XMVector3Normalize(XMLoadFloat3(&m_vLook));

	_matrix Rot = XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), XMConvertToRadians(m_fAngle)));
	_matrix matRot = Rot * XMLoadFloat4x4(&m_ParentMatrix);
	matRot.r[3] += vTargetLook * 15.f * fTimeDelta;
	XMStoreFloat4x4(&m_ParentMatrix, matRot);

}

E::UPtr<CGurdianWeapon> CGurdianWeapon::Create()
{
	auto pInstance = E::ToUPtr(new CGurdianWeapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGurdianWeapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGurdianWeapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGurdianWeapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGurdianWeapon");
		return nullptr;
	}

	return pInstance;
}
