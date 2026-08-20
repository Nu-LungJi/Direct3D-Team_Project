#include "pch.h"
#include "FlyMiniGameController.h"
#include "FlightRing.h"
NS_USING(Client)

CFlyminiGameController::CFlyminiGameController()
{
}

CFlyminiGameController::~CFlyminiGameController()
{
}

void CFlyminiGameController::UpdateGUI()
{
}

HRESULT CFlyminiGameController::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CFlyminiGameController::Initialize(void* pArg)
{
	if (pArg == nullptr)
		return E_FAIL;

	CFlyminiGameController_DESC* desc = static_cast<CFlyminiGameController_DESC*>(pArg);
	m_hPlayer = desc->hPlayer;

	const std::vector<CHandle>* pVecRings = CGameInstance::Get().GetGameObjectLayer("고리 레이어 이름");
	if (pVecRings == nullptr)
		return E_FAIL;

	const size_t ringCount = pVecRings->size();

	m_vecFlightRing.reserve(ringCount);
	for (size_t i = 0; i < ringCount; ++i)
	{
		// 핸들 복사
		m_vecFlightRing.push_back(pVecRings->at(i));
	}

	return S_OK;
}

void CFlyminiGameController::PriorityUpdate(E::_float fTimeDelta)
{
	auto& gameInstance = CGameInstance::Get();

	for (size_t i = 0; i < m_vecFlightRing.size(); ++i)
	{
		CFlightRing* ring = gameInstance.GetGameObjectByHandleT<CFlightRing>(m_vecFlightRing[i]);
		if (ring == nullptr)
			continue;

		if (!ring->IsCheckComplete())
		{
			if (ring->PassCheck(m_hPlayer))
				m_iPassRing++;
		}
	}
}

void CFlyminiGameController::Update(E::_float fTimeDelta)
{
}

void CFlyminiGameController::LateUpdate(E::_float fTimeDelta)
{
}

E::UPtr<CFlyminiGameController> CFlyminiGameController::Create()
{
	auto pInstance = E::ToUPtr(new CFlyminiGameController{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CFlyminiGameController");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CFlyminiGameController::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CFlyminiGameController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFlyminiGameController");
		return nullptr;
	}

	return pInstance;
}
