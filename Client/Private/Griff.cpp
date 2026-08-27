#include "pch.h"
#include "Griff.h"
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
#include "DbgLineRender.h"
#include "GriffChild.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CGriff::CGriff()
{
}


CGriff::~CGriff()
{
}

void CGriff::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CGriff::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGriff::Initialize(void* pArg)
{
	auto WorldAgentDesc = static_cast<WORLD_AGENT_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}

	
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);

	m_WayPoint.push_back(_float3(30.f, 75.f, -346.f));
	m_WayPoint.push_back(_float3(-8.f, 135.f, -61.f));
	m_WayPoint.push_back(_float3(213.f, 122.f, -82.f));
	m_WayPoint.push_back(_float3(360.f, 169.f, 19.f));
	m_WayPoint.push_back(_float3(292.f, 186.f, 215.f));
	m_WayPoint.push_back(_float3(-196.f, 139.f, 23.f));
	m_WayPoint.push_back(_float3(30.f, 75.f, -346.f));
	return S_OK;
}

void CGriff::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
	if (m_WayPoint.empty())
		return;

	if (m_WayPoint.size() <= m_iIndex)
	{
		m_bLoop = false;
		--m_iIndex;
	}
	else if (m_iIndex <= 0)
	{
		m_bLoop = true;
	}

	_vector vNextPos = XMLoadFloat3(&m_WayPoint[m_iIndex]);
	_vector vCurPos = XMLoadFloat3(&GetTransform().GetPosition());

	_vector vLen = vNextPos - vCurPos;
	_vector vNextDir = XMVector3Normalize(vLen);

	_float fDis = XMVectorGetX(XMVector3Length(vLen));

	if (fDis < 3.f)
	{
		if (m_bLoop)
			++m_iIndex;
		else
			--m_iIndex;
	}


	_float3 vLastDir{};
	XMStoreFloat3(&vLastDir, vNextDir);

	m_pMoveIntent->SetFacingIntent(vLastDir, 60.f);
	m_pMoveIntent->SetMoveIntent(vLastDir, 30.f);
}

void CGriff::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CGriff::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CGriff::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}

void CGriff::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

void CGriff::Set_Child()
{
	WORLD_AGENT_DESC Child{};
	Child.TargetHandle = GetHandle();
	Child.sObjectTag = "GriffChild";
	Child.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Child.ReSourceTag = "Model_Resource_Griff";
	Child.bPhyx = false;
	_float3 vOffset = m_WayPoint.front();
	_float iCnt = 10.f;
	for (size_t i = 0; i < size_t(iCnt); ++i)
	{
		Child.vPos = _float3(vOffset.x + Randf(-iCnt, iCnt), 
			vOffset.y + Randf(-iCnt, iCnt), vOffset.z + Randf(-iCnt, iCnt));
			
		auto Griff = CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_GriffChild, "02_GriffChild", &Child);
		if (Griff)
			m_ChildHandles.push_back(Griff.value());
	}
}

E::UPtr<CGriff> CGriff::Create()
{
	auto pInstance = E::ToUPtr(new CGriff{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGriff");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGriff::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGriff{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGriff");
		return nullptr;
	}

	return pInstance;
}
