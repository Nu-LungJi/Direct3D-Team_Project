#include "pch.h"
#include "BTDecHit.h"
#include "ComBeHavior.h"
NS_USING(Client)

CBTDecHit::CBTDecHit()
{

}
CBTDecHit::CBTDecHit(const CBTDecHit& rhs) : CBTDecorator(rhs)
{

}

CBTDecHit::~CBTDecHit()
{
}
HRESULT CBTDecHit::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHit";
	return S_OK;
}
HRESULT CBTDecHit::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecHit::Evaluate(_float fTimeDelta)
{
	if (CGameInstance::Get().KeyDown(DIK_2))
		return EVALUATE::SUCCESS;
	else return EVALUATE::FAILED;
	if (auto pBT = Get_ComBT())
	{
		if (Check_Flag(ETOUI(BTFLAG::HIT) | ETOUI(BTFLAG::SUPERARMOR)))
			return EVALUATE::SUCCESS;

		if (auto pCam = CGameInstance::Get().GetActiveCamera())
		{
			const auto& [vOri, vDir] = pCam->GetRay();
			_float fDist{};
			if (auto pVec = CGameInstance::Get().GetColliderGroup("CollTestGob"))
			{
				for (const auto& coll : *pVec)
				{
					if (coll->Intersect(vOri, vDir, fDist))
					{
						//맞으면 hit 진행 되는동안 다른 노드 절대 진입못하게 하고 기존에 Run상태인 애니매이션들 초기화하기

						uint32_t iFlag = ETOUI(BTFLAG::HIT) | ETOUI(BTFLAG::ABORT);
						Set_Flag(iFlag, FLAGTYPE::ADD);
						return __super::Evaluate(fTimeDelta);
					}
				}
			}
		}
	}
	return m_eDebug = EVALUATE::FAILED;
}
void CBTDecHit::Update_Gui()
{
}
E::UPtr<CBTDecHit> CBTDecHit::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHit{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHit");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHit::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHit{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHit");
		return nullptr;
	}

	return pInstance;
}
