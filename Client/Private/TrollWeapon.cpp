#include "pch.h"
#include "TrollWeapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"

NS_USING(Client)

CTrollWeapon::CTrollWeapon()
	: CMon_Weapon{}
{
}

CTrollWeapon::~CTrollWeapon()
{
}

void CTrollWeapon::UpdateGUI()
{
	CMon_Weapon::UpdateGUI();

}

HRESULT CTrollWeapon::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
		return E_FAIL;

	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CTrollWeapon::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CTrollWeapon::PriorityUpdate(E::_float fTimeDelta)
{
	auto ParentHandle = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle);
	if(m_bDead)
		Dead_Parent(fTimeDelta);
	__super::PriorityUpdate(fTimeDelta);

}

void CTrollWeapon::Update(E::_float fTimeDelta)
{
	if (m_bDead)
		return;


	__super::Update(fTimeDelta);


}

void CTrollWeapon::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bDead)
	{
		if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
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
	}


	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	

	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/
}

void CTrollWeapon::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}

void CTrollWeapon::Dead_Parent(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	_float t = m_fTick / 3.f;
	m_fDissolveintensity = 0 + (1 - 0) * t;
	if (t >= 1.f)
	{
		m_fDissolveintensity = 0.f;
		m_fTick = 0.f;
		SetPendingDestroy();

	}
}

_bool CTrollWeapon::UpdateSocketParentMatrix()
{
	auto* pParent = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle);
	if (!pParent || m_iBoneSocketIndex < 0)
		return false;

	auto* pModel = pParent->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pModel)
		return false;

	const auto& CombinedBoneMatrices = pModel->Get_CombinedBoneMatrices();
	const size_t iBoneIndex = static_cast<size_t>(m_iBoneSocketIndex);
	if (iBoneIndex >= CombinedBoneMatrices.size())
		return false;

	_matrix matSocket = XMLoadFloat4x4(&CombinedBoneMatrices[iBoneIndex]);
	for (uint32_t i = 0; i < 3; ++i)
	{
		const _float fLengthSq = XMVectorGetX(XMVector3LengthSq(matSocket.r[i]));
		if (fLengthSq <= std::numeric_limits<_float>::epsilon())
			return false;
		matSocket.r[i] = XMVector3Normalize(matSocket.r[i]);
	}

	XMStoreFloat4x4(
		&m_ParentMatrix,
		matSocket * pParent->GetTransform().GetLoadedWorldMatrix());
	return true;
}

E::UPtr<CTrollWeapon> CTrollWeapon::Create()
{
	auto pInstance = E::ToUPtr(new CTrollWeapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTrollWeapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTrollWeapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTrollWeapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTrollWeapon");
		return nullptr;
	}

	return pInstance;
}
