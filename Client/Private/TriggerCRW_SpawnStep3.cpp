#include "pch.h"
#include "TriggerCRW_SpawnStep3.h"
#include "MyMagicSquareStepController.h"
NS_USING(Client)

HRESULT CTriggerCRW_SpawnStep3::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_SpawnStep3::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnStep2] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{

		auto pvec = CGameInstance::Get().GetGameObjectLayer("22_MyMagicSquareStepController");
		if (!(pvec || pvec->empty()))
		{
			return;
		}

		auto pController = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStepController>(pvec->front());
		if (!pController)
		{
			return;
		}

		
		const StringID CircleGroupID{ "CreatureMagicCircleGrid" };
		if (!pController->SpawnGroup(CircleGroupID))
		{
			return;
		}

		m_bSpawned = true;
		CMyMagicSquareStepController::RISE_PATTERN_DESC RiseDesc{};
		//RiseDesc.fStartTargetY = -227.f;
		RiseDesc.fStartTargetY = -230.f;
		RiseDesc.fEndTargetY = -230.f;
		RiseDesc.fMoveSpeed = 15.f;
		RiseDesc.fBounceHeight = 0.6f;
		RiseDesc.fBounceSettleSpeed = 5.f;
		RiseDesc.fLineInterval = 0.5f;
		RiseDesc.fStepInterval = 0.5f;
		RiseDesc.fStepTimingCurve = 0.5f;
		RiseDesc.fStepTimingJitter = 0.5f;
		RiseDesc.eFillMode =
			CMyMagicSquareStepController::
			RISE_FILL_MODE::Z;
		RiseDesc.eHeightAxis =
			CMyMagicSquareStepController::
			FILL_AXIS::Z;
		RiseDesc.eDirection =
			CMyMagicSquareStepController::
			FILL_DIRECTION::FORWARD;
		RiseDesc.eFillMode =
			CMyMagicSquareStepController::
			RISE_FILL_MODE::RADIAL;

		if (!pController->StartRisePattern(CircleGroupID, RiseDesc))
		{
			return;
		}
	}
}

void CTriggerCRW_SpawnStep3::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnStep3] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_SpawnStep3> CTriggerCRW_SpawnStep3::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnStep3{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_SpawnStep3::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnStep3{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
