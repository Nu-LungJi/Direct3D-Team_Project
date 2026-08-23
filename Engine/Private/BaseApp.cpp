#include "pch.h"
#include "BaseApp.h"
#include "Level.h"
#include "GameInstance.h"
#include "ISerializable.h"
#include "SerializerInterface.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"

#include "BinSerializer.h"
#include "JsonSerializer.h"
#include "BinDeSerializer.h"

NS_USING(Engine)


CBaseApp::CBaseApp()
{
}

CBaseApp::~CBaseApp()
{
	CGameInstance::Get().Release_Engine();
}

HRESULT CBaseApp::Loop()
{
	//constexpr float MAX_DELTA = 1.f;
	const float fPerfTime = Engine::CGameInstance::Get().UpdateTimeProvider();

	m_UpdateTimer.AppendCurrTime(fPerfTime);
	if (m_UpdateTimer.Get_JustFinished())
		{
			const float fGoalTime = m_UpdateTimer.Get_GoalTime();
		float fCurrTime = m_UpdateTimer.Get_CurrTime();

		//float fDeltaTime = std::min(fCurrTime, MAX_DELTA);
		float fDeltaTime = fCurrTime;
		Engine::CGameInstance::Get().BeginFrameTime(fDeltaTime);

		{
			ZoneScopedN("FrameStart");
			FrameStart(fDeltaTime);
		}

		m_FixedUpdateTimer.AppendCurrTime(fDeltaTime);
		if (m_FixedUpdateTimer.Get_JustFinished())
		{
			const float fFixedGoalTime = m_FixedUpdateTimer.Get_GoalTime();
			float fFixedCurrTime = m_FixedUpdateTimer.Get_CurrTime();
			const uint32_t iMaxFixedUpdateCountPerFrame =
				m_bSingleFixedUpdatePerFrame ? 1u : 8u;
			uint32_t iFixedUpdateCount = 0;

			while (fFixedCurrTime >= fFixedGoalTime)
			{
				if (iFixedUpdateCount >= iMaxFixedUpdateCountPerFrame)
				{
					//char szLog[192]{};
					//sprintf_s(szLog,
					//	"[FixedUpdate] Per-frame limit exceeded. Limit: %u, discarded accumulated time: %.6fs\n",
					//	iMaxFixedUpdateCountPerFrame,
					//	fFixedCurrTime);
					//DEBUG_LOG(szLog);
					fFixedCurrTime = fmodf(fFixedCurrTime, fFixedGoalTime);
					break;
				}

				{
					ZoneScopedN("FixedUpdate");
					FixedUpdate(fFixedGoalTime);
				}
				
				++iFixedUpdateCount;
				fFixedCurrTime -= fFixedGoalTime;
			}

			m_FixedUpdateTimer.Reset(fFixedCurrTime);
		}

		{
			ZoneScopedN("Update"); 
			Update(fDeltaTime);
		}


		float remain = fmodf(fDeltaTime, fGoalTime);
		m_UpdateTimer.Reset(remain);
		//fDeltaTime = fmodf(fDeltaTime, fGoalTime);
		//m_UpdateTimer.Reset(fDeltaTime);

		float fUpdateGoalTime = m_UpdateTimer.Get_GoalTime();
		float fInterpolatoin = m_UpdateTimer.Get_CurrTime() / fUpdateGoalTime;

		if (FAILED(Render(fInterpolatoin)))
		{
			MSG_BOX("CMainApp::Render FAILED");
			return E_FAIL;
		}
		

		{
			ZoneScopedN("FrameEnd");
			FrameEnd(fDeltaTime);
		}

		FrameMark;
	}

	m_MeasureTimer.AppendCurrTime(fPerfTime);
	if (m_MeasureTimer.Get_JustFinished())
	{
		//m_iMeasureFixedUpdateCntPerSec = m_iMeasureFixedUpdateCnt;
		m_iMeasureUpdateCntPerSec = m_iMeasureUpdateCnt;
		//m_iMeasureRenderCntPerSec = m_iMeasureRenderCnt;

		//m_iMeasureFixedUpdateCnt = 0;
		m_iMeasureUpdateCnt = 0;
		//m_iMeasureRenderCnt = 0;
		_tchar szFps[32];
		swprintf_s(szFps, 32, L"%d", m_iMeasureUpdateCntPerSec);
		SetWindowText(CGameInstance::Get().GetHwnd(), szFps);

		m_MeasureTimer.Reset(fmodf(m_MeasureTimer.Get_CurrTime(), m_MeasureTimer.Get_GoalTime()));
	}
	return S_OK;
}

void CBaseApp::UpdateGUI()
{
	Engine::CGameInstance::Get().ImguiNewFrame();

	if (ImGui::Begin("BaseApp FixedUpdate"))
	{
		ImGui::Checkbox(
			"Limit FixedUpdate To One Per Frame",
			&m_bSingleFixedUpdatePerFrame);
		ImGui::Text(
			"Max Fixed Updates Per Frame: %u",
			m_bSingleFixedUpdatePerFrame ? 1u : 8u);
	}
	ImGui::End();

	Engine::CGameInstance::Get().UpdateGUI();
}

void CBaseApp::RenderGUI() const
{

	Engine::CGameInstance::Get().ImguiEndFrameAndRender();
}

void CBaseApp::FixedUpdate(_float fTimeDelta)
{
	Engine::CGameInstance::Get().FixedUpdateEngine(fTimeDelta);
}

void CBaseApp::Update(_float fTimeDelta)
{
	Engine::CGameInstance::Get().UpdateEngine(fTimeDelta);

	if (Engine::CGameInstance::Get().KeyDown(DIK_GRAVE))
	{
		Engine::CGameInstance::Get().ImguiSetActive(!Engine::CGameInstance::Get().ImguiGetActive());
	}

	++m_iMeasureUpdateCnt;
}

HRESULT CBaseApp::Render(_float fInterpolation)
{
	ZoneScopedN("Render");
	if (FAILED(Engine::CGameInstance::Get().Draw()))
	{
		return E_FAIL;
	}

#ifdef IMGUI_ENABLE
	if (Engine::CGameInstance::Get().ImguiGetActive())
	{
		ZoneScopedN("RenderGUI");
		RenderGUI();
	}
#endif

	if (FAILED(Engine::CGameInstance::Get().Present()))
	{
			return  E_FAIL;
	}

	return S_OK;
}

void CBaseApp::FrameStart(_float fTimeDelta)
{
	Engine::CGameInstance::Get().FrameStart(fTimeDelta);
#ifdef IMGUI_ENABLE
	if (Engine::CGameInstance::Get().ImguiGetActive())
	{
		UpdateGUI();
	}
#endif
}

void CBaseApp::FrameEnd(_float fTimeDelta)
{
	Engine::CGameInstance::Get().FrameEnd(fTimeDelta);
}

HRESULT CBaseApp::Initialize(const ENGINE_DESC& engineDesc)
{
	m_UpdateTimer.Set_GoalTime(1.f / 60.f);
	m_FixedUpdateTimer.Set_GoalTime(1.f/ 60.);
	m_MeasureTimer.Set_GoalTime(1.f);

	Engine::ENGINE_DESC EngineDesc{ engineDesc };

	if (FAILED(Engine::CGameInstance::Get().InitializeEngine(EngineDesc, m_pDevice, m_pContext)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CBaseApp::StartLevel(UPtr<CLevel> pStartLevel)
{
	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(
		std::move(pStartLevel))))
	{
		return E_FAIL;
	}

	return S_OK;
}
