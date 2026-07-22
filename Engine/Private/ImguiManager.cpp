#include "pch.h"
#include "ImguiManager.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <ImGuizmo.h>
#include "NodeEditor.h"
NS_USING(Engine)
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

CImguiManager::CImguiManager()
{
}
CImguiManager::~CImguiManager()
{
	if (ImGui::GetCurrentContext() != nullptr)
	{
		if (m_bNewFrame)
		{
			ImGui::EndFrame();
			m_bNewFrame = false;
		}

		m_pNodeEditor.reset();
		ImGui::DestroyPlatformWindows();
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (m_pContext != nullptr)
	{
		m_pContext->ClearState();
		m_pContext->Flush();
	}
	m_pContext = nullptr;
}

HRESULT CImguiManager::Ready_Imgui(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	//ImGui_ImplWin32_EnableDpiAwareness();

	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	static const ImWchar ranges[] = { 0x0020, 0x00FF, 0xAC00, 0xD7A3, 0x3130, 0x318F, 0 };
	io.Fonts->AddFontFromFileTTF("./Resources/Engine/Font/Pretendard-Medium.ttf", 15.0f, NULL, ranges);

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 키보드 컨트롤 활성화
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.FontGlobalScale = main_scale;

	ImGui::StyleColorsDark();

	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(pDevice, pContext);

	io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	m_bViewportsAvailable = true;

	m_pContext = pContext;

	m_pNodeEditor = CNodeEditor::Create();
	if (nullptr == m_pNodeEditor)
		return E_FAIL;

	return S_OK;
}
//CGameInstance::Get().ImguiEnableDocking(true, true);

void CImguiManager::Update_Imgui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	//m_pNodeEditor->UpdateGUI();
	m_bNewFrame = true;
}

void CImguiManager::Render_Imgui()
{
	if (m_bNewFrame)
	{
		ImGuiIO& io = ImGui::GetIO();

		m_pNodeEditor->RenderGUI(); //imgui node 랜더
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) && m_bViewportsEnabled)
		{
			ID3D11RenderTargetView* pBackupRTV = nullptr;
			ID3D11DepthStencilView* pBackupDSV = nullptr;
			m_pContext->OMGetRenderTargets(1, &pBackupRTV, &pBackupDSV);

			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();

			m_pContext->OMSetRenderTargets(1, &pBackupRTV, pBackupDSV);
			if (pBackupRTV != nullptr)
			{
				pBackupRTV->Release();
			}
			if (pBackupDSV != nullptr)
			{
				pBackupDSV->Release();
			}
		}

		m_bNewFrame = false;
	}
}


void CImguiManager::EnableDocking(_bool bEnableDocking, _bool bEnableViewports)
{
	ImGuiIO& io = ImGui::GetIO();

	if (bEnableDocking)
	{
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}
	else
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
	}

	if (bEnableViewports)
	{
		if (m_bViewportsAvailable)
		{
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			io.ConfigViewportsNoAutoMerge = false;

			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 0.f;
			style.Colors[ImGuiCol_WindowBg].w = 1.f;
		}
	}
	else
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	}

	m_bViewportsEnabled = bEnableViewports && m_bViewportsAvailable;
}

_bool CImguiManager::WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return false;
}

UPtr<CImguiManager> CImguiManager::Create(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	auto pInstance = UPtr<CImguiManager>(new CImguiManager{});
	if (FAILED(pInstance->Ready_Imgui(hWnd, pDevice, pContext)))
	{
		MSG_BOX("CImguiManager Create Failed");
		return nullptr;
	}
	return pInstance;
}
