#pragma once
#include "Engine_Defines.h"
#include <atomic>

NS_BEGIN(Engine)

class ENGINE_DLL CResource: public CEngineBase, public std::enable_shared_from_this<CResource>
{
public:
	DECLARE_DERIVED_TYPE(CResource, CEngineBase)
public:
	enum class STATE
	{
		UNLOAD,
		LOADING,
		LOADFAIL,
		LOADED,
		END
	};

protected:
	explicit CResource(const _string& sPath);
	virtual ~CResource();

public:
	void UpdateGUI();

public:
	const _string& GetPath() const { return m_sPath; }
	STATE GetState() const { return m_eState.load(std::memory_order_acquire); }
	_string GetStateStr() const;

public:
	virtual HRESULT Load(const std::any& arg = {}) = 0;
	virtual HRESULT Unload(const std::any& arg = {}) = 0;

protected:
	const _string m_sPath;
	std::atomic<STATE> m_eState{ STATE::UNLOAD };

protected:
	void Free() override;
};

NS_END
