#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)
class CLevelCreatureEditor final : public Engine::CLevel
{

private:
	explicit CLevelCreatureEditor();
	~CLevelCreatureEditor() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;

public:
	void Picking();
private:
	void		Resources();
	void		Objects();
	void		BeHaviors();
public:
	static Engine::UPtr<CLevelCreatureEditor> Create();

private:
	_float3		m_fPos{};
	_bool		m_bSpawn{ false };
	int32_t  m_iResourceSelect{ 0 }, m_iObjSelect{0};
	const	_string m_strLevelName = { "LEVEL_CREATURE" };
	_string	m_SelectResourceTag{}, m_SelectObjecteTag{}, m_SelectFileName{}, m_SelectFilePath{};

	std::map<_string, _string>		m_BeHaviorJsonList;
	std::vector<CHandle>				m_MedDebrisHandles{};

private:
	HRESULT InitializeMyMagicSquareStep();
private:
	void Free() override;
};

NS_END

