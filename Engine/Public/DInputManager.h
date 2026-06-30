#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CDInputManager final : public CEngineBase
{
private:
	CDInputManager();
	~CDInputManager();

public:
	HRESULT Ready_InputDev(HINSTANCE hInst, HWND hWnd);
	void Update_InputDev();

public:
	_bool KeyPressing(_ubyte byKeyID) const { return m_bKeyPressingState[byKeyID]; }
	_bool KeyUp(_ubyte byKeyID) const { return m_bKeyUpState[byKeyID]; }
	_bool KeyDown(_ubyte byKeyID) const { return m_bKeyDownState[byKeyID]; }

	int32_t	MouseMove(MOUSEMOVESTATE eMouseState) const { return *(((int32_t*)&m_tMouseState) + static_cast<int32_t>(eMouseState)); }
	_bool MousePressing(MOUSEKEYSTATE eMouseState) const { return m_bMousePressingState[static_cast<uint32_t>(eMouseState)]; }
	_bool MouseUp(MOUSEKEYSTATE eMouseState) const { return m_bMouseUpState[static_cast<uint32_t>(eMouseState)]; }
	_bool MouseDown(MOUSEKEYSTATE eMouseState) const { return m_bMouseDownState[static_cast<uint32_t>(eMouseState)]; }

private:
	ComPtr<IDirectInput8> m_pInputSDK{};
	ComPtr<IDirectInputDevice8> m_pKeyBoard{};
	ComPtr<IDirectInputDevice8> m_pMouse{};

private:
	_byte m_byKeyState[256]{};
	DIMOUSESTATE m_tMouseState{};

private:
	bool m_bKeyPressingState[256]{};
	bool m_bKeyUpState[256]{};
	bool m_bKeyDownState[256]{};

	bool m_bMousePressingState[static_cast<uint32_t>(MOUSEKEYSTATE::END)]{};
	bool m_bMouseUpState[static_cast<uint32_t>(MOUSEKEYSTATE::END)]{};
	bool m_bMouseDownState[static_cast<uint32_t>(MOUSEKEYSTATE::END)]{};

public:
	static UPtr<CDInputManager> Create(HINSTANCE hInst, HWND hWnd);
};

NS_END