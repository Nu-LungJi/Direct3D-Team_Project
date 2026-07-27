#include "pch.h"
#include "TriggerCRW_StairStep.h"
#include "MyMagicSquareStepController.h"
NS_USING(Client)

HRESULT CTriggerCRW_StairStep::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_StairStep::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_StairStep] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{
		

		auto pvec = CGameInstance::Get().GetGameObjectLayer("22_MyMagicSquareStepController");
		if (!(pvec || pvec->empty()))
		{
			return;
		}

		if (auto pController = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStepController>(pvec->front()))
		{
			const StringID GroupID{ "MagicSquareGrid1" };
			CMyMagicSquareStepController::RISE_PATTERN_DESC RiseDesc{};
			RiseDesc.fStartTargetY = -227.f;
			RiseDesc.fEndTargetY = -214.f;
			RiseDesc.fMoveSpeed = 5.f;
			RiseDesc.fBounceHeight = 0.3f;
			RiseDesc.fBounceSettleSpeed = 1.f;
			RiseDesc.fLineInterval = 0.05f;
			RiseDesc.fStepInterval = 0.02f;
			RiseDesc.fStepTimingCurve = 0.55f;
			RiseDesc.fStepTimingJitter = 0.41f;
			RiseDesc.eFillMode =
				CMyMagicSquareStepController::
				RISE_FILL_MODE::RADIAL;
			RiseDesc.eHeightAxis =
				CMyMagicSquareStepController::
				FILL_AXIS::Z;
			RiseDesc.eDirection =
				CMyMagicSquareStepController::
				FILL_DIRECTION::FORWARD;
			if (pController->StartRisePattern(GroupID, RiseDesc))
			{
				m_bSpawned = true;
			}
		}
	}
}

void CTriggerCRW_StairStep::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_StairStep] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_StairStep> CTriggerCRW_StairStep::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_StairStep{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_StairStep::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_StairStep{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
