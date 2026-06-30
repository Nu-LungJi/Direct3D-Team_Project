#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CImguiManager final : public CEngineBase
{
private:
	CImguiManager();
	~CImguiManager();

public:
	_bool Get_Active() const { return m_bActive; };
	void Set_Active(_bool bActive) { m_bActive = bActive; }

public:
	HRESULT Ready_Imgui(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Update_Imgui();
	void Render_Imgui();

	_bool WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	_bool m_bActive{ true };
	_bool m_bNewFrame{ false };

public:
	static UPtr<CImguiManager> Create(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
};

NS_END