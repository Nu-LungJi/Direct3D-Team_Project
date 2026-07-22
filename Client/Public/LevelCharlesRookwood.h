#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelCharlesRookwood final : public CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelCharlesRookwood, CLevel)

private:
	explicit CLevelCharlesRookwood();
	~CLevelCharlesRookwood() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static UPtr<CLevelCharlesRookwood> Create();

private:
	void Free() override;
};

NS_END
