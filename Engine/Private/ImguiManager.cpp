#include "pch.h"
#include "ImguiManager.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include "NodeEditor.h"
NS_USING(Engine)
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

CImguiManager::CImguiManager()
{
}
CImguiManager::~CImguiManager()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
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
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 키보드 컨트롤 활성화
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.FontGlobalScale = main_scale;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(pDevice, pContext);

	m_pNodeEditor = CNodeEditor::Create();
	if (nullptr == m_pNodeEditor)
		return E_FAIL;

	return S_OK;
}

void CImguiManager::Update_Imgui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	//m_pNodeEditor->UpdateGUI();
	m_bNewFrame = true;
}

void CImguiManager::Render_Imgui()
{
	if (m_bNewFrame)
	{
		m_pNodeEditor->RenderGUI(); //imgui node 랜더
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		m_bNewFrame = false;
	}
}

void CImguiManager::Update_ImguiNodeEditor()
{
	m_pNodeEditor->UpdateGUI();
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
