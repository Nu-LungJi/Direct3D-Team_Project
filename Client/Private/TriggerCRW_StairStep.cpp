#include "pch.h"
#include "TriggerCRW_StairStep.h"
#include "MyMagicSquareStepController.h"
#include "Player.h"
#include "Client_Defines.h"
#include "Player.h"
#include "MyMagicSquareStep.h"
NS_USING(Client)

HRESULT CTriggerCRW_StairStep::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}


void CTriggerCRW_StairStep::Update(_float fTimeDelta)
{
	// 벡터를 순회하면서 타이머 업데이트
	if (false)
		for (auto it = m_DelayedTasks.begin(); it != m_DelayedTasks.end(); )
		{
			it->Timer.AppendCurrTime(fTimeDelta);

			// 목표 시간에 도달했는지 확인
			if (it->Timer.Get_Finished())
			{
				it->Callback();                 // 사운드 재생 콜백 실행
				it = m_DelayedTasks.erase(it);  // 완료된 태스크는 삭제하고 다음 반복자로 이동
			}
			else
			{
				++it;
			}
		}

	for (auto it = m_DelayedTasks.begin(); it != m_DelayedTasks.end(); )
	{
		if (auto pStep = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStep>(it->hStep))
		{
			if (pStep->GetState() == CMyMagicSquareStep::STATE::BOUNCE_SETTLE)
			{
				it->Callback();
				it = m_DelayedTasks.erase(it);
			}
			else
			{
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}

void CTriggerCRW_StairStep::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_StairStep] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	auto pPlayer = Cast<CPlayer>(pObj);
	if (!pPlayer)
		return;

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

			RiseDesc.fnEventCallback = [hController = pController->GetHandle(), hPlayer = pPlayer->GetHandle(), hTrigger = GetHandle()](const CMyMagicSquareStepController::PATTERN_EVENT_DATA& Event)
				{
					auto* pController = CGameInstance::Get()
						.GetGameObjectByHandleT<CMyMagicSquareStepController>(hController);

					auto* pPlayer = CGameInstance::Get()
						.GetGameObjectByHandleT<CPlayer>(hPlayer);

					if (!pController)
						return;

					if (!pPlayer)
						return;

					CComSound* pSound = pController->GetSound();
					if (!pSound)
						return;

					switch (Event.eEvent)
					{
					case CMyMagicSquareStepController::PATTERN_EVENT::STARTED:
						DEBUG_LOG("CMyMagicSquareStepController::PATTERN_EVENT::STARTED+_+\n");
						// 시작음 + 이동 루프 시작
						break;

					//case CMyMagicSquareStepController::PATTERN_EVENT::LINE_ISSUED:
					//	DEBUG_LOG("CMyMagicSquareStepController::PATTERN_EVENT::LINE_ISSUED+_+\n");
					//	// 이동 루프 위치 변경 + 제한된 충격음

					//	{
					//		auto tmp = Event.iLineIndex % 10;
					//		if (tmp == 0)
					//		{
					//			//./Resources/SampleClient/Sound/CharlesRookwood/MagicStep/MagicStep_Issue_1.wav
					//			auto id = CGameInstance::Get().GetSoundManager()->Play3D(
					//				"./Resources/SampleClient/Sound/CharlesRookwood/MagicStep/MagicStep_Issue_1.wav",
					//				SOUND_3D_DESC{
					//					.vPosition = pPlayer->GetTransform().GetPosition(),
					//					.fMinDistance = 30.f,
					//					.fMaxDistance = 60.f,
					//					.eRolloff = SOUND_3D_ROLLOFF::LINEAR
					//				},
					//				SOUND_PLAY_DESC{
					//					.sBusID = SOUND_BUS::SFX,
					//					.fVolume = 1.f,
					//					.fPitch = 1.f,
					//					//.fFadeInDuration = 6.5f,
					//					.iPriority = 64,
					//					.bLoop = false
					//				}
					//			);
					//		}

					//		//if (id == INVALID_SOUND_ID)
					//		//{
					//		//	MSG_BOX("INVALID_SOUND_ID");
					//		//}
					//	}
					//	break;
					case CMyMagicSquareStepController::PATTERN_EVENT::LINE_ISSUED:
						DEBUG_LOG("CMyMagicSquareStepController::PATTERN_EVENT::LINE_ISSUED+_+\n");
						// 이동 루프 위치 변경 + 제한된 충격음

						{
							auto playSoundLogic = [hPlayer, hStep = Event.hStep]()
								{
									auto* pPlayer = CGameInstance::Get().GetGameObjectByHandle(hPlayer);
									if (!pPlayer) return;

									auto pStep = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStep>(hStep);
									if (!pStep)	return;

									CGameInstance::Get().GetSoundManager()->Play3D(
										"./Resources/SampleClient/Sound/CharlesRookwood/MagicStep/MagicStep_Issue_1.wav",
										SOUND_3D_DESC{
											.vPosition = pStep->GetTransform().GetPosition(),
											.fMinDistance = 35.f,
											.fMaxDistance = 100.f,
											.eRolloff = SOUND_3D_ROLLOFF::LINEAR
										},
										SOUND_PLAY_DESC{
											.sBusID = SOUND_BUS::SFX,
											.fVolume = 0.3f,
											.fPitch = 1.f,
											.iPriority = 64,
											.bLoop = false
										}
									);
								};

							auto tmp = Event.iLineIndex % 3;
							if (tmp == 0)
							{
								E::CTimer soundTimer;
								soundTimer.Set_GoalTime(1.0f);
								if (auto pTrigger = CGameInstance::Get().GetGameObjectByHandleT<CTriggerCRW_StairStep>(hTrigger))
								{
									pTrigger->m_DelayedTasks.push_back({ soundTimer, playSoundLogic, Event.hStep });
								}
							}

						}
						break;

					case CMyMagicSquareStepController::PATTERN_EVENT::COMPLETED:
						DEBUG_LOG("CMyMagicSquareStepController::PATTERN_EVENT::COMPLETED+_+\n");
						//{
						//	auto id = CGameInstance::Get().GetSoundManager()->Play3D(
						//		"./Resources/SampleClient/Sound/CharlesRookwood/MagicStep/MagicStep_Issue_1.wav",
						//		SOUND_3D_DESC{
						//			.vPosition = pPlayer->GetTransform().GetPosition(),
						//			.fMinDistance = 30.f,
						//			.fMaxDistance = 60.f,
						//			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
						//		},
						//		SOUND_PLAY_DESC{
						//			.sBusID = SOUND_BUS::SFX,
						//			.fVolume = 1.f,
						//			.fPitch = 1.f,
						//			.iPriority = 64,
						//			.bLoop = false
						//		}
						//	);
						//}
						// 이동 루프 페이드아웃 + 완료음
						break;

					case CMyMagicSquareStepController::PATTERN_EVENT::FAILED:
						DEBUG_LOG("CMyMagicSquareStepController::PATTERN_EVENT::FAILED+_+\n");
						// 이동 루프 정지
						break;
					}
				};
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
