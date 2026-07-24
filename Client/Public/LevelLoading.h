#pragma once

#include "Client_Defines.h"

#include "Level_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelLoading final : public Engine::CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelLoading, CLevel)

private:
	explicit CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept;
	~CLevelLoading() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameEnd(E::_float fTimeDelta) override;
	bool IsLevelChangeLocked() const override;
private:
	HRESULT LoadEnd();
	void StartUnload();
	void CheckUnload();
	void StartLoad();
	void CheckLoad();

private:
	enum class PHASE
	{
		READY,
		UNLOADING,
		LOADING,
		COMPLETE,
		FAILED
	};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

private:
	std::optional<LEVEL> m_ePreviousLevelIndex{};
	const LEVEL m_eNextLevelIndex;

	bool m_bLoadEnd{ false };
	PHASE m_ePhase{ PHASE::READY };
	std::future<bool> m_futUnloadFinish{};
	std::future<bool> m_futLoadFinish{};

public:
	static Engine::UPtr< CLevelLoading> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex);

private:
	void Free() override;

};

NS_END
