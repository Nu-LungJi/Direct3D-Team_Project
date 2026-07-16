#pragma once

#include "Client_Defines.h"

#include "Level_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelLoading final : public Engine::CLevel
{
private:
	explicit CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept;
	~CLevelLoading() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameEnd(E::_float fTimeDelta) override;
private:
	HRESULT LoadEnd();
	void ThreadStart();
	void LoadingCheck();

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

private:
	std::optional<LEVEL> m_ePreviousLevelIndex{};
	const LEVEL m_eNextLevelIndex;

	bool m_bThreadStart{ false };
	bool m_bLoadEnd{ false };
	std::future<bool> m_futLoadFinish{};

public:
	static Engine::UPtr< CLevelLoading> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex);

private:
	void Free() override;

};

NS_END
