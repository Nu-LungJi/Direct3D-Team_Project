#pragma once
#include "Prototype.h"
#include "Handle.h"

NS_BEGIN(Engine)
class CGameObject;
class ENGINE_DLL CComponent : public CPrototype
{
public:
	DECLARE_DERIVED_TYPE(CComponent, CPrototype)
	CComponent& operator=(const CComponent&) = delete;

public:
	typedef struct tagComponentDesc: public CEngineBase
	{
		CGameObject* pGameObject{};
	}DESC;

protected:
	explicit CComponent();
	explicit CComponent(const CComponent& Prototype);
	~CComponent() override;

public:
	virtual void UpdateGUI();

protected:
	virtual HRESULT Initialize(void* pArg);

protected:
	CGameObject* m_pGameObject{};
public:
	CGameObject* GetGameObject() const { return m_pGameObject; }

protected:
	void Free() override;

};

NS_END
