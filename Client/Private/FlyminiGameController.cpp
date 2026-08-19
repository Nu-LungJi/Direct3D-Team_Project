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

E::UPtr<E::CPrototype> CFlyminiGameController::Clone(void* pArg)
{
	return E::UPtr<E::CPrototype>();
}
