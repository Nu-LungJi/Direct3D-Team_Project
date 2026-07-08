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

struct TestStruct: public ISerializable
{
	int i = 0;
	float f = 0;


	std::vector<TestStruct> childs{ };
	std::map<std::string, TestStruct> maps{};
	std::unordered_map<StringID, TestStruct> umaps{};

	std::unordered_map<int, TestStruct> uidmaps{};

	_float3 vTmpPos{};

	_float4x4 matWorld{};

	void Serialize(ISerializer& serializer) const override;
	void Deserialize(IDeserializer& deserializer) override;
};

void TestStruct::Serialize(ISerializer& serializer) const
{
	serializer.Write("i", i);
	serializer.Write("f", f);
	serializer.WriteArray("childs", childs);
	serializer.WriteMap("maps", maps);
	serializer.WriteMap("umaps", umaps);
	serializer.WriteMap("uidmaps", uidmaps);
	serializer.Write("vTmpPos", vTmpPos);
	serializer.Write("matWorld", matWorld);
}

void TestStruct::Deserialize(IDeserializer& deserializer)
{
	deserializer.Read("i", i);
	deserializer.Read("f", f);
	deserializer.ReadArray("childs", childs);
	deserializer.ReadMap("maps", maps);
	deserializer.ReadMap("umaps", umaps);
	deserializer.ReadMap("uidmaps", uidmaps);
	deserializer.Read("vTmpPos", vTmpPos);
	deserializer.Read("matWorld", matWorld);
}

CBaseApp::CBaseApp()
{
	auto jsonDes = CJsonDeSerializer::Create("./TestStruct.json");
	if (jsonDes) 
	{
		TestStruct myStruct;
		myStruct.Deserialize(*jsonDes);
		int x = 0;
	}


	auto binDes = CBinDeSerializer::Create("./TestStruct.bin");
	if (binDes)
	{
		TestStruct myStruct;
		myStruct.Deserialize(*jsonDes);
		int x = 0;
	}
	

	TestStruct hello{};
	hello.i = 1;
	hello.f = 777.f;

	TestStruct hello2{};
	hello2.i = 2;
	hello2.f = 77237.f;

	TestStruct hello3{};
	hello3.i = 3;
	hello3.f = 77237.f;

	TestStruct hello4{};
	hello4.i = 4;
	hello4.f = 77237.f;
	hello4.vTmpPos = { 123.f, 456.f, 789.f };

	TestStruct hello5{};
	hello5.i = 5;
	hello5.f = 77237.f;

	hello4.umaps.emplace("JJJ", hello5);
	hello3.childs.push_back(hello4);

	TestStruct hihi{};
	hihi.i = 6;
	hihi.f = 888.3f;
	hihi.childs.push_back(hello2);
	hihi.umaps.emplace("ZZZ", hello3);

	XMMatrixIdentity();

	TestStruct ts{};
	ts.i = 7;
	ts.f = 123.66f;
	ts.childs.push_back(hello);
	ts.umaps.emplace("wow", hihi);

	auto jsonSer = CJsonSerializer::Create();
	ts.Serialize(*jsonSer);
	jsonSer->SaveToFile("./TestStruct.json");

	auto binSer = CBinSerializer::Create();
	ts.Serialize(*binSer);
	binSer->SaveToFile("./TestStruct.bin");


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

		{
			ZoneScopedN("FrameStart");
			FrameStart(fDeltaTime);
		}

		m_FixedUpdateTimer.AppendCurrTime(fDeltaTime);
		if (m_FixedUpdateTimer.Get_JustFinished())
		{
			const float fFixedGoalTime = m_FixedUpdateTimer.Get_GoalTime();
			float fFixedCurrTime = m_FixedUpdateTimer.Get_CurrTime();

			while (fFixedCurrTime >= fFixedGoalTime)
			{
				{
					ZoneScopedN("FixedUpdate");
					FixedUpdate(fFixedGoalTime);
				}
				
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

void CBaseApp::UpdateGUI() const
{
	Engine::CGameInstance::Get().ImguiNewFrame();

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
			return E_FAIL;
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
