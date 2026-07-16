#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)

class CLevelPercival final : public CLevel
{
private:
	explicit CLevelPercival();
	~CLevelPercival() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

public:
	static UPtr<CLevelPercival> Create();

private:
	void Free() override;
};

NS_END
