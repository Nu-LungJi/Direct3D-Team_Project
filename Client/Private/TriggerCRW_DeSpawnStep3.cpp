#include "pch.h"
#include "TriggerCRW_DeSpawnStep3.h"
#include "MyMagicSquareStepController.h"
NS_USING(Client)

HRESULT CTriggerCRW_DeSpawnStep3::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_DeSpawnStep3::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_DeSpawnStep3] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{
		m_bSpawned = true;

		auto pvec = CGameInstance::Get().GetGameObjectLayer("22_MyMagicSquareStepController");
		if (!(pvec || pvec->empty()))
		{
			return;
		}

		if (auto pController = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStepController>(pvec->front()))
		{
			const StringID GroupID{ "CreatureMagicCircleGrid" };
			pController->DeleteGroup(GroupID);
			//CMyMagicSquareStepController::RISE_PATTERN_DESC RiseDesc{};
			////RiseDesc.fStartTargetY = -227.f;
			//RiseDesc.fStartTargetY = -214.f;
			//RiseDesc.fEndTargetY = -214.f;
			//RiseDesc.fMoveSpeed = 15.f;
			//RiseDesc.fBounceHeight = 0.3f;
			//RiseDesc.fBounceSettleSpeed = 1.f;
			//RiseDesc.fLineInterval = 0.05f;
			//RiseDesc.fStepInterval = 0.02f;
			//RiseDesc.fStepTimingCurve = 0.55f;
			//RiseDesc.fStepTimingJitter = 1.01f;
			//RiseDesc.eFillMode =
			//	CMyMagicSquareStepController::
			//	RISE_FILL_MODE::Z;
			//RiseDesc.eHeightAxis =
			//	CMyMagicSquareStepController::
			//	FILL_AXIS::Z;
			//RiseDesc.eDirection =
			//	CMyMagicSquareStepController::
			//	FILL_DIRECTION::REVERSE;
			//if (!pController->StartRisePattern(GroupID, RiseDesc))
			//{
			//	return;
			//}
		}
	}
}

void CTriggerCRW_DeSpawnStep3::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_DeSpawnStep3] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_DeSpawnStep3> CTriggerCRW_DeSpawnStep3::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_DeSpawnStep3{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_DeSpawnStep3::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_DeSpawnStep3{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
